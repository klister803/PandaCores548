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
#include "ObjectMgr.h"
#include "World.h"
#include "WorldPacket.h"

#include "BattlegroundMgr.h"
#include "BattlegroundAV.h"
#include "BattlegroundAB.h"
#include "BattlegroundEY.h"
#include "BattlegroundWS.h"
#include "BattlegroundNA.h"
#include "BattlegroundBE.h"
#include "BattlegroundAA.h"
#include "BattlegroundRL.h"
#include "BattlegroundSA.h"
#include "BattlegroundDS.h"
#include "BattlegroundRV.h"
#include "BattlegroundIC.h"
#include "BattlegroundRB.h"
#include "BattlegroundTP.h"
#include "BattlegroundBFG.h"
#include "BattlegroundKT.h"
#include "BattlegroundSSM.h"
#include "BattlegroundTTP.h"
#include "BattlegroundTV.h"
#include "BattlegroundDG.h"
#include "Chat.h"
#include "Map.h"
#include "MapInstanced.h"
#include "MapManager.h"
#include "Player.h"
#include "GameEventMgr.h"
#include "SharedDefines.h"
#include "Formulas.h"
#include "DisableMgr.h"
#include "BracketMgr.h"
#include "LFG.h"

/*********************************************************/
/***            BATTLEGROUND MANAGER                   ***/
/*********************************************************/

BattlegroundMgr::BattlegroundMgr() : m_ArenaTesting(false)
{
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; i++)
        m_Battlegrounds[i].clear();
    m_NextRatedArenaUpdate = sWorld->getIntConfig(CONFIG_ARENA_RATED_UPDATE_TIMER);
    m_Testing = false;
    holidayWS = 0;
}

BattlegroundMgr::~BattlegroundMgr()
{
    DeleteAllBattlegrounds();
}

void BattlegroundMgr::DeleteAllBattlegrounds()
{
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        for (BattlegroundSet::iterator itr = m_Battlegrounds[i].begin(); itr != m_Battlegrounds[i].end();)
        {
            Battleground* bg = itr->second;
            m_Battlegrounds[i].erase(itr++);
            if (!m_ClientBattlegroundIds[i][bg->GetBracketId()].empty())
                m_ClientBattlegroundIds[i][bg->GetBracketId()].erase(bg->GetClientInstanceID());
            delete bg;
        }
    }

    // destroy template battlegrounds that listed only in queues (other already terminated)
    for (uint32 bgTypeId = 0; bgTypeId < MAX_BATTLEGROUND_TYPE_ID; ++bgTypeId)
    {
        // ~Battleground call unregistring BG from queue
        while (!BGFreeSlotQueue[bgTypeId].empty())
            delete BGFreeSlotQueue[bgTypeId].front();
    }
}

// used to update running battlegrounds, and delete finished ones
void BattlegroundMgr::Update(uint32 diff)
{
    BattlegroundSet::iterator itr, next;
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        itr = m_Battlegrounds[i].begin();
        // skip updating battleground template
        if (itr != m_Battlegrounds[i].end())
            ++itr;
        for (; itr != m_Battlegrounds[i].end(); itr = next)
        {
            next = itr;
            ++next;
            itr->second->Update(diff);
            // use the SetDeleteThis variable
            // direct deletion caused crashes
            if (itr->second->ToBeDeleted())
            {
                Battleground* bg = itr->second;
                m_Battlegrounds[i].erase(itr);
                if (!m_ClientBattlegroundIds[i][bg->GetBracketId()].empty())
                    m_ClientBattlegroundIds[i][bg->GetBracketId()].erase(bg->GetClientInstanceID());

                delete bg;
            }
        }
    }

    // update events timer
    for (int qtype = BATTLEGROUND_QUEUE_NONE; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        m_BattlegroundQueues[qtype].UpdateEvents(diff);

    // update scheduled queues
    if (!m_QueueUpdateScheduler.empty())
    {
        std::vector<QueueSchedulerItem*> scheduled;
        {
            //copy vector and clear the other
            scheduled = std::vector<QueueSchedulerItem*>(m_QueueUpdateScheduler);
            m_QueueUpdateScheduler.clear();
            //release lock
        }

        for (uint8 i = 0; i < scheduled.size(); i++)
        {
            uint32 MMRating = scheduled[i]->_MMRating;
            uint8 joinType = scheduled[i]->_joinType;
            BattlegroundQueueTypeId bgQueueTypeId = scheduled[i]->_bgQueueTypeId;
            BattlegroundTypeId bgTypeId = scheduled[i]->_bgTypeId;
            BattlegroundBracketId bracket_id = scheduled[i]->_bracket_id;
            m_BattlegroundQueues[bgQueueTypeId].BattlegroundQueueUpdate(diff, bgTypeId, bracket_id, joinType, MMRating > 0, MMRating);
        }
           
        for (std::vector<QueueSchedulerItem*>::iterator itr = scheduled.begin();
            itr != scheduled.end(); ++itr)
            delete *itr;
    }

    for (uint8 i = BATTLEGROUND_QUEUE_2v2; i <= BATTLEGROUND_QUEUE_5v5; i++)
        m_BattlegroundQueues[i].HandleArenaQueueUpdate(diff, BattlegroundMgr::BGJoinType(BattlegroundQueueTypeId(i)));

    // if rating difference counts, maybe force-update queues
//     if (sWorld->getIntConfig(CONFIG_ARENA_MAX_RATING_DIFFERENCE) && sWorld->getIntConfig(CONFIG_ARENA_RATED_UPDATE_TIMER))
//     {
//         // it's time to force update
//         if (m_NextRatedArenaUpdate < diff)
//         {
//             // forced update for rated arenas (scan all, but skipped non rated)
//             sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BattlegroundMgr: UPDATING ARENA QUEUES");
//             for (int qtype = BATTLEGROUND_QUEUE_2v2; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
//                 for (int bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
//                 {
//                     m_BattlegroundQueues[qtype].BattlegroundQueueUpdate(diff, BATTLEGROUND_AA, BattlegroundBracketId(bracket),
//                         BattlegroundMgr::BGJoinType(BattlegroundQueueTypeId(qtype)), false, 0);
//                 }
// 
//             m_NextRatedArenaUpdate = sWorld->getIntConfig(CONFIG_ARENA_RATED_UPDATE_TIMER);
//         }
//         else
//             m_NextRatedArenaUpdate -= diff;
//     }
}

