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
    SPELL_TRUSTED_BY_THE_ASHTONGUE      = 134206,

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
    EVENT_6,
    EVENT_7,
    EVENT_8,
    EVENT_9,
    EVENT_10,
    EVENT_11,
    EVENT_12,
    EVENT_13,
    EVENT_14,
    EVENT_15,
    EVENT_16,
    EVENT_17,
    EVENT_18,
    EVENT_19,
    EVENT_20,
};

enum Actions
{
    ACTION_NONE,

    ACTION_1,
    ACTION_2,
};

Position const atPos[]
{
    {703.721f, 574.901f, 112.628f} //< 8696
};

enum Sounds
{ };

Position const akamaWP[]
{
    {721.4444f, 355.3133f, 125.3953f}, //<  +0
    {734.1794f, 343.3147f, 125.4458f}, //<  +2
    {749.3447f, 321.5173f, 125.4249f}, //<  +3
    {763.5404f, 314.8681f, 125.3804f}, //<  +3
    {792.8096f, 306.7504f, 113.0866f}, //<  +4
    {802.1400f, 278.0078f, 112.9999f}, //<  +4
    {804.4133f, 246.3474f, 113.0019f}, //<  +5
    {804.2388f, 128.3550f, 112.5224f}, //<  +13
    {792.6276f, 99.26649f, 113.0114f}, //<  +17
    {757.0466f, 65.21021f, 112.9879f}, //<  +6
    {754.4785f, 64.58849f, 112.9997f}, //<  +5
    {694.0858f, 67.66933f, 112.8861f}, //<  +3
};

class npc_akama : public CreatureScript
{
public:
    npc_akama() : CreatureScript("npc_akama") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            player->ADD_GOSSIP_ITEM(1, "Король Ринн разыскивает членов совета Мрачной Жатвы, которые недавно были здесь.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        return false;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        switch(action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->ADD_GOSSIP_ITEM(1, "Акама, покажи дорогу!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                creature->AI()->DoAction(ACTION_2);
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34543, 1); //< set stage 3
                break;
            default:
                break;
        }
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
            timer = 0;
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_1, 2 * IN_MILLISECONDS, 1);
                    break;
                case ACTION_2:
                    events.ScheduleEvent(EVENT_5, 1 * IN_MILLISECONDS, 1);
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
                        events.ScheduleEvent(EVENT_3, 2 * IN_MILLISECONDS, 1);
                        me->RemoveAura(SPELL_STEALTH);
                        if (Player* plr = me->FindNearestPlayer(50.0f))
                        {
                            plr->CastSpell(plr, SPELL_UPDATE_PLAYER_PHASE_AURAS);
                            plr->AddAura(SPELL_TRUSTED_BY_THE_ASHTONGUE, plr);
                        }
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
                    case EVENT_5:
                        events.ScheduleEvent(EVENT_6, 2 * IN_MILLISECONDS, 1);
                        Talk(7);
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    case EVENT_6:
                        events.ScheduleEvent(EVENT_7, timer= + 0 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_8, timer= + 2 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_9, timer= + 3 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_10, timer += 3 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_11, timer += 4 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_12, timer += 4 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_13, timer += 5 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_14, timer += 13 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_15, timer += 17 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_16, timer += 6 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_17, timer += 5 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_18, timer += 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_7:
                        Talk(8);
                        me->GetMotionMaster()->MovePoint(1, akamaWP[0]);
                        break;
                    case EVENT_8:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[1]);
                        break;
                    case EVENT_9:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[2]);
                        break;
                    case EVENT_10:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[3]);
                        break;
                    case EVENT_11:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[4]);
                        break;
                    case EVENT_12:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[5]);
                        break;
                    case EVENT_13:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[6]);
                        Talk(0);
                        break;
                    case EVENT_14:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[7]);
                        break;
                    case EVENT_15:
                        Talk(1);
                        me->GetMotionMaster()->MovePoint(1, akamaWP[8]);
                        break;
                    case EVENT_16:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[9]);
                        break;
                    case EVENT_17:
                        me->GetMotionMaster()->MovePoint(1, akamaWP[10]);
                        break;
                    case EVENT_18:
                        events.ScheduleEvent(EVENT_19, 11 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(1, akamaWP[11]);
                        break;
                    case EVENT_19:
                        events.ScheduleEvent(EVENT_20, 6 * IN_MILLISECONDS);
                        Talk(2);
                        break;
                    case EVENT_20:
                        Talk(3);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        InstanceScript* instance;
        EventMap events;
        uint32 timer;
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

        void MoveInLineOfSight(Unit* who)
        {
            if (who->HasAura(SPELL_TRUSTED_BY_THE_ASHTONGUE))
                return;

            if (!me->GetDistance(atPos[0]) < 50.0f && !talk)
            {
                Talk(1);
                talk = true;
            }

            if (me->GetDistance(atPos[0]) < 50.0f || (me->GetDistance(atPos[0]) < 50.0f && who->GetEntry() == 1860) || who->GetEntry() == 58960)
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
            callForHelp = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (!callForHelp)
            {
                Talk(0);
                me->CallForHelp(50.0f);
                me->DoFleeToGetAssistance();
                callForHelp = true;
            }
        }

        void UpdateAI(uint32 diff)
        {
            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
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
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34539, 1);  //< set stage 2
                    return true;
                case 8698:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT, 34547, 1);
                    return true;
                case 8699:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT, 34545, 1);  //< set stage 4
                    break;
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
