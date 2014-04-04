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
    SPELL_STATIC_BURST     = 137162,
    SPELL_STATIC_WOUND     = 138349,
    SPELL_BALL_LIGHTNING   = 136620,
    SPELL_LIGHTNING_STORM  = 137313,//AOE 
};

//This is entry triggers(maybe needed, but not need spawn)
//69593
//69509
//69676
//69609

class boss_jinrokh : public CreatureScript
{
    public:
        boss_jinrokh() : CreatureScript("boss_jinrokh") {}

        struct boss_jinrokhAI : public BossAI
        {
            boss_jinrokhAI(Creature* creature) : BossAI(creature, DATA_JINROKH)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
            }

            void DoAction(int32 const action)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
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
            return new boss_jinrokhAI(creature);
        }
};

void AddSC_boss_jinrokh()
{
    new boss_jinrokh();
}
