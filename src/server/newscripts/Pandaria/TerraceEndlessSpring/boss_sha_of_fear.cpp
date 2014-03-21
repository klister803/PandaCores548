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
    //Sha
    SPELL_EERIE_SKULL        = 119519,
    SPELL_REACHING_ATTACK    = 119775,
    //Dread spawn
    SPELL_PENETRATING_BOLT   = 129077,
};

enum sSummons
{
    NPC_DREAD_SPAWN          = 61003,
};

enum eEvents
{
    //Sha
    EVENT_SKULL              = 1,
    EVENT_R_ATTACK           = 2, //Use if no target for melee attack
    EVENT_DREAD_SPAWN        = 3,
    //Summon
    EVENT_BOLT               = 4,
};

Position const dspos[2] = 
{
    {-1054.36f, -2783.48f, 38.2692f},
    {-981.31f,  -2782.95f, 38.2682f},
};

float const minpullpos = -2847.0258f;

bool CheckLeiShi(InstanceScript* instance, Creature* caller)
{
    if (instance && caller)
    {
        if (Creature* ls = caller->GetCreature(*caller, instance->GetData64(NPC_LEI_SHI)))
        {
            if (ls->isAlive())
                return true;
        }
    }
    return false;
}

class boss_sha_of_fear : public CreatureScript
{
    public:
        boss_sha_of_fear() : CreatureScript("boss_sha_of_fear") {}

        struct boss_sha_of_fearAI : public BossAI
        {
            boss_sha_of_fearAI(Creature* creature) : BossAI(creature, DATA_SHA_OF_FEAR)
            {
                instance = creature->GetInstanceScript();
                if (instance)
                {
                    if (CheckLeiShi(instance, me))
                    {
                        me->SetVisible(false);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
            }

            InstanceScript* instance;
            uint32 checkvictim;

            void Reset()
            {
                _Reset();
                me->SetReactState(REACT_DEFENSIVE);
                me->setPowerType(POWER_ENERGY);
                me->SetPower(POWER_ENERGY, 0);
                checkvictim = 0;
            }

            void JustReachedHome()
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_DEFENSIVE);
            }

            void RegeneratePower(Powers power, int32 &value)
            {
                if (!me->isInCombat())
                    value = 0;
                else 
                    value = 2;
            }

            void EnterCombat(Unit* who)
            {
                if (instance)
                {
                    if (!CheckPullPlayerPos(who))
                    {
                        EnterEvadeMode();
                        return;
                    }
                }
                _EnterCombat();
                checkvictim = 1500;
                events.ScheduleEvent(EVENT_DREAD_SPAWN, urand(20000, 30000));
                events.ScheduleEvent(EVENT_SKULL,       urand(10000, 15000));
            }

            bool CheckPullPlayerPos(Unit* who)
            {
                if (!who->ToPlayer() || who->GetPositionY() < minpullpos)
                    return false;

                return true;
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(const uint32 diff)
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
                                me->AttackStop();
                                me->SetReactState(REACT_PASSIVE);
                                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                EnterEvadeMode();
                                checkvictim = 0;
                            }
                            else
                                checkvictim = 1500;
                        }
                    }
                    else
                        checkvictim -= diff;
                }

                events.Update(diff);

                while (uint32 eventid = events.ExecuteEvent())
                {
                    switch (eventid)
                    {
                    case EVENT_SKULL:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                            DoCast(target, SPELL_EERIE_SKULL);
                        events.ScheduleEvent(EVENT_SKULL, urand(10000, 15000));
                        break;
                    case EVENT_DREAD_SPAWN:
                        for (uint8 n = 0; n < 2; n++)
                        {
                            if (Creature* ds = me->SummonCreature(NPC_DREAD_SPAWN, dspos[n]))
                                ds->AI()->DoZoneInCombat(ds, 100.0f);
                        }
                        events.ScheduleEvent(EVENT_DREAD_SPAWN, urand(20000, 30000));
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sha_of_fearAI(creature);
        }
};

class npc_dread_spawn : public CreatureScript
{
    public:
        npc_dread_spawn() : CreatureScript("npc_dread_spawn") {}

        struct npc_dread_spawnAI : public ScriptedAI
        {
            npc_dread_spawnAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset(){}
            
            void EnterCombat(Unit* attacker)
            {
                events.ScheduleEvent(EVENT_BOLT, 3000);
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (me->isInFront(attacker))
                    damage = 0; 
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    if (eventId == EVENT_BOLT)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                            DoCast(target, SPELL_PENETRATING_BOLT);
                        events.ScheduleEvent(EVENT_BOLT, 3000);
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_dread_spawnAI(creature);
        }
};

void AddSC_boss_sha_of_fear()
{
    new boss_sha_of_fear();
    new npc_dread_spawn();
}
