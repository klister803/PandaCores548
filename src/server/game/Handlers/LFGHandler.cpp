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
#include "GameEventMgr.h"
#include "InstanceScript.h"

void BuildPlayerLockDungeonBlock(ByteBuffer& data, const LfgLockMap& lock)
{  
    data << uint32(lock.size());
    for (LfgLockMap::const_iterator it = lock.begin(); it != lock.end(); ++it)
    {
        data << uint32(it->first);                         // Dungeon entry (id + type)
        data << uint32(it->second);                        // Lock status
        data << uint32(0);                                 // Unknown 4.2.2
        data << uint32(0);                                 // Unknown 4.2.2
    }
}

void BuildPartyLockDungeonBlock(WorldPacket& data, const LfgLockPartyMap& lockMap)
{
    ByteBuffer dataBuffer;
    data.WriteBits(lockMap.size(), 24);
    data.WriteBit(0);
    for (LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
    {
        ObjectGuid guid = it->first;                         // Player guid
        data.WriteBit(guid[3]);
        data.WriteBit(guid[4]);
        data.WriteBit(guid[5]);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[0]);
        data.WriteBit(guid[6]);
        data.WriteBit(guid[7]);
        data.WriteBits(it->second.size(), 22); // Size of lock dungeons
        data.WriteBit(guid[1]);
        dataBuffer.WriteByteSeq(guid[2]);
        dataBuffer.WriteByteSeq(guid[3]);
        dataBuffer.WriteByteSeq(guid[7]);
        dataBuffer.WriteByteSeq(guid[5]);
        BuildPlayerLockDungeonBlock(dataBuffer, it->second);
        dataBuffer.WriteByteSeq(guid[6]);
        dataBuffer.WriteByteSeq(guid[4]);
        dataBuffer.WriteByteSeq(guid[0]);
        dataBuffer.WriteByteSeq(guid[1]);
    }
    data.FlushBits();
    if (!dataBuffer.empty())
        data.append(dataBuffer);
}

void WorldSession::HandleLfgJoinOpcode(WorldPacket& recvData)
{
    uint32 numDungeons;
    uint32 dungeon;
    uint32 roles;
    uint8 length = 0;
    uint8 unk8 = 0;

    recvData >> roles;
    recvData >> unk8;
    for (int i = 0; i < 3; ++i)
        recvData.read_skip<uint32>();
    numDungeons = recvData.ReadBits(22);
    recvData.ReadBit();
    length = recvData.ReadBits(8);
    LfgDungeonSet newDungeons;
    for (uint32 i = 0; i < numDungeons; ++i)
    {
        recvData >> dungeon;
        newDungeons.insert(dungeon);       // remove the type from the dungeon entry
    }
    std::string comment = recvData.ReadString(length);

    if (!numDungeons)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_JOIN [" UI64FMTD "] no dungeons selected", GetPlayer()->GetGUID());
        return;
    }

    const LFGDungeonEntry* entry = sLFGDungeonStore.LookupEntry(*newDungeons.begin() & 0xFFFFFF);
    uint8 type = LFG_TYPE_DUNGEON;
    uint8 maxGroupSize = 5;
    if (entry != NULL)
        type = entry->difficulty == RAID_TOOL_DIFFICULTY ? LFG_TYPE_RAID : entry->isScenario() ? LFG_TYPE_SCENARIO : LFG_TYPE_DUNGEON;
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

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_JOIN [" UI64FMTD "] roles: %u, Dungeons: %u, Comment: %s", GetPlayer()->GetGUID(), roles, uint8(newDungeons.size()), comment.c_str());
    sLFGMgr->Join(GetPlayer(), uint8(roles), newDungeons, comment);
}

void WorldSession::HandleLfgLeaveOpcode(WorldPacket&  recvData)
{
    recvData.rfinish();

    Group* grp = GetPlayer()->GetGroup();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_LEAVE [" UI64FMTD "] in group: %u", GetPlayer()->GetGUID(), grp ? 1 : 0);

    // Check cheating - only leader can leave the queue
    if (!grp || grp->GetLeaderGUID() == GetPlayer()->GetGUID())
        sLFGMgr->Leave(GetPlayer(), grp);
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

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_PROPOSAL_RESULT [" UI64FMTD "] proposal: %u accept: %u", GetPlayer()->GetGUID(), proposalId, accept ? 1 : 0);
    sLFGMgr->UpdateProposal(proposalId, GetPlayer()->GetGUID(), accept);
}

