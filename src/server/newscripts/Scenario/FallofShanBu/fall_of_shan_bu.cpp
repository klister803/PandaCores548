/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "fall_of_shan_bu.h"
#include "ScriptedCreature.h"

enum Texts
{
    TEXT_WRATHION_START = 15618,
};

enum Spells
{
    //< player self
    SPELL_ATTACKED_PLAYER                       = 138952,
    SPELL_POWER_SURGE_TRIGGER                   = 140068,

    //< NPC_SHANZE_ELECTRO_CUTIONER2
    SPELL_LIGHTING_CHARGE                       = 136495,

    //< NPC_SHADO_PAN_DEFENDER
    SPELL_TAUNT                                 = 138937,
    SPELL_WOUNDED                               = 138919,
    SPELL_JOIN_PLAYER_PARTY                     = 139282,
    SPELL_PERIODIC_TAUNT_TARGETTING             = 139245,
    SPELL_HEALING_ORB                           = 139249, //< AT
    SPELL_HEALING_ORB_TRIGGER                   = 132744,
    SPELL_LEAVE_PLAYER_PARTY                    = 141057,

    //< NPC_SHANZE_WARRIOR
    SPELL_LUMBERING_CHARGE3                     = 120051,
    SPELL_LUMBERING_CHARGE2                     = 120052,
    SPELL_LUMBERING_CHARGE                      = 142587, //< AT
    SPELL_CLEAVE                                = 59992,

    //< NPC_INVISIBLE_STALKER
    SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1  = 140334,

    //< NPC_LIGHTING_PILAR_BEAM_STALKER
    SPELL_THUNDER_FORGE_CHARGE                  = 140382,
    SPELL_COSMETIC_LIGHTING_PILLAR_BEAM         = 138090,

    //< NPC_SHANZE_BATTLEMASTER
    SPELL_STORM_STRIKE                          = 138712,
    SPELL_STORM_STRIKE2                         = 32175,
    SPELL_STORM_STRIKE_OFF_HAND                 = 32175,
    SPELL_FIXATE_BEAM_COSMETIC                  = 142477,
    SPELL_FIXATE                                = 139034,
    SPELL_LIGHTING_WHIRLWIND                    = 138198,
    SPELL_LIGHTING_WHIRLWIND2                   = 138191,
    SPELL_LIGHTING_WHIRLWIND3                   = 138190,

    //< NPC_THUNDER_FORGE_2
    SPELL_GROUNDED                              = 139252,
    SPELL_LIGHTING_CHASE                        = 138721,
    SPELL_LIGHTING_STRIKE_TRIGGERED_BY_CHASE    = 138720,

    //< NPC_FORGEMASTER_VULKON
    SPELL_LIGHTING_STRIKE_2                     = 138826,
    SPELL_FACE_PLAYER                           = 139359,
    SPELL_FORGEMASTER_SPAWN_COSMETIC            = 142594,
    SPELL_ELECTRIFIED                           = 138821,
    SPELL_DISCHARGE                             = 138820,
    SPELL_LIGHTING_BOLT                         = 15801,
    SPELL_LIGHTING_STRIKE_TARGETTING            = 139302,
    SPELL_THUNDER_SMASH_DUMMY                   = 139357,
    SPELL_LIGHTING_SMASH                        = 138831,
    SPELL_LIGHTING_SMASH_TRIGGER                = 138832,

    //< NPC_SHA_FIEND
    SPELL_SMALL_SHA_FIXATE                      = 138692,
    SPELL_DARK_SUNDER                           = 138677,
    SPELL_SHA_BLAST                             = 138681,

    //< NPC_CELESTIAL_BLACKSMITH
    SPELL_THUNDER_FORGE_CHARGING                = 140487,
    SPELL_FORGING                               = 138869,
    SPELL_STRIKE_ANVIL_COSMETIC                 = 138875,
    SPELL_THUNDER_FORGE_CHARGE_TRIGGER          = 140489,
    SPELL_ACTIVATE_CLOSEST_AVNIL                = 140343,

    //< NPC_CELESTIAL_DEFENDER
    SPELL_COSMIC_SLASH                          = 138232,
    SPELL_COSMIC_SLASH_TRIGGER                  = 138229,
    SPELL_STAR_SLAM                             = 138935,
    SPELL_CELESTIAL_ROAR                        = 138624,
    SPELL_ASTRAL_ENDURANCE                      = 139127,
    SPELL_DAMAGE_SELF_50_PERCENT                = 136890,
    SPELL_POWER_SURGE                           = 140067, //< AT
    SPELL_CELESTIAL_STORM                       = 138634,
    SPELL_CELESTIAL_STORM_TRIGGER               = 138637,
    SPELL_SUMMON_CONSTELATIONS                  = 138940,
    SPELL_SUMMON_CONSTELATIONS_TRIGGER          = 138723,
    SPELL_ASTRAL_DEFENCE                        = 138630,
    SPELL_DEFENDER_HIGH_HP_COSMETIC             = 140337,
    SPELL_CELESTIAL_RESTORATION                 = 140065,

    //< NPC_SHA_BEAST
    SPELL_EMPOWERED                             = 138947,
    SPELL_DARK_BITE                             = 138956,
    SPELL_ABSORB_EVIL                           = 138950, //< AURA
    SPELL_LETHARGY                              = 138949,

    //< NPC_ANVIL_STALKER
    SPELL_THUNDER_SURGE                         = 138834,
    SPELL_ANVIL_ACTIVATE_COSMETIC_DND           = 140134,
    SPELL_LIGHTING_STRIKE_COSMETIC              = 140101,

    //< NPC_PHASE3_ROOM_CENTER_STALKER
    SPELL_DEACTIVATE_ALL_AVNILS_TRIGGER         = 140545, //< SS
    SPELL_FIND_AVNIL_STALKER_BEST_DUMMY         = 140140, //< SS
    SPELL_DEACTIVATE_ALL_AVNILS                 = 140350,
    SPELL_ACTIVATE_ALL_AVNILS                   = 140027,
    SPELL_ELECTIC_DISCHARGE                     = 140047,
    SPELL_ELECTRIC_DISCHARGE_VISUAL             = 140061,

    //< NPC_LIGHTING_LANCE
    SPELL_RIDE_VEHICLE_HARDCODED                = 46598,
    SPELL_THUNDER_FORGE_SPEAR_COSMETIC_LSTRIKES = 140100,
    SPELL_THUNDER_FORGE_SPEAR_COSMETIC_SPARKLES = 139956,

    //< NPC_SHA_AMALGAMATION
    SPELL_INSANITY_TRIGGER                      = 139381,
    SPELL_INSANITY                              = 139382, //< 30S COOLDOWN
    SPELL_SHADOW_BURST                          = 139375, //< 5S COOLDOWN
    SPELL_DARK_BINDING                          = 139358, //< 5S
    SPELL_METEOR_SOTRM_TRIGGER                  = 139602,
    SPELL_METEOR_STORM_PLAYER_TARGETING         = 139606,
    SPELL_METEOR_STORM                          = 139447, //< 5S COOLDOWN
    SPELL_METEOR_STORM_2                        = 139445,
    SPELL_SUMMON_LEECHING_SHA                   = 139990,
    SPELL_SHADOW_CRASH_TARGETING                = 139360,
    SPELL_SHADOW_CRASH                          = 139312,
    SPELL_SHADOW_CRASH_TRIGGER                  = 139351,
    SPELL_SHADOW_CRASH_CAST                     = 139353,

    //< NPC_DEBILITATING_SHA - 70462
    SPELL_DEBILITATE                            = 139966,

    //< NPC_SHANZE_ELECTRO_CUTIONER
    SPELL_CALL_LIGHTING                         = 138741,

    //< NPC_THUNDER_FORGE
    SPELL_LIGHTING_STRIKE                       = 140678,
    SPELL_LIGHTING_STRIKE_COSMETIC_3            = 140681,
    SPELL_LIGHTING_STRIKE_COSMETIC_4            = 140507,
    SPELL_LIGHTING_STRIKE_COSMETIC_2            = 140857,

    //< NPC_WRATHION
    SPELL_SPEC_TEST                             = 138928,
    SPELL_UPDATE_PHASE_SHIFT                    = 134715,
    SPELL_LIGHTING_STRIKE_COSMETIC_5            = 140527,
    SPELL_WRATHION_TRUE_FORM                    = 109924,
    SPELL_LIGHTING_STRIKE_3                     = 140540,
    // SPELL_LIGHTING_STRIKE_COSMETIC

    //< NPC_SHADO_PAN_WARRIOR
    // SPELL_WOUNDED
    // SPELL_JOIN_PLAYER_PARTY
    // SPELL_LEAVE_PLAYER_PARTY

    //< NPC_SHANZE_PYROMANCER
    SPELL_METEOR_TARGETING                      = 139301,
    SPELL_METEOR_AURA                           = 138168,
    SPELL_METEOR_AT                             = 140660,
    SPELL_FIREBALL                              = 9053,
    
    //< NPC_LIGHTING_PILAR_SPARK_STALKER
    SPELL_LIGHTING_PILAR_SPARK_COSMETIC         = 138152,
    SPELL_LIGHTING_PILAR_SPARK_COSMETIC_3       = 138091,
    SPELL_LIGHTING_PILAR_SPARK_COSMETIC_2       = 138153,

