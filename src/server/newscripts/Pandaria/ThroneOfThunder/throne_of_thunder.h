//UWoWCore
//Throne of Thunder

#ifndef THRONEOFTHUNDER
#define THRONEOFTHUNDER

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

enum eData
{
    DATA_JINROKH            = 1,
    DATA_HORRIDON           = 2,
    DATA_COUNCIL_OF_ELDERS  = 3,
    DATA_TORTOS             = 4,
    DATA_MEGAERA            = 5,
    DATA_JI_KUN             = 6,
    DATA_DURUMU             = 7,
    DATA_PRIMORDIUS         = 8,
    DATA_DARK_ANIMUS        = 9,
    DATA_IRON_QON           = 10,
    DATA_TWIN_CONSORTS      = 11,
    DATA_LEI_SHEN           = 12,
    DATA_RA_DEN             = 13,
};

enum eCreatures
{
    //Npc
    NPC_STORM_CALLER      = 70236,
    NPC_LIGHTNING_BALL    = 69232,

    //Bosses
    NPC_JINROKH           = 69465,
    NPC_HORRIDON          = 68476,
    //Council of Elders
    NPC_FROST_KING_MALAKK = 69131,
    NPC_PRINCESS_MARLI    = 69132,
    NPC_KAZRAJIN          = 69134,
    NPC_SUL_SANDCRAWLER   = 69078,
    //
    NPC_TORTOS            = 67977,
    //Megaera
    NPC_FLAMING_HEAD      = 70212,
    NPC_FROZEN_HEAD       = 70235,
    NPC_VENOMOUS_HEAD     = 70247,
    //
    NPC_JI_KUN            = 69712,
    NPC_DURUMU            = 68036,
    NPC_PRIMORDIUS        = 69017,
    NPC_DARK_ANIMUS       = 69427,
    NPC_IRON_QON          = 68078,
    //Twin consorts
    NPC_SULIN             = 68904,
    NPC_LULIN             = 68905,
    //
    NPC_LEI_SHEN          = 68397,
    NPC_RA_DEN            = 69473,
};

enum eGameObjects
{  
    GO_JINROKH_PRE_DOOR   = 218665,
    GO_JINROKH_ENT_DOOR   = 218664,
    GO_MOGU_SR            = 218675,
    GO_MOGU_NR            = 218676,
    GO_MOGU_NL            = 218677,
    GO_MOGU_SL            = 218678,
    GO_JINROKH_EX_DOOR    = 218663,
};
//This is triggers entry(maybe needed, but not need spawn)
//69593
//69509
//69676
//69609
//69469 - Condictive water
#endif THRONEOFTHUNDER
