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
    //Immerseus
    SPELL_CORROSIVE_BLAST    = 143436, 
    SPELL_SHA_BOLT           = 143293, 
    SPELL_SWIRL              = 143309, 
    SPELL_SWIRL_DMG          = 143413,
    SPELL_SWIRL_SEARCHER     = 113762,
    SPELL_SUBMERGE           = 139832,
    SPELL_SUBMERGE_2         = 143281,
    //npc sha pool
    SPELL_SLEEPING_SHA       = 143281,
    SPELL_SHA_POOL           = 143462,
    //npc sha puddle
    SPELL_ERUPTING_SHA       = 143498,
    SPELL_SHA_RESIDUE        = 143459,//buff
    //npc contaminated puddle
    SPELL_ERUPTING_WATER     = 145377,
    SPELL_PURIFIED_RESIDUE   = 143524,//buff
};

enum Events
{
    //Immerseus
    EVENT_CORROSIVE_BLAST    = 1,
    EVENT_SHA_BOLT           = 2,
    EVENT_SWIRL              = 3,
    EVENT_INTRO_PHASE_TWO    = 4,
    //Summons
    EVENT_START_MOVING       = 5,
    EVENT_CHECK_DIST         = 6,
};

enum Actions
{
    //Immerseus
    ACTION_RE_ATTACK         = 1,
    ACTION_INTRO_PHASE_ONE   = 2,
    //Summons
    ACTION_SPAWN             = 2,
};

enum SData
{
    //Immerseus
    DATA_SP_DONE             = 1, 
    DATA_CP_DONE             = 2, 
    DATA_P_FINISH_MOVE       = 3, 
    DATA_SEND_F_P_COUNT      = 4, 

    //Summons
    DATA_SEND_INDEX          = 5, 
};

//puddle spawn pos
Position const psp[24] =  
{
    {1417.32f, 658.89f, 246.8535f},
    {1404.85f, 665.43f, 246.8601f},
    {1390.78f, 677.95f, 246.8363f},
    //
    {1365.75f, 703.08f, 246.8346f},
    {1358.45f, 718.70f, 246.8346f},
    {1354.80f, 733.24f, 246.8355f},
    //
    {1353.71f, 771.01f, 246.8345f},
    {1357.54f, 784.59f, 246.8345f},
    {1365.33f, 801.40f, 246.8345f},
    //
    {1396.49f, 830.86f, 246.8346f}, 
    {1409.99f, 836.37f, 246.8346f}, 
    {1428.83f, 840.66f, 246.8346f},
    //
    {1454.44f, 842.49f, 246.8346f},
    {1474.15f, 835.67f, 246.8346f},
    {1492.23f, 825.11f, 246.8332f},
    //
    {1520.68f, 798.13f, 246.8348f},
    {1526.22f, 783.98f, 246.8348f},
    {1529.01f, 771.96f, 246.8348f},
    //
    {1529.04f, 730.04f, 246.7348f},
    {1525.13f, 718.31f, 246.8348f},
    {1517.43f, 701.80f, 246.8348f},
    //
    {1487.90f, 673.78f, 246.8346f},
    {1478.02f, 665.65f, 246.8619f},
    {1462.59f, 657.93f, 246.8514f},
};

uint32 const spwave[24] = 
{
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_SHA_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
};

uint32 const cpwave[24] =
{
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
    NPC_SHA_PUDDLE,  
    NPC_CONTAMINATED_PUDDLE,
    NPC_CONTAMINATED_PUDDLE,
};

class boss_immerseus : public CreatureScript
{
    public:
        boss_immerseus() : CreatureScript("boss_immerseus") {}

        struct boss_immerseusAI : public BossAI
        {
            boss_immerseusAI(Creature* creature) : BossAI(creature, DATA_IMMEREUS)
            {
                instance = creature->GetInstanceScript();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* instance;
            uint32 checkvictim;
            uint8 donecp, donesp, maxpcount;
            bool phase_two;

            void Reset()
            {
                _Reset();
                me->SetReactState(REACT_DEFENSIVE);
                me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                me->RemoveAurasDueToSpell(SPELL_SUBMERGE_2);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 100);
                phase_two = false;
                checkvictim = 0;
                donecp = 0; 
                donesp = 0;
                maxpcount = 0;
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                checkvictim = 2000;
                events.ScheduleEvent(EVENT_CORROSIVE_BLAST, 35000);
                events.ScheduleEvent(EVENT_SWIRL,           13000);
                events.ScheduleEvent(EVENT_SHA_BOLT,         6000);
            }

