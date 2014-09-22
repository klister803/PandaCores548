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
    // Encounter States/Boss GUIDs
    DATA_IMMERSEUS                  = 1,
    DATA_F_PROTECTORS               = 2,
    DATA_NORUSHEN                   = 3,
    DATA_SHA_OF_PRIDE               = 4,
    DATA_GALAKRAS                   = 5,
    DATA_IRON_JUGGERNAUT            = 6,
    DATA_KORKRON_D_SHAMAN           = 7,
    DATA_GENERAL_NAZGRIM            = 8,
    DATA_MALKOROK                   = 9,
    DATA_SPOLLS_OF_PANDARIA         = 10,
    DATA_THOK                       = 11,
    DATA_BLACKFUSE                  = 12,
    DATA_KLAXXI                     = 13,
    DATA_GARROSH                    = 14,
    DATA_MAX,

    // Additional data
    DATA_IMMERSEUS_INTRO,
};

enum eCreatures
{
    //Npc or summons
    NPC_LOREWALKER_CHO              = 73330,
    NPC_CONTAMINATED_PUDDLE         = 71604,
    NPC_AQUEOUS_DEFENDER            = 73191,
    NPC_PUDDLE_POINT                = 90000,
    NPC_SHA_POOL                    = 71544,
    NPC_SHA_PUDDLE                  = 71603,
    NPC_B_H_CONTROLLER              = 90008,
    NPC_BLIND_HATRED                = 72565,

    NPC_IMMERSEUS                   = 71543,
    //Fallen Protectors
    NPC_ROOK_STONETOE               = 71475,
    NPC_SUN_TENDERHEART             = 71480,
    NPC_HE_SOFTFOOT                 = 71479,
    NPC_GOLD_LOTOS                  = 71686, 
    NPC_GOLD_LOTOS_MAIN       = 71683,
    NPC_GOLD_LOTOS_MOVER            = 71695,
    NPC_EMBODIED_DESPIRE            = 71474,
    NPC_EMBODIED_MISERY             = 71476,
    NPC_EMBODIED_GLOOM              = 71477,
    NPC_EMBODIED_ANGUISH            = 71478,
    NPC_EMBODIED_SORROW             = 71481,
    NPC_EMBODIED_DESPERATION        = 71482,
    //  
    NPC_NORUSHEN                    = 71967,
    NPC_AMALGAM_OF_CORRUPTION       = 72276,
    NPC_ESSENCE_OF_CORRUPTION       = 71976,
    NPC_MANIFESTATION_OF_CORRUPTION = 72264,
    NPC_TITANIC_CORRUPTION          = 72051,
    NPC_GREATER_CORRUPTION          = 72001,
    //
    NPC_SHA_OF_PRIDE                = 71734,
    NPC_GALAKRAS                    = 72249,
    NPC_IRON_JUGGERNAUT             = 71466,
    NPC_KORKRON_D_SHAMAN            = 71859,
    NPC_GENERAL_NAZGRIM             = 71515,
    NPC_MALKOROK                    = 71454,
    NPC_THOK                        = 71529,
    NPC_BLACKFUSE                   = 71504,
    //Paragons of the Klaxxi
    NPC_KILRUK                      = 71161,
    NPC_XARIL                       = 71157,
    NPC_KAZTIK                      = 71156,
    NPC_KORVEN                      = 71155,
    NPC_IYYOKYK                     = 71160,
    NPC_KAROZ                       = 71154,
    NPC_SKEER                       = 71152,
    NPC_RIKKAL                      = 71158,
    NPC_HISEK                       = 71153,
    //
    NPC_GARROSH                     = 71865,
};

enum eGameObjects
{   
    //Immerseus
    GO_IMMERSEUS_EX_DOOR            = 221663,

    //Norushen
    GO_LIGTH_QUARANTINE             = 223142,
    GO_LIGTH_QUARANTINE_2           = 223143,
    GO_LIGTH_QUARANTINE_3           = 223144,
    GO_LIGTH_QUARANTINE_4           = 223145,
    GO_LIGTH_QUARANTINE_5           = 223146,
    GO_LIGTH_QUARANTINE_6           = 223147,
    GO_NORUSHEN_EX_DOOR             = 221447,

    //Sha of Pride
    GO_SHA_OF_PRIDE_ENT_DOOR        = 221446,
};

#endif SIEGEOFORGRIMMAR
