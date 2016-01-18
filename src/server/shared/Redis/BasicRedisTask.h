/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef _BASICREDISTASK_H
#define _BASICREDISTASK_H

#include "RedisOperation.h"

class BasicRedisTask : public RedisOperation
{
    public:
        BasicRedisTask(const char* key, const char* value = NULL);
        ~BasicRedisTask();

        bool Execute() override;

    private:
        const char* m_key;      //- query key to be executed
        const char* m_value;    //- query set value to be executed
};

#endif
