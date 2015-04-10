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

enum eSpells
{
    SPELL_ERADICATE               = 143916,
    SPELL_FATAL_STRIKE            = 146254, 
    SPELL_ARCING_SMASH            = 142815, 
    SPELL_ARCING_SMASH_JUMP       = 142898,
    SPELL_ARCING_SMASH_DUMMY      = 142826,
    SPELL_ARCING_SMASH_AM_C       = 143805,
    SPELL_SEISMIC_SLAM            = 142849, 
    SPELL_BLOOD_RAGE              = 142879,
    SPELL_BLOOD_RAGE_DMG          = 142890,
    SPELL_RELENTLESS_ASSAULT      = 143261,
    SPELL_DISPLACED_ENERGY        = 142913,
    SPELL_DISPLACED_ENERGY_DMG    = 142928,
    SPELL_ANCIENT_BARRIER         = 142862, 
    SPELL_ANCIENT_MIASMA_H_A      = 142861, 
    SPELL_ANCIENT_MIASMA_DMG      = 142906,
    SPELL_ANCIENT_MIASMA          = 143018, 
    SPEEL_WEAK_ANCIENT_BARRIER    = 142863,
    SPELL_NORMAL_ANCIENT_BARRIER  = 142864,
    SPELL_STRONG_ANCIENT_BARRIER  = 142865,
    SPELL_BREATH_OF_YSHAARJ       = 142816,
    SPELL_BREATH_OF_YSHAARJD      = 142842,
    SPELL_IMPLODING_ENERGYV2      = 142980, 
    SPELL_IMPLODING_ENERGY1       = 142986, 
    SPELL_IMPLODING_ENERGY2       = 142987, 
    SPELL_IMPLODING_ENERGYV       = 144069, 
    SPELL_IMPLODING_ENERGYD       = 142988, 
    SPELL_EXPEL_MIASMA            = 143199,
};

enum Events
{
    EVENT_ERADICATE               = 1,
    EVENT_PREPARE                 = 2,
    EVENT_ARCING_SMASH            = 3,
    EVENT_SEISMIC_SLAM            = 4,
    EVENT_RE_ATTACK               = 5,
    EVENT_BREATH_OF_YSHAARJ       = 6,
    EVENT_EXPLOSE                 = 7,
    EVENT_BLOOD_RAGE              = 8,
};

enum Actions
{
    ACTION_PHASE_ONE,
    ACTION_PHASE_TWO,
    ACTION_RE_ATTACK,
    ACTION_BREATH_OF_YSHAARJ,
};

enum Phase
{
    PHASE_ONE,
    PHASE_TWO,
};

enum CreatureText
{
    SAY_PULL,
    SAY_ARCING_SMASH,
    SAY_ARSING_SMASH2,
    SAY_BREATH_OF_YSHAARJ,
    SAY_PHASE_TWO,
    SAY_DIED,
};

struct _ang
{
    float minang;
    float maxang;
};

_ang modang[3]
{
    {0.0f, 1.5f},
    {2.0f, 3.5f},
    {4.0f, 6.0f},
};

uint32 ancientbarrierbar[3] = 
{
    SPEEL_WEAK_ANCIENT_BARRIER,
    SPELL_NORMAL_ANCIENT_BARRIER,
    SPELL_STRONG_ANCIENT_BARRIER,
};

class boss_malkorok : public CreatureScript
{
    public:
        boss_malkorok() : CreatureScript("boss_malkorok") {}

        struct boss_malkorokAI : public BossAI
        {
            boss_malkorokAI(Creature* creature) : BossAI(creature, DATA_MALKOROK)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            std::vector<uint64> asGuids;
            uint32 powercheck, displacedenergy;
            Phase phase;

