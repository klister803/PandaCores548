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

#ifndef TRINITYCORE_AREATRIGGER_H
#define TRINITYCORE_AREATRIGGER_H

#include "Object.h"

class Unit;
class SpellInfo;
class Spell;

enum AreaTriggerActionMoment
{
    AT_ACTION_MOMENT_ENTER      = 0,                // when unit enters areatrigger
    AT_ACTION_MOMENT_LEAVE      = 1,                // when unit exits areatrigger
    AT_ACTION_MOMENT_UPDATE     = 2,                // on areatrigger update
    AT_ACTION_MOMENT_MAX        = 3,
};

enum AreaTriggerActionType
{
    AT_ACTION_TYPE_CAST_SPELL   = 0,
    AT_ACTION_TYPE_REMOVE_AURA  = 1,
    AT_ACTION_TYPE_MAX          = 2,
};

enum AreaTriggerTargetFlags
{
    AT_TARGET_FLAG_FRIENDLY          = 0x01,             // casted on targets that are friendly to areatrigger owner
    AT_TARGET_FLAG_HOSTILE           = 0x02,             // casted on targets that are hostile to areatrigger owner
    AT_TARGET_FLAG_OWNER             = 0x04,             // casted only on areatrigger owner
    AT_TARGET_FLAG_PLAYER            = 0x08,             // casted only on players
    AT_TARGET_FLAG_NOT_PET           = 0x10,             // casted on everyone except pets
    AT_TARGET_FLAG_CAST_AT_SRC       = 0x20,             // casted on areatrigger position as dest
    AT_TARGET_FLAG_CASTER_IS_TARGET  = 0x40,             // casted on areatrigger position as dest

    AT_TARGET_MASK_REQUIRE_TARGET = 
        AT_TARGET_FLAG_FRIENDLY | AT_TARGET_FLAG_HOSTILE | AT_TARGET_FLAG_OWNER | AT_TARGET_FLAG_PLAYER |
        AT_TARGET_FLAG_NOT_PET  | AT_TARGET_FLAG_CASTER_IS_TARGET,
};

struct AreaTriggerAction
{
    uint8 id;
    AreaTriggerActionMoment moment;
    AreaTriggerActionType actionType;
    AreaTriggerTargetFlags targetFlags;
    uint32 spellId;
    int8 maxCharges;
    uint32 chargeRecoveryTime;
};

typedef std::list<AreaTriggerAction> AreaTriggerActionList;

struct AreaTriggerInfo
{
    AreaTriggerInfo() : radius(1.0f), radius2(1.0f), activationDelay(0), updateDelay(0), maxCount(0), 
        visualId(1){}

    float radius;
    float radius2;
    uint32 visualId;    //unk520 on 5.4.8 parse at SMSG_UPDATE_OBJECT
    uint32 activationDelay;
    uint32 updateDelay;
    uint8 maxCount;
    AreaTriggerActionList actions;
};

class AreaTrigger : public WorldObject, public GridObject<AreaTrigger>
{
    private:
        struct ActionInfo
        {
            ActionInfo() : charges(0), recoveryTime(0), action(NULL) { }
            ActionInfo(AreaTriggerAction const* _action) :
                charges(_action->maxCharges), recoveryTime(0), action(_action) { }

            uint8 charges;
            uint32 recoveryTime;
            AreaTriggerAction const* action;
        };
        typedef std::map<uint8, ActionInfo> ActionInfoMap;

    public:

        AreaTrigger();
        ~AreaTrigger();

        void AddToWorld();
        void RemoveFromWorld();

        bool CreateAreaTrigger(uint32 guidlow, uint32 triggerEntry, Unit* caster, Spell* spell, Position const& pos);
        void Update(uint32 p_time);
        void UpdateAffectedList(uint32 p_time, bool despawn);
        void Remove();
        uint32 GetSpellId() const { return GetUInt32Value(AREATRIGGER_SPELLID); }
        uint64 GetCasterGUID() const { return GetUInt64Value(AREATRIGGER_CASTER); }
        Unit* GetCaster() const;
        int32 GetDuration() const { return _duration; }
        void SetDuration(int32 newDuration) { _duration = newDuration; }
        void Delay(int32 delaytime) { SetDuration(GetDuration() - delaytime); }
        float GetVisualScale(bool max = false) const;
        float GetRadius(bool second = false) const;
        float GetVisualId() const;
        bool IsUnitAffected(uint64 guid) const;
        void AffectUnit(Unit* unit, bool enter);
        void UpdateOnUnit(Unit* unit, uint32 p_time);
        void DoAction(Unit* unit, ActionInfo& action);
        bool CheckActionConditions(AreaTriggerAction const& action, Unit* unit);
        void UpdateActionCharges(uint32 p_time);

        void BindToCaster();
        void UnbindFromCaster();

    private:
        bool _HasActionsWithCharges();

    protected:
        Unit* _caster;
        int32 _duration;
        uint32 _activationDelay;
        uint32 _updateDelay;
        std::list<uint64> affectedPlayers;
        float _radius;
        AreaTriggerInfo atInfo;
        ActionInfoMap _actionInfo;
        
        bool _on_unload;
};
#endif
