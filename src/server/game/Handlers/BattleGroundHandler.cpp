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
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "Chat.h"
#include "Language.h"
#include "Log.h"
#include "Player.h"
#include "Object.h"
#include "Opcodes.h"
#include "DisableMgr.h"
#include "Group.h"
#include "Bracket.h"

void WorldSession::HandleBattlemasterHelloOpcode(WorldPacket & recvData)
{
    uint64 guid;
    recvData >> guid;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEMASTER_HELLO Message from (GUID: %u TypeId:%u)", GUID_LOPART(guid), GuidHigh2TypeId(GUID_HIPART(guid)));

    Creature* unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if (!unit->isBattleMaster())                             // it's not battlemaster
        return;

    // Stop the npc if moving
    unit->StopMoving();

    BattlegroundTypeId bgTypeId = sBattlegroundMgr->GetBattleMasterBG(unit->GetEntry());

    if (!_player->GetBGAccessByLevel(bgTypeId))
    {
                                                            // temp, must be gossip message...
        SendNotification(LANG_YOUR_BG_LEVEL_REQ_ERROR);
        return;
    }

    SendBattleGroundList(guid, bgTypeId);
}

void WorldSession::SendBattleGroundList(uint64 guid, BattlegroundTypeId bgTypeId)
{
    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundListPacket(&data, guid, _player, bgTypeId);
    SendPacket(&data);
}

