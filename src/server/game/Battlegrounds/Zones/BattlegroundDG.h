/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef __BATTLEGROUNDDG_H
#define __BATTLEGROUNDDG_H

#include "Battleground.h"

#define CAPTURE_TIME 40000
#define GOLD_UPDATE 5000

#define BG_DG_MAX_TEAM_SCORE 1600

enum BG_DG_ObjectTypes
{
    BG_DG_DOOR_1 = 0,
    BG_DG_DOOR_2,
    BG_DG_DOOR_3,
    BG_DG_DOOR_4,

    BG_DG_CART_ALLIANCE,
    BG_DG_CART_HORDE,

    BG_DG_SPIRIT_MAIN_ALLIANCE,
    BG_DG_SPIRIT_MAIN_HORDE,

    BG_DG_OBJECT_MAX
};

enum BG_DG_UnitTypes
{
    BG_DG_UNIT_FLAG_BOT,
    BG_DG_UNIT_FLAG_MID,
    BG_DG_UNIT_FLAG_TOP,

    BG_DG_SPIRIT_ALLIANCE_BOT,
    BG_DG_SPIRIT_HORDE_BOT,
    BG_DG_SPIRIT_ALLIANCE_TOP,
    BG_DG_SPIRIT_HORDE_TOP,

    BG_DG_UNIT_MAX
};

#define MAX_POINTS 3

enum BG_DG_SPELLS
{
    BG_DG_AURA_NEUTRAL          = 98554,
    BG_DG_AURA_HORDE_CONTEST    = 98545,
    BG_DG_AURA_ALLIANCE_CONTEST = 98543,
    BG_DG_AURA_HORDE_CAPTURED   = 98527,
    BG_DG_AURA_ALLIANCE_CATURED = 98519,

    BG_DG_AURA_CART_HORDE       = 141555,
    BG_DG_AURA_CART_ALLIANCE    = 141551,


    BG_DG_AURA_CARTS_CHAINS     = 141553,

    BG_DG_CAPTURE_SPELL         = 97388,

    BG_DG_SPELL_SPAWN_ALLIANCE_CART = 141554,
    BG_DG_SPELL_SPAWN_HORDE_CART    = 141550,

    BG_DG_AURA_PLAYER_FLAG_HORDE    = 141210,
    BG_DG_AURA_PLAYER_FLAG_ALLIANCE = 140876
};

enum BG_DG_UnitEntry
{
    BG_DG_ENTRY_FLAG                = 53194
};

enum BG_DG_LOCS
{
    BG_DG_LOC_SPIRIT_ALLIANCE_BOT = 4488,
    BG_DG_LOC_SPIRIT_HORDE_BOT    = 4545,
    BG_DG_LOC_SPIRIT_ALLIANCE_TOP = 4546,
    BG_DG_LOC_SPIRIT_HORDE_TOP    = 4489
};

enum BG_DG_ObjectEntry
{
    BG_DG_ENTRY_DOOR_1    = 220159,
    BG_DG_ENTRY_DOOR_2    = 220160,
    BG_DG_ENTRY_DOOR_3    = 220161,
    BG_DG_ENTRY_DOOR_4    = 220366,

    BG_DG_ENTRY_CART_ALLIANCE   = 220164,
    BG_DG_ENTRY_CART_HORDE      = 220166
};

enum PointStates
{
    POINT_STATE_NEUTRAL,
    POINT_STATE_CONTESTED_ALLIANCE,
    POINT_STATE_CONTESTED_HORDE,
    POINT_STATE_CAPTURED_ALLIANCE,
    POINT_STATE_CAPTURED_HORDE
};

enum BG_DG_SOUNDS
{
    BG_DG_SOUND_NODE_CLAIMED            = 8192,
    BG_DG_SOUND_NODE_CAPTURED_ALLIANCE  = 8173,
    BG_DG_SOUND_NODE_CAPTURED_HORDE     = 8213,
    BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE = 8212,
    BG_DG_SOUND_NODE_ASSAULTED_HORDE    = 8174
};

enum BG_DG_Objectives
{
    DG_OBJECTIVE_CAPTURE_CART               = 457,
    DG_OBJECTIVE_RETURN_CART                = 458,
    DG_OBJECTIVE_CAPTURE_FLAG               = 459,
    DG_OBJECTIVE_DEFENDED_FLAG              = 460,
};

