/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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

#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "Opcodes.h"

#include "Bracket.h"

//This send to player windows for invite player to join the war
//Param1:(m_Guid) the BattleId of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
//Param3:(time) Time in second that the player have for accept
//! 5.4.1
void WorldSession::SendBfInvitePlayerToWar(uint64 guid, uint32 zoneId, uint32 pTime)
{
    //Send packet
    ObjectGuid guidBytes = guid;
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE);

    data << uint32(time(NULL) + pTime); // Invite lasts until
    data << uint32(zoneId);         // Zone Id
    data.WriteGuidMask<0, 4, 3, 6, 5, 7, 1, 2>(guidBytes);
    data.WriteGuidBytes<5, 4, 6, 1, 3, 7, 0, 2>(guidBytes);

    //Sending the packet to player
    SendPacket(&data);
}

//This send invitation to player to join the queue
//Param1:(guid) the guid of Bf
//! 5.4.1
void WorldSession::SendBfInvitePlayerToQueue(uint64 guid)
{
    ObjectGuid guidBytes = guid;
    bool warmup = true;

    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE);
    
    data.WriteBit(1); // 24
    data.WriteBit(!warmup); // warmup
    data.WriteGuidMask<5>(guidBytes);
    data.WriteBit(1); // 1c
    data.WriteGuidMask<2>(guidBytes);
    data.WriteBit(0); // 30
    data.WriteGuidMask<3, 6>(guidBytes);
    data.WriteBit(1); // 2c
    data.WriteGuidMask<4, 1>(guidBytes);
    data.WriteBit(1); // 20
    data.WriteBit(1); // 28
    data.WriteGuidMask<7, 0>(guidBytes);

    data.WriteGuidBytes<4, 5, 1, 0>(guidBytes);
    //if (v2C)
    //    p.ReadInt32("dword2C");
    data.WriteGuidBytes<2>(guidBytes);
    data << uint8(1);  //warmup. 1 - WG, 2 - TB
    //if (v1C)
    //    p.ReadInt32("dword1C");

    //if (v24)
    //    p.ReadInt32("dword24");

    //if (v28)
    //    p.ReadInt32("dword28");
    data.WriteGuidBytes<3, 7>(guidBytes);
    //if (v20)
    //    p.ReadInt32("dword20");
    data.WriteGuidBytes<6>(guidBytes);

    //Sending packet to player
    SendPacket(&data);
}

//This send packet for inform player that he join queue
//Param1:(guid) the guid of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
//Param3:(CanQueue) if able to queue
//Param4:(Full) on log in is full
//! 5.4.1
void WorldSession::SendBfQueueInviteResponse(uint64 guid,uint32 ZoneId, bool CanQueue, bool Full)
{
    bool hasSecondGuid = false;
    ObjectGuid bgGuid = guid;
    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE);
    data.WriteGuidMask<6, 2, 4>(bgGuid);
    data.WriteBit((Full ? 0 : 1)); // //Logging In        //0 wg full                 //1 queue for upcoming (we may need to swap it)
    data.WriteGuidMask<1>(bgGuid);
    data.WriteBit(!hasSecondGuid); // Has Second guid
    //p.StartBitStream(guid20, 1, 4, 2, 3, 6, 0, 5, 7);
    data.WriteGuidMask<0, 3, 7, 5>(bgGuid);

    //p.ParseBitStream(guid20, 2, 5, 4, 3, 0, 7, 1, 6);
    data.WriteGuidBytes<4>(bgGuid);
    data << uint32(ZoneId);
    data.WriteGuidBytes<1>(bgGuid);
    data << uint8(1); // Warmup
    data.WriteGuidBytes<3, 0, 7, 5>(bgGuid);
    data << uint8((CanQueue ? 1 : 0));  //Accepted          //0 you cannot queue wg     //1 you are queued
    data.WriteGuidBytes<6, 2>(bgGuid);

    SendPacket(&data);
}

//This is call when player accept to join war
//Param1:(guid) the guid of Bf
//! 5.4.1
void WorldSession::SendBfEntered(uint64 guid)
{
    ObjectGuid bgGuid = guid;
//    m_PlayerInWar[player->GetTeamId()].insert(player->GetGUID());
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTERED);
    data.WriteGuidMask<6>(bgGuid);
    data.WriteBit(1); // unk
    data.WriteGuidMask<1>(bgGuid);
    data.WriteBit(_player->isAFK() ? 1 : 0); //Clear AFK
    data.WriteBit(1); // unk
    data.WriteGuidMask<7, 2, 5, 4, 3, 0>(bgGuid);

    data.WriteGuidBytes<4, 5, 3, 1, 7, 6, 0, 2>(bgGuid);
    SendPacket(&data);
}

//! 5.4.1
void WorldSession::SendBfLeaveMessage(uint64 guid, BFLeaveReason reason)
{
    ObjectGuid bgGuid = guid;
    WorldPacket data(SMSG_BATTLEFIELD_MGR_EJECTED);
    data << uint8(2); // byte BattleStatus
    data << uint8(reason); // byte Reason

    data.WriteGuidMask<2, 7>(bgGuid);
    data.WriteBit(0); // relocated
    data.WriteGuidMask<0, 1, 4, 5, 3, 6>(bgGuid);

    data.FlushBits();

    data.WriteGuidBytes<6, 7, 1, 5, 4, 0, 2, 3>(bgGuid);

    SendPacket(&data);
}

