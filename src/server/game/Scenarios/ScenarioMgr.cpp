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
#include "Group.h"
#include "ScenarioMgr.h"
#include "LFGMgr.h"
#include "InstanceSaveMgr.h"
#include "WorldSession.h"

ScenarioProgress::ScenarioProgress(uint32 _instanceId, lfg::LFGDungeonData const* _dungeonData)
    : instanceId(_instanceId), dungeonData(_dungeonData),
    m_achievementMgr(this), currentStep(0), currentTree(0)
{
    type = ScenarioMgr::GetScenarioType(GetScenarioId());
    ScenarioSteps const* _steps = sScenarioMgr->GetScenarioSteps(GetScenarioId());
    ASSERT(_steps);

    steps = *_steps;
    currentTree = GetScenarioCriteriaByStep(currentStep);
}

void ScenarioProgress::LoadFromDB()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SCENARIO_CRITERIAPROGRESS);
    stmt->setUInt32(0, instanceId);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    m_achievementMgr.LoadFromDB(NULL, result);
    UpdateCurrentStep(true);

    rewarded = IsCompleted(false);
    bonusRewarded = IsCompleted(true);
}

void ScenarioProgress::SaveToDB(SQLTransaction& trans)
{
    m_achievementMgr.SaveToDB(trans);
}

void ScenarioProgress::DeleteFromDB()
{
    m_achievementMgr.DeleteFromDB(instanceId, 0);
}

Map* ScenarioProgress::GetMap()
{
    return sMapMgr->FindMap(dungeonData->map, instanceId);
}

uint32 ScenarioProgress::GetScenarioId() const
{
    return dungeonData->dbc->scenarioId;
}

bool ScenarioProgress::IsCompleted(bool bonus) const
{
    if (!HasBonusStep())
        return false;

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
        //Not check if ctep already complete
        if(currentStep > itr->second->m_orderIndex)
            continue;

        CriteriaTreeEntry const* criteriaTree = sCriteriaTreeStore.LookupEntry(itr->second->m_criteriaTreeId);
        if (!criteriaTree)
            continue;

        if (GetAchievementMgr().IsCompletedScenarioTree(criteriaTree))
        {
            currentStep = itr->second->m_orderIndex + 1;
            currentTree = GetScenarioCriteriaByStep(currentStep);;
            continue;
        }
    }

    if (currentStep != oldStep && !loading)
    {
        SendStepUpdate();
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        SaveToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        if (IsCompleted(false))
            Reward(false);
        else if (IsCompleted(true))
            Reward(true);
    }

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "UpdateCurrentStep currentStep %u oldStep %u loading %u", currentStep, oldStep, loading);
    return currentStep;
}

uint32 ScenarioProgress::GetScenarioCriteriaByStep(uint8 step)
{
    for (ScenarioSteps::const_iterator itr = steps.begin(); itr != steps.end(); ++itr)
        if(step == itr->second->m_orderIndex)
            return itr->second->m_criteriaTreeId;

    return 0;
}

void ScenarioProgress::Reward(bool bonus)
{
    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "ScenarioProgress::Reward bonus %u rewarded %u bonusRewarded %u", bonus, rewarded, bonusRewarded);

    if (bonus && bonusRewarded)
        return;

    if (!bonus && rewarded)
        return;

    if (bonus)
    {
        bonusRewarded = true;

        // no bonus steps for this scenario...
        if (!HasBonusStep())
            return;
    }
    else
        rewarded = true;

    uint32 groupGuid = 0;
    Map* map = GetMap();
    // map not created? bye-bye reward
    if (!map)
        return;

    Map::PlayerList const& players = map->GetPlayers();
    // try to find group guid to refer to
    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        Player* player = itr->getSource();
        if (!player || !player->IsInWorld() || player->isGameMaster())
            continue;

        if (!groupGuid)
            groupGuid = player->GetGroup() ? player->GetGroup()->GetGUID() : 0;

        if (groupGuid)
            break;
    }

    // should not happen
    if (!groupGuid)
        return;

    uint32 dungeonId = sLFGMgr->GetDungeon(groupGuid);
    // lfg dungeons are rewarded through lfg
    if (dungeonId)
    {
        // lfg dungeon that we are in is not current scenario
        if (dungeonId != dungeonData->id)
            return;

        sLFGMgr->FinishDungeon(groupGuid, dungeonId);
    }
    // we have challenge
    else
    {
        ASSERT(GetType() != SCENARIO_TYPE_NORMAL);

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* player = itr->getSource();
            if (!player || !player->IsInWorld() || player->isGameMaster())
                continue;

            lfg::LfgReward const* reward = sLFGMgr->GetDungeonReward(dungeonData->id, player->getLevel());
            if (!reward)
                continue;

            // TODO: find challenge reward opcode ?
            bool alreadyCompleted = reward->RewardPlayer(player, dungeonData, dungeonData, bonus);
            lfg::LfgPlayerRewardData data = lfg::LfgPlayerRewardData(dungeonData->Entry(), dungeonData->Entry(), alreadyCompleted, bonus, reward);
            player->GetSession()->SendLfgPlayerReward(data);
        }
    }
}

