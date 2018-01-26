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

namespace lfg
{

LFGMgr::LFGMgr(): m_QueueTimer(0), m_lfgProposalId(1),
    m_options(sWorld->getIntConfig(CONFIG_LFG_OPTIONSMASK))
{
    new LFGPlayerScript();
    new LFGGroupScript();
}

LFGMgr::~LFGMgr()
{
    for (LfgRewardContainer::iterator itr = RewardMapStore.begin(); itr != RewardMapStore.end(); ++itr)
        delete itr->second;
}

void LFGMgr::_LoadFromDB(Field* fields, uint64 guid)
{
    if (!fields)
        return;

    if (!IS_GROUP(guid))
        return;

    SetLeader(guid, MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));

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

/// Load rewards for completing dungeons
void LFGMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    for (LfgRewardContainer::iterator itr = RewardMapStore.begin(); itr != RewardMapStore.end(); ++itr)
        delete itr->second;
    RewardMapStore.clear();

    // ORDER BY is very important for GetDungeonReward!
    QueryResult result = WorldDatabase.Query("SELECT dungeonId, maxLevel, firstQuestId, otherQuestId, bonusQuestId FROM lfg_dungeon_rewards ORDER BY dungeonId, maxLevel ASC");

    if (!result)
    {
        TC_LOG_ERROR("server", ">> Loaded 0 lfg dungeon rewards. DB table `lfg_dungeon_rewards` is empty!");
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
        uint32 otherQuestId = fields[3].GetUInt32();
        uint32 bonusQuestId = fields[4].GetUInt32();

        if (!GetLFGDungeonEntry(dungeonId))
        {
            TC_LOG_ERROR("sql", "Dungeon %u specified in table `lfg_dungeon_rewards` does not exist!", dungeonId);
            continue;
        }

        if (!maxLevel || maxLevel > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            TC_LOG_ERROR("sql", "Level %u specified for dungeon %u in table `lfg_dungeon_rewards` can never be reached!", maxLevel, dungeonId);
            maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
        }

        if (!firstQuestId || !sObjectMgr->GetQuestTemplate(firstQuestId))
        {
            TC_LOG_ERROR("sql", "First quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", firstQuestId, dungeonId);
            continue;
        }

        if (otherQuestId && !sObjectMgr->GetQuestTemplate(otherQuestId))
        {
            TC_LOG_ERROR("sql", "Other quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", otherQuestId, dungeonId);
            otherQuestId = 0;
        }

        if (bonusQuestId && !sObjectMgr->GetQuestTemplate(bonusQuestId))
        {
            TC_LOG_ERROR("sql", "Bonus quest %u specified for dungeon %u in table `lfg_dungeon_rewards` does not exist!", bonusQuestId, dungeonId);
            bonusQuestId = 0;
        }

        RewardMapStore.insert(LfgRewardContainer::value_type(dungeonId, new LfgReward(maxLevel, firstQuestId, otherQuestId)));
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server", ">> Loaded %u lfg dungeon rewards in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

LFGDungeonData const* LFGMgr::GetLFGDungeon(uint32 id)
{
    LFGDungeonContainer::const_iterator itr = LfgDungeonStore.find(id);
    if (itr != LfgDungeonStore.end())
        return &(itr->second);

    return NULL;
}

LFGDungeonData const* LFGMgr::GetLFGDungeon(uint32 id, uint32 team)
{
    LFGDungeonContainer::const_iterator itr = LfgDungeonStore.find(id);
    if (itr != LfgDungeonStore.end())
    {
        LFGDungeonEntry const* dungeonEntry = itr->second.dbc;

        if (dungeonEntry && dungeonEntry->FitsTeam(team))
            return &itr->second;
    }

    return NULL;
}

LFGDungeonData const* LFGMgr::GetLFGDungeon(uint32 mapId, Difficulty diff, uint32 team)
{
    for (LFGDungeonContainer::const_iterator itr = LfgDungeonStore.begin(); itr != LfgDungeonStore.end(); ++itr)
    {
        LFGDungeonEntry const* dungeonEntry = itr->second.dbc;

        if (dungeonEntry->difficulty == uint8(diff) && dungeonEntry->map == mapId &&
            dungeonEntry->FitsTeam(team))
            return &itr->second;
    }

    return NULL;
}

void LFGMgr::LoadLFGDungeons(bool reload /* = false */)
{
    uint32 oldMSTime = getMSTime();

    LfgDungeonStore.clear();

    // Initialize Dungeon map with data from dbcs
    for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);

        if (!dungeon || dungeon->type == LFG_TYPE_ZONE)
            continue;

        LfgDungeonStore[dungeon->ID] = LFGDungeonData(dungeon);
    }

    // Fill teleport locations from DB
    QueryResult result = WorldDatabase.Query("SELECT dungeonId, position_x, position_y, position_z, orientation FROM lfg_entrances");

