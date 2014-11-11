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

#include "Battleground.h"
#include "BattlegroundWS.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

BattlegroundWS::BattlegroundWS()
{
    /// Resize BG miscellaneous
    BgObjects.resize(BG_WS_OBJECT_MAX);
    BgCreatures.resize(BG_CREATURES_MAX_WS);

    /// Start Messages Initialization
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_WS_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;

    /// Misc Stuff - See declaration for more info
    _minutesElapsed = 0;
    _flagSpellForceTimer = 0;
    _bothFlagsKept = false;
    _flagDebuffState = 0;
    
    m_ReputationCapture = 0;
}

BattlegroundWS::~BattlegroundWS()
{
}

void BattlegroundWS::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        /// Total time it's supposed to be 20 (game) + 2 (when the battle begins and doors are closed) minutes
        if (GetElapsedTime() >= 22 * MINUTE * IN_MILLISECONDS) ///< End of game - Verify score
        {
            if (m_TeamScores[TEAM_ALLIANCE] == 0)
            {
                if (m_TeamScores[TEAM_HORDE] == 0)  ///< No one scored
                    EndBattleground(WINNER_NONE);
                else                                ///< Horde has more points (Alliance has 0) - Horde Wins
                    EndBattleground(HORDE);
            }
            else if (m_TeamScores[TEAM_HORDE] == 0) ///< Alliance has more than 0, Horde has 0 so Alliance Wins
                EndBattleground(ALLIANCE);
            else if (m_TeamScores[TEAM_HORDE] == m_TeamScores[TEAM_ALLIANCE]) ///< Team Score equal and both scored at least once so winer is the last team witch captured the flag
                EndBattleground(_lastFlagCaptureTeam);
            else if (m_TeamScores[TEAM_HORDE] > m_TeamScores[TEAM_ALLIANCE])  // Last but not least, check who has the higher score
                EndBattleground(HORDE);
            else
                EndBattleground(ALLIANCE);
        }
        // First Update is needed only after 1 minute in battle
        else if (GetElapsedTime() > uint32((_minutesElapsed + 1) * MINUTE * IN_MILLISECONDS) +  2 * MINUTE * IN_MILLISECONDS)
        {
            ++_minutesElapsed;
            UpdateWorldState(BG_WS_STATE_TIMER, 20 - _minutesElapsed); //< Time remaining showed on top of the screen via world state
        }

        /// Flags state update:
        for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
        {
            switch(_flagState[team])
            {
                case BG_WS_FLAG_STATE_WAIT_RESPAWN: ///< Flag is nowhere - Is in this situation if a team captured the flag
                {
                    _flagsTimer -= diff; ///< Update Flag Timer with the time between the last server update and now
                    if (_flagsTimer <= 0)
                    {
                        _flagsTimer = 0;
                        RespawnFlag(team, true);
                        UpdateWorldState(BG_WS_FLAG_UNKNOWN, 0); ///< From sniffs
                    }
                    break;
                }
                case BG_WS_FLAG_STATE_ON_GROUND: ///< Flag is on the ground waiting someone to click
                {
                    _flagsDropTimer[team] -= diff; ///< Update Flag Timer with the time between the last server update and now
                    if (_flagsDropTimer[team] < 0)
                    {
                        _flagsDropTimer[team] = 0;

                        /// Respawn flag
                        RespawnFlag(team, false);

                        /// Delete flag from ground
                        if (GameObject* obj = GetBgMap()->GetGameObject(_droppedFlagGUID[team]))
                                obj->Delete();
                            else
                                sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundWS: An error has occurred in PostUpdateImpl: Unknown dropped flag GUID: %u", GUID_LOPART(_droppedFlagGUID[team]));

                        _droppedFlagGUID[team] = 0;

                        /// If both flags are kept and 1 of cariers dies and no one clicked on flag set _bothflagskept = false
                        if (_bothFlagsKept)
                        {
                            _bothFlagsKept = false;
                            _flagDebuffState = 0;
                            _flagSpellForceTimer = 0;
                        }
                    }
                    break;
                }

            }
        }


        /**
        * If both flags are kept, for more than 3 minutes, both cariers should recive an course (Focused Assault).
        * Every minute afterward, an additional stack will be applied.
        * After 7 minutes, Brutal Assault will be applied in place of Focused Assault.
        */
        if (_bothFlagsKept)
        {
            _flagSpellForceTimer += diff; /// Update the timer

            if (_flagSpellForceTimer >= (3 + _flagDebuffState) * MINUTE* IN_MILLISECONDS && _flagDebuffState < 10)///< More than 3 minutes and amount of stacks from debuff lower than 10
            {
                _flagDebuffState = _flagSpellForceTimer / MINUTE / IN_MILLISECONDS - 2;
                if (_flagDebuffState <= 5) ///< Cast on player Focused Assaultupposed to maintain number of stacks for debuff
                {
                    for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
                    {
                        if (Player* player = ObjectAccessor::FindPlayer(_flagKeepers[team]))
                            player->CastSpell(player, WS_SPELL_FOCUSED_ASSAULT, true);
                        else
                            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundWS: Can't find flag keeper with GUID: %u, in team: %s", _flagKeepers[team], team == TEAM_ALLIANCE ? "ALLIANCE" : "HORDE");
                    }
                }
                else if (_flagDebuffState == 6) ///< Moment when you change the stacks
                {
                    for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
                    {
                        if (Player* player = ObjectAccessor::FindPlayer(_flagKeepers[team]))
                        {
                            player->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT); ///< Remove aura from Focused Assault
                            player->CastCustomSpell(WS_SPELL_BRUTAL_ASSAULT, SPELLVALUE_AURA_STACK, _flagDebuffState, player);
                        }
                        else
                            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundWS: Can't find flag keeper with GUID: %u, in team: %s", _flagKeepers[team], team == TEAM_ALLIANCE ? "ALLIANCE" : "HORDE");
                    }
                }
                else //< Continue to cast brutal Assault on target
                {
                    for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
                    {
                        if (Player* player = ObjectAccessor::FindPlayer(_flagKeepers[team]))
                            player->CastSpell(player, WS_SPELL_BRUTAL_ASSAULT, true);
                        else
                            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundWS: Can't find flag keeper with GUID: %u, in team: %s", _flagKeepers[team], team == TEAM_ALLIANCE ? "ALLIANCE" : "HORDE");
                    }
                }
            }
        }
    }
}