//! ToDo: arenatype -> JoinType
void BattlegroundMgr::BuildBattlegroundStatusPacket(WorldPacket* data, Battleground* bg, Player * pPlayer, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, uint8 arenatype, uint64 bgGUID)
{
    // we can be in 2 queues in same time...
    if (!bg)
        StatusID = STATUS_NONE;

    ObjectGuid guidBytes1 = pPlayer->GetGUID();
    ObjectGuid guidBytes2 = bgGUID ? bgGUID : bg ? bg->GetGUID() : 0;

    switch (StatusID)
    {
        case STATUS_NONE:
        {
            //! 5.4.1
            data->Initialize(SMSG_BATTLEFIELD_STATUS);

            data->WriteGuidMask<2, 0, 3, 6, 1, 5, 4, 7>(guidBytes1);
            data->WriteGuidBytes<7, 3, 2, 6, 4>(guidBytes1);
            *data << uint32(Time1);                                                                      // Join Time. Posible status.
            data->WriteGuidBytes<5>(guidBytes1);
            *data << uint32(QueueSlot);                                                                  // Queue slot
            data->WriteGuidBytes<1>(guidBytes1);
            if (bg)
                *data << uint32((bg->isArena() || bg->IsRBG()) ? arenatype : 1);                         // unk, always 1
            else
                *data << uint32(1);
            data->WriteGuidBytes<0>(guidBytes1);
            break;
        }
        case STATUS_WAIT_QUEUE:
        {
            //! 5.4.1
            data->Initialize(SMSG_BATTLEFIELD_STATUS_QUEUED);

            data->WriteGuidMask<7>(guidBytes1);
            data->WriteGuidMask<0>(guidBytes2);
            data->WriteGuidMask<1, 0, 6, 2, 3>(guidBytes1);
            data->WriteGuidMask<5, 7>(guidBytes2);
            data->WriteGuidMask<5>(guidBytes1);
            data->WriteGuidMask<2>(guidBytes2);
            data->WriteGuidMask<4>(guidBytes1);
            data->WriteGuidMask<1>(guidBytes2);
            data->WriteBit(bg->isRated());
            data->WriteBit(0);   // Join Failed, 1 when it's arena ...
            data->WriteGuidMask<4>(guidBytes2);
            data->WriteBit(1); // // Eligible In Queue
            data->WriteGuidMask<3>(guidBytes2);
            data->WriteBit(0);  // Waiting On Other Activity // JoinAsGroup
            data->WriteGuidMask<6>(guidBytes2);

            data->FlushBits();

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            *data << uint8(0);
            *data << uint32(Time1);             //Time of the join
            data->WriteGuidBytes<3>(guidBytes1);
            data->WriteGuidBytes<1>(guidBytes2);
            data->WriteGuidBytes<2>(guidBytes1);
            *data << uint32((bg->isArena() || bg->IsRBG()) ? arenatype : 1);    //should be max 
            *data << uint32(Time2);                     // Estimated Wait Time
            data->WriteGuidBytes<7>(guidBytes2);
            data->WriteGuidBytes<4>(guidBytes1);
            data->WriteGuidBytes<0>(guidBytes2);
            data->WriteGuidBytes<6>(guidBytes1);
            *data << uint32(GetMSTimeDiffToNow(Time2));
            *data << uint8(bg->GetMinLevel()); //BG Min level. ToDo: for arena basic template has always 10 min.lvl.
            *data << uint32(QueueSlot);
            *data << uint8(0);
            data->WriteGuidBytes<1>(guidBytes1);
            data->WriteGuidBytes<2, 6, 3, 5>(guidBytes2);
            data->WriteGuidBytes<7, 0>(guidBytes1);
            data->WriteGuidBytes<4>(guidBytes2);
            data->WriteGuidBytes<5>(guidBytes1);
            break;
        }
        case STATUS_WAIT_JOIN:
        {
            //! 5.4.1
            data->Initialize(SMSG_BATTLEFIELD_STATUS_NEEDCONFIRMATION, 44);
            
            uint8 roles = pPlayer->GetBattleGroundRoles();

            data->WriteBit(roles == lfg::PLAYER_ROLE_DAMAGE);
            data->WriteGuidMask<2>(guidBytes2);
            data->WriteGuidMask<1, 7>(guidBytes1);
            data->WriteGuidMask<5, 3, 7>(guidBytes2);
            data->WriteGuidMask<6, 2, 0, 4, 3>(guidBytes1);
            data->WriteGuidMask<1, 0>(guidBytes2);
            data->WriteBit(bg->isRated());                  // Is Rated
            data->WriteGuidMask<5>(guidBytes1);
            data->WriteGuidMask<4, 6>(guidBytes2);

            data->FlushBits();

            *data << uint8(0);
            *data << uint32(Time1);                     // Time until closed
            data->WriteGuidBytes<6>(guidBytes2);
            *data << uint8(bg->GetMinLevel());
            data->WriteGuidBytes<3>(guidBytes1);
            *data << uint8(0);                          // bool. hide exit on sub invite.
            data->WriteGuidBytes< 6, 4>(guidBytes1);
            data->WriteGuidBytes<0, 1, 2, 3>(guidBytes2);
            data->WriteGuidBytes<7>(guidBytes1);
            *data << uint32(bg->GetClientInstanceID());

            if (roles != lfg::PLAYER_ROLE_DAMAGE)
                *data << uint8(roles == lfg::PLAYER_ROLE_TANK ? 0 : 1);
            
            *data << uint32(QueueSlot);
            data->WriteGuidBytes<7>(guidBytes2);
            *data << uint32(bg->GetMapId());
            *data << uint32((bg->isArena() || bg->IsRBG()) ? arenatype : 1);
            data->WriteGuidBytes<0, 2>(guidBytes1);
            *data << uint32(Time2);
            data->WriteGuidBytes<5>(guidBytes1);
            data->WriteGuidBytes<4, 5>(guidBytes2);
            data->WriteGuidBytes<1>(guidBytes1);
            
            break;
        }
        case STATUS_IN_PROGRESS:
        {
            //! 5.4.1
            data->Initialize(SMSG_BATTLEFIELD_STATUS_ACTIVE, 49);

            data->WriteBit(bg->isRated());
            data->WriteGuidMask<5, 2>(guidBytes1);
            data->WriteGuidMask<0, 7>(guidBytes2);
            data->WriteGuidMask<7, 6>(guidBytes1);
            data->WriteGuidMask<1, 2>(guidBytes2);
            data->WriteGuidMask<1, 4>(guidBytes1);
            data->WriteBit(bg->isRated() && bg->GetStatus() != STATUS_WAIT_LEAVE);  // Block
            data->WriteGuidMask<3>(guidBytes2);
            data->WriteGuidMask<0, 3>(guidBytes1);
            data->WriteGuidMask<6, 5, 4>(guidBytes2);
            data->WriteBit(pPlayer->GetBGTeam() == HORDE ? 0 : 1);      // Battlefield Faction ( 0 horde, 1 alliance )

            data->FlushBits();

            data->WriteGuidBytes<7>(guidBytes2);
            data->WriteGuidBytes<5, 4>(guidBytes1);
            data->WriteGuidBytes<6>(guidBytes2);
            *data << uint32(Time1);                                    // Time
            data->WriteGuidBytes<5>(guidBytes2);
            data->WriteGuidBytes<6>(guidBytes1);
            data->WriteGuidBytes<3>(guidBytes2);
            *data << uint32((bg->isArena() || bg->IsRBG()) ? arenatype : 1);
            *data << uint32(bg->GetMapId());            // Map Id
            data->WriteGuidBytes<3>(guidBytes1);
            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            data->WriteGuidBytes<1, 7>(guidBytes1);
            *data << uint32(bg->GetRemainingTime());                     // Time until closed
            data->WriteGuidBytes<0>(guidBytes1);
            data->WriteGuidBytes<4>(guidBytes2);
            *data << uint8(0);                          // unk
            *data << uint32(QueueSlot);                 // Queue slot
            data->WriteGuidBytes<0, 2>(guidBytes2);
            *data << uint8(0);                          // unk
            *data << uint32(Time2);
            data->WriteGuidBytes<1>(guidBytes2);
            data->WriteGuidBytes<2>(guidBytes1);
            *data << uint8(bg->GetMinLevel());          // Min Level
            break;
        }
        case STATUS_WAIT_LEAVE:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_WAITFORGROUPS, 48);
            
			*data << uint8(bg->GetMinLevel());
			*data << uint8(0);          // byte3A
			*data << uint32(bg->GetStatus());
			*data << uint32(bg->GetClientInstanceID());
			*data << uint32(bg->GetMapId());            // time1OrMapId
			*data << uint8(0);          // byte48
			*data << uint8(0);          // byte49
			*data << uint8(0);          // byte38
			*data << uint32(bg->GetMapId());            // mapIdOrTime1
			*data << uint32(Time2);
			*data << uint8(0);          // byte4B
            *data << uint32(QueueSlot);
			*data << uint8(0);          // byte4A      

			data->WriteGuidMask<5, 4>(guidBytes2);
			data->WriteGuidMask<2>(guidBytes1);
			data->WriteGuidMask<7>(guidBytes2);
			data->WriteGuidMask<0>(guidBytes1);
			data->WriteBit(bg->isRated());
			data->WriteGuidMask<7>(guidBytes1);
			data->WriteGuidMask<0, 1>(guidBytes2);
			data->WriteGuidMask<3, 5, 1>(guidBytes1);
			data->WriteGuidMask<2>(guidBytes2);
			data->WriteGuidMask<4>(guidBytes1);
			data->WriteGuidMask<6, 3>(guidBytes2);
			data->WriteGuidMask<6>(guidBytes1);

            data->FlushBits();

			data->WriteGuidBytes<7, 2, 4, 6>(guidBytes1);
			data->WriteGuidBytes<5, 4, 2, 1>(guidBytes2);
			data->WriteGuidBytes<5, 1, 0, 3>(guidBytes1);
			data->WriteGuidBytes<3, 0, 6, 7>(guidBytes2);            
            break;
        }
    }
}

