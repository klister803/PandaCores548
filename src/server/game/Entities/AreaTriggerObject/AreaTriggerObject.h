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

#ifndef AREATRIGGEROBJECT_H
#define AREATRIGGEROBJECT_H

#include "Object.h"

class Unit;
class Aura;
class SpellInfo;

class AreaTriggerObject : public WorldObject, public GridObject<AreaTriggerObject>
{
    public:
        AreaTriggerObject(bool isWorldObject);
        ~AreaTriggerObject();

        void AddToWorld();
        void RemoveFromWorld();

        bool CreateAreaTriggerObject(uint32 guidlow, Unit* caster, uint32 spellId, uint32 spellVisualId, Position const& pos);
        void Update(uint32 p_time);
        void Remove();
		int32 GetDuration() const;
		void SetDuration(int32 newDuration);
        void Delay(int32 delaytime);
        void SetAura(AuraPtr aura);
        void RemoveAura();
        Unit* GetCaster() const { return _caster; }
        void BindToCaster();
        void UnbindFromCaster();
        uint32 GetSpellId() const {  return GetUInt32Value(AREATRIGGER_SPELLID); }
        uint64 GetCasterGUID() const { return GetUInt64Value(AREATRIGGER_CASTER); }
        float GetSpellVisualId() const { return GetUInt32Value(AREATRIGGER_SPELLVISUALID); }

    protected:
        AuraPtr _aura;
        AuraPtr _removedAura;
        int32 _duration;
        Unit* _caster;
};
#endif
