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
    SPELL_GAZE            = 134029,
    SPELL_HARD_STARE      = 133765,
    SPELL_OBLITERATE      = 137747,
    SPELL_LINGERING_GAZE  = 134040,
    SPELL_ARTERIAL_CUT    = 133768,
    SPELL_FORCE_OF_WILL   = 136413,
};

enum sEvents
{
    EVENT_ENRAGE          = 1,
    EVENT_HARD_STARE      = 2,
    EVENT_FORCE_OF_WILL   = 3,
};

enum sActions
{
    ACTION_RE_ATTACK      = 1,
};

class boss_durumu : public CreatureScript
{
    public:
        boss_durumu() : CreatureScript("boss_durumu") {}

        struct boss_durumuAI : public BossAI
        {
            boss_durumuAI(Creature* creature) : BossAI(creature, DATA_DURUMU)
            {
                instance = creature->GetInstanceScript();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* instance;
            uint32 checkvictim;

            void Reset()
            {
                _Reset();
                me->SetReactState(REACT_DEFENSIVE);
                checkvictim = 3000;
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_ENRAGE, 600000);
                events.ScheduleEvent(EVENT_HARD_STARE, 12000);
                events.ScheduleEvent(EVENT_FORCE_OF_WILL, 20000);
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_RE_ATTACK)
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 150.0f);
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                
                if (checkvictim <= diff)
                {
                    if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                    {
                        if (me->GetDistance(me->getVictim()) < 60.0f)
                            DoCastAOE(SPELL_GAZE);
                        else
                            EnterEvadeMode();
                    }
                    checkvictim = 3000;
                }
                else checkvictim -= diff;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_HARD_STARE:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_HARD_STARE);
                        events.ScheduleEvent(EVENT_HARD_STARE, 12000);
                        break;
                    case EVENT_ENRAGE:
                        DoCastAOE(SPELL_OBLITERATE);
                        break;
                    case EVENT_FORCE_OF_WILL:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                        {
                            me->AttackStop();
                            me->SetReactState(REACT_PASSIVE);
                            me->SetFacingToObject(target);
                            DoCast(target, SPELL_FORCE_OF_WILL);
                        }
                        events.ScheduleEvent(EVENT_FORCE_OF_WILL, 20000);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_durumuAI(creature);
        }
};

//133768
class spell_arterial_cut : public SpellScriptLoader
{
    public:
        spell_arterial_cut() : SpellScriptLoader("spell_arterial_cut") { }

        class spell_arterial_cut_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_arterial_cut_AuraScript)

            void HandlePeriodicTick(AuraEffect const * /*aurEff*/)
            {
                if (GetTarget())
                {
                    if (GetTarget()->GetHealth() == GetTarget()->GetMaxHealth())
                        GetTarget()->RemoveAurasDueToSpell(SPELL_ARTERIAL_CUT);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_arterial_cut_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_arterial_cut_AuraScript();
        }
};

//136413
//reset aggro and melee attack after cast
class spell_force_of_will : public SpellScriptLoader
{
    public:
        spell_force_of_will() : SpellScriptLoader("spell_force_of_will") { }
        
        class spell_force_of_will_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_force_of_will_SpellScript);
            
            void OnAfterCast()
            {
                if (GetCaster() && GetCaster()->ToCreature())
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_force_of_will_SpellScript::OnAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_force_of_will_SpellScript();
        }
};

void AddSC_boss_durumu()
{
    new boss_durumu();
    new spell_arterial_cut();
    new spell_force_of_will();
}
