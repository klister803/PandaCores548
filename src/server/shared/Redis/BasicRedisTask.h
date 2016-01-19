/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef _BASICREDISTASK_H
#define _BASICREDISTASK_H

#include "RedisOperation.h"

class Player;

class BasicRedisTask : public RedisOperation
{
    public:
        BasicRedisTask(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler);
        ~BasicRedisTask();

        bool Execute() override;

    private:
        const char* m_key;      //- query key to be executed
        const char* m_value;    //- query set value to be executed
        uint64 m_guid;          //- query owner guid
        const boost::function<void(const RedisValue &, uint64)>& m_hander;
        bool m_get;
        const char* m_cmd;
};

#endif
