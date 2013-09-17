/* Copyright (C) 2010 Easy for TrinityCore <http://trinity-core.ru/>
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ScriptPCH.h"
#include "ruby_sanctum.h"

enum eScriptTexts
{
    SAY_AGGRO           = 0,
    SAY_SLAY1           = 1,
    SAY_SLAY2           = 2,
    SAY_SUMMON_CLONE    = 3,
    SAY_DEATH           = 4,

    SAY_XERESTRASZA_START    = 0,
    SAY_XERESTRASZA_1        = 1,
    SAY_XERESTRASZA_2        = 2,
    SAY_XERESTRASZA_3        = 3,
    SAY_XERESTRASZA_4        = 4,
    SAY_XERESTRASZA_5        = 5,
    SAY_XERESTRASZA_6        = 6,
    SAY_XERESTRASZA_7        = 7,
    SAY_XERESTRASZA_INTRO    = 8
};

enum eSpells
{
    SPELL_BARRIER_CHANNEL       = 76221,
    SPELL_ENERVATING_BRAND      = 74502,
    SPELL_SIPHONED_MIGHT        = 74507,
    SPELL_CLEAVE                = 40504,
    SPELL_BLADE_TEMPEST         = 75125,
    SPELL_CLONE                 = 74511,
    SPELL_REPELLING_WAVE        = 74509,
    SPELL_CLEAR_DEBUFFS         = 34098,
    SPELL_SPAWN_EFFECT          = 64195,
    SPELL_SUMMON_CLONE          = 74511,
};

enum eEvents
{
    EVENT_CAST_CLEAVE           = 1,
    EVENT_CAST_REPELLING_WAVE   = 2,
    EVENT_CAST_ENERVATING_BRAND = 3,
    EVENT_CAST_BLADE_TEMPEST    = 4,
    EVENT_CAST_SUMMON_CLONE     = 5,
    
    ACTION_START_EVENT          = 6,
    EVENT_XERESTRASZA_3         = 7,
    EVENT_XERESTRASZA_4         = 8,
    EVENT_XERESTRASZA_5         = 9,
    EVENT_XERESTRASZA_6         = 10,
    EVENT_XERESTRASZA_7         = 11,
    EVENT_XERESTRASZA_8         = 12,
    EVENT_XERESTRASZA_9         = 13,
    EVENT_MOVE_ON               = 14
};

class npc_xerestrasza : public CreatureScript
{
    public:
        npc_xerestrasza() : CreatureScript("npc_xerestrasza") { }

        struct npc_xerestraszaAI : public ScriptedAI
        {
            npc_xerestraszaAI(Creature *pCreature) : ScriptedAI(pCreature)
            {
                pInstance = me->GetInstanceScript();
            }

            void Reset()
            {
                events.Reset();
                bIntro = false;
                pInstance->SetData(TYPE_XERESTRASZA,NOT_STARTED);
            }

            void MoveInLineOfSight(Unit*)
            {
                if(!bIntro)
                {
                    Talk(SAY_XERESTRASZA_INTRO);
                    if(pInstance)
                        pInstance->SetData(TYPE_XERESTRASZA, NOT_STARTED);
                    bIntro = true;
                }
            }

            void DoAction(const int32 action)
            {
                if (action == ACTION_START_EVENT)
                {
                    me->setFaction(35);
                    me->GetMotionMaster()->MovePoint(1, 3153.5490f, 385.53f, 86.33f);
                    if (pInstance)
                        pInstance->SetData(TYPE_XERESTRASZA,IN_PROGRESS);
                    Talk(SAY_XERESTRASZA_START);
                    events.ScheduleEvent(EVENT_XERESTRASZA_3,9000); 
                    events.ScheduleEvent(EVENT_XERESTRASZA_4,20000); 
                    events.ScheduleEvent(EVENT_XERESTRASZA_5,31000); 
                    events.ScheduleEvent(EVENT_XERESTRASZA_6,42000); 
                    events.ScheduleEvent(EVENT_XERESTRASZA_7,53000); 
                    events.ScheduleEvent(EVENT_XERESTRASZA_8,64000); 
                    events.ScheduleEvent(EVENT_XERESTRASZA_9,75000); 
                }
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_XERESTRASZA_3:
                            Talk(SAY_XERESTRASZA_1);
                            break;
                        case EVENT_XERESTRASZA_4:
                            Talk(SAY_XERESTRASZA_2);
                            break;
                        case EVENT_XERESTRASZA_5:
                            Talk(SAY_XERESTRASZA_3);
                            break;
                        case EVENT_XERESTRASZA_6:
                            Talk(SAY_XERESTRASZA_4);
                            break;
                        case EVENT_XERESTRASZA_7:
                            Talk(SAY_XERESTRASZA_5);
                            break;
                        case EVENT_XERESTRASZA_8:
                            Talk(SAY_XERESTRASZA_6);
                            break;
                        case EVENT_XERESTRASZA_9:
                            Talk(SAY_XERESTRASZA_7);
                            if (pInstance)
                                pInstance->SetData(TYPE_XERESTRASZA, DONE);
                            break;
                    }
                }
            }

        private:
            bool bIntro;
            EventMap events;
            InstanceScript* pInstance;
        };
        
        CreatureAI* GetAI(Creature *pCreature) const
        {
            return new npc_xerestraszaAI(pCreature);
        }
};

class boss_baltharus_the_warborn : public CreatureScript
{
    public:
        boss_baltharus_the_warborn() : CreatureScript("boss_baltharus_the_warborn") { }

        struct boss_baltharus_the_warbornAI : public BossAI
        {
            boss_baltharus_the_warbornAI(Creature* pCreature) : BossAI(pCreature, TYPE_BALTHARUS)
            {
                instance = me->GetInstanceScript();
            }

            void Reset()
            {
                
                if (instance)
                    instance->SetData(TYPE_BALTHARUS, NOT_STARTED);
                bClone = false;
                events.Reset();
                events.ScheduleEvent(EVENT_CAST_CLEAVE, urand(2000,3000));
                events.ScheduleEvent(EVENT_CAST_REPELLING_WAVE, urand(20000,30000));
                events.ScheduleEvent(EVENT_CAST_ENERVATING_BRAND, urand(30000,45000));
                events.ScheduleEvent(EVENT_CAST_BLADE_TEMPEST, urand(7000,7500));
            }

            void EnterCombat(Unit*)
            {
                if (instance)
                    instance->SetData(TYPE_BALTHARUS, IN_PROGRESS);
                Talk(SAY_AGGRO);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() || !CheckInRoom())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CAST_CLEAVE:
                            if (me->getVictim())
                                DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CAST_CLEAVE, urand(2000,3000));
                            break;
                        case EVENT_CAST_REPELLING_WAVE:
                            DoCast(SPELL_REPELLING_WAVE);
                            events.ScheduleEvent(EVENT_CAST_REPELLING_WAVE, urand(20000,30000));
                            break;
                        case EVENT_CAST_ENERVATING_BRAND:
                            if (me->getVictim())
                                DoCastVictim(SPELL_ENERVATING_BRAND);
                            events.ScheduleEvent(EVENT_CAST_ENERVATING_BRAND, urand(30000,45000));
                            break;
                        case EVENT_CAST_BLADE_TEMPEST:
                            me->SetWalk(false);
                            DoCast(SPELL_BLADE_TEMPEST);
                            events.ScheduleEvent(EVENT_CAST_BLADE_TEMPEST, urand(30000,45000));
                            events.ScheduleEvent(EVENT_MOVE_ON, 4000);
                            break;
                        case EVENT_MOVE_ON:
                            me->SetWalk(true);
                            break;
                    }
                }

                if (!bClone)
                {
                    if(me->GetHealth() <= ((me->GetMaxHealth() / 100) * 50))
                    {
                        bClone = true;
                        Talk(SAY_SUMMON_CLONE);
                        if (Creature* Clone = me->SummonCreature(39899, me->GetPositionX()+5, me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            Clone->AI()->DoZoneInCombat();
                        }
                    }
                }

                DoMeleeAttackIfReady();
            }

            void JustSummoned(Creature *summon)
            {
                summons.Summon(summon);
            }

            void KilledUnit(Unit *victim)
            {
                Talk(RAND(SAY_SLAY1,SAY_SLAY2));
            }

            void JustReachedHome()
            {
                summons.DespawnAll();
                if(instance)
                    instance->SetData(TYPE_BALTHARUS, FAIL);
            }

            void JustDied(Unit*)
            {
                Talk(SAY_DEATH);
                if (instance)
                    instance->SetData(TYPE_BALTHARUS, DONE);
                if (Creature* pXerestrasza = ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_XERESTRASZA)))
                        CAST_AI(npc_xerestrasza::npc_xerestraszaAI, pXerestrasza->AI())->DoAction(ACTION_START_EVENT);
                _JustDied();
            }

        private:
            bool bClone;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature *pCreature) const
        {
            return new boss_baltharus_the_warbornAI(pCreature);
        }

};

class npc_baltharus_the_warborn_clone : public CreatureScript
{
    public:
        npc_baltharus_the_warborn_clone() : CreatureScript("npc_baltharus_the_warborn_clone") { }

        struct npc_baltharus_the_warborn_cloneAI : public ScriptedAI
        {
            npc_baltharus_the_warborn_cloneAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
                pInstance = me->GetInstanceScript();
            }

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_CAST_CLEAVE, urand(2000,3000));
                events.ScheduleEvent(EVENT_CAST_ENERVATING_BRAND, urand(30000,45000));
                events.ScheduleEvent(EVENT_CAST_BLADE_TEMPEST, urand(7000,7500));
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CAST_CLEAVE:
                            if (me->getVictim())
                                DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CAST_CLEAVE, urand(2000,3000));
                            break;
                        case EVENT_CAST_ENERVATING_BRAND:
                            if (me->getVictim())
                                DoCastVictim(SPELL_ENERVATING_BRAND);
                            events.ScheduleEvent(EVENT_CAST_ENERVATING_BRAND, urand(30000,45000));
                            break;
                        case EVENT_CAST_BLADE_TEMPEST:
                            me->SetWalk(false);
                            DoCast(SPELL_BLADE_TEMPEST);
                            events.ScheduleEvent(EVENT_CAST_BLADE_TEMPEST, urand(30000,45000));
                            events.ScheduleEvent(EVENT_MOVE_ON, 4000);
                            break;
                        case EVENT_MOVE_ON:
                            me->SetWalk(true);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            InstanceScript* pInstance;

        };

        CreatureAI* GetAI(Creature *pCreature) const
        {
            return new npc_baltharus_the_warborn_cloneAI(pCreature);
        }

};

class spell_baltharus_enervating_brand_trigger : public SpellScriptLoader
{
    public:
        spell_baltharus_enervating_brand_trigger() : SpellScriptLoader("spell_baltharus_enervating_brand_trigger") { }

        class spell_baltharus_enervating_brand_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_baltharus_enervating_brand_trigger_SpellScript);

            void CheckDistance()
            {
                if (Unit* caster = GetOriginalCaster())
                {
                    if (Unit* target = GetHitUnit())
                        target->CastSpell(caster, SPELL_SIPHONED_MIGHT, true);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_baltharus_enervating_brand_trigger_SpellScript::CheckDistance);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_baltharus_enervating_brand_trigger_SpellScript();
        }
};

class at_baltharus_plateau : public AreaTriggerScript
{
    public:
        at_baltharus_plateau() : AreaTriggerScript("at_baltharus_plateau") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            // Only trigger once
            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (Creature* xerestrasza = ObjectAccessor::GetCreature(*player, instance->GetData64(NPC_XERESTRASZA)))
                    xerestrasza->AI()->DoAction(ACTION_INTRO_BALTHARUS);

                if (Creature* baltharus = ObjectAccessor::GetCreature(*player, instance->GetData64(NPC_BALTHARUS)))
                    baltharus->AI()->DoAction(ACTION_INTRO_BALTHARUS);
            }

            return true;
        }
};

void AddSC_boss_baltharus_the_warborn()
{
    new boss_baltharus_the_warborn();
    new npc_baltharus_the_warborn_clone();
    new npc_xerestrasza();
    new spell_baltharus_enervating_brand_trigger();
    new at_baltharus_plateau();
}