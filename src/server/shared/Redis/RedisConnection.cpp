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

void RedisConnection::ExecuteAsyncSet(const char* cmd, const char* key, Json::Value& value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    m_worker->SetKeyAsync(cmd, key, value, guid, handler);
}

void RedisConnection::ExecuteAsync(const char* cmd, const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    m_worker->GetKeyAsync(cmd, key, guid, handler);
}

void RedisConnection::ExecuteAsyncHSet(const char* cmd, const char* key, const char* field, Json::Value& value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    m_worker->HSetKeyAsync(cmd, key, field, value, guid, handler);
}

void RedisConnection::ExecuteAsyncH(const char* cmd, const char* key, const char* field, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    m_worker->HGetKeyAsync(cmd, key, field, guid, handler);
}

const RedisValue RedisConnection::ExecuteSet(const char* cmd, const char* key, Json::Value& value)
{
    return m_worker->SetKey(cmd, key, value);
}

const RedisValue RedisConnection::Execute(const char* cmd, const char* key)
{
    return m_worker->GetKey(cmd, key);
}

const RedisValue RedisConnection::ExecuteSetH(const char* cmd, const char* key, const char* field, Json::Value& value)
{
    return m_worker->SetKeyH(cmd, key, field, value);
}

const RedisValue RedisConnection::ExecuteH(const char* cmd, const char* key, const char* field)
{
    return m_worker->GetKeyH(cmd, key, field);
}


