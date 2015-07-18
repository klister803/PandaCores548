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

    //< S3
    SPELL_TRUSTED_BY_THE_ASHTONGUE      = 134206,

    //< S4
    SPELL_SHADOW_BOLT_2                 = 12739,
    SPELL_SOUL_BLAST                    = 50992,
    SPELL_SUMMON_HUNGERING_SOUL_FRAGMENT = 134207, //< SS
    SPELL_MEMORY_OF_THE_RELIQUARY       = 134210, //< Scene
    SPELL_SPAWN_THE_RELIQUARY           = 134211,

    //< S5
    SPELL_SPELLFLAME                    = 134234, //< dummy
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
    SPELL_SUMMON_FEL_IMP                = 112866,
    SPELL_INTERRUPTED                   = 134340,
    SPELL_PLUNDER                       = 134323,
    SPELL_UPDATE_PHASE_SHIFT            = 82238,
    SPELL_APPRAISAL                     = 134280,
    SPELL_GOLD_RING                     = 134290,
    SPELL_HIGH_ELF_STATUE               = 134294,
    SPELL_FAMILY_JEWELS                 = 134298,
    SPELL_ANCIENT_ORC_SHIELD            = 134302,
    SPELL_EXPENSIVE_RUBY                = 134283,
    SPELL_SPELLSTONE_NECKABLE           = 134287,
    SPELL_SMALL_PILE_OF_GOLD            = 134291,
    SPELL_GOLDON_POTION                 = 134295,
    SPELL_FRUIT_BOWL                    = 134299,
    SPELL_RUNEBLADE                     = 134303,
    SPELL_SPARKLING_SAPPHIRE            = 134284,
    SPELL_DIAMONG_RING                  = 134288,
    SPELL_LARGE_PILE_OF_GOLD            = 134292,
    SPELL_GOLDER_PLATTER                = 134296,
    SPELL_ORNATE_PORTRAIT               = 134300,
    SPELL_FRAGRANT_PERFUME              = 134281,
    SPELL_JADE_KITTEN                   = 134285,
    SPELL_RUBY_RING                     = 134289,
    SPELL_GOLDEN_GOBLET                 = 134293,
    SPELL_YARN                          = 134297,
    SPELL_ROPE_BINDINGS                 = 134301,
    SPELL_CHEAP_COLOGNE                 = 134282,
    SPELL_RUBY_NEACKABLE                = 134286,

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
    EVENT_21,
    EVENT_22,
    EVENT_23,
    EVENT_24,
    EVENT_25,
    EVENT_26,
    EVENT_27,
    EVENT_28,
    EVENT_29,
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
    //< s 6 / 7 points
    {777.0364f, 71.70834f, 112.8220f}, //<  +0
    {778.7274f, 92.57291f, 112.7408f}, //<  +5
    {772.1349f, 110.3631f, 112.7561f}, //<  +2
};

