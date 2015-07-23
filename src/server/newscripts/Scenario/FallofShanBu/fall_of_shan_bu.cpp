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

#include "fall_of_shan_bu.h"
#include "ScriptedCreature.h"

enum Texts
{
    TEXT_WRATHION_START = 15618,
};

enum Spells
{
    //< player self
    SPELL_ATTACKED_PLAYER                       = 138952,
    SPELL_POWER_SURGE_TRIGGER                   = 140068,
    
    //< NPC_WRATHION
    SPELL_SPEC_TEST                             = 138928,
    SPELL_UPDATE_PHASE_SHIFT                    = 134715,
    SPELL_LIGHTING_STRIKE_COSMETIC_5            = 140527,
    SPELL_WRATHION_TRUE_FORM                    = 109924,
    SPELL_LIGHTING_STRIKE_3                     = 140540,
    // SPELL_LIGHTING_STRIKE_COSMETIC

    //< NPC_INVISIBLE_STALKER
    SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1  = 140334,

    //< NPC_THUNDER_FORGE_2
    SPELL_GROUNDED                              = 139252,
    SPELL_LIGHTING_CHASE                        = 138721,
    SPELL_LIGHTING_STRIKE_TRIGGERED_BY_CHASE    = 138720,

    //< NPC_LIGHTING_LANCE
    SPELL_RIDE_VEHICLE_HARDCODED                = 46598,
    SPELL_THUNDER_FORGE_SPEAR_COSMETIC_LSTRIKES = 140100,
    SPELL_THUNDER_FORGE_SPEAR_COSMETIC_SPARKLES = 139956,

    //< NPC_THUNDER_FORGE2
    SPELL_THUNDER_FORGE_BUFF_PEREODIC           = 139422,
    SPELL_THUNDER_FORGE_BUFF                    = 139401,

    // NPC_THUNDER_FORGE_3
    SPELL_THUNDER_FORGE_BUFF_3                  = 139431, //< AT
    SPELL_OVERCHARGED                           = 139397,

    //< NPC_LIGHTING_PILAR_BEAM_STALKER
    SPELL_THUNDER_FORGE_CHARGE                  = 140382,
    SPELL_COSMETIC_LIGHTING_PILLAR_BEAM         = 138090,

    //< NPC_SHANZE_ELECTRO_CUTIONER2
    SPELL_LIGHTING_CHARGE                       = 136495,

    //< NPC_LIGHTING_PILAR_SPARK_STALKER
    SPELL_LIGHTING_PILAR_SPARK_COSMETIC         = 138152,
    SPELL_LIGHTING_PILAR_SPARK_COSMETIC_3       = 138091,
    SPELL_LIGHTING_PILAR_SPARK_COSMETIC_2       = 138153,

    //< NPC_THUNDER_FORGE
    SPELL_LIGHTING_STRIKE                       = 140678,
    SPELL_LIGHTING_STRIKE_COSMETIC_3            = 140681,
    SPELL_LIGHTING_STRIKE_COSMETIC_4            = 140507,
    SPELL_LIGHTING_STRIKE_COSMETIC_2            = 140857,
    
    //< NPC_SHADO_PAN_WARRIOR
    // SPELL_WOUNDED
    // SPELL_JOIN_PLAYER_PARTY
    // SPELL_LEAVE_PLAYER_PARTY

    //< NPC_SHADO_PAN_DEFENDER
    SPELL_TAUNT                                 = 138937,
    SPELL_WOUNDED                               = 138919,
    SPELL_JOIN_PLAYER_PARTY                     = 139282,
    SPELL_PERIODIC_TAUNT_TARGETTING             = 139245,
    SPELL_HEALING_ORB                           = 139249, //< AT
    SPELL_HEALING_ORB_TRIGGER                   = 132744,
    SPELL_LEAVE_PLAYER_PARTY                    = 141057,

    //< NPC_SHANZE_WARRIOR
    SPELL_LUMBERING_CHARGE3                     = 120051,
    SPELL_LUMBERING_CHARGE2                     = 120052,
    SPELL_LUMBERING_CHARGE                      = 142587, //< AT
    SPELL_CLEAVE                                = 59992,

    //< NPC_SHANZE_BATTLEMASTER
    SPELL_STORM_STRIKE                          = 138712,
    SPELL_STORM_STRIKE2                         = 32175,
    SPELL_STORM_STRIKE_OFF_HAND                 = 32175,
    SPELL_FIXATE_BEAM_COSMETIC                  = 142477,
    SPELL_FIXATE                                = 139034,
    SPELL_LIGHTING_WHIRLWIND                    = 138198,
    SPELL_LIGHTING_WHIRLWIND2                   = 138191,
    SPELL_LIGHTING_WHIRLWIND3                   = 138190,

    //< NPC_SHANZE_ELECTRO_CUTIONER
    SPELL_CALL_LIGHTING                         = 138741,

    //< NPC_SHANZE_PYROMANCER
    SPELL_METEOR_TARGETING                      = 139301,
    SPELL_METEOR_AURA                           = 138168,
    SPELL_METEOR_AT                             = 140660,
    SPELL_FIREBALL                              = 9053,
    
    //< NPC_SHANZE_SHADOWCASTER
    SPELL_SHADOW_BOLT                           = 12471,
    SPELL_CORRUPTION                            = 138174,
        
    //< NPC_METEOR_SUMMONER_STALKER
    SPELL_METEOR_STORM_3                          = 139443,
    SPELL_METEOR_STORM_4                          = 139658,
    // SPELL_METEOR_STORM_2

    //< NPC_FORGEMASTER_VULKON
    SPELL_LIGHTING_STRIKE_2                     = 138826,
    SPELL_FACE_PLAYER                           = 139359,
    SPELL_FORGEMASTER_SPAWN_COSMETIC            = 142594,
    SPELL_ELECTRIFIED                           = 138821,
    SPELL_DISCHARGE                             = 138820,
    SPELL_LIGHTING_BOLT                         = 15801,
    SPELL_LIGHTING_STRIKE_TARGETTING            = 139302,
    SPELL_THUNDER_SMASH_DUMMY                   = 139357,
    SPELL_LIGHTING_SMASH                        = 138831,
    SPELL_LIGHTING_SMASH_TRIGGER                = 138832,


    //< lr

    //< NPC_SHA_FIEND
    SPELL_SMALL_SHA_FIXATE                      = 138692,
    SPELL_DARK_SUNDER                           = 138677,
    SPELL_SHA_BLAST                             = 138681,

    //< NPC_CELESTIAL_BLACKSMITH
    SPELL_THUNDER_FORGE_CHARGING                = 140487,
    SPELL_FORGING                               = 138869,
    SPELL_STRIKE_ANVIL_COSMETIC                 = 138875,
    SPELL_THUNDER_FORGE_CHARGE_TRIGGER          = 140489,
    SPELL_ACTIVATE_CLOSEST_AVNIL                = 140343,

