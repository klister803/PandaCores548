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

#include "Common.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GroupMgr.h"
#include "Group.h"
#include "Formulas.h"
#include "ObjectAccessor.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "MapManager.h"
#include "InstanceSaveMgr.h"
#include "MapInstanced.h"
#include "Util.h"
#include "LFGMgr.h"
#include "UpdateFieldFlags.h"
#include "GuildMgr.h"
#include "Bracket.h"
#include "RedisBuilderMgr.h"
#include "InstanceScript.h"

void Group::SaveGroup()
{
    GroupData["m_leaderGuid"] = GUID_LOPART(m_leaderGuid);
    GroupData["m_lootMethod"] = m_lootMethod;
    GroupData["m_looterGuid"] = GUID_LOPART(m_looterGuid);
    GroupData["m_targetIcons0"] = m_targetIcons[0];
    GroupData["m_targetIcons1"] = m_targetIcons[1];
    GroupData["m_targetIcons2"] = m_targetIcons[2];
    GroupData["m_targetIcons3"] = m_targetIcons[3];
    GroupData["m_targetIcons4"] = m_targetIcons[4];
    GroupData["m_targetIcons5"] = m_targetIcons[5];
    GroupData["m_targetIcons6"] = m_targetIcons[6];
    GroupData["m_targetIcons7"] = m_targetIcons[7];
    GroupData["m_groupType"] = m_groupType;
    GroupData["m_dungeonDifficulty"] = m_dungeonDifficulty;
    GroupData["m_raidDifficulty"] = m_raidDifficulty;
    GroupData["LfgState"] = 0;
    GroupData["LfgDungeon"] = 0;
    GroupData["m_lootThreshold"] = m_lootThreshold;

    std::string index = std::to_string(m_dbStoreId);

    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupKey(), index.c_str(), sRedisBuilder->BuildString(GroupData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::SaveGroup guid %u", guid);
    });
}

void Group::SaveGroupMembers()
{
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        if (!IS_PLAYER_GUID(citr->guid))
            continue;

        std::string memberId = std::to_string(GUID_LOPART(citr->guid));
        GroupMemberData[memberId.c_str()]["name"] = citr->name;
        GroupMemberData[memberId.c_str()]["group"] = citr->group;
        GroupMemberData[memberId.c_str()]["flags"] = citr->flags;
        GroupMemberData[memberId.c_str()]["roles"] = citr->roles;
    }

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupMemberKey(), index.c_str(), sRedisBuilder->BuildString(GroupMemberData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::SaveGroupMembers guid %u", guid);
    });
}

void Group::DeleteMember(uint64 guid)
{
    std::string memberId = std::to_string(GUID_LOPART(guid));
    GroupMemberData.removeMember(memberId.c_str());

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupMemberKey(), index.c_str(), sRedisBuilder->BuildString(GroupMemberData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::SaveGroupMembers guid %u", guid);
    });
}

void Group::UpdateMember(MemberSlot* member)
{
    std::string memberId = std::to_string(GUID_LOPART(member->guid));

    GroupMemberData[memberId.c_str()]["name"] = member->name;
    GroupMemberData[memberId.c_str()]["group"] = member->group;
    GroupMemberData[memberId.c_str()]["flags"] = member->flags;
    GroupMemberData[memberId.c_str()]["roles"] = member->roles;

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupMemberKey(), index.c_str(), sRedisBuilder->BuildString(GroupMemberData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::SaveGroupMembers guid %u", guid);
    });
}

void Group::DeleteFromRedis()
{
    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteH("HDEL", sGroupMgr->GetGroupKey(), index.c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::DeleteFromRedis m_dbStoreId %u", guid);
    });
    RedisDatabase.AsyncExecuteH("HDEL", sGroupMgr->GetGroupMemberKey(), index.c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::DeleteFromRedis m_dbStoreId %u", guid);
    });
}

