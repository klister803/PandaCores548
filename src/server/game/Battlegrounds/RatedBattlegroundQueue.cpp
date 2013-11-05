/*
 * Copyright (C)   2013    uWoW <http://uwow.biz>
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#include "RatedBattlegroundQueue.h"
#include "Player.h"
#include "Group.h"
#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "RatedBattleground.h"

RatedBattlegroundQueue::RatedBattlegroundQueue()
{
    m_queueStore.clear();
    m_playersQueueStore.clear();
}

RatedBattlegroundQueue::~RatedBattlegroundQueue()
{
}

void RatedBattlegroundQueue::Update()
{
    if (m_queueStore.size() < 2)
        return;

    uint16 maxMMVDiff = 500;

    TRINITY_GUARD(ACE_Thread_Mutex, Lock);

    for (QueueStore::iterator itr1 = m_queueStore.begin(); itr1 != m_queueStore.end(); ++itr1)
        for (QueueStore::iterator itr2 = m_queueStore.begin(); itr2 != m_queueStore.end(); ++itr2)
        {
            GroupQueueInfo* qInfo1 = *itr1;
            GroupQueueInfo* qInfo2 = *itr2;

            if (qInfo1 == qInfo2)
                continue;

            if (abs(int16(qInfo1->RbgMMV) - int16(qInfo2->RbgMMV)) < maxMMVDiff)
            {
                Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_RATED_10_VS_10);
                if (!bg_template)
                {
                    sLog->outError(LOG_FILTER_BATTLEGROUND, "RatedBattlegroundQueue Update: bg template not found for %u", BATTLEGROUND_RATED_10_VS_10);
                    continue;
                }

                PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg_template->GetMapId(), 90);
                if (!bracketEntry)
                {
                    sLog->outError(LOG_FILTER_BATTLEGROUND, "RatedBattlegroundQueue Update: bg bracket entry not found for map %u bracket id %u", bg_template->GetMapId(), 90);
                    continue;
                }

                Battleground* rbg = sBattlegroundMgr->CreateNewBattleground(BATTLEGROUND_RATED_10_VS_10, bracketEntry, 0, false);
                if (rbg)
                {
                    if (qInfo1->Team == qInfo2->Team)
                    {
                        rbg->SetSameTeamId(qInfo1->Team);
                    }
                    else
                    {
                        if (qInfo1->Team == HORDE)
                        {
                            GroupQueueInfo* qInfoTmp = qInfo2;
                            qInfo2 = qInfo1;
                            qInfo1 = qInfoTmp;
                        }
                    }

                    m_queueStore.erase(itr1);
                    m_queueStore.erase(itr2);

                    qInfo1->OponentsRbgMMV = qInfo2->RbgMMV;
                    qInfo2->OponentsRbgMMV = qInfo1->RbgMMV;

                    rbg->SetTypeID(BATTLEGROUND_RATED_10_VS_10);
                    rbg->SetRated(true);

                    qInfo1->Team = ALLIANCE;
                    qInfo2->Team = HORDE;

                    rbg->SetArenaMatchmakerRating(ALLIANCE, qInfo1->RbgMMV);
                    rbg->SetArenaMatchmakerRating(   HORDE, qInfo2->RbgMMV);

                    InviteGroup(qInfo1, rbg, ALLIANCE);
                    InviteGroup(qInfo2, rbg, HORDE);

                    rbg->SetRBG(true);
                    rbg->StartBattleground();

                    return;
                }
            }
        }
}

GroupQueueInfo* RatedBattlegroundQueue::AddGroup(Player *leader)
{
    Group* group = leader->GetGroup();
    if (!group)
        return NULL;

    GroupQueueInfo* queueInfo   = new GroupQueueInfo;
    queueInfo->BgTypeId         = BATTLEGROUND_RATED_10_VS_10;
    queueInfo->JoinTime         = getMSTime();
    queueInfo->RemoveInviteTime = 0;
    queueInfo->IsInvitedToBGInstanceGUID = 0;
    queueInfo->Team             = leader->GetTeam();

    queueInfo->Players.clear();

    uint8 onlineMembers = 0;
    uint16 mmv = 0;
    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->getSource();
        if (!member)
            continue;   // this should never happen

        onlineMembers++;
        mmv += member->getRBG()->getMMV();

        PlayerQueueInfo* pl_info = new PlayerQueueInfo;
        pl_info->LastOnlineTime  = queueInfo->JoinTime;
        pl_info->GroupInfo       = queueInfo;
        queueInfo->Players[member->GetGUID()]  = pl_info;

        m_playersQueueStore[member->GetGUID()] = queueInfo;
    }

    if (mmv)
        mmv /= onlineMembers;

    queueInfo->RbgMMV = mmv;

    m_queueStore.insert(queueInfo);

    return queueInfo;
}

void RatedBattlegroundQueue::RemovePlayer(uint64 playerGuid)
{
    GroupQueueInfo* gInfo = GetQueueInfoByPlayer(playerGuid);
    if (!gInfo)
        return;

    if (!gInfo->IsInvitedToBGInstanceGUID)
    {
        TRINITY_GUARD(ACE_Thread_Mutex, Lock);

        for (std::map<uint64, PlayerQueueInfo*>::iterator itr = gInfo->Players.begin(); itr != gInfo->Players.end(); ++itr)
        {
            // get the player
            Player* player = ObjectAccessor::FindPlayer(itr->first);
            // if offline, skip him, this should not happen - player is removed from queue when he logs out
            if (!player)
                continue;

            Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(gInfo->BgTypeId);
            BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(gInfo->BgTypeId, gInfo->ArenaType);
            uint32 queueSlot = player->GetBattlegroundQueueIndex(bgQueueTypeId);

            player->RemoveBattlegroundQueueId(bgQueueTypeId); // must be called this way, because if you move this call to
                                                            // queue->removeplayer, it causes bugs
            WorldPacket data;
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, player, queueSlot, STATUS_NONE, player->GetBattlegroundQueueJoinTime(gInfo->BgTypeId), 0, 0);
            player->GetSession()->SendPacket(&data);

            m_playersQueueStore.erase(player->GetGUID());
        }
        delete gInfo;
    }
    else
    {
        TRINITY_GUARD(ACE_Thread_Mutex, Lock);

        m_playersQueueStore.erase(playerGuid);

        std::map<uint64, PlayerQueueInfo*>::iterator itr = gInfo->Players.find(playerGuid);
        if (itr != gInfo->Players.end())
        {
            delete itr->second;
            gInfo->Players.erase(itr);
        }

        if (gInfo->Players.empty())
            delete gInfo;
    }
}

bool RatedBattlegroundQueue::GetQueueInfoByPlayer(uint64 playerGuid, GroupQueueInfo* result)
{
    PlayersQueueStore::iterator itr = m_playersQueueStore.find(playerGuid);
    if (itr == m_playersQueueStore.end())
        return false;

    *result = *(itr->second);
    return true;
}

GroupQueueInfo* RatedBattlegroundQueue::GetQueueInfoByPlayer(uint64 playerGuid)
{
    PlayersQueueStore::iterator itr = m_playersQueueStore.find(playerGuid);
    if (itr == m_playersQueueStore.end())
        return NULL;

    return itr->second;
}

bool RatedBattlegroundQueue::InviteGroup(GroupQueueInfo *ginfo, Battleground *bg, uint32 side)
{
    if(!ginfo)
        return false;


    // set side if needed
    if (side)
        ginfo->Team = side;

    if (!ginfo->IsInvitedToBGInstanceGUID)
    {
        // not yet invited
        // set invitation
        ginfo->IsInvitedToBGInstanceGUID = bg->GetInstanceID();
        BattlegroundTypeId bgTypeId = bg->GetTypeID();
        BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, bg->GetArenaType());

        ginfo->RemoveInviteTime = getMSTime() + INVITE_ACCEPT_WAIT_TIME;

        if (ginfo->Players.empty())
            return false;

        // loop through the players
        for (std::map<uint64, PlayerQueueInfo*>::iterator itr = ginfo->Players.begin(); itr != ginfo->Players.end(); ++itr)
        {
            if(itr == ginfo->Players.end())
                continue;
            if(!itr->first)
                continue;
            // get the player
            Player* player = ObjectAccessor::FindPlayer(itr->first);
            // if offline, skip him, this should not happen - player is removed from queue when he logs out
            if (!player)
                continue;

            // set invited player counters
            bg->IncreaseInvitedCount(ginfo->Team);

            player->SetInviteForBattlegroundQueueType(bgQueueTypeId, ginfo->IsInvitedToBGInstanceGUID);

            WorldPacket data;

            uint32 queueSlot = player->GetBattlegroundQueueIndex(bgQueueTypeId);

            sLog->outInfo(LOG_FILTER_WORLDSERVER, "invite qieue slot: %u; bgQueueTypeId: %u; bgTypeId: %u", queueSlot, bgQueueTypeId, bgTypeId);

            sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Battleground: invited player %s (%u) to BG instance %u queueindex %u bgtype %u",
                 player->GetName(), player->GetGUIDLow(), bg->GetInstanceID(), queueSlot, bg->GetTypeID());

            // send status packet
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, player, queueSlot, STATUS_WAIT_JOIN, INVITE_ACCEPT_WAIT_TIME, player->GetBattlegroundQueueJoinTime(bgTypeId), ginfo->ArenaType);
            player->GetSession()->SendPacket(&data);
        }
        return true;
    }

    return false;
}
