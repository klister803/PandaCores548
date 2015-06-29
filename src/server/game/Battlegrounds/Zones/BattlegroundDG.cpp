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

#include "Battleground.h"
#include "BattlegroundDG.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldPacket.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

BattlegroundDG::BattlegroundDG()
{
    BgObjects.resize(BG_DG_OBJECT_MAX);
    BgCreatures.resize(BG_DG_UNIT_MAX);

    m_goldUpdate = GOLD_UPDATE;

    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_DG_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_DG_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_DG_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_DG_START_HAS_BEGUN;

    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
        m_gold[i] = 0;

    for (uint8 i = 0; i < 3; ++i)
        m_points[i] = NULL;

    m_carts[TEAM_ALLIANCE] = NULL;
    m_carts[TEAM_HORDE]    = NULL;
}

BattlegroundDG::~BattlegroundDG()
{
    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        delete m_points[i];

    for (uint8 i = 0; i < 2; ++i)
        delete m_carts[i];
}

void BattlegroundDG::StartingEventCloseDoors()
{
    for (uint32 i = BG_DG_DOOR_1; i <= BG_DG_DOOR_4; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
}

void BattlegroundDG::StartingEventOpenDoors()
{
    for (uint32 i = BG_DG_DOOR_1; i <= BG_DG_DOOR_4; ++i)
        DoorOpen(i);
}

void BattlegroundDG::UpdatePlayerScore(Player *player, uint32 type, uint32 addvalue, bool addHonor)
{
    if (!player)
        return;

    BattlegroundScoreMap::iterator itr = PlayerScores.find(player->GetGUID());
    if (itr == PlayerScores.end())                         // player not found...
        return;

    switch (type)
    {
        case SCORE_CARTS_CAPTURED:
            ((BattlegroundDGScore*)itr->second)->cartsCaptured += addvalue;
            break;
        case SCORE_CARTS_DEFENDED:
            ((BattlegroundDGScore*)itr->second)->cartsDefended += addvalue;
            break;
        case SCORE_POINTS_CAPTURED:
            ((BattlegroundDGScore*)itr->second)->pointsCaptured += addvalue;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_CAPTURE_FLAG, 1);
            break;
        case SCORE_POINTS_DEFENDED:
            ((BattlegroundDGScore*)itr->second)->pointsCaptured += addvalue;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_DEFENDED_FLAG, 1);
            break;
    }
}

const WorldSafeLocsEntry *BattlegroundDG::GetClosestGraveYard(Player *player)
{
    WorldSafeLocsEntry const* pGraveyard = NULL;
    WorldSafeLocsEntry const* entry = NULL;
    float x, y;

    player->GetPosition(x, y);

    if (player->GetBGTeam() == ALLIANCE)
    {
        pGraveyard = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_ALLIANCE_BOT);
        entry = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_ALLIANCE_TOP);
    }
    else
    {
        pGraveyard = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_HORDE_TOP);
        entry = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_HORDE_BOT);
    }

    float dist = (entry->x - x)*(entry->x - x)+(entry->y - y)*(entry->y - y);
    float minDist = (pGraveyard->x - x)*(pGraveyard->x - x)+(pGraveyard->y - y)*(pGraveyard->y - y);

    if (dist < minDist)
        pGraveyard = entry;

    return pGraveyard;
}

void BattlegroundDG::AddPlayer(Player* player)
{
    //create score and add it to map, default values are set in constructor
    AddPlayerScore(player->GetGUID(), new BattlegroundDGScore);
    Battleground::AddPlayer(player);
}

