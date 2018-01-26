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

#include "WorldSession.h"
#include "WorldPacket.h"
#include "DBCStores.h"
#include "Player.h"
#include "Group.h"
#include "LFGMgr.h"
#include "ObjectMgr.h"
#include "GroupMgr.h"
#include "LFGQueue.h"

void WorldSession::HandleLfgJoinOpcode(WorldPacket& recvData)
{
    uint32 numDungeons;
    uint32 roles;
    uint8 length = 0;
    uint8 unk8 = 0;

    recvData >> roles;
    recvData >> unk8;               // 127 only
    for (int i = 0; i < 3; ++i)
        recvData.read_skip<uint32>();
    numDungeons = recvData.ReadBits(22);
    recvData.ReadBit();             // 1 only
    length = recvData.ReadBits(8);
    lfg::LfgDungeonSet newDungeons;
    for (uint32 i = 0; i < numDungeons; ++i)
        newDungeons.insert(recvData.read<uint32>() & 0xFFFFF);  // received with dungeon type mask

    std::string comment = recvData.ReadString(length);

    if (!numDungeons)
    {
        TC_LOG_DEBUG("lfg", "CMSG_LFG_JOIN %s no dungeons selected", GetPlayerName().c_str());
        return;
    }

    if (!sLFGMgr->isOptionEnabled(lfg::LFG_OPTION_ENABLE_DUNGEON_FINDER | lfg::LFG_OPTION_ENABLE_RAID_BROWSER))
        return;

    if (Group* group = _player->GetGroup())
    for (lfg::LfgDungeonSet::const_iterator itr = newDungeons.begin(); itr != newDungeons.end(); ++itr)
    {
        lfg::LFGDungeonData const * entry = sLFGMgr->GetLFGDungeon(*newDungeons.begin() & 0xFFFFF, _player->GetTeam());
        if (!entry)
            return;

        // old wtf checks
        /*
        uint8 type = LFG_TYPE_DUNGEON;
        uint8 maxGroupSize = 5;
        if (entry != NULL)
            type = entry->difficulty == RAID_TOOL_DIFFICULTY ? LFG_TYPE_RAID : entry->IsScenario() ? LFG_TYPE_SCENARIO : LFG_TYPE_DUNGEON;
        if (type == LFG_TYPE_RAID)
            maxGroupSize = 25;
        if (type == LFG_TYPE_SCENARIO)
            maxGroupSize = 3;

        if (!sWorld->getBoolConfig(CONFIG_DUNGEON_FINDER_ENABLE) ||
        (GetPlayer()->GetGroup() && GetPlayer()->GetGroup()->GetLeaderGUID() != GetPlayer()->GetGUID() &&
        (GetPlayer()->GetGroup()->GetMembersCount() == maxGroupSize || !GetPlayer()->GetGroup()->isLFGGroup())))
        {
            return;
        }
        */
    }

    TC_LOG_DEBUG("lfg", "CMSG_LFG_JOIN %s roles: %u, Dungeons: %u, Comment: %s",
        GetPlayerName().c_str(), roles, uint8(newDungeons.size()), comment.c_str());
    sLFGMgr->JoinLfg(GetPlayer(), uint8(roles), newDungeons, comment);
}

void WorldSession::HandleLfgLeaveOpcode(WorldPacket&  recvData)
{
    recvData.rfinish();

    Group* group = GetPlayer()->GetGroup();
    uint64 guid = GetPlayer()->GetGUID();
    uint64 gguid = group ? group->GetGUID() : guid;

    TC_LOG_DEBUG("lfg", "CMSG_LFG_LEAVE %s in group: %u",
        GetPlayerName().c_str(), group ? 1 : 0);

    // Check cheating - only leader can leave the queue
    if (!group || group->GetLeaderGUID() == GetPlayer()->GetGUID())
        sLFGMgr->LeaveLfg(gguid);
}

