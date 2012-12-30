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

#ifndef TRINITY_COMBATAI_H
#define TRINITY_COMBATAI_H

#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "ConditionMgr.h"

class Creature;

class AggressorAI : public CreatureAI
{
    public:
        explicit AggressorAI(CreaturePtr c) : CreatureAI(c) {}

        void UpdateAI(const uint32);
        static int Permissible(constCreaturePtr);
};

typedef std::vector<uint32> SpellVct;

class CombatAI : public CreatureAI
{
    public:
        explicit CombatAI(CreaturePtr c) : CreatureAI(c) {}

        void InitializeAI();
        void Reset();
        void EnterCombat(UnitPtr who);
        void JustDied(UnitPtr killer);
        void UpdateAI(const uint32 diff);
        static int Permissible(constCreaturePtr);
    protected:
        EventMap events;
        SpellVct spells;
};

class CasterAI : public CombatAI
{
    public:
        explicit CasterAI(CreaturePtr c) : CombatAI(c) { m_attackDist = MELEE_RANGE; }
        void InitializeAI();
        void AttackStart(UnitPtr victim) { AttackStartCaster(victim, m_attackDist); }
        void UpdateAI(const uint32 diff);
        void EnterCombat(UnitPtr /*who*/);
    private:
        float m_attackDist;
};

struct ArcherAI : public CreatureAI
{
    public:
        explicit ArcherAI(CreaturePtr c);
        void AttackStart(UnitPtr who);
        void UpdateAI(const uint32 diff);

        static int Permissible(constCreaturePtr);
    protected:
        float m_minRange;
};

struct TurretAI : public CreatureAI
{
    public:
        explicit TurretAI(CreaturePtr c);
        bool CanAIAttack(constUnitPtr who) const;
        void AttackStart(UnitPtr who);
        void UpdateAI(const uint32 diff);

        static int Permissible(constCreaturePtr);
    protected:
        float m_minRange;
};

#define VEHICLE_CONDITION_CHECK_TIME 1000
#define VEHICLE_DISMISS_TIME 5000
struct VehicleAI : public CreatureAI
{
    public:
        explicit VehicleAI(CreaturePtr c);

        void UpdateAI(const uint32 diff);
        static int Permissible(constCreaturePtr);
        void Reset();
        void MoveInLineOfSight(UnitPtr) {}
        void AttackStart(UnitPtr) {}
        void OnCharmed(bool apply);

    private:
        VehiclePtr m_vehicle;
        bool m_IsVehicleInUse;
        void LoadConditions();
        void CheckConditions(const uint32 diff);
        ConditionList conditions;
        uint32 m_ConditionsTimer;
        bool m_DoDismiss;
        uint32 m_DismissTimer;
};

#endif