            void Reset()
            {
                _Reset();
                SetGasStateAndBuffPlayers(false);
                me->RemoveAurasDueToSpell(SPELL_BLOOD_RAGE);
                me->RemoveAurasDueToSpell(SPELL_FATAL_STRIKE);
                me->RemoveAurasDueToSpell(SPELL_RELENTLESS_ASSAULT);
                me->SetReactState(REACT_DEFENSIVE);
                me->setPowerType(POWER_ENERGY);
                me->SetPower(POWER_ENERGY, 0);
                asGuids.clear();
                phase = PHASE_ONE;
                displacedenergy = 0;
                powercheck = 0;
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                Talk(SAY_PULL);
                SetGasStateAndBuffPlayers(true);
                powercheck = 1300;
                DoCast(me, SPELL_FATAL_STRIKE, true);
                events.ScheduleEvent(EVENT_SEISMIC_SLAM, 5000);
                events.ScheduleEvent(EVENT_PREPARE, 13000);
                events.ScheduleEvent(EVENT_ERADICATE, 300000);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type == EFFECT_MOTION_TYPE)
                {
                    if (pointId == SPELL_ARCING_SMASH_JUMP)
                    {
                        float ang = float(urand(0, 6));
                        me->SetFacingTo(ang);
                        if (Creature* as = me->SummonCreature(NPC_ARCING_SMASH, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), ang))
                        {
                            Talk(urand(SAY_ARCING_SMASH, SAY_ARSING_SMASH2));
                            me->PlayOneShotAnimKit(4308);
                            as->CastSpell(as, SPELL_ARCING_SMASH);
                            asGuids.push_back(as->GetGUID());
                        }
                    }
                }
            }
            
            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_PHASE_ONE:
                    me->RemoveAurasDueToSpell(SPELL_BLOOD_RAGE);
                    DoCast(me, SPELL_RELENTLESS_ASSAULT, true);
                    DoCast(me, SPELL_EXPEL_MIASMA);
                    asGuids.clear();
                    powercheck = 1300;
                    DoCast(me, SPELL_FATAL_STRIKE, true);
                    SetGasStateAndBuffPlayers(true);
                    events.ScheduleEvent(EVENT_SEISMIC_SLAM, 5000);
                    events.ScheduleEvent(EVENT_PREPARE, 13000);
                    break;
                case ACTION_PHASE_TWO:
                    me->RemoveAurasDueToSpell(SPELL_FATAL_STRIKE);
                    events.CancelEvent(EVENT_PREPARE);
                    events.CancelEvent(EVENT_SEISMIC_SLAM);
                    Talk(SAY_PHASE_TWO);
                    DoCast(me, SPELL_BLOOD_RAGE);
                    powercheck = 1000;
                    SetGasStateAndBuffPlayers(false);
                    events.ScheduleEvent(EVENT_BLOOD_RAGE, 2000);
                    displacedenergy = 4000;
                    break;
                case ACTION_RE_ATTACK:
                    if (!asGuids.empty())
                        if (asGuids.size() == 3)
                            events.ScheduleEvent(EVENT_BREATH_OF_YSHAARJ, 10000);
                    if (Creature* am = me->GetCreature(*me, instance->GetData64(NPC_ANCIENT_MIASMA)))
                    {
                        float x, y;
                        for (uint8 n = 0; n < 3; n++)
                        {
                            GetPositionWithDistInOrientation(am, float(urand(15, 30)), urand(modang[n].minang, modang[n].maxang), x, y);
                            me->SummonCreature(NPC_IMPLOSION, x, y, am->GetPositionZ());
                        }
                    }
                    events.ScheduleEvent(EVENT_RE_ATTACK, 1000);
                    events.ScheduleEvent(EVENT_PREPARE, 20000);
                    break;
                case ACTION_BREATH_OF_YSHAARJ:
                    for (std::vector<uint64>::const_iterator itr = asGuids.begin(); itr != asGuids.end(); itr++)
                        if (Creature* as = me->GetCreature(*me, *itr))
                            as->AI()->DoAction(ACTION_BREATH_OF_YSHAARJ);
                    asGuids.clear();
                    events.ScheduleEvent(EVENT_RE_ATTACK, 1500);
                    break;
                }
            }

            void SetGasStateAndBuffPlayers(bool state)
            {
                if (!instance)
                    return;

                if (Creature* am = me->GetCreature(*me, instance->GetData64(NPC_ANCIENT_MIASMA)))
                {
                    if (state)
                    {
                        am->AddAura(SPELL_ANCIENT_MIASMA, am);
                        DoCast(me, SPELL_ANCIENT_MIASMA_H_A, true);
                    }
                    else
                    {
                        am->RemoveAurasDueToSpell(SPELL_ANCIENT_MIASMA);
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ANCIENT_MIASMA_H_A);
                    }                
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (powercheck)
                {
                    if (powercheck <= diff)
                    {
                        switch (phase)
                        {
                        case PHASE_ONE:
                            if (me->GetPower(POWER_ENERGY) <= 99)
                            {
                                me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 1);
                                powercheck = 1300;
                                if (me->GetPower(POWER_ENERGY) == 100)
                                {
                                    phase = PHASE_TWO;
                                    powercheck = NULL;
                                    DoAction(ACTION_PHASE_TWO);
                                }
                            }
                            break;
                        case PHASE_TWO:
                            powercheck = 1000;
                            if (me->GetPower(POWER_ENERGY) == 0)
                            {
                                phase = PHASE_ONE;
                                powercheck = NULL;
                                displacedenergy = 0;
                                events.CancelEvent(EVENT_BLOOD_RAGE);
                                DoAction(ACTION_PHASE_ONE);
                            }
                            break;
                        }
                    }
                    else
                        powercheck -= diff;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (displacedenergy)
                {
                    if (displacedenergy <= diff)
                    {
                        DoCastAOE(SPELL_DISPLACED_ENERGY, true);
                        displacedenergy = 15000;
                    }
                    else
                        displacedenergy -= diff;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_PREPARE:
                        me->PlayOneShotAnimKit(0);
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        me->StopMoving();
                        me->GetMotionMaster()->Clear(false);
                        DoCast(me, SPELL_ARCING_SMASH_JUMP);
                        break;
                    case EVENT_BREATH_OF_YSHAARJ:
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        me->StopMoving();
                        me->GetMotionMaster()->Clear(false);
                        Talk(SAY_BREATH_OF_YSHAARJ);
                        DoCast(me, SPELL_BREATH_OF_YSHAARJD);
                        break;
                    case EVENT_RE_ATTACK:
                        me->StopMoving();
                        me->GetMotionMaster()->Clear(false);
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (me->getVictim())
                            me->GetMotionMaster()->MoveChase(me->getVictim());
                        break;
                    case EVENT_ERADICATE:
                        DoCastAOE(SPELL_ERADICATE);
                        break;
                    case EVENT_BLOOD_RAGE:
                        DoCast(me, SPELL_BLOOD_RAGE_DMG, true);
                        events.ScheduleEvent(EVENT_BLOOD_RAGE, 2000);
                        break;
                    case EVENT_SEISMIC_SLAM:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                            DoCast(target, SPELL_SEISMIC_SLAM, true);
                        events.ScheduleEvent(EVENT_SEISMIC_SLAM, 15000);
                        break;
                    }
                }
                if (phase == PHASE_ONE)
                    DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DIED);
                _JustDied();
                SetGasStateAndBuffPlayers(false);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_malkorokAI(creature);
        }
};