Position const soulsPositions[]
{
    {449.8871f, 198.7309f, 95.17565f},
    {541.7239f, 156.0469f, 95.00831f},
    {445.8160f, 136.8576f, 94.91354f},
    {457.3507f, 122.5260f, 94.83651f},
    {458.5434f, 204.1754f, 95.42035f},
    {469.0729f, 215.0104f, 94.84925f},
    {531.6129f, 218.2778f, 94.89791f},
    {546.2726f, 200.2431f, 94.89764f},
    {543.2257f, 135.7830f, 95.17218f},
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
        switch (action)
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
            stage7 = 0;
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
                case ACTION_3:
                    events.ScheduleEvent(EVENT_21, 2 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER && me->GetDistance2d(who) < 30.0f && !stage7)
            {
                Talk(4);
                stage7 = true;
            }
        }

        void WaypointReached(uint32 id)
        {
            switch (id)
            {
                case EVENT_23:
                    me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
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
                        events.ScheduleEvent(EVENT_7, timer = +0 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_8, timer = +2 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_9, timer = +3 * IN_MILLISECONDS);
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
                        me->GetMotionMaster()->MovePoint(EVENT_7, akamaWP[0]);
                        break;
                    case EVENT_8:
                        me->GetMotionMaster()->MovePoint(EVENT_8, akamaWP[1]);
                        break;
                    case EVENT_9:
                        me->GetMotionMaster()->MovePoint(EVENT_9, akamaWP[2]);
                        break;
                    case EVENT_10:
                        me->GetMotionMaster()->MovePoint(EVENT_10, akamaWP[3]);
                        break;
                    case EVENT_11:
                        me->GetMotionMaster()->MovePoint(EVENT_11, akamaWP[4]);
                        break;
                    case EVENT_12:
                        me->GetMotionMaster()->MovePoint(EVENT_12, akamaWP[5]);
                        break;
                    case EVENT_13:
                        me->GetMotionMaster()->MovePoint(EVENT_13, akamaWP[6]);
                        Talk(0);
                        break;
                    case EVENT_14:
                        me->GetMotionMaster()->MovePoint(EVENT_14, akamaWP[7]);
                        break;
                    case EVENT_15:
                        Talk(1);
                        me->GetMotionMaster()->MovePoint(EVENT_15, akamaWP[8]);
                        break;
                    case EVENT_16:
                        me->GetMotionMaster()->MovePoint(EVENT_16, akamaWP[9]);
                        break;
                    case EVENT_17:
                        me->GetMotionMaster()->MovePoint(EVENT_17, akamaWP[10]);
                        break;
                    case EVENT_18:
                        events.ScheduleEvent(EVENT_19, 11 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(EVENT_18, akamaWP[11]);
                        break;
                    case EVENT_19:
                        events.ScheduleEvent(EVENT_20, 5 * IN_MILLISECONDS);
                        Talk(2);
                        break;
                    case EVENT_20:
                        Talk(3);
                        break;
                    case EVENT_21:
                        events.ScheduleEvent(EVENT_22, 5 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(EVENT_21, akamaWP[12]);
                        me->SetOrientation(3.266473f);
                        break;
                    case EVENT_22:
                        events.ScheduleEvent(EVENT_23, 2 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(EVENT_22, akamaWP[13]);
                        break;
                    case EVENT_23:
                        me->GetMotionMaster()->MovePoint(EVENT_23, akamaWP[14]);
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
        bool stage7;
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

class npc_suffering_soul_fragment : public CreatureScript
{
public:
    npc_suffering_soul_fragment() : CreatureScript("npc_suffering_soul_fragment") { }

    struct npc_suffering_soul_fragmentAI : public ScriptedAI
    {
        npc_suffering_soul_fragmentAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_1, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_2, 6 * IN_MILLISECONDS);
        }

        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_1, 6 * IN_MILLISECONDS);
                        DoCast(SPELL_SHADOW_BOLT_2);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_2, 15 * IN_MILLISECONDS);
                        DoCast(SPELL_SOUL_BLAST);
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
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_suffering_soul_fragmentAI>(creature);
    }
};

//< should be summoned by SPELL_SUMMON_HUNGERING_SOUL_FRAGMENT and cast some spells, but this data missed in sniffs ;(
class npc_hungering_soul_fragment : public CreatureScript
{
public:
    npc_hungering_soul_fragment() : CreatureScript("npc_hungering_soul_fragment") { }

    struct npc_hungering_soul_fragmentAI : public ScriptedAI
    {
        npc_hungering_soul_fragmentAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/)
        { }

        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
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
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_hungering_soul_fragmentAI>(creature);
    }
};

class npc_essence_of_order : public CreatureScript
{
public:
    npc_essence_of_order() : CreatureScript("npc_essence_of_order") { }

    struct npc_essence_of_orderAI : public ScriptedAI
    {
        npc_essence_of_orderAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetVisible(false);
        }

        void DoAction(const int32 action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_1, 2 * MINUTE + 42 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();

            summons.DespawnAll();
        }

        void EnterCombat(Unit* /*who*/)
        {
            instance->SetData(DATA_ESSENCE_OF_ORDER_EVENT, IN_PROGRESS);

            events.ScheduleEvent(EVENT_4, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_5, 35 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_6, 45 * IN_MILLISECONDS);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(3);
            summons.DespawnAll();

            instance->SetData(DATA_ESSENCE_OF_ORDER_EVENT, DONE);
        }

        void EnterEvadeMode()
        {
            summons.DespawnAll();

            instance->SetData(DATA_ESSENCE_OF_ORDER_EVENT, TO_BE_DECIDED);

            me->Respawn(true);
            me->SetVisible(true);
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);

            switch (summon->GetEntry())
            {
                case NPC_LOST_SOULS:
                    if (Unit* target = summon->FindNearestPlayer(200.0f))
                    {
                        summon->AddThreat(target, 10.0f);
                        summon->Attack(target, true);
                        summon->GetMotionMaster()->MoveChase(target);
                    }
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_2, 2 * IN_MILLISECONDS);
                        me->SetVisible(true);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_3, 5 * IN_MILLISECONDS);
                        Talk(0);
                        break;
                    case EVENT_3:
                        Talk(1);
                        break;
                    case EVENT_4:
                        events.ScheduleEvent(EVENT_4, 15 * IN_MILLISECONDS);
                        events.DelayEvents(3 * IN_MILLISECONDS);
                        DoCast(SPELL_SPELLFLAME);
                        break;
                    case EVENT_5:
                        events.ScheduleEvent(EVENT_5, 35 * IN_MILLISECONDS);
                        events.DelayEvents(2 * IN_MILLISECONDS);
                        DoCast(SPELL_HELLFIRE);
                        break;
                    case EVENT_6:
                        events.ScheduleEvent(EVENT_6, 45 * IN_MILLISECONDS);
                        for (uint8 i = 0; i < 8; i++)
                            me->SummonCreature(NPC_LOST_SOULS, soulsPositions[i]);
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
        SummonList summons;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<npc_essence_of_orderAI>(creature);
    }
};