//! 5.4.1
void WorldSession::HandleBattlemasterJoinOpcode(WorldPacket & recvData)
{
    ObjectGuid guid;
    uint32 bgTypeId_ = 0;
    uint8 instanceId = 0;   // wtf could be 0-8
    bool isPremade = false;
    Group* grp = NULL;
    IgnorMapInfo ignore;
    recvData >> ignore.map[0] >> ignore.map[1];

    if (recvData.ReadBit())
        instanceId = 8;
    recvData.ReadGuidMask<2, 4, 0, 3, 7, 1>(guid);
    bool joinAsGroup = recvData.ReadBit();
    recvData.ReadGuidMask<5, 6>(guid);
    recvData.ReadGuidBytes<0, 1, 7, 2, 4, 6, 5, 3>(guid);
    if (instanceId != 8)
        recvData >> instanceId;

    bgTypeId_ = GUID_LOPART(guid);

    if (!sBattlemasterListStore.LookupEntry(bgTypeId_))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: invalid bgtype (%u) received. possible cheater? player guid %u", bgTypeId_, _player->GetGUIDLow());
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeId_, NULL))
    {
        ChatHandler(this).PSendSysMessage(LANG_BG_DISABLED);
        return;
    }

    BattlegroundTypeId bgTypeId = BattlegroundTypeId(bgTypeId_);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEMASTER_JOIN Message from (GUID: %u TypeId:%u)", GUID_LOPART(guid), GuidHigh2TypeId(GUID_HIPART(guid)));

    // can do this, since it's battleground, not arena
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, 0);
    BattlegroundQueueTypeId bgQueueTypeIdRandom = BattlegroundMgr::BGQueueTypeId(BATTLEGROUND_RB, 0);

    // ignore if player is already in BG
    if (_player->InBattleground())
        return;

    // get bg instance or bg template if instance not found
    Battleground* bg = NULL;
    if (instanceId)
        bg = sBattlegroundMgr->GetBattlegroundThroughClientInstance(instanceId, bgTypeId);

    if (!bg)
        bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
    if (!bg)
        return;

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;

    // check queue conditions
    if (!joinAsGroup)
    {
        if (GetPlayer()->isUsingLfg())
        {
            // player is using dungeon finder or raid finder
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_LFG_CANT_USE_BATTLEGROUND);
            GetPlayer()->GetSession()->SendPacket(&data);
            return;
        }

        // check Deserter debuff
        if (!_player->CanJoinToBattleground())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        if (_player->GetBattlegroundQueueIndex(bgQueueTypeIdRandom) < PLAYER_MAX_BATTLEGROUND_QUEUES)
        {
            //player is already in random queue
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_IN_RANDOM_BG);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        if (_player->InBattlegroundQueue() && bgTypeId == BATTLEGROUND_RB)
        {
            //player is already in queue, can't start random queue
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_IN_NON_RANDOM_BG);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        // check if already in queue
        if (_player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
            //player is already in this queue
            return;

        // check if has free queue slots
        if (!_player->HasFreeBattlegroundQueueId())
        {
            WorldPacket data;
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, ERR_BATTLEGROUND_TOO_MANY_QUEUES);
            _player->GetSession()->SendPacket(&data);
            return;
        }

        BattlegroundQueue& bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId];

        GroupQueueInfo* ginfo = bgQueue.AddGroup(_player, NULL, bgTypeId, bracketEntry, 0, false, isPremade, ignore);
        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        uint32 queueSlot = _player->AddBattlegroundQueueId(bgQueueTypeId);

        // add joined time data
        _player->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        WorldPacket data; // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, ginfo->JoinType);
        SendPacket(&data);

        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, _player->GetGUIDLow(), _player->GetName());
    }
    else
    {
        grp = _player->GetGroup();

        if (!grp)
            return;

        if (grp->GetLeaderGUID() != _player->GetGUID())
            return;

        err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, 0, bg->GetMaxPlayersPerTeam(), false, 0);
        isPremade = (grp->GetMembersCount() >= bg->GetMinPlayersPerTeam());

        BattlegroundQueue& bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId];
        GroupQueueInfo* ginfo = NULL;
        uint32 avgTime = 0;

        if (!err)
        {
            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: the following players are joining as group:");
            ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, 0, false, isPremade, ignore);
            avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        }

        for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member)
                continue;   // this should never happen

            if (err)
            {
                WorldPacket data;
                sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, err);
                member->GetSession()->SendPacket(&data);
                continue;
            }

            // add to queue
            uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

            // add joined time data
            member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

            WorldPacket data; // send status packet (in queue)
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, ginfo->JoinType);
            member->GetSession()->SendPacket(&data);

            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
        }
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: group end");

    }

    sBattlegroundMgr->ScheduleQueueUpdate(0, 0, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleBattlegroundPlayerPositionsOpcode(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEGROUND_PLAYER_POSITIONS Message");

    Battleground* bg = _player->GetBattleground();
    if (!bg)                                                 // can't be received if player not in battleground
        return;

    uint32 flagCarrierCount = 0;
    Player* allianceFlagCarrier = NULL;
    Player* hordeFlagCarrier = NULL;

    if (uint64 guid = bg->GetFlagPickerGUID(TEAM_ALLIANCE))
    {
        allianceFlagCarrier = ObjectAccessor::FindPlayer(guid);
         if (allianceFlagCarrier)
            ++flagCarrierCount;
    }

    if (uint64 guid = bg->GetFlagPickerGUID(TEAM_HORDE))
    {
        hordeFlagCarrier = ObjectAccessor::FindPlayer(guid);
        if (hordeFlagCarrier)
            ++flagCarrierCount;
    }

    WorldPacket data(MSG_BATTLEGROUND_PLAYER_POSITIONS, 4 + 4 + 16 * flagCarrierCount);
   // Used to send several player positions (found used in AV)
    data << 0;  // CGBattlefieldInfo__m_numPlayerPositions
    /*
    for (CGBattlefieldInfo__m_numPlayerPositions)
        data << guid << posx << posy;
    */
    if (allianceFlagCarrier)
    {
        data << uint64(allianceFlagCarrier->GetGUID());
        data << float(allianceFlagCarrier->GetPositionX());
        data << float(allianceFlagCarrier->GetPositionY());
    }

    if (hordeFlagCarrier)
    {
        data << uint64(hordeFlagCarrier->GetGUID());
        data << float(hordeFlagCarrier->GetPositionX());
        data << float(hordeFlagCarrier->GetPositionY());
    }

    SendPacket(&data);
}

void WorldSession::HandlePVPLogDataOpcode(WorldPacket & /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd MSG_PVP_LOG_DATA Message");

    Battleground* bg = _player->GetBattleground();
    if (!bg)
        return;

    // Prevent players from sending BuildPvpLogDataPacket in an arena except for when sent in BattleGround::EndBattleGround.
    if (bg->isArena())
        return;

    WorldPacket data;
    sBattlegroundMgr->BuildPvpLogDataPacket(&data, bg);
    SendPacket(&data);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent MSG_PVP_LOG_DATA Message");
}

void WorldSession::HandleBattlefieldListOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEFIELD_LIST Message");

    uint32 bgTypeId;
    recvData >> bgTypeId;                                  // id from DBC

    BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
    if (!bl)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BattlegroundHandler: invalid bgtype (%u) with player (Name: %s, GUID: %u) received.", bgTypeId, _player->GetName(), _player->GetGUIDLow());
        return;
    }

    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundListPacket(&data, 0, _player, BattlegroundTypeId(bgTypeId));
    SendPacket(&data);
}

