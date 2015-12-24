/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#ifndef __BATTLEGROUNDSSM_H
#define __BATTLEGROUNDSSM_H

#include "Battleground.h"

class BattleGroundSSMScore : public BattlegroundScore
{
    public:
        BattleGroundSSMScore() : CartsTaken(0) {}
        virtual ~BattleGroundSSMScore() {}
        uint8 CartsTaken;
};

enum BG_SSM_ObjectTypes
{
    BG_SSM_CART_EAST,
    BG_SSM_CART_NORTH,
    BG_SSM_CART_SOUTH,

    BG_SSM_SPIRIT_MAIN_ALLIANCE,
    BG_SSM_SPIRIT_MAIN_HORDE,

    BG_DOOR_1,
    BG_DOOR_2,
    BG_DOOR_3,
    BG_DOOR_4,
    
    BG_SSM_OBJECT_SPEEDBUFF,
    BG_SSM_OBJECT_REGENBUFF,
    BG_SSM_OBJECT_BERSERKBUFF,

    BG_POINT_END1,
    BG_POINT_END2,
    BG_POINT_END3,
    BG_POINT_END4,

    BG_SSM_CREATURE_TRACK1,
    BG_SSM_CREATURE_TRACK2,

    BG_SSM_OBJECT_MAX,
};

enum BG_SSM_SPELLS
{
    BG_SSM_SPELL_CONTROL_NEUTRAL                        = 118001,
    BG_SSM_SPELL_CONTROL_ALLIANCE                       = 116086,
    BG_SSM_SPELL_CONTROL_HORDE                          = 116085,

    BG_SSM_SPELL_CART_CONTROL_CAPTURE_POINT_UNIT_SOUTH  = 125696,
    BG_SSM_SPELL_CART_CONTROL_CAPTURE_POINT_UNIT_NORTH  = 125695,
    BG_SSM_SPELL_CART_CONTROL_CAPTURE_POINT_UNIT_EAST   = 125620,

    BG_SSM_SPELL_DEFENDING_CART_AURA                    = 128646,
    BG_SSM_SPELL_CART_CAP                               = 115904,
};

enum BG_SSM_Greveyards
{
    BG_SSM_HORDE_GRAVEYARD      = 4061,
    BG_SSM_ALLIANCE_GRAVEYARD   = 4062
};

uint32 const SSM_SCORE_UPDATE_TIMER =  5 * IN_MILLISECONDS;
uint32 const SSM_CARTS_UPDATE_TIMER = 2 * IN_MILLISECONDS;

enum BG_SSM_UnitEntry
{
    CREATURE_BG_SSM_CART            = 60140,
    CREATURE_BG_SSM_TRACK_SWITCH    = 60283,
};

static const uint16 SSM_MAX_TEAM_POINTS = 1500;

enum BG_SSM_ObjectEntry
{
    OBJECT_BG_SSM_RESERVOIR                 = 212080,
    OBJECT_BG_SSM_THE_DESPOSITION_OF_LAVA   = 212081,
    OBJECT_BG_SSM_THE_DESPOSITS_OF_DIAMONDS = 212082,
    OBJECT_BG_SSM_BACKLOG_TROLLS            = 212083,

    OBJECT_BG_SSM_DOOR1                     = 212939,
    OBJECT_BG_SSM_DOOR2                     = 212940,
    OBJECT_BG_SSM_DOOR3                     = 212941,
    OBJECT_BG_SSM_DOOR4                     = 212942,
};

static const uint32 BroadcastTextCartTaken[BG_TEAMS_COUNT] = {59689, 59690};
static const uint32 BroadcastTextCartControlTaken[BG_TEAMS_COUNT] = {60441, 60442};
static const uint32 BroadcastTextArrow[BG_TEAMS_COUNT] = {60031, 60032};
static const uint32 BroadcastCartSpawn = 60444;
static const uint32 SoundKitCartSpawn = 9431;
static const uint32 BG_SSM_MAX_CARTS = 3;

enum BG_SSM_ControlState
{
    SSM_CONTROL_NEUTRAL,
    SSM_CONTROL_ALLIANCE,
    SSM_CONTROL_HORDE
};

enum BgSSMTrackSwitchState
{
    TRACK_SWITCH_STATE_DEFAULT     = 1,
    TRACK_SWITCH_STATE_EXTRA_WAY   = 2
};