void BattlegroundWS::AddPlayer(Player* player)
{
    ///Create score for player
    AddPlayerScore(player->GetGUID(), new BattlegroundWGScore);
    Battleground::AddPlayer(player);
}


void BattlegroundWS::StartingEventCloseDoors()
{
    for (uint32 i = BG_WS_OBJECT_DOOR_A_1; i <= BG_WS_OBJECT_DOOR_H_3; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }
    for (uint32 i = BG_WS_OBJECT_A_FLAG; i <= BG_WS_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);
}

void BattlegroundWS::StartingEventOpenDoors()
{
    for (uint32 i = BG_WS_OBJECT_DOOR_A_1; i <= BG_WS_OBJECT_DOOR_A_6; ++i)
        DoorOpen(i);
    for (uint32 i = BG_WS_OBJECT_DOOR_H_1; i <= BG_WS_OBJECT_DOOR_H_4; ++i)
        DoorOpen(i);

    for (uint32 i = BG_WS_OBJECT_A_FLAG; i <= BG_WS_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);

    SpawnBGObject(BG_WS_OBJECT_DOOR_A_5, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_A_6, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_H_3, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_H_4, RESPAWN_ONE_DAY);
    
    // players joining later are not eligibles
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, WS_EVENT_START_BATTLE);
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT2, WS_EVENT_START_BATTLE);

    UpdateWorldState(BG_WS_STATE_TIMER_ACTIVE, 1);
    UpdateWorldState(BG_WS_STATE_TIMER, 20);
}

