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

#include "Common.h"
#include "SharedDefines.h"
#include "DBCStores.h"
#include "DisableMgr.h"
#include "ObjectMgr.h"
#include "SocialMgr.h"
#include "LFGMgr.h"
#include "LFGScripts.h"
#include "LFGGroupData.h"
#include "LFGPlayerData.h"
#include "Chat.h"

#include "LFGQueue.h"
#include "Group.h"
#include "Player.h"
#include "GroupMgr.h"
#include "GameEventMgr.h"

LFGMgr::LFGMgr(): m_QueueTimer(0), m_lfgProposalId(1),
    m_options(sWorld->getBoolConfig(CONFIG_DUNGEON_FINDER_ENABLE)),
    m_lfgPlayerScript(new LFGPlayerScript()), m_lfgGroupScript(new LFGGroupScript())
{ }

LFGMgr::~LFGMgr()
{
    for (LfgRewardMap::iterator itr = m_RewardMap.begin(); itr != m_RewardMap.end(); ++itr)
        delete itr->second;

    delete m_lfgPlayerScript;
    delete m_lfgGroupScript;
}

void LFGMgr::_LoadFromDB(Field* fields, uint64 guid)
{
    if (!fields)
        return;

    if (!IS_GROUP(guid))
        return;

    uint32 dungeon = fields[16].GetUInt32();

    uint8 state = fields[17].GetUInt8();

    if (!dungeon || !state)
        return;

    SetDungeon(guid, dungeon);

    switch (state)
    {
        case LFG_STATE_DUNGEON:
        case LFG_STATE_FINISHED_DUNGEON:
            SetState(guid, (LfgState)state);
            break;
        default:
            break;
    }
}

void LFGMgr::_SaveToDB(uint64 guid, uint32 db_guid)
{
    if (!IS_GROUP(guid))
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_LFG_DATA);

    stmt->setUInt32(0, db_guid);

    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_LFG_DATA);
    stmt->setUInt32(0, db_guid);

    stmt->setUInt32(1, GetDungeon(guid));
    stmt->setUInt32(2, GetState(guid));

    CharacterDatabase.Execute(stmt);
}

std::string LFGMgr::ConcatenateDungeons(LfgDungeonSet const& dungeons)
{
    std::string dungeonstr = "";
    if (!dungeons.empty())
    {
        std::ostringstream o;
        LfgDungeonSet::const_iterator it = dungeons.begin();
        o << (*it);
        for (++it; it != dungeons.end(); ++it)
            o << ", " << uint32(*it);
        dungeonstr = o.str();
    }
    return dungeonstr;
}

std::string LFGMgr::GetRolesString(uint8 roles)
{
    std::string rolesstr = "";

    if (roles & PLAYER_ROLE_TANK)
         rolesstr.append(sObjectMgr->GetTrinityStringForDBCLocale(LANG_LFG_ROLE_TANK));

    if (roles & PLAYER_ROLE_HEALER)
    {
        if (!rolesstr.empty())
            rolesstr.append(", ");
        rolesstr.append(sObjectMgr->GetTrinityStringForDBCLocale(LANG_LFG_ROLE_HEALER));
    }

    if (roles & PLAYER_ROLE_DAMAGE)
    {
        if (!rolesstr.empty())
            rolesstr.append(", ");
        rolesstr.append(sObjectMgr->GetTrinityStringForDBCLocale(LANG_LFG_ROLE_DAMAGE));
    }

    if (roles & PLAYER_ROLE_LEADER)
    {
        if (!rolesstr.empty())
            rolesstr.append(", ");
        rolesstr.append(sObjectMgr->GetTrinityStringForDBCLocale(LANG_LFG_ROLE_LEADER));
    }

    if (rolesstr.empty())
        rolesstr.append(sObjectMgr->GetTrinityStringForDBCLocale(LANG_LFG_ROLE_NONE));

    return rolesstr;
}

char const * LFGMgr::GetStateString(LfgState state)
{
    int32 entry = LANG_LFG_ERROR;
    switch (state)
    {
        case LFG_STATE_NONE:
            entry = LANG_LFG_STATE_NONE;
            break;
        case LFG_STATE_ROLECHECK:
            entry = LANG_LFG_STATE_ROLECHECK;
            break;
        case LFG_STATE_QUEUED:
            entry = LANG_LFG_STATE_QUEUED;
            break;
        case LFG_STATE_PROPOSAL:
            entry = LANG_LFG_STATE_PROPOSAL;
            break;
        case LFG_STATE_DUNGEON:
            entry = LANG_LFG_STATE_DUNGEON;
            break;
        case LFG_STATE_BOOT:
            entry = LANG_LFG_STATE_BOOT;
            break;
        case LFG_STATE_FINISHED_DUNGEON:
            entry = LANG_LFG_STATE_FINISHED_DUNGEON;
            break;
        case LFG_STATE_RAIDBROWSER:
            entry = LANG_LFG_STATE_RAIDBROWSER;
    }
    char const * const str = sObjectMgr->GetTrinityStringForDBCLocale(entry);
    return str;
}

