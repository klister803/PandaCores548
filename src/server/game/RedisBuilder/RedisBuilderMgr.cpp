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

bool RedisBuilder::CheckKey(char* _key)
{
    RedisValue v = RedisDatabase.Execute("EXISTS", _key);
    if (!v.isOk() || v.isNull())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilder::CheckKey data not found %s", _key);
        return false;
    }

    if (!v.toInt())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilder::CheckKey key not exist %s", _key);
        return false;
    }

    return true;
}

void RedisBuilder::InitRedisKey()
{
    queryGuidKey = new char[18];
    petKey = new char[18];
    bracketKey = new char[18];
    aucItemKey = new char[18];
    aucKey = new char[18];
    guildKey = new char[18];
    guildFKey = new char[15];
    guildFMKey = new char[20];
    groupKey = new char[12];
    groupMemberKey = new char[20];
    groupInstanceKey = new char[18];
    challengeKey = new char[18];

    sprintf(queryGuidKey, "r{%u}HIGHESTGUIDS", realmID);
    sprintf(petKey, "r{%u}pets", realmID);
    sprintf(bracketKey, "r{%u}bracket", realmID);
    sprintf(aucItemKey, "r{%i}auc{%i}items", realmID, 0);
    sprintf(aucKey, "r{%i}auc", realmID);
    sprintf(guildFKey, "r{%u}finder", realmID);
    sprintf(guildFMKey, "r{%u}findermember", realmID);
    sprintf(groupKey, "r{%i}group", realmID);
    sprintf(groupMemberKey, "r{%i}groupmember", realmID);
    sprintf(groupInstanceKey, "r{%i}instance", realmID);
    sprintf(challengeKey, "r{%i}challenge", realmID);
}