    //< NPC_CELESTIAL_DEFENDER
    SPELL_COSMIC_SLASH                          = 138232,
    SPELL_COSMIC_SLASH_TRIGGER                  = 138229,
    SPELL_STAR_SLAM                             = 138935,
    SPELL_CELESTIAL_ROAR                        = 138624,
    SPELL_ASTRAL_ENDURANCE                      = 139127,
    SPELL_DAMAGE_SELF_50_PERCENT                = 136890,
    SPELL_POWER_SURGE                           = 140067, //< AT
    SPELL_CELESTIAL_STORM                       = 138634,
    SPELL_CELESTIAL_STORM_TRIGGER               = 138637,
    SPELL_SUMMON_CONSTELATIONS                  = 138940,
    SPELL_SUMMON_CONSTELATIONS_TRIGGER          = 138723,
    SPELL_ASTRAL_DEFENCE                        = 138630,
    SPELL_DEFENDER_HIGH_HP_COSMETIC             = 140337,
    SPELL_CELESTIAL_RESTORATION                 = 140065,

    //< NPC_SHA_BEAST
    SPELL_EMPOWERED                             = 138947,
    SPELL_DARK_BITE                             = 138956,
    SPELL_ABSORB_EVIL                           = 138950, //< AURA
    SPELL_LETHARGY                              = 138949,

    //< NPC_ANVIL_STALKER
    SPELL_THUNDER_SURGE                         = 138834,
    SPELL_ANVIL_ACTIVATE_COSMETIC_DND           = 140134,
    SPELL_LIGHTING_STRIKE_COSMETIC              = 140101,

    //< NPC_PHASE3_ROOM_CENTER_STALKER
    SPELL_DEACTIVATE_ALL_AVNILS_TRIGGER         = 140545, //< SS
    SPELL_FIND_AVNIL_STALKER_BEST_DUMMY         = 140140, //< SS
    SPELL_DEACTIVATE_ALL_AVNILS                 = 140350,
    SPELL_ACTIVATE_ALL_AVNILS                   = 140027,
    SPELL_ELECTIC_DISCHARGE                     = 140047,
    SPELL_ELECTRIC_DISCHARGE_VISUAL             = 140061,

    //< NPC_SHA_AMALGAMATION
    SPELL_INSANITY_TRIGGER                      = 139381,
    SPELL_INSANITY                              = 139382, //< 30S COOLDOWN
    SPELL_SHADOW_BURST                          = 139375, //< 5S COOLDOWN
    SPELL_DARK_BINDING                          = 139358, //< 5S
    SPELL_METEOR_SOTRM_TRIGGER                  = 139602,
    SPELL_METEOR_STORM_PLAYER_TARGETING         = 139606,
    SPELL_METEOR_STORM                          = 139447, //< 5S COOLDOWN
    SPELL_METEOR_STORM_2                        = 139445,
    SPELL_SUMMON_LEECHING_SHA                   = 139990,
    SPELL_SHADOW_CRASH_TARGETING                = 139360,
    SPELL_SHADOW_CRASH                          = 139312,
    SPELL_SHADOW_CRASH_TRIGGER                  = 139351,
    SPELL_SHADOW_CRASH_CAST                     = 139353,

    //< NPC_DEBILITATING_SHA - 70462
    SPELL_DEBILITATE                            = 139966,

    //< NPC_NALAK
    SPELL_STATIC_SHIELD_TRIGGER                 = 136343,
    SPELL_STATIC_SHIELD                         = 136342,

    //< NPC_CONSTELLATION
    SPELL_ARCANE_OVERLOAD                       = 138715,
    SPELL_ARCANE_OVERLOAD_TRIGGER               = 138716,

    //< NPC_COSMETIC_SHA_BOSS
    SPELL_SHA_BOSS_STALKER_COSMETIC             = 139908,
    SPELL_SHA_BOSS_COSMETIC_SPAWN               = 139444,
    SPELL_SHA_BOSS_COSMETIC_MISSLE              = 139907,
};

enum Events
{
    EVENT_NONE,

    EVENT_CHECK_WIPE = 30,

    EVENT_WRATHION_MOVE_3,

    EVENT_JOIN_PARTY,
    EVENT_INTRO_PART_2,
    EVENT_HELPERS_MOVE,

    EVENT_HEALING_ORB,
    EVENT_STAGE_1_COMPLETED,

    EVENT_SUMMONS,
    EVENT_COMPLETE_FIRST_PART,

    EVENT_LIGHTING_BOLT,
    EVENT_CHANGE_WEAPON,
    EVENT_LIGHTING_SMASH,
    EVENT_FORGE_CAST,
    EVENT_FORGE_CAST_P2,
    EVENT_FORGE_CAST_P3,
    EVENT_THUNDER_SMASH,

    EVENT_METEOR,
    EVENT_FIREBALL,
    EVENT_CALL_LIGHTING,
    EVENT_LUMBERING_CHARGE,
    EVENT_SHADOW_BOLT,

    EVENT_LR_EMOTE_1,
    EVENT_LR_EMOTE_2,
    EVENT_LR_EMOTE_3,
    EVENT_LR_MOVE,
    EVENT_MOVE_TO_LR,
    EVENT_WIPE_CHECK_2,
    EVENT_LR_0,
    EVENT_LR_1,
    EVENT_LR_2,
    EVENT_LR_3,
    EVENT_LR_4,
    EVENT_LR_5,
    EVENT_LR_6,
    EVENT_LR_7,
    EVENT_LR_8,
    EVENT_LR_9,
    EVENT_LR_10,
    EVENT_LR_11,
    EVENT_LR_12,
    EVENT_LR_13,
    EVENT_LR_14,
    EVENT_LR_15,
    EVENT_LR_16,
    EVENT_LR_17,
    EVENT_LR_18,
    EVENT_LR_19,
    EVENT_LR_20,
    EVENT_LR_21,
    EVENT_LR_22,
    EVENT_LR_23,
    EVENT_LR_24,
    EVENT_LR_25,
    EVENT_LR_26,
    EVENT_LR_27,
    EVENT_LR_28,
    EVENT_LR_29,
    EVENT_LR_30,
    EVENT_LR_31,
    EVENT_LR_32,
    EVENT_LR_33,
    EVENT_LR_34,
    EVENT_LR_35,
    EVENT_LR_36,
    EVENT_LR_37,
    EVENT_LR_38,

    EVENT_NSANITY,
    EVENT_SHADOW_BURST,
    EVENT_DARK_BINDING,
    EVENT_METEOR_STORM,
    EVENT_SHADOW_CRASH,
    EVENT_SMALL_SHA_FIXATE,
    EVENT_DARK_SUNDER,
    EVENT_SHA_BLAST,
    EVENT_EMPOWERED,
    EVENT_DARK_BITE,
    EVENT_LETHARGY,
    EVENT_ABSORB_EVIL,
};

enum Phases
{
    PHASE_NONE,

    PHASE_ONE,
    PHASE_TWO,
};

enum Points
{
    POINT_NONE,

    POINT_NEW_HOME,
    POINT_EVADE_POS,
};

enum Sounds
{
    //< SMSG_PLAY_OBJECT_SOUND
    SOUND_0 = 36049,
    SOUND_1 = 36050,
    SOUND_2 = 36051,
    SOUND_3 = 36052,
    SOUND_4 = 36053,
    SOUND_5 = 36054,
    SOUND_6 = 36055,

    //< SMSG_PLAY_SOUND
    SOUND_7 = 36021,
    SOUND_8 = 36022,
    SOUND_9 = 36405,
    SOUND_10 = 36023,
    SOUND_11 = 36026,
    SOUND_12 = 36027,
    SOUND_13 = 36028,
    SOUND_14 = 36030,
    SOUND_15 = 36039,
    SOUND_16 = 36040,
    SOUND_17 = 36041,
    SOUND_18 = 36042,
    SOUND_19 = 36048,
    SOUND_20 = 36406,
    SOUND_21 = 36407,
    SOUND_22 = 36408,
    SOUND_23 = 36409,
    SOUND_24 = 36410,
    SOUND_25 = 36411
};

