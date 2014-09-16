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
    SPELL_CORRUPTION           = 144421, 
    SPELL_ICY_FEAR             = 145733,
    SPELL_UNLEASHED_ANGER      = 145214,
    SPELL_UNLEASHED_ANGER_DMG  = 145212,
    SPELL_UNCHECKED_CORRUPTION = 145679,
    SPELL_SELF_DOUBT           = 146124,
    SPELL_QUARANTINE_SAFETY    = 145779,
    //Blind Hatred
    SPELL_BLIND_HATRED_V       = 145226,
    SPELL_BLIND_HATRED_D       = 145227,
};

enum sEvents
{
    EVENT_CHECK_VICTIM         = 1,
    EVENT_QUARANTINE_SAFETY    = 2, 
    EVENT_UNLEASHED_ANGER      = 3,
    EVENT_BLIND_HATRED         = 4,
    EVENT_GET_NEW_POS          = 5,
};

enum sData
{
    DATA_GET_NEW_POS           = 1,
    DATA_START_MOVING          = 2,
};

enum sAction
{
    ACTION_START_EVENT         = 1,
};

float const radius = 38.0f;

class boss_norushen : public CreatureScript
{
    public:
        boss_norushen() : CreatureScript("boss_norushen") {}

        struct boss_norushenAI : public ScriptedAI
        {
            boss_norushenAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
            }

            void EnterCombat(Unit* who)
            {
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
            }

            void DoAction(int32 const action)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_norushenAI(creature);
        }
};

class boss_amalgam_of_corruption : public CreatureScript
{
    public:
        boss_amalgam_of_corruption() : CreatureScript("boss_amalgam_of_corruption") {}

        struct boss_amalgam_of_corruptionAI : public BossAI
        {
            boss_amalgam_of_corruptionAI(Creature* creature) : BossAI(creature, DATA_NORUSHEN)
            {
                instance = creature->GetInstanceScript();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
                me->SetReactState(REACT_DEFENSIVE);
                me->ModifyAuraState(AURA_STATE_UNKNOWN22, true);
                ApplyOrRemoveBar(false);
            }

            void ApplyOrRemoveBar(bool state)
            {
                Map* pMap = me->GetMap();
                if (pMap && pMap->IsDungeon())
                {
                    Map::PlayerList const &players = pMap->GetPlayers();
                    for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                    {
                        if (Player* pl = i->getSource())
                        {
                            if (pl->isAlive() && state)
                                pl->AddAura(SPELL_CORRUPTION, pl);
                            else
                                pl->RemoveAurasDueToSpell(SPELL_CORRUPTION);
                        }
                    }
                }
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                ApplyOrRemoveBar(true);
                //TestTimers
                events.ScheduleEvent(EVENT_CHECK_VICTIM, 2000);
                events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);
                events.ScheduleEvent(EVENT_QUARANTINE_SAFETY, 420000);
                events.ScheduleEvent(EVENT_BLIND_HATRED, 12000);
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
                ApplyOrRemoveBar(false);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CHECK_VICTIM:
                        if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                            DoCastAOE(SPELL_UNCHECKED_CORRUPTION);
                        events.ScheduleEvent(EVENT_CHECK_VICTIM, 2000);
                        break;
                    case EVENT_QUARANTINE_SAFETY:
                        DoCastAOE(SPELL_QUARANTINE_SAFETY);
                        break;
                    case EVENT_UNLEASHED_ANGER:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_UNLEASHED_ANGER);
                        events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);
                        break;
                    case EVENT_BLIND_HATRED:
                        float ang = (float)urand(0, 6);
                        if (Creature* bhc = me->SummonCreature(NPC_B_H_CONTROLLER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), ang, TEMPSUMMON_TIMED_DESPAWN, 32000))
                            bhc->AI()->DoAction(ACTION_START_EVENT);
                        events.ScheduleEvent(EVENT_BLIND_HATRED, 40000);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_amalgam_of_corruptionAI(creature);
        }
};

//90008 new trigger
class npc_blind_hatred_controller : public CreatureScript
{
public:
    npc_blind_hatred_controller() : CreatureScript("npc_blind_hatred_controller") { }

    struct npc_blind_hatred_controllerAI : public ScriptedAI
    {
        npc_blind_hatred_controllerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetDisplayId(11686);
        }