void WorldSession::HandleLfgProposalResultOpcode(WorldPacket& recvData)
{
    uint32 proposalId;                                      // proposal id
    bool accept;                                            // Accept to join?

    recvData >> proposalId;
    recvData.read_skip<uint32>();                           // group id
    recvData.read_skip<uint32>();                           // time
    recvData.read_skip<uint32>();                           // 3

    ObjectGuid playerGuid;
    ObjectGuid instanceGuid;

    recvData.ReadGuidMask<0, 7, 5, 1>(instanceGuid);
    recvData.ReadGuidMask<2, 6, 0, 5, 7, 1>(playerGuid);
    recvData.ReadGuidMask<2>(instanceGuid);
    recvData.ReadGuidMask<3>(playerGuid);
    recvData.ReadGuidMask<6, 3, 4>(instanceGuid);
    recvData.ReadGuidMask<4>(playerGuid);
    accept = recvData.ReadBit();

    recvData.ReadGuidBytes<6, 1, 2, 4>(instanceGuid);
    recvData.ReadGuidBytes<0, 5, 1>(playerGuid);
    recvData.ReadGuidBytes<3>(instanceGuid);
    recvData.ReadGuidBytes<7, 6, 4, 2>(playerGuid);
    recvData.ReadGuidBytes<7, 5>(instanceGuid);
    recvData.ReadGuidBytes<3>(playerGuid);
    recvData.ReadGuidBytes<0>(instanceGuid);

    TC_LOG_DEBUG("lfg", "CMSG_LFG_PROPOSAL_RESULT %s proposal: %u accept: %u",
        GetPlayerName().c_str(), proposalId, accept ? 1 : 0);
    sLFGMgr->UpdateProposal(proposalId, GetPlayer()->GetGUID(), accept);
}

void WorldSession::HandleLfgSetRolesOpcode(WorldPacket& recvData)
{
    uint32 roles;
    uint8 unk;
    recvData >> roles;                                    // Player Group Roles
    recvData >> unk;

    uint64 guid = GetPlayer()->GetGUID();
    Group* group = GetPlayer()->GetGroup();
    if (!group)
    {
        TC_LOG_DEBUG("lfg", "CMSG_LFG_SET_ROLES %s Not in group",
            GetPlayerName().c_str());
        return;
    }
    uint64 gguid = group->GetGUID();
    TC_LOG_DEBUG("lfg", "CMSG_LFG_SET_ROLES: Group %u, Player %s, Roles: %u",
        GUID_LOPART(gguid), GetPlayerName().c_str(), roles);
    sLFGMgr->UpdateRoleCheck(gguid, guid, roles);
}

void WorldSession::HandleLfgSetCommentOpcode(WorldPacket&  recvData)
{
    ObjectGuid guid;
    recvData.read_skip<uint32>();
    recvData.read_skip<uint32>();
    recvData.read_skip<uint32>();
    recvData.ReadGuidMask<1, 4, 3>(guid);
    uint32 len = recvData.ReadBits(8);
    recvData.ReadGuidMask<6, 2, 5, 0, 7>(guid);

    recvData.ReadGuidBytes<3, 7>(guid);
    std::string comment = recvData.ReadString(len);
    recvData.ReadGuidBytes<0, 5, 4, 6, 1, 2>(guid);

    TC_LOG_DEBUG("lfg", "CMSG_LFG_SET_COMMENT %s comment: %s",
        GetPlayerName().c_str(), comment.c_str());

    sLFGMgr->SetComment(GetPlayer()->GetGUID(), comment);
}

void WorldSession::HandleLfgSetBootVoteOpcode(WorldPacket& recvData)
{
    bool agree = recvData.ReadBit();                            // Agree to kick player

    uint64 guid = GetPlayer()->GetGUID();
    TC_LOG_DEBUG("lfg", "CMSG_LFG_SET_BOOT_VOTE %s agree: %u",
        GetPlayerName().c_str(), agree ? 1 : 0);
    sLFGMgr->UpdateBoot(guid, agree);
}

void WorldSession::HandleLfgTeleportOpcode(WorldPacket& recvData)
{
    bool out = recvData.ReadBit();

    TC_LOG_DEBUG("lfg", "CMSG_LFG_TELEPORT %s out: %u",
        GetPlayerName().c_str(), out ? 1 : 0);
    sLFGMgr->TeleportPlayer(GetPlayer(), out, true);
}

