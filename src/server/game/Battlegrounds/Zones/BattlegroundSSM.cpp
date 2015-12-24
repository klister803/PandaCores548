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
#include "MoveSplineInit.h"

BattlegroundSSM::BattlegroundSSM()
{
    BgCreatures.resize(BG_SSM_OBJECT_MAX);
    BgObjects.resize(BG_SSM_OBJECT_MAX);
    
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_SSM_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_SSM_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_SSM_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_SSM_HAS_BEGUN;


    for (uint8 i = 0; i < BG_SSM_MAX_CARTS; ++i)
    {
        _cartsState[i] = SSM_CONTROL_NEUTRAL;
        _cartsCapturePoints[i] = BG_SSM_PROGRESS_BAR_STATE_MIDDLE;
        _cartsAdded[i] = false;

        _playersNearPoint[i].clear();
        _playersNearPoint[i].reserve(15);

        _cart[i] = nullptr;
    }

    _tractSwitchState[BG_SSM_CART_EAST] = TRACK_SWITCH_STATE_DEFAULT;
    _tractSwitchState[BG_SSM_CART_NORTH] = TRACK_SWITCH_STATE_DEFAULT;

    _playersNearPoint[BG_SSM_MAX_CARTS].clear();
    _playersNearPoint[BG_SSM_MAX_CARTS].reserve(30);

    _timerPointsUpdate = SSM_SCORE_UPDATE_TIMER;
    _timerCartsUpdate = SSM_CARTS_UPDATE_TIMER;
}

BattlegroundSSM::~BattlegroundSSM()
{ }

bool BattlegroundSSM::SetupBattleground()
{
    if (!AddObject(BG_DOOR_1, OBJECT_BG_SSM_DOOR3, 640.48f, 209.58f, 328.84f, 0.116671f, 0, 0, 0.058f, 0.99f, RESPAWN_IMMEDIATELY) ||
        !AddObject(BG_DOOR_2, OBJECT_BG_SSM_DOOR3, 657.515f, 230.798f, 328.932f, 0.116671f, 0, 0, 0.058f, 0.99f, RESPAWN_IMMEDIATELY) ||
        !AddObject(BG_DOOR_3, OBJECT_BG_SSM_DOOR3, 825.491f, 144.609f, 328.926f, 2.91383f, 0, 0, 0.993f, 0.113f, RESPAWN_IMMEDIATELY) ||
        !AddObject(BG_DOOR_4, OBJECT_BG_SSM_DOOR3, 847.954f, 156.54f, 328.801f, 3.09369f, 0, 0, 0.99f, 0.023f, RESPAWN_IMMEDIATELY) ||

        !AddObject(BG_SSM_OBJECT_SPEEDBUFF, BG_OBJECTID_SPEEDBUFF_ENTRY, 865.844f, 10.441f, 362.424f, 1.719f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME) ||
        !AddObject(BG_SSM_OBJECT_REGENBUFF, BG_OBJECTID_REGENBUFF_ENTRY, 787.352f, 271.7178f, 358.240f, 5.729f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME) ||
        !AddObject(BG_SSM_OBJECT_BERSERKBUFF, BG_OBJECTID_BERSERKERBUFF_ENTRY, 756.198f, 75.984f, 371.229f, 1.354f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME) ||
        !AddObject(BG_POINT_END1, OBJECT_BG_SSM_THE_DESPOSITS_OF_DIAMONDS, BgSSMObjectsPos[0][0], BgSSMObjectsPos[0][1], RESPAWN_IMMEDIATELY) ||
        !AddObject(BG_POINT_END2, OBJECT_BG_SSM_RESERVOIR, BgSSMObjectsPos[1][0], BgSSMObjectsPos[1][1], RESPAWN_IMMEDIATELY) ||
        !AddObject(BG_POINT_END3, OBJECT_BG_SSM_THE_DESPOSITION_OF_LAVA, BgSSMObjectsPos[2][0], BgSSMObjectsPos[2][1], RESPAWN_IMMEDIATELY) ||
        !AddObject(BG_POINT_END4, OBJECT_BG_SSM_BACKLOG_TROLLS, BgSSMObjectsPos[3][0], BgSSMObjectsPos[3][1], RESPAWN_IMMEDIATELY))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundSSM: Failed to spawn some object Battleground not created!");
        return false;
    }

    if (!AddCreature(CREATURE_BG_SSM_TRACK_SWITCH, BG_SSM_CREATURE_TRACK1, TEAM_NEUTRAL, BgSSMCreaturePos[0], RESPAWN_IMMEDIATELY) ||
        !AddCreature(CREATURE_BG_SSM_TRACK_SWITCH, BG_SSM_CREATURE_TRACK2, TEAM_NEUTRAL, BgSSMCreaturePos[1], RESPAWN_IMMEDIATELY))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundSSM: Failed to spawn some creatures Battleground not created!");
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
    if (GetElapsedTime() < 1 * MINUTE * IN_MILLISECONDS)
        return;

    if (!_cartsAdded[BG_SSM_CART_EAST])
    {
        _cart[BG_SSM_CART_EAST] = _AddCart(BG_SSM_CART_EAST, WayEastBase[0]);
        _cartsAdded[BG_SSM_CART_EAST] = true;
    }

    if (!_cartsAdded[BG_SSM_CART_SOUTH])
    {
        _cart[BG_SSM_CART_SOUTH] = _AddCart(BG_SSM_CART_SOUTH, WaySouth[0]);
        _cartsAdded[BG_SSM_CART_SOUTH] = true;
    }

    if (!_cartsAdded[BG_SSM_CART_NORTH])
    {
        _cart[BG_SSM_CART_NORTH] = _AddCart(BG_SSM_CART_NORTH, WayNorthBase[0]);
        _cartsAdded[BG_SSM_CART_NORTH] = true;
    }

    _CheckPlayersAtCars();

    for (uint8 i = 0; i < BG_SSM_MAX_CARTS; ++i)
        _cart[i] = _UpdateCart(i, false);

    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (_timerCartsUpdate <= diff)
    {
        _UpdatePoints();
        _timerCartsUpdate = SSM_CARTS_UPDATE_TIMER;
    }
    else
        _timerCartsUpdate -= diff;

    if (_timerPointsUpdate <= diff)
    {
        _UpdateScore();
        _timerPointsUpdate = SSM_SCORE_UPDATE_TIMER;
    }
    else
        _timerPointsUpdate -= diff;
}