bool BattlegroundWS::SetupBattleground()
{
    // flags
    if (!AddObject(BG_WS_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_WS_ENTRY, 1540.423f, 1481.325f, 351.8284f, 3.089233f, 0, 0, 0.9996573f, 0.02617699f, BG_WS_FLAG_RESPAWN_TIME/1000)
        || !AddObject(BG_WS_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_WS_ENTRY, 916.0226f, 1434.405f, 345.413f, 0.01745329f, 0, 0, 0.008726535f, 0.9999619f, BG_WS_FLAG_RESPAWN_TIME/1000)
        // buffs
        || !AddObject(BG_WS_OBJECT_SPEEDBUFF_1, BG_OBJECTID_SPEEDBUFF_ENTRY, 1449.93f, 1470.71f, 342.6346f, -1.64061f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_SPEEDBUFF_2, BG_OBJECTID_SPEEDBUFF_ENTRY, 1005.171f, 1447.946f, 335.9032f, 1.64061f, 0, 0, 0.7313537f, 0.6819984f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_REGENBUFF_1, BG_OBJECTID_REGENBUFF_ENTRY, 1317.506f, 1550.851f, 313.2344f, -0.2617996f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_REGENBUFF_2, BG_OBJECTID_REGENBUFF_ENTRY, 1110.451f, 1353.656f, 316.5181f, -0.6806787f, 0, 0, 0.333807f, -0.9426414f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_BERSERKBUFF_1, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1320.09f, 1378.79f, 314.7532f, 1.186824f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_BERSERKBUFF_2, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1139.688f, 1560.288f, 306.8432f, -2.443461f, 0, 0, 0.9396926f, -0.3420201f, BUFF_RESPAWN_TIME)
        // alliance gates
        || !AddObject(BG_WS_OBJECT_DOOR_A_1, BG_OBJECT_DOOR_A_1_WS_ENTRY, 1503.335f, 1493.466f, 352.1888f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_2, BG_OBJECT_DOOR_A_2_WS_ENTRY, 1492.478f, 1457.912f, 342.9689f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_3, BG_OBJECT_DOOR_A_3_WS_ENTRY, 1468.503f, 1494.357f, 351.8618f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_4, BG_OBJECT_DOOR_A_4_WS_ENTRY, 1471.555f, 1458.778f, 362.6332f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_5, BG_OBJECT_DOOR_A_5_WS_ENTRY, 1492.347f, 1458.34f, 342.3712f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_6, BG_OBJECT_DOOR_A_6_WS_ENTRY, 1503.466f, 1493.367f, 351.7352f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY)
        // horde gates
        || !AddObject(BG_WS_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_WS_ENTRY, 949.1663f, 1423.772f, 345.6241f, -0.5756807f, -0.01673368f, -0.004956111f, -0.2839723f, 0.9586737f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_WS_ENTRY, 953.0507f, 1459.842f, 340.6526f, -1.99662f, -0.1971825f, 0.1575096f, -0.8239487f, 0.5073641f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_H_3, BG_OBJECT_DOOR_H_3_WS_ENTRY, 949.9523f, 1422.751f, 344.9273f, 0.0f, 0, 0, 0, 1, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_H_4, BG_OBJECT_DOOR_H_4_WS_ENTRY, 950.7952f, 1459.583f, 342.1523f, 0.05235988f, 0, 0, 0.02617695f, 0.9996573f, RESPAWN_IMMEDIATELY)
        )
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundWS: Failed to spawn some object Battleground not created!");
        return false;
    }

    WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_ALLIANCE);
    if (!sg || !AddSpiritGuide(WS_SPIRIT_MAIN_ALLIANCE, sg->x, sg->y, sg->z, 3.124139f, ALLIANCE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundWS: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_HORDE);
    if (!sg || !AddSpiritGuide(WS_SPIRIT_MAIN_HORDE, sg->x, sg->y, sg->z, 3.193953f, HORDE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundWS: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    sLog->outDebug(LOG_FILTER_BATTLEGROUND, "BatteGroundWS: BG objects and spirit guides spawned");

    return true;
}

void BattlegroundWS::Reset()
{
    /// Call Main Battleground reset
    Battleground::Reset();

    /// Start reset for Battleground specific miscelanouse
    for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
    {
        /// Unbind all flag stuff
        _flagKeepers[team] = 0;
        _droppedFlagGUID[team] = 0;
        _flagsDropTimer[team] = 0;
        _flagState[team] = BG_WS_FLAG_STATE_ON_BASE;

        /// Reset score
        m_TeamScores[team] = 0;
        
        if (sBattlegroundMgr->IsBGWeekend(GetTypeID()))
        {
            m_ReputationCapture = 45;
            //m_HonorWinKills = 3;
            //m_HonorEndKills = 4;
        }
        else
        {
            m_ReputationCapture = 35;
            //m_HonorWinKills = 1;
            //m_HonorEndKills = 2;
        }
    }

    _flagsTimer = 0;
}

void BattlegroundWS::FillInitialWorldStates(WorldPacket& data)
{
    /// Show how many flags had been captured
    FillInitialWorldState(data, BG_WS_FLAG_CAPTURES_ALLIANCE, m_TeamScores[TEAM_ALLIANCE]);
    FillInitialWorldState(data, BG_WS_FLAG_CAPTURES_HORDE, m_TeamScores[TEAM_HORDE]);

    /// Show MAX number of flags (x/3)
    FillInitialWorldState(data, BG_WS_FLAG_CAPTURES_MAX, BG_WS_MAX_TEAM_SCORE);

    /// Next Stuff showed only if BG is in progress
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        /// Show Flag state - if flag is on player / ground / in base
        for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
        {
            switch(_flagState[team])
            {
                case BG_WS_FLAG_STATE_ON_GROUND:
                    FillInitialWorldState(data, BG_WS_FLAG_UNK_ALLIANCE + team, -1);
                    FillInitialWorldState(data, BG_WS_FLAG_STATE_HORDE + team, 3); ///< Show if team's flag is carried
                    break;
                case BG_WS_FLAG_STATE_ON_PLAYER:
                    FillInitialWorldState(data, BG_WS_FLAG_UNK_ALLIANCE + team, 1);
                    FillInitialWorldState(data, BG_WS_FLAG_STATE_HORDE + team, 2); ///< Show if team's flag is carried
                    break;
                default: ///< In Base
                    FillInitialWorldState(data, BG_WS_FLAG_UNK_ALLIANCE + team, 0);
                    FillInitialWorldState(data, BG_WS_FLAG_STATE_HORDE + team, 1); ///< Show if team's flag is carried
                    break;
            }
        }

        /// Show Timer
        FillInitialWorldState(data, BG_WS_STATE_TIMER_ACTIVE, 1);
        FillInitialWorldState(data, BG_WS_STATE_TIMER, 20 - _minutesElapsed);
    }
    else
    {
        /// No timer for begining
        FillInitialWorldState(data, BG_WS_STATE_TIMER_ACTIVE, 0);

        /// Just show the maxscore and actual score (0)
        FillInitialWorldState(data, BG_WS_FLAG_UNK_ALLIANCE, 0);
        FillInitialWorldState(data, BG_WS_FLAG_UNK_HORDE, 0);
        FillInitialWorldState(data, BG_WS_FLAG_STATE_HORDE, 1);
        FillInitialWorldState(data, BG_WS_FLAG_STATE_ALLIANCE, 1);
    }
}

void BattlegroundWS::EndBattleground(uint32 winner)
{
    /// If BG ends with equal flag captured (draw) and both flags are kept the debuffs + flag aura stays on player, and it shouldn't
    for (uint8 team = TEAM_ALLIANCE; team <= TEAM_HORDE; ++team)
    {
        if (Player* player = ObjectAccessor::FindPlayer(_flagKeepers[team]))
        {
            player->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
            player->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
        }
    }

    /// Update a lot of world states
    UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 0);
    UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 0);
    UpdateWorldState(BG_WS_FLAG_STATE_ALLIANCE, 1);
    UpdateWorldState(BG_WS_FLAG_STATE_HORDE, 1);
    UpdateWorldState(BG_WS_STATE_TIMER_ACTIVE, 0);

    uint32 realWinner = WINNER_NONE;
    if (winner == TEAM_ALLIANCE)
        realWinner = ALLIANCE;
    else if (winner == TEAM_HORDE)
        realWinner = HORDE;
    else if (winner > WINNER_NONE)
        realWinner = winner;

    Battleground::EndBattleground(realWinner);
}

