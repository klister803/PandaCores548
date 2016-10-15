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
    //Tortos
    SPELL_FURIOS_STONE_BREATH  = 133939,
    SPELL_SNAPPING_BITE        = 135251,
    SPELL_QUAKE_STOMP          = 134920,
    SPELL_CALL_OF_TORTOS       = 136294,
    SPELL_ROCKFALL_P_DMG       = 134539,

    //Whirl turtle
    SPELL_SHELL_BLOCK          = 133971,
    SPELL_SPINNING_SHELL_V     = 133974,
    SPELL_SPINNING_SHELL_DMG   = 134011,

    //Vampiric cave bat
    SPELL_DRAIN_THE_WEAK       = 135103,
    SPELL_DRAIN_THE_WEAK_HEAL  = 135102,
    SPELL_DRAIN_THE_WEAK_DMG   = 135101,
};

enum eEvents
{
    //Tortos
    EVENT_SNAPPING_BITE        = 1, 
    EVENT_QUAKE_STOMP          = 2, 
    EVENT_CALL_OF_TORTOS       = 3,
    EVENT_SUMMON_BATS          = 4,
    EVENT_STONE_BREATH         = 5,

    //Whirl turtle
    EVENT_SPINNING_SHELL       = 5,
};

const float maxpullpos = 4988.0f;

class boss_tortos : public CreatureScript
{
public:
    boss_tortos() : CreatureScript("boss_tortos") {}

    struct boss_tortosAI : public BossAI
    {
        boss_tortosAI(Creature* creature) : BossAI(creature, DATA_TORTOS)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 checkvictim;

        void Reset()
        {
            _Reset();
            checkvictim = 0;
            me->setPowerType(POWER_ENERGY);
            me->SetPower(POWER_ENERGY, 100);
        }

        void JustReachedHome()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_DEFENSIVE);
        }

        void EnterCombat(Unit* who)
        {
            _EnterCombat();
            checkvictim = 1500;
            events.ScheduleEvent(EVENT_SUMMON_BATS, 45000);
            events.ScheduleEvent(EVENT_CALL_OF_TORTOS, 60000);
            events.ScheduleEvent(EVENT_STONE_BREATH, urand(60000, 70000));
            events.ScheduleEvent(EVENT_SNAPPING_BITE, urand(8000, 10000));
            events.ScheduleEvent(EVENT_QUAKE_STOMP, urand(45000, 50000));
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
        }

        bool CheckPullPlayerPos(Unit* who)
        {
            if (!who->ToPlayer() || who->GetPositionY() > maxpullpos)
                return false;
            return true;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (checkvictim && instance)
            {
                if (checkvictim <= diff)
                {
                    if (me->getVictim())
                    {
                        if (!CheckPullPlayerPos(me->getVictim()))
                        {
                            checkvictim = 0;
                            me->AttackStop();
                            me->SetReactState(REACT_PASSIVE);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                            EnterEvadeMode();
                        }
                        else
                            checkvictim = 1500;
                    }
                }
                else
                    checkvictim -= diff;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SNAPPING_BITE:
                    if (me->getVictim())
                        DoCast(me->getVictim(), SPELL_SNAPPING_BITE);
                    events.ScheduleEvent(EVENT_SNAPPING_BITE, urand(8000, 10000));
                    break;
                case EVENT_QUAKE_STOMP:
                    DoCastAOE(SPELL_QUAKE_STOMP);
                    events.ScheduleEvent(EVENT_QUAKE_STOMP, urand(45000, 50000));
                    break;
                case EVENT_CALL_OF_TORTOS:
                    DoCastAOE(SPELL_CALL_OF_TORTOS);
                    for (uint8 n = 0; n < 3; n++)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                        {
                            Position pos;
                            target->GetPosition(&pos);
                            me->SummonCreature(NPC_WHIRL_TURTLE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 0.0f);
                        }
                    }
                    events.ScheduleEvent(EVENT_CALL_OF_TORTOS, 60000);
                    break;
                case EVENT_SUMMON_BATS:
                    for (uint8 n = 0; n < 5; n++)
                        if (Creature* vb = me->SummonCreature(NPC_VAMPIRIC_CAVE_BAT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f))
                            vb->AI()->DoZoneInCombat(vb, 100.0f);
                    events.ScheduleEvent(EVENT_SUMMON_BATS, 45000);
                    break;
                case EVENT_STONE_BREATH:
                    DoCast(me, SPELL_FURIOS_STONE_BREATH);
                    events.ScheduleEvent(EVENT_STONE_BREATH, urand(60000, 70000));
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_tortosAI(creature);
    }
};

//67966
class npc_whirl_turtle : public CreatureScript
{
public:
    npc_whirl_turtle() : CreatureScript("npc_whirl_turtle") {}

    struct npc_whirl_turtleAI : public ScriptedAI
    {
        npc_whirl_turtleAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
        }
        InstanceScript* pInstance;
        bool done;

        void Reset()
        {
            done = false;
            DoCast(me, SPELL_SPINNING_SHELL_V, true);
            me->GetMotionMaster()->MoveRandom(8.0f);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;

            if (HealthBelowPct(10) && !done)
            {
                done = true;
                me->GetMotionMaster()->MoveIdle();
                me->RemoveAurasDueToSpell(SPELL_SPINNING_SHELL_V);
                DoCast(me, SPELL_SHELL_BLOCK, true);
            }
        }

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_whirl_turtleAI(creature);
    }
};

//69352
class npc_vampiric_cave_bat : public CreatureScript
{
public:
    npc_vampiric_cave_bat() : CreatureScript("npc_vampiric_cave_bat") {}

    struct npc_vampiric_cave_batAI : public ScriptedAI
    {
        npc_vampiric_cave_batAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
        }

        InstanceScript* pInstance;

        void Reset(){}

        void EnterCombat(Unit* who)
        {
            DoCast(me, SPELL_DRAIN_THE_WEAK, true);
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
        return new npc_vampiric_cave_batAI(creature);
    }
};

class spell_quake_stomp : public SpellScriptLoader
{
public:
    spell_quake_stomp() : SpellScriptLoader("spell_quake_stomp") { }

    class spell_quake_stomp_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_quake_stomp_SpellScript);

        void DealDamage()
        {
            if (GetHitUnit())
            {
                int32 curdmg = ((GetHitUnit()->GetMaxHealth() / 2) + (GetHitUnit()->GetMaxHealth() / 10));
                if (curdmg)
                    SetHitDamage(curdmg);
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_quake_stomp_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_quake_stomp_SpellScript();
    }
};

//135101
class spell_drain_the_weak : public SpellScriptLoader
{
public:
    spell_drain_the_weak() : SpellScriptLoader("spell_drain_the_weak") { }

    class spell_drain_the_weak_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_drain_the_weak_SpellScript);

        void DealDamage()
        {
            if (GetCaster() && GetHitUnit())
            {
                float heal = float(GetHitDamage() * 10);
                GetCaster()->CastCustomSpell(SPELL_DRAIN_THE_WEAK_HEAL, SPELLVALUE_BASE_POINT0, heal, GetCaster(), true);
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_drain_the_weak_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_drain_the_weak_SpellScript();
    }
};

void AddSC_boss_tortos()
{
    new boss_tortos();
    new npc_whirl_turtle();
    new npc_vampiric_cave_bat();
    new spell_quake_stomp();
    new spell_drain_the_weak();
}
