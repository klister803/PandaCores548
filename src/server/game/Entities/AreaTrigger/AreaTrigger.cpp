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
    _radius(1.0f), atInfo(), _on_despawn(false), m_spellInfo(NULL), _moveSpeed(0.0f), _moveTime(0), _realEntry(0), _hitCount(1), _areaTriggerCylinder(false),
    _on_remove(false), _waitTime(0)
{
    m_objectType |= TYPEMASK_AREATRIGGER;
    m_objectTypeId = TYPEID_AREATRIGGER;

    m_updateFlag = UPDATEFLAG_STATIONARY_POSITION | UPDATEFLAG_AREA_TRIGGER;

    m_valuesCount = AREATRIGGER_END;
    m_currentNode = 0;
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

bool AreaTrigger::CreateAreaTrigger(uint32 guidlow, uint32 triggerEntry, Unit* caster, SpellInfo const* info, Position const& pos, Position const& posMove, Spell* spell /*=NULL*/, uint64 targetGuid /*=0*/)
{
    m_spellInfo = info;

    if (!info)
    {
        TC_LOG_ERROR("server", "AreaTrigger (entry %u) caster %s no spellInfo", triggerEntry, caster->GetString().c_str());
        return false;
    }

    // Caster not in world, might be spell triggered from aura removal
    if (!caster->IsInWorld())
        return false;

    if (!caster->IsAlive())
    {
        TC_LOG_ERROR("server", "AreaTrigger (spell %u) caster %s is dead ", info->Id, caster->GetString().c_str());
        return false;
    }

    SetMap(caster->GetMap());
    Relocate(pos);
    if (!IsPositionValid())
    {
        TC_LOG_ERROR("server", "AreaTrigger (spell %u) not created. Invalid coordinates (X: %f Y: %f)", info->Id, GetPositionX(), GetPositionY());
        return false;
    }

    if (AreaTriggerInfo const* infoAt = sObjectMgr->GetAreaTriggerInfo(triggerEntry))
    {
        atInfo = *infoAt;
        _activationDelay = atInfo.activationDelay;

        for (AreaTriggerActionList::const_iterator itr = atInfo.actions.begin(); itr != atInfo.actions.end(); ++itr)
            _actionInfo[itr->id] = ActionInfo(&*itr);
    }

    //test implemented attach areatrigger to unit
    if (targetGuid && atInfo.HasAttached)
    {
        m_movementInfo.transportGUID = targetGuid;
        m_updateFlag |= UPDATEFLAG_GO_TRANSPORT_POSITION;
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

    CalculateRadius(spell);

    if (atInfo.Height && !atInfo.polygon)
        _areaTriggerCylinder = true;

    SetUInt64Value(AREATRIGGER_CASTER, caster->GetGUID());
    SetUInt32Value(AREATRIGGER_SPELLID, info->Id);
    SetUInt32Value(AREATRIGGER_SPELLVISUALID, info->SpellVisual[0] ? info->SpellVisual[0] : info->SpellVisual[1]);
    SetUInt32Value(AREATRIGGER_DURATION, duration);
    SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, 1.0f);
    SetTargetGuid(targetGuid);

    // culculate destination point
    if (isMoving())
    {
        _waitTime = atInfo.waitTime;

        if (atInfo.speed)
            _moveSpeed = atInfo.speed;
        else
            _moveSpeed = (GetSpellInfo()->GetMaxRange() / duration) * 1000.0f;

        switch (atInfo.moveType)
        {
            case AT_MOVE_TYPE_DEFAULT:
            {
                G3D::Vector3 curPos, nextPos;
                pos.PositionToVector(curPos);
                m_movePath.push_back(curPos);
                m_movePath.push_back(curPos);
                pos.SimplePosXYRelocationByAngle(nextPos.x, nextPos.y, nextPos.z, GetSpellInfo()->GetMaxRange(), 0.0f);
                m_movePath.push_back(nextPos);
                m_movePath.push_back(nextPos);
                break;
            }
            case AT_MOVE_TYPE_LIMIT:
            {
                G3D::Vector3 curPos, nextPos;
                pos.PositionToVector(curPos);
                posMove.PositionToVector(nextPos);
                m_movePath.push_back(curPos);
                m_movePath.push_back(curPos);
                m_movePath.push_back(nextPos);
                m_movePath.push_back(nextPos);
                _moveSpeed = (curPos - nextPos).length() / (duration - _waitTime) * 1000.0f;
                SetDuration(duration);
                break;
            }
            case AT_MOVE_TYPE_SPIRAL:
            {
                G3D::Vector3 curPos;
                pos.PositionToVector(curPos);
                m_movePath.push_back(curPos);
                m_movePath.push_back(curPos);
                float maxDist = duration / 1000.0f * _moveSpeed;

                for (float a = 0;; a += 45) // next angel 45
                {
                    float rad = a * 3.14159265358979323846f / 180.0f;
                    float r = 1.0f * exp(rad * 0.3f);
                    G3D::Vector3 nextPos {curPos.x - (r * sin(rad)), curPos.y + (r * cos(rad)), curPos.z};
                    m_movePath.push_back(nextPos);

                    //TC_LOG_DEBUG("spell", "AreaTrigger::Create nextPos %f %f %f maxDist %f length %f rad %f r %f",
                    //nextPos.x, nextPos.y, nextPos.z, maxDist, (m_movePath[m_movePath.size() - 2] - m_movePath[m_movePath.size() - 1]).length(), rad, r);

                    maxDist -= (m_movePath[m_movePath.size() - 2] - m_movePath[m_movePath.size() - 1]).length();
                    if (maxDist <= 0.0f)
                    {
                        m_movePath.push_back(nextPos);
                        break;
                    }
                }
                break;
            }
        }

        _nextMoveTime = (m_movePath[0] - m_movePath[1]).length() * _moveSpeed;
    }

    FillCustiomData();

    setActive(true);

    if (!GetMap()->AddToMap(this))
        return false;

    #ifdef WIN32
    TC_LOG_DEBUG("spell", "AreaTrigger::Create AreaTrigger caster %s spellID %u spell rage %f dist %f dest - X:%f,Y:%f,Z:%f _nextMoveTime %i _moveSpeed %f duration %i", 
    caster->GetString().c_str(), info->Id, _radius, GetSpellInfo()->GetMaxRange(), m_movePath.empty() ? 0.0f : m_movePath[m_currentNode].x, m_movePath.empty() ? 0.0f : m_movePath[m_currentNode].y, m_movePath.empty() ? 0.0f : m_movePath[m_currentNode].z, _nextMoveTime, _moveSpeed, duration);
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
        bool _updateUnit = true;

        if (atInfo.updateDelay)
        {
            _updateDelay += p_time;

            if (_updateDelay < atInfo.updateDelay)
                _updateUnit = false;
            else
                _updateDelay = 0;
        }

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

            if(atInfo.polygon)
            {
                if (!IsInPolygon(unit, searcher))
                {
                    affectedPlayers.erase(itr);
                    AffectUnit(unit, AT_ACTION_MOMENT_LEAVE);
                    continue;
                }
            }
            else
            {
                if (!IsInHeight(unit, searcher) || !unit->IsWithinDistInMap(searcher, GetRadius(), true, false) ||
                    (isMoving() && _HasActionsWithCharges(AT_ACTION_MOMENT_ON_THE_WAY) && !unit->IsInBetween(this, m_movePath[m_currentNode + 1].x, m_movePath[m_currentNode + 1].y)))
                {
                    affectedPlayers.erase(itr);
                    AffectUnit(unit, AT_ACTION_MOMENT_LEAVE);
                    continue;
                }
            }

            UpdateOnUnit(unit);
        }

        std::list<Unit*> unitList;
        searcher->GetAttackableUnitListInRange(unitList, GetRadius(), false);
        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
        {
            if (!IsUnitAffected((*itr)->GetGUID()))
            {
                if(atInfo.polygon && !IsInPolygon((*itr), searcher))
                    continue;

                if (!IsInHeight((*itr), searcher))
                    continue;

                //No 
                if (isMoving() && _HasActionsWithCharges(AT_ACTION_MOMENT_ON_THE_WAY) && !(*itr)->IsInBetween(this, m_movePath[m_currentNode + 1].x, m_movePath[m_currentNode + 1].y))
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

void AreaTrigger::UpdateOnUnit(Unit* unit)
{
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

    //TC_LOG_DEBUG("spell", "AreaTrigger::DoAction caster %s unit %s type %u spellID %u, moment %u, targetFlags %u",
    //caster->GetString().c_str(), unit->GetString().c_str(), action.action->actionType, action.action->spellId, action.action->moment, action.action->targetFlags);

    if (action.action->targetFlags & AT_TARGET_FLAG_FRIENDLY)
        if (!caster || !caster->IsFriendlyTo(unit))
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_HOSTILE)
        if (!caster || !caster->IsHostileTo(unit))
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_VALIDATTACK)
        if (!caster || !caster->IsValidAttackTarget(unit))
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_OWNER)
    if (unit->GetGUID() != GetCasterGUID())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_PLAYER)
        if (!unit->ToPlayer())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_PET)
        if (unit->IsPet())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_FULL_HP)
        if (unit->IsFullHealth())
            return;
    if (action.action->targetFlags & AT_TARGET_FLAG_GROUP_OR_RAID)
        if (!unit->IsInRaidWith(caster))
            return;
    //action on self
    if (action.action->targetFlags & AT_TARGET_FLAG_TARGET_IS_CASTER)
        unit = caster;

    if (action.action->targetFlags & AT_TARGET_FLAT_IN_FRONT)
        if (!HasInArc(static_cast<float>(M_PI), unit))
            return;

    if (action.action->aura > 0 && !unit->HasAura(action.action->aura))
        return;
    else if (action.action->aura < 0 && unit->HasAura(abs(action.action->aura)))
        return;

    if (action.action->hasspell > 0 && !unit->HasSpell(action.action->hasspell))
        return;
    else if (action.action->hasspell < 0 && unit->HasSpell(abs(action.action->hasspell)))
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
            if(spellInfo->Effects[i]->Effect == SPELL_EFFECT_ENERGIZE)
                energeType = Powers(spellInfo->Effects[i]->MiscValue);
        }

        if (energeType == POWER_NULL || unit->GetMaxPower(energeType) == 0 || unit->GetMaxPower(energeType) == unit->GetPower(energeType))
            return;
    }

    // should cast on self.
    if (spellInfo->Effects[EFFECT_0]->TargetA.GetTarget() == TARGET_UNIT_CASTER
        || action.action->targetFlags & AT_TARGET_FLAG_CASTER_IS_TARGET)
        caster = unit;

    if (action.action->hitMaxCount < 0)
    {
        if (!affectedPlayersForAllTime.empty())
            for (std::list<uint64>::iterator itr = affectedPlayersForAllTime.begin(); itr != affectedPlayersForAllTime.end(); ++itr)
                if (unit->GetGUID() == (*itr))
                    return;
    }
    else if (action.action->hitMaxCount && int32(action.hitCount) >= action.action->hitMaxCount)
        return;

    switch (action.action->actionType)
    {
        case AT_ACTION_TYPE_CAST_SPELL:
        {
            if (_on_remove)
                return;

            if (!unit->IsAlive() || !unit->IsInWorld())
                return;

            if (caster)
            {
                if (action.action->targetFlags & AT_TARGET_FLAG_CAST_AT_SRC)
                    caster->CastSpell(GetPositionX(), GetPositionY(), GetPositionZH(), action.action->spellId, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_CASTED_BY_AREATRIGGER));
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
        case AT_ACTION_TYPE_CHANGE_SCALE: //limit scale by hit
        {
            float scale = GetFloatValue(AREATRIGGER_EXPLICIT_SCALE) + action.action->scale;
            SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
            break;
        }
        case AT_ACTION_TYPE_SHARE_DAMAGE:
        {
            if (caster)
            {
                float bp0 = spellInfo->GetEffect(EFFECT_0, caster->GetSpawnMode())->BasePoints / _hitCount;
                float bp1 = spellInfo->GetEffect(EFFECT_1, caster->GetSpawnMode())->BasePoints / _hitCount;
                float bp2 = spellInfo->GetEffect(EFFECT_2, caster->GetSpawnMode())->BasePoints / _hitCount;

                if (action.action->targetFlags & AT_TARGET_FLAG_CAST_AT_SRC)
                {
                    SpellCastTargets targets;
                    targets.SetDst(GetPositionX(), GetPositionY(), GetPositionZH(), GetOrientation());

                    CustomSpellValues values;
                    if (bp0)
                        values.AddSpellMod(SPELLVALUE_BASE_POINT0, bp0);
                    if (bp1)
                        values.AddSpellMod(SPELLVALUE_BASE_POINT1, bp1);
                    if (bp2)
                        values.AddSpellMod(SPELLVALUE_BASE_POINT2, bp2);
                    caster->CastSpell(targets, spellInfo, &values, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_CASTED_BY_AREATRIGGER));
                }
                else
                    caster->CastCustomSpell(unit, action.action->spellId, &bp0, &bp1, &bp2, true);
            }
            break;
        }
        case AT_ACTION_TYPE_APPLY_MOVEMENT_FORCE:
        {
            unit->SendMovementForce(this, atInfo.windX, atInfo.windY, atInfo.windZ, atInfo.windSpeed, atInfo.windType, true);
            break;
        }
        case AT_ACTION_TYPE_REMOVE_MOVEMENT_FORCE:
        {
            unit->SendMovementForce(this, atInfo.windX, atInfo.windY, atInfo.windZ, atInfo.windSpeed, atInfo.windType, false);
            break;
        }
        case AT_ACTION_TYPE_CHANGE_DURATION_ANY_AT:
        {
            float searchRange = 0.0f;
            for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                if (float radius = spellInfo->Effects[j]->CalcRadius(caster))
                    searchRange = radius;
            }
            if(!searchRange)
                break;
            std::list<AreaTrigger*> atlist;
            GetAreaTriggerListWithEntryInGrid(atlist, GetEntry(), searchRange);
            if (!atlist.empty())
            {
                for (std::list<AreaTrigger*>::const_iterator itr = atlist.begin(); itr != atlist.end(); ++itr)
                {
                    if(AreaTrigger* at = (*itr))
                        if(at->GetDuration() > 500)
                            at->SetDuration(100);
                }
            }
            break;
        }
        case AT_ACTION_TYPE_CHANGE_AMOUNT_FROM_HEALT:
        {
            if(_on_remove || !action.amount)
                return;

            if (caster)
            {
                float health = unit->GetMaxHealth() - unit->GetHealth();
                if(health >= action.amount)
                {
                    health = action.amount;
                    action.amount = 0;
                }
                else
                    action.amount -= health;

                caster->CastCustomSpell(unit, action.action->spellId, &health, &health, &health, true);
            }

            if(!action.amount && action.charges > 0)
            {
                _on_despawn = true;
                SetDuration(0);
            }
            return;
        }
    }

    if (action.action->hitMaxCount < 0)
        affectedPlayersForAllTime.push_back(unit->GetGUID());

    action.hitCount++;
    if (atInfo.hitType & (1 << action.action->actionType))
        _hitCount++;

    //TC_LOG_DEBUG("spell", "AreaTrigger::DoAction action _hitCount %i hitCount %i hitMaxCount %i hitType %i actionType %i", _hitCount, action.hitCount, action.action->hitMaxCount, atInfo.hitType, action.action->actionType);

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

