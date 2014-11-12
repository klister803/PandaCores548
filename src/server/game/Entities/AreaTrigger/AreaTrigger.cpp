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

#include "ObjectAccessor.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "Log.h"
#include "AreaTrigger.h"
#include "GridNotifiers.h"
#include "Chat.h"

AreaTrigger::AreaTrigger() : WorldObject(false), _duration(0), _activationDelay(0), _updateDelay(0), _on_unload(false), _caster(NULL),
    _radius(1.0f), atInfo(), _on_despawn(false), m_spellInfo(NULL), _moveSpeed(0.0f), _moveTime(0), _realEntry(0)
{
    m_objectType |= TYPEMASK_AREATRIGGER;
    m_objectTypeId = TYPEID_AREATRIGGER;

    m_updateFlag = UPDATEFLAG_STATIONARY_POSITION | UPDATEFLAG_AREA_TRIGGER;

    m_valuesCount = AREATRIGGER_END;
}

AreaTrigger::~AreaTrigger()
{
}

void AreaTrigger::AddToWorld()
{
    ///- Register the AreaTrigger for guid lookup and for caster
    if (!IsInWorld())
    {
        sObjectAccessor->AddObject(this);
        WorldObject::AddToWorld();
        BindToCaster();
    }
}

void AreaTrigger::RemoveFromWorld()
{
    ///- Remove the AreaTrigger from the accessor and from all lists of objects in world
    if (IsInWorld())
    {
        UnbindFromCaster();
        WorldObject::RemoveFromWorld();
        sObjectAccessor->RemoveObject(this);
    }
}

bool AreaTrigger::CreateAreaTrigger(uint32 guidlow, uint32 triggerEntry, Unit* caster, SpellInfo const* info, Position const& pos, Spell* spell /*=NULL*/, uint64 targetGuid /*=0*/)
{
    m_spellInfo = info;

    if (!info)
    {
        sLog->outError(LOG_FILTER_GENERAL, "AreaTrigger (entry %u) caster %s no spellInfo", triggerEntry, caster->GetString().c_str());
        return false;
    }

    // Caster not in world, might be spell triggered from aura removal
    if (!caster->IsInWorld())
        return false;

    if (!caster->isAlive())
    {
        sLog->outError(LOG_FILTER_GENERAL, "AreaTrigger (spell %u) caster %s is dead ", info->Id, caster->GetString().c_str());
        return false;
    }

    SetMap(caster->GetMap());
    Relocate(pos);
    if (!IsPositionValid())
    {
        sLog->outError(LOG_FILTER_GENERAL, "AreaTrigger (spell %u) not created. Invalid coordinates (X: %f Y: %f)", info->Id, GetPositionX(), GetPositionY());
        return false;
    }

    if (AreaTriggerInfo const* infoAt = sObjectMgr->GetAreaTriggerInfo(triggerEntry))
    {
        atInfo = *infoAt;
        _activationDelay = atInfo.activationDelay;
        _updateDelay = atInfo.updateDelay;

        for (AreaTriggerActionList::const_iterator itr = atInfo.actions.begin(); itr != atInfo.actions.end(); ++itr)
            _actionInfo[itr->id] = ActionInfo(&*itr);
    }

    WorldObject::_Create(guidlow, HIGHGUID_AREATRIGGER, caster->GetPhaseMask());
    SetPhaseId(caster->GetPhaseId(), false);

    _realEntry = triggerEntry;
    SetEntry(_realEntry);
    uint32 duration = info->GetDuration();

    Player* modOwner = caster->GetSpellModOwner();
    if (duration != -1 && modOwner)
        modOwner->ApplySpellMod(info->Id, SPELLMOD_DURATION, duration);

    SetDuration(duration);
    SetObjectScale(1);

    // on some spells radius set on dummy aura, not on create effect.
    // overwrite by radius from spell if exist.
    bool find = false;
    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        if (float r = info->Effects[j].CalcRadius(caster))
        {
            _radius = r * (spell ? spell->m_spellValue->RadiusMod : 1.0f);
            find = true;
        }

    if (!find && atInfo.radius)
        _radius = atInfo.radius;

    SetUInt64Value(AREATRIGGER_CASTER, caster->GetGUID());
    SetUInt32Value(AREATRIGGER_SPELLID, info->Id);
    SetUInt32Value(AREATRIGGER_SPELLVISUALID, info->SpellVisual[0] ? info->SpellVisual[0] : info->SpellVisual[1]);
    SetUInt32Value(AREATRIGGER_DURATION, duration);
    SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, 1);
    SetTargetGuid(targetGuid);

    // culculate destination point
    if (isMoving())
    {
        _startPosition.Relocate(pos);
        pos.SimplePosXYRelocationByAngle(_destPosition, GetSpellInfo()->GetMaxRange(), 0.0f);
        _moveSpeed = GetSpellInfo()->GetMaxRange() / duration;
    }

    FillCustiomData();

    setActive(true);

    if (!GetMap()->AddToMap(this))
        return false;

    #ifdef WIN32
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AreaTrigger::Create AreaTrigger caster %s spellID %u spell rage %f dist %f dest - X:%f,Y:%f,Z:%f", caster->GetString().c_str(), info->Id, _radius, GetSpellInfo()->GetMaxRange(), _destPosition.GetPositionX(), _destPosition.GetPositionY(), _destPosition.GetPositionZ());
    #endif

    if (atInfo.maxCount)
    {
        std::list<AreaTrigger*> oldTriggers;
        caster->GetAreaObjectList(oldTriggers, info->Id);
        oldTriggers.sort(Trinity::GuidValueSorterPred());
        while (oldTriggers.size() > atInfo.maxCount)
        {
            AreaTrigger* at = oldTriggers.front();
            oldTriggers.remove(at);
            if (at->GetCasterGUID() == caster->GetGUID())
                at->Remove(false);
        }
    }
    UpdateAffectedList(0, AT_ACTION_MOMENT_SPAWN);

    return true;
}

