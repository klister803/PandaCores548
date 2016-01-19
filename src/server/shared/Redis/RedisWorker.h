/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef _REDISWORKER_H
#define _REDISWORKER_H

#include <thread>
#include "ProducerConsumerQueue.h"
#include "Log.h"

#include <src/redisclient/redisasyncclient.h>
#include <src/redisclient/redissyncclient.h>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>


class RedisConnection;
class RedisOperation;
class Player;


class RedisWorker
{
    public:
        RedisWorker(ProducerConsumerQueue<RedisOperation*>* newQueue, RedisConnection* connection);
        ~RedisWorker();

        // Run when query to redis is complete
        void onAsyncConnect(bool connected, const std::string &errorMessage);
        void onSet(const RedisValue &value);
        void onGet(const RedisValue &value);

        // Execute
        const RedisValue GetKey(const char* cmd, const char* key);
        const RedisValue SetKey(const char* cmd, const char* key, const char* value);

        void GetKeyAsync(const char* cmd, const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        void SetKeyAsync(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);

        /// Get an io_service to use.
        boost::asio::io_service& get_io_service();

        bool IsConnected()  { return m_connected; }

    private:
        typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
        typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

        /// The pool of io_services.
        std::vector<io_service_ptr> io_services_;

        /// The work that keeps the io_services running.
        std::vector<work_ptr> work_;

        /// The next io_service to use for a connection.
        std::size_t next_io_service_;

        ProducerConsumerQueue<RedisOperation*>* _queue;
        RedisConnection* _connection;

        void WorkerThread();
        boost::thread _workerThread;
        boost::thread _clientThread;

        std::atomic_bool _cancelationToken;
        RedisAsyncClient*     m_aclient;
        RedisSyncClient*      m_client;
        bool m_connected;

        RedisWorker(RedisWorker const& right) = delete;
        RedisWorker& operator=(RedisWorker const& right) = delete;
};

#endif
