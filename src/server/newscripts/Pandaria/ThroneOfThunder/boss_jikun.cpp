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
    //Special spells (feather - fly)
    SPELL_LESSON_OF_ICARUS      = 140571,
    SPELL_DAEDALIAN_WINGS       = 134339,//ovveride spells aura
    SPELL_INCUBATE_ZONE         = 137526,
    SPELL_INCUBATE_TARGET_AURA  = 134347,

    SPELL_FEATHER_AT            = 134338,

    SPELL_JUMP_TO_B_PLATFORM    = 138360,

    //From sniffs
    //139168 м€гко но уверенно
    //138360 jump to platform AT
    //138406 заставить игрока прыгнуть на платформу

    SPELL_JI_KUN_FEATHER_AURA   = 140014,
    SPELL_JI_KUN_FEATHER_USE_EF = 140013,

    //Ji Kun
    SPELL_CAW                   = 138926,
    SPELL_QUILLS                = 134380,
    SPELL_TALON_RAKE            = 134366,
    SPELL_INFECTED_TALONS       = 140092,
    SPELL_DOWN_DRAFT_AT         = 134370,
};

enum eEvents
{
    EVENT_CAW                   = 1,
    EVENT_QUILLS                = 2, 
    EVENT_TALON_RAKE            = 3,
};

class boss_jikun : public CreatureScript
{
public:
    boss_jikun() : CreatureScript("boss_jikun") {}

    struct boss_jikunAI : public BossAI
    {
        boss_jikunAI(Creature* creature) : BossAI(creature, DATA_JI_KUN)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset()
        {
            _Reset();
            me->SetReactState(REACT_DEFENSIVE);
        }

        void EnterCombat(Unit* who)
        {
            _EnterCombat();
            //instance->SetData(DATA_CREATE_NEST_LIST_ORDER, me->GetMap()->Is25ManRaid() ? 1 : 0);
            events.ScheduleEvent(EVENT_CAW, 35000);
            events.ScheduleEvent(EVENT_TALON_RAKE, 20000);
            events.ScheduleEvent(EVENT_QUILLS, 60000);
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_TALON_RAKE:
                    if (me->getVictim())
                    {
                        if (me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                        {
                            uint8 pos = urand(0, 1);
                            switch (pos)
                            {
                            case 0:
                                DoCast(me->getVictim(), SPELL_TALON_RAKE);
                                break;
                            case 1:
                                DoCast(me->getVictim(), SPELL_INFECTED_TALONS);
                                break;
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_TALON_RAKE, 20000);
                    break;
                case EVENT_QUILLS:
                    DoCast(me, SPELL_QUILLS);
                    events.ScheduleEvent(EVENT_QUILLS, 60000);
                    break;
                case EVENT_CAW:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                        DoCast(target, SPELL_CAW);
                    events.ScheduleEvent(EVENT_CAW, 35000);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_jikunAI(creature);
    }
};

//69626
class npc_incubater : public CreatureScript
{
public:
    npc_incubater() : CreatureScript("npc_incubater") {}

    struct npc_incubaterAI : public ScriptedAI
    {
        npc_incubaterAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetDisplayId(1126);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* pInstance;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_incubaterAI(creature);
    }
};

//68194
class npc_young_egg_of_jikun : public CreatureScript
{
public:
    npc_young_egg_of_jikun() : CreatureScript("npc_young_egg_of_jikun") {}

    struct npc_young_egg_of_jikunAI : public ScriptedAI
    {
        npc_young_egg_of_jikunAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* pInstance;

        void Reset(){}

        void JustDied(Unit* killer)
        {
            //DoCast(me, SPELL_FEATHER_AT, true);
            /*float x, y;
            GetPosInRadiusWithRandomOrientation(me, 3.0f, x, y);
            me->SummonCreature(NPC_HATCHLING, x, y, me->GetPositionZ() + 3.0f);*/
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_young_egg_of_jikunAI(creature);
    }
};

//69885
class npc_jump_to_boss_platform : public CreatureScript
{
public:
    npc_jump_to_boss_platform() : CreatureScript("npc_jump_to_boss_platform") {}

    struct npc_jump_to_boss_platformAI : public ScriptedAI
    {
        npc_jump_to_boss_platformAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
        }
        InstanceScript* pInstance;

        void Reset()
        {
            DoCast(me, SPELL_JUMP_TO_B_PLATFORM, true);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jump_to_boss_platformAI(creature);
    }
};

//218543
class go_ji_kun_feather : public GameObjectScript
{
public:
    go_ji_kun_feather() : GameObjectScript("go_ji_kun_feather") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        InstanceScript* pInstance = (InstanceScript*)go->GetInstanceScript();
        if (!pInstance)
            return false;

        if (!player->HasAura(SPELL_JIKUN_FLY))
            player->CastSpell(player, SPELL_JIKUN_FLY, true);

        return true;

        /*if (player->HasAura(SPELL_LESSON_OF_ICARUS))
            return true;

        player->CastCustomSpell(SPELL_DAEDALIAN_WINGS, SPELLVALUE_AURA_STACK, 4, player);
        go->Delete();
        return false;*/
    }
};

//133755
class spell_jikun_fly : public SpellScriptLoader
{
public:
    spell_jikun_fly() : SpellScriptLoader("spell_jikun_fly") { }

    class spell_jikun_fly_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_jikun_fly_SpellScript);

        SpellCastResult CheckCast()
        {
            if (GetCaster())
            {
                if (GetCaster()->GetMap() && GetCaster()->GetMap()->GetId() == 1098) //Throne of Thunder
                    if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                        if (instance->GetBossState(DATA_JI_KUN) == IN_PROGRESS)
                            return SPELL_CAST_OK;

                //Check cast failed, remove aura
                GetCaster()->RemoveAurasDueToSpell(SPELL_DAEDALIAN_WINGS);
            }
            return SPELL_FAILED_NOT_HERE;
        }

        void HandleAfterCast()
        {
            if (GetCaster())
            {
                if (Aura* aura = GetCaster()->GetAura(SPELL_DAEDALIAN_WINGS))
                {
                    if (aura->GetStackAmount() > 1)
                        aura->SetStackAmount(aura->GetStackAmount() - 1);
                    else
                        aura->Remove();
                }
            }
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_jikun_fly_SpellScript::CheckCast);
            AfterCast += SpellCastFn(spell_jikun_fly_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_jikun_fly_SpellScript();
    }
};

void AddSC_boss_jikun()
{
    new boss_jikun();
    new npc_incubater();
    new npc_young_egg_of_jikun();
    new npc_jump_to_boss_platform();
    new go_ji_kun_feather();
    new spell_jikun_fly();
}
