/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef _REDISWORKER_H
#define _REDISWORKER_H

#include <thread>
#include "ProducerConsumerQueue.h"
#include "Log.h"

#include <src/redisclient/redissyncclient.h>
#include <src/redisclient/redisasyncclient.h>

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
        const RedisValue GetKeyH(const char* cmd, const char* key, const char* field);
        const RedisValue SetKeyH(const char* cmd, const char* key, const char* field, const char* value);

        void GetKeyAsync(const char* cmd, const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        void SetKeyAsync(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);

        void HGetKeyAsync(const char* cmd, const char* key, const char* field, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        void HSetKeyAsync(const char* cmd, const char* key, const char* field, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);

        /// Get an io_service to use.
        boost::asio::io_service& get_io_service();

        bool IsConnected()  { if (_queue) return m_aclient->stateValid(); else return m_client->stateValid(); }
        void Reconnect();

    private:
        typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
        typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;
        typedef boost::shared_ptr<boost::asio::ip::tcp::endpoint> endpoint_ptr;

        /// The pool of io_services.
        std::vector<io_service_ptr> io_services_;

        /// The work that keeps the io_services running.
        std::vector<work_ptr> work_;

        /// Save endpoint
        std::vector<endpoint_ptr> m_endpoint;

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
