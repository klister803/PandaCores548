/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef __RedisSaveBuilder_H
#define __RedisSaveBuilder_H

class RedisBuilder
{
    private:
        RedisBuilder();
        ~RedisBuilder();

    public:
        static RedisBuilder* instance()
        {
            static RedisBuilder instance;
            return &instance;
        }
        char const* BuildString(Json::Value& data);
        bool LoadFromRedis(const RedisValue* v, Json::Value& data);
        bool LoadFromRedisArray(const RedisValue* v, std::vector<RedisValue>& data);

        Json::Reader Reader;
        Json::FastWriter Builder;
        //Value ItemData;
};

#define sRedisBuilder RedisBuilder::instance()


#endif