//Send by client when he click on accept for queue
//! 5.4.1
void WorldSession::HandleBfQueueInviteResponse(WorldPacket & recvData)
{
    uint8 accepted;
    ObjectGuid guid;

    recvData.ReadGuidMask<6, 5, 1, 0>(guid);
    accepted = recvData.ReadBit();
    recvData.ReadGuidMask<3, 7, 4, 2>(guid);

    recvData.ReadGuidBytes< 2, 3, 6, 1, 5, 0, 4, 7>(guid);
    sLog->outError(LOG_FILTER_GENERAL, "HandleQueueInviteResponse: GUID:" UI64FMTD " Accepted:%u", (uint64)guid, accepted);

    if(!accepted)
        return;

    Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid);
    if (!bf)
        return;
    
    if (accepted)
        bf->PlayerAcceptInviteToQueue(_player);
    else if (bf->IsWarTime() || bf->IsOnStartGrouping())
    {
        if (_player->GetZoneId() == bf->GetZoneId())
            bf->KickPlayerFromBattlefield(_player->GetGUID());
    }
}

//Send by client on clicking in accept or refuse of invitation windows for join game
//! 5.4.1
void WorldSession::HandleBfEntryInviteResponse(WorldPacket & recvData)
{
    uint8 accepted;
    ObjectGuid guid;
    recvData.ReadGuidMask<4, 7, 6, 2, 3, 1, 0>(guid);
    accepted = recvData.ReadBit();
    recvData.ReadGuidMask<5>(guid);

    recvData.ReadGuidBytes<7, 1, 0, 6, 2, 4, 3, 5>(guid);

    sLog->outError(LOG_FILTER_GENERAL, "HandleBattlefieldInviteResponse: GUID:" UI64FMTD " Accepted:%u", uint64(guid), accepted);

    Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid);
    if (!bf)
        return;

    if (accepted)
        bf->PlayerAcceptInviteToWar(_player);
    else
        if (_player->GetZoneId() == bf->GetZoneId())
            bf->KickPlayerFromBattlefield(_player->GetGUID());
}

//! 5.4.1
void WorldSession::HandleBfQueueRequest(WorldPacket& recvData)
{
    ObjectGuid guid;

    recvData.ReadGuidMask<6, 3, 1, 2, 0, 4, 7, 5>(guid);
    recvData.ReadGuidBytes<4, 3, 0, 1, 6, 5, 2, 7>(guid);

    sLog->outError(LOG_FILTER_GENERAL, "HandleBfQueueRequest: GUID:" UI64FMTD " ", (uint64)guid);

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid))
    {
        if (bf->IsWarTime())
            bf->InvitePlayerToWar(_player);
        else
        {
            uint32 timer = bf->GetTimer() / 1000;
            if (timer < 15 * MINUTE)
                bf->InvitePlayerToQueue(_player);
        }
    }
}


//! 5.4.1
void WorldSession::HandleBfExitQueueRequest(WorldPacket & recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<0, 7, 5, 2, 4, 1, 6, 3>(guid);
    recvData.ReadGuidBytes<7, 1, 2, 5, 0, 6, 4, 3>(guid);

    sLog->outError(LOG_FILTER_GENERAL, "HandleBfExitQueueRequest: GUID:" UI64FMTD " ", (uint64)guid);

    SendBfLeaveMessage(guid);

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid))
        bf->AskToLeaveQueue(_player);
}

void WorldSession::HandleBfExitRequest(WorldPacket& recv_data)
{
    sLog->outError(LOG_FILTER_GENERAL, "HandleBfExitRequest");
    Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(_player->GetZoneId());

    if (!bf)
        return;

    SendBfLeaveMessage(bf->GetBattleId());
    bf->AskToLeaveQueue(_player);
    bf->KickPlayerFromBattlefield(_player->GetGUID());
}

//! 5.4.1
void WorldSession::HandleReportPvPAFK(WorldPacket & recvData)
{
    ObjectGuid playerGuid;
    recvData.ReadGuidMask<5, 3, 7, 0, 4, 2, 6, 1>(playerGuid);
    recvData.ReadGuidBytes<5, 7, 3, 0, 2, 6, 4, 1>(playerGuid);
    Player* reportedPlayer = ObjectAccessor::FindPlayer(playerGuid);

    if (!reportedPlayer)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "WorldSession::HandleReportPvPAFK: player not found");
        return;
    }

    sLog->outDebug(LOG_FILTER_BATTLEGROUND, "WorldSession::HandleReportPvPAFK: %s reported %s", _player->GetName(), reportedPlayer->GetName());

    reportedPlayer->ReportedAfkBy(_player);
}

void WorldSession::HandleBfSetPreferedCemetry(WorldPacket & recvData)
{
    uint32 graveID = 0;
    recvData >> graveID;
    sLog->outError(LOG_FILTER_GENERAL, "HandleBfSetPreferedCemetry: GraveID: %u", graveID);
}