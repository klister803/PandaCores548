/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateData.h"
#include "Item.h"
#include "Map.h"
#include "Transport.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "SpellInfo.h"

namespace {

bool shouldCallMoveInLineOfSight(Creature const *c, Unit const *u)
{
    return c != u
            && c->IsAIEnabled
            && c->isAlive()
            && u->isAlive()
            && !u->isInFlight()
            && !c->HasUnitState(UNIT_STATE_SIGHTLESS)
            && (c->HasReactState(REACT_AGGRESSIVE) || c->AI()->CanSeeEvenInPassiveMode())
            && c->canSeeOrDetect(u, false, true);
}

} // namespace

using namespace Trinity;

void VisibleNotifier::SendToSelf()
{
    // at this moment i_clientGUIDs have guids that not iterate at grid level checks
    // but exist one case when this possible and object not out of range: transports
    if (Transport* transport = i_player.GetTransport())
        for (Transport::PlayerSet::const_iterator itr = transport->GetPassengers().begin();itr != transport->GetPassengers().end();++itr)
        {
            if (vis_guids.find((*itr)->GetGUID()) != vis_guids.end())
            {
                vis_guids.erase((*itr)->GetGUID());

                i_player.UpdateVisibilityOf((*itr), i_data, i_visibleNow);
                (*itr)->UpdateVisibilityOf(&i_player);
            }
        }

    for (auto it = vis_guids.begin(); it != vis_guids.end(); ++it)
    {
        // extralook shouldn't be removed by missing creature in grid where is curently player
        // if (i_player.HaveExtraLook(*it))
            // continue;

        i_player.m_clientGUIDs.erase(*it);
        i_data.AddOutOfRangeGUID(*it);

        if (IS_PLAYER_GUID(*it))
        {
            Player* player = ObjectAccessor::FindPlayer(*it);
            if (player && player->IsInWorld()/* && !player->onVisibleUpdate()*/)
                player->UpdateVisibilityOf(&i_player);
        }
    }

    if (!i_data.HasData())
        return;

    WorldPacket packet;
    if (i_data.BuildPacket(&packet))
    {
        i_player.GetSession()->SendPacket(&packet);
        i_player.SendVignette();
    }

    for (std::set<Unit*>::const_iterator it = i_visibleNow.begin(); it != i_visibleNow.end(); ++it)
        i_player.SendInitialVisiblePackets(*it);
}

void VisibleChangesNotifier::Visit(PlayerMapType &m)
{
    for (auto &source : m)
    {
        if (source == &i_object)
            continue;

        source->UpdateVisibilityOf(&i_object);

        if (source->GetSharedVisionList().empty())
            continue;

        for (auto &player : source->GetSharedVisionList())
            if (player->m_seer == source)
                player->UpdateVisibilityOf(&i_object);
    }
}

void VisibleChangesNotifier::Visit(CreatureMapType &m)
{
    for (auto &source : m)
    {
        if (source->GetSharedVisionList().empty())
            continue;

        for (auto &player : source->GetSharedVisionList())
            if (player->m_seer == source)
                player->UpdateVisibilityOf(&i_object);
    }
}

void VisibleChangesNotifier::Visit(DynamicObjectMapType &m)
{
    for (auto &obj : m)
    {
        auto const guid = obj->GetCasterGUID();
        if (IS_PLAYER_GUID(guid))
            continue;

        auto const caster = (Player*)obj->GetCaster();
        if (caster && caster->m_seer == obj)
            caster->UpdateVisibilityOf(&i_object);
    }
}

void VisibleChangesNotifier::Visit(AreaTriggerMapType &m)
{
    for (auto &obj : m)
    {
        auto const guid = obj->GetCasterGUID();
        if (IS_PLAYER_GUID(guid))
            continue;

        auto const caster = (Player*)obj->GetCaster();
        if (caster && caster->m_seer == obj)
            caster->UpdateVisibilityOf(&i_object);
    }
}

void AIRelocationNotifier::Visit(CreatureMapType &m)
{
    if (unit_->GetTypeId() == TYPEID_UNIT)
    {
        for (auto &creature : m)
        {
            if (shouldCallMoveInLineOfSight(creature, unit_))
            {
                auto const both = shouldCallMoveInLineOfSight(static_cast<Creature*>(unit_), creature);
                movedInLos_.emplace_back(creature, both);
            }
        }
    }
    else
    {
        for (auto &creature : m)
            if (shouldCallMoveInLineOfSight(creature, unit_))
                movedInLos_.emplace_back(creature, false);
    }

    if (movedInLos_.empty())
        return;

    for (auto &pair : movedInLos_)
    {
        pair.first->AI()->MoveInLineOfSight_Safe(unit_);
        if (pair.second)
            static_cast<Creature*>(unit_)->AI()->MoveInLineOfSight_Safe(pair.first);
    }

    movedInLos_.clear();
}

void MessageDistDeliverer::Visit(PlayerMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        // Send packet to all who are sharing the player's vision
        if (!target->GetSharedVisionList().empty())
        {
            for (auto &player : target->GetSharedVisionList())
                if (player->m_seer == target)
                    SendPacket(player);
        }

        if (target->m_seer == target || target->GetVehicle())
            SendPacket(target);
    }
}

void MessageDistDeliverer::Visit(CreatureMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        // Send packet to all who are sharing the creature's vision
        if (!target->GetSharedVisionList().empty())
        {
            for (auto &player : target->GetSharedVisionList())
                if (player->m_seer == target)
                    SendPacket(player);
        }
    }
}

