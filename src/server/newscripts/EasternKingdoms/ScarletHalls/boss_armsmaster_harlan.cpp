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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scarlet_halls.h"

enum Says
{
    
};

enum Spells
{
    
};

class boss_armsmaster_harlan : public CreatureScript
{
public:
    boss_armsmaster_harlan() : CreatureScript("boss_armsmaster_harlan") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_armsmaster_harlanAI (creature);
    }

    struct boss_armsmaster_harlanAI : public ScriptedAI
    {
        boss_armsmaster_harlanAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset()
        {
            
        }

        void EnterCombat(Unit* /*who*/)
        {
            
        }

        void KilledUnit(Unit* /*victim*/)
        {
            
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            
        }

        void JustDied(Unit* /*killer*/)
        {
            
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_armsmaster_harlan()
{
    new boss_armsmaster_harlan();
}
