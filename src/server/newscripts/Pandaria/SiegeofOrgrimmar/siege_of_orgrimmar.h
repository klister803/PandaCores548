//UWoWCore
//Siege of Orgrimmar

#ifndef SIEGEOFORGRIMMAR
#define SIEGEOFORGRIMMAR

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

enum eData
{
    DATA_IMMEREUS           = 1,
    DATA_F_PROTECTORS       = 2,
    DATA_NORUSHEN           = 3,
    DATA_SHA_OF_PRIDE       = 4,
    DATA_GALAKRAS           = 5,
    DATA_IRON_JUGGERNAUT    = 6,
    DATA_KORKRON_D_SHAMAN   = 7,
    DATA_GENERAL_NAZGRIM    = 8,
    DATA_MALKOROK           = 9,
    DATA_SPOLLS_OF_PANDARIA = 10,
    DATA_THOK               = 11,
    DATA_BLACKFUSE          = 12,
    DATA_KLAXXI             = 13,
    DATA_GARROSH            = 14,
};

enum eCreatures
{
    NPC_IMMERSEUS           = 71543,
    //Fallen Protectors
    NPC_ROOK_STONETOE       = 71475,
    NPC_SUN_TENDERHEART     = 71480,
    NPC_HE_SOFTFOOT         = 71479,
    //  
    NPC_NORUSHEN            = 71965,
    NPC_SHA_OF_PRIDE        = 71734,
    NPC_GALAKRAS            = 72249,
    NPC_IRON_JUGGERNAUT     = 71466,
    NPC_KORKRON_D_SHAMAN    = 71859,
    NPC_GENERAL_NAZGRIM     = 71515,
    NPC_MALKOROK            = 71454,
    NPC_THOK                = 71529,
    NPC_BLACKFUSE           = 71504,
    //Paragons of the Klaxxi
    NPC_KILRUK              = 71161,
    NPC_XARIL               = 71157,
    NPC_KAZTIK              = 71156,
    NPC_KORVEN              = 71155,
    NPC_IYYOKYK             = 71160,
    NPC_KAROZ               = 71154,
    NPC_SKEER               = 71152,
    NPC_RIKKAL              = 71158,
    NPC_HISEK               = 71153,
    //
    NPC_GARROSH             = 71865,
};

enum eGameObjects
{   
};

#endif SIEGEOFORGRIMMAR
