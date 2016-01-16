/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _REDISWORKERPOOL_H
#define _REDISWORKERPOOL_H

#include "Common.h"
#include "Callback.h"
#include "RedisConnection.h"
#include "RedisWorker.h"
#include "Log.h"

#include <src/redisclient/redisasyncclient.h>

#include <mysqld_error.h>
#include <memory>

template <class T>
class RedisWorkerPool
{
    private:
        enum InternalIndex
        {
            IDX_ASYNC,
            IDX_SYNCH,
            IDX_SIZE
        };

    public:
        /* Activity state */
        RedisWorkerPool() : _queue(new ProducerConsumerQueue<RedisOperation*>())
        {
            memset(_connectionCount, 0, sizeof(_connectionCount));
            _connections.resize(IDX_SIZE);
        }

        ~RedisWorkerPool()
        {
            _queue->Cancel();
        }

        bool Open(const std::string& infoString, uint8 async_threads, uint8 synch_threads)
        {
            bool res = true;
            _connectionInfo.reset(new RedisConnectionInfo(infoString));

            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "Opening DatabasePool '%s'. Asynchronous connections: %u, synchronous connections: %u.", GetDatabaseName(), async_threads, synch_threads);

            //! Open asynchronous connections (delayed operations)
            _connections[IDX_ASYNC].resize(async_threads);
            for (uint8 i = 0; i < async_threads; ++i)
            {
                T* t = new T(_queue.get(), *_connectionInfo);
                res = t->Open();
                _connections[IDX_ASYNC][i] = t;
                ++_connectionCount[IDX_ASYNC];
            }

            //! Open synchronous connections (direct, blocking operations)
            _connections[IDX_SYNCH].resize(synch_threads);
            for (uint8 i = 0; i < synch_threads; ++i)
            {
                T* t = new T(*_connectionInfo);
                res = t->Open();
                _connections[IDX_SYNCH][i] = t;
                ++_connectionCount[IDX_SYNCH];
            }