    //< NPC_THUNDER_FORGE2
    SPELL_THUNDER_FORGE_BUFF_PEREODIC           = 139422,
    SPELL_THUNDER_FORGE_BUFF                    = 139401,

    //< NPC_SHANZE_SHADOWCASTER
    SPELL_SHADOW_BOLT                           = 12471,
    SPELL_CORRUPTION                            = 138174,
    
    // NPC_THUNDER_FORGE_3
    SPELL_THUNDER_FORGE_BUFF                    = 139431, //< AT
    SPELL_OVERCHARGED                           = 139397,
    
    //< NPC_NALAK
    SPELL_STATIC_SHIELD_TRIGGER                 = 136343,
    SPELL_STATIC_SHIELD                         = 136342,

    //< NPC_CONSTELLATION
    SPELL_ARCANE_OVERLOAD                       = 138715,
    SPELL_ARCANE_OVERLOAD_TRIGGER               = 138716,

    //< NPC_COSMETIC_SHA_BOSS
    SPELL_SHA_BOSS_STALKER_COSMETIC             = 139908,
    SPELL_SHA_BOSS_COSMETIC_SPAWN               = 139444,
    SPELL_SHA_BOSS_COSMETIC_MISSLE              = 139907,

    //< NPC_METEOR_SUMMONER_STALKER
    SPELL_METEOR_STORM_3                          = 139443,
    SPELL_METEOR_STORM_4                          = 139658,
    // SPELL_METEOR_STORM_2
};

enum Events
{
    EVENT_NONE,

    EVENT_CHECK_WIPE,

    EVENT_WRATHION_MOVE_1,
    EVENT_WRATHION_MOVE_2,
    EVENT_WRATHION_MOVE_3,

    EVENT_INTRO_PART_1,
    EVENT_JOIN_PARTY,
    EVENT_INTRO_PART_2,
    EVENT_HELPERS_MOVE,

    EVENT_HEALING_ORB,
    EVENT_STAGE_1_COMPLETED,

    EVENT_SUMMONS,
    EVENT_COMPLETE_FIRST_PART,

    EVENT_LIGHTING_BOLT,
    EVENT_CHANGE_WEAPON,
    EVENT_LIGHTING_SMASH,
    EVENT_FORGE_CAST,
    EVENT_FORGE_CAST_P2,
    EVENT_FORGE_CAST_P3,
    EVENT_THUNDER_SMASH,

    EVENT_METEOR,
    EVENT_FIREBALL,
    EVENT_CALL_LIGHTING,
    EVENT_LUMBERING_CHARGE,
    EVENT_SHADOW_BOLT,

    EVENT_LR_EMOTE_1,
    EVENT_LR_EMOTE_2,
    EVENT_LR_EMOTE_3,
    EVENT_LR_MOVE,
    EVENT_MOVE_TO_LR,
    EVENT_WIPE_CHECK_2,
    EVENT_LR_0,
    EVENT_LR_1,
    EVENT_LR_2,
    EVENT_LR_3,
    EVENT_LR_4,
    EVENT_LR_5,
    EVENT_LR_6,
    EVENT_LR_7,
    EVENT_LR_8,
    EVENT_LR_9,
    EVENT_LR_10,
    EVENT_LR_11,
    EVENT_LR_12,
    EVENT_LR_13,
    EVENT_LR_14,
    EVENT_LR_15,
    EVENT_LR_16,
    EVENT_LR_17,
    EVENT_LR_18,
    EVENT_LR_19,
    EVENT_LR_20,
    EVENT_LR_21,
    EVENT_LR_22,
    EVENT_LR_23,
    EVENT_LR_24,
    EVENT_LR_25,
    EVENT_LR_26,
    EVENT_LR_27,
    EVENT_LR_28,
    EVENT_LR_29,
    EVENT_LR_30,
    EVENT_LR_31,
    EVENT_LR_32,
    EVENT_LR_33,
    EVENT_LR_34,
    EVENT_LR_35,
    EVENT_LR_36,
    EVENT_LR_37,
    EVENT_LR_38,

    EVENT_NSANITY,
    EVENT_SHADOW_BURST,
    EVENT_DARK_BINDING,
    EVENT_METEOR_STORM,
    EVENT_SHADOW_CRASH,
    EVENT_SMALL_SHA_FIXATE,
    EVENT_DARK_SUNDER,
    EVENT_SHA_BLAST,
    EVENT_EMPOWERED,
    EVENT_DARK_BITE,
    EVENT_LETHARGY,
    EVENT_ABSORB_EVIL,
};

enum Action
{
    ACTION_NONE,

    ACTION_WRATHION_START,
    ACTION_EVENT_PART_1,
    ACTION_JOIN_PARTY,
    ACTION_INISIBLE_HUNTER_AURA,

    ACTION_CHARGING_1,
    ACTION_CHARGING_2,
    ACTION_CHARGING_3,
    ACTION_CHARGING_4,

    ACTION_LR_P1 = 11,

    ACTION_CB_START_MOVING,
    ACTION_FORGE_CAST,
    ACTION_CANCEL_FORGE_EVENTS,
    ACTION_M_ENERGY,
    ACTION_SHA_FIXATE,
    ACTION_FIRST_WAVE,

    ACTION_SCENARIO_COMPLETED

};

enum Phases
{
    PHASE_NONE,

    PHASE_ONE,
    PHASE_TWO,
};

enum Points
{
    POINT_NONE,

    POINT_NEW_HOME,
    POINT_EVADE_POS,
};

enum Sounds
{
    //< SMSG_PLAY_OBJECT_SOUND
    SOUND_0 = 36049,
    SOUND_1 = 36050,
    SOUND_2 = 36051,
    SOUND_3 = 36052,
    SOUND_4 = 36053,
    SOUND_5 = 36054,
    SOUND_6 = 36055,

    //< SMSG_PLAY_SOUND
    SOUND_7 = 36021,
    SOUND_8 = 36022,
    SOUND_9 = 36405,
    SOUND_10 = 36023,
    SOUND_11 = 36026,
    SOUND_12 = 36027,
    SOUND_13 = 36028,
    SOUND_14 = 36030,
    SOUND_15 = 36039,
    SOUND_16 = 36040,
    SOUND_17 = 36041,
    SOUND_18 = 36042,
    SOUND_19 = 36048,
    SOUND_20 = 36406,
    SOUND_21 = 36407,
    SOUND_22 = 36408,
    SOUND_23 = 36409,
    SOUND_24 = 36410,
    SOUND_25 = 36411
};

Position const addsPositions[]
{
    {7255.153f, 5307.322f, 66.06771f},
    {7266.683f, 5325.157f, 66.07448f},
    {7268.239f, 5327.411f, 66.38251f},
    {7257.758f, 5271.01f, 66.06776f},
    {7217.198f, 5311.208f, 65.88635f},
    {7266.987f, 5324.924f, 66.10201f},
};

Position const WrathionWP[]
{
    {7214.340f, 5285.522f, 66.05622f},
    {7207.432f, 5270.125f, 66.05622f},
    {7204.241f, 5266.851f, 66.05622f},
    {7200.241f, 5255.292f, 65.98731f}
};

Position const DefenderPoints[]
{
    {7213.67f, 5266.370f, 65.98440f}, //< Jump to this position
    {7221.26f, 5276.239f, 66.05622f}  //< move here after jump
};

Position const warriorPoints[]
{
    {7210.650f, 5247.510f, 65.98440f}, //< Jump to this position right
    {7223.458f, 5262.069f, 65.98731f}, //< right
};

Position const dHomePoints2[]
{
    {7196.58f, 5233.76f, 85.5807f},
    {7175.76f, 5255.42f, 85.5464f},
    {7166.76f, 5264.76f, 85.5331f}
};

Position const warriorPoints2[]
{
    {7210.650f, 5247.510f, 65.98440f}, //< Jump to this position right
    {7223.458f, 5262.069f, 65.98731f}, //< right
};

float const cosmeticPilarPos[4][3]
{
    {7195.710f, 5249.874f, 67.64626f},
    {7195.721f, 5250.080f, 69.25357f},
    {7195.720f, 5250.080f, 73.64575f},
    {7195.721f, 5250.080f, 80.91364f}
};

Position const sparkStalker[]
{
    {7195.72f, 5250.08f, 75.0836f},
};

int32 const phasingTargets[]
{
    70100, 70106, 70106, 70099, 70577, 62142, 65183, 69162, 69170, 69200, 69217,
    69223, 69244, 69269, 69379, 69397, 69404, 69412, 69481, 69798, 69813, 69827,
    69833, 69835, 69923, 70042, 70070, 70074, 70283, 70556, 69156, 69411, 69509
};

Position const centerPos[]
{
    {7368.795898f, 5160.241211f, 49.531322f}
};
G3D::Vector3 const dPoints3[]
{
    {7380.517090f, 5190.142578f, 49.614887f},
    {7363.884277f, 5190.941406f, 49.616081f},
    {7346.669434f, 5185.123535f, 49.633316f},
    {7338.504395f, 5168.315430f, 49.615425f},
    {7339.720703f, 5145.416504f, 49.617168f},
    {7353.019531f, 5132.396484f, 49.613995f},
    {7368.584473f, 5129.186035f, 49.628078f},
    {7383.784180f, 5131.549805f, 49.620625f},
    {7394.010254f, 5140.544922f, 49.623112f},
    {7400.024414f, 5150.685059f, 49.626301f},
    {7398.979492f, 5169.995605f, 49.615177f},
};