class npc_wrathion : public CreatureScript
{
public:
    npc_wrathion() : CreatureScript("npc_wrathion") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            player->ADD_GOSSIP_ITEM_DB(TEXT_WRATHION_START, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/)
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return false;

        if (Helper::IsNextStageAllowed(instance, STAGE_2))
        {
            creature->CastSpell(player, SPELL_SPEC_TEST);
            creature->AI()->DoAction(ACTION_WRATHION_START);
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        else
        {
            instance->SetData(DATA_LR_START, SPECIAL);
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->AI()->DoAction(ACTION_LR_P1);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        return false;
    }

    struct npc_wrathionAI : public ScriptedAI
    {
        npc_wrathionAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
            step = 0;
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_WRATHION_START:
                    events.ScheduleEvent(EVENT_1, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_1:
                    events.ScheduleEvent(EVENT_5, 7 * IN_MILLISECONDS);
                    break;
                case ACTION_2:
                    if (step == 0)
                    {
                        events.ScheduleEvent(EVENT_6, 2 * IN_MILLISECONDS);
                        step = 1;
                    }
                    break;


                case ACTION_LR_P1:
                    events.ScheduleEvent(EVENT_MOVE_TO_LR, 2 * IN_MILLISECONDS);
                    me->SummonCreature(NPC_CELESTIAL_BLACKSMITH, celestialBlacksmithPoints[0]);
                    me->SummonCreature(NPC_CELESTIAL_DEFENDER, celestialDefenderPoints[0]);
                    break;
                case ACTION_M_ENERGY:
                    Talk(13);
                    events.ScheduleEvent(EVENT_LR_37, 5 * IN_MILLISECONDS);
                    break;
                case ACTION_SCENARIO_COMPLETED:
                    events.ScheduleEvent(EVENT_LR_30, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void JustSummoned(Creature* sum)
        {
            summons.Summon(sum);

            switch (sum->GetEntry())
            {
                case NPC_THUNDER_FORGE:
                    sum->CastSpell(sum, SPELL_LIGHTING_STRIKE_COSMETIC_3);
                    sum->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
                    break;


                case NPC_CELESTIAL_BLACKSMITH:
                case NPC_CELESTIAL_DEFENDER:
                    sum->AI()->DoAction(ACTION_CB_START_MOVING);
                    break;
                default:
                    break;
            }
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case EVENT_1:
                    events.ScheduleEvent(EVENT_2, 3 * IN_MILLISECONDS);
                    break;
                case EVENT_2:
                    events.ScheduleEvent(EVENT_3, 2 * IN_MILLISECONDS);
                    Talk(1);
                    if (GameObject* go = GameObject::GetGameObject(*me, instance->GetData64(DATA_MOGU_CRUCIBLE)))
                        me->SetFacingToObject(go);
                    break;
                case EVENT_7:
                    me->GetMotionMaster()->MovePoint(EVENT_1 + NPC_THUNDER_FORGE, 7200.241f, 5255.292f, 65.98731f);
                    break;
                case EVENT_1 + NPC_THUNDER_FORGE:
                    events.ScheduleEvent(EVENT_2 + NPC_THUNDER_FORGE, 4 * IN_MILLISECONDS);
                    if (Creature* forge = Creature::GetCreature(*me, instance->GetData64(DATA_THUNDER_FORGE)))
                        me->CastSpell(forge, SPELL_LIGHTING_STRIKE_COSMETIC_5);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        me->GetMotionMaster()->MovePoint(EVENT_1, WrathionWP[1]);
                        Talk(0);
                        break;
                    case EVENT_2:
                        me->GetMotionMaster()->MovePoint(EVENT_2, WrathionWP[2]);
                        break;
                    case EVENT_3:
                        me->SummonCreature(NPC_LIGHTING_PILAR_BEAM_STALKER, 7195.71f, 5249.874f, 67.64626f);
                        me->SummonCreature(NPC_INVISIBLE_STALKER, 7202.491f, 5242.387f, 66.06776f);
                        me->SummonCreature(NPC_THUNDER_FORGE, 7195.93f, 5249.743f, 85.89191f);
                        me->SummonCreature(NPC_LIGHTING_PILAR_SPARK_STALKER, 7196.897f, 5252.677f, 66.06777f);

                        if (GameObject* go = GameObject::GetGameObject(*me, instance->GetData64(DATA_MOGU_CRUCIBLE)))
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case EVENT_4:
                        me->SetOrientation(0.7853982f);
                        break;
                    case EVENT_5:
                        events.ScheduleEvent(EVENT_8, 3 * IN_MILLISECONDS);
                        Talk(26);
                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_1)))
                        {
                            warrior->GetMotionMaster()->MoveJump(7210.65f, 5247.51f, 65.9844f, 15.0f, 25.0f);
                            
                            Map::PlayerList const& players = me->GetMap()->GetPlayers();
				            if (!players.isEmpty())
				            {
					            if (Player* plr = players.begin()->getSource())
						            plr->GetGroup()->AddCreatureMember(warrior);
				            }
                        }

                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_2)))
                        {
                            warrior->GetMotionMaster()->MoveJump(7195.13f, 5266.81f, 65.9844f, 15.0f, 25.0f);
                            Map::PlayerList const& players = me->GetMap()->GetPlayers();
				            if (!players.isEmpty())
				            {
					            if (Player* plr = players.begin()->getSource())
						            plr->GetGroup()->AddCreatureMember(warrior);
				            }
                        }

                        if (Creature* defender = Creature::GetCreature(*me, instance->GetData64(DATA_DEFENDER)))
                        {
                            defender->GetMotionMaster()->MoveJump(7213.67f, 5266.37f, 65.9844f, 15.0f, 25.0f);
                            Map::PlayerList const& players = me->GetMap()->GetPlayers();
				            if (!players.isEmpty())
				            {
					            if (Player* plr = players.begin()->getSource())
						            plr->GetGroup()->AddCreatureMember(defender);
				            }
                        }

                        break;
                    case EVENT_6:
                        events.ScheduleEvent(EVENT_7, 6 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_9, 200 * IN_MILLISECONDS);
                        me->GetMotionMaster()->MovePoint(EVENT_6, 7195.7f, 5255.59f, 66.05624f);
                        Talk(2);
                        if (Creature* stalker = me->FindNearestCreature(NPC_INVISIBLE_STALKER, 150.0f))
                            stalker->AI()->DoAction(ACTION_1);
                        break;
                    case EVENT_8:
                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_1)))
                            warrior->GetMotionMaster()->MovePoint(EVENT_8, 7223.458f, 5262.069f, 65.98731f);

                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_2)))
                            warrior->GetMotionMaster()->MovePoint(EVENT_8, 7209.367f, 5279.108f, 66.05622f); //< or 7208.249 Y: 5278.209 Z: 66.27031 ?

                        if (Creature* defender = Creature::GetCreature(*me, instance->GetData64(DATA_DEFENDER)))
                            defender->GetMotionMaster()->MovePoint(EVENT_8, 7221.26f, 5276.239f, 66.05622f);
                        break;
                    case EVENT_7:
                        me->GetMotionMaster()->MovePoint(EVENT_7, 7199.577f, 5254.172f, 66.27177f);
                        Talk(3);
                        break;
                    case EVENT_2 + NPC_THUNDER_FORGE:
                        events.ScheduleEvent(EVENT_2 + NPC_THUNDER_FORGE, 10 * IN_MILLISECONDS);
                        if (Creature* forge = Creature::GetCreature(*me, instance->GetData64(DATA_THUNDER_FORGE)))
                        {
                            me->CastSpell(forge, SPELL_LIGHTING_STRIKE_COSMETIC_5);
                            forge->CastSpell(forge, SPELL_LIGHTING_STRIKE_COSMETIC_4);
                        }
                        break;
                    case EVENT_9:
                        if (Creature* stalker = me->FindNearestCreature(NPC_INVISIBLE_STALKER, 150.0f))
                            stalker->AI()->DoAction(ACTION_2);
                        break;





                    case EVENT_LR_30:
                        events.ScheduleEvent(EVENT_LR_31, 3 * IN_MILLISECONDS);
                        Talk(24);
                        break;
                    case EVENT_LR_31:
                        events.ScheduleEvent(EVENT_LR_32, 3 * IN_MILLISECONDS);
                        Talk(25);
                        break;
                    case EVENT_LR_32:
                        //< disable this - should be called just after quest accepting
                        if (Player* plr = me->FindNearestPlayer(100.0f))
                            me->DestroyForPlayer(plr);
                        break;
                    case EVENT_LR_37:
                        Talk(15);
                        me->SummonCreature(NPC_SHA_AMALGAMATION, 7348.246f, 5179.011f, 49.38733f, 2.254864f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_38, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_38:
                        Talk(16);
                        break;
                    case EVENT_LR_0:
                        events.CancelEvent(EVENT_FORGE_CAST);
                        events.ScheduleEvent(EVENT_LR_EMOTE_1, 9 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_WIPE_CHECK_2, 3 * IN_MILLISECONDS);
                        Talk(5);
                        break;
                    case EVENT_LR_EMOTE_1:
                        events.ScheduleEvent(EVENT_LR_EMOTE_2, 9 * IN_MILLISECONDS);
                        Talk(6);
                        break;
                    case EVENT_WIPE_CHECK_2:
                        if (instance->IsWipe() || instance->GetData(DATA_LR_STAGE_2) == FAIL)
                            events.RescheduleEvent(EVENT_LR_EMOTE_2, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_EMOTE_2:
                        events.ScheduleEvent(EVENT_LR_EMOTE_3, 9 * IN_MILLISECONDS);
                        Talk(7);
                        break;
                    case EVENT_LR_EMOTE_3:
                        me->SetDynamicWorldEffects(505, 1);
                        Talk(8);
                        me->SetFlag64(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    case EVENT_LR_MOVE:
                        me->RemoveAurasDueToSpell(SPELL_WRATHION_TRUE_FORM);
                        events.ScheduleEvent(EVENT_LR_3, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_3:
                    {
                        Talk(9);
                        events.ScheduleEvent(EVENT_LR_4, 8 * IN_MILLISECONDS);
                        break;
                    }
                    case EVENT_LR_4:
                        Talk(10);
                        events.ScheduleEvent(EVENT_LR_5, 8 * IN_MILLISECONDS);
                        doAction(me, NPC_CELESTIAL_BLACKSMITH, ACTION_FIRST_WAVE);
                        break;
                    case EVENT_LR_5:
                        Talk(11);
                        events.ScheduleEvent(EVENT_LR_6, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_6:
                        events.ScheduleEvent(EVENT_LR_7, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_7:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_8, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_8:
                        events.ScheduleEvent(EVENT_LR_9, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_9:
                        events.ScheduleEvent(EVENT_LR_9, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_11:
                        events.ScheduleEvent(EVENT_LR_12, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_12:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_13, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_13:
                        events.ScheduleEvent(EVENT_LR_14, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_14:
                        events.ScheduleEvent(EVENT_LR_15, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_15:
                        events.ScheduleEvent(EVENT_LR_16, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_16:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_17, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_17:
                        events.ScheduleEvent(EVENT_LR_18, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_18:
                        events.ScheduleEvent(EVENT_LR_19, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_19:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_3);
                        events.ScheduleEvent(EVENT_LR_20, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_20:
                        events.ScheduleEvent(EVENT_LR_21, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_21:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_COSMETIC);
                        events.ScheduleEvent(EVENT_LR_22, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_22:
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE, 200.0f))
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_COSMETIC);
                        break;
                    case EVENT_STAGE_1_COMPLETED:
                    {
                        instance->HandleGameObject(instance->GetData64(GO_THUNDER_FORGE_DOOR), true);
                        doAction(me, NPC_THUNDER_FORGE2, ACTION_CANCEL_FORGE_EVENTS);
                        instance->SetData(DATA_LR_START, IN_PROGRESS);
                        Talk(4);

                        me->AddAura(SPELL_WRATHION_TRUE_FORM, me);
                        me->CastSpell(me, SPELL_WRATHION_TRUE_FORM);
                        events.ScheduleEvent(EVENT_LR_MOVE, 40 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_0, 40 * IN_MILLISECONDS);
                        break;
                    }
                    case EVENT_CHECK_WIPE:
                        if (instance->IsWipe())
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    case EVENT_INTRO_PART_2:
                        

                        events.ScheduleEvent(EVENT_CHECK_WIPE, 2 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_FORGE_CAST, 15 * IN_MILLISECONDS);

                        Talk(3);
                        break;
                    case EVENT_FORGE_CAST:
                        //events.ScheduleEvent(EVENT_FORGE_CAST, 30 * IN_MILLISECONDS);
                        doAction(me, NPC_THUNDER_FORGE2, ACTION_FORGE_CAST);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        InstanceScript* instance;
        EventMap events;
        SummonList summons;
        Position pos;
        uint8 step;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wrathionAI(creature);
    }
};

class go_mogu_crucible : public GameObjectScript
{
public:
    go_mogu_crucible() : GameObjectScript("go_mogu_crucible") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Helper::IsNextStageAllowed(instance, STAGE_3))
            {
                go->SendGameObjectActivateAnimKit(3809, true);
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                return true;
            }
        }
        return false;
    }
};

class npc_shado_pan_defender : public CreatureScript
{
public:
    npc_shado_pan_defender() : CreatureScript("npc_shado_pan_defender") { }

    struct npc_shado_pan_defenderAI : public ScriptedAI
    {
        npc_shado_pan_defenderAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case EVENT_8:
                    instance->SetData(DATA_WAVE_COUNTER, 1);
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    break;
                default:
                    break;
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->CallAssistance();
        }

        void EnterEvadeMode()
        {
            Talk(1);

            me->DeleteThreatList();
            me->CombatStop(true);
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());
            events.ScheduleEvent(EVENT_HEALING_ORB, urand(5, 15) * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_COMPLETE_FIRST_PART:
                        //instance->HandleGameObject(instance->GetData64(DATA_TRUNDER_FORGE_DOOR), true);
                        //Talk(2);
                        //DoCast(SPELL_LEAVE_PLAYER_PARTY);
                        //me->GetMotionMaster()->MoveJump(helpersLastJumpPos[2].m_positionX, helpersLastJumpPos[2].m_positionY, helpersLastJumpPos[2].m_positionZ,
                        //    20.0f, 20.0f);

                        //me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                        break;
                    case EVENT_HEALING_ORB:
                        DoCast(SPELL_HEALING_ORB);
                        Talk(0);
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
        return new npc_shado_pan_defenderAI(creature);
    }
};

class npc_shado_pan_warrior : public CreatureScript
{
public:
    npc_shado_pan_warrior() : CreatureScript("npc_shado_pan_warrior") { }

    struct npc_shado_pan_warriorAI : public ScriptedAI
    {
        npc_shado_pan_warriorAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case EVENT_8:
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    break;
                default:
                    break;
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->CallAssistance();
        }

        void EnterEvadeMode()
        {
            me->DeleteThreatList();
            me->CombatStop(true);
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
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
        return new npc_shado_pan_warriorAI(creature);
    }
};

class npc_thunder_forge_second : public CreatureScript
{
public:
    npc_thunder_forge_second() : CreatureScript("npc_thunder_forge_second") { }

    struct npc_thunder_forge_secondAI : public Scripted_NoMovementAI
    {
        npc_thunder_forge_secondAI(Creature* creature) : Scripted_NoMovementAI(creature)
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
                case ACTION_FORGE_CAST:
                    events.ScheduleEvent(EVENT_FORGE_CAST, 3 * IN_MILLISECONDS);
                    break;
                case ACTION_CANCEL_FORGE_EVENTS:
                    events.CancelEvent(EVENT_FORGE_CAST);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    //case EVENT_FORGE_CAST:
                    //    events.ScheduleEvent(EVENT_FORGE_CAST, 30 * IN_MILLISECONDS);
                    //    me->CastSpell(me, SPELL_THUNDER_FORGE_BUFF_PEREODIC);
                    //    events.ScheduleEvent(EVENT_FORGE_CAST_P3, 20 * IN_MILLISECONDS);
                    //    break;
                    //case EVENT_FORGE_CAST_P3:
                    //    me->CastSpell(me, SPELL_THUNDER_FORGE_BUFF);
                    //    Talk(0);
                    //    break;
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
        return new npc_thunder_forge_secondAI(creature);
    }
};

class npc_invisible_hunter : public CreatureScript
{
public:
    npc_invisible_hunter() : CreatureScript("npc_invisible_hunter") { }

    struct npc_invisible_hunterAI : public Scripted_NoMovementAI
    {
        npc_invisible_hunterAI(Creature* creature) : Scripted_NoMovementAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            instance = creature->GetInstanceScript();
        }

        void Reset()
        { }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    if (Player* player = me->FindNearestPlayer(300.0f))
                    {
                        me->AddAura(SPELL_THUNDER_FORGE_CHARGE, player);
                        me->AddAura(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1, player);
                    }
                    break;
                case ACTION_2:
                    instance->SetData(DATA_STAGE1_P2, IN_PROGRESS);
                    if (Player* player = me->FindNearestPlayer(300.0f))
                    {
                        me->RemoveAura(SPELL_THUNDER_FORGE_CHARGE);
                        me->RemoveAura(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1);
                    }
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff) { }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_invisible_hunterAI(creature);
    }
};

class npc_lighting_pilar_beam_stalker : public CreatureScript
{
public:
    npc_lighting_pilar_beam_stalker() : CreatureScript("npc_lighting_pilar_beam_stalker") { }

    struct npc_lighting_pilar_beam_stalkerAI : public CreatureAI
    {
        npc_lighting_pilar_beam_stalkerAI(Creature* creature) : CreatureAI(creature)
        {
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->GetMotionMaster()->Clear();
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
                case NPC_FORGEMASTER_VULKON:
                    summon->CastSpell(summon, SPELL_FORGEMASTER_SPAWN_COSMETIC);
                    break;
                default:
                    break;
            }
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CHARGING_1:
                    events.ScheduleEvent(EVENT_FORGE_CAST_P2, 1 * IN_MILLISECONDS);
                    me->NearTeleportTo(cosmeticPilarPos[1][0], cosmeticPilarPos[1][1], cosmeticPilarPos[1][2], 0.0f);
                    break;
                case ACTION_CHARGING_2:
                    me->NearTeleportTo(cosmeticPilarPos[2][0], cosmeticPilarPos[2][1], cosmeticPilarPos[2][2], 0.0f);
                    break;
                case ACTION_CHARGING_3:
                    me->NearTeleportTo(cosmeticPilarPos[3][0], cosmeticPilarPos[3][1], cosmeticPilarPos[3][2], 0.0f);
                    break;
                case ACTION_CHARGING_4:
                    me->SummonCreature(NPC_FORGEMASTER_VULKON, 7207.826f, 5262.409f, 66.06776f, 6.170584f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FORGE_CAST_P2:
                        events.ScheduleEvent(EVENT_FORGE_CAST_P2, 2 * IN_MILLISECONDS);
                        if (Unit* target = me->FindNearestCreature(NPC_THUNDER_FORGE_3, 200.0f))
                        {
                            me->AddAura(SPELL_LIGHTING_STRIKE_COSMETIC_2, target);
                            me->CastSpell(target, SPELL_LIGHTING_STRIKE_COSMETIC_2);
                        }
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
        return new npc_lighting_pilar_beam_stalkerAI(creature);
    }
};

class npc_lighting_pilar_spark_stalker : public CreatureScript
{
public:
    npc_lighting_pilar_spark_stalker() : CreatureScript("npc_lighting_pilar_spark_stalker") { }

    struct npc_lighting_pilar_spark_stalkerAI : public CreatureAI
    {
        npc_lighting_pilar_spark_stalkerAI(Creature* creature) : CreatureAI(creature)
        {
            me->SetDisableGravity(true);
            me->SetCanFly(true);
            instance = creature->GetInstanceScript();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CHARGING_1:
                    me->AddAura(SPELL_LIGHTING_PILAR_SPARK_COSMETIC, me);
                    break;
                case ACTION_CHARGING_2:
                    me->AddAura(SPELL_LIGHTING_PILAR_SPARK_COSMETIC_3, me);
                    break;
                case ACTION_CHARGING_3:
                    me->AddAura(SPELL_LIGHTING_PILAR_SPARK_COSMETIC_2, me);
                    break;
                default:
                    break;
            }
        }

        void Reset() { }

        void UpdateAI(uint32 /*diff*/) { }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lighting_pilar_spark_stalkerAI(creature);
    }
};

class npc_forgemaster_vulkon : public CreatureScript
{
public:
    npc_forgemaster_vulkon() : CreatureScript("npc_forgemaster_vulkon") { }

    struct npc_forgemaster_vulkonAI : public ScriptedAI
    {
        npc_forgemaster_vulkonAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_1, 3 * IN_MILLISECONDS, 0, PHASE_ONE);
                    me->CastSpell(me, SPELL_FORGEMASTER_SPAWN_COSMETIC);
                    break;
                case ACTION_2:
                    events.ScheduleEvent(EVENT_6, 3 * IN_MILLISECONDS, 0, PHASE_TWO);
                    SetEquipmentSlots(false, 82347, 0);
                    Talk(1);
                    me->AddAura(SPELL_DISCHARGE, me);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();
            phase = PHASE_ONE;
            events.SetPhase(PHASE_ONE);

            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            SetEquipmentSlots(true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            DoZoneInCombat(me);
        }
        
        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
        {
            if (HealthBelowPct(30) && phase != PHASE_TWO)
            {
                phase = PHASE_TWO;
                events.SetPhase(PHASE_TWO);
                DoAction(ACTION_2);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_LIGHTING_BOLT, 25 * IN_MILLISECONDS, 0, PHASE_ONE);

                        if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST))
                            me->AI()->AttackStart(target);

                        SetEquipmentSlots(false, 94565, 94565);
                        me->AddAura(SPELL_DISCHARGE, me);
                        break;
                    case EVENT_LIGHTING_BOLT:
                        events.ScheduleEvent(EVENT_2, 2 * IN_MILLISECONDS, 0, PHASE_ONE);
                        Talk(0);
                        SetEquipmentSlots(false, 89148, 0);
                        me->RemoveAura(SPELL_DISCHARGE);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_3, 5 * IN_MILLISECONDS, 0, PHASE_ONE);
                        DoCast(SPELL_LIGHTING_BOLT);
                        break;
                    case EVENT_3:
                        events.ScheduleEvent(EVENT_3, 15 * IN_MILLISECONDS, 0, PHASE_ONE);
                        events.ScheduleEvent(EVENT_4, 1 * IN_MILLISECONDS, 0, PHASE_ONE);
                        DoCast(SPELL_LIGHTING_STRIKE_TARGETTING);
                        break;
                    case EVENT_4:
                        DoCast(SPELL_LIGHTING_STRIKE_2);
                        break;
                    case EVENT_6:
                        events.ScheduleEvent(EVENT_6, 15 * IN_MILLISECONDS, 0, PHASE_ONE);
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 10.0f, true))
                        {
                            me->CastSpell(target, SPELL_FACE_PLAYER);
                            me->CastSpell(target, SPELL_LIGHTING_SMASH);
                        }
                        break;
                    default:
                        break;
                }
            }

            if (phase == PHASE_ONE)
                DoSpellAttackIfReady(SPELL_LIGHTING_BOLT);
            else
                DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

    private:
        InstanceScript* instance;
        EventMap events;
        Phases phase;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_forgemaster_vulkonAI(creature);
    }
};

