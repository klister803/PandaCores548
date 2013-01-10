/*
    Dungeon : Gate of the Setting Sun 90-90
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
    NPC_KIPTILAK                = 56906,
    NPC_GADOK                   = 56589,
    NPC_RIMOK                   = 56636,
    NPC_RAIGONN                 = 56877,

    // Kip'Tilak
    NPC_STABLE_MUNITION         = 56917,

    NPC_EXPLOSION_BUNNY_N_M     = 56911,
    NPC_EXPLOSION_BUNNY_S_M     = 56918,
    NPC_EXPLOSION_BUNNY_E_M     = 56919,
    NPC_EXPLOSION_BUNNY_W_M     = 56920,

    NPC_EXPLOSION_BUNNY_N_P     = 59205,
    NPC_EXPLOSION_BUNNY_S_P     = 59206,
    NPC_EXPLOSION_BUNNY_E_P     = 59207,
    NPC_EXPLOSION_BUNNY_W_P     = 59208,

    // Gadok
    NPC_STALKER_NORTH_SOUTH     = 56932,
    NPC_STALKER_WEST_EAST       = 56913,

    NPC_KRIKTHIK_STRIKER        = 59778, // Appear on Gadok bombardment
    NPC_KRIKTHIK_DISRUPTOR      = 59794, // Bombard tower when Gadok is in fight
};

enum ObjectsIds
{
    GO_KIPTILAK_ENTRANCE_DOOR   = 212982,
    GO_KIPTILAK_WALLS           = 214629,
    GO_KIPTILAK_MANTID_BOMBS    = 215757,
    GO_KIPTILAK_EXIT_DOOR       = 212983
};

#endif // STORMSTOUT_BREWERY_H_
