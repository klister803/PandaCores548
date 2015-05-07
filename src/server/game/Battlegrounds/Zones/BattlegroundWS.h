/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#ifndef __BATTLEGROUNDWS_H
#define __BATTLEGROUNDWS_H

#include "Battleground.h"


enum BG_WS_TimerOrScore
{
    BG_WS_MAX_TEAM_SCORE                    = 3,
    BG_WS_FLAG_RESPAWN_TIME                 = 23000,
    BG_WS_FLAG_DROP_TIME                    = 10000,
    BG_WS_SPELL_FORCE_TIME                  = 600000,
    BG_WS_SPELL_BRUTAL_TIME                 = 900000
};

enum BG_WS_Sound
{
    BG_WS_SOUND_FLAG_CAPTURED_ALLIANCE      = 8173,
    BG_WS_SOUND_FLAG_CAPTURED_HORDE         = 8213,
    BG_WS_SOUND_FLAG_PLACED                 = 8232,
    BG_WS_SOUND_FLAG_RETURNED               = 8192,
    BG_WS_SOUND_HORDE_FLAG_PICKED_UP        = 8212,
    BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP     = 8174,
    BG_WS_SOUND_FLAGS_RESPAWNED             = 8232
};

enum BG_WS_SpellId
{
    BG_WS_SPELL_HORDE_FLAG                  = 23333,
    BG_WS_SPELL_HORDE_FLAG_DROPPED          = 23334,
    BG_WS_SPELL_HORDE_FLAG_PICKED           = 61266,    ///< Fake Spell - Used as a start timer event
    BG_WS_SPELL_ALLIANCE_FLAG               = 23335,
    BG_WS_SPELL_ALLIANCE_FLAG_DROPPED       = 23336,
    BG_WS_SPELL_ALLIANCE_FLAG_PICKED        = 61265,    ///< Fake Spell - Used as a start timer event
    BG_WS_SPELL_FOCUSED_ASSAULT             = 46392,
    BG_WS_SPELL_BRUTAL_ASSAULT              = 46393,
    BG_WS_SPELL_REMOVE_CARRIED_FLAG         = 45919     ///< Need implementation. This spell if casted on each player from 1 faction when the flag is captured / dropped. + Remove unnecesary RemoveauraDueToSpell() from code.
};

/// To Do: Find what unk world states means and rename
enum BG_WS_WorldStates
{
    BG_WS_FLAG_UNK_ALLIANCE                 = 1545, ///< Value: -1 when alliance flag is dropped | 1 when alliance flag is on player | 0 On base | -2 ???
    BG_WS_FLAG_UNK_HORDE                    = 1546, ///< Value: -1 when horde flag is dropped    | 1 when horde flag is on player    | 0 On base | -2 ???
    BG_WS_FLAG_UNKNOWN                      = 1547, ///< -1 before capturing flag, 0 after both flags respawned
    BG_WS_FLAG_CAPTURES_ALLIANCE            = 1581,
    BG_WS_FLAG_CAPTURES_HORDE               = 1582,
    BG_WS_FLAG_CAPTURES_MAX                 = 1601,
    BG_WS_FLAG_STATE_HORDE                  = 2338,
    BG_WS_FLAG_STATE_ALLIANCE               = 2339,
    BG_WS_STATE_TIMER                       = 4248,
    BG_WS_STATE_TIMER_ACTIVE                = 4247,
    BG_WS_STATE_UNKNOWN                     = 4249, ///< Used after flag is captured (value: 1)
};

enum BG_WS_ObjectTypes
{
    BG_WS_OBJECT_DOOR_A_1       = 0,
    BG_WS_OBJECT_DOOR_A_2       = 1,
    BG_WS_OBJECT_DOOR_A_3       = 2,
    BG_WS_OBJECT_DOOR_A_4       = 3,
    BG_WS_OBJECT_DOOR_A_5       = 4,
    BG_WS_OBJECT_DOOR_A_6       = 5,
    BG_WS_OBJECT_DOOR_H_1       = 6,
    BG_WS_OBJECT_DOOR_H_2       = 7,
    BG_WS_OBJECT_DOOR_H_3       = 8,
    BG_WS_OBJECT_DOOR_H_4       = 9,
    BG_WS_OBJECT_A_FLAG         = 10,
    BG_WS_OBJECT_H_FLAG         = 11,
    BG_WS_OBJECT_SPEEDBUFF_1    = 12,
    BG_WS_OBJECT_SPEEDBUFF_2    = 13,
    BG_WS_OBJECT_REGENBUFF_1    = 14,
    BG_WS_OBJECT_REGENBUFF_2    = 15,
    BG_WS_OBJECT_BERSERKBUFF_1  = 16,
    BG_WS_OBJECT_BERSERKBUFF_2  = 17,
    BG_WS_OBJECT_MAX            = 18
};

