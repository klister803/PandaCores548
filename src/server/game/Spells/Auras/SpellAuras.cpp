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

#include "Common.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "DynamicObject.h"
#include "ObjectAccessor.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Vehicle.h"

AuraApplication::AuraApplication(Unit* target, Unit* caster, Aura* aura, uint32 effMask):
_target(target), _base(aura), _removeMode(AURA_REMOVE_NONE), _slot(MAX_AURAS),
_flags(AFLAG_NONE), _effectsToApply(effMask), _needClientUpdate(false), _effectMask(NULL)
{
    ASSERT(GetTarget() && GetBase());

    if (GetBase()->CanBeSentToClient())
    {
        // Try find slot for aura
        uint8 slot = MAX_AURAS;
        // Lookup for auras already applied from spell
        if (AuraApplication * foundAura = GetTarget()->GetAuraApplication(GetBase()->GetId(), GetBase()->GetCasterGUID(), GetBase()->GetCastItemGUID()))
        {
            // allow use single slot only by auras from same caster
            slot = foundAura->GetSlot();
        }
        else
        {
            Unit::VisibleAuraMap const* visibleAuras = GetTarget()->GetVisibleAuras();
            // lookup for free slots in units visibleAuras
            Unit::VisibleAuraMap::const_iterator itr = visibleAuras->find(0);
            for (uint32 freeSlot = 0; freeSlot < MAX_AURAS; ++itr, ++freeSlot)
            {
                if (itr == visibleAuras->end() || itr->first != freeSlot)
                {
                    slot = freeSlot;
                    break;
                }
            }
        }

        // Register Visible Aura
        if (slot < MAX_AURAS)
        {
            _slot = slot;
            GetTarget()->SetVisibleAura(slot, this);
            SetNeedClientUpdate();
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura: %u Effect: %d put to unit visible auras slot: %u", GetBase()->GetId(), GetEffectMask(), slot);
        }
        else
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura: %u Effect: %d could not find empty unit visible slot", GetBase()->GetId(), GetEffectMask());
    }

    _InitFlags(caster, effMask);
}

void AuraApplication::_Remove()
{
    uint8 slot = GetSlot();

    if (slot >= MAX_AURAS)
        return;

    if (AuraApplication * foundAura = _target->GetAuraApplication(GetBase()->GetId(), GetBase()->GetCasterGUID(), GetBase()->GetCastItemGUID()))
    {
        // Reuse visible aura slot by aura which is still applied - prevent storing dead pointers
        if (slot == foundAura->GetSlot())
        {
            if (GetTarget()->GetVisibleAura(slot) == this)
            {
                GetTarget()->SetVisibleAura(slot, foundAura);
                foundAura->SetNeedClientUpdate();
            }
            // set not valid slot for aura - prevent removing other visible aura
            slot = MAX_AURAS;
        }
    }

    // update for out of range group members
    if (slot < MAX_AURAS)
    {
        GetTarget()->RemoveVisibleAura(slot);
        ClientUpdate(true);
    }
}

void AuraApplication::_InitFlags(Unit* caster, uint32 effMask)
{
    // mark as selfcasted if needed
    _flags |= (GetBase()->GetCasterGUID() == GetTarget()->GetGUID()) ? AFLAG_CASTER : AFLAG_NONE;

    // aura is casted by self or an enemy
    // one negative effect and we know aura is negative
    if (IsSelfcasted() || !caster || !caster->IsFriendlyTo(GetTarget()))
    {
        bool negativeFound = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (((1<<i) & effMask) && !GetBase()->GetSpellInfo()->IsPositiveEffect(i, IsSelfcasted()))
            {
                negativeFound = true;
                break;
            }
        }
        _flags |= negativeFound ? AFLAG_NEGATIVE : AFLAG_POSITIVE;
    }
    // aura is casted by friend
    // one positive effect and we know aura is positive
    else
    {
        bool positiveFound = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (((1<<i) & effMask) && GetBase()->GetSpellInfo()->IsPositiveEffect(i))
            {
                positiveFound = true;
                break;
            }
        }
        _flags |= positiveFound ? AFLAG_POSITIVE : AFLAG_NEGATIVE;
    }

    if (GetBase()->GetSpellInfo()->AttributesEx8 & SPELL_ATTR8_AURA_SEND_AMOUNT)
        _flags |= AFLAG_ANY_EFFECT_AMOUNT_SENT;
}

void AuraApplication::_HandleEffect(uint8 effIndex, bool apply)
{
    AuraEffect* aurEff = GetBase()->GetEffect(effIndex);
    ASSERT(aurEff);
    ASSERT(HasEffect(effIndex) == (!apply));
    ASSERT((1<<effIndex) & _effectsToApply);
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraApplication::_HandleEffect: GetId %i, GetAuraType %u, apply: %u: amount: %i, m_send_baseAmount: %i, effIndex: %i", GetBase()->GetId(), aurEff->GetAuraType(), apply, aurEff->GetAmount(), aurEff->GetBaseSendAmount(), effIndex);

    if (apply)
    {
        ASSERT(!(_effectMask & (1<<effIndex)));
        _effectMask |= 1<<effIndex;
        aurEff->HandleEffect(this, AURA_EFFECT_HANDLE_REAL, true);
    }
    else
    {
        ASSERT(_effectMask & (1<<effIndex));
        _effectMask &= ~(1<<effIndex);
        aurEff->HandleEffect(this, AURA_EFFECT_HANDLE_REAL, false);

        // Remove all triggered by aura spells vs unlimited duration
        aurEff->CleanupTriggeredSpells(GetTarget());
    }
    SetNeedClientUpdate();
}

void AuraApplication::BuildBitUpdatePacket(ByteBuffer& data, bool remove) const
{
    if (!data.WriteBit(!remove))
    {
        //ASSERT(!_target->GetVisibleAura(_slot));
        return;
    }

    uint32 flags = _flags;
    Aura const* aura = GetBase();
    if (aura->GetMaxDuration() > 0 && !(aura->GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_HIDE_DURATION))
        flags |= AFLAG_DURATION;

    if (data.WriteBit(!(flags & AFLAG_CASTER)))
        data.WriteGuidMask<2, 3, 4, 0, 1, 6, 7, 5>(aura->GetCasterGUID());
    uint32 count = 0;
    bool sendEffect = false;
    bool nosendEffect = false;
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if(!(_effectsToApply & (1 << i)))
            continue;

        if(aura->GetSpellInfo()->Effects[i].IsEffect())
        {
            ++count;
            if (AuraEffect const* eff = aura->GetEffect(i))
            {
                switch (eff->GetAuraType())
                {
                    case SPELL_AURA_SCHOOL_ABSORB:
                    case SPELL_AURA_SCHOOL_HEAL_ABSORB:
                    case SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS:
                    case SPELL_AURA_ADD_FLAT_MODIFIER:
                    case SPELL_AURA_ADD_PCT_MODIFIER:
                        nosendEffect = true;
                        break;
                    default:
                        if(eff->GetAmount() != eff->GetBaseSendAmount())
                            sendEffect = true;
                        break;
                }
            }
        }
    }
    data.WriteBits((sendEffect && !nosendEffect) ? count : 0, 22);  // effect count 2
    data.WriteBits(count, 22);  // effect count
    data.WriteBit(flags & AFLAG_DURATION);  // has duration
    data.WriteBit(flags & AFLAG_DURATION);  // has max duration
}

void AuraApplication::BuildByteUpdatePacket(ByteBuffer& data, bool remove, uint32 overrideAura) const
{
    if (remove)
    {
        //ASSERT(!_target->GetVisibleAura(_slot));
        data << uint8(_slot);
        return;
    }

    uint32 flags = _flags;
    Aura const* aura = GetBase();
    if (aura->GetMaxDuration() > 0 && !(aura->GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_HIDE_DURATION))
        flags |= AFLAG_DURATION;

    bool sendEffect = false;
    bool nosendEffect = false;
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if(!(_effectsToApply & (1 << i)))
            continue;

        if(aura->GetSpellInfo()->Effects[i].IsEffect())
        {
            if (AuraEffect const* eff = aura->GetEffect(i))
            {
                switch (eff->GetAuraType())
                {
                    case SPELL_AURA_SCHOOL_ABSORB:
                    case SPELL_AURA_SCHOOL_HEAL_ABSORB:
                    case SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS:
                    case SPELL_AURA_ADD_FLAT_MODIFIER:
                    case SPELL_AURA_ADD_PCT_MODIFIER:
                        nosendEffect = true;
                        break;
                    default:
                        if(eff->GetAmount() != eff->GetBaseSendAmount())
                            sendEffect = true;
                        break;
                }
            }
        }
    }

    if(sendEffect && !nosendEffect)
    {
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if(!(_effectsToApply & (1 << i)))
                continue;
            if(aura->GetSpellInfo()->Effects[i].IsEffect())
            {
                if (AuraEffect const* eff = aura->GetEffect(i))
                {
                    if(eff->GetAmount() != eff->GetBaseSendAmount())
                        data << float(eff->GetAmount());
                    else
                        data << float(0.0f);
                }
                else
                    data << float(0.0f);
            }
        }
    }
    else
    {
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if(!(_effectsToApply & (1 << i)))
                continue;

            if(aura->GetSpellInfo()->Effects[i].IsEffect())
            {
                if (AuraEffect const* eff = aura->GetEffect(i))
                    data << float(eff->GetAmount());
                else
                    data << float(0.0f);
            }
        }
    }

    data << uint16(aura->GetCasterLevel());
    if (!(flags & AFLAG_CASTER))
        data.WriteGuidBytes<0, 6, 1, 4, 5, 3, 2, 7>(aura->GetCasterGUID());
    data << uint8(flags);
    if (flags & AFLAG_DURATION)
        data << uint32(aura->GetMaxDuration());

    //effect2 send base amount
    if(sendEffect && !nosendEffect)
    {
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if(!(_effectsToApply & (1 << i)))
                continue;

            if(aura->GetSpellInfo()->Effects[i].IsEffect())
            {
                if (AuraEffect const* eff = aura->GetEffect(i))
                    data << float(eff->GetAmount());
                else
                    data << float(0.0f);
            }
        }
    }

    // send stack amount for aura which could be stacked (never 0 - causes incorrect display) or charges
    // stack amount has priority over charges (checked on retail with spell 50262)
    data << uint8((aura->GetStackAmount() > 1 || !aura->GetSpellInfo()->ProcCharges) ? aura->GetStackAmount() : aura->GetCharges());
    data << uint32(GetEffectMask());
    if (flags & AFLAG_DURATION)
        data << uint32(aura->GetDuration());
    data << uint32(overrideAura ? overrideAura : aura->GetId());
    data << uint8(_slot);
}

void AuraApplication::ClientUpdate(bool remove)
{
    if (!GetTarget())
        return;

    _needClientUpdate = false;

    ObjectGuid targetGuid = GetTarget()->GetObjectGuid();

    WorldPacket data(SMSG_AURA_UPDATE);
    data.WriteGuidMask<0>(targetGuid);
    data.WriteBit(0);   // has power unit
    data.WriteBit(0);   // full update
    data.WriteGuidMask<6>(targetGuid);
    /*
    if (hasPowerData) { }
    */
    data.WriteGuidMask<4, 7, 3>(targetGuid);
    data.WriteBits(1, 24);
    data.WriteGuidMask<1, 5, 2>(targetGuid);

    BuildBitUpdatePacket(data, remove);
    BuildByteUpdatePacket(data, remove);

    /*
    if (hasPowerData) { }
    */

    data.WriteGuidBytes<7, 4, 2, 0, 6, 5, 1, 3>(targetGuid);

    _target->SendMessageToSet(&data, true);
}