void AreaTrigger::FillCustiomData()
{
    //custom visual case.
    if (GetCustomVisualId())
        SetUInt32Value(AREATRIGGER_SPELLVISUALID, GetCustomVisualId());

    if (GetCustomEntry())
        SetEntry(GetCustomEntry());

    switch(GetSpellId())
    {
        case 143961:    //OO: Defiled Ground
            //ToDo: should cast only 1/4 of circle
            SetSpellId(143960);
            SetDuration(-1);
            _radius = 8.0f;
            //infrontof
            break;
        default:
            break;
    }
}

void AreaTrigger::UpdateAffectedList(uint32 p_time, AreaTriggerActionMoment actionM)
{
    if (atInfo.actions.empty())
        return;

    WorldObject const* searcher = this;
    if(uint64 targetGuid = GetTargetGuid())
        if(Unit* target = ObjectAccessor::GetUnit(*this, targetGuid))
            if(_caster->GetMap() == target->GetMap())
                searcher = target;

    if (actionM & AT_ACTION_MOMENT_ENTER)
    {
        for (std::list<uint64>::iterator itr = affectedPlayers.begin(), next; itr != affectedPlayers.end(); itr = next)
        {
            next = itr;
            ++next;

            Unit* unit = ObjectAccessor::GetUnit(*this, *itr);
            if (!unit)
            {
                affectedPlayers.erase(itr);
                continue;
            }
            
            if (!unit->IsWithinDistInMap(searcher, GetRadius()) ||
                (isMoving() && _HasActionsWithCharges(AT_ACTION_MOMENT_ON_THE_WAY) && !unit->IsInBetween(this, _destPosition.GetPositionX(), _destPosition.GetPositionY())))
            {
                affectedPlayers.erase(itr);
                AffectUnit(unit, AT_ACTION_MOMENT_LEAVE);
                continue;
            }

            UpdateOnUnit(unit, p_time);
        }

        std::list<Unit*> unitList;
        searcher->GetAttackableUnitListInRange(unitList, GetRadius());
        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
        {
            if (!IsUnitAffected((*itr)->GetGUID()))
            {
                //No 
                if (isMoving() && _HasActionsWithCharges(AT_ACTION_MOMENT_ON_THE_WAY) && !(*itr)->IsInBetween(this, _destPosition.GetPositionX(), _destPosition.GetPositionY()))
                    continue;
                affectedPlayers.push_back((*itr)->GetGUID());
                AffectUnit(*itr, actionM);
            }
        }
    }
    else
    {
        for (std::list<uint64>::iterator itr = affectedPlayers.begin(), next; itr != affectedPlayers.end(); itr = next)
        {
            next = itr;
            ++next;

            Unit* unit = ObjectAccessor::GetUnit(*this, *itr);
            if (!unit)
            {
                affectedPlayers.erase(itr);
                continue;
            }

            AffectUnit(unit, actionM);
            affectedPlayers.erase(itr);
        }
        AffectOwner(actionM);
    }
}

void AreaTrigger::UpdateActionCharges(uint32 p_time)
{
    for (ActionInfoMap::iterator itr = _actionInfo.begin(); itr != _actionInfo.end(); ++itr)
    {
        ActionInfo& info = itr->second;
        if (!info.charges || !info.action->chargeRecoveryTime)
            continue;
        if (info.charges >= info.action->maxCharges)
            continue;

        info.recoveryTime += p_time;
        if (info.recoveryTime >= info.action->chargeRecoveryTime)
        {
            info.recoveryTime -= info.action->chargeRecoveryTime;
            ++info.charges;
            if (info.charges == info.action->maxCharges)
                info.recoveryTime = 0;
        }
    }
}

