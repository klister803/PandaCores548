/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#include "DBCStores.h"
#include "ScenarioMgr.h"
#include "LFGMgr.h"
#include "InstanceSaveMgr.h"
#include "WorldSession.h"

ScenarioProgress::ScenarioProgress(uint32 _mapId, uint32 _instanceId, uint32 _scenarioId)
    : mapId(_mapId), instanceId(_instanceId), scenarioId(_scenarioId),
    m_achievementMgr(this), currentStep(0)
{
    type = ScenarioMgr::GetScenarioType(scenarioId);
    ScenarioSteps const* _steps = sScenarioMgr->GetScenarioSteps(scenarioId);
    ASSERT(_steps);

    steps = *_steps;
}

void ScenarioProgress::LoadFromDB()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SCENARIO_CRITERIAPROGRESS);
    stmt->setUInt32(0, instanceId);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    m_achievementMgr.LoadFromDB(NULL, result);
    UpdateCurrentStep(true);
}

void ScenarioProgress::SaveToDB(SQLTransaction& trans)
{
    bool commit = false;
    if (!trans)
    {
        trans = CharacterDatabase.BeginTransaction();
        commit = true;
    }

    m_achievementMgr.SaveToDB(trans);

    if (commit)
        CharacterDatabase.CommitTransaction(trans);
}

bool ScenarioProgress::IsCompleted(bool bonus) const
{
    return currentStep == GetStepCount(bonus);
}

uint8 ScenarioProgress::GetBonusStepCount() const
{
    uint8 count = 0;
    for (ScenarioSteps::const_iterator itr = steps.begin(); itr != steps.end(); ++itr)
        if (itr->second->IsBonusObjective())
            ++count;

    return count;
}

bool ScenarioProgress::HasBonusStep() const
{
    return GetBonusStepCount() > 0;
}

uint8 ScenarioProgress::GetStepCount(bool withBonus) const
{
    if (withBonus)
        return steps.size();

    uint8 count = 0;
    for (ScenarioSteps::const_iterator itr = steps.begin(); itr != steps.end(); ++itr)
        if (!itr->second->IsBonusObjective())
            ++count;

    return count;
}

uint8 ScenarioProgress::UpdateCurrentStep(bool loading)
{
    uint8 oldStep = currentStep;
    for (ScenarioSteps::const_iterator itr = steps.begin(); itr != steps.end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = sCriteriaTreeStore.LookupEntry(itr->second->m_criteriaTreeId);
        if (!criteriaTree)
            continue;

        if (GetAchievementMgr().IsCompletedCriteria(criteriaTree, NULL))
            currentStep = itr->second->m_orderIndex + 1;
    }

    if (currentStep != oldStep && !loading)
    {
        SendStepUpdate();
        SaveToDB(SQLTransaction(NULL));

        /*if (IsCompleted(false))
            Reward(false);
        else if (IsCompleted(true))
            Reward(true);*/
    }

    return currentStep;
}

