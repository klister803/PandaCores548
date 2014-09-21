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
#include "siege_of_orgrimmar.h"
#include "CreatureTextMgr.h"

enum eSpells
{
    SPELL_BOUND_OF_GOLDEN_LOTUS         = 143497, //Bond of the Golden Lotus

    //Rook
    SPELL_VENGEFUL_STRIKE               = 144396, //Vengeful Strikes
    SPELL_CORRUPTED_BREW                = 143019, //Corrupted Brew
    SPELL_CLASH                         = 143027, //Clash   cast 143028
    SPELL_CORRUPTION_KICK               = 143007, //Corruption Kick
    SPELL_MISERY_SORROW_GLOOM           = 143955, //Misery, Sorrow, and Gloom

    //He
    SPELL_GARROTE                       = 143198, //Garrote
    SPELL_GOUGE                         = 143301, //Gouge
    SPELL_NOXIOUS_POISON                = 143225, //Noxious Poison
    SPELL_INSTANT_POISON                = 143210, //Instant Poison

    //Sun
    SPELL_SHA_SEAR                      = 143423, //Sha Sear
    SPELL_SHADOW_WORD_BANE              = 143434, //Shadow Word: Bane
    SPELL_CALAMITY                      = 143491, //Calamity
};

enum Phases
{
    PHASE_BATTLE                    = 1,
    PHASE_DESPERATE_MEASURES        = 2,
    PHASE_BOND_GOLDEN_LOTUS         = 3
};


struct boss_fallen_protectors : public BossAI
{
    boss_fallen_protectors(Creature* creature) : BossAI(creature, DATA_F_PROTECTORS)
    {
        _healthPhase = 0;
    }

    int8 _healthPhase;

    void Reset()
    {
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
    }

    void EnterCombat(Unit* who)
    {
        events.SetPhase(PHASE_BATTLE);
    }

    void HealReceived(Unit* /*done_by*/, uint32& addhealth)
    {
        float newpct = GetHealthPctWithHeal(addhealth);
        if ((_healthPhase == 0 && newpct <= 66.0f) ||
            (_healthPhase == 1 && newpct <= 33.0f))
            --_healthPhase;

        events.SetPhase(PHASE_BATTLE);
    }

    void DamageTaken(Unit* attacker, uint32 &damage)
    {
        if (me->GetHealth() <= 1 || int(me->GetHealth() - damage) <= 1)
        {
            damage = me->GetHealth() - damage;

            if (!events.IsInPhase(PHASE_BOND_GOLDEN_LOTUS))
            {
                me->InterruptNonMeleeSpells(false);
                DoCast(me, SPELL_BOUND_OF_GOLDEN_LOTUS, false);

                events.SetPhase(PHASE_BOND_GOLDEN_LOTUS);
                events.RescheduleEvent(EVENT_1, 1*IN_MILLISECONDS, 0, PHASE_BOND_GOLDEN_LOTUS);   //BreakIfAny
            }
            return;
        }

        if ((_healthPhase == 0 && GetHealthPct(damage) <= 66.0f) ||
            (_healthPhase == 1 && GetHealthPct(damage) <= 33.0f))
        {
            ++_healthPhase;
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            events.SetPhase(PHASE_DESPERATE_MEASURES);
        }
    }

    void DoAction(int32 const action)
    {
        switch(action)
        {
            //Check if all boses have 1 pct. If == true -> setbossstate(DONE)
            case EVENT_1:
                events.RescheduleEvent(EVENT_1, 1*IN_MILLISECONDS, 0, PHASE_BOND_GOLDEN_LOTUS);   //BreakIfAny
                break;
        }
    }

    void JustDied(Unit* /*killer*/)
    {
    }
};

//Rook Stonetoe
class boss_rook_stonetoe : public CreatureScript
{
    public:
        boss_rook_stonetoe() : CreatureScript("boss_rook_stonetoe") {}

        struct boss_rook_stonetoeAI : public boss_fallen_protectors
        {
            boss_rook_stonetoeAI(Creature* creature) : boss_fallen_protectors(creature)
            {
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
            }

            enum local
            {
                EVENT_VENGEFUL_STRIKE   = 5,
                EVENT_CORRUPTED_BREW    = 6,
                EVENT_CLASH             = 7,
            };

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);

