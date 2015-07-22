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

#include "NewScriptPCH.h"
#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MoveSplineInit.h"

#ifndef DEF_FALL_OF_SHANBU
#define DEF_FALL_OF_SHANBU

enum Data
{
    DATA_NONE,

    DATA_PLAYER_ROLE,

    DATA_MOGU_CRUCIBLE,
    DATA_ALLOWED_STAGE,
    DATA_WAVE_COUNTER,
    DATA_THUNDER_FORGE,


    DATA_TRUNDER_FORGE_DOOR,
    DATA_START_EVENT,
    DATA_EVENT_PART_1,
    DATA_EVENT_PART_2,
    DATA_JUMP_POS,
    DATA_COMPLETE_EVENT_STAGE_1,
    DATA_SUMMONS_COUNTER,

    DATA_LR_START,
    DATA_LR_STAGE_2,
};

enum Actions
{

    ACTION_NONE,

    ACTION_WRATHION_START,
    ACTION_1,
    ACTION_2,
    ACTION_3,


    ACTION_INISIBLE_HUNTER_AURA,
    ACTION_CHARGING_1,
    ACTION_CHARGING_2,
    ACTION_CHARGING_3,
    ACTION_CHARGING_4,
    ACTION_EVADE,
    ACTION_COMPLETE_FIRST_PART,
    ACTION_LR_P1,
    ACTION_CB_START_MOVING,
    ACTION_FORGE_CAST,
    ACTION_CANCEL_FORGE_EVENTS,
    ACTION_M_ENERGY,
    ACTION_SHA_FIXATE,
    ACTION_FIRST_WAVE,

    ACTION_SCENARIO_COMPLETED
};

enum eCreatures
{
    NPC_WRATHION                        = 70100,
    NPC_SHADO_PAN_WARRIOR               = 70106, //< x2
    NPC_SHADO_PAN_DEFENDER              = 70099,

    NPC_THUNDER_FORGE                   = 70577,
    NPC_THUNDER_FORGE_3                 = 70292,
    NPC_THUNDER_FORGE2                  = 70283,
    NPC_THUNDER_FORGE_CRUCIBLE          = 70556,

    NPC_FORGEMASTER_VULKON              = 70074,

    NPC_SHANZE_SHADOWCASTER             = 69827,
    NPC_SHANZE_WARRIOR                  = 69833,
    NPC_SHANZE_BATTLEMASTER             = 69835,
    NPC_SHANZE_ELECTRO_CUTIONER         = 70070,
    NPC_SHANZE_ELECTRO_CUTIONER2        = 69216,
    NPC_SHANZE_PYROMANCER               = 69824,

    NPC_INVISIBLE_STALKER               = 62142,
    NPC_SPIRIT_HEALER                   = 65183,
    NPC_JUVENILE_SKYSCREAMER            = 69162,
    NPC_ZANALARI_COMMONER               = 69170,
    NPC_DRAKKARI_GOD_HULK               = 69200,
    NPC_MANFRED                         = 69217,
    NPC_ZANALARI_STONE_SHIELD           = 69223,
    NPC_FIERCE_ANKLEBITER               = 69244,
    NPC_ZANDALARI_PROSPECT              = 69269,
    NPC_ZANDALARI_BEASTCALLER           = 69379,
    NPC_ZANDALARI_BEASTCALLER2          = 69397,
    NPC_JUVENILE_SKYSCREAMER2           = 69404,
    NPC_ZANDALARI_BEASTCALLER3          = 69412,
    NPC_LORTHEMAR_THERON                = 69481,
    NPC_LIGHTING_PILAR_BEAM_STALKER     = 69798,
    NPC_LIGHTING_PILAR_SPARK_STALKER    = 69813,
    NPC_SLAVEMASTER_SHIAXU              = 69923,
    NPC_SCOUT_CAPTAIN_ELSIA             = 70042,
    NPC_NALAK                           = 69099,
    NPC_CONSTELLATION                   = 70058,
    NPC_COSMETIC_SHA_BOSS               = 70449,
    NPC_METEOR_SUMMONER_STALKER         = 70299,

    //< VEH
    NPC_ZANDALARI_SKYSCREAMER           = 69156,
    NPC_ZANDALARI_SKYSCREAMER2          = 69411,
    NPC_THUNDERWING                     = 69509,

    //< second room
    NPC_CELESTIAL_BLACKSMITH            = 69828,
    NPC_CELESTIAL_DEFENDER              = 69837,

    NPC_PHASE3_ROOM_CENTER_STALKER      = 70481,
    NPC_ANVIL_STALKER                   = 70079,
    NPC_THUNDER_FORGE_2                 = 70061,
    NPC_LIGHTING_SPEAR_FLOAT_STALKER    = 70500,
    NPC_LIGHTING_LANCE                  = 70460,

    NPC_SHA_AMALGAMATION                = 70228,
    NPC_SHA_FIEND                       = 70039,
    NPC_SHA_BEAST                       = 70048,
};

enum eGameObects
{
    GO_MOGU_CRICUBLE            = 218910,
    GO_THUNDER_FORGE_DOOR       = 218832,
    GO_THUNDER_FORGE_AVNIL      = 218741,
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

enum Stages
{
    STAGE_NONE,

    STAGE_1,
    STAGE_2,
    STAGE_3,
    STAGE_4,
    STAGE_5,
    STAGE_6,
    STAGE_7,
    STAGE_8,
    STAGE_LAST
};
namespace Helper{
    bool IsNextStageAllowed(InstanceScript* instance, uint8 stage);}
void doAction(Unit* unit, uint32 creature, uint8 action, float range = 200.0f);

#endif
