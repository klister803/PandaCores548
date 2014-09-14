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
};

class boss_fallen_protectors : public CreatureScript
{
    public:
        boss_fallen_protectors() : CreatureScript("boss_fallen_protectors") {}

        struct boss_fallen_protectorsAI : public ScriptedAI
        {
            boss_fallen_protectorsAI(Creature* creature) : ScriptedAI(creature)
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
            return new boss_fallen_protectorsAI(creature);
        }
};


//Golden Lotus
class npc_golden_lotus_control : public CreatureScript
{
public:
    npc_golden_lotus_control() : CreatureScript("npc_golden_lotus_control") { }

    struct npc_wind_vehicleAI : public ScriptedAI
    {        
        npc_wind_vehicleAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();            
        }

        enum data
        {
            SUMMON_MOVER        = 143705,
            SPELL_FACE_CHANNEL  = 116351,
        };

        InstanceScript* instance;
        EventMap events;

        //void JustSummoned(Creature* creature)
        //{
        //    if (creature->GetEntry() == NPC_GOLD_LOTOS_MOVER)
        //    {
        //        creature->SetDisplayId(48920);
        //        creature->GetMotionMaster()->MoveIdle();
        //        creature->LoadPath(creature->GetEntry());
        //        creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        //        creature->GetMotionMaster()->Initialize();
        //        if (creature->isAlive())                            // dead creature will reset movement generator at respawn
        //        {
        //            creature->setDeathState(JUST_DIED);
        //            creature->Respawn(true);
        //        }
        //    }
        //};

        void Reset()
        {
            //Use manual spawn.
            //me->CastSpell(me, SUMMON_MOVER, true);
            events.RescheduleEvent(EVENT_1, 1000);
        }
        
        void OnCharmed(bool /*apply*/)
        {
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (apply)
            {
                if (Creature* mover = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MOVER)))
                {
                    who->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                    if (!me->HasAura(SPELL_FACE_CHANNEL))
                        me->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {

            }
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wind_vehicleAI(creature);
    }
};

void AddSC_boss_fallen_protectors()
{
    new boss_fallen_protectors();
    new npc_golden_lotus_control();
}
