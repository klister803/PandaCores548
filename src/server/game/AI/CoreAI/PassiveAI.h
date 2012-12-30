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

#ifndef TRINITY_PASSIVEAI_H
#define TRINITY_PASSIVEAI_H

#include "CreatureAI.h"

class PassiveAI : public CreatureAI
{
    public:
        explicit PassiveAI(CreaturePtr c);

        void MoveInLineOfSight(UnitPtr) {}
        void AttackStart(UnitPtr) {}
        void UpdateAI(const uint32);

        static int Permissible(constCreaturePtr) { return PERMIT_BASE_IDLE;  }
};

class PossessedAI : public CreatureAI
{
    public:
        explicit PossessedAI(CreaturePtr c);

        void MoveInLineOfSight(UnitPtr) {}
        void AttackStart(UnitPtr target);
        void UpdateAI(const uint32);
        void EnterEvadeMode() {}

        void JustDied(UnitPtr);
        void KilledUnit(UnitPtr victim);

        static int Permissible(constCreaturePtr) { return PERMIT_BASE_IDLE;  }
};

class NullCreatureAI : public CreatureAI
{
    public:
        explicit NullCreatureAI(CreaturePtr c);

        void MoveInLineOfSight(UnitPtr) {}
        void AttackStart(UnitPtr) {}
        void UpdateAI(const uint32) {}
        void EnterEvadeMode() {}
        void OnCharmed(bool /*apply*/) {}

        static int Permissible(constCreaturePtr) { return PERMIT_BASE_IDLE;  }
};

class CritterAI : public PassiveAI
{
    public:
        explicit CritterAI(CreaturePtr c) : PassiveAI(c) {}

        void DamageTaken(UnitPtr done_by, uint32& /*damage*/);
        void EnterEvadeMode();
};

class TriggerAI : public NullCreatureAI
{
    public:
        explicit TriggerAI(CreaturePtr c) : NullCreatureAI(c) {}
        void IsSummonedBy(UnitPtr summoner);
};

#endif