        InstanceScript* pInstance;

        void Reset(){}
        
        void EnterEvadeMode(){}

        void EnterCombat(){}

        void DoAction(int32 const action)
        {
            if (action == ACTION_START_EVENT)
            {
                if (me->ToTempSummon())
                {
                    float x, y;
                    GetPositionWithDistInOrientation(me, radius, me->GetOrientation(), x, y);
                    if (Unit* ac = me->ToTempSummon()->GetSummoner())
                    {
                        if (Creature* bh = ac->SummonCreature(NPC_BLIND_HATRED, x, y, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        {
                            me->GetMotionMaster()->MoveRotate(60000, rand()%2 ? ROTATE_DIRECTION_LEFT : ROTATE_DIRECTION_RIGHT);
                            DoCast(bh, SPELL_BLIND_HATRED_V, true);
                            bh->AI()->SetData(DATA_START_MOVING, 0);
                        }
                    }
                }
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_GET_NEW_POS && pInstance)
            {
                float x, y;
                GetPositionWithDistInOrientation(me, radius, me->GetOrientation(), x, y);
                if (Creature* bh = me->GetCreature(*me, pInstance->GetData64(NPC_BLIND_HATRED)))
                    bh->GetMotionMaster()->MoveCharge(x, y, me->GetPositionZ(), 5.0f, 0);
            }
        }

        void UpdateAI(uint32 diff){}        
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blind_hatred_controllerAI(pCreature);
    }
};

//72565 
class npc_blind_hatred : public CreatureScript
{
public:
    npc_blind_hatred() : CreatureScript("npc_blind_hatred") { }

    struct npc_blind_hatredAI : public ScriptedAI
    {
        npc_blind_hatredAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
        }

        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        void EnterEvadeMode(){}

        void EnterCombat(){}

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_START_MOVING)
                events.ScheduleEvent(EVENT_GET_NEW_POS, 1000);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (id == 0)
                    events.ScheduleEvent(EVENT_GET_NEW_POS, 100); 
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_GET_NEW_POS && pInstance)
                {
                    if (Creature* bhc = me->GetCreature(*me, pInstance->GetData64(NPC_B_H_CONTROLLER)))
                        bhc->AI()->SetData(DATA_GET_NEW_POS, 0);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blind_hatredAI(pCreature);
    }
};

//145216
class spell_unleashed_anger : public SpellScriptLoader
{
    public:
        spell_unleashed_anger() : SpellScriptLoader("spell_unleashed_anger") { }

        class spell_unleashed_anger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_unleashed_anger_SpellScript);

            void DealDamage()
            {
                if (!GetCaster() || !GetHitUnit())
                    return;

                GetCaster()->CastSpell(GetHitUnit(), SPELL_UNLEASHED_ANGER_DMG, true);
                GetCaster()->AddAura(SPELL_SELF_DOUBT, GetHitUnit());
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_unleashed_anger_SpellScript::DealDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_unleashed_anger_SpellScript();
        }
};

class BlindHatredDmgSelector
{
public:
    BlindHatredDmgSelector(Unit* caster, Creature* blindhatred) : _caster(caster), _blindhatred(blindhatred) {}
    
    bool operator()(WorldObject* target)
    {
        Unit* unit = target->ToUnit();
        
        if (!unit)
            return true;

        if (unit->IsInBetween(_caster, _blindhatred))
            return false;
        
        return true;
    }
private:
    Unit* _caster;
    Creature* _blindhatred;
};

//145227
class spell_blind_hatred : public SpellScriptLoader
{
    public:
        spell_blind_hatred() : SpellScriptLoader("spell_blind_hatred") { }

        class spell_blind_hatred_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blind_hatred_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                if (Creature* bh = GetCaster()->FindNearestCreature(NPC_BLIND_HATRED, 50.0f, true))
                {
                    unitList.remove_if (BlindHatredDmgSelector(GetCaster(), bh));
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blind_hatred_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blind_hatred_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blind_hatred_SpellScript();
        }
};

void AddSC_boss_norushen()
{
    new boss_norushen();
    new boss_amalgam_of_corruption();
    new npc_blind_hatred_controller();
    new npc_blind_hatred();
    new spell_unleashed_anger();
    new spell_blind_hatred();
}