/// Load rewards for completing dungeons
void LFGMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    for (LfgRewardMap::iterator itr = m_RewardMap.begin(); itr != m_RewardMap.end(); ++itr)
        delete itr->second;
    m_RewardMap.clear();

    // ORDER BY is very important for GetRandomDungeonReward!
    QueryResult result = WorldDatabase.Query("SELECT dungeonId, maxLevel, firstQuestId, firstMoneyVar, firstXPVar, otherQuestId, otherMoneyVar, otherXPVar FROM lfg_dungeon_rewards ORDER BY dungeonId, maxLevel ASC");

    if (!result)
    {
        sLog->outError(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 lfg dungeon rewards. DB table `lfg_dungeon_rewards` is empty!");
        return;
    }

    uint32 count = 0;

    Field* fields = NULL;
    do
    {
        fields = result->Fetch();
        uint32 dungeonId = fields[0].GetUInt32();
        uint32 maxLevel = fields[1].GetUInt8();
        uint32 firstQuestId = fields[2].GetUInt32();
        uint32 firstMoneyVar = fields[3].GetUInt32();
        uint32 firstXPVar = fields[4].GetUInt32();
        uint32 otherQuestId = fields[5].GetUInt32();
        uint32 otherMoneyVar = fields[6].GetUInt32();
        uint32 otherXPVar = fields[7].GetUInt32();

        if (!GetLFGDungeon(dungeonId))
        {
            sLog->outError(LOG_FILTER_SQL, "Dungeon %u specified in table `lfg_dungeon_rewards` does not exist!", dungeonId);
            continue;
        }

        if (!maxLevel || maxLevel > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            sLog->outError(LOG_FILTER_SQL, "Level %u specified for dungeon %u in table `lfg_dungeon_rewards` can never be reached!", maxLevel, dungeonId);
            maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
        }

        if (firstQuestId && !sObjectMgr->GetQuestTemplate(firstQuestId))
        {
            sLog->outError(LOG_FILTER_SQL, "First quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", firstQuestId, dungeonId);
            firstQuestId = 0;
        }

        if (otherQuestId && !sObjectMgr->GetQuestTemplate(otherQuestId))
        {
            sLog->outError(LOG_FILTER_SQL, "Other quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", otherQuestId, dungeonId);
            otherQuestId = 0;
        }

        m_RewardMap.insert(LfgRewardMap::value_type(dungeonId, new LfgReward(maxLevel, firstQuestId, firstMoneyVar, firstXPVar, otherQuestId, otherMoneyVar, otherXPVar)));
        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u lfg dungeon rewards in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

LFGDungeonEntry const* LFGMgr::GetLFGDungeon(uint32 id)
{
    LFGDungeonMap::const_iterator itr = m_LfgDungeonMap.find(id);
    if (itr != m_LfgDungeonMap.end())
        return &(itr->second);

    return NULL;
}

LFGDungeonMap & LFGMgr::GetLFGDungeonMap()
{
    return m_LfgDungeonMap;
}

void LFGMgr::LoadLFGDungeons(bool reload /* = false */)
{
    uint32 oldMSTime = getMSTime();

    m_LfgDungeonMap.clear();

    // Initialize Dungeon map with data from dbcs
    for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
    {
        LFGDungeonEntryDbc const* dungeon = sLFGDungeonStore.LookupEntry(i);
        if (!dungeon)
            continue;

        switch (dungeon->type)
        {
            case LFG_TYPE_DUNGEON:
            case LFG_TYPE_HEROIC:
            case LFG_TYPE_RAID:
            case LFG_TYPE_RANDOM:
                m_LfgDungeonMap[dungeon->ID] = LFGDungeonEntry(dungeon);
                break;
        }
    }

    // Fill teleport locations from DB
    QueryResult result = WorldDatabase.Query("SELECT dungeonId, position_x, position_y, position_z, orientation FROM lfg_entrances");

    if (!result)
    {
        sLog->outError(LOG_FILTER_SQL, ">> Loaded 0 lfg entrance positions. DB table `lfg_entrances` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 dungeonId = fields[0].GetUInt32();
        LFGDungeonMap::iterator dungeonItr = m_LfgDungeonMap.find(dungeonId);
        if (dungeonItr == m_LfgDungeonMap.end())
        {
            sLog->outError(LOG_FILTER_SQL, "table `lfg_entrances` contains coordinates for wrong dungeon %u", dungeonId);
            continue;
        }

        LFGDungeonEntry& data = dungeonItr->second;
        data.x = fields[1].GetFloat();
        data.y = fields[2].GetFloat();
        data.z = fields[3].GetFloat();
        data.o = fields[4].GetFloat();
        ++count;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u lfg entrance positions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

    // Fill all other teleport coords from areatriggers
    for (LFGDungeonMap::iterator itr = m_LfgDungeonMap.begin(); itr != m_LfgDungeonMap.end(); ++itr)
    {
        LFGDungeonEntry& dungeon = itr->second;
        // No teleport coords in database, load from areatriggers
        if (dungeon.x == 0.0f && dungeon.y == 0.0f && dungeon.z == 0.0f)
        {
            AreaTrigger const* at = sObjectMgr->GetMapEntranceTrigger(dungeon.map);
            if (!at)
            {
                sLog->outError(LOG_FILTER_LFG, "LFGMgr::LoadLFGDungeons: Failed to load dungeon %s, cant find areatrigger for map %u", dungeon.name.c_str(), dungeon.map);
                continue;
            }

            dungeon.map = at->target_mapId;
            dungeon.x = at->target_X;
            dungeon.y = at->target_Y;
            dungeon.z = at->target_Z;
            dungeon.o = at->target_Orientation;
        }

        if (dungeon.type != LFG_TYPE_RANDOM)
            m_CachedDungeonMap[dungeon.random_id].insert(dungeon.id);
        m_CachedDungeonMap[0].insert(dungeon.id);
    }

    if (reload)
    {
        m_CachedDungeonMap.clear();
        // Recalculate locked dungeons
        for (LfgPlayerDataMap::const_iterator it = m_Players.begin(); it != m_Players.end(); ++it)
            if (Player* player = ObjectAccessor::FindPlayer(it->first))
                InitializeLockedDungeons(player);
    }
}

void LFGMgr::Update(uint32 diff)
{
    if (!m_options)
        return;

    time_t currTime = time(NULL);

    // Remove obsolete role checks
    for (LfgRoleCheckMap::iterator it = m_RoleChecks.begin(); it != m_RoleChecks.end();)
    {
        LfgRoleCheckMap::iterator itRoleCheck = it++;
        LfgRoleCheck& roleCheck = itRoleCheck->second;
        if (currTime < roleCheck.cancelTime)
            continue;
        roleCheck.state = LFG_ROLECHECK_MISSING_ROLE;

        for (LfgRolesMap::const_iterator itRoles = roleCheck.roles.begin(); itRoles != roleCheck.roles.end(); ++itRoles)
        {
            uint64 guid = itRoles->first;
            ClearState(guid, "Remove Obsolete RoleCheck");
            SendLfgRoleCheckUpdate(guid, roleCheck, false);
            if (guid == roleCheck.leader)
                SendLfgJoinResult(guid, LfgJoinResultData(LFG_JOIN_FAILED, LFG_ROLECHECK_MISSING_ROLE));
        }
        m_RoleChecks.erase(itRoleCheck);
    }

    // Remove obsolete proposals
    for (LfgProposalMap::iterator it = m_Proposals.begin(); it != m_Proposals.end();)
    {
        LfgProposalMap::iterator itRemove = it++;
        if (itRemove->second.cancelTime < currTime)
            RemoveProposal(itRemove, LFG_UPDATETYPE_PROPOSAL_FAILED);
    }

    // Remove obsolete kicks
    for (LfgPlayerBootMap::iterator it = m_Boots.begin(); it != m_Boots.end();)
    {
        LfgPlayerBootMap::iterator itBoot = it++;
        LfgPlayerBoot& boot = itBoot->second;
        if (boot.cancelTime < currTime)
        {
            boot.inProgress = false;
            for (LfgAnswerMap::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
            {
                uint64 pguid = itVotes->first;
                if (pguid != boot.victim)
                    SendLfgBootProposalUpdate(pguid, boot);
            }
            m_Boots.erase(itBoot);
        }
    }

    uint32 lastProposalId = m_lfgProposalId;
    // Check if a proposal can be formed with the new groups being added
    for (LfgQueueMap::iterator it = m_Queues.begin(); it != m_Queues.end(); ++it)
        if (uint8 newProposals = it->second.FindGroups())
            sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::Update: Found %u new groups in queue %u", newProposals, it->first);

    if (lastProposalId != m_lfgProposalId)
    {
        // FIXME lastProposalId ? lastProposalId +1 ?
        for (LfgProposalMap::const_iterator itProposal = m_Proposals.find(m_lfgProposalId); itProposal != m_Proposals.end(); ++itProposal)
        {
            uint32 proposalId = itProposal->first;
            LfgProposal& proposal = m_Proposals[proposalId];

            uint64 guid = 0;
            for (LfgProposalPlayerMap::const_iterator itPlayers = proposal.players.begin(); itPlayers != proposal.players.end(); ++itPlayers)
            {
                guid = itPlayers->first;
                SetState(guid, LFG_STATE_PROPOSAL);
                if (uint64 gguid = GetGroup(guid))
                {
                    SetState(gguid, LFG_STATE_PROPOSAL);
                    SendLfgUpdateParty(guid, LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                }
                else
                    SendLfgUpdatePlayer(guid, LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                SendLfgUpdateProposal(guid, m_lfgProposalId, proposal);
            }

            if (proposal.state == LFG_PROPOSAL_SUCCESS)
                UpdateProposal(proposalId, guid, true);
        }
    }

    /*for (LfgGuidListMap::iterator it = m_newToQueue.begin(); it != m_newToQueue.end(); ++it)
    {
        uint8 queueId = it->first;
        LfgGuidList& newToQueue = it->second;
        LfgGuidList& currentQueue = m_currentQueue[queueId];
        LfgGuidList firstNew;
        while (!newToQueue.empty())
        {
            uint64 frontguid = newToQueue.front();
            sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::Update: QueueId %u: checking [" UI64FMTD "] newToQueue(%u), currentQueue(%u)", queueId, frontguid, uint32(newToQueue.size()), uint32(currentQueue.size()));
            firstNew.push_back(frontguid);
            newToQueue.pop_front();
            uint8 alreadyInQueue = 0;
            LfgGuidList temporalList = currentQueue;
            if (LfgProposal* pProposal = FindNewGroups(firstNew, temporalList, LFG_TYPE_DUNGEON)) // Group found!
            {
                // Remove groups in the proposal from new and current queues (not from queue map)
                for (LfgGuidList::const_iterator itQueue = pProposal->queues.begin(); itQueue != pProposal->queues.end(); ++itQueue)
                {
                    currentQueue.remove(*itQueue);
                    newToQueue.remove(*itQueue);
                }
                m_Proposals[++m_lfgProposalId] = pProposal;

                uint64 guid = 0;
                for (LfgProposalPlayerMap::const_iterator itPlayers = pProposal->players.begin(); itPlayers != pProposal->players.end(); ++itPlayers)
                {
                    guid = itPlayers->first;
                    SetState(guid, LFG_STATE_PROPOSAL);
                    if (Player* player = ObjectAccessor::FindPlayer(itPlayers->first))
                    {
                        Group* grp = player->GetGroup();
                        if (grp)
                         {
                            uint64 gguid = grp->GetGUID();
                            SetState(gguid, LFG_STATE_PROPOSAL);
                            player->GetSession()->SendLfgUpdateParty(LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                        }
                        else
                            player->GetSession()->SendLfgUpdatePlayer(LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                        player->GetSession()->SendLfgUpdateProposal(m_lfgProposalId, *pProposal);
                    }
                }

                if (pProposal->state == LFG_PROPOSAL_SUCCESS)
                    UpdateProposal(m_lfgProposalId, guid, true);
            }
            else
            {
                if (std::find(currentQueue.begin(), currentQueue.end(), frontguid) == currentQueue.end()) //already in queue?
                    ++alreadyInQueue; //currentQueue.push_back(frontguid);         // Lfg group not found, add this group to the queue.
                temporalList = currentQueue;
                m_CompatibleMap.clear();
            }

            if (LfgProposal* pProposal = FindNewGroups(firstNew, temporalList, LFG_TYPE_RAID)) // Group found!
            {
                // Remove groups in the proposal from new and current queues (not from queue map)
                for (LfgGuidList::const_iterator itQueue = pProposal->queues.begin(); itQueue != pProposal->queues.end(); ++itQueue)
                {
                    currentQueue.remove(*itQueue);
                    newToQueue.remove(*itQueue);
                }
                m_Proposals[++m_lfgProposalId] = pProposal;

                uint64 guid = 0;
                for (LfgProposalPlayerMap::const_iterator itPlayers = pProposal->players.begin(); itPlayers != pProposal->players.end(); ++itPlayers)
                {
                    guid = itPlayers->first;
                    SetState(guid, LFG_STATE_PROPOSAL);
                    if (Player* player = ObjectAccessor::FindPlayer(itPlayers->first))
                    {
                        Group* grp = player->GetGroup();
                        if (grp)
                         {
                            uint64 gguid = grp->GetGUID();
                            SetState(gguid, LFG_STATE_PROPOSAL);
                            player->GetSession()->SendLfgUpdateParty(LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                        }
                        else
                            player->GetSession()->SendLfgUpdatePlayer(LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                        player->GetSession()->SendLfgUpdateProposal(m_lfgProposalId, *pProposal);
                    }
                }

                if (pProposal->state == LFG_PROPOSAL_SUCCESS)
                    UpdateProposal(m_lfgProposalId, guid, true);
            }
            else
            {
                if (std::find(currentQueue.begin(), currentQueue.end(), frontguid) == currentQueue.end()) //already in queue?
                    ++alreadyInQueue; //currentQueue.push_back(frontguid);         // Lfg group not found, add this group to the queue.
                temporalList = currentQueue;
                m_CompatibleMap.clear();
            }

            if (LfgProposal* pProposal = FindNewGroups(firstNew, temporalList, LFG_TYPE_SCENARIO)) // Group found!
            {
                // Remove groups in the proposal from new and current queues (not from queue map)
                for (LfgGuidList::const_iterator itQueue = pProposal->queues.begin(); itQueue != pProposal->queues.end(); ++itQueue)
                {
                    currentQueue.remove(*itQueue);
                    newToQueue.remove(*itQueue);
                }
                m_Proposals[++m_lfgProposalId] = pProposal;

                uint64 guid = 0;
                for (LfgProposalPlayerMap::const_iterator itPlayers = pProposal->players.begin(); itPlayers != pProposal->players.end(); ++itPlayers)
                {
                    guid = itPlayers->first;
                    SetState(guid, LFG_STATE_PROPOSAL);
                    if (Player* player = ObjectAccessor::FindPlayer(itPlayers->first))
                    {
                        Group* grp = player->GetGroup();
                        if (grp)
                         {
                            uint64 gguid = grp->GetGUID();
                            SetState(gguid, LFG_STATE_PROPOSAL);
                            player->GetSession()->SendLfgUpdateParty(LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                        }
                        else
                            player->GetSession()->SendLfgUpdatePlayer(LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                        player->GetSession()->SendLfgUpdateProposal(m_lfgProposalId, *pProposal);
                    }
                }

                if (pProposal->state == LFG_PROPOSAL_SUCCESS)
                    UpdateProposal(m_lfgProposalId, guid, true);
            }
            else
            {
                if (std::find(currentQueue.begin(), currentQueue.end(), frontguid) == currentQueue.end()) //already in queue?
                    ++alreadyInQueue; //currentQueue.push_back(frontguid);         // Lfg group not found, add this group to the queue.
                temporalList = currentQueue;
            }

            if (alreadyInQueue == 3 && std::find(currentQueue.begin(), currentQueue.end(), frontguid) == currentQueue.end())
                currentQueue.push_back(frontguid);

            firstNew.clear();
        }
    }*/

    // Update all players status queue info
    if (m_QueueTimer > LFG_QUEUEUPDATE_INTERVAL)
    {
        m_QueueTimer = 0;
        time_t currTime = time(NULL);
        for (LfgQueueMap::iterator it = m_Queues.begin(); it != m_Queues.end(); ++it)
            it->second.UpdateQueueTimers(currTime);
    }
    else
        m_QueueTimer += diff;
}

/**
    Generate the dungeon lock map for a given player

   @param[in]     player Player we need to initialize the lock status map
*/
void LFGMgr::InitializeLockedDungeons(Player* player, uint8 level /* = 0 */)
{
    uint64 guid = player->GetGUID();
    if (!level)
        level = player->getLevel();
    uint8 expansion = player->GetSession()->Expansion();
    LfgDungeonSet const& dungeons = GetDungeonsByRandom(0);
    LfgLockMap lock;

    for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end(); ++it)
    {
        LFGDungeonEntry const* dungeon = GetLFGDungeon(*it);
        if (!dungeon) // should never happen - We provide a list from sLFGDungeonStore
            continue;

        uint32 lockData = 0;
        if (dungeon->expansion > expansion)
            lockData = LFG_LOCKSTATUS_INSUFFICIENT_EXPANSION;
        else if (DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, dungeon->map, player))
            lockData = LFG_LOCKSTATUS_RAID_LOCKED;
        else if (dungeon->difficulty > REGULAR_DIFFICULTY && player->GetBoundInstance(dungeon->map, Difficulty(dungeon->difficulty)))
            lockData = LFG_LOCKSTATUS_RAID_LOCKED;
        else if (dungeon->minlevel > level)
            lockData = LFG_LOCKSTATUS_TOO_LOW_LEVEL;
        else if (dungeon->maxlevel < level)
            lockData = LFG_LOCKSTATUS_TOO_HIGH_LEVEL;
        else if (dungeon->seasonal && !IsSeasonActive(dungeon->id))
            lockData = LFG_LOCKSTATUS_NOT_IN_SEASON;
        else if (AccessRequirement const* ar = sObjectMgr->GetAccessRequirement(dungeon->map, Difficulty(dungeon->difficulty)))
        {
            if (ar->achievement && !player->GetAchievementMgr().HasAchieved(ar->achievement))
                lockData = LFG_LOCKSTATUS_RAID_LOCKED;       // FIXME: Check the correct lock value
            else if (player->GetTeam() == ALLIANCE && ar->quest_A && !player->GetQuestRewardStatus(ar->quest_A))
                lockData = LFG_LOCKSTATUS_QUEST_NOT_COMPLETED;
            else if (player->GetTeam() == HORDE && ar->quest_H && !player->GetQuestRewardStatus(ar->quest_H))
                lockData = LFG_LOCKSTATUS_QUEST_NOT_COMPLETED;
            else
                if (ar->item)
                {
                    if (!player->HasItemCount(ar->item) && (!ar->item2 || !player->HasItemCount(ar->item2)))
                        lockData = LFG_LOCKSTATUS_MISSING_ITEM;
                }
                else if (ar->item2 && !player->HasItemCount(ar->item2))
                    lockData = LFG_LOCKSTATUS_MISSING_ITEM;
        }
        /* TODO VoA closed if WG is not under team control (LFG_LOCKSTATUS_RAID_LOCKED)
            lockData = LFG_LOCKSTATUS_TOO_LOW_GEAR_SCORE;
            lockData = LFG_LOCKSTATUS_TOO_HIGH_GEAR_SCORE;
            lockData = LFG_LOCKSTATUS_ATTUNEMENT_TOO_LOW_LEVEL;
            lockData = LFG_LOCKSTATUS_ATTUNEMENT_TOO_HIGH_LEVEL;
        */

        if (lockData != LFG_LOCKSTATUS_OK)
            lock[dungeon->Entry()] = lockData;
    }
    SetLockedDungeons(guid, lock);
}

/**
    Adds the player/group to lfg queue. If player is in a group then it is the leader
    of the group tying to join the group. Join conditions are checked before adding
    to the new queue.

   @param[in]     player Player trying to join (or leader of group trying to join)
   @param[in]     roles Player selected roles
   @param[in]     dungeons Dungeons the player/group is applying for
   @param[in]     comment Player selected comment
*/
void LFGMgr::JoinLfg(Player* player, uint8 roles, LfgDungeonSet& dungeons, const std::string& comment)
{
    if (!player || !player->GetSession() || dungeons.empty())
       return;

    Group* group = player->GetGroup();
    uint64 playerGuid = player->GetGUID();
    uint64 groupGuid = group ? group->GetGUID() : playerGuid;
    LfgJoinResultData joinData;
    LfgGuidSet players;
    uint32 rDungeonId = 0;
    bool isContinueDungeonRequest = group && group->isLFGGroup() && GetState(groupGuid) != LFG_STATE_FINISHED_DUNGEON;

    // Do not allow to change dungeon in the middle of a current dungeon
    if (isContinueDungeonRequest)
    {
        dungeons.clear();
        dungeons.insert(GetDungeon(groupGuid));
    }

    // Already in queue?
    LfgState state = GetState(groupGuid);
    if (state == LFG_STATE_QUEUED)
    {
        LfgQueue& queue = GetQueue(groupGuid);

        // currently core cannot queue in LFG/Flex/Scenarios at the same time
        if (!dungeons.empty())
        {
            LFGDungeonEntry const* entry = sLFGMgr->GetLFGDungeon(*dungeons.begin() & 0xFFFFF);
            if (queue.GetQueueType(groupGuid) != entry->internalType)
            {
                ChatHandler(player).PSendSysMessage("You cannot queue in different type queues at the same time.");
                joinData.result = LFG_JOIN_INTERNAL_ERROR;
            }
        }

        if (joinData.result == LFG_JOIN_OK)
        {
            LfgDungeonSet const& playerDungeons = GetSelectedDungeons(playerGuid);
            if (playerDungeons == dungeons)                     // Joining the same dungeons -- Send OK
             {
                LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, dungeons, comment);
                player->GetSession()->SendLfgJoinResult(joinData); // Default value of joinData.result = LFG_JOIN_OK
                if (group)
                {
                    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                        if (itr->getSource() && itr->getSource()->GetSession())
                            itr->getSource()->GetSession()->SendLfgUpdateParty(updateData);
                }
                return;
            }
            else // Remove from queue and rejoin
            {
                SendUpdateStatus(player, GetComment(player->GetGUID()), playerDungeons, false, true);
                queue.RemoveFromQueue(groupGuid);
            }
        }
    }

    // Check player or group member restrictions
    if (player->InBattleground() || player->InArena() || player->InBattlegroundQueue())
        joinData.result = LFG_JOIN_USING_BG_SYSTEM;
    else if (player->HasAura(LFG_SPELL_DUNGEON_DESERTER))
        joinData.result = LFG_JOIN_DESERTER;
    else if (player->HasAura(LFG_SPELL_DUNGEON_COOLDOWN))
        joinData.result = LFG_JOIN_RANDOM_COOLDOWN;
    else if (dungeons.empty())
        joinData.result = LFG_JOIN_NOT_MEET_REQS;
    else if (group)
    {
        uint8 memberCount = 0;
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL && joinData.result == LFG_JOIN_OK; itr = itr->next())
        {
            if (Player* groupPlayer = itr->getSource())
            {
                if (groupPlayer->HasAura(LFG_SPELL_DUNGEON_DESERTER))
                    joinData.result = LFG_JOIN_PARTY_DESERTER;
                else if (groupPlayer->HasAura(LFG_SPELL_DUNGEON_COOLDOWN))
                    joinData.result = LFG_JOIN_PARTY_RANDOM_COOLDOWN;
                else if (groupPlayer->InBattleground() || groupPlayer->InArena() || groupPlayer->InBattlegroundQueue())
                    joinData.result = LFG_JOIN_USING_BG_SYSTEM;
                ++memberCount;
                players.insert(groupPlayer->GetGUID());
            }
        }
        if (memberCount != group->GetMembersCount() && joinData.result == LFG_JOIN_OK)
            joinData.result = LFG_JOIN_DISCONNECTED;
    }
    else if (joinData.result == LFG_JOIN_OK)
        players.insert(player->GetGUID());

    // Check if all dungeons are valid
    if (joinData.result == LFG_JOIN_OK)
    {
        bool isDungeon = false;
        bool isRaid = false;
        for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end() && joinData.result == LFG_JOIN_OK; ++it)
        {
            LFGDungeonEntry const* entry = sLFGMgr->GetLFGDungeon(*it & 0xFFFFF);
            switch (entry->dbc->subType)
            {
                case LFG_SUBTYPE_DUNGEON:
                case LFG_SUBTYPE_SCENARIO:
                    isDungeon = true;
                    break;
                case LFG_SUBTYPE_RAID:
                case LFG_SUBTYPE_FLEX:
                    isRaid = true;
                    break;
            }

            if (isDungeon && isRaid)
            {
                //joinData.result = LFG_JOIN_MIXED_RAID_DUNGEON;
                joinData.result = LFG_JOIN_INTERNAL_ERROR;
            }

            switch (entry->dbc->type)
            {
                // FIXME: can join to random dungeon and random scenario at the same time
                case LFG_TYPE_RANDOM:
                    if (dungeons.size() > 1)
                        joinData.result = LFG_JOIN_INTERNAL_ERROR;
                    else
                        rDungeonId = *it;
                    break;
                case LFG_TYPE_DUNGEON:
                case LFG_TYPE_RAID:
                    break;
                default:
                    joinData.result = LFG_JOIN_DUNGEON_INVALID;
                    break;
            }

            // check members count for a group
            if (group)
            {
                uint32 minPlayerCount = entry->dbc->GetMinGroupSize();
                uint32 maxPlayerCount = entry->dbc->GetMaxGroupSize();
                uint32 groupSize = group->GetMembersCount();
                if (groupSize < minPlayerCount && !isContinueDungeonRequest)
                    joinData.result = LFG_TOO_FEW_MEMBERS;
                if (groupSize > maxPlayerCount)
                    joinData.result = LFG_JOIN_TOO_MUCH_MEMBERS;
            }
        }

        // it could be changed
        if (joinData.result == LFG_JOIN_OK)
        {
            // Expand random dungeons and check restrictions
            if (rDungeonId)
                dungeons = GetDungeonsByRandom(rDungeonId & 0xFFFFF);

            GetCompatibleDungeons(dungeons, players, joinData.lockmap);
            // if we have lockmap then there are no compatible dungeons
            if (dungeons.empty())
                joinData.result = group ? LFG_JOIN_PARTY_NOT_MEET_REQS : LFG_JOIN_NOT_MEET_REQS;
        }
    }

    // Can't join. Send result
    if (joinData.result != LFG_JOIN_OK)
    {
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::Join: [" UI64FMTD "] joining with %u members. result: %u", playerGuid, group ? group->GetMembersCount() : 1, joinData.result);
        if (!dungeons.empty())                             // Only should show lockmap when have no dungeons available
            joinData.lockmap.clear();
        player->GetSession()->SendLfgJoinResult(joinData);
        return;
    }

    // FIXME - Raid browser not supported yet
    // FIXME: raid finder dungeons are raids
    /*if (isRaid)
    {
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::Join: [" UI64FMTD "] trying to join raid browser and it's disabled.", playerGuid);
        return;
    }*/

    SetComment(playerGuid, comment);

    std::string debugNames = "";
    if (group)                                               // Begin rolecheck
    {
        // Create new rolecheck
        LfgRoleCheck& roleCheck = m_RoleChecks[groupGuid];
        roleCheck.cancelTime = time_t(time(NULL)) + LFG_TIME_ROLECHECK;
        roleCheck.state = LFG_ROLECHECK_INITIALITING;
        roleCheck.leader = playerGuid;
        roleCheck.dungeons = dungeons;
        roleCheck.rDungeonId = rDungeonId;

        if (rDungeonId)
        {
            dungeons.clear();
            dungeons.insert(rDungeonId);
        }

        SetState(groupGuid, LFG_STATE_ROLECHECK);
        // Send update to player
        LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_JOIN_PROPOSAL, dungeons, comment);
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* plrg = itr->getSource())
            {
                uint64 pguid = plrg->GetGUID();
                SetState(pguid, LFG_STATE_ROLECHECK);
                if (!isContinueDungeonRequest)
                    SetSelectedDungeons(pguid, dungeons);
                roleCheck.roles[pguid] = 0;
                if (!debugNames.empty())
                    debugNames.append(", ");
                debugNames.append(plrg->GetName());
            }
        }
        // Update leader role
        UpdateRoleCheck(groupGuid, playerGuid, roles);
    }
    else                                                   // Add player to queue
    {
        LFGDungeonEntry const* entry = sLFGMgr->GetLFGDungeon(*dungeons.begin() & 0xFFFFF);

        // Queue player
        LfgRolesMap rolesMap;
        rolesMap[playerGuid] = roles;
        LfgQueue& queue = GetQueue(groupGuid);
        queue.AddQueueData(playerGuid, time_t(time(NULL)), dungeons, rolesMap);

        if (!isContinueDungeonRequest)
        {
            if (rDungeonId)
            {
                dungeons.clear();
                dungeons.insert(rDungeonId);
            }
            SetSelectedDungeons(playerGuid, dungeons);
        }
        // Send update to player
        player->GetSession()->SendLfgJoinResult(joinData);
        //player->GetSession()->SendLfgUpdatePlayer(LfgUpdateData(LFG_UPDATETYPE_JOIN_PROPOSAL, dungeons, comment));
        SendUpdateStatus(player, comment, dungeons, true, false);
        SetState(groupGuid, LFG_STATE_QUEUED);
        SetRoles(playerGuid, roles);
        debugNames.append(player->GetName());
    }

    if (sLog->ShouldLog(LOG_FILTER_LFG, LOG_LEVEL_DEBUG))
    {
        std::ostringstream o;
        o << "LFGMgr::Join: [" << playerGuid << "] joined (" << (group ? "group" : "player") << ") Members: " << debugNames.c_str()
          << ". Dungeons (" << uint32(dungeons.size()) << "): " << ConcatenateDungeons(dungeons);
        sLog->outDebug(LOG_FILTER_LFG, "%s", o.str().c_str());
    }
}