void WorldSession::HandleLfgPlayerLockInfoRequestOpcode(WorldPacket& recvData)
{
    recvData.rfinish();
    uint64 guid = GetPlayer()->GetGUID();
    TC_LOG_DEBUG("lfg", "CMSG_LFG_PLAYER_LOCK_INFO_REQUEST %s",
        GetPlayerName().c_str());

    // Get Random dungeons that can be done at a certain level and expansion
    uint8 level = GetPlayer()->getLevel();
    lfg::LfgDungeonSet const& rewardableDungeons =
        sLFGMgr->GetRewardableDungeons(level, GetPlayer()->GetSession()->Expansion());

    // Get player locked Dungeons
    lfg::LfgLockMap const& lock = sLFGMgr->GetLockedDungeons(guid);
    uint32 rsize = uint32(rewardableDungeons.size());
    uint32 lsize = uint32(lock.size());

    TC_LOG_DEBUG("lfg", "SMSG_LFG_PLAYER_INFO %s", GetPlayerName().c_str());
    WorldPacket data(SMSG_LFG_PLAYER_INFO, 1 + rsize * (4 + 1 + 4 + 4 + 4 + 4 + 1 + 4 + 4 + 4) + 4 + lsize * (1 + 4 + 4 + 4 + 4 + 1 + 4 + 4 + 4));

    uint32 rewardableDungeonsCount = 0;
    data.WriteBit(0);                               // not has guid
    data.WriteBits(lock.size(), 20);
    uint32 bitpos = data.bitwpos();
    data.WriteBits(rewardableDungeonsCount, 17);    // Rewardable Dungeon count

    ByteBuffer buff;
    for (lfg::LfgDungeonSet::const_iterator it = rewardableDungeons.begin(); it != rewardableDungeons.end(); ++it)
    {
        lfg::LfgReward const* reward = sLFGMgr->GetDungeonReward(*it, level);
        if (!reward)
            continue;

        uint32 bonusValor = sLFGMgr->GetBonusValorPoints(*it);
        Quest const* rewardQuest = sObjectMgr->GetQuestTemplate(reward->firstQuest);
        Quest const* bonusQuest = sObjectMgr->GetQuestTemplate(reward->bonusQuestId);

        bool firstCompletion = rewardQuest ? GetPlayer()->CanRewardQuest(rewardQuest, false) : true;
        if (!firstCompletion)
            rewardQuest = sObjectMgr->GetQuestTemplate(reward->otherQuest);

        uint32 rewardItemCount = 0;
        uint32 rewardCurrencyCount1 = 0;
        uint32 rewardCurrencyCount2 = 0;

        data.WriteBits(rewardQuest ? rewardQuest->GetRewItemsCount() : 0, 20);
        data.WriteBits((bonusQuest ? bonusQuest->GetRewCurrencyCount() : 0) + (bonusValor ? 1 : 0), 21);
        data.WriteBit(firstCompletion); // can be rewarded
        data.WriteBits(rewardQuest ? rewardQuest->GetRewCurrencyCount() : 0, 21);
        data.WriteBits(0, 19);          // role bonus count
        data.WriteBit(1);               // eligible to role shortage reward

        buff << uint32(0);              // cap currency purse Quantity
        buff << uint32(*it);            // dungeon entry
        buff << uint32(0);              // cap currency specific quantity

        if (bonusQuest)
        {
            for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
                if (uint32 cur = bonusQuest->RewardCurrencyId[i])
                {
                    buff << uint32(bonusQuest->RewardCurrencyCount[i] * GetCurrencyPrecision(cur));
                    buff << uint32(cur);
                }
        }
        if (bonusValor)
        {
            buff << uint32(bonusValor * GetCurrencyPrecision(CURRENCY_TYPE_VALOR_POINTS));
            buff << uint32(CURRENCY_TYPE_VALOR_POINTS);
        }

        if (rewardQuest)
        {
            for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            {
                uint32 itemId = rewardQuest->RewardItemId[i];
                if (!itemId)
                    continue;

                ItemTemplate const* iProto = sObjectMgr->GetItemTemplate(rewardQuest->RewardItemId[i]);
                buff << uint32(iProto ? iProto->DisplayInfoID : 0);
                buff << uint32(rewardQuest->RewardItemIdCount[i]);
                buff << uint32(itemId);
            }
        }

        buff << uint32(0);      // cap currency Quantity
        buff << uint32(0);      // cap currencyID
        buff << uint32(0);      // cap currency overall Limit
        buff << uint32(rewardQuest ? rewardQuest->XPValue(GetPlayer()) : 0);
        buff << uint32(0);
        buff << uint32(0);      // cap currency overall Quantity
        buff << uint32(0);      // cap currency period Purse Limit
        buff << uint32(0);      // completed encounters mask
        buff << uint32(rewardQuest ? rewardQuest->GetRewOrReqMoney() : 0);

        if (rewardQuest)
        {
            for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
                if (uint32 cur = rewardQuest->RewardCurrencyId[i])
                {
                    buff << uint32(cur);
                    buff << uint32(rewardQuest->RewardCurrencyCount[i] * GetCurrencyPrecision(cur));
                }
        }

        buff << uint32(0);
        buff << uint32(0);      // cap currency purse Limit
        buff << uint32(0);      // cap currency period Purse Quantity
        buff << uint32(0);
        buff << uint32(0);      // cap currency specific Limit

        ++rewardableDungeonsCount;
    }

    data.FlushBits();
    if (!buff.empty())
        data.append(buff);

    data.PutBits(bitpos, rewardableDungeonsCount, 17);

    for (lfg::LfgLockMap::const_iterator it = lock.begin(); it != lock.end(); ++it)
    {
        data << uint32(it->second.currItemLevel);           // curr player item level
        data << uint32(it->second.reqItemLevel);            // req player item level
        data << uint32(it->second.status);                  // Lock status
        data << uint32(it->first);                          // Dungeon entry (id + type)
    }

    SendPacket(&data);
}

