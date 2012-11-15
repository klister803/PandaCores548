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

#ifndef __BattleGroundKT_H
#define __BattleGroundKT_H

#include "Battleground.h"

enum BG_KT_NPC
{
    BG_SM_NPC_POWERBALL					= 29265,
};

#define BG_KT_MAX_TEAM_SCORE      1600
#define BG_KT_ORB_POINTS_MAX      1600
#define BG_KT_FLAG_RESPAWN_TIME   (23*IN_MILLISECONDS)
#define BG_KT_FLAG_DROP_TIME      (10*IN_MILLISECONDS)
#define BG_KT_TIME_LIMIT          (25*MINUTE*IN_MILLISECONDS)
#define BG_KT_EVENT_START_BATTLE  8563

enum BG_KT_Sound
{
    BG_KT_SOUND_ORB_PLACED        = 8232,
    BG_KT_SOUND_A_ORB_PICKED_UP   = 8174,
	BG_KT_SOUND_H_ORB_PICKED_UP   = 8174,
    BG_KT_SOUND_ORB_RESPAWNED     = 8232
};

enum BG_KT_SpellId
{
    BG_KT_SPELL_ORB               = 116524,         //ZONE1 GREEN
	BG_KT_SPELL_ORB_AURA          = 121220,         //ZONE1 GREEN
	BG_KT_SPELL_ORB1              = 121175,			//ZONE2 YELLOW
	BG_KT_SPELL_ORB1_AURA         = 121217,			//ZONE2 YELLOW
	BG_KT_SPELL_ORB2              = 112055,         //ZONE3 RED
	BG_KT_SPELL_ORB2_AURA         = 121221,         //ZONE3 RED
	BG_KT_SPELL_ORB_DROPPED       = 112839,
};

enum BG_KT_WorldStates
{
    BG_KT_ICON_A                  = 6308,
	BG_KT_ICON_H                  = 6307,
	BG_KT_ORB_POINTS_A            = 6303,
	BG_KT_ORB_POINTS_H            = 6304,
    BG_KT_ORB_STATE               = 6309,
    BG_KT_TIME_ENABLED            = 4247,
    BG_KT_TIME_REMAINING          = 4248
};

enum BG_KT_OrbState
{
    BG_KT_ORB_STATE_ON_BASE       = 0,
    BG_KT_ORB_STATE_WAIT_RESPAWN  = 1,
    BG_KT_ORB_STATE_ON_PLAYER     = 2,
    BG_KT_ORB_STATE_ON_GROUND     = 3
};

enum BG_KT_Graveyards
{
    KT_GRAVEYARD_RECTANGLEA1      = 3552,
    KT_GRAVEYARD_RECTANGLEA2      = 4058,
    KT_GRAVEYARD_RECTANGLEH1      = 3553,
    KT_GRAVEYARD_RECTANGLEH2      = 4057,
};

class BattleGroundKTScore : public BattlegroundScore
{
    public:
        BattleGroundKTScore() : OrbHandles(0) {};
        virtual ~BattleGroundKTScore() {};
        uint32 OrbHandles;
};


enum BG_TK_Events
{
    KT_EVENT_ORB                  = 0,
    // spiritguides will spawn (same moment, like TP_EVENT_DOOR_OPEN)
    KT_EVENT_SPIRITGUIDES_SPAWN   = 2
};

//tick point according to which zone
const uint32 BG_KT_TickPoints[3] = {2, 4, 6};
const uint32 BG_KTTickIntervals[2] = {0, 5};

class BattlegroundKT : public Battleground
{
    friend class BattleGroundMgr;

    public:
        /* Construction */
        BattlegroundKT();
        ~BattlegroundKT();
        void Update(uint32 diff);

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player *plr);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();

        /* BG Orbs */
        void SetAllianceOrbPicker(ObjectGuid guid) { m_OrbKeepers[BG_TEAM_ALLIANCE] = guid; }
		void SetHordeOrbPicker(ObjectGuid guid)    { m_OrbKeepers[BG_TEAM_HORDE] = guid; }
        void ClearAllianceOrbPicker()              { m_OrbKeepers[BG_TEAM_ALLIANCE].Clear(); }
        void ClearHordeOrbPicker()                 { m_OrbKeepers[BG_TEAM_HORDE].Clear(); }
        bool IsAllianceOrbPickedup() const         { return !m_OrbKeepers[BG_TEAM_ALLIANCE].IsEmpty(); }
		bool IsHordeOrbPickedup() const            { return !m_OrbKeepers[BG_TEAM_HORDE].IsEmpty(); }
        uint8 GetOrbState(Team team)             { return m_OrbState[GetTeamIndexByTeamId(team)]; }

        /* Battleground Events */
        virtual void EventPlayerDroppedOrb(Player *Source);
        virtual void EventPlayerClickedOnOrb(Player *Source, GameObject* target_obj);

        void RemovePlayer(Player *plr, ObjectGuid guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        void HandleKillPlayer(Player *player, Player *killer);
        bool SetupBattleGround();
        virtual void Reset();
        void EndBattleGround(Team winner);
        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        uint32 GetRemainingTimeInMinutes() { return m_EndTimer ? (m_EndTimer-1) / (MINUTE * IN_MILLISECONDS) + 1 : 0; }

        void UpdateOrbState(Team team, uint32 value);
        void UpdateTeamScore(Team team);
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value);
        void SetDroppedOrbGuid(ObjectGuid guid, Team team)  { m_DroppedOrbGuid[GetTeamIndexByTeamId(team)] = guid;}
        void ClearDroppedOrbGuid(Team team)  { m_DroppedOrbGuid[GetTeamIndexByTeamId(team)].Clear();}
        ObjectGuid const& GetDroppedOrbGuid(Team team) const { return m_DroppedOrbGuid[GetTeamIndexByTeamId(team)];}
        virtual void FillInitialWorldStates(WorldPacket& data, uint32& count);

        /* Scorekeeping */
        uint32 GetTeamScore(Team team) const            { return m_TeamScores[GetTeamIndexByTeamId(team)]; }
        void AddPoint(Team team, uint32 Points = 1)     { m_TeamScores[GetTeamIndexByTeamId(team)] += Points; }
        void SetTeamPoint(Team team, uint32 Points = 0) { m_TeamScores[GetTeamIndexByTeamId(team)] = Points; }
        void RemovePoint(Team team, uint32 Points = 1)  { m_TeamScores[GetTeamIndexByTeamId(team)] -= Points; }
    private:
        ObjectGuid m_OrbKeepers[BG_TEAMS_COUNT];

        ObjectGuid m_DroppedOrbGuid[BG_TEAMS_COUNT];
        uint8 m_OrbState[BG_TEAMS_COUNT];
        int32 m_OrbsTimer[BG_TEAMS_COUNT];
        int32 m_OrbsDropTimer[BG_TEAMS_COUNT];

        uint32 m_ReputationCapture;
        uint32 m_HonorWinKills;
        uint32 m_HonorEndKills;
        uint32 m_EndTimer;
        Team   m_LastCapturedOrbTeam;
};

#endif