class BattlegroundDGScore : public BattlegroundScore
{
    public:
        BattlegroundDGScore() : cartsCaptured(0), cartsDefended(0), pointsCaptured(0), pointsDefended(0) {}
        virtual ~BattlegroundDGScore() {}

        uint32 cartsCaptured;
        uint32 cartsDefended;
        uint32 pointsCaptured;
        uint32 pointsDefended;
};

class BattlegroundDG : public Battleground
{
public:
    friend class Point;
    friend class TopPoint;
    friend class BotPoint;

    BattlegroundDG();
    ~BattlegroundDG();

    /* inherited from BattlegroundClass */
    void AddPlayer(Player* player);
    virtual void StartingEventCloseDoors();
    virtual void StartingEventOpenDoors();

    void UpdatePlayerScore(Player* player, uint32 type, uint32 addvalue, bool doAddHonor);

    WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);

    void RemovePlayer(Player* player, uint64 guid, uint32 team);
    void HandleAreaTrigger(Player* Source, uint32 Trigger);
    bool SetupBattleground();
    virtual void Reset();
    virtual void FillInitialWorldStates(WorldPacket &d);
    void HandleKillPlayer(Player* player, Player* killer);
    bool HandlePlayerUnderMap(Player* player);

    void HandlePointCapturing(Player* player, Creature* creature);

    void EventPlayerUsedGO(Player* player, GameObject* go);
    void EventPlayerDroppedFlag(Player* Source);

    void UpdatePointsCountPerTeam();

    uint32 ModGold(uint8 teamId, int32 val);
    uint32 GetCurrentGold(uint8 teamId) { return m_gold[teamId]; }

    uint64 GetFlagPickerGUID(int32 team) const;

private:
        class Point
        {
        public:
            Point(BattlegroundDG* bg);
            ~Point();

            BattlegroundDG* GetBg() { return m_bg; }

            virtual void UpdateState(PointStates state);

            uint8 GetState() { return m_state; }

            void PointClicked(Player* player);
            void Update(uint32 diff);

            uint32 TakeGoldCredit() { return m_goldCredit; }

            void SetCreaturePoint(Creature* creature) { m_point = creature; }
            Creature* GetCreaturePoint() { return m_point; }

        protected:
            typedef std::pair<uint32, uint32> WorldState;
            WorldState m_currentWorldState;

            uint8 m_state;
            uint32 m_prevAura;

            BattlegroundDG* m_bg;
            Creature* m_point;

            int32 m_timer;

            uint32 m_goldCredit;
        };

        class TopPoint : public Point
        {
        public:
            TopPoint(BattlegroundDG* bg) : Point(bg) {}

            void UpdateState(PointStates state);
        };

        class BotPoint : public Point
        {
        public:
            BotPoint(BattlegroundDG* bg): Point(bg) {}

            void UpdateState(PointStates state);
        };

        class MiddlePoint : public Point
        {
        public:
            MiddlePoint(BattlegroundDG* bg): Point(bg) {}

            void UpdateState(PointStates state);
        };

        class Cart
        {
        public:
            Cart(BattlegroundDG* bg);
            ~Cart() {}

            void ToggleCaptured(Player* player);
            void CartDropped();
            void CartDelivered();
            void UnbindCartFromPlayer();

            void SetGameObject(GameObject* obj, uint32 id) { m_goCart = obj; m_goBgId = id; }
            GameObject* GetGameObject() { return m_goCart; }

            void SetTeamId(uint32 team) { m_team = team; }
            uint32 TeamId() { return m_team; }

            uint64 TakePlayerWhoDroppedFlag() { uint64 v = m_playerDroppedCart; m_playerDroppedCart = 0; return v; }

            Player* ControlledBy();
            uint64  ControlledByPlayerWithGuid() { return m_controlledBy; }

            BattlegroundDG* GetBg() { return m_bg; }
        private:
            BattlegroundDG* m_bg;
            uint64 m_controlledBy;

            GameObject* m_goCart;
            uint32 m_goBgId;

            uint32 m_team;

            uint64 m_playerDroppedCart;
            uint32 m_stolenGold;
        };

private:
        virtual void PostUpdateImpl(uint32 diff);

        Point* m_points[MAX_POINTS];
        Cart*  m_carts[MAX_TEAMS];

        int32 m_flagsUpdTimer;
        int32 m_goldUpdate;

        uint32 m_gold[MAX_TEAMS];
};

#endif