//71455
class npc_arcing_smash : public CreatureScript
{
public:
    npc_arcing_smash() : CreatureScript("npc_arcing_smash") {}

    struct npc_arcing_smashAI : public ScriptedAI
    {
        npc_arcing_smashAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;
        EventMap events;

        void DoAction(int32 const action)
        {
            if (action == ACTION_BREATH_OF_YSHAARJ)
            {
                DoZoneInCombat(me, 150.0f);
                DoCast(me, SPELL_BREATH_OF_YSHAARJ, true);
                me->DespawnOrUnsummon(1500);
            }
        }

        void IsSummonedBy(Unit* summoner)
        {
            if (summoner->ToCreature())
                events.ScheduleEvent(EVENT_RE_ATTACK, 6000);
        }

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_RE_ATTACK)
                {
                    if (Unit* sum = me->ToTempSummon()->GetSummoner())
                    {
                        if (sum->ToCreature())
                        {
                            if (Creature* malkorok = sum->ToCreature())
                                malkorok->AI()->DoAction(ACTION_RE_ATTACK);
                        }
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arcing_smashAI(creature);
    }
};

//71513
class npc_ancient_miasma : public CreatureScript
{
public:
    npc_ancient_miasma() : CreatureScript("npc_ancient_miasma") {}

    struct npc_ancient_miasmaAI : public ScriptedAI
    {
        npc_ancient_miasmaAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;

