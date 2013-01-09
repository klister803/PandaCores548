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

uint32 const EncounterCount = 4;

enum DataTypes
{
    DATA_IN_FIGHT           = 0,

    DATA_KIPTILAK           = 1,
    DATA_GADOK              = 2,
    DATA_RIMOK              = 3,
    DATA_RAIGONN            = 4,

    DATA_OPEN_FIRST_DOOR    = 5,
    MAX_DATA
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

enum ObjectsIds
{
    GO_KIPTILAK_ENTRANCE_DOOR   = 212982,
    GO_KIPTILAK_WALLS           = 214629,
    GO_KIPTILAK_EXIT_DOOR       = 212983
};

#endif // STORMSTOUT_BREWERY_H_
