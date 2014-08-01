/*
 * Copyright (C) 2011-2012 Haloperidolum <http://wow-mig.ru/>
 * This is private source based on TrinityCore
 * for WoW-Mig project
 * This is no GPL code.
 */

#ifndef BATTLEFIELD_TB_
#define BATTLEFIELD_TB_

#include "../Battlefield.h"
#include "Group.h"
#include "WorldPacket.h"
#include "World.h"
#include "ObjectMgr.h"

const uint32 ClockBTWorldState[2] = {5333,5332};
const uint32 TolBaradFaction[3] = {1732, 1735, 35};

#define POS_Z_TOWER 150.0f
#define POS_X_START -900.0f

class BattlefieldTB;
class BfCapturePointTB;

struct BfTBGameObjectBuilding;
struct BfTBWorkShopData;

typedef std::set<BfTBGameObjectBuilding*> TbGameObjectBuilding;
typedef std::set<BfTBWorkShopData*> TbWorkShop;
typedef std::set<BfCapturePointTB*> BfCapturePointSet;
typedef std::set<Group*> GroupSet;

enum eTBPhasemask
{
    PHASEMASK_ALLIANCE_DEF = 2,
    PHASEMASK_HORDE_DEF = 4,
};

enum eTBData32
{
    BATTLEFIELD_TB_DATA_CAPTURED,
    BATTLEFIELD_TB_DATA_DESTROYED,
    BATTLEFIELD_TB_DATA_MAX,
};

enum eCapturePointsWSDiff
{
    HORDE_DEFENCE = 0,
    HORDE_ATTACK,
    NEUTRAL,
    ALLIANCE_ATTACK,
    ALLIANCE_DEFENCE,
    MAX_CP_DIFF
};

enum eBuildingsWSDiff
{
    BUILDING_HORDE_DEFENCE = 0,
    BUILDING_HORDE_DEFENCE_DAMAGED,
    BUILDING_DESTROYED,
    BUILDING_ALLIANCE_DEFENCE,
    BUILDING_ALLIANCE_DEFENCE_DAMAGED,
    BUILDING_MAX_DIFF
};

enum eWorldStates
{
    WS_TB_BATTLE_TIMER_ENABLED                      = 5346,
    WS_TB_BATTLE_TIMER                              = 5333,
    WS_TB_COUNTER_BUILDINGS                         = 5348,
    WS_TB_COUNTER_BUILDINGS_ENABLED                 = 5349,
    WS_TB_HORDE_DEFENCE                             = 5384,
    WS_TB_ALLIANCE_DEFENCE                          = 5385,
    WS_TB_NEXT_BATTLE_TIMER                         = 5332,
    WS_TB_NEXT_BATTLE_TIMER_ENABLED                 = 5387,

    WS_TB_SOUTH_CAPTURE_POINT                       = 5418,
    WS_TB_EAST_CAPTURE_POINT                        = 5428,
    WS_TB_WEST_CAPTURE_POINT                        = 5423,

    WS_TB_EAST_SPIRE                                = 5433,
    WS_TB_SOUTH_SPIRE                               = 5438,
    WS_TB_WEST_SPIRE                                = 5443,
    
    WS_TB_KEEP_HORDE_DEFENCE                        = 5469,
    WS_TB_KEEP_ALLIANCE_DEFENCE                     = 5470,
    
    WS_TB_ALLIANCE_ATTACK                           = 5546,
    WS_TB_HORDE_ATTACK                              = 5547,
};

enum eTBpell
{
    SPELL_TB_SPIRITUAL_IMMUNITY                     = 95332,
    SPELL_TB_ALLIANCE_FLAG                          = 14268,
    SPELL_TB_HORDE_FLAG                             = 14267,
    SPELL_TB_VETERAN                                = 84655,

    // Reward spells
    SPELL_TB_VICTORY_REWARD_ALLIANCE                = 89789,
    SPELL_TB_VICTORY_REWARD_HORDE                   = 89791,
    SPELL_TB_LOOSER_REWARD                          = 89793,

    SPELL_TB_TOL_BARAD_TOWER_DESTROYED              = 89796,
    SPELL_TB_TOL_BARAD_TOWER_DAMAGED                = 89795,
    SPELL_TB_TOL_BARAD_TOWER_DEFENDED               = 89794,

    SPELL_PHASE_TB_NON_BATTLE                       = 64576, // alow see trash out of battle
};

