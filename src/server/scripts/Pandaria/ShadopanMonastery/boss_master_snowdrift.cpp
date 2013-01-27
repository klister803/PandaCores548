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
#include "shadopan_monastery.h"

enum eSpells
{};

enum eEvents
{};

enum eActions
{};

enum ePhases
{};

class boss_master_snowdrift : public CreatureScript
{
    public:
        boss_master_snowdrift() : CreatureScript("boss_master_snowdrift") {}

        struct boss_master_snowdriftAI : public BossAI
        {
            boss_master_snowdriftAI(Creature* creature) : BossAI(creature, DATA_MASTER_SNOWDRIFT)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();
            }

            void JustReachedHome()
            {
                pInstance->SetBossState(DATA_GU_CLOUDSTRIKE, FAIL);
                summons.DespawnAll();
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {}

            void DoAction(const int32 action)
            {}

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

            void EnterCombat(Unit* who)
            {}

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_master_snowdriftAI(creature);
        }
};

void AddSC_boss_master_snowdrift()
{
    new boss_master_snowdrift();
}
