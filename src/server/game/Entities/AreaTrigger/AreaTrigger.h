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
    AT_ACTION_MOMENT_ENTER      = 0x01,                // when unit enters areatrigger
    AT_ACTION_MOMENT_LEAVE      = 0x02,                // when unit exits areatrigger
    AT_ACTION_MOMENT_UPDATE     = 0x04,                // on areatrigger update
    AT_ACTION_MOMENT_DESPAWN    = 0x08,                // when areatrigger despawn
    AT_ACTION_MOMENT_SPAWN      = 0x10,                // when areatrigger spawn
    AT_ACTION_MOMENT_REMOVE     = 0x20,                // when areatrigger remove
    //range should be = distance.
    AT_ACTION_MOMENT_ON_THE_WAY = 0x40,                // when target is betwin source and dest points. For movement only. WARN! Should add AT_ACTION_MOMENT_ENTER flag too
};

enum AreaTriggerActionType
{
    AT_ACTION_TYPE_CAST_SPELL   = 0,
    AT_ACTION_TYPE_REMOVE_AURA  = 1,
    AT_ACTION_TYPE_ADD_STACK    = 2,
    AT_ACTION_TYPE_REMOVE_STACK = 3,
    AT_ACTION_TYPE_MAX          = 4,
};

enum AreaTriggerTargetFlags
{
    AT_TARGET_FLAG_FRIENDLY          = 0x001,             // casted on targets that are friendly to areatrigger owner
    AT_TARGET_FLAG_HOSTILE           = 0x002,             // casted on targets that are hostile to areatrigger owner
    AT_TARGET_FLAG_OWNER             = 0x004,             // casted only on areatrigger owner
    AT_TARGET_FLAG_PLAYER            = 0x008,             // casted only on players
    AT_TARGET_FLAG_NOT_PET           = 0x010,             // casted on everyone except pets
    AT_TARGET_FLAG_CAST_AT_SRC       = 0x020,             // casted on areatrigger position as dest
    AT_TARGET_FLAG_CASTER_IS_TARGET  = 0x040,             // casted on areatrigger position as dest
    AT_TARGET_FLAG_NOT_FULL_HP       = 0x080,             // casted on targets if not full hp
    AT_TARGET_FLAG_ALWAYS_TRIGGER    = 0x100,             // casted always at any action on owner
    AT_TARGET_FLAT_IN_FRONT          = 0x200,             // WARNING! If target come from back he not get cast. ToDo it..
    AT_TARGET_FLAG_NOT_FULL_ENERGY   = 0x400,             // casted on targets if not full enegy

    AT_TARGET_MASK_REQUIRE_TARGET = 
        AT_TARGET_FLAG_FRIENDLY | AT_TARGET_FLAG_HOSTILE | AT_TARGET_FLAG_OWNER | AT_TARGET_FLAG_PLAYER |
        AT_TARGET_FLAG_NOT_PET  | AT_TARGET_FLAG_CASTER_IS_TARGET | AT_TARGET_FLAG_NOT_FULL_HP | AT_TARGET_FLAG_ALWAYS_TRIGGER | AT_TARGET_FLAT_IN_FRONT,
};

struct AreaTriggerAction
{
    uint32 id;
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
        visualId(1), customEntry(0), isMoving(false){}

    bool isMoving;
    float radius;
    float radius2;
    uint32 visualId;    //unk520 on 5.4.8 parse at SMSG_UPDATE_OBJECT
    uint32 activationDelay;
    uint32 updateDelay;
    uint32 customEntry;
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

        bool CreateAreaTrigger(uint32 guidlow, uint32 triggerEntry, Unit* caster, SpellInfo const* info, Position const& pos, Spell* spell = NULL, uint64 targetGuid = 0);
        void Update(uint32 p_time);
        void UpdateAffectedList(uint32 p_time, AreaTriggerActionMoment actionM);
        void Remove(bool duration = true);
        uint32 GetSpellId() const { return GetUInt32Value(AREATRIGGER_SPELLID); }
        void SetSpellId(uint32 spell) { return SetUInt32Value(AREATRIGGER_SPELLID, spell); }
        uint64 GetCasterGUID() const { return GetUInt64Value(AREATRIGGER_CASTER); }
        Unit* GetCaster() const;
        void SetTargetGuid(uint64 targetGuid) { _targetGuid = targetGuid; }
        uint64 GetTargetGuid() { return _targetGuid; }
        int32 GetDuration() const { return _duration; }
        void SetDuration(int32 newDuration) { _duration = newDuration; }
        void Delay(int32 delaytime) { SetDuration(GetDuration() - delaytime); }
        float GetVisualScale(bool max = false) const;
        float GetRadius() const { return _radius; }
        float GetCustomVisualId() const { return atInfo.visualId; }
        uint32 GetCustomEntry() const { return atInfo.customEntry; }
        uint32 GetRealEntry() const { return _realEntry; }
        bool IsUnitAffected(uint64 guid) const;
        void AffectUnit(Unit* unit, AreaTriggerActionMoment actionM);
        void AffectOwner(AreaTriggerActionMoment actionM);
        void UpdateOnUnit(Unit* unit, uint32 p_time);
        void DoAction(Unit* unit, ActionInfo& action);
        bool CheckActionConditions(AreaTriggerAction const& action, Unit* unit);
        void UpdateActionCharges(uint32 p_time);

        void BindToCaster();
        void UnbindFromCaster();

        SpellInfo const*  GetSpellInfo() { return m_spellInfo; }

        //movement
        void UpdateMovement(uint32 diff);
        bool isMoving() const { return atInfo.isMoving; }
        float getMoveSpeed() const { return _moveSpeed; }
        uint32 GetObjectMovementParts() const;
        void PutObjectUpdateMovement(ByteBuffer* data) const;

    private:
        bool _HasActionsWithCharges(AreaTriggerActionMoment action = AT_ACTION_MOMENT_ENTER);
        void FillCustiomData();

    protected:
        Unit* _caster;
        uint64 _targetGuid;
        int32 _duration;
        uint32 _activationDelay;
        uint32 _updateDelay;
        std::list<uint64> affectedPlayers;
        float _radius;
        AreaTriggerInfo atInfo;
        ActionInfoMap _actionInfo;
        Position _destPosition;
        Position _startPosition;
        uint32 _realEntry;
        float _moveSpeed;
        uint32 _moveTime;
        bool _on_unload;
        bool _on_despawn;

        SpellInfo const* m_spellInfo;
};
#endif
