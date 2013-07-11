#define DRAGON_SOUL_MAGIC   0x4452534F

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"

enum eData
{
    DATA_INTRO                  = 0,
    DATA_MORCHOK                = 1,
    DATA_WARLORD_ZONOZZ         = 2,
    DATA_YORSAHJ                = 3,
    DATA_HAGARA                 = 4,
    DATA_ULTRAXION              = 5,
    DATA_WARMASTER_BLACKHORN    = 6,
    DATA_SPINE_OF_DEATHWING     = 7,
    DATA_MADNESS_OF_DEATHWING   = 8,
    DATA_KOHCROM                = 9,
    MAX_ENCOUNTER               = 10,
};

enum eNpcs
{
    // Boss
    NPC_MORCHOK                     = 55265,
    NPC_MORCHOK_COPY                = 57773,
    NPC_WARLORD_ZONOZZ              = 55308,
    NPC_YORSAHJ                     = 55312,
    NPC_KOHCROM                     = 57773,

    // Friendly Mobs
    NPC_VALEERA                     = 57289,
    NPC_EIENDORMI                   = 57288,

    // Other mobs
    NPC_FROZEN_CRYSTAL              = 56136,
    NPC_ICE_WAVE                    = 56104,

    NPC_NETHESTRASZ                 = 57287, // teleport upstairs
    NPC_TRAVEL_TO_WYRMREST_TEMPLE   = 57328, //
    NPC_DASNURIMI                   = 58153, // trader
};

enum GameObjects
{
    GO_INNER_WALL = 209596,
};

enum SharedSpells
{
    SPELL_TELEPORT_VISUAL_ACTIVE    = 108203,
    SPELL_TELEPORT_VISUAL_DISABLED  = 108227,
};

const Position teleportPos[2] =
{
    {-1779.503662f, -2393.439941f, 45.61f, 3.20f},  // Wyrmrest Temple
    {-1854.233154f, -3068.658691f, -178.34f, 0.46f} // Yor'sahj The Unsleeping
};

enum eCurrencys
{
    CURRENCY_MOTE_OF_DARKNESS   = 614,
    CURRENCY_ECD                = 615,
};

void WhisperToAllPlayerInZone(int32 TextId, Creature* sender);