void BattlegroundWS::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    /// Call event player droped flag
    if(player->HasAura(BG_WS_SPELL_HORDE_FLAG) || player->HasAura(BG_WS_SPELL_ALLIANCE_FLAG))
        EventPlayerDroppedFlag(player);

    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundWS::HandleAreaTrigger(Player* player, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch (trigger)
    {
        case 3686:                                          // Alliance elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = BgObjects[BG_WS_OBJECT_SPEEDBUFF_1];
            break;
        case 3687:                                          // Horde elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = BgObjects[BG_WS_OBJECT_SPEEDBUFF_2];
            break;
        case 3706:                                          // Alliance elixir of regeneration spawn
            //buff_guid = BgObjects[BG_WS_OBJECT_REGENBUFF_1];
            break;
        case 3708:                                          // Horde elixir of regeneration spawn
            //buff_guid = BgObjects[BG_WS_OBJECT_REGENBUFF_2];
            break;
        case 3707:                                          // Alliance elixir of berserk spawn
            //buff_guid = BgObjects[BG_WS_OBJECT_BERSERKBUFF_1];
            break;
        case 3709:                                          // Horde elixir of berserk spawn
            //buff_guid = BgObjects[BG_WS_OBJECT_BERSERKBUFF_2];
            break;
        case 3646:                                          // Alliance Flag spawn
            if (_flagState[TEAM_HORDE] && !_flagState[TEAM_ALLIANCE])
                if (_flagKeepers[TEAM_HORDE] == player->GetGUID())
                    EventPlayerCapturedFlag(player);
            break;
        case 3647:                                          // Horde Flag spawn
            if (_flagState[BG_TEAM_ALLIANCE] && !_flagState[BG_TEAM_HORDE])
                if (GetFlagPickerGUID(BG_TEAM_ALLIANCE) == player->GetGUID())
                    EventPlayerCapturedFlag(player);
            break;
        case 3649:                                          // unk1
        case 3688:                                          // unk2
        case 4628:                                          // unk3
        case 4629:                                          // unk4
        case 8966:                                          // unk5
        case 8965:                                          // unk6
            break;
        default:
            sLog->outError(LOG_FILTER_BATTLEGROUND, "WARNING: Unhandled AreaTrigger in Battleground: %u", trigger);
            player->GetSession()->SendNotification("Warning: Unhandled AreaTrigger in Battleground: %u", trigger);
            break;
    }
}

