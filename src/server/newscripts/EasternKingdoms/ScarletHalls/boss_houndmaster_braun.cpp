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

class boss_houndmaster_braun : public CreatureScript
{
public:
    boss_houndmaster_braun() : CreatureScript("boss_houndmaster_braun") { }

    struct boss_houndmaster_braunAI : public ScriptedAI
    {
        boss_houndmaster_braunAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset()
        {
            
        }

        void EnterCombat(Unit* /*who*/)
        {
            
        }

        void KilledUnit(Unit* /*Victim*/)
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

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_houndmaster_braunAI (creature);
    }
};

void AddSC_boss_houndmaster_braun()
{
    new boss_houndmaster_braun();
}