void AreaTrigger::Update(uint32 p_time)
{
    //TMP. For debug info.
    uint32 spell = GetSpellId();

    if (GetDuration() != -1)
    {
        if (GetDuration() > int32(p_time))
        {
            _duration -= p_time;

            if (_activationDelay >= p_time)
                _activationDelay -= p_time;
            else
                _activationDelay = 0;
        }else
        {
            Remove(!_on_despawn); // expired
            return;
        }
    }

    UpdateActionCharges(p_time);
    UpdateMovement(p_time);

    if (!_activationDelay)
        UpdateAffectedList(p_time, AT_ACTION_MOMENT_ENTER);

    //??
    //WorldObject::Update(p_time);
}

bool AreaTrigger::IsUnitAffected(uint64 guid) const
{
    return std::find(affectedPlayers.begin(), affectedPlayers.end(), guid) != affectedPlayers.end();
}

void AreaTrigger::AffectUnit(Unit* unit, AreaTriggerActionMoment actionM)
{
    for (ActionInfoMap::iterator itr =_actionInfo.begin(); itr != _actionInfo.end(); ++itr)
    {
        ActionInfo& info = itr->second;
        if (!(info.action->moment & actionM))
            continue;

        DoAction(unit, info);
        // if(unit != _caster)
            // AffectOwner(actionM);//action if action on area trigger
    }
}

void AreaTrigger::AffectOwner(AreaTriggerActionMoment actionM)
{
    for (ActionInfoMap::iterator itr =_actionInfo.begin(); itr != _actionInfo.end(); ++itr)
    {
        ActionInfo& info = itr->second;
        if (!(info.action->targetFlags & AT_TARGET_FLAG_ALWAYS_TRIGGER))
            continue;
        if (!(info.action->moment & actionM))
            continue;

        DoAction(_caster, info);
    }
}

void AreaTrigger::UpdateOnUnit(Unit* unit, uint32 p_time)
{
    if (atInfo.updateDelay)
    {
        if (_updateDelay > p_time)
        {
            _updateDelay -= p_time;
            return;
        }
        else
            _updateDelay = atInfo.updateDelay;
    }

    for (ActionInfoMap::iterator itr =_actionInfo.begin(); itr != _actionInfo.end(); ++itr)
    {
        ActionInfo& info = itr->second;
        if (!(info.action->moment & AT_ACTION_MOMENT_UPDATE))
            continue;

        DoAction(unit, info);
    }
}

bool AreaTrigger::_HasActionsWithCharges(AreaTriggerActionMoment action /*= AT_ACTION_MOMENT_ENTER*/)
{
    for (ActionInfoMap::iterator itr =_actionInfo.begin(); itr != _actionInfo.end(); ++itr)
    {
        ActionInfo& info = itr->second;
        if (info.action->moment & action)
        {
            if (info.charges || !info.action->maxCharges)
                return true;
        }
    }
    return false;
}