void WorldSession::HandleLfgSetRolesOpcode(WorldPacket& recvData)
{
    uint32 roles;
    uint8 unk;
    recvData >> roles;                                    // Player Group Roles
    recvData >> unk;

    uint64 guid = GetPlayer()->GetGUID();
    Group* grp = GetPlayer()->GetGroup();
    if (!grp)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_SET_ROLES [" UI64FMTD "] Not in group", guid);
        return;
    }
    uint64 gguid = grp->GetGUID();
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_SET_ROLES: Group [" UI64FMTD "], Player [" UI64FMTD "], Roles: %u", gguid, guid, roles);
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

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_SET_LFG_COMMENT [" UI64FMTD "] comment: %s", (uint64)guid, comment.c_str());

    sLFGMgr->SetComment(guid, comment);
}

void WorldSession::HandleLfgSetBootVoteOpcode(WorldPacket& recvData)
{
    bool agree = recvData.ReadBit();                            // Agree to kick player

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_SET_BOOT_VOTE [" UI64FMTD "] agree: %u", GetPlayer()->GetGUID(), agree ? 1 : 0);
    sLFGMgr->UpdateBoot(GetPlayer(), agree);
}

void WorldSession::HandleLfgTeleportOpcode(WorldPacket& recvData)
{
    bool out = recvData.ReadBit();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFG_TELEPORT [" UI64FMTD "] out: %u", GetPlayer()->GetGUID(), out ? 1 : 0);
    sLFGMgr->TeleportPlayer(GetPlayer(), out, true);
}

void WorldSession::HandleLfgPlayerLockInfoRequestOpcode(WorldPacket& recvData)
{
    recvData.rfinish();
    uint64 guid = GetPlayer()->GetGUID();
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFD_PLAYER_LOCK_INFO_REQUEST [" UI64FMTD "]", guid);

    // Get Random dungeons that can be done at a certain level and expansion
    LfgDungeonSet randomDungeons;
    uint8 level = GetPlayer()->getLevel();
    uint8 expansion = GetPlayer()->GetSession()->Expansion();
    for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);
        if (dungeon && dungeon->expansion <= expansion && dungeon->minlevel <= level && level <= dungeon->maxlevel)
        {
            if (dungeon->flags & LFG_FLAG_SEASONAL)
            {
                if (HolidayIds holiday = sLFGMgr->GetDungeonSeason(dungeon->ID))
                    if (!IsHolidayActive(holiday))
                        continue;
            }
            else if (dungeon->type != LFG_TYPE_RANDOM)
                continue;
            randomDungeons.insert(dungeon->Entry());
        }
    }

    // Get player locked Dungeons
    LfgLockMap lock = sLFGMgr->GetLockedDungeons(guid);
    uint32 rsize = uint32(randomDungeons.size());
    uint32 lsize = uint32(lock.size());

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_PLAYER_INFO [" UI64FMTD "]", guid);
    WorldPacket data(SMSG_LFG_PLAYER_INFO, 1 + rsize * (4 + 1 + 4 + 4 + 4 + 4 + 1 + 4 + 4 + 4) + 4 + lsize * (1 + 4 + 4 + 4 + 4 + 1 + 4 + 4 + 4));

    data.WriteBit(0);                               // not has guid
    data.WriteBits(lock.size(), 20);
    data.WriteBits(randomDungeons.size(), 17);      // Random Dungeon count

    ByteBuffer buff;
    for (LfgDungeonSet::const_iterator it = randomDungeons.begin(); it != randomDungeons.end(); ++it)
    {
        LfgReward const* reward = sLFGMgr->GetRandomDungeonReward(*it, level);
        Quest const* qRew[2] = { NULL, NULL };
        qRew[0] = sObjectMgr->GetQuestTemplate(reward->reward[0].questId);
        if (qRew[0])
            qRew[1] = sObjectMgr->GetQuestTemplate(reward->reward[1].questId);
        uint32 rewardItemCount = 0;
        uint32 rewardCurrencyCount1 = 0;
        uint32 rewardCurrencyCount2 = 0;

        data.WriteBits(qRew[0] ? qRew[0]->GetRewItemsCount() : 0, 20);
        data.WriteBits(qRew[1] ? qRew[1]->GetRewCurrencyCount() : 0, 21);
        data.WriteBit(qRew[0] ? !GetPlayer()->CanRewardQuest(qRew[0], false) : false);
        data.WriteBits(qRew[0] ? qRew[0]->GetRewCurrencyCount() : 0, 21);
        data.WriteBits(0, 19);          // role bonus count
        data.WriteBit(1);               // unk

        buff << uint32(0);
        buff << uint32(*it);
        buff << uint32(0);

        if (qRew[1])
        {
            for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
                if (uint32 cur = qRew[1]->RewardCurrencyId[i])
                {
                    buff << uint32(qRew[1]->RewardCurrencyCount[i] * GetCurrencyPrecision(cur));
                    buff << uint32(cur);
                }
        }

        if (qRew[0])
        {
            for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            {
                uint32 itemId = qRew[0]->RewardItemId[i];
                if (!itemId)
                    continue;

                ItemTemplate const* iProto = sObjectMgr->GetItemTemplate(qRew[0]->RewardItemId[i]);
                buff << uint32(iProto ? iProto->DisplayInfoID : 0);
                buff << uint32(qRew[0]->RewardItemIdCount[i]);
                buff << uint32(itemId);
            }
        }

        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(qRew[0] ? qRew[0]->XPValue(GetPlayer()) : 0);
        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(0);      // completed encounters mask?
        buff << uint32(qRew[0] ? qRew[0]->GetRewOrReqMoney() : 0);

        if (qRew[0])
        {
            for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
                if (uint32 cur = qRew[0]->RewardCurrencyId[i])
                {
                    buff << uint32(cur);
                    buff << uint32(qRew[0]->RewardCurrencyCount[i] * GetCurrencyPrecision(cur));
                }
        }

        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(0);
        buff << uint32(0);
    }

    if (!buff.empty())
    {
        data.FlushBits();
        data.append(buff);
    }

    for (LfgLockMap::const_iterator it = lock.begin(); it != lock.end(); ++it)
    {
        data << uint32(0);                                  // curr player item level
        data << uint32(0);                                  // req player item level
        data << uint32(it->second);                         // Lock status
        data << uint32(it->first);                          // Dungeon entry (id + type)
    }

    SendPacket(&data);
}

