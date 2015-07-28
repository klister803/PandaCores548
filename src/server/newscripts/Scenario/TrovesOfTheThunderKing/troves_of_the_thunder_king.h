/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#include "NewScriptPCH.h"
#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MoveSplineInit.h"
#include "AchievementMgr.h"

#ifndef DEF_TROVES_OF_THE_THUNDER_KING
#define DEF_TROVES_OF_THE_THUNDER_KING

enum Spells
{
    //< Player self cast spells
    //Spelld: 98877	    Sanctuary
    SPELL_TIMED_RUN_STARTED_SPELL               = 139955,
    SPELL_LIMITED_TIME                          = 140000,
    SPELL_TROVES_OF_THE_THUNDER_KING            = 137275,
    //Spelld: 138545	Rare Drop Tracking Quest
    //Spelld: 140173	Scenario Completion Credit
    //Spelld: 140005	Scenario Completion Blackout Aura
    //Spelld: 140172	Loot Room Completion Area
    //Spelld: 140156	Unlocking Lei Shen's Burial Trove
    //Spelld: 136635	Sunreaver's Cloak
    //Spelld: 139389	Player Kill Handler
    //Spelld: 137225	Carrying the Head


    //< NPC_CLOUD_TILE_TRAP_BUNNY
    SPELL_DESPAWN_AREA_TRIGGERS                 = 124044,

    //< NPC_LIGHTING_PILAR_MASTER_BUNNY
    SPELL_LIGHTING_SURGE                        = 139806,
    SPELL_LIGHTING_SURGE_2                      = 139804,

    //< NPC_SPEED_RUNE
    SPELL_CREATE_MOGU_RUNE_GREEN_AREA_TRIGGER   = 130535,
    SPELL_SPIRITS_SWIFTNESS                     = 139794,
    // SPELL_DESPAWN_AREA_TRIGGERS

    //< NPC_LIGHTING_PILAR_TARGET_BUNNY
    SPELL_LIGHTING_SURGE_3                      = 139805,
    SPELL_LIGHTING_SURGE_4                      = 140469,

    //< NPC_STONE_SENTIEL
    SPELL_STONE_SMASH                           = 139777,
    SPELL_STATUE_FROZEN_SLAM_PREP               = 139778,

    //< NPC_FIRE_TILE_TRAP_BUNNY
    SPELL_FLAME_SPOUT                           = 120321,
    SPELL_FLAME_TILE                            = 139799,
    // SPELL_DESPAWN_AREA_TRIGGERS

    //< NPC_ARROW_TILE_TRAP_BUNNY
    SPELL_ARROW_TRAP                            = 127473,
    SPELL_ARROW_TRAP_2                          = 127426,
    SPELL_ARROW_TILE                            = 139802,
    // SPELL_DESPAWN_AREA_TRIGGERS

    //< NPC_STASIS_RUNE
    SPELL_RUNE_TRAP                             = 139798,
    // SPELL_DESPAWN_AREA_TRIGGERS

    //< NPC_LIGHTING_TILE_TRAP_BUNNY
    SPELL_LIGHTING_CRASH                        = 123795,
    SPELL_LIGHTING_TILE                         = 139801,
    // SPELL_DESPAWN_AREA_TRIGGERS



    //< NPC_ZANDALARI_BATTLE_BEAST
    SPELL_SMASH                                 = 140449,

//Creature entry: 70295
    SPELL_BACKSTAB                              = 75360, 

//Creature entry: 70298
    //SPELL_BACKSTAB

//Creature entry: 70293
    SPELL_SHOOT                                 = 120148,

//Creature entry: 69826
    SPELL_STRIKE_FROM_SHADOWS                   = 140438,

//Creature entry: 70284
    SPELL_DISARM                                = 6713,
    SPELL_IMPALE                                = 79444,
    SPELL_TAUNT                                 = 26281,

//Creature entry: 70317
    SPELL_FIREBALL                              = 79854,
    SPELL_MOLTEN_ARMOR                          = 79849,
    SPELL_BLAST_WAVE                            = 79857,
    SPELL_FLAMESTRIKE                           = 79856,

//Creature entry: 70327
    SPELL_GURTHANS_WATCH_PING                   = 128452,

//Creature entry: 70401
    SPELL_ARCANE_BALL                           = 13748,

//Creature entry: 71297
    SPELL_REND                                  = 140396,

//Creature entry: 58071
    SPELL_BLOODY_EXPLOSION                      = 116686,

//Creature entry: 71298
    SPELL_HUNTERS_MARK                          = 80016,

};