    if (!result)
    {
        TC_LOG_ERROR("server", ">> Loaded 0 lfg entrance positions. DB table `lfg_entrances` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 dungeonId = fields[0].GetUInt32();
        LFGDungeonContainer::iterator dungeonItr = LfgDungeonStore.find(dungeonId);
        if (dungeonItr == LfgDungeonStore.end())
        {
            TC_LOG_ERROR("sql", "table `lfg_entrances` contains coordinates for wrong dungeon %u", dungeonId);
            continue;
        }

        LFGDungeonData& data = dungeonItr->second;
        data.x = fields[1].GetFloat();
        data.y = fields[2].GetFloat();
        data.z = fields[3].GetFloat();
        data.o = fields[4].GetFloat();

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server", ">> Loaded %u lfg entrance positions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

    if (reload)
        CachedDungeonMapStore.clear();

    // Fill all other teleport coords from areatriggers
    for (LFGDungeonContainer::iterator itr = LfgDungeonStore.begin(); itr != LfgDungeonStore.end(); ++itr)
    {
        LFGDungeonData& dungeon = itr->second;

        // No teleport coords in database, load from areatriggers
        if (dungeon.type != LFG_TYPE_RANDOM && dungeon.x == 0.0f && dungeon.y == 0.0f && dungeon.z == 0.0f)
        {
            if (AreaTriggerStruct const* at = sObjectMgr->GetMapEntranceTrigger(dungeon.map))
            {
                dungeon.map = at->target_mapId;
                dungeon.x = at->target_X;
                dungeon.y = at->target_Y;
                dungeon.z = at->target_Z;
                dungeon.o = at->target_Orientation;
            }
            else
                TC_LOG_ERROR("lfg", "LFGMgr::LoadLFGDungeons: Failed to load teleport positions for dungeon %s, cant find areatrigger for map %u", dungeon.name.c_str(), dungeon.map);
        }

        if (dungeon.type != LFG_TYPE_RANDOM)
            CachedDungeonMapStore[dungeon.random_id].insert(dungeon.id);
        CachedDungeonMapStore[0].insert(dungeon.id);
    }
}

void LFGMgr::Update(uint32 diff)
{
    if (!isOptionEnabled(LFG_OPTION_ENABLE_DUNGEON_FINDER | LFG_OPTION_ENABLE_RAID_BROWSER))
        return;

    time_t currTime = time(NULL);

    // Remove obsolete role checks
    for (LfgRoleCheckContainer::iterator it = RoleChecksStore.begin(); it != RoleChecksStore.end();)
    {
        LfgRoleCheckContainer::iterator itRoleCheck = it++;
        LfgRoleCheck& roleCheck = itRoleCheck->second;
        if (currTime < roleCheck.cancelTime)
            continue;
        roleCheck.state = LFG_ROLE_CHECK_FAILED_TIMEOUT;

        for (LfgRolesMap::const_iterator itRoles = roleCheck.roles.begin(); itRoles != roleCheck.roles.end(); ++itRoles)
        {
            uint64 guid = itRoles->first;
            RestoreState(guid, "Remove Obsolete RoleCheck");
            SendLfgRoleCheckUpdate(guid, roleCheck);
            if (guid == roleCheck.leader)
                SendLfgJoinResult(guid, LfgJoinResultData(LFG_JOIN_FAILED, LFG_ROLE_CHECK_FAILED_TIMEOUT));
        }

        RestoreState(itRoleCheck->first, "Remove Obsolete RoleCheck");
        RoleChecksStore.erase(itRoleCheck);
    }

    // Remove obsolete proposals
    for (LfgProposalContainer::iterator it = ProposalsStore.begin(); it != ProposalsStore.end();)
    {
        LfgProposalContainer::iterator itRemove = it++;
        if (itRemove->second.cancelTime < currTime)
            RemoveProposal(itRemove, LFG_UPDATETYPE_PROPOSAL_FAILED);
    }

    // Remove obsolete kicks
    for (LfgPlayerBootContainer::iterator it = BootsStore.begin(); it != BootsStore.end();)
    {
        LfgPlayerBootContainer::iterator itBoot = it++;
        LfgPlayerBoot& boot = itBoot->second;
        if (boot.cancelTime < currTime)
        {
            boot.inProgress = false;
            for (LfgAnswerContainer::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
            {
                uint64 pguid = itVotes->first;
                if (pguid != boot.victim)
                    SendLfgBootProposalUpdate(pguid, boot);
                SetState(pguid, LFG_STATE_DUNGEON);
            }
            SetState(itBoot->first, LFG_STATE_DUNGEON);
            BootsStore.erase(itBoot);
        }
    }

    uint32 lastProposalId = m_lfgProposalId;
    // Check if a proposal can be formed with the new groups being added
    for (LfgQueueContainer::iterator it = QueuesStore.begin(); it != QueuesStore.end(); ++it)
        if (uint8 newProposals = it->second.FindGroups())
            TC_LOG_DEBUG("lfg", "LFGMgr::Update: Found %u new groups in queue %u", newProposals, it->first);

    if (lastProposalId != m_lfgProposalId)
    {
        // FIXME lastProposalId ? lastProposalId +1 ?
        for (LfgProposalContainer::const_iterator itProposal = ProposalsStore.find(m_lfgProposalId); itProposal != ProposalsStore.end(); ++itProposal)
        {
            uint32 proposalId = itProposal->first;
            LfgProposal& proposal = ProposalsStore[proposalId];

            uint64 guid = 0;
            for (LfgProposalPlayerContainer::const_iterator itPlayers = proposal.players.begin(); itPlayers != proposal.players.end(); ++itPlayers)
            {
                guid = itPlayers->first;
                SetState(guid, LFG_STATE_PROPOSAL);
                if (uint64 gguid = GetGroup(guid))
                {
                    SetState(gguid, LFG_STATE_PROPOSAL);
                    LfgDungeonSet const& dungeons = GetSelectedDungeons(guid);
                    std::string const& comment = GetComment(guid);
                    //SendLfgUpdateParty(guid, LfgUpdateData(LFG_UPDATETYPE_ROLECHECK_ABORTED, dungeons, comment));
                    SendLfgUpdateParty(guid, LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, dungeons, comment));
                }
                else
                    SendLfgUpdatePlayer(guid, LfgUpdateData(LFG_UPDATETYPE_PROPOSAL_BEGIN, GetSelectedDungeons(guid), GetComment(guid)));
                SendLfgUpdateProposal(guid, proposal);
            }

            if (proposal.state == LFG_PROPOSAL_SUCCESS)
                UpdateProposal(proposalId, guid, true);
        }
    }

    // Update all players status queue info
    if (m_QueueTimer > LFG_QUEUEUPDATE_INTERVAL)
    {
        m_QueueTimer = 0;
        time_t currTime = time(NULL);
        for (LfgQueueContainer::iterator it = QueuesStore.begin(); it != QueuesStore.end(); ++it)
            it->second.UpdateQueueTimers(currTime);
    }
    else
        m_QueueTimer += diff;
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

        uint32 oldrDungeonId = 0;
        LfgDungeonSet const& selectedDungeons = GetSelectedDungeons(player->GetGUID());
        if (!selectedDungeons.empty())
        {
            LFGDungeonData const* rDungeonData = GetLFGDungeon(*selectedDungeons.begin() & 0xFFFFF, player->GetTeam());
            if (rDungeonData && rDungeonData->type == LFG_TYPE_RANDOM)
                oldrDungeonId = rDungeonData->id;
        }

        if (oldrDungeonId)
            dungeons.insert(oldrDungeonId);
        else
            dungeons.insert(GetDungeon(groupGuid));
    }

    // Already in queue?
    LfgState state = GetState(groupGuid);
    if (state == LFG_STATE_QUEUED)
    {
        LFGQueue& queue = GetQueue(groupGuid);

        // currently core cannot queue in LFG/Flex/Scenarios at the same time
        if (!dungeons.empty())
        {
            LFGDungeonData const* entry = sLFGMgr->GetLFGDungeon(*dungeons.begin() & 0xFFFFF, player->GetTeam());
            if (entry && queue.GetQueueType(groupGuid) != entry->internalType)
            {
                ChatHandler(player).PSendSysMessage("You cannot queue in different type queues at the same time.");
                joinData.result = LFG_JOIN_INTERNAL_ERROR;
            }
        }

        if (joinData.result == LFG_JOIN_OK) // ??
            queue.RemoveFromQueue(groupGuid);
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
        bool isScenario = false;
        bool isRaid = false;
        for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end() && joinData.result == LFG_JOIN_OK; ++it)
        {
            LFGDungeonData const* entry = sLFGMgr->GetLFGDungeon(*it & 0xFFFFF, player->GetTeam());

            if(!entry)
            {
                joinData.result = LFG_JOIN_DUNGEON_INVALID;
                break;
            }

            switch (entry->dbc->subType)
            {
                case LFG_SUBTYPE_DUNGEON:
                    isDungeon = true;
                    break;
                case LFG_SUBTYPE_SCENARIO:
                    isScenario = true;
                    break;
                case LFG_SUBTYPE_RAID:
                case LFG_SUBTYPE_FLEX:
                    isRaid = true;
                    break;
            }

            if (isDungeon && isRaid || isDungeon && isScenario || isRaid && isScenario)
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
                    TC_LOG_ERROR("lfg", "Wrong dungeon type %u for dungeon %u", entry->dbc->type, *it);
                    joinData.result = LFG_JOIN_DUNGEON_INVALID;
                    break;
            }

            switch (entry->dbc->subType)
            {
                case LFG_SUBTYPE_SCENARIO:
                {
                    if (entry->dbc->difficulty == HEROIC_SCENARIO_DIFFICULTY && !isContinueDungeonRequest)
                    {
                        if (sWorld->getBoolConfig(CONFIG_LFG_DEBUG_JOIN))
                            break;

                        // heroic scenarios can be queued only in full group
                        if (!group)
                            joinData.result = LFG_JOIN_PARTY_INFO_FAILED;
                        else if (group->GetMembersCount() < entry->dbc->GetMinGroupSize())
                            joinData.result = LFG_JOIN_TOO_FEW_MEMBERS;
                    }
                    break;
                }
                case LFG_SUBTYPE_FLEX:
                {
                    if (sWorld->getBoolConfig(CONFIG_LFG_DEBUG_JOIN))
                        break;

                    // flex can be queued only in group
                    if (!group)
                        joinData.result = LFG_JOIN_PARTY_INFO_FAILED;
                    else if (group->GetMembersCount() < entry->dbc->GetMinGroupSize() && !isContinueDungeonRequest)
                        joinData.result = LFG_JOIN_TOO_FEW_MEMBERS;
                    break;
                }
                default:
                    break;
            }

            // check max members count for a group
            if (group && entry->dbc->GetMaxGroupSize() < group->GetMembersCount())
                joinData.result = LFG_JOIN_TOO_MUCH_MEMBERS;
        }

        // it could be changed
        if (joinData.result == LFG_JOIN_OK)
        {
            // Expand random dungeons and check restrictions
            if (rDungeonId)
                dungeons = GetDungeonsByRandom(rDungeonId & 0xFFFFF);

            // if we have lockmap then there are no compatible dungeons
            GetCompatibleDungeons(dungeons, players, joinData.lockmap);
            if (dungeons.empty())
                joinData.result = /*group ? LFG_JOIN_PARTY_NOT_MEET_REQS : */LFG_JOIN_NOT_MEET_REQS;
        }

        // only damagers allowed for scenarios
        if (isScenario)
            roles = roles & PLAYER_ROLE_LEADER | PLAYER_ROLE_DAMAGE;
    }

    // Can't join. Send result
    if (joinData.result != LFG_JOIN_OK)
    {
        TC_LOG_DEBUG("lfg", "LFGMgr::Join: [" UI64FMTD "] joining with %u members. result: %u", playerGuid, group ? group->GetMembersCount() : 1, joinData.result);
        if (!dungeons.empty())                             // Only should show lockmap when have no dungeons available
            joinData.lockmap.clear();
        player->GetSession()->SendLfgJoinResult(joinData);
        return;
    }

    SetComment(playerGuid, comment);

    // FIXME - Raid browser not supported yet
    // FIXME: raid finder dungeons are raids
    /*if (isRaid)
    {
        TC_LOG_DEBUG("lfg", "LFGMgr::Join: [" UI64FMTD "] trying to join raid browser and it's disabled.", playerGuid);
        return;
    }*/

    std::string debugNames = "";
    if (group)                                               // Begin rolecheck
    {
        // Create new rolecheck
        LfgRoleCheck& roleCheck = RoleChecksStore[groupGuid];
        roleCheck.roles.clear();
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
        //LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_JOIN_QUEUE, dungeons, comment);
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* plrg = itr->getSource())
            {
                uint64 pguid = plrg->GetGUID();
                SetRoles(pguid, roles);
                //plrg->GetSession()->SendLfgUpdateParty(updateData);
                plrg->GetSession()->SendLfgJoinResult(joinData);
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
        LfgRolesMap rolesMap;
        rolesMap[playerGuid] = roles;
        LFGQueue& queue = GetQueue(playerGuid);
        queue.AddQueueData(playerGuid, time(NULL), dungeons, rolesMap);

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
        SetState(groupGuid, LFG_STATE_QUEUED);
        SetRoles(playerGuid, roles);
        player->GetSession()->SendLfgUpdatePlayer(LfgUpdateData(LFG_UPDATETYPE_JOIN_QUEUE, dungeons, comment));
        player->GetSession()->SendLfgUpdatePlayer(LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, dungeons, comment));
        debugNames.append(player->GetName());
    }

    if (sLog->ShouldLog("lfg", LOG_LEVEL_DEBUG))
    {
        std::ostringstream o;
        o << "LFGMgr::Join: [" << playerGuid << "] joined (" << (group ? "group" : "player") << ") Members: " << debugNames.c_str()
          << ". Dungeons (" << uint32(dungeons.size()) << "): " << ConcatenateDungeons(dungeons);
        TC_LOG_DEBUG("lfg", "%s", o.str().c_str());
    }
}

