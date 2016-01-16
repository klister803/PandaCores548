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


#include "Common.h"
#include <errmsg.h>

#include "RedisConnection.h"
#include "RedisOperation.h"
#include "RedisWorker.h"
#include "Timer.h"
#include "Log.h"
#include "ProducerConsumerQueue.h"

RedisConnection::RedisConnection(RedisConnectionInfo& connInfo) :
m_reconnecting(false),
m_queue(NULL),
m_worker(NULL),
m_client(NULL),
m_connectionInfo(connInfo) { }

RedisConnection::RedisConnection(ProducerConsumerQueue<RedisOperation*>* queue, RedisConnectionInfo& connInfo) :
m_reconnecting(false),
m_queue(queue),
m_client(NULL),
m_connectionInfo(connInfo)
{
    m_worker = new RedisWorker(m_queue, this);
}

RedisConnection::~RedisConnection()
{
    delete m_worker;
}

void RedisConnection::Close()
{
    delete m_client;
    delete this;
}

uint32 RedisConnection::Open()
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string(m_connectionInfo.host);
    const unsigned int port = std::stoi(m_connectionInfo.port_or_socket);
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    m_client = new RedisAsyncClient(*m_worker->m_ioService);

    m_client->asyncConnect(endpoint, boost::bind(&RedisWorker::onConnect, m_worker, _1, _2));

    return 0;
}

bool RedisConnection::Execute(const char* sql)
{
    return true;
}

ResultSet* RedisConnection::Query(const char* sql)
{
    return NULL;
}

