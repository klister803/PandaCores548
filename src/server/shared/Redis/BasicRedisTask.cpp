/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "BasicRedisTask.h"
#include "RedisConnection.h"
#include "Log.h"
#include <boost/thread/thread.hpp>

/*! Basic, ad-hoc queries. */
BasicRedisTask::BasicRedisTask(const char* key, const char* value)
{
    m_key = key;
    m_value = value;
}

BasicRedisTask::~BasicRedisTask() { }

bool BasicRedisTask::Execute()
{
    return m_conn->Execute(m_key, m_value, [&](const RedisValue &) {});
}
