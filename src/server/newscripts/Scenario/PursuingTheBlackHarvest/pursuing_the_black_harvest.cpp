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

#include "pursuing_the_black_harvest.h"
#include "ScriptedCreature.h"

enum Texts
{ };

enum Spells
{
    //< Misc
    SPELL_PLACE_EMPOWED_SOULCORE        = 138680, //< SS
    SPELL_DRAIN_FEL_ENERGY              = 139200,

    //< S0
    SPELL_SEARCHING_FOR_INTRUDERS       = 134110, //< AT

    //< S1
    SPELL_NETTED                        = 134111,
    SPELL_BLACKOUT                      = 134112,

    //< S2
    SPELL_UPDATE_PLAYER_PHASE_AURAS     = 134209,
    SPELL_SAP                           = 134205,
    SPELL_STEALTH                       = 86603,

    //< S3 - NO SPELLS
    //< S4 - NO SPELLS

    //< S5
    SPELL_SPELLFLAME                    = 134234, //< SS
    SPELL_SPELLFLAME_TRIGGER            = 134235,
    SPELL_HELLFIRE                      = 134225,
    SPELL_HELLFIRE_TRIGGER              = 134224,

    //< S6
    SPELL_SHOOT                         = 41188,
    SPELL_MULTI_SHOOT                   = 41187,
    SPELL_CLEAVE                        = 15284,
    SPELL_SWEEPING_WING_CLIP            = 39584,
    SPELL_SONIC_STRIKE                  = 41168,
    SPELL_SHADOW_BOLT                   = 34344,
    SPELL_DAZED                         = 1604,
    SPELL_FIREBOLT                      = 134245,
    SPELL_LIGHTING_BOLT                 = 42024,

    //< S7
    SPELL_SUMMON_SHADOWFIEND            = 41159,
    SPELL_SHADOW_INFERNO                = 39646,
    SPELL_SMELT_FLASH                   = 37629,

    //< S8
    SPELL_DEMONIC_GATEWAY               = 138649, //< SS
    SPELL_BURNING_EMBERS                = 138557, //< SS
    SPELL_SOULSHARDS                    = 138556, //< SS
    SPELL_METAMORPHOSIS                 = 138555,
    SPELL_FACE_PLAYER                   = 139053, //< SS
    SPELL_RITUAL_ENSLAVEMENT            = 22987,  //< SS
    SPELL_DOOMGUARD_SUMMON_DND          = 42010,  //< SS
    SPELL_DOOM_BOLT                     = 85692,  //< SS
    SPELL_SUMMONING_PIT_LORD            = 138789, //< SS
    SPELL_FEL_FLAME_BREATH              = 138814,
    SPELL_FEL_FLAME_BREATH_DUMMY        = 138813,
    SPELL_RAID_OF_FIRE                  = 138561,
    SPELL_SEED_OF_TERRIBLE_DESTRUCTION  = 138587,
    SPELL_CLEAVE_2                      = 138794,
    SPELL_AURA_OF_OMNIPOTENCE           = 138563,
    SPELL_CURSE_OF_ULTIMATE_DOOM        = 138558,
    SPELL_EXCRUCIATING_AGONY            = 138560,
    SPELL_DEMONIC_SIPHON                = 138829,
    SPELL_CHAOS_BOLT                    = 138559,
    SPELL_BACKFIRE                      = 138619,
    SPELL_SOULFIRE                      = 138554,
    SPELL_CATACLYSM                     = 138564,
    SPELL_CHARGE                        = 138796,
    SPELL_CHARGE_TRIGGER                = 138827,
    SPELL_FEL_FIREBOLT                  = 138747,

    //< Last Step
    SPELL_ANNIHILATE_DEMONS             = 139141,
    SPELL_DEMONIC_GRASP                 = 139142,
    SPELL_ETERNAL_BANISHMENT            = 139186,

};

enum Events
{
    EVENT_NONE,

    EVENT_1,
    EVENT_2,
    EVENT_3,
    EVENT_4,
    EVENT_5,
};

enum Actions
{
    ACTION_NONE,

    ACTION_1,
};

Position const atPos[]
{
    {703.721f, 574.901f, 112.628f} //< 8696
};

enum Sounds
{ };