class npc_shanze_shadowcaster : public CreatureScript
{
public:
    npc_shanze_shadowcaster() : CreatureScript("npc_shanze_shadowcaster") { }

    struct npc_shanze_shadowcasterAI : public ScriptedAI
    {
        npc_shanze_shadowcasterAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);

            events.ScheduleEvent(EVENT_SHADOW_BOLT, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADOW_BOLT:
                        events.ScheduleEvent(EVENT_SHADOW_BOLT, 10 * IN_MILLISECONDS);
                        DoCast(SPELL_SHADOW_BOLT);
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
        return new npc_shanze_shadowcasterAI(creature);
    }
};

class npc_shanze_battlemaster : public CreatureScript
{
public:
    npc_shanze_battlemaster() : CreatureScript("npc_shanze_battlemaster") { }

    struct npc_shanze_battlemasterAI : public ScriptedAI
    {
        npc_shanze_battlemasterAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(1);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);
            Talk(0);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            /*while (uint32 eventId = events.ExecuteEvent())
            {
            switch (eventId)
            {
            }
            }*/

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_shanze_battlemasterAI(creature);
    }
};

class npc_shanze_warrior : public CreatureScript
{
public:
    npc_shanze_warrior() : CreatureScript("npc_shanze_warrior") { }

    struct npc_shanze_warriorAI : public ScriptedAI
    {
        npc_shanze_warriorAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);
            Talk(0);

