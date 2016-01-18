/*
 * Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
 */

#ifndef _REDISWORKERPOOL_H
#define _REDISWORKERPOOL_H

#include "Common.h"
#include "Callback.h"
#include "RedisConnection.h"
#include "RedisWorker.h"
#include "Log.h"
#include "BasicRedisTask.h"

#include <src/redisclient/redisasyncclient.h>
#include <boost/function.hpp>

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

            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "Opening RedisPool '%s'. Asynchronous connections: %u, synchronous connections: %u.", GetDatabaseName(), async_threads, synch_threads);

            //! Open asynchronous connections (delayed operations)
            _connections[IDX_ASYNC].resize(async_threads);
            for (uint8 i = 0; i < async_threads; ++i)
            {
                T* t = new T(_queue.get(), *_connectionInfo);
                _connections[IDX_ASYNC][i] = t;
                ++_connectionCount[IDX_ASYNC];
            }
 
            //! Open synchronous connections (direct, blocking operations)
            _connections[IDX_SYNCH].resize(synch_threads);
            for (uint8 i = 0; i < synch_threads; ++i)
            {
                T* t = new T(*_connectionInfo);
                _connections[IDX_SYNCH][i] = t;
                ++_connectionCount[IDX_SYNCH];
            }

            if (res)
                sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisPool '%s' opened successfully. %u total connections running.", GetDatabaseName(), (_connectionCount[IDX_SYNCH] + _connectionCount[IDX_ASYNC]));
            else
                sLog->outError(LOG_FILTER_SQL_DRIVER, "RedisPool %s NOT opened. There were errors opening the MySQL connections. Check your SQLDriverLogFile for specific errors.", GetDatabaseName());
            return res;
        }

        void Close()
        {
            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "Closing down RedisPool '%s'.", GetDatabaseName());

            for (uint8 i = 0; i < _connectionCount[IDX_ASYNC]; ++i)
            {
                T* t = _connections[IDX_ASYNC][i];
                t->Close();         //! Closes the actualy connection.
            }

            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "Asynchronous connections on RedisPool '%s' terminated. Proceeding with synchronous connections.", GetDatabaseName());

            //! Shut down the synchronous connections
            //! There's no need for locking the connection, because RedisWorkerPool<>::Close
            //! should only be called after any other thread tasks in the core have exited,
            //! meaning there can be no concurrent access at this point.
            for (uint8 i = 0; i < _connectionCount[IDX_SYNCH]; ++i)
                _connections[IDX_SYNCH][i]->Close();

            sLog->outInfo(LOG_FILTER_SQL_DRIVER, "All connections on RedisPool '%s' closed.", GetDatabaseName());
        }

        inline RedisConnectionInfo const* GetConnectionInfo() const
        {
            return _connectionInfo.get();
        }

        //! Enqueues a one-way SQL operation in string format that will be executed asynchronously.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        void ExecuteGet(const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
        {
            T* t = GetFreeConnection();
            t->ExecuteGet(key, guid, handler);
        }

        void ExecuteSet(const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
        {
            T* t = GetFreeConnection();
            t->ExecuteSet(key, value, guid, handler);
        }

        //! Directly executes a one-way SQL operation in string format, that will block the calling thread until finished.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        void DirectExecute(const char* sql)
        {
            if (!sql)
                return;

            T* t = GetFreeConnection();
            t->ExecuteGet(sql);
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

        /**
            Asynchronous query (with resultset) methods.
        */

        //! Enqueues a query in string format that will set the value of the QueryResultFuture return object as soon as the query is executed.
        //! The return value is then processed in ProcessQueryCallback methods.
        QueryResultFuture AsyncQuery(const char* sql)
        {
            BasicRedisTask* task = new BasicRedisTask(sql);
            // Store future result before enqueueing - task might get already processed and deleted before returning from this method
            QueryResultFuture result = task->GetFuture();
            Enqueue(task);
            return result;
        }

        void AsyncExecuteGet(const char* key)
        {
            BasicRedisTask* task = new BasicRedisTask(key);
            Enqueue(task);
        }

        void AsyncExecuteSet(const char* key, const char* value)
        {
            BasicRedisTask* task = new BasicRedisTask(key, value);
            Enqueue(task);
        }

    private:
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