void Group::LoadGroupFromDB(Json::Value groupData, uint32 groupId)
{
    m_dbStoreId = groupId;
    m_guid = MAKE_NEW_GUID(sGroupMgr->GenerateGroupId(), 0, HIGHGUID_GROUP);
    m_leaderGuid = MAKE_NEW_GUID(groupData["m_leaderGuid"].asUInt(), 0, HIGHGUID_PLAYER);

    // group leader not exist
    if (!ObjectMgr::GetPlayerNameByGUID(groupData["m_leaderGuid"].asUInt(), m_leaderName))
        return;

    m_lootMethod = LootMethod(groupData["m_lootMethod"].asUInt());
    m_looterGuid = MAKE_NEW_GUID(groupData["m_looterGuid"].asUInt(), 0, HIGHGUID_PLAYER);
    m_lootThreshold = ItemQualities(groupData["m_lootThreshold"].asUInt());

    m_targetIcons[0] = groupData["m_targetIcons0"].asUInt();
    m_targetIcons[1] = groupData["m_targetIcons1"].asUInt();
    m_targetIcons[2] = groupData["m_targetIcons2"].asUInt();
    m_targetIcons[3] = groupData["m_targetIcons3"].asUInt();
    m_targetIcons[4] = groupData["m_targetIcons4"].asUInt();
    m_targetIcons[5] = groupData["m_targetIcons5"].asUInt();
    m_targetIcons[6] = groupData["m_targetIcons6"].asUInt();
    m_targetIcons[7] = groupData["m_targetIcons7"].asUInt();


    m_groupType  = GroupType(groupData["m_groupType"].asUInt());
    if (m_groupType & GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    uint32 diff = groupData["m_dungeonDifficulty"].asUInt();
    if (!IsValidDifficulty(diff, false))
        m_dungeonDifficulty = REGULAR_DIFFICULTY;
    else
        m_dungeonDifficulty = Difficulty(diff);

    uint32 r_diff = groupData["m_raidDifficulty"].asUInt();
    if (!IsValidDifficulty(r_diff, true))
        m_raidDifficulty = MAN10_DIFFICULTY;
    else
        m_raidDifficulty = Difficulty(r_diff);

    m_dungeon = groupData["m_dungeon"].asUInt();
    m_state = groupData["m_state"].asUInt();

    if (m_groupType & GROUPTYPE_LFG)
        sLFGMgr->_LoadFromDB(m_dungeon, m_state, GetGUID(), m_leaderGuid);
}

void GroupMgr::LoadGroupsRedis()
{
    {
        uint32 oldMSTime = getMSTime();

        RedisValue v = RedisDatabase.Execute("HGETALL", groupKey);

        std::vector<RedisValue> groupVector;
        if (!sRedisBuilder->LoadFromRedisArray(&v, groupVector))
        {
            sLog->outInfo(LOG_FILTER_REDIS, "GroupMgr::LoadGroupsRedis group not found");
            return;
        }

        uint32 count = 0;
        for (auto itr = groupVector.begin(); itr != groupVector.end();)
        {
            uint32 groupID = atoi(itr->toString().c_str());
            ++itr;
            if (itr->isInt())
            {
                ++itr;
                continue;
            }

            Json::Value groupData;
            if (!sRedisBuilder->LoadFromRedis(&(*itr), groupData))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "GroupMgr::LoadGroupsRedis not parse groupID %i", groupID);
                continue;
            }
            else
                ++itr;

            Group* group = new Group;
            group->LoadGroupFromDB(groupData, groupID);
            AddGroup(group);

            // Get the ID used for storing the group in the database and register it in the pool.
            uint32 storageId = group->GetDbStoreId();
            RegisterGroupDbStoreId(storageId, group);

            // Increase the next available storage ID
            if (storageId == NextGroupDbStoreId)
                NextGroupDbStoreId++;

            ++count;
        }

        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u group definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading Group members...");
    {
        uint32 oldMSTime = getMSTime();

        RedisValue v = RedisDatabase.Execute("HGETALL", groupMemberKey);

        std::vector<RedisValue> groupMemberVector;
        if (!sRedisBuilder->LoadFromRedisArray(&v, groupMemberVector))
        {
            sLog->outInfo(LOG_FILTER_REDIS, "GroupMgr::LoadGroupsRedis group member is clean");
            return;
        }

        uint32 count = 0;
        for (auto itr = groupMemberVector.begin(); itr != groupMemberVector.end();)
        {
            uint32 groupID = atoi(itr->toString().c_str());
            ++itr;
            if (itr->isInt())
            {
                ++itr;
                continue;
            }

            Json::Value groupMemberData;
            if (!sRedisBuilder->LoadFromRedis(&(*itr), groupMemberData))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "GroupMgr::LoadGroupsRedis not parse groupID %i groupMemberData", groupID);
                continue;
            }
            else
                ++itr;

            if (Group* group = GetGroupByDbStoreId(groupID))
            {
                for (auto iter = groupMemberData.begin(); iter != groupMemberData.end(); ++iter)
                {
                    uint32 memberGuid = atoi(iter.memberName());
                    auto dataValue = *iter;
                    group->LoadMemberFromDB(memberGuid, dataValue["flags"].asUInt(), dataValue["group"].asUInt(), dataValue["roles"].asUInt(), dataValue["name"].asString());
                }
            }
            else
            {
                sLog->outError(LOG_FILTER_GENERAL, "GroupMgr::LoadGroups: Consistency failed, can't find group (storage id: %u)", groupID);
            }

            ++count;
        }

        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u group members in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading Group instance saves...");
    {
        uint32 oldMSTime = getMSTime();

        RedisValue v = RedisDatabase.Execute("HGETALL", groupInstanceKey);

        std::vector<RedisValue> groupVector;
        if (!sRedisBuilder->LoadFromRedisArray(&v, groupVector))
        {
            sLog->outInfo(LOG_FILTER_REDIS, "GroupMgr::LoadGroupsRedis group instance saves not found");
            return;
        }

        uint32 count = 0;
        for (auto itr = groupVector.begin(); itr != groupVector.end();)
        {
            uint32 groupID = atoi(itr->toString().c_str());
            ++itr;
            if (itr->isInt())
            {
                ++itr;
                continue;
            }

            Json::Value instanceData;
            if (!sRedisBuilder->LoadFromRedis(&(*itr), instanceData))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "GroupMgr::LoadGroupsRedis not parse groupID %i instanceData", groupID);
                continue;
            }
            else
                ++itr;

            Group* group = GetGroupByDbStoreId(groupID);
            if (!group)
                continue;

            for (auto iter = instanceData.begin(); iter != instanceData.end(); ++iter)
            {
                uint32 instanceId = atoi(iter.memberName());
                auto dataValue = *iter;

                bool perm = dataValue["perm"].asBool();
                uint32 mapId = dataValue["map"].asUInt();
                time_t saveTime = time_t(dataValue["saveTime"].asUInt());
                MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
                if (!mapEntry || !mapEntry->IsDungeon())
                {
                    sLog->outError(LOG_FILTER_SQL, "Incorrect entry in group_instance table : no dungeon map %d", mapId);
                    continue;
                }

                uint32 diff = dataValue["difficulty"].asUInt();
                MapDifficulty const* mapDiff = GetMapDifficultyData(mapId, Difficulty(diff));
                if (!mapDiff || sWorld->getOldInstanceResetTime(mapDiff->resetTime) > saveTime)
                    continue;

                if (!mapEntry->IsDifficultyModeSupported(diff))
                {
                    sLog->outError(LOG_FILTER_SQL, "Wrong dungeon difficulty use in group_instance table: %d", diff + 1);
                    diff = 0;                                   // default for both difficaly types
                }

                InstanceSave* save = sInstanceSaveMgr->AddInstanceSave(mapId, instanceId, Difficulty(diff), dataValue["completedEncounters"].asUInt(), dataValue["challenge"].asUInt(), dataValue["data"].asString(), !perm, true);
                group->BindToInstance(save, perm, true);
                save->SetSaveTime(saveTime);
                save->SetPerm(perm);
            }
            ++count;
        }
 
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u group-instance saves in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
}