void AreaTrigger::Despawn()
{
    if (_on_remove)
        return;
    _on_remove = true;

    if (IsInWorld())
    {
        UpdateAffectedList(0, AT_ACTION_MOMENT_REMOVE);//any remove from world
        UpdateAffectedList(0, AT_ACTION_MOMENT_DESPAWN);//remove from world with time

        // Possibly this?
        if (!IsInWorld())
            return;

        SendObjectDeSpawnAnim(GetGUID());
        RemoveFromWorld();
        AddObjectToRemoveList();
    }
}

float AreaTrigger::GetVisualScale(bool target /*=false*/) const
{
    if(_areaTriggerCylinder) // Send only for sphere
        return false;

    if (target) return atInfo.RadiusTarget;
    return atInfo.Radius;
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
    return m_movePath.size();
}

void AreaTrigger::PutObjectUpdateMovement(ByteBuffer* data) const
{
    for (uint32 i = 0; i < m_movePath.size(); ++i)
    {
        *data << float(m_movePath[i].x);
        *data << float(m_movePath[i].z);
        *data << float(m_movePath[i].y);
    }
}

void AreaTrigger::UpdateMovement(uint32 diff)
{
    if (!isMoving() || m_movePath.empty())
        return;

    if (m_currentNode == m_movePath.size() - 1)
        return;

    if (_waitTime > 0)
    {
        if (diff >= _waitTime)
            _waitTime = 0;
        else
            _waitTime -= diff;
        return;
    }

    Position tempPos;
    tempPos.VectorToPosition(m_movePath[m_currentNode]);
    float angle = tempPos.GetAngle(m_movePath[m_currentNode + 1].x, m_movePath[m_currentNode + 1].y);

    //float speed = getMoveSpeed() / (((m_movePath.size() - 1) / 4) * m_currentNode);
    float speed = getMoveSpeed();
    if(atInfo.moveType == AT_MOVE_TYPE_SPIRAL)
    {
        int32 mod = int32(m_currentNode / 4);
        if(mod)
            speed = getMoveSpeed() / 2 * mod;
        else
            speed = getMoveSpeed() / 4;
    }
    _moveTime += diff;

    if(_moveTime >= _nextMoveTime)
    {
        tempPos.SimplePosXYRelocationByAngle(*this, (speed * _nextMoveTime) / 1000.0f, angle, true);

        m_currentNode++;
        _moveTime = 0;
        if (m_currentNode == m_movePath.size() - 1)
            return;

        float speedNext = getMoveSpeed();
        if(atInfo.moveType == AT_MOVE_TYPE_SPIRAL)
        {
            int32 mod = int32(m_currentNode / 4);
            if(mod)
                speedNext = getMoveSpeed() / 2 * mod;
            else
                speedNext = getMoveSpeed() / 4;
        }
        //_caster->SummonCreature(44548, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation(),TEMPSUMMON_TIMED_DESPAWN, 20000); // For visual point test
        _nextMoveTime = (m_movePath[m_currentNode] - m_movePath[m_currentNode+1]).length() / speedNext * 1000;
        //TC_LOG_DEBUG("spell", "AreaTrigger::UpdateMovement speedNext %f _nextMoveTime %u length %f abs %i", speedNext, _nextMoveTime, (m_movePath[m_currentNode] - m_movePath[m_currentNode+1]).length(), int32(m_currentNode/4));
    }
    else
        tempPos.SimplePosXYRelocationByAngle(*this, (speed * _moveTime) / 1000.0f, angle, true);

    //TC_LOG_DEBUG("spell", "AreaTrigger::UpdateMovement %f %f %f %f %i angle %f _nextMoveTime %i m_currentNode %i speed %f", GetPositionX(), GetPositionY(), GetPositionZ(), getMoveSpeed(), _moveTime, angle, _nextMoveTime, m_currentNode, speed);

}