enum BG_SSM_ProgressBarConsts
{
    BG_SSM_POINT_RADIUS                  = 22,
    BG_SSM_PROGRESS_BAR_DONT_SHOW        = 0,
    BG_SSM_PROGRESS_BAR_SHOW             = 1,
    BG_SSM_PROGRESS_BAR_PERCENT_GREY     = 0,
    BG_SSM_PROGRESS_BAR_STATE_MIDDLE     = 50,
    BG_SSM_PROGRESS_BAR_HORDE_CONTROLLED = 0,
    BG_SSM_PROGRESS_BAR_ALI_CONTROLLED   = 100
};

Position const WayEastBase[] =
{
    {744.5086f, 183.1756f, 319.5443f, 4.338116f},
    {744.9193f, 184.1136f, 319.5439f},
    {744.5174f, 183.1979f, 319.5439f},
    {742.8542f, 178.9635f, 319.6017f},
    {739.8906f, 171.5191f, 319.3651f},
    {735.5955f, 163.5434f, 319.2191f},
    {730.4774f, 155.1944f, 319.1304f},
    {728.8577f, 152.4063f, 319.6251f},
    {723.9236f, 143.1042f, 319.6667f},
    {719.3646f, 131.0938f, 319.5531f},
    {717.6007f, 122.7222f, 320.0978f},
    {716.9514f, 113.5174f, 320.8443f},
    {716.9514f, 113.5174f, 320.8443f},
};

Position const ExtraWayEast1[] =
{
    {715.8229f, 109.0373f, 321.0153f},
    {713.3229f, 104.2873f, 320.2653f},
    {706.8229f, 99.28732f, 319.0153f},
    {701.8229f, 97.28732f, 317.7653f},
    {695.0729f, 96.53732f, 316.2653f},
    {686.5729f, 95.53732f, 314.5153f},
    {680.3229f, 94.53732f, 312.7653f},
    {670.3229f, 90.03732f, 309.5153f},
    {663.8229f, 86.53732f, 307.0153f},
    {660.3229f, 84.53732f, 305.7653f},
    {647.3229f, 82.03732f, 302.0153f},
    {638.5729f, 81.53732f, 299.5153f}, // 638.5729 81.53732 299.5153
};

Position const ExtraWayEast2[] =
{
    {716.9749f, 108.5017f, 321.1373f},
    {719.9749f, 101.5017f, 321.3873f},
    {724.4749f, 98.25174f, 322.1373f},
    {732.7249f, 94.75174f, 323.8873f},
    {737.7249f, 93.75174f, 325.8873f},
    {744.4749f, 92.25174f, 328.1373f},
    {758.7249f, 89.25174f, 332.8873f},
    {767.2249f, 86.00174f, 336.1373f},
    {774.2249f, 82.25174f, 339.3873f},
    {784.2249f, 79.50174f, 342.8873f},
    {793.7249f, 78.25174f, 346.1373f},
    {798.2249f, 77.25174f, 348.1373f},
    {803.9749f, 74.75174f, 349.8873f},
    {807.7249f, 72.25174f, 351.1373f},
    {813.4749f, 67.75174f, 352.8873f},
    {819.9749f, 64.00174f, 354.8873f},
    {823.9749f, 63.00174f, 355.8873f},
    {829.7249f, 63.00174f, 357.1373f},
    {833.4749f, 64.25174f, 357.8873f},
    {842.2249f, 70.50174f, 359.8873f},
    {850.4749f, 71.50174f, 361.3873f},
    {859.4749f, 69.00174f, 362.8873f},
    {865.4749f, 65.25174f, 363.8873f},
    {873.4749f, 58.00174f, 364.6373f},
    {879.7249f, 51.50174f, 364.6373f}
};

Position const WaySouth[] =
{
    {738.8957f, 204.154f,  319.6037f, 2.248367f},
    {738.9358f, 204.1042f, 319.6031f},
    {736.0469f, 207.6944f, 319.6429f},
    {732.5243f, 213.625f,  320.1088f},
    {730.5608f, 219.1163f, 320.544f},
    {728.0052f, 231.5764f, 321.037f},
    {725.6285f, 236.9444f, 321.0317f},
    {722.8368f, 241.6771f, 320.7483f},
    {720.7118f, 244.5208f, 321.2489f},
    {715.6458f, 251.4236f, 321.1337f},
    {710.8594f, 261.0868f, 320.8995f},
    {707.2899f, 270.0538f, 320.633f},
    {703.6528f, 278.1771f, 320.5885f},
    {696.5243f, 290.1788f, 320.6527f},
    {686.2604f, 301.6042f, 321.0385f},
    {667.9496f, 317.1371f, 323.1469f},
    {660.3976f, 323.9063f, 324.7097f},
    {651.4028f, 331.2066f, 327.3729f},
    {638.6805f, 342.3385f, 332.2758f},
    {633.8316f, 345.2587f, 334.5666f},
    {626.217f,  345.4913f, 337.4414f},
    {616.7413f, 343.224f,  340.5497f},
    {601.7222f, 340.3073f, 344.5738f},
    {592.059f,  339.2743f, 346.0716f},
    {585.4739f, 337.6736f, 346.3017f},
    {585.4739f, 337.6736f, 346.3017f},
};