void BattlegroundMgr::BuildPvpLogDataPacket(WorldPacket* data, Battleground* bg)
{
    BracketType bType = BattlegroundMgr::BracketByJoinType(bg->GetJoinType());
    uint8 isRated = (bg->isRated() ? 1 : 0);               // type (normal=0/rated=1) -- ATM arena or bg, RBG NYI
    uint8 isArena = (bg->isArena() ? 1 : 0);               // Arena names
    uint8 counta2 = 0;
    uint8 counth2 = 0;

    //! 5.4.1
    data->Initialize(SMSG_PVP_LOG_DATA, (1+1+4+40*bg->GetPlayerScoresSize()));

    size_t count_pos = data->bitwpos();
    data->WriteBits(0, 19);     // Placeholder

    int32 count = 0;
    uint32 team = 0;
    ByteBuffer buff;
    Player* player = NULL;
    Bracket* bracket = NULL;
    ObjectGuid guid = 0;
    Battleground::BattlegroundScoreMap::const_iterator itr2 = bg->GetPlayerScoresBegin();
    for (Battleground::BattlegroundScoreMap::const_iterator itr = itr2; itr != bg->GetPlayerScoresEnd();)
    {
        itr2 = itr++;
        team = itr2->second->Team;

        // Check if player finish game. If he leave game he will be true untill _ProcessOfflineQueue
        if (!bg->IsPlayerInBattleground(itr2->first))
        {
            //sLog->outError(LOG_FILTER_BATTLEGROUND, "Player " UI64FMTD " has scoreboard entry for battleground %u but is not in battleground!", itr->first, bg->GetTypeID(true));
            continue;
        }

        guid = itr2->first;
        player = ObjectAccessor::FindPlayer(itr2->first);

        if (isRated)    //bracket used only on rated bg, no need find it for non ranked
            bracket = player ? player->getBracket(bType) : sBracketMgr->TryGetOrCreateBracket(itr2->first, bType);
        else
            bracket = NULL;

        buff.WriteGuidBytes<2, 7>(guid);
        buff << int32(player ? player->GetSpecializationId(player->GetActiveSpec()) : 0);
        buff.WriteGuidBytes<3>(guid);

        if (isRated)
            buff << int32(bracket->getMMV() - bracket->getLastMMRChange());

        if (!isArena) // Unk 3 prolly is (bg)
        {
            buff << uint32(itr2->second->Deaths);
            buff << uint32(itr2->second->HonorableKills);  
            buff << uint32(itr2->second->BonusHonor / 100);
        }

        switch (bg->GetTypeID(true))                             // Custom values
        {
            case BATTLEGROUND_RB:
                switch (bg->GetMapId())
                {
                    case 489:
                        data->WriteBits(2, 22);
                        buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagCaptures);        // flag captures
                        buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagReturns);         // flag returns
                        break;
                    case 566:
                        data->WriteBits(1, 22);
                        buff << uint32(((BattlegroundEYScore*)itr2->second)->FlagCaptures);        // flag captures
                        break;
                    case 529:
                        data->WriteBits(2, 22);
                        buff << uint32(((BattlegroundABScore*)itr2->second)->BasesAssaulted);      // bases asssulted
                        buff << uint32(((BattlegroundABScore*)itr2->second)->BasesDefended);       // bases defended
                        break;
                    case 30:
                        data->WriteBits(5, 22);
                        buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsAssaulted); // GraveyardsAssaulted
                        buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsDefended);  // GraveyardsDefended
                        buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersAssaulted);     // TowersAssaulted
                        buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersDefended);      // TowersDefended
                        buff << uint32(((BattlegroundAVScore*)itr2->second)->MinesCaptured);       // MinesCaptured
                        break;
                    case 607:
                        data->WriteBits(2, 22);
                        buff << uint32(((BattlegroundSAScore*)itr2->second)->demolishers_destroyed);
                        buff << uint32(((BattlegroundSAScore*)itr2->second)->gates_destroyed);
                        break;
                    case 628:                                   // IC
                        data->WriteBits(2, 22);
                        buff << uint32(((BattlegroundICScore*)itr2->second)->BasesAssaulted);       // bases asssulted
                        buff << uint32(((BattlegroundICScore*)itr2->second)->BasesDefended);        // bases defended
                        break;
                    case 726:
                        data->WriteBits(2, 22);
                        buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagCaptures);         // flag captures
                        buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagReturns);          // flag returns
                        break;
                    case 761:
                        data->WriteBits(2, 22);
                        buff << uint32(((BattlegroundBFGScore*)itr2->second)->BasesAssaulted);      // bases asssulted
                        buff << uint32(((BattlegroundBFGScore*)itr2->second)->BasesDefended);       // bases defended
                        break;
                    default:
                        data->WriteBits(0, 22);
                        break;
                }
                break;
            case BATTLEGROUND_AV:
                data->WriteBits(5, 22);
                buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsAssaulted); // GraveyardsAssaulted
                buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsDefended);  // GraveyardsDefended
                buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersAssaulted);     // TowersAssaulted
                buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersDefended);      // TowersDefended
                buff << uint32(((BattlegroundAVScore*)itr2->second)->MinesCaptured);       // MinesCaptured
                break;
            case BATTLEGROUND_WS:
                data->WriteBits(2, 22);
                buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagCaptures);        // flag captures
                buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagReturns);         // flag returns
                break;
            case BATTLEGROUND_AB:
                data->WriteBits(2, 22);
                buff << uint32(((BattlegroundABScore*)itr2->second)->BasesAssaulted);      // bases asssulted
                buff << uint32(((BattlegroundABScore*)itr2->second)->BasesDefended);       // bases defended
                break;
            case BATTLEGROUND_EY:
                data->WriteBits(1, 22);
                buff << uint32(((BattlegroundEYScore*)itr2->second)->FlagCaptures);        // flag captures
                break;
            case BATTLEGROUND_SA:
                data->WriteBits(2, 22);
                buff << uint32(((BattlegroundSAScore*)itr2->second)->demolishers_destroyed);
                buff << uint32(((BattlegroundSAScore*)itr2->second)->gates_destroyed);
                break;
            case BATTLEGROUND_IC:
                data->WriteBits(2, 22);
                buff << uint32(((BattlegroundICScore*)itr2->second)->BasesAssaulted);       // bases asssulted
                buff << uint32(((BattlegroundICScore*)itr2->second)->BasesDefended);        // bases defended
                break;
            case BATTLEGROUND_TP:
                data->WriteBits(2, 22);
                buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagCaptures);         // flag captures
                buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagReturns);          // flag returns
                break;
            case BATTLEGROUND_BFG:
                data->WriteBits(2, 22);
                buff << uint32(((BattlegroundBFGScore*)itr2->second)->BasesAssaulted);      // bases asssulted
                buff << uint32(((BattlegroundBFGScore*)itr2->second)->BasesDefended);       // bases defended
                break;
            case BATTLEGROUND_KT:
                data->WriteBits(2, 22);
                buff << uint32(((BattleGroundKTScore*)itr2->second)->OrbHandles);
                buff << uint32(((BattleGroundKTScore*)itr2->second)->Score * 10);
                break;
            case BATTLEGROUND_SSM:
                data->WriteBits(1, 22);
                buff << uint32(((BattleGroundSSMScore*)itr2->second)->CartsTaken);
                break;
            case BATTLEGROUND_DG:
                data->WriteBits(4, 22);
                buff << uint32(((BattlegroundDGScore*)itr2->second)->cartsCaptured);
                buff << uint32(((BattlegroundDGScore*)itr2->second)->cartsDefended);
                buff << uint32(((BattlegroundDGScore*)itr2->second)->pointsCaptured);
                buff << uint32(((BattlegroundDGScore*)itr2->second)->pointsDefended);
                break;
            case BATTLEGROUND_NA:
            case BATTLEGROUND_TV:
            case BATTLEGROUND_TTP:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:
            case BATTLEGROUND_DS:                                   // wotlk
            case BATTLEGROUND_RV:                                   // wotlk
                data->WriteBits(0, 22);
                break;
            default:
                data->WriteBits(0, 22);
                break;
        }

        data->WriteGuidMask<2, 4, 6, 7>(guid);
        data->WriteBit(isArena);
        data->WriteGuidMask<5>(guid);
        data->WriteBit(isRated);                                    // Has Plr rating
        data->WriteBit(team == ALLIANCE);                           // Reversed team
        data->WriteBit(isRated);                                    // Pre-match mmr
        data->WriteBit(isRated);                                    // rating changed
        data->WriteGuidMask<3, 1>(guid);
        data->WriteBit(isRated);                                    // Has MMR Change
        data->WriteBit(!isArena);                                   // Unk 3 -- Prolly if (bg)
        data->WriteGuidMask<0>(guid);

        //byte part
        buff << uint32(itr2->second->KillingBlows);
        buff.WriteGuidBytes<6, 4>(guid);
        if (isRated)
        {
            buff << int32(bracket->getRatingLastChange());
            buff << uint32(bracket->getRating());
        }
        buff << uint32(itr2->second->DamageDone);                   // damage done
        buff.WriteGuidBytes<5, 0, 1>(guid);
        buff << uint32(itr2->second->HealingDone);                  // healing done

        if (isRated)
            buff << int32(bracket->getLastMMRChange());
         
        if (team == ALLIANCE)
            ++counta2;
        else
            ++counth2;

        ++count;
        // hardcap for client pvp log
        if (count >= 80)
            break;
        //sLog->outError(LOG_FILTER_BATTLEGROUND, "Battleground::PVP_LOG mmr: %i, last mmr change: %i, rating: %i,  last rating change: %i", bracket->getMMV(), bracket->getLastMMRChange(), bracket->getRating(), bracket->getRatingLastChange());
    }

    data->WriteBit(false);                                          // not used. old isRated
    data->WriteBit(bg->GetStatus() == STATUS_WAIT_LEAVE);           // If Ended
    data->WriteBit(false);                                          // not used. old isArena

    data->FlushBits();

    data->PutBits<int32>(count_pos, count, 19);

    data->append(buff);
    
    *data << uint8(counth2);
    *data << uint8(counta2);

    if (bg->GetStatus() == STATUS_WAIT_LEAVE)
        *data << uint8(bg->GetWinner());                               // who win
}