const Position TbDefencerStartPosition[5] = 
{
    {-1300.45f,1034.45f,145.11f,5.88f},
    {-1285.40f, 923.85f,145.11f,5.88f},
    {-1174.90f, 925.72f,145.11f,2.93f},
    {-1182.56f,1040.25f,145.11f,2.93f},
    {-802.728f,1187.91f,110.65f,3.12f},
};

/*#########################
*####### Graveyards ######*
#########################*/

class BfGraveYardTB: public BfGraveyard
{
public:
    BfGraveYardTB(BattlefieldTB* Bf);
    void SetTextId(uint32 textid){m_GossipTextId=textid;}
    uint32 GetTextId(){return m_GossipTextId;}
protected:
    uint32 m_GossipTextId;
};

enum eTBGraveyardId
{
    BATTLEFIELD_TB_GY_BARADIN_HOLD,
    BATTLEFIELD_TB_GY_EAST_SPIRE,
    BATTLEFIELD_TB_GY_IRONCLAD_GAMSON,
    BATTLEFIELD_TB_GY_SLAGWORKS,
    BATTLEFIELD_TB_GY_SOUTH_SPIRE,
    BATTLEFIELD_TB_GY_WARDENS_VIGIL,
    BATTLEFIELD_TB_GY_WEST_SPIRE,
    BATTLEFIELD_TB_GY_MAX,
};

enum eTBGossipText // TODO
{
    BATTLEFIELD_TB_GOSSIPTEXT_GY_WARDENS_VIGIL    = -1850501,
    BATTLEFIELD_TB_GOSSIPTEXT_GY_IRONGLAD         = -1850502,
    BATTLEFIELD_TB_GOSSIPTEXT_GY_SLAGWORKS        = -1850504,
    BATTLEFIELD_TB_GOSSIPTEXT_GY_NORTH_WEST       = -1850503,
    BATTLEFIELD_TB_GOSSIPTEXT_GY_KEEP             = -1850500,
    BATTLEFIELD_TB_GOSSIPTEXT_GY_SOUTH            = -1850505,
    BATTLEFIELD_TB_GOSSIPTEXT_GY_NORTH_EAST       = -1850506,
};

enum eTBNpc
{
    BATTLEFIELD_TB_NPC_GUARD_H                    = 51166,
    BATTLEFIELD_TB_NPC_GUARD_A                    = 51165,
    BATTLEFIELD_TB_NPC_HUNTER_A                   = 47595,
    BATTLEFIELD_TB_NPC_MAGE_A                     = 47598,
    BATTLEFIELD_TB_NPC_PALADIN_A                  = 47600,
    BATTLEFIELD_TB_NPC_DRUID_H                    = 47607,
    BATTLEFIELD_TB_NPC_MAGE_H                     = 47608,
    BATTLEFIELD_TB_NPC_ROGUE_H                    = 47609,
    BATTLEFIELD_TB_NPC_SHAMAN_H                   = 47610,
};

struct BfTBCoordGY
{
    float x;
    float y;
    float z;
    float o;
    uint32 gyid;
    uint8 type;
    uint32 entrya;
    uint32 entryh;
};

const BfTBCoordGY TBGraveYard[BATTLEFIELD_TB_GY_MAX] =
{
    {-1244.58f,981.233f,155.425f,0.733038f,1789,BATTLEFIELD_TB_GY_BARADIN_HOLD,    45070, 45078}, // entry not fould in wowhead
    {-944.34f, 576.111f,157.543f,5.317802f,1788,BATTLEFIELD_TB_GY_EAST_SPIRE,      45070, 45078},
    {-970.465f,1088.33f,132.992f,6.268125f,1783,BATTLEFIELD_TB_GY_IRONCLAD_GAMSON, 45068, 45074},
    {-1343.65f,568.823f,139.158f,0.008500f,1787,BATTLEFIELD_TB_GY_SLAGWORKS,       45073, 45076},
    {-1600.28f,869.21f, 193.948f,4.819249f,1786,BATTLEFIELD_TB_GY_SOUTH_SPIRE,     45071, 45079},
    {-1572.14f,1169.94f,159.501f,3.971623f,1785,BATTLEFIELD_TB_GY_WARDENS_VIGIL,   45069, 45075},
    {-1052.1f, 1490.65f,191.407f,1.972220f,1332,BATTLEFIELD_TB_GY_WEST_SPIRE,      45072, 45077},
};

/*#########################
* BfCapturePointTB       *
#########################*/

class BfCapturePointTB: public BfCapturePoint
{
public:
    BfCapturePointTB(BattlefieldTB* bf,TeamId control);
    void LinkToWorkShop(BfTBWorkShopData* ws) { m_WorkShop = ws; }