void ScenarioProgress::SendStepUpdate(Player* player, bool full)
{
    WorldPacket data(SMSG_SCENARIO_PROGRESS_UPDATE, 3 + 7 * 4);
    data.WriteBit(0);                           // unk not used
    data.WriteBit(IsCompleted(true));           // bonus step completed
    uint32 bitpos = data.bitwpos();
    data.WriteBits(0, 19);                      // criteria data

    ByteBuffer buff;
    if (full)
    {
        uint32 count = 0;
        CriteriaProgressMap const* progressMap = GetAchievementMgr().GetCriteriaProgressMap();
        for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
        {
            CriteriaProgress const& progress = itr->second;
            CriteriaTreeEntry const* criteriaTreeEntry = sCriteriaTreeStore.LookupEntry(itr->first);
            if (!criteriaTreeEntry)
                continue;

            ObjectGuid criteriaGuid = MAKE_NEW_GUID(1, scenarioId, HIGHGUID_SCENARIO_CRITERIA);
            uint64 counter = progress.counter;

            data.WriteGuidMask<7, 6, 0, 4>(criteriaGuid);
            data.WriteGuidMask<7>(counter);
            data.WriteGuidMask<5, 1>(criteriaGuid);
            data.WriteGuidMask<1, 3, 0, 2>(counter);
            data.WriteGuidMask<3>(criteriaGuid);
            data.WriteGuidMask<5>(counter);
            data.WriteBits(0, 4);
            data.WriteGuidMask<4>(counter);
            data.WriteGuidMask<2>(criteriaGuid);
            data.WriteGuidMask<6>(counter);

            buff.WriteGuidBytes<1>(counter);
            buff << uint32(time(NULL) - progress.date);
            buff.WriteGuidBytes<3>(counter);
            buff.WriteGuidBytes<3, 4, 0, 1>(criteriaGuid);
            buff << secsToTimeBitFields(progress.date);
            buff.WriteGuidBytes<6, 7>(criteriaGuid);
            buff.WriteGuidBytes<0>(counter);
            buff.WriteGuidBytes<5>(criteriaGuid);
            buff.WriteGuidBytes<4, 5>(counter);
            buff << uint32(criteriaTreeEntry->criteria);
            buff.WriteGuidBytes<2>(counter);
            buff << uint32(time(NULL) - progress.date);
            buff.WriteGuidBytes<6, 7>(counter);
            buff.WriteGuidBytes<2>(criteriaGuid);

            ++count;
        }

        data.FlushBits();
        data.append(buff);
        data.PutBits(bitpos, count, 19);
    }

    data << uint32(0);                          // proving grounds max wave
    data << uint32(0);                          // proving grounds diff id
    data << uint32(scenarioId);
    data << uint32(GetBonusStepCount());
    data << uint32(0);                          // proving grounds curr wave
    data << uint32(0);                          // proving grounds duration
    data << uint32(currentStep);

    if (player)
        player->SendDirectMessage(&data);
    else
        BroadCastPacket(data);
}

void ScenarioProgress::SendCriteriaUpdate(uint32 criteriaId, uint32 counter, time_t date)
{
    WorldPacket data(SMSG_SCENARIO_CRITERIA_UPDATE, 8 + 8 + 4 * 4 + 1);

    ObjectGuid criteriaGuid = MAKE_NEW_GUID(1, scenarioId, HIGHGUID_SCENARIO_CRITERIA);

    data.WriteGuidMask<3>(counter);
    data.WriteGuidMask<7, 2, 0>(criteriaGuid);
    data.WriteGuidMask<4>(counter);
    data.WriteGuidMask<4, 6>(criteriaGuid);
    data.WriteBits(0, 4);               // always 0
    data.WriteGuidMask<2>(counter);
    data.WriteGuidMask<3>(criteriaGuid);
    data.WriteGuidMask<1>(counter);
    data.WriteGuidMask<1>(criteriaGuid);
    data.WriteGuidMask<6, 5>(counter);
    data.WriteGuidMask<5>(criteriaGuid);
    data.WriteGuidMask<7, 0>(counter);

    data.WriteGuidBytes<6, 4>(counter);
    data << uint32(criteriaId);
    data << uint32(time(NULL) - date);
    data.WriteGuidBytes<4, 5>(criteriaGuid);
    data.WriteGuidBytes<3, 1>(counter);
    data.WriteGuidBytes<3, 1, 6>(criteriaGuid);
    data.WriteGuidBytes<7>(counter);
    data.WriteGuidBytes<0>(criteriaGuid);
    data << uint32(time(NULL) - date);
    data << secsToTimeBitFields(date);
    data.WriteGuidBytes<5>(counter);
    data.WriteGuidBytes<2>(criteriaGuid);
    data.WriteGuidBytes<2>(counter);
    data.WriteGuidBytes<7>(criteriaGuid);
    data.WriteGuidBytes<0>(counter);

    BroadCastPacket(data);
}

void ScenarioProgress::BroadCastPacket(WorldPacket& data)
{
    Map* map = sMapMgr->FindMap(mapId, instanceId);
    if (!map)
        return;

    map->SendToPlayers(&data);
}

bool ScenarioProgress::CanUpdateCriteria(uint32 criteriaId) const
{
    // ......
    return true;
}

ScenarioMgr::ScenarioMgr() : updateDiff(0)
{
    Initialize();
}

ScenarioMgr::~ScenarioMgr()
{
}

ScenarioProgress* ScenarioMgr::GetScenarioProgress(uint32 instanceId)
{
    ScenarioProgressMap::iterator itr = m_scenarioProgressMap.find(instanceId);
    return itr != m_scenarioProgressMap.end() ? &itr->second : NULL;
}

