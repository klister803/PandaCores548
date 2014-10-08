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
#include "ScriptedEscortAI.h"
#include "CreatureTextMgr.h"

// UPDATE `creature` SET `spawnMask` = '16632' WHERE map = 1136;
enum eSpells
{
    SPELL_CORRUPTED_PRISON          = 144574, //Corrupted Prison
    SPELL_CORRUPTED_PRISON2         = 144636, //Corrupted Prison
    SPELL_IMPRISON                  = 144563, //Imprison
    SPELL_MARK_OF_ARROGANCE         = 144351, //Mark of Arrogance
    SPELL_REACHING_ATTACK           = 144774, //Reaching Attack
    SPELL_SELF_REFLECTION           = 144800, //Self-Reflection
    SPELL_WOUNDED_PRIDE             = 144358, //Wounded Pride

    SPELL_UNLEASHED                 = 144832, //Unleashed

    //Pride
    SPELL_SWELLING_PRIDE            = 144400, //Swelling Pride
    SPELL_BURSTING_PRIDE            = 144910, //Bursting Pride
    SPELL_PROJECTION                = 146822, //Projection
    SPELL_AURA_OF_PRIDE             = 146817, //Aura of Pride
    SPELL_OVERCOME                  = 144843, //Overcome
    
    //Norushen
    SPELL_DOOR_CHANNEL              = 145979, //Door Channel
    SPELL_FINAL_GIFT                = 144854, //Final Gift
    SPELL_GIFT_OF_THE_TITANS        = 144359, //Gift of the Titans
    SPELL_POWER_OF_THE_TITANS       = 144364, //Power of the Titans

    //Lingering Corruption
    SPELL_CORRUPTION_TOUCH          = 149207, //Corrupted Touch

    //Reflection
    SPELL_SELF_REFLECTION_CAST      = 144788, //Self-Reflection
};

class boss_sha_of_pride : public CreatureScript
{
    public:
        boss_sha_of_pride() : CreatureScript("boss_sha_of_pride") {}

        struct boss_sha_of_prideAI : public ScriptedAI
        {
            boss_sha_of_prideAI(Creature* creature) : ScriptedAI(creature)
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
            return new boss_sha_of_prideAI(creature);
        }
};

class npc_sha_of_pride_norushen : public CreatureScript
{
public:
    npc_sha_of_pride_norushen() : CreatureScript("npc_sha_of_pride_norushen") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_norushenAI (creature);
    }

    struct npc_sha_of_pride_norushenAI : public npc_escortAI
    {
        npc_sha_of_pride_norushenAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        bool start;

        void Reset()
        {
            start = false;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (start)return;
            start = true;

            ZoneTalk(TEXT_GENERIC_0, me->GetGUID());
            Start(false, false);
        }

        void WaypointReached(uint32 i)
        {
            switch(i)
            {
                case 4:
                    SetEscortPaused(true);
                    DoCast(me, SPELL_DOOR_CHANNEL, false);
                    //
                    if (GameObject* door = instance->instance->GetGameObject(instance->GetData64(GO_NORUSHEN_EX_DOOR)))
                        door->SetGoState(GO_STATE_ACTIVE);
                    events.ScheduleEvent(EVENT_1, 2000);
                    break;
                case 5:
                    SetEscortPaused(true);
                    ZoneTalk(TEXT_GENERIC_1, me->GetGUID());
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        if (Creature* lo = instance->instance->GetCreature(instance->GetData64(NPC_LOREWALKER_CHO3)))
                            lo->AI()->DoAction(EVENT_1);
                        SetEscortPaused(false);
                        break;
                }
            }
            
        }
    };
};

class npc_sha_of_pride_lowerwalker : public CreatureScript
{
public:
    npc_sha_of_pride_lowerwalker() : CreatureScript("npc_sha_of_pride_lowerwalker") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_lowerwalkerAI (creature);
    }

    struct npc_sha_of_pride_lowerwalkerAI : public npc_escortAI
    {
        npc_sha_of_pride_lowerwalkerAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            Start(false, true);
        }

        void DoAction(int32 const action)
        {
            SetEscortPaused(false);
        }

        void WaypointReached(uint32 i)
        {
            switch(i)
            {
                case 5:
                    SetEscortPaused(true);
                    //no break
                case 6:
                    SetEscortPaused(true);
                    events.ScheduleEvent(EVENT_1, 1000);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            //while (uint32 eventId = events.ExecuteEvent())
            //{
            //    switch (eventId)
            //    {

            //    }
            //}
        }
    };
};
void AddSC_boss_sha_of_pride()
{
    new boss_sha_of_pride();
    new npc_sha_of_pride_norushen();
    new npc_sha_of_pride_lowerwalker();
}
