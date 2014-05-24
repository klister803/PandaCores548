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

bool ScenarioProgress::IsCompleted() const
{
    return currentStep >= GetStepCount(false);
}

bool ScenarioProgress::IsBonusStepCompleted() const
{
    for (ScenarioSteps::const_iterator itr = steps.begin(); itr != steps.end(); ++itr)
        if (itr->second->IsBonusObjective())
            if (currentStep > itr->second->m_orderIndex)
                return true;

    return false;
}

bool ScenarioProgress::HasBonusStep() const
{
    for (ScenarioSteps::const_iterator itr = steps.begin(); itr != steps.end(); ++itr)
        if (itr->second->IsBonusObjective())
            return true;

    return false;
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
        SendStepUpdate();

    return currentStep;
}

void ScenarioProgress::SendStepUpdate()
{
    WorldPacket data(SMSG_SCENARIO_PROGRESS_UPDATE, 3 + 7 * 4);
    data.WriteBit(0);                           // unk not used
    data.WriteBit(IsBonusStepCompleted());
    data.WriteBits(0, 19);                      // criteria data

    data << uint32(0);                          // proving grounds max wave
    data << uint32(0);                          // proving grounds diff id
    data << uint32(scenarioId);
    data << uint32(HasBonusStep());
    data << uint32(0);                          // proving grounds curr wave
    data << uint32(0);                          // proving grounds duration
    data << uint32(currentStep);

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

ScenarioSteps const* ScenarioMgr::GetScenarioSteps(uint32 scenarioId)
{
    ScenarioStepsByScenarioMap::const_iterator itr = m_stepMap.find(scenarioId);
    return itr != m_stepMap.end() ? &itr->second : NULL;
}

void WorldSession::HandleScenarioPOIQuery(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SCENARIO_POI_QUERY");

    // ........
}

