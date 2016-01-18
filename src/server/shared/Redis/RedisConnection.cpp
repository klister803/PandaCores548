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

bool RedisConnection::ExecuteSet(const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisConnection::Execute %i", boost::this_thread::get_id());

    m_worker->SetKey(key, value, guid, handler);

    return true;
}

bool RedisConnection::ExecuteGet(const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisConnection::Execute %i", boost::this_thread::get_id());

    m_worker->GetKey(key, guid, handler);

    return true;
}