void Group::SaveGroupInstances()
{
    GroupInstanceData.clear();
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        //std::string diff = std::to_string(i);

        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if (itr->second.perm)
            {
                InstanceSave* save = itr->second.save;

                Map* map = sMapMgr->FindMap(save->GetMapId(), save->GetInstanceId());
                if (map && map->IsDungeon())
                {
                    if (InstanceScript* instanceScript = ((InstanceMap*)map)->GetInstanceScript())
                    {
                        save->SetData(instanceScript->GetSaveData());
                        save->SetCompletedEncountersMask(instanceScript->GetCompletedEncounterMask());
                        save->SetChallenge(instanceScript->GetChallengeProgresTime());
                    }
                }

                std::string instanceID = std::to_string(save->GetInstanceId());
                GroupInstanceData[instanceID.c_str()]["perm"] = itr->second.perm;
                GroupInstanceData[instanceID.c_str()]["map"] = save->GetMapId();
                GroupInstanceData[instanceID.c_str()]["difficulty"] = save->GetDifficulty();
                GroupInstanceData[instanceID.c_str()]["challenge"] = save->GetChallenge();
                GroupInstanceData[instanceID.c_str()]["data"] = save->GetData();
                GroupInstanceData[instanceID.c_str()]["completedEncounters"] = save->GetCompletedEncounterMask();
                GroupInstanceData[instanceID.c_str()]["saveTime"] = save->GetSaveTime();
            }
        }
    }

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupInstanceKey(), index.c_str(), sRedisBuilder->BuildString(GroupInstanceData).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::SaveGroupInstances guid %u", guid);
    });
}