/**
    Leaves Dungeon System. Player/Group is removed from queue, rolechecks, proposals
    or votekicks. Player or group needs to be not NULL and using Dungeon System

   @param[in]     guid Player or group guid
*/
void LFGMgr::LeaveLfg(uint64 guid)
{
    LfgState state = GetState(guid);
    uint64 gguid = IS_GROUP(guid) ? guid : GetGroup(guid);

    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::Leave: [" UI64FMTD "]", guid);
    switch (state)
    {
        case LFG_STATE_QUEUED:
            {
                LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE);

                if (gguid)
                {
                    RestoreState(gguid, "Leave queue");
                    const LfgGuidSet& players = GetPlayers(gguid);
                    for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
                    {
                        uint64 guid = (*it);
                        ClearState(guid, "Leave queue");
                        SendLfgUpdateParty(guid, updateData);
                    }
                    LfgQueue& queue = GetQueue(gguid);
                    queue.RemoveFromQueue(gguid);
                }
                else
                {
                    Player* player = ObjectAccessor::FindPlayer(guid);
                    if (player)
                        SendUpdateStatus(player, updateData.comment, updateData.dungeons, false, true);
                    ClearState(guid, "Leave queue");
                    LfgQueue& queue = GetQueue(guid);
                    queue.RemoveFromQueue(guid);
                }
            }
            break;
        case LFG_STATE_ROLECHECK:
            if (gguid)
                UpdateRoleCheck(gguid);                    // No player to update role = LFG_ROLECHECK_ABORTED
            break;
        case LFG_STATE_PROPOSAL:
        {
            // Remove from Proposals
            LfgProposalMap::iterator it = m_Proposals.begin();
            uint64 pguid = gguid == guid ? GetLeader(gguid) : guid;
            while (it != m_Proposals.end())
            {
                LfgProposalPlayerMap::iterator itPlayer = it->second.players.find(pguid);
                if (itPlayer != it->second.players.end())
                {
                    // Mark the player/leader of group who left as didn't accept the proposal
                    itPlayer->second.accept = LFG_ANSWER_DENY;
                    break;
                }
                ++it;
            }

            // Remove from queue - if proposal is found, RemoveProposal will call RemoveFromQueue
            if (it != m_Proposals.end())
                RemoveProposal(it, LFG_UPDATETYPE_PROPOSAL_DECLINED);
            break;
        }
        default:
            break;
    }
}