            events.ScheduleEvent(EVENT_LUMBERING_CHARGE, 3 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LUMBERING_CHARGE:
                        events.ScheduleEvent(EVENT_LUMBERING_CHARGE, 10 * IN_MILLISECONDS);
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 10.0f))
                            me->CastSpell(target, SPELL_LUMBERING_CHARGE);
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
        return new npc_shanze_warriorAI(creature);
    }
};

class npc_shanze_electro_coutioner : public CreatureScript
{
public:
    npc_shanze_electro_coutioner() : CreatureScript("npc_shanze_electro_coutioner") { }

    struct npc_shanze_electro_coutionerAI : public ScriptedAI
    {
        npc_shanze_electro_coutionerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);
            Talk(0);

            events.ScheduleEvent(EVENT_LIGHTING_BOLT, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CALL_LIGHTING, 23 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LIGHTING_BOLT:
                        events.ScheduleEvent(EVENT_LIGHTING_BOLT, 10 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_LIGHTING_BOLT);
                        break;
                    case EVENT_CALL_LIGHTING:
                        DoCastVictim(SPELL_CALL_LIGHTING);
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
        return new npc_shanze_electro_coutionerAI(creature);
    }
};

class npc_shanze_pyromancer : public CreatureScript
{
public:
    npc_shanze_pyromancer() : CreatureScript("npc_shanze_pyromancer") { }