void Group::UpdateInstance(InstanceSave* save)
{
    std::string instanceID = std::to_string(save->GetInstanceId());

    Map* map = sMapMgr->FindMap(save->GetMapId(), save->GetInstanceId());
    if (map && map->IsDungeon())
    {
        if (InstanceScript* instanceScript = ((InstanceMap*)map)->GetInstanceScript())
        {
            save->SetData(instanceScript->GetSaveData());
            save->SetCompletedEncountersMask(instanceScript->GetCompletedEncounterMask());
            save->SetChallenge(instanceScript->GetChallengeProgresTime());
        }
    }

    GroupInstanceData[instanceID.c_str()]["perm"] = save->GetPerm();
    GroupInstanceData[instanceID.c_str()]["map"] = save->GetMapId();
    GroupInstanceData[instanceID.c_str()]["difficulty"] = save->GetDifficulty();
    GroupInstanceData[instanceID.c_str()]["challenge"] = save->GetChallenge();
    GroupInstanceData[instanceID.c_str()]["data"] = save->GetData();
    GroupInstanceData[instanceID.c_str()]["completedEncounters"] = save->GetCompletedEncounterMask();
    GroupInstanceData[instanceID.c_str()]["saveTime"] = save->GetSaveTime();

    sLog->outInfo(LOG_FILTER_REDIS, "Group::UpdateInstance GetSaveTime %u", save->GetSaveTime());

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupInstanceKey(), index.c_str(), sRedisBuilder->BuildString(GroupInstanceData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::UpdateInstance guid %u", guid);
    });
}

void Group::DeleteInstance(uint32 instance)
{
    std::string instanceID = std::to_string(instance);
    GroupInstanceData.removeMember(instanceID.c_str());

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupInstanceKey(), index.c_str(), sRedisBuilder->BuildString(GroupInstanceData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::DeleteInstance guid %u", guid);
    });
}

void Group::RemoveAllInstance()
{
    GroupInstanceData.clear();

    std::string index = std::to_string(m_dbStoreId);
    RedisDatabase.AsyncExecuteHSet("HSET", sGroupMgr->GetGroupInstanceKey(), index.c_str(), sRedisBuilder->BuildString(GroupInstanceData).c_str(), m_dbStoreId, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Group::DeleteInstance guid %u", guid);
    });
}