class npc_akama : public CreatureScript
{
public:
    npc_akama() : CreatureScript("npc_akama") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            player->ADD_GOSSIP_ITEM_DB(1111111, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        return false;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/)
    { 
        creature->AI()->DoAction(1);
        creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        player->CLOSE_GOSSIP_MENU();

        return true;
    }

    struct npc_akamaAI : public ScriptedAI
    {
        npc_akamaAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_1, 2 * IN_MILLISECONDS, 1);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_2, 3 * IN_MILLISECONDS, 1);
                        if (Player* plr = me->FindNearestPlayer(50.0f))
                            me->AddAura(SPELL_SAP, plr);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_3, 1 * IN_MILLISECONDS, 1);
                        me->RemoveAura(SPELL_STEALTH);
                        if (Player* plr = me->FindNearestPlayer(50.0f))
                            me->CastSpell(plr, SPELL_UPDATE_PLAYER_PHASE_AURAS);
                        break;
                    case EVENT_3:
                        events.ScheduleEvent(EVENT_4, 2 * IN_MILLISECONDS, 1);
                        Talk(5);
                        break;
                    case EVENT_4:
                        Talk(6);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        events.CancelEventGroup(1);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_akamaAI>(creature);
    }
};

class npc_asthongue_primalist : public CreatureScript
{
public:
    npc_asthongue_primalist() : CreatureScript("npc_asthongue_primalist") { }

    struct npc_asthongue_primalistAI : public ScriptedAI
    {
        npc_asthongue_primalistAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();

            callForHelp = false;
            talk = false;
        }

        void DamageTaken(Unit* /*attacker*/, uint32 &damage)
        {
            if (damage && me->GetHealthPct() < 50.0f && !callForHelp)
            {
                me->CallForHelp(50.0f);
                me->AttackStop();
            }
        }

        void EnterCombat(Unit* who)
        {
            if (me->GetDistance(atPos[0]) < 50.0f && (who->GetEntry() == 1860) || who->GetEntry() == 58960)
            {
                me->AttackStop();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                Talk(2);
            }
            else
            {
                events.ScheduleEvent(EVENT_1, 3 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_2, 6 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_2, 10 * IN_MILLISECONDS);
            }
        }

        void MoveInLineOfSight(Unit* /*who*/)
        {
            if (!me->GetDistance(atPos[0]) < 50.0f && !talk)
            {
                Talk(1);
                talk = true;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(SPELL_NETTED);
                        break;
                    case EVENT_2:
                        Talk(0);
                        break;
                    case EVENT_3:
                        events.ScheduleEvent(EVENT_2, 25 * IN_MILLISECONDS);
                        DoCast(SPELL_BLACKOUT);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
        bool callForHelp;
        bool talk;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_asthongue_primalistAI>(creature);
    }
};

class npc_ashtongue_worker : public CreatureScript
{
public:
    npc_ashtongue_worker() : CreatureScript("npc_ashtongue_worker") { }

    struct npc_ashtongue_workerAI : public ScriptedAI
    {
        npc_ashtongue_workerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();

            callForHelp = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (!callForHelp)
            {
                Talk(0);
                me->CallForHelp(50.0f);
                me->DoFleeToGetAssistance();
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
        bool callForHelp;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_ashtongue_workerAI>(creature);
    }
};

class at_pursuing_the_black_harvest_main : public AreaTriggerScript
{
public:
    at_pursuing_the_black_harvest_main() : AreaTriggerScript("at_pursuing_the_black_harvest_main") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* at, bool enter)
    {
        if (enter)
        {
            switch (at->id)
            {
                case 8696:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34539, 1);
                    return true;
                case 8698:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT, 34547, 1);
                    return true;
                default:
                    return false;
            }
        }

        return false;
    }
};

class go_cospicuous_illidari_scroll : public GameObjectScript
{
public:
    go_cospicuous_illidari_scroll() : GameObjectScript("go_cospicuous_illidari_scroll") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Creature* akama = go->FindNearestCreature(NPC_AKAMA, 50.0f))
                akama->AI()->DoAction(ACTION_1);

            return true;
        }

        return false;
    }
};

void AddSC_pursing_the_black_harvest()
{
    new npc_akama();
    new npc_asthongue_primalist();
    new npc_ashtongue_worker();

    new at_pursuing_the_black_harvest_main();

    new go_cospicuous_illidari_scroll();
}
