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

#ifndef __ANTICHEAT_MGR_H
#define __ANTICHEAT_MGR_H

#include "Common.h"
#include "SharedDefines.h"
#include "Unit.h"

enum PlayerState
{
    PLAYER_STATE_NONE = 0x00,
    PLAYER_STATE_LAUNCHED = 0x01,
    PLAYER_STATE_SWIMMING = 0x02,
    PLAYER_STATE_HOVER = 0x04,
    PLAYER_STATE_WATER_WALK = 0x08,
    PLAYER_STATE_FALLING = 0x10,
    PLAYER_STATE_ONTRANSPORT = 0x20,
    PLAYER_STATE_CAN_FLY = 0x40,
    PLAYER_STATE_FLYING = 0x80
};

class Player;

class AnticheatMgr
{
public:
    explicit AnticheatMgr(Player* owner);
    ~AnticheatMgr() {}

public:
    void Init();
    void Update(uint32 diff);

    void AddState(PlayerState s) { state |= s; }
    void RemoveState(PlayerState s) { state &= ~s; }
    bool HasState(PlayerState s) const { return state & s; }
    PlayerState GetState() const { return PlayerState(state); }

    void AddMovementFlag(MovementFlags f) { moveFlags_C |= f; }
    void RemoveMovementFlag(MovementFlags f) { moveFlags_C &= ~f; }
    bool HasMovementFlag(MovementFlags f) const { return moveFlags_C & f; }
    MovementFlags GetMovementFlags() const { return MovementFlags(moveFlags_C); }
    void SetMovementFlags(MovementFlags f) { moveFlags_C = f; }

    void SetCurrentClientSpeed(float speed) { currentSpeed_C = speed; }
    float GetCurrentClientSpeed() { return currentSpeed_C; }

    void SetCurrentServerSpeed(float speed) { currentSpeed_S = speed; }
    float GetCurrentServerSpeed() { return currentSpeed_S; }

    void SetBaseRunSpeed(float speed) { baseRunSpeed_C = speed; }
    float GetBaseRunSpeed() { return baseRunSpeed_C; }

    void SetBaseFlightSpeed(float speed) { baseFlightSpeed_C = speed; }
    float GetBaseFlightSpeed() { return baseFlightSpeed_C; }

    void SetBaseSwimSpeed(float speed) { baseSwimSpeed_C = speed; }
    float GetBaseSwimSpeed() { return baseSwimSpeed_C; }

    void SetVerticalDelta(float delta) { verticalDelta_C = delta; }
    float GetVerticalDelta() { return verticalDelta_C; }

    void SetSpeedXY(float speed) { speedXY = speed; }
    float GetSpeedXY() { return speedXY; }

    void SetSpeedZ(float speed) { speedZ = speed; }
    float GetSpeedZ() { return speedZ; }

    void SetSpline(bool apply) { spline = apply; }
    bool HasSpline() { return spline; }

    void SetSplineDuration(uint32 duration) { splineDuration = duration; }
    uint32 GetSplineDuration() { return splineDuration; }

    void SetValidData(bool apply) { validData = apply; }
    bool IsValidData() { return validData; }

private:
    // client vars
    float currentSpeed_C;
    float baseRunSpeed_C;
    float baseFlightSpeed_C;
    float baseSwimSpeed_C;
    float verticalDelta_C;
    uint32 moveFlags_C;
    // server vars
    float currentSpeed_S;
    float speedXY;
    float speedZ;
    // some data
    uint8 state;
    bool spline;
    uint32 splineDuration;
    // player
    Player* m_player;
    // service vars
    bool validData;
};

#endif