Position const WayNorthBase[] =
{
    {759.3847f, 198.3586f, 319.5317f, 0.4215083f},
    {758.3997f, 197.9515f, 319.5306f},
    {759.3246f, 198.3316f, 319.5306f},
    {762.0156f, 199.5382f, 319.5831f},
    {765.5938f, 202.5295f, 319.9603f},
    {770.7864f, 210.3819f, 321.3461f},
    {776.9566f, 222.9167f, 323.3075f},
    {780.1962f, 232.6215f, 324.9792f},
    {783.5833f, 241.6458f, 327.9297f},
    {787.6268f, 249.401f,  330.7884f},
    {794.5851f, 259.5799f, 336.815f},
    {803.3455f, 269.5903f, 341.3372f},
    {816.4271f, 282.5642f, 344.849f},
    {823.8924f, 289.4271f, 345.9665f},
    {830.2726f, 294.8576f, 346.5565f},
    {835.592f,  300.4184f, 346.956f},
    {835.592f,  300.4184f, 346.956f},
};

Position const WayNorthLeft[] =
{
    {837.7413f, 303.2179f, 347.3937f},
    {840.7413f, 307.4679f, 347.3937f},
    {844.7413f, 314.2179f, 347.3937f},
    {847.9913f, 321.7179f, 347.6437f},
    {848.7413f, 326.9679f, 347.3937f},
    {846.9913f, 342.7179f, 347.3937f},
    {843.4913f, 350.9679f, 347.6437f},
    {831.9913f, 372.4679f, 347.3937f},
    {822.4913f, 397.4679f, 347.1437f},
    {818.7413f, 406.9679f, 348.1437f},
    {817.9913f, 410.4679f, 348.8937f},
    {816.7413f, 419.2179f, 350.6437f},
    {817.2413f, 422.7179f, 351.3937f},
    {815.9913f, 429.2179f, 353.3937f},
    {814.2413f, 434.7179f, 355.1437f},
    {810.9913f, 442.2179f, 357.1437f},
    {807.4913f, 457.7179f, 358.8937f},
    {806.7413f, 466.4679f, 359.1437f},
    {803.9913f, 472.9679f, 359.3937f},
    {800.9913f, 476.9679f, 359.6437f},
    {793.7413f, 485.2179f, 359.6437f}  /// 793.7413 485.2179 359.6437
};

Position const WayNorthRight[] =
{
    {838.257f, 302.2188f, 347.3339f},
    {843.257f, 303.4688f, 347.3339f},
    {848.257f, 303.7188f, 347.3339f},
    {854.007f, 303.2188f, 347.3339f},
    {860.007f, 301.4688f, 347.5839f},
    {863.507f, 299.9688f, 347.5839f},
    {867.257f, 297.4688f, 347.5839f},
    {877.257f, 287.7188f, 347.5839f},
    {881.757f, 279.2188f, 347.3339f},
    {885.757f, 270.4688f, 346.8339f},
    {889.257f, 261.2188f, 346.3339f},
    {893.507f, 248.7188f, 346.0839f},
    {894.757f, 241.7188f, 346.5839f},
    {897.757f, 226.4688f, 350.0839f},
    {904.257f, 210.4688f, 354.8339f},
    {911.507f, 178.4688f, 363.8339f},
    {912.507f, 168.2188f, 366.5839f},
    {912.007f, 163.2188f, 366.8339f},
    {909.007f, 153.4688f, 366.8339f},
    {907.007f, 149.7188f, 366.8339f},
    {898.257f, 138.7188f, 366.8339f},
    {888.257f, 126.2188f, 366.5839f},
    {885.507f, 120.4688f, 366.3339f},
    {882.757f, 109.2188f, 365.8339f},
    {882.257f, 104.7188f, 365.5839f},
    {883.757f, 97.21875f, 365.3339f},
    {889.007f, 89.46875f, 365.0839f},
    {891.007f, 82.46875f, 365.0839f},
    {891.007f, 76.46875f, 364.8339f},
    {890.257f, 70.96875f, 365.0839f},
    {890.757f, 62.96875f, 364.8339f},
    {891.757f, 56.21875f, 364.5839f}
};

