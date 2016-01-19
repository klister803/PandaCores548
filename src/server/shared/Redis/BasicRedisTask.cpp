/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "BasicRedisTask.h"
#include "RedisConnection.h"
#include "Log.h"
#include <boost/thread/thread.hpp>

BasicRedisTask::BasicRedisTask(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler) :
m_hander(handler)
{
    m_cmd = strdup(cmd);
    m_key = strdup(key);
    m_value = strdup(value);
    m_guid = guid;
}

BasicRedisTask::~BasicRedisTask()
{
    free((void*)m_cmd);
    free((void*)m_key);
    if (m_value)
        free((void*)m_value);
}

bool BasicRedisTask::Execute()
{
    sLog->outInfo(LOG_FILTER_SQL_DRIVER, "asicRedisTask::Execute cmd %s key %s value %s %i", m_cmd, m_key, m_value, boost::this_thread::get_id());

    if (m_value)
        m_conn->ExecuteAsyncSet(m_cmd, m_key, m_value, m_guid, m_hander);
    else
        m_conn->ExecuteAsync(m_cmd, m_key, m_guid, m_hander);

    return true;
}
