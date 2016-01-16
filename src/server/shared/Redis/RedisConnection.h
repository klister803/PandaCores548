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

#include "RedisWorkerPool.h"
#include "RedisOperation.h"
#include "Transaction.h"
#include "Util.h"
#include "ProducerConsumerQueue.h"

#include <src/redisclient/redisasyncclient.h>

#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#ifndef _REDISCONNECTION_H
#define _REDISCONNECTION_H

class RedisWorker;

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

        virtual uint32 Open();
        void Close();

    public:
        bool Execute(const char* sql);
        ResultSet* Query(const char* sql);

        operator bool() const { return m_client != NULL; }

        RedisConnectionInfo&  m_connectionInfo;             //! Connection info (used for logging)

    protected:
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

        RedisAsyncClient* GetHandle()  { return m_client; }

    protected:
        bool                                 m_reconnecting;  //! Are we reconnecting?

    private:
        ProducerConsumerQueue<RedisOperation*>* m_queue;      //! Queue shared with other asynchronous connections.
        RedisWorker*          m_worker;                     //! Core worker task.
        RedisAsyncClient*     m_client;                     //! Redis Handle.
        boost::mutex          m_Mutex;
        std::string           m_errmsg;

        RedisConnection(RedisConnection const& right) = delete;
        RedisConnection& operator=(RedisConnection const& right) = delete;
};

#endif