/**
   Update the Role check info with the player selected role.

   @param[in]     grp Group guid to update rolecheck
   @param[in]     guid Player guid (0 = rolecheck failed)
   @param[in]     roles Player selected roles
*/
void LFGMgr::UpdateRoleCheck(uint64 gguid, uint64 guid /* = 0 */, uint8 roles /* = PLAYER_ROLE_NONE */)
{
    if (!gguid)
        return;

    LfgRolesMap check_roles;
    LfgRoleCheckMap::iterator itRoleCheck = m_RoleChecks.find(gguid);
    if (itRoleCheck == m_RoleChecks.end())
        return;

    LfgRoleCheck& roleCheck = itRoleCheck->second;
    bool sendRoleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && guid;

    LfgDungeonSet dungeons;
    if (roleCheck.rDungeonId)
        dungeons.insert(roleCheck.rDungeonId);
    else
        dungeons = roleCheck.dungeons;

    if (!guid)
        roleCheck.state = LFG_ROLECHECK_ABORTED;
    else if (roles < PLAYER_ROLE_TANK)                            // Player selected no role.
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    else
    {
        roleCheck.roles[guid] = roles;

        // Check if all players have selected a role
        LfgRolesMap::const_iterator itRoles = roleCheck.roles.begin();
        while (itRoles != roleCheck.roles.end() && itRoles->second != PLAYER_ROLE_NONE)
            ++itRoles;

        if (itRoles == roleCheck.roles.end())
        {
            // use temporal var to check roles, CheckGroupRoles modifies the roles
            LFGDungeonEntry const* entry = sLFGMgr->GetLFGDungeon(*roleCheck.dungeons.begin() & 0xFFFFF);
            check_roles = roleCheck.roles;
            roleCheck.state = CheckGroupRoles(check_roles, entry ? LfgType(entry->internalType) : LFG_TYPE_DUNGEON)
                ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_WRONG_ROLES;;
        }
    }

    LfgJoinResultData joinData = LfgJoinResultData(LFG_JOIN_FAILED, roleCheck.state);
    for (LfgRolesMap::const_iterator it = roleCheck.roles.begin(); it != roleCheck.roles.end(); ++it)
    {
        uint64 pguid = it->first;

        if (!sendRoleChosen)
            SendLfgRoleChosen(pguid, guid, roles);

        SendLfgRoleCheckUpdate(pguid, roleCheck, roleCheck.state == LFG_ROLECHECK_FINISHED);
        switch (roleCheck.state)
        {
            case LFG_ROLECHECK_INITIALITING:
                continue;
            case LFG_ROLECHECK_FINISHED:
                SetState(pguid, LFG_STATE_QUEUED);
                SetRoles(pguid, it->second);
                SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE, dungeons, GetComment(pguid)));
                SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, dungeons, GetComment(pguid)));
                break;
            default:
                if (roleCheck.leader == pguid)
                    SendLfgJoinResult(pguid, joinData);
                SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_ROLECHECK_FAILED));
                ClearState(pguid, "Role check Failed");
                break;
        }
    }

    if (roleCheck.state == LFG_ROLECHECK_FINISHED)
    {
        SetState(gguid, LFG_STATE_QUEUED);
        LfgQueue& queue = GetQueue(gguid);
        queue.AddQueueData(gguid, time_t(time(NULL)), roleCheck.dungeons, roleCheck.roles);
    }
    else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
        RestoreState(gguid, "Rolecheck Failed");

    m_RoleChecks.erase(itRoleCheck);
}

