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
    BG_SSM_CART_1,
    BG_SSM_CART_2,
    BG_SSM_CART_3,

    BG_SSM_SPIRIT_MAIN_ALLIANCE,
    BG_SSM_SPIRIT_MAIN_HORDE,

    BG_DOOR_1,
    BG_DOOR_2,
    BG_DOOR_3,
    BG_DOOR_4,
    
    BG_SSM_OBJECT_SPEEDBUFF,
    BG_SSM_OBJECT_REGENBUFF,
    BG_SSM_OBJECT_BERSERKBUFF,

    BG_SSM_OBJECT_MAX
};

enum BG_SSM_SPELLS
{
    BG_SSM_SPELL_CONTROL_NEUTRAL    = 118001,
    BG_SSM_SPELL_CONTROL_ALLIANCE   = 116086,
    BG_SSM_SPELL_CONTROL_HORDE      = 116085,

    BG_SSM_SPELL_CART_MOVE          = 52406,
};


enum BG_SSM_Greveyards
{
    BG_SSM_HORDE_GRAVEYARD      = 4061,
    BG_SSM_ALLIANCE_GRAVEYARD   = 4062
};

enum BG_SSM_Timers
{
    SSM_SCORE_UPDATE_TIMER = 5000,
    SSM_CARTS_UPDATE_TIMER = 2000
};

enum BG_SSM_UnitEntry
{
    BG_SSM_CART = 60379,
};

enum BG_SSM_ObjectEntry
{
    BG_SSM_CHEST_1 = 212080,
    BG_SSM_CHEST_2 = 212081,

    BG_SSM_DOOR    = 212941
};

enum BG_SSM_ControlState
{
    SSM_CONTROL_ALLIANCE,
    SSM_CONTROL_NEUTRAL,
    SSM_CONTROL_HORDE
};

struct Location
{
    float x, y, z, o;
};

enum BG_SSM_STATE
{
    SSM_PROGRESS_BAR_PERCENT_GREY       = 6877,                 //100 = empty (only grey), 0 = blue|red (no grey)
    SSM_PROGRESS_BAR_STATUS             = 6876,                 //50 init!, 48 ... hordak bere .. 33 .. 0 = full 100% hordacky, 100 = full alliance
    SSM_PROGRESS_BAR_SHOW               = 6875,                 //1 init, 0 druhy send - bez messagu, 1 = controlled aliance

    SSM_INIT_POINTS_ALLIANCE            = 6441,
    SSM_INIT_POINTS_HORDE               = 6443,
    SSM_POINTS_ALLIANCE                 = 6437,
    SSM_POINTS_HORDE                    = 6438

};

enum SSMBattlegroundPoints
{
    CART1_POINT     = 0,
    CART2_POINT     = 1,
    CART3_POINT     = 2,

    SSM_PLAYERS_OUT_OF_POINTS  = 3,
    SSM_POINTS_MAX             = 3
};

enum BG_SSM_ProgressBarConsts
{
    BG_SSM_POINT_MAX_CAPTURERS_COUNT     = 5,
    BG_SSM_POINT_RADIUS                  = 22,
    BG_SSM_PROGRESS_BAR_DONT_SHOW        = 0,
    BG_SSM_PROGRESS_BAR_SHOW             = 1,
    BG_SSM_PROGRESS_BAR_PERCENT_GREY     = 0,
    BG_SSM_PROGRESS_BAR_STATE_MIDDLE     = 50,
    BG_SSM_PROGRESS_BAR_HORDE_CONTROLLED = 0,
    BG_SSM_PROGRESS_BAR_NEUTRAL_LOW      = 30,
    BG_SSM_PROGRESS_BAR_NEUTRAL_HIGH     = 70,
    BG_SSM_PROGRESS_BAR_ALI_CONTROLLED   = 100
};

static Location Way1[]=
{
    {743.79f, 181.42f, 319.455f, 4.305f},
    {726.75f, 148.67f, 319.761f, 0.0f},
    {719.93f, 131.73f, 319.543f, 0.0f},
    {717.36f, 117.88f, 320.616f, 0.0f},
    {711.39f, 102.45f, 319.568f, 0.0f},
    {703.11f, 97.82f,  317.838f, 0.0f},
    {680.12f, 94.16f,  312.439f, 0.0f},
    {661.25f, 85.20f,  305.530f, 0.0f},
    {644.10f, 81.36f,  300.502f, 0.0f},
    {616.47f, 79.83f,  298.076f, 0.0f}
};

