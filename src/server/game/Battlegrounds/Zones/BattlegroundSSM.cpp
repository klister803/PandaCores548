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

#include "Battleground.h"
#include "BattlegroundSSM.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

BattlegroundSSM::BattlegroundSSM()
{
    BgCreatures.resize(BG_SSM_OBJECT_MAX);
    BgObjects.resize(BG_SSM_OBJECT_MAX);
    
    /// Start Messages Initialization
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_SSM_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_SSM_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_SSM_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_SSM_HAS_BEGUN;

    m_waysStep[0] = 1;
    m_waysStep[1] = 1;
    m_waysStep[2] = 1;

    m_waysMap[0] = Way1;
    m_waysMap[1] = Way2;
    m_waysMap[2] = Way3;

    m_cart[0] = NULL;
    m_cart[1] = NULL;
    m_cart[2] = NULL;

    m_timerPointsUpdate = SSM_SCORE_UPDATE_TIMER;
    m_timerCartsUpdate = SSM_CARTS_UPDATE_TIMER;

    m_TeamScores[BG_TEAM_ALLIANCE] = 0;
    m_TeamScores[BG_TEAM_HORDE] = 0;

    for (uint8 i = 0; i < SSM_POINTS_MAX; ++i)
    {
        m_cartsState[i] = SSM_CONTROL_NEUTRAL;
        m_cartsCapturePoints[i] = 50;

        m_PlayersNearPoint[i].clear();
        m_PlayersNearPoint[i].reserve(15);
    }

    m_PlayersNearPoint[SSM_POINTS_MAX].clear();
    m_PlayersNearPoint[SSM_POINTS_MAX].reserve(30);
}

BattlegroundSSM::~BattlegroundSSM()
{
}

bool BattlegroundSSM::SetupBattleground()
{
    m_cart[0] = AddCart(BG_SSM_CART_1, Way1[0]);
    m_cart[1] = AddCart(BG_SSM_CART_2, Way2[0]);
    m_cart[2] = AddCart(BG_SSM_CART_3, Way3[0]);

    // gates
    if (!AddObject(BG_DOOR_1, BG_SSM_DOOR, 640.48f, 209.58f, 328.84f, 0.116671f, 0, 0, 0.058f, 0.99f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DOOR_2, BG_SSM_DOOR, 657.515f, 230.798f, 328.932f, 0.116671f, 0, 0, 0.058f, 0.99f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DOOR_3, BG_SSM_DOOR, 825.491f, 144.609f, 328.926f, 2.91383f, 0, 0, 0.993f, 0.113f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DOOR_4, BG_SSM_DOOR, 847.954f, 156.54f, 328.801f, 3.09369f, 0, 0, 0.99f, 0.023f, RESPAWN_IMMEDIATELY)   
        // buffs
        || !AddObject(BG_SSM_OBJECT_SPEEDBUFF, BG_OBJECTID_SPEEDBUFF_ENTRY, 865.844f, 10.441f, 362.424f, 1.719f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_SSM_OBJECT_REGENBUFF, BG_OBJECTID_REGENBUFF_ENTRY, 787.352f, 271.7178f, 358.240f, 5.729f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_SSM_OBJECT_BERSERKBUFF, BG_OBJECTID_BERSERKERBUFF_ENTRY, 756.198f, 75.984f, 371.229f, 1.354f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME)
        )
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundSSM: Failed to spawn some object Battleground not created!");
        return false;
    }

    WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(BG_SSM_ALLIANCE_GRAVEYARD);
    if (!sg || !AddSpiritGuide(BG_SSM_SPIRIT_MAIN_ALLIANCE, sg->x, sg->y, sg->z, 3.124139f, ALLIANCE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundWS: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_SSM_HORDE_GRAVEYARD);
    if (!sg || !AddSpiritGuide(BG_SSM_SPIRIT_MAIN_HORDE, sg->x, sg->y, sg->z, 3.193953f, HORDE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundWS: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    return true;
}

void BattlegroundSSM::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        m_cart[0] = UpdateCart(0);
        m_cart[1] = UpdateCart(1);
        m_cart[2] = UpdateCart(2);

        this->CheckSomeoneJoinedPoint();
        //check if player left point
        this->CheckSomeoneLeftPoint();

        if (m_timerCartsUpdate <= int32(diff))
        {
            UpdatePoints();
            m_timerCartsUpdate = SSM_CARTS_UPDATE_TIMER;
        }
        else
            m_timerCartsUpdate -= diff;

        if (m_timerPointsUpdate <= int32(diff))
        {
            UpdateScore();
            m_timerPointsUpdate = SSM_SCORE_UPDATE_TIMER;
        }
        else
            m_timerPointsUpdate -= diff;
    }

}