void BattlegroundSSM::HandleAreaTrigger(Player* player, uint32 trigger/*, bool entered*/)
{
    //switch (trigger)
    //{
    //    case 8493: // Alliance start loc
    //    case 8494: // Horde start loc
    //        if (!entered && GetStatus() == STATUS_WAIT_JOIN)
    //            player->TeleportTo(GetMapId(), GetTeamStartPosition(player->GetTeamId()));
    //        break;
    //    default:
    //        Battleground::HandleAreaTrigger(player, trigger/*, entered*/);
    //        break;
    //}
}

Creature* BattlegroundSSM::_AddCart(uint32 type, Position const loc)
{
    Creature* creature = AddCreature(CREATURE_BG_SSM_CART, type, TEAM_NEUTRAL, loc);
    if (!creature)
        return nullptr;

    PlaySoundToAll(SoundKitCartSpawn/*, creature->GetGUID()*/);
    //SendBroadcastTextToAll(BroadcastCartSpawn, CHAT_MSG_BG_SYSTEM_NEUTRAL, creature);

    creature->CastSpell(creature, BG_SSM_SPELL_CONTROL_NEUTRAL);

    switch (type)
    {
        case BG_SSM_CART_SOUTH:
            creature->CastSpell(creature, BG_SSM_SPELL_CART_CONTROL_CAPTURE_POINT_UNIT_SOUTH);
            break;
        case BG_SSM_CART_NORTH:
            creature->CastSpell(creature, BG_SSM_SPELL_CART_CONTROL_CAPTURE_POINT_UNIT_NORTH);
            break;
        case BG_SSM_CART_EAST:
            creature->CastSpell(creature, BG_SSM_SPELL_CART_CONTROL_CAPTURE_POINT_UNIT_EAST);
            break;
        default:
            break;
    }

    creature->CastSpell(creature, BG_SSM_SPELL_DEFENDING_CART_AURA);
    creature->SetUInt32Value(UNIT_FIELD_FLAGS, 0);
    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);

    _cartsState[type] = SSM_CONTROL_NEUTRAL;
    _cartsCapturePoints[type] = BG_SSM_PROGRESS_BAR_STATE_MIDDLE;
    _UpdateCart(type);

    return creature;
}