    struct npc_shanze_pyromancerAI : public ScriptedAI
    {
        npc_shanze_pyromancerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat(me);

            events.ScheduleEvent(EVENT_METEOR, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FIREBALL, 15 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_METEOR:
                        events.ScheduleEvent(EVENT_METEOR, 19 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_METEOR_TARGETING);
                        DoCastVictim(SPELL_METEOR_AT, true);
                        break;
                    case EVENT_FIREBALL:
                        events.ScheduleEvent(EVENT_FIREBALL, 10 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_FIREBALL);
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
        return new npc_shanze_pyromancerAI(creature);
    }
};

//< second room

class npc_celestial_blacksmith : public CreatureScript
{
public:
    npc_celestial_blacksmith() : CreatureScript("npc_celestial_blacksmith") { }

    struct npc_celestial_blacksmithAI : public CreatureAI
    {
        npc_celestial_blacksmithAI(Creature* creature) : CreatureAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->setRegeneratingHealth(false);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CB_START_MOVING:
                    events.ScheduleEvent(EVENT_LR_0, 2 * IN_MILLISECONDS);

                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 45123);
                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 94564);
                    me->CastSpell(me, SPELL_JOIN_PLAYER_PARTY);
                    instance->SetData(DATA_LR_STAGE_2, IN_PROGRESS);
                    DoCast(SPELL_DEACTIVATE_ALL_AVNILS);

                    if (Player* plr = me->FindNearestPlayer(200.0f))
                        plr->AddAura(SPELL_THUNDER_FORGE_CHARGING, plr);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();
            summons.DespawnAll();