void BattlegroundMgr::BuildStatusFailedPacket(WorldPacket* data, Battleground* bg, Player* player, uint8 QueueSlot, GroupJoinBattlegroundResult result)
{
    ObjectGuid guidBytes1 = player->GetGUID(); // player who caused the error
    ObjectGuid guidBytes2 = bg->GetGUID();
    ObjectGuid unkGuid3 = 0;

    //! 5.4.1
    data->Initialize(SMSG_BATTLEFIELD_STATUS_FAILED);

    *data << uint32(QueueSlot);                                                              // Queue slot
    *data << uint32((bg->isArena() || bg->IsRBG()) ? bg->GetJoinType() : 1);                 // Unk, always 1 
    *data << uint32(player->GetBattlegroundQueueJoinTime(bg->GetTypeID()));                  // Join Time RANDOM
    *data << uint32(result);

    data->WriteGuidMask<0>(guidBytes1);
    data->WriteGuidMask<4>(guidBytes2);
    data->WriteGuidMask<4>(guidBytes1);
    data->WriteGuidMask<7>(unkGuid3);
    data->WriteGuidMask<1>(guidBytes2);
    data->WriteGuidMask<1>(guidBytes1);
    data->WriteGuidMask<2>(unkGuid3);
    data->WriteGuidMask<2>(guidBytes1);
    data->WriteGuidMask<3>(guidBytes2);
    data->WriteGuidMask<4, 3>(unkGuid3);
    data->WriteGuidMask<5>(guidBytes2);
    data->WriteGuidMask<6>(guidBytes1);
    data->WriteGuidMask<6>(unkGuid3);
    data->WriteGuidMask<0, 6, 7>(guidBytes2);
    data->WriteGuidMask<5, 0>(unkGuid3);
    data->WriteGuidMask<2>(guidBytes2);
    data->WriteGuidMask<7, 3, 5>(guidBytes1);
    data->WriteGuidMask<1>(unkGuid3);
            
    data->FlushBits();
            
    data->WriteGuidBytes<6>(unkGuid3);
    data->WriteGuidBytes<5>(guidBytes1);
    data->WriteGuidBytes<6>(guidBytes2);
    data->WriteGuidBytes<4>(guidBytes1);
    data->WriteGuidBytes<3>(guidBytes2);
    data->WriteGuidBytes<1>(guidBytes1);
    data->WriteGuidBytes<3, 1>(unkGuid3);
    data->WriteGuidBytes<0>(guidBytes2);
    data->WriteGuidBytes<6>(guidBytes1);
    data->WriteGuidBytes<4, 5>(guidBytes2);
    data->WriteGuidBytes<2, 7, 0>(unkGuid3);
    data->WriteGuidBytes<7>(guidBytes2);
    data->WriteGuidBytes<5, 4>(unkGuid3);
    data->WriteGuidBytes<1>(guidBytes2);
    data->WriteGuidBytes<3, 0, 2, 7>(guidBytes1);
    data->WriteGuidBytes<2>(guidBytes2);
}

void BattlegroundMgr::BuildUpdateWorldStatePacket(WorldPacket* data, uint32 field, uint32 value)
{
    data->Initialize(SMSG_UPDATE_WORLD_STATE, 9);
    *data << uint8(0);   //zero bit
    *data << uint32(field);
    *data << uint32(value);
}

//! 5.4.1
void BattlegroundMgr::BuildPlaySoundPacket(WorldPacket* data, uint32 soundid)
{
    ObjectGuid guid = 0;

    data->Initialize(SMSG_PLAY_SOUND, 10);
    data->WriteGuidMask<0, 2, 4, 7, 6, 5, 1, 3>(guid);
    data->WriteGuidBytes<3, 4, 2, 6, 1, 5, 0>(guid);
    *data << uint32(soundid);
    data->WriteGuidBytes<7>(guid);
}

//! 5.4.1
void BattlegroundMgr::BuildPlayerLeftBattlegroundPacket(WorldPacket* data, uint64 guid)
{
    ObjectGuid guidBytes = guid;

    data->Initialize(SMSG_BATTLEGROUND_PLAYER_LEFT, 8 + 1);
    data->WriteGuidMask<7, 6, 2, 5, 0, 3, 1, 4>(guidBytes);
    data->WriteGuidBytes<5, 1, 7, 6, 3, 2, 0, 4>(guidBytes);
}

//! 5.4.1
void BattlegroundMgr::BuildPlayerJoinedBattlegroundPacket(WorldPacket* data, uint64 guid)
{
    ObjectGuid guidBytes = guid;

    data->Initialize(SMSG_BATTLEGROUND_PLAYER_JOINED, 8);
    data->WriteGuidMask<5, 1, 7, 6, 3, 0, 2, 4>(guidBytes);
    data->WriteGuidBytes<4, 6, 2, 7, 0, 3, 1, 5>(guidBytes);
}

Battleground* BattlegroundMgr::GetBattlegroundThroughClientInstance(uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    //cause at HandleBattlegroundJoinOpcode the clients sends the instanceid he gets from
    //SMSG_BATTLEFIELD_LIST we need to find the battleground with this clientinstance-id
    Battleground* bg = GetBattlegroundTemplate(bgTypeId);
    if (!bg)
        return NULL;

    if (bg->isArena())
        return GetBattleground(instanceId, bgTypeId);

    for (BattlegroundSet::iterator itr = m_Battlegrounds[bgTypeId].begin(); itr != m_Battlegrounds[bgTypeId].end(); ++itr)
    {
        if (itr->second->GetClientInstanceID() == instanceId)
            return itr->second;
    }
    return NULL;
}

Battleground* BattlegroundMgr::GetBattleground(uint32 InstanceID, BattlegroundTypeId bgTypeId)
{
    if (!InstanceID)
        return NULL;
    //search if needed
    BattlegroundSet::iterator itr;
    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
    {
        for (uint32 i = BATTLEGROUND_AV; i < MAX_BATTLEGROUND_TYPE_ID; i++)
        {
            itr = m_Battlegrounds[i].find(InstanceID);
            if (itr != m_Battlegrounds[i].end())
                return itr->second;
        }
        return NULL;
    }
    itr = m_Battlegrounds[bgTypeId].find(InstanceID);
    return ((itr != m_Battlegrounds[bgTypeId].end()) ? itr->second : NULL);
}

