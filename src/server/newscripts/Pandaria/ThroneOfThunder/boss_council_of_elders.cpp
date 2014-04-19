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

#include "NewScriptPCH.h"
#include "throne_of_thunder.h"

enum eSpells
{
};

uint32 councilentry[4] =
{
    NPC_FROST_KING_MALAKK,
    NPC_PRINCESS_MARLI,
    NPC_KAZRAJIN,
    NPC_SUL_SANDCRAWLER,
};

void CheckDone(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        if (Creature* council = caller->GetCreature(*caller, instance->GetData64(callerEntry)))
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, council);
    }

    uint8 donecount = 0;
    for (uint8 n = 0; n < 4; n++)
    {
        if (Creature* pcouncil = caller->GetCreature(*caller, instance->GetData64(councilentry[n])))
        {
            if (!pcouncil->isAlive())
                donecount++;
        }
    }

    if (donecount == 4)
        instance->SetBossState(DATA_COUNCIL_OF_ELDERS, DONE);
}

class boss_council_of_elders : public CreatureScript
{
    public:
        boss_council_of_elders() : CreatureScript("boss_council_of_elders") {}

        struct boss_council_of_eldersAI : public ScriptedAI
        {
            boss_council_of_eldersAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                if (instance)
                    instance->SetBossState(DATA_COUNCIL_OF_ELDERS, NOT_STARTED);
            }

            void EnterCombat(Unit* who)
            {
                if (instance)
                    instance->SetBossState(DATA_COUNCIL_OF_ELDERS, IN_PROGRESS);
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
            }

            void DoAction(int32 const action)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
                CheckDone(instance, me, me->GetEntry());
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
            return new boss_council_of_eldersAI(creature);
        }
};

void AddSC_boss_council_of_elders()
{
    new boss_council_of_elders();
}