            void SpawnWave()
            {
                if (!donecp && !donesp) //first proc
                {
                    for (uint8 n = 0; n < 24; n++)
                    {
                        if (Creature* p = me->SummonCreature(spwave[n], me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 7.0f))
                            p->AI()->SetData(DATA_SEND_INDEX, n);
                    }
                }
                else if (donesp > donecp)
                {
                    for (uint8 n = 0; n < 24; n++)
                    {
                        if (Creature* p = me->SummonCreature(cpwave[n], me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 7.0f))
                            p->AI()->SetData(DATA_SEND_INDEX, n);
                    }
                }
                else if (donecp > donesp)
                {
                    for (uint8 n = 0; n < 24; n++)
                    {
                        if (Creature* p = me->SummonCreature(spwave[n], me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 7.0f))
                            p->AI()->SetData(DATA_SEND_INDEX, n);
                    }
                }
                maxpcount = 0; 
                donecp = 0; 
                donesp = 0;
            }

            void SetData(uint32 type, uint32 data)
            {
                switch (type)
                {
                case DATA_SP_DONE:
                    maxpcount++;
                    donesp++;
                    if (me->GetPower(POWER_ENERGY) >= 1)
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) - 1);
                    break;
                case DATA_CP_DONE:
                    maxpcount++;
                    donecp++;
                    if (me->GetPower(POWER_ENERGY) >= 1)
                        me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) - 1);
                    break;
                case DATA_P_FINISH_MOVE:
                    maxpcount++;
                    break;
                }
            }

            uint32 GetData(uint32 type)
            {
                if (type == DATA_SEND_F_P_COUNT)
                    return maxpcount;
                else
                    return 0;
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (damage >= me->GetHealth() && !phase_two)
                {
                    damage = 0;
                    phase_two = true;
                    checkvictim = 0;
                    events.Reset();
                    me->InterruptNonMeleeSpells(true);
                    summons.DespawnAll();
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->AddAura(SPELL_SUBMERGE, me);
                    me->AddAura(SPELL_SUBMERGE_2, me);
                    me->SetFullHealth();
                    SpawnWave();
                }
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_RE_ATTACK:
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 150.0f);
                    break;
                case ACTION_INTRO_PHASE_ONE:
                    if (me->GetPower(POWER_ENERGY) == 0)
                    {
                        //Done
                        me->Kill(me, true);
                        return;
                    }
                    me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                    me->RemoveAurasDueToSpell(SPELL_SUBMERGE_2);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 150.0f);
                    phase_two = false;
                    checkvictim = 2000;
                    events.ScheduleEvent(EVENT_CORROSIVE_BLAST, 35000);
                    events.ScheduleEvent(EVENT_SWIRL,           13000);
                    events.ScheduleEvent(EVENT_SHA_BOLT,         6000);
                    break;
                }
            }

            void JustDied(Unit* killer)
            {
                me->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                if (killer == me)
                {
                    _JustDied();
                    me->setFaction(35);
                    me->SummonGameObject(221776, 1441.22f, 821.749f, 246.836f, 4.727f, 0.0f, 0.0f, 0.701922f, -0.712254f, 604800);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim()) 
                    return;

                if (checkvictim && !phase_two)
                {
                    if (checkvictim <= diff)
                    {
                        if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                            EnterEvadeMode();
                    }
                    else
                        checkvictim -= diff;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CORROSIVE_BLAST:
                        if (me->getVictim())
                        {
                            uint64 vG = me->getVictim()->GetGUID();
                            me->AttackStop();
                            me->SetReactState(REACT_PASSIVE);
                            if (Unit* target = me->GetUnit(*me, vG))
                            {
                                me->SetFacingToObject(target);
                                DoCastAOE(SPELL_CORROSIVE_BLAST);
                            }
                        }
                        events.ScheduleEvent(EVENT_CORROSIVE_BLAST, 35000);
                        break;
                    case EVENT_SHA_BOLT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                            DoCast(target, SPELL_SHA_BOLT);
                        events.ScheduleEvent(EVENT_SHA_BOLT, 15000);
                        break;
                    case EVENT_SWIRL:
                        me->AttackStop();
                        me->SetReactState(REACT_PASSIVE);
                        me->GetMotionMaster()->MoveRotate(10000, ROTATE_DIRECTION_RIGHT);
                        DoCast(me, SPELL_SWIRL);
                        events.ScheduleEvent(EVENT_SWIRL, 48500);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_immerseusAI(creature);
        }
};

//71544
class npc_sha_pool : public CreatureScript
{
    public:
        npc_sha_pool() : CreatureScript("npc_sha_pool") {}

