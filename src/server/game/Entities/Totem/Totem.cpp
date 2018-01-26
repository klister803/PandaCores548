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

#include "Totem.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Group.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "SpellInfo.h"

Totem::Totem(SummonPropertiesEntry const* properties, Unit* owner) : Minion(properties, owner, false)
{
    m_unitTypeMask |= UNIT_MASK_TOTEM;
    m_duration = 0;
    m_type = TOTEM_PASSIVE;
}

void Totem::Update(uint32 time)
{
    if (!m_owner->IsAlive() || !IsAlive())
    {
        UnSummon();                                         // remove self
        return;
    }

    if (m_duration <= time)
    {
        UnSummon();                                         // remove self
        return;
    }
    else
        m_duration -= time;

    Creature::Update(time);
}

void Totem::InitStats(uint32 duration)
{
    bool damageSet = false;
    // client requires SMSG_TOTEM_CREATED to be sent before adding to world and before removing old totem
    if (m_owner->GetTypeId() == TYPEID_PLAYER
        && m_Properties->Slot >= SUMMON_SLOT_TOTEM
        && m_Properties->Slot < MAX_TOTEM_SLOT)
    {
        // set display id depending on caster's race
        if (uint32 display = m_owner->GetModelForTotem(PlayerTotemType(m_Properties->Id)))
            SetDisplayId(display);

        // Totemic Encirclement
        if (m_owner->HasAura(58057)
            && GetUInt32Value(UNIT_CREATED_BY_SPELL) != 120214
            && GetUInt32Value(UNIT_CREATED_BY_SPELL) != 120217
            && GetUInt32Value(UNIT_CREATED_BY_SPELL) != 120218
            && GetUInt32Value(UNIT_CREATED_BY_SPELL) != 120219)
        {
            for (int i = SUMMON_SLOT_TOTEM; i < MAX_TOTEM_SLOT; ++i)
            {
                if (i == m_Properties->Slot)
                    continue;
                else
                {
                    if(m_owner->m_SummonSlot[i])
                        if(Creature* oldSummon = GetMap()->GetCreature(m_owner->m_SummonSlot[i]))
                            continue;
                    switch (i)
                    {
                        case 1:
                            m_owner->CastSpell(m_owner, 120217, true); // Fake Fire Totem
                            break;
                        case 2:
                            m_owner->CastSpell(m_owner, 120218, true); // Fake Earth Totem
                            break;
                        case 3:
                            m_owner->CastSpell(m_owner, 120214, true); // Fake Water Totem
                            break;
                        case 4:
                            m_owner->CastSpell(m_owner, 120219, true); // Fake Wind Totem
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    Minion::InitStats(duration);

    // Get spell cast by totem
    if (SpellInfo const* totemSpell = sSpellMgr->GetSpellInfo(GetSpell()))
        if (totemSpell->CalcCastTime())   // If spell has cast time -> its an active totem
            m_type = TOTEM_ACTIVE;

    if (GetEntry() == SENTRY_TOTEM_ENTRY)
        SetReactState(REACT_AGGRESSIVE);

    m_duration = duration;

    SetLevel(m_owner->getLevel());

    InitBaseStat(GetEntry(), damageSet);
}

void Totem::InitSummon()
{
    if (m_type == TOTEM_PASSIVE && GetSpell())
        CastSpell(this, GetSpell(), true);

    // Some totems can have both instant effect and passive spell
    if (GetSpell(1))
        CastSpell(this, GetSpell(1), true);
}

void Totem::UnSummon(uint32 msTime)
{
    if (msTime)
    {
        m_Events.AddEvent(new ForcedUnsummonDelayEvent(*this), m_Events.CalculateTime(msTime));
        return;
    }

    // Totemic Restoration
    if (m_duration > 0 || !GetHealth())
    {
        if (m_owner->HasAura(108284))
        {
            float pct = m_duration * 100.0f / GetTimer();

            if (pct > 50.0f)
                pct = 50.0f;

            Player* _player = m_owner->ToPlayer();
            uint32 spellId = GetUInt32Value(UNIT_CREATED_BY_SPELL);
            if (_player && spellId)
            {
                if (_player->HasSpellCooldown(spellId))
                {
                    uint32 totalCooldown = sSpellMgr->GetSpellInfo(spellId)->RecoveryTime;
                    int32 lessCooldown = CalculatePct(totalCooldown, int32(pct));

                    _player->ModifySpellCooldown(spellId, -lessCooldown);
                }
            }
        }
    }

    CombatStop();
    RemoveAurasDueToSpell(GetSpell(), GetGUID());
    CastPetAuras(false);

    // clear owner's totem slot
    for (int i = SUMMON_SLOT_TOTEM; i < MAX_TOTEM_SLOT; ++i)
    {
        if (m_owner->m_SummonSlot[i] == GetGUID())
        {
            m_owner->m_SummonSlot[i] = 0;
            break;
        }
    }

    m_owner->RemoveAurasDueToSpell(GetSpell(), GetGUID());

    // Remove Sentry Totem Aura
    if (GetEntry() == SENTRY_TOTEM_ENTRY)
        m_owner->RemoveAurasDueToSpell(SENTRY_TOTEM_SPELLID);

    //remove aura all party members too
    if (Player* owner = m_owner->ToPlayer())
    {
        owner->SendAutoRepeatCancel(this);

        if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(GetUInt32Value(UNIT_CREATED_BY_SPELL)))
            owner->SendCooldownEvent(spell, 0, NULL, false);

        if (Group* group = owner->GetGroup())
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* target = itr->getSource();
                if (target && group->SameSubGroup(owner, target))
                    target->RemoveAurasDueToSpell(GetSpell(), GetGUID());
            }
        }
    }

    AddObjectToRemoveList();
}

bool Totem::IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const
{
    return Creature::IsImmunedToSpellEffect(spellInfo, index);
}

bool Totem::IsImmunedToSpell(SpellInfo const* spellInfo)
{
    if (!spellInfo)
        return false;

    if (spellInfo->AttributesEx5 & SPELL_ATTR5_CANT_IMMUNITY_SPELL)
        return false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellInfo->Effects[i]->Effect)
        {
            case SPELL_EFFECT_HEALTH_LEECH:
            case SPELL_EFFECT_HEAL:
            case SPELL_EFFECT_HEAL_MAX_HEALTH:
            case SPELL_EFFECT_HEAL_PCT:
            case SPELL_EFFECT_SPIRIT_HEAL:
                return true;
            case SPELL_EFFECT_APPLY_AURA:
            {
                switch (spellInfo->Effects[i]->ApplyAuraName)
                {
                    case SPELL_AURA_MOD_FEAR:
                    case SPELL_AURA_MOD_FEAR_2:
                    case SPELL_AURA_TRANSFORM:
                    case SPELL_AURA_MOD_CONFUSE:
                    case SPELL_AURA_MOD_CHARM:
                    case SPELL_AURA_MOD_STUN:
                    case SPELL_AURA_MOD_SILENCE:
                    case SPELL_AURA_SCHOOL_ABSORB:
                    case SPELL_AURA_PERIODIC_HEAL:
                        return true;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
    return false;
}
