/*
    Dungeon : Mogushan Palace 88-90
    Instance General Script
*/

#ifndef MOGUSHAN_VAULT_H_
#define MOGUSHAN_VAULT_H_

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

enum eData
{
    DATA_STONE_GUARD                = 0,
    DATA_FENG                       = 1,
    DATA_GARAJAL                    = 2,
    DATA_SPIRIT_KINGS               = 4,
    DATA_ELEGON                     = 5,
    DATA_WILL_OF_EMPEROR            = 6,
    DATA_MAX_BOSS_DATA              = 7
};

enum eActions
{
    ACTION_ENTER_COMBAT             = 1,
    ACTION_GUARDIAN_DIED            = 2,
    ACTION_PETRIFICATION            = 3,
    ACTION_FAIL                     = 4,
};

enum eCreatures
{
    // The Stone Guard
    NPC_STONE_GUARD_CONTROLER       = 60089,
    NPC_JASPER                      = 59915,
    NPC_JADE                        = 60043,
    NPC_AMETHYST                    = 60047,
    NPC_COBALT                      = 60051,

    NPC_FENG                        = 60009,

    NPC_GARAJAL                     = 60143,

    // Spirit kings
    NPC_ZIAN                        = 60701,
    NPC_MENG                        = 60708,
    NPC_QIANG                       = 60709,
    NPC_SUBETAI                     = 60710,

    NPC_ELEGON                      = 60410,
    
    NPC_QANXI                       = 60399,
    NPC_JANXI                       = 60400
};

enum eGameObjects
{
    GOB_STONE_GUARD_DOOR_ENTRANCE   = 214497,
    GOB_STONE_GUARD_DOOR_EXIT       = 214526,
    GOB_FENG_DOOR_FENCE             = 214452, // Both inner entrance and exit
    GOB_FENG_DOOR_EXIT              = 214696,

    GOB_SPEAR_STATUE                = 213245,
    GOB_FIST_STATUE                 = 213246,
    GOB_SHIELD_STATUE               = 213247,
    GOB_STAFF_STATUE                = 213248,

};

#endif // MOGUSHAN_VAULT_H_