uint32 Aura::BuildEffectMaskForOwner(SpellInfo const* spellProto, uint32 avalibleEffectMask, WorldObject* owner)
{
    ASSERT(spellProto);
    ASSERT(owner);
    uint32 effMask = 0;
    switch (owner->GetTypeId())
    {
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
            {
                if (spellProto->Effects[i].IsUnitOwnedAuraEffect())
                    effMask |= 1 << i;
            }
            break;
        case TYPEID_DYNAMICOBJECT:
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
            {
                if (spellProto->Effects[i].Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
                    effMask |= 1 << i;
            }
            break;
        default:
            break;
    }
    return effMask & avalibleEffectMask;
}

Aura* Aura::TryRefreshStackOrCreate(SpellInfo const* spellproto, uint32 tryEffMask, WorldObject* owner, Unit* caster, int32* baseAmount /*= NULL*/, Item* castItem /*= NULL*/, uint64 casterGUID /*= 0*/, bool* refresh /*= NULL*/, uint16 stackAmount, Spell* spell)
{
    ASSERT(spellproto);
    ASSERT(owner);
    ASSERT(caster || casterGUID);
    ASSERT(tryEffMask <= MAX_EFFECT_MASK);
    if (refresh)
        *refresh = false;
    uint32 effMask = Aura::BuildEffectMaskForOwner(spellproto, tryEffMask, owner);
    if(caster)
        effMask = CalculateEffMaskFromDummy(caster, owner, effMask, spellproto);
    if (!effMask)
        return NULL;

    Aura* foundAura = owner->ToUnit()->_TryStackingOrRefreshingExistingAura(spellproto, effMask, caster, baseAmount, castItem, casterGUID);
    if (foundAura != NULL && !stackAmount)
    {
        // we've here aura, which script triggered removal after modding stack amount
        // check the state here, so we won't create new Aura object
        if (foundAura->IsRemoved())
            return NULL;

        if (refresh)
            *refresh = true;
        return foundAura;
    }
    else
        return Create(spellproto, effMask, owner, caster, baseAmount, castItem, casterGUID, stackAmount, spell);
}

Aura* Aura::TryCreate(SpellInfo const* spellproto, uint32 tryEffMask, WorldObject* owner, Unit* caster, int32* baseAmount /*= NULL*/, Item* castItem /*= NULL*/, uint64 casterGUID /*= 0*/)
{
    ASSERT(spellproto);
    ASSERT(owner);
    ASSERT(caster || casterGUID);
    ASSERT(tryEffMask <= MAX_EFFECT_MASK);
    uint32 effMask = Aura::BuildEffectMaskForOwner(spellproto, tryEffMask, owner);
    if(caster)
        effMask = CalculateEffMaskFromDummy(caster, owner, effMask, spellproto);
    if (!effMask)
        return NULL;
    return Create(spellproto, effMask, owner, caster, baseAmount, castItem, casterGUID);
}

Aura* Aura::Create(SpellInfo const* spellproto, uint32 effMask, WorldObject* owner, Unit* caster, int32* baseAmount, Item* castItem, uint64 casterGUID, uint16 stackAmount, Spell* spell)
{
    ASSERT(effMask);
    ASSERT(spellproto);
    ASSERT(owner);
    ASSERT(caster || casterGUID);
    ASSERT(effMask <= MAX_EFFECT_MASK);
    // try to get caster of aura
    if (casterGUID)
    {
        if (owner->GetGUID() == casterGUID)
            caster = owner->ToUnit();
        else
            caster = ObjectAccessor::GetUnit(*owner, casterGUID);
    }
    else
        casterGUID = caster->GetGUID();

    // check if aura can be owned by owner
    if (owner->isType(TYPEMASK_UNIT))
        if (!owner->IsInWorld() || ((Unit*)owner)->IsDuringRemoveFromWorld())
            // owner not in world so don't allow to own not self casted single target auras
            if (casterGUID != owner->GetGUID() && spellproto->IsSingleTarget(caster))
                return NULL;

    Aura* aura = NULL;
    if(spellproto->IsSingleTarget(caster))
    {
        bool moving = false;
        Unit::AuraList& scAuras = caster->GetSingleCastAuras();
        for (Unit::AuraList::iterator itr = scAuras.begin(); itr != scAuras.end();)
        {
            if ((*itr)->GetId() == spellproto->Id)
            {
                //test code
                /*sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura* Aura::Create aura %u, GetCasterGUID %u", (*itr)->GetId(), caster->GetGUID());
                Aura::ApplicationMap const& appMap = (*itr)->GetApplicationMap();
                for (Aura::ApplicationMap::const_iterator app = appMap.begin(); app!= appMap.end();)
                {
                    AuraApplication * aurApp = app->second;
                    ++app;
                    Unit* target = aurApp->GetTarget();
                    aurApp->SetRemoveMode(AURA_REMOVE_BY_DEFAULT);
                    (*itr)->_UnapplyForTarget(target, caster, aurApp);
                    (*itr)->ChangeOwner(owner);
                }
                return (*itr);*/
                stackAmount = (*itr)->GetStackAmount();
            }
            ++itr;
        }
    }

    switch (owner->GetTypeId())
    {
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            aura = new UnitAura(spellproto, effMask, owner, caster, baseAmount, castItem, casterGUID, stackAmount);
            if(spell)
                aura->m_damage_amount = spell->GetDamage();
            aura->_InitEffects(effMask, caster, baseAmount);
            aura->GetUnitOwner()->_AddAura((UnitAura*)aura, caster);
            break;
        case TYPEID_DYNAMICOBJECT:
            aura = new DynObjAura(spellproto, effMask, owner, caster, baseAmount, castItem, casterGUID, stackAmount);
            aura->_InitEffects(effMask, caster, baseAmount);
            aura->GetDynobjOwner()->SetAura(aura);

            ASSERT(aura->GetDynobjOwner());
            ASSERT(aura->GetDynobjOwner()->IsInWorld());
            ASSERT(aura->GetDynobjOwner()->GetMap() == aura->GetCaster()->GetMap());
            break;
        default:
            ASSERT(false);
            return NULL;
    }
    // aura can be removed in Unit::_AddAura call
    if (aura->IsRemoved())
        return NULL;
    return aura;
}

uint32 Aura::CalculateEffMaskFromDummy(Unit* caster, WorldObject* target, uint32 effMask, SpellInfo const* spellproto)
{
    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(spellproto->Id))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            Unit* _caster = caster;
            Unit* _targetAura = caster;
            bool check = false;

            if(!_caster)
                return effMask;

            if(itr->targetaura == 1 && _caster->ToPlayer()) //get target pet
            {
                if (Pet* pet = _caster->ToPlayer()->GetPet())
                    _targetAura = (Unit*)pet;
            }
            if(itr->targetaura == 2) //get target owner
            {
                if (Unit* owner = _caster->GetOwner())
                    _targetAura = owner;
            }
            if(itr->targetaura == 3 && target->ToUnit()) //get target
                _targetAura = target->ToUnit();

            if(!_targetAura)
                _targetAura = _caster;

            switch (itr->option)
            {
                case SPELL_DUMMY_MOD_EFFECT_MASK: //4
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && !_targetAura->HasAura(itr->spellDummyId))
                    {
                        effMask &= ~itr->effectmask;
                        check = true;
                    }
                    if(itr->spellDummyId < 0 && _targetAura->HasAura(abs(itr->spellDummyId)))
                    {
                        effMask &= ~itr->effectmask;
                        check = true;
                    }
                    break;
                }
            }
            if(check && itr->removeAura)
                _caster->RemoveAurasDueToSpell(itr->removeAura);
        }
    }

    return effMask;
}

void Aura::CalculateDurationFromDummy(int32 &duration)
{
    Unit* _caster = GetCaster();
    if(!_caster)
        return;

    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(GetId()))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            Unit* _targetAura = _caster;
            bool check = false;

            if(itr->targetaura == 1 && _caster->ToPlayer()) //get target pet
            {
                if (Pet* pet = _caster->ToPlayer()->GetPet())
                    _targetAura = (Unit*)pet;
            }
            if(itr->targetaura == 2) //get target owner
            {
                if (Unit* owner = _caster->GetOwner())
                    _targetAura = owner;
            }
            if(itr->targetaura == 3 && m_owner->ToUnit()) //get target
                _targetAura = m_owner->ToUnit();

            if(!_targetAura)
                _targetAura = _caster;

            switch (itr->option)
            {
                case SPELL_DUMMY_DURATION_ADD_PERC: //11
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            duration += CalculatePct(duration, bp);
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(abs(itr->spellDummyId)))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            duration -= CalculatePct(duration, bp);
                            check = true;
                        }
                    }
                    break;
                }
                case SPELL_DUMMY_DURATION_ADD_VALUE: //12
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            duration += bp;
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(abs(itr->spellDummyId)))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            duration -= bp;
                            check = true;
                        }
                    }
                    break;
                }
            }
            if(check && itr->removeAura)
                _caster->RemoveAurasDueToSpell(itr->removeAura);
        }
    }
}

Aura::Aura(SpellInfo const* spellproto, WorldObject* owner, Unit* caster, Item* castItem, uint64 casterGUID, uint16 stackAmount) :
m_spellInfo(spellproto), m_casterGuid(casterGUID ? casterGUID : caster->GetGUID()),
m_castItemGuid(castItem ? castItem->GetGUID() : 0), m_applyTime(time(NULL)),
m_owner(owner), m_timeCla(0), m_updateTargetMapInterval(0),
m_casterLevel(caster ? caster->getLevel() : m_spellInfo->SpellLevel), m_procCharges(0), m_stackAmount(stackAmount ? stackAmount: 1),
m_isRemoved(false), m_isSingleTarget(false), m_isUsingCharges(false), m_fromAreatrigger(false), m_inArenaNerf(false), m_aura_amount(0),
m_diffMode(caster ? caster->GetSpawnMode() : 0), m_spellDynObjGuid(0), m_spellAreaTrGuid(0), m_customData(0), m_damage_amount(0), m_removeDelay(0)
{
    SpellPowerEntry power;
    if (!GetSpellInfo()->GetSpellPowerByCasterPower(GetCaster(), power))
        power = GetSpellInfo()->GetPowerInfo(0);

    if (power.powerPerSecond || power.powerPerSecondPercentage)
        m_timeCla = 1 * IN_MILLISECONDS;

    LoadScripts();

    m_maxDuration = CalcMaxDuration(caster);
    m_duration = m_maxDuration;
    m_allDuration = 0;
    m_procCharges = CalcMaxCharges(caster);
    m_isUsingCharges = m_procCharges != 0;

    //For scaling trinket
    if((m_spellInfo->AttributesEx11 & SPELL_ATTR11_SEND_ITEM_LEVEL) && castItem)
        m_casterLevel = castItem->GetLevel();

    if (castItem && castItem->GetLevelBeforeCap() && castItem->GetLevel() > 502)
    {
        m_inArenaNerf = true;
    }

    if(SpellScalingEntry const* _scaling = m_spellInfo->GetSpellScaling())
    {
        if(_scaling->ScalesFromItemLevel && castItem)
            m_casterLevel = castItem->GetLevel();
        //For scaling max level
        if(_scaling->MaxScalingLevel && caster && caster->getLevel() > _scaling->MaxScalingLevel)
            m_casterLevel = _scaling->MaxScalingLevel;
    }
}

void Aura::_InitEffects(uint32 effMask, Unit* caster, int32 *baseAmount)
{
    // shouldn't be in constructor - functions in AuraEffect::AuraEffect use polymorphism
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (effMask & (uint8(1) << i))
        {
            m_effects[i] = new AuraEffect(this, i, baseAmount ? baseAmount + i : NULL, caster);

            m_effects[i]->CalculatePeriodic(caster, true, false);
            m_effects[i]->SetAmount(m_effects[i]->CalculateAmount(caster, m_aura_amount));
            m_effects[i]->CalculateSpellMod();
        }
        else
            m_effects[i] = NULL;
    }
}

Aura::~Aura()
{
    // unload scripts
    while (!m_loadedScripts.empty())
    {
        std::list<AuraScript*>::iterator itr = m_loadedScripts.begin();
        (*itr)->_Unload();
        delete (*itr);
        m_loadedScripts.erase(itr);
    }

    ASSERT(m_applications.empty());
    _DeleteRemovedApplications();
}

Unit* Aura::GetCaster() const
{
    if (GetOwner()->GetGUID() == GetCasterGUID())
        return GetUnitOwner();
    if (AuraApplication const* aurApp = GetApplicationOfTarget(GetCasterGUID()))
        return aurApp->GetTarget();

    return ObjectAccessor::GetUnit(*GetOwner(), GetCasterGUID());
}

AuraObjectType Aura::GetType() const
{
    return (m_owner->GetTypeId() == TYPEID_DYNAMICOBJECT) ? DYNOBJ_AURA_TYPE : UNIT_AURA_TYPE;
}

void Aura::_ApplyForTarget(Unit* target, Unit* caster, AuraApplication * auraApp)
{
    ASSERT(target);
    ASSERT(auraApp);
    // aura mustn't be already applied on target
    ASSERT (!IsAppliedOnTarget(target->GetGUID()) && "Aura::_ApplyForTarget: aura musn't be already applied on target");

    m_applications[target->GetGUID()] = auraApp;

    // set infinity cooldown state for spells
    if (caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_spellInfo->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = m_castItemGuid ? caster->ToPlayer()->GetItemByGuid(m_castItemGuid) : NULL;
            caster->ToPlayer()->AddSpellAndCategoryCooldowns(m_spellInfo, castItem ? castItem->GetEntry() : 0, NULL, true);
        }
    }
}

void Aura::_UnapplyForTarget(Unit* target, Unit* caster, AuraApplication * auraApp)
{
    ASSERT(target);
    ASSERT(auraApp->GetRemoveMode());
    ASSERT(auraApp);

    ApplicationMap::iterator itr = m_applications.find(target->GetGUID());

    // TODO: Figure out why this happens
    if (itr == m_applications.end())
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "Aura::_UnapplyForTarget, target:%u, caster:%u, spell:%u was not found in owners application map!",
        target->GetGUIDLow(), caster ? caster->GetGUIDLow() : 0, auraApp->GetBase()->GetSpellInfo()->Id);
        //ASSERT(false);
        return;
    }

    // aura has to be already applied
    ASSERT(itr->second == auraApp);
    m_applications.erase(itr);

    m_removedApplications.push_back(auraApp);

    // reset cooldown state for spells
    if (caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetSpellInfo()->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE && !(GetSpellInfo()->Id == 34477 && caster->HasAura(56829) && (caster->GetPetGUID() == target->GetGUID())))
            // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existed cases)
            caster->ToPlayer()->SendCooldownEvent(GetSpellInfo());
    }
}