void WorldSession::HandleLfgPartyLockInfoRequestOpcode(WorldPacket&  /*recvData*/)
{
    uint64 guid = GetPlayer()->GetGUID();
    TC_LOG_DEBUG("lfg", "CMSG_LFG_PARTY_LOCK_INFO_REQUEST %s", GetPlayerName().c_str());
}

void WorldSession::HandleLfrJoinOpcode(WorldPacket& recv_data)
{
    uint32 entry;                                          // Raid id to search
    recv_data >> entry;
     TC_LOG_DEBUG("lfg", "CMSG_LFG_LFR_JOIN %s dungeon entry: %u",
        GetPlayerName().c_str(), entry);
    //SendLfrUpdateListOpcode(entry);
}

void WorldSession::HandleLfrLeaveOpcode(WorldPacket& recvData)
{
    uint32 dungeonId;                                      // Raid id queue to leave
    recvData >> dungeonId;
    TC_LOG_DEBUG("lfg", "CMSG_LFG_LFR_LEAVE %s dungeonId: %u",
        GetPlayerName().c_str(), dungeonId);
    //sLFGMgr->LeaveLfr(GetPlayer(), dungeonId);
}

void WorldSession::HandleLfgGetStatus(WorldPacket& /*recvData*/)
{
    uint64 guid = GetPlayer()->GetGUID();
    TC_LOG_DEBUG("lfg", "CMSG_LFG_GET_STATUS %s", GetPlayerName().c_str());

    TC_LOG_DEBUG("lfg", "SMSG_LFG_GET_STATUS %s", GetPlayerName().c_str());
    lfg::LfgUpdateData updateData = sLFGMgr->GetLfgStatus(guid);

    if (GetPlayer()->GetGroup())
    {
        SendLfgUpdateParty(updateData);
        updateData.dungeons.clear();
        SendLfgUpdatePlayer(updateData);
    }
    else
    {
        SendLfgUpdatePlayer(updateData);
        updateData.dungeons.clear();
        SendLfgUpdateParty(updateData);
    }
}

void WorldSession::SendLfgUpdatePlayer(lfg::LfgUpdateData const& updateData)
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_UPDATE_PLAYER %s updatetype: %u",
        GetPlayerName().c_str(), updateData.updateType);
    sLFGMgr->SendUpdateStatus(GetPlayer(), updateData, false);
}

void WorldSession::SendLfgUpdateParty(lfg::LfgUpdateData const& updateData)
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_UPDATE_PARTY %s updatetype: %u",
        GetPlayerName().c_str(), updateData.updateType);
    sLFGMgr->SendUpdateStatus(GetPlayer(), updateData, true);
}

void WorldSession::SendLfgRoleChosen(uint64 guid, uint8 roles)
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_ROLE_CHOSEN %s guid: %u roles: %u",
        GetPlayerName().c_str(), GUID_LOPART(guid), roles);

    WorldPacket data(SMSG_LFG_ROLE_CHOSEN, 8 + 1 + 1 + 4);
    data.WriteGuidMask<0, 6, 1, 4, 7, 3>(guid);
    data.WriteBit(roles != 0);                             // Ready
    data.WriteGuidMask<2, 5>(guid);

    data.WriteGuidBytes<2, 4, 5, 1, 3, 0>(guid);
    data << uint32(roles);                                 // Roles
    data.WriteGuidBytes<7, 6>(guid);

    SendPacket(&data);
}