G3D::Vector3 const wPoints3[]
{
    {7199.381836f, 5263.923828f, 65.985260f},
    {7216.750488f, 5266.845215f, 65.985695f},
    {7235.419922f, 5262.388672f, 64.806145f},
    {7251.270508f, 5258.604980f, 64.806145f},
    {7265.938965f, 5251.916992f, 65.985840f},
    {7280.606445f, 5241.892090f, 65.958626f},
    {7294.209961f, 5232.281250f, 65.984169f},
    {7314.681641f, 5212.746582f, 65.507530f},
    {7350.040039f, 5178.683594f, 49.388092f},
    //< 
    {7368.597168f, 5171.087891f, 49.585335f}, //< 9
    {7360.373535f, 5169.993164f, 49.583530f},
    {7356.601074f, 5160.032227f, 49.387676f},
};

Position const wrathionS2Points[]
{
    //< 0
    {7367.086f, 5171.830f, 49.83737f}, //< 0
    {7368.374f, 5170.050f, 49.59216f},
    //< 1
    {7365.798f, 5171.189f, 49.83737f}, //< 2
    {7364.298f, 5171.939f, 49.58737f},
    {7362.798f, 5172.689f, 49.83737f},
    {7360.548f, 5170.439f, 49.83737f},
    //< 2
    {7359.222f, 5169.328f, 49.58258f}, //< 6
    //< 3
    {7364.523f, 5172.068f, 49.50574f}, //< 7
    {7362.523f, 5172.818f, 49.75574f},
    {7360.523f, 5170.568f, 49.75574f},
    //< 4
    {7359.222f, 5169.328f, 49.58258f}, //< 10
    //< 5
    {7358.805f, 5168.212f, 49.83258f}, //< 11
    {7358.305f, 5166.212f, 49.58258f},
    {7358.055f, 5164.712f, 49.83258f},
    {7357.555f, 5163.212f, 49.83258f},
    {7357.305f, 5161.962f, 49.83258f},
    {7357.055f, 5160.712f, 49.83258f},
    {7356.555f, 5159.212f, 49.58258f}, //< 17
    //< 6
    {7356.554f, 5159.073f, 49.55445f}, //< 18
    {7356.054f, 5157.073f, 49.80445f},
    {7357.804f, 5155.073f, 49.80445f},
    {7358.304f, 5154.323f, 49.55445f},
    {7360.054f, 5151.573f, 49.55445f}, //< 22
    //< 7
    {7360.411f, 5151.082f, 49.58258f}, //< 23
    //< 8
    {7362.169f, 5150.341f, 49.58258f}, //< 24
    {7365.169f, 5149.091f, 49.58258f},
    {7366.419f, 5148.591f, 49.58258f},
    //< 9
    {7369.137f, 5147.261f, 49.75739f}, //<  27
    {7371.887f, 5148.261f, 49.75739f},
    //< 10
    {7372.073f, 5148.054f, 49.83258f}, //< 29
    {7374.073f, 5148.804f, 49.83258f},
    {7376.323f, 5150.554f, 49.83258f}, //< 31
    //< 11
    {7377.702f, 5151.790f, 49.58258f}, //< 32
    //< 12
    {7378.202f, 5153.209f, 49.58288f}, //< 33
    {7378.952f, 5154.709f, 49.58288f},
    {7379.702f, 5156.959f, 49.83288f},
    {7379.952f, 5158.459f, 49.83288f},
    {7380.702f, 5160.459f, 49.58288f},
    {7381.202f, 5161.959f, 49.83288f}, //< 38
    //< 13
    {7381.265f, 5162.115f, 49.7605f}, //< 39
    {7381.765f, 5163.365f, 49.7605f},
    {7381.015f, 5164.365f, 49.7605f},
    {7379.515f, 5166.365f, 49.7605f},
    {7378.765f, 5168.115f, 49.7605f}, //< 43
    //< 14
    {7377.641f, 5169.406f, 49.58258f}, //< 44
    //< 15
    {7376.432f, 5170.843f, 49.83258f}, //< 45
    {7374.682f, 5171.593f, 49.83258f},
    {7372.432f, 5172.843f, 49.83258f},
    {7370.182f, 5172.843f, 49.83258f}, //< 48
    //< 16
    {7368.723f, 5172.779f, 49.58258f}, //< 49
};

Position const celestialDefenderPoints[]
{
    {7412.179f, 5204.663f, 55.41367f},

    /* 0 */  {7401.632f, 5196.375f, 54.60491f},
    /* 1 */  {7398.632f, 5192.875f, 52.60491f},
    /* 2 */  {7397.382f, 5191.125f, 51.60491f},
    /* 3 */  {7395.632f, 5190.875f, 50.85491f},
    /* 4 */  {7393.632f, 5190.625f, 50.10491f},
    /* 5 */  {7383.132f, 5189.375f, 50.10491f},
};

Position const celestialBlacksmithPoints[]
{
    {7323.568f, 5114.839f, 55.45367f},
};

Position const cBlacksmithPositions[]
{
    {7339.918f, 5160.253f, 49.58442f}, // t = 0;
    {7348.574f, 5140.081f, 49.57350f}, // t += 40;
    {7368.478f, 5132.272f, 49.58466f}, // t += 32;
    {7388.370f, 5140.493f, 49.58445f}, // t += 35;
    {7396.211f, 5160.753f, 49.58324f}, // t += 35;
    {7388.043f, 5180.270f, 49.58472f}, // t += 36;
    {7368.527f, 5187.700f, 49.58317f}, // t += 34;
};

Position const shaFinedsPositions[]
{
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7312.857f, 5209.722f, 65.48965f, 5.4242920f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7356.898f, 5186.524f, 49.56894f, 5.8754060f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7351.568f, 5187.763f, 49.62176f, 5.7390580f},
    {7341.887f, 5193.620f, 51.39300f, 5.7391410f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7342.526f, 5193.744f, 51.20268f, 5.8395330f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7335.335f, 5202.951f, 56.86404f, 5.5179640f},
    {7319.052f, 5216.208f, 65.49776f, 5.6599760f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
    {7417.073f, 5111.687f, 55.45368f, 2.2810680f},
    {7310.219f, 5212.780f, 65.59111f, 4.6407990f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7417.035f, 5203.043f, 55.45367f, 3.7625580f},
    {7412.241f, 5209.885f, 55.45366f, 3.9634490f},
    {7319.145f, 5110.835f, 55.45368f, 0.6222018f},
    {7315.756f, 5218.577f, 65.59111f, 5.0987890f},
};

Position const shaBeastPositions[]
{
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7317.721f, 5109.741f, 55.45368f, 0.878840f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7317.721f, 5109.741f, 55.45368f, 0.878840f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7317.721f, 5109.741f, 55.45368f, 0.878840f},
    {7312.302f, 5217.170f, 65.59111f, 5.390892f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7312.302f, 5217.170f, 65.59111f, 5.390892f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
    {7312.302f, 5217.170f, 65.59111f, 5.691250f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7415.530f, 5208.013f, 55.45368f, 3.720318f},
    {7418.579f, 5110.358f, 55.45368f, 2.369927f},
};

class npc_wrathion : public CreatureScript
{
public:
    npc_wrathion() : CreatureScript("npc_wrathion") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            player->ADD_GOSSIP_ITEM_DB(TEXT_WRATHION_START, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/)
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return false;