// removes aura from all targets
// and marks aura as removed
void Aura::_Remove(AuraRemoveMode removeMode)
{
    ASSERT (!m_isRemoved);
    m_isRemoved = true;
    ApplicationMap::iterator appItr = m_applications.begin();
    for (appItr = m_applications.begin(); appItr != m_applications.end();)
    {
        AuraApplication * aurApp = appItr->second;
        Unit* target = aurApp->GetTarget();
        target->_UnapplyAura(aurApp, removeMode);
        appItr = m_applications.begin();
    }
}

void Aura::UpdateTargetMap(Unit* caster, bool apply)
{
    if (IsRemoved())
        return;

    m_updateTargetMapInterval = UPDATE_TARGET_MAP_INTERVAL;

    // fill up to date target list
    //       target, effMask
    std::map<Unit*, uint32> targets;

    FillTargetMap(targets, caster);

    UnitList targetsToRemove;

    // mark all auras as ready to remove
    for (ApplicationMap::iterator appIter = m_applications.begin(); appIter != m_applications.end();++appIter)
    {
        std::map<Unit*, uint32>::iterator existing = targets.find(appIter->second->GetTarget());
        // not found in current area - remove the aura
        if (existing == targets.end())
            targetsToRemove.push_back(appIter->second->GetTarget());
        else
        {
            // needs readding - remove now, will be applied in next update cycle
            // (dbcs do not have auras which apply on same type of targets but have different radius, so this is not really needed)
            if (appIter->second->GetEffectMask() != existing->second || !CanBeAppliedOn(existing->first))
                targetsToRemove.push_back(appIter->second->GetTarget());
            // nothing todo - aura already applied
            // remove from auras to register list
            targets.erase(existing);
        }
    }

    // register auras for units
    for (std::map<Unit*, uint32>::iterator itr = targets.begin(); itr!= targets.end();)
    {
        // aura mustn't be already applied on target
        if (AuraApplication * aurApp = GetApplicationOfTarget(itr->first->GetGUID()))
        {
            // the core created 2 different units with same guid
            // this is a major failue, which i can't fix right now
            // let's remove one unit from aura list
            // this may cause area aura "bouncing" between 2 units after each update
            // but because we know the reason of a crash we can remove the assertion for now
            if (aurApp->GetTarget() != itr->first)
            {
                // remove from auras to register list
                targets.erase(itr++);
                continue;
            }
            else
            {
                // ok, we have one unit twice in target map (impossible, but...)
                ASSERT(false);
            }
        }

        bool addUnit = true;
        // check target immunities
        for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        {
            if (itr->first->IsImmunedToSpellEffect(GetSpellInfo(), effIndex))
                itr->second &= ~(1 << effIndex);
        }
        if (!itr->second
            || itr->first->IsImmunedToSpell(GetSpellInfo())
            || !CanBeAppliedOn(itr->first))
            addUnit = false;

        if (addUnit)
        {
            // persistent area aura does not hit flying targets
            if (GetType() == DYNOBJ_AURA_TYPE)
            {
                if (itr->first->isInFlight())
                    addUnit = false;
            }
            // unit auras can not stack with each other
            else // (GetType() == UNIT_AURA_TYPE)
            {
                // Allow to remove by stack when aura is going to be applied on owner
                if (itr->first != GetOwner())
                {
                    // check if not stacking aura already on target
                    // this one prevents unwanted usefull buff loss because of stacking and prevents overriding auras periodicaly by 2 near area aura owners
                    for (Unit::AuraApplicationMap::iterator iter = itr->first->GetAppliedAuras().begin(); iter != itr->first->GetAppliedAuras().end(); ++iter)
                    {
                        Aura const* aura = iter->second->GetBase();
                        if (!CanStackWith(aura))
                        {
                            addUnit = false;
                            break;
                        }
                    }
                }
            }
        }
        if (!addUnit)
            targets.erase(itr++);
        else
        {
            // owner has to be in world, or effect has to be applied to self
            if (!GetOwner()->IsSelfOrInSameMap(itr->first))
            {
                //TODO: There is a crash caused by shadowfiend load addon
                sLog->outFatal(LOG_FILTER_SPELLS_AURAS, "Aura %u: Owner %s (map %u) is not in the same map as target %s (map %u).", GetSpellInfo()->Id,
                    GetOwner()->GetName(), GetOwner()->IsInWorld() ? GetOwner()->GetMap()->GetId() : uint32(-1),
                    itr->first->GetName(), itr->first->IsInWorld() ? itr->first->GetMap()->GetId() : uint32(-1));
                ASSERT(false);
            }
            itr->first->_CreateAuraApplication(this, itr->second);
            ++itr;
        }
    }

    // remove auras from units no longer needing them
    for (UnitList::iterator itr = targetsToRemove.begin(); itr != targetsToRemove.end();++itr)
        if(Unit* unit = (*itr))
            if (AuraApplication * aurApp = GetApplicationOfTarget(unit->GetGUID()))
                unit->_UnapplyAura(aurApp, AURA_REMOVE_BY_DEFAULT);

    if (!apply)
        return;

    // apply aura effects for units
    for (std::map<Unit*, uint32>::iterator itr = targets.begin(); itr!= targets.end();++itr)
    {
        if (AuraApplication * aurApp = GetApplicationOfTarget(itr->first->GetGUID()))
        {
            // owner has to be in world, or effect has to be applied to self
            ASSERT((!GetOwner()->IsInWorld() && GetOwner() == itr->first) || GetOwner()->IsInMap(itr->first));
            itr->first->_ApplyAura(aurApp, itr->second);
        }
    }
}

// targets have to be registered and not have effect applied yet to use this function
void Aura::_ApplyEffectForTargets(uint8 effIndex)
{
    // prepare list of aura targets
    UnitList targetList;
    for (ApplicationMap::iterator appIter = m_applications.begin(); appIter != m_applications.end(); ++appIter)
    {
        if ((appIter->second->GetEffectsToApply() & (1<<effIndex)) && !appIter->second->HasEffect(effIndex))
            targetList.push_back(appIter->second->GetTarget());
    }

    // apply effect to targets
    for (UnitList::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
    {
        if (GetApplicationOfTarget((*itr)->GetGUID()))
        {
            // owner has to be in world, or effect has to be applied to self
            ASSERT((!GetOwner()->IsInWorld() && GetOwner() == *itr) || GetOwner()->IsInMap(*itr));
            (*itr)->_ApplyAuraEffect(this, effIndex);
        }
    }
}
void Aura::UpdateOwner(uint32 diff, WorldObject* owner)
{
    if(owner != m_owner)
        return;

    Unit* caster = GetCaster();
    // Apply spellmods for channeled auras
    // used for example when triggered spell of spell:10 is modded
    Spell* modSpell = NULL;
    Player* modOwner = NULL;
    if (caster)
    {
        modOwner = caster->GetSpellModOwner();
        if (modOwner)
        {
            modSpell = modOwner->FindCurrentSpellBySpellId(GetId());
            if (modSpell)
                modOwner->SetSpellModTakingSpell(modSpell, true);
        }
    }

    Update(diff, caster);

    if (m_updateTargetMapInterval <= int32(diff))
        UpdateTargetMap(caster);
    else
        m_updateTargetMapInterval -= diff;

    // update aura effects
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i])
            m_effects[i]->Update(diff, caster);

    // remove spellmods after effects update
    if (modSpell)
        modOwner->SetSpellModTakingSpell(modSpell, false);

    _DeleteRemovedApplications();
}

void Aura::Update(uint32 diff, Unit* caster)
{
    if (m_removeDelay > 0)
    {
        m_removeDelay += diff;
        if (m_removeDelay >= 300)
            GetUnitOwner()->RemoveOwnedAura(this, AURA_REMOVE_BY_DEFAULT);
    }

    if (m_duration > 0 || (!IsPassive() && m_duration == -1))
    {
        if (m_duration > 0)
        {
            m_duration -= diff;
            m_allDuration += diff;
            if (m_duration < 0)
                m_duration = 0;
        }

        // handle powerPerSecond/PowerPerSecondPercentage
        if (m_timeCla)
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (GetSpellInfo()->Effects[i].Amplitude)
                {
                    if (m_effects[i])
                        break;

                    return;
                }

            if (m_timeCla > int32(diff))
                m_timeCla -= diff;
            else if (caster)
            {                
                m_timeCla += 1000 - diff;

                SpellPowerEntry power;

                bool channeled = GetSpellInfo()->IsChanneled();

                if (!GetSpellInfo()->GetSpellPowerByCasterPower(caster, power))
                    power = GetSpellInfo()->GetPowerInfo(0);

                /*if (channeled)
                    if (Unit* owner = GetUnitOwner())
                        if (owner->GetGUID() != caster->GetGUID())
                            if (!caster->canSeeOrDetect(owner))
                                caster->CastStop();*/

                if (power.powerPerSecond || power.powerPerSecondPercentage)
                {
                    Powers powertype = Powers(power.powerType);
                    if (powertype == POWER_HEALTH)
                    {
                        uint32 reqHealth = power.powerPerSecond;
                        if (power.powerPerSecondPercentage)
                            reqHealth += caster->CountPctFromMaxHealth(power.powerPerSecondPercentage);
                        
                        if (reqHealth < caster->GetHealth())
                            caster->ModifyHealth(-1 * reqHealth);
                        else
                        {
                            /*if (channeled)
                            {
                                if (Spell* _spell = caster->FindCurrentSpellBySpellId(GetId()))
                                    _spell->RemoveAuraForAllTargets();
                            }
                            else*/
                                Remove();

                            return;
                        }
                    }
                    else
                    {
                        int32 reqPower = power.powerPerSecond;
                        if (power.powerPerSecondPercentage)
                            reqPower += caster->CountPctFromMaxPower(power.powerPerSecondPercentage, powertype);

                        if (reqPower <= caster->GetPower(powertype))
                            caster->ModifyPower(powertype, -1 * reqPower, true);
                        else
                        {
                            /*if (channeled)
                            {
                                if (Spell* _spell = caster->FindCurrentSpellBySpellId(GetId()))
                                    _spell->RemoveAuraForAllTargets();
                            }
                            else*/
                                Remove();
                            
                            return;
                        }
                    }
                }
            }
        }
    }
}

int32 Aura::CalcMaxDuration(Unit* caster)
{
    Player* modOwner = NULL;
    int32 maxDuration;

    if (caster)
    {
        modOwner = caster->GetSpellModOwner();
        maxDuration = caster->CalcSpellDuration(m_spellInfo);
    }
    else
        maxDuration = m_spellInfo->GetDuration();

    if (IsPassive() && !m_spellInfo->DurationEntry)
        maxDuration = -1;

    CallScriptCalcMaxDurationHandlers(maxDuration);

    // IsPermanent() checks max duration (which we are supposed to calculate here)
    if (maxDuration != -1 && modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_DURATION, maxDuration);

    return maxDuration;
}

void Aura::SetDuration(int32 duration, bool withMods)
{
     //! no need chech for -1 or 0
    if (withMods && duration > 0)
    {
        if (Unit* caster = GetCaster())
            if (Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_DURATION, duration);
    }
    m_duration = duration;
    SetNeedClientUpdateForTargets();
}

void Aura::RefreshDuration(bool /*recalculate*/)
{
    SetDuration(GetMaxDuration());

    Unit* caster = GetCaster();
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (HasEffect(i))
            GetEffect(i)->CalculatePeriodic(caster, (GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_START_PERIODIC_AT_APPLY), false);

    SpellPowerEntry power;
    if (!GetSpellInfo()->GetSpellPowerByCasterPower(GetCaster(), power))
        power = GetSpellInfo()->GetPowerInfo(0);

    if (power.powerPerSecond || power.powerPerSecondPercentage)
        m_timeCla = 1 * IN_MILLISECONDS;
}

void Aura::RefreshTimers()
{
    m_maxDuration = CalcMaxDuration();

    RefreshDuration(false);
}

void Aura::SetCharges(uint8 charges)
{
    if (m_procCharges == charges)
        return;
    m_procCharges = charges;
    m_isUsingCharges = m_procCharges != 0;
    SetNeedClientUpdateForTargets();
}

uint8 Aura::CalcMaxCharges(Unit* caster, bool add) const
{
    uint32 maxProcCharges = m_spellInfo->ProcCharges;
    if (SpellProcEntry const* procEntry = sSpellMgr->GetSpellProcEntry(GetId()))
        if(add)
            maxProcCharges = procEntry->modcharges;
        else
            maxProcCharges = procEntry->charges;

    if (caster)
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, maxProcCharges);
    return uint8(maxProcCharges);
}

bool Aura::ModCharges(int32 num, AuraRemoveMode removeMode)
{
    if (IsUsingCharges() || (m_spellInfo->ProcFlags & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_POS_NEG)))
    {
        //if aura not modify and have stack and have charges aura use stack for drop stack visual
        if(m_spellInfo->StackAmount > 1 && GetId() != 114637 && GetId() != 128863 && GetId() != 88819)
        {
            bool _useStack = true;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (AuraEffect* aurEff = GetEffect(i))
                    if (aurEff->GetAuraType() == SPELL_AURA_ADD_FLAT_MODIFIER || aurEff->GetAuraType() == SPELL_AURA_ADD_PCT_MODIFIER)
                        _useStack = false;

            if(_useStack || (m_spellInfo->ProcFlags & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_POS_NEG)) || GetId() == 122355)
            {
                ModStackAmount(num);
                return false;
            }
        }

        int32 charges = m_procCharges + num;
        int32 maxCharges = CalcMaxCharges();

        // limit charges (only on charges increase, charges may be changed manually)
        if ((num > 0) && (charges > int32(maxCharges)))
            charges = maxCharges;
        // we're out of charges, remove
        else if (charges <= 0)
        {
            Remove(removeMode);
            return true;
        }

        SetCharges(charges);
    }
    return false;
}