void WorldSession::HandleLfgPartyLockInfoRequestOpcode(WorldPacket&  /*recvData*/)
{
    uint64 guid = GetPlayer()->GetGUID();
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_LFD_PARTY_LOCK_INFO_REQUEST [" UI64FMTD "]", guid);

    Group* grp = GetPlayer()->GetGroup();
    if (!grp)
        return;

    // Get the locked dungeons of the other party members
    LfgLockPartyMap lockMap;
    for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* plrg = itr->getSource();
        if (!plrg)
            continue;

        uint64 pguid = plrg->GetGUID();
        if (pguid == guid)
            continue;

        lockMap[pguid] = sLFGMgr->GetLockedDungeons(pguid);
    }

    uint32 size = 0;
    for (LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
        size += 8 + 4 + uint32(it->second.size()) * (4 + 4 + 4 + 4);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_PARTY_INFO [" UI64FMTD "]", guid);
    WorldPacket data(SMSG_LFG_PARTY_INFO, 1 + size);
    BuildPartyLockDungeonBlock(data, lockMap);
    SendPacket(&data);
}

void WorldSession::HandleLfrSearchOpcode(WorldPacket& recvData)
{
    uint32 entry;                                          // Raid id to search
    recvData >> entry;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_SEARCH_LFG_JOIN [" UI64FMTD "] dungeon entry: %u", GetPlayer()->GetGUID(), entry);
    //SendLfrUpdateListOpcode(entry);
}

void WorldSession::HandleLfrLeaveOpcode(WorldPacket& recvData)
{
    uint32 dungeonId;                                      // Raid id queue to leave
    recvData >> dungeonId;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_SEARCH_LFG_LEAVE [" UI64FMTD "] dungeonId: %u", GetPlayer()->GetGUID(), dungeonId);
    //sLFGMgr->LeaveLfr(GetPlayer(), dungeonId);
}

void WorldSession::SendLfgUpdatePlayer(const LfgUpdateData& updateData)
{
    bool queued = false;
    bool extrainfo = false;
    bool quit = false;

    switch (updateData.updateType)
    {
        case LFG_UPDATETYPE_JOIN_PROPOSAL:
        case LFG_UPDATETYPE_ADDED_TO_QUEUE:
            queued = true;
            extrainfo = true;
            break;
        //case LFG_UPDATETYPE_CLEAR_LOCK_LIST: // TODO: Sometimes has extrainfo - Check ocurrences...
        case LFG_UPDATETYPE_PROPOSAL_BEGIN:
            extrainfo = true;
            break;
        case LFG_UPDATETYPE_GROUP_DISBAND:
        case LFG_UPDATETYPE_GROUP_FOUND:
        case LFG_UPDATETYPE_CLEAR_LOCK_LIST:
        case LFG_UPDATETYPE_REMOVED_FROM_QUEUE:
            quit = true;
            break;
        default:
            break;
    }
    sLFGMgr->SendUpdateStatus(GetPlayer(), updateData.comment, updateData.dungeons, false, quit);
    /*uint64 guid = GetPlayer()->GetGUID();
    uint8 size = uint8(updateData.dungeons.size());

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_UPDATE_PLAYER [" UI64FMTD "] updatetype: %u", guid, updateData.updateType);
    WorldPacket data(SMSG_LFG_UPDATE_PLAYER, 1 + 1 + (extrainfo ? 1 : 0) * (1 + 1 + 1 + 1 + size * 4 + updateData.comment.length()));
    data << uint8(updateData.updateType);                 // Lfg Update type
    data << uint8(extrainfo);                             // Extra info
    if (extrainfo)
    {
        data << uint8(queued);                            // Join the queue
        data << uint8(0);                                 // unk - Always 0
        data << uint8(0);                                 // unk - Always 0

        data << uint8(size);
        if (size)
            for (LfgDungeonSet::const_iterator it = updateData.dungeons.begin(); it != updateData.dungeons.end(); ++it)
                data << uint32(*it);
        data << updateData.comment;
    }
    SendPacket(&data);*/
}

