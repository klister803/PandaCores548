/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "RedisWorkerPool.h"
#include "RedisOperation.h"
#include "RedisWorker.h"
#include "Transaction.h"
#include "Util.h"
#include "ProducerConsumerQueue.h"

#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <src/redisclient/redisasyncclient.h>

#ifndef _REDISCONNECTION_H
#define _REDISCONNECTION_H

class Player;

struct RedisConnectionInfo
{
    explicit RedisConnectionInfo(std::string const& infoString)
    {
        Tokenizer tokens(infoString, ';');

        if (tokens.size() != 5)
            return;

        uint8 i = 0;

        host.assign(tokens[i++]);
        port_or_socket.assign(tokens[i++]);
        user.assign(tokens[i++]);
        password.assign(tokens[i++]);
        database.assign(tokens[i++]);
    }

    std::string user; // not use
    std::string password;
    std::string database; // digital number (default base number 0-15)
    std::string host;
    std::string port_or_socket;
};

class RedisConnection
{
    template <class T> friend class RedisWorkerPool;

    public:
        RedisConnection(RedisConnectionInfo& connInfo);                               //! Constructor for synchronous connections.
        RedisConnection(ProducerConsumerQueue<RedisOperation*>* queue, RedisConnectionInfo& connInfo);  //! Constructor for asynchronous connections.
        virtual ~RedisConnection();

        void Close();

    public:
        void ExecuteAsyncSet(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        void ExecuteAsync(const char* cmd, const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        void ExecuteAsyncHSet(const char* cmd, const char* key, const char* field, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        void ExecuteAsyncH(const char* cmd, const char* key, const char* field, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);

        const RedisValue ExecuteSet(const char* cmd, const char* key, const char* value);
        const RedisValue Execute(const char* cmd, const char* key);
        const RedisValue ExecuteSetH(const char* cmd, const char* key, const char* field, const char* value);
        const RedisValue ExecuteH(const char* cmd, const char* key, const char* field);

        operator bool() const { return m_worker != NULL; }

        RedisWorker* GetWorker()  { return m_worker; }

        RedisConnectionInfo&  m_connectionInfo;             //! Connection info (used for logging)

        bool LockIfReady()
        {
            /// Tries to acquire lock. If lock is acquired by another thread
            /// the calling parent will just try another connection
            return m_Mutex.try_lock();
        }

        void Unlock()
        {
            /// Called by parent databasepool. Will let other threads access this connection
            m_Mutex.unlock();
        }

    protected:
        bool                                 m_reconnecting;  //! Are we reconnecting?

    private:
        ProducerConsumerQueue<RedisOperation*>* m_queue;      //! Queue shared with other asynchronous connections.
        RedisWorker*          m_worker;                     //! Core worker task.
        boost::mutex          m_Mutex;
        std::string           m_errmsg;

        RedisConnection(RedisConnection const& right) = delete;
        RedisConnection& operator=(RedisConnection const& right) = delete;
};

#endif
