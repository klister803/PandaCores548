/*
 * Copyright (C) 2012-2013 JadeCore <http://pandashan.com/>
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
#include "UpdateMask.h"
#include "Opcodes.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "ScriptMgr.h"

AreaTriggerObject::AreaTriggerObject(bool isWorldObject) : WorldObject(isWorldObject),
    _caster(NULL), _duration(0)
{
    m_objectType |= TYPEMASK_AREATRIGGER;
    m_objectTypeId = TYPEID_AREATRIGGEROBJECT;

    m_updateFlag = UPDATEFLAG_STATIONARY_POSITION;

    m_valuesCount = AREATRIGGER_END;
}

AreaTriggerObject::~AreaTriggerObject()
{
    // make sure all references were properly removed
    ASSERT(!_caster);
    ASSERT(!_aura);
}

void AreaTriggerObject::AddToWorld()
{
    ///- Register the AreaTriggerObject for guid lookup and for caster
    if (!IsInWorld())
    {
        sObjectAccessor->AddObject(this);
        WorldObject::AddToWorld();
        BindToCaster();
    }
}

void AreaTriggerObject::RemoveFromWorld()
{
    ///- Remove the AreaTriggerObject from the accessor and from all lists of objects in world
    if (IsInWorld())
    {
        if (!IsInWorld())
            return;
        
        if (_aura)
            RemoveAura();

        UnbindFromCaster();
        WorldObject::RemoveFromWorld();
        sObjectAccessor->RemoveObject(this);
    }
}

bool AreaTriggerObject::CreateAreaTriggerObject(uint32 guidlow, Unit* caster, uint32 spellId, uint32 spellVisualId, Position const& pos)
{
	SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
	if (!spellInfo)
		return false;
	
    SetMap(caster->GetMap());
    Relocate(pos);
    if (!IsPositionValid())
    {
        sLog->outError(LOG_FILTER_GENERAL, "AreaTriggerObject (spell %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)", spellId, GetPositionX(), GetPositionY());
        return false;
    }

    WorldObject::_Create(guidlow, HIGHGUID_AREATRIGGEROBJECT, caster->GetPhaseMask());

    SetEntry(spellInfo->Id);
    SetObjectScale(1);
    SetUInt64Value(AREATRIGGER_CASTER, caster->GetGUID());
    SetUInt32Value(AREATRIGGER_SPELLVISUALID, spellVisualId);
    SetUInt32Value(AREATRIGGER_SPELLID, spellInfo->Id);

    if (IsWorldObject())
        setActive(true);    //must before add to map to be put in world container

    if (!GetMap()->AddToMap(this))
        return false;

    return true;
}

void AreaTriggerObject::Update(uint32 p_time)
{
    ASSERT(_caster);
	
	bool expired = false;
	
    if (_aura)
    {
        if (!_aura->IsRemoved())
            _aura->UpdateOwner(p_time, this);

        // _aura may be set to null in Aura::UpdateOwner call
        if (_aura && (_aura->IsRemoved() || _aura->IsExpired()))
            expired = true;
    }
    else
    {
	    if (GetDuration() > int32(p_time))
		    _duration -= p_time;
	    else
		    expired = true;
    }
		
	if (expired)
		Remove();
	else
		sScriptMgr->OnAreaTriggerObjectUpdate(this, p_time);
}

void AreaTriggerObject::Remove()
{
    if (IsInWorld())
    {
        SendObjectDeSpawnAnim(GetGUID());
        RemoveFromWorld();
        AddObjectToRemoveList();
    }
}

int32 AreaTriggerObject::GetDuration() const
{
    if (!_aura)
        return _duration;
    else
        return _aura->GetDuration();
}

void AreaTriggerObject::SetDuration(int32 newDuration)
{
    if (!_aura)
        _duration = newDuration;
    else
        _aura->SetDuration(newDuration);
}

void AreaTriggerObject::Delay(int32 delaytime)
{
    SetDuration(GetDuration() - delaytime);
}

void AreaTriggerObject::SetAura(AuraPtr aura)
{
    ASSERT(!_aura && aura);
    _aura = aura;
}

void AreaTriggerObject::RemoveAura()
{
    ASSERT(_aura && !_removedAura);
    _removedAura = _aura;
    _aura = NULL;
    if (!_removedAura->IsRemoved())
        _removedAura->_Remove(AURA_REMOVE_BY_DEFAULT);
}

void AreaTriggerObject::BindToCaster()
{
    ASSERT(!_caster);
    _caster = ObjectAccessor::GetUnit(*this, GetCasterGUID());
    ASSERT(_caster);
    ASSERT(_caster->GetMap() == GetMap());
    _caster->_RegisterAreaTriggerObject(this);
}

void AreaTriggerObject::UnbindFromCaster()
{
    ASSERT(_caster);
    _caster->_UnregisterAreaTriggerObject(this);
    _caster = NULL;
}