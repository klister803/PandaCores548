/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _REDISOPERATION_H
#define _REDISOPERATION_H

#include "Log.h"
#include <boost/thread/thread.hpp>

class RedisConnection;

class RedisOperation
{
    public:
        RedisOperation(): m_conn(NULL) { }
        virtual ~RedisOperation() { }

        virtual int call()
        {
            Execute();
            return 0;
        }
        virtual bool Execute() = 0;
        virtual void SetConnection(RedisConnection* con) { m_conn = con; }

        RedisConnection* m_conn;

    private:
        RedisOperation(RedisOperation const& right) = delete;
        RedisOperation& operator=(RedisOperation const& right) = delete;
};

#endif