void BattlegroundWS::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
    /// Find player in map
    BattlegroundScoreMap::iterator itr = PlayerScores.find(player->GetGUID());
    if (itr == PlayerScores.end()) ///< Player not found
        return;

    /// Update Achievements + scores
    switch (type)
    {
        case SCORE_FLAG_CAPTURES:
            ((BattlegroundWGScore*)itr->second)->FlagCaptures += value;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, WS_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:
            ((BattlegroundWGScore*)itr->second)->FlagReturns += value;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, WS_OBJECTIVE_RETURN_FLAG);
            break;
        default:///< else only count another kill
            Battleground::UpdatePlayerScore(player, type, value, doAddHonor);
            break;
    }
}

void BattlegroundWS::UpdateTeamScore(uint32 team)
{
    ///< After a capture update score
    if (team == TEAM_ALLIANCE)
        UpdateWorldState(BG_WS_FLAG_CAPTURES_ALLIANCE, m_TeamScores[team]);
    else
        UpdateWorldState(BG_WS_FLAG_CAPTURES_HORDE, m_TeamScores[team]);
}

/**
* Next method finds the right graveyard for player to resurect
* As 4.1: The right graveyard is not the nearest one
* So:
* Players will now only spawn at their base graveyard when they die in the enemy base.
* Defending players will respawn at the middle graveyard.
* Midfield players will respawn at the middle graveyard.
* Attacking players will respawn at their base graveyard.
* It's not the best "name" for this method
*/
WorldSafeLocsEntry const* BattlegroundWS::GetClosestGraveYard(Player* player)
{
    //if status in progress, it returns main graveyards with spiritguides
    //else it will return the graveyard in the flagroom - this is especially good
    //if a player dies in preparation phase - then the player can't cheat
    //and teleport to the graveyard outside the flagroom
    //and start running around, while the doors are still closed
    if (player->GetBGTeam() == ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_ALLIANCE);
        else
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_FLAGROOM_ALLIANCE);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_HORDE);
        else
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_FLAGROOM_HORDE);
    }
}