            talk = false;
            dead = false;
            power = 0;
        }

        void DamageTaken(Unit* /*attacker*/, uint32 &damage)
        {
            if ((me->HealthBelowPctDamaged(25, damage) || (me->HealthBelowPctDamaged(50, damage))) && !dead)
            {
                dead = true;
                Talk(2);
            }
        }

        void EnterCombat(Unit* /*who*/)
        { }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LR_0:
                        t = 0;
                        events.ScheduleEvent(EVENT_LR_1, t += 0 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_2, t += 40 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_3, t += 32 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_4, t += 35 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_5, t += 35 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_6, t += 36 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LR_7, t += 34 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_1:
                        t += 5;
                        AvnilHelper(0);
                        break;
                    case EVENT_LR_2:
                        AvnilHelper(1);
                        break;
                    case EVENT_LR_3:
                        AvnilHelper(2);
                        break;
                    case EVENT_LR_4:
                        AvnilHelper(3);
                        break;
                    case EVENT_LR_5:
                        AvnilHelper(4);
                        break;
                    case EVENT_LR_6:
                        AvnilHelper(5);
                        break;
                    case EVENT_LR_7:
                        AvnilHelper(6);
                        if (power < 90)
                            events.ScheduleEvent(EVENT_LR_0, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_8:
                        if (Unit* stalker = me->FindNearestCreature(NPC_ANVIL_STALKER, 15.0f))
                            me->SetFacingTo(stalker);
                        me->AddAura(SPELL_FORGING, me);
                        break;
                    case EVENT_LR_9:
                    {
                        if (Unit* stalker = me->FindNearestCreature(NPC_ANVIL_STALKER, 15.0f))
                        {
                            me->AddAura(SPELL_ANVIL_ACTIVATE_COSMETIC_DND, stalker);
                            me->AddAura(SPELL_ACTIVATE_CLOSEST_AVNIL, stalker);
                        }

                        if (!talk)
                        {
                            talk = true;
                            Talk(0);
                        }
                        else
                            Talk(1);

                        if (Unit* plr = me->FindNearestPlayer(150.0f))
                            for (uint32 i = 0; i < 5; i++)
                            {
                                plr->CastSpell(plr, SPELL_THUNDER_FORGE_CHARGE_TRIGGER);
                                ++power;
                            }

                        switch (power)
                        {
                            case 5:
                            case 15:
                            case 25:
                            case 35:
                            case 45:
                                break;
                            case 10:
                            case 20:
                            case 30:
                            case 40:
                                break;
                            case 50:
                                doAction(me, NPC_WRATHION, ACTION_M_ENERGY);
                                break;
                            case 90:
                                if (Unit* stalker = me->SummonCreature(NPC_LIGHTING_SPEAR_FLOAT_STALKER, 7368.375f, 5181.912f, 52.79837f))
                                    if (Unit* lance = me->SummonCreature(NPC_LIGHTING_LANCE, 7368.375f, 5181.912f, 55.04837f))
                                        stalker->CastSpell(lance, VEHICLE_SPELL_RIDE_HARDCODED);
                                break;
                            case 100:
                                if (Player* plr = me->FindNearestPlayer(200.0f))
                                    plr->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGING);
                                events.CancelEvent(EVENT_LR_0);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->SetData(DATA_LR_STAGE_2, FAIL);
            summons.DespawnAll();
        }

        void AvnilHelper(uint8 ID)
        {
            Movement::MoveSplineInit init(*me);
            init.MoveTo(cBlacksmithPositions[ID].GetPositionX(), cBlacksmithPositions[ID].GetPositionY(), cBlacksmithPositions[ID].GetPositionZ(), true);
            init.SetSmooth();
            init.Launch();

            events.ScheduleEvent(EVENT_LR_8, 8 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_LR_9, 35 * IN_MILLISECONDS);
        }

    private:
        InstanceScript* instance;
        EventMap events;
        uint32 t;
        bool talk;
        bool dead;
        uint32 power;
        SummonList summons;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_celestial_blacksmithAI(creature);
    }
};

class npc_celestial_defender : public CreatureScript
{
public:
    npc_celestial_defender() : CreatureScript("npc_celestial_defender") { }

    struct npc_celestial_defenderAI : public CreatureAI
    {
        npc_celestial_defenderAI(Creature* creature) : CreatureAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->setRegeneratingHealth(false);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_CB_START_MOVING:
                    events.ScheduleEvent(EVENT_LR_1, 1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_LR_2, 3 * IN_MILLISECONDS);
                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 94156);
                    me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 94156);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.CancelEvent(EVENT_LR_3);
            events.ScheduleEvent(EVENT_LR_5, 15 * IN_MILLISECONDS);
        }

        void EnterEvadeMode()
        {
            events.ScheduleEvent(EVENT_LR_3, 10 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LR_1:
                        me->AddAura(SPELL_ASTRAL_ENDURANCE, me);
                        me->CastSpell(me, SPELL_JOIN_PLAYER_PARTY);

                        if (Player* plr = me->FindNearestPlayer(200.0f))
                            if (plr->GetRoleForGroup(plr->GetActiveSpec()) == ROLES_HEALER)
                                me->CastSpell(me, SPELL_DAMAGE_SELF_50_PERCENT);
                        break;
                    case EVENT_LR_2:
                        me->GetMotionMaster()->MovePoint(1, 7390.238770f, 5182.048340f, 49.603447f);
                        events.ScheduleEvent(EVENT_LR_3, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_3:
                        events.ScheduleEvent(EVENT_LR_3, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_LR_5:
                        events.ScheduleEvent(EVENT_LR_5, 40 * IN_MILLISECONDS);
                        me->AddAura(SPELL_POWER_SURGE, me);
                        Talk(0);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(2);
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_celestial_defenderAI(creature);
    }
};

class npc_sha_beast : public CreatureScript
{
public:
    npc_sha_beast() : CreatureScript("npc_sha_beast") { }

    struct npc_sha_beastAI : public ScriptedAI
    {
        npc_sha_beastAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_EMPOWERED, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DARK_BITE, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_LETHARGY, 25 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_ABSORB_EVIL, 40 * IN_MILLISECONDS);
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
                    case EVENT_EMPOWERED:
                        events.ScheduleEvent(EVENT_EMPOWERED, 19 * IN_MILLISECONDS);
                        DoCast(SPELL_EMPOWERED);
                        break;
                    case EVENT_DARK_BITE:
                        events.ScheduleEvent(EVENT_DARK_BITE, 25 * IN_MILLISECONDS);
                        DoCast(SPELL_DARK_BITE);
                        break;
                    case EVENT_LETHARGY:
                        events.ScheduleEvent(EVENT_LETHARGY, 40 * IN_MILLISECONDS);
                        DoCast(SPELL_LETHARGY);
                        break;
                    case EVENT_ABSORB_EVIL:
                        events.ScheduleEvent(EVENT_ABSORB_EVIL, 45 * IN_MILLISECONDS);
                        DoCast(SPELL_ABSORB_EVIL);
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
        return new npc_sha_beastAI(creature);
    }
};

class npc_sha_fiend : public CreatureScript
{
public:
    npc_sha_fiend() : CreatureScript("npc_sha_fiend") { }

    struct npc_sha_fiendAI : public ScriptedAI
    {
        npc_sha_fiendAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        { }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_DARK_SUNDER, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHA_BLAST, 15 * IN_MILLISECONDS);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_SHA_FIXATE:
                    events.ScheduleEvent(EVENT_SMALL_SHA_FIXATE, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SMALL_SHA_FIXATE:
                    {
                        Unit* target = me->FindNearestCreature(NPC_CELESTIAL_BLACKSMITH, 250.0f);
                        if (!target)
                            return;

                        if (me->GetDistance2d(target) > 8.0f)
                        {
                            me->GetMotionMaster()->MovePoint(1, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                            me->AddAura(SPELL_SMALL_SHA_FIXATE, target);
                        }
                        else
                        {
                            me->SetReactState(REACT_AGGRESSIVE);
                            AttackStart(SelectTarget(SELECT_TARGET_RANDOM));
                            me->RemoveAura(SPELL_SMALL_SHA_FIXATE);
                        }
                        break;
                    }
                    case EVENT_DARK_SUNDER:
                        events.ScheduleEvent(EVENT_DARK_SUNDER, 15 * IN_MILLISECONDS);
                        if (Unit* target = me->FindNearestCreature(NPC_CELESTIAL_BLACKSMITH, 150.0f))
                            me->CastSpell(target, SPELL_DARK_SUNDER);
                        break;
                    case EVENT_SHA_BLAST:
                        events.ScheduleEvent(EVENT_SHA_BLAST, 10 * IN_MILLISECONDS);
                        DoCast(SPELL_SHA_BLAST);
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
        return new npc_sha_fiendAI(creature);
    }
};

class npc_sha_amalgamation : public CreatureScript
{
public:
    npc_sha_amalgamation() : CreatureScript("npc_sha_amalgamation") { }

    struct npc_sha_amalgamationAI : public ScriptedAI
    {
        npc_sha_amalgamationAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        {
            doAction(me, NPC_WRATHION, ACTION_SCENARIO_COMPLETED);
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_NSANITY, 1 * MINUTE * IN_MILLISECONDS + 30 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHADOW_BURST, urand(40, 50) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DARK_BINDING, urand(15, 30) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_METEOR_STORM, 6 * MINUTE);
            events.ScheduleEvent(EVENT_SHADOW_CRASH, 2 * MINUTE);
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
                    case EVENT_NSANITY:
                        events.ScheduleEvent(EVENT_NSANITY, 2 * MINUTE);
                        DoCast(SPELL_INSANITY);
                        Talk(0);
                        break;
                    case EVENT_SHADOW_BURST:
                        DoCast(SPELL_SHADOW_BURST);
                        events.ScheduleEvent(EVENT_SHADOW_BURST, urand(40, 50) * IN_MILLISECONDS);
                        break;
                    case EVENT_DARK_BINDING:
                        events.ScheduleEvent(EVENT_DARK_BINDING, urand(30, 60) * IN_MILLISECONDS);
                        if (Unit* defender = me->FindNearestCreature(NPC_CELESTIAL_DEFENDER, 150.0f))
                        {
                            me->CastSpell(defender, SPELL_DARK_BINDING);
                            Talk(1);
                        }
                        break;
                    case EVENT_METEOR_STORM:
                        me->AddAura(SPELL_METEOR_STORM, me);
                        break;
                    case EVENT_SHADOW_CRASH:
                        events.ScheduleEvent(EVENT_SHADOW_CRASH, 1 * MINUTE * IN_MILLISECONDS + 30 * IN_MILLISECONDS);
                        DoCast(SPELL_SHADOW_CRASH);
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
        return new npc_sha_amalgamationAI(creature);
    }
};

class spell_phase_shift_update : public SpellScriptLoader
{
public:
    spell_phase_shift_update() : SpellScriptLoader("spell_phase_shift_update") { }

    class spell_phase_shift_update_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_phase_shift_update_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            return;
            /* dead code
            targets.clear();

            for (int8 i = 0; i < 32; i++)
            targets.remove_if(TargetFilter(phasingTargets[i]));
            */
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_phase_shift_update_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }

        class TargetFilter
        {
        public:
            TargetFilter(uint32 entry) { ID = entry; }
            bool operator()(WorldObject* obj) const
            {
                if (!obj->ToCreature())
                    return true;

                return (obj->ToCreature()->GetEntry() != ID);
            }

        private:
            uint32 ID;
        };
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_phase_shift_update_SpellScript();
    }
};

