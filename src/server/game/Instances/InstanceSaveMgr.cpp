/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "Player.h"
#include "GridNotifiers.h"
#include "Log.h"
#include "GridStates.h"
#include "CellImpl.h"
#include "Map.h"
#include "MapManager.h"
#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "Timer.h"
#include "GridNotifiersImpl.h"
#include "Config.h"
#include "Transport.h"
#include "ObjectMgr.h"
#include "World.h"
#include "Group.h"
#include "InstanceScript.h"
#include "ScenarioMgr.h"

uint16 InstanceSaveManager::ResetTimeDelay[] = {3600, 900, 300, 60};

InstanceSaveManager::~InstanceSaveManager()
{
    // it is undefined whether this or objectmgr will be unloaded first
    // so we must be prepared for both cases
    lock_instLists = true;
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end(); ++itr)
    {
        InstanceSave* save = itr->second;

        for (InstanceSave::PlayerListType::iterator itr2 = save->m_playerList.begin(), next = itr2; itr2 != save->m_playerList.end(); itr2 = next)
        {
            ++next;
            (*itr2)->UnbindInstance(save->GetMapId(), save->GetDifficulty(), true);
        }
        save->m_playerList.clear();

        for (InstanceSave::GroupListType::iterator itr2 = save->m_groupList.begin(), next = itr2; itr2 != save->m_groupList.end(); itr2 = next)
        {
            ++next;
            (*itr2)->UnbindInstance(save->GetMapId(), save->GetDifficulty(), true);
        }
        save->m_groupList.clear();
        delete save;
    }
}

/*
- adding instance into manager
- called from InstanceMap::Add, _LoadBoundInstances, LoadGroups
*/
InstanceSave* InstanceSaveManager::AddInstanceSave(uint32 mapId, uint32 instanceId, Difficulty difficulty, bool canReset, bool load)
{
    if (InstanceSave* old_save = GetInstanceSave(instanceId))
        return old_save;

    const MapEntry* entry = sMapStore.LookupEntry(mapId);
    if (!entry)
    {
        sLog->outError(LOG_FILTER_GENERAL, "InstanceSaveManager::AddInstanceSave: wrong mapid = %d, instanceid = %d!", mapId, instanceId);
        return NULL;
    }

    if (instanceId == 0)
    {
        sLog->outError(LOG_FILTER_GENERAL, "InstanceSaveManager::AddInstanceSave: mapid = %d, wrong instanceid = %d!", mapId, instanceId);
        return NULL;
    }

    if (!entry->IsDifficultyModeSupported(difficulty))
    {
        sLog->outError(LOG_FILTER_GENERAL, "InstanceSaveManager::AddInstanceSave: mapid = %d, instanceid = %d, wrong dificalty %u!", mapId, instanceId, difficulty);
        return NULL;
    }

    // initialize reset time
    // for normal instances if no creatures are killed the instance will reset in two hours
    if (entry->map_type != MAP_RAID && difficulty <= REGULAR_DIFFICULTY)
    {
        time_t resetTime = time(NULL) + 12 * HOUR;
        // normally this will be removed soon after in InstanceMap::Add, prevent error
        ScheduleReset(true, resetTime, InstResetEvent(0, mapId, difficulty, instanceId));
    }

    sLog->outDebug(LOG_FILTER_MAPS, "InstanceSaveManager::AddInstanceSave: mapid = %d, instanceid = %d", mapId, instanceId);

    InstanceSave* save = new InstanceSave(mapId, instanceId, difficulty, canReset);
    if (!load)
        save->SaveToDB();

    m_instanceSaveById[instanceId] = save;
    return save;
}

InstanceSave* InstanceSaveManager::GetInstanceSave(uint32 InstanceId)
{
    InstanceSaveHashMap::iterator itr = m_instanceSaveById.find(InstanceId);
    return itr != m_instanceSaveById.end() ? itr->second : NULL;
}

void InstanceSaveManager::DeleteInstanceFromDB(uint32 instanceid)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INSTANCE_BY_INSTANCE);
    stmt->setUInt32(0, instanceid);
    CharacterDatabase.DirectExecute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_BY_INSTANCE);
    stmt->setUInt32(0, instanceid);
    CharacterDatabase.DirectExecute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INSTANCE_BY_INSTANCE);
    stmt->setUInt32(0, instanceid);
    CharacterDatabase.DirectExecute(stmt);
    // Respawn times should be deleted only when the map gets unloaded

    sScenarioMgr->RemoveScenarioProgress(instanceid);
}

void InstanceSaveManager::RemoveInstanceSave(uint32 InstanceId)
{
    InstanceSaveHashMap::iterator itr = m_instanceSaveById.find(InstanceId);
    if (itr != m_instanceSaveById.end())
    {
        itr->second->SetToDelete(true);
        m_instanceSaveById.erase(itr);
    }
}