Battleground* BattlegroundMgr::GetBattlegroundTemplate(BattlegroundTypeId bgTypeId)
{
    //map is sorted and we can be sure that lowest instance id has only BG template
    return m_Battlegrounds[bgTypeId].empty() ? NULL : m_Battlegrounds[bgTypeId].begin()->second;
}

uint32 BattlegroundMgr::CreateClientVisibleInstanceId(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id)
{
    if (IsArenaType(bgTypeId))
        return 0;                                           //arenas don't have client-instanceids

    // we create here an instanceid, which is just for
    // displaying this to the client and without any other use..
    // the client-instanceIds are unique for each battleground-type
    // the instance-id just needs to be as low as possible, beginning with 1
    // the following works, because std::set is default ordered with "<"
    // the optimalization would be to use as bitmask std::vector<uint32> - but that would only make code unreadable
    uint32 lastId = 0;
    for (std::set<uint32>::iterator itr = m_ClientBattlegroundIds[bgTypeId][bracket_id].begin(); itr != m_ClientBattlegroundIds[bgTypeId][bracket_id].end();)
    {
        if ((++lastId) != *itr)                             //if there is a gap between the ids, we will break..
            break;
        lastId = *itr;
    }
    m_ClientBattlegroundIds[bgTypeId][bracket_id].insert(lastId + 1);
    return lastId + 1;
}

// create a new battleground that will really be used to play
Battleground* BattlegroundMgr::CreateNewBattleground(BattlegroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry, uint8 joinType, bool isRated, BattlegroundTypeId generatedType/*=bgTypeId*/)
{
    bool isRandom = false;

    if (bgTypeId == BATTLEGROUND_RB)
    {
        isRandom = true;
        bgTypeId = generatedType;
    }

    // get the template BG
    Battleground* bg_template = GetBattlegroundTemplate(bgTypeId);
    BattlegroundSelectionWeightMap* selectionWeights = NULL;
    BattlegroundTypeId oldbgTypeId = bgTypeId;

    if (!bg_template)
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "Battleground: CreateNewBattleground - bg template not found for %u", bgTypeId);
        return NULL;
    }

    isRandom = bg_template->isArena() || bgTypeId == BATTLEGROUND_RATED_10_VS_10;

    // get templet for generated rbg type
    if (isRandom)
    {
        //ASSERT(generatedType != BATTLEGROUND_TYPE_NONE &&   //cyberbrest:don't comment, if where is error no generation come, or system has fatal error
        //    generatedType != BATTLEGROUND_RB &&
        //    generatedType != BATTLEGROUND_RATED_10_VS_10);
        if (generatedType == BATTLEGROUND_TYPE_NONE || generatedType == BATTLEGROUND_RB || generatedType == BATTLEGROUND_RATED_10_VS_10)
        {
            sLog->outU(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> WTF %u-%u", generatedType, bgTypeId);
            return NULL;
        }
        bgTypeId = generatedType;
        bg_template = GetBattlegroundTemplate(bgTypeId);
        if (!bg_template)
        {
            sLog->outError(LOG_FILTER_BATTLEGROUND, "Battleground: CreateNewBattleground - bg generated template not found for %u", bgTypeId);
            return NULL;
        }
    }
    
    // for now not all bg`s works due to factions
    if (oldbgTypeId == BATTLEGROUND_RATED_10_VS_10)
    {
        uint8 randbg = urand(1,3);
        switch (randbg)
        {
            case 1:
                bgTypeId = BATTLEGROUND_AB;
                break;
            case 2:
                bgTypeId = BATTLEGROUND_DG;
                break;
            case 3:
                bgTypeId = BATTLEGROUND_WS;
                break;
        }

        bg_template = GetBattlegroundTemplate(bgTypeId);
    }
    if ((oldbgTypeId == ARENA_TYPE_5v5 || joinType == 5) && sWorld->getBoolConfig(CONFIG_CUSTOM_FOOTBALL))
    {
       bgTypeId = BATTLEGROUND_DS;
       bg_template = GetBattlegroundTemplate(bgTypeId);
    }

    Battleground* bg = NULL;
    // create a copy of the BG template
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV:
            bg = new BattlegroundAV(*(BattlegroundAV*)bg_template);
            break;
        case BATTLEGROUND_WS:
            bg = new BattlegroundWS(*(BattlegroundWS*)bg_template);
            break;
        case BATTLEGROUND_AB:
            bg = new BattlegroundAB(*(BattlegroundAB*)bg_template);
            break;
        case BATTLEGROUND_NA:
            bg = new BattlegroundNA(*(BattlegroundNA*)bg_template);
            break;
        case BATTLEGROUND_TTP:
            bg = new BattlegroundTTP(*(BattlegroundTTP*)bg_template);
            break;
        case BATTLEGROUND_TV:
            bg = new BattlegroundTV(*(BattlegroundTV*)bg_template);
            break;
        case BATTLEGROUND_BE:
            bg = new BattlegroundBE(*(BattlegroundBE*)bg_template);
            break;
        case BATTLEGROUND_AA:
            bg = new BattlegroundAA(*(BattlegroundAA*)bg_template);
            break;
        case BATTLEGROUND_EY:
            bg = new BattlegroundEY(*(BattlegroundEY*)bg_template);
            break;
        case BATTLEGROUND_RL:
            bg = new BattlegroundRL(*(BattlegroundRL*)bg_template);
            break;
        case BATTLEGROUND_SA:
            bg = new BattlegroundSA(*(BattlegroundSA*)bg_template);
            break;
        case BATTLEGROUND_DS:
            bg = new BattlegroundDS(*(BattlegroundDS*)bg_template);
            break;
        case BATTLEGROUND_RV:
            bg = new BattlegroundRV(*(BattlegroundRV*)bg_template);
            break;
        case BATTLEGROUND_IC:
            bg = new BattlegroundIC(*(BattlegroundIC*)bg_template);
            break;
        case BATTLEGROUND_DG:
            bg = new BattlegroundDG(*(BattlegroundDG*)bg_template);
            break;
        case BATTLEGROUND_TP:
            bg = new BattlegroundTP(*(BattlegroundTP*)bg_template);
            break;
        case BATTLEGROUND_BFG:
            bg = new BattlegroundBFG(*(BattlegroundBFG*)bg_template);
            break;
        case BATTLEGROUND_RATED_10_VS_10:
        case BATTLEGROUND_RB:
            //bg = new BattlegroundRB(*(BattlegroundRB*)bg_template);
            // should never happend
            return NULL;
        case BATTLEGROUND_KT:
            bg = new BattlegroundKT(*(BattlegroundKT*)bg_template);
            break;
        case BATTLEGROUND_SSM:
            bg = new BattlegroundSSM(*(BattlegroundSSM*)bg_template);
            break;
        default:
            //error, but it is handled few lines above
            return 0;
    }

    // set battelground difficulty before initialization
    bg->SetBracket(bracketEntry);

    // generate a new instance id
    bg->SetInstanceID(sMapMgr->GenerateInstanceId()); // set instance id
    bg->SetClientInstanceID(CreateClientVisibleInstanceId(isRandom ? BATTLEGROUND_RB : bgTypeId, bracketEntry->GetBracketId()));

    // reset the new bg (set status to status_wait_queue from status_none)
    bg->Reset();

    // start the joining of the bg
    bg->SetStatus(STATUS_WAIT_JOIN);
    bg->SetJoinType(joinType);
    bg->SetRated(isRated);
    bg->SetTypeID(isRandom ? oldbgTypeId : bgTypeId);       //oldbgTypeId can be BATTLEGROUND_RATED_10_VS_10 || BATTLEGROUND_RB
    bg->SetRBG(oldbgTypeId == BATTLEGROUND_RATED_10_VS_10);
    bg->SetRandomTypeID(bgTypeId);
    bg->InitGUID();

    return bg;
}