class at_pursuing_the_black_harvest_main : public AreaTriggerScript
{
public:
    at_pursuing_the_black_harvest_main() : AreaTriggerScript("at_pursuing_the_black_harvest_main") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* at, bool enter)
    {
        if (enter && player->GetInstanceScript())
        {
            switch (at->id)
            {
                case 8696:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34539, 1);  //< set stage 2
                    return true;
                case 8699:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT, 34545, 1);  //< set stage 4
                    return true;
                case 8698:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT, 34547, 1); //< set stage 5
                    player->CastSpell(player, SPELL_MEMORY_OF_THE_RELIQUARY);
                    player->CastSpell(player, SPELL_SPAWN_THE_RELIQUARY);
                    if (Creature* essence = player->FindNearestCreature(NPC_ESSENCE_OF_ORDER, 200.0f))
                        essence->AI()->DoAction(ACTION_1);
                    return true;
                case 8701:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34554, 1); //< set stage 7
                    return true;
                case 8706:
                    if (player->GetInstanceScript()->GetData(DATA_NOBEL_EVENT) == NOT_STARTED)
                    {
                        player->AddAura(SPELL_INTERRUPTED, player);
                        player->GetInstanceScript()->SetData(DATA_NOBEL_EVENT, DONE);
                    }
                    return true;
                case 8702:
                    player->AddAura(SPELL_UPDATE_PHASE_SHIFT, player);
                    player->AddAura(SPELL_PLUNDER, player);
                    player->PlayerTalkClass->SendQuestQueryResponse(32340); //< i hope it's right way... SMSG_QUEST_GIVER_QUEST_DETAILS in sniffs
                    return true;
                case 8708:
                    player->TeleportTo(1112, 786.0955f, 304.3524f, 319.7598f, 0.0f);
                    return true;
                case 8908:
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34559, 1); //< set final stage
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

class go_treasure_chest : public GameObjectScript
{
public:
    go_treasure_chest() : GameObjectScript("go_treasure_chest") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34558, 1); //< set stage 8

            player->CastSpell(player, SPELL_APPRAISAL);

            for (uint8 i = 0; i < 20; i++)
            {
                player->CastSpell(player, SPELL_GOLD_RING);
                player->CastSpell(player, SPELL_HIGH_ELF_STATUE);
                player->CastSpell(player, SPELL_FAMILY_JEWELS);
                player->CastSpell(player, SPELL_ANCIENT_ORC_SHIELD);
                player->CastSpell(player, SPELL_EXPENSIVE_RUBY);
                player->CastSpell(player, SPELL_SPELLSTONE_NECKABLE);
                player->CastSpell(player, SPELL_SMALL_PILE_OF_GOLD);
                player->CastSpell(player, SPELL_GOLDON_POTION);
                player->CastSpell(player, SPELL_FRUIT_BOWL);
                player->CastSpell(player, SPELL_RUNEBLADE);
                player->CastSpell(player, SPELL_SPARKLING_SAPPHIRE);
                player->CastSpell(player, SPELL_DIAMONG_RING);
                player->CastSpell(player, SPELL_LARGE_PILE_OF_GOLD);
                player->CastSpell(player, SPELL_GOLDER_PLATTER);
                player->CastSpell(player, SPELL_ORNATE_PORTRAIT);
                player->CastSpell(player, SPELL_FRAGRANT_PERFUME);
                player->CastSpell(player, SPELL_JADE_KITTEN);
                player->CastSpell(player, SPELL_RUBY_RING);
                player->CastSpell(player, SPELL_GOLDEN_GOBLET);
                player->CastSpell(player, SPELL_YARN);
                player->CastSpell(player, SPELL_ROPE_BINDINGS);
                player->CastSpell(player, SPELL_CHEAP_COLOGNE);
                player->CastSpell(player, SPELL_RUBY_NEACKABLE);
            }

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
    new npc_suffering_soul_fragment();
    new npc_hungering_soul_fragment();
    new npc_essence_of_order();

    new at_pursuing_the_black_harvest_main();

    new go_cospicuous_illidari_scroll();
    new go_treasure_chest();
}