/**
    Leaves Dungeon System. Player/Group is removed from queue, rolechecks, proposals
    or votekicks. Player or group needs to be not NULL and using Dungeon System

   @param[in]     guid Player or group guid
*/
void LFGMgr::LeaveLfg(uint64 guid)
{
    uint64 gguid = IS_GROUP(guid) ? guid : GetGroup(guid);

    TC_LOG_DEBUG("lfg", "LFGMgr::LeaveLfg: [" UI64FMTD "]", guid);

    LfgState state = GetState(guid);
    switch (state)
    {
        case LFG_STATE_QUEUED:
            if (gguid)
            {
                SetState(gguid, LFG_STATE_NONE);
                LfgGuidSet const& players = GetPlayers(gguid);
                for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
                {
                    SetState(*it, LFG_STATE_NONE);
                    SendLfgUpdateParty(*it, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE, GetSelectedDungeons(*it)));
                }
                LFGQueue& queue = GetQueue(gguid);
                queue.RemoveFromQueue(gguid);
            }
            else
            {
                SendLfgUpdatePlayer(guid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE, GetSelectedDungeons(guid)));
                SetState(guid, LFG_STATE_NONE);
                LFGQueue& queue = GetQueue(guid);
                queue.RemoveFromQueue(guid);
            }
            break;
        case LFG_STATE_ROLECHECK:
            if (gguid)
                UpdateRoleCheck(gguid);                    // No player to update role = LFG_ROLECHECK_ABORTED
            break;
        case LFG_STATE_PROPOSAL:
        {
            // Remove from Proposals
            LfgProposalContainer::iterator it = ProposalsStore.begin();
            uint64 pguid = gguid == guid ? GetLeader(gguid) : guid;
            while (it != ProposalsStore.end())
            {
                LfgProposalPlayerContainer::iterator itPlayer = it->second.players.find(pguid);
                if (itPlayer != it->second.players.end())
                {
                    // Mark the player/leader of group who left as didn't accept the proposal
                    itPlayer->second.accept = LFG_ANSWER_DENY;
                    break;
                }
                ++it;
            }

            // Remove from queue - if proposal is found, RemoveProposal will call RemoveFromQueue
            if (it != ProposalsStore.end())
                RemoveProposal(it, LFG_UPDATETYPE_PROPOSAL_FAILED/*LFG_UPDATETYPE_PROPOSAL_DECLINED*/);
            break;
        }
        case LFG_STATE_NONE:
        case LFG_STATE_RAIDBROWSER:
            break;
        case LFG_STATE_DUNGEON:
        case LFG_STATE_FINISHED_DUNGEON:
        case LFG_STATE_BOOT:
            if (guid != gguid) // Player
                SetState(guid, LFG_STATE_NONE);
            break;
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
    LfgRoleCheckContainer::iterator itRoleCheck = RoleChecksStore.find(gguid);
    if (itRoleCheck == RoleChecksStore.end())
        return;

    LfgRoleCheck& roleCheck = itRoleCheck->second;
    bool sendRoleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && guid;

    if (!guid)
        roleCheck.state = LFG_ROLECHECK_ABORTED;
    else if (roles < PLAYER_ROLE_TANK)                            // Player selected no role.
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    else
    {
        roleCheck.roles[guid] = roles;

        // Check if all players have selected a role
        LfgRolesMap::iterator itRoles = roleCheck.roles.begin();
        while (itRoles != roleCheck.roles.end() && itRoles->second != PLAYER_ROLE_NONE)
            ++itRoles;

        if (itRoles == roleCheck.roles.end())
        {
            // use temporal var to check roles, CheckGroupRoles modifies the roles
            check_roles = roleCheck.roles;

            uint32 n = 0;
            roleCheck.state = CheckGroupRoles(check_roles, LfgRoleData(*roleCheck.dungeons.begin() & 0xFFFFF), n)
                ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_WRONG_ROLES;
        }
    }

    LfgDungeonSet dungeons;
    if (roleCheck.rDungeonId)
        dungeons.insert(roleCheck.rDungeonId);
    else
        dungeons = roleCheck.dungeons;

    // only damagers in scenarios
    if (roles)
    {
        if (LFGDungeonData const* dungeonData = GetLFGDungeon(*dungeons.begin()))
            if (dungeonData->dbc->IsScenario() && !dungeonData->dbc->IsChallenge())
                roles = roles & (PLAYER_ROLE_LEADER | PLAYER_ROLE_DAMAGE);
    }

    LfgJoinResultData joinData = LfgJoinResultData(LFG_JOIN_FAILED, roleCheck.state);
    for (LfgRolesMap::const_iterator it = roleCheck.roles.begin(); it != roleCheck.roles.end(); ++it)
    {
        uint64 pguid = it->first;
        auto player = ObjectAccessor::FindPlayer(pguid);
        if (!player)
        {
            if (roleCheck.state == LFG_ROLECHECK_FINISHED)
                SetState(pguid, LFG_STATE_QUEUED);
            else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
                ClearState(pguid);
            continue;
        }

        if (sendRoleChosen)
            SendLfgRoleChosen(pguid, guid, roles);

        SendLfgRoleCheckUpdate(pguid, roleCheck);
        switch (roleCheck.state)
        {
            case LFG_ROLECHECK_INITIALITING:
                continue;
            case LFG_ROLECHECK_FINISHED:
                SetState(pguid, LFG_STATE_QUEUED);
                SetRoles(pguid, it->second);
                //SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE, dungeons, GetComment(pguid)));
                SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_ADDED_TO_QUEUE, dungeons, GetComment(pguid)));
                break;
            default:
                if (roleCheck.leader == pguid)
                    SendLfgJoinResult(pguid, joinData);
                //SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_ROLECHECK_FAILED, dungeons));
                SendLfgUpdateParty(pguid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE, dungeons));
                RestoreState(pguid, "Rolecheck Failed");
                break;
        }
    }

    if (roleCheck.state == LFG_ROLECHECK_FINISHED)
    {
        SetState(gguid, LFG_STATE_QUEUED);
        LFGQueue& queue = GetQueue(gguid);
        queue.AddQueueData(gguid, time(NULL), roleCheck.dungeons, roleCheck.roles);
        RoleChecksStore.erase(itRoleCheck);
    }
    else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
    {
        RestoreState(gguid, "Rolecheck Failed");
        RoleChecksStore.erase(itRoleCheck);
    }
}

/**
   Given a list of dungeons remove the dungeons players have restrictions.

   @param[in, out] dungeons Dungeons to check restrictions
   @param[in]     players Set of players to check their dungeon restrictions
   @param[out]    lockMap Map of players Lock status info of given dungeons (Empty if dungeons is not empty)
*/
void LFGMgr::GetCompatibleDungeons(LfgDungeonSet& dungeons, LfgGuidSet const& players, LfgLockPartyMap& lockMap)
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
bool LFGMgr::CheckGroupRoles(LfgRolesMap& groles, LfgRoleData const& roleData, uint32 &n, bool removeLeaderFlag /*= true*/)
{
    if (groles.empty())
        return false;

    uint8 damage = 0;
    uint8 tank = 0;
    uint8 healer = 0;

    if (removeLeaderFlag)
        for (LfgRolesMap::iterator it = groles.begin(); it != groles.end(); ++it)
            it->second &= ~PLAYER_ROLE_LEADER;

    for (LfgRolesMap::iterator it = groles.begin(); it != groles.end(); ++it, ++n)
    {
        if (n > 1000)
            return false;

        if (it->second == PLAYER_ROLE_NONE || (it->second & ROLE_FULL_MASK) == 0)
            return false;

        if (it->second & PLAYER_ROLE_TANK)
        {
            if (it->second != PLAYER_ROLE_TANK)                 // if not one role taken - check enother
            {
                it->second -= PLAYER_ROLE_TANK;                 // exclude role for recurse check
                if (CheckGroupRoles(groles, roleData, n, false))   // check role with it
                    return true;                                // if plr not tank group can be completed
                it->second += PLAYER_ROLE_TANK;                 // return back excluded role.
            }
            else if (tank >= roleData.tanksNeeded)              // if set one role check needed count for tank
                return false;
            //else                                              // no. should alway incrase counter. not only when plr set one role
                                                                //- but when he set 2-3 roles but only one of them mach.
                                                                //- when no need increase - where is return :)
                ++tank;                                         // set role.
        }

        if (it->second & PLAYER_ROLE_DAMAGE)
        {
            if (it->second != PLAYER_ROLE_DAMAGE)
            {
                it->second -= PLAYER_ROLE_DAMAGE;
                if (CheckGroupRoles(groles, roleData, n, false))
                    return true;
                it->second += PLAYER_ROLE_DAMAGE;
            }
            else if (damage >= roleData.dpsNeeded)
                return false;
            //else
                ++damage;
        }

        if (it->second & PLAYER_ROLE_HEALER)
        {
            if (it->second != PLAYER_ROLE_HEALER)
            {
                it->second -= PLAYER_ROLE_HEALER;
                if (CheckGroupRoles(groles, roleData, n, false))
                    return true;
                it->second += PLAYER_ROLE_HEALER;
            }
            else if (healer >= roleData.healerNeeded)
                return false;
            //else
                ++healer;
        }
    }
    return tank + healer + damage == uint8(groles.size());
}