void ScenarioMgr::AddScenarioProgress(uint32 mapId, uint32 instanceId, uint32 scenarioId, bool loading)
{
    if (m_scenarioProgressMap.find(instanceId) != m_scenarioProgressMap.end())
        return;

    m_scenarioProgressMap[instanceId] = ScenarioProgress(mapId, instanceId, scenarioId);
    if (loading)
        m_scenarioProgressMap[instanceId].LoadFromDB();
}

void ScenarioMgr::SaveToDB(SQLTransaction& trans)
{
    bool commit = false;
    if (!trans)
    {
        trans = CharacterDatabase.BeginTransaction();
        commit = true;
    }

    for (ScenarioProgressMap::iterator itr = m_scenarioProgressMap.begin(); itr != m_scenarioProgressMap.end(); ++itr)
        itr->second.SaveToDB(trans);

    if (commit)
        CharacterDatabase.CommitTransaction(trans);
}

ScenarioType ScenarioMgr::GetScenarioType(uint32 scenarioId)
{
    ScenarioEntry const* entry = sScenarioStore.LookupEntry(scenarioId);
    ASSERT(entry);

    if (entry->IsProvingGrounds())
        return SCENARIO_TYPE_PROVING_GROUNDS;
    if (entry->IsChallenge())
        return SCENARIO_TYPE_CHALLENGE;

    return SCENARIO_TYPE_NORMAL;
}

void ScenarioMgr::Initialize()
{
    for (uint32 i = 0; i < sScenarioStepStore.GetNumRows(); ++i)
    {
        ScenarioStepEntry const* entry = sScenarioStepStore.LookupEntry(i);
        if (!entry || !sScenarioStore.LookupEntry(entry->m_scenarioId))
            continue;

        m_stepMap[entry->m_scenarioId][entry->m_orderIndex] = entry;
    }
}

void ScenarioMgr::Update(uint32 diff)
{
    updateDiff += diff;
    if (updateDiff < 5 * MINUTE * IN_MILLISECONDS)
        return;

    updateDiff -= 5 * MINUTE * IN_MILLISECONDS;

    /*SaveToDB(SQLTransaction(NULL));*/
}

ScenarioSteps const* ScenarioMgr::GetScenarioSteps(uint32 scenarioId)
{
    ScenarioStepsByScenarioMap::const_iterator itr = m_stepMap.find(scenarioId);
    return itr != m_stepMap.end() ? &itr->second : NULL;
}

void WorldSession::HandleScenarioPOIQuery(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SCENARIO_POI_QUERY");

    uint32 count = recvData.ReadBits(22);
    if (!count)
        return;

    WorldPacket data(SMSG_SCENARIO_POI, 200);
    data.WriteBits(count, 21);

    ByteBuffer buff;
    for (uint32 i = 0; i < count; ++i)
    {
        uint32 criteriaTreeId;
        recvData >> criteriaTreeId;

        ScenarioPOIVector const* POI = sObjectMgr->GetScenarioPOIVector(criteriaTreeId);
        if (!POI)
        {
            data.WriteBits(0, 19);
            buff << uint32(criteriaTreeId);
            continue;
        }

        data.WriteBits(POI->size(), 19);

        for (ScenarioPOIVector::const_iterator itr = POI->begin(); itr != POI->end(); ++itr)
        {
            buff << uint32(itr->Id);                // POI index
            buff << uint32(itr->MapId);             // mapid
            buff << uint32(itr->Unk24);
            buff << uint32(itr->Unk12);
            buff << uint32(itr->Unk16);

            data.WriteBits(itr->points.size(), 21); // POI points count
            for (std::vector<ScenarioPOIPoint>::const_iterator itr2 = itr->points.begin(); itr2 != itr->points.end(); ++itr2)
            {
                buff << int32(itr2->y);             // POI point y
                buff << int32(itr2->x);             // POI point x
            }

            buff << uint32(itr->WorldMapAreaId);
            buff << uint32(itr->Unk28);
            buff << uint32(itr->Unk20);
        }

        buff << uint32(criteriaTreeId);             // criteria tree id
    }

    data.FlushBits();
    if (!buff.empty())
        data.append(buff);

    SendPacket(&data);
}

