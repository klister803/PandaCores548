/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "BasicRedisTask.h"
#include "RedisConnection.h"
#include "Log.h"
#include <boost/thread/thread.hpp>

BasicRedisTask::BasicRedisTask(const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler, bool get)
{
    m_key = key;
    m_value = value;
    m_guid = guid;
    m_hander = &handler;
}

BasicRedisTask::~BasicRedisTask() { }

bool BasicRedisTask::Execute()
{
    if (m_get)
        return m_conn->ExecuteGet(m_key, m_guid, *m_hander);
    else
        return m_conn->ExecuteSet(m_key, m_value, m_guid, *m_hander);
}