/**
   Makes a new group given a proposal
   @param[in]     proposal Proposal to get info from
*/
void LFGMgr::MakeNewGroup(LfgProposal const& proposal)
{
    LfgGuidList players;
    LfgGuidList playersToTeleport;

    for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
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
    LFGDungeonData const* dungeon = GetLFGDungeon(proposal.dungeonId);
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
            group->RemoveMember(player->GetGUID());

        if (!grp)
        {
            grp = new Group();
            grp->Create(player);
            grp->ConvertToLFG(dungeon);
            uint64 gguid = grp->GetGUID();
            SetDungeon(gguid, dungeon->Entry());
            SetState(gguid, LFG_STATE_DUNGEON);

            if (dungeon->dbc->GetInternalType() == LFG_TYPE_RAID)
                grp->SetRaidDifficulty(Difficulty(dungeon->difficulty));
            else
                grp->SetDungeonDifficulty(Difficulty(dungeon->difficulty));

            sGroupMgr->AddGroup(grp);
        }
        else if (group != grp)
            grp->AddMember(player);

        grp->SetLfgRoles(pguid, proposal.players.find(pguid)->second.role);

        // Add the cooldown spell if queued for a random dungeon
        if (dungeon->type == LFG_TYPE_RANDOM)
            player->CastSpell(player, LFG_SPELL_DUNGEON_COOLDOWN, false);
    }

    ASSERT(grp);
    _SaveToDB(grp->GetGUID(), grp->GetDbStoreId());

    // Update group info
    grp->SendUpdate();

    // Teleport Player
    for (LfgGuidList::const_iterator it = playersToTeleport.begin(); it != playersToTeleport.end(); ++it)
        if (Player* player = ObjectAccessor::FindPlayer(*it))
            TeleportPlayer(player, false);
}

uint32 LFGMgr::AddProposal(LfgProposal& proposal)
{
    proposal.id = ++m_lfgProposalId;
    ProposalsStore[m_lfgProposalId] = proposal;
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
    LfgProposalContainer::iterator itProposal = ProposalsStore.find(proposalId);
    if (itProposal == ProposalsStore.end())
        return;

    LfgProposal& proposal = itProposal->second;

    // Check if proposal have the current player
    LfgProposalPlayerContainer::iterator itProposalPlayer = proposal.players.find(guid);
    if (itProposalPlayer == proposal.players.end())
        return;

    LfgProposalPlayer& player = itProposalPlayer->second;
    player.accept = LfgAnswer(accept);

    TC_LOG_DEBUG("lfg", "LFGMgr::UpdateProposal: Player [" UI64FMTD "] of proposal %u selected: %u", guid, proposalId, accept);
    if (!accept)
    {
        RemoveProposal(itProposal, LFG_UPDATETYPE_PROPOSAL_FAILED/*LFG_UPDATETYPE_PROPOSAL_DECLINED*/);
        return;
    }

    // check if all have answered and reorder players (leader first)
    bool allAnswered = true;
    for (LfgProposalPlayerContainer::const_iterator itPlayers = proposal.players.begin(); itPlayers != proposal.players.end(); ++itPlayers)
        if (itPlayers->second.accept != LFG_ANSWER_AGREE)   // No answer (-1) or not accepted (0)
            allAnswered = false;

    if (!allAnswered)
    {
        for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
            SendLfgUpdateProposal(it->first, proposal);

        return;
    }

    bool sendUpdate = proposal.state != LFG_PROPOSAL_SUCCESS;
    proposal.state = LFG_PROPOSAL_SUCCESS;
    time_t joinTime = time(NULL);

    LFGQueue& queue = GetQueue(guid);
    for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        uint64 pguid = it->first;
        uint64 gguid = it->second.group;

        uint32 dungeonId = proposal.dungeonId;
        int32 waitTime = -1;

        LfgDungeonSet const& selectedDungeons = GetSelectedDungeons(pguid);
        if (!selectedDungeons.empty())
            dungeonId = *selectedDungeons.begin();

        LfgUpdateData updateData = LfgUpdateData(LFG_UPDATETYPE_GROUP_FOUND, selectedDungeons);

        if (sendUpdate)
           SendLfgUpdateProposal(pguid, proposal);

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

        SetState(pguid, LFG_STATE_DUNGEON);
    }

    // Remove players/groups from Queue
    for (LfgGuidList::const_iterator it = proposal.queues.begin(); it != proposal.queues.end(); ++it)
        queue.RemoveFromQueue(*it);

    MakeNewGroup(proposal);
    ProposalsStore.erase(itProposal);
}