bool AreaTrigger::IsInHeight(Unit* unit, WorldObject const* obj)
{
    if (!atInfo.Height)
        return true;

    float z_source = unit->GetPositionZH() - obj->GetPositionZH();
    if(z_source > atInfo.Height)
        return false;

    return true;
}

bool AreaTrigger::IsInPolygon(Unit* unit, WorldObject const* obj)
{
    if (atInfo.polygonPoints.size() < 3)
        return false;

    if(!IsInHeight(unit, obj))
        return false;

    static const int q_patt[2][2]= { {0,1}, {3,2} };
    float x_source = unit->GetPositionX() - obj->GetPositionX(); //Point X on polygon
    float y_source = unit->GetPositionY() - obj->GetPositionY(); //Point Y on polygon
    float angle = atan2(y_source, x_source) - obj->GetOrientation(); angle = (angle >= 0) ? angle : 2 * M_PI + angle;

    float dist = sqrt(x_source*x_source + y_source*y_source);
    float x = dist * std::cos(angle);
    float y = dist * std::sin(angle);

    //TC_LOG_DEBUG("spell", "AreaTrigger::IsInPolygon x_source %f y_source %f angle %f dist %f x %f y %f", x_source, y_source, angle, dist, x, y);

    G3D::Vector2 pred_pt = atInfo.polygonPoints[0];
    pred_pt.x -= x;
    pred_pt.y -= y;

    int pred_q = q_patt[pred_pt.y < 0][pred_pt.x < 0];

    int w = 0;

    for (uint16 i = 0; i < atInfo.polygonPoints.size(); ++i)
    {
        G3D::Vector2 cur_pt = atInfo.polygonPoints[i];

        cur_pt.x -= x;
        cur_pt.y -= y;

        int q = q_patt[cur_pt.y < 0][cur_pt.x < 0];

        switch (q - pred_q)
        {
            case -3:
                ++w;
                break;
            case 3:
                --w;
                break;
            case -2:
                if (pred_pt.x * cur_pt.y >= pred_pt.y * cur_pt.x)
                    ++w;
                break;
            case 2:
                if (!(pred_pt.x * cur_pt.y >= pred_pt.y * cur_pt.x))
                    --w;
                break;
        }

        pred_pt = cur_pt;
        pred_q = q;
    }
    return w != 0;
}