void BattlegroundWS::EventPlayerDroppedFlag(Player* source)
{
    uint8 team = source->GetBGTeamId();

    /// Mainly used when a player captures the flag, it prevents spawn the flag on ground
    if (!_flagKeepers[team ^ 1])
        return;

    /// Most probably useless - If a GM applies the aura on a player
    if (!_flagKeepers[team ^ 1] == source->GetGUID())
    {
        sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundWS: An error have occured in EventPlayerDroppedFlag, player: %u who carried the flag is not the flag keeper: %u.", source->GetGUID(), _flagKeepers[team ^ 1]);
        return;
    }

    /// Unaura assaults debuff (if there are)
        source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
        source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);

    /// Update flag state and flagkeepers
    UpdateFlagState(team ^ 1, BG_WS_FLAG_STATE_ON_GROUND);

    /// "Drop the flag" + Set timer for flag
    source->CastSpell(source, team == TEAM_ALLIANCE ? BG_WS_SPELL_HORDE_FLAG_DROPPED : BG_WS_SPELL_ALLIANCE_FLAG_DROPPED, true);
    _flagsDropTimer[team ^ 1] = BG_WS_FLAG_DROP_TIME;

    /// Remove flag
    source->RemoveAurasDueToSpell(team == TEAM_ALLIANCE ? BG_WS_SPELL_HORDE_FLAG : BG_WS_SPELL_ALLIANCE_FLAG);

    /// Apply debuff for recently dropped flag
    source->CastSpell(source, SPELL_RECENTLY_DROPPED_FLAG, true);

    /// Send message to all
    SendMessageToAll(team == TEAM_ALLIANCE ? LANG_BG_WS_DROPPED_HF : LANG_BG_WS_DROPPED_AF, team == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_HORDE : CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
}