void WorldSession::SendLfgRoleCheckUpdate(lfg::LfgRoleCheck const& roleCheck)
{
    bool updateAll = roleCheck.state == lfg::LFG_ROLECHECK_FINISHED || roleCheck.state == lfg::LFG_ROLECHECK_NO_ROLE;

    lfg::LfgDungeonSet dungeons;
    if (roleCheck.rDungeonId)
        dungeons.insert(roleCheck.rDungeonId);
    else
        dungeons = roleCheck.dungeons;

    TC_LOG_DEBUG("lfg", "SMSG_LFG_ROLE_CHECK_UPDATE %s", GetPlayerName().c_str());

    ByteBuffer buff;
    ObjectGuid guid = _player->GetObjectGuid();
    WorldPacket data(SMSG_LFG_ROLE_CHECK_UPDATE, 4 + 1 + 1 + dungeons.size() * 4 + 1 + roleCheck.roles.size() * (8 + 1 + 4 + 1));
    data.WriteBit(roleCheck.state == lfg::LFG_ROLECHECK_INITIALITING);
    data.WriteGuidMask<0>(guid);
    data.WriteBits(updateAll ? roleCheck.roles.size() : 1, 21);
    data.WriteGuidMask<6, 4>(guid);

    if (!roleCheck.roles.empty())
    {
        // Player info MUST be sent 1st :S
        ObjectGuid memberGuid = guid;
        uint8 roles = roleCheck.roles.find(guid)->second;
        Player* player = ObjectAccessor::FindPlayer(guid);

        data.WriteBit(roles != 0);
        data.WriteGuidMask<5, 6, 0, 7, 2, 1, 4, 3>(guid);

        buff.WriteGuidBytes<4, 6, 3, 0>(guid);
        buff << uint32(roles);                                      // Roles
        buff.WriteGuidBytes<5, 1, 2>(guid);
        buff << uint8(player ? player->getLevel() : 0);             // Level
        buff.WriteGuidBytes<7>(guid);

        for (lfg::LfgRolesMap::const_reverse_iterator it = roleCheck.roles.rbegin(); it != roleCheck.roles.rend(); ++it)
        {
            if (it->first == GetPlayer()->GetGUID() || !updateAll)
                continue;

            memberGuid = it->first;
            roles = it->second;
            player = ObjectAccessor::FindPlayer(guid);

            data.WriteBit(roles != 0);
            data.WriteGuidMask<5, 6, 0, 7, 2, 1, 4, 3>(guid);

            buff.WriteGuidBytes<4, 6, 3, 0>(guid);
            buff << uint32(roles);                                      // Roles
            buff.WriteGuidBytes<5, 1, 2>(guid);
            buff << uint8(player ? player->getLevel() : 0);             // Level
            buff.WriteGuidBytes<7>(guid);
        }
    }

    data.WriteGuidMask<3>(guid);
    data.WriteBits(dungeons.size(), 22);
    data.WriteGuidMask<1, 2, 7, 5>(guid);

    if (!buff.empty())
    {
        data.FlushBits();
        data.append(buff);
    }

    data.WriteGuidBytes<1>(guid);
    for (lfg::LfgDungeonSet::iterator it = dungeons.begin(); it != dungeons.end(); ++it)
        data << uint32(sLFGMgr->GetLFGDungeonEntry(*it)); // Dungeon

    data.WriteGuidBytes<4, 7>(guid);
    data << uint8(roleCheck.state);
    data.WriteGuidBytes<6, 2, 3, 0, 5>(guid);
    data << uint8(1);                                       // roles

    SendPacket(&data);
}