    void ChangeTeam(TeamId oldteam);
    TeamId GetTeam() const { return m_team; }
    void SetTeam(TeamId team) { m_team = team; }

    void UpdateCapturePointValue()
    {
        if (m_team == TEAM_ALLIANCE)
        {
            m_value = m_maxValue;
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE;
        }
        else 
        {
            m_value = -m_maxValue;
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE;
        }
    }

protected:
    BfTBWorkShopData* m_WorkShop;
};

/*#########################
* TolBarad Battlefield    *
#########################*/

class BattlefieldTB: public Battlefield
{
public:
    void OnBattleStart();
    void OnBattleEnd(bool endbytimer);
    bool Update(uint32 diff);
    void OnCreatureCreate(Creature *creature, bool add);
    void BrokenWallOrTower(TeamId team);
    void AddDamagedTower(TeamId team);
    void AddBrokenTower(TeamId team);
    void AddPlayerToResurrectQueue(ObjectGuid npc_guid, ObjectGuid player_guid);
    bool SetupBattlefield();

    void RewardMarkOfHonor(Player *plr, uint32 count);
    
    void UpdateVehicleCountTB();

    void FillInitialWorldStates(WorldPacket &p);

    void HandleKill(Player *killer, Unit *victim);

    void PromotePlayer(Player *killer);
    void CapturePoint(uint32 team);
    
    void OnDestroyed();
    void OnDamaged();
    
    void OnPlayerJoinWar(Player* player);
    void OnPlayerLeaveZone(Player* plr);
    void OnPlayerLeaveWar(Player* plr);
    void OnPlayerEnterZone(Player* player);

    void ProcessEvent(GameObject *obj, uint32 eventId);

    /**
        * \brief Called when a gameobject is created
        */
    void OnGameObjectCreate(GameObject* go);

protected:
    TbGameObjectBuilding BuildingsInZone;
    GuidSet Vehicles;
    GuidSet questgiversA;
    GuidSet questgiversH;
    GuidSet npcAlliance;
    GuidSet npcHorde;
    GuidSet goDoors;
    GuidSet OutsideCreature[2];
    TbWorkShop WorkshopsList; 
    BfCapturePointSet CapturePoints;
    GuidSet m_PlayersIsSpellImu;  //Player is dead
    uint32 m_saveTimer;
};

enum eTBGameObjectBuildingType // Correct this.
{
    BATTLEFIELD_TB_OBJECTTYPE_TOWER,
};

enum eTBGameObjectState // Correct this.
{
    BATTLEFIELD_TB_OBJECTSTATE_HORDE_INTACT,
    BATTLEFIELD_TB_OBJECTSTATE_HORDE_DAMAGE,
    BATTLEFIELD_TB_OBJECTSTATE_HORDE_DESTROY,
    BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_INTACT,
    BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_DAMAGE,
    BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_DESTROY,
};

enum eTBTeamControl
{
    BATTLEFIELD_TB_TEAM_ALLIANCE,
    BATTLEFIELD_TB_TEAM_HORDE,
    BATTLEFIELD_TB_TEAM_NEUTRAL,    
};

enum eTBText
{
    BATTLEFIELD_TB_TEXT_IRONCLAD_GARRION_1 = 12100,
    BATTLEFIELD_TB_TEXT_WARDENS_VIGIL_1 = 12101,
    BATTLEFIELD_TB_TEXT_SLAGWORKS_1 = 12102,
    BATTLEFIELD_TB_TEXT_ALLIANCE_TAKEN = 12103,
    BATTLEFIELD_TB_TEXT_HORDE_TAKEN = 12104,
    BATTLEFIELD_TB_TEXT_ALLIANCE_LOSE = 12105,
    BATTLEFIELD_TB_TEXT_HORDE_LOSE = 12106,
    BATTLEFIELD_TB_TEXT_WILL_START = 12107,
    BATTLEFIELD_TB_TEXT_START = 12108,
    BATTLEFIELD_TB_TEXT_TOWER_1 = 12109,
    BATTLEFIELD_TB_TEXT_TOWER_2 = 12110,
    BATTLEFIELD_TB_TEXT_TOWER_3 = 12111,
    BATTLEFIELD_TB_TEXT_TOWER_DAMAGE = 12112,
    BATTLEFIELD_TB_TEXT_TOWER_DESTROY = 12113,
    BATTLEFIELD_TB_TEXT_ALLIANCE_TAKEN_TOLBARAD = 12114,
    BATTLEFIELD_TB_TEXT_HORDE_TAKEN_TOLBARAD = 12115,
    BATTLEFIELD_TB_TEXT_ALLIANCE_DEF_TOLBARAD = 12116,
    BATTLEFIELD_TB_TEXT_HORDE_DEF_TOLBARAD = 12117,
    BATTLEFIELD_TB_TEXT_IRONCLAD_GARRION_2 = 12118,
    BATTLEFIELD_TB_TEXT_WARDENS_VIGIL_2 = 12119,
    BATTLEFIELD_TB_TEXT_SLAGWORKS_2 = 12120,
    BATTLEFIELD_TB_TEXT_TEMPLATE = 12121,
};