// used to create the BG templates
uint32 BattlegroundMgr::CreateBattleground(CreateBattlegroundData& data)
{
    // Create the BG
    Battleground* bg = NULL;
    switch (data.bgTypeId)
    {
        case BATTLEGROUND_AV: bg = new BattlegroundAV; break;
        case BATTLEGROUND_WS: bg = new BattlegroundWS; break;
        case BATTLEGROUND_AB: bg = new BattlegroundAB; break;
        case BATTLEGROUND_NA: bg = new BattlegroundNA; break;
        case BATTLEGROUND_TTP: bg = new BattlegroundTTP; break;
        case BATTLEGROUND_TV: bg = new BattlegroundTV; break;
        case BATTLEGROUND_BE: bg = new BattlegroundBE; break;
        case BATTLEGROUND_AA: bg = new BattlegroundAA; break;
        case BATTLEGROUND_EY: bg = new BattlegroundEY; break;
        case BATTLEGROUND_RL: bg = new BattlegroundRL; break;
        case BATTLEGROUND_SA: bg = new BattlegroundSA; break;
        case BATTLEGROUND_DS: bg = new BattlegroundDS; break;
        case BATTLEGROUND_RV: bg = new BattlegroundRV; break;
        case BATTLEGROUND_IC: bg = new BattlegroundIC; break;
        case BATTLEGROUND_TP: bg = new BattlegroundTP; break;
        case BATTLEGROUND_BFG: bg = new BattlegroundBFG; break;
        case BATTLEGROUND_RB: bg = new BattlegroundRB; break;
        case BATTLEGROUND_KT: bg = new BattlegroundKT; break;
        case BATTLEGROUND_SSM: bg = new BattlegroundSSM; break;
        case BATTLEGROUND_DG: bg = new BattlegroundDG; break;
        default:
            bg = new Battleground;
            break;
    }

    bg->SetMapId(data.MapID);
    bg->SetTypeID(data.bgTypeId);
    bg->InitGUID();
    bg->SetInstanceID(0);
    bg->SetArenaorBGType(data.IsArena);
    bg->SetRBG(data.IsRbg);
    bg->SetMinPlayersPerTeam(data.MinPlayersPerTeam);
    bg->SetMaxPlayersPerTeam(data.MaxPlayersPerTeam);
    bg->SetMinPlayers(data.MinPlayersPerTeam* 2);
    bg->SetMaxPlayers(data.MaxPlayersPerTeam* 2);
    bg->SetName(data.BattlegroundName);
    bg->SetTeamStartLoc(ALLIANCE, data.Team1StartLocX, data.Team1StartLocY, data.Team1StartLocZ, data.Team1StartLocO);
    bg->SetTeamStartLoc(HORDE,    data.Team2StartLocX, data.Team2StartLocY, data.Team2StartLocZ, data.Team2StartLocO);
    bg->SetStartMaxDist(data.StartMaxDist);
    bg->SetLevelRange(data.LevelMin, data.LevelMax);
    bg->SetHolidayId(data.holiday);
    bg->SetScriptId(data.scriptId);

    // add bg to update list
    AddBattleground(bg->GetInstanceID(), bg->GetTypeID(), bg);

    // return some not-null value, bgTypeId is good enough for me
    return data.bgTypeId;
}

void BattlegroundMgr::CreateInitialBattlegrounds()
{
    uint32 oldMSTime = getMSTime();

    uint8 selectionWeight;
    BattlemasterListEntry const* bl;

    //                                               0   1                  2                  3       4       5                 6               7              8            9             10      11       12
    QueryResult result = WorldDatabase.Query("SELECT id, MinPlayersPerTeam, MaxPlayersPerTeam, MinLvl, MaxLvl, AllianceStartLoc, AllianceStartO, HordeStartLoc, HordeStartO, StartMaxDist, Weight, holiday, ScriptName FROM battleground_template");

    if (!result)
    {
        sLog->outError(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 battlegrounds. DB table `battleground_template` is empty.");
        return;
    }

    uint32 count = 0, startId;

    do
    {
        Field* fields = result->Fetch();

        uint32 bgTypeID_ = fields[0].GetUInt32();
        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeID_, NULL))
            continue;

        // can be overwrite by values from DB
        bl = sBattlemasterListStore.LookupEntry(bgTypeID_);
        if (!bl)
        {
            sLog->outError(LOG_FILTER_BATTLEGROUND, "Battleground ID %u not found in BattlemasterList.dbc. Battleground not created.", bgTypeID_);
            continue;
        }

        CreateBattlegroundData data;
        data.bgTypeId = BattlegroundTypeId(bgTypeID_);
        data.IsArena = (bl->type == TYPE_ARENA);
        data.IsRbg  = (bl->ReatedData == 2);
        data.MinPlayersPerTeam = fields[1].GetUInt16();
        data.MaxPlayersPerTeam = fields[2].GetUInt16();
        data.LevelMin = fields[3].GetUInt8();
        data.LevelMax = fields[4].GetUInt8();

        // check values from DB
        if (data.MaxPlayersPerTeam == 0 || data.MinPlayersPerTeam > data.MaxPlayersPerTeam)
        {
            sLog->outError(LOG_FILTER_SQL, "Table `battleground_template` for id %u has bad values for MinPlayersPerTeam (%u) and MaxPlayersPerTeam(%u)",
                data.bgTypeId, data.MinPlayersPerTeam, data.MaxPlayersPerTeam);
            continue;
        }

        if (data.LevelMin == 0 || data.LevelMax == 0 || data.LevelMin > data.LevelMax)
        {
            sLog->outError(LOG_FILTER_SQL, "Table `battleground_template` for id %u has bad values for LevelMin (%u) and LevelMax(%u)",
                data.bgTypeId, data.LevelMin, data.LevelMax);
            continue;
        }

        startId = fields[5].GetUInt32();
        if (WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(startId))
        {
            data.Team1StartLocX = start->x;
            data.Team1StartLocY = start->y;
            data.Team1StartLocZ = start->z;
            data.Team1StartLocO = fields[6].GetFloat();
        }
        else if (data.bgTypeId == BATTLEGROUND_AA || data.bgTypeId == BATTLEGROUND_RB || data.bgTypeId == BATTLEGROUND_RATED_10_VS_10)
        {
            data.Team1StartLocX = 0;
            data.Team1StartLocY = 0;
            data.Team1StartLocZ = 0;
            data.Team1StartLocO = fields[6].GetFloat();
        }
        else
        {
            sLog->outError(LOG_FILTER_SQL, "Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `AllianceStartLoc`. BG not created.", data.bgTypeId, startId);
            continue;
        }

        startId = fields[7].GetUInt32();
        if (WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(startId))
        {
            data.Team2StartLocX = start->x;
            data.Team2StartLocY = start->y;
            data.Team2StartLocZ = start->z;
            data.Team2StartLocO = fields[8].GetFloat();
        }
        else if (data.bgTypeId == BATTLEGROUND_AA || data.bgTypeId == BATTLEGROUND_RB || data.bgTypeId == BATTLEGROUND_RATED_10_VS_10)
        {
            data.Team2StartLocX = 0;
            data.Team2StartLocY = 0;
            data.Team2StartLocZ = 0;
            data.Team2StartLocO = fields[8].GetFloat();
        }
        else
        {
            sLog->outError(LOG_FILTER_SQL, "Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `HordeStartLoc`. BG not created.", data.bgTypeId, startId);
            continue;
        }

        data.StartMaxDist = fields[9].GetFloat();

        selectionWeight = fields[10].GetUInt8();
        data.holiday = fields[11].GetUInt32();
        data.scriptId = sObjectMgr->GetScriptId(fields[12].GetCString());

        data.BattlegroundName = bl->name;
        data.MapID = bl->mapid[0];

        if (!CreateBattleground(data))
            continue;

        if (data.IsArena)
        {
            if (data.bgTypeId != BATTLEGROUND_AA)
                m_ArenaSelectionWeights[data.bgTypeId] = selectionWeight;
        }
        else if (bl->mapid[1] <= 0)   // map 1-15 is random generation list.
            m_BGSelectionWeights[data.bgTypeId] = selectionWeight;
        ++count;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u battlegrounds in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void BattlegroundMgr::BuildBattlegroundListPacket(WorldPacket* data, ObjectGuid guid, Player* player, BattlegroundTypeId bgTypeId)
{
    if (!player)
        return;

    uint32 winner_conquest = (player->GetRandomWinner() ? BG_REWARD_WINNER_CONQUEST_LAST : BG_REWARD_WINNER_CONQUEST_FIRST) / 100;
    uint32 winner_honor = (player->GetRandomWinner() ? BG_REWARD_WINNER_HONOR_LAST : BG_REWARD_WINNER_HONOR_FIRST) / 100;
    uint32 loser_honor = (player->GetRandomWinner() ? BG_REWARD_LOSER_HONOR_LAST : BG_REWARD_LOSER_HONOR_FIRST) / 100;

    ByteBuffer dataBuffer;

    data->Initialize(SMSG_BATTLEFIELD_LIST, 83);
    *data << uint32(winner_honor);              // holiday
    *data << uint32(winner_honor);              // random
    *data << uint8(0);                          // min level
    *data << uint8(0);                          // max level
    *data << uint32(bgTypeId);                  // battleground id
    *data << uint32(winner_conquest);           // holiday
    *data << uint32(loser_honor);               // random
    *data << uint32(winner_conquest);           // random
    *data << uint32(loser_honor);               // holiday

    data->WriteGuidMask<1>(guid);
    data->WriteBit(!guid);                      // from where
    data->WriteGuidMask<4>(guid);

    if (bgTypeId == BATTLEGROUND_AA)            // arena
        data->WriteBits(0, 22);                 // instance count
    else                                        // battleground
    {
        uint32 count = 0;
        if (Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId))
        {
            // expected bracket entry
            if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), player->getLevel()))
            {
                BattlegroundBracketId bracketId = bracketEntry->GetBracketId();
                for (std::set<uint32>::iterator itr = m_ClientBattlegroundIds[bgTypeId][bracketId].begin(); itr != m_ClientBattlegroundIds[bgTypeId][bracketId].end();++itr)
                {
                    dataBuffer << uint32(*itr);
                    ++count;
                }
            }
        }
        data->WriteBits(count, 22);
    }

    data->WriteGuidMask<0, 6>(guid);
    data->WriteBit(player->GetRandomWinner());  // holiday
    data->WriteGuidMask<5>(guid);
    data->WriteBit(1);                          // unk
    data->WriteGuidMask<2, 3, 7>(guid);
    data->WriteBit(player->GetRandomWinner());  // random
    data->FlushBits();

    data->WriteGuidBytes<2, 5, 7>(guid);
    if (!dataBuffer.empty())
        data->append(dataBuffer);
    data->WriteGuidBytes<1, 3, 6, 4, 0>(guid);
}