void InstanceSaveManager::UnloadInstanceSave(uint32 InstanceId)
{
    if (InstanceSave* save = GetInstanceSave(InstanceId))
    {
        save->UnloadIfEmpty();
        if (save->m_toDelete)
            delete save;
    }
}

InstanceSave::InstanceSave(uint16 MapId, uint32 InstanceId, Difficulty difficulty, bool canReset)
: m_instanceid(InstanceId), m_mapid(MapId), m_toDelete(false), m_difficulty(difficulty), m_canReset(canReset)
{
}

InstanceSave::~InstanceSave()
{
    // the players and groups must be unbound before deleting the save
    ASSERT(m_playerList.empty() && m_groupList.empty());
}

/*
    Called from AddInstanceSave
*/
void InstanceSave::SaveToDB()
{
    // save instance data too
    std::string data;
    uint32 completedEncounters = 0;
    uint32 challenge = 0;
    Map* map = sMapMgr->FindMap(GetMapId(), m_instanceid);
    if (map)
    {
        ASSERT(map->IsDungeon());
        if (InstanceScript* instanceScript = ((InstanceMap*)map)->GetInstanceScript())
        {
            data = instanceScript->GetSaveData();
            completedEncounters = instanceScript->GetCompletedEncounterMask();
            challenge = instanceScript->GetChallengeProgresTime();
        }
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_INSTANCE_SAVE);
    stmt->setUInt32(0, m_instanceid);
    stmt->setUInt16(1, GetMapId());
    stmt->setUInt8(2, uint8(GetDifficulty()));
    stmt->setUInt32(3, challenge);
    stmt->setUInt32(4, completedEncounters);
    stmt->setString(5, data);
    CharacterDatabase.Execute(stmt);
}

// to cache or not to cache, that is the question
InstanceTemplate const* InstanceSave::GetTemplate()
{
    return sObjectMgr->GetInstanceTemplate(m_mapid);
}

MapEntry const* InstanceSave::GetMapEntry()
{
    return sMapStore.LookupEntry(m_mapid);
}

void InstanceSave::DeleteFromDB()
{
    InstanceSaveManager::DeleteInstanceFromDB(GetInstanceId());
}

/* true if the instance save is still valid */
bool InstanceSave::UnloadIfEmpty()
{
    if (m_playerList.empty() && m_groupList.empty())
    {
        // don't remove the save if there are still players inside the map
        if (Map* map = sMapMgr->FindMap(GetMapId(), GetInstanceId()))
            if (map->HavePlayers())
                return true;

        if (!sInstanceSaveMgr->lock_instLists)
            sInstanceSaveMgr->RemoveInstanceSave(GetInstanceId());

        return false;
    }
    else
        return true;
}

void InstanceSaveManager::LoadInstances()
{
    uint32 oldMSTime = getMSTime();

    // Delete invalid character_instance and group_instance references
    CharacterDatabase.DirectExecute("DELETE ci.* FROM character_instance AS ci LEFT JOIN characters AS c ON ci.guid = c.guid WHERE c.guid IS NULL");
    CharacterDatabase.DirectExecute("DELETE gi.* FROM group_instance     AS gi LEFT JOIN groups     AS g ON gi.guid = g.guid WHERE g.guid IS NULL");

    // Delete invalid instance references
    CharacterDatabase.DirectExecute("DELETE i.* FROM instance AS i LEFT JOIN character_instance AS ci ON i.id = ci.instance LEFT JOIN group_instance AS gi ON i.id = gi.instance WHERE ci.guid IS NULL AND gi.guid IS NULL");

    // Delete invalid references to instance
    CharacterDatabase.DirectExecute("DELETE FROM creature_respawn WHERE instanceId > 0 AND instanceId NOT IN (SELECT id FROM instance)");
    CharacterDatabase.DirectExecute("DELETE FROM gameobject_respawn WHERE instanceId > 0 AND instanceId NOT IN (SELECT id FROM instance)");
    CharacterDatabase.DirectExecute("DELETE tmp.* FROM character_instance AS tmp LEFT JOIN instance ON tmp.instance = instance.id WHERE tmp.instance > 0 AND instance.id IS NULL");
    CharacterDatabase.DirectExecute("DELETE tmp.* FROM group_instance     AS tmp LEFT JOIN instance ON tmp.instance = instance.id WHERE tmp.instance > 0 AND instance.id IS NULL");

    // Clean invalid references to instance
    CharacterDatabase.DirectExecute("UPDATE corpse SET instanceId = 0 WHERE instanceId > 0 AND instanceId NOT IN (SELECT id FROM instance)");
    CharacterDatabase.DirectExecute("UPDATE characters AS tmp LEFT JOIN instance ON tmp.instance_id = instance.id SET tmp.instance_id = 0 WHERE tmp.instance_id > 0 AND instance.id IS NULL");

    // Initialize instance id storage (Needs to be done after the trash has been clean out)
    sMapMgr->InitInstanceIds();

    // Load reset times and clean expired instances
    sInstanceSaveMgr->LoadResetTimes();

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded instances in %u ms", GetMSTimeDiffToNow(oldMSTime));

}