/**
   Given a list of dungeons remove the dungeons players have restrictions.

   @param[in, out] dungeons Dungeons to check restrictions
   @param[in]     players Set of players to check their dungeon restrictions
   @param[out]    lockMap Map of players Lock status info of given dungeons (Empty if dungeons is not empty)
*/
void LFGMgr::GetCompatibleDungeons(LfgDungeonSet& dungeons, const LfgGuidSet& players, LfgLockPartyMap& lockMap)
{
    lockMap.clear();
    for (LfgGuidSet::const_iterator it = players.begin(); it != players.end() && !dungeons.empty(); ++it)
    {
        uint64 guid = (*it);
        LfgLockMap const& cachedLockMap = GetLockedDungeons(guid);
        for (LfgLockMap::const_iterator it2 = cachedLockMap.begin(); it2 != cachedLockMap.end() && !dungeons.empty(); ++it2)
        {
            uint32 dungeonId = (it2->first & 0xFFFFF); // Compare dungeon ids
            LfgDungeonSet::iterator itDungeon = dungeons.find(dungeonId);
            if (itDungeon != dungeons.end())
            {
                dungeons.erase(itDungeon);
                lockMap[guid][dungeonId] = it2->second;
            }
        }
    }
    if (!dungeons.empty())
        lockMap.clear();
}

/**
   Check if a group can be formed with the given group roles

   @param[in]     groles Map of roles to check
   @param[in]     removeLeaderFlag Determines if we have to remove leader flag (only used first call, Default = true)
   @return True if roles are compatible
*/
bool LFGMgr::CheckGroupRoles(LfgRolesMap& groles, LfgType type, bool removeLeaderFlag /*= true*/)
{
    if (groles.empty())
        return false;

    uint8 damage = 0;
    uint8 tank = 0;
    uint8 healer = 0;

    uint8 dpsNeeded = 0;
    uint8 healerNeeded = 0;
    uint8 tankNeeded = 0;

    switch (type)
    {
    case LFG_TYPE_DUNGEON:
        dpsNeeded = 3;
        healerNeeded = 1;
        tankNeeded = 1;
        break;
    case LFG_TYPE_RAID:
        dpsNeeded = 17;
        healerNeeded = 6;
        tankNeeded = 2;
        break;
    case LFG_TYPE_SCENARIO:
        dpsNeeded = 3;
        healerNeeded = 0;
        tankNeeded = 0;
        break;
    default:
        dpsNeeded = 3;
        healerNeeded = 1;
        tankNeeded = 1;
        break;
    }

    if (removeLeaderFlag)
        for (LfgRolesMap::iterator it = groles.begin(); it != groles.end(); ++it)
            it->second &= ~PLAYER_ROLE_LEADER;

    for (LfgRolesMap::iterator it = groles.begin(); it != groles.end(); ++it)
    {
        if (it->second == PLAYER_ROLE_NONE)
            return false;

        if (it->second & PLAYER_ROLE_TANK)
        {
            if (it->second != PLAYER_ROLE_TANK)
            {
                it->second -= PLAYER_ROLE_TANK;
                if (CheckGroupRoles(groles, type, false))
                    return true;
                it->second += PLAYER_ROLE_TANK;
            }
            else if (tank == tankNeeded)
                return false;
            else
                tank++;
        }

        if (it->second & PLAYER_ROLE_HEALER)
        {
            if (it->second != PLAYER_ROLE_HEALER)
            {
                it->second -= PLAYER_ROLE_HEALER;
                if (CheckGroupRoles(groles, type, false))
                    return true;
                it->second += PLAYER_ROLE_HEALER;
            }
            else if (healer == healerNeeded)
                return false;
            else
                healer++;
        }

        if (it->second & PLAYER_ROLE_DAMAGE)
        {
            if (it->second != PLAYER_ROLE_DAMAGE)
            {
                it->second -= PLAYER_ROLE_DAMAGE;
                if (CheckGroupRoles(groles, type, false))
                    return true;
                it->second += PLAYER_ROLE_DAMAGE;
            }
            else if (damage == dpsNeeded)
                return false;
            else
                damage++;
        }
    }
    return (tank + healer + damage) == uint8(groles.size());
}

/**
   Makes a new group given a proposal
   @param[in]     proposal Proposal to get info from
*/
void LFGMgr::MakeNewGroup(const LfgProposal& proposal)
{
    LfgGuidList players;
    LfgGuidList playersToTeleport;

    for (LfgProposalPlayerMap::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        uint64 guid = it->first;
        if (guid == proposal.leader)
            players.push_front(guid);
        else
            players.push_back(guid);

        if (proposal.isNew || GetGroup(guid) != proposal.group)
            playersToTeleport.push_back(guid);
    }

    // Set the dungeon difficulty
    LFGDungeonEntry const* dungeon = GetLFGDungeon(proposal.dungeonId);
    ASSERT(dungeon);

    Group* grp = proposal.group ? sGroupMgr->GetGroupByGUID(GUID_LOPART(proposal.group)) : NULL;
    for (LfgGuidList::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        uint64 pguid = (*it);
        Player* player = ObjectAccessor::FindPlayer(pguid);
        if (!player)
            continue;

        Group* group = player->GetGroup();
        if (group && group != grp)
            player->RemoveFromGroup();

        if (!grp)
        {
            grp = new Group();
            grp->ConvertToLFG(dungeon);
            grp->Create(player);
            uint64 gguid = grp->GetGUID();
            SetState(gguid, LFG_STATE_PROPOSAL);
            sGroupMgr->AddGroup(grp);
        }
        else if (group != grp)
            grp->AddMember(player);

        grp->SetLfgRoles(pguid, proposal.players.find(pguid)->second.role);

        // Add the cooldown spell if queued for a random dungeon
        if (dungeon->type == LFG_TYPE_RANDOM)
            player->CastSpell(player, LFG_SPELL_DUNGEON_COOLDOWN, false);
    }

    grp->SetDungeonDifficulty(Difficulty(dungeon->difficulty));
    uint64 gguid = grp->GetGUID();
    SetDungeon(gguid, dungeon->Entry());
    SetState(gguid, LFG_STATE_DUNGEON);

    _SaveToDB(gguid, grp->GetDbStoreId());

    // Teleport Player
    for (LfgGuidList::const_iterator it = playersToTeleport.begin(); it != playersToTeleport.end(); ++it)
        if (Player* player = ObjectAccessor::FindPlayer(*it))
            TeleportPlayer(player, false);

    // Update group info
    grp->SendUpdate();
}

uint32 LFGMgr::AddProposal(LfgProposal const& proposal)
{
    m_Proposals[++m_lfgProposalId] = proposal;
    return m_lfgProposalId;
}