            if (res)
                sLog->outInfo(LOG_FILTER_SQL_DRIVER, "DatabasePool '%s' opened successfully. %u total connections running.", GetDatabaseName(), (_connectionCount[IDX_SYNCH] + _connectionCount[IDX_ASYNC]));
            else
                sLog->outError(LOG_FILTER_SQL_DRIVER, "DatabasePool %s NOT opened. There were errors opening the MySQL connections. Check your SQLDriverLogFile for specific errors.", GetDatabaseName());
            return res;
        }

        void Close()
        {
            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "Closing down DatabasePool '%s'.", GetDatabaseName());

            for (uint8 i = 0; i < _connectionCount[IDX_ASYNC]; ++i)
            {
                T* t = _connections[IDX_ASYNC][i];
                t->Close();         //! Closes the actualy MySQL connection.
            }

            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "Asynchronous connections on DatabasePool '%s' terminated. Proceeding with synchronous connections.", GetDatabaseName());

            //! Shut down the synchronous connections
            //! There's no need for locking the connection, because RedisWorkerPool<>::Close
            //! should only be called after any other thread tasks in the core have exited,
            //! meaning there can be no concurrent access at this point.
            for (uint8 i = 0; i < _connectionCount[IDX_SYNCH]; ++i)
                _connections[IDX_SYNCH][i]->Close();

            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "All connections on DatabasePool '%s' closed.", GetDatabaseName());
        }

        inline RedisConnectionInfo const* GetConnectionInfo() const
        {
            return _connectionInfo.get();
        }

        //! Enqueues a one-way SQL operation in string format that will be executed asynchronously.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        void GetExecute(const char* query_key)
        {
            BasicStatementTask* task = new BasicStatementTask(redis_query);
            Enqueue(task);
        }

        //! Enqueues a one-way SQL operation in string format -with variable args- that will be executed asynchronously.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        template<typename Format, typename... Args>
        void PExecute(Format&& sql, Args&&... args)
        {
            Execute(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        //! Directly executes a one-way SQL operation in string format, that will block the calling thread until finished.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        void DirectExecute(const char* sql)
        {
            if (!sql)
                return;

            T* t = GetFreeConnection();
            t->Execute(sql);
            t->Unlock();
        }

        //! Directly executes a one-way SQL operation in string format -with variable args-, that will block the calling thread until finished.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        template<typename Format, typename... Args>
        void DirectPExecute(Format&& sql, Args&&... args)
        {
            DirectExecute(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        //! Directly executes an SQL query in string format that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        QueryResult Query(const char* sql, T* conn = nullptr)
        {
            if (!conn)
                conn = GetFreeConnection();

            ResultSet* result = conn->Query(sql);
            conn->Unlock();
            if (!result || !result->GetRowCount() || !result->NextRow())
            {
                delete result;
                return QueryResult(NULL);
            }

            return QueryResult(result);
        }

        //! Directly executes an SQL query in string format -with variable args- that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        template<typename Format, typename... Args>
        QueryResult PQuery(Format&& sql, T* conn, Args&&... args)
        {
            return Query(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str(), conn);
        }

        //! Directly executes an SQL query in string format -with variable args- that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        template<typename Format, typename... Args>
        QueryResult PQuery(Format&& sql, Args&&... args)
        {
            return Query(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        /**
            Asynchronous query (with resultset) methods.
        */

        //! Enqueues a query in string format that will set the value of the QueryResultFuture return object as soon as the query is executed.
        //! The return value is then processed in ProcessQueryCallback methods.
        QueryResultFuture AsyncQuery(const char* sql)
        {
            BasicStatementTask* task = new BasicStatementTask(sql, true);
            // Store future result before enqueueing - task might get already processed and deleted before returning from this method
            QueryResultFuture result = task->GetFuture();
            Enqueue(task);
            return result;
        }

        //! Enqueues a query in string format -with variable args- that will set the value of the QueryResultFuture return object as soon as the query is executed.
        //! The return value is then processed in ProcessQueryCallback methods.
        template<typename Format, typename... Args>
        QueryResultFuture AsyncPQuery(Format&& sql, Args&&... args)
        {
            return AsyncQuery(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        //! Apply escape string'ing for current collation. (utf8)
        void EscapeString(std::string& str)
        {
            if (str.empty())
                return;

            char* buf = new char[str.size() * 2 + 1];
            EscapeString(buf, str.c_str(), uint32(str.length()));
            str = buf;
            delete[] buf;
        }

        //! Keeps all our MySQL connections alive, prevent the server from disconnecting us.
        void KeepAlive()
        {
            //! Ping synchronous connections
            for (uint8 i = 0; i < _connectionCount[IDX_SYNCH]; ++i)
            {
                T* t = _connections[IDX_SYNCH][i];
                if (t->LockIfReady())
                {
                    t->Ping();
                    t->Unlock();
                }
            }

            //! Assuming all worker threads are free, every worker thread will receive 1 ping operation request
            //! If one or more worker threads are busy, the ping operations will not be split evenly, but this doesn't matter
            //! as the sole purpose is to prevent connections from idling.
            //for (size_t i = 0; i < _connections[IDX_ASYNC].size(); ++i)
                //Enqueue(new PingOperation);
        }

    private:
        uint32 OpenConnections(InternalIndex type, uint8 numConnections)
        {
            _connections[type].resize(numConnections);
            for (uint8 i = 0; i < numConnections; ++i)
            {
                T* t;

                if (type == IDX_ASYNC)
                    t = new T(_queue.get(), *_connectionInfo);
                else if (type == IDX_SYNCH)
                    t = new T(*_connectionInfo);
                else
                    ABORT();

                _connections[type][i] = t;
                ++_connectionCount[type];
            }

            // Everything is fine
            return 0;
        }

        unsigned long EscapeString(char *to, const char *from, unsigned long length)
        {
            if (!to || !from || !length)
                return 0;

            return mysql_real_escape_string(_connections[IDX_SYNCH][0]->GetHandle(), to, from, length);
        }

        void Enqueue(RedisOperation* op)
        {
            _queue->Push(op);
        }

        //! Gets a free connection in the synchronous connection pool.
        //! Caller MUST call t->Unlock() after touching the MySQL context to prevent deadlocks.
        T* GetFreeConnection()
        {
            uint8 i = 0;
            size_t num_cons = _connectionCount[IDX_SYNCH];
            T* t = NULL;
            //! Block forever until a connection is free
            for (;;)
            {
                t = _connections[IDX_SYNCH][++i % num_cons];
                //! Must be matched with t->Unlock() or you will get deadlocks
                if (t->LockIfReady())
                    break;
            }

            return t;
        }

        char const* GetDatabaseName() const
        {
            return _connectionInfo->database.c_str();
        }

        //! Queue shared by async worker threads.
        std::unique_ptr<ProducerConsumerQueue<RedisOperation*>> _queue;
        std::vector<std::vector<T*>> _connections;
        //! Counter of MySQL connections;
        uint32 _connectionCount[IDX_SIZE];
        std::unique_ptr<RedisConnectionInfo> _connectionInfo;
        uint8 _async_threads, _synch_threads;
};

#endif
