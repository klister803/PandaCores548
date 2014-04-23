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
#include "Language.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "World.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "ChallengeMgr.h"

void WorldSession::HandleChallengeModeRequestOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_CHALLENGE_MODE_REQUEST_LEADERS");

    uint32 mapID, unk, time;
    recvPacket >> mapID >> unk >> time;

    ObjectGuid guid = _player->GetGUID();

    WorldPacket data(SMSG_CHALLENGE_MODE_REQUEST_LEADERS_RESULT);
    data.WriteBits(0, 19);  //guild
    data.WriteBits(1, 19);  //server
    //
    {
        data.WriteBits(1, 20);
        data.WriteGuidMask<6, 0, 2, 3, 1, 4, 7, 5>(guid);
    }

    data.FlushBits();

    {
        {
            data.WriteGuidBytes<2>(guid);
            data << uint32(realmID);
            data << uint32(262);                  //specID
            data.WriteGuidBytes< 4, 7, 6, 3, 5, 1>(guid);
            data << uint32(realmID);
            data.WriteGuidBytes<0>(guid);
        }
        data << uint32(246155);                 // 246155
        data << uint32(realmID);                // 50659408
        data << uint32(3);                      // seasonID
        data.AppendPackedTime(sWorld->GetGameTime());
        data << uint32(624653);         //recorde time on ms
    }
    data << uint32(getMSTime());
    data << mapID;
    data << uint32(624653);
    SendPacket(&data);
}

void WorldSession::HandleChallengeModeRequestRewardInfoOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_CHALLENGE_MODE_REQUEST_REWARD_INFO");

    ByteBuffer dataBuffer;

    WorldPacket data(SMSG_CHALLENGE_MODE_REWARD_INFO, 1000);
    data.WriteBits(0, 20);                                           //Guild
    data.WriteBits(sMapChallengeModeStore.GetFieldCount()-1, 21);    //Self

    for (uint32 i = 0; i < sMapChallengeModeStore.GetNumRows(); ++i)
    {
        if (MapChallengeModeEntry const* mChallenge = sMapChallengeModeStore.LookupEntry(i))
        {
            data.WriteBits(4, 20);
            for(int32 i = 0; i < 4; ++i)
            {
                data.WriteBits(1, 21);
                data.WriteBits(0, 20);

                //count14_d0::dword8
                {
                    dataBuffer << uint32(CURRENCY_TYPE_VALOR_POINTS);              //Currency Reward ID
                    dataBuffer << uint32(10500 + (i*2000));                        //Reward Count
                }
                dataBuffer << uint32(198450);   //best time in seconds.
            }
            dataBuffer << uint32(mChallenge->map);
        }
    }
    data.FlushBits();
    data.append(dataBuffer);
    
    SendPacket(&data);
}

void WorldSession::HandleChallengeModeRequestCompletionInfoOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_CHALLENGE_MODE_REQUEST_COMPLETION_INFO");

    WorldPacket data(SMSG_CHALLENGE_MODE_COMPLETION_INFO, 100);
    data.WriteBits(1, 19);
    data.WriteBits(5, 23);                              //numRewards 
    data.FlushBits();
    {
        data << uint32(500790);                         //Record Time
        data.AppendPackedTime(sWorld->GetGameTime());   //completionTime
        data << uint32(960);
        //for(int32 i = 0; i < 4; ++i)
        {
            data << uint16(251);
            data << uint16(105);
            data << uint16(73);
            data << uint16(255);
            data << uint16(269);
        }
        data << uint32(CHALLENGE_MEDAL_PLAT);           //medal
        data << uint32(1012790);                        //Last Record Time
    }
    SendPacket(&data);
}