#ifndef DEF_FIRELANDS_H
#define DEF_FIRELANDS_H

#define FLScriptName "instance_firelands"

enum Data
{
    DATA_INTRO                  = 0,
    DATA_SHANNOX                = 1,
    DATA_RHYOLITH               = 2,
    DATA_BETHTILAC              = 3,
    DATA_ALYSRAZOR              = 4,
    DATA_BALEROC                = 5,
    DATA_STAGHELM               = 6,
    DATA_RAGNAROS               = 7,
    DATA_RIPLIMB                = 8,
    DATA_RAGEFACE               = 9,
    DATA_Crystal_Shard          = 10,
    DATA_RAGNAROS_FLOOR         = 11,
    DATA_RAGNAROS_CACHE_10      = 12,
    DATA_RAGNAROS_CACHE_25      = 13,
    DATA_RHYOLITH_HEALTH_SHARED = 14,
    DATA_EVENT                  = 15,
};

enum CreatureIds
{
    NPC_SHANNOX     = 53691,
    NPC_RAGEFACE    = 53695, 
    NPC_RIPLIMB     = 53694,
    NPC_RHYOLITH    = 52558,
    NPC_BETHTILAC   = 52498,
    NPC_ALYSRAZOR   = 52530, 
    NPC_BALEROC     = 53494,
    NPC_STAGHELM    = 52571,
    NPC_RAGNAROS    = 52409,

    // Ragnaros
    NPC_ENGULFING_FLAMES_TRIGGER    = 53485,
    NPC_SPLITTING_BLOW_TRIGGER      = 53393,
    NPC_MAGMA_POOL_TRIGGER          = 53729,
    NPC_PLATFORM_TRIGGER            = 53952,
    NPC_SULFURAS_HAMMER             = 53420,
    NPC_SULFURAS_FLAME_WALL         = 38327,
    NPC_SULFURAS_SMASH_TARGET_1     = 53268,
    NPC_MOLTEN_SEED                 = 53186,
    // Baleroc
    BOSS_BALEROC                    = 53494,
    NPC_Crystal_Shard               = 53495,
    // Alyrazor
    NPC_BLAZING_MONSTROSITY_LEFT = 53786,
    NPC_BLAZING_MONSTROSITY_RIGHT = 53791,
    NPC_EGG_PILE = 53795,
    NPC_HARBINGER_OF_FLAME = 53793,
    NPC_MOLTEN_EGG_TRASH = 53914,
    NPC_SMOULDERING_HATCHLING = 53794,
    NPC_MOLTEN_ELEMENTAR    = 53189,

    NPC_CIRCLE_OF_THRONES_PORTAL    = 54247,
};

enum GameobjectIds
{
    GO_CIRCLE_OF_THORNS_PORTAL  = 209137,
    GO_CIRCLE_OF_THORNS_PORTAL2 = 209346,
    GO_CIRCLE_OF_THORNS_PORTAL3 = 209098,

    GO_BRIDGE_OF_RHYOLITH       = 209255,
    GO_FIRE_WALL_BALEROC        = 209066,
    GO_RAID_BRIDGE_FORMING      = 209277,
    GO_RAGNAROS_FLOOR           = 208835,
    GO_STICKY_WEB               = 208877,
    GO_MOLTEN_METEOR            = 208966,
    GO_FIRE_WALL_FENDRAL        = 208906,
    GO_CACHE_OF_THE_FIRELORD    = 208967,
    GO_CACHE_OF_THE_FIRELORD_H  = 209261,
    GO_FIRE_WALL_FANDRAL_1      = 208906,
    GO_FIRE_WALL_FANDRAL_2      = 208873,
    GO_SULFURON_KEEP            = 209073,
    GO_CACHE_OF_THE_FIRELORD_10 = 208967,
    GO_CACHE_OF_THE_FIRELORD_25 = 208968,
    GO_CACHE_OF_THE_FIRELORD_10h = 208969,
    GO_CACHE_OF_THE_FIRELORD_25h = 209261,
};

enum MovePoints
{
    POINT_HAMMER,
    POINT_RAGNAROS_DOWN,         //end of each phase
    POINT_RAGNAROS_UP,           //start of each next phase
    POINT_RAGNAROS_STANDUP,      //only on heroic mode
    POINT_SULFURAS_SMASH,        //target for smashes
};

enum QuestDefines
{
    // quest
    GO_BRANCH_OF_NORDRASSIL                     = 209100,

    // Cannot find rhyolith fragment, summon it manualy
    SPELL_CREATE_EMBERSTONE_FRAGMENT            = 100518,
    SPELL_CREATE_CHITINOUS_FRAGMENT             = 100520,
    SPELL_CREATE_PYRESHELL_FRAGMENT             = 100519,

    SPELL_CHARGED_RHYOLITH_FOCUS                = 100481,
    SPELL_CHARGED_EMBERSTONE_FOCUS              = 100499,
    SPELL_CHARGED_CHITINOUS_FOCUS               = 100501,
    SPELL_CHARGED_PYRESHELL_FOCUS               = 100500,

    SPELL_TRANSFORM_CHARGED_RHYOLITH_FOCUS      = 100477,
    SPELL_TRANSFORM_CHARGED_EMBERSTONE_FOCUS    = 100496,
    SPELL_TRANSFORM_CHARGED_CHITINOUS_FOCUS     = 100498,
    SPELL_TRANSFORM_CHARGED_PYRESHELL_FOCUS     = 100497,

    GO_RHYOLITH_FRAGMENT                        = 209033,
    GO_EMBERSTONE_FRAGMENT                      = 209035,
    GO_PYRESHELL_FRAGMENT                       = 209036,
    GO_OBSIDIAN_FLECKED_CHITIN                  = 209037,

    NPC_DULL_RHYOLITH_FOCUS                     = 53951,
    NPC_DULL_EMBERSTONE_FOCUS                   = 53968,
    NPC_DULL_CHITINOUS_FOCUS                    = 53970,
    NPC_DULL_PYRESHELL_FOCUS                    = 53963,

    NPC_CHARGED_RHYOLITH_FOCUS                  = 53955,
    NPC_CHARGED_EMBERSTONE_FOCUS                = 53969,
    NPC_CHARGED_CHITINOUS_FOCUS                 = 53971,
    NPC_CHARGED_PYRESHELL_FOCUS                 = 53967,

    EVENT_PORTALS                               = 28888,

    SPELL_LEGENDARY_PORTAL_OPENING              = 101029,
    SPELL_BRANCH_OF_NORDRASSIL_WIN_COSMETIC     = 100326,
    SPELL_SMOLDERING_AURA                       = 101093,
    SPELL_SIPHON_ESSENCE_CREDIT                 = 101149,

    QUEST_HEART_OF_FLAME_ALLIANCE               = 29307,
    QUEST_HEART_OF_FLAME_HORDE                  = 29308,
};

static void AddSmoulderingAura(Creature* pCreature)
{
    Map::PlayerList const &PlayerList = pCreature->GetMap()->GetPlayers();
    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player* pPlayer = i->getSource())
                if (pPlayer->GetQuestStatus(QUEST_HEART_OF_FLAME_ALLIANCE) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_HEART_OF_FLAME_HORDE) == QUEST_STATUS_INCOMPLETE)
                {
                    pCreature->CastSpell(pCreature, SPELL_SMOLDERING_AURA, true);
                    break;
                }
}

#endif
