/*
    Dungeon : Shandopan Monastery 87-89
    Instance General Script
*/

#ifndef SHADOPAN_MONASTERY_H_
#define SHADOPAN_MONASTERY_H_

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"

uint32 const EncounterCount = 4;

enum DataTypes
{
    DATA_GU_CLOUDSTRIKE         = 1,
    DATA_MASTER_SNOWDRIFT       = 2,
    DATA_SHA_VIOLENCE           = 3,
    DATA_TARAN_ZHU              = 4,

    MAX_DATA
};

enum CreaturesIds
{
    NPC_GU_CLOUDSTRIKE          = 56747,
    NPC_MASTER_SNOWDRIFT        = 56541,
    NPC_SHA_VIOLENCE            = 56719,
    NPC_TARAN_ZHU               = 56884
};

enum ObjectsIds
{
    GO_CLOUDSTRIKE_ENTRANCE     = 210863,
    GO_CLOUDSTRIKE_EXIT         = 210864,

    GO_SNOWDRIFT_ENTRANCE       = 213194,
    GO_SNOWDRIFT_FIRE_WALL      = 212908,
    GO_SNOWDRIFT_DOJO_DOOR      = 210800,
    GO_SNOWDRIFT_EXIT           = 210862,

    GO_SHA_ENTRANCE             = 210867,
    GO_SHA_EXIT                 = 210866,

};

#endif // SHADOPAN_MONASTERY_H_