enum eTBObject
{
    GAMEOBJECT_TB_NORTH_CAPTURE_POINT_AD         = 205068,
    GAMEOBJECT_TB_WEST_CAPTURE_POINT_AD          = 205101,
    GAMEOBJECT_TB_EAST_CAPTURE_POINT_AD          = 205138,
    GAMEOBJECT_TB_NORTH_CAPTURE_POINT_HD         = 205096,
    GAMEOBJECT_TB_WEST_CAPTURE_POINT_HD          = 205103,
    GAMEOBJECT_TB_EAST_CAPTURE_POINT_HD          = 205139,
    GAMEOBJECT_TB_SOUTH_SPIRE                    = 204590,
    GAMEOBJECT_TB_WEST_SPIRE                     = 204588,
    GAMEOBJECT_TB_EAST_SPIRE                     = 204589,
};

struct BfTBObjectPosition
{
    float x;
    float y;
    float z;
    float o;
    uint32 entrya;
    uint32 entryh;
};

//*********************************************************
//************Destructible (Wall,Tower..)******************
//*********************************************************

struct BfTBBuildingSpawnData
{
    uint32 entry;
    uint32 WorldState;
    float x;
    float y;
    float z;
    float o;
    uint32 type;
    uint32 nameid;
};

#define TB_MAX_OBJ 3
const BfTBBuildingSpawnData TBGameObjectBuillding[TB_MAX_OBJ] = 
{
    // Wall
    // Entry                    WS    X          Y         Z         O          type                             NameID
    {GAMEOBJECT_TB_SOUTH_SPIRE, 5438, -1618.91f, 954.542f, 168.601f, 0.069812f, BATTLEFIELD_TB_OBJECTTYPE_TOWER, 0},
    {GAMEOBJECT_TB_WEST_SPIRE,  5433, -950.41f,  1469.1f,  176.596f, -2.10312f, BATTLEFIELD_TB_OBJECTTYPE_TOWER, 0},
    {GAMEOBJECT_TB_EAST_SPIRE,  5443, -1013.28f, 529.538f, 146.427f, 1.97222f,  BATTLEFIELD_TB_OBJECTTYPE_TOWER, 0},
};

const BfTBObjectPosition TBGameobjectsDoor[3] =
{
    {-1086.686f,   1150.33f,     125.7371f,   1.535887f,     206844, 0},
    {-1233.360f,   783.5536f,    125.2064f,   -0.008726f,    206843, 0},
    {-1204.353f,   1075.035f,    123.6819f,   0.026179f,     206576, 0},
};

#define TB_MAX_DESTROY_MACHINE_NPC          6
const BfTBObjectPosition TBDestroyMachineNPC[TB_MAX_DESTROY_MACHINE_NPC] =
{
    {-1438.3f,     1095.24f,     121.136f,    5.28f,     45344, 0},
    {-1213.01f,    782.236f,     121.4473f,   1.67f,     45344, 0},
    {-1442.3f,     1141.07f,     123.6323f,   4.24f,     45344, 0},
    {-1108.52f,    1111.33f,     121.2783f,   1.37f,     45344, 0},
    {-1258.26f,    780.497f,     122.4413f,   1.48f,     45344, 0},
    {-1106.57f,    1196.34f,     121.802f,    0.40f,     45344, 0},
};