void AreaTrigger::DoAction(Unit* unit, ActionInfo& action)
{
    // do not process depleted actions
    if (!action.charges && action.action->maxCharges)
        return;

    Unit* caster = _caster;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AreaTrigger::DoAction caster %s unit %s type %u spellID %u, moment %u, targetFlags %u",
    //caster->GetString().c_str(), unit->GetString().c_str(), action.action->actionType, action.action->spellId, action.action->moment, action.action->targetFlags);

    if (action.action->targetFlags & AT_TARGET_FLAG_FRIENDLY)
        if (!caster || !caster->IsFriendlyTo(unit))
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_HOSTILE)
        if (!caster || !caster->IsHostileTo(unit))
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_OWNER)
    if (unit->GetGUID() != GetCasterGUID())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_PLAYER)
        if (!unit->ToPlayer())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_PET)
        if (unit->isPet())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_FULL_HP)
        if (unit->IsFullHealth())
            return;

    if (action.action->targetFlags & AT_TARGET_FLAT_IN_FRONT)
        if (!HasInArc(static_cast<float>(M_PI), unit))
            return;

    if (!CheckActionConditions(*action.action, unit))
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(action.action->spellId);
    if (!spellInfo)
        return;

    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_FULL_ENERGY)
    {
        Powers energeType = POWER_NULL;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if(spellInfo->Effects[i].Effect == SPELL_EFFECT_ENERGIZE)
                energeType = Powers(spellInfo->Effects[i].MiscValue);
        }

        if (energeType == POWER_NULL || unit->GetMaxPower(energeType) == 0 || unit->GetMaxPower(energeType) == unit->GetPower(energeType))
            return;
    }

    // should cast on self.
    if (spellInfo->Effects[EFFECT_0].TargetA.GetTarget() == TARGET_UNIT_CASTER
        || action.action->targetFlags & AT_TARGET_FLAG_CASTER_IS_TARGET)
        caster = unit;

    switch (action.action->actionType)
    {
        case AT_ACTION_TYPE_CAST_SPELL:
        {
            if (caster)
            {
                if (action.action->targetFlags & AT_TARGET_FLAG_CAST_AT_SRC)
                    caster->CastSpell(GetPositionX(), GetPositionY(), GetPositionZ(), action.action->spellId, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_CASTED_BY_AREATRIGGER));
                else
                    caster->CastSpell(unit, action.action->spellId, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_CASTED_BY_AREATRIGGER));
            }
            break;
        }
        case AT_ACTION_TYPE_REMOVE_AURA:
        {
            unit->RemoveAura(action.action->spellId);       //only one aura should be removed.
            break;
        }
        case AT_ACTION_TYPE_ADD_STACK:
        {
            if(Aura* aura = unit->GetAura(action.action->spellId))
                aura->ModStackAmount(1);
            break;
        }
        case AT_ACTION_TYPE_REMOVE_STACK:
        {
            if(Aura* aura = unit->GetAura(action.action->spellId))
                aura->ModStackAmount(-1);
            break;
        }
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AreaTrigger::DoAction action");

    if (action.charges > 0)
    {
        --action.charges;
        //unload at next update.
        if (!action.charges && !_HasActionsWithCharges()) //_noActionsWithCharges check any action at enter.
        {
            _on_despawn = true;
            SetDuration(0);
        }
    }
}

void AreaTrigger::Remove(bool duration)
{
    if (_on_unload)
        return;
    _on_unload = true;

    if (IsInWorld())
    {
        UpdateAffectedList(0, AT_ACTION_MOMENT_REMOVE);//any remove from world

        if(duration)
            UpdateAffectedList(0, AT_ACTION_MOMENT_DESPAWN);//remove from world with time
        else
            UpdateAffectedList(0, AT_ACTION_MOMENT_LEAVE);//remove from world in action


        // Possibly this?
        if (!IsInWorld())
            return;

        SendObjectDeSpawnAnim(GetGUID());
        RemoveFromWorld();
        AddObjectToRemoveList();
    }
}

float AreaTrigger::GetVisualScale(bool max /*=false*/) const
{
    if (max) return atInfo.radius2;
    return atInfo.radius;
}

Unit* AreaTrigger::GetCaster() const
{
    return ObjectAccessor::GetUnit(*this, GetCasterGUID());
}

bool AreaTrigger::CheckActionConditions(AreaTriggerAction const& action, Unit* unit)
{
    Unit* caster = GetCaster();
    if (!caster)
        return false;

    ConditionSourceInfo srcInfo = ConditionSourceInfo(caster, unit);
    return sConditionMgr->IsObjectMeetToConditions(srcInfo, sConditionMgr->GetConditionsForAreaTriggerAction(GetEntry(), action.id));
}

void AreaTrigger::BindToCaster()
{
    ASSERT(!_caster);
    _caster = ObjectAccessor::GetUnit(*this, GetCasterGUID());
    ASSERT(_caster);
    ASSERT(_caster->GetMap() == GetMap());
    _caster->_RegisterAreaObject(this);
}

void AreaTrigger::UnbindFromCaster()
{
    ASSERT(_caster);
    _caster->_UnregisterAreaObject(this);
    _caster = NULL;
}

uint32 AreaTrigger::GetObjectMovementParts() const
{
    //now only source and destination points send.
    //ToDo: find interval calculation. On some spels only 2 points send (each 2 times)
    return 4;
}

void AreaTrigger::PutObjectUpdateMovement(ByteBuffer* data) const
{
    //Source position 2 times.
    *data << float(GetPositionX());
    *data << float(GetPositionZ());
    *data << float(GetPositionY());
            
    *data << float(GetPositionX());
    *data << float(GetPositionZ());
    *data << float(GetPositionY());

    //Dest position 2 times.
    *data << float(_destPosition.GetPositionX());
    *data << float(_destPosition.GetPositionZ());
    *data << float(_destPosition.GetPositionY());

    *data << float(_destPosition.GetPositionX());
    *data << float(_destPosition.GetPositionZ());
    *data << float(_destPosition.GetPositionY());
}

void AreaTrigger::UpdateMovement(uint32 diff)
{
    if (!isMoving())
        return;

    _moveTime += diff;
    _startPosition.SimplePosXYRelocationByAngle(*this, getMoveSpeed() * _moveTime, 0.0f);
}