enum BG_WS_ObjectEntry
{
    BG_OBJECT_DOOR_A_1_WS_ENTRY          = 179918,
    BG_OBJECT_DOOR_A_2_WS_ENTRY          = 179919,
    BG_OBJECT_DOOR_A_3_WS_ENTRY          = 179920,
    BG_OBJECT_DOOR_A_4_WS_ENTRY          = 179921,
    BG_OBJECT_DOOR_A_5_WS_ENTRY          = 180322,
    BG_OBJECT_DOOR_A_6_WS_ENTRY          = 180322,
    BG_OBJECT_DOOR_H_1_WS_ENTRY          = 179916,
    BG_OBJECT_DOOR_H_2_WS_ENTRY          = 179917,
    BG_OBJECT_DOOR_H_3_WS_ENTRY          = 180322,
    BG_OBJECT_DOOR_H_4_WS_ENTRY          = 180322,
    BG_OBJECT_A_FLAG_WS_ENTRY            = 179830,
    BG_OBJECT_H_FLAG_WS_ENTRY            = 179831,
    BG_OBJECT_A_FLAG_GROUND_WS_ENTRY     = 179785,
    BG_OBJECT_H_FLAG_GROUND_WS_ENTRY     = 179786
};

enum BG_WS_FlagState
{
    BG_WS_FLAG_STATE_ON_BASE                = 0,
    BG_WS_FLAG_STATE_WAIT_RESPAWN,
    BG_WS_FLAG_STATE_ON_PLAYER,
    BG_WS_FLAG_STATE_ON_GROUND,
};

enum BG_WS_Graveyards
{
    WS_GRAVEYARD_FLAGROOM_ALLIANCE = 769,
    WS_GRAVEYARD_FLAGROOM_HORDE    = 770,
    WS_GRAVEYARD_MAIN_ALLIANCE     = 771,
    WS_GRAVEYARD_MAIN_HORDE        = 772
};

enum BG_WS_CreatureTypes
{
    WS_SPIRIT_MAIN_ALLIANCE   = 0,
    WS_SPIRIT_MAIN_HORDE      = 1,

    BG_CREATURES_MAX_WS       = 2
};

enum BG_WS_CarrierDebuffs
{
    WS_SPELL_FOCUSED_ASSAULT                = 46392,
    WS_SPELL_BRUTAL_ASSAULT                 = 46393
};

enum BG_WS_Objectives
{
    WS_OBJECTIVE_CAPTURE_FLAG   = 42,
    WS_OBJECTIVE_RETURN_FLAG    = 44
};

#define WS_EVENT_START_BATTLE   8563

// Class for scorekeeping
class BattlegroundWGScore : public BattlegroundScore
{
    public:
        BattlegroundWGScore() : FlagCaptures(0), FlagReturns(0) {};
        virtual ~BattlegroundWGScore() {};
        uint32 FlagCaptures;
        uint32 FlagReturns;
};

// Main class for Twin Peaks Battleground
class BattlegroundWS : public Battleground
{
    friend class BattlegroundMgr;


    public:
        BattlegroundWS();
        ~BattlegroundWS();
        /**
         * \brief Called every time for update battle data
         */
        void PostUpdateImpl(uint32 diff);

        /* Inherited from BattlegroundClass */

        /// Called when a player join battle
        void AddPlayer(Player* player);
        /// Called when a player leave battleground
        void RemovePlayer(Player* player, uint64 guid, uint32 team);

        /// Called when battle start
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();
        /// Called for initialize battleground, after that the first player be entered (Mainly used to generate NPCs)
        bool SetupBattleground();
        void Reset();
        /// Called for generate packet contain worldstate data (Time + Score on the top of the screen)
        void FillInitialWorldStates(WorldPacket& data);

        /// Called on battleground ending
        void EndBattleground(uint32 winner);