void BattlegroundWS::EventPlayerClickedOnFlag(Player* source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 team = source->GetBGTeamId();

    if (source->IsWithinDistInMap(target_obj, 10)) ///< Check if target is in distance with flag
    {
        switch (_flagState[team ^ 1]) ///< Check opposite team faction state
        {
            case BG_WS_FLAG_STATE_ON_BASE: ///< If opposite team has flag in base
            {
                if (BgObjects[BG_WS_OBJECT_A_FLAG + (team ^ 1)] == target_obj->GetGUID()) ///< If clicked object is what we want
                {
                    ///Dispawn BGObject
                    SpawnBGObject(team == TEAM_ALLIANCE ? BG_WS_OBJECT_H_FLAG : BG_WS_OBJECT_A_FLAG, RESPAWN_ONE_DAY);

                    /// Update flag state + world state
                    UpdateFlagState(team ^ 1, BG_WS_FLAG_STATE_ON_PLAYER, source->GetGUID());

                    /// "Pick up the flag"
                    source->CastSpell(source, team == TEAM_ALLIANCE ? BG_WS_SPELL_HORDE_FLAG : BG_WS_SPELL_ALLIANCE_FLAG , true);

                    /// Start achievement criteria
                    // source->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, TEAM_ALLIANCE ? BG_WS_SPELL_HORDE_FLAG_PICKED : BG_WS_SPELL_ALLIANCE_FLAG_PICKED);

                    /// Verify if both flag are carried
                    if (_flagKeepers[team])
                        _bothFlagsKept = true;

                    /// Send message to all players + Play sound
                    SendMessageToAll(team == TEAM_ALLIANCE ? LANG_BG_WS_PICKEDUP_HF : LANG_BG_WS_PICKEDUP_AF, team == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE : CHAT_MSG_BG_SYSTEM_HORDE, source);
                    PlaySoundToAll(team == TEAM_ALLIANCE ? BG_WS_SOUND_HORDE_FLAG_PICKED_UP : BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP);
                }
                break;
            }
            case BG_WS_FLAG_STATE_ON_GROUND:
            {
                if (_droppedFlagGUID[team ^ 1] == target_obj->GetGUID()) ///< Check opposite team faction flag
                {
                    /// Reapply flag on target
                    source->CastSpell(source, team == TEAM_ALLIANCE? BG_WS_SPELL_HORDE_FLAG : BG_WS_SPELL_ALLIANCE_FLAG, true);

                    /// Update flag state
                    UpdateFlagState(team ^ 1, BG_WS_FLAG_STATE_ON_PLAYER, source->GetGUID());

                    /// Reapply debuff
                    if (_flagDebuffState  && _flagDebuffState < 6)
                        source->CastCustomSpell(WS_SPELL_FOCUSED_ASSAULT, SPELLVALUE_AURA_STACK, _flagDebuffState, source);
                    else if (_flagDebuffState >= 6)
                        source->CastCustomSpell(WS_SPELL_BRUTAL_ASSAULT, SPELLVALUE_AURA_STACK, _flagDebuffState, source);

                    /// Reset flag on the ground counter
                    _flagsDropTimer[team ^ 1] = 0;

                    /// Announce players
                    PlaySoundToAll(team == TEAM_ALLIANCE ? BG_WS_SOUND_HORDE_FLAG_PICKED_UP : BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP);
                    SendMessageToAll(team == TEAM_ALLIANCE ? LANG_BG_WS_PICKEDUP_HF : LANG_BG_WS_PICKEDUP_AF, team == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE : CHAT_MSG_BG_SYSTEM_HORDE, source);
                }
                break;
            }
        }

        /// If player from same side click on the flag that stand on ground return to base
        if (_flagState[team] == BG_WS_FLAG_STATE_ON_GROUND &&
            _droppedFlagGUID[team] == target_obj->GetGUID())
        {
            /// Update world states
            UpdateFlagState(team, BG_WS_FLAG_STATE_WAIT_RESPAWN);
            UpdatePlayerScore(source, SCORE_FLAG_RETURNS, 1);

            /// Respawn Flag
            RespawnFlag(team, false);

            /// Announce players
            PlaySoundToAll(BG_WS_SOUND_FLAG_RETURNED);
            SendMessageToAll(team == TEAM_ALLIANCE ? LANG_BG_WS_RETURNED_AF: LANG_BG_WS_RETURNED_HF, team == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE : CHAT_MSG_BG_SYSTEM_HORDE, source);

            /// Reset both flags things
            _bothFlagsKept = false;
            _flagSpellForceTimer = 0;
        }
    }

   source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundWS::EventPlayerCapturedFlag(Player* source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 team = source->GetBGTeamId();

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    _flagDebuffState = 0;

    /// Dispawn Flags
    SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_ONE_DAY);

    /// Unaura assaults debuff (if there are)
        source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
        source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);

    /// Reward flag capture with 2x honorable kills
    RewardHonorToTeam(GetBonusHonorFromKill(2), source->GetBGTeam());

    if (source->GetBGTeam() == ALLIANCE)
        RewardReputationToTeam(890, m_ReputationCapture, ALLIANCE);
    else
        RewardReputationToTeam(889, m_ReputationCapture, HORDE);

    AddPoint(team == TEAM_ALLIANCE ? ALLIANCE : HORDE);

    /// Update scores + flag state + count flag caputured to carrier
    UpdateTeamScore(team);
    UpdateWorldState(BG_WS_FLAG_UNKNOWN, -1); ///< From sniffs
    UpdateFlagState(team ^ 1, BG_WS_FLAG_STATE_WAIT_RESPAWN);
    UpdatePlayerScore(source, SCORE_FLAG_CAPTURES, 1);
    UpdateWorldState(BG_WS_STATE_UNKNOWN, 1); ///< From sniffs

    /// Set last team that captured flag
    _lastFlagCaptureTeam = source->GetBGTeam();

    /// Play sound + Send message to all
    PlaySoundToAll(team == TEAM_ALLIANCE ? BG_WS_SOUND_FLAG_CAPTURED_ALLIANCE : BG_WS_SOUND_FLAG_CAPTURED_HORDE);
    SendMessageToAll(team == TEAM_ALLIANCE ? LANG_BG_WS_CAPTURED_HF : LANG_BG_WS_CAPTURED_AF, team == TEAM_ALLIANCE ? CHAT_MSG_BG_SYSTEM_ALLIANCE : CHAT_MSG_BG_SYSTEM_HORDE, source);

    /// IMPORTANT: Do not remove aura before flag update! Makes the flag spawn like is dropped
    source->RemoveAurasDueToSpell(team == TEAM_ALLIANCE ? BG_WS_SPELL_HORDE_FLAG : BG_WS_SPELL_ALLIANCE_FLAG);

    /// Last but not least verify if score is max to end
    if (m_TeamScores[team] == BG_WS_MAX_TEAM_SCORE)
        EndBattleground(team);
    else
        _flagsTimer = BG_WS_FLAG_RESPAWN_TIME;
}

