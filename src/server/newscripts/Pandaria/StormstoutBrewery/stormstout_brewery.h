/*
    Dungeon : Stormstout Brewery 85-87
    Instance General Script
*/

#ifndef STORMSTOUT_BREWERY_H_
#define STORMSTOUT_BREWERY_H_

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define SBScriptName "instance_stormstout_brewery"

enum DataTypes
{
    DATA_OOK_OOK   = 0,
    DATA_HOPTALLUS = 1,
    DATA_YAN_ZHU   = 2
};

enum CreaturesIds
{
    NPC_OOK_OOK     = 56637,
    NPC_HOPTALLUS   = 56717,
    NPC_YAN_ZHU     = 59479
};

enum GameObjectIds
{
    GO_EXIT_OOK_OOK  = 211132,
    GO_DOOR          = 211134,
    GO_DOOR2         = 211133,
    GO_DOOR3         = 211135,
    GO_DOOR4         = 211137,
    GO_LAST_DOOR     = 211136,
    GO_CARROT_DOOR   = 211126
};

#endif // STORMSTOUT_BREWERY_H_