//! 5.4.1
void WorldSession::HandleBattleFieldPortOpcode(WorldPacket &recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEFIELD_PORT Message");

    uint32 time;
    uint32 queueSlot;                                            // guessed
    uint32 unk;                                                  // type id from dbc
    uint8 action;                                                // enter battle 0x1, leave queue 0x0
    ObjectGuid guid;

    action = recvData.ReadBit();
    recvData.ResetBitReader();

    recvData >> queueSlot;
    recvData >> time;
    recvData >> unk;
   
    recvData.ReadGuidMask<5, 0, 4, 2, 6, 1, 3, 7>(guid);
    recvData.ReadGuidBytes<2, 5, 4, 6, 3, 0, 7, 1>(guid);

	WorldPacket data;

    if (queueSlot > PLAYER_MAX_BATTLEGROUND_QUEUES)
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "HandleBattleFieldPortOpcode queueSlot %u", queueSlot);
        return;
    }

    if (!_player || !_player->InBattlegroundQueue())
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BattlegroundHandler: Invalid CMSG_BATTLEFIELD_PORT received from player (Name: %s, GUID: %u), he is not in bg_queue.", _player->GetName(), _player->GetGUIDLow());
        return;
    }

    BattlegroundQueueTypeId bgQueueTypeId = _player->GetBattlegroundQueueTypeId(queueSlot);
    if (bgQueueTypeId == BATTLEGROUND_QUEUE_NONE)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BattlegroundHandler: invalid queueSlot (%u) received.", queueSlot);
        return;
    }

    if (bgQueueTypeId > MAX_BATTLEGROUND_QUEUE_TYPES)
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "HandleBattleFieldPortOpcode bgQueueTypeId %u", bgQueueTypeId);
        return;
    }

    BattlegroundQueue& bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId];
    BattlegroundTypeId bgTypeId = BattlegroundMgr::BGTemplateId(bgQueueTypeId);

    if (bgTypeId == BATTLEGROUND_TYPE_NONE || bgTypeId >= MAX_BATTLEGROUND_TYPE_ID)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BattlegroundHandler: invalid bgTypeId (%u) received.", bgTypeId);
        return;
    }

    // we must use temporary variable, because GroupQueueInfo pointer can be deleted in BattlegroundQueue::RemovePlayer() function
    GroupQueueInfo ginfo;
    if (!bgQueue.GetPlayerGroupInfoData(_player->GetGUID(), &ginfo))
    {
        //sLog->outError(LOG_FILTER_NETWORKIO, "BattlegroundHandler: itrplayerstatus not found.");
        return;
    }

    // if action == 1, then instanceId is required
    if (!ginfo.IsInvitedToBGInstanceGUID && action == 1)
    {
        //sLog->outError(LOG_FILTER_NETWORKIO, "BattlegroundHandler: instance not found.");
        return;
    }

    // BGTemplateId returns BATTLEGROUND_AA when it is arena queue.
    // Do instance id search as there is no AA bg instances.
    Battleground* bg = sBattlegroundMgr->GetBattleground(ginfo.IsInvitedToBGInstanceGUID, (bgTypeId == BATTLEGROUND_AA || bgTypeId == BATTLEGROUND_RATED_10_VS_10) ? BATTLEGROUND_TYPE_NONE : bgTypeId);

    // bg template might and must be used in case of leaving queue, when instance is not created yet
    if (!bg && action == 0)
        bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
    if (!bg)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "BattlegroundHandler: bg_template not found for type id %u.", bgTypeId);
        return;
    }

    // get real bg type
    bgTypeId = bg->GetTypeID();

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;

    //some checks if player isn't cheating - it is not exactly cheating, but we cannot allow it
    if (action == 1 && ginfo.JoinType == 0)
    {
        //if player is trying to enter battleground (not arena!) and he has deserter debuff, we must just remove him from queue
        if (!_player->CanJoinToBattleground())
        {
            //send bg command result to show nice message
            WorldPacket data2;
            sBattlegroundMgr->BuildStatusFailedPacket(&data2, bg, _player, 0, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data2);
            action = 0;
            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player %s (%u) has a deserter debuff, do not port him to battleground!", _player->GetName(), _player->GetGUIDLow());
        }
        //if player don't match battleground max level, then do not allow him to enter! (this might happen when player leveled up during his waiting in queue
        if (_player->getLevel() > bg->GetMaxLevel())
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: Player %s (%u) has level (%u) higher than maxlevel (%u) of battleground (%u)! Do not port him to battleground!",
                _player->GetName(), _player->GetGUIDLow(), _player->getLevel(), bg->GetMaxLevel(), bg->GetTypeID());
            action = 0;
        }
    }

    switch (action)
    {
        case 1:                                         // port to battleground
            if (!_player->IsInvitedForBattlegroundQueueType(bgQueueTypeId))
                return;                                 // cheating?

            if (!_player->InBattleground())
                _player->SetBattlegroundEntryPoint();

            // resurrect the player
            if (!_player->isAlive())
            {
                _player->ResurrectPlayer(1.0f);
                _player->SpawnCorpseBones();
            }
            // stop taxi flight at port
            if (_player->isInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }

            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_IN_PROGRESS, _player->GetBattlegroundQueueJoinTime(bgTypeId), bg->GetElapsedTime(), ginfo.JoinType);
            _player->GetSession()->SendPacket(&data);
            // remove battleground queue status from BGmgr
            bgQueue.RemovePlayer(_player->GetGUID(), false);
            // this is still needed here if battleground "jumping" shouldn't add deserter debuff
            // also this is required to prevent stuck at old battleground after SetBattlegroundId set to new
            if (Battleground* currentBg = _player->GetBattleground())
                currentBg->RemovePlayerAtLeave(_player->GetGUID(), false, true);

            // set the destination instance id
            _player->SetBattlegroundId(bg->GetInstanceID(), bgTypeId);
            // set the destination team
            _player->SetBGTeam(ginfo.Team);
            // bg->HandleBeforeTeleportToBattleground(_player);
            sBattlegroundMgr->SendToBattleground(_player, ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
            // add only in HandleMoveWorldPortAck()
            // bg->AddPlayer(_player, team);

            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player %s (%u) joined battle for bg %u, bgtype %u, queue type %u.", _player->GetName(), _player->GetGUIDLow(), bg->GetInstanceID(), bg->GetTypeID(), bgQueueTypeId);
            break;
        case 0:                                         // leave queue
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_NONE,  0, 0, ginfo.JoinType);
            SendPacket(&data);
    
            sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, queueSlot, ERR_LEAVE_QUEUE);
            SendPacket(&data);

            if (bg && bg->isRated())
                if (bg->GetStatus() == STATUS_WAIT_JOIN || bg->GetStatus() == STATUS_IN_PROGRESS)
                    _player->HandleArenaDeserter();

            _player->RemoveBattlegroundQueueId(bgQueueTypeId);  // must be called this way, because if you move this call to queue->removeplayer, it causes bugs
            bgQueue.RemovePlayer(_player->GetGUID(), true);
            // player left queue, we should update it - do not update Arena Queue
            if (!ginfo.JoinType)
                sBattlegroundMgr->ScheduleQueueUpdate(ginfo.MatchmakerRating, ginfo.JoinType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());

            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player %s (%u) left queue for bgtype %u, queue type %u.", _player->GetName(), _player->GetGUIDLow(), bg->GetTypeID(), bgQueueTypeId);
            break;
        default:
            sLog->outError(LOG_FILTER_NETWORKIO, "Battleground port: unknown action %u", action);
            break;
    }
}

void WorldSession::HandleLeaveBattlefieldOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_LEAVE_BATTLEFIELD Message");

    /*recvData.read_skip<uint8>();                         // unk1
    recvData.read_skip<uint8>();                           // unk2
    recvData.read_skip<uint32>();                          // BattlegroundTypeId
    recvData.read_skip<uint16>();                          // unk3*/

    // not allow leave battleground in combat
    if (Battleground* bg = _player->GetBattleground())
        if (_player->isInCombat() || bg->isArena())
            if (bg->GetStatus() != STATUS_WAIT_LEAVE)
                return;

    _player->LeaveBattleground();
}

void WorldSession::HandleBattlefieldStatusOpcode(WorldPacket & /*recvData*/)
{
    // empty opcode
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEFIELD_STATUS Message");

    WorldPacket data;
    // we must update all queues here
    Battleground* bg = NULL;
    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        BattlegroundQueueTypeId bgQueueTypeId = _player->GetBattlegroundQueueTypeId(i);
        if (!bgQueueTypeId)
            continue;
        BattlegroundTypeId bgTypeId = BattlegroundMgr::BGTemplateId(bgQueueTypeId);
        uint8 JoinType = BattlegroundMgr::BGJoinType(bgQueueTypeId);
        if (bgTypeId == _player->GetBattlegroundTypeId())
        {
            bg = _player->GetBattleground();
            //i cannot check any variable from player class because player class doesn't know if player is in 2v2 / 3v3 or 5v5 arena
            //so i must use bg pointer to get that information
            if (bg && bg->GetJoinType() == JoinType)
            {
                // this line is checked, i only don't know if GetElapsedTime() is changing itself after bg end!
                // send status in Battleground
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, i, STATUS_IN_PROGRESS, _player->GetBattlegroundQueueJoinTime(bgTypeId), bg->GetElapsedTime(), JoinType);
                SendPacket(&data);
                continue;
            }
        }

        //we are sending update to player about queue - he can be invited there!
        //get GroupQueueInfo for queue status
        BattlegroundQueue& bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId];
        GroupQueueInfo ginfo;
        if (!bgQueue.GetPlayerGroupInfoData(_player->GetGUID(), &ginfo))
            continue;
        if (ginfo.IsInvitedToBGInstanceGUID)
        {
            bg = sBattlegroundMgr->GetBattleground(ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
            if (!bg)
                continue;

            // send status invited to Battleground
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, GetPlayer(), i, STATUS_WAIT_JOIN, getMSTimeDiff(getMSTime(), ginfo.RemoveInviteTime), _player->GetBattlegroundQueueJoinTime(bgTypeId), JoinType);
            SendPacket(&data);
        }
        else
        {
            bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
            if (!bg)
                continue;

            // expected bracket entry
            PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
            if (!bracketEntry)
                continue;

            uint32 avgTime = bgQueue.GetAverageQueueWaitTime(&ginfo, bracketEntry->GetBracketId());
            // send status in Battleground Queue
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, GetPlayer(), i, STATUS_WAIT_QUEUE, avgTime, _player->GetBattlegroundQueueJoinTime(bgTypeId), JoinType);
            SendPacket(&data);
        }
    }
}

