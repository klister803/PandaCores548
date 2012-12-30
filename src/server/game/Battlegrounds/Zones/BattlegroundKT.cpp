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

#include "Object.h"
#include "Player.h"
#include "Battleground.h"
#include "BattlegroundKT.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "WorldPacket.h"
#include "Language.h"
#include "MapManager.h"
#include "SpellAuraEffects.h"

BattlegroundKT::BattlegroundKT()
{
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_KT_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_KT_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_KT_HAS_BEGUN;
}

BattlegroundKT::~BattlegroundKT()
{
}

void BattlegroundKT::Update(uint32 diff)
{
    Battleground::Update(diff);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (m_EndTimer <= diff)
        {
            uint32 allianceScore = GetTeamScore(ALLIANCE);
            uint32 hordeScore    = GetTeamScore(HORDE);

            if (allianceScore > hordeScore)
                EndBattleGround(ALLIANCE);
            else if (allianceScore < hordeScore)
                EndBattleGround(HORDE);
            else
            {
                // if 0 => tie
                EndBattleGround(m_LastCapturedOrbTeam);
            }
        }
        else
        {
            uint32 minutesLeftPrev = GetRemainingTimeInMinutes();
            m_EndTimer -= diff;
            uint32 minutesLeft = GetRemainingTimeInMinutes();

            if (minutesLeft != minutesLeftPrev)
                UpdateWorldState(BG_KT_TIME_REMAINING, minutesLeft);
        }
    }
}

void BattlegroundKT::StartingEventCloseDoors()
{
}

void BattlegroundKT::StartingEventOpenDoors()
{
    //OpenDoorEvent(BG_EVENT_DOOR);

    // TODO implement timer to despawn doors after a short while

    //SpawnEvent(KT_EVENT_SPIRITGUIDES_SPAWN, 0, true);
    //SpawnEvent(KT_EVENT_ORB, 0, true);

    // Players that join battleground after start are not eligible to get achievement.
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, BG_KT_EVENT_START_BATTLE);
}

void BattlegroundKT::AddPlayer(PlayerPtr plr)
{
    Battleground::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattleGroundKTScore* sc = new BattleGroundKTScore;

    PlayerScores[plr->GetObjectGuid()] = sc;
}

void BattlegroundKT::EventPlayerDroppedOrb(PlayerPtr Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        if (!IsAllianceOrbPickedup())
			return;

		if (Source->GetTeam() == ALLIANCE)
        {
		    Source->RemoveAurasDueToSpell(BG_KT_SPELL_ORB);
		    Source->RemoveAurasDueToSpell(BG_KT_SPELL_ORB1);
		    Source->RemoveAurasDueToSpell(BG_KT_SPELL_ORB2);
		    m_OrbState[BG_TEAM_ALLIANCE] = BG_KT_ORB_STATE_ON_GROUND;
		    Source->CastSpell(Source, BG_KT_SPELL_ORB_DROPPED, true);
		    UpdateOrbState(Team(Source->GetTeam()), 1);
		    SendMessageToAll(LANG_BG_KT_DROPPED, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
		    UpdateWorldState(BG_KT_ICON_A, uint32(-1));            
	    }
    }

    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        if (!IsHordeOrbPickedup())
			return;

		if (Source->GetTeam() == HORDE)
        {
		    Source->RemoveAurasDueToSpell(BG_KT_SPELL_ORB);
		    Source->RemoveAurasDueToSpell(BG_KT_SPELL_ORB1);
		    Source->RemoveAurasDueToSpell(BG_KT_SPELL_ORB2);
		    m_OrbState[BG_TEAM_HORDE] = BG_KT_ORB_STATE_ON_GROUND;
		    Source->CastSpell(Source, BG_KT_SPELL_ORB_DROPPED, true);
		    UpdateOrbState(Team(Source->GetTeam()), 1);
		    SendMessageToAll(LANG_BG_KT_DROPPED, CHAT_MSG_BG_SYSTEM_HORDE, Source);
		    UpdateWorldState(BG_KT_ICON_H, uint32(-1));            
	    }
    }

}