void Aura::SetStackAmount(uint8 stackAmount)
{
    m_stackAmount = stackAmount;
    Unit* caster = GetCaster();

    std::list<AuraApplication*> applications;
    GetApplicationList(applications);

    for (std::list<AuraApplication*>::const_iterator apptItr = applications.begin(); apptItr != applications.end(); ++apptItr)
        if (!(*apptItr)->GetRemoveMode())
            HandleAuraSpecificMods(*apptItr, caster, false, true);

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (HasEffect(i))
            m_effects[i]->ChangeAmount(m_effects[i]->CalculateAmount(caster, m_aura_amount), false, true);

    for (std::list<AuraApplication*>::const_iterator apptItr = applications.begin(); apptItr != applications.end(); ++apptItr)
        if (!(*apptItr)->GetRemoveMode())
            HandleAuraSpecificMods(*apptItr, caster, true, true);

    SetNeedClientUpdateForTargets();
}

bool Aura::ModStackAmount(int32 num, AuraRemoveMode removeMode)
{
    int32 stackAmount = m_stackAmount + num;
    int32 maxStackAmount = m_spellInfo->StackAmount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_STACKAMOUNT, maxStackAmount);

    // limit the stack amount (only on stack increase, stack amount may be changed manually)
    if ((num > 0) && (stackAmount > int32(maxStackAmount)))
    {
        // not stackable aura - set stack amount to 1
        if (!maxStackAmount)
            stackAmount = 1;
        else
            stackAmount = maxStackAmount;
    }
    // we're out of stacks, remove
    else if (stackAmount <= 0)
    {
        Remove(removeMode);
        return true;
    }

    bool refresh = stackAmount >= GetStackAmount();

    // Update stack amount
    SetStackAmount(stackAmount);

    if (refresh)
    {
        RefreshSpellMods();
        RefreshTimers();

        // Fix Backdraft
        if (m_spellInfo->Id == 117828)
            ModCharges(3);
        // reset charges
        else
            SetCharges(CalcMaxCharges());

        // FIXME: not a best way to synchronize charges, but works
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (AuraEffect* aurEff = GetEffect(i))
                if (aurEff->GetAuraType() == SPELL_AURA_ADD_FLAT_MODIFIER || aurEff->GetAuraType() == SPELL_AURA_ADD_PCT_MODIFIER)
                    if (SpellModifier* mod = aurEff->GetSpellModifier())
                        mod->charges = GetCharges();
    }

    SetNeedClientUpdateForTargets();
    return false;
}

void Aura::SetMaxStackAmount()
{
    int32 maxStackAmount = m_spellInfo->StackAmount;

    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_STACKAMOUNT, maxStackAmount);

    bool refresh = maxStackAmount >= GetStackAmount();

    // Update stack amount
    SetStackAmount(maxStackAmount);

    if (refresh)
    {
        RefreshSpellMods();
        RefreshTimers();

        SetCharges(CalcMaxCharges());

        // FIXME: not a best way to synchronize charges, but works
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (AuraEffect* aurEff = GetEffect(i))
                if (aurEff->GetAuraType() == SPELL_AURA_ADD_FLAT_MODIFIER || aurEff->GetAuraType() == SPELL_AURA_ADD_PCT_MODIFIER)
                    if (SpellModifier* mod = aurEff->GetSpellModifier())
                        mod->charges = GetCharges();
    }

    SetNeedClientUpdateForTargets();
}

void Aura::RefreshSpellMods()
{
    for (Aura::ApplicationMap::const_iterator appIter = m_applications.begin(); appIter != m_applications.end(); ++appIter)
        if (Player* player = appIter->second->GetTarget()->ToPlayer())
            player->RestoreAllSpellMods(0, this);
}

bool Aura::IsArea() const
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (HasEffect(i) && GetSpellInfo()->Effects[i].IsAreaAuraEffect())
            return true;
    }
    return false;
}

bool Aura::IsPassive() const
{
    return GetSpellInfo()->IsPassive();
}

bool Aura::IsDeathPersistent() const
{
    return GetSpellInfo()->IsDeathPersistent();
}

bool Aura::CanBeSaved() const
{
    if (IsPassive() && !(GetSpellInfo()->AttributesCu & SPELL_ATTR0_CU_CAN_BE_SAVED_IN_DB))
        return false;

    if (GetCasterGUID() != GetOwner()->GetGUID())
        if (GetSpellInfo()->IsSingleTarget(GetCaster()))
            return false;

    // Can't be saved - aura handler relies on calculated amount and changes it
    if (HasEffectType(SPELL_AURA_CONVERT_RUNE))
        return false;

    // No point in saving this, since the stable dialog can't be open on aura load anyway.
    if (HasEffectType(SPELL_AURA_OPEN_STABLE))
        return false;

    // Can't save vehicle auras, it requires both caster & target to be in world
    if (HasEffectType(SPELL_AURA_CONTROL_VEHICLE))
        return false;

    // don't save auras casted by entering areatriggers
    if (m_fromAreatrigger)
        return false;

    if (HasEffectType(SPELL_AURA_MOD_NEXT_SPELL))
        return false;

    // don't save auras casted by summons
    if (GetCaster() && GetCaster()->isAnySummons())
        return false;

    switch (GetId())
    {
        // Incanter's Absorbtion - considering the minimal duration and problems with aura stacking
        // we skip saving this aura
        case 44413:
        // When a druid logins, he doesnt have either eclipse power, nor the marker auras, nor the eclipse buffs. Dont save them.
        case 33763:
        case 67483:
        case 67484:
        case 48517:
        case 48518:
        case 107095:
        case 118694:
        case 119048:
        case 108446:
        case 68338:
        case 69303:
        case 72885:
        case 104571:
        case 124458:
        case 130324:
        case 126119:
        case 114695:
        case 144607:
        case 30482:
            return false;
        default:
            break;
    }

    // not save area auras (phase auras, e.t.c)
    SpellAreaMapBounds saBounds = sSpellMgr->GetSpellAreaMapBounds(GetId());
    if (saBounds.first != saBounds.second)
        return false;

    // for correct work it should send cast packets. Never save it. 
    // If need save it -> perfome manual cast for example spell_area table or by script.
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if(GetSpellInfo()->Effects[i].IsEffect())
        {
            if (AuraEffect const* eff = GetEffect(i))
                if(eff->GetAuraType() == SPELL_AURA_OVERRIDE_SPELLS)
                    return false;
        }
    }

    // don't save auras removed by proc system
    if (IsUsingCharges() && !GetCharges())
        return false;

    return true;
}

bool Aura::CanBeSentToClient() const
{
    return !IsPassive() || GetSpellInfo()->HasAreaAuraEffect()
    || HasEffectType(SPELL_AURA_ABILITY_IGNORE_AURASTATE)
    || HasEffectType(SPELL_AURA_MOD_IGNORE_SHAPESHIFT)
    || HasEffectType(SPELL_AURA_CAST_WHILE_WALKING)
    || HasEffectType(SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING)
    || HasEffectType(SPELL_AURA_MOD_SPELL_COOLDOWN_BY_HASTE)
    || HasEffectType(SPELL_AURA_WORGEN_ALTERED_FORM)
    || HasEffectType(SPELL_AURA_MOD_CHARGES)
    || HasEffectType(SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS)
    || HasEffectType(SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2)
    || HasEffectType(SPELL_AURA_MOD_SPELL_VISUAL);
}

bool Aura::IsSingleTargetWith(Aura const* aura) const
{
    // Same spell?
    if (GetSpellInfo()->IsRankOf(aura->GetSpellInfo()))
        return true;

    SpellSpecificType spec = GetSpellInfo()->GetSpellSpecific();
    // spell with single target specific types
    switch (spec)
    {
        case SPELL_SPECIFIC_JUDGEMENT:
        case SPELL_SPECIFIC_MAGE_POLYMORPH:
            if (aura->GetSpellInfo()->GetSpellSpecific() == spec)
                return true;
            break;
        default:
            break;
    }

    if (HasEffectType(SPELL_AURA_CONTROL_VEHICLE) && aura->HasEffectType(SPELL_AURA_CONTROL_VEHICLE))
        return true;

    return false;
}

void Aura::UnregisterCasterAuras()
{
    Unit* caster = GetCaster();
    // TODO: find a better way to do this.
    if (!caster)
        caster = ObjectAccessor::GetObjectInOrOutOfWorld(GetCasterGUID(), (Unit*)NULL);
    if(!caster)
        return;
    caster->GetMyCastAuras().remove(this);

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (GetSpellInfo()->Effects[i].TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_SUMMON)
        {
            if (Player* player = caster->ToPlayer())
                if (player->GetSession() && player->GetSession()->PlayerLogout())
                    return;

            if(!caster->IsInWorld() || !GetUnitOwner() || !GetUnitOwner()->IsInWorld() || caster != GetUnitOwner())
                return;

            caster->RemovePetAndOwnerAura(GetId(), GetUnitOwner());
        }
    }
}

void Aura::UnregisterSingleTarget()
{
    ASSERT(m_isSingleTarget);
    Unit* caster = GetCaster();
    // TODO: find a better way to do this.
    if (!caster)
        caster = ObjectAccessor::GetObjectInOrOutOfWorld(GetCasterGUID(), (Unit*)NULL);
    //ASSERT(caster);
    if(!caster)
        return;
    caster->GetSingleCastAuras().remove(this);
    SetIsSingleTarget(false);
}

int32 Aura::CalcDispelChance(Unit* auraTarget, bool offensive) const
{
    // we assume that aura dispel chance is 100% on start
    // need formula for level difference based chance
    int32 resistChance = 0;

    // Apply dispel mod from aura caster
    if (Unit* caster = GetCaster())
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_RESIST_DISPEL_CHANCE, resistChance);

    // Dispel resistance from target SPELL_AURA_MOD_DISPEL_RESIST
    // Only affects offensive dispels
    if (offensive && auraTarget)
        resistChance += auraTarget->GetTotalAuraModifier(SPELL_AURA_MOD_DISPEL_RESIST);

    resistChance = resistChance < 0 ? 0 : resistChance;
    resistChance = resistChance > 100 ? 100 : resistChance;
    return 100 - resistChance;
}

void Aura::SetLoadedState(int32 maxduration, int32 duration, int32 charges, uint8 stackamount, uint32 recalculateMask, int32 * amount)
{
    m_maxDuration = maxduration;
    m_duration = duration;
    m_procCharges = charges;
    m_isUsingCharges = m_procCharges != 0;
    m_stackAmount = stackamount;
    Unit* caster = GetCaster();
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i])
        {
            m_effects[i]->SetAmount(amount[i]);
            m_effects[i]->SetCanBeRecalculated(recalculateMask & (1<<i));
            m_effects[i]->CalculatePeriodic(caster, false, true);
            m_effects[i]->CalculateSpellMod();
            m_effects[i]->RecalculateAmount(caster);
        }
}

bool Aura::HasEffectType(AuraType type) const
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (HasEffect(i) && m_effects[i]->GetAuraType() == type)
            return true;
    }
    return false;
}

void Aura::RecalculateAmountOfEffects(bool setCanRecalc)
{
    ASSERT (!IsRemoved());
    Unit* caster = GetCaster();
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (HasEffect(i))
        {
            if(setCanRecalc && !IsRemoved())
                m_effects[i]->SetCanBeRecalculated(true);
            m_effects[i]->RecalculateAmount(caster);
        }
}

void Aura::HandleAllEffects(AuraApplication * aurApp, uint8 mode, bool apply)
{
    ASSERT (!IsRemoved());
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i] && !IsRemoved())
            m_effects[i]->HandleEffect(aurApp, mode, apply);
}

void Aura::GetApplicationList(std::list<AuraApplication*> & applicationList) const
{
    for (Aura::ApplicationMap::const_iterator appIter = m_applications.begin(); appIter != m_applications.end(); ++appIter)
    {
        if (appIter->second->GetEffectMask())
            applicationList.push_back(appIter->second);
    }
}

void Aura::SetNeedClientUpdateForTargets() const
{
    for (ApplicationMap::const_iterator appIter = m_applications.begin(); appIter != m_applications.end(); ++appIter)
        appIter->second->SetNeedClientUpdate();
}