static Location Way2[]=
{
    {743.69f, 203.00f, 319.460f, 2.241f},
    {736.25f, 207.34f, 319.618f,  0.0f},
    {730.46f, 218.65f, 320.11f, 0.0f},
    {730.10f, 220.65f, 320.272f, 0.0f},
    {728.06f, 230.64f, 320.834f, 0.0f},
    {725.10f, 237.83f, 320.991f, 0.0f},
    {715.93f, 251.04f, 321.065f, 0.0f},
    {710.94f, 260.93f, 320.724f, 0.0f},
    {708.08f, 268.11f, 320.550f, 0.0f},
    {703.60f, 277.87f, 320.465f, 0.0f},
    {696.13f, 290.62f, 320.584f, 0.0f},
    {686.61f, 301.18f, 320.949f, 0.0f},
    {660.49f, 323.98f, 324.571f, 0.0f},
    {635.64f, 344.49f, 333.491f, 0.0f},
    {630.90f, 345.68f, 335.593f, 0.0f},
    {626.63f, 345.54f, 337.171f, 0.0f},
    {600.92f, 340.08f, 344.630f, 0.0f},
    {592.82f, 339.31f, 345.912f, 0.0f},
    {582.41f, 337.46f, 346.268f, 0.0f},
    {567.36f, 337.05f, 346.699f, 0.0f}
};

static Location Way3[]=
{
    {759.53f, 198.51f, 319.447f, 2.241f},
    {763.78f, 200.66f, 319.723f,  0.0f},
    {767.77f, 205.07f, 320.092f,  0.0f},
    {776.07f, 220.79f, 322.746f,  0.0f},
    {779.29f, 229.48f, 324.179f,  0.0f},
    {780.95f, 235.61f, 326.164f,  0.0f},
    {789.14f, 252.78f, 322.241f,  0.0f},
    {795.20f, 260.67f, 337.090f,  0.0f},
    {803.95f, 270.18f, 341.383f,  0.0f},
    {820.96f, 286.88f, 345.404f,  0.0f},
    {830.16f, 294.54f, 346.450f,  0.0f},
    {839.56f, 305.30f, 346.885f,  0.0f},
    {844.49f, 313.06f, 346.994f,  0.0f},
    {847.29f, 320.28f, 347.179f,  0.0f},
    {848.81f, 328.52f, 346.934f,  0.0f},
    {846.38f, 342.11f, 347.179f,  0.0f},
    {844.58f, 348.15f, 347.068f,  0.0f},
    {837.21f, 362.95f, 346.899f,  0.0f},
    {831.90f, 372.03f, 346.809f,  0.0f},
    {820.96f, 401.81f, 346.562f,  0.0f},
    {817.66f, 409.91f, 348.054f,  0.0f},
    {816.72f, 415.63f, 349.214f,  0.0f},
    {815.86f, 423.68f, 351.201f,  0.0f},
    {816.00f, 428.88f, 352.965f,  0.0f},
    {811.26f, 441.78f, 356.470f,  0.0f},
    {808.41f, 452.58f, 357.814f,  0.0f},
    {806.30f, 466.04f, 358.731f,  0.0f},
    {803.36f, 473.41f, 359.129f,  0.0f},
    {795.19f, 484.20f, 359.337f,  0.0f},
    {786.11f, 492.55f, 359.303f,  0.0f},
    {778.57f, 500.93f, 359.276f,  0.0f}
};

static uint16 WaysSize[] = {10, 20, 31};

class BattlegroundSSM : public Battleground
{
    public:
        typedef std::map<uint8, Location*> WaysMap;

        BattlegroundSSM();
        ~BattlegroundSSM();

        bool SetupBattleground();
        void PostUpdateImpl(uint32 diff);

        void AddPlayer(Player* player);
        void RemovePlayer(Player* player, uint64 guid, uint32 team);

        void StartingEventOpenDoors();

        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);

    private:
        Creature* AddCart(uint32 type, Location loc);
        Creature* UpdateCart(uint32 type);

        void CheckSomeoneLeftPoint();
        void CheckSomeoneJoinedPoint();

        void UpdatePoints();
        void UpdateScore();

        void SetCartControl(uint32 type, uint32 team);

        void AddScore(uint32 team, int32 points);

        Creature* m_cart[3];
        uint32 m_waysStep[3];
        WaysMap m_waysMap;

        uint32 m_cartsState[3];
        int32 m_cartsCapturePoints[3];

        int32 m_timerPointsUpdate;
        int32 m_timerCartsUpdate;

        typedef std::vector<uint64> PlayersNearPointType;
        PlayersNearPointType m_PlayersNearPoint[SSM_POINTS_MAX + 1];
};

#endif
