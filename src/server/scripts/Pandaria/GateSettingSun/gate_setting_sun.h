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

enum DataTypes
{
    DATA_KIPTILAK   = 1,
    DATA_GADOK      = 2,
    DATA_RIMOK      = 3,
    DATA_RAIGONN    = 4
};

enum CreaturesIds
{
    NPC_KIPTILAK    = 56906,
    NPC_GADOK       = 56589,
    NPC_RIMOK       = 56636,
    NPC_RAIGONN     = 56877,

    NPC_STABLE_MUNITION     = 56917,

    NPC_EXPLOSION_BUNNY_N   = 56911,
    NPC_EXPLOSION_BUNNY_S   = 56918,
    NPC_EXPLOSION_BUNNY_E   = 56919,
    NPC_EXPLOSION_BUNNY_W   = 56920,
};

#endif // STORMSTOUT_BREWERY_H_