void WorldSession::JoinBracket(uint8 slot)
{
    // ignore if we already in BG or BG queue
    if (_player->InBattleground())
        return;

    BattlegroundTypeId bgTypeId = slot < BRACKET_TYPE_RATED_BG ? BATTLEGROUND_AA : BATTLEGROUND_RATED_10_VS_10;

    uint32 arenaRating = 0;
    uint32 matchmakerRating = 0;

    uint8 Jointype = BattlegroundMgr::GetJoinTypeByBracketSlot(slot);

    //check existance
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
    if (!bg)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: template bg (all arenas) not found");
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeId, NULL))
    {
        ChatHandler(this).PSendSysMessage(LANG_ARENA_DISABLED);
        return;
    }

    bgTypeId = bg->GetTypeID();
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, Jointype);

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), _player->getLevel());
    if (!bracketEntry)
        return;
    
    uint32 avgTime = 0;
    GroupQueueInfo* ginfo;
    GroupJoinBattlegroundResult err = ERR_BATTLEGROUND_NONE;
    Group* grp = _player->GetGroup();

    //! Custom conection for join withoud group
    if (!grp && sBattlegroundMgr->isArenaTesting())
    {
        BattlegroundQueue &bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

        IgnorMapInfo ignore;    //empty
        ginfo = bgQueue.AddGroup(_player, NULL, bgTypeId, bracketEntry, Jointype, true, false, ignore, 1500);
        avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId()); 

        uint32 queueSlot = _player->AddBattlegroundQueueId(bgQueueTypeId);
        _player->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        WorldPacket data; // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, _player, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, Jointype);
        _player->GetSession()->SendPacket(&data);
        sBattlegroundMgr->ScheduleQueueUpdate(1500, Jointype, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
        return;
    }

     // no group found, error
     if (!grp || grp->GetLeaderGUID() != _player->GetGUID())
         return;

    BattlegroundQueue &bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);

    err = grp->CanJoinBattlegroundQueue(bg, bgQueueTypeId, Jointype, Jointype, true, slot);
    if (!err || (err && sBattlegroundMgr->isArenaTesting()))
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: leader %s join type %u", _player->GetName(), Jointype);

        matchmakerRating = grp->GetAverageMMR(BracketType(slot));
        IgnorMapInfo ignore;
        ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, Jointype, true, false, ignore, matchmakerRating);
        avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());        
    }
    else
    {
         WorldPacket data;
         sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, _player, 0, err);
         SendPacket(&data);

         sLog->outError(LOG_FILTER_NETWORKIO, "Battleground: Join Fail. QueueTypeID: %u, JoinType: %u, Slot: %u, Err %u", bgQueueTypeId, Jointype, slot, err);
         return;
    }

    for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->getSource();
        if (!member)
            continue;

         // add to queue
        uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

        // add joined time data
        member->AddBattlegroundQueueJoinTime(bgTypeId, ginfo->JoinTime);

        WorldPacket data; // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, avgTime, ginfo->JoinTime, Jointype);
        member->GetSession()->SendPacket(&data);

        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: player joined queue for arena as group bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
    }
    sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, Jointype, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