const BfTBObjectPosition QuestGivers[21] =
{
    {-1297.28f,     1039.83f,     119.878f,      5.45916f,        48066, 48069}, // static
    {-1300.23f,     1035.44f,     120.141f,      5.52279f,        48074, 48062},
    {-1302.61f,     1031.08f,     120.33f,       5.85108f,        48039, 48071},
    {-1303.48f,     1026.9f,      120.637f,      5.91783f,        48061, 48070},
    // guards
    {-1217.08f, 905.762f, 120.342f, 4.33694f,        51165, 51166},
    {-1166.84f, 999.469f, 119.712f, 5.44827f,        51165, 51166},
    {-1155.08f, 1006.99f, 119.495f, 5.8606f,        51165, 51166},
    {-1152.4f, 962.582f, 119.496f, 0.00153065f,        51165, 51166},
    {-1153.82f, 972.416f, 119.933f, 6.1574f,        51165, 51166},
    {-1165.99f, 970.111f, 119.515f, 0.00379658f,        51165, 51166},
    {-1155.5f, 997.169f, 119.97f, 6.27913f,        51165, 51166},
    {-1185.93f, 939.158f, 119.75f, 6.24082f,        51165, 51166},
    {-1235.69f, 910.97f, 119.727f, 4.68259f,        51165, 51166},
    {-1160.57f, 980.582f, 119.938f, 6.24162f,        51165, 51166},
    {-1159.53f, 988.522f, 120.06f, 6.14223f,        51165, 51166},
    {-1160.88f, 945.342f, 119.495f, 2.86597f,        51165, 51166},
    {-1173.94f, 914.448f, 119.628f, 2.31932f,        51165, 51166},
    {-1186.55f, 958.094f, 119.701f, 6.05232f,        51165, 51166},
    {-1219.01f, 909.048f, 119.723f, 4.28989f,        51165, 51166},
    {-1228.21f, 906.927f, 119.728f, 4.53337f,        51165, 51166},
    {-1241.97f, 905.671f, 119.722f, 4.95747f,        51165, 51166},

};

const BfTBObjectPosition CentrNPC[37] =
{
    {-1182.93f,     936.213f,     119.728f,    5.80603f,     50173, 50167},
};


const BfTBObjectPosition AllianceSpawnNPC[27] =
{
    {-837.3768f,    1196.082f,    114.2994f,   3.036873f,    51165, 0},
    {-837.809f,     1179.842f,    114.1356f,   3.159046f,    51165, 0},
    {-762.5504f,    1179.019f,    107.2137f,   3.159046f,    51165, 0},
    {-762.118f,     1195.259f,    107.2007f,   3.036873f,    51165, 0},
    {-1360.68f,     685.882f,     123.426f,    2.6943f,      47595, 0},
    {-1410.67f,     626.736f,     123.423f,    0.80149f,     47595, 0},
    {-1466.99f,     653.838f,     123.423f,    0.397003f,    47595, 0},
    {-1528.59f,     656.61f,      123.421f,    3.93915f,     47595, 0},
    {-830.932f,     928.307f,     121.441f,    2.83331f,     47595, 0},
    {-883.376f,     970.492f,     121.441f,    4.62402f,     47595, 0},
    {-1491.68f,     692.527f,     123.422f,    3.00452f,     47598, 0},
    {-1419.68f,     680.449f,     123.421f,    2.09346f,     47598, 0},
    {-1424.75f,     626.612f,     123.422f,    0.97427f,     47598, 0},
    {-955.18f,      925.935f,     121.441f,    3.13176f,     47598, 0},
    {-880.304f,     917.123f,     121.441f,    3.46949f,     47598, 0},
    {-1440.05f,     747.537f,     123.422f,    1.10387f,     47600, 0},
    {-1521.92f,     673.305f,     123.422f,    0.935005f,    47600, 0},
    {-1444.05f,     596.387f,     123.421f,    4.11194f,     47600, 0},
    {-1372.16f,     686.296f,     123.422f,    1.33948f,     47600, 0},
    {-961.609f,     933.868f,     121.442f,    3.28884f,     47600, 0},
    {-834.797f,     963.605f,     121.44f,     1.40388f,     47600, 0},
    {-1466.75f,     582.644f,     123.421f,    0.0514269f,   47599, 0},
    {-1482.3f,      629.489f,     123.421f,    3.9313f,      47599, 0},
    {-1414.15f,     717.203f,     123.421f,    4.34756f,     47599, 0},
    {-1447.06f,     757.232f,     123.422f,    1.89319f,     47599, 0},
    {-845.793f,     922.752f,     121.44f,     0.0294366f,   47599, 0},
    {-950.795f,     960.499f,     121.443f,    1.73376f,     47599, 0},
};