/**
   Remove a proposal from the pool, remove the group that didn't accept (if needed) and readd the other members to the queue

   @param[in]     itProposal Iterator to the proposal to remove
   @param[in]     type Type of removal (LFG_UPDATETYPE_PROPOSAL_FAILED, LFG_UPDATETYPE_PROPOSAL_DECLINED)
*/
void LFGMgr::RemoveProposal(LfgProposalContainer::iterator itProposal, LfgUpdateType type)
{
    LfgProposal& proposal = itProposal->second;
    proposal.state = LFG_PROPOSAL_FAILED;

    TC_LOG_DEBUG("lfg", "LFGMgr::RemoveProposal: Proposal %u, state FAILED, UpdateType %u", itProposal->first, type);
    // Mark all people that didn't answered as no accept
    if (type == LFG_UPDATETYPE_PROPOSAL_FAILED)
        for (LfgProposalPlayerContainer::iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
            if (it->second.accept == LFG_ANSWER_PENDING)
                it->second.accept = LFG_ANSWER_DENY;

    // Mark players/groups to be removed
    LfgGuidSet toRemove;
    for (LfgProposalPlayerContainer::iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
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
    for (LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        uint64 guid = it->first;
        uint64 gguid = it->second.group ? it->second.group : guid;

        SendLfgUpdateProposal(guid, proposal);

        if (toRemove.find(gguid) != toRemove.end())         // Didn't accept or in same group that someone that didn't accept
        {
            LfgUpdateData updateData;
            updateData.dungeons = GetSelectedDungeons(guid);

            if (it->second.accept == LFG_ANSWER_DENY)
            {
                updateData.updateType = type;
                TC_LOG_DEBUG("lfg", "LFGMgr::RemoveProposal: [" UI64FMTD "] didn't accept. Removing from queue and compatible cache", guid);
            }
            else
            {
                updateData.updateType = LFG_UPDATETYPE_REMOVED_FROM_QUEUE;
                TC_LOG_DEBUG("lfg", "LFGMgr::RemoveProposal: [" UI64FMTD "] in same group that someone that didn't accept. Removing from queue and compatible cache", guid);
            }

            RestoreState(guid, "Proposal Fail (didn't accepted or in group with someone that didn't accept");
            if (gguid != guid)
            {
                RestoreState(it->second.group, "Proposal Fail (someone in group didn't accepted)");
                SendLfgUpdateParty(guid, updateData);
            }
            else
                SendLfgUpdatePlayer(guid, updateData);
        }
        else
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::RemoveProposal: Readding [" UI64FMTD "] to queue.", guid);
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

    LFGQueue& queue = GetQueue(proposal.players.begin()->first);
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

    ProposalsStore.erase(itProposal);
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

    LfgPlayerBoot& boot = BootsStore[gguid];
    boot.inProgress = true;
    boot.cancelTime = time_t(time(NULL)) + LFG_TIME_BOOT;
    boot.reason = reason;
    boot.victim = victim;
    boot.votesNeeded = GetVotesNeededForKick(gguid);

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
   @param[in]     player answer
*/
void LFGMgr::UpdateBoot(uint64 guid, bool accept)
{
    uint64 gguid = GetGroup(guid);
    if (!gguid)
        return;

    LfgPlayerBootContainer::iterator itBoot = BootsStore.find(gguid);
    if (itBoot == BootsStore.end())
        return;

    LfgPlayerBoot& boot = itBoot->second;

    if (boot.votes[guid] != LFG_ANSWER_PENDING)    // Cheat check: Player can't vote twice
        return;

    boot.votes[guid] = LfgAnswer(accept);

    uint8 votesNum = 0;
    uint8 agreeNum = 0;
    for (LfgAnswerContainer::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
    {
        if (itVotes->second != LFG_ANSWER_PENDING)
        {
            ++votesNum;
            if (itVotes->second == LFG_ANSWER_AGREE)
                ++agreeNum;
        }
    }

    // if we don't have enough votes (agree or deny) do nothing
    if (agreeNum < boot.votesNeeded && (votesNum - agreeNum) < boot.votesNeeded)
        return;

    // Send update info to all players
    boot.inProgress = false;
    for (LfgAnswerContainer::const_iterator itVotes = boot.votes.begin(); itVotes != boot.votes.end(); ++itVotes)
    {
        uint64 pguid = itVotes->first;
        if (pguid != boot.victim)
        {
            SetState(pguid, LFG_STATE_DUNGEON);
            SendLfgBootProposalUpdate(pguid, boot);
        }
    }

    SetState(gguid, LFG_STATE_DUNGEON);
    if (agreeNum >= boot.votesNeeded)                   // Vote passed - Kick player
    {
        if (Group* group = sGroupMgr->GetGroupByGUID(GUID_LOPART(gguid)))
            Player::RemoveFromGroup(group, boot.victim, GROUP_REMOVEMETHOD_KICK_LFG);
        DecreaseKicksLeft(gguid);
    }
    BootsStore.erase(itBoot);
}

/**
   Teleports the player in or out the dungeon

   @param[in]     player Player to teleport
   @param[in]     out Teleport out (true) or in (false)
   @param[in]     fromOpcode Function called from opcode handlers? (Default false)
*/
void LFGMgr::TeleportPlayer(Player* player, bool out, bool fromOpcode /*= false*/)
{
    LFGDungeonData const* dungeon = NULL;
    Group* group = player->GetGroup();

    if (group && group->isLFGGroup())
        dungeon = GetLFGDungeon(GetDungeon(group->GetGUID()), player->GetTeam());

    if (!dungeon)
    {
        TC_LOG_DEBUG("lfg", "TeleportPlayer: Player %s not in group/lfggroup or dungeon not found!",
            player->GetName());
        player->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_TELEPORT_LOCATION));
        return;
    }

    if (out)
    {
        TC_LOG_DEBUG("lfg", "TeleportPlayer: Player %s is being teleported out. Current Map %u - Expected Map %u",
            player->GetName(), player->GetMapId(), uint32(dungeon->map));
        if (player->GetMapId() == uint32(dungeon->map))
        {
            TeleportEvent* e = new TeleportEvent(player, player->GetBattlegroundEntryPoint(), 0, false, true, 1000);
            if (!e->Schedule())
                delete e;
        }

        return;
    }

    LfgTeleportError error = LFG_TELEPORTERROR_OK;

    if (!player->isAlive())
        error = LFG_TELEPORTERROR_PLAYER_DEAD;
    else if (player->IsFalling() || player->HasUnitState(UNIT_STATE_JUMPING))
        error = LFG_TELEPORTERROR_NOT_WHILE_FALLING;
    else if (player->IsMirrorTimerActive(FATIGUE_TIMER))
        error = LFG_TELEPORTERROR_NOT_WHILE_FATIGUED;
    /*else if (player->GetVehicle())
        error = LFG_TELEPORTERROR_IN_VEHICLE;
    else if (player->GetCharmGUID())
        error = LFG_TELEPORTERROR_CHARMING;*/
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
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL && !mapid; itr = itr->next())
            {
                Player* plrg = itr->getSource();
                if (plrg && plrg != player && plrg->GetMapId() == uint32(dungeon->map))
                {
                    mapid = plrg->GetMapId();
                    x = plrg->GetPositionX();
                    y = plrg->GetPositionY();
                    z = plrg->GetPositionZ();
                    orientation = plrg->GetOrientation();
                    break;
                }
            }
        }

        if (!player->GetMap()->IsDungeon())
            player->SetBattlegroundEntryPoint();
        
        if (player->isInFlight())
        {
            player->GetMotionMaster()->MovementExpired();
            player->CleanupAfterTaxiFlight();
        }

        // hack....
        TeleportEvent* e = new TeleportEvent(player, mapid, x, y, z, orientation, 0, true, false, 1000);
        if (!e->Schedule())
            delete e;
        return;
    }
    else
        error = LFG_TELEPORTERROR_INVALID_TELEPORT_LOCATION;

    //if (error != LFG_TELEPORTERROR_OK)
        player->GetSession()->SendLfgTeleportError(uint8(error));

    TC_LOG_DEBUG("lfg", "TeleportPlayer: Player %s is being teleported in to map %u "
        "(x: %f, y: %f, z: %f) Result: %u", player->GetName(), dungeon->map,
        dungeon->x, dungeon->y, dungeon->z, error);
}

void LFGMgr::SendUpdateStatus(Player* player, LfgUpdateData const& updateData, bool party)
{
    ObjectGuid guid = player->GetGUID();
    ObjectGuid gguid = player->GetGroup() ? player->GetGroup()->GetGUID() : player->GetGUID();
    LFGQueue& queue = GetQueue(gguid);

    bool queued = false;
    bool active = false;
    bool join = false;
    bool lfgJoined = true;

    switch (updateData.updateType)
    {
        case LFG_UPDATETYPE_LEADER_UNK1:
            lfgJoined = false;
            break;
        case LFG_UPDATETYPE_ROLECHECK_FAILED:
        case LFG_UPDATETYPE_REMOVED_FROM_QUEUE:
        case LFG_UPDATETYPE_PROPOSAL_FAILED:
        case LFG_UPDATETYPE_GROUP_FOUND:
            break;
        case LFG_UPDATETYPE_ADDED_TO_QUEUE:
        case LFG_UPDATETYPE_ROLECHECK_ABORTED:
            join = true;
            queued = true;
            active = true;
            break;
        case LFG_UPDATETYPE_JOIN_QUEUE:
        case LFG_UPDATETYPE_PROPOSAL_BEGIN:
        case LFG_UPDATETYPE_SUSPEND_QUEUE:
            join = true;
            queued = true;
            break;
        case LFG_UPDATETYPE_UPDATE_STATUS:
            join = updateData.state != LFG_STATE_ROLECHECK && updateData.state != LFG_STATE_NONE;
            queued = updateData.state == LFG_STATE_QUEUED;
            active = true;  // fixme
            break;
        case LFG_UPDATETYPE_GROUP_MEMBER_OFFLINE:
        case LFG_UPDATETYPE_GROUP_DISBAND_UNK16:
        case LFG_UPDATETYPE_PARTY_ROLE_NOT_AVAILABLE:
        case LFG_UPDATETYPE_LFG_OBJECT_FAILED:
        case LFG_UPDATETYPE_REMOVED_LEVELUP:
        case LFG_UPDATETYPE_REMOVED_XP_TOGGLE:
        case LFG_UPDATETYPE_REMOVED_FACTION_CHANGE:
            // ???
            break;
        default:
            break;
    }

    LfgQueueData const* queueData = queue.GetQueueData(gguid);

    WorldPacket data(SMSG_LFG_UPDATE_STATUS, 60);
    data.WriteBits(updateData.comment.size(), 8);   // comment
    data.WriteBits(0, 24);                          // guids size
    data.WriteGuidMask<5>(guid);

    data.WriteBit(party);                           // in group
    data.WriteBit(join);                            // joined
    data.WriteBit(lfgJoined);                       // display or not the lfr button, lfg join ?, 0 for last one

    data.WriteGuidMask<3, 7, 1>(guid);
    data.WriteBit(queued);                          // 1 - show notification
    data.WriteGuidMask<0>(guid);
    data.WriteBits(updateData.dungeons.size(), 22);
    data.WriteGuidMask<6, 4>(guid);
    data.WriteBit(1/*!queued ? 0 : active*/);            // 1 - active, 0 - paused
    data.WriteGuidMask<2>(guid);

    data.WriteString(updateData.comment);
    data << uint8(queueData ? queueData->subType : LFG_SUBTYPE_DUNGEON);  // 1 - dungeon finder, 2 - raid finder, 3 - scenarios, 4 - flex
    data << uint8(updateData.updateType);           // error?   1, 11, 17 - succed, other - failed
    data.WriteGuidBytes<4>(guid);
    data << uint32(GetRoles(guid));                 // roles mask
    data.WriteGuidBytes<5, 7>(guid);
    data << uint32(3);                              // queue id. 4 - looking for raid, 3 - others
    data.WriteGuidBytes<3>(guid);
    for (LfgDungeonSet::const_iterator i = updateData.dungeons.begin(); i != updateData.dungeons.end(); ++i)
        data << uint32(GetLFGDungeonEntry(*i));     // Dungeon entries
    data.WriteGuidBytes<0>(guid);
    data << uint32(player->GetTeam());              // group id?
    data.WriteGuidBytes<1>(guid);
    data << uint32(queueData ? queueData->joinTime : time(NULL));
    data.WriteGuidBytes<6>(guid);
    for (int i = 0; i < 3; ++i)
        data << uint8(0);                           //unk8 always 0 ?
    data.WriteGuidBytes<2>(guid);

    player->GetSession()->SendPacket(&data);
}