Creature* BattlegroundSSM::_UpdateCart(uint32 type, bool initial /*= true*/)
{
    Creature* cart = GetBGCreature(type);
    if (!cart)
        return nullptr;

    bool _isExtraWay = cart->GetDistance(CartPointData[type].Pos) < 1.0f;
    bool _isMoving = cart->isMoving();

    if (initial || (_isExtraWay && !_isMoving))
    {
        uint32 waySize = 0;
        Movement::MoveSplineInit init(*cart);
        switch (type)
        {
            case BG_SSM_CART_EAST:
                if (initial || _isExtraWay)
                {
                    if (_isExtraWay)
                        waySize = ExtraWaysSize[type][_tractSwitchState[type] == TRACK_SWITCH_STATE_DEFAULT ? 0 : 1];
                    else
                        waySize = WaysSize[type];

                    for (uint8 i = 1; i < waySize; ++i)
                        init.PushPath(_isExtraWay ? (_tractSwitchState[type] == TRACK_SWITCH_STATE_DEFAULT ? ExtraWayEast1[i] : ExtraWayEast2[i]) : WayEastBase[i]);
                }
                break;
            case BG_SSM_CART_NORTH:
                if (initial || _isExtraWay)
                {
                    if (_isExtraWay)
                        waySize = ExtraWaysSize[type][_tractSwitchState[type] == TRACK_SWITCH_STATE_DEFAULT ? 0 : 1];
                    else
                        waySize = WaysSize[type];

                    for (uint8 i = 1; i < waySize; ++i)
                        init.PushPath(_isExtraWay ? (_tractSwitchState[type] == TRACK_SWITCH_STATE_DEFAULT ? WayNorthLeft[i] : WayNorthRight[i]) : WayNorthBase[i]);
                    break;
                }
            case BG_SSM_CART_SOUTH:
                if (initial)
                    for (uint8 i = 1; i < WaysSize[type]; ++i)
                        init.PushPath(WaySouth[i]);
                break;
            default:
                break;
        }

        init.SetWalk(true);
        init.Launch();
    }

    if (!_isMoving && (cart->GetDistance(CartPointData[type].EndPos) < 1.0f || cart->GetDistance(CartPointData[type].EndPos2) < 1.0f))
    {
        _cartsAdded[type] = false;

        if (_cartsState[type] == SSM_CONTROL_ALLIANCE)
            _AddScore(TEAM_ALLIANCE, 200);
        else if (_cartsState[type] == SSM_CONTROL_HORDE)
            _AddScore(TEAM_HORDE, 200);

        cart->Kill(cart);
        DelCreature(type);
    }

    return cart;
}

void BattlegroundSSM::_CheckPlayersAtCars()
{
    for (uint8 i = 0; i < BG_SSM_MAX_CARTS; ++i)
    {
        Creature* obj = _cart[i];
        if (!obj)
            continue;

        size_t j = 0;
        while (j < _playersNearPoint[BG_SSM_MAX_CARTS].size())
        {
            Player* player = ObjectAccessor::FindPlayer(_playersNearPoint[BG_SSM_MAX_CARTS][j]);
            if (!player)
            {
                ++j;
                continue;
            }
            if (player->CanCaptureTowerPoint() && player->IsWithinDistInMap(obj, BG_SSM_POINT_RADIUS))
            {
                UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_PERCENT_GREY, BG_SSM_PROGRESS_BAR_PERCENT_GREY, player);
                UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, _cartsCapturePoints[i], player);
                UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_SHOW, BG_SSM_PROGRESS_BAR_SHOW, player);

                _playersNearPoint[i].push_back(_playersNearPoint[BG_SSM_MAX_CARTS][j]);
                _playersNearPoint[BG_SSM_MAX_CARTS].erase(_playersNearPoint[BG_SSM_MAX_CARTS].begin() + j);
            }
            else
                ++j;
        }

        size_t x = 0;
        while (x < _playersNearPoint[i].size())
        {
            Player* player = ObjectAccessor::FindPlayer(_playersNearPoint[i][x]);
            if (!player)
            {
                _playersNearPoint[BG_SSM_MAX_CARTS].push_back(_playersNearPoint[i][x]);
                _playersNearPoint[i].erase(_playersNearPoint[i].begin() + x);
                continue;
            }
            if (!player->CanCaptureTowerPoint() || !player->IsWithinDistInMap(obj, BG_SSM_POINT_RADIUS))
            {
                _playersNearPoint[BG_SSM_MAX_CARTS].push_back(_playersNearPoint[i][x]);
                _playersNearPoint[i].erase(_playersNearPoint[i].begin() + x);
                UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_SHOW, BG_SSM_PROGRESS_BAR_DONT_SHOW, player);
            }
            else
                ++x;
        }
    }
}

