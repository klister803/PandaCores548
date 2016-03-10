/*
 * Copyright (C) 2005-2016 Uwow <http://uwow.biz/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Map.h"
#include "GridStates.h"
#include "ScriptMgr.h"
#include "VMapFactory.h"
#include "MMapFactory.h"
#include "MapInstanced.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Transport.h"
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Group.h"
#include "DynamicTree.h"
#include "Vehicle.h"
#include "LFGMgr.h"
#include "ChallengeMgr.h"
#include "ScenarioMgr.h"
#include "RedisBuilderMgr.h"

void Map::SaveCreatureRespawnTime(uint32 dbGuid, time_t respawnTime)
{
    if (!respawnTime)
    {
        // Delete only
        RemoveCreatureRespawnTime(dbGuid);
        return;
    }

    _creatureRespawnTimes[dbGuid] = respawnTime;

    std::string index = std::to_string(dbGuid);

    Json::Value _data;
    CreatureRespawnData[index.c_str()] = respawnTime;
    _data = respawnTime;

    RedisDatabase.AsyncExecuteHSet("HSET", GetCreatureRespawmKey(), index.c_str(), _data, GetId(), [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Map::SaveCreatureRespawnTime guid %u", guid);
    });
}

void Map::RemoveCreatureRespawnTime(uint32 dbGuid)
{
    _creatureRespawnTimes.erase(dbGuid);

    std::string index = std::to_string(dbGuid);

    CreatureRespawnData.removeMember(index.c_str());

    RedisDatabase.AsyncExecuteH("HDEL", GetCreatureRespawmKey(), index.c_str(), GetId(), [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Map::RemoveCreatureRespawnTime guid %u", guid);
    });
}

void Map::SaveGORespawnTime(uint32 dbGuid, time_t respawnTime)
{
    if (!respawnTime)
    {
        // Delete only
        RemoveGORespawnTime(dbGuid);
        return;
    }

    _goRespawnTimes[dbGuid] = respawnTime;

    std::string index = std::to_string(dbGuid);

    Json::Value _data;
    GORespawnData[index.c_str()] = respawnTime;
    _data = respawnTime;

    RedisDatabase.AsyncExecuteHSet("HSET", GetGoRespawmKey(), index.c_str(), _data, GetId(), [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Map::SaveGORespawnTime guid %u", guid);
    });
}

void Map::RemoveGORespawnTime(uint32 dbGuid)
{
    _goRespawnTimes.erase(dbGuid);

    std::string index = std::to_string(dbGuid);

    GORespawnData.removeMember(index.c_str());

    RedisDatabase.AsyncExecuteH("HDEL", GetGoRespawmKey(), index.c_str(), GetId(), [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Map::RemoveGORespawnTime guid %u", guid);
    });
}

void Map::LoadGoRespawnTimes(const RedisValue* v)
{
    std::vector<RedisValue> respawnVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(v, respawnVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "Map::LoadGoRespawnTimes data is empty");
        return;
    }

    for (auto itr = respawnVector.begin(); itr != respawnVector.end();)
    {
        uint32 loguid = atoi(itr->toString().c_str());
        ++itr;

        Json::Value _data;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), _data))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Map::LoadGoRespawnTimes not parse Id %i", loguid);
            continue;
        }
        else
            ++itr;

        _goRespawnTimes[loguid] = time_t(_data.asInt());
    }
}

void Map::LoadCreatureRespawnTimes(const RedisValue* v)
{
    std::vector<RedisValue> respawnVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(v, respawnVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "Map::LoadCreatureRespawnTimes data is empty");
        return;
    }

    for (auto itr = respawnVector.begin(); itr != respawnVector.end();)
    {
        uint32 loguid = atoi(itr->toString().c_str());
        ++itr;

        Json::Value _data;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), _data))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Map::LoadCreatureRespawnTimes not parse Id %i", loguid);
            continue;
        }
        else
            ++itr;

        _creatureRespawnTimes[loguid] = time_t(_data.asInt());
    }
}

void Map::DeleteRespawnTimes()
{
    _creatureRespawnTimes.clear();
    _goRespawnTimes.clear();

    GORespawnData.clear();
    CreatureRespawnData.clear();

    DeleteRespawnTimesInDB(GetId(), GetInstanceId());
}

void Map::DeleteRespawnTimesInDB(uint16 mapId, uint32 instanceId)
{
    char* queryGoKey = new char[30];
    char* queryCrKey = new char[30];
    sprintf(queryGoKey, "r{%u}m{%u}i{%u}go", realmID, mapId, instanceId);

    RedisDatabase.AsyncExecute("DEL", queryGoKey, mapId, [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Map::DeleteRespawnTimesInDB guid %u", guid);
    });

    sprintf(queryCrKey, "r{%u}m{%u}i{%u}cr", realmID, mapId, instanceId);
    RedisDatabase.AsyncExecute("DEL", queryCrKey, mapId, [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Map::DeleteRespawnTimesInDB guid %u", guid);
    });
}
