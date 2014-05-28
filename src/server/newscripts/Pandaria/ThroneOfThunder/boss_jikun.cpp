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
    SPELL_JI_KUN_FEATHER_AURA   = 140014,
    SPELL_JI_KUN_FEATHER_USE_EF = 140013,

    //Ji Kun
    SPELL_CAW                   = 138926,
    SPELL_QUILLS                = 134380,
    SPELL_TALON_RAKE            = 134366,
    SPELL_INFECTED_TALONS       = 140092,
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
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_CAW,        35000);
                events.ScheduleEvent(EVENT_TALON_RAKE, 20000);
                events.ScheduleEvent(EVENT_QUILLS,     60000);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

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

            if (player->GetPositionZ() > -7.0f) //on platform
                player->GetMotionMaster()->MoveJump(6112.08f, 4297.98f, -31.8619f, 25.0f, 25.0f);
            else
                player->GetMotionMaster()->MoveJump(6096.35f, 4408.16f, -6.2604f, 25.0f, 25.0f);
            return true;
        }
};

void AddSC_boss_jikun()
{
    new boss_jikun();
    new go_ji_kun_feather();
}
