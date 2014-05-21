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

    Challenge * bestServer = sChallengeMgr->BestServerChallenge(mapID);
    Challenge * bestGuild = sChallengeMgr->BestGuildChallenge(_player->GetGuildId(), mapID);

    WorldPacket data(SMSG_CHALLENGE_MODE_REQUEST_LEADERS_RESULT);
    data.WriteBits(bestGuild ? 1 : 0, 19);      //guild
    data.WriteBits(bestServer ? 1 : 0, 19);      //server

    if (bestGuild)
    {
        data.WriteBits(bestGuild->member.size(), 20);
        for(ChallengeMemberList::iterator itr = bestGuild->member.begin(); itr != bestGuild->member.end(); ++itr)
        {
            ChallengeMember member = *itr;
            data.WriteGuidMask<5, 1, 4, 2, 6, 3, 0, 7>(member.guid);
        }
    }

    if (bestServer)
    {
        data.WriteBits(bestServer->member.size(), 20);
        for(ChallengeMemberList::iterator itr = bestServer->member.begin(); itr != bestServer->member.end(); ++itr)
        {
            ChallengeMember member = *itr;
            data.WriteGuidMask<6, 0, 2, 3, 1, 4, 7, 5>(member.guid);
        }
    }


    data.FlushBits();

    if (bestServer)
    {
        for(ChallengeMemberList::iterator itr = bestServer->member.begin(); itr != bestServer->member.end(); ++itr)
        {
            ChallengeMember member = *itr;
            data.WriteGuidBytes<2>(member.guid);
            data << uint32(realmID);
            data << uint32(member.specId);
            data.WriteGuidBytes<4, 7, 6, 3, 5, 1>(member.guid);
            data << uint32(realmID);
            data.WriteGuidBytes<0>(member.guid);
        }
        data << uint32(bestServer->guildId);    // 246155
        data << uint32(realmID);                // 50659408
        data << uint32(3);                      // seasonID
        data.AppendPackedTime(bestServer->date);
        data << uint32(bestServer->recordTime);  //recorde time on ms
    }

    if (bestGuild)
    {
        data << uint32(3);                      // seasonID
        data << uint32(bestGuild->guildId);     // 246155
        for(ChallengeMemberList::iterator itr = bestGuild->member.begin(); itr != bestGuild->member.end(); ++itr)
        {
            ChallengeMember member = *itr;

            data.WriteGuidBytes<7, 0>(member.guid);
            data << uint32(realmID);
            data.WriteGuidBytes<4, 1, 3, 5, 2>(member.guid);
            data << uint32(member.specId);
            data << uint32(realmID);
            data.WriteGuidBytes<6>(member.guid);
        }
        data.AppendPackedTime(bestServer->date);
        data << uint32(realmID);                // 50659408
        data << uint32(bestServer->recordTime);  //recorde time on ms
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
            for(int32 i = CHALLENGE_MEDAL_NONE; i < CHALLENGE_MEDAL_PLAT; ++i)
            {
                data.WriteBits(1, 21);
                data.WriteBits(0, 20);

                //count14_d0::dword8
                {
                    dataBuffer << uint32(CURRENCY_TYPE_VALOR_POINTS);              //Currency Reward ID
                    dataBuffer << uint32(sChallengeMgr->GetValorPointsReward(i));  //Reward Count
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

    ChallengeByMap * best = sChallengeMgr->BestForMember(_player->GetGUID());

    data.WriteBits(best ? best->size() : 0, 19);
    if(best)
    {
        for(ChallengeByMap::iterator itr = best->begin(); itr != best->end(); ++itr)
            data.WriteBits(itr->second->member.size(), 23);                  //num membeers 

        data.FlushBits();

        for(ChallengeByMap::iterator itr = best->begin(); itr != best->end(); ++itr)
        {
            data << uint32(itr->second->recordTime);                         //Record Time
            data.AppendPackedTime(itr->second->date);                        //completionTime
            data << uint32(itr->second->mapID);

            for(ChallengeMemberList::iterator i = itr->second->member.begin(); i != itr->second->member.end(); ++i)
            {
                ChallengeMember member = *i;
                data << uint16(member.specId);
            }
            data << uint32(itr->second->medal);                              //medal
            //ToDo: need create one more holder on challenge mgr with last record. Is it trully need?
            data << uint32(itr->second->recordTime);                         //Last Record Time
        }
    }
    SendPacket(&data);
}