/**
   Update Proposal info with player answer

   @param[in]     proposalId Proposal id to be updated
   @param[in]     guid Player guid to update answer
   @param[in]     accept Player answer
*/
void LFGMgr::UpdateProposal(uint32 proposalId, uint64 guid, bool accept)
{
    // Check if the proposal exists
    LfgProposalMap::iterator itProposal = m_Proposals.find(proposalId);
    if (itProposal == m_Proposals.end())
        return;

    LfgProposal& proposal = itProposal->second;

    // Check if proposal have the current player
    LfgProposalPlayerMap::iterator itProposalPlayer = proposal.players.find(guid);
    if (itProposalPlayer == proposal.players.end())
        return;

    LfgProposalPlayer& player = itProposalPlayer->second;
    player.accept = LfgAnswer(accept);
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::UpdateProposal: Player [" UI64FMTD "] of proposal %u selected: %u", guid, proposalId, accept);
    if (!accept)
    {
        RemoveProposal(itProposal, LFG_UPDATETYPE_PROPOSAL_DECLINED);
        return;
    }

    // check if all have answered and reorder players (leader first)
    bool allAnswered = true;
    for (LfgProposalPlayerMap::const_iterator itPlayers = proposal.players.begin(); itPlayers != proposal.players.end(); ++itPlayers)
        if (itPlayers->second.accept != LFG_ANSWER_AGREE)   // No answer (-1) or not accepted (0)
            allAnswered = false;

    if (!allAnswered)
    {
        for (LfgProposalPlayerMap::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
        {
            uint64 guid = it->first;
            SendLfgUpdateProposal(guid, proposalId, proposal);
        }
        return;
    }

    bool sendUpdate = proposal.state != LFG_PROPOSAL_SUCCESS;
    proposal.state = LFG_PROPOSAL_SUCCESS;
    time_t joinTime = time_t(time(NULL));

    LfgQueue& queue = GetQueue(guid);
    LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_GROUP_FOUND);
    for (LfgProposalPlayerMap::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        uint64 pguid = it->first;
        uint64 gguid = it->second.group;
        uint32 dungeonId = (*GetSelectedDungeons(pguid).begin());
        int32 waitTime = -1;
        if (sendUpdate)
           SendLfgUpdateProposal(pguid, proposalId, proposal);

        if (gguid)
        {
            waitTime = int32((joinTime - queue.GetJoinTime(gguid)) / IN_MILLISECONDS);
            SendLfgUpdateParty(pguid, updateData);
        }
        else
        {
            waitTime = int32((joinTime - queue.GetJoinTime(pguid)) / IN_MILLISECONDS);
            SendLfgUpdatePlayer(pguid, updateData);
        }

        updateData.updateType = LFG_UPDATETYPE_REMOVED_FROM_QUEUE;
        SendLfgUpdatePlayer(pguid, updateData);
        SendLfgUpdateParty(pguid, updateData);

        // Update timers
        uint8 role = GetRoles(pguid);
        role &= ~PLAYER_ROLE_LEADER;
        switch (role)
        {
            case PLAYER_ROLE_DAMAGE:
                queue.UpdateWaitTimeDps(waitTime, dungeonId);
                break;
            case PLAYER_ROLE_HEALER:
                queue.UpdateWaitTimeHealer(waitTime, dungeonId);
                break;
            case PLAYER_ROLE_TANK:
                queue.UpdateWaitTimeTank(waitTime, dungeonId);
                break;
            default:
                queue.UpdateWaitTimeAvg(waitTime, dungeonId);
                break;
        }

        m_teleport.push_back(pguid);
        SetState(pguid, LFG_STATE_DUNGEON);
    }

    // Remove players/groups from Queue
    for (LfgGuidList::const_iterator it = proposal.queues.begin(); it != proposal.queues.end(); ++it)
        queue.RemoveFromQueue(*it);

    MakeNewGroup(proposal);
    m_Proposals.erase(itProposal);
}

/**
   Remove a proposal from the pool, remove the group that didn't accept (if needed) and readd the other members to the queue

   @param[in]     itProposal Iterator to the proposal to remove
   @param[in]     type Type of removal (LFG_UPDATETYPE_PROPOSAL_FAILED, LFG_UPDATETYPE_PROPOSAL_DECLINED)
*/
void LFGMgr::RemoveProposal(LfgProposalMap::iterator itProposal, LfgUpdateType type)
{
    LfgProposal& proposal = itProposal->second;
    proposal.state = LFG_PROPOSAL_FAILED;

    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RemoveProposal: Proposal %u, state FAILED, UpdateType %u", itProposal->first, type);
    // Mark all people that didn't answered as no accept
    if (type == LFG_UPDATETYPE_PROPOSAL_FAILED)
        for (LfgProposalPlayerMap::iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
            if (it->second.accept == LFG_ANSWER_PENDING)
                it->second.accept = LFG_ANSWER_DENY;

    // Mark players/groups to be removed
    LfgGuidSet toRemove;
    for (LfgProposalPlayerMap::iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        if (it->second.accept == LFG_ANSWER_AGREE)
            continue;

        uint64 guid = it->second.group ? it->second.group : it->first;
        // Player didn't accept or still pending when no secs left
        if (it->second.accept == LFG_ANSWER_DENY || type == LFG_UPDATETYPE_PROPOSAL_FAILED)
        {
            it->second.accept = LFG_ANSWER_DENY;
            toRemove.insert(guid);
        }
    }

    // Notify players
    for (LfgProposalPlayerMap::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        uint64 guid = it->first;
        uint64 gguid = it->second.group ? it->second.group : guid;
        SendLfgUpdateProposal(guid, itProposal->first, proposal);

        if (toRemove.find(gguid) != toRemove.end())         // Didn't accept or in same group that someone that didn't accept
        {
            LfgUpdateData updateData;
            if (it->second.accept == LFG_ANSWER_DENY)
            {
                updateData.updateType = type;
                sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RemoveProposal: [" UI64FMTD "] didn't accept. Removing from queue and compatible cache", guid);
            }
            else
            {
                updateData.updateType = LFG_UPDATETYPE_REMOVED_FROM_QUEUE;
                sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RemoveProposal: [" UI64FMTD "] in same group that someone that didn't accept. Removing from queue and compatible cache", guid);
            }
            ClearState(guid, "Proposal Fail (didn't accepted or in group with someone that didn't accept");
            if (gguid != guid)
            {
                RestoreState(gguid, "Proposal Fail (someone in group didn't accepted)");
                SendLfgUpdateParty(guid, updateData);
            }
            else
                SendLfgUpdatePlayer(guid, updateData);
        }
        else
        {
            sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RemoveProposal: Readding [" UI64FMTD "] to queue.", guid);
            SetState(guid, LFG_STATE_QUEUED);
            if (gguid != guid)
            {
                SetState(gguid, LFG_STATE_QUEUED);
                SendLfgUpdateParty(guid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, GetSelectedDungeons(guid), GetComment(guid)));
            }
            else
                SendLfgUpdatePlayer(guid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, GetSelectedDungeons(guid), GetComment(guid)));
        }
    }

    LfgQueue& queue = GetQueue(proposal.players.begin()->first);
    // Remove players/groups from queue
    for (LfgGuidSet::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it)
    {
        uint64 guid = *it;
        queue.RemoveFromQueue(guid);
        proposal.queues.remove(guid);
    }

    // Readd to queue
    for (LfgGuidList::const_iterator it = proposal.queues.begin(); it != proposal.queues.end(); ++it)
    {
        uint64 guid = *it;
        queue.AddToQueue(guid);
    }

    m_Proposals.erase(itProposal);
}

/**
   Initialize a boot kick vote

   @param[in]     gguid Group the vote kicks belongs to
   @param[in]     kicker Kicker guid
   @param[in]     victim Victim guid
   @param[in]     reason Kick reason
*/
void LFGMgr::InitBoot(uint64 gguid, uint64 kicker, uint64 victim, std::string const& reason)
{
    SetState(gguid, LFG_STATE_BOOT);

    LfgPlayerBoot& boot = m_Boots[gguid];
    boot.inProgress = true;
    boot.cancelTime = time_t(time(NULL)) + LFG_TIME_BOOT;
    boot.reason = reason;
    boot.victim = victim;

    LfgGuidSet const& players = GetPlayers(gguid);

    // Set votes
    for (LfgGuidSet::const_iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        uint64 guid = (*itr);
        SetState(guid, LFG_STATE_BOOT);
        boot.votes[guid] = LFG_ANSWER_PENDING;
    }

    boot.votes[victim] = LFG_ANSWER_DENY;                  // Victim auto vote NO
    boot.votes[kicker] = LFG_ANSWER_AGREE;                 // Kicker auto vote YES

    // Notify players
    for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
        SendLfgBootProposalUpdate(*it, boot);
}