void BattlegroundKT::EventPlayerClickedOnOrb(PlayerPtr Source, GameObjectPtr target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    int32 message_id = 0;
    ChatMsg type;

    //uint8 event = (sBattlegroundMgr->GetGameObjectEventIndex(target_obj->GetGUIDLow())).event1;

    //Orb on base
    if (Source->IsWithinDistInMap(target_obj, 10))
    {
        if (Source->GetTeam() == HORDE)
        {
            message_id = LANG_BG_KT_PICKEDUP;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            PlaySoundToAll(BG_KT_SOUND_A_ORB_PICKED_UP);
            //SpawnEvent(KT_EVENT_ORB, 0, false);
            SetAllianceOrbPicker(Source->GetObjectGuid());
            Source->CastSpell(Source, BG_KT_SPELL_ORB, true);
            m_OrbState[BG_TEAM_ALLIANCE] = BG_KT_ORB_STATE_ON_PLAYER;
            UpdateOrbState(HORDE, BG_KT_ORB_STATE_ON_PLAYER);
            UpdateWorldState(BG_KT_ICON_A, 1);
        }
        else
        {
            message_id = LANG_BG_KT_PICKEDUP;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            PlaySoundToAll(BG_KT_SOUND_H_ORB_PICKED_UP);
            //SpawnEvent(KT_EVENT_ORB, 0, false);
            SetHordeOrbPicker(Source->GetObjectGuid());
            Source->CastSpell(Source, BG_KT_SPELL_ORB, true);
            m_OrbState[BG_TEAM_HORDE] = BG_KT_ORB_STATE_ON_PLAYER;
            UpdateOrbState(ALLIANCE, BG_KT_ORB_STATE_ON_PLAYER);
            UpdateWorldState(BG_KT_ICON_H, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }

    if (!message_id)
        return;

    SendMessageToAll(message_id, type, Source);
    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundKT::RemovePlayer(PlayerPtr plr, ObjectGuid guid)
{
    // sometimes flag aura not removed :(
    if (IsAllianceOrbPickedup() && m_OrbKeepers[BG_TEAM_ALLIANCE] == guid)
    {
            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattleGroundKT: Removing offline player who has the FLAG!!");
            ClearAllianceOrbPicker();
            EventPlayerDroppedOrb(plr);
    }
    if (IsHordeOrbPickedup() && m_OrbKeepers[BG_TEAM_HORDE] == guid)
    {
            sLog->outError(LOG_FILTER_BATTLEGROUND, "BattleGroundKT: Removing offline player who has the FLAG!!");
            ClearHordeOrbPicker();
            EventPlayerDroppedOrb(plr);
    }
}

void BattlegroundKT::UpdateOrbState(Team team, uint32 value)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_KT_ICON_A, value);
    else
        UpdateWorldState(BG_KT_ICON_H, value);
}

void BattlegroundKT::UpdateTeamScore(Team team)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_KT_ORB_POINTS_A, GetTeamScore(team));
    else
        UpdateWorldState(BG_KT_ORB_POINTS_H, GetTeamScore(team));
}