const BfTBObjectPosition HordeSpawnNPC[27] =
{
    {-837.3768f,    1196.082f,    114.2994f,   3.036873f,    51166, 0},
    {-837.809f,     1179.842f,    114.1356f,   3.159046f,    51166, 0},
    {-762.5504f,    1179.019f,    107.2137f,   3.159046f,    51166, 0},
    {-762.118f,     1195.259f,    107.2007f,   3.036873f,    51166, 0},
    {-879.097f,     1031.14f,     121.441f,    3.43414f,     47607, 0},
    {-846.13f,      1035.92f,     121.589f,    0.104053f,    47607, 0},
    {-1564.3f,      1320.38f,     133.584f,    5.13218f,     47607, 0},
    {-1499.51f,     1383.7f,      133.591f,    2.26548f,     47607, 0},
    {-1427.8f,      1306.14f,     133.584f,    1.55862f,     47607, 0},
    {-1478.19f,     1239.36f,     133.584f,    3.35718f,     47607, 0},
    {-957.143f,     987.354f,     121.505f,    3.36738f,     47608, 0},
    {-1543.83f,     1286.87f,     133.584f,    2.25369f,     47608, 0},
    {-1503.17f,     1351.55f,     152.961f,    5.60734f,     47608, 0},
    {-1442.06f,     1334.72f,     133.754f,    1.05989f,     47608, 0},
    {-1466.6f,      1278.0f,      133.584f,    5.60342f,     47608, 0},
    {-953.343f,     1023.93f,     121.441f,    3.65798f,     47609, 0},
    {-832.976f,     1030.49f,     121.441f,    3.2103f,      47609, 0},
    {-1529.07f,     1256.59f,     133.816f,    4.5824f,      47609, 0},
    {-1426.26f,     1291.56f,     133.601f,    0.372661f,    47609, 0},
    {-1545.93f,     1362.95f,     133.615f,    0.906734f,    47609, 0},
    {-832.029f,     989.578f,     121.441f,    4.8243f,      47610, 0},
    {-884.68f,      991.661f,     121.441f,    4.69471f,     47610, 0},
    {-970.775f,     1031.12f,     121.441f,    5.82175f,     47610, 0},
    {-1447.47f,     1356.02f,     133.64f,     3.5928f,      47610, 0},
    {-1499.62f,     1232.78f,     133.585f,    6.05109f,     47610, 0},
    {-1521.11f,     1381.87f,     133.584f,    0.180243f,    47610, 0},
    {-1515.72f,     1303.58f,     152.961f,    2.25369f,     47610, 0},
};


//*********************************************************
//*****************WorkShop Data & Element*****************
//*********************************************************

enum eTBWorkShopType
{
    BATTLEFIELD_TB_NOTH_CP,
    BATTLEFIELD_TB_WEST_CP,
    BATTLEFIELD_TB_EAST_CP,
    TB_MAX_WORKSHOP,
};


struct BfTBWorkShopDataBase
{
    uint32 worldstate;
    uint32 type;
    uint32 nameid1;
    uint32 nameid2;
    BfTBObjectPosition CapturePoint;
};

const BfTBWorkShopDataBase TBWorkShopDataBase[TB_MAX_WORKSHOP]=
{
    {WS_TB_SOUTH_CAPTURE_POINT, BATTLEFIELD_TB_NOTH_CP, BATTLEFIELD_TB_TEXT_IRONCLAD_GARRION_1, BATTLEFIELD_TB_TEXT_IRONCLAD_GARRION_2, { -896.960000f, 979.497000f, 121.441000f, 3.124123f, GAMEOBJECT_TB_NORTH_CAPTURE_POINT_AD, GAMEOBJECT_TB_NORTH_CAPTURE_POINT_HD}},
    {WS_TB_EAST_CAPTURE_POINT,  BATTLEFIELD_TB_EAST_CP, BATTLEFIELD_TB_TEXT_WARDENS_VIGIL_1,    BATTLEFIELD_TB_TEXT_WARDENS_VIGIL_2,    { -1492.34000f, 1309.87000f, 152.961000f, -0.82030f, GAMEOBJECT_TB_EAST_CAPTURE_POINT_AD, GAMEOBJECT_TB_EAST_CAPTURE_POINT_HD}},
    {WS_TB_WEST_CAPTURE_POINT,  BATTLEFIELD_TB_WEST_CP, BATTLEFIELD_TB_TEXT_SLAGWORKS_1,        BATTLEFIELD_TB_TEXT_SLAGWORKS_2,        { -1437.00000f, 685.556000f, 123.421000f, 0.802851f, GAMEOBJECT_TB_WEST_CAPTURE_POINT_AD, GAMEOBJECT_TB_WEST_CAPTURE_POINT_HD}},
};

//*********************************


//********************************************************************
//*                Structs using for Building,Graveyard,Workshop         *
//********************************************************************

//Structure for different building witch can be destroy during battle
struct BfTBGameObjectBuilding
{
    BfTBGameObjectBuilding(BattlefieldTB* TB)
    {
        m_TB = TB;
        m_Team = 0;
        m_Build = NULL;
        m_Type = 0;
        m_WorldState = 0;
        m_State = 0;
        m_NameId = 0;
    }
    //Team witch control this point
    uint8 m_Team;