void BattlegroundSSM::AddPlayer(Player* player)
{
    AddPlayerScore(player->GetGUID(), new BattleGroundSSMScore);
    Battleground::AddPlayer(player);

    //player->SendDirectMessage(WorldPackets::Battleground::Init(SSM_MAX_TEAM_POINTS).Write());
    //Battleground::SendBattleGroundPoints(player->GetTeamId() != TEAM_ALLIANCE, m_TeamScores[player->GetTeamId()], false, player);

    _playersNearPoint[BG_SSM_MAX_CARTS].push_back(player->GetGUID());
}

void BattlegroundSSM::_UpdateScore()
{
    int16 allianceChange = 0;
    int16 hordeChange = 0;

    for (uint8 i = 0; i < BG_SSM_MAX_CARTS; ++i)
    {
        if (_cartsState[i] == SSM_CONTROL_ALLIANCE)
            allianceChange += 1;
        else if (_cartsState[i] == SSM_CONTROL_HORDE)
            hordeChange += 1;
    }

    _AddScore(TEAM_ALLIANCE, allianceChange);
    _AddScore(TEAM_HORDE, hordeChange);
}

void BattlegroundSSM::_AddScore(TeamId team, int32 points)
{
    m_TeamScores[team] += points;

    //Battleground::SendBattleGroundPoints(team != TEAM_ALLIANCE, m_TeamScores[team]);

    if (m_TeamScores[team] > SSM_MAX_TEAM_POINTS)
        m_TeamScores[team] = SSM_MAX_TEAM_POINTS;

    UpdateWorldState(team == TEAM_ALLIANCE ? SSM_POINTS_ALLIANCE : SSM_POINTS_HORDE, m_TeamScores[team]);

    if (m_TeamScores[team] == SSM_MAX_TEAM_POINTS)
    {
        EndBattleground(GetTeamByTeamId(team));
        CastSpellOnTeam(135787, GetTeamByTeamId(team)); // Quest credit "The Lion Roars"
    }
}

void BattlegroundSSM::_UpdatePoints()
{
    for (uint8 point = 0; point < BG_SSM_MAX_CARTS; ++point)
    {
        if (_playersNearPoint[point].empty())
            continue;

        uint8 hordeCount = 0;
        uint8 allianceCount = 0;
        for (size_t i = 0; i < _playersNearPoint[point].size(); ++i)
        {
            Player* player = ObjectAccessor::FindPlayer(_playersNearPoint[point][i]);
            if (player)
            {
                if (player->GetBGTeam() == HORDE)
                    hordeCount++;
                else
                    allianceCount++;
            }
        }

        _cartsCapturePoints[point] -= (hordeCount * 5);
        _cartsCapturePoints[point] += (allianceCount * 5);

        if (_cartsCapturePoints[point] > BG_SSM_PROGRESS_BAR_ALI_CONTROLLED)
            _cartsCapturePoints[point] = BG_SSM_PROGRESS_BAR_ALI_CONTROLLED;
        else if (_cartsCapturePoints[point] < BG_SSM_PROGRESS_BAR_HORDE_CONTROLLED)
            _cartsCapturePoints[point] = BG_SSM_PROGRESS_BAR_HORDE_CONTROLLED;

        if (_cartsCapturePoints[point] > BG_SSM_PROGRESS_BAR_STATE_MIDDLE)
        {
            if (_cartsState[point] != SSM_CONTROL_ALLIANCE)
            {
                for (size_t i = 0; i < _playersNearPoint[point].size(); ++i)
                {
                    Player* player = ObjectAccessor::FindPlayer(_playersNearPoint[point][i]);
                    if (player && player->GetTeamId() == TEAM_ALLIANCE)
                        UpdatePlayerScore(player, SCORE_CARTS_HELPED, 1);
                }

                _cartsState[point] = SSM_CONTROL_ALLIANCE;
                _SetCartControl(point, TEAM_ALLIANCE);
            }
        }
        else if (_cartsCapturePoints[point] < BG_SSM_PROGRESS_BAR_STATE_MIDDLE)
        {
            if (_cartsState[point] != SSM_CONTROL_HORDE)
            {
                for (size_t i = 0; i < _playersNearPoint[point].size(); ++i)
                {
                    Player* player = ObjectAccessor::FindPlayer(_playersNearPoint[point][i]);
                    if (player && player->GetTeamId() == TEAM_HORDE)
                        UpdatePlayerScore(player, SCORE_CARTS_HELPED, 1);
                }

                _cartsState[point] = SSM_CONTROL_HORDE;
                _SetCartControl(point, TEAM_HORDE);
            }
        }
        else
        {
            if (_cartsState[point] != SSM_CONTROL_NEUTRAL)
            {
                _cartsState[point] = SSM_CONTROL_NEUTRAL;
                _SetCartControl(point, TEAM_NEUTRAL);
            }
        }

        for (size_t i = 0; i < _playersNearPoint[point].size(); ++i)
        {
            Player* player = ObjectAccessor::FindPlayer(_playersNearPoint[point][i]);
            if (player)
                UpdateWorldStateForPlayer(SSM_PROGRESS_BAR_STATUS, _cartsCapturePoints[point], player);
        }
    }
}