Creature* BattlegroundSSM::AddCart(uint32 type, Location loc)
{
    Creature* creature = NULL;
    if (creature = AddCreature(BG_SSM_CART, type, TEAM_NEUTRAL, loc.x, loc.y, loc.z, loc.o))
    {
        creature->CastSpell(creature, BG_SSM_SPELL_CART_MOVE);
        creature->CastSpell(creature, BG_SSM_SPELL_CONTROL_NEUTRAL);

        creature->SetSpeed(MOVE_WALK, 0.2f);
        creature->SetSpeed(MOVE_RUN,  0.2f);

        creature->SetUInt32Value(UNIT_FIELD_FLAGS, 0);
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

        m_cartsState[type] = SSM_CONTROL_NEUTRAL;
        m_cartsCapturePoints[type] = 50;
    }

    return creature;
}

Creature* BattlegroundSSM::UpdateCart(uint32 type)
{
    Creature* cart = GetBGCreature(type);

    if (cart)
    {
        if (m_waysStep[type] >= WaysSize[type] && !cart->isMoving())
        {
            if (m_cartsState[type] == SSM_CONTROL_ALLIANCE)
                AddScore(BG_TEAM_ALLIANCE, 200);
            else if (m_cartsState[type] == SSM_CONTROL_HORDE)
                AddScore(BG_TEAM_HORDE, 200);

            cart->Kill(cart);
            DelCreature(type);
            cart = AddCart(type, (m_waysMap[type])[0]);
            SendMessageToAll(LANG_BG_SSM_SPAWN_CART, CHAT_MSG_BG_SYSTEM_NEUTRAL);

            m_waysStep[type] = 1;
        }

        if (!cart->isMoving())
        {
            uint16 id = m_waysStep[type];
            cart->MonsterMoveWithSpeed((m_waysMap[type])[id].x, (m_waysMap[type])[id].y, (m_waysMap[type])[id].z, 1.0f);
            m_waysStep[type]++;
        }
    }

    return cart;
}

void BattlegroundSSM::CheckSomeoneLeftPoint()
{
    for (uint8 i = 0; i < SSM_POINTS_MAX; ++i)
    {
        Creature* obj = m_cart[i];
        if (obj)
        {
            uint8 j = 0;
            while (j < m_PlayersNearPoint[i].size())
            {
                Player* player = ObjectAccessor::FindPlayer(m_PlayersNearPoint[i][j]);
                if (!player)
                {
                    sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundSSM:CheckSomeoneLeftPoint Player (GUID: %u) not found!", GUID_LOPART(m_PlayersNearPoint[i][j]));
                    //move not existed player to "free space" - this will cause many error showing in log, but it is a very important bug
                    m_PlayersNearPoint[SSM_POINTS_MAX].push_back(m_PlayersNearPoint[i][j]);
                    m_PlayersNearPoint[i].erase(m_PlayersNearPoint[i].begin() + j);
                    continue;
                }
                if (!player->CanCaptureTowerPoint() || !player->IsWithinDistInMap(obj, BG_SSM_POINT_RADIUS))
                    //move player out of point (add him to players that are out of points
                {
                    m_PlayersNearPoint[SSM_POINTS_MAX].push_back(m_PlayersNearPoint[i][j]);
                    m_PlayersNearPoint[i].erase(m_PlayersNearPoint[i].begin() + j);
                    this->UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_SHOW, BG_SSM_PROGRESS_BAR_DONT_SHOW, player);
                }
                else
                {
                    ++j;
                }
            }
        }
    }

}