void BattlegroundDG::RemovePlayer(Player* player, uint64 /*guid*/, uint32 /*team*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    if(player)
        EventPlayerDroppedFlag(player);
}

void BattlegroundDG::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    for (uint8 i = 0; i < 2; ++i)
        if (m_carts[i]->TakePlayerWhoDroppedFlag() == player->GetGUID())
        {
            UpdatePlayerScore(killer, SCORE_CARTS_DEFENDED, 1, false);
            if (killer != player)
                killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_RETURN_CART, 1);
        }

    if (player)
        EventPlayerDroppedFlag(player);

    Battleground::HandleKillPlayer(player, killer);
}

bool BattlegroundDG::HandlePlayerUnderMap(Player* player)
{
    player->TeleportTo(GetMapId(), -218.7987f, 198.7388f, 133.0327f, player->GetOrientation(), false);
    return true;
}

void BattlegroundDG::HandlePointCapturing(Player *player, Creature *creature)
{
    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        if (m_points[i]->GetCreaturePoint() == creature)
            m_points[i]->PointClicked(player);
}

void BattlegroundDG::EventPlayerUsedGO(Player *player, GameObject *go)
{
    for (uint8 i = 0; i < 2; ++i)
        if (m_carts[i]->GetGameObject() == go)
            m_carts[i]->ToggleCaptured(player);
}

void BattlegroundDG::EventPlayerDroppedFlag(Player *Source)
{
    for (uint8 i = 0; i < 2; ++i)
        if (m_carts[i]->ControlledByPlayerWithGuid() == Source->GetGUID())
            m_carts[i]->CartDropped();
}

uint64 BattlegroundDG::GetFlagPickerGUID(int32 team) const
{
    if (Player* player = m_carts[team == TEAM_ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE]->ControlledBy())
        return player->GetGUID();

    return 0;
}

void BattlegroundDG::UpdatePointsCountPerTeam()
{
    uint8 allincePoints = 0;
    uint8 hordePoints = 0;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
    {
        if (m_points[i]->GetState() == POINT_STATE_CAPTURED_ALLIANCE)
            allincePoints++;
        else if (m_points[i]->GetState() == POINT_STATE_CAPTURED_HORDE)
            hordePoints++;
    }

    UpdateWorldState(8230, allincePoints);
    UpdateWorldState(8231, hordePoints);
}

uint32 BattlegroundDG::ModGold(uint8 teamId, int32 val)
{
    m_gold[teamId] = (val < 0 && int32(int32(m_gold[teamId]) + val) < 0) ? 0 : m_gold[teamId] + val;

    UpdateWorldState(teamId == TEAM_ALLIANCE ? 7880 : 7881, m_gold[teamId]);

    return m_gold[teamId];
}

void BattlegroundDG::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        m_points[i]->Update(diff);


    if (m_goldUpdate < 0)
    {
        uint32 alliancemod = 0;
        uint32 hordemod = 0;

        uint32 modPerPoint = 16;

        for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        {
            if (m_points[i]->GetState() == POINT_STATE_CAPTURED_ALLIANCE)
                alliancemod += modPerPoint;
            else if (m_points[i]->GetState() == POINT_STATE_CAPTURED_HORDE)
                hordemod += modPerPoint;
        }

        if (alliancemod)
            ModGold(TEAM_ALLIANCE, alliancemod);

        if (hordemod)
            ModGold(TEAM_HORDE, hordemod);

        m_goldUpdate = GOLD_UPDATE;
    }
    else
        m_goldUpdate -= diff;

    // Test win condition
    if (GetCurrentGold(BG_TEAM_ALLIANCE) >= BG_DG_MAX_TEAM_SCORE)
        EndBattleground(ALLIANCE);
    if (GetCurrentGold(BG_TEAM_HORDE) >= BG_DG_MAX_TEAM_SCORE)
        EndBattleground(HORDE);
}

void BattlegroundDG::HandleAreaTrigger(Player* Source, uint32 Trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch (Trigger)
    {
        case 9013:                                          // near alliance cart
        {
            if (m_carts[BG_TEAM_ALLIANCE]->ControlledByPlayerWithGuid() == Source->GetGUID())
                m_carts[BG_TEAM_ALLIANCE]->CartDelivered();
            break;
        }
        case 9012:                                          // near horde cart
        {
            if (m_carts[BG_TEAM_HORDE]->ControlledByPlayerWithGuid() == Source->GetGUID())
                m_carts[BG_TEAM_HORDE]->CartDelivered();
            break;
        }
//        default:
//            sLog->outError(LOG_FILTER_BATTLEGROUND, "WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
//            Source->GetSession()->SendNotification("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
//            break;
    }

}

void BattlegroundDG::FillInitialWorldStates(WorldPacket &data)
{
    FillInitialWorldState(data, 7939, 1);
    FillInitialWorldState(data, 7938, 1);
    FillInitialWorldState(data, 7935, 1);

    FillInitialWorldState(data, 7904, (m_carts[TEAM_HORDE] && m_carts[TEAM_HORDE]->ControlledByPlayerWithGuid() ? 2 : 1));
    FillInitialWorldState(data, 7887, (m_carts[TEAM_ALLIANCE] && m_carts[TEAM_ALLIANCE]->ControlledByPlayerWithGuid() ? 2 : 1));

    FillInitialWorldState(data, 7880, m_gold[TEAM_ALLIANCE]);
    FillInitialWorldState(data, 7881, m_gold[TEAM_HORDE]);

    if (m_points[0])
        UpdatePointsCountPerTeam();
}

void BattlegroundDG::Reset()
{
    //call parent's class reset
    Battleground::Reset();
}

bool BattlegroundDG::SetupBattleground()
{
    m_points[0] = new BotPoint(this);
    m_points[1] = new MiddlePoint(this);
    m_points[2] = new TopPoint(this);

    //gates
    if (!AddObject(BG_DG_DOOR_1, BG_DG_ENTRY_DOOR_1, -263.455f, 218.163f, 132.43f, 4.72984f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)
            || !AddObject(BG_DG_DOOR_2, BG_DG_ENTRY_DOOR_2, -213.712f, 201.043f, 132.488f, 3.9619f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)
            || !AddObject(BG_DG_DOOR_3, BG_DG_ENTRY_DOOR_3, -69.8785f, 781.837f, 132.43f, 1.58825f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)
            || !AddObject(BG_DG_DOOR_4, BG_DG_ENTRY_DOOR_4, -119.621f, 798.957f, 132.488f, 0.820303f, 0, 0, 1.43143f, 4.48416f, RESPAWN_IMMEDIATELY)

            // carts
            || !AddObject(BG_DG_CART_ALLIANCE, BG_DG_ENTRY_CART_ALLIANCE, -241.741f, 208.611f, 133.747f, 0.84278f, 0, 0, 1.93617f, 4.48416f, RESPAWN_IMMEDIATELY)
            || !AddObject(BG_DG_CART_HORDE, BG_DG_ENTRY_CART_HORDE, -91.6163f, 791.361f, 133.747f, 4.02356f, 0, 0, 1.93617f, 4.48416f, RESPAWN_IMMEDIATELY)

            )
        return false;

    m_carts[0] = new Cart(this);
    m_carts[1] = new Cart(this);

    m_carts[0]->SetTeamId(BG_TEAM_ALLIANCE);
    m_carts[1]->SetTeamId(BG_TEAM_HORDE);

    m_carts[0]->SetGameObject(GetBgMap()->GetGameObject(BgObjects[BG_DG_CART_ALLIANCE]), BG_DG_CART_ALLIANCE);
    m_carts[1]->SetGameObject(GetBgMap()->GetGameObject(BgObjects[BG_DG_CART_HORDE]), BG_DG_CART_HORDE);

    if (!AddCreature(BG_DG_ENTRY_FLAG, BG_DG_UNIT_FLAG_MID, TEAM_NEUTRAL, -167.5035f, 499.059f, 92.62903f, 1.249549f) ||
            !AddCreature(BG_DG_ENTRY_FLAG, BG_DG_UNIT_FLAG_TOP, TEAM_NEUTRAL, 68.39236f, 431.1771f, 111.4928f, 1.229f) ||
            !AddCreature(BG_DG_ENTRY_FLAG, BG_DG_UNIT_FLAG_BOT, TEAM_NEUTRAL, -397.7726f, 574.368f, 111.0529f, 1.437866f))
        return false;

    for (uint8 i = BG_DG_UNIT_FLAG_BOT; i <= BG_DG_UNIT_FLAG_TOP; ++i)
        if (Creature* flag = GetBgMap()->GetCreature(BgCreatures[i]))
        {
            flag->AddUnitState(UNIT_STATE_CANNOT_TURN);
            flag->AddUnitState(UNIT_STATE_NOT_MOVE);
            flag->SetInt32Value(UNIT_FIELD_INTERACT_SPELL_ID, BG_DG_CAPTURE_SPELL);
            m_points[i]->SetCreaturePoint(flag);
            m_points[i]->UpdateState(POINT_STATE_NEUTRAL);
        }

    WorldSafeLocsEntry const* sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_ALLIANCE_BOT);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_ALLIANCE_BOT, sg->x, sg->y, sg->z, 3.124139f, ALLIANCE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundDG: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_HORDE_BOT);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_HORDE_BOT, sg->x, sg->y, sg->z, 3.193953f, HORDE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundDG: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_ALLIANCE_TOP);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_ALLIANCE_TOP, sg->x, sg->y, sg->z, 3.124139f, ALLIANCE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundDG: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(BG_DG_LOC_SPIRIT_HORDE_TOP);
    if (!sg || !AddSpiritGuide(BG_DG_SPIRIT_HORDE_TOP, sg->x, sg->y, sg->z, 3.193953f, HORDE))
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundDG: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    return true;
}

BattlegroundDG::Point::Point(BattlegroundDG *bg) : m_bg(bg)
{
    m_state = 0;
    m_timer = 0;

    m_goldCredit = 0;

    m_currentWorldState = std::make_pair(0, 0);

    m_point = nullptr;
}

BattlegroundDG::Point::~Point()
{
}

void BattlegroundDG::Point::UpdateState(PointStates state)
{
    if (m_prevAura)
    {
        m_point->RemoveAura(m_prevAura);
        m_prevAura = 0;
    }

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
        {
            m_point->AddAura(BG_DG_AURA_ALLIANCE_CONTEST, m_point);
            m_prevAura = BG_DG_AURA_ALLIANCE_CONTEST;
            m_timer = CAPTURE_TIME;

            GetBg()->PlaySoundToAll(m_state == POINT_STATE_NEUTRAL ? BG_DG_SOUND_NODE_CLAIMED : BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE);

            break;
        }
        case POINT_STATE_CONTESTED_HORDE:
        {
            m_point->AddAura(BG_DG_AURA_HORDE_CONTEST, m_point);
            m_prevAura = BG_DG_AURA_HORDE_CONTEST;
            m_timer = CAPTURE_TIME;

            GetBg()->PlaySoundToAll(m_state == POINT_STATE_NEUTRAL ? BG_DG_SOUND_NODE_CLAIMED : BG_DG_SOUND_NODE_ASSAULTED_HORDE);

            break;
        }
        case POINT_STATE_CAPTURED_ALLIANCE:
        {
            m_point->AddAura(BG_DG_AURA_ALLIANCE_CATURED, m_point);
            m_prevAura = BG_DG_AURA_ALLIANCE_CATURED;

            GetBg()->PlaySoundToAll(BG_DG_SOUND_NODE_CAPTURED_ALLIANCE);

            break;
        }
        case POINT_STATE_CAPTURED_HORDE:
        {
            m_point->AddAura(BG_DG_AURA_HORDE_CAPTURED, m_point);
            m_prevAura = BG_DG_AURA_HORDE_CAPTURED;

            GetBg()->PlaySoundToAll(BG_DG_SOUND_NODE_CAPTURED_HORDE);

            break;
        }
        case POINT_STATE_NEUTRAL:
        {
            m_point->AddAura(BG_DG_AURA_NEUTRAL, m_point);
            m_prevAura = BG_DG_AURA_NEUTRAL;
            break;
        }
    }

    m_state = state;


    GetBg()->UpdatePointsCountPerTeam();
}

void BattlegroundDG::Point::PointClicked(Player *player)
{
    uint32 newState = (player->GetBGTeamId() == TEAM_ALLIANCE) ? POINT_STATE_CONTESTED_ALLIANCE : POINT_STATE_CONTESTED_HORDE;
    if (newState == m_state || newState + 2 == m_state)
        return;

    if (m_state == POINT_STATE_NEUTRAL || m_state == POINT_STATE_CAPTURED_ALLIANCE || m_state == POINT_STATE_CAPTURED_HORDE)
        GetBg()->UpdatePlayerScore(player, SCORE_POINTS_CAPTURED, 1, false);
    else
        GetBg()->UpdatePlayerScore(player, SCORE_POINTS_DEFENDED, 1, false);

    UpdateState((PointStates)newState);
}

void BattlegroundDG::Point::Update(uint32 diff)
{
    if (m_state == POINT_STATE_CONTESTED_ALLIANCE || m_state == POINT_STATE_CONTESTED_HORDE)
    {
        if (m_timer < 0)
        {
            m_timer = 0;
            UpdateState(PointStates(m_state + 2));
        }
        else
            m_timer -= diff;
    }
}

void BattlegroundDG::TopPoint::UpdateState(PointStates state)
{
    uint32 oldstate = m_state;
    Point::UpdateState(state);

    if (m_currentWorldState.first)
        GetBg()->UpdateWorldState(m_currentWorldState.first, !m_currentWorldState.second);

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
        {
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(7935, 0);

            GetBg()->UpdateWorldState(7857, 1);
            m_currentWorldState = std::make_pair(7857, 1);

            break;
        }
        case POINT_STATE_CONTESTED_HORDE:
        {
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(7935, 0);

            GetBg()->UpdateWorldState(7861, 1);
            m_currentWorldState = std::make_pair(7861, 1);

            break;
        }
        case POINT_STATE_CAPTURED_ALLIANCE:
        {
            GetBg()->UpdateWorldState(7855, 2);
            m_currentWorldState = std::make_pair(7855, 2);

            break;
        }
        case POINT_STATE_CAPTURED_HORDE:
        {
            GetBg()->UpdateWorldState(7855, 1);
            m_currentWorldState = std::make_pair(7855, 1);

            break;
        }
    }
}

void BattlegroundDG::BotPoint::UpdateState(PointStates state)
{
    uint32 oldstate = m_state;
    Point::UpdateState(state);

    if (m_currentWorldState.first)
        GetBg()->UpdateWorldState(m_currentWorldState.first, !m_currentWorldState.second);

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
        {
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(7938, 0);

            GetBg()->UpdateWorldState(7864, 1);
            m_currentWorldState = std::make_pair(7864, 1);

            break;
        }
        case POINT_STATE_CONTESTED_HORDE:
        {
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(7938, 0);

            GetBg()->UpdateWorldState(7865, 1);
            m_currentWorldState = std::make_pair(7865, 1);

            break;
        }
        case POINT_STATE_CAPTURED_ALLIANCE:
        {
            GetBg()->UpdateWorldState(7856, 2);
            m_currentWorldState = std::make_pair(7856, 2);

            break;
        }
        case POINT_STATE_CAPTURED_HORDE:
        {
            GetBg()->UpdateWorldState(7856, 1);
            m_currentWorldState = std::make_pair(7856, 1);

            break;
        }
    }
}

void BattlegroundDG::MiddlePoint::UpdateState(PointStates state)
{
    uint32 oldstate = m_state;
    Point::UpdateState(state);

    if (m_currentWorldState.first)
        GetBg()->UpdateWorldState(m_currentWorldState.first, !m_currentWorldState.second);

    switch (state)
    {
        case POINT_STATE_CONTESTED_ALLIANCE:
        {
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(7939, 0);

            GetBg()->UpdateWorldState(7934, 1);
            m_currentWorldState = std::make_pair(7934, 1);

            break;
        }
        case POINT_STATE_CONTESTED_HORDE:
        {
            if (oldstate == POINT_STATE_NEUTRAL)
                GetBg()->UpdateWorldState(7939, 0);

            GetBg()->UpdateWorldState(7936, 1);
            m_currentWorldState = std::make_pair(7936, 1);

            break;
        }
        case POINT_STATE_CAPTURED_ALLIANCE:
        {
            GetBg()->UpdateWorldState(7932, 2);
            m_currentWorldState = std::make_pair(7932, 2);

            break;
        }
        case POINT_STATE_CAPTURED_HORDE:
        {
            GetBg()->UpdateWorldState(7932, 1);
            m_currentWorldState = std::make_pair(7932, 1);

            break;
        }
    }
}

BattlegroundDG::Cart::Cart(BattlegroundDG *bg) : m_bg(bg)
{
    m_controlledBy = 0;
    m_team = 0;
    m_goCart = 0;
    m_playerDroppedCart = 0;
    m_stolenGold = 0;
}

void BattlegroundDG::Cart::ToggleCaptured(Player *player)
{
    if (m_controlledBy)
        return;

    uint32 summonSpellId;
    uint32 cartEntry;
    uint32 flagStateField;
    uint32 cartAuraId;

    if (player->GetBGTeamId() == TEAM_ALLIANCE)
    {
        summonSpellId = BG_DG_SPELL_SPAWN_HORDE_CART;
        cartEntry = 71073;
        flagStateField = 7904;
        cartAuraId = BG_DG_AURA_CART_HORDE;
        GetBg()->PlaySoundToAll(BG_DG_SOUND_NODE_ASSAULTED_ALLIANCE);
    }
    else
    {
        summonSpellId = BG_DG_SPELL_SPAWN_ALLIANCE_CART;
        cartEntry = 71071;
        flagStateField = 7887;
        cartAuraId = BG_DG_AURA_CART_ALLIANCE;

        GetBg()->PlaySoundToAll(BG_DG_SOUND_NODE_ASSAULTED_HORDE);
    }

    player->CastSpell(player, summonSpellId);

    CellCoord p(Trinity::ComputeCellCoord(player->GetPositionX(), player->GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    Creature* cart = NULL;
    Trinity::AllCreaturesOfEntryInRange check(player, cartEntry, SIZE_OF_GRIDS);
    Trinity::CreatureSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(player, cart, check);
    TypeContainerVisitor<Trinity::CreatureSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

    cell.Visit(p, visitor, *(player->GetMap()), *player, SIZE_OF_GRIDS);

    if (cart)
    {
        cart->GetMotionMaster()->MoveFollow(player, 2.f, 0.f);
        cart->CastSpell(player, BG_DG_AURA_CARTS_CHAINS);
        cart->AddAura(cartAuraId, cart);
        // WTF? It's already set creature type, or u need set 0x10000? ->SetUInt16Value(OBJECT_FIELD_TYPE, 1, 1);
        // This nothing change before!
        //cart->SetUInt16Value(OBJECT_FIELD_TYPE, 0, 65545);
        cart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
        cart->setFaction(35);
        cart->SetSpeed(MOVE_RUN, 3.f);

//        PlaySoundToAll(8174);

        GetBg()->UpdateWorldState(flagStateField, 2);

        m_controlledBy = player->GetGUID();

        GetBg()->SendFlagsPositionsUpdate(FLAGS_UPDATE);

        uint32 goldBeffore = GetBg()->GetCurrentGold(TeamId());
        int32 takeGold = -200;
        GetBg()->ModGold(TeamId(), takeGold);
        m_stolenGold = goldBeffore - GetBg()->GetCurrentGold(TeamId());
    }
}

void BattlegroundDG::Cart::CartDropped()
{
    m_playerDroppedCart = m_controlledBy;
    UnbindCartFromPlayer();
    GetBg()->ModGold(TeamId(), m_stolenGold);
    m_stolenGold = 0;
}

Player *BattlegroundDG::Cart::ControlledBy()
{
    return ObjectAccessor::FindPlayer(m_controlledBy);
}

void BattlegroundDG::Cart::CartDelivered()
{
    Player* player = ControlledBy();
    GetBg()->UpdatePlayerScore(player, SCORE_CARTS_CAPTURED, 1, false);
    GetBg()->ModGold(player->GetBGTeamId(), m_stolenGold);
    m_stolenGold = 0;
    UnbindCartFromPlayer();
    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, DG_OBJECTIVE_CAPTURE_CART, 1);

    GetBg()->PlaySoundToAll(player->GetBGTeamId() == TEAM_ALLIANCE ? BG_DG_SOUND_NODE_CAPTURED_ALLIANCE : BG_DG_SOUND_NODE_CAPTURED_HORDE);
}

void BattlegroundDG::Cart::UnbindCartFromPlayer()
{
    Player* player = NULL;
    Unit* cart = NULL;
    if (player = ControlledBy())
        if (Aura* aura = player->GetAura(BG_DG_AURA_CARTS_CHAINS))
            if (Unit* unit = aura->GetCaster())
                cart = unit;

    if (cart && player)
    {
        player->RemoveAura(BG_DG_AURA_PLAYER_FLAG_HORDE);
        player->RemoveAura(BG_DG_AURA_PLAYER_FLAG_ALLIANCE);

        cart->ToCreature()->DespawnOrUnsummon();

        GetBg()->SpawnBGObject(m_goBgId, 0);

        m_controlledBy = 0;

        uint32 statefield = (player->GetBGTeamId() == TEAM_ALLIANCE) ? 7904 : 7887;
        GetBg()->UpdateWorldState(statefield, 1);

        GetBg()->SendFlagsPositionsUpdate(FLAGS_UPDATE);
    }
}