/**
   Update Boot info with player answer

   @param[in]     guid Player who has answered
   @param[in]     accept player answer
*/
void LFGMgr::UpdateBoot(uint64 guid, bool accept)
{
    uint64 gguid = GetGroup(guid);
    if (!gguid)
        return;

    LfgPlayerBootMap::iterator itBoot = m_Boots.find(gguid);
    if (itBoot == m_Boots.end())
        return;

    LfgPlayerBoot& boot = itBoot->second;

    if (boot.votes[guid] != LFG_ANSWER_PENDING)             // Cheat check: Player can't vote twice
        return;

    boot.votes[guid] = LfgAnswer(accept);

    uint8 votesNum = 0;
    uint8 agreeNum = 0;
    for (LfgAnswerMap::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
    {
        if (itVotes->second != LFG_ANSWER_PENDING)
        {
            ++votesNum;
            if (itVotes->second == LFG_ANSWER_AGREE)
                ++agreeNum;
        }
    }

    // if we don't have enough votes (agree or deny) do nothing
    if (agreeNum < LFG_GROUP_KICK_VOTES_NEEDED && (votesNum - agreeNum) < LFG_GROUP_KICK_VOTES_NEEDED)
        return;

    // Send update info to all players
    boot.inProgress = false;
    for (LfgAnswerMap::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
    {
        uint64 pguid = itVotes->first;
        if (pguid != boot.victim)
        {
            SetState(pguid, LFG_STATE_DUNGEON);
            SendLfgBootProposalUpdate(pguid, boot);
        }
    }

    SetState(gguid, LFG_STATE_DUNGEON);
    if (agreeNum == LFG_GROUP_KICK_VOTES_NEEDED)           // Vote passed - Kick player
    {
        if (Group* group = sGroupMgr->GetGroupByGUID(GUID_LOPART(gguid)))
            Player::RemoveFromGroup(group, boot.victim, GROUP_REMOVEMETHOD_KICK_LFG);
        DecreaseKicksLeft(gguid);
    }
    m_Boots.erase(itBoot);
}

/**
   Teleports the player in or out the dungeon

   @param[in]     player Player to teleport
   @param[in]     out Teleport out (true) or in (false)
   @param[in]     fromOpcode Function called from opcode handlers? (Default false)
*/
void LFGMgr::TeleportPlayer(Player* player, bool out, bool fromOpcode /*= false*/)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::TeleportPlayer: [" UI64FMTD "] is being teleported %s", player->GetGUID(), out ? "out" : "in");

    Group* grp = player->GetGroup();
    uint64 gguid = grp->GetGUID();
    LFGDungeonEntry const* dungeon = GetLFGDungeon(GetDungeon(gguid));
    if (!dungeon || (out && player->GetMapId() != uint32(dungeon->map)))
        return;

    if (out)
    {
        player->RemoveAurasDueToSpell(LFG_SPELL_LUCK_OF_THE_DRAW);
        player->TeleportToBGEntryPoint();
        return;
    }

    LfgTeleportError error = LFG_TELEPORTERROR_OK;

    if (!grp || !grp->isLFGGroup())                        // should never happen, but just in case...
        error = LFG_TELEPORTERROR_INVALID_TELEPORT_LOCATION;
    else if (!player->isAlive())
        error = LFG_TELEPORTERROR_PLAYER_DEAD;
    else if (player->IsFalling() || player->HasUnitState(UNIT_STATE_JUMPING))
        error = LFG_TELEPORTERROR_NOT_WHILE_FALLING;
    else if (player->IsMirrorTimerActive(FATIGUE_TIMER))
        error = LFG_TELEPORTERROR_NOT_WHILE_FATIGUED;
    /*else if (player->GetVehicle())
        error = LFG_TELEPORTERROR_IN_VEHICLE;
    else if (player->GetCharmGUID())
        error = LFG_TELEPORTERROR_CHARMING;*/
    else
    {
        if (!dungeon)
            error = LFG_TELEPORTERROR_INVALID_TELEPORT_LOCATION;
        else if (player->GetMapId() != uint32(dungeon->map))  // Do not teleport players in dungeon to the entrance
        {
            uint32 mapid = dungeon->map;
            float x = dungeon->x;
            float y = dungeon->y;
            float z = dungeon->z;
            float orientation = dungeon->o;

            if (!fromOpcode)
            {
                // Select a player inside to be teleported to
                for (GroupReference* itr = grp->GetFirstMember(); itr != NULL && !mapid; itr = itr->next())
                {
                    Player* plrg = itr->getSource();
                    if (plrg && plrg != player && plrg->GetMapId() == uint32(dungeon->map))
                    {
                        mapid = plrg->GetMapId();
                        x = plrg->GetPositionX();
                        y = plrg->GetPositionY();
                        z = plrg->GetPositionZ();
                        orientation = plrg->GetOrientation();
                    }
                }
            }

            if (error == LFG_TELEPORTERROR_OK)
            {
                if (!player->GetMap()->IsDungeon())
                    player->SetBattlegroundEntryPoint();

                if (player->isInFlight())
                {
                    player->GetMotionMaster()->MovementExpired();
                    player->CleanupAfterTaxiFlight();
                }

                if (!player->TeleportTo(mapid, x, y, z, orientation))
                {
                    error = LFG_TELEPORTERROR_INVALID_TELEPORT_LOCATION;
                    sLog->outError(LOG_FILTER_LFG, "LfgMgr::TeleportPlayer: Failed to teleport [" UI64FMTD "] to map %u: ", player->GetGUID(), mapid);
                }
            }
        }
    }

    //if (error != LFG_TELEPORTERROR_OK)
        player->GetSession()->SendLfgTeleportError(uint8(error));
}

void LFGMgr::SendUpdateStatus(Player* player, const std::string& comment, const LfgDungeonSet& selectedDungeons, bool pause, bool quit)
{
    ObjectGuid guid = player->GetGUID();

    LfgQueue& queue = GetQueue(player->GetGroup() ? player->GetGroup()->GetGUID() : guid);

    LFGDungeonEntry const* dungeonEntry = NULL;
    if (!selectedDungeons.empty())
        dungeonEntry = sLFGMgr->GetLFGDungeon(*selectedDungeons.begin() & 0xFFFFF);

    WorldPacket data(SMSG_LFG_UPDATE_STATUS, 60);
    data.WriteBits(comment.size(), 8);              // comment
    data.WriteBits(0, 24);                          // guids size
    data.WriteGuidMask<5>(guid);

    data.WriteBit(player->GetGroup() != NULL);      // in group
    data.WriteBit(1);
    data.WriteBit(!quit);                           // display or not the lfr button, lfg join ?, 0 for last one

    data.WriteGuidMask<3, 7, 1>(guid);
    data.WriteBit(1);                               // 1 - show notification
    data.WriteGuidMask<0>(guid);
    data.WriteBits(selectedDungeons.size(), 22);
    data.WriteGuidMask<6, 4>(guid);
    data.WriteBit(1);                               // 1 - active, 0 - paused
    data.WriteGuidMask<2>(guid);

    data.WriteString(comment);
    data << uint8(dungeonEntry ? dungeonEntry->dbc->subType : LFG_SUBTYPE_DUNGEON);  // 1 - dungeon finder, 2 - raid finder, 3 - scenarios, 4 - flex
    data << uint8(11);                              // error?   1, 11, 17 - succed, other - failed
    data.WriteGuidBytes<4>(guid);
    data << uint32(8);                              // unk
    data.WriteGuidBytes<5, 7>(guid);
    data << uint32(3);                              // queue id. 4 - looking for raid, 3 - others
    data.WriteGuidBytes<3>(guid);
    for (LfgDungeonSet::const_iterator i = selectedDungeons.begin(); i != selectedDungeons.end(); ++i)
        data << uint32(*i);                         // Dungeon entries
    data.WriteGuidBytes<0>(guid);
    data << uint32(player->GetTeam());              // group id?
    data.WriteGuidBytes<1>(guid);
    data << uint32(queue.GetJoinTime(guid));
    data.WriteGuidBytes<6>(guid);
    for (int i = 0; i < 3; ++i)
        data << uint8(0);                           //unk8 always 0 ?
    data.WriteGuidBytes<2>(guid);

    player->GetSession()->SendPacket(&data);
}

/**
   Give completion reward to player

   @param[in]     dungeonId Id of the dungeon finished
   @param[in]     player Player to reward
*/
void LFGMgr::RewardDungeonDoneFor(const uint32 dungeonId, Player* player)
{
    Group* group = player->GetGroup();
    if (!group || !group->isLFGGroup())
    {
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RewardDungeonDoneFor: [" UI64FMTD "] is not in a group or not a LFGGroup. Ignoring", player->GetGUID());
        return;
    }

    uint64 guid = player->GetGUID();
    uint64 gguid = player->GetGroup()->GetGUID();
    uint32 gDungeonId = GetDungeon(gguid);
    if (gDungeonId != dungeonId)
    {
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RewardDungeonDoneFor: [" UI64FMTD "] Finished dungeon %u but group queued for %u. Ignoring", guid, dungeonId, gDungeonId);
        return;
    }

    if (GetState(guid) == LFG_STATE_FINISHED_DUNGEON)
    {
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RewardDungeonDoneFor: [" UI64FMTD "] Already rewarded player. Ignoring", guid);
        return;
    }

    // Mark dungeon as finished
    SetState(gguid, LFG_STATE_FINISHED_DUNGEON);

    // Clear player related lfg stuff
    uint32 rDungeonId = (*GetSelectedDungeons(guid).begin());
    ClearState(guid, "Dungeon Finished");
    SetState(guid, LFG_STATE_FINISHED_DUNGEON);

    // Give rewards only if its a random or seasonal  dungeon
    LFGDungeonEntry const* dungeon = GetLFGDungeon(rDungeonId);
    if (!dungeon || (dungeon->type != LFG_TYPE_RANDOM && !dungeon->seasonal))
    {
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RewardDungeonDoneFor: [" UI64FMTD "] dungeon %u is not random nor seasonal", guid, rDungeonId);
        return;
    }

    // Update achievements
    if (dungeon->difficulty == HEROIC_DIFFICULTY)
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS, 1);

    LfgReward const* reward = GetRandomDungeonReward(rDungeonId, player->getLevel());
    if (!reward)
        return;

    uint8 index = 0;
    Quest const* qReward = sObjectMgr->GetQuestTemplate(reward->reward[index].questId);
    if (!qReward)
        return;

    // if we can take the quest, means that we haven't done this kind of "run", IE: First Heroic Random of Day.
    if (player->CanRewardQuest(qReward, false))
        player->RewardQuest(qReward, 0, NULL, false);
    else
    {
        index = 1;
        qReward = sObjectMgr->GetQuestTemplate(reward->reward[index].questId);
        if (!qReward)
            return;
        // we give reward without informing client (retail does this)
        player->RewardQuest(qReward, 0, NULL, false);
    }

    // Give rewards
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RewardDungeonDoneFor: [" UI64FMTD "] done dungeon %u, %s previously done.", player->GetGUID(), GetDungeon(gguid), index > 0 ? " " : " not");
    player->GetSession()->SendLfgPlayerReward(dungeon->Entry(), GetDungeon(gguid, false), index, reward, qReward);
}

// --------------------------------------------------------------------------//
// Auxiliar Functions
// --------------------------------------------------------------------------//

/**
   Get the dungeon list that can be done given a random dungeon entry.

   @param[in]     randomdungeon Random dungeon id (if value = 0 will return all dungeons)
   @returns Set of dungeons that can be done.
*/
const LfgDungeonSet& LFGMgr::GetDungeonsByRandom(uint32 randomdungeon)
{
    return m_CachedDungeonMap[randomdungeon];
}

/**
   Get the reward of a given random dungeon at a certain level

   @param[in]     dungeon dungeon id
   @param[in]     level Player level
   @returns Reward
*/
LfgReward const* LFGMgr::GetRandomDungeonReward(uint32 dungeon, uint8 level)
{
    LfgReward const* rew = NULL;
    LfgRewardMapBounds bounds = m_RewardMap.equal_range(dungeon & 0xFFFFF);
    for (LfgRewardMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        rew = itr->second;
        // ordered properly at loading
        if (itr->second->maxLevel >= level)
            break;
    }

    return rew;
}

/**
   Given a Dungeon id returns the dungeon Type

   @param[in]     dungeon dungeon id
   @returns Dungeon type
*/
LfgType LFGMgr::GetDungeonType(uint32 dungeonId)
{
    LFGDungeonEntry const* dungeon = GetLFGDungeon(dungeonId);
    if (!dungeon)
        return LFG_TYPE_NONE;

    return LfgType(dungeon->type);
}

LfgState LFGMgr::GetState(uint64 guid)
{
    LfgState state;
    if (IS_GROUP(guid))
        state = m_Groups[guid].GetState();
    else
        state = m_Players[guid].GetState();

    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetState: [" UI64FMTD "] = %u", guid, state);
    return state;
}