void BattlegroundMgr::SendToBattleground(Player* player, uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    Battleground* bg = GetBattleground(instanceId, bgTypeId);
    if (bg)
    {
        uint32 mapid = bg->GetMapId();
        float x, y, z, O;
        uint32 team = player->GetBGTeam();
        if (team == 0)
            team = player->GetTeam();
        bg->GetTeamStartLoc(team, x, y, z, O);

        sLog->outInfo(LOG_FILTER_BATTLEGROUND, "BATTLEGROUND: Sending %s to map %u, X %f, Y %f, Z %f, O %f", player->GetName(), mapid, x, y, z, O);
        if (bg->GetJoinType() == 5 && sWorld->getBoolConfig(CONFIG_CUSTOM_FOOTBALL))
            if (team == ALLIANCE)
               player->TeleportTo(mapid, 941.9f, 244.74f, 0.0f, 0.0f);  //ALLIANCE //gold 
            else 
               player->TeleportTo(mapid, 1022.0f, 247.21f, 0.0f, 3.1f);  //HORDE //green
        else
         player->TeleportTo(mapid, x, y, z, O);
    }
    else
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "player %u is trying to port to non-existent bg instance %u", player->GetGUIDLow(), instanceId);
    }
}

void BattlegroundMgr::SendAreaSpiritHealerQueryOpcode(Player* player, Battleground* bg, uint64 guid)
{
    WorldPacket data(SMSG_AREA_SPIRIT_HEALER_TIME, 12);
    uint32 time_ = 30000 - bg->GetLastResurrectTime();      // resurrect every 30 seconds
    if (time_ == uint32(-1))
        time_ = 0;
    data.WriteGuidMask<6, 5, 4, 2, 7, 0, 3, 1>(guid);
    data.WriteGuidBytes<4, 5, 7, 3, 1>(guid);
    data << uint32(time_);
    data.WriteGuidBytes<0, 2, 6>(guid);
    player->GetSession()->SendPacket(&data);
}

bool BattlegroundMgr::IsArenaType(BattlegroundTypeId bgTypeId)
{
    return (bgTypeId == BATTLEGROUND_AA ||
        bgTypeId == BATTLEGROUND_BE ||
        bgTypeId == BATTLEGROUND_NA ||
        bgTypeId == BATTLEGROUND_DS ||
        bgTypeId == BATTLEGROUND_RV ||
        bgTypeId == BATTLEGROUND_RL ||
        bgTypeId == BATTLEGROUND_TTP ||
        bgTypeId == BATTLEGROUND_TV);
}

BracketType BattlegroundMgr::BracketByJoinType(uint8 joinType)
{
    switch (joinType)
    {
        case ARENA_TYPE_2v2:
            return BRACKET_TYPE_ARENA_2;
        case ARENA_TYPE_3v3:
            return BRACKET_TYPE_ARENA_3;
        case ARENA_TYPE_5v5:
            return BRACKET_TYPE_ARENA_5;
        case JOIN_TYPE_RATED_BG_10v10:
        case JOIN_TYPE_RATED_BG_15v15:
        case JOIN_TYPE_RATED_BG_25v25:
            return BRACKET_TYPE_RATED_BG;
        default:
            break;
    }

    return BRACKET_TYPE_MAX;
}

uint8 BattlegroundMgr::GetJoinTypeByBracketSlot(uint8 slot)
{
    switch (slot)
    {
    case BRACKET_TYPE_ARENA_2:
        return ARENA_TYPE_2v2;
    case BRACKET_TYPE_ARENA_3:
        return ARENA_TYPE_3v3;
    case BRACKET_TYPE_ARENA_5:
        return ARENA_TYPE_5v5;
    case BRACKET_TYPE_RATED_BG:
        return JOIN_TYPE_RATED_BG_10v10;
    default:
        break;
    }
    sLog->outError(LOG_FILTER_ARENAS, "FATAL: Unknown arena slot %u", slot);
    return 0xFF;
}

BattlegroundQueueTypeId BattlegroundMgr::BGQueueTypeId(BattlegroundTypeId bgTypeId, uint8 arenaType)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_WS:
            return BATTLEGROUND_QUEUE_WS;
        case BATTLEGROUND_RATED_10_VS_10:
            return BATTLEGROUND_QUEUE_RBG;
        case BATTLEGROUND_AB:
            return BATTLEGROUND_QUEUE_AB;
        case BATTLEGROUND_AV:
            return BATTLEGROUND_QUEUE_AV;
        case BATTLEGROUND_EY:
            return BATTLEGROUND_QUEUE_EY;
        case BATTLEGROUND_SA:
            return BATTLEGROUND_QUEUE_SA;
        case BATTLEGROUND_IC:
            return BATTLEGROUND_QUEUE_IC;
        case BATTLEGROUND_TP:
            return BATTLEGROUND_QUEUE_TP;
        case BATTLEGROUND_BFG:
            return BATTLEGROUND_QUEUE_BFG;
        case BATTLEGROUND_RB:
            return BATTLEGROUND_QUEUE_RB;
        case BATTLEGROUND_KT:
            return BATTLEGROUND_QUEUE_KT;
        case BATTLEGROUND_CTF3:
            return BATTLEGROUND_QUEUE_CTF3;
        case BATTLEGROUND_SSM:
            return BATTLEGROUND_QUEUE_SSM;
        case BATTLEGROUND_DG:
            return BATTLEGROUND_QUEUE_DG;
        case BATTLEGROUND_AA:
        case BATTLEGROUND_NA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
        case BATTLEGROUND_TTP:
        case BATTLEGROUND_TV:
            switch (arenaType)
            {
                case ARENA_TYPE_2v2:
                    return BATTLEGROUND_QUEUE_2v2;
                case ARENA_TYPE_3v3:
                    return BATTLEGROUND_QUEUE_3v3;
                case ARENA_TYPE_5v5:
                    return BATTLEGROUND_QUEUE_5v5;
                default:
                    return BATTLEGROUND_QUEUE_NONE;
            }
        default:
            return BATTLEGROUND_QUEUE_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::BGTemplateId(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_WS:
            return BATTLEGROUND_WS;
        case BATTLEGROUND_QUEUE_AB:
            return BATTLEGROUND_AB;
        case BATTLEGROUND_QUEUE_AV:
            return BATTLEGROUND_AV;
        case BATTLEGROUND_QUEUE_EY:
            return BATTLEGROUND_EY;
        case BATTLEGROUND_QUEUE_SA:
            return BATTLEGROUND_SA;
        case BATTLEGROUND_QUEUE_IC:
            return BATTLEGROUND_IC;
        case BATTLEGROUND_QUEUE_TP:
            return BATTLEGROUND_TP;
        case BATTLEGROUND_QUEUE_BFG:
            return BATTLEGROUND_BFG;
        case BATTLEGROUND_QUEUE_RB:
            return BATTLEGROUND_RB;
        case BATTLEGROUND_QUEUE_KT:
            return BATTLEGROUND_KT;
        case BATTLEGROUND_QUEUE_CTF3:
            return BATTLEGROUND_CTF3;
        case BATTLEGROUND_QUEUE_SSM:
            return BATTLEGROUND_SSM;
        case BATTLEGROUND_QUEUE_TV:
            return BATTLEGROUND_TV;
        case BATTLEGROUND_QUEUE_DG:
            return BATTLEGROUND_DG;
        case BATTLEGROUND_QUEUE_2v2:
        case BATTLEGROUND_QUEUE_3v3:
        case BATTLEGROUND_QUEUE_5v5:
            return BATTLEGROUND_AA;
        case BATTLEGROUND_QUEUE_RBG:
            return BATTLEGROUND_RATED_10_VS_10;
        default:
            return BattlegroundTypeId(0);                   // used for unknown template (it existed and do nothing)
    }
}