        /// Return the nearest graveyard where player can respawn (Spirits in this Battle are: in Base, in Middle and if a player dies before battle start, to prevent cheating in main room(improbably to happen))
        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        /// Called when a player is muredered by another player (If the killed player has the flag to drop it) (In this BG a player can murdered only by another player)
        void HandleKillPlayer(Player *player, Player *killer);

        /// Called in HandleBattlegroundPlayerPositionsOpcode for tracking player on map
        uint64 GetFlagPickerGUID(int32 team) const
        {
            if (team == TEAM_ALLIANCE || team == TEAM_HORDE)
                return _flagKeepers[team];
            return 0;
        }
        void SetAllianceFlagPicker(uint64 guid)                 { _flagKeepers[TEAM_ALLIANCE] = guid; }
        void SetHordeFlagPicker(uint64 guid)                    { _flagKeepers[TEAM_HORDE] = guid; }
        bool IsAllianceFlagPickedup() const                     { return _flagKeepers[TEAM_ALLIANCE] != 0; }
        bool IsHordeFlagPickedup() const                        { return _flagKeepers[TEAM_HORDE] != 0; }

        /// Called when a player hits an area. (Like when is within distance to capture the flag (mainly used for this))
        void HandleAreaTrigger(Player* Source, uint32 Trigger);

        void SendFlagsPositionsUpdate(bool sendIfEmpty = false);
        
        uint8 GetFlagState(uint32 team)             { return _flagState[GetTeamIndexByTeamId(team)]; }
        /* Update Score */
        /// Update score board
        void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);
        /// Update score on the top of screen by worldstates
        void UpdateTeamScore(uint32 team);

        void SetDroppedFlagGUID(uint64 guid, uint32 TeamID)  { _droppedFlagGUID[GetTeamIndexByTeamId(TeamID)] = guid;}

private:
        /// Internal Battlegorund methods

        /// Scorekeeping
        /// Add 1 point after a team captures the flag
        void AddPoint(uint32 teamID)                            { ++m_TeamScores[GetTeamIndexByTeamId(teamID)]; }

        /// Flag Events
        /// Update Flag state of one team, if the flag is in base, is waitng for respawn, is on player or on ground(if a player droped it)
        void UpdateFlagState(uint32 team, uint32 value, uint64 flagKeeperGUID = 0);
        /// Used to maintain the last team witch captured the flag (see def of _lastFlagCaptureTeam)
        void SetLastFlagCapture(uint32 teamID)                  { _lastFlagCaptureTeam = teamID; }
        /// Respawn flag method
        void RespawnFlag(uint32 team, bool captured = false);
        /// EVENT: Happened when a player drops the flag
        void EventPlayerDroppedFlag(Player* source);
        /// EVENT: Happened when a player clicks on the flag
        void EventPlayerClickedOnFlag(Player* source, GameObject* target_obj);
        /// EVENT: Happened when a player captured(placed it in base) the flag
        void EventPlayerCapturedFlag(Player* source);

        /// Members:
        uint64 _flagKeepers[2];         ///< Maintains the flag picker GUID: 0 for ALLIANCE FLAG and 1 for HORDE FLAG (EX: _flagKeepers[TEAM_ALLIANCE] is guid for a horde player)
        uint64 _droppedFlagGUID[2];     ///< If the flag is on the ground(dropped by a player) we must maintain its guid to dispawn it when a player clicks on it. (else it will automatically dispawn)
        uint8 _flagState[2];            ///< Show where flag is (in base / on ground / on player)
        int32 _flagsTimer;              ///< Timer for flags that are unspawn after a capture
        int32 _flagsDropTimer[2];       ///< Used for counting how much time have passed since the flag dropped
        uint32 _lastFlagCaptureTeam;    ///< If the score is equal and the time expires the winer is based on witch team captured the last flag
        int32 _flagSpellForceTimer;     ///< Used for counting how much time have passed since the both flags are kept
        bool _bothFlagsKept;            ///< shows if both flags are kept
        uint8 _flagDebuffState;         ///< This maintain the debuff state of the flag carrier. If the flag is on a player for more then X minutes, the player will be cursed with an debuff. (0 - No debuff, 1 - Focus assault, 2 - Brutal assault)
        uint8 _minutesElapsed;          ///< Elapsed time since the beginning of the battleground (It counts as well the beginning time(when the doors are closed))
        
        uint32 m_ReputationCapture;
        uint32 m_HonorWinKills;
        uint32 m_HonorEndKills;
        
        int32 _flagPosTimer;
};

#endif