void BattlegroundKT::HandleAreaTrigger(PlayerPtr Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch(Trigger)
    {
        case 6060:                                          // ZONE1
		    if (Source->GetTeam() == HORDE && !IsAllianceOrbPickedup())
            {
			    SetHordeOrbPicker(Source->GetObjectGuid());
                Source->CastSpell(Source, BG_KT_SPELL_ORB, true);
		    }
		    if (Source->GetTeam() == ALLIANCE && !IsHordeOrbPickedup())
            {
			    SetAllianceOrbPicker(Source->GetObjectGuid());
                Source->CastSpell(Source, BG_KT_SPELL_ORB, true);
		    }
			break;
		case 6061:                                          // ZONE2
		    if (Source->GetTeam() == HORDE && !IsAllianceOrbPickedup())
            {
			    SetHordeOrbPicker(Source->GetObjectGuid());
                Source->CastSpell(Source, BG_KT_SPELL_ORB1, true);
		    }
		    if (Source->GetTeam() == ALLIANCE && !IsHordeOrbPickedup())
            {
			    SetAllianceOrbPicker(Source->GetObjectGuid());
                Source->CastSpell(Source, BG_KT_SPELL_ORB1, true);
		    }
			break;
		case 6136:                                          // ZONE3
		    if (Source->GetTeam() == HORDE && !IsAllianceOrbPickedup())
            {
			    SetHordeOrbPicker(Source->GetObjectGuid());
                Source->CastSpell(Source, BG_KT_SPELL_ORB2, true);
		    }
		    if (Source->GetTeam() == ALLIANCE && !IsHordeOrbPickedup())
            {
			    SetAllianceOrbPicker(Source->GetObjectGuid());
                Source->CastSpell(Source, BG_KT_SPELL_ORB2, true);
		    }
			break;
        default:
            sLog->outError(LOG_FILTER_BATTLEGROUND, "WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

bool BattlegroundKT::SetupBattleGround()
{
    return true;
}

void BattlegroundKT::Reset()
{
    //call parent's class reset
    Battleground::Reset();

    // spiritguides and flags not spawned at beginning
    //ActiveEvents[KT_EVENT_SPIRITGUIDES_SPAWN] = BG_EVENT_NONE;
    //ActiveEvents[KT_EVENT_ORB] = BG_EVENT_NONE;

    for(uint32 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        m_DroppedOrbGuid[i].Clear();
        m_OrbKeepers[i].Clear();
        m_OrbState[i]       = BG_KT_ORB_STATE_ON_BASE;
        m_TeamScores[i]      = 0;
    }
    bool isBGWeekend = BattlegroundMgr::IsBGWeekend(GetTypeID());
    m_ReputationCapture = (isBGWeekend) ? 45 : 35;
    m_HonorWinKills = (isBGWeekend) ? 3 : 1;
    m_HonorEndKills = (isBGWeekend) ? 4 : 2;

    m_EndTimer = BG_KT_TIME_LIMIT;
    m_LastCapturedOrbTeam = TEAM_NONE;
}

void BattlegroundKT::EndBattleGround(Team winner)
{
    //win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), HORDE);
    //complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), HORDE);

    Battleground::EndBattleground(winner);
}

void BattlegroundKT::HandleKillPlayer(PlayerPtr player, PlayerPtr killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedOrb(player);

    Battleground::HandleKillPlayer(player, killer);
}


void BattlegroundKT::UpdatePlayerScore(PlayerPtr Source, uint32 type, uint32 value)
{

    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetObjectGuid());
    if(itr == PlayerScores.end())                         // player not found
        return;

    switch(type)
    {
        case SCORE_ORB_HANDLES:                           // orb handles
            ((BattleGroundKTScore*)itr->second)->OrbHandles += value;
            break;
        default:
            Battleground::UpdatePlayerScore(Source, type, value);
            break;
    }
}

WorldSafeLocsEntry const* BattlegroundKT::GetClosestGraveYard(PlayerPtr player)
{
    //if status in progress, it returns main graveyards with spiritguides
    //else it will return the graveyard in the flagroom - this is especially good
    //if a player dies in preparation phase - then the player can't cheat
    //and teleport to the graveyard outside the flagroom
    //and start running around, while the doors are still closed
    if (player->GetTeam() == ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(KT_GRAVEYARD_RECTANGLEA1);
        else
            return sWorldSafeLocsStore.LookupEntry(KT_GRAVEYARD_RECTANGLEA2);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(KT_GRAVEYARD_RECTANGLEH1);
        else
            return sWorldSafeLocsStore.LookupEntry(KT_GRAVEYARD_RECTANGLEH2);
    }
}

void BattlegroundKT::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, BG_KT_ORB_POINTS_A, GetTeamScore(ALLIANCE));
    FillInitialWorldState(data, count, BG_KT_ORB_POINTS_H, GetTeamScore(HORDE));

    if (m_OrbState[BG_TEAM_ALLIANCE] == BG_KT_ORB_STATE_ON_GROUND)
        FillInitialWorldState(data, count, BG_KT_ICON_A, -1);
    else if (m_OrbState[BG_TEAM_ALLIANCE] == BG_KT_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_KT_ICON_A, 1);
    else
        FillInitialWorldState(data, count, BG_KT_ICON_A, 0);

    if (m_OrbState[BG_TEAM_HORDE] == BG_KT_ORB_STATE_ON_GROUND)
        FillInitialWorldState(data, count, BG_KT_ICON_H, -1);
    else if (m_OrbState[BG_TEAM_HORDE] == BG_KT_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_KT_ICON_H, 1);
    else
        FillInitialWorldState(data, count, BG_KT_ICON_H, 0);

    FillInitialWorldState(data, count, BG_KT_ORB_POINTS_MAX, BG_KT_MAX_TEAM_SCORE);

    if (m_OrbState[BG_TEAM_HORDE] == BG_KT_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_KT_ORB_STATE, 2);
    else
        FillInitialWorldState(data, count, BG_KT_ORB_STATE, 1);

    if (m_OrbState[BG_TEAM_ALLIANCE] == BG_KT_ORB_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_KT_ORB_STATE, 2);
    else
        FillInitialWorldState(data, count, BG_KT_ORB_STATE, 1);

    FillInitialWorldState(data, count, BG_KT_TIME_ENABLED, 1);
    FillInitialWorldState(data, count, BG_KT_TIME_REMAINING, GetRemainingTimeInMinutes());
}