void ScenarioProgress::SendStepUpdate(Player* player, bool full)
{
    WorldPacket data(SMSG_SCENARIO_STATE, 3 + 7 * 4);
    data.WriteBit(IsCompleted(true));                           // hasBonusStepComplete
    data.WriteBit(HasBonusStep());                              // hasBonusStep
    uint32 bitpos = data.bitwpos();
    data.WriteBits(0, 19);                                      // criteria data

    if (full)
    {
        ByteBuffer buff;
        uint32 count = 0;
        if (CriteriaProgressMap const* progressMap = GetAchievementMgr().GetCriteriaProgressMap())
        {
            for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
            {
                CriteriaTreeProgress const& progress = itr->second;
                CriteriaTreeEntry const* criteriaTreeEntry = sCriteriaTreeStore.LookupEntry(itr->first);
                if (!criteriaTreeEntry)
                    continue;

                ObjectGuid criteriaGuid = MAKE_NEW_GUID(1, GetScenarioId(), HIGHGUID_SCENARIO_CRITERIA);
                uint64 counter = progress.counter;

                data.WriteGuidMask<7, 6, 0, 4>(criteriaGuid);
                data.WriteGuidMask<7>(counter);
                data.WriteGuidMask<5, 1>(criteriaGuid);
                data.WriteGuidMask<1, 3, 0, 2>(counter);
                data.WriteGuidMask<3>(criteriaGuid);
                data.WriteGuidMask<5>(counter);
                data.WriteBits(0, 4);           // criteria flags
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
        }

        data.FlushBits();
        data.append(buff);
        data.PutBits(bitpos, count, 19);
    }

    data << uint32(0);                          // waveMax (only for ProvingGrounds)
    data << uint32(0);                          // difficultyID (only for ProvingGrounds)
    data << uint32(GetScenarioId());
    data << uint32(IsCompleted(false));         // scenarioCompleted?
    data << uint32(0);                          // waveCurrent (only for ProvingGrounds)
    data << uint32(0);                          // timerDuration (only for ProvingGrounds)
    data << uint32(currentStep);

    if (player)
        player->SendDirectMessage(&data);
    else
        BroadCastPacket(data);
}

void ScenarioProgress::SendCriteriaUpdate(uint32 criteriaId, uint32 counter, time_t date)
{
    WorldPacket data(SMSG_SCENARIO_CRITERIA_UPDATE, 8 + 8 + 4 * 4 + 1);

    ObjectGuid criteriaGuid = MAKE_NEW_GUID(1, GetScenarioId(), HIGHGUID_SCENARIO_CRITERIA);

    data.WriteGuidMask<3>(counter);
    data.WriteGuidMask<7, 2, 0>(criteriaGuid);
    data.WriteGuidMask<4>(counter);
    data.WriteGuidMask<4, 6>(criteriaGuid);
    data.WriteBits(0, 4);               // criteria flags
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
    Map* map = GetMap();
    if (!map)
        return;

    map->SendToPlayers(&data);
}

bool ScenarioProgress::CanUpdateCriteria(uint32 criteriaId, uint32 recursTree /*=0*/) const
{
    std::list<uint32> const* cTreeList = GetCriteriaTreeList(recursTree ? recursTree : currentTree);
    for (std::list<uint32>::const_iterator itr = cTreeList->begin(); itr != cTreeList->end(); ++itr)
    {
        if(CriteriaTreeEntry const* criteriaTree = sCriteriaTreeStore.LookupEntry(*itr))
        {
            if(criteriaTree->criteria == 0)
            {
                if(CanUpdateCriteria(criteriaId, *itr))
                    return true;
            }
            else if(criteriaTree->ID == criteriaId)
                return true;
        }
    }

    return false;
}

ScenarioMgr::ScenarioMgr() : updateDiff(0)
{
    Initialize();
}

ScenarioMgr::~ScenarioMgr()
{
    for (ScenarioProgressMap::iterator itr = m_scenarioProgressMap.begin(); itr != m_scenarioProgressMap.end(); ++itr)
        delete itr->second;
}

ScenarioProgress* ScenarioMgr::GetScenarioProgress(uint32 instanceId)
{
    ScenarioProgressMap::iterator itr = m_scenarioProgressMap.find(instanceId);
    return itr != m_scenarioProgressMap.end() ? itr->second : NULL;
}

void ScenarioMgr::AddScenarioProgress(uint32 instanceId, lfg::LFGDungeonData const* dungeonData, bool loading)
{
    if (m_scenarioProgressMap.find(instanceId) != m_scenarioProgressMap.end())
        return;

    ScenarioProgress* progress = new ScenarioProgress(instanceId, dungeonData);
    m_scenarioProgressMap[instanceId] = progress;
    if (loading)
        progress->LoadFromDB();
}

void ScenarioMgr::RemoveScenarioProgress(uint32 instanceId)
{
    ScenarioProgressMap::iterator itr = m_scenarioProgressMap.find(instanceId);
    if (itr == m_scenarioProgressMap.end())
        return;

    itr->second->DeleteFromDB();
    delete itr->second;
    m_scenarioProgressMap.erase(itr);
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
        itr->second->SaveToDB(trans);

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
            buff << uint32(itr->WorldEffectID);
            buff << uint32(itr->Floor);
            buff << uint32(itr->Unk16);

            data.WriteBits(itr->points.size(), 21); // POI points count
            for (std::vector<ScenarioPOIPoint>::const_iterator itr2 = itr->points.begin(); itr2 != itr->points.end(); ++itr2)
            {
                buff << int32(itr2->y);             // POI point y
                buff << int32(itr2->x);             // POI point x
            }

            buff << uint32(itr->WorldMapAreaId);
            buff << uint32(itr->Unk28);             // PlayerConditionID?
            buff << uint32(itr->Unk20);
        }

        buff << uint32(criteriaTreeId);             // criteria tree id
    }

    data.FlushBits();
    if (!buff.empty())
        data.append(buff);

    SendPacket(&data);
}

