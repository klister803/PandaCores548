#ifndef DEF_THRONEOFTHEFOURWINDS_H
#define DEF_THRONEOFTHEFOURWINDS_H

enum Data
{
    DATA_CONCLAVE_OF_WIND,
    DATA_ALAKIR,
    DATA_MAX_BOSSES,

    DATA_IS_CONCLAVE_DONE,
    DATA_CONCLAVE_RESET,
    DATA_CONCLAVE_DONE,
    DATA_HEAL_DONE,
};

enum CreatureIds
{
    BOSS_ANSHAL                     = 45870,
    BOSS_NEZIR                      = 45871,
    BOSS_ROHASH                     = 45872,

    BOSS_ALAKIR                     = 46753,

    // Conclave of Wind
    NPC_SOOTHING_BREEZE             = 46246,
    NPC_RAVENOUS_CREEPER            = 45812,
    NPC_RAVENOUS_CREEPER_TRIGGER    = 45813,
    NPC_ICE_PATCH                   = 46186,
    NPC_CYCLONE                     = 46419,
    NPC_SPLIP_STREAM                = 47066,
};

enum InstanceSpells
{
    SPELL_PRE_COMBAT_EFFECT_ANSHAL  = 85537,
    SPELL_PRE_COMBAT_EFFECT_NEZIR   = 85532,
    SPELL_PRE_COMBAT_EFFECT_ROHASH  = 85538,
};

enum ObjectIds
{
    GO_ALAKIR_PLATFORM                  = 207737,
    GO_HEART_OF_THE_WIND				= 207891,
    GO_HEART_OF_THE_WIND_H              = 207892,

    GOB_WIND_DRAFTEFFECT_CENTER         = 207922,

    GOB_WIND_DRAFTEFFECT_1              = 207923,
    GOB_WIND_DRAFTEFFECT_2              = 207924,
    GOB_WIND_DRAFTEFFECT_3              = 207925,
    GOB_WIND_DRAFTEFFECT_4              = 207926,
    GOB_WIND_DRAFTEFFECT_5              = 207929,
    GOB_WIND_DRAFTEFFECT_6              = 207930,
    GOB_WIND_DRAFTEFFECT_7              = 207927,
    GOB_WIND_DRAFTEFFECT_8              = 207928,

    GOB_DEIJING_HEALING                 = 206699,
    GOB_DEIJING_FROST                   = 206700,
    GOB_DEIJING_TORNADO                 = 206701
};

enum areatrig
{
    AREA_TO4W = 5638,
};

enum InstanceSpell
{
    SPELL_TFW_SERENITY                  = 89750,
    SPELL_ALAKIR_SLIPSTREAM             = 87713,
};

#endif