        void Reset(){}
        
        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ancient_miasmaAI(creature);
    }
};

//71470
class npc_implosion : public CreatureScript
{
public:
    npc_implosion() : CreatureScript("npc_implosion") {}

    struct npc_implosionAI : public ScriptedAI
    {
        npc_implosionAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;
        EventMap events;
        uint8 hit;

        void Reset()
        {
            hit = 0;
            DoCast(me, SPELL_IMPLODING_ENERGYD);
            events.ScheduleEvent(EVENT_EXPLOSE, 4000);
            me->AddAura(SPELL_IMPLODING_ENERGYV, me);
        }
        
        void SpellHitTarget(Unit* target, SpellInfo const *spell)
        {
            if (spell->Id == SPELL_IMPLODING_ENERGYD)
                hit++;
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_EXPLOSE)
                {
                    if (!hit)
                        DoCast(me, SPELL_IMPLODING_ENERGY2);
                    else
                        DoCast(me, SPELL_IMPLODING_ENERGY1);
                    me->DespawnOrUnsummon(1000);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_implosionAI(creature);
    }
};

//142861 
class spell_ancient_miasma : public SpellScriptLoader
{
public:
    spell_ancient_miasma() : SpellScriptLoader("spell_ancient_miasma") { }

    class spell_ancient_miasma_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ancient_miasma_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
            {
                GetTarget()->AddAura(SPELL_ANCIENT_BARRIER, GetTarget());
                GetTarget()->AddAura(SPELL_ANCIENT_MIASMA_DMG, GetTarget());
            }
        }

        void OnPeriodic(AuraEffect const* aurEff)
        {
            if (GetTarget())
            {
                if (GetCaster() && GetCaster()->isInCombat())
                {
                    //player revive in combat with boss
                    if (!GetTarget()->HasAura(SPELL_ANCIENT_MIASMA_DMG))
                        GetTarget()->AddAura(SPELL_ANCIENT_MIASMA_DMG, GetTarget());
                }
                else
                {
                    //for safe (buff in aura bar, but player not combat with boss)
                    GetTarget()->RemoveAurasDueToSpell(SPELL_ANCIENT_MIASMA_H_A);
                    GetTarget()->RemoveAurasDueToSpell(SPELL_ANCIENT_BARRIER);
                    GetTarget()->RemoveAurasDueToSpell(SPELL_ANCIENT_MIASMA_DMG);
                    for (uint8 n = 0; n < 3; n++)
                        GetTarget()->RemoveAurasDueToSpell(ancientbarrierbar[n]);
                }
            }
        }