void WorldSession::SendLfgJoinResult(lfg::LfgJoinResultData const& joinData)
{
    uint32 size = 0;
    for (lfg::LfgLockPartyMap::const_iterator it = joinData.lockmap.begin(); it != joinData.lockmap.end(); ++it)
        size += 8 + 3 + 1 + uint32(it->second.size()) * 4 * 4;

    TC_LOG_DEBUG("lfg", "SMSG_LFG_JOIN_RESULT %s checkResult: %u checkValue: %u",
        GetPlayerName().c_str(), joinData.result, joinData.state);
    ObjectGuid guid = GetPlayer()->GetObjectGuid();
    lfg::LfgLockPartyMap const& lockMap = joinData.lockmap;

    WorldPacket data(SMSG_LFG_JOIN_RESULT, 3 + 4 * 3 + 8 + 1 + 1 + size);
    data.WriteGuidMask<2, 6, 4, 1, 0, 3>(guid);

    data.WriteBits(lockMap.size(), 22);
    for (lfg::LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
    {
        ObjectGuid playerGuid = it->first;      // Player guid

        data.WriteGuidMask<1, 7, 6, 2, 3, 4, 0>(playerGuid);
        data.WriteBits(it->second.size(), 20);  // Size of lock dungeons
        data.WriteGuidMask<5>(playerGuid);
    }
    data.WriteGuidMask<7, 5>(guid);

    data << uint32(time(NULL));

    for (lfg::LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
    {
        ObjectGuid playerGuid = it->first;      // Player guid
        lfg::LfgLockMap const& lockMap2 = it->second;

        for (lfg::LfgLockMap::const_iterator itr = lockMap2.begin(); itr != lockMap2.end(); ++itr)
        {
            data << uint32(itr->first);                     // Dungeon entry (id + type)
            data << uint32(itr->second.reqItemLevel);       // needed item level
            data << uint32(itr->second.currItemLevel);      // current item level
            data << uint32(itr->second.status);             // Lock status
        }

        data.WriteGuidBytes<6, 5, 7, 1, 4, 0, 3, 2>(playerGuid);
    }
    data.WriteGuidBytes<7>(guid);
    data << uint32(_player->GetTeam());                     // group id
    data << uint32(3);                                      // lfg queue id?
    //data << uint8(joinData.result == LFG_JOIN_OK ? 0 : joinData.result);    // Check Result
    data << uint8(joinData.result);                         // Check Result
    data.WriteGuidBytes<0, 5, 1, 6, 3>(guid);
    data << uint8(joinData.state);                          // Check Value
    data.WriteGuidBytes<4, 2>(guid);

    SendPacket(&data);
}

void WorldSession::SendLfgQueueStatus(lfg::LfgQueueStatusData const& queueData)
{
    lfg::LfgQueueData* queueInfo = queueData.queueInfo;

    Player* player = GetPlayer();
    uint64 guid = player->GetGUID();
    TC_LOG_DEBUG("lfg", "SMSG_LFG_QUEUE_STATUS %s dungeon: %u, waitTime: %d, "
        "avgWaitTime: %d, waitTimeTanks: %d, waitTimeHealer: %d, waitTimeDps: %d, "
        "queuedTime: %u, tanks: %u, healers: %u, dps: %u",
        GetPlayerName().c_str(), queueData.dungeonId, queueData.waitTime, queueData.waitTimeAvg,
        queueData.waitTimeTank, queueData.waitTimeHealer, queueData.waitTimeDps,
        queueData.queuedTime, queueInfo->tanks, queueInfo->healers, queueInfo->dps);

    WorldPacket data(SMSG_LFG_QUEUE_STATUS, 4 + 4 + 4 + 4 + 4 +4 + 1 + 1 + 1 + 4);
    data.WriteGuidMask<0, 3, 6, 2, 4, 7, 5, 1>(guid);
    data.WriteGuidBytes<7>(guid);
    data << uint32(queueData.queuedTime);                   // Player wait time in queue
    data << uint32(time(NULL) - queueData.queuedTime);      // join time
    data << uint32(3);
    data << int32(queueData.waitTimeTank);                  // Wait Tanks
    data << uint8(queueInfo->tanks);                        // Tanks needed
    data << int32(queueData.waitTimeHealer);                // Wait Healers
    data << uint8(queueInfo->healers);                      // Healers needed
    data << int32(queueData.waitTimeDps);                   // Wait Dps
    data << uint8(queueInfo->dps);                          // Dps needed
    data << uint32(GetPlayer()->GetTeam());                 // group id
    data.WriteGuidBytes<2, 4>(guid);
    data << int32(queueData.waitTime);                      // Wait Time
    data << int32(queueData.waitTimeAvg);                   // Average Wait time
    data.WriteGuidBytes<6, 5, 0>(guid);
    data << uint32(sLFGMgr->GetLFGDungeonEntry(queueData.dungeonId));   // Dungeon
    data.WriteGuidBytes<3, 1>(guid);

    SendPacket(&data);
}

void WorldSession::SendLfgPlayerReward(lfg::LfgPlayerRewardData const& rewardData)
{
    if (!rewardData.rdungeonEntry || !rewardData.sdungeonEntry)
        return;

    Quest const* quest = NULL;
    if (rewardData.bonus)
        quest = sObjectMgr->GetQuestTemplate(rewardData.reward->bonusQuestId);
    else if (rewardData.done)
        quest = sObjectMgr->GetQuestTemplate(rewardData.reward->otherQuest);
    else
        quest = sObjectMgr->GetQuestTemplate(rewardData.reward->firstQuest);

    if (!quest)
        return;

    uint8 itemCount = quest->GetRewItemsCount() + quest->GetRewCurrencyCount();

    TC_LOG_DEBUG("lfg", "SMSG_LFG_PLAYER_REWARD %s rdungeonEntry: %u, sdungeonEntry: %u",
        GetPlayerName().c_str(), rewardData.rdungeonEntry, rewardData.sdungeonEntry);

    uint32 bonusValor = 0;

    // dungeons have bonus valor points only when queued by random
    if (!rewardData.done && !rewardData.bonus && rewardData.rdungeonEntry != rewardData.sdungeonEntry)
        bonusValor = sLFGMgr->GetBonusValorPoints(rewardData.sdungeonEntry);

    WorldPacket data(SMSG_LFG_PLAYER_REWARD, 4 * 4 + itemCount + itemCount * 4 * 4);
    data.WriteBits(itemCount, 20);
    for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        if (quest->RewardItemId[i])
            data.WriteBit(0);
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
        if (quest->RewardCurrencyId[i])
            data.WriteBit(1);
    for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
    {
        uint32 itemId = quest->RewardItemId[i];
        if (!itemId)
            continue;

        ItemTemplate const* iProto = sObjectMgr->GetItemTemplate(itemId);

        data << uint32(quest->RewardItemIdCount[i]);
        data << uint32(itemId);
        data << uint32(iProto ? iProto->DisplayInfoID : 0);
        data << uint32(0);
    }
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        uint32 cur = quest->RewardCurrencyId[i];
        if (!cur)
            continue;

        uint32 add = cur == CURRENCY_TYPE_VALOR_POINTS ? bonusValor : 0;
        // add only once
        if (add)
            bonusValor = 0;

        data << uint32((quest->RewardCurrencyCount[i] + add) * GetCurrencyPrecision(cur));
        data << uint32(cur);
        data << uint32(0);
        data << uint32(add);                               // bonus valor points
    }

    data << uint32(quest->XPValue(GetPlayer()));
    data << uint32(quest->GetRewOrReqMoney());
    data << uint32(rewardData.rdungeonEntry);              // Random Dungeon Finished
    data << uint32(rewardData.sdungeonEntry);              // Dungeon Finished

    SendPacket(&data);
}

void WorldSession::SendLfgBootProposalUpdate(lfg::LfgPlayerBoot const& boot)
{
    uint64 guid = GetPlayer()->GetGUID();
    lfg::LfgAnswer playerVote = boot.votes.find(guid)->second;
    uint8 votesNum = 0;
    uint8 agreeNum = 0;
    uint32 secsleft = uint8((boot.cancelTime - time(NULL)) / 1000);
    for (lfg::LfgAnswerContainer::const_iterator it = boot.votes.begin(); it != boot.votes.end(); ++it)
    {
        if (it->second != lfg::LFG_ANSWER_PENDING)
        {
            ++votesNum;
            if (it->second == lfg::LFG_ANSWER_AGREE)
                ++agreeNum;
        }
    }
    TC_LOG_DEBUG("lfg", "SMSG_LFG_BOOT_PROPOSAL_UPDATE %s inProgress: %u - "
        "didVote: %u - agree: %u - victim: %u votes: %u - agrees: %u - left: %u - "
        "needed: %u - reason %s",
        GetPlayerName().c_str(), uint8(boot.inProgress), uint8(playerVote != lfg::LFG_ANSWER_PENDING),
        uint8(playerVote == lfg::LFG_ANSWER_AGREE), GUID_LOPART(boot.victim), votesNum, agreeNum,
        secsleft, boot.votesNeeded, boot.reason.c_str());

    ObjectGuid victimGuid = boot.victim;

    WorldPacket data(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 8 + 1 + 1 + 4 + 4 + 4 + 4 + 1 + boot.reason.size());
    // TODO: maybe my vote and agree need to be swapped
    data.WriteGuidMask<4, 5, 7>(victimGuid);
    data.WriteBit(0);                                       // My vote
    data.WriteGuidMask<2>(victimGuid);
    data.WriteBit(playerVote == lfg::LFG_ANSWER_AGREE);     // Agree
    data.WriteGuidMask<6>(victimGuid);
    data.WriteBit(!boot.reason.size());
    data.WriteGuidMask<1>(victimGuid);
    data.WriteBit(boot.inProgress);                         // Vote in progress
    if (uint32 len = boot.reason.size())
        data.WriteBits(len, 8);
    data.WriteGuidMask<0, 3>(victimGuid);
    data.WriteBit(playerVote != lfg::LFG_ANSWER_PENDING);   // Did Vote

    data.WriteGuidBytes<5, 7>(victimGuid);
    if (boot.reason.size())
        data.WriteString(boot.reason);                      // Kick reason
    data.WriteGuidBytes<2>(victimGuid);
    data << uint32(votesNum);                               // Total Votes
    data.WriteGuidBytes<6, 4, 3>(victimGuid);
    data << uint32(boot.votesNeeded);                       // Needed Votes
    data.WriteGuidBytes<1>(victimGuid);
    data << uint32(agreeNum);                               // Agree Count
    data.WriteGuidBytes<0>(victimGuid);
    data << uint32(secsleft);                               // Time Left
    SendPacket(&data);
}

void WorldSession::SendLfgUpdateProposal(lfg::LfgProposal const& proposal)
{
    uint64 guid = GetPlayer()->GetGUID();
    uint64 gguid = proposal.players.find(guid)->second.group;
    bool silent = !proposal.isNew && gguid == proposal.group;
    uint32 dungeonEntry = proposal.dungeonId;

    TC_LOG_DEBUG("lfg", "SMSG_LFG_PROPOSAL_UPDATE %s state: %u",
        GetPlayerName().c_str(), proposal.state);

    // show random dungeon if player selected random dungeon and it's not lfg group
    if (!silent)
    {
        lfg::LfgDungeonSet const& playerDungeons = sLFGMgr->GetSelectedDungeons(guid);
        if (!playerDungeons.empty() && playerDungeons.find(proposal.dungeonId) == playerDungeons.end())
            dungeonEntry = (*playerDungeons.begin());
    }

    dungeonEntry = sLFGMgr->GetLFGDungeonEntry(dungeonEntry);

    ObjectGuid playerGUID = guid;
    ObjectGuid InstanceSaveGUID = MAKE_NEW_GUID(dungeonEntry, 0, HIGHGUID_INSTANCE_SAVE);

    WorldPacket data(SMSG_LFG_PROPOSAL_UPDATE, 4 + 1 + 4 + 4 + 1 + 1 + proposal.players.size() * (4 + 1 + 1 + 1 + 1 +1));
    data << uint32(dungeonEntry);                           // Dungeon
    data << uint8(proposal.state);                          // Result states
    data << uint32(proposal.encounters);                    // Bosses killed
    data << uint32(time(NULL));                             // Date
    data << uint32(proposal.id);                            // Proposal Id
    data << uint32(3);                                      // queue id
    data << uint32(GetPlayer()->GetTeam());                 // group Id?

    data.WriteGuidMask<5, 7>(InstanceSaveGUID);
    data.WriteGuidMask<2, 4>(playerGUID);
    data.WriteGuidMask<6>(InstanceSaveGUID);
    data.WriteGuidMask<3>(playerGUID);
    data.WriteBit(!proposal.isNew);                         // dungeon in progress
    data.WriteGuidMask<1, 7, 6, 0>(playerGUID);
    data.WriteGuidMask<0, 2>(InstanceSaveGUID);
    data.WriteGuidMask<5>(playerGUID);
    data.WriteBit(silent);
    data.WriteGuidMask<4>(InstanceSaveGUID);

    data.WriteBits(proposal.players.size(), 21);
    for (lfg::LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
    {
        lfg::LfgProposalPlayer const& player = it->second;

        data.WriteBit(player.accept == lfg::LFG_ANSWER_AGREE);   // Accepted
        data.WriteBit(player.accept!= lfg::LFG_ANSWER_PENDING);  // responded
        data.WriteBit(player.group ? player.group == proposal.group : false);   // In dungeon (silent)
        data.WriteBit(it->first == guid);                   // Self player
        data.WriteBit(player.group ? player.group == gguid : false);            // Same Group than player
    }
    data.WriteGuidMask<1, 3>(InstanceSaveGUID);

    data.WriteGuidBytes<7>(playerGUID);
    data.WriteGuidBytes<2, 4>(InstanceSaveGUID);
    data.WriteGuidBytes<3>(playerGUID);
    data.WriteGuidBytes<7>(InstanceSaveGUID);
    data.WriteGuidBytes<6, 2>(playerGUID);
    data.WriteGuidBytes<0>(InstanceSaveGUID);
    data.WriteGuidBytes<4, 1>(playerGUID);
    for (lfg::LfgProposalPlayerContainer::const_iterator it = proposal.players.begin(); it != proposal.players.end(); ++it)
        data << uint32(it->second.role);                    // Role
    data.WriteGuidBytes<0>(playerGUID);
    data.WriteGuidBytes<6, 1, 3>(InstanceSaveGUID);
    data.WriteGuidBytes<5>(playerGUID);
    data.WriteGuidBytes<5>(InstanceSaveGUID);

    SendPacket(&data);
}

void WorldSession::SendLfgLfrList(bool update)
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_LFR_LIST %s update: %u",
        GetPlayerName().c_str(), update ? 1 : 0);
    WorldPacket data(SMSG_LFG_UPDATE_SEARCH, 1);
    data << uint8(update);                                 // In Lfg Queue?
    SendPacket(&data);
}

