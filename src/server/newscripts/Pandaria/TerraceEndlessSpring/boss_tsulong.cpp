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
#include "terrace_of_endless_spring.h"

enum eSpells
{
    //Tsulong
    //Night phase
    SPELL_DREAD_SHADOWS       = 122767,
    SPELL_DREAD_SHADOWS_TR_EF = 122768,
    SPELL_NIGHTMARES          = 122770,
    SPELL_SHADOW_BREATH       = 122752,
    SPELL_SUNBEAM             = 122782,
    SPELL_SUNBEAM_TR_EF       = 122789,
    //Day phase
    SPELL_SUN_BREATH          = 122855,
    SPELL_BATHED_INLIGHT      = 122858,
    //
    //Sha Summons
};

enum eSummons
{
    NPC_SUNBEAM               = 90740,
};

enum eEvents
{
    EVENT_SHADOW_BREATH       = 1,
    EVENT_NIGHTMARE           = 2,
    EVENT_SUNBEAM             = 3,
    EVENT_SUN_BREATH          = 4,
};

enum Phase
{
    PHASE_NULL           = 0, 
    PHASE_NIGHT          = 1,
    PHASE_DAY            = 2,
};

enum Action
{
    ACTION_INTRO_DAY     = 1,
    ACTION_INTRO_NIGHT   = 2,
};

Position const sunbeampos[4] = 
{
    {-1017.82f, -3078.43f, 12.5717f},
    {-1017.74f, -3023.0f,  12.5803f},
    {-992.80f,  -3049.35f, 12.5840f},
    {-1044.39f, -3049.58f, 12.5793f},
};

enum DisplayId
{
    NIGHT_ID = 42532,
    DAY_ID   = 42533,
};

bool CheckProtectors(InstanceScript* instance, Creature* caller)
{
    if (instance && caller)
    {
        if (Creature* pr = caller->GetCreature(*caller, instance->GetData64(NPC_PROTECTOR_KAOLAN)))
        {
            if (pr->isAlive())
                return true;
        }
    }
    return false;
}

class boss_tsulong : public CreatureScript
{
    public:
        boss_tsulong() : CreatureScript("boss_tsulong") {}

        struct boss_tsulongAI : public BossAI
        {
            boss_tsulongAI(Creature* creature) : BossAI(creature, DATA_TSULONG)
            {
                instance = creature->GetInstanceScript();
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ENERGIZE, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_ENERGIZE, true);
                if (instance)
                {
                    if (CheckProtectors(instance, me))
                    {
                        me->SetVisible(false);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
            }

            InstanceScript* instance;
            uint32 checkpower, lowpower;
            bool done;
            Phase phase;

            void Reset()
            {              
                _Reset();
                phase = PHASE_NULL;
                done = false;
                lowpower = 0;
                checkpower = 0;
                me->setFaction(16);
                me->SetDisplayId(NIGHT_ID);
                me->SetReactState(REACT_DEFENSIVE);
                me->setPowerType(POWER_ENERGY);
                me->SetPower(POWER_ENERGY, 0);
                me->RemoveAurasDueToSpell(SPELL_DREAD_SHADOWS);
            }
            
            void RegeneratePower(Powers power, float &value)
            {
                if (phase == PHASE_NIGHT)
                    value = 2;
                else
                    value = 0;
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_INTRO_DAY:
                    me->RemoveAurasDueToSpell(SPELL_DREAD_SHADOWS);
                    me->setFaction(35);
                    me->SetDisplayId(DAY_ID);
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->GetMotionMaster()->MoveTargetedHome();
                    me->SetHealth(me->GetMaxHealth() - me->GetHealth());
                    lowpower = 1000;
                    events.ScheduleEvent(EVENT_SUN_BREATH,    urand(10000, 15000));
                    break;
                case ACTION_INTRO_NIGHT:
                    me->SetHealth(me->GetMaxHealth() - me->GetHealth());
                    me->setFaction(16);
                    me->SetDisplayId(NIGHT_ID);
                    me->SetReactState(REACT_AGGRESSIVE);
                    if (!checkpower)
                        checkpower = 1000;
                    DoZoneInCombat(me, 150.0f);
                    if (me->getVictim())
                        me->GetMotionMaster()->MoveChase(me->getVictim());
                    me->AddAura(SPELL_DREAD_SHADOWS, me);
                    events.ScheduleEvent(EVENT_SHADOW_BREATH, urand(25000, 35000));
                    events.ScheduleEvent(EVENT_NIGHTMARE,     urand(15000, 25000));
                    events.ScheduleEvent(EVENT_SUNBEAM,       urand(15000, 20000));
                    break;
                }
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                events.SetPhase(PHASE_NIGHT);
                phase = PHASE_NIGHT;
                checkpower = 1000; 
                me->AddAura(SPELL_DREAD_SHADOWS, me);
                events.ScheduleEvent(EVENT_SHADOW_BREATH, urand(25000, 35000));
                events.ScheduleEvent(EVENT_NIGHTMARE,     urand(15000, 25000));
                events.ScheduleEvent(EVENT_SUNBEAM,       urand(15000, 20000));
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (phase == PHASE_NIGHT)
                {
                    if (damage >= me->GetHealth() && !done)
                    {
                        done = true;
                        damage = 0;
                        SendDone();
                    }
                }
            }