/// @used for CMSG_BATTLEMASTER_JOIN_ARENA and CMSG_BATTLEMASTER_JOIN_RATED
void WorldSession::HandleBattlemasterJoinArena(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_BATTLEMASTER_JOIN_ARENA");

    uint8 arenaslot;                                        // 2v2, 3v3 or 5v5
    recvData >> arenaslot;
    
    JoinBracket(arenaslot);
}

void WorldSession::HandleBattlemasterJoinRated(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_BATTLEMASTER_JOIN_RATED");

    JoinBracket(BRACKET_TYPE_RATED_BG);
}

void WorldSession::HandleRequestRatedInfo(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_RATED_INFO");
    _player->SendPvpRatedStats();
}

void WorldSession::HandleRequestPvpOptions(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_PVP_OPTIONS_ENABLED");

    /// @Todo: perfome research in this case
    WorldPacket data(SMSG_PVP_OPTIONS_ENABLED, 1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.FlushBits();
    SendPacket(&data);
}

void WorldSession::HandleRequestPvpReward(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_PVP_REWARDS");

    _player->SendPvpRewards();
}

void WorldSession::HandleRequestRatedBgStats(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_REQUEST_RATED_BG_STATS");

    Bracket* bracket = _player->getBracket(BRACKET_TYPE_RATED_BG);
    ASSERT(bracket);

    WorldPacket data(SMSG_BATTLEFIELD_RATED_INFO, 29);
    data << uint32(bracket->getRating());  //rating
    data << uint32(0);  //unk1
    data << uint32(0);  //unk2
    //data << _player->GetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_META_BG, true);
    data << uint32(0);  //unk3
    data << _player->GetCurrency(CURRENCY_TYPE_CONQUEST_POINTS) / GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS);
    data << uint8(3);   //unk4
    data << uint32(0);  //unk5
    SendPacket(&data);
}

//! This is const data used for calc some field for SMSG_PVP_RATED_STATS 
void WorldSession::HandlePersonalRatedInfoRequest(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_PERSONAL_RATED_INFO_REQUEST");
    WorldPacket data(SMSG_PERSONAL_RATED_INFO, 20);
    data << uint32(3500);
    data << float(1639.28f);
    data << uint32(2000);
    data << float(1511.26f);
    data << float(0.00412f);
    SendPacket(&data);
}