void BattlegroundSSM::CheckSomeoneJoinedPoint()
{
    for (uint8 i = 0; i < SSM_POINTS_MAX; ++i)
    {
        Creature* obj = m_cart[i];
        if (obj)
        {
            uint8 j = 0;
            while (j < m_PlayersNearPoint[SSM_POINTS_MAX].size())
            {
                Player* player = ObjectAccessor::FindPlayer(m_PlayersNearPoint[SSM_POINTS_MAX][j]);
                if (!player)
                {
                    //sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundSSM:CheckSomeoneJoinedPoint: Player (GUID: %u) not found!", GUID_LOPART(m_PlayersNearPoint[SSM_POINTS_MAX][j]));
                    ++j;
                    continue;
                }
                if (player->CanCaptureTowerPoint() && player->IsWithinDistInMap(obj, BG_SSM_POINT_RADIUS))
                {
                    //player joined point!
                    //show progress bar
                    UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_PERCENT_GREY, BG_SSM_PROGRESS_BAR_PERCENT_GREY, player);
                    UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_cartsCapturePoints[i], player);
                    UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_SHOW, BG_SSM_PROGRESS_BAR_SHOW, player);
                    //add player to point
                    m_PlayersNearPoint[i].push_back(m_PlayersNearPoint[SSM_POINTS_MAX][j]);
                    //remove player from "free space"
                    m_PlayersNearPoint[SSM_POINTS_MAX].erase(m_PlayersNearPoint[SSM_POINTS_MAX].begin() + j);
                }
                else
                    ++j;
            }
        }
    }
}

void BattlegroundSSM::AddPlayer(Player* player)
{
    //create score and add it to map
    AddPlayerScore(player->GetGUID(), new BattleGroundSSMScore);
    Battleground::AddPlayer(player);

    this->UpdateWorldStateForPlayer(SSM_POINTS_ALLIANCE, 30, player);
    this->UpdateWorldStateForPlayer(SSM_POINTS_HORDE, 40, player);

    m_PlayersNearPoint[SSM_POINTS_MAX].push_back(player->GetGUID());
}

void BattlegroundSSM::UpdateScore()
{
    int16 allianceChange = 0;
    int16 hordeChange = 0;

    for (uint8 i = 0; i < SSM_POINTS_MAX; ++i)
    {
        if (m_cartsState[i] == SSM_CONTROL_ALLIANCE)
            allianceChange += 1;
        else if (m_cartsState[i] == SSM_CONTROL_HORDE)
            hordeChange += 1;
    }

    if (allianceChange)
        AddScore(BG_TEAM_ALLIANCE, allianceChange);
    if (hordeChange)
        AddScore(BG_TEAM_HORDE, hordeChange);
}

void BattlegroundSSM::AddScore(uint32 team, int32 points)
{
    m_TeamScores[team] += points;

    if (m_TeamScores[team] >= 1600)
    {
        EndBattleground(team == BG_TEAM_ALLIANCE ? ALLIANCE : HORDE);
        CastSpellOnTeam(135787, (team == BG_TEAM_ALLIANCE ? ALLIANCE : HORDE)); // Quest credit "The Lion Roars"
    }

    if (team == BG_TEAM_ALLIANCE)
    {
        UpdateWorldState(SSM_INIT_POINTS_ALLIANCE, m_TeamScores[team]);
        UpdateWorldState(SSM_POINTS_ALLIANCE, m_TeamScores[team]);
    }
    else
    {
        UpdateWorldState(SSM_INIT_POINTS_HORDE, m_TeamScores[team]);
        UpdateWorldState(SSM_POINTS_HORDE, m_TeamScores[team]);
    }
}

