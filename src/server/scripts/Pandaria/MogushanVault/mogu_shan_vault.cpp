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
#include "mogu_shan_vault.h"

enum spells
{
    SPIRIT_BOLT = 121224,
    GROUND_SLAM = 121087,
};

class mob_cursed_mogu_sculture : public CreatureScript
{
    public:
        mob_cursed_mogu_sculture() : CreatureScript("mob_cursed_mogu_sculture") {}

        struct mob_cursed_mogu_scultureAI : public ScriptedAI
        {
            mob_cursed_mogu_scultureAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            uint32 spiritBoltTimer;
            uint32 groundSlamTimer;

            void Reset()
            {
                spiritBoltTimer = urand(5000, 8000);
                groundSlamTimer = urand(8000, 10500);

                me->AddAura(120661, me);
                me->AddAura(120613, me);
            }

            void EnterCombat(Unit* attacker)
            {
                me->RemoveAurasDueToSpell(120661);
                me->RemoveAurasDueToSpell(120613);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (spiritBoltTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                        if (!target->IsFriendlyTo(me))
                            me->CastSpell(target, SPIRIT_BOLT, true);
                    spiritBoltTimer = urand(5000, 8000);
                }
                else
                    spiritBoltTimer -= diff;
                 
                if (groundSlamTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                        if (!target->IsFriendlyTo(me))
                            me->CastSpell(target, GROUND_SLAM, true);
                    groundSlamTimer = urand(8000, 10500);
                }
                else
                    groundSlamTimer -= diff;

                DoMeleeAttackIfReady();
            }
            
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_cursed_mogu_scultureAI(creature);
        }
};

void AddSC_mogu_shan_vault()
{
    new mob_cursed_mogu_sculture();
}