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
    PETRIFICATION = 125090,
    PETRIFIED = 125092,
    FULLY_PETRIFIED = 115877,
    MONSTROUS_BITE = 125096,
    SUNDERING_BITE = 116970,
    PROTECTIVE_FENZY = 116982,
    SHATTERING_STONE = 116977,
    FOCUSED_ASSAULT = 116990,
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

class mob_enormous_stone_quilen : public CreatureScript
{
    public:
        mob_enormous_stone_quilen() : CreatureScript("mob_enormous_stone_quilen") {}

        struct mob_enormous_stone_quilenAI : public ScriptedAI
        {
            mob_enormous_stone_quilenAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            uint32 monstrousBiteTimer;

            void Reset()
            {
                monstrousBiteTimer = urand (3000, 5000);
            }

            void EnterCombat(Unit* attacker)
            {
                me->AddAura(PETRIFICATION, me);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (monstrousBiteTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                    {
                        if (!target->IsFriendlyTo(me))
                            me->CastSpell(target, MONSTROUS_BITE, true);
                    }
                    monstrousBiteTimer = urand(6000, 8000);
                }
                else
                    monstrousBiteTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_enormous_stone_quilenAI(creature);
        }
};

class mob_stone_quilen : public CreatureScript
{
    public:
        mob_stone_quilen() : CreatureScript("mob_stone_quilen") {}

        struct mob_stone_quilenAI : public ScriptedAI
        {
            mob_stone_quilenAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            uint32 sunderingBiteTimer;
            uint32 shatteringStoneTimer;
            uint32 focusedAssaultTimer;

            void Reset()
            {
                sunderingBiteTimer = urand (5000, 6000);
                shatteringStoneTimer = urand (10000, 12000);
                focusedAssaultTimer = urand (500, 5000);
            }

            void EnterCombat(Unit* attacker)
            {
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->GetHealth() <= me->GetMaxHealth()/10)
                    me->CastSpell(me, PROTECTIVE_FENZY, true);

                if (focusedAssaultTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                    {
                        me->AddAura(FOCUSED_ASSAULT, target);
                    }
                    focusedAssaultTimer = urand(500, 5000);
                }
                else
                    shatteringStoneTimer -= diff;

                if (shatteringStoneTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                    {
                        me->CastSpell(target, SHATTERING_STONE, true);
                    }
                    shatteringStoneTimer = urand(10000, 12000);
                }
                else
                    shatteringStoneTimer -= diff;

                if (sunderingBiteTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                    {
                        me->CastSpell(target, SUNDERING_BITE, true);
                    }
                    sunderingBiteTimer = urand(5000, 6000);
                }
                else
                    sunderingBiteTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_stone_quilenAI(creature);
        }
};

class spell_mogu_petrification : public SpellScriptLoader
{
    public:
        spell_mogu_petrification() : SpellScriptLoader("spell_mogu_petrification") { }

        class spell_mogu_petrification_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mogu_petrification_AuraScript);

            uint32 stack;

            void OnApply(constAuraEffectPtr aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetTarget())
                    {
                        if (target->HasAura(PETRIFIED))
                        {
                            stack = GetTarget()->GetAura(PETRIFIED)->GetStackAmount();

                            if (stack >= 100)
                                target->AddAura(FULLY_PETRIFIED, target);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_mogu_petrification_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_DECREASE_SPEED, AURA_EFFECT_HANDLE_REAPPLY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mogu_petrification_AuraScript();
        }
};

class spell_mogu_monstrous_bite : public SpellScriptLoader
{
    public:
        spell_mogu_monstrous_bite() : SpellScriptLoader("spell_mogu_monstrous_bite") { }

        class spell_mogu_monstrous_bite_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mogu_monstrous_bite_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        SetHitDamage(uint32(GetHitDamage() * 2));
                }
            }
            void Register()
            {
                OnHit += SpellHitFn(spell_mogu_monstrous_bite_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mogu_monstrous_bite_SpellScript();
        }
};

void AddSC_mogu_shan_vault()
{
    new mob_cursed_mogu_sculture();
    new mob_enormous_stone_quilen();
    new mob_stone_quilen();
    new spell_mogu_petrification();
    new spell_mogu_monstrous_bite();
}