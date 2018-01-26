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
#include "BattlegroundBE.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldPacket.h"

BattlegroundBE::BattlegroundBE()
{
    BgObjects.resize(BG_BE_OBJECT_MAX);

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

BattlegroundBE::~BattlegroundBE()
{

}

void BattlegroundBE::StartingEventCloseDoors()
{
    for (uint32 i = BG_BE_OBJECT_DOOR_1; i <= BG_BE_OBJECT_DOOR_4; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);

    for (uint32 i = BG_BE_OBJECT_BUFF_1; i <= BG_BE_OBJECT_BUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);
}

void BattlegroundBE::StartingEventOpenDoors()
{
    for (uint32 i = BG_BE_OBJECT_DOOR_1; i <= BG_BE_OBJECT_DOOR_2; ++i)
        DoorOpen(i);

    for (uint32 i = BG_BE_OBJECT_BUFF_1; i <= BG_BE_OBJECT_BUFF_2; ++i)
        SpawnBGObject(i, 60);
}

void BattlegroundBE::AddPlayer(Player* player)
{
    //create score and add it to map, default values are set in constructor
    AddPlayerScore(player->GetGUID(), new BattlegroundBEScore);
    Battleground::AddPlayer(player);
    UpdateArenaWorldState();
}

void BattlegroundBE::RemovePlayer(Player* /*player*/, uint64 /*guid*/, uint32 /*team*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateArenaWorldState();
    CheckArenaWinConditions();
}

void BattlegroundBE::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!killer)
    {
        TC_LOG_ERROR("bg", "Killer player not found");
        return;
    }

    Battleground::HandleKillPlayer(player, killer);

    UpdateArenaWorldState();
    CheckArenaWinConditions();
}

bool BattlegroundBE::HandlePlayerUnderMap(Player* player)
{
    player->TeleportTo(GetMapId(), 6238.930176f, 262.963470f, 2.73f, player->GetOrientation(), false);
    return true;
}

void BattlegroundBE::HandleAreaTrigger(Player* Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch (Trigger)
    {
        case 4538:                                          // buff trigger?
            //buff_guid = BgObjects[BG_BE_OBJECT_BUFF_1];
            break;
        case 4539:                                          // buff trigger?
            //buff_guid = BgObjects[BG_BE_OBJECT_BUFF_2];
            break;
        case 4944: //Fall 20 yards
        case 5039: //Fall 60 yards
        case 5040: //Fall 80 yards
            HandlePlayerUnderMap(Source);
            break;
        default:
            TC_LOG_ERROR("bg", "WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendNotification("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid, Source);
}

void BattlegroundBE::FillInitialWorldStates(WorldPacket &data)
{
    FillInitialWorldState(data, 0x9f3, 1);
    UpdateArenaWorldState();
}

void BattlegroundBE::Reset()
{
    //call parent's class reset
    Battleground::Reset();
}

bool BattlegroundBE::SetupBattleground()
{
    // gates
    if (!AddObject(BG_BE_OBJECT_DOOR_1, BG_BE_OBJECT_TYPE_DOOR_1, 6287.277f, 282.1877f, 3.810925f, -2.260201f, 0, 0, 0.9044551f, -0.4265689f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_BE_OBJECT_DOOR_2, BG_BE_OBJECT_TYPE_DOOR_2, 6189.546f, 241.7099f, 3.101481f, 0.8813917f, 0, 0, 0.4265689f, 0.9044551f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_BE_OBJECT_DOOR_3, BG_BE_OBJECT_TYPE_DOOR_3, 6299.116f, 296.5494f, 3.308032f, 0.8813917f, 0, 0, 0.4265689f, 0.9044551f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_BE_OBJECT_DOOR_4, BG_BE_OBJECT_TYPE_DOOR_4, 6177.708f, 227.3481f, 3.604374f, -2.260201f, 0, 0, 0.9044551f, -0.4265689f, RESPAWN_IMMEDIATELY)
        // buffs
        || !AddObject(BG_BE_OBJECT_BUFF_1, BG_BE_OBJECT_TYPE_BUFF_1, 6266.2973f, 295.8756f, 4.4902f, -1.448624f, 0, 0, 0.6626201f, -0.7489557f, 120)
        || !AddObject(BG_BE_OBJECT_BUFF_2, BG_BE_OBJECT_TYPE_BUFF_2, 6211.0698f, 228.6061f, 4.4624f, -0.06981307f, 0, 0, 0.03489945f, -0.9993908f, 120))
    {
        TC_LOG_ERROR("sql", "BatteGroundBE: Failed to spawn some object!");
        return false;
    }

    return true;
}

void BattlegroundBE::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{

    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetGUID());
    if (itr == PlayerScores.end())                         // player not found...
        return;

    //there is nothing special in this score
    Battleground::UpdatePlayerScore(Source, type, value, doAddHonor);

}

/*
21:45:46 id:231310 [S2C] SMSG_INIT_WORLD_STATES (706 = 0x02C2) len: 86
0000: 32 02 00 00 76 0e 00 00 00 00 00 00 09 00 f3 09  |  2...v...........
0010: 00 00 01 00 00 00 f1 09 00 00 01 00 00 00 f0 09  |  ................
0020: 00 00 02 00 00 00 d4 08 00 00 00 00 00 00 d8 08  |  ................
0030: 00 00 00 00 00 00 d7 08 00 00 00 00 00 00 d6 08  |  ................
0040: 00 00 00 00 00 00 d5 08 00 00 00 00 00 00 d3 08  |  ................
0050: 00 00 00 00 00 00                                |  ......

spell 32724 - Gold Team
spell 32725 - Green Team
35774 Gold Team
35775 Green Team
*/
