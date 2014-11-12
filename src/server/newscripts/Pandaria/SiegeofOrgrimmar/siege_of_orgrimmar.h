//UWoWCore
//Siege of Orgrimmar

#ifndef SIEGEOFORGRIMMAR
#define SIEGEOFORGRIMMAR

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

enum globalSpells
{
    SPELL_SHA_VORTEX                = 146024,
    SPELL_TP_ORGRIMMAR_1            = 148032, //Horde
    SPELL_TP_ORGRIMMAR_2            = 148034, //Aliance
};

enum eData
{
    // Encounter States/Boss GUIDs
    DATA_IMMERSEUS                  = 0,
    DATA_F_PROTECTORS               = 1,
    DATA_NORUSHEN                   = 2,
    DATA_SHA_OF_PRIDE               = 3,
    DATA_GALAKRAS                   = 4,
    DATA_IRON_JUGGERNAUT            = 5,
    DATA_KORKRON_D_SHAMAN           = 6,
    DATA_GENERAL_NAZGRIM            = 7,
    DATA_MALKOROK                   = 8,
    DATA_SPOLLS_OF_PANDARIA         = 9,
    DATA_THOK                       = 10,
    DATA_BLACKFUSE                  = 11,
    DATA_KLAXXI                     = 12,
    DATA_GARROSH                    = 13,
    DATA_MAX,

    DATA_FIELD_OF_SHA,
    DATA_FP_EVADE,
    DATA_SHA_PRE_EVENT,

    // Additional data
    DATA_IMMERSEUS_INTRO,
};

enum eCreatures
{
    //Npc or summons
    NPC_LOREWALKER_CHO              = 73330,
    NPC_LOREWALKER_CHO2             = 72872,
    NPC_LOREWALKER_CHO3             = 61348,
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
    NPC_GOLD_LOTOS_MAIN             = 71683,
    NPC_GOLD_LOTOS_MOVER            = 71695,
    NPC_GOLD_LOTOS_HE               = 71711,
    NPC_GOLD_LOTOS_SUN              = 71684,
    NPC_GOLD_LOTOS_ROOK             = 71686,

    NPC_EMBODIED_ANGUISH_OF_HE      = 71478,
    NPC_EMBODIED_DESPERATION_OF_SUN = 71482,
    NPC_EMBODIED_DESPIRE_OF_SUN     = 71474,
    NPC_EMBODIED_MISERY_OF_ROOK     = 71476,
    NPC_EMBODIED_GLOOM_OF_ROOK      = 71477,
    NPC_EMBODIED_SORROW_OF_ROOK     = 71481,

    //Field of Sha
    NPC_ZEAL                        = 72661, //Zeal
    NPC_ARROGANCE                   = 72663, //Arrogance
    NPC_VANITY                      = 72662, //Vanity

    //Sha of Pride
    NPC_SHA_OF_PRIDE                = 71734,
    NPC_LADY_JAINA_PROUDMORORE      = 73598, //Lady Jaina Proudmoore
    NPC_SHA_TARAN_ZHU               = 72779, //Taran Zhu <Lord of the Shado-Pan>
    NPC_SHA_NORUSHEN                = 71965,
    NPC_REFLECTION                  = 72172, //Reflection
    NPC_LINGERING_CORRUPTION        = 72791, //Lingering Corruption
    NPC_MANIFEST_OF_PRIDE           = 71946, //Manifestation of Pride
    NPC_SLG_GENERIC_MOP             = 68553, //SLG Generic MoP
    NPC_RIFT_OF_CORRUPTION          = 72846, //Rift of Corruption
    NPC_SHA_OF_PRIDE_END_LADY_JAINA = 73598,
    NPC_SHA_OF_PRIDE_END_THERON     = 73605,
    NPC_PORTAL_TO_ORGRIMMAR         = 73536,
    //controller 68553?? Sha Vortex triger 146034

    //  Norushen
    NPC_NORUSHEN                        = 71967,
    NPC_AMALGAM_OF_CORRUPTION           = 72276,
    NPC_ESSENCE_OF_CORRUPTION_C         = 71976,
    NPC_ESSENCE_OF_CORRUPTION           = 72263, //released
    NPC_MANIFESTATION_OF_CORRUPTION_C   = 71977,
    NPC_MANIFESTATION_OF_CORRUPTION     = 72264, //FRAYED & release
    NPC_TITANIC_CORRUPTION              = 72051,
    NPC_GREATER_CORRUPTION              = 72001,
    NPC_PURIFYING_LIGHT                 = 72065,
    NPC_RESIDUAL_CORRUPTION             = 72550, // by spell 145522
    NPC_NN_HEAL_EVENT_PROTECTOR_1       = 71996,
    NPC_NN_HEAL_EVENT_PROTECTOR_2       = 72000,
    NPC_NN_HEAL_EVENT_PROTECTOR_3       = 71995,
    //
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
    GO_SHIP_ALLIANCE                = 223459,
    GO_SHIP_HORDE                   = 223464,

