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

#include "Battleground.h"
#include "BattlegroundTTP.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldPacket.h"

BattlegroundTTP::BattlegroundTTP()
{
    BgObjects.resize(BG_TTP_OBJECT_MAX);

    StartDelayTimes[BG_STARTING_EVENT_FIRST]  = BG_START_DELAY_1M;
    StartDelayTimes[BG_STARTING_EVENT_SECOND] = BG_START_DELAY_30S;
    StartDelayTimes[BG_STARTING_EVENT_THIRD]  = BG_START_DELAY_15S;
    StartDelayTimes[BG_STARTING_EVENT_FOURTH] = BG_START_DELAY_NONE;
    //we must set messageIds
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_ARENA_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_ARENA_THIRTY_SECONDS;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_ARENA_FIFTEEN_SECONDS;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_ARENA_HAS_BEGUN;
}

BattlegroundTTP::~BattlegroundTTP()
{

}

void BattlegroundTTP::StartingEventCloseDoors()
{
    for (uint32 i = BG_TTP_OBJECT_DOOR_1; i <= BG_TTP_OBJECT_DOOR_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
}

void BattlegroundTTP::StartingEventOpenDoors()
{
    for (uint32 i = BG_TTP_OBJECT_DOOR_1; i <= BG_TTP_OBJECT_DOOR_2; ++i)
        DoorOpen(i);
        
    for (uint32 i = BG_TTP_OBJECT_BUFF_1; i <= BG_TTP_OBJECT_BUFF_2; ++i)
        SpawnBGObject(i, 60);
}

void BattlegroundTTP::AddPlayer(Player* player)
{
    //create score and add it to map, default values are set in constructor
    AddPlayerScore(player->GetGUID(), new BattlegroundTTPScore);
    Battleground::AddPlayer(player);
    UpdateArenaWorldState();
}

void BattlegroundTTP::RemovePlayer(Player* /*player*/, uint64 /*guid*/, uint32 /*team*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateArenaWorldState();
    CheckArenaWinConditions();
}

void BattlegroundTTP::HandleKillPlayer(Player* player, Player* killer)
{
    Battleground::HandleKillPlayer(player, killer);

    UpdateArenaWorldState();
    CheckArenaWinConditions();
}

bool BattlegroundTTP::HandlePlayerUnderMap(Player* player)
{
    player->TeleportTo(GetMapId(), 4055.504395f, 2919.660645f, 13.611241f, player->GetOrientation(), false);
    return true;
}

void BattlegroundTTP::HandleAreaTrigger(Player* Source, uint32 Trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch (Trigger)
    {
        case 9127:                                          // start position
        case 9126:                                          // start position
            break;
        default:
            sLog->outError(LOG_FILTER_BATTLEGROUND, "WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendNotification("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid, Source);
}

void BattlegroundTTP::FillInitialWorldStates(WorldPacket &data)
{
    FillInitialWorldState(data, 0xa11, 1);
    UpdateArenaWorldState();
}

void BattlegroundTTP::Reset()
{
    //call parent's class reset
    Battleground::Reset();
}

bool BattlegroundTTP::SetupBattleground()
{
    // gates
    if (!AddObject(BG_TTP_OBJECT_DOOR_1, BG_TTP_OBJECT_TYPE_DOOR_1, 501.932f, 633.429f, 380.708f, 0.0262353f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_TTP_OBJECT_DOOR_2, BG_TTP_OBJECT_TYPE_DOOR_2, 632.101f, 633.791f, 380.704f, 3.20989f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // buffs
        || !AddObject(BG_TTP_OBJECT_BUFF_1, BG_TTP_OBJECT_TYPE_BUFF_1, 566.6805f, 602.2274f, 383.6813f, 3.316144f, 0, 0, -1.f, 0, 120)
        || !AddObject(BG_TTP_OBJECT_BUFF_2, BG_TTP_OBJECT_TYPE_BUFF_2, 566.6563f, 664.566f, 383.6809f, 2.460913f, 0, 0, 0, 1.f, 120))
    {
        sLog->outError(LOG_FILTER_SQL, "BattlegroundTTP: Failed to spawn some object!");
        return false;
    }

    return true;
}