    // TB object
    BattlefieldTB* m_TB;

    //Linked gameobject
    GameObject* m_Build; 

    //eTBGameObjectBuildingType
    uint32 m_Type;

    //WorldState
    uint32 m_WorldState;

    //eTBGameObjectState
    uint32 m_State;
    
    //Name id for warning text
    uint32 m_NameId;

    void Rebuild()
    {
        if (!m_Build)
            return;

        m_Team = m_TB->GetDefenderTeam();
        //Rebuild gameobject
        m_Build->Refresh();
        //Updating worldstate
        m_State = BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_INTACT-(m_Team*3);
        m_Build->SetUInt32Value(GAMEOBJECT_FACTION,TolBaradFaction[m_Team]);
    }

    //Called when associate gameobject is damaged
    void Damaged()
    {
        m_TB->OnDamaged();

        for (int i = 0; i < BUILDING_MAX_DIFF; i++)
        {
            if (i == BUILDING_HORDE_DEFENCE_DAMAGED)
            {
                if (m_TB->GetDefenderTeam() == TEAM_HORDE)
                    m_TB->SendUpdateWorldState(m_WorldState + i, 1);
                else
                    m_TB->SendUpdateWorldState(m_WorldState + i, 0);
            }
            else if (i == BUILDING_ALLIANCE_DEFENCE_DAMAGED)
            {
                if (m_TB->GetDefenderTeam() == TEAM_ALLIANCE)
                    m_TB->SendUpdateWorldState(m_WorldState + i, 1);
                else
                    m_TB->SendUpdateWorldState(m_WorldState + i, 0);
            }
            else
                m_TB->SendUpdateWorldState(m_WorldState + i, 0);
        }

        //Updating worldstate
        m_State = BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_DAMAGE-(m_Team*3);

        //Send warning message
        //if(m_NameId)
        //    m_TB->SendWarningToAllInZone(BATTLEFIELD_TB_TEXT_TOWER_DAMAGE,sObjectMgr->GetTrinityStringForDBCLocale(m_NameId));

        m_TB->AddDamagedTower(m_TB->GetAttackerTeam());
    }
    
    //Called when associate gameobject is destroy
    void Destroyed()
    {
        m_TB->OnDestroyed();

        m_TB->SetTimer(m_TB->GetTimer() + 5 * 60 * 1000);
        m_TB->SendUpdateWorldState(WS_TB_BATTLE_TIMER, (time(NULL) + m_TB->GetTimer() / 1000));

        for (int i = 0; i < BUILDING_MAX_DIFF; i++)
        {
            if (i == BUILDING_DESTROYED)
                m_TB->SendUpdateWorldState(m_WorldState + i, 1);
            else
                m_TB->SendUpdateWorldState(m_WorldState + i, 0);
        }

        //Updating worldstate
        m_State = BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_DESTROY-(m_Team*3);

        //Warning 
        //if(m_NameId)
        //    m_TB->SendWarningToAllInZone(BATTLEFIELD_TB_TEXT_TOWER_DESTROY,sObjectMgr->GetTrinityStringForDBCLocale(m_NameId));

        m_TB->AddBrokenTower(TeamId(m_Team));
        m_TB->BrokenWallOrTower(TeamId(m_Team));
    }

    void Init(GameObject* go, uint32 type, uint32 worldstate, uint32 nameid)
    {
        m_Build = go;
        m_Type = type;
        m_WorldState = worldstate;
        m_NameId = nameid;
        m_Team = m_TB->GetDefenderTeam();

        switch (m_State)
        {
            case BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_INTACT:
            case BATTLEFIELD_TB_OBJECTSTATE_HORDE_INTACT:
                if(m_Build)
                    m_Build->Refresh();
                break;
            case BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_DESTROY:
            case BATTLEFIELD_TB_OBJECTSTATE_HORDE_DESTROY:
                if(m_Build){
                    m_Build->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
                    m_Build->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    m_Build->SetDisplayId(m_Build->GetGOInfo()->building.destroyedDisplayId);
                }
                break;
            case BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_DAMAGE:
            case BATTLEFIELD_TB_OBJECTSTATE_HORDE_DAMAGE:
                if(m_Build){
                    m_Build->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
                    m_Build->SetDisplayId(m_Build->GetGOInfo()->building.damagedDisplayId);
                }
                break;
        }
    }

