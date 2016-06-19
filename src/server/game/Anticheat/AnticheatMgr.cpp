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

#include "AnticheatMgr.h"
#include "Player.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"

AnticheatMgr::AnticheatMgr(Player* owner) : m_player(owner)
{
    currentSpeed_C = 0.0f;
    baseRunSpeed_C = 0.0f;
    baseSwimSpeed_C = 0.0f;
    verticalDelta_C = 0.0f;
    moveFlags_C = 0.0f;
    currentSpeed_S = 0.0f;
    speedXY = 0.0f;
    speedZ = 0.0f;
    state = PLAYER_STATE_NONE;
    spline = false;
    splineDuration = 0;
    validData = true;
}

void AnticheatMgr::Update(uint32 diff)
{
    if (!validData)
        return;

    // TODO: more correct work with splines
    if (spline && splineDuration > 0)
    {
        if (diff >= splineDuration)
        {
            spline = false;
            splineDuration = 0;
        }
        else
            splineDuration -= diff;
    }
}