uint32 LFGMgr::GetDungeon(uint64 guid, bool asId /*= true */)
{
    uint32 dungeon = m_Groups[guid].GetDungeon(asId);
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetDungeon: [" UI64FMTD "] asId: %u = %u", guid, asId, dungeon);
    return dungeon;
}

uint8 LFGMgr::GetRoles(uint64 guid)
{
    uint8 roles = m_Players[guid].GetRoles();
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetRoles: [" UI64FMTD "] = %u", guid, roles);
    return roles;
}

const std::string& LFGMgr::GetComment(uint64 guid)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetComment: [" UI64FMTD "] = %s", guid, m_Players[guid].GetComment().c_str());
    return m_Players[guid].GetComment();
}

bool LFGMgr::IsTeleported(uint64 pguid)
{
    if (std::find(m_teleport.begin(), m_teleport.end(), pguid) != m_teleport.end())
    {
        m_teleport.remove(pguid);   // wtf hack
        return true;
    }
    return false;
}

const LfgDungeonSet& LFGMgr::GetSelectedDungeons(uint64 guid)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetSelectedDungeons: [" UI64FMTD "]", guid);
    return m_Players[guid].GetSelectedDungeons();
}

const LfgLockMap& LFGMgr::GetLockedDungeons(uint64 guid)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetLockedDungeons: [" UI64FMTD "]", guid);
    return m_Players[guid].GetLockedDungeons();
}

uint8 LFGMgr::GetKicksLeft(uint64 guid)
{
    uint8 kicks = m_Groups[guid].GetKicksLeft();
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::GetKicksLeft: [" UI64FMTD "] = %u", guid, kicks);
    return kicks;
}

void LFGMgr::RestoreState(uint64 guid, char const *debugMsg)
{
    LfgGroupData& data = m_Groups[guid];
    char const * const ps = GetStateString(data.GetState());
    char const * const os = GetStateString(data.GetOldState());
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RestoreState: Group: [" UI64FMTD "] (%s), State: %s, oldState: %s", guid, debugMsg, ps, os);
    data.RestoreState();
}

void LFGMgr::ClearState(uint64 guid, char const *debugMsg)
{
    LfgPlayerData& data = m_Players[guid];
    char const * const ps = GetStateString(data.GetState());
    char const * const os = GetStateString(data.GetOldState());
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::ClearState: Player: [" UI64FMTD "] (%s) State: %s, oldState: %s", guid, debugMsg, ps, os);
    data.ClearState();
}

void LFGMgr::SetState(uint64 guid, LfgState state)
{
    if (IS_GROUP(guid))
    {
        LfgGroupData& data = m_Groups[guid];
        char const * const ns = GetStateString(state);
        char const * const ps = GetStateString(data.GetState());
        char const * const os = GetStateString(data.GetOldState());
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetState: Group: [" UI64FMTD "] newState: %s, previous: %s, oldState: %s", guid, ns, ps, os);
        data.SetState(state);
    }
    else
    {
        LfgPlayerData& data = m_Players[guid];
        char const * const ns = GetStateString(state);
        char const * const ps = GetStateString(data.GetState());
        char const * const os = GetStateString(data.GetOldState());
        sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetState: Player: [" UI64FMTD "] newState: %s, previous: %s, oldState: %s", guid, ns, ps, os);
        data.SetState(state);
    }
}

void LFGMgr::SetDungeon(uint64 guid, uint32 dungeon)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetDungeon: [" UI64FMTD "] dungeon %u", guid, dungeon);
    m_Groups[guid].SetDungeon(dungeon);
}

void LFGMgr::SetRoles(uint64 guid, uint8 roles)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetRoles: [" UI64FMTD "] roles: %u", guid, roles);
    m_Players[guid].SetRoles(roles);
}

void LFGMgr::SetComment(uint64 guid, const std::string& comment)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetComment: [" UI64FMTD "] comment: %s", guid, comment.c_str());
    m_Players[guid].SetComment(comment);
}

void LFGMgr::SetSelectedDungeons(uint64 guid, const LfgDungeonSet& dungeons)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetSelectedDungeons: [" UI64FMTD "]", guid);
    m_Players[guid].SetSelectedDungeons(dungeons);
}

void LFGMgr::SetLockedDungeons(uint64 guid, const LfgLockMap& lock)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::SetLockedDungeons: [" UI64FMTD "]", guid);
    m_Players[guid].SetLockedDungeons(lock);
}

void LFGMgr::DecreaseKicksLeft(uint64 guid)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::DecreaseKicksLeft: [" UI64FMTD "]", guid);
    m_Groups[guid].DecreaseKicksLeft();
}

void LFGMgr::RemovePlayerData(uint64 guid)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RemovePlayerData: [" UI64FMTD "]", guid);
    LfgPlayerDataMap::iterator it = m_Players.find(guid);
    if (it != m_Players.end())
        m_Players.erase(it);
}

void LFGMgr::RemoveGroupData(uint64 guid)
{
    sLog->outDebug(LOG_FILTER_LFG, "LFGMgr::RemoveGroupData: [" UI64FMTD "]", guid);
    LfgGroupDataMap::iterator it = m_Groups.find(guid);
    if (it != m_Groups.end())
        m_Groups.erase(it);
}

uint8 LFGMgr::GetTeam(uint64 guid)
{
    return m_Players[guid].GetTeam();
}

uint8 LFGMgr::RemovePlayerFromGroup(uint64 gguid, uint64 guid)
{
    return m_Groups[gguid].RemovePlayer(guid);
}

void LFGMgr::AddPlayerToGroup(uint64 gguid, uint64 guid)
{
    m_Groups[gguid].AddPlayer(guid);
}

void LFGMgr::SetLeader(uint64 gguid, uint64 leader)
{
    m_Groups[gguid].SetLeader(leader);
}

void LFGMgr::SetTeam(uint64 guid, uint8 team)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP))
        team = 0;

    m_Players[guid].SetTeam(team);
}

uint64 LFGMgr::GetGroup(uint64 guid)
{
    return m_Players[guid].GetGroup();
}

void LFGMgr::SetGroup(uint64 guid, uint64 group)
{
    m_Players[guid].SetGroup(group);
}

const LfgGuidSet& LFGMgr::GetPlayers(uint64 guid)
{
    return m_Groups[guid].GetPlayers();
}

uint64 LFGMgr::GetLeader(uint64 guid)
{
    return m_Groups[guid].GetLeader();
}

bool LFGMgr::HasIgnore(uint64 guid1, uint64 guid2)
{
    Player* plr1 = ObjectAccessor::FindPlayer(guid1);
    Player* plr2 = ObjectAccessor::FindPlayer(guid2);
    uint32 low1 = GUID_LOPART(guid1);
    uint32 low2 = GUID_LOPART(guid2);
    return plr1 && plr2 && (plr1->GetSocial()->HasIgnore(low2) || plr2->GetSocial()->HasIgnore(low1));
}

void LFGMgr::SendLfgRoleChosen(uint64 guid, uint64 pguid, uint8 roles)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgRoleChosen(pguid, roles);
}

void LFGMgr::SendLfgRoleCheckUpdate(uint64 guid, const LfgRoleCheck& roleCheck, bool updateAll)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgRoleCheckUpdate(roleCheck, updateAll);
}

void LFGMgr::SendLfgUpdatePlayer(uint64 guid, const LfgUpdateData& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgUpdatePlayer(data);
}

void LFGMgr::SendLfgUpdateParty(uint64 guid, const LfgUpdateData& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgUpdateParty(data);
}

void LFGMgr::SendLfgJoinResult(uint64 guid, const LfgJoinResultData& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgJoinResult(data);
}

void LFGMgr::SendLfgBootProposalUpdate(uint64 guid, const LfgPlayerBoot& boot)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgBootProposalUpdate(boot);
}

void LFGMgr::SendLfgUpdateProposal(uint64 guid, uint32 proposalId, const LfgProposal& proposal)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgUpdateProposal(proposalId, proposal);
}

void LFGMgr::SendLfgQueueStatus(uint64 guid, const LfgQueueStatusData& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgQueueStatus(data);
}

bool LFGMgr::IsLfgGroup(uint64 guid)
{
    return guid && IS_GROUP(guid) && m_Groups[guid].IsLfgGroup();
}

LfgQueue& LFGMgr::GetQueue(uint64 guid)
{
    uint8 queueId = 0;
    if (IS_GROUP(guid))
    {
        const LfgGuidSet& players = GetPlayers(guid);
        uint64 pguid = players.empty() ? 0 : (*players.begin());
        if (pguid)
            queueId = GetTeam(pguid);
    }
    else
        queueId = GetTeam(guid);
    return m_Queues[queueId];
}

bool LFGMgr::AllQueued(const LfgGuidList& check)
{
    if (check.empty())
        return false;

    for (LfgGuidList::const_iterator it = check.begin(); it != check.end(); ++it)
        if (GetState(*it) != LFG_STATE_QUEUED)
            return false;
    return true;
}

bool LFGMgr::IsSeasonActive(uint32 dungeonId)
{
    switch (dungeonId)
    {
        case 285: // The Headless Horseman
            return IsHolidayActive(HOLIDAY_HALLOWS_END);
        case 286: // The Frost Lord Ahune
            return IsHolidayActive(HOLIDAY_FIRE_FESTIVAL);
        case 287: // Coren Direbrew
            return IsHolidayActive(HOLIDAY_BREWFEST);
        case 288: // The Crown Chemical Co.
            return IsHolidayActive(HOLIDAY_LOVE_IS_IN_THE_AIR);
        // pre-cata event dungeons
        case 296:   // Grand Ambassador Flamelash
        case 297:   // Crown Princess Theradras
        case 298:   // Kai'ju Gahz'rilla
        case 299:   // Prince Sarsarun
        case 306:   // Kai'ju Gahz'rilla
        case 308:   // Grand Ambassador Flamelash
        case 309:   // Crown Princess Theradras
        case 310:   // Prince Sarsarun
            return false;
        default:
            break;
    }
    return false;
}