    void Save()
    {
        sWorld->setWorldState(m_WorldState,m_State);
    }
};

//Structure for the workshop
struct BfTBWorkShopData
{
    BattlefieldTB* m_TB;
    GameObject* m_Build;
    uint32 m_Type;
    uint32 m_State;
    uint32 m_WorldState;
    uint32 m_TeamControl;
    uint32 m_NameId1;
    uint32 m_NameId2;

    BfTBWorkShopData(BattlefieldTB* TB)
    {
        m_TB = TB;
        m_Build = NULL;
        m_Type = 0;
        m_State = 0;
        m_WorldState = 0;
        m_TeamControl = 0;
        m_NameId1 = 0;
        m_NameId2 = 0;
    }

    //Init method, setup variable
    void Init(uint32 worldstate, uint32 type, uint32 nameid1, uint32 nameid2)
    {
        m_WorldState = worldstate;
        m_Type = type;
        m_NameId1 = nameid1;
        m_NameId2 = nameid2;
    }

    //Called on change faction in CapturePoint class
    void ChangeControl(uint8 team, bool init/* for first call in setup*/)
    {
        if (!init)
            m_TB->CapturePoint(team);

        switch (team)
        {
            case BATTLEFIELD_TB_TEAM_NEUTRAL:
            {
                if (m_TeamControl == BATTLEFIELD_TB_TEAM_ALLIANCE)
                {
                    for (int i = 0; i < MAX_CP_DIFF; i++)
                    {
                        if (i == HORDE_ATTACK)
                            m_TB->SendUpdateWorldState(m_WorldState + i, 1);
                        else
                            m_TB->SendUpdateWorldState(m_WorldState + i, 0);
                    }
                }
                else
                {
                    for (int i = 0; i < MAX_CP_DIFF; i++)
                    {
                        if (i == ALLIANCE_ATTACK)
                            m_TB->SendUpdateWorldState(m_WorldState + i, 1);
                        else
                            m_TB->SendUpdateWorldState(m_WorldState + i, 0);
                    }
                }

                //Send warning message to all player for inform a faction attack a workshop
                //m_TB->SendWarningToAllInZone(BATTLEFIELD_TB_TEXT_TEMPLATE, sObjectMgr->GetTrinityStringForDBCLocale(m_TeamControl ? BATTLEFIELD_TB_TEXT_ALLIANCE_LOSE : BATTLEFIELD_TB_TEXT_HORDE_LOSE), sObjectMgr->GetTrinityStringForDBCLocale(m_NameId1));
                break;
            }
            case BATTLEFIELD_TB_TEAM_ALLIANCE:
            {
                for (int i = 0; i < MAX_CP_DIFF; i++)
                {
                    if (i == ALLIANCE_DEFENCE)
                        m_TB->SendUpdateWorldState(m_WorldState + i, 1);
                    else
                        m_TB->SendUpdateWorldState(m_WorldState + i, 0);
                }

                //Updating worldstate
                m_State = BATTLEFIELD_TB_OBJECTSTATE_ALLIANCE_INTACT;

                //Warning message
                //if (!init)
                //    m_TB->SendWarningToAllInZone(BATTLEFIELD_TB_TEXT_TEMPLATE, sObjectMgr->GetTrinityStringForDBCLocale(BATTLEFIELD_TB_TEXT_ALLIANCE_TAKEN), sObjectMgr->GetTrinityStringForDBCLocale(m_NameId2));

                m_TeamControl = team;
                break;
            }
            case BATTLEFIELD_TB_TEAM_HORDE:
            {
                for (int i = 0; i < MAX_CP_DIFF; i++)
                {
                    if (i == HORDE_DEFENCE)
                        m_TB->SendUpdateWorldState(m_WorldState + i, 1);
                    else
                        m_TB->SendUpdateWorldState(m_WorldState + i, 0);
                }

                //Update worlstate
                m_State = BATTLEFIELD_TB_OBJECTSTATE_HORDE_INTACT;

                //Warning message
                //if (!init)
                //    m_TB->SendWarningToAllInZone(BATTLEFIELD_TB_TEXT_TEMPLATE, sObjectMgr->GetTrinityStringForDBCLocale(BATTLEFIELD_TB_TEXT_HORDE_TAKEN), sObjectMgr->GetTrinityStringForDBCLocale(m_NameId1));

                m_TeamControl = team;
                break;
            }
        }
    }

    void UpdateWorkshop()
    {
        ChangeControl(m_TB->GetDefenderTeam(), true);
    }

    void Save()
    {
        sWorld->setWorldState(m_WorldState,m_State);
    }
};

#endif