                events.RescheduleEvent(EVENT_VENGEFUL_STRIKE, urand(10*IN_MILLISECONDS, 20*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_CORRUPTED_BREW, urand(IN_MILLISECONDS, 5*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_CLASH, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
            }

            /*REMOVE IT AFTER COMPLETE*/
            void AttackStart(Unit* target)
            {
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    boss_fallen_protectors::DoAction(eventId);
                    switch (eventId)
                    {
                        case EVENT_VENGEFUL_STRIKE:
                            DoCastVictim(SPELL_VENGEFUL_STRIKE);
                            events.RescheduleEvent(EVENT_VENGEFUL_STRIKE, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_CORRUPTED_BREW:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 0.0f, true))
                                DoCast(target, SPELL_CORRUPTED_BREW, true);
                            events.RescheduleEvent(EVENT_CORRUPTED_BREW, urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_CLASH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_CLASH);
                            //DoCastVictim(SPELL_CLASH);
                            events.RescheduleEvent(EVENT_CLASH, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rook_stonetoeAI(creature);
        }
};

//He Softfoot
class boss_he_softfoot : public CreatureScript
{
    public:
        boss_he_softfoot() : CreatureScript("boss_he_softfoot") {}

        struct boss_he_softfootAI : public boss_fallen_protectors
        {
            boss_he_softfootAI(Creature* creature) : boss_fallen_protectors(creature)
            {
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GARROTE);
                me->RemoveAllAreaObjects();
            }

            enum local
            {
                EVENT_GARROTE                   = 5,
                EVENT_GOUGE                     = 6,
                EVENT_POISON_NOXIOUS            = 7,
                EVENT_POISON_INSTANT            = 8,
            };

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);

                events.RescheduleEvent(EVENT_GARROTE, 5*IN_MILLISECONDS, 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_GOUGE, urand(IN_MILLISECONDS, 5*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_POISON_NOXIOUS, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                DoCast(who, SPELL_INSTANT_POISON, false);
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            bool AllowSelectNextVictim(Unit* target)
            {
                // Go next raid member.
                return !target->HasUnitState(UNIT_STATE_STUNNED);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

               /* while (uint32 eventId = events.ExecuteEvent())
                {
                    boss_fallen_protectors::DoAction(eventId);
                    switch (eventId)
                    {
                        case EVENT_GARROTE:
                            if (Unit* target = me->getVictim())
                                if (!target->HasAura(SPELL_GARROTE))
                                    DoCast(target, SPELL_GARROTE, false);
                            events.RescheduleEvent(EVENT_GARROTE, 5*IN_MILLISECONDS, 0, PHASE_BATTLE);
                            break;
                        case EVENT_GOUGE:
                            DoCastVictim(SPELL_GOUGE);
                            events.RescheduleEvent(EVENT_GOUGE, urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_POISON_NOXIOUS:
                            events.RescheduleEvent(EVENT_POISON_INSTANT, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            DoCastVictim(SPELL_NOXIOUS_POISON);
                            break;
                        case EVENT_POISON_INSTANT:
                            DoCastVictim( SPELL_INSTANT_POISON);
                            events.RescheduleEvent(EVENT_POISON_NOXIOUS, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            me->RemoveAllAreaObjects();
                            break;
                    }
                }*/
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_he_softfootAI(creature);
        }
};

//Sun Tenderheart
class boss_sun_tenderheart : public CreatureScript
{
    public:
        boss_sun_tenderheart() : CreatureScript("boss_sun_tenderheart") {}

        struct boss_sun_tenderheartAI : public boss_fallen_protectors
        {
            boss_sun_tenderheartAI(Creature* creature) : boss_fallen_protectors(creature)
            {
                SetCombatMovement(false);
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
            }

            enum local
            {
                EVENT_SHA_SEAR            = 5,
                EVENT_SHADOW_WORD_BANE    = 6,
                EVENT_CALAMITY            = 7,
            };


            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);

                events.RescheduleEvent(EVENT_SHA_SEAR, urand(5*IN_MILLISECONDS, 10*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_SHADOW_WORD_BANE, urand(15*IN_MILLISECONDS, 25*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_CALAMITY, urand(60*IN_MILLISECONDS, 70*IN_MILLISECONDS), 0, PHASE_BATTLE);
            }

            /*REMOVE IT AFTER COMPLETE*/
            void AttackStart(Unit* target)
            {
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

              /*  while (uint32 eventId = events.ExecuteEvent())
                {
                    boss_fallen_protectors::DoAction(eventId);
                    switch (eventId)
                    {
                        case EVENT_SHA_SEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 0.0f, true))
                                DoCast(target, SPELL_SHA_SEAR, true);
                            events.RescheduleEvent(EVENT_SHA_SEAR, urand(5*IN_MILLISECONDS, 10*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_SHADOW_WORD_BANE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 0.0f, true))
                                DoCast(target, SPELL_SHADOW_WORD_BANE, true);
                            events.RescheduleEvent(EVENT_SHADOW_WORD_BANE, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_CALAMITY:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_2, 0);
                            DoCastVictim(SPELL_CALAMITY);
                            events.RescheduleEvent(EVENT_CALAMITY, urand(60*IN_MILLISECONDS, 70*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                    }
                }
                //DoMeleeAttackIfReady();
                }*/
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sun_tenderheartAI(creature);
        }
};

//Golden Lotus
class npc_golden_lotus_control : public CreatureScript
{
public:
    npc_golden_lotus_control() : CreatureScript("npc_golden_lotus_control") { }

    struct npc_wind_vehicleAI : public ScriptedAI
    {        
        npc_wind_vehicleAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();            
        }

        enum data
        {
            SUMMON_MOVER        = 143705,
            SPELL_FACE_CHANNEL  = 116351,
        };

        InstanceScript* instance;
        EventMap events;

        //void JustSummoned(Creature* creature)
        //{
        //    if (creature->GetEntry() == NPC_GOLD_LOTOS_MOVER)
        //    {
        //        creature->SetDisplayId(48920);
        //        creature->GetMotionMaster()->MoveIdle();
        //        creature->LoadPath(creature->GetEntry());
        //        creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        //        creature->GetMotionMaster()->Initialize();
        //        if (creature->isAlive())                            // dead creature will reset movement generator at respawn
        //        {
        //            creature->setDeathState(JUST_DIED);
        //            creature->Respawn(true);
        //        }
        //    }
        //};

        void Reset()
        {
            //Use manual spawn.
            //me->CastSpell(me, SUMMON_MOVER, true);
            events.RescheduleEvent(EVENT_1, 1000);
        }
        
        void OnCharmed(bool /*apply*/)
        {
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (apply)
            {
                if (Creature* mover = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MOVER)))
                {
                    who->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                    if (!me->HasAura(SPELL_FACE_CHANNEL))
                        me->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {

            }
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wind_vehicleAI(creature);
    }
};

class spell_clash : public SpellScriptLoader
{
    public:
        spell_clash() : SpellScriptLoader("spell_OO_clash") { }

        class spell_clash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_clash_SpellScript);

            enum proc
            {
                SPELL_PROCK     = 143028,
            };
            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        caster->CastSpell(target, SPELL_PROCK, false);
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_clash_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_clash_SpellScript();
        }
};

//Corrupted Brew
class spell_corrupted_brew : public SpellScriptLoader
{
    public:
        spell_corrupted_brew() : SpellScriptLoader("spell_OO_corrupted_brew") { }

        class spell_corrupted_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_corrupted_brew_SpellScript);

            enum proc
            {
                SPELL_PROCK     = 143021,
            };

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        caster->CastSpell(target, SPELL_PROCK, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_corrupted_brew_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }

        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_corrupted_brew_SpellScript();
        }
};

class spell_gouge : public SpellScriptLoader
{
    public:
        spell_gouge() : SpellScriptLoader("spell_OO_gouge") { }

        class spell_gouge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gouge_SpellScript);

            void HandleEffect(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();
                if (!target)
                    return;

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (target->HasInArc(static_cast<float>(M_PI), caster))
                {
                    caster->getThreatManager().modifyThreatPercent(target, -100);
                    target->DeleteFromThreatList(caster);
                }else
                {
                    PreventHitAura();
                }
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_gouge_SpellScript::HandleEffect, EFFECT_1, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gouge_SpellScript();
        }
};

void AddSC_boss_fallen_protectors()
{
    new boss_rook_stonetoe();
    new boss_he_softfoot();
    new boss_sun_tenderheart();
    new npc_golden_lotus_control();
    new spell_clash();
    new spell_corrupted_brew();
    new spell_gouge();
}