struct CartPointDataStruct
{
    uint32 ID;
    Position Pos;
    Position EndPos;
    Position EndPos2;
};

CartPointDataStruct const CartPointData[] = 
{
    {BG_SSM_CART_EAST,  WayEastBase[12],    ExtraWayEast1[11],  ExtraWayEast2[24]},
    {BG_SSM_CART_NORTH, WayNorthBase[16],   WayNorthRight[31],  WayNorthLeft[20]},
    {BG_SSM_CART_SOUTH, WaySouth[25],       WaySouth[25],       WaySouth[25]}
};

static uint32 WaysSize[] = {13, 17, 26};
static uint32 ExtraWaysSize[][2] =
{
    {21, 32},   //< WayNorthLeft WayNorthRight
    {12, 25}    //< ExtraWayEast1 ExtraWayEast2
};

Position const BgSSMObjectsPos[][2] = 
{
    {896.7936f, 25.29027f, 364.1431f, 3.539994f, -0.009816647f, 0.03659821f, -0.9795389f, 0.1976555f},  // OBJECT_BG_SSM_THE_DESPOSITS_OF_DIAMONDS
    {564.8048f, 337.0873f, 347.1427f, 1.571254f, 0.0112772f,    0.006951332f, 0.7072344f, 0.7068551f},  // OBJECT_BG_SSM_RESERVOIR
    {615.5099f, 79.41812f, 298.2867f, 1.654058f, -0.03407955f,  -0.02646542f, 0.7354441f, 0.6762102f},  // OBJECT_BG_SSM_THE_DESPOSITION_OF_LAVA
    {777.7377f, 502.3111f, 359.4537f, 0.719448f, -0.01845312f,  -0.01729774f, 0.3516912f, 0.9357743f}   // OBJECT_BG_SSM_BACKLOG_TROLLS
};

Position const BgSSMCreaturePos[] = 
{
    {715.6424f, 100.1649f, 320.2845f, 4.592556f},
    {845.5573f, 307.5521f, 347.0379f, 0.6224777f},
};

enum SsmWorldStates
{
    SSM_PROGRESS_BAR_PERCENT_GREY       = 6877,
    SSM_PROGRESS_BAR_STATUS             = 6876,
    SSM_PROGRESS_BAR_SHOW               = 6875,
    SSM_EAST_TRACK_SWITCH               = 6467,
    SSM_NORTH_TRACK_SWITCH              = 6468,

    SSM_INIT_POINTS_ALLIANCE            = 6441,
    SSM_INIT_POINTS_HORDE               = 6443,
    SSM_POINTS_ALLIANCE                 = 6437,
    SSM_POINTS_HORDE                    = 6438,
};

class BattlegroundSSM : public Battleground
{
    public:
        BattlegroundSSM();
        ~BattlegroundSSM();

        bool SetupBattleground() override;
        void PostUpdateImpl(uint32 diff) override;

        void HandleAreaTrigger(Player* player, uint32 trigger) override;

        void AddPlayer(Player* player) override;
        void RemovePlayer(Player* player, uint64 guid, uint32 team) override;
        void HandleKillPlayer(Player* player, Player* killer) override;

        void StartingEventOpenDoors() override;
        void FillInitialWorldStates(WorldPacket& data) override;
        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player) override;

    private:
        Creature* _AddCart(uint32 type, Position const loc);
        Creature* _UpdateCart(uint32 type, bool initial = true);

        void _CheckPlayersAtCars();

        void _UpdatePoints();
        void _UpdateScore();

        void _SetCartControl(uint32 type, TeamId teamID);

        void _AddScore(TeamId team, int32 points);

        Creature* _cart[BG_SSM_MAX_CARTS];

        uint32 _cartsState[BG_SSM_MAX_CARTS];
        int32 _cartsCapturePoints[BG_SSM_MAX_CARTS];

        uint32 _timerPointsUpdate;
        uint32 _timerCartsUpdate;

        bool _cartsAdded[BG_SSM_MAX_CARTS];
        uint32 _tractSwitchState[2];

        std::vector<ObjectGuid> _playersNearPoint[BG_SSM_MAX_CARTS + 1];
};

#endif