        struct npc_sha_poolAI : public ScriptedAI
        {
            npc_sha_poolAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetDisplayId(11686);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            }

            void Reset()
            {
                me->SetFloatValue(OBJECT_FIELD_SCALE_X, 2.0f);
                me->AddAura(SPELL_SHA_POOL, me);
            }

            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void UpdateAI(const uint32 diff){}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sha_poolAI(creature);
        }
};

void CalcPuddle(InstanceScript* instance, Creature* caller, uint32 callerEntry, bool done)
{
    if (caller && instance)
    {
        if (Creature* i = caller->GetCreature(*caller, instance->GetData64(NPC_IMMERSEUS)))
        {
            if (done)
            {
                switch (callerEntry)
                {    
                case NPC_SHA_PUDDLE:
                    i->AI()->SetData(DATA_SP_DONE, 0);
                    break;
                case NPC_CONTAMINATED_PUDDLE:
                    i->AI()->SetData(DATA_CP_DONE, 0);
                    break;
                }
            }
            else
                i->AI()->SetData(DATA_P_FINISH_MOVE, 0);

            caller->DespawnOrUnsummon();

            if (i->AI()->GetData(DATA_SEND_F_P_COUNT) >= 24)
                i->AI()->DoAction(ACTION_INTRO_PHASE_ONE);
        }
    }
}

//71603
class npc_sha_puddle : public CreatureScript
{
    public:
        npc_sha_puddle() : CreatureScript("npc_sha_puddle") {}

        struct npc_sha_puddleAI : public ScriptedAI
        {
            npc_sha_puddleAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            }

            InstanceScript* instance;
            EventMap events;
            uint8 index;
            bool finish;

            void Reset()
            {
                events.Reset();
                finish = false;
                index = 0;
            }

            void SetData(uint32 type, uint32 data)
            {
                if (type == DATA_SEND_INDEX)
                {
                    index = data;
                    DoAction(ACTION_SPAWN);
                }
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_SPAWN)
                    me->GetMotionMaster()->MoveJump(psp[index].GetPositionX(), psp[index].GetPositionY(), psp[index].GetPositionZ(), 25.0f, 25.0f, 0);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type == EFFECT_MOTION_TYPE)
                {
                    if (pointId == 0)
                    {
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        events.ScheduleEvent(EVENT_START_MOVING, 1000);  
                    }
                }
            }
            
            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (damage >= me->GetHealth())
                {
                    DoCast(me, SPELL_SHA_RESIDUE, true);
                    return;
                }

                if (attacker->HasAura(SPELL_SHA_RESIDUE))
                    damage += damage/4;
            }

            void JustDied(Unit* killer)
            {
                CalcPuddle(instance, me, me->GetEntry(), true);
            }

            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_START_MOVING:
                        if (instance)
                        {
                            if (Creature* pp = me->GetCreature(*me, instance->GetData64(NPC_PUDDLE_POINT)))
                            {
                                me->GetMotionMaster()->MoveFollow(pp, 10.0f, 0.0f);
                                events.ScheduleEvent(EVENT_CHECK_DIST, 1000);
                            }
                        }
                        break;
                    case EVENT_CHECK_DIST:
                        if (instance)
                        {
                            if (Creature* pp = me->GetCreature(*me, instance->GetData64(NPC_PUDDLE_POINT)))
                            {
                                if (me->GetDistance(pp) <= 20.0f && !finish)
                                {
                                    finish = true;
                                    me->StopMoving();
                                    me->GetMotionMaster()->Clear();
                                    DoCast(me, SPELL_ERUPTING_SHA);
                                    CalcPuddle(instance, me, me->GetEntry(), false);
                                }
                                else if (me->GetDistance(pp) > 20.0f && !finish)
                                    events.ScheduleEvent(EVENT_CHECK_DIST, 1000);
                            }
                        }
                        break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sha_puddleAI(creature);
        }
};

//71604
class npc_contaminated_puddle : public CreatureScript
{
    public:
        npc_contaminated_puddle() : CreatureScript("npc_contaminated_puddle") {}