void WorldSession::SendLfgUpdateParty(const LfgUpdateData& updateData, uint32 joinTime)
{
    bool join = false;
    bool extrainfo = false;
    bool queued = false;
    bool quit = false;
    bool pause = false;


    switch (updateData.updateType)
    {
        case LFG_UPDATETYPE_JOIN_PROPOSAL:
            extrainfo = true;
            break;
        case LFG_UPDATETYPE_ADDED_TO_QUEUE:
            extrainfo = true;
            join = true;
            queued = true;
            break;
        case LFG_UPDATETYPE_CLEAR_LOCK_LIST:
            // join = true;  // TODO: Sometimes queued and extrainfo - Check ocurrences...
            queued = true;
            quit = true;
            break;
        case LFG_UPDATETYPE_PROPOSAL_BEGIN:
            extrainfo = true;
            join = true;
            break;
        case LFG_UPDATETYPE_GROUP_DISBAND:
        case LFG_UPDATETYPE_GROUP_FOUND:
            quit = true;
            break;
        case LFG_UPDATETYPE_REMOVED_FROM_QUEUE:
            quit = true;
            pause = true;
            break;
        default:
            break;
    }

    uint64 guid = GetPlayer()->GetGUID();
    uint8 size = uint8(updateData.dungeons.size());
    sLFGMgr->SendUpdateStatus(GetPlayer(), updateData.comment, updateData.dungeons, pause, quit);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_UPDATE_PARTY [" UI64FMTD "] updatetype: %u", guid, updateData.updateType);
    /*WorldPacket data(SMSG_LFG_UPDATE_PARTY, 1 + 1 + (extrainfo ? 1 : 0) * (1 + 1 + 1 + 1 + 1 + size * 4 + updateData.comment.length()));
    data << uint8(updateData.updateType);                 // Lfg Update type
    data << uint8(extrainfo);                             // Extra info
    if (extrainfo)
    {
        data << uint8(join);                              // LFG Join
        data << uint8(queued);                            // Join the queue
        data << uint8(0);                                 // unk - Always 0
        data << uint8(0);                                 // unk - Always 0
        for (uint8 i = 0; i < 3; ++i)
            data << uint8(0);                             // unk - Always 0

        data << uint8(size);
        if (size)
            for (LfgDungeonSet::const_iterator it = updateData.dungeons.begin(); it != updateData.dungeons.end(); ++it)
                data << uint32(*it);
        data << updateData.comment;
    }
    SendPacket(&data);*/
}

void WorldSession::SendLfgRoleChosen(uint64 guid, uint8 roles)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_ROLE_CHOSEN [" UI64FMTD "] guid: [" UI64FMTD "] roles: %u", GetPlayer()->GetGUID(), guid, roles);

    WorldPacket data(SMSG_LFG_ROLE_CHOSEN, 8 + 1 + 1 + 4);
    data.WriteGuidMask<0, 6, 1, 4, 7, 3>(guid);
    data.WriteBit(roles != 0);                             // Ready
    data.WriteGuidMask<2, 5>(guid);

    data.WriteGuidBytes<2, 4, 5, 1, 3, 0>(guid);
    data << uint32(roles);                                 // Roles
    data.WriteGuidBytes<7, 6>(guid);

    SendPacket(&data);
}