// trigger effects on real aura apply/remove
void Aura::HandleAuraSpecificMods(AuraApplication const* aurApp, Unit* caster, bool apply, bool onReapply)
{
    Unit* target = aurApp->GetTarget();
    AuraRemoveMode removeMode = aurApp->GetRemoveMode();
    // handle spell_area table
    SpellAreaForAreaMapBounds saBounds = sSpellMgr->GetSpellAreaForAuraMapBounds(GetId());
    if (saBounds.first != saBounds.second)
    {
        uint32 zone, area;
        target->GetZoneAndAreaId(zone, area);

        for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        {
            // some auras remove at aura remove
            if (!itr->second->IsFitToRequirements((Player*)target, zone, area))
                target->RemoveAurasDueToSpell(itr->second->spellId);
            // some auras applied at aura apply
            else if (itr->second->autocast)
            {
                if (!target->HasAura(itr->second->spellId))
                    target->CastSpell(target, itr->second->spellId, true);
            }
        }
    }

    // handle spell_linked_spell table
    if (!onReapply)
    {
        //Phase
        if (Player* player = target->ToPlayer())
        {
            if (apply)	
                player->GetPhaseMgr().RegisterPhasingAura(GetId(), target);
            else	
                player->GetPhaseMgr().UnRegisterPhasingAura(GetId(), target);
        }

        // apply linked auras
        if (apply)
        {
            if (std::vector<SpellLinked> const* spellTriggered = sSpellMgr->GetSpellLinked(GetId() + SPELL_LINK_AURA))
            {
                for (std::vector<SpellLinked>::const_iterator itr = spellTriggered->begin(); itr != spellTriggered->end(); ++itr)
                {
                    Unit* _target = target;
                    Unit* _caster = caster;

                    if (itr->target)
                        _target = target->GetUnitForLinkedSpell(caster, target, itr->target);

                    if (itr->caster && caster)
                        _caster = caster->GetUnitForLinkedSpell(caster, target, itr->caster);

                    if(itr->hastalent)
                        if(target->HasAuraLinkedSpell(_caster, _target, itr->hastype, itr->hastalent))
                            continue;

                    if(itr->hastalent2)
                        if(target->HasAuraLinkedSpell(_caster, _target, itr->hastype2, itr->hastalent2))
                            continue;

                    if(!_target)
                        continue;

                    if (itr->effect < 0)
                    {
                        switch (itr->actiontype)
                        {
                            case LINK_ACTION_DEFAULT:
                                _target->ApplySpellImmune(GetId(), IMMUNITY_ID, -(itr->effect), true);
                                break;
                            case LINK_ACTION_LEARN:
                            {
                                if(Player* _lplayer = _target->ToPlayer())
                                    _lplayer->removeSpell(abs(itr->effect));
                                break;
                            }
                            case LINK_ACTION_AURATYPE:
                                _target->RemoveAurasByType(AuraType(itr->hastalent2));
                                break;
                        }
                    }
                    else if (_caster)
                    {
                        if(itr->chance != 0 && !roll_chance_i(itr->chance))
                            continue;
                        if(itr->cooldown != 0 && _target->GetTypeId() == TYPEID_PLAYER && _target->ToPlayer()->HasSpellCooldown(itr->effect))
                            continue;

                        switch (itr->actiontype)
                        {
                            case LINK_ACTION_DEFAULT:
                                _caster->AddAura(itr->effect, _target);
                                break;
                            case LINK_ACTION_LEARN:
                            {
                                if(Player* _lplayer = _caster->ToPlayer())
                                    _lplayer->learnSpell(itr->effect, false);
                                break;
                            }
                            case LINK_ACTION_CASTINAURA:
                                _caster->CastSpell(_target, itr->effect, true, NULL, NULL, GetCasterGUID());
                                break;
                        }

                        if(itr->cooldown != 0 && _target->GetTypeId() == TYPEID_PLAYER)
                            _target->ToPlayer()->AddSpellCooldown(itr->effect, 0, getPreciseTime() + (double)itr->cooldown);
                    }
                }
            }
            if (std::vector<SpellLinked> const* spellTriggered = sSpellMgr->GetSpellLinked(GetId() + SPELL_LINK_AURA_HIT))
            {
                for (std::vector<SpellLinked>::const_iterator itr = spellTriggered->begin(); itr != spellTriggered->end(); ++itr)
                {
                    Unit* _target = target;
                    Unit* _caster = caster;

                    if (itr->target)
                        _target = target->GetUnitForLinkedSpell(caster, target, itr->target);

                    if (itr->caster && caster)
                        _caster = caster->GetUnitForLinkedSpell(caster, target, itr->caster);

                    if(itr->hastalent)
                        if(target->HasAuraLinkedSpell(_caster, _target, itr->hastype, itr->hastalent))
                            continue;

                    if(itr->hastalent2)
                        if(target->HasAuraLinkedSpell(_caster, _target, itr->hastype2, itr->hastalent2))
                            continue;
                    if(!_target)
                        continue;

                    if (itr->effect < 0)
                    {
                        switch (itr->actiontype)
                        {
                            case LINK_ACTION_DEFAULT:
                                _target->RemoveAurasDueToSpell(-(itr->effect));
                                break;
                            case LINK_ACTION_LEARN:
                            {
                                if(Player* _lplayer = _target->ToPlayer())
                                    _lplayer->removeSpell(abs(itr->effect));
                                break;
                            }
                            case LINK_ACTION_AURATYPE:
                                _target->RemoveAurasByType(AuraType(itr->hastalent2));
                                break;
                        }
                    }
                    else if (_caster)
                    {
                        if(itr->chance != 0 && !roll_chance_i(itr->chance))
                            continue;
                        if(itr->cooldown != 0 && _target->GetTypeId() == TYPEID_PLAYER && _target->ToPlayer()->HasSpellCooldown(itr->effect))
                            continue;

                        switch (itr->actiontype)
                        {
                            case LINK_ACTION_DEFAULT:
                                _caster->AddAura(itr->effect, _target);
                                break;
                            case LINK_ACTION_LEARN:
                            {
                                if(Player* _lplayer = _caster->ToPlayer())
                                    _lplayer->learnSpell(itr->effect, false);
                                break;
                            }
                            case LINK_ACTION_CASTINAURA:
                                _caster->CastSpell(_target, itr->effect, true, NULL, NULL, GetCasterGUID());
                                break;
                        }

                        if(itr->cooldown != 0 && _target->GetTypeId() == TYPEID_PLAYER)
                            _target->ToPlayer()->AddSpellCooldown(itr->effect, 0, getPreciseTime() + (double)itr->cooldown);
                    }
                }
            }
        }
        else
        {
            bool loginOut = false;

            if (caster)
                if (!caster->IsInWorld())
                    loginOut = true;
            // remove linked auras
            if (!loginOut)
            {
                if (std::vector<SpellLinked> const* spellTriggered = sSpellMgr->GetSpellLinked(-(int32)GetId()))
                {
                    for (std::vector<SpellLinked>::const_iterator itr = spellTriggered->begin(); itr != spellTriggered->end(); ++itr)
                    {
                        Unit* _target = target;
                        Unit* _caster = target;

                        if (itr->target)
                            _target = target->GetUnitForLinkedSpell(caster, target, itr->target);

                        if (itr->caster && caster)
                            _caster = caster->GetUnitForLinkedSpell(caster, target, itr->caster);

                        if(itr->hastalent)
                            if(target->HasAuraLinkedSpell(_caster, _target, itr->hastype, itr->hastalent))
                                continue;

                        if(itr->hastalent2)
                            if(target->HasAuraLinkedSpell(_caster, _target, itr->hastype2, itr->hastalent2))
                                continue;

                        if(!_target)
                            continue;

                        if (itr->effect < 0)
                        {
                            switch (itr->actiontype)
                            {
                                case LINK_ACTION_DEFAULT:
                                    _target->RemoveAurasDueToSpell(-(itr->effect));
                                    break;
                                case LINK_ACTION_LEARN:
                                {
                                    if(Player* _lplayer = _target->ToPlayer())
                                        _lplayer->removeSpell(abs(itr->effect));
                                    break;
                                }
                                case LINK_ACTION_AURATYPE:
                                    _target->RemoveAurasByType(AuraType(itr->hastalent2));
                                    break;
                            }
                        }
                        else if (removeMode != AURA_REMOVE_BY_DEATH)
                        {
                            if(itr->removeMask && !(itr->removeMask & (1 << removeMode)))
                                continue;

                            if(itr->chance != 0 && !roll_chance_i(itr->chance))
                                continue;
                            if(itr->cooldown != 0 && _target->GetTypeId() == TYPEID_PLAYER && _target->ToPlayer()->HasSpellCooldown(itr->effect))
                                continue;

                            switch (itr->actiontype)
                            {
                                case LINK_ACTION_DEFAULT:
                                    if(_caster)
                                        _caster->CastSpell(_target, itr->effect, true, NULL, NULL, GetCasterGUID());
                                    break;
                                case LINK_ACTION_LEARN:
                                {
                                    if(Player* _lplayer = _target->ToPlayer())
                                        _lplayer->learnSpell(itr->effect, false);
                                    break;
                                }
                                case LINK_ACTION_CASTINAURA:
                                    _caster->CastSpell(_target, itr->effect, true, NULL, NULL, GetCasterGUID());
                                    break;
                            }

                            if(itr->cooldown != 0 && _target->GetTypeId() == TYPEID_PLAYER)
                                _target->ToPlayer()->AddSpellCooldown(itr->effect, 0, getPreciseTime() + (double)itr->cooldown);
                        }
                    }
                }
                if (std::vector<SpellLinked> const* spellTriggered = sSpellMgr->GetSpellLinked(GetId() + SPELL_LINK_AURA))
                {
                    for (std::vector<SpellLinked>::const_iterator itr = spellTriggered->begin(); itr != spellTriggered->end(); ++itr)
                    {
                        Unit* _target = target;

                        if (itr->target)
                            _target = target->GetUnitForLinkedSpell(caster, target, itr->target);

                        if(!_target)
                            continue;

                        if (itr->effect < 0)
                        {
                            switch (itr->actiontype)
                            {
                                case LINK_ACTION_DEFAULT:
                                    _target->ApplySpellImmune(GetId(), IMMUNITY_ID, -(itr->effect), false);
                                    break;
                                case LINK_ACTION_LEARN:
                                {
                                    if(Player* _lplayer = _target->ToPlayer())
                                        _lplayer->removeSpell(abs(itr->effect));
                                    break;
                                }
                                case LINK_ACTION_AURATYPE:
                                    _target->RemoveAurasByType(AuraType(itr->hastalent2));
                                    break;
                            }
                        }
                        else
                            _target->RemoveAura(itr->effect, GetCasterGUID(), 0, removeMode);
                    }
                }
            }
        }
    }
    else if (apply)
    {
        // modify stack amount of linked auras
        if (std::vector<SpellLinked> const* spellTriggered = sSpellMgr->GetSpellLinked(GetId() + SPELL_LINK_AURA))
        {
            for (std::vector<SpellLinked>::const_iterator itr = spellTriggered->begin(); itr != spellTriggered->end(); ++itr)
                if (itr->effect > 0)
                {
                    if(itr->hastalent)
                        if(target->HasAuraLinkedSpell(caster, target, itr->hastype, itr->hastalent))
                            continue;

                    if(itr->hastalent2)
                        if(target->HasAuraLinkedSpell(caster, target, itr->hastype2, itr->hastalent2))
                            continue;

                    if(itr->chance != 0 && !roll_chance_i(itr->chance))
                        continue;
                    if(itr->cooldown != 0 && target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->HasSpellCooldown(itr->effect))
                        continue;

                    switch (itr->actiontype)
                    {
                        case LINK_ACTION_DEFAULT:
                        {
                            if (Aura* triggeredAura = target->GetAura(itr->effect, GetCasterGUID()))
                                triggeredAura->ModStackAmount(GetStackAmount() - triggeredAura->GetStackAmount());
                            break;
                        }
                        case LINK_ACTION_LEARN:
                        {
                            if(Player* _lplayer = target->ToPlayer())
                                _lplayer->learnSpell(itr->effect, false);
                            break;
                        }
                    }

                    if(itr->cooldown != 0 && target->GetTypeId() == TYPEID_PLAYER)
                        target->ToPlayer()->AddSpellCooldown(itr->effect, 0, getPreciseTime() + (double)itr->cooldown);
                }
        }
    }

    // mods at aura apply
    if (apply)
    {
        switch (GetSpellInfo()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch (GetId())
                {
                    case 32474: // Buffeting Winds of Susurrus
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            target->ToPlayer()->ActivateTaxiPathTo(506, GetId());
                        break;
                    case 33572: // Gronn Lord's Grasp, becomes stoned
                        if (GetStackAmount() >= 5 && !target->HasAura(33652))
                            target->CastSpell(target, 33652, true);
                        break;
                    case 50836: //Petrifying Grip, becomes stoned
                        if (GetStackAmount() >= 5 && !target->HasAura(50812))
                            target->CastSpell(target, 50812, true);
                        break;
                    case 60970: // Heroic Fury (remove Intercept cooldown)
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            target->ToPlayer()->RemoveSpellCooldown(20252, true);
                        break;
                }
                break;
            case SPELLFAMILY_MAGE:
                if (!caster)
                    break;
                // Todo: This should be moved to similar function in spell::hit
                if (GetSpellInfo()->SpellFamilyFlags[0] & 0x01000000)
                {
                    // Polymorph Sound - Sheep && Penguin
                    if (GetSpellInfo()->SpellIconID == 82 && GetSpellInfo()->SpellVisual[0] == 12978)
                    {
                        // Glyph of the Penguin
                        if (caster->HasAura(52648))
                            caster->CastSpell(target, 61635, true);
                        else
                            caster->CastSpell(target, 61634, true);
                    }
                }
                switch (GetId())
                {
                    case 12536: // Clearcasting
                    case 12043: // Presence of Mind
                        // Arcane Potency
                        if (AuraEffect const* aurEff = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_MAGE, 2120, 0))
                        {
                            uint32 spellId = 0;

                            switch (aurEff->GetId())
                            {
                                case 31571: spellId = 57529; break;
                                case 31572: spellId = 57531; break;
                                default:
                                    sLog->outError(LOG_FILTER_SPELLS_AURAS, "Aura::HandleAuraSpecificMods: Unknown rank of Arcane Potency (%d) found", aurEff->GetId());
                            }
                            if (spellId)
                                caster->CastSpell(caster, spellId, true);
                        }
                        break;
                    default:
                        break;
                }
                break;
            case SPELLFAMILY_PRIEST:
                if (!caster)
                    break;
                // Devouring Plague
                if (GetSpellInfo()->SpellFamilyFlags[0] & 0x02000000 && GetEffect(0))
                {
                    // Improved Devouring Plague
                    if (AuraEffect const* aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 3790, 0))
                    {
                        uint32 damage = caster->SpellDamageBonusDone(target, GetSpellInfo(), GetEffect(0)->GetAmount(), DOT, EFFECT_0);
                        damage = target->SpellDamageBonusTaken(caster, GetSpellInfo(), damage);
                        int32 basepoints0 = aurEff->GetAmount() * GetEffect(0)->GetTotalTicks() * int32(damage) / 100;
                        int32 heal = int32(CalculatePct(basepoints0, 15));

                        caster->CastCustomSpell(target, 63675, &basepoints0, NULL, NULL, true, NULL, GetEffect(0));
                        caster->CastCustomSpell(caster, 75999, &heal, NULL, NULL, true, NULL, GetEffect(0));
                    }
                }
                break;
            case SPELLFAMILY_PALADIN:
                if (!caster)
                    break;

                switch (m_spellInfo->Id)
                {
                    case 85416: // Grand Crusader
                        caster->ToPlayer()->RemoveSpellCooldown(31935, true);
                        break;
                    default:
                        break;
                }
                break;
            case SPELLFAMILY_MONK:
            {
                if (!caster)
                    break;

                switch (m_spellInfo->Id)
                {
                    case 115078: // Cap
                    {
                        if (target->isInBack(caster))
                        {
                            int32 duration = GetMaxDuration();
                            AddPct(duration, m_spellInfo->Effects[EFFECT_1].BasePoints);
                            SetMaxDuration(duration);
                            SetDuration(duration);
                        }
                        break;
                    }
                    case 126060: // Desperate Measures
                    {
                        if (Player * monk = caster->ToPlayer())
                        {
                            monk->RemoveSpellCooldown(115072, true);
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Sprint (skip non player casted spells by category)
                if (GetSpellInfo()->SpellFamilyFlags[0] & 0x40 && GetSpellInfo()->Category == 44)
                {
                    // in official maybe there is only one icon?
                    if (target->HasAura(58039)) // Glyph of Blurred Speed
                        target->CastSpell(target, 61922, true); // Sprint (waterwalk)
                }

                switch (GetId())
                {
                    case 121471: // Item - Rogue T14 4P Bonus
                    {
                        if (caster->HasAura(123122))
                        {
                            uint32 bonustime = caster->HasAura(79096) ? 6000: 12000;
                            SetDuration(GetMaxDuration() + bonustime);
                        }
                        break;
                    }
                    case 2094: // Blind
                    {
                        // Glyph of Blind
                        if (caster && caster->HasAura(91299))
                        {
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, 0, target->GetAura(32409));
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
                        }
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                switch (GetId())
                {
                    case 19503: // Scatter Shot
                    case 3355: // Freezing Trap
                    {
                        // Glyph of Solace
                        if (caster && caster->HasAura(119407))
                        {
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, 0, target->GetAura(32409));
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
    // mods at aura remove
    else
    {
        switch (GetSpellInfo()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch (GetId())
                {
                    case 72368: // Shared Suffering
                    case 72369:
                        if (caster)
                        {
                            if (AuraEffect* aurEff = GetEffect(0))
                            {
                                int32 remainingDamage = aurEff->GetAmount() * (aurEff->GetTotalTicks() - aurEff->GetTickNumber());
                                if (remainingDamage > 0)
                                    caster->CastCustomSpell(caster, 72373, NULL, &remainingDamage, NULL, true);
                            }
                        }
                        break;
                    case 49440: // Racer Slam, Slamming
                        if (Creature* racerBunny = target->FindNearestCreature(27674, 25.0f))
                            target->CastSpell(racerBunny, 49302, false);
                        break;
                }
                break;
            case SPELLFAMILY_MAGE:
                switch (GetId())
                {
                    case 66: // Invisibility
                        if (removeMode != AURA_REMOVE_BY_EXPIRE)
                            break;
                        target->CastSpell(target, 32612, true, NULL, GetEffect(1));
                        target->CombatStop();
                        break;
                    default:
                        break;
                }
                if (!caster)
                    break;
                break;
            case SPELLFAMILY_WARLOCK:
                if (!caster)
                    break;
                // Improved Fear
                if (GetSpellInfo()->SpellFamilyFlags[1] & 0x00000400)
                {
                    if (AuraEffect* aurEff = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_WARLOCK, 98, 0))
                    {
                        uint32 spellId = 0;
                        switch (aurEff->GetId())
                        {
                            case 53759: spellId = 60947; break;
                            case 53754: spellId = 60946; break;
                            default:
                                sLog->outError(LOG_FILTER_SPELLS_AURAS, "Aura::HandleAuraSpecificMods: Unknown rank of Improved Fear (%d) found", aurEff->GetId());
                        }
                        if (spellId)
                            caster->CastSpell(target, spellId, true);
                    }
                }
                break;
            case SPELLFAMILY_PRIEST:
                if (!caster)
                    break;
                break;
            case SPELLFAMILY_ROGUE:
            {
                switch (GetId())
                {
                    case 137619: // Marked for Death
                    {
                        if (removeMode != AURA_REMOVE_BY_DEATH)
                            break;

                        if (!caster)
                            break;

                        if (Player* rogue = caster->ToPlayer())
                        {
                            rogue->RemoveSpellCooldown(137619, true);
                        }
                        break;
                    }
                    case 89775:  // Hemorrhage
                    case 703:    // Garrote
                    case 1943:   // Rupture
                    case 121411: // Crimson Tempest
                    {
                        if (!caster)
                            break;

                        if (caster->HasAura(79147)) // Sanguinary Vein
                        {
                            bool hasbleed = false;

                            if      (target->HasAura(703,    m_casterGuid)) hasbleed = true;
                            else if (target->HasAura(1943,   m_casterGuid)) hasbleed = true;
                            else if (target->HasAura(121411, m_casterGuid)) hasbleed = true;
                            else if (caster->HasAura(146631))
                                 if (target->HasAura(89775,  m_casterGuid)) hasbleed = true;

                            if (!hasbleed) target->RemoveAura(124271, m_casterGuid);
                        }
                        break;
                    }
                    case 11327: // Vanish
                    {
                        uint32 spellid = caster->HasAura(108208) ? 115191: 1784;

                        if (Player* player =  caster->ToPlayer())
                        {
                            player->RemoveSpellCooldown(spellid, false);
                            caster->CastSpell(caster, spellid, true);
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                switch (GetId())
                {
                    case 56835: // Reaping
                    case 50034: // Blood Rites
                    {
                        if (Player* plr = caster->ToPlayer())
                            plr->RestoreAllBaseRunes();
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
                // Glyph of Freezing Trap
                if (GetSpellInfo()->SpellFamilyFlags[0] & 0x00000008)
                    if (caster && caster->HasAura(56845))
                        target->CastSpell(target, 61394, true);
                
                // Glyph of Misdirection
                if(caster && caster->ToPlayer())
                if (GetId() == 34477 && caster->ToPlayer()->GetPetGUID() == caster->GetMisdirectionTargetGuid() && caster->HasAura(56829))
                {
                    caster->ToPlayer()->RemoveSpellCooldown(34477, true);
                }
                break;
        }
    }

    // mods at aura apply or remove
    switch (GetSpellInfo()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (GetId())
            {
                case 84745: //  GreenBuff
                case 84746: // YellowBuff
                case 84747: //    RedBuff
                {
                    if (apply && caster)
                    {
                        switch (m_spellInfo->Id)
                        {
                            case 84745: caster->insightCount  = 0; break;
                            case 84746: caster->RemoveAura(84745); break;
                            case 84747: caster->RemoveAura(84746); break;
                        }
                    }
                    else if (caster)
                        caster->insightCount = 0;
                    break;
                }
                case 50720: // Vigilance
                    if (apply)
                        target->CastSpell(caster, 59665, true, 0, NULL, caster->GetGUID());
                    else
                        target->SetReducedThreatPercent(0, 0);
                    break;
            }
            break;
        case SPELLFAMILY_DRUID:
            // Enrage
            if ((GetSpellInfo()->SpellFamilyFlags[0] & 0x80000) && GetSpellInfo()->SpellIconID == 961)
            {
                if (target->HasAura(70726)) // Item - Druid T10 Feral 4P Bonus
                    if (apply)
                        target->CastSpell(target, 70725, true);
                break;
            }
            break;
        case SPELLFAMILY_ROGUE:
        {
            switch (GetId())
            {
                case 1784:   // Stealth
                case 11327:  // Vanish
                case 115191: // Subterfuge (Stealth)
                {
                    if (caster && caster->HasAura(108209)) // Shadow Focus
                    {
                        if (apply)
                        {
                            caster->CastSpell(caster, 112942, true);
                        }
                        else
                        {
                            if (!caster->HasAura(1784) && !caster->HasAura(11327))
                            {
                                caster->RemoveAura(112942);
                            }
                        }
                    }

                    if (caster && caster->HasAura(31223)) // Master of subtlety
                    {
                        if (apply)
                        {
                            caster->CastSpell(caster, 31665, true);
                        }
                        else
                        {
                            if (!caster->HasAura(1784) && !caster->HasAura(11327) && !caster->HasAura(115191))
                            {
                                if (Aura * aur = caster->GetAura(31665))
                                {
                                    aur->SetAuraTimer(6000);
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
            switch (GetId())
            {
                case 19574: // Bestial Wrath
                    // The Beast Within cast on owner if talent present
                    if (Unit* owner = target->GetOwner())
                    {
                        // Search talent
                        if (owner->HasAura(34692))
                        {
                            if (apply)
                                owner->CastSpell(owner, 34471, true, 0, GetEffect(0));
                            else
                                owner->RemoveAurasDueToSpell(34471);
                        }
                    }
                    break;
                case 56453: // test
                        caster->ToPlayer()->RemoveSpellCooldown(53301, true);
                    break;
            }
            break;
        case SPELLFAMILY_MONK:
        {
            switch (GetId())
            {
                case 116740: // Tigereye Brew
                case 120273: // Tiger Strikes
                {
                    if (Creature* crt = target->ToCreature())
                        if (CreatureAI* ai = crt->AI())
                            ai->RecalcStats();
                    break;
                }
                case 125195: // Tigereye Brew
                {
                    if (!caster)
                        return;
                    if (apply)
                    {
                        if (GetStackAmount() >= 10)
                            caster->AddAura(137591, caster);
                    }
                    else
                    {
                        caster->RemoveAura(137591);
                    }
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
            switch (GetId())
            {
                case 31821:
                    // Aura Mastery Triggered Spell Handler
                    // If apply Concentration Aura -> trigger -> apply Aura Mastery Immunity
                    // If remove Concentration Aura -> trigger -> remove Aura Mastery Immunity
                    // If remove Aura Mastery -> trigger -> remove Aura Mastery Immunity
                    // Do effects only on aura owner
                    if (GetCasterGUID() != target->GetGUID())
                        break;

                    if (apply)
                    {
                        if ((GetSpellInfo()->Id == 31821 && target->HasAura(19746, GetCasterGUID())) || (GetSpellInfo()->Id == 19746 && target->HasAura(31821)))
                            target->CastSpell(target, 64364, true);
                    }
                    else
                        target->RemoveAurasDueToSpell(64364, GetCasterGUID());
                    break;
                case 31842: // Divine Favor
                    // Item - Paladin T10 Holy 2P Bonus
                    if (target->HasAura(70755))
                    {
                        if (apply)
                            target->CastSpell(target, 71166, true);
                        else
                            target->RemoveAurasDueToSpell(71166);
                    }
                    break;
            }
            break;
    }
}

bool Aura::CanBeAppliedOn(Unit* target)
{
    // unit not in world or during remove from world
    if (!target->IsInWorld() || target->IsDuringRemoveFromWorld())
    {
        // area auras mustn't be applied
        if (GetOwner() != target)
            return false;
        // not selfcasted single target auras mustn't be applied
        if (GetCasterGUID() != GetOwner()->GetGUID() && GetSpellInfo()->IsSingleTarget(GetCaster()))
            return false;
        return true;
    }
    else
        return CheckAreaTarget(target);
}

bool Aura::CheckAreaTarget(Unit* target)
{
    return CallScriptCheckAreaTargetHandlers(target);
}

bool Aura::CanStackWith(Aura const* existingAura) const
{
    if(!existingAura)
        return false;

    // Can stack with self
    if (this == existingAura)
        return true;

    // Dynobj auras always stack
    if (existingAura->GetType() == DYNOBJ_AURA_TYPE)
        return true;

    SpellInfo const* existingSpellInfo = existingAura->GetSpellInfo();
    bool sameCaster = GetCasterGUID() == existingAura->GetCasterGUID();

    // passive auras don't stack with another rank of the spell cast by same caster
    if (IsPassive() && sameCaster && m_spellInfo->IsDifferentRankOf(existingSpellInfo))
        return false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // prevent remove triggering aura by triggered aura
        if (existingSpellInfo->Effects[i].TriggerSpell == GetId()
            // prevent remove triggered aura by triggering aura refresh
            || m_spellInfo->Effects[i].TriggerSpell == existingAura->GetId())
            return true;
    }

    // check spell specific stack rules
    if (m_spellInfo->IsAuraExclusiveBySpecificWith(existingSpellInfo, sameCaster)
        || (sameCaster && m_spellInfo->IsAuraExclusiveBySpecificPerCasterWith(existingSpellInfo)))
        return false;

    // check spell group stack rules
    SpellGroupStackRule stackRule = sSpellMgr->CheckSpellGroupStackRules(m_spellInfo, existingSpellInfo);
    if (stackRule)
    {
        if (stackRule == SPELL_GROUP_STACK_RULE_EXCLUSIVE)
            return false;
        if (sameCaster && stackRule == SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER)
            return false;
    }

    if (m_spellInfo->SpellFamilyName != existingSpellInfo->SpellFamilyName)
        return true;

    if (!sameCaster)
    {
        // Channeled auras can stack if not forbidden by db or aura type
        if (existingAura->GetSpellInfo()->IsChanneled())
            return true;

        if (m_spellInfo->AttributesEx3 & SPELL_ATTR3_STACK_FOR_DIFF_CASTERS)
            return true;

        // check same periodic auras
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            switch (m_spellInfo->Effects[i].ApplyAuraName)
            {
                // DOT or HOT or frame from different casters will stack
                case SPELL_AURA_PERIODIC_DAMAGE:
                case SPELL_AURA_PERIODIC_DUMMY:
                case SPELL_AURA_PERIODIC_HEAL:
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
                case SPELL_AURA_PERIODIC_ENERGIZE:
                case SPELL_AURA_PERIODIC_MANA_LEECH:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_POWER_BURN:
                case SPELL_AURA_OBS_MOD_POWER:
                case SPELL_AURA_OBS_MOD_HEALTH:
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
                    // periodic auras which target areas are not allowed to stack this way (replenishment for example)
                    if (m_spellInfo->Effects[i].IsTargetingArea() || existingSpellInfo->Effects[i].IsTargetingArea())
                        break;
                    return true;
                case SPELL_AURA_ENABLE_BOSS1_UNIT_FRAME:
                    return true;
                default:
                    break;
            }
        }

        if (!m_spellInfo->_IsPositiveSpell() && !existingSpellInfo->_IsPositiveSpell())
            return true;
    }

    if (HasEffectType(SPELL_AURA_CONTROL_VEHICLE) && existingAura->HasEffectType(SPELL_AURA_CONTROL_VEHICLE))
    {
        Vehicle* veh = NULL;
        if (GetOwner()->ToUnit())
            veh = GetOwner()->ToUnit()->GetVehicleKit();

        if (!veh)           // We should probably just let it stack. Vehicle system will prevent undefined behaviour later
            return true;

        if (!veh->GetAvailableSeatCount())
            return false;   // No empty seat available

        return true; // Empty seat available (skip rest)
    }

    // spell of same spell rank chain
    if (m_spellInfo->IsRankOf(existingSpellInfo))
    {
        // don't allow passive area auras to stack
        if (m_spellInfo->IsMultiSlotAura() && !IsArea())
            return true;
        if (GetCastItemGUID() && existingAura->GetCastItemGUID())
            if (GetCastItemGUID() != existingAura->GetCastItemGUID() && (m_spellInfo->AttributesCu & SPELL_ATTR0_CU_ENCHANT_PROC))
                return true;
        // same spell with same caster should not stack
        return false;
    }

    return true;
}

bool Aura::IsProcOnCooldown() const
{
    /*if (m_procCooldown)
    {
        if (m_procCooldown > time(NULL))
            return true;
    }*/
    return false;
}

void Aura::AddProcCooldown(uint32 /*msec*/)
{
    //m_procCooldown = time(NULL) + msec;
}

void Aura::PrepareProcToTrigger(AuraApplication* aurApp, ProcEventInfo& eventInfo)
{
    // take one charge, aura expiration will be handled in Aura::TriggerProcOnEvent (if needed)
    if (IsUsingCharges())
    {
        --m_procCharges;
        SetNeedClientUpdateForTargets();
    }

    SpellProcEntry const* procEntry = sSpellMgr->GetSpellProcEntry(GetId());

    ASSERT(procEntry);

    // cooldowns should be added to the whole aura (see 51698 area aura)
    AddProcCooldown(procEntry->cooldown);
}

bool Aura::IsProcTriggeredOnEvent(AuraApplication* aurApp, ProcEventInfo& eventInfo) const
{
    SpellProcEntry const* procEntry = sSpellMgr->GetSpellProcEntry(GetId());
    // only auras with spell proc entry can trigger proc
    if (!procEntry)
        return false;

    // check if we have charges to proc with
    if (IsUsingCharges() && !GetCharges())
        return false;

    // check proc cooldown
    if (IsProcOnCooldown())
        return false;

    // TODO:
    // something about triggered spells triggering, and add extra attack effect

    // do checks against db data
    if (!sSpellMgr->CanSpellTriggerProcOnEvent(*procEntry, eventInfo))
        return false;

    // do checks using conditions table
    ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL_PROC, GetId());
    ConditionSourceInfo condInfo = ConditionSourceInfo(eventInfo.GetActor(), eventInfo.GetActionTarget());
    if (!sConditionMgr->IsObjectMeetToConditions(condInfo, conditions))
        return false;

    // TODO:
    // do allow additional requirements for procs
    // this is needed because this is the last moment in which you can prevent aura charge drop on proc
    // and possibly a way to prevent default checks (if there're going to be any)

    // Check if current equipment meets aura requirements
    // do that only for passive spells
    // TODO: this needs to be unified for all kinds of auras
    Unit* target = aurApp->GetTarget();
    if (IsPassive() && target->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetSpellInfo()->EquippedItemClass == ITEM_CLASS_WEAPON)
        {
            if (target->ToPlayer()->IsInFeralForm())
                return false;

            if (eventInfo.GetDamageInfo())
            {
                WeaponAttackType attType = eventInfo.GetDamageInfo()->GetAttackType();
                Item* item = NULL;
                if (attType == BASE_ATTACK)
                    item = target->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
                else if (attType == OFF_ATTACK)
                    item = target->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

                if (!item || item->IsBroken() || item->GetTemplate()->Class != ITEM_CLASS_WEAPON || !((1<<item->GetTemplate()->SubClass) & GetSpellInfo()->EquippedItemSubClassMask))
                    return false;
            }
        }
        else if (GetSpellInfo()->EquippedItemClass == ITEM_CLASS_ARMOR)
        {
            // Check if player is wearing shield
            Item* item = target->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if (!item || item->IsBroken() || item->GetTemplate()->Class != ITEM_CLASS_ARMOR || !((1<<item->GetTemplate()->SubClass) & GetSpellInfo()->EquippedItemSubClassMask))
                return false;
        }
    }

    return roll_chance_f(CalcProcChance(*procEntry, eventInfo));
}

float Aura::CalcProcChance(SpellProcEntry const& procEntry, ProcEventInfo& eventInfo) const
{
    float chance = procEntry.chance;
    // calculate chances depending on unit with caster's data
    // so talents modifying chances and judgements will have properly calculated proc chance
    if (Unit* caster = GetCaster())
    {
        // calculate ppm chance if present and we're using weapon
        if (eventInfo.GetDamageInfo() && procEntry.ratePerMinute != 0)
        {
            uint32 WeaponSpeed = caster->GetAttackTime(eventInfo.GetDamageInfo()->GetAttackType());
            chance = caster->GetPPMProcChance(WeaponSpeed, procEntry.ratePerMinute, GetSpellInfo());
        }
        // apply chance modifer aura, applies also to ppm chance (see improved judgement of light spell)
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_CHANCE_OF_SUCCESS, chance);
    }
    return chance;
}

void Aura::TriggerProcOnEvent(AuraApplication* aurApp, ProcEventInfo& eventInfo)
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (aurApp->HasEffect(i))
            // OnEffectProc / AfterEffectProc hooks handled in AuraEffect::HandleProc()
            GetEffect(i)->HandleProc(aurApp, eventInfo, (SpellEffIndex) i);

    // Remove aura if we've used last charge to proc
    if (IsUsingCharges() && !GetCharges())
        Remove();
}

void Aura::_DeleteRemovedApplications()
{
    while (!m_removedApplications.empty())
    {
        delete m_removedApplications.front();
        m_removedApplications.pop_front();
    }
}

void Aura::LoadScripts()
{
    sScriptMgr->CreateAuraScripts(m_spellInfo->Id, m_loadedScripts);
    for (std::list<AuraScript*>::iterator itr = m_loadedScripts.begin(); itr != m_loadedScripts.end();)
    {
        if (!(*itr)->_Load(this))
        {
            std::list<AuraScript*>::iterator bitr = itr;
            ++itr;
            delete (*bitr);
            m_loadedScripts.erase(bitr);
            continue;
        }
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura::LoadScripts: Script `%s` for aura `%u` is loaded now", (*itr)->_GetScriptName()->c_str(), m_spellInfo->Id);
        (*itr)->Register();
        ++itr;
    }
}

void Aura::CallScriptCheckTargetsListHandlers(std::list<Unit*>& unitTargets)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_CHECK_TARGETS_LIST);
        std::list<AuraScript::CheckTargetsListHandler>::iterator hookItrEnd = (*scritr)->DoCheckTargetsList.end(), hookItr = (*scritr)->DoCheckTargetsList.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr, unitTargets);
        (*scritr)->_FinishScriptCall();
    }
}

bool Aura::CallScriptCheckAreaTargetHandlers(Unit* target)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_CHECK_AREA_TARGET);
        std::list<AuraScript::CheckAreaTargetHandler>::iterator hookItrEnd = (*scritr)->DoCheckAreaTarget.end(), hookItr = (*scritr)->DoCheckAreaTarget.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            if (!(*hookItr).Call(*scritr, target))
                return false;
        (*scritr)->_FinishScriptCall();
    }
    return true;
}

void Aura::CallScriptDispel(DispelInfo* dispelInfo)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_DISPEL);
        std::list<AuraScript::AuraDispelHandler>::iterator hookItrEnd = (*scritr)->OnDispel.end(), hookItr = (*scritr)->OnDispel.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr, dispelInfo);
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptAfterDispel(DispelInfo* dispelInfo)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_AFTER_DISPEL);
        std::list<AuraScript::AuraDispelHandler>::iterator hookItrEnd = (*scritr)->AfterDispel.end(), hookItr = (*scritr)->AfterDispel.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr, dispelInfo);
        (*scritr)->_FinishScriptCall();
    }
}

bool Aura::CallScriptEffectApplyHandlers(AuraEffect const* aurEff, AuraApplication const* aurApp, AuraEffectHandleModes mode)
{
    bool preventDefault = false;
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_APPLY, aurApp);
        std::list<AuraScript::EffectApplyHandler>::iterator effEndItr = (*scritr)->OnEffectApply.end(), effItr = (*scritr)->OnEffectApply.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, mode);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

bool Aura::CallScriptEffectRemoveHandlers(AuraEffect const* aurEff, AuraApplication const* aurApp, AuraEffectHandleModes mode)
{
    bool preventDefault = false;
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_REMOVE, aurApp);
        std::list<AuraScript::EffectApplyHandler>::iterator effEndItr = (*scritr)->OnEffectRemove.end(), effItr = (*scritr)->OnEffectRemove.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, mode);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

void Aura::CallScriptAfterEffectApplyHandlers(AuraEffect const* aurEff, AuraApplication const* aurApp, AuraEffectHandleModes mode)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_AFTER_APPLY, aurApp);
        std::list<AuraScript::EffectApplyHandler>::iterator effEndItr = (*scritr)->AfterEffectApply.end(), effItr = (*scritr)->AfterEffectApply.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, mode);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptAfterEffectRemoveHandlers(AuraEffect const* aurEff, AuraApplication const* aurApp, AuraEffectHandleModes mode)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_AFTER_REMOVE, aurApp);
        std::list<AuraScript::EffectApplyHandler>::iterator effEndItr = (*scritr)->AfterEffectRemove.end(), effItr = (*scritr)->AfterEffectRemove.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, mode);
        }
        (*scritr)->_FinishScriptCall();
    }
}