uint8 BattlegroundMgr::BGJoinType(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_2v2:
            return ARENA_TYPE_2v2;
        case BATTLEGROUND_QUEUE_3v3:
            return ARENA_TYPE_3v3;
        case BATTLEGROUND_QUEUE_5v5:
            return ARENA_TYPE_5v5;
        case BATTLEGROUND_QUEUE_RBG:
            return JOIN_TYPE_RATED_BG_10v10;
        default:
            return 0;
    }
}

void BattlegroundMgr::ToggleTesting()
{
    m_Testing = !m_Testing;
    if (m_Testing)
        sWorld->SendWorldText(LANG_DEBUG_BG_ON);
    else
        sWorld->SendWorldText(LANG_DEBUG_BG_OFF);
}

void BattlegroundMgr::ToggleArenaTesting()
{
    m_ArenaTesting = !m_ArenaTesting;
    if (m_ArenaTesting)
        sWorld->SendWorldText(LANG_DEBUG_ARENA_ON);
    else
        sWorld->SendWorldText(LANG_DEBUG_ARENA_OFF);
}

void BattlegroundMgr::SetHolidayWeekends(std::list<uint32> activeHolidayId)
{
    for (uint32 bgtype = 1; bgtype < MAX_BATTLEGROUND_TYPE_ID; ++bgtype)
    {
        if (Battleground* bg = GetBattlegroundTemplate(BattlegroundTypeId(bgtype)))
        {
            bool holidayActivate = false;

            if (uint32 holidayId = bg->GetHolidayId())
                for (std::list<uint32>::iterator apptItr = activeHolidayId.begin(); apptItr != activeHolidayId.end(); ++apptItr)
                    if (holidayId == (*apptItr))
                        holidayActivate = true;

            bg->SetHoliday(holidayActivate);
        }
    }
}

void BattlegroundMgr::FillHolidayWorldStates(WorldPacket &data)
{
    if (!holidayWS)
        return;

    FillInitialWorldState(data, holidayWS, 1);
}

void BattlegroundMgr::ScheduleQueueUpdate(uint32 arenaMatchmakerRating, uint8 arenaType, BattlegroundQueueTypeId bgQueueTypeId, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id)
{
    //This method must be atomic, TODO add mutex
    //we will use only 1 number created of bgTypeId and bracket_id
    QueueSchedulerItem* schedule_id = new QueueSchedulerItem(arenaMatchmakerRating, arenaType, bgQueueTypeId, bgTypeId, bracket_id);
    bool found = false;
    for (uint8 i = 0; i < m_QueueUpdateScheduler.size(); i++)
    {
        if (m_QueueUpdateScheduler[i]->_MMRating == arenaMatchmakerRating
            && m_QueueUpdateScheduler[i]->_joinType == arenaType
            && m_QueueUpdateScheduler[i]->_bgQueueTypeId == bgQueueTypeId
            && m_QueueUpdateScheduler[i]->_bgTypeId == bgTypeId
            && m_QueueUpdateScheduler[i]->_bracket_id == bracket_id)
        {
            found = true;
            break;
        }
    }
    if (!found)
        m_QueueUpdateScheduler.push_back(schedule_id);
    else
        delete schedule_id;
}

uint32 BattlegroundMgr::GetMaxRatingDifference() const
{
    // this is for stupid people who can't use brain and set max rating difference to 0
    uint32 diff = sWorld->getIntConfig(CONFIG_ARENA_MAX_RATING_DIFFERENCE);
    if (diff == 0)
        diff = 5000;
    return diff;
}

uint32 BattlegroundMgr::GetRatingDiscardTimer() const
{
    return sWorld->getIntConfig(CONFIG_ARENA_RATING_DISCARD_TIMER);
}

uint32 BattlegroundMgr::GetPrematureFinishTime() const
{
    return sWorld->getIntConfig(CONFIG_BATTLEGROUND_PREMATURE_FINISH_TIMER);
}

void BattlegroundMgr::LoadBattleMastersEntry()
{
    uint32 oldMSTime = getMSTime();

    mBattleMastersMap.clear();                                  // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, bg_template FROM battlemaster_entry");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 battlemaster entries. DB table `battlemaster_entry` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint32 bgTypeId  = fields[1].GetUInt32();
        if (!sBattlemasterListStore.LookupEntry(bgTypeId))
        {
            sLog->outError(LOG_FILTER_SQL, "Table `battlemaster_entry` contain entry %u for not existed battleground type %u, ignored.", entry, bgTypeId);
            continue;
        }

        mBattleMastersMap[entry] = BattlegroundTypeId(bgTypeId);
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u battlemaster entries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

HolidayIds BattlegroundMgr::BGTypeToWeekendHolidayId(BattlegroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV: return HOLIDAY_CALL_TO_ARMS_AV;
        case BATTLEGROUND_EY: return HOLIDAY_CALL_TO_ARMS_EY;
        case BATTLEGROUND_WS: return HOLIDAY_CALL_TO_ARMS_WS;
        case BATTLEGROUND_SA: return HOLIDAY_CALL_TO_ARMS_SA;
        case BATTLEGROUND_AB: return HOLIDAY_CALL_TO_ARMS_AB;
        case BATTLEGROUND_IC: return HOLIDAY_CALL_TO_ARMS_IC;
        case BATTLEGROUND_TP: return HOLIDAY_CALL_TO_ARMS_TP;
        case BATTLEGROUND_BFG: return HOLIDAY_CALL_TO_ARMS_BFG;
        default: return HOLIDAY_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::WeekendHolidayIdToBGType(HolidayIds holiday)
{
    switch (holiday)
    {
        case HOLIDAY_CALL_TO_ARMS_AV: return BATTLEGROUND_AV;
        case HOLIDAY_CALL_TO_ARMS_EY: return BATTLEGROUND_EY;
        case HOLIDAY_CALL_TO_ARMS_WS: return BATTLEGROUND_WS;
        case HOLIDAY_CALL_TO_ARMS_SA: return BATTLEGROUND_SA;
        case HOLIDAY_CALL_TO_ARMS_AB: return BATTLEGROUND_AB;
        case HOLIDAY_CALL_TO_ARMS_IC: return BATTLEGROUND_IC;
        case HOLIDAY_CALL_TO_ARMS_TP: return BATTLEGROUND_TP;
        case HOLIDAY_CALL_TO_ARMS_BFG: return BATTLEGROUND_BFG;
        default: return BATTLEGROUND_TYPE_NONE;
    }
}

bool BattlegroundMgr::IsBGWeekend(BattlegroundTypeId bgTypeId)
{
    return IsHolidayActive(BGTypeToWeekendHolidayId(bgTypeId));
}
