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

#ifndef TRINITY_PETAI_H
#define TRINITY_PETAI_H

#include "CreatureAI.h"
#include "Timer.h"

class Creature;
class Spell;

class PetAI : public CreatureAI
{
    public:

        explicit PetAI(CreaturePtr c);

        void EnterEvadeMode();
        void JustDied(UnitPtr /*who*/) { _stopAttack(); }

        void UpdateAI(const uint32);
        static int Permissible(constCreaturePtr);

        void KilledUnit(UnitPtr /*victim*/);
        void AttackStart(UnitPtr target);
        void MovementInform(uint32 moveType, uint32 data);
        void OwnerDamagedBy(UnitPtr attacker);
        void OwnerAttacked(UnitPtr target);
        void ReceiveEmote(PlayerPtr player, uint32 textEmote);

    private:
        bool _isVisible(UnitPtr) const;
        bool _needToStop(void);
        void _stopAttack(void);

        void UpdateAllies();

        TimeTracker i_tracker;
        bool inCombat;
        std::set<uint64> m_AllySet;
        uint32 m_updateAlliesTimer;

        UnitPtr SelectNextTarget();
        void HandleReturnMovement();
        void DoAttack(UnitPtr target, bool chase);
        bool CanAttack(UnitPtr target);
};
#endif