bool Aura::CallScriptEffectPeriodicHandlers(AuraEffect const* aurEff, AuraApplication const* aurApp)
{
    bool preventDefault = false;
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_PERIODIC, aurApp);
        std::list<AuraScript::EffectPeriodicHandler>::iterator effEndItr = (*scritr)->OnEffectPeriodic.end(), effItr = (*scritr)->OnEffectPeriodic.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

void Aura::CallScriptEffectUpdateHandlers(uint32 diff, AuraEffect* aurEff)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_UPDATE);
        std::list<AuraScript::EffectUpdateHandler>::iterator effEndItr = (*scritr)->OnEffectUpdate.end(), effItr = (*scritr)->OnEffectUpdate.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, diff, aurEff);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectUpdatePeriodicHandlers(AuraEffect* aurEff)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_UPDATE_PERIODIC);
        std::list<AuraScript::EffectUpdatePeriodicHandler>::iterator effEndItr = (*scritr)->OnEffectUpdatePeriodic.end(), effItr = (*scritr)->OnEffectUpdatePeriodic.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectCalcAmountHandlers(AuraEffect const* aurEff, int32 & amount, bool & canBeRecalculated)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_AMOUNT);
        std::list<AuraScript::EffectCalcAmountHandler>::iterator effEndItr = (*scritr)->DoEffectCalcAmount.end(), effItr = (*scritr)->DoEffectCalcAmount.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, amount, canBeRecalculated);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectBeforeCalcAmountHandlers(AuraEffect const* aurEff, int32 & amount, bool & canBeRecalculated)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_AMOUNT);
        std::list<AuraScript::EffectCalcAmountHandler>::iterator effEndItr = (*scritr)->DoEffectBeforeCalcAmount.end(), effItr = (*scritr)->DoEffectBeforeCalcAmount.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, amount, canBeRecalculated);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptCalcMaxDurationHandlers(int32& maxDuration)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_CALC_MAX_DURATION);
        std::list<AuraScript::CalcMaxDurationHandler>::iterator hookItrEnd = (*scritr)->DoCalcMaxDuration.end(), hookItr = (*scritr)->DoCalcMaxDuration.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr, maxDuration);
        (*scritr)->_FinishScriptCall();
    }
    CalculateDurationFromDummy(maxDuration);
}