void InstanceSaveManager::LoadResetTimes()
{
    time_t now = time(NULL);
    time_t today = (now / DAY) * DAY;

    // NOTE: Use DirectPExecute for tables that will be queried later

    // get the current reset times for normal instances (these may need to be updated)
    // these are only kept in memory for InstanceSaves that are loaded later
    // resettime = 0 in the DB for raid/heroic instances so those are skipped
    typedef std::pair<uint32 /*PAIR32(map, difficulty)*/, time_t> ResetTimeMapDiffType;
    typedef std::map<uint32, ResetTimeMapDiffType> InstResetTimeMapDiffType;
    InstResetTimeMapDiffType instResetTime;

    QueryResult result = CharacterDatabase.Query("SELECT id, map, difficulty FROM instance ORDER BY id ASC");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 instanceId = fields[0].GetUInt32();
            uint32 difficulty = fields[2].GetUInt8();

            const MapEntry* entry = sMapStore.LookupEntry(fields[1].GetUInt16());
            if (!entry)
            {
                sLog->outError(LOG_FILTER_GENERAL, "InstanceSaveManager::AddInstanceSave: wrong mapid = %d, instanceid = %d!", entry->MapID, instanceId);
                continue;
            }
            // Instances are pulled in ascending order from db and nextInstanceId is initialized with 1,
            // so if the instance id is used, increment until we find the first unused one for a potential new instance
            if (sMapMgr->GetNextInstanceId() == instanceId)
                sMapMgr->SetNextInstanceId(instanceId + 1);

            // Mark instance id as being used
            sMapMgr->RegisterInstanceId(instanceId);

            // initialize reset time
            // for normal instances if no creatures are killed the instance will reset in two hours
            // Set 12H, no more. In any way regular dung plr could restary himself, but collecting regular dunges wuth inf spawn time is bad idea.
            if (entry->map_type != MAP_RAID && difficulty <= REGULAR_DIFFICULTY)
                instResetTime[instanceId] = ResetTimeMapDiffType(MAKE_PAIR32(entry->MapID, difficulty), time(NULL) + 12 * HOUR);
        }
        while (result->NextRow());

        // schedule the reset times
        for (InstResetTimeMapDiffType::iterator itr = instResetTime.begin(); itr != instResetTime.end(); ++itr)
            if (itr->second.second > now)
                ScheduleReset(true, itr->second.second, InstResetEvent(0, PAIR32_LOPART(itr->second.first), Difficulty(PAIR32_HIPART(itr->second.first)), itr->first));
    }
}

void InstanceSaveManager::ScheduleReset(bool add, time_t time, InstResetEvent event)
{
    if (!add)
    {
        // find the event in the queue and remove it
        ResetTimeQueue::iterator itr;
        std::pair<ResetTimeQueue::iterator, ResetTimeQueue::iterator> range;
        range = m_resetTimeQueue.equal_range(time);
        for (itr = range.first; itr != range.second; ++itr)
        {
            if (itr->second == event)
            {
                m_resetTimeQueue.erase(itr);
                return;
            }
        }

        // in case the reset time changed (should happen very rarely), we search the whole queue
        if (itr == range.second)
        {
            for (itr = m_resetTimeQueue.begin(); itr != m_resetTimeQueue.end(); ++itr)
            {
                if (itr->second == event)
                {
                    m_resetTimeQueue.erase(itr);
                    return;
                }
            }

            if (itr == m_resetTimeQueue.end())
                sLog->outError(LOG_FILTER_GENERAL, "InstanceSaveManager::ScheduleReset: cannot cancel the reset, the event(%d, %d, %d) was not found!", event.type, event.mapid, event.instanceId);
        }
    }
    else
        m_resetTimeQueue.insert(std::pair<time_t, InstResetEvent>(time, event));
}

void InstanceSaveManager::Update()
{
    time_t now = time(NULL);
    time_t t;

    while (!m_resetTimeQueue.empty())
    {
        t = m_resetTimeQueue.begin()->first;
        if (t >= now)
            break;

        InstResetEvent &event = m_resetTimeQueue.begin()->second;
        if (event.type == 0)
        {
            // for individual normal instances, max creature respawn + X hours
            _ResetInstance(event.mapid, event.instanceId);
            m_resetTimeQueue.erase(m_resetTimeQueue.begin());
        }
    }
}