        void AfterAbsorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
        {
            if (!GetTarget()->HasAura(SPELL_ANCIENT_BARRIER))
                GetTarget()->AddAura(SPELL_ANCIENT_BARRIER, GetTarget());

            if (GetTarget()->HasAura(SPELL_ANCIENT_BARRIER))
            {
                if (AuraEffect* aurEffb = GetTarget()->GetAura(SPELL_ANCIENT_BARRIER)->GetEffect(0))
                {
                    int32 maxbarrier = int32(GetTarget()->GetMaxHealth());
                    int32 lastbarrier = aurEffb->GetAmount();
                    int32 newbarrier = absorbAmount + lastbarrier;
                    uint32 buff = 0;
                    if (newbarrier >= maxbarrier)
                    {
                        newbarrier = maxbarrier;
                        buff = ancientbarrierbar[2];
                    }
                    else if (newbarrier < maxbarrier && newbarrier > int32(GetTarget()->CountPctFromMaxHealth(30)))
                        buff = ancientbarrierbar[1];
                    else if (newbarrier <= int32(GetTarget()->CountPctFromMaxHealth(30)))
                        buff = ancientbarrierbar[0];

                    for (uint8 n = 0; n < 3; n++)
                        GetTarget()->RemoveAurasDueToSpell(ancientbarrierbar[n]);
                    aurEffb->SetAmount(newbarrier);
                    GetTarget()->CastCustomSpell(buff, SPELLVALUE_BASE_POINT0, newbarrier);        
                }
            }
        }

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
            {
                GetTarget()->RemoveAurasDueToSpell(SPELL_ANCIENT_BARRIER);
                GetTarget()->RemoveAurasDueToSpell(SPELL_ANCIENT_MIASMA_DMG);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_ancient_miasma_AuraScript::OnApply, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ancient_miasma_AuraScript::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectAbsorb += AuraEffectAbsorbFn(spell_ancient_miasma_AuraScript::AfterAbsorb, EFFECT_0, SPELL_AURA_SCHOOL_HEAL_ABSORB);
            OnEffectRemove += AuraEffectRemoveFn(spell_ancient_miasma_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ancient_miasma_AuraScript();
    }
};

//142862
class spell_ancient_barrier : public SpellScriptLoader
{
public:
    spell_ancient_barrier() : SpellScriptLoader("spell_ancient_barrier") { }

    class spell_ancient_barrier_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ancient_barrier_AuraScript);
        
        void AfterAbsorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
        {
            if (aurEff->GetAmount())
            {
                int32 curbarrier = aurEff->GetAmount();
                uint32 buff = 0;
                if (curbarrier > int32(GetTarget()->CountPctFromMaxHealth(30)))
                    buff = ancientbarrierbar[1];
                else if (curbarrier <= int32(GetTarget()->CountPctFromMaxHealth(30)))
                    buff = ancientbarrierbar[0];
                for (uint8 n = 0; n < 3; n++)
                    GetTarget()->RemoveAurasDueToSpell(ancientbarrierbar[n]);
                GetTarget()->CastCustomSpell(buff, SPELLVALUE_BASE_POINT0, curbarrier);
            }
        }
        
        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
            {
                for (uint8 n = 0; n < 3; n++)
                    GetTarget()->RemoveAurasDueToSpell(ancientbarrierbar[n]);
            }
        }

        void Register()
        {
            AfterEffectAbsorb += AuraEffectAbsorbFn(spell_ancient_barrier_AuraScript::AfterAbsorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectRemove += AuraEffectRemoveFn(spell_ancient_barrier_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ancient_barrier_AuraScript();
    }
};

//142842
class spell_breath_of_yshaarj_dummy : public SpellScriptLoader
{
public:
    spell_breath_of_yshaarj_dummy() : SpellScriptLoader("spell_breath_of_yshaarj_dummy") { }

    class spell_breath_of_yshaarj_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_breath_of_yshaarj_dummy_SpellScript);

        void HandleAfterCast()
        {
            if (GetCaster() && GetCaster()->ToCreature())
                GetCaster()->ToCreature()->AI()->DoAction(ACTION_BREATH_OF_YSHAARJ);
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_breath_of_yshaarj_dummy_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_breath_of_yshaarj_dummy_SpellScript();
    }
};

//142913
class spell_displased_energy : public SpellScriptLoader
{
public:
    spell_displased_energy() : SpellScriptLoader("spell_displased_energy") { }

    class spell_displased_energy_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_displased_energy_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
                GetTarget()->CastSpell(GetTarget(), SPELL_DISPLACED_ENERGY_DMG, true);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_displased_energy_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_displased_energy_AuraScript();
    }
};

void AddSC_boss_malkorok()
{
    new boss_malkorok();
    new npc_arcing_smash();
    new npc_ancient_miasma();
    new spell_ancient_barrier();
    new npc_implosion();
    new spell_ancient_miasma();
    new spell_breath_of_yshaarj_dummy();
    new spell_displased_energy();
}
