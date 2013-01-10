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
#include "gate_setting_sun.h"

enum eSpells
{
};

enum eEvents
{
};

class boss_raigonn : public CreatureScript
{
    public:
        boss_raigonn() : CreatureScript("boss_raigonn") {}

        struct boss_raigonnAI : public BossAI
        {
            boss_raigonnAI(Creature* creature) : BossAI(creature, DATA_RAIGONN)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            uint8 WorldInFlamesEvents;

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
                instance->SetBossState(DATA_RAIGONN, FAIL);
                summons.DespawnAll();
            }

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

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

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_raigonnAI(creature);
        }
};

void AddSC_boss_raigonn()
{
    new boss_raigonn();
}
