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
#include "heart_of_fear.h"

enum eSpells
{
    SPELL_SONG_OF_THE_IMPERATOR   = 123793,
    SPELL_PHEROMONES_OF_ZEAL      = 123812,
    SPELL_PHEROMONES_OF_ZEAL_BUFF = 123833,
    SPELL_GAS_VISUAL              = 123811,
};

enum eEvents
{
    //Vizier Zorlok
    EVENT_GO_LAST_POS         = 6,
    EVENT_GO_NEXT_PLATFORM    = 7,

    //Gas Controller
    EVENT_CHECK_PLAYERS       = 8,
};

enum Actions
{
    //Gas Controller
    ACTION_GAS_ON             = 3,
    ACTION_GAS_OFF            = 4,
};

Position const centerpos = {-2275.27f, 259.1f, 415.34f}; //In air
Position const centerlandpos = {-2275.41f, 258.33f, 406.38f}; //In land

Position const platformpos[3] = 
{
    {-2312.74f, 298.69f, 409.89f}, 
    {-2235.27f, 217.29f, 409.89f}, 
    {-2312.05f, 221.74f, 409.89f}, 
};

float const curplpos = 409.149f;

class boss_vizier_zorlok : public CreatureScript
{
    public:
        boss_vizier_zorlok() : CreatureScript("boss_vizier_zorlok") {}

        struct boss_vizier_zorlokAI : public BossAI
        {
            boss_vizier_zorlokAI(Creature* creature) : BossAI(creature, DATA_VIZIER_ZORLOK)
            {
                instance = creature->GetInstanceScript();
                me->SetCanFly(true);
                me->SetDisableGravity(true);
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            }

            InstanceScript* instance;
            uint8 newindex, lastindex, flycount;

            void Reset()
            {
                _Reset();
                GasControl(false);
                FlyControl(true);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->RemoveAurasDueToSpell(SPELL_PHEROMONES_OF_ZEAL_BUFF);
                newindex = 0;
                lastindex = urand(0, 2);
                flycount = 0;
            }

            void GasControl(bool state)
            {
                if (instance)
                {
                    if (state)
                    {
                        if (Creature* gc = me->GetCreature(*me, instance->GetData64(NPC_GAS_CONTROLLER)))
                            gc->AI()->DoAction(ACTION_GAS_ON);
                    }
                    else
                    {
                        if (Creature* gc = me->GetCreature(*me, instance->GetData64(NPC_GAS_CONTROLLER)))
                            gc->AI()->DoAction(ACTION_GAS_OFF);
                    }
                }
            }
            
            void FlyControl(bool apply)
            {
                if (apply)
                {
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetCanFly(true);
                    me->SetDisableGravity(true);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                }
                else
                {
                    me->SetCanFly(false);
                    me->SetDisableGravity(false);
                    me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                }
            }
            
            void EnterCombat(Unit* /*who*/)
            {
                if (instance)
                    instance->SetBossState(DATA_VIZIER_ZORLOK, IN_PROGRESS);
                FlyControl(true);
                GasControl(true);
                GoNextRandomPlatform();
            }
            
            void MovementInform(uint32 type, uint32 id)
            {
                if (type == POINT_MOTION_TYPE)
                {
                    if (id == 1)
                    {
                        FlyControl(false);
                        if (flycount < 3)
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        else
                            me->AddAura(SPELL_PHEROMONES_OF_ZEAL_BUFF, me);
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f);
                    }
                }
            }
            
            void GoNextRandomPlatform()
            {
                do
                {
                    newindex = urand(0, 2);
                }
                while (newindex == lastindex);

                lastindex = newindex;
                // In future send newindex in id point, for specific platform events.
                me->GetMotionMaster()->MovePoint(1, platformpos[newindex].GetPositionX(), platformpos[newindex].GetPositionY(), platformpos[newindex].GetPositionZ());
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (HealthBelowPct(80) && !flycount ||
                    HealthBelowPct(60) && flycount == 1)
                {
                    flycount++;
                    FlyControl(true);
                    me->GetMotionMaster()->MoveJump(centerpos.GetPositionX(), centerpos.GetPositionY(), centerpos.GetPositionZ(),10.0f, 10.0f );
                    events.ScheduleEvent(EVENT_GO_NEXT_PLATFORM, 3000);
                }
                else if (HealthBelowPct(40) && flycount == 2)
                {
                    flycount++;
                    FlyControl(true);
                    GasControl(false);
                    me->GetMotionMaster()->MoveJump(centerpos.GetPositionX(), centerpos.GetPositionY(), centerpos.GetPositionZ(),10.0f, 10.0f );
                    events.ScheduleEvent(EVENT_GO_LAST_POS, 3000);
                }
            }
            
            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                Map::PlayerList const& players = me->GetMap()->GetPlayers();
                if (!players.isEmpty())
                {
                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if (Player* pPlayer = itr->getSource())
                            me->GetMap()->ToInstanceMap()->PermBindAllPlayers(pPlayer);
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_GO_NEXT_PLATFORM:
                        GoNextRandomPlatform();
                        break;
                    case EVENT_GO_LAST_POS:
                        me->GetMotionMaster()->MovePoint(1, centerlandpos.GetPositionX(), centerlandpos.GetPositionY(), centerlandpos.GetPositionZ());
                        break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_vizier_zorlokAI(creature);
        }
};

class npc_gas_controller : public CreatureScript
{
    public:
        npc_gas_controller() : CreatureScript("npc_gas_controller") {}

        struct npc_gas_controllerAI : public CreatureAI
        {
            npc_gas_controllerAI(Creature* creature) : CreatureAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);
                me->SetDisplayId(11686);
                gaseoff = false;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* instance;
            EventMap events;
            bool gaseoff;

            void Reset(){}
            
            void DoAction(const int32 action)
            {
                switch (action)
                {
                case ACTION_GAS_ON:
                    DoCast(me, SPELL_GAS_VISUAL);
                    events.ScheduleEvent(EVENT_CHECK_PLAYERS, 1000);
                    break;
                case ACTION_GAS_OFF:
                    gaseoff = true;
                    if (DynamicObject* tr = me->GetDynObject(SPELL_GAS_VISUAL))
                        tr->RemoveAura();
                    break;
                }
            }

            void CheckPlayers()
            {
                if (Map* map = me->GetMap())
                    if (map->IsDungeon())
                    {
                        Map::PlayerList const &players = map->GetPlayers();
                        for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                        {
                            if (Player* pl = i->getSource())
                            {
                                if (pl->isAlive() && pl->GetPositionZ() < curplpos)
                                {
                                    if (!pl->HasAura(SPELL_PHEROMONES_OF_ZEAL))
                                        pl->AddAura(SPELL_PHEROMONES_OF_ZEAL, pl);
                                }
                                else if (pl->isAlive() && pl->GetPositionZ() >= curplpos)
                                    pl->RemoveAurasDueToSpell(SPELL_PHEROMONES_OF_ZEAL);
                            }
                        }

                        if (gaseoff)
                        {
                            gaseoff = false;
                            events.CancelEvent(EVENT_CHECK_PLAYERS);
                            if (instance)
                                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PHEROMONES_OF_ZEAL);
                        }
                        else
                            events.ScheduleEvent(EVENT_CHECK_PLAYERS, 1000);
                    }
            }
            
            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                case EVENT_CHECK_PLAYERS:
                    CheckPlayers();
                    break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_gas_controllerAI(creature);
        }
};

void AddSC_boss_vizier_zorlok()
{
    new boss_vizier_zorlok();
    new npc_gas_controller();
}