void MessageDistDeliverer::Visit(DynamicObjectMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        if (IS_PLAYER_GUID(target->GetCasterGUID()))
        {
            // Send packet back to the caster if the caster has vision of dynamic object
            auto const caster = (Player*)target->GetCaster();
            if (caster && caster->m_seer == target)
                SendPacket(caster);
        }
    }
}

void ChatMessageDistDeliverer::Visit(PlayerMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        // Send packet to all who are sharing the player's vision
        if (!target->GetSharedVisionList().empty())
        {
            for (auto &player : target->GetSharedVisionList())
                if (player->m_seer == target)
                    SendPacket(player);
        }

        if (target->m_seer == target || target->GetVehicle())
            SendPacket(target);
    }
}

void ChatMessageDistDeliverer::Visit(CreatureMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        // Send packet to all who are sharing the creature's vision
        if (!target->GetSharedVisionList().empty())
        {
            for (auto &player : target->GetSharedVisionList())
                if (player->m_seer == target)
                    SendPacket(player);
        }
    }
}

void ChatMessageDistDeliverer::Visit(DynamicObjectMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        if (IS_PLAYER_GUID(target->GetCasterGUID()))
        {
            // Send packet back to the caster if the caster has vision of dynamic object
            auto const caster = (Player*)target->GetCaster();
            if (caster && caster->m_seer == target)
                SendPacket(caster);
        }
    }
}

void ChatMessageDistDeliverer::SendPacket(Player* player)
{
    // never send packet to self
    if (player == i_source || (team && player->GetTeam() != team) || skipped_receiver == player)
        return;

    if (!player->HaveAtClient(i_source))
        return;

    if (WorldSession* session = player->GetSession())
    {
        LanguageDesc const* langDesc = GetLanguageDescByID(i_c.language);
        bool needCoded = false;
        bool needEmpty = false;
        if (i_c.language != LANG_ADDON && (i_c.chatType == CHAT_MSG_SAY || i_c.chatType == CHAT_MSG_YELL))
        {
            needCoded = !player->CanSpeakLanguage(i_c.language) && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT);

            if (i_source->GetTypeId() == TYPEID_PLAYER)
                if (Battleground* bg = player->GetBattleground())
                    needEmpty = bg->GetPlayerTeam(i_source->GetGUID()) != bg->GetPlayerTeam(player->GetGUID());
        }

        if (needEmpty)
        {
            if (!i_emptyMessage)
            {
                i_emptyMessage = new WorldPacket();
                Trinity::BuildChatPacket(*i_emptyMessage, i_c, needCoded, needEmpty);
            }

            session->SendPacket(i_emptyMessage);
        }
        else if (needCoded)
        {
            if (!i_codedMessage)
            {
                i_codedMessage = new WorldPacket();
                Trinity::BuildChatPacket(*i_codedMessage, i_c, needCoded, needEmpty);
            }

            session->SendPacket(i_codedMessage);
        }
        else
        {
            if (!i_normalMessage)
            {
                i_normalMessage = new WorldPacket();
                Trinity::BuildChatPacket(*i_normalMessage, i_c, needCoded, needEmpty);
            }

            session->SendPacket(i_normalMessage);
        }
    }
}

/*
void
MessageDistDeliverer::VisitObject(Player* player)
{
    if (!i_ownTeamOnly || (i_source.GetTypeId() == TYPEID_PLAYER && player->GetTeam() == ((Player&)i_source).GetTeam()))
    {
        SendPacket(player);
    }
}
*/

void UnfriendlyMessageDistDeliverer::Visit(PlayerMapType &m)
{
    for (auto &target : m)
    {
        if (!target->InSamePhase(i_phaseMask) || !i_source->InSamePhaseId(target))
            continue;

        if (target->GetExactDist2dSq(i_source) > i_distSq)
            continue;

        // Send packet to all who are sharing the player's vision
        if (!target->GetSharedVisionList().empty())
        {
            for (auto &player : target->GetSharedVisionList())
                if (player->m_seer == target)
                    SendPacket(player);
        }

        if (target->m_seer == target || target->GetVehicle())
            SendPacket(target);
    }
}

bool AnyDeadUnitObjectInRangeCheck::operator()(Player* u)
{
    return !u->isAlive() && !u->HasAuraType(SPELL_AURA_GHOST) && i_searchObj->IsWithinDistInMap(u, i_range);
}

bool AnyDeadUnitObjectInRangeCheck::operator()(Corpse* u)
{
    return u->GetType() != CORPSE_BONES && i_searchObj->IsWithinDistInMap(u, i_range);
}

bool AnyDeadUnitObjectInRangeCheck::operator()(Creature* u)
{
    return !u->isAlive() && i_searchObj->IsWithinDistInMap(u, i_range);
}

bool AnyDeadUnitSpellTargetInRangeCheck::operator()(Player* u)
{
    return AnyDeadUnitObjectInRangeCheck::operator()(u) && i_check(u);
}

bool AnyDeadUnitSpellTargetInRangeCheck::operator()(Corpse* u)
{
    return AnyDeadUnitObjectInRangeCheck::operator()(u) && i_check(u);
}

bool AnyDeadUnitSpellTargetInRangeCheck::operator()(Creature* u)
{
    return AnyDeadUnitObjectInRangeCheck::operator()(u) && i_check(u);
}