void BattlegroundWS::RemovePlayer(Player* player, uint64 guid, uint32 /* team */)
{
    if (!player)
        return;

    uint8 team = player->GetBGTeamId();

    if (_flagKeepers[team ^ 1] == guid)
    {
        /// First of all respawn flag and second unaura player, if is vice versa player will drop the flag and spawn it on ground.
        RespawnFlag(team ^ 1, false);
        player->RemoveAurasDueToSpell(team == TEAM_ALLIANCE? BG_WS_SPELL_HORDE_FLAG : BG_WS_SPELL_ALLIANCE_FLAG);

        if (_bothFlagsKept)
        {
            /// Remove aura from both of them
            for (uint8 teamTemp = TEAM_ALLIANCE; teamTemp <= TEAM_HORDE; ++teamTemp)
            {
                if (Player* pl = ObjectAccessor::FindPlayer(_flagKeepers[team]))
                {
                    if (_flagDebuffState && _flagDebuffState < 6)
                        pl->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                    else if (_flagDebuffState >= 6)
                        pl->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
                }
                else
                    sLog->outError(LOG_FILTER_BATTLEGROUND, "BattlegroundWS: An error has occurred in RemovePlayer: player with GUID: %u haven't been found. (_bothflagsKept is TRUE).", _flagKeepers[team]);
            }

            /// Reset both flags things
            _bothFlagsKept = false;
            _flagDebuffState = 0;
            _flagSpellForceTimer = 0;
        }
    }
}

void BattlegroundWS::RespawnFlag(uint32 team, bool captured)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (captured)
    {
        SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_F_PLACED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    else
    {
        SpawnBGObject(BG_WS_OBJECT_A_FLAG + team, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_ALLIANCE_FLAG_RESPAWNED + team, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }

    PlaySoundToAll(BG_WS_SOUND_FLAGS_RESPAWNED);

    UpdateFlagState(team, BG_WS_FLAG_STATE_ON_BASE);
}

void BattlegroundWS::UpdateFlagState(uint32 team, uint32 value, uint64 flagKeeperGUID)
{
    switch (value)
    {
        /// Values from sniffs
        case BG_WS_FLAG_STATE_WAIT_RESPAWN:
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_UNK_ALLIANCE : BG_WS_FLAG_UNK_HORDE, 0);
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_STATE_HORDE : BG_WS_FLAG_STATE_ALLIANCE, 1);
            break;
        case BG_WS_FLAG_STATE_ON_BASE:
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_UNK_ALLIANCE : BG_WS_FLAG_UNK_HORDE, 0);
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_STATE_HORDE : BG_WS_FLAG_STATE_ALLIANCE, 1);
            break;
        case BG_WS_FLAG_STATE_ON_GROUND:
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_UNK_ALLIANCE : BG_WS_FLAG_UNK_HORDE, -1);
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_STATE_HORDE : BG_WS_FLAG_STATE_ALLIANCE, BG_WS_FLAG_STATE_ON_GROUND);
            break;
        case BG_WS_FLAG_STATE_ON_PLAYER:
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_UNK_ALLIANCE : BG_WS_FLAG_UNK_HORDE, 1);
            UpdateWorldState(team == TEAM_ALLIANCE ? BG_WS_FLAG_STATE_HORDE : BG_WS_FLAG_STATE_ALLIANCE, 2);
            break;
        default:
            break;
    }

    _flagState[team] = value;
    _flagKeepers[team] = flagKeeperGUID;
}