enum Data
{
    DATA_NONE,

    DATA_EVENT_STARTED,
};

enum eCreatures
{
    NPC_TAOSHI                      = 70320,

    NPC_GENERIC_CONTROLLER_BUNNY    = 44403,
    NPC_FIRE_TILE_TRAP_BUNNY        = 70329,
    NPC_LIGHTING_TILE_TRAP_BUNNY    = 70330,
    NPC_CLOUD_TILE_TRAP_BUNNY       = 70331,
    NPC_ARROW_TILE_TRAP_BUNNY       = 70332,
    NPC_LIGHTING_PILAR_MASTER_BUNNY = 70409,
    NPC_LIGHTING_PILAR_TARGET_BUNNY = 70410,
    NPC_SPEED_RUNE                  = 70405,
    NPC_STASIS_RUNE                 = 70406,

    NPC_SHANZE_BLOODSEEKER          = 69431,
    NPC_SUPPLIER_BAO                = 70319,
    NPC_STONE_SENTIEL               = 70324,
    NPC_STONE_SENTRY                = 70326,
    NPC_ZANDALARE_VENOMBLADE        = 70328,
    NPC_SENTRY_TOTEM                = 70413,
    NPC_ZANDALARI_BATTLE_BEAST      = 69795,

    NPC_TENWU_OF_THE_RED_SMOKE      = 70321,
    NPC_WU_KAO_ROGUE                = 70322,
};

enum eGameObects
{
    GO_MOGU_TREASURE_CHEST                          = 218757,
    GO_GOLDEN_TREASURE_CHEST                        = 218772,
    GO_LEI_SHENS_BURIAL_TROVE                       = 218949,
    GO_TREASURE_ZANDALARI_CHEST_03_00               = 218780,
    GO_TREASURE_ZANDALARI_CHEST_01                  = 218800,
    GO_TREASURE_ZANDALARI_CHEST_03                  = 218761,
    GO_TREASURE_MOGU_CHEST                          = 218773,
    GO_TREASURE_COVER_MOGU_CHEST                    = 218774,
    GO_TREASURE_COVER_MOGU_CHEST_2                  = 218775,
    GO_TREASURE_COVER_MOGU_CHEST_3                  = 218776,

    GO_DOODAD_THUNDERISLE_RAID_DOOR_007             = 218933,
    GO_DOAD_THUNDER_ELSE_RAID_DOOR_03               = 218939,
    GO_DOODAD_MOGU_ARENA_GATE_SMALL_01_DRAKKARI001  = 218941,
    GO_DOODAD_MOGU_ARENA_GATE_SMALL_01_GURUBASHI001 = 218942,
    GO_DOODAD_MOGU_ARENA_GATE_SMALL_01_FARRAKI001   = 218943,
    GO_DOODAD_MOGU_ARENA_GATE_SMALL_01_AMANI001     = 218944,
    GO_DOODAD_MOGU_ARENA_GATE_LARGE_001             = 218945,
    GO_LEVER                                        = 218796,
    GO_PILAR_OF_LIGHTING                            = 218854,
    GO_TRAPS_WITH_ARROWS                            = 218923,
    GO_CLOUD_TRAP                                   = 218924,
    GO_FIRE_TRAP                                    = 218925,
    GO_ELECTRIC_TRAP                                = 218926,
    GO_ANCIENT_GATE_1                               = 218935,
    GO_ANCIENT_GATE_2                               = 218936,
    GO_ANCIENT_GATE_3                               = 218937,
    GO_ANCIENT_GATE_4                               = 218938,
    GO_ANCIENT_GATE_5                               = 218940,
    GO_TREASURE_RESIDENT_CHEST                      = 218777,
    GO_TREASURE_THE_PEDISTAL_OF_THE_STATUE_MOGU     = 218779,
    GO_TREASURE_BARBED_VINE_1                       = 218820,
    GO_TREASURE_BARBED_VINE_2                       = 218821,
    GO_TREASURE_PEBBLE_1                            = 218829,
    GO_TREASURE_PEBBLE_2                            = 218830,
    GO_TREASURE_PEBBLE_3                            = 218831,
};

#endif