void BattlegroundSSM::_SetCartControl(uint32 type, TeamId teamID)
{
    Creature* cart = _cart[type];
    if (!cart)
        return;

    cart->RemoveAura(BG_SSM_SPELL_CONTROL_ALLIANCE);
    cart->RemoveAura(BG_SSM_SPELL_CONTROL_HORDE);
    cart->RemoveAura(BG_SSM_SPELL_CONTROL_NEUTRAL);

    if (teamID == TEAM_HORDE)
        cart->CastSpell(cart, BG_SSM_SPELL_CONTROL_HORDE);
    else if (teamID == TEAM_ALLIANCE)
        cart->CastSpell(cart, BG_SSM_SPELL_CONTROL_ALLIANCE);
    else
        cart->CastSpell(cart, BG_SSM_SPELL_CONTROL_NEUTRAL);
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
    for (int8 j = BG_SSM_MAX_CARTS; j >= 0; --j)
    {
        for (size_t i = 0; i < _playersNearPoint[j].size(); ++i)
            if (_playersNearPoint[j][i] == guid)
                _playersNearPoint[j].erase(_playersNearPoint[j].begin() + i);
    }
}

void BattlegroundSSM::HandleKillPlayer(Player* player, Player* killer)
{
    Battleground::HandleKillPlayer(player, killer);

    _AddScore(killer->GetTeamId(), 1);
}

void BattlegroundSSM::FillInitialWorldStates(WorldPacket& data)
{
    data << uint32(SSM_POINTS_ALLIANCE)             << uint32(m_TeamScores[TEAM_ALLIANCE]);
    data << uint32(SSM_POINTS_HORDE)                << uint32(m_TeamScores[TEAM_HORDE]);
    data << uint32(6439) << uint32(0);
    data << uint32(6440) << uint32(0); // related carts capturing? 1 << uint32(after capturing 0
    data << uint32(SSM_INIT_POINTS_ALLIANCE)        << uint32(1);
    data << uint32(6442) << uint32(1);
    data << uint32(SSM_INIT_POINTS_HORDE)           << uint32(1);
    data << uint32(6455) << uint32(0);
    data << uint32(6456) << uint32(0);
    data << uint32(6457) << uint32(0);
    data << uint32(6458) << uint32(0);
    data << uint32(6459) << uint32(0);
    data << uint32(SSM_EAST_TRACK_SWITCH)           << uint32(_tractSwitchState[BG_SSM_CART_EAST]);
    data << uint32(SSM_NORTH_TRACK_SWITCH)          << uint32(_tractSwitchState[BG_SSM_CART_NORTH]);
    data << uint32(SSM_PROGRESS_BAR_SHOW)           << uint32(BG_SSM_PROGRESS_BAR_DONT_SHOW);
    data << uint32(SSM_PROGRESS_BAR_STATUS)         << uint32(BG_SSM_PROGRESS_BAR_DONT_SHOW);
    data << uint32(SSM_PROGRESS_BAR_PERCENT_GREY)   << uint32(BG_SSM_PROGRESS_BAR_PERCENT_GREY);
    data << uint32(6879) << uint32(0);
    data << uint32(6880) << uint32(0);
    data << uint32(6881) << uint32(0);
    data << uint32(6882) << uint32(0);
    data << uint32(9601) << uint32(0);
    data << uint32(9602) << uint32(0);
    data << uint32(9603) << uint32(0);
    data << uint32(9604) << uint32(0);
    data << uint32(9605) << uint32(0);
    data << uint32(9606) << uint32(0);
}

WorldSafeLocsEntry const* BattlegroundSSM::GetClosestGraveYard(Player* player)
{
    return player->GetTeamId() == TEAM_ALLIANCE ? sWorldSafeLocsStore.LookupEntry(BG_SSM_ALLIANCE_GRAVEYARD) : sWorldSafeLocsStore.LookupEntry(BG_SSM_HORDE_GRAVEYARD);
}