void WorldSession::SendLfgDisabled()
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_DISABLED %s", GetPlayerName().c_str());
    WorldPacket data(SMSG_LFG_DISABLED, 0);
    SendPacket(&data);
}

void WorldSession::SendLfgOfferContinue(uint32 dungeonEntry)
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_OFFER_CONTINUE %s dungeon entry: %u",
        GetPlayerName().c_str(), dungeonEntry);
    WorldPacket data(SMSG_LFG_OFFER_CONTINUE, 4);
    data << uint32(dungeonEntry);
    SendPacket(&data);
}

void WorldSession::SendLfgTeleportError(uint8 err)
{
    TC_LOG_DEBUG("lfg", "SMSG_LFG_TELEPORT_DENIED %s reason: %u",
        GetPlayerName().c_str(), err);
    WorldPacket data(SMSG_LFG_TELEPORT_DENIED, 4);
    data.WriteBits(err, 4);                                   // Error
    data.FlushBits();
    SendPacket(&data);
}

/*
void WorldSession::SendLfrUpdateListOpcode(uint32 dungeonEntry)
{
    TC_LOG_DEBUG("network", "SMSG_LFG_UPDATE_LIST %s dungeon entry: %u",
        GetPlayerName().c_str(), dungeonEntry);
    WorldPacket data(SMSG_LFG_UPDATE_LIST);
    SendPacket(&data);
}
*/
