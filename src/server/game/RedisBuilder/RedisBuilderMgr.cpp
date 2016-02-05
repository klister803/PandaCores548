/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "RedisBuilderMgr.h"

RedisBuilder::RedisBuilder()
{
}

RedisBuilder::~RedisBuilder()
{
}

std::string RedisBuilder::BuildString(Json::Value& data)
{
    return Builder.write(data);
}

bool RedisBuilder::LoadFromRedis(const RedisValue* v, Json::Value& data)
{
    if (!v->isOk() || v->isNull() || !v->isString())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilder::DeSerialization data is empty");
        return false;
    }

    bool isReader = Reader.parse(v->toString().c_str(), data);
    if (!isReader)
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilder::DeSerialization parce json false");
        return false;
    }

    return true;
}

bool RedisBuilder::LoadFromRedisArray(const RedisValue* v, std::vector<RedisValue>& data)
{
    if (!v->isOk() || v->isNull())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilder::LoadFromRedisArray data is empty");
        return false;
    }

    if (!v->isArray())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilder::LoadFromRedisArray is not array");
        return false;
    }

    data = v->toArray();

    return true;
}
