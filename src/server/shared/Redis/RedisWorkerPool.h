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
            _connected = true;
        }

        ~RedisWorkerPool()
        {
            _queue->Cancel();
        }

        bool Open(const std::string& infoString, uint8 async_threads, uint8 synch_threads)
        {
            _connected = true;
            bool res = true;
            _connectionInfo.reset(new RedisConnectionInfo(infoString));

            sLog->outInfo(LOG_FILTER_REDIS, "Opening RedisPool '%s'. Asynchronous connections: %u, synchronous connections: %u.", GetDatabaseName(), async_threads, synch_threads);

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
                sLog->outInfo(LOG_FILTER_REDIS, "RedisPool '%s' opened successfully. %u total connections running.", GetDatabaseName(), (_connectionCount[IDX_SYNCH] + _connectionCount[IDX_ASYNC]));
            else
                sLog->outError(LOG_FILTER_REDIS, "RedisPool %s NOT opened. There were errors opening the Redis connections. Check your SQLDriverLogFile for specific errors.", GetDatabaseName());

            Sleep(1000);
            return res;
        }

        void Close()
        {
            sLog->outInfo(LOG_FILTER_REDIS, "Closing down RedisPool '%s'.", GetDatabaseName());

            for (uint8 i = 0; i < _connectionCount[IDX_ASYNC]; ++i)
            {
                T* t = _connections[IDX_ASYNC][i];
                t->Close();         //! Closes the actualy connection.
            }

            sLog->outInfo(LOG_FILTER_REDIS, "Asynchronous connections on RedisPool '%s' terminated. Proceeding with synchronous connections.", GetDatabaseName());

            //! Shut down the synchronous connections
            //! There's no need for locking the connection, because RedisWorkerPool<>::Close
            //! should only be called after any other thread tasks in the core have exited,
            //! meaning there can be no concurrent access at this point.
            for (uint8 i = 0; i < _connectionCount[IDX_SYNCH]; ++i)
                _connections[IDX_SYNCH][i]->Close();

            sLog->outInfo(LOG_FILTER_REDIS, "All connections on RedisPool '%s' closed.", GetDatabaseName());
        }

        //! Check Redis connections
        void CheckConnect()
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "CheckConnect");

            //uint32 oldMSTime = getMSTime();
            for (uint8 i = 0; i < _connectionCount[IDX_SYNCH]; ++i)
            {
                T* t = _connections[IDX_SYNCH][i];
                if (!t->GetWorker()->IsConnected())
                    boost::thread(&RedisWorker::Reconnect, t->GetWorker());
                else
                    _connected = true;
            }
            //sLog->outInfo(LOG_FILTER_REDIS, "CheckConnect %u ms", GetMSTimeDiffToNow(oldMSTime));
            for (uint8 i = 0; i < _connectionCount[IDX_ASYNC]; ++i)
            {
                T* t = _connections[IDX_ASYNC][i];
                if (!t->GetWorker()->IsConnected())
                    t->GetWorker()->Reconnect();
                else
                    _connected = true;
            }
            //sLog->outInfo(LOG_FILTER_REDIS, "CheckConnect %u ms", GetMSTimeDiffToNow(oldMSTime));
        }

        bool isConnected(InternalIndex idx = IDX_ASYNC)
        {
            if (!_connected)
                return false;

            //sLog->outInfo(LOG_FILTER_REDIS, "isConnected");

            _connected = false;
            for (uint8 i = 0; i < _connectionCount[idx]; ++i)
            {
                T* t = _connections[idx][i];
                if (t->GetWorker()->IsConnected())
                {
                    _connected = true;
                    break;
                }
                //else
                    //t->GetWorker()->Reconnect();
            }
            //sLog->outInfo(LOG_FILTER_REDIS, "isConnected false");
            return _connected;
        }

        inline RedisConnectionInfo const* GetConnectionInfo() const
        {
            return _connectionInfo.get();
        }

        //! Enqueues a one-way SQL operation in string format that will be executed asynchronously.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        const RedisValue Execute(const char* cmd, const char* key)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecute");

            if (!isConnected(IDX_SYNCH))
                return RedisValue::RedisValue();

            T* t = GetFreeConnection();
            return t->Execute(cmd, key);
        }

        const RedisValue ExecuteSet(const char* cmd, const char* key, const char* value)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecute");

            if (!isConnected(IDX_SYNCH))
                return RedisValue::RedisValue();

            T* t = GetFreeConnection();
            return t->ExecuteSet(cmd, key, value);
        }

        const RedisValue ExecuteH(const char* cmd, const char* key, const char* field)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecute");

            if (!isConnected(IDX_SYNCH))
                return RedisValue::RedisValue();

            T* t = GetFreeConnection();
            return t->ExecuteH(cmd, key, field);
        }

        const RedisValue ExecuteSetH(const char* cmd, const char* key, const char* field, const char* value)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecute");

            if (!isConnected(IDX_SYNCH))
                return RedisValue::RedisValue();

            T* t = GetFreeConnection();
            return t->ExecuteSetH(cmd, key, field, value);
        }

        /**
            Asynchronous query (with resultset) methods.
        */

        void AsyncExecute(const char* cmd, const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecute");

            if (!isConnected())
                return;

            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecuteSet cmd %s key %s", cmd, key);

            T* t = GetFreeConnection(IDX_ASYNC);
            t->ExecuteAsync(cmd, key, guid, handler);

            //BasicRedisTask* task = new BasicRedisTask(cmd, key, nullptr, guid, handler);
            //Enqueue(task);
        }

        void AsyncExecuteH(const char* cmd, const char* key, const char* field, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecute");

            if (!isConnected())
                return;

            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecuteSet cmd %s key %s", cmd, key);

            T* t = GetFreeConnection(IDX_ASYNC);
            t->ExecuteAsyncH(cmd, key, field, guid, handler);

            //BasicRedisTask* task = new BasicRedisTask(cmd, key, nullptr, guid, handler);
            //Enqueue(task);
        }

        void AsyncExecuteSet(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecuteSet");

            if (!isConnected())
                return;

            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecuteSet cmd %s key %s value %s", cmd, key, value);

            T* t = GetFreeConnection(IDX_ASYNC);
            t->ExecuteAsyncSet(cmd, key, value, guid, handler);

            //BasicRedisTask* task = new BasicRedisTask(cmd, key, value, guid, handler);
            //Enqueue(task);
        }

        void AsyncExecuteHSet(const char* cmd, const char* key, const char* field, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
        {
            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecuteSet");

            if (!isConnected())
                return;

            //sLog->outInfo(LOG_FILTER_REDIS, "AsyncExecuteSet cmd %s key %s value %s", cmd, key, value);

            T* t = GetFreeConnection(IDX_ASYNC);
            t->ExecuteAsyncHSet(cmd, key, field, value, guid, handler);

            //BasicRedisTask* task = new BasicRedisTask(cmd, key, value, guid, handler);
            //Enqueue(task);
        }

    private:
        void Enqueue(RedisOperation* op)
        {
            _queue->Push(op);
        }

        //! Gets a free connection in the synchronous connection pool.
        //! Caller MUST call t->Unlock() after touching the Redis context to prevent deadlocks.
        T* GetFreeConnection(InternalIndex idx = IDX_SYNCH)
        {
            uint8 i = 0;
            size_t num_cons = _connectionCount[idx];
            T* t = NULL;
            //! Block forever until a connection is free
            for (;;)
            {
                t = _connections[idx][++i % num_cons];

                //sLog->outInfo(LOG_FILTER_REDIS, "GetFreeConnection i %i idx %i IsConnected %i", i, idx, t->GetWorker()->IsConnected());

                //! Must be matched with t->Unlock() or you will get deadlocks
                if (t->GetWorker()->IsConnected() && t->LockIfReady())
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
        //! Counter of Redis connections;
        uint32 _connectionCount[IDX_SIZE];
        std::unique_ptr<RedisConnectionInfo> _connectionInfo;
        uint8 _async_threads, _synch_threads;

        bool _connected;
};

#endif
