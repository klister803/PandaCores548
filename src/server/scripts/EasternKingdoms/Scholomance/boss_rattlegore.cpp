/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_the_ravenian
SD%Complete: 100
SDComment:
SDCategory: Scholomance
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scholomance.h"

class boss_rattlegore : public CreatureScript
{
public:
    boss_rattlegore() : CreatureScript("boss_rattlegore") { }

    struct boss_rattlegoreAI : public BossAI
    {
        boss_rattlegoreAI(Creature* creature) : BossAI(creature, DATA_RATTLEGORE) {}

        void Reset()
        {
           _Reset();
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_rattlegoreAI (creature);
    }

};

void AddSC_boss_rattlegore()
{
    new boss_rattlegore();
}