void AreaTrigger::CalculateRadius(Spell* spell/* = nullptr*/)
{
    Unit* caster = GetCaster();

    if(atInfo.polygon)
        _radius = CalculateRadiusPolygon();
    else
    {
        bool find = false;
        if(atInfo.Radius || atInfo.RadiusTarget)
        {
            if(atInfo.Radius > atInfo.RadiusTarget)
                _radius = atInfo.Radius;
            else
                _radius = atInfo.RadiusTarget;
            find = true;
        }

        if(caster && !find && m_spellInfo)
        {
            for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                if (m_spellInfo->EffectMask < uint32(1 << j))
                    break;

                if (float r = m_spellInfo->Effects[j]->CalcRadius(GetCaster()))
                    _radius = r * (spell ? spell->m_spellValue->RadiusMod : 1.0f);
            }
        }
    }

    if (caster && m_spellInfo)
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RADIUS, _radius, spell);
}

float AreaTrigger::CalculateRadiusPolygon()
{
    //calc maxDist for search zone
    float distance = 0.0f;
    for (uint16 i = 0; i < atInfo.polygonPoints.size(); ++i)
    {
        G3D::Vector2 cur_pt = atInfo.polygonPoints[i];
        float distsq = fabs(cur_pt.x) > fabs(cur_pt.y) ? fabs(cur_pt.x) : fabs(cur_pt.y);
        if(distsq > distance)
            distance = distsq;
    }

    // TC_LOG_DEBUG("spell", "AreaTrigger::CalculateRadius distance %f", distance);

    return distance;
}