        if (instance->GetData(DATA_COMPLETE_EVENT_STAGE_1) == DONE)
        {
            instance->SetData(DATA_LR_START, SPECIAL);
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->AI()->DoAction(ACTION_LR_P1);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        else
        {
            instance->SetData(DATA_START_EVENT, IN_PROGRESS);
            instance->SetData(DATA_EVENT_PART_1, NOT_STARTED);
            creature->AI()->DoAction(ACTION_WRATHION_START);
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        return false;
    }

    struct npc_wrathionAI : public ScriptedAI
    {
        npc_wrathionAI(Creature* creature) : ScriptedAI(creature), summon(me)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_WRATHION_START:
                    events.ScheduleEvent(EVENT_WRATHION_MOVE_1, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_EVENT_PART_1:
                {
                    events.ScheduleEvent(EVENT_INTRO_PART_1, 1 * IN_MILLISECONDS);

                    std::list<Creature*> creatures;
                    GetCreatureListWithEntryInGrid(creatures, me, NPC_THUNDER_FORGE, 200.0f);
                    if (!creatures.empty())
                        for (std::list<Creature*>::iterator forge = creatures.begin(); forge != creatures.end(); ++forge)
                            (*forge)->CastSpell((*forge), SPELL_LIGHTING_STRIKE);

                    break;
                }
                case ACTION_LR_P1:
                    events.ScheduleEvent(EVENT_MOVE_TO_LR, 2 * IN_MILLISECONDS);
                    instance->SetData(DATA_SUMMONS_COUNTER, 0);
                    me->SummonCreature(NPC_CELESTIAL_BLACKSMITH, celestialBlacksmithPoints[0]);
                    me->SummonCreature(NPC_CELESTIAL_DEFENDER, celestialDefenderPoints[0]);
                    break;
                case ACTION_M_ENERGY:
                    me->PlayDistanceSound(36406);
                    Talk(13);
                    events.ScheduleEvent(EVENT_LR_37, 5 * IN_MILLISECONDS);
                    break;
                case ACTION_SCENARIO_COMPLETED:
                    events.ScheduleEvent(EVENT_LR_30, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void JustSummoned(Creature* sum)
        {
            summon.Summon(sum);
            switch (sum->GetEntry())
            {
                case NPC_CELESTIAL_BLACKSMITH:
                case NPC_CELESTIAL_DEFENDER:
                    sum->AI()->DoAction(ACTION_CB_START_MOVING);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (instance->GetData(DATA_COMPLETE_EVENT_STAGE_1) == SPECIAL)
            {
                events.ScheduleEvent(EVENT_STAGE_1_COMPLETED, 3 * IN_MILLISECONDS);
                instance->SetData(DATA_COMPLETE_EVENT_STAGE_1, DONE);
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LR_30:
                        events.ScheduleEvent(EVENT_LR_31, 3 * IN_MILLISECONDS);
                        me->PlayDistanceSound(36054);
                        Talk(24);
                        break;
                    case EVENT_LR_31:
                        events.ScheduleEvent(EVENT_LR_32, 3 * IN_MILLISECONDS);
                        me->PlayDistanceSound(36055);
                        Talk(25);
                        break;
                    case EVENT_LR_32:
                        //< disable this - should be called just after quest accepting
                        if (Player* plr = me->FindNearestPlayer(100.0f))
                            me->DestroyForPlayer(plr);
                        break;
                    case EVENT_LR_37:
                        Talk(15);
                        me->SummonCreature(NPC_SHA_AMALGAMATION, 7348.246f, 5179.011f, 49.38733f, 2.254864f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_38, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_38:
                        Talk(16);
                        break;
                    case EVENT_LR_0:
                        events.CancelEvent(EVENT_FORGE_CAST);
                        events.ScheduleEvent(EVENT_LR_EMOTE_1, 9 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_WIPE_CHECK_2, 3 * IN_MILLISECONDS);
                        Talk(5);
                        me->PlayDistanceSound(SOUND_12);
                        break;
                    case EVENT_LR_EMOTE_1:
                        events.ScheduleEvent(EVENT_LR_EMOTE_2, 9 * IN_MILLISECONDS);
                        me->PlayDistanceSound(SOUND_13);
                        Talk(6);
                        break;
                    case EVENT_WIPE_CHECK_2:
                        if (instance->IsWipe() || instance->GetData(DATA_LR_STAGE_2) == FAIL)
                            events.RescheduleEvent(EVENT_LR_EMOTE_2, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_EMOTE_2:
                        events.ScheduleEvent(EVENT_LR_EMOTE_3, 9 * IN_MILLISECONDS);
                        me->PlayDistanceSound(SOUND_14);
                        Talk(7);
                        break;
                    case EVENT_LR_EMOTE_3:
                        me->SetDynamicWorldEffects(505, 1);
                        me->PlayDistanceSound(SOUND_15);
                        Talk(8);
                        me->SetFlag64(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    case EVENT_LR_MOVE:
                        me->RemoveAurasDueToSpell(SPELL_WRATHION_TRUE_FORM);
                        me->GetMotionMaster()->MovePoint(EVENT_LR_MOVE, wrathionS2Points[1]);
                        events.ScheduleEvent(EVENT_LR_3, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_3:
                    {
                        Movement::MoveSplineInit init(*me);
                        for (uint8 i = 9; i < 11; ++i)
                            init.Path().push_back(wPoints3[i]);
                        init.SetSmooth();
                        init.Launch();

                        me->PlayDistanceSound(SOUND_16);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                        Talk(9);
                        events.ScheduleEvent(EVENT_LR_4, 8 * IN_MILLISECONDS);
                        break;
                    }
                    case EVENT_LR_4:
                        me->PlayDistanceSound(SOUND_17);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                        Talk(10);
                        events.ScheduleEvent(EVENT_LR_5, 8 * IN_MILLISECONDS);
                        doAction(me, NPC_CELESTIAL_BLACKSMITH, ACTION_FIRST_WAVE);
                        break;
                    case EVENT_LR_5:
                        me->PlayDistanceSound(SOUND_18);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_TALK_NO_SHEATHE);
                        Talk(11);
                        events.ScheduleEvent(EVENT_LR_6, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_6:
                        events.ScheduleEvent(EVENT_LR_7, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_7:
                        me->GetMotionMaster()->MovePoint(EVENT_LR_7, wrathionS2Points[10]);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_8, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_8:
                        events.ScheduleEvent(EVENT_LR_9, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_9:
                        events.ScheduleEvent(EVENT_LR_9, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_11:
                        me->GetMotionMaster()->MovePoint(EVENT_LR_11, wrathionS2Points[23]);
                        events.ScheduleEvent(EVENT_LR_12, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_12:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_13, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_13:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
                        events.ScheduleEvent(EVENT_LR_14, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_14:
                        events.ScheduleEvent(EVENT_LR_15, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_15:
                        events.ScheduleEvent(EVENT_LR_16, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_16:
                        me->GetMotionMaster()->MovePoint(EVENT_LR_16, wrathionS2Points[32]);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_17, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_17:
                        events.ScheduleEvent(EVENT_LR_18, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_18:
                        events.ScheduleEvent(EVENT_LR_19, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_19:
                        me->GetMotionMaster()->MovePoint(EVENT_LR_19, wrathionS2Points[44]);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_20, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_20:
                        events.ScheduleEvent(EVENT_LR_21, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_21:
                        me->GetMotionMaster()->MovePoint(EVENT_LR_21, wrathionS2Points[49]);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_COSMETIC);
                        events.ScheduleEvent(EVENT_LR_22, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_22:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_COSMETIC);
                        break;
                    case EVENT_STAGE_1_COMPLETED:
                    {
                        doAction(me, NPC_THUNDER_FORGE2, ACTION_CANCEL_FORGE_EVENTS);
                        instance->SetData(DATA_LR_START, IN_PROGRESS);
                        Talk(4);

                        me->AddAura(SPELL_WRATHION_TRUE_FORM, me);
                        me->CastSpell(me, SPELL_WRATHION_TRUE_FORM);

                        Movement::MoveSplineInit init(*me);
                        for (uint8 i = 0; i < 8; ++i)
                            init.Path().push_back(wPoints3[i]);
                        init.SetSmooth();
                        init.Launch();

                        events.ScheduleEvent(EVENT_LR_MOVE, 40 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_0, 40 * IN_MILLISECONDS);
                        break;
                    }
                    case EVENT_CHECK_WIPE:
                        if (instance->IsWipe())
                        {
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                            if (instance->GetData(DATA_EVENT_PART_2) != DONE)
                                instance->SetData(DATA_START_EVENT, NOT_STARTED);

                            if (instance->GetData(DATA_COMPLETE_EVENT_STAGE_1) == IN_PROGRESS)
                                instance->SetData(DATA_COMPLETE_EVENT_STAGE_1, NOT_STARTED);
                        }
                        else
                        {
                            events.ScheduleEvent(EVENT_CHECK_WIPE, 2 * IN_MILLISECONDS);
                            if (instance->GetData(DATA_COMPLETE_EVENT_STAGE_1) == SPECIAL)
                                events.CancelEvent(EVENT_CHECK_WIPE);
                        }
                        break;
                    case EVENT_WRATHION_MOVE_1:
                        events.ScheduleEvent(EVENT_WRATHION_MOVE_2, 3 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(EVENT_WRATHION_MOVE_1, WrathionWP[1]);
                        Talk(0);
                        break;
                    case EVENT_WRATHION_MOVE_2:
                        events.ScheduleEvent(EVENT_WRATHION_MOVE_3, 5 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(EVENT_WRATHION_MOVE_2, WrathionWP[2]);
                        break;
                    case EVENT_WRATHION_MOVE_3:
                        Talk(1);
                        me->SetFacingTo(0.7853982f);
                        me->SetHomePosition(WrathionWP[3].m_positionX, WrathionWP[3].m_positionY, WrathionWP[3].m_positionZ, 0.7853982f);
                        break;
                    case EVENT_INTRO_PART_1:
                        doAction(me, NPC_SHADO_PAN_DEFENDER, ACTION_JOIN_PARTY);
                        doAction(me, NPC_SHADO_PAN_WARRIOR, ACTION_JOIN_PARTY);

                        events.ScheduleEvent(EVENT_INTRO_PART_2, 5 * IN_MILLISECONDS);
                        instance->SetData(DATA_COMPLETE_EVENT_STAGE_1, IN_PROGRESS);
                        Talk(2);
                        break;
                    case EVENT_INTRO_PART_2:
                        if (Creature* stalker = me->FindNearestCreature(NPC_INVISIBLE_STALKER, 50.0f))
                        {
                            stalker->AI()->DoAction(ACTION_INISIBLE_HUNTER_AURA);
                            stalker->NearTeleportTo(cosmeticPilarPos[0][0], cosmeticPilarPos[0][1], cosmeticPilarPos[0][2], 0.0f);
                        }

                        events.ScheduleEvent(EVENT_CHECK_WIPE, 2 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_FORGE_CAST, 15 * IN_MILLISECONDS);

                        instance->SetData(DATA_EVENT_PART_1, DONE);
                        instance->SetData(DATA_START_EVENT, DONE);
                        instance->SetData(DATA_EVENT_PART_2, IN_PROGRESS);

                        Talk(3);
                        break;
                    case EVENT_FORGE_CAST:
                        //events.ScheduleEvent(EVENT_FORGE_CAST, 30 * IN_MILLISECONDS);
                        doAction(me, NPC_THUNDER_FORGE2, ACTION_FORGE_CAST);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        InstanceScript* instance;
        EventMap events;
        SummonList summon;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_wrathionAI>(creature);
    }
};

//< 218910
class go_mogu_crucible : public GameObjectScript
{
public:
    go_mogu_crucible() : GameObjectScript("go_mogu_crucible") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        //ServerToClient: SMSG_GAME_OBJECT_ACTIVATE_ANIM_KIT (0x0C8A) Length: 12 - build 18414
        //unk bit: True
        //unk int32: 3809
        //Guid: Full: 0xF113571E0000489C Type: GameObject Entry: 218910 Low: 18588

        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (instance->GetData(DATA_START_EVENT) == IN_PROGRESS)
            {
                instance->SetData(DATA_EVENT_PART_1, IN_PROGRESS);
                if (Creature* wrathion = go->FindNearestCreature(NPC_WRATHION, 50.0f))
                    wrathion->AI()->DoAction(ACTION_EVENT_PART_1);

                return true;
            }
        }
        return false;
    }
};

class npc_shado_pan_defender : public CreatureScript
{
public:
    npc_shado_pan_defender() : CreatureScript("npc_shado_pan_defender") { }

    struct npc_shado_pan_defenderAI : public ScriptedAI
    {
        npc_shado_pan_defenderAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_JOIN_PARTY:
                    events.ScheduleEvent(EVENT_JOIN_PARTY, 1 * IN_MILLISECONDS);
                    me->CastSpell(me->FindNearestPlayer(200.0f), SPELL_JOIN_PLAYER_PARTY);
                    me->AddAura(SPELL_WOUNDED, me);
                    break;
                case ACTION_EVADE:
                    EnterEvadeMode();
                    break;
                case ACTION_COMPLETE_FIRST_PART:
                    events.ScheduleEvent(EVENT_COMPLETE_FIRST_PART, 3 * IN_MILLISECONDS);
                    break;
                case ACTION_CANCEL_SUMMONS:
                    events.CancelEvent(EVENT_SUMMONS);
                    break;
                default:
                    break;
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_HEALING_ORB, urand(10, 17) * IN_MILLISECONDS);
            me->CallForHelp(100.0f);
        }

        void EnterEvadeMode()
        {
            Talk(1);

            me->GetMotionMaster()->MovePoint(POINT_EVADE_POS, DefenderPoints[1]);

            if (instance->GetData(DATA_COMPLETE_EVENT_STAGE_1) == IN_PROGRESS)
                events.ScheduleEvent(EVENT_SUMMONS, 5 * IN_MILLISECONDS);
        }

        void JustSummoned(Creature* summon)
        {
            Map::PlayerList const &PlList = summon->GetMap()->GetPlayers();
            if (PlList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (player->isAlive())
                    {
                        summon->SetInCombatWith(player);
                        player->SetInCombatWith(summon);
                    }
                }
            }

            me->SetInCombatWithZone();
            summon->GetMotionMaster()->MovePoint(1, DefenderPoints[1]);

            switch (summon->GetEntry())
            {
                case NPC_SHANZE_SHADOWCASTER:
                case NPC_SHANZE_WARRIOR:
                case NPC_SHANZE_BATTLEMASTER:
                case NPC_SHANZE_ELECTRO_CUTIONER:
                case NPC_SHANZE_PYROMANCER:
                    instance->SetData(DATA_SUMMONS_COUNTER, instance->GetData(DATA_SUMMONS_COUNTER) + 1);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_COMPLETE_FIRST_PART:
                        instance->HandleGameObject(instance->GetData64(DATA_TRUNDER_FORGE_DOOR), true);
                        Talk(2);
                        DoCast(SPELL_LEAVE_PLAYER_PARTY);
                        me->GetMotionMaster()->MoveJump(dHomePoints2[2].m_positionX, dHomePoints2[2].m_positionY, dHomePoints2[2].m_positionZ,
                            20.0f, 20.0f);

                        me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                        break;
                    case EVENT_JOIN_PARTY:
                        me->GetMotionMaster()->MoveJump(DefenderPoints[0].m_positionX, DefenderPoints[0].m_positionY, DefenderPoints[0].m_positionZ,
                            20.0f, 20.0f);

                        me->CastSpell(me->FindNearestPlayer(200.0f), SPELL_JOIN_PLAYER_PARTY);

                        events.ScheduleEvent(EVENT_HELPERS_MOVE, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_HELPERS_MOVE:
                        me->GetMotionMaster()->MovePoint(EVENT_HELPERS_MOVE, DefenderPoints[1]);
                        me->SetHomePosition(DefenderPoints[1]);
                        events.ScheduleEvent(EVENT_SUMMONS, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_HEALING_ORB:
                        DoCast(SPELL_HEALING_ORB);
                        Talk(0);
                        break;
                    case EVENT_SUMMONS:
                    {
                        if (instance->GetData(DATA_SUMMONS_COUNTER))
                            events.ScheduleEvent(EVENT_SUMMONS, 10 * IN_MILLISECONDS);
                        else
                            switch (urand(1, 5))
                        {
                                case 1:
                                {
                                    for (uint8 i = 0; i < 1; ++i)
                                        me->SummonCreature(NPC_SHANZE_WARRIOR, addsPositions[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3 * IN_MILLISECONDS);

                                    me->SummonCreature(NPC_SHANZE_PYROMANCER, addsPositions[urand(2, 5)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3 * IN_MILLISECONDS);
                                    break;
                                }
                                case 2:
                                    me->SummonCreature(NPC_SHANZE_BATTLEMASTER, addsPositions[urand(0, 5)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3 * IN_MILLISECONDS);
                                    break;
                                case 3:
                                    me->SummonCreature(NPC_SHANZE_WARRIOR, addsPositions[urand(0, 2)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3 * IN_MILLISECONDS);
                                    me->SummonCreature(NPC_SHANZE_PYROMANCER, addsPositions[urand(2, 4)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3 * IN_MILLISECONDS);
                                    break;
                                case 4:
                                    me->SummonCreature(NPC_SHANZE_ELECTRO_CUTIONER, addsPositions[urand(0, 5)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3 * IN_MILLISECONDS);
                                    break;
                                case 5:
                                    me->SummonCreature(NPC_SHANZE_SHADOWCASTER, addsPositions[urand(0, 5)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS);
                                    break;
                                default:
                                    break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shado_pan_defenderAI>(creature);
    }
};

class npc_shado_pan_warrior : public CreatureScript
{
public:
    npc_shado_pan_warrior() : CreatureScript("npc_shado_pan_warrior") { }

    struct npc_shado_pan_warriorAI : public ScriptedAI
    {
        npc_shado_pan_warriorAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_JOIN_PARTY:
                    events.ScheduleEvent(EVENT_JOIN_PARTY, urand(3, 5) * IN_MILLISECONDS);
                    me->CastSpell(me->FindNearestPlayer(200.0f), SPELL_JOIN_PLAYER_PARTY);
                    me->AddAura(SPELL_WOUNDED, me);
                    break;
                case ACTION_EVADE:
                    EnterEvadeMode();
                    break;
                case ACTION_COMPLETE_FIRST_PART:
                    events.ScheduleEvent(EVENT_COMPLETE_FIRST_PART, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->CallForHelp(100.0f);
        }

        void EnterEvadeMode()
        {
            me->GetMotionMaster()->MovePoint(POINT_EVADE_POS, instance->GetData(DATA_JUMP_POS) == 1 ? warriorPoints[1] : warriorPoints2[1]);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_JOIN_PARTY:

                        if (instance->GetData(DATA_JUMP_POS) == 1)
                            me->GetMotionMaster()->MoveJump(warriorPoints[0].m_positionX, warriorPoints[0].m_positionY, warriorPoints[0].m_positionZ, 30.0f, 25.0f);
                        else
                        {
                            me->GetMotionMaster()->MoveJump(warriorPoints2[0].m_positionX, warriorPoints2[0].m_positionY, warriorPoints2[0].m_positionZ, 30.0f, 25.0f);
                            instance->SetData(DATA_JUMP_POS, instance->GetData(DATA_JUMP_POS) + 1);
                        }
                        events.ScheduleEvent(EVENT_HELPERS_MOVE, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_HELPERS_MOVE:
                        me->GetMotionMaster()->MovePoint(POINT_NEW_HOME, instance->GetData(DATA_JUMP_POS) == 1 ? warriorPoints[1] : warriorPoints2[1]);
                        me->SetHomePosition(instance->GetData(DATA_JUMP_POS) == 1 ? warriorPoints[1] : warriorPoints2[1]);
                        break;
                    case EVENT_COMPLETE_FIRST_PART:
                        DoCast(SPELL_LEAVE_PLAYER_PARTY);
                        if (instance->GetData(DATA_JUMP_POS) == 1)
                            me->GetMotionMaster()->MoveJump(dHomePoints2[0].m_positionX, dHomePoints2[0].m_positionY, dHomePoints2[0].m_positionZ, 20.0f, 20.0f);
                        else
                            me->GetMotionMaster()->MoveJump(dHomePoints2[1].m_positionX, dHomePoints2[1].m_positionY, dHomePoints2[1].m_positionZ, 20.0f, 20.0f);

                        me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shado_pan_warriorAI>(creature);
    }
};

class npc_thunder_forge_second : public CreatureScript
{
public:
    npc_thunder_forge_second() : CreatureScript("npc_thunder_forge_second") { }

    struct npc_thunder_forge_secondAI : public Scripted_NoMovementAI
    {
        npc_thunder_forge_secondAI(Creature* creature) : Scripted_NoMovementAI(creature)
        {
            me->SetVisible(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_FORGE_CAST:
                    events.ScheduleEvent(EVENT_FORGE_CAST, 3 * IN_MILLISECONDS);
                    break;
                case ACTION_CANCEL_FORGE_EVENTS:
                    events.CancelEvent(EVENT_FORGE_CAST);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FORGE_CAST:
                        events.ScheduleEvent(EVENT_FORGE_CAST, 30 * IN_MILLISECONDS);
                        me->CastSpell(me, SPELL_THUNDER_FORGE_BUFF_PEREODIC);
                        events.ScheduleEvent(EVENT_FORGE_CAST_P3, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_FORGE_CAST_P3:
                        me->CastSpell(me, SPELL_THUNDER_FORGE_BUFF);
                        Talk(0);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_thunder_forge_secondAI>(creature);
    }
};

class npc_invisible_hunter : public CreatureScript
{
public:
    npc_invisible_hunter() : CreatureScript("npc_invisible_hunter") { }

    struct npc_invisible_hunterAI : public Scripted_NoMovementAI
    {
        npc_invisible_hunterAI(Creature* creature) : Scripted_NoMovementAI(creature)
        {
            me->SetVisible(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            instance = creature->GetInstanceScript();
        }

        void Reset()
        { }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_INISIBLE_HUNTER_AURA:
                    if (Player* player = me->FindNearestPlayer(300.0f))
                    {
                        me->AddAura(SPELL_THUNDER_FORGE_CHARGE, player);
                        me->AddAura(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1, player);
                        me->AddAura(SPELL_UPDATE_PHASE_SHIFT, player);
                        DoCastAOE(SPELL_UPDATE_PHASE_SHIFT);
                    }
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff) { }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_invisible_hunterAI>(creature);
    }
};

class npc_lighting_pilar_beam_stalker : public CreatureScript
{
public:
    npc_lighting_pilar_beam_stalker() : CreatureScript("npc_lighting_pilar_beam_stalker") { }

    struct npc_lighting_pilar_beam_stalkerAI : public CreatureAI
    {
        npc_lighting_pilar_beam_stalkerAI(Creature* creature) : CreatureAI(creature)
        {
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
                case NPC_FORGEMASTER_VULKON:
                    summon->CastSpell(summon, SPELL_FORGEMASTER_SPAWN_COSMETIC);
                    break;
                default:
                    break;
            }
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CHARGING_1:
                    events.ScheduleEvent(EVENT_FORGE_CAST_P2, 1 * IN_MILLISECONDS);
                    me->NearTeleportTo(cosmeticPilarPos[1][0], cosmeticPilarPos[1][1], cosmeticPilarPos[1][2], 0.0f);
                    break;
                case ACTION_CHARGING_2:
                    me->NearTeleportTo(cosmeticPilarPos[2][0], cosmeticPilarPos[2][1], cosmeticPilarPos[2][2], 0.0f);
                    break;
                case ACTION_CHARGING_3:
                    me->NearTeleportTo(cosmeticPilarPos[3][0], cosmeticPilarPos[3][1], cosmeticPilarPos[3][2], 0.0f);
                    break;
                case ACTION_CHARGING_4:
                    me->SummonCreature(NPC_FORGEMASTER_VULKON, 7207.826f, 5262.409f, 66.06776f, 6.170584f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FORGE_CAST_P2:
                        events.ScheduleEvent(EVENT_FORGE_CAST_P2, 2 * IN_MILLISECONDS);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE_3, 200.0f))
                        {
                            me->AddAura(SPELL_LIGHTING_STRIKE_COSMETIC_2, target);
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_COSMETIC_2);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_lighting_pilar_beam_stalkerAI>(creature);
    }
};

class npc_lighting_pilar_spark_stalker : public CreatureScript
{
public:
    npc_lighting_pilar_spark_stalker() : CreatureScript("npc_lighting_pilar_spark_stalker") { }

    struct npc_lighting_pilar_spark_stalkerAI : public CreatureAI
    {
        npc_lighting_pilar_spark_stalkerAI(Creature* creature) : CreatureAI(creature)
        {
            me->SetDisableGravity(true);
            me->SetCanFly(true);
            instance = creature->GetInstanceScript();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CHARGING_1:
                    me->AddAura(SPELL_LIGHTING_PILAR_SPARK_COSMETIC, me);
                    break;
                case ACTION_CHARGING_2:
                    me->AddAura(SPELL_LIGHTING_PILAR_SPARK_COSMETIC_3, me);
                    break;
                case ACTION_CHARGING_3:
                    me->AddAura(SPELL_LIGHTING_PILAR_SPARK_COSMETIC_2, me);
                    break;
                default:
                    break;
            }
        }

        void Reset() { }

        void UpdateAI(uint32 /*diff*/) { }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_lighting_pilar_spark_stalkerAI>(creature);
    }
};

class npc_forgemaster_vulkon : public CreatureScript
{
public:
    npc_forgemaster_vulkon() : CreatureScript("npc_forgemaster_vulkon") { }

    struct npc_forgemaster_vulkonAI : public CreatureAI
    {
        npc_forgemaster_vulkonAI(Creature* creature) : CreatureAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->AddAura(SPELL_ELECTRIFIED, me);
        }

        void Reset()
        {
            me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 94565);
            me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 94565);

            events.Reset();
            events.SetPhase(PHASE_ONE);

            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void EnterCombat(Unit* /*who*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

            DoZoneInCombat(me);

            events.ScheduleEvent(EVENT_LIGHTING_BOLT, 18 * IN_MILLISECONDS, 0, PHASE_ONE);
            events.ScheduleEvent(EVENT_CHANGE_WEAPON, 40 * IN_MILLISECONDS, 0, PHASE_ONE);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LIGHTING_BOLT:
                        Talk(0);
                        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 89148);
                        events.ScheduleEvent(EVENT_THUNDER_SMASH, 2 * IN_MILLISECONDS, 0, PHASE_ONE);
                        break;
                    case EVENT_THUNDER_SMASH:
                        events.ScheduleEvent(EVENT_THUNDER_SMASH, 10 * IN_MILLISECONDS, 0, PHASE_ONE);
                        DoCastVictim(SPELL_LIGHTING_STRIKE_TARGETTING);
                        DoCastVictim(SPELL_LIGHTING_STRIKE_2);
                        break;
                    case EVENT_CHANGE_WEAPON:
                        Talk(1);
                        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 82347);
                        events.SetPhase(PHASE_TWO);
                        DoCast(SPELL_THUNDER_SMASH_DUMMY);
                        events.ScheduleEvent(EVENT_LIGHTING_SMASH, 2 * IN_MILLISECONDS, 0, PHASE_TWO);
                        break;
                    case EVENT_LIGHTING_SMASH:
                        events.ScheduleEvent(EVENT_LIGHTING_SMASH, 15 * IN_MILLISECONDS, 0, PHASE_TWO);
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 10.0f, true))
                        {
                            me->CastSpell(target, SPELL_FACE_PLAYER);
                            me->CastSpell(target, SPELL_LIGHTING_SMASH);
                        }
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            instance->SetData(DATA_COMPLETE_EVENT_STAGE_1, SPECIAL);
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_forgemaster_vulkonAI>(creature);
    }
};

class npc_shanze_shadowcaster : public CreatureScript
{
public:
    npc_shanze_shadowcaster() : CreatureScript("npc_shanze_shadowcaster") { }

    struct npc_shanze_shadowcasterAI : public ScriptedAI
    {
        npc_shanze_shadowcasterAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);

            events.ScheduleEvent(EVENT_SHADOW_BOLT, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADOW_BOLT:
                        events.ScheduleEvent(EVENT_SHADOW_BOLT, 10 * IN_MILLISECONDS);
                        DoCast(SPELL_SHADOW_BOLT);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shanze_shadowcasterAI>(creature);
    }
};

class npc_shanze_battlemaster : public CreatureScript
{
public:
    npc_shanze_battlemaster() : CreatureScript("npc_shanze_battlemaster") { }

    struct npc_shanze_battlemasterAI : public ScriptedAI
    {
        npc_shanze_battlemasterAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(1);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);
            Talk(0);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            /*while (uint32 eventId = events.ExecuteEvent())
            {
            switch (eventId)
            {
            }
            }*/

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shanze_battlemasterAI>(creature);
    }
};

class npc_shanze_warrior : public CreatureScript
{
public:
    npc_shanze_warrior() : CreatureScript("npc_shanze_warrior") { }

    struct npc_shanze_warriorAI : public ScriptedAI
    {
        npc_shanze_warriorAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);
            Talk(0);

            events.ScheduleEvent(EVENT_LUMBERING_CHARGE, 3 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LUMBERING_CHARGE:
                        events.ScheduleEvent(EVENT_LUMBERING_CHARGE, 10 * IN_MILLISECONDS);
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 10.0f))
                            me->CastSpell(target, SPELL_LUMBERING_CHARGE);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shanze_warriorAI>(creature);
    }
};

class npc_shanze_electro_coutioner : public CreatureScript
{
public:
    npc_shanze_electro_coutioner() : CreatureScript("npc_shanze_electro_coutioner") { }

    struct npc_shanze_electro_coutionerAI : public ScriptedAI
    {
        npc_shanze_electro_coutionerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);
            Talk(0);

            events.ScheduleEvent(EVENT_LIGHTING_BOLT, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CALL_LIGHTING, 23 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LIGHTING_BOLT:
                        events.ScheduleEvent(EVENT_LIGHTING_BOLT, 10 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_LIGHTING_BOLT);
                        break;
                    case EVENT_CALL_LIGHTING:
                        DoCastVictim(SPELL_CALL_LIGHTING);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shanze_electro_coutionerAI>(creature);
    }
};

class npc_shanze_pyromancer : public CreatureScript
{
public:
    npc_shanze_pyromancer() : CreatureScript("npc_shanze_pyromancer") { }

    struct npc_shanze_pyromancerAI : public ScriptedAI
    {
        npc_shanze_pyromancerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);

            events.ScheduleEvent(EVENT_METEOR, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FIREBALL, 15 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_METEOR:
                        events.ScheduleEvent(EVENT_METEOR, 19 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_METEOR_TARGETING);
                        DoCastVictim(SPELL_METEOR_AT, true);
                        break;
                    case EVENT_FIREBALL:
                        events.ScheduleEvent(EVENT_FIREBALL, 10 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_FIREBALL);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_shanze_pyromancerAI>(creature);
    }
};

//< second room

class npc_celestial_blacksmith : public CreatureScript
{
public:
    npc_celestial_blacksmith() : CreatureScript("npc_celestial_blacksmith") { }

    struct npc_celestial_blacksmithAI : public CreatureAI
    {
        npc_celestial_blacksmithAI(Creature* creature) : CreatureAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->setRegeneratingHealth(false);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CB_START_MOVING:
                    events.ScheduleEvent(EVENT_LR_0, 2 * IN_MILLISECONDS);

                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 45123);
                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 94564);
                    me->CastSpell(me, SPELL_JOIN_PLAYER_PARTY);
                    instance->SetData(DATA_LR_STAGE_2, IN_PROGRESS);
                    DoCast(SPELL_DEACTIVATE_ALL_AVNILS);

                    if (Player* plr = me->FindNearestPlayer(200.0f))
                    {
                        plr->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1);
                        plr->AddAura(SPELL_THUNDER_FORGE_CHARGING, plr);
                    }
                    break;
                case ACTION_FIRST_WAVE:
                    SummonAdds(1);
                    SummonAdds(2);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();
            summons.DespawnAll();

            talk = false;
            dead = false;
            power = 0;
        }

        void DamageTaken(Unit* /*attacker*/, uint32 &damage)
        {
            if ((me->HealthBelowPctDamaged(25, damage) || (me->HealthBelowPctDamaged(50, damage))) && !dead)
            {
                dead = true;
                Talk(2);
            }
        }

        void EnterCombat(Unit* /*who*/)
        { }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);

            switch (summon->GetEntry())
            {
                case NPC_SHA_FIEND:
                    doAction(me, NPC_SHA_FIEND, ACTION_SHA_FIXATE);
                    break;
                case NPC_SHA_BEAST:
                    summon->GetMotionMaster()->MovePoint(1, 7368.795898f, 5160.241211f, 49.531322f);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LR_0:
                        t = 0;
                        events.ScheduleEvent(EVENT_LR_1, t += 0 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_2, t += 40 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_3, t += 32 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_4, t += 35 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_5, t += 35 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_6, t += 36 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_7, t += 34 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_1:
                        t += 5;
                        AvnilHelper(0);
                        break;
                    case EVENT_LR_2:
                        AvnilHelper(1);
                        break;
                    case EVENT_LR_3:
                        AvnilHelper(2);
                        break;
                    case EVENT_LR_4:
                        AvnilHelper(3);
                        break;
                    case EVENT_LR_5:
                        AvnilHelper(4);
                        break;
                    case EVENT_LR_6:
                        AvnilHelper(5);
                        break;
                    case EVENT_LR_7:
                        AvnilHelper(6);
                        if (power < 90)
                            events.ScheduleEvent(EVENT_LR_0, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_8:
                        if (Unit* stalker = me->FindNearestCreature(NPC_ANVIL_STALKER, 15.0f))
                            me->SetFacingTo(stalker);
                        me->AddAura(SPELL_FORGING, me);
                        break;
                    case EVENT_LR_9:
                    {
                        if (Unit* stalker = me->FindNearestCreature(NPC_ANVIL_STALKER, 15.0f))
                        {
                            me->AddAura(SPELL_ANVIL_ACTIVATE_COSMETIC_DND, stalker);
                            me->AddAura(SPELL_ACTIVATE_CLOSEST_AVNIL, stalker);
                        }

                        if (!talk)
                        {
                            talk = true;
                            Talk(0);
                        }
                        else
                            Talk(1);

                        if (Unit* plr = me->FindNearestPlayer(150.0f))
                            for (uint32 i = 0; i < 5; i++)
                            {
                                plr->CastSpell(plr, SPELL_THUNDER_FORGE_CHARGE_TRIGGER);
                                ++power;
                            }

                        switch (power)
                        {
                            case 5:
                            case 15:
                            case 25:
                            case 35:
                            case 45:
                                SummonAdds(1);
                                break;
                            case 10:
                            case 20:
                            case 30:
                            case 40:
                                SummonAdds(2);
                                break;
                            case 50:
                                doAction(me, NPC_WRATHION, ACTION_M_ENERGY);
                                break;
                            case 90:
                                if (Unit* stalker = me->SummonCreature(NPC_LIGHTING_SPEAR_FLOAT_STALKER, 7368.375f, 5181.912f, 52.79837f))
                                    if (Unit* lance = me->SummonCreature(NPC_LIGHTING_LANCE, 7368.375f, 5181.912f, 55.04837f))
                                        stalker->CastSpell(lance, VEHICLE_SPELL_RIDE_HARDCODED);
                                break;
                            case 100:
                                if (Player* plr = me->FindNearestPlayer(200.0f))
                                    plr->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGING);
                                events.CancelEvent(EVENT_LR_0);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void SummonAdds(uint8 type)
        {
            switch (type)
            {
                case 1:
                    for (uint32 i = 0; i < 8; i++)
                        me->SummonCreature(NPC_SHA_FIEND, shaFinedsPositions[urand(0, 79)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15 * IN_MILLISECONDS);
                    break;
                case 2:
                    me->SummonCreature(NPC_SHA_BEAST, shaBeastPositions[urand(0, 22)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->SetData(DATA_LR_STAGE_2, FAIL);
            summons.DespawnAll();
        }

        void AvnilHelper(uint8 ID)
        {
            Movement::MoveSplineInit init(*me);
            init.MoveTo(cBlacksmithPositions[ID].GetPositionX(), cBlacksmithPositions[ID].GetPositionY(), cBlacksmithPositions[ID].GetPositionZ(), true);
            init.SetSmooth();
            init.Launch();

            events.ScheduleEvent(EVENT_LR_8, 8 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_LR_9, 35 * IN_MILLISECONDS);
        }

    private:
        InstanceScript* instance;
        EventMap events;
        uint32 t;
        bool talk;
        bool dead;
        uint32 power;
        SummonList summons;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_celestial_blacksmithAI>(creature);
    }
};

class npc_celestial_defender : public CreatureScript
{
public:
    npc_celestial_defender() : CreatureScript("npc_celestial_defender") { }

    struct npc_celestial_defenderAI : public CreatureAI
    {
        npc_celestial_defenderAI(Creature* creature) : CreatureAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->setRegeneratingHealth(false);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CB_START_MOVING:
                    events.ScheduleEvent(EVENT_LR_1, 1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_LR_2, 3 * IN_MILLISECONDS);
                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 94156);
                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 94156);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.CancelEvent(EVENT_LR_3);
            events.ScheduleEvent(EVENT_LR_5, 15 * IN_MILLISECONDS);
        }

        void EnterEvadeMode()
        {
            events.ScheduleEvent(EVENT_LR_3, 10 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LR_1:
                        me->AddAura(SPELL_ASTRAL_ENDURANCE, me);
                        me->CastSpell(me, SPELL_JOIN_PLAYER_PARTY);

                        if (Player* plr = me->FindNearestPlayer(200.0f))
                            if (plr->GetRoleForGroup(plr->GetActiveSpec()) == ROLES_HEALER)
                                me->CastSpell(me, SPELL_DAMAGE_SELF_50_PERCENT);
                        break;
                    case EVENT_LR_2:
                        me->GetMotionMaster()->MovePoint(1, 7390.238770f, 5182.048340f, 49.603447f);
                        events.ScheduleEvent(EVENT_LR_3, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_3:
                    {
                        events.ScheduleEvent(EVENT_LR_3, 20 * IN_MILLISECONDS);

                        Movement::MoveSplineInit init(*me);
                        for (uint8 i = 0; i < 10; ++i)
                            init.Path().push_back(dPoints3[i]);
                        init.SetSmooth();
                        init.SetCyclic();
                        init.Launch();
                        break;
                    }
                    case EVENT_LR_5:
                        events.ScheduleEvent(EVENT_LR_5, 40 * IN_MILLISECONDS);
                        me->AddAura(SPELL_POWER_SURGE, me);
                        Talk(0);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(2);
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_celestial_defenderAI>(creature);
    }
};

class npc_sha_beast : public CreatureScript
{
public:
    npc_sha_beast() : CreatureScript("npc_sha_beast") { }

    struct npc_sha_beastAI : public ScriptedAI
    {
        npc_sha_beastAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_EMPOWERED, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DARK_BITE, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_LETHARGY, 25 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_ABSORB_EVIL, 40 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EMPOWERED:
                        events.ScheduleEvent(EVENT_EMPOWERED, 19 * IN_MILLISECONDS);
                        DoCast(SPELL_EMPOWERED);
                        break;
                    case EVENT_DARK_BITE:
                        events.ScheduleEvent(EVENT_DARK_BITE, 25 * IN_MILLISECONDS);
                        DoCast(SPELL_DARK_BITE);
                        break;
                    case EVENT_LETHARGY:
                        events.ScheduleEvent(EVENT_LETHARGY, 40 * IN_MILLISECONDS);
                        DoCast(SPELL_LETHARGY);
                        break;
                    case EVENT_ABSORB_EVIL:
                        events.ScheduleEvent(EVENT_ABSORB_EVIL, 45 * IN_MILLISECONDS);
                        DoCast(SPELL_ABSORB_EVIL);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_sha_beastAI>(creature);
    }
};

class npc_sha_fiend : public CreatureScript
{
public:
    npc_sha_fiend() : CreatureScript("npc_sha_fiend") { }

    struct npc_sha_fiendAI : public ScriptedAI
    {
        npc_sha_fiendAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_DARK_SUNDER, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHA_BLAST, 15 * IN_MILLISECONDS);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_SHA_FIXATE:
                    events.ScheduleEvent(EVENT_SMALL_SHA_FIXATE, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SMALL_SHA_FIXATE:
                    {
                        Unit* target = me->FindNearestCreature(NPC_CELESTIAL_BLACKSMITH, 250.0f);
                        if (!target)
                            return;

                        if (me->GetDistance2d(target) > 8.0f)
                        {
                            me->GetMotionMaster()->MovePoint(1, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                            me->AddAura(SPELL_SMALL_SHA_FIXATE, target);
                        }
                        else
                        {
                            me->SetReactState(REACT_AGGRESSIVE);
                            AttackStart(SelectTarget(SELECT_TARGET_RANDOM));
                            me->RemoveAura(SPELL_SMALL_SHA_FIXATE);
                        }
                        break;
                    }
                    case EVENT_DARK_SUNDER:
                        events.ScheduleEvent(EVENT_DARK_SUNDER, 15 * IN_MILLISECONDS);
                        if (Unit* target = me->FindNearestCreature(NPC_CELESTIAL_BLACKSMITH, 150.0f))
                            me->CastSpell(target, SPELL_DARK_SUNDER);
                        break;
                    case EVENT_SHA_BLAST:
                        events.ScheduleEvent(EVENT_SHA_BLAST, 10 * IN_MILLISECONDS);
                        DoCast(SPELL_SHA_BLAST);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_sha_fiendAI>(creature);
    }
};

class npc_sha_amalgamation : public CreatureScript
{
public:
    npc_sha_amalgamation() : CreatureScript("npc_sha_amalgamation") { }

    struct npc_sha_amalgamationAI : public ScriptedAI
    {
        npc_sha_amalgamationAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        {
            doAction(me, NPC_WRATHION, ACTION_SCENARIO_COMPLETED);
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_NSANITY, 1 * MINUTE * IN_MILLISECONDS + 30 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHADOW_BURST, urand(40, 50) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DARK_BINDING, urand(15, 30) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_METEOR_STORM, 6 * MINUTE);
            events.ScheduleEvent(EVENT_SHADOW_CRASH, 2 * MINUTE);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_NSANITY:
                        events.ScheduleEvent(EVENT_NSANITY, 2 * MINUTE);
                        DoCast(SPELL_INSANITY);
                        Talk(0);
                        break;
                    case EVENT_SHADOW_BURST:
                        DoCast(SPELL_SHADOW_BURST);
                        events.ScheduleEvent(EVENT_SHADOW_BURST, urand(40, 50) * IN_MILLISECONDS);
                        break;
                    case EVENT_DARK_BINDING:
                        events.ScheduleEvent(EVENT_DARK_BINDING, urand(30, 60) * IN_MILLISECONDS);
                        if (Unit* defender = me->FindNearestCreature(NPC_CELESTIAL_DEFENDER, 150.0f))
                        {
                            me->CastSpell(defender, SPELL_DARK_BINDING);
                            Talk(1);
                        }
                        break;
                    case EVENT_METEOR_STORM:
                        me->AddAura(SPELL_METEOR_STORM, me);
                        break;
                    case EVENT_SHADOW_CRASH:
                        events.ScheduleEvent(EVENT_SHADOW_CRASH, 1 * MINUTE * IN_MILLISECONDS + 30 * IN_MILLISECONDS);
                        DoCast(SPELL_SHADOW_CRASH);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_sha_amalgamationAI>(creature);
    }
};

class TargetFilter
{
public:
    TargetFilter(uint32 entry) { ID = entry; }
    bool operator()(WorldObject* obj) const
    {
        if (!obj->ToCreature())
            return true;

        return (obj->ToCreature()->GetEntry() != ID);
    }

private:
    uint32 ID;
};

class spell_phase_shift_update : public SpellScriptLoader
{
public:
    spell_phase_shift_update() : SpellScriptLoader("spell_phase_shift_update") { }

    class spell_phase_shift_update_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_phase_shift_update_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            return;
            /* dead code
            targets.clear();

            for (int8 i = 0; i < 32; i++)
            targets.remove_if(TargetFilter(phasingTargets[i]));
            */
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_phase_shift_update_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_phase_shift_update_SpellScript();
    }
};

class spell_thundder_forge_charging : public SpellScriptLoader
{
public:
    spell_thundder_forge_charging() : SpellScriptLoader("spell_thundder_forge_charging") { }

    class spell_thundder_forge_charging_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_thundder_forge_charging_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            if (InstanceScript* instance = target->GetInstanceScript())
            {
                Creature* beam = target->FindNearestCreature(NPC_LIGHTING_PILAR_BEAM_STALKER, 200.0f);
                if (!beam)
                    return;

                Creature* spark = target->FindNearestCreature(NPC_LIGHTING_PILAR_SPARK_STALKER, 200.0f);
                if (!spark)
                    return;

                switch (target->GetPower(POWER_ALTERNATE_POWER))
                {
                    case 50:
                        beam->AI()->DoAction(ACTION_CHARGING_1);
                        spark->AI()->DoAction(ACTION_CHARGING_1);
                        break;
                    case 100:
                        beam->AI()->DoAction(ACTION_CHARGING_2);
                        spark->AI()->DoAction(ACTION_CHARGING_2);
                        break;
                    case 150:
                        beam->AI()->DoAction(ACTION_CHARGING_3);
                        spark->AI()->DoAction(ACTION_CHARGING_3);
                        break;
                    case 200:
                    {
                        doAction(beam, NPC_SHADO_PAN_DEFENDER, ACTION_CANCEL_SUMMONS);
                        beam->AI()->DoAction(ACTION_CHARGING_4);
                        for (auto const& itr : target->GetMap()->GetPlayers())
                        {
                            itr.getSource()->SetPower(POWER_ALTERNATE_POWER, 0);
                            itr.getSource()->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1);
                            itr.getSource()->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGE);
                            itr.getSource()->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGE_TRIGGER);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_thundder_forge_charging_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_ENERGIZE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_thundder_forge_charging_AuraScript();
    }
};

class spell_forging : public SpellScriptLoader
{
public:
    spell_forging() : SpellScriptLoader("spell_forging") { }

    class spell_forging_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_forging_AuraScript);

        void OnTick(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->CastSpell(caster, SPELL_STRIKE_ANVIL_COSMETIC);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_forging_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_forging_AuraScript();
    }
};

class spell_avnil_click_dummy : public SpellScriptLoader
{
public:
    spell_avnil_click_dummy() : SpellScriptLoader("spell_avnil_click_dummy") { }

    class spell_avnil_click_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_avnil_click_dummy_SpellScript);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (Unit* stalker = caster->FindNearestCreature(NPC_ANVIL_STALKER, 20.0f))
            {
                stalker->CastSpell(stalker->GetPositionX(), stalker->GetPositionY(), stalker->GetPositionZ(), SPELL_THUNDER_SURGE);
                stalker->RemoveAllAuras();
            }
        }

        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_avnil_click_dummy_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_avnil_click_dummy_SpellScript();
    }
};

void AddSC_fall_of_shan_bu()
{
    new npc_wrathion();

    new go_mogu_crucible();

    new npc_shado_pan_defender();
    new npc_shado_pan_warrior();
    new npc_thunder_forge_second();
    new npc_invisible_hunter();
    new npc_lighting_pilar_beam_stalker();
    new npc_lighting_pilar_spark_stalker();
    new npc_forgemaster_vulkon();
    new npc_shanze_shadowcaster();
    new npc_shanze_battlemaster();
    new npc_shanze_warrior();
    new npc_shanze_electro_coutioner();
    new npc_shanze_pyromancer();
    new npc_celestial_blacksmith();
    new npc_celestial_defender();
    new npc_sha_beast();
    new npc_sha_fiend();
    new npc_sha_amalgamation();

    new spell_phase_shift_update();
    new spell_thundder_forge_charging();
    new spell_forging();
    new spell_avnil_click_dummy();
}
