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

#include "ScriptPCH.h"
#include "siege_of_orgrimmar.h"

enum eSpells
{
};

class boss_malkorok : public CreatureScript
{
    public:
        boss_malkorok() : CreatureScript("boss_malkorok") {}

        struct boss_malkorokAI : public ScriptedAI
        {
            boss_malkorokAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
            }

            void EnterCombat(Unit* who)
            {
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
            }

            void DoAction(int32 const action)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
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
            return new boss_malkorokAI(creature);
        }
};

void AddSC_boss_malkorok()
{
    new boss_malkorok();
}
