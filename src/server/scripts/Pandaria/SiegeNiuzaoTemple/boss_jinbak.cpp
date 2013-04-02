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
#include "siege_of_the_niuzoa_temple.h"

enum eSpells
{
    test = 119990,
    test2 = 120001
};

enum eEvents
{
};

class boss_jinbak : public CreatureScript
{
    public:
        boss_jinbak() : CreatureScript("boss_jinbak") {}

        struct boss_jinbakAI : public BossAI
        {
            boss_jinbakAI(Creature* creature) : BossAI(creature, DATA_JINBAK)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
            }

            void JustReachedHome()
            {
                instance->SetBossState(DATA_JINBAK, FAIL);
                summons.DespawnAll();
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {}

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

            void UpdateAI(const uint32 diff)
            {
                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_jinbakAI(creature);
        }
};

void AddSC_boss_jinbak()
{
    new boss_jinbak();
}
