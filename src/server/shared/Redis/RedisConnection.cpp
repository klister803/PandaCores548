/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/


#include "Common.h"
#include <errmsg.h>

#include "RedisConnection.h"
#include "RedisOperation.h"
#include "Timer.h"
#include "Log.h"
#include "ProducerConsumerQueue.h"

RedisConnection::RedisConnection(RedisConnectionInfo& connInfo) :
m_reconnecting(false),
m_queue(NULL),
m_worker(NULL),
m_connectionInfo(connInfo)
{
    m_worker = new RedisWorker(nullptr, this);
}

RedisConnection::RedisConnection(ProducerConsumerQueue<RedisOperation*>* queue, RedisConnectionInfo& connInfo) :
m_reconnecting(false),
m_queue(queue),
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
    delete this;
}

bool RedisConnection::Execute(const char* key, const char* value, const boost::function<void(const RedisValue &)> &handler)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisConnection::Execute %i", boost::this_thread::get_id());

    if (value)
        m_worker->SetKey(key, value, handler);
    else
        m_worker->GetKey(key, handler);

    return true;
}

ResultSet* RedisConnection::Query(const char* sql)
{
    return NULL;
}