    //Immerseus
    GO_IMMERSEUS_EX_DOOR            = 221663,

    //Fallen Protectors
    GO_SHA_FIELD                    = 221611,

    //Field of Sha
    GO_SHA_ENERGY_WALL              = 221602,

    //Sha
    GO_VAULT_OF_FORBIDDEN_TREASURES_1 = 221739, //Vault of Forbidden Treasures
    GO_VAULT_OF_FORBIDDEN_TREASURES_2 = 221741,
    GO_VAULT_OF_FORBIDDEN_TREASURES_3 = 221742, //25Heroic
    GO_VAULT_OF_FORBIDDEN_TREASURES_4 = 221740,
    GO_VAULT_OF_FORBIDDEN_TREASURES_5 = 233029, //Mythic
    GO_VAULT_OF_FORBIDDEN_TREASURES_6 = 232163,
    GO_VAULT_OF_FORBIDDEN_TREASURES_7 = 232164,
    
    GO_CORRUPTED_PRISON_WEST        = 221678,
    GO_CORRUPTED_BUTTON_WEST_1      = 221753,
    GO_CORRUPTED_BUTTON_WEST_2      = 221751,
    GO_CORRUPTED_BUTTON_WEST_3      = 221752,
    GO_CORRUPTED_PRISON_EAST        = 221676,
    GO_CORRUPTED_BUTTON_EAST_1      = 221758,
    GO_CORRUPTED_BUTTON_EAST_2      = 221759,
    GO_CORRUPTED_BUTTON_EAST_3      = 221757,
    GO_CORRUPTED_PRISON_NORTH       = 221677,
    GO_CORRUPTED_BUTTON_NORTH_1     = 221755,
    GO_CORRUPTED_BUTTON_NORTH_2     = 221750,
    GO_CORRUPTED_BUTTON_NORTH_3     = 221754,
    GO_CORRUPTED_PRISON_SOUTH       = 221679,
    GO_CORRUPTED_BUTTON_SOUTH_1     = 221761,
    GO_CORRUPTED_BUTTON_SOUTH_2     = 221760,
    GO_CORRUPTED_BUTTON_SOUTH_3     = 221756,

    //Norushen
    GO_LIGTH_QUARANTINE             = 223142,
    GO_LIGTH_QUARANTINE_2           = 223143,
    GO_LIGTH_QUARANTINE_3           = 223144,
    GO_LIGTH_QUARANTINE_4           = 223145,
    GO_LIGTH_QUARANTINE_5           = 223146,
    GO_LIGTH_QUARANTINE_6           = 223147,
    GO_LIGHT_RAY_01                 = 223192,
    GO_LIGHT_RAY_02                 = 223020,
    GO_LIGHT_RAY_03                 = 223019,
    GO_LIGHT_RAY_04                 = 223018,
    GO_LIGHT_RAY_05                 = 223017,
    GO_LIGHT_RAY_06                 = 223016,
    GO_LIGHT_RAY_07                 = 223015,
    GO_LIGHT_RAY_08                 = 223014,
    GO_LIGHT_RAY_09                 = 223013,
    GO_LIGHT_RAY_10                 = 223012,
    GO_LIGHT_RAY_11                 = 223011,
    GO_LIGHT_RAY_12                 = 223010,
    GO_LIGHT_RAY_13                 = 223009,
    GO_LIGHT_RAY_14                 = 223008,
    GO_LIGHT_RAY_15                 = 223007,
    GO_LIGHT_RAY_16                 = 223021,

    GO_NORUSHEN_EX_DOOR             = 221447,

    //Sha of Pride
    GO_SHA_OF_PRIDE_ENT_DOOR        = 221446,
};

enum miscData
{
    TRANSPORT_PERIOD                = 98995,
};

static uint8 vehSlotForMeasures(uint32 entry)
{
    switch(entry)
    {
        case NPC_EMBODIED_DESPERATION_OF_SUN:
        case NPC_EMBODIED_MISERY_OF_ROOK:
            return 1;
        case NPC_EMBODIED_GLOOM_OF_ROOK:
            return 2;
    }
    return 0;
}
#endif SIEGEOFORGRIMMAR