void InstanceSaveManager::_ResetSave(InstanceSaveHashMap::iterator &itr)
{
    // unbind all players bound to the instance
    // do not allow UnbindInstance to automatically unload the InstanceSaves
    lock_instLists = true;

    InstanceSave::PlayerListType &pList = itr->second->m_playerList;
    while (!pList.empty())
    {
        Player* player = *(pList.begin());
        player->UnbindInstance(itr->second->GetMapId(), itr->second->GetDifficulty(), true);
    }

    InstanceSave::GroupListType &gList = itr->second->m_groupList;
    while (!gList.empty())
    {
        Group* group = *(gList.begin());
        group->UnbindInstance(itr->second->GetMapId(), itr->second->GetDifficulty(), true);
    }

    delete itr->second;
    m_instanceSaveById.erase(itr++);

    lock_instLists = false;
}

void InstanceSaveManager::_ResetInstance(uint32 mapid, uint32 instanceId)
{
    sLog->outDebug(LOG_FILTER_MAPS, "InstanceSaveMgr::_ResetInstance %u, %u", mapid, instanceId);
    Map const* map = sMapMgr->CreateBaseMap(mapid);
    if (!map->Instanceable())
        return;

    InstanceSaveHashMap::iterator itr = m_instanceSaveById.find(instanceId);
    if (itr != m_instanceSaveById.end())
        _ResetSave(itr);

    DeleteInstanceFromDB(instanceId);                       // even if save not loaded

    Map* iMap = ((MapInstanced*)map)->FindInstanceMap(instanceId);

    if (iMap && iMap->IsDungeon())
        ((InstanceMap*)iMap)->Reset(INSTANCE_RESET_RESPAWN_DELAY);

    if (iMap)
        iMap->DeleteRespawnTimes();
    else
        Map::DeleteRespawnTimesInDB(mapid, instanceId);

    // Free up the instance id and allow it to be reused
    sMapMgr->FreeInstanceId(instanceId);
}

void InstanceSaveManager::ResetOrWarnAll(uint32 mapid, Difficulty difficulty)
{
    // global reset for all instances of the given map
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
    if (!mapEntry || !mapEntry->Instanceable())
        return;

    // remove all binds to instances of the given map
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end();)
    {
        if (itr->second->GetMapId() == mapid && itr->second->GetDifficulty() == difficulty)
            _ResetSave(itr);
        else
            ++itr;
    }

    // delete them from the DB, even if not loaded
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INSTANCE_BY_MAP_DIFF);
    stmt->setUInt16(0, uint16(mapid));
    stmt->setUInt8(1, uint8(difficulty));
    CharacterDatabase.DirectExecute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_BY_MAP_DIFF);
    stmt->setUInt16(0, uint16(mapid));
    stmt->setUInt8(1, uint8(difficulty));
    CharacterDatabase.DirectExecute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INSTANCE_BY_MAP_DIFF);
    stmt->setUInt16(0, uint16(mapid));
    stmt->setUInt8(1, uint8(difficulty));
    CharacterDatabase.DirectExecute(stmt);


    // note: this isn't fast but it's meant to be executed very rarely
    Map const* map = sMapMgr->CreateBaseMap(mapid);          // _not_ include difficulty
    MapInstanced::InstancedMaps &instMaps = ((MapInstanced*)map)->GetInstancedMaps();
    MapInstanced::InstancedMaps::iterator mitr;

    for (mitr = instMaps.begin(); mitr != instMaps.end(); ++mitr)
    {
        Map* map2 = mitr->second;
        if (!map2->IsDungeon())
            continue;

        ((InstanceMap*)map2)->Reset(INSTANCE_RESET_GLOBAL);
    }

    // TODO: delete creature/gameobject respawn times even if the maps are not loaded
}

uint32 InstanceSaveManager::GetNumBoundPlayersTotal()
{
    uint32 ret = 0;
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end(); ++itr)
        ret += itr->second->GetPlayerCount();

    return ret;
}

uint32 InstanceSaveManager::GetNumBoundGroupsTotal()
{
    uint32 ret = 0;
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end(); ++itr)
        ret += itr->second->GetGroupCount();

    return ret;
}

time_t InstanceSave::GetResetTime()
{
    if(MapDifficulty const* mapDiff = GetMapDifficultyData(GetMapId(), GetDifficulty()))
        if (mapDiff->resetTime)
            return sWorld->getInstanceResetTime(mapDiff->resetTime);

    // normal mode. 12h after plr leave dung in InstanceMap::SetResetSchedule
    return time(NULL) + 12 * HOUR;
}