/**
   Finish a dungeon and give reward, if any.

   @param[in]     guid Group guid
   @param[in]     dungeonId Dungeonid
*/
void LFGMgr::FinishDungeon(uint64 gguid, const uint32 dungeonId)
{
    uint32 gDungeonId = GetDungeon(gguid);
    if (gDungeonId != dungeonId)
    {
        TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] Finished dungeon %u but group queued for %u. Ignoring", gguid, dungeonId, gDungeonId);
        return;
    }

    if (GetState(gguid) == LFG_STATE_FINISHED_DUNGEON) // Shouldn't happen. Do not reward multiple times
    {
        TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] Already rewarded group. Ignoring", gguid);
        return;
    }

    SetState(gguid, LFG_STATE_FINISHED_DUNGEON);

    const LfgGuidSet& players = GetPlayers(gguid);
    for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        uint64 guid = (*it);
        if (GetState(guid) == LFG_STATE_FINISHED_DUNGEON)
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] Already rewarded player. Ignoring", guid);
            continue;
        }

        uint32 rDungeonId = 0;
        const LfgDungeonSet& dungeons = GetSelectedDungeons(guid);
        if (!dungeons.empty())
            rDungeonId = (*dungeons.begin());

        SetState(guid, LFG_STATE_FINISHED_DUNGEON);

        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || !player->IsInWorld())
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] not found in world", guid);
            continue;
        }

        // Give rewards only if its a rewardable dungeon
        LFGDungeonData const* rDungeon = GetLFGDungeon(rDungeonId, player->GetTeam());
        if (!rDungeon)
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] dungeon %u does not exist", guid, rDungeonId);
            continue;
        }
        // if 'random' dungeon is not random nor seasonal, check actual dungeon (it can be raid finder)
        if (rDungeon->type != LFG_TYPE_RANDOM && !rDungeon->seasonal)
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] dungeon %u type %i is not random nor seasonal %i and can't be rewarded, rDungeon->id %i", guid, rDungeonId, rDungeon->type, rDungeon->seasonal, rDungeon->id);
            continue;
        }

        LFGDungeonData const* dungeonDone = GetLFGDungeon(dungeonId, player->GetTeam());
        if(!dungeonDone)
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: dungeonDone %i not found", dungeonId);
            continue;
        }
        /*if(!dungeonDone->dbc->CanBeRewarded())
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: dungeonDone %i not CanBeRewarded %i", dungeonId, dungeonDone->dbc->CanBeRewarded());
            continue;
        }*/

        // there can be more that 1 non-random dungeon selected, so fall back to current dungeon id
        rDungeonId = dungeonDone->random_id;
        rDungeon = dungeonDone;

        uint32 mapId = dungeonDone ? uint32(dungeonDone->map) : 0;

        if (player->GetMapId() != mapId)
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] is in map %u and should be in %u to get reward", guid, player->GetMapId(), mapId);
            continue;
        }

        // Update achievements
        if (rDungeon->difficulty == HEROIC_DIFFICULTY)
            player->UpdateAchievementCriteria(CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS, 1);

        player->UpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_RAID, dungeonDone->dbc->GetMaxGroupSize());

        LfgReward const* reward = GetDungeonReward(rDungeonId, player->getLevel());
        if (!reward)
        {
            TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] Don`t find reward for DungeonId %i player %u level", guid, rDungeonId, player->getLevel());
            continue;
        }

        // Give rewards
        bool done = reward->RewardPlayer(player, rDungeon, dungeonDone, false);

        TC_LOG_DEBUG("lfg", "LFGMgr::FinishDungeon: [" UI64FMTD "] done dungeon %u, %s previously done.", player->GetGUID(), GetDungeon(gguid), done? " " : " not");
        LfgPlayerRewardData data = LfgPlayerRewardData(rDungeon->Entry(), dungeonDone->Entry(), done, false, reward);
        player->GetSession()->SendLfgPlayerReward(data);
    }

    // send update to players
    if (Group* group = sGroupMgr->GetGroupByGUID(gguid))
        group->SendUpdate();
}

// --------------------------------------------------------------------------//
// Auxiliar Functions
// --------------------------------------------------------------------------//

/**
   Get the dungeon list that can be done given a random dungeon entry.

   @param[in]     randomdungeon Random dungeon id (if value = 0 will return all dungeons)
   @returns Set of dungeons that can be done.
*/
LfgDungeonSet const& LFGMgr::GetDungeonsByRandom(uint32 randomdungeon)
{
    return CachedDungeonMapStore[randomdungeon];
}

/**
   Get the reward of a given dungeon at a certain level

   @param[in]     dungeon dungeon id
   @param[in]     level Player level
   @returns Reward
*/
LfgReward const* LFGMgr::GetDungeonReward(uint32 dungeon, uint8 level)
{
    LfgReward const* rew = NULL;
    LfgRewardContainerBounds bounds = RewardMapStore.equal_range(dungeon & 0x00FFFFFF);
    for (LfgRewardContainer::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
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
    LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId);
    if (!dungeon)
        return LFG_TYPE_NONE;

    return LfgType(dungeon->type);
}

LfgState LFGMgr::GetState(uint64 guid)
{
    LfgState state;
    if (IS_GROUP(guid))
        state = GroupsStore[guid].GetState();
    else
        state = PlayersStore[guid].GetState();

    TC_LOG_TRACE("lfg", "LFGMgr::GetState: [" UI64FMTD "] = %u", guid, state);
    return state;
}

LfgState LFGMgr::GetOldState(uint64 guid)
{
    LfgState state;
    if (IS_GROUP(guid))
        state = GroupsStore[guid].GetOldState();
    else
        state = PlayersStore[guid].GetOldState();

    TC_LOG_TRACE("lfg", "LFGMgr::GetOldState: [" UI64FMTD "] = %u", guid, state);
    return state;
}

uint32 LFGMgr::GetDungeon(uint64 guid, bool asId /*= true */)
{
    uint32 dungeon = GroupsStore[guid].GetDungeon(asId);
    TC_LOG_TRACE("lfg", "LFGMgr::GetDungeon: [" UI64FMTD "] asId: %u = %u", guid, asId, dungeon);
    return dungeon;
}

uint32 LFGMgr::GetDungeonMapId(uint64 guid)
{
    uint32 dungeonId = GroupsStore[guid].GetDungeon(true);
    uint32 mapId = 0;
    if (dungeonId)
        if (LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId))
            mapId = dungeon->map;

    TC_LOG_TRACE("lfg", "LFGMgr::GetDungeonMapId: [" UI64FMTD "] = %u (DungeonId = %u)", guid, mapId, dungeonId);
    return mapId;
}

uint8 LFGMgr::GetRoles(uint64 guid)
{
    uint8 roles = PlayersStore[guid].GetRoles();
    TC_LOG_TRACE("lfg", "LFGMgr::GetRoles: [" UI64FMTD "] = %u", guid, roles);
    return roles;
}

const std::string& LFGMgr::GetComment(uint64 guid)
{
    TC_LOG_TRACE("lfg", "LFGMgr::GetComment: [" UI64FMTD "] = %s", guid, PlayersStore[guid].GetComment().c_str());
    return PlayersStore[guid].GetComment();
}

LfgDungeonSet const& LFGMgr::GetSelectedDungeons(uint64 guid)
{
    TC_LOG_TRACE("lfg", "LFGMgr::GetSelectedDungeons: [" UI64FMTD "]", guid);
    return PlayersStore[guid].GetSelectedDungeons();
}

LfgLockMap const LFGMgr::GetLockedDungeons(uint64 guid)
{
    TC_LOG_TRACE("lfg", "LFGMgr::GetLockedDungeons: [" UI64FMTD "]", guid);
    LfgLockMap lock;
    Player* player = ObjectAccessor::FindPlayer(guid);
    if (!player)
    {
        TC_LOG_DEBUG("lfg", "Player: %u not ingame while retrieving his LockedDungeons.", GUID_LOPART(guid));
        return lock;
    }

    uint8 level = player->getLevel();
    uint8 expansion = player->GetSession()->Expansion();
    LfgDungeonSet const& dungeons = GetDungeonsByRandom(0);

    for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end(); ++it)
    {
        LFGDungeonData const* dungeon = GetLFGDungeon(*it);
        if (!dungeon) // should never happen - We provide a list from sLFGDungeonStore
            continue;

        LockData lockData;
        if (dungeon->expansion > expansion)
            lockData.status = LFG_LOCKSTATUS_INSUFFICIENT_EXPANSION;
        else if (dungeon->difficulty > REGULAR_DIFFICULTY && player->GetBoundInstance(dungeon->map, Difficulty(dungeon->difficulty)) && 
            !dungeon->dbc->IsScenario() && !dungeon->dbc->IsRaidFinder() && !dungeon->dbc->IsFlex())
            lockData.status = LFG_LOCKSTATUS_RAID_LOCKED;
        else if (dungeon->minlevel > level)
            lockData.status = LFG_LOCKSTATUS_TOO_LOW_LEVEL;
        else if (dungeon->maxlevel != 0 && dungeon->maxlevel < level)
            lockData.status = LFG_LOCKSTATUS_TOO_HIGH_LEVEL;
        else if (dungeon->seasonal && !IsSeasonActive(dungeon->id))
            lockData.status = LFG_LOCKSTATUS_NOT_IN_SEASON;
        // merge faction check with check on invalid TP pos and check on test invalid maps (BUT we still have to send it! LOL, in WoD blizz deleted invalid maps from client DBC)
        else if (!dungeon->dbc->FitsTeam(player->GetTeam()) || DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, dungeon->map, player)
            || (dungeon->type != LFG_TYPE_RANDOM && dungeon->x == 0.0f && dungeon->y == 0.0f && dungeon->z == 0.0f) || !dungeon->dbc->IsValid())
            // TODO: for non-faction check find correct reason
            lockData.status = LFG_LOCKSTATUS_WRONG_FACTION;
        else
        {
            AccessRequirement const* ar = sObjectMgr->GetAccessRequirement(dungeon->map, Difficulty(dungeon->difficulty), dungeon->id);
            if (!ar)
                ar = sObjectMgr->GetAccessRequirement(dungeon->map, Difficulty(dungeon->difficulty));
            if (ar)
            {
                uint32 avgItemLevel = player->GetAverageItemLevel();
                if (ar->item_level && avgItemLevel < ar->item_level)
                {
                    lockData.currItemLevel = avgItemLevel;
                    lockData.reqItemLevel = ar->item_level;
                    lockData.status = LFG_LOCKSTATUS_TOO_LOW_GEAR_SCORE;
                }
                else if (ar->achievement && !player->HasAchieved(ar->achievement))
                    lockData.status = LFG_LOCKSTATUS_MISSING_ACHIEVEMENT;
                else if (player->GetTeam() == ALLIANCE && ar->quest_A && !player->GetQuestRewardStatus(ar->quest_A))
                    lockData.status = LFG_LOCKSTATUS_QUEST_NOT_COMPLETED;
                else if (player->GetTeam() == HORDE && ar->quest_H && !player->GetQuestRewardStatus(ar->quest_H))
                    lockData.status = LFG_LOCKSTATUS_QUEST_NOT_COMPLETED;
                else
                {
                    if (ar->item)
                    {
                        if (!player->HasItemCount(ar->item) && (!ar->item2 || !player->HasItemCount(ar->item2)))
                            lockData.status = LFG_LOCKSTATUS_MISSING_ITEM;
                    }
                    else if (ar->item2 && !player->HasItemCount(ar->item2))
                        lockData.status = LFG_LOCKSTATUS_MISSING_ITEM;
                }
            }
        }

        /* @todo VoA closed if WG is not under team control (LFG_LOCKSTATUS_RAID_LOCKED)
        lockData = LFG_LOCKSTATUS_TOO_HIGH_GEAR_SCORE;
        lockData = LFG_LOCKSTATUS_ATTUNEMENT_TOO_LOW_LEVEL;
        lockData = LFG_LOCKSTATUS_ATTUNEMENT_TOO_HIGH_LEVEL;
        */

        if (lockData.status != LFG_LOCKSTATUS_OK)
            lock[dungeon->Entry()] = lockData;
    }

    return lock;
}

uint8 LFGMgr::GetKicksLeft(uint64 guid)
{
    uint8 kicks = GroupsStore[guid].GetKicksLeft();
    TC_LOG_TRACE("lfg", "LFGMgr::GetKicksLeft: [" UI64FMTD "] = %u", guid, kicks);
    return kicks;
}

void LFGMgr::RestoreState(uint64 guid, char const* debugMsg)
{
    if (IS_GROUP(guid))
    {
        LfgGroupData& data = GroupsStore[guid];
        if (sLog->ShouldLog("lfg", LOG_LEVEL_DEBUG))
        {
            std::string const& ps = GetStateString(data.GetState());
            std::string const& os = GetStateString(data.GetOldState());
            TC_LOG_TRACE("lfg", "LFGMgr::RestoreState: Group: [" UI64FMTD "] (%s) State: %s, oldState: %s",
                guid, debugMsg, ps.c_str(), os.c_str());
        }
        data.RestoreState();
    }
    else
    {
        LfgPlayerData& data = PlayersStore[guid];
        if (sLog->ShouldLog("lfg", LOG_LEVEL_DEBUG))
        {
            std::string const& ps = GetStateString(data.GetState());
            std::string const& os = GetStateString(data.GetOldState());
            TC_LOG_TRACE("lfg", "LFGMgr::RestoreState: Player: [" UI64FMTD "] (%s) State: %s, oldState: %s",
                guid, debugMsg, ps.c_str(), os.c_str());
        }
        data.RestoreState();
    }
}

void LFGMgr::SetState(uint64 guid, LfgState state)
{
    if (IS_GROUP(guid))
    {
        LfgGroupData& data = GroupsStore[guid];
        if (sLog->ShouldLog("lfg", LOG_LEVEL_TRACE))
        {
            std::string const& ns = GetStateString(state);
            std::string const& ps = GetStateString(data.GetState());
            std::string const& os = GetStateString(data.GetOldState());
            TC_LOG_TRACE("lfg", "LFGMgr::SetState: Group: [" UI64FMTD "] newState: %s, previous: %s, oldState: %s",
                guid, ns.c_str(), ps.c_str(), os.c_str());
        }
        data.SetState(state);
    }
    else
    {
        LfgPlayerData& data = PlayersStore[guid];
        if (sLog->ShouldLog("lfg", LOG_LEVEL_TRACE))
        {
            std::string const& ns = GetStateString(state);
            std::string const& ps = GetStateString(data.GetState());
            std::string const& os = GetStateString(data.GetOldState());
            TC_LOG_TRACE("lfg", "LFGMgr::SetState: Player: [" UI64FMTD "] newState: %s, previous: %s, oldState: %s",
                guid, ns.c_str(), ps.c_str(), os.c_str());
        }
        data.SetState(state);
    }
}

void LFGMgr::SetDungeon(uint64 guid, uint32 dungeon)
{
    TC_LOG_TRACE("lfg", "LFGMgr::SetDungeon: [" UI64FMTD "] dungeon %u", guid, dungeon);
    GroupsStore[guid].SetDungeon(dungeon);
}

void LFGMgr::ClearState(uint64 guid)
{
    TC_LOG_DEBUG("lfg", "LFGMgr::ClearState: [" UI64FMTD "]", guid);
    PlayersStore[guid].ClearState();
}

void LFGMgr::SetRoles(uint64 guid, uint8 roles)
{
    TC_LOG_TRACE("lfg", "LFGMgr::SetRoles: [" UI64FMTD "] roles: %u", guid, roles);
    PlayersStore[guid].SetRoles(roles);
}

void LFGMgr::SetComment(uint64 guid, std::string const& comment)
{
    TC_LOG_TRACE("lfg", "LFGMgr::SetComment: [" UI64FMTD "] comment: %s", guid, comment.c_str());
    PlayersStore[guid].SetComment(comment);
}

void LFGMgr::SetSelectedDungeons(uint64 guid, LfgDungeonSet const& dungeons)
{
    TC_LOG_TRACE("lfg", "LFGMgr::SetSelectedDungeons: [" UI64FMTD "] Dungeons: %s", guid, ConcatenateDungeons(dungeons).c_str());
    PlayersStore[guid].SetSelectedDungeons(dungeons);
}

void LFGMgr::DecreaseKicksLeft(uint64 guid)
{
    TC_LOG_TRACE("lfg", "LFGMgr::DecreaseKicksLeft: [" UI64FMTD "]", guid);
    GroupsStore[guid].DecreaseKicksLeft();
}

void LFGMgr::RemovePlayerData(uint64 guid)
{
    TC_LOG_TRACE("lfg", "LFGMgr::RemovePlayerData: [" UI64FMTD "]", guid);
    LfgPlayerDataContainer::iterator it = PlayersStore.find(guid);
    if (it != PlayersStore.end())
        PlayersStore.erase(it);
}

void LFGMgr::RemoveGroupData(uint64 guid)
{
    TC_LOG_TRACE("lfg", "LFGMgr::RemoveGroupData: [" UI64FMTD "]", guid);
    LfgGroupDataContainer::iterator it = GroupsStore.find(guid);
    if (it == GroupsStore.end())
        return;

    LfgState state = GetState(guid);
    // If group is being formed after proposal success do nothing more
    LfgGuidSet const& players = it->second.GetPlayers();
    for (LfgGuidSet::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        uint64 guid = (*it);
        SetGroup(*it, 0);
        if (state != LFG_STATE_PROPOSAL)
        {
            SetState(*it, LFG_STATE_NONE);
            SendLfgUpdateParty(guid, LfgUpdateData(LFG_UPDATETYPE_REMOVED_FROM_QUEUE, GetSelectedDungeons(guid)));
        }
    }
    GroupsStore.erase(it);
}

uint8 LFGMgr::GetTeam(uint64 guid)
{
    return PlayersStore[guid].GetTeam();
}

uint8 LFGMgr::RemovePlayerFromGroup(uint64 gguid, uint64 guid)
{
    return GroupsStore[gguid].RemovePlayer(guid);
}

void LFGMgr::AddPlayerToGroup(uint64 gguid, uint64 guid)
{
    GroupsStore[gguid].AddPlayer(guid);
}

void LFGMgr::SetLeader(uint64 gguid, uint64 leader)
{
    GroupsStore[gguid].SetLeader(leader);
}

void LFGMgr::SetTeam(uint64 guid, uint8 team)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP))
        team = 0;

    PlayersStore[guid].SetTeam(team);
}

uint64 LFGMgr::GetGroup(uint64 guid)
{
    return PlayersStore[guid].GetGroup();
}

void LFGMgr::SetGroup(uint64 guid, uint64 group)
{
    PlayersStore[guid].SetGroup(group);
}

LfgGuidSet const& LFGMgr::GetPlayers(uint64 guid)
{
    return GroupsStore[guid].GetPlayers();
}

uint8 LFGMgr::GetPlayerCount(uint64 guid)
{
    return GroupsStore[guid].GetPlayerCount();
}

uint64 LFGMgr::GetLeader(uint64 guid)
{
    return GroupsStore[guid].GetLeader();
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

void LFGMgr::SendLfgRoleCheckUpdate(uint64 guid, LfgRoleCheck const& roleCheck)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgRoleCheckUpdate(roleCheck);
}

void LFGMgr::SendLfgUpdatePlayer(uint64 guid, LfgUpdateData const& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgUpdatePlayer(data);
}

void LFGMgr::SendLfgUpdateParty(uint64 guid, LfgUpdateData const& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgUpdateParty(data);
}

void LFGMgr::SendLfgJoinResult(uint64 guid, LfgJoinResultData const& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgJoinResult(data);
}

void LFGMgr::SendLfgBootProposalUpdate(uint64 guid, LfgPlayerBoot const& boot)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgBootProposalUpdate(boot);
}

void LFGMgr::SendLfgUpdateProposal(uint64 guid, LfgProposal const& proposal)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgUpdateProposal(proposal);
}

void LFGMgr::SendLfgQueueStatus(uint64 guid, LfgQueueStatusData const& data)
{
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        player->GetSession()->SendLfgQueueStatus(data);
}

bool LFGMgr::IsLfgGroup(uint64 guid)
{
    return guid && IS_GROUP(guid) && GroupsStore[guid].IsLfgGroup();
}

LFGQueue& LFGMgr::GetQueue(uint64 guid)
{
    uint8 queueId = 0;
    if (IS_GROUP(guid))
    {
        LfgGuidSet const& players = GetPlayers(guid);
        uint64 pguid = players.empty() ? 0 : (*players.begin());
        if (pguid)
            queueId = GetTeam(pguid);
    }
    else
        queueId = GetTeam(guid);
    return QueuesStore[queueId];
}

bool LFGMgr::AllQueued(LfgGuidList const& check)
{
    if (check.empty())
        return false;

    for (LfgGuidList::const_iterator it = check.begin(); it != check.end(); ++it)
        if (GetState(*it) != LFG_STATE_QUEUED)
            return false;
    return true;
}

// Only for debugging purposes
void LFGMgr::Clean()
{
    QueuesStore.clear();
}

bool LFGMgr::isOptionEnabled(uint32 option)
{
    return m_options & option;
}

uint32 LFGMgr::GetOptions()
{
    return m_options;
}

void LFGMgr::SetOptions(uint32 options)
{
    m_options = options;
}

LfgUpdateData LFGMgr::GetLfgStatus(uint64 guid)
{
    LfgPlayerData& playerData = PlayersStore[guid];
    return LfgUpdateData(LFG_UPDATETYPE_UPDATE_STATUS, playerData.GetState(), playerData.GetSelectedDungeons());
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

std::string LFGMgr::DumpQueueInfo(bool full)
{
    uint32 size = uint32(QueuesStore.size());
    std::ostringstream o;

    o << "Number of Queues: " << size << "\n";
    for (LfgQueueContainer::const_iterator itr = QueuesStore.begin(); itr != QueuesStore.end(); ++itr)
    {
        std::string const& queued = itr->second.DumpQueueInfo();
        std::string const& compatibles = itr->second.DumpCompatibleInfo(full);
        o << queued << compatibles;
    }

    return o.str();
}

void LFGMgr::SetupGroupMember(uint64 guid, uint64 gguid)
{
    LfgDungeonSet dungeons;
    dungeons.insert(GetDungeon(gguid));
    SetSelectedDungeons(guid, dungeons);
    SetState(guid, GetState(gguid));
    SetGroup(guid, gguid);
    AddPlayerToGroup(gguid, guid);
}

bool LFGMgr::selectedRandomLfgDungeon(uint64 guid)
{
    if (GetState(guid) != LFG_STATE_NONE)
    {
        LfgDungeonSet const& dungeons = GetSelectedDungeons(guid);
        if (!dungeons.empty())
        {
             LFGDungeonData const* dungeon = GetLFGDungeon(*dungeons.begin());
             if (dungeon && (dungeon->type == LFG_TYPE_RANDOM || dungeon->seasonal))
                 return true;
        }
    }

    return false;
}

bool LFGMgr::inLfgDungeonMap(uint64 guid, uint32 map, Difficulty difficulty)
{
    if (!IS_GROUP(guid))
        guid = GetGroup(guid);

    if (uint32 dungeonId = GetDungeon(guid, true))
        if (LFGDungeonData const* dungeon = GetLFGDungeon(dungeonId))
            if (uint32(dungeon->map) == map && dungeon->difficulty == difficulty)
                return true;

    return false;
}

uint32 LFGMgr::GetLFGDungeonEntry(uint32 id)
{
    if (id)
        if (LFGDungeonData const* dungeon = GetLFGDungeon(id))
            return dungeon->Entry();

    return 0;
}

LfgDungeonSet LFGMgr::GetRewardableDungeons(uint8 level, uint8 expansion)
{
    LfgDungeonSet randomDungeons;
    for (LFGDungeonContainer::const_iterator itr = LfgDungeonStore.begin(); itr != LfgDungeonStore.end(); ++itr)
    {
        LFGDungeonData const& dungeon = itr->second;
        if (dungeon.dbc->CanBeRewarded() && (!dungeon.seasonal || sLFGMgr->IsSeasonActive(dungeon.id))
            && dungeon.expansion <= expansion && dungeon.minlevel <= level && level <= dungeon.maxlevel)
            randomDungeons.insert(dungeon.Entry());
    }
    return randomDungeons;
}

uint32 LFGMgr::GetBonusValorPoints(uint32 dungeonId) const
{
    switch (dungeonId & 0xFFFFF)
    {
        case 492:   // Greenstone Village
        case 539:   // Brewmoon Festival
        case 589:   // A Little Patience
        case 619:   // A Little Patience
            return 5;
        case 470:   // Shado-Pan Monastery
            return 15;
        case 472:   // Scholomance
            return 10;
        case 554:   // Siege of Niuzao Temple
            return 5;
        case 566:   // Theramore's Fall
        case 567:   // Theramore's Fall
            return 5;
        default:
            break;
    }

    return 0;
}

LfgRoleData::LfgRoleData(uint32 dungeonId)
{
    Init(sLFGMgr->GetLFGDungeon(dungeonId));
}

LfgRoleData::LfgRoleData(LFGDungeonData const* data)
{
    Init(data);
}

void LfgRoleData::Init(LFGDungeonData const* data)
{
    tanksNeeded = data->dbc->tankNeeded;
    healerNeeded = data->dbc->healerNeeded;
    dpsNeeded = data->dbc->dpsNeeded;

    minTanksNeeded = data->dbc->minTankNeeded;
    minHealerNeeded = data->dbc->minHealerNeeded;
    minDpsNeeded = data->dbc->minDpsNeeded;
}

bool LfgReward::RewardPlayer(Player* player, LFGDungeonData const* randomDungeon, LFGDungeonData const* normalDungeon, bool bonusObjective) const
{
    bool done = false;
    Quest const* quest = sObjectMgr->GetQuestTemplate(bonusObjective ? bonusQuestId : firstQuest);
    if (!quest)
        return false;

    // if we can take the quest, means that we haven't done this kind of "run", IE: First Heroic Random of Day.
    if (player->CanRewardQuest(quest, false))
    {
        player->RewardQuest(quest, 0, NULL, false);

        // reward lfg bonus reputation on first completion
        if (uint32 bonusRep = randomDungeon && !bonusObjective ? randomDungeon->dbc->bonusRepAmt : 0)
        {
            if (uint32 faction = player->GetLfgBonusFaction())
                player->GetReputationMgr().ModifyReputation(sFactionStore.LookupEntry(faction), bonusRep);
        }
    }
    else if (!bonusObjective)
    {
        done = true;
        // we give reward without informing client (retail does this)
        if (quest = sObjectMgr->GetQuestTemplate(otherQuest))
            player->RewardQuest(quest, 0, NULL, false);
    }

    // reward bonus valor points for certain dungeons
    if (uint32 bonusValor = !bonusObjective && normalDungeon && normalDungeon != randomDungeon ? sLFGMgr->GetBonusValorPoints(normalDungeon->id) : 0)
        player->ModifyCurrency(CURRENCY_TYPE_VALOR_POINTS, bonusValor * GetCurrencyPrecision(CURRENCY_TYPE_VALOR_POINTS));

    return done;
}

uint8 LFGMgr::GetVotesNeededForKick(uint64 gguid)
{
    LFGDungeonData const* dungeonData = GetLFGDungeon(GetDungeon(gguid, true));
    if (!dungeonData)
        return LFG_DUNGEON_KICK_VOTES_NEEDED;

    switch (dungeonData->dbc->GetInternalType())
    {
        case LFG_TYPE_SCENARIO:
            return LFG_SCENARIO_KICK_VOTES_NEEDED;
        case LFG_TYPE_RAID:
            return LFG_RAID_KICK_VOTES_NEEDED;
        default:
            break;
    }

    return LFG_DUNGEON_KICK_VOTES_NEEDED;
}

} // namespace lfg