void BattlegroundSSM::UpdatePoints()
{
    for (uint8 point = 0; point < SSM_POINTS_MAX; ++point)
    {
        if (m_PlayersNearPoint[point].empty())
            continue;

        uint8 hordeCount = 0;
        uint8 allianceCount = 0;
        for (uint8 i = 0; i < m_PlayersNearPoint[point].size(); ++i)
        {
            Player* player = ObjectAccessor::FindPlayer(m_PlayersNearPoint[point][i]);
            if (player)
            {
                if (player->GetBGTeam() == HORDE)
                    hordeCount++;
                else
                    allianceCount++;
            }
        }

        m_cartsCapturePoints[point] -= (hordeCount * 5);
        m_cartsCapturePoints[point] += (allianceCount * 5);

        if (m_cartsCapturePoints[point] > 100)
            m_cartsCapturePoints[point] = 100;
        else if (m_cartsCapturePoints[point] < 0)
            m_cartsCapturePoints[point] = 0;

        if (m_cartsCapturePoints[point] > 50)
        {
            if (m_cartsState[point] != SSM_CONTROL_ALLIANCE)
            {
                for (uint8 i = 0; i < m_PlayersNearPoint[point].size(); ++i)
                {
                    Player* player = ObjectAccessor::FindPlayer(m_PlayersNearPoint[point][i]);
                    if (player && player->GetBGTeam() == ALLIANCE)
                        ((BattleGroundSSMScore*)PlayerScores[player->GetGUID()])->CartsTaken++;
                }

                m_cartsState[point] = SSM_CONTROL_ALLIANCE;
                SetCartControl(point, ALLIANCE);
            }
        }
        else if (m_cartsCapturePoints[point] < 50)
        {
            if (m_cartsState[point] != SSM_CONTROL_HORDE)
            {
                for (uint8 i = 0; i < m_PlayersNearPoint[point].size(); ++i)
                {
                    Player* player = ObjectAccessor::FindPlayer(m_PlayersNearPoint[point][i]);
                    if (player && player->GetBGTeam() == HORDE)
                        ((BattleGroundSSMScore*)PlayerScores[player->GetGUID()])->CartsTaken++;
                }

                m_cartsState[point] = SSM_CONTROL_HORDE;
                SetCartControl(point, HORDE);
            }
        }
        else
        {
            if (m_cartsState[point] != SSM_CONTROL_NEUTRAL)
            {
                m_cartsState[point] = SSM_CONTROL_NEUTRAL;
                SetCartControl(point, TEAM_NONE);
            }
        }

        for (uint8 i = 0; i < m_PlayersNearPoint[point].size(); ++i)
        {
            Player* player = ObjectAccessor::FindPlayer(m_PlayersNearPoint[point][i]);
            if (player)
                UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, m_cartsCapturePoints[point], player);
        }
    }

}

void BattlegroundSSM::SetCartControl(uint32 type, uint32 team)
{
    Creature* cart = m_cart[type];
    if (cart)
    {
        cart->RemoveAura(BG_SSM_SPELL_CONTROL_ALLIANCE);
        cart->RemoveAura(BG_SSM_SPELL_CONTROL_HORDE);
        cart->RemoveAura(BG_SSM_SPELL_CONTROL_NEUTRAL);


        if (team == HORDE)
            cart->CastSpell(cart, BG_SSM_SPELL_CONTROL_HORDE);
        else if (team == ALLIANCE)
            cart->CastSpell(cart, BG_SSM_SPELL_CONTROL_ALLIANCE);
        else
            cart->CastSpell(cart, BG_SSM_SPELL_CONTROL_NEUTRAL);
    }
}

void BattlegroundSSM::StartingEventOpenDoors()
{
    DoorOpen(BG_DOOR_1);
    DoorOpen(BG_DOOR_2);
    DoorOpen(BG_DOOR_3);
    DoorOpen(BG_DOOR_4);
}

void BattlegroundSSM::RemovePlayer(Player* player, uint64 guid, uint32 /*team*/)
{
    for (int j = SSM_POINTS_MAX; j >= 0; --j)
    {
        for (size_t i = 0; i < m_PlayersNearPoint[j].size(); ++i)
            if (m_PlayersNearPoint[j][i] == guid)
                m_PlayersNearPoint[j].erase(m_PlayersNearPoint[j].begin() + i);
    }
}

WorldSafeLocsEntry const* BattlegroundSSM::GetClosestGraveYard(Player* player)
{
    return (player->GetTeam() == ALLIANCE) ? sWorldSafeLocsStore.LookupEntry(BG_SSM_ALLIANCE_GRAVEYARD) :
                                             sWorldSafeLocsStore.LookupEntry(BG_SSM_HORDE_GRAVEYARD);
}