void Aura::CallScriptEffectChangeTickDamageHandlers(AuraEffect const* aurEff, int32 & amount, Unit* target, bool crit)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_AMOUNT);
        std::list<AuraScript::EffectChangeTickDamageHandler>::iterator effEndItr = (*scritr)->DoEffectChangeTickDamage.end(), effItr = (*scritr)->DoEffectChangeTickDamage.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, amount, target, crit);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectCalcPeriodicHandlers(AuraEffect const* aurEff, bool & isPeriodic, int32 & amplitude)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_PERIODIC);
        std::list<AuraScript::EffectCalcPeriodicHandler>::iterator effEndItr = (*scritr)->DoEffectCalcPeriodic.end(), effItr = (*scritr)->DoEffectCalcPeriodic.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, isPeriodic, amplitude);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectCalcSpellModHandlers(AuraEffect const* aurEff, SpellModifier* & spellMod)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_SPELLMOD);
        std::list<AuraScript::EffectCalcSpellModHandler>::iterator effEndItr = (*scritr)->DoEffectCalcSpellMod.end(), effItr = (*scritr)->DoEffectCalcSpellMod.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, spellMod);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectAbsorbHandlers(AuraEffect* aurEff, AuraApplication const* aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount, bool& defaultPrevented)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_ABSORB, aurApp);
        std::list<AuraScript::EffectAbsorbHandler>::iterator effEndItr = (*scritr)->OnEffectAbsorb.end(), effItr = (*scritr)->OnEffectAbsorb.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        defaultPrevented = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectAfterAbsorbHandlers(AuraEffect* aurEff, AuraApplication const* aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_AFTER_ABSORB, aurApp);
        std::list<AuraScript::EffectAbsorbHandler>::iterator effEndItr = (*scritr)->AfterEffectAbsorb.end(), effItr = (*scritr)->AfterEffectAbsorb.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectManaShieldHandlers(AuraEffect* aurEff, AuraApplication const* aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount, bool & /*defaultPrevented*/)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_MANASHIELD, aurApp);
        std::list<AuraScript::EffectManaShieldHandler>::iterator effEndItr = (*scritr)->OnEffectManaShield.end(), effItr = (*scritr)->OnEffectManaShield.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectAfterManaShieldHandlers(AuraEffect* aurEff, AuraApplication const* aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_AFTER_MANASHIELD, aurApp);
        std::list<AuraScript::EffectManaShieldHandler>::iterator effEndItr = (*scritr)->AfterEffectManaShield.end(), effItr = (*scritr)->AfterEffectManaShield.begin();
        for (; effItr != effEndItr; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::SetScriptData(uint32 type, uint32 data)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
        (*scritr)->SetData(type, data);
}