class spell_thundder_forge_charging : public SpellScriptLoader
{
public:
    spell_thundder_forge_charging() : SpellScriptLoader("spell_thundder_forge_charging") { }

    class spell_thundder_forge_charging_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_thundder_forge_charging_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            if (InstanceScript* instance = target->GetInstanceScript())
            {
                Creature* beam = target->FindNearestCreature(NPC_LIGHTING_PILAR_BEAM_STALKER, 200.0f);
                if (!beam)
                    return;

                Creature* spark = target->FindNearestCreature(NPC_LIGHTING_PILAR_SPARK_STALKER, 200.0f);
                if (!spark)
                    return;

                switch (target->GetPower(POWER_ALTERNATE_POWER))
                {
                    case 50:
                        beam->AI()->DoAction(ACTION_CHARGING_1);
                        spark->AI()->DoAction(ACTION_CHARGING_1);
                        break;
                    case 100:
                        beam->AI()->DoAction(ACTION_CHARGING_2);
                        spark->AI()->DoAction(ACTION_CHARGING_2);
                        break;
                    case 150:
                        beam->AI()->DoAction(ACTION_CHARGING_3);
                        spark->AI()->DoAction(ACTION_CHARGING_3);
                        break;
                    case 200:
                    {
                        beam->AI()->DoAction(ACTION_CHARGING_4);
                        for (auto const& itr : target->GetMap()->GetPlayers())
                        {
                            itr.getSource()->SetPower(POWER_ALTERNATE_POWER, 0);
                            itr.getSource()->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGE);
                            itr.getSource()->RemoveAurasDueToSpell(SPELL_THUNDER_FORGE_CHARGE_TRIGGER);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_thundder_forge_charging_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_ENERGIZE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_thundder_forge_charging_AuraScript();
    }
};

class spell_forging : public SpellScriptLoader
{
public:
    spell_forging() : SpellScriptLoader("spell_forging") { }

    class spell_forging_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_forging_AuraScript);

        void OnTick(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->CastSpell(caster, SPELL_STRIKE_ANVIL_COSMETIC);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_forging_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_forging_AuraScript();
    }
};

class spell_avnil_click_dummy : public SpellScriptLoader
{
public:
    spell_avnil_click_dummy() : SpellScriptLoader("spell_avnil_click_dummy") { }

    class spell_avnil_click_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_avnil_click_dummy_SpellScript);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (Unit* stalker = caster->FindNearestCreature(NPC_ANVIL_STALKER, 20.0f))
            {
                stalker->CastSpell(stalker->GetPositionX(), stalker->GetPositionY(), stalker->GetPositionZ(), SPELL_THUNDER_SURGE);
                stalker->RemoveAllAuras();
            }
        }

        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_avnil_click_dummy_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_avnil_click_dummy_SpellScript();
    }
};

class spell_spec_test : public SpellScriptLoader
{
public:
    spell_spec_test() : SpellScriptLoader("spell_spec_test") { }

    class spell_spec_test_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spec_test_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Player* player = GetHitUnit()->ToPlayer();
            if (!GetCaster() || !player)
                return;
            
            if (InstanceScript* instance = player->GetInstanceScript())
                instance->SetData(DATA_PLAYER_ROLE, player->GetRoleForGroup(player->GetActiveSpec()));
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_spec_test_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_spec_test_SpellScript();
    }
};

void AddSC_fall_of_shan_bu()
{
    new npc_wrathion();

    new go_mogu_crucible();

    new npc_shado_pan_defender();
    new npc_shado_pan_warrior();
    new npc_thunder_forge_second();
    new npc_invisible_hunter();
    new npc_lighting_pilar_beam_stalker();
    new npc_lighting_pilar_spark_stalker();
    new npc_forgemaster_vulkon();
    new npc_shanze_shadowcaster();
    new npc_shanze_battlemaster();
    new npc_shanze_warrior();
    new npc_shanze_electro_coutioner();
    new npc_shanze_pyromancer();
    new npc_celestial_blacksmith();
    new npc_celestial_defender();
    new npc_sha_beast();
    new npc_sha_fiend();
    new npc_sha_amalgamation();

    new spell_phase_shift_update();
    new spell_thundder_forge_charging();
    new spell_forging();
    new spell_avnil_click_dummy();
    new spell_spec_test();
}