            void SendDone()
            {
                summons.DespawnAll();
                me->SetVisible(false);
                me->Kill(me, true);
                if (instance)
                {
                    instance->SetBossState(DATA_TSULONG, DONE);
                    if (Creature* leishi = me->GetCreature(*me, instance->GetData64(NPC_LEI_SHI)))
                    {
                        leishi->SetVisible(true);
                        leishi->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
                if(!me->GetMap()->IsLfr())
                    me->SummonGameObject(212922, me->GetPositionX() + 6, me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 604800);
            }

            void JustDied(Unit* killer)
            {
                me->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (checkpower)
                {
                    if (checkpower <= diff)
                    {
                        if (me->GetPower(POWER_ENERGY) == 100 && phase == PHASE_NIGHT)
                        {
                            phase = PHASE_DAY;
                            checkpower = 0; 
                            events.Reset();
                            DoAction(ACTION_INTRO_DAY);
                        }
                    }
                    else 
                        checkpower -= diff;
                }

                if (lowpower && phase == PHASE_DAY)
                {
                    if (lowpower <= diff)
                    {
                        if (instance)
                        {
                            if (instance->IsWipe())
                            {
                                EnterEvadeMode();
                                return;
                            }
                        }

                        if (me->GetHealth() == me->GetMaxHealth() && !done)
                        {
                            done = true;
                            SendDone();
                            return;
                        }

                        if (me->GetPower(POWER_ENERGY) >= 1)
                            me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) - 1);

                        if (me->GetPower(POWER_ENERGY) == 0 && phase == PHASE_DAY)
                        {
                            phase = PHASE_NIGHT;
                            lowpower = 0;
                            events.Reset();
                            DoAction(ACTION_INTRO_NIGHT);
                        }
                        else
                            lowpower = 1000;
                    }
                    else
                        lowpower -= diff;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_SHADOW_BREATH:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_SHADOW_BREATH);
                        events.ScheduleEvent(EVENT_SHADOW_BREATH, urand(25000, 35000));
                        break;
                    case EVENT_NIGHTMARE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                            DoCast(target, SPELL_NIGHTMARES);
                        events.ScheduleEvent(EVENT_NIGHTMARE, urand(15000, 25000));
                        break;
                    case EVENT_SUNBEAM:
                        me->SummonCreature(NPC_SUNBEAM, sunbeampos[urand(0, 3)]);
                        events.ScheduleEvent(EVENT_SUNBEAM, urand(15000, 20000));                      
                        break;
                    case EVENT_SUN_BREATH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        {
                            me->SetFacingToObject(target);
                            DoCastAOE(SPELL_SUN_BREATH);
                        }
                        events.ScheduleEvent(EVENT_SUN_BREATH, urand(10000, 15000));
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_tsulongAI(creature);
        }
};

class npc_sunbeam : public CreatureScript
{
    public:
        npc_sunbeam() : CreatureScript("npc_sunbeam") {}

        struct npc_sunbeamAI : public ScriptedAI
        {
            npc_sunbeamAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            }
            InstanceScript* instance;
            uint8 hitval;
            uint8 maxhitval;

            void Reset()
            {
                hitval = 0;
                maxhitval = 0;
                SetMaxHitVal();
                me->SetFloatValue(OBJECT_FIELD_SCALE_X, 2.0f);
                me->AddAura(SPELL_SUNBEAM, me);
            }

            void SetMaxHitVal()
            {
                switch (me->GetMap()->GetDifficulty())
                {
                case MAN10_DIFFICULTY :
                case MAN10_HEROIC_DIFFICULTY:
                    maxhitval = 20;
                    break;
                case MAN25_DIFFICULTY:
                case MAN25_HEROIC_DIFFICULTY:
                    maxhitval = 50;
                    break;
                }
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (maxhitval)
                {
                    if (spell->Id == SPELL_SUNBEAM_TR_EF && target->GetTypeId() == TYPEID_PLAYER && hitval < maxhitval)
                        hitval++;
                    
                    if (hitval == maxhitval)
                    {
                        maxhitval = 0;
                        me->DespawnOrUnsummon();
                    }
                }
            }

            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void UpdateAI(uint32 diff){}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sunbeamAI(creature);
        }
};

class spell_sunbeam : public SpellScriptLoader
{
    public:
        spell_sunbeam() : SpellScriptLoader("spell_sunbeam") { }

        class spell_sunbeam_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sunbeam_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget())
                {
                    if (GetTarget()->HasAura(SPELL_DREAD_SHADOWS_TR_EF))
                        GetTarget()->RemoveAurasDueToSpell(SPELL_DREAD_SHADOWS_TR_EF);
                    GetTarget()->ApplySpellImmune(SPELL_DREAD_SHADOWS_TR_EF, 0, 0, true);
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget())
                    GetTarget()->ApplySpellImmune(SPELL_DREAD_SHADOWS_TR_EF, 0, 0, false);
            }

            void Register()
            {
                OnEffectApply     += AuraEffectApplyFn(spell_sunbeam_AuraScript::OnApply,   EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                AfterEffectRemove += AuraEffectRemoveFn(spell_sunbeam_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sunbeam_AuraScript();
        }
};

class spell_sun_breath : public SpellScriptLoader
{
    public:
        spell_sun_breath() : SpellScriptLoader("spell_sun_breath") { }

        class spell_sun_breath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sun_breath_SpellScript);

            void DealDamage()
            {
                if (GetCaster() && GetHitUnit())
                {
                    if (GetHitUnit()->ToPlayer())
                    {
                        SetHitDamage(0);
                        GetCaster()->CastSpell(GetHitUnit(), SPELL_BATHED_INLIGHT);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sun_breath_SpellScript::DealDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sun_breath_SpellScript();
        }
};

void AddSC_boss_tsulong()
{
    new boss_tsulong();
    new npc_sunbeam();
    new spell_sunbeam();
    new spell_sun_breath();
}