        struct npc_contaminated_puddleAI : public ScriptedAI
        {
            npc_contaminated_puddleAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);
            }

            InstanceScript* instance;
            EventMap events;
            uint8 index, slowval;
            bool done, finish;

            void Reset()
            {
                events.Reset();
                me->SetHealth(1);
                finish = false;
                done = false;
                index = 0; 
                slowval = 0;
            }

            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void SpellHit(Unit* caster, SpellInfo const *spell)
            {
                for (uint8 n = 0; n < MAX_SPELL_EFFECTS; n++)
                {
                    if (spell->GetEffect(n).Effect == SPELL_EFFECT_HEAL || spell->GetEffect(n).Effect == SPELL_EFFECT_HEAL_PCT)
                    {
                        if (HealthAbovePct(25) && !slowval)
                        {
                            slowval++;
                            me->SetSpeed(MOVE_RUN, 0.8f);
                        }
                        else if (HealthAbovePct(50) && slowval == 1)
                        {
                            slowval++;
                            me->SetSpeed(MOVE_RUN, 0.7f);
                        }
                        else if (HealthAbovePct(75) && slowval == 2)
                        {
                            slowval++;
                            me->SetSpeed(MOVE_RUN, 0.6f);
                        }
                    }
                }

                if (me->GetHealth() == me->GetMaxHealth() && !done)
                {
                    done = true;
                    DoCast(me, SPELL_PURIFIED_RESIDUE);
                }
            }

            void SetData(uint32 type, uint32 data)
            {
                if (type == DATA_SEND_INDEX)
                {
                    index = data;
                    DoAction(ACTION_SPAWN);
                }
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_SPAWN)
                    me->GetMotionMaster()->MoveJump(psp[index].GetPositionX(), psp[index].GetPositionY(), psp[index].GetPositionZ(), 25.0f, 25.0f, 0);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type == EFFECT_MOTION_TYPE)
                {
                    if (pointId == 0)
                    {
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        events.ScheduleEvent(EVENT_START_MOVING, 1000);
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);
                
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_START_MOVING:
                        if (instance)
                        {
                            if (Creature* pp = me->GetCreature(*me, instance->GetData64(NPC_PUDDLE_POINT)))
                            {
                                me->GetMotionMaster()->MoveFollow(pp, 10.0f, 0.0f);
                                events.ScheduleEvent(EVENT_CHECK_DIST, 1000);
                            }
                        }
                        break;
                    case EVENT_CHECK_DIST:
                        if (instance)
                        {
                            if (Creature* pp = me->GetCreature(*me, instance->GetData64(NPC_PUDDLE_POINT)))
                            {
                                if (me->GetDistance(pp) <= 20.0f && !finish)
                                {
                                    finish = true;
                                    me->StopMoving();
                                    me->GetMotionMaster()->Clear();
                                    DoCast(me, SPELL_ERUPTING_WATER);
                                    CalcPuddle(instance, me, me->GetEntry(), done ? true : false);
                                }
                                else if (me->GetDistance(pp) > 20.0f && !finish)
                                    events.ScheduleEvent(EVENT_CHECK_DIST, 1000);
                            }
                        }
                        break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_contaminated_puddleAI(creature);
        }
};

//143436
class spell_corrosive_blast : public SpellScriptLoader
{
    public:
        spell_corrosive_blast() : SpellScriptLoader("spell_corrosive_blast") { }
        
        class spell_corrosive_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_corrosive_blast_SpellScript);
            
            void OnAfterCast()
            {
                if (GetCaster() && GetCaster()->ToCreature())
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_corrosive_blast_SpellScript::OnAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_corrosive_blast_SpellScript();
        }
};

//143309
class spell_swirl : public SpellScriptLoader
{
    public:
        spell_swirl() : SpellScriptLoader("spell_swirl") { }

        class spell_swirl_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_swirl_AuraScript);
            
            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster() && GetCaster()->ToCreature())
                    GetCaster()->AddAura(SPELL_SWIRL_SEARCHER, GetCaster());
            }

            void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster() && GetCaster()->ToCreature())
                {
                    GetCaster()->RemoveAurasDueToSpell(SPELL_SWIRL_SEARCHER);
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_swirl_AuraScript::OnApply, EFFECT_1, SPELL_AURA_395, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_swirl_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_395, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_swirl_AuraScript();
        }
};

//125925
class spell_swirl_searcher : public SpellScriptLoader
{
    public:
        spell_swirl_searcher() : SpellScriptLoader("spell_swirl_searcher") { }

        class spell_swirl_searcher_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_swirl_searcher_SpellScript);

            void ApplyHit()
            {
                if (GetHitUnit())
                    GetHitUnit()->CastSpell(GetHitUnit(), SPELL_SWIRL_DMG);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_swirl_searcher_SpellScript::ApplyHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_swirl_searcher_SpellScript();
        }
};

void AddSC_boss_immerseus()
{
    new boss_immerseus();
    new npc_sha_pool();
    new npc_sha_puddle();
    new npc_contaminated_puddle();
    new spell_corrosive_blast();
    new spell_swirl();
    new spell_swirl_searcher();
}
