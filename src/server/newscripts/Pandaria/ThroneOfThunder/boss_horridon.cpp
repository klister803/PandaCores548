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
    //Horridon
    SPELL_RAMPAGE          = 136821, //targets - 1,0;
    SPELL_TRIPLE_PUNCTURE  = 136767,
    SPELL_DOUBLE_SWIPE     = 136741,
    SPELL_HORRIDON_CHARGE  = 136769,
    //Jalak
    SPELL_BESTIAL_CRY      = 136817,
};

enum sEvents
{
    //Horridon
    EVENT_TRIPLE_PUNCTURE  = 1,
    EVENT_DOUBLE_SWIPE     = 2,
    EVENT_CHARGES          = 3,
    //Jalak
    EVENT_INTRO            = 4,
    EVENT_BESTIAL_CRY      = 5,
};

enum sAction
{
    //Jalak
    ACTION_INTRO           = 1,
};

class boss_horridon : public CreatureScript
{
    public:
        boss_horridon() : CreatureScript("boss_horridon") {}

        struct boss_horridonAI : public BossAI
        {
            boss_horridonAI(Creature* creature) : BossAI(creature, DATA_HORRIDON)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            bool jalakintro;

            void Reset()
            {
                _Reset();
                jalakintro = false;
                me->SetReactState(REACT_AGGRESSIVE);
                Jalak_Reset();
            }

            void Jalak_Reset()
            {
                if (instance)
                {
                    if (Creature* jalak = me->GetCreature(*me, instance->GetData64(NPC_JALAK)))
                    {
                        if (!jalak->isAlive())
                        {
                            jalak->Respawn();
                            jalak->GetMotionMaster()->MoveTargetedHome();
                        }
                        else if (jalak->isInCombat())
                            jalak->AI()->EnterEvadeMode();
                    }
                }
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_TRIPLE_PUNCTURE, urand(11000, 15000));
                events.ScheduleEvent(EVENT_DOUBLE_SWIPE,    urand(17000, 20000));
                events.ScheduleEvent(EVENT_CHARGES,         urand(50000, 60000));
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (HealthBelowPct(30) && !jalakintro)
                {
                    jalakintro = true;
                    if (instance)
                    {
                        if (Creature* jalak = me->GetCreature(*me, instance->GetData64(NPC_JALAK)))
                            jalak->AI()->DoAction(ACTION_INTRO);
                    }
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() ||me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_TRIPLE_PUNCTURE:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_TRIPLE_PUNCTURE);
                        events.ScheduleEvent(EVENT_TRIPLE_PUNCTURE, urand(11000, 15000));
                        break;
                    case EVENT_DOUBLE_SWIPE:
                        DoCast(me, SPELL_DOUBLE_SWIPE);
                        events.ScheduleEvent(EVENT_DOUBLE_SWIPE, urand(17000, 20000));
                        break;
                    case EVENT_CHARGES:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                            DoCast(target, SPELL_HORRIDON_CHARGE);
                        events.ScheduleEvent(EVENT_CHARGES, urand(50000, 60000));
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_horridonAI(creature);
        }
};

class boss_jalak : public CreatureScript
{
    public:
        boss_jalak() : CreatureScript("boss_jalak") {}

        struct boss_jalakAI : public CreatureAI
        {
            boss_jalakAI(Creature* creature) : CreatureAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                Horridon_Reset();
            }

            void Horridon_Reset()
            {
                if (instance)
                {
                    if (Creature* horridon = me->GetCreature(*me, instance->GetData64(NPC_HORRIDON)))
                    {
                        if (!horridon->isAlive())
                        {
                            horridon->Respawn();
                            horridon->GetMotionMaster()->MoveTargetedHome();
                        }
                    }
                }
            }

            void EnterCombat(Unit* who)
            {
                events.ScheduleEvent(EVENT_BESTIAL_CRY, 5000);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE && type != EFFECT_MOTION_TYPE)
                    return;

                if (pointId == 0)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 150.0f);
                }
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_INTRO)
                    me->GetMotionMaster()->MoveJump(5433.32f, 5745.32f, 129.6066f, 25.0f, 25.0f, 0);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (instance)
                {
                    if (Creature* horridon = me->GetCreature(*me, instance->GetData64(NPC_HORRIDON)))
                        horridon->AddAura(SPELL_RAMPAGE, horridon);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    if (eventId == EVENT_BESTIAL_CRY)
                    {
                        DoCast(me, SPELL_BESTIAL_CRY);
                        events.ScheduleEvent(EVENT_BESTIAL_CRY, 10000);
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_jalakAI(creature);
        }
};

class spell_horridon_charge : public SpellScriptLoader
{
    public:
        spell_horridon_charge() : SpellScriptLoader("spell_horridon_charge") { }

        class spell_horridon_charge_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_horridon_charge_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster())
                    GetCaster()->CastSpell(GetCaster(), SPELL_DOUBLE_SWIPE);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_horridon_charge_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_horridon_charge_AuraScript();
        }
};

void AddSC_boss_horridon()
{
    new boss_horridon();
    new boss_jalak();
    new spell_horridon_charge();
}
