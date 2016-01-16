/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#ifndef _REDISATABASE_H
#define _REDISATABASE_H

#include "RedisWorkerPool.h"
#include "RedisConnection.h"

class RedisDatabaseConnection : public RedisConnection
{
public:
    //- Constructors for sync and async connections
    RedisDatabaseConnection(RedisConnectionInfo& connInfo) : RedisConnection(connInfo) { }
    RedisDatabaseConnection(ProducerConsumerQueue<RedisOperation*>* q, RedisConnectionInfo& connInfo) : RedisConnection(q, connInfo) { }
};

typedef RedisWorkerPool<RedisDatabaseConnection> RedisDatabaseWorkerPool;

#endif