void Aura::SetScriptGuid(uint32 type, uint64 data)
{
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
        (*scritr)->SetGuid(type, data);
}

bool Aura::CallScriptEffectProcHandlers(AuraEffect const* aurEff, AuraApplication const* aurApp, ProcEventInfo& eventInfo)
{
    bool preventDefault = false;
    for (std::list<AuraScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_PROC, aurApp);
        std::list<AuraScript::EffectProcHandler>::iterator effEndItr  = (*scritr)->OnEffectProc.end(), effItr  = (*scritr)->OnEffectProc.begin();
        for (; effItr  != effEndItr; ++effItr )
        {
            if ((*effItr).IsEffectAffected(m_spellInfo, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, eventInfo);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

UnitAura::UnitAura(SpellInfo const* spellproto, uint32 effMask, WorldObject* owner, Unit* caster, int32 *baseAmount, Item* castItem, uint64 casterGUID, uint16 stackAmount)
    : Aura(spellproto, owner, caster, castItem, casterGUID, stackAmount)
{
    m_AuraDRGroup = DIMINISHING_NONE;
}

void UnitAura::_ApplyForTarget(Unit* target, Unit* caster, AuraApplication * aurApp)
{
    Aura::_ApplyForTarget(target, caster, aurApp);

    // register aura diminishing on apply
    if (DiminishingGroup group = GetDiminishGroup())
        target->ApplyDiminishingAura(group, true);
}

void UnitAura::_UnapplyForTarget(Unit* target, Unit* caster, AuraApplication * aurApp)
{
    Aura::_UnapplyForTarget(target, caster, aurApp);

    // unregister aura diminishing (and store last time)
    if (DiminishingGroup group = GetDiminishGroup())
        target->ApplyDiminishingAura(group, false);
}

void UnitAura::Remove(AuraRemoveMode removeMode)
{
    if (IsRemoved() || m_removeDelay > 0)
        return;

    if (GetSpellInfo()->AttributesCu & SPELL_ATTR0_CU_REMOVE_AFTER_DELAY)
    {
        m_removeDelay = 1;
        return;
    }

    GetUnitOwner()->RemoveOwnedAura(this, removeMode);
}

void UnitAura::FillTargetMap(std::map<Unit*, uint32> & targets, Unit* caster)
{
    for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
    {
        if (!HasEffect(effIndex))
            continue;
        UnitList targetList;
        // non-area aura
        if (GetSpellInfo()->Effects[effIndex].Effect == SPELL_EFFECT_APPLY_AURA)
        {
            targetList.push_back(GetUnitOwner());
        }
        else
        {
            float radius = GetSpellInfo()->Effects[effIndex].CalcRadius(caster);

            if (!GetUnitOwner()->HasUnitState(UNIT_STATE_ISOLATED))
            {
                switch (GetSpellInfo()->Effects[effIndex].Effect)
                {
                    case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                    case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
                    {
                        targetList.push_back(GetUnitOwner());
                        Trinity::AnyGroupedUnitInObjectRangeCheck u_check(GetUnitOwner(), GetUnitOwner(), radius, GetSpellInfo()->Effects[effIndex].Effect == SPELL_EFFECT_APPLY_AREA_AURA_RAID);
                        Trinity::UnitListSearcher<Trinity::AnyGroupedUnitInObjectRangeCheck> searcher(GetUnitOwner(), targetList, u_check);
                        GetUnitOwner()->VisitNearbyObject(radius, searcher);
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
                    {
                        targetList.push_back(GetUnitOwner());
                        Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(GetUnitOwner(), GetUnitOwner(), radius);
                        Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(GetUnitOwner(), targetList, u_check);
                        GetUnitOwner()->VisitNearbyObject(radius, searcher);
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
                    {
                        Trinity::AnyAoETargetUnitInObjectRangeCheck u_check(GetUnitOwner(), GetUnitOwner(), radius); // No GetCharmer in searcher
                        Trinity::UnitListSearcher<Trinity::AnyAoETargetUnitInObjectRangeCheck> searcher(GetUnitOwner(), targetList, u_check);
                        if (GetUnitOwner()->isAlive())
                            GetUnitOwner()->VisitNearbyObject(radius, searcher);
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AREA_AURA_PET:
                    {
                        if (GetUnitOwner()->isAlive())
                            targetList.push_back(GetUnitOwner());
                        if (Unit* owner = GetUnitOwner()->GetCharmerOrOwner())
                            if (GetUnitOwner()->IsWithinDistInMap(owner, radius))
                                targetList.push_back(owner);
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
                    {
                        if (Unit* owner = GetUnitOwner()->GetCharmerOrOwner())
                        {
                            if (GetUnitOwner()->IsWithinDistInMap(owner, radius))
                                targetList.push_back(owner);
                        }
                        else
                            targetList.push_back(GetUnitOwner());
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AURA_ON_PET_OR_SELF:
                    {
                        if (!(m_spellInfo->AttributesEx & SPELL_ATTR1_CANT_TARGET_SELF))
                            targetList.push_back(GetUnitOwner());
                        if (Unit* pet = GetUnitOwner()->GetGuardianPet())
                            if (GetUnitOwner()->IsWithinDistInMap(pet, radius))
                                targetList.push_back(pet);
                        break;
                    }
                }
            }
        }

        for (UnitList::iterator itr = targetList.begin(); itr!= targetList.end();++itr)
        {
            std::map<Unit*, uint32>::iterator existing = targets.find(*itr);
            if (existing != targets.end())
                existing->second |= 1<<effIndex;
            else
            {
                if (m_spellInfo->AttributesEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF)
                    if ((*itr)->GetTypeId() != TYPEID_PLAYER && (*itr) != caster)
                        if ((*itr)->GetOwner() && (*itr)->GetOwner()->GetTypeId() == TYPEID_PLAYER)
                            continue;

                targets[*itr] = 1<<effIndex;
            }
        }
    }
}

DynObjAura::DynObjAura(SpellInfo const* spellproto, uint32 effMask, WorldObject* owner, Unit* caster, int32 *baseAmount, Item* castItem, uint64 casterGUID, uint16 stackAmount)
    : Aura(spellproto, owner, caster, castItem, casterGUID, stackAmount)
{
}

void DynObjAura::Remove(AuraRemoveMode removeMode)
{
    if (IsRemoved())
        return;
    _Remove(removeMode);
}

void DynObjAura::FillTargetMap(std::map<Unit*, uint32> & targets, Unit* /*caster*/)
{
    Unit* dynObjOwnerCaster = GetDynobjOwner()->GetCaster();
    float radius = GetDynobjOwner()->GetRadius();

    for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
    {
        if (!HasEffect(effIndex))
            continue;
        UnitList targetList;
        if (GetSpellInfo()->Effects[effIndex].TargetB.GetTarget() == TARGET_DEST_DYNOBJ_ALLY
            || GetSpellInfo()->Effects[effIndex].TargetB.GetTarget() == TARGET_UNIT_DEST_AREA_ALLY)
        {
            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(GetDynobjOwner(), dynObjOwnerCaster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(GetDynobjOwner(), targetList, u_check);
            GetDynobjOwner()->VisitNearbyObject(radius, searcher);
        }
        else
        {
            Trinity::AnyAoETargetUnitInObjectRangeCheck u_check(GetDynobjOwner(), dynObjOwnerCaster, radius);
            Trinity::UnitListSearcher<Trinity::AnyAoETargetUnitInObjectRangeCheck> searcher(GetDynobjOwner(), targetList, u_check);
            GetDynobjOwner()->VisitNearbyObject(radius, searcher);
        }

        CallScriptCheckTargetsListHandlers(targetList);
        for (UnitList::iterator itr = targetList.begin(); itr!= targetList.end();++itr)
        {
            std::map<Unit*, uint32>::iterator existing = targets.find(*itr);
            if (existing != targets.end())
                existing->second |= 1<<effIndex;
            else
                targets[*itr] = 1<<effIndex;
        }
    }
}

void Aura::SetAuraTimer(int32 time, uint64 guid)
{
    if(GetDuration() == -1 || GetDuration() > time)
    {
        SetDuration(time);
        SetMaxDuration(time);
        if(AuraApplication *aur = GetApplicationOfTarget(guid ? guid : m_casterGuid))
            aur->ClientUpdate();
    }
}

uint32 Aura::CalcAgonyTickDamage(uint32 damage)
{
    uint8 stack = GetStackAmount();

    if (stack < GetSpellInfo()->StackAmount)
        if (AuraEffect* eff = GetEffect(EFFECT_0))
        {
            m_stackAmount = stack + 1;
            eff->SetAmount((eff->GetAmount() / stack) * (stack + 1));
            damage = eff->GetAmount();
            eff->SetCritAmount(damage * 2);

            if (AuraEffect* eff1 = GetEffect(EFFECT_1))
                eff1->SetAmount(damage);
        }
    return damage;
}