void WorldSession::SendLfgRoleCheckUpdate(const LfgRoleCheck* pRoleCheck, bool updateAll)
{
    ASSERT(pRoleCheck);
    LfgDungeonSet dungeons;
    if (pRoleCheck->rDungeonId)
        dungeons.insert(pRoleCheck->rDungeonId);
    else
        dungeons = pRoleCheck->dungeons;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_ROLE_CHECK_UPDATE [" UI64FMTD "]", GetPlayer()->GetGUID());
    ByteBuffer buff;
    ObjectGuid guid = _player->GetObjectGuid();
    WorldPacket data(SMSG_LFG_ROLE_CHECK_UPDATE, 4 + 1 + 1 + dungeons.size() * 4 + 1 + pRoleCheck->roles.size() * (8 + 1 + 4 + 1));
    data.WriteBit(pRoleCheck->state == LFG_ROLECHECK_INITIALITING);
    data.WriteGuidMask<0>(guid);
    data.WriteBits(updateAll ? pRoleCheck->roles.size() : 1, 21);
    data.WriteGuidMask<6, 4>(guid);

    if (!pRoleCheck->roles.empty())
    {
        // Player info MUST be sent 1st :S
        ObjectGuid memberGuid = guid;
        uint8 roles = pRoleCheck->roles.find(guid)->second;
        Player* player = ObjectAccessor::FindPlayer(guid);

        data.WriteBit(roles != 0);
        data.WriteGuidMask<5, 6, 0, 7, 2, 1, 4, 3>(guid);

        buff.WriteGuidBytes<4, 6, 3, 0>(guid);
        buff << uint32(roles);                                      // Roles
        buff.WriteGuidBytes<5, 1, 2>(guid);
        buff << uint8(player ? player->getLevel() : 0);             // Level
        buff.WriteGuidBytes<7>(guid);

        for (LfgRolesMap::const_reverse_iterator it = pRoleCheck->roles.rbegin(); it != pRoleCheck->roles.rend(); ++it)
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
    for (LfgDungeonSet::iterator it = dungeons.begin(); it != dungeons.end(); ++it)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(*it);
        data << uint32(dungeon ? dungeon->Entry() : 0); // Dungeon
    }
    data.WriteGuidBytes<4, 7>(guid);
    data << uint8(pRoleCheck->state);
    data.WriteGuidBytes<6, 2, 3, 0, 5>(guid);
    data << uint8(0);                                   // roles

    SendPacket(&data);
}

void WorldSession::SendLfgJoinResult(const LfgJoinResultData& joinData)
{
    uint32 size = 0;
    for (LfgLockPartyMap::const_iterator it = joinData.lockmap.begin(); it != joinData.lockmap.end(); ++it)
        size += 8 + 3 + 1 + uint32(it->second.size()) * 4 * 4;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_JOIN_RESULT [" UI64FMTD "] checkResult: %u checkValue: %u", GetPlayer()->GetGUID(), joinData.result, joinData.state);
    ObjectGuid guid = GetPlayer()->GetObjectGuid();
    const LfgLockPartyMap& lockMap = joinData.lockmap;

    WorldPacket data(SMSG_LFG_JOIN_RESULT, 3 + 4 * 3 + 8 + 1 + 1 + size);
    data.WriteGuidMask<2, 6, 4, 1, 0, 3>(guid);

    data.WriteBits(lockMap.size(), 22);
    for (LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
    {
        ObjectGuid playerGuid = it->first;      // Player guid

        data.WriteGuidMask<1, 7, 6, 2, 3, 4, 0>(playerGuid);
        data.WriteBits(it->second.size(), 20);  // Size of lock dungeons
        data.WriteGuidMask<5>(playerGuid);
    }
    data.WriteGuidMask<7, 5>(guid);

    data << uint32(time(NULL));

    for (LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
    {
        ObjectGuid playerGuid = it->first;      // Player guid
        const LfgLockMap& lockMap2 = it->second;

        for (LfgLockMap::const_iterator itr = lockMap2.begin(); itr != lockMap2.end(); ++itr)
        {
            data << uint32(itr->first);                     // Dungeon entry (id + type)
            data << uint32(0);                              // needed item level
            data << uint32(0);                              // current item level
            data << uint32(itr->second);                    // Lock status
        }

        data.WriteGuidBytes<6, 5, 7, 1, 4, 0, 3, 2>(playerGuid);
    }
    data.WriteGuidBytes<7>(guid);
    data << uint32(_player->GetTeam());                     // group id
    data << uint32(3);                                      // lfg queue id?
    data << uint8(joinData.result);                         // Check Result
    data.WriteGuidBytes<0, 5, 1, 6, 3>(guid);
    data << uint8(joinData.state);                          // Check Value
    data.WriteGuidBytes<4, 2>(guid);

    SendPacket(&data);
}

void WorldSession::SendLfgQueueStatus(uint32 dungeon, int32 waitTime, int32 avgWaitTime, int32 waitTimeTanks, int32 waitTimeHealer, int32 waitTimeDps, uint32 queuedTime, uint8 tanks, uint8 healers, uint8 dps)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_QUEUE_STATUS [" UI64FMTD "] dungeon: %u - waitTime: %d - avgWaitTime: %d - waitTimeTanks: %d - waitTimeHealer: %d - waitTimeDps: %d - queuedTime: %u - tanks: %u - healers: %u - dps: %u", GetPlayer()->GetGUID(), dungeon, waitTime, avgWaitTime, waitTimeTanks, waitTimeHealer, waitTimeDps, queuedTime, tanks, healers, dps);
    LfgQueueInfo* info = sLFGMgr->GetLfgQueueInfo(GetPlayer()->GetGroup() ? GetPlayer()->GetGroup()->GetGUID() : GetPlayer()->GetGUID());
    if (!info)
        return;

    ObjectGuid guid = GetPlayer()->GetObjectGuid();
    WorldPacket data(SMSG_LFG_QUEUE_STATUS, 4 + 4 + 4 + 4 + 4 +4 + 1 + 1 + 1 + 4);
    data.WriteGuidMask<0, 3, 6, 2, 4, 7, 5, 1>(guid);

    data.WriteGuidBytes<7>(guid);
    data << uint32(queuedTime);                             // Player wait time in queue
    data << uint32(info->joinTime);
    data << uint32(3);
    data << int32(waitTimeTanks);                           // Wait Tanks
    data << uint8(tanks);                                   // Tanks needed
    data << int32(waitTimeHealer);                          // Wait Healers
    data << uint8(healers);                                 // Healers needed
    data << int32(waitTimeDps);                             // Wait Dps
    data << uint8(dps);                                     // Dps needed
    data << uint32(GetPlayer()->GetTeam());                 // group id
    data.WriteGuidBytes<2, 4>(guid);
    data << int32(waitTime);
    data << int32(avgWaitTime);
    data.WriteGuidBytes<6, 5, 0>(guid);
    data << uint32(dungeon);                               // Dungeon
    data.WriteGuidBytes<3, 1>(guid);

    SendPacket(&data);
}

void WorldSession::SendLfgPlayerReward(uint32 rdungeonEntry, uint32 sdungeonEntry, uint8 done, const LfgReward* reward, const Quest* qRew)
{
    if (!rdungeonEntry || !sdungeonEntry || !qRew)
        return;

    uint8 itemCount = qRew->GetRewItemsCount() + qRew->GetRewCurrencyCount();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_PLAYER_REWARD [" UI64FMTD "] rdungeonEntry: %u - sdungeonEntry: %u - done: %u", GetPlayer()->GetGUID(), rdungeonEntry, sdungeonEntry, done);
    WorldPacket data(SMSG_LFG_PLAYER_REWARD, 4 * 4 + itemCount + itemCount * 4 * 4);
    data.WriteBits(itemCount, 20);
    for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        if (qRew->RewardItemId[i])
            data.WriteBit(0);
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
        if (qRew->RewardCurrencyId[i])
            data.WriteBit(1);
    for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
    {
        uint32 itemId = qRew->RewardItemId[i];
        if (!itemId)
            continue;

        ItemTemplate const* iProto = sObjectMgr->GetItemTemplate(itemId);

        data << uint32(qRew->RewardItemIdCount[i]);
        data << uint32(itemId);
        data << uint32(iProto ? iProto->DisplayInfoID : 0);
        data << uint32(0);
    }
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        uint32 cur = qRew->RewardCurrencyId[i];
        if (!cur)
            continue;

        data << uint32(qRew->RewardCurrencyCount[i] * GetCurrencyPrecision(cur));
        data << uint32(cur);
        data << uint32(0);
        data << uint32(0);                                  // bonus valor points
    }

    data << uint32(qRew->XPValue(GetPlayer()));
    data << uint32(qRew->GetRewOrReqMoney());
    data << uint32(rdungeonEntry);                         // Random Dungeon Finished
    data << uint32(sdungeonEntry);                         // Dungeon Finished

    SendPacket(&data);
}

void WorldSession::SendLfgBootPlayer(const LfgPlayerBoot* pBoot)
{
    uint64 guid = GetPlayer()->GetGUID();
    LfgAnswer playerVote = pBoot->votes.find(guid)->second;
    uint8 votesNum = 0;
    uint8 agreeNum = 0;
    uint32 secsleft = uint8((pBoot->cancelTime - time(NULL)) / 1000);
    for (LfgAnswerMap::const_iterator it = pBoot->votes.begin(); it != pBoot->votes.end(); ++it)
    {
        if (it->second != LFG_ANSWER_PENDING)
        {
            ++votesNum;
            if (it->second == LFG_ANSWER_AGREE)
                ++agreeNum;
        }
    }
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_BOOT_PROPOSAL_UPDATE [" UI64FMTD "] inProgress: %u - didVote: %u - agree: %u - victim: [" UI64FMTD "] votes: %u - agrees: %u - left: %u - needed: %u - reason %s",
        guid, uint8(pBoot->inProgress), uint8(playerVote != LFG_ANSWER_PENDING), uint8(playerVote == LFG_ANSWER_AGREE), pBoot->victim, votesNum, agreeNum, secsleft, pBoot->votedNeeded, pBoot->reason.c_str());

    ObjectGuid victimGuid = pBoot->victim;

    WorldPacket data(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 8 + 1 + 1 + 4 + 4 + 4 + 4 + 1 + pBoot->reason.size());
    // TODO: maybe my vote and agree need to be swapped
    data.WriteGuidMask<4, 5, 7>(victimGuid);
    data.WriteBit(0);                                       // My vote
    data.WriteGuidMask<2>(victimGuid);
    data.WriteBit(playerVote == LFG_ANSWER_AGREE);          // Agree
    data.WriteGuidMask<6>(victimGuid);
    data.WriteBit(!pBoot->reason.size());
    data.WriteGuidMask<1>(victimGuid);
    data.WriteBit(pBoot->inProgress);                       // Vote in progress
    if (uint32 len = pBoot->reason.size())
        data.WriteBits(len, 8);
    data.WriteGuidMask<0, 3>(victimGuid);
    data.WriteBit(playerVote != LFG_ANSWER_PENDING);        // Did Vote

    data.WriteGuidBytes<5, 7>(victimGuid);
    if (pBoot->reason.size())
        data.WriteString(pBoot->reason);
    data.WriteGuidBytes<2>(victimGuid);
    data << uint32(votesNum);                               // Total Votes
    data.WriteGuidBytes<6, 4, 3>(victimGuid);
    data << uint32(pBoot->votedNeeded);                     // Needed Votes
    data.WriteGuidBytes<1>(victimGuid);
    data << uint32(agreeNum);                               // Agree Count
    data.WriteGuidBytes<0>(victimGuid);
    data << uint32(secsleft);                               // Time Left

    data << pBoot->reason.c_str();                          // Kick reason
    SendPacket(&data);
}

void WorldSession::SendLfgUpdateProposal(uint32 proposalId, const LfgProposal* pProp)
{
    if (!pProp)
        return;

    uint64 guid = GetPlayer()->GetGUID();
    LfgProposalPlayerMap::const_iterator itPlayer = pProp->players.find(guid);
    if (itPlayer == pProp->players.end())                  // Player MUST be in the proposal
        return;

    LfgProposalPlayer* ppPlayer = itPlayer->second;
    uint32 pLowGroupGuid = ppPlayer->groupLowGuid;
    uint32 dLowGuid = pProp->groupLowGuid;
    uint32 dungeonId = pProp->dungeonId;
    bool isSameDungeon = false;
    bool isContinue = false;
    Group* grp = dLowGuid ? sGroupMgr->GetGroupByGUID(dLowGuid) : NULL;
    uint32 completedEncounters = 0;
    if (grp)
    {
        uint64 gguid = grp->GetGUID();
        isContinue = grp->isLFGGroup() && sLFGMgr->GetState(gguid) != LFG_STATE_FINISHED_DUNGEON;
        isSameDungeon = GetPlayer()->GetGroup() == grp && isContinue;
    }

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_PROPOSAL_UPDATE [" UI64FMTD "] state: %u", GetPlayer()->GetGUID(), pProp->state);
    WorldPacket data(SMSG_LFG_PROPOSAL_UPDATE, 4 + 1 + 4 + 4 + 1 + 1 + pProp->players.size() * (4 + 1 + 1 + 1 + 1 +1));

    if (!isContinue)                                       // Only show proposal dungeon if it's continue
    {
        LfgDungeonSet playerDungeons = sLFGMgr->GetSelectedDungeons(guid);
        if (playerDungeons.size() == 1)
            dungeonId = *playerDungeons.begin();
    }

    if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(dungeonId))
    {
        dungeonId = dungeon->Entry();

        // Select a player inside to be get completed encounters from
        if (grp)
        {
            for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* groupMember = itr->getSource();
                if (groupMember && groupMember->GetMapId() == uint32(dungeon->map))
                {
                    if (InstanceScript* instance = groupMember->GetInstanceScript())
                        completedEncounters = instance->GetCompletedEncounterMask();
                    break;
                }
            }
        }
    }

    ObjectGuid playerGUID = guid;
    ObjectGuid InstanceSaveGUID = MAKE_NEW_GUID(dungeonId, 0, HIGHGUID_INSTANCE_SAVE);

    data << uint32(dungeonId);                              // Dungeon
    data << uint8(pProp->state);                            // Result states
    data << uint32(completedEncounters);                    // Bosses killed
    data << uint32(time(NULL));                             // Date
    data << uint32(proposalId);                             // Proposal Id
    data << uint32(3);                                      // queue id
    data << uint32(GetPlayer()->GetTeam());                 // group Id?


    data.WriteGuidMask<5, 7>(InstanceSaveGUID);
    data.WriteGuidMask<2, 4>(playerGUID);
    data.WriteGuidMask<6>(InstanceSaveGUID);
    data.WriteGuidMask<3>(playerGUID);
    data.WriteBit(isSameDungeon);
    data.WriteGuidMask<1, 7, 6, 0>(playerGUID);
    data.WriteGuidMask<0, 2>(InstanceSaveGUID);
    data.WriteGuidMask<5>(playerGUID);
    data.WriteBit(0);                                       // silent?
    data.WriteGuidMask<4>(InstanceSaveGUID);

    data.WriteBits(pProp->players.size(), 21);
    for (itPlayer = pProp->players.begin(); itPlayer != pProp->players.end(); ++itPlayer)
    {
        data.WriteBit(itPlayer->second->accept == LFG_ANSWER_AGREE);    // Accepted
        data.WriteBit(itPlayer->second->accept!= LFG_ANSWER_PENDING);   // responded
        data.WriteBit(itPlayer->second->groupLowGuid ? itPlayer->second->groupLowGuid == dLowGuid : false);         // In dungeon (silent)
        data.WriteBit(itPlayer->first == guid);                         // Self player
        data.WriteBit(itPlayer->second->groupLowGuid ? itPlayer->second->groupLowGuid == pLowGroupGuid : false);    // Same Group than player
    }
    data.WriteGuidMask<1, 3>(InstanceSaveGUID);

    data.WriteGuidBytes<7>(playerGUID);
    data.WriteGuidBytes<2, 4>(InstanceSaveGUID);
    data.WriteGuidBytes<3>(playerGUID);
    data.WriteGuidBytes<7>(InstanceSaveGUID);
    data.WriteGuidBytes<6, 2>(playerGUID);
    data.WriteGuidBytes<0>(InstanceSaveGUID);
    data.WriteGuidBytes<4, 1>(playerGUID);
    for (itPlayer = pProp->players.begin(); itPlayer != pProp->players.end(); ++itPlayer)
        data << uint32(itPlayer->second->role);                     // Role
    data.WriteGuidBytes<0>(playerGUID);
    data.WriteGuidBytes<6, 1, 3>(InstanceSaveGUID);
    data.WriteGuidBytes<5>(playerGUID);
    data.WriteGuidBytes<5>(InstanceSaveGUID);

    SendPacket(&data);
}

void WorldSession::SendLfgUpdateSearch(bool update)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_UPDATE_SEARCH [" UI64FMTD "] update: %u", GetPlayer()->GetGUID(), update ? 1 : 0);
    WorldPacket data(SMSG_LFG_UPDATE_SEARCH, 1);
    data << uint8(update);                                 // In Lfg Queue?
    SendPacket(&data);
}

void WorldSession::SendLfgDisabled()
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_DISABLED [" UI64FMTD "]", GetPlayer()->GetGUID());
    WorldPacket data(SMSG_LFG_DISABLED, 0);
    SendPacket(&data);
}

void WorldSession::SendLfgOfferContinue(uint32 dungeonEntry)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_OFFER_CONTINUE [" UI64FMTD "] dungeon entry: %u", GetPlayer()->GetGUID(), dungeonEntry);
    WorldPacket data(SMSG_LFG_OFFER_CONTINUE, 4);
    data << uint32(dungeonEntry);
    SendPacket(&data);
}

void WorldSession::SendLfgTeleportError(uint8 err)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_LFG_TELEPORT_DENIED [" UI64FMTD "] reason: %u", GetPlayer()->GetGUID(), err);
    WorldPacket data(SMSG_LFG_TELEPORT_DENIED, 4);
    data.WriteBits(err, 4);                                   // Error
    data.FlushBits();
    SendPacket(&data);
}

/*
void WorldSession::SendLfrUpdateListOpcode(uint32 dungeonEntry)
{
    sLog->outDebug(LOG_FILTER_PACKETIO, "SMSG_LFG_UPDATE_LIST [" UI64FMTD "] dungeon entry: %u", GetPlayer()->GetGUID(), dungeonEntry);
    WorldPacket data(SMSG_LFG_UPDATE_LIST);
    SendPacket(&data);
}
*/
