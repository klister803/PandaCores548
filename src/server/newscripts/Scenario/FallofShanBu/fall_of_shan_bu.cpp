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

/*
TODO:

0   normolize trash summon groups at first phase and drop shitty code for it...

1   wipe checks for:
        1 step ( trash summon )
        2 step ( at fight with forgemaster_vulkon )
        3 step ( trash summon in last room - at celestial blacksmith )
        4 step ( at fight with sha_amalgamation )

2   scenario_poi;

3   scenario criteria tree fix - for normolize steps UI

4   kill scripts for thunder_forge_0 and sparks ( it should be handled by AI()->.... )
        - cos they have multiple spawnas and different actions, or handle it with instance data

5   corrections to NPC_THUNDER_FORGE_X actions (THUNDER_FORGE_BUFF summons; visual actions... )

*/

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

    //< NPC_THUNDER_FORGE_4
    SPELL_GROUNDED                              = 139252,
    SPELL_LIGHTING_CHASE                        = 138721,
    SPELL_LIGHTING_STRIKE_TRIGGERED_BY_CHASE    = 138720,

    //< NPC_LIGHTING_LANCE
    SPELL_RIDE_VEHICLE_HARDCODED                = 46598,
    SPELL_THUNDER_FORGE_SPEAR_COSMETIC_LSTRIKES = 140100,
    SPELL_THUNDER_FORGE_SPEAR_COSMETIC_SPARKLES = 139956,

    //< NPC_THUNDER_FORGE_2
    SPELL_THUNDER_FORGE_BUFF_PEREODIC           = 139422,
    SPELL_THUNDER_FORGE_BUFF_TRIGGER            = 139401,

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
    SPELL_METEOR_STORM_3                        = 139443,
    SPELL_METEOR_STORM_4                        = 139658,
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
    SPELL_EMPOWERED_TRIGGER                     = 138948,
    SPELL_DARK_BITE                             = 138956,
    SPELL_ABSORB_EVIL                           = 138950,
    SPELL_LETHARGY                              = 138949,

    //< NPC_ANVIL_STALKER
    SPELL_THUNDER_SURGE                         = 138834,
    SPELL_ANVIL_ACTIVATE_COSMETIC_DND           = 140134,
    SPELL_LIGHTING_STRIKE_COSMETIC              = 140101,

    //< NPC_PHASE3_ROOM_CENTER_STALKER
    SPELL_DEACTIVATE_ALL_AVNILS_TRIGGER         = 140545,
    SPELL_FIND_AVNIL_STALKER_BEST_DUMMY         = 140140,
    SPELL_DEACTIVATE_ALL_AVNILS                 = 140350,
    SPELL_ACTIVATE_ALL_AVNILS                   = 140027,
    SPELL_ELECTIC_DISCHARGE                     = 140047,
    SPELL_ELECTRIC_DISCHARGE_VISUAL             = 140061,

    //< NPC_SHA_AMALGAMATION
    SPELL_INSANITY_TRIGGER                      = 139381,
    SPELL_INSANITY                              = 139382,
    SPELL_SHADOW_BURST                          = 139375,
    SPELL_DARK_BINDING                          = 139358,
    SPELL_METEOR_SOTRM_TRIGGER                  = 139602,
    SPELL_METEOR_STORM_PLAYER_TARGETING         = 139606,
    SPELL_METEOR_STORM                          = 139447,
    SPELL_METEOR_STORM_2                        = 139445,
    SPELL_SUMMON_LEECHING_SHA                   = 139990,
    SPELL_SHADOW_CRASH_TARGETING                = 139360,
    SPELL_SHADOW_CRASH                          = 139312,
    SPELL_SHADOW_CRASH_TRIGGER                  = 139351,
    SPELL_SHADOW_CRASH_CAST                     = 139353,

    //< NPC_DEBILITATING_SHA
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

    // NPC_LIGHTING_SPEAR_FLOAT_STALKER
    SPELL_THROW_LANCE                           = 139696,
};

enum Events
{
    EVENT_NONE,

    EVENT_CHECK_WIPE = 30,

    EVENT_INTRO_PART_2,

    EVENT_STAGE_1_COMPLETED,


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

G3D::Vector3 wPoints[]
{
    {7194.057f, 5256.851f, 66.05624f},
    {7208.982f, 5269.049f, 66.05623f},
    {7216.752f, 5274.640f, 66.05622f},
    {7225.784f, 5277.924f, 66.05622f},
    {7235.042f, 5280.352f, 66.05622f},
    {7252.313f, 5272.183f, 66.05622f},
    {7288.097f, 5241.227f, 66.05624f},
    {7294.803f, 5234.296f, 66.01247f},
    {7308.595f, 5220.985f, 65.51178f},
    {7317.680f, 5212.476f, 65.64245f},
    {7318.930f, 5210.976f, 65.64245f},
    {7324.180f, 5205.226f, 61.64245f},
    {7328.180f, 5200.976f, 59.14245f},
    {7332.680f, 5196.226f, 55.89245f},
    {7333.930f, 5194.726f, 54.64245f},
    {7345.426f, 5183.055f, 49.62558f},
    {7352.973f, 5177.140f, 49.58258f},
    {7361.798f, 5174.109f, 49.58258f},
    {7368.374f, 5170.050f, 49.59216f}
};

G3D::Vector3 bStartPoints[] // 1
{
    {7323.568f, 5114.839f, 55.453f},
    {7330.496f, 5122.524f, 55.370f},
    {7332.888f, 5125.176f, 55.018f},
    {7335.917f, 5130.837f, 52.242f},
    {7336.928f, 5144.771f, 49.652f},
    {7337.110f, 5151.978f, 49.618f},
    {7339.918f, 5160.253f, 49.584f}
};

G3D::Vector3 bF1Points[] // 2
{
    {7341.496f, 5156.917f, 49.828f},
    {7343.496f, 5152.667f, 49.578f},
    {7344.246f, 5150.417f, 49.578f},
    {7344.746f, 5148.917f, 49.578f},
    {7346.996f, 5144.167f, 49.578f},
    {7348.574f, 5140.081f, 49.573f}
};

G3D::Vector3 bF2Points[] // 3
{
    {7352.025f, 5138.677f, 49.829f},
    {7354.525f, 5137.927f, 49.579f},
    {7360.775f, 5135.427f, 49.579f},
    {7365.025f, 5133.927f, 49.579f},
    {7368.478f, 5132.272f, 49.584f}
};

G3D::Vector3 bF3Points[] // 4
{
    {7371.424f, 5133.633f, 49.834f},
    {7376.424f, 5135.633f, 49.584f},
    {7378.424f, 5136.383f, 49.584f},
    {7380.174f, 5137.133f, 49.584f},
    {7384.924f, 5139.133f, 49.584f},
    {7388.370f, 5140.493f, 49.584f}
};

G3D::Vector3 bF4Points[] // 5
{
    {7389.790f, 5143.873f, 49.833f},
    {7391.540f, 5148.373f, 49.583f},
    {7392.290f, 5150.623f, 49.583f},
    {7393.040f, 5152.623f, 49.583f},
    {7394.790f, 5156.873f, 49.583f},
    {7396.211f, 5160.753f, 49.583f}
};

G3D::Vector3 bF5Points[] // 6
{
    {7395.127f, 5163.762f, 49.833f},
    {7393.627f, 5167.262f, 49.583f},
    {7391.627f, 5172.512f, 49.583f},
    {7389.877f, 5176.512f, 49.583f},
    {7388.043f, 5180.270f, 49.584f}
};

G3D::Vector3 bF6Points[] // 7
{
    {7385.785f, 5181.485f, 49.833f},
    {7384.785f, 5181.735f, 49.833f},
    {7380.535f, 5183.235f, 49.583f},
    {7378.285f, 5183.985f, 49.583f},
    {7376.285f, 5184.735f, 49.583f},
    {7372.285f, 5186.485f, 49.583f},
    {7368.527f, 5187.700f, 49.583f}
};

G3D::Vector3 fStalkerPoints[]
{
    {7357.19f, 5171.19f, 49.684f},
    {7368.48f, 5160.31f, 65.293f},
};

class npc_wrathion : public CreatureScript
{
public:
    npc_wrathion() : CreatureScript("npc_wrathion") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Helper::IsNextStageAllowed(instance, STAGE_2))
                player->ADD_GOSSIP_ITEM_DB(15618, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            //else if () // after first wipe in second room - add instance data for checing this
            //    player->ADD_GOSSIP_ITEM_DB(15615, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            else
                player->ADD_GOSSIP_ITEM_DB(15618, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return false;

        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            creature->CastSpell(player, SPELL_SPEC_TEST);
            creature->AI()->DoAction(ACTION_WRATHION_START);
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
        else
        {
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->AI()->DoAction(ACTION_4);
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
                        events.ScheduleEvent(EVENT_6, 1 * IN_MILLISECONDS);
                        step = 1;
                    }
                    break;
                case ACTION_3:
                    events.ScheduleEvent(EVENT_10, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_4:
                    events.ScheduleEvent(EVENT_16, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_5:
                    events.ScheduleEvent(EVENT_22, 5 * IN_MILLISECONDS);
                    break;
                case ACTION_6:
                    events.ScheduleEvent(EVENT_27, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_7:
                    events.ScheduleEvent(EVENT_28, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_8:
                    events.ScheduleEvent(EVENT_29, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_9:
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    //< call texts 25 & 26 after quest completing -> call objectDestroy
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
                        me->SetFacingTo(0.7853982f);
                        me->SetOrientation(0.7853982f);
                        break;
                    case EVENT_5:
                    {
                        events.ScheduleEvent(EVENT_8, 3 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_6, 5 * IN_MILLISECONDS);
                        Talk(26);

                        Map::PlayerList const& players = me->GetMap()->GetPlayers();
                        if (players.isEmpty())
                            break;
                        Player* plr = players.begin()->getSource();
                        if (!plr)
                            break;

                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_1)))
                        {
                            warrior->GetMotionMaster()->MoveJump(7210.65f, 5247.51f, 65.9844f, 15.0f, 25.0f);
                            if (Group* group = plr->GetGroup())
                                group->AddCreatureMember(warrior);
                        }

                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_2)))
                        {
                            warrior->GetMotionMaster()->MoveJump(7195.13f, 5266.81f, 65.9844f, 15.0f, 25.0f);
                            if (Group* group = plr->GetGroup())
                                group->AddCreatureMember(warrior);
                        }

                        if (Creature* defender = Creature::GetCreature(*me, instance->GetData64(DATA_DEFENDER)))
                        {
                            defender->GetMotionMaster()->MoveJump(7213.67f, 5266.37f, 65.9844f, 15.0f, 25.0f);
                            if (Group* group = plr->GetGroup())
                                group->AddCreatureMember(defender);
                        }
                        break;
                    }
                    case EVENT_6:
                        events.ScheduleEvent(EVENT_7, 6 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_9, 200 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_11, 15 * IN_MILLISECONDS);

                        if (Creature* forge2 = me->FindNearestCreature(NPC_THUNDER_FORGE_2, 150.0f))
                            forge2->AI()->DoAction(ACTION_1);

                        me->GetMotionMaster()->MovePoint(EVENT_6, 7195.7f, 5255.59f, 66.05624f);
                        Talk(2);
                        if (Creature* stalker = me->FindNearestCreature(NPC_INVISIBLE_STALKER, 150.0f))
                            if (Player* player = me->FindNearestPlayer(300.0f))
                            {
                                stalker->AddAura(SPELL_THUNDER_FORGE_CHARGE, player);
                                stalker->AddAura(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1, player);
                            }
                        break;
                    case EVENT_8:
                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_1)))
                            warrior->GetMotionMaster()->MovePoint(EVENT_8, 7223.458f, 5262.069f, 65.98731f);

                        if (Creature* warrior = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_2)))
                            warrior->GetMotionMaster()->MovePoint(EVENT_8, 7209.367f, 5279.108f, 66.05622f);

                        if (Creature* defender = Creature::GetCreature(*me, instance->GetData64(DATA_DEFENDER)))
                            defender->GetMotionMaster()->MovePoint(EVENT_8, 7221.26f, 5276.239f, 66.05622f);
                        break;
                    case EVENT_7:
                        me->GetMotionMaster()->MovePoint(EVENT_7, 7199.577f, 5254.172f, 66.27177f);
                        Talk(3);
                        break;
                    case EVENT_2 + NPC_THUNDER_FORGE:
                        events.ScheduleEvent(EVENT_3 + NPC_THUNDER_FORGE, 25 * IN_MILLISECONDS);
                        if (Creature* forge = me->FindNearestCreature(NPC_THUNDER_FORGE, 50.0f))
                            me->CastSpell(forge, SPELL_LIGHTING_STRIKE_COSMETIC_5);
                        break;
                    case EVENT_3 + NPC_THUNDER_FORGE:
                        events.ScheduleEvent(EVENT_2 + NPC_THUNDER_FORGE, 5 * IN_MILLISECONDS);
                        if (Creature* forge = me->FindNearestCreature(NPC_THUNDER_FORGE, 50.0f))
                            forge->CastSpell(forge, SPELL_LIGHTING_STRIKE_COSMETIC_4, true);
                        break;
                    case EVENT_9:
                        instance->SetData(DATA_WAVE_COUNTER, SPECIAL);
                        instance->SetData(DATA_STAGE1_P2, IN_PROGRESS);
                        if (Player* player = me->FindNearestPlayer(300.0f))
                        {
                            player->RemoveAura(SPELL_THUNDER_FORGE_CHARGE);
                            player->RemoveAura(SPELL_THUNDER_FORGE_CHARGING_EVENT_STAGE_1);
                        }
                        events.CancelEvent(EVENT_11);
                        break;
                    case EVENT_10:
                    {
                        events.ScheduleEvent(EVENT_12, 55 * IN_MILLISECONDS);
                        Talk(4);

                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_2)))
                        {
                            cre->GetMotionMaster()->MoveJump(helpersLastJumpPos[0].m_positionX, helpersLastJumpPos[0].m_positionY, helpersLastJumpPos[0].m_positionZ, 20.0f, 20.0f);
                            cre->ForcedDespawn(1 * IN_MILLISECONDS);
                        }

                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_WARRIOR_1)))
                        {
                            cre->GetMotionMaster()->MoveJump(helpersLastJumpPos[1].m_positionX, helpersLastJumpPos[1].m_positionY, helpersLastJumpPos[1].m_positionZ, 20.0f, 20.0f);
                            cre->ForcedDespawn(1 * IN_MILLISECONDS);
                        }

                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_DEFENDER)))
                        {
                            cre->AI()->Talk(2);
                            cre->GetMotionMaster()->MoveJump(helpersLastJumpPos[2].m_positionX, helpersLastJumpPos[2].m_positionY, helpersLastJumpPos[2].m_positionZ, 20.0f, 20.0f);
                            cre->ForcedDespawn(1 * IN_MILLISECONDS);
                        }

                        Map::PlayerList const& players = me->GetMap()->GetPlayers();
                        if (!players.isEmpty())
                        {
                            if (Player* plr = players.begin()->getSource())
                                if (Group* group = plr->GetGroup())
                                {
                                    group->RemoveCreatureMember(instance->GetData64(DATA_WARRIOR_1));
                                    group->RemoveCreatureMember(instance->GetData64(DATA_WARRIOR_2));
                                    group->RemoveCreatureMember(instance->GetData64(DATA_DEFENDER));
                                }
                        }

                        if (Creature* forge2 = me->FindNearestCreature(NPC_THUNDER_FORGE_2, 150.0f))
                            forge2->AI()->DoAction(ACTION_2);

                        me->CastSpell(me, SPELL_WRATHION_TRUE_FORM);

                        Movement::MoveSplineInit init(*me);
                        for (int8 i = 0; i < 19; i++)
                            init.Path().push_back(wPoints[i]);

                        init.SetWalk(false);
                        init.SetSmooth();
                        init.Launch();
                        break;
                    }
                    case EVENT_11:
                        if (Creature* cre = me->FindNearestCreature(NPC_SHADO_PAN_DEFENDER, 150.0f))
                            cre->AI()->DoAction(ACTION_1);
                        break;

                        //< Events in last room
                    case EVENT_12:
                        events.ScheduleEvent(EVENT_13, 10 * IN_MILLISECONDS);
                        me->RemoveAura(SPELL_WRATHION_TRUE_FORM);
                        Talk(5);
                        me->GetMotionMaster()->MovePoint(EVENT_13, 7368.f, 5170.95f, 49.58f);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING);
                        break;
                    case EVENT_13:
                        events.ScheduleEvent(EVENT_14, 8 * IN_MILLISECONDS);
                        Talk(6);
                        break;
                    case EVENT_14:
                    {
                        events.ScheduleEvent(EVENT_15, 11 * IN_MILLISECONDS);
                        Talk(7);
                        me->SetDynamicWorldEffects(505, 0);

                        Player* plr = me->GetMap()->GetPlayers().begin()->getSource();
                        if (!plr)
                            break;

                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_CELESTIAL_BLACKSMITH)))
                            if (Group* group = plr->GetGroup())
                                group->AddCreatureMember(cre);
                        break;
                    }
                    case EVENT_15:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                        Talk(8);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    case EVENT_16: // called from gossip
                    {
                        events.ScheduleEvent(EVENT_17, 6 * IN_MILLISECONDS);
                        me->SetFacingTo(4.709836f);
                        me->SetOrientation(4.709836f);

                        Player* plr = me->GetMap()->GetPlayers().begin()->getSource();
                        if (!plr)
                            break;

                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_CELESTIAL_DEFENDER)))
                            if (Group* group = plr->GetGroup())
                                group->AddCreatureMember(cre);

                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_CELESTIAL_BLACKSMITH)))
                            cre->AI()->DoAction(ACTION_1);

                        break;
                    }
                    case EVENT_17:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING);
                        events.ScheduleEvent(EVENT_18, 6 * IN_MILLISECONDS);
                        Talk(9);
                        break;
                    case EVENT_18:
                        events.ScheduleEvent(EVENT_19, 4 * IN_MILLISECONDS);
                        Talk(10);
                        break;
                    case EVENT_19:
                        events.ScheduleEvent(EVENT_20, 6 * IN_MILLISECONDS);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                        me->GetMotionMaster()->MovePoint(EVENT_19, 7368.f, 5170.95f, 49.58f);
                        Talk(11);
                        break;
                    case EVENT_20:
                        events.ScheduleEvent(EVENT_21, 3 * IN_MILLISECONDS);
                        if (Creature* forge = me->FindNearestCreature(NPC_THUNDER_FORGE, 8.0f))
                            me->SetFacingTo(forge);
                        break;
                    case EVENT_21:
                        events.ScheduleEvent(EVENT_22, 7 * IN_MILLISECONDS);
                        if (Creature* forge = me->FindNearestCreature(NPC_THUNDER_FORGE, 8.0f))
                            me->CastSpell(forge, SPELL_LIGHTING_STRIKE_3);
                        break;

                        //< after NPC_CELESTIAL_BLACKSMITH despawn
                    case EVENT_22: // 07:23:08.000
                    {
                        events.ScheduleEvent(EVENT_23, 9 * IN_MILLISECONDS);
                        Talk(13);

                        std::list<Creature*> sha;
                        GetCreatureListWithEntryInGrid(sha, me, NPC_SHA_BEAST, 150.f);
                        GetCreatureListWithEntryInGrid(sha, me, NPC_SHA_FIEND, 150.f);
                        for (std::list<Creature*>::const_iterator itr = sha.begin(); itr != sha.end(); ++itr)
                            (*itr)->ForcedDespawn(8 * IN_MILLISECONDS);

                        if (Creature* cre = me->FindNearestCreature(NPC_LIGHTING_LANCE, 100.f))
                            if (Creature* stalker = me->FindNearestCreature(NPC_LIGHTING_SPEAR_FLOAT_STALKER, 100.f))
                                cre->EnterVehicle(stalker, 0);

                        if (Creature* cosmetic = me->FindNearestCreature(NPC_COSMETIC_SHA_BOSS, 100.f))
                            cosmetic->AddAura(SPELL_SHA_BOSS_STALKER_COSMETIC, cosmetic);
                        break;
                    }
                    case EVENT_23:
                        events.ScheduleEvent(EVENT_24, 10 * IN_MILLISECONDS);
                        if (Creature* cosmetic = me->FindNearestCreature(NPC_COSMETIC_SHA_BOSS, 100.f))
                        {
                            cosmetic->CastSpell(cosmetic, SPELL_SHA_BOSS_COSMETIC_SPAWN);
                            cosmetic->CastSpell(cosmetic, SPELL_SHA_BOSS_COSMETIC_MISSLE);
                        }
                        Talk(14);
                        break;
                    case EVENT_24:
                        events.ScheduleEvent(EVENT_25, 8 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_SHA_AMALGAMATION, 7348.246f, 5179.011f, 49.38733f);
                        Talk(15);
                        break;
                    case EVENT_25:
                        events.ScheduleEvent(EVENT_26, 6 * IN_MILLISECONDS);
                        Talk(16);
                        break;
                    case EVENT_26:
                        Talk(17);
                        break;
                    case EVENT_27: // after sha death
                        break;
                    case EVENT_28: // at sha 20%
                        // seems uselles now...
                        break;
                    case EVENT_29:
                        events.ScheduleEvent(EVENT_30, 5 * IN_MILLISECONDS);
                        Talk(18);
                        break;
                    case EVENT_30:
                        events.ScheduleEvent(EVENT_31, 3 * IN_MILLISECONDS);
                        Talk(19);
                        break;
                    case EVENT_31:
                        events.ScheduleEvent(EVENT_32, 3 * IN_MILLISECONDS);
                        Talk(20);
                        break;
                    case EVENT_32:
                        events.ScheduleEvent(EVENT_33, 3 * IN_MILLISECONDS);
                        Talk(21);
                        break;
                    case EVENT_33:
                        events.ScheduleEvent(EVENT_34, 3 * IN_MILLISECONDS);
                        Talk(22);
                        break;
                    case EVENT_34:
                        Talk(23);
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
                    instance->SetData(DATA_WAVE_COUNTER, DONE);
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    break;
                default:
                    break;
            }
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_3, 1 * IN_MILLISECONDS);
                    me->SetHealth(me->GetMaxHealth());
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
            if (instance->GetData(DATA_STAGE1_P2) != DONE)
            {
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());
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
                    case EVENT_3:
                        events.ScheduleEvent(EVENT_4, 3 * IN_MILLISECONDS);
                        Talk(1);
                        break;
                    case EVENT_4:
                        instance->SetData(DATA_WAVE_COUNTER, DONE);
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
        { }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case EVENT_8:
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    break;
                case 3:
                    me->SetHealth(me->GetMaxHealth());
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
            if (instance->GetData(DATA_STAGE1_P2) != DONE)
            {
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_shado_pan_warriorAI(creature);
    }
};

class npc_thunder_forge : public CreatureScript
{
public:
    npc_thunder_forge() : CreatureScript("npc_thunder_forge") { }

    struct npc_thunder_forge_secondAI : public ScriptedAI
    {
        npc_thunder_forge_secondAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.ScheduleEvent(EVENT_1, 1 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_2, 1 * IN_MILLISECONDS);
                        me->AddAura(SPELL_LIGHTING_STRIKE, me);
                        break;
                    case EVENT_2:
                        me->CastSpell(me, SPELL_LIGHTING_STRIKE_COSMETIC_3);
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
        return new npc_thunder_forge_secondAI(creature);
    }
};

class npc_thunder_forge_second : public CreatureScript
{
public:
    npc_thunder_forge_second() : CreatureScript("npc_thunder_forge_second") { }

    struct npc_thunder_forge_secondAI : public ScriptedAI
    {
        npc_thunder_forge_secondAI(Creature* creature) : ScriptedAI(creature)
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
                    events.ScheduleEvent(EVENT_1, 15 * IN_MILLISECONDS);
                    break;
                case ACTION_2:
                    events.ScheduleEvent(EVENT_3, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void JustSummoned(Creature* sum)
        {
            switch (sum->GetEntry())
            {
                case NPC_THUNDER_FORGE_3:
                    sum->AI()->DoAction(ACTION_1);
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
                        me->AddAura(SPELL_THUNDER_FORGE_BUFF_PEREODIC, me);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_2, 30 * IN_MILLISECONDS);
                        Talk(0);
                        break;
                    case EVENT_3:
                        events.CancelEvent(EVENT_2);
                        me->RemoveAura(SPELL_THUNDER_FORGE_BUFF_PEREODIC);
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
        return new npc_thunder_forge_secondAI(creature);
    }
};

class npc_thunder_forge_third : public CreatureScript
{
public:
    npc_thunder_forge_third() : CreatureScript("npc_thunder_forge_third") { }

    struct npc_thunder_forge_thirdAI : public ScriptedAI
    {
        npc_thunder_forge_thirdAI(Creature* creature) : ScriptedAI(creature)
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
                    events.ScheduleEvent(EVENT_1, 1 * IN_MILLISECONDS);
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
                        events.ScheduleEvent(EVENT_2, 1 * IN_MILLISECONDS);
                        if (Creature* forge = me->FindNearestCreature(NPC_THUNDER_FORGE, 50.0f))
                            forge->CastSpell(me, SPELL_LIGHTING_STRIKE_COSMETIC_2);
                        break;
                    case EVENT_2:
                        me->CastSpell(me, SPELL_THUNDER_FORGE_BUFF_3);
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
        return new npc_thunder_forge_thirdAI(creature);
    }
};

class npc_invisible_stalker : public CreatureScript
{
public:
    npc_invisible_stalker() : CreatureScript("npc_invisible_stalker") { }

    struct npc_invisible_hunterAI : public Scripted_NoMovementAI
    {
        npc_invisible_hunterAI(Creature* creature) : Scripted_NoMovementAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            instance = creature->GetInstanceScript();
        }

        void Reset()
        { }

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
                    events.ScheduleEvent(EVENT_1, 1 * IN_MILLISECONDS);
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
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_1, 2 * IN_MILLISECONDS);
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
            if (HealthBelowPct(50) && phase != PHASE_TWO)
            {
                events.ScheduleEvent(EVENT_6, 3 * IN_MILLISECONDS, 0, PHASE_TWO);

                phase = PHASE_TWO;
                events.SetPhase(PHASE_TWO);

                SetEquipmentSlots(false, 82347, 0);
                Talk(1);
                me->AddAura(SPELL_DISCHARGE, me);
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
                        events.ScheduleEvent(EVENT_7, 25 * IN_MILLISECONDS, 0, PHASE_ONE);

                        if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST))
                            me->AI()->AttackStart(target);

                        SetEquipmentSlots(false, 94565, 94565);
                        me->AddAura(SPELL_DISCHARGE, me);
                        break;
                    case EVENT_7:
                        events.ScheduleEvent(EVENT_2, 2 * IN_MILLISECONDS, 0, PHASE_ONE);
                        Talk(0);
                        SetEquipmentSlots(false, 89148, 0);
                        me->RemoveAura(SPELL_DISCHARGE);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_3, 5 * IN_MILLISECONDS, 0, PHASE_TWO);
                        DoCast(SPELL_LIGHTING_BOLT);
                        break;
                    case EVENT_3:
                        events.ScheduleEvent(EVENT_3, 10 * IN_MILLISECONDS, 0, PHASE_TWO);
                        events.ScheduleEvent(EVENT_4, 1 * IN_MILLISECONDS, 0, PHASE_TWO);
                        DoCast(SPELL_LIGHTING_STRIKE_TARGETTING);
                        break;
                    case EVENT_4:
                        DoCast(SPELL_LIGHTING_STRIKE_2);
                        break;
                    case EVENT_6:
                        events.ScheduleEvent(EVENT_6, 15 * IN_MILLISECONDS, 0, PHASE_TWO);
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
            {
                if (me->IsWithinDist(me->GetTargetUnit(), 10.0f))
                    DoSpellAttackIfReady(SPELL_LIGHTING_BOLT);
            }
            else
                DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            instance->SetData(DATA_STAGE1_P2, DONE);
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

            events.ScheduleEvent(EVENT_1, 5 * IN_MILLISECONDS);
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
                        events.ScheduleEvent(EVENT_1, 15 * IN_MILLISECONDS);
                        DoCast(SPELL_CORRUPTION);
                        break;
                    default:
                        break;
                }
            }

            if (me->IsWithinDist(me->GetTargetUnit(), 10.0f))
                DoSpellAttackIfReady(SPELL_SHADOW_BOLT);
            else
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

            events.ScheduleEvent(EVENT_1, 7 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_2, 15 * IN_MILLISECONDS);
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
                        events.ScheduleEvent(EVENT_1, 7 * IN_MILLISECONDS);
                        DoCast(SPELL_STORM_STRIKE);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_2, 15 * IN_MILLISECONDS);
                        DoCast(SPELL_LIGHTING_WHIRLWIND2);
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

            events.ScheduleEvent(EVENT_1, 3 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_2, 10 * IN_MILLISECONDS);
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
                        events.ScheduleEvent(EVENT_1, 15 * IN_MILLISECONDS);
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST))
                        {
                            me->CastSpell(target, SPELL_LUMBERING_CHARGE);
                            me->CastSpell(target, SPELL_LUMBERING_CHARGE2);
                        }
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_2, 5 * IN_MILLISECONDS);
                        DoCast(SPELL_CLEAVE);
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

            events.ScheduleEvent(EVENT_1, 2 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_2, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_3, 13 * IN_MILLISECONDS);
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
                        DoCast(SPELL_LIGHTING_CHARGE);
                        break;
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_1, 10 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_LIGHTING_BOLT);
                        break;
                    case EVENT_3:
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

            events.ScheduleEvent(EVENT_1, 5 * IN_MILLISECONDS);
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
                        events.ScheduleEvent(EVENT_1, 15 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_METEOR_TARGETING);
                        DoCastVictim(SPELL_METEOR_AT, true);
                        break;
                    default:
                        break;
                }
            }

            if (me->IsWithinDist(me->GetTargetUnit(), 10.0f))
                DoSpellAttackIfReady(SPELL_FIREBALL);
            else
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

class npc_celestial_blacksmith : public CreatureScript
{
public:
    npc_celestial_blacksmith() : CreatureScript("npc_celestial_blacksmith") { }

    struct npc_celestial_blacksmithAI : public CreatureAI
    {
        npc_celestial_blacksmithAI(Creature* creature) : CreatureAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->setRegeneratingHealth(false);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_1, 17 * IN_MILLISECONDS);
                    InitMovementToForge(ACTION_1);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            events.Reset();

            dead = false;
        }

        void DamageTaken(Unit* /*attacker*/, uint32 &damage)
        {
            if (me->HealthBelowPctDamaged(25, damage) && !dead)
            {
                dead = true;
                Talk(2);
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
                    {
                        events.ScheduleEvent(EVENT_2, 2 * IN_MILLISECONDS);
                        Map::PlayerList const& players = me->GetMap()->GetPlayers();
                        if (Player* plr = players.begin()->getSource())
                            me->AddAura(SPELL_THUNDER_FORGE_CHARGING, plr);
                        break;
                    }
                    //< Forge: 1 07:19:03.000
                    case EVENT_2:
                        events.ScheduleEvent(EVENT_3, 30 * IN_MILLISECONDS);
                        InitCast(0.006548852f);
                        break;
                    case EVENT_3:
                        AvnilActivation(EVENT_4);
                        break;
                    case EVENT_4:
                        events.ScheduleEvent(EVENT_5, 6 * IN_MILLISECONDS);
                        InitMovementToForge(EVENT_4);
                        break;
                        //< Forge: 2 07:19:39.000 
                    case EVENT_5:
                        events.ScheduleEvent(EVENT_6, 30 * IN_MILLISECONDS);
                        InitCast(0.7986659f);
                        break;
                    case EVENT_6:
                        AvnilActivation(EVENT_7);
                        break;
                    case EVENT_7:
                        events.ScheduleEvent(EVENT_8, 6 * IN_MILLISECONDS);
                        InitMovementToForge(EVENT_7);
                        break;
                        //< Forge: 3 07:20:15.000
                    case EVENT_8:
                        events.ScheduleEvent(EVENT_9, 30 * IN_MILLISECONDS);
                        InitCast(1.573552f);
                        break;
                    case EVENT_9:
                        AvnilActivation(EVENT_10);
                        break;
                    case EVENT_10:
                        events.ScheduleEvent(EVENT_11, 6 * IN_MILLISECONDS);
                        InitMovementToForge(EVENT_10);
                        break;
                        //< Forge: 4 07:20:50.000
                    case EVENT_11:
                        events.ScheduleEvent(EVENT_12, 30 * IN_MILLISECONDS);
                        InitCast(2.35677f);
                        break;
                    case EVENT_12:
                        AvnilActivation(EVENT_13);
                        break;
                    case EVENT_13:
                        events.ScheduleEvent(EVENT_14, 6 * IN_MILLISECONDS);
                        InitMovementToForge(EVENT_13);
                        break;
                        //< Forge: 5 07:21:25.000
                    case EVENT_14:
                        events.ScheduleEvent(EVENT_15, 30 * IN_MILLISECONDS);
                        InitCast(3.152864f);
                        break;
                    case EVENT_15:
                        AvnilActivation(EVENT_16);
                        break;
                    case EVENT_16:
                        events.ScheduleEvent(EVENT_17, 6 * IN_MILLISECONDS);
                        InitMovementToForge(EVENT_16);
                        break;
                        //< Forge: 6 07:22:00.000
                    case EVENT_17:
                        events.ScheduleEvent(EVENT_18, 30 * IN_MILLISECONDS);
                        InitCast(3.931716f);
                        break;
                    case EVENT_18:
                        AvnilActivation(EVENT_19);
                        break;
                    case EVENT_19:
                        events.ScheduleEvent(EVENT_20, 6 * IN_MILLISECONDS);
                        InitMovementToForge(EVENT_19);
                        break;
                        //< Forge: 7 07:22:35.000
                    case EVENT_20:
                        events.ScheduleEvent(EVENT_21, 30 * IN_MILLISECONDS);
                        InitCast(4.707732f);
                        break;
                    case EVENT_21:
                        AvnilActivation(EVENT_22);
                        break;
                    case EVENT_22:
                        events.ScheduleEvent(EVENT_23, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_23:
                    {
                        if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_WRATHION)))
                            cre->AI()->DoAction(ACTION_5);

                        Map::PlayerList const& players = me->GetMap()->GetPlayers();
                        if (!players.isEmpty())
                        {
                            if (Player* plr = players.begin()->getSource())
                            {
                                if (Group* group = plr->GetGroup())
                                    group->RemoveCreatureMember(me->GetGUID());

                                me->DestroyForPlayer(plr);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void AvnilActivation(uint32 eventID)
        {
            events.ScheduleEvent(eventID, 1);
            DoCast(SPELL_ACTIVATE_CLOSEST_AVNIL);
            Talk(1);

            if (Creature* stalker = me->FindNearestCreature(NPC_ANVIL_STALKER, 7.0f))
                stalker->AI()->DoAction(ACTION_1);

            if (GameObject* forge5 = me->FindNearestGameObject(GO_THUNDER_FORGE_AVNIL_5, 7.0f))
                forge5->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }

        void InitCast(float facing)
        {
            me->SetFacingTo(facing);
            me->SetOrientation(facing);
            me->AddAura(SPELL_FORGING, me);
        }

        void InitMovementToForge(uint32 event)
        {
            me->SetSpeed(MOVE_RUN, 7.0f, true);
            me->SetSpeed(MOVE_RUN_BACK, 3.15f, true);
            me->SetSpeed(MOVE_SWIM, 3.305554f, true);
            me->SetSpeed(MOVE_FLIGHT, 4.9f, true);

            Movement::MoveSplineInit init(*me);

            switch (event)
            {
                case ACTION_1:
                    for (int8 i = 0; i < 7; i++)
                        init.Path().push_back(bStartPoints[i]);
                    break;
                case EVENT_4:
                    for (int8 i = 0; i < 6; i++)
                        init.Path().push_back(bF1Points[i]);
                    break;
                case EVENT_7:
                    for (int8 i = 0; i < 5; i++)
                        init.Path().push_back(bF2Points[i]);
                    break;
                case EVENT_10:
                    for (int8 i = 0; i < 6; i++)
                        init.Path().push_back(bF3Points[i]);
                    break;
                case EVENT_13:
                    for (int8 i = 0; i < 6; i++)
                        init.Path().push_back(bF4Points[i]);
                    break;
                case EVENT_16:
                    for (int8 i = 0; i < 5; i++)
                        init.Path().push_back(bF5Points[i]);
                    break;
                case EVENT_19:
                    for (int8 i = 0; i < 7; i++)
                        init.Path().push_back(bF6Points[i]);
                    break;
                default:
                    break;
            }

            init.SetVelocity(7.0f);
            init.SetWalk(false);
            init.SetSmooth();
            init.Launch();
        }

        void JustDied(Unit* /*killer*/)
        {
            // disable all events and restart battle! //! Нет, нет, так не пойдет. Уходим отсюда!
        }

    private:
        InstanceScript* instance;
        EventMap events;
        bool dead;
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
        { }

        void EnterEvadeMode()
        { }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
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
        return new npc_celestial_defenderAI(creature);
    }
};

class npc_lighting_spear_float_stalker : public VehicleScript
{
public:
    npc_lighting_spear_float_stalker() : VehicleScript("npc_lighting_spear_float_stalker") { }

    void OnAddPassenger(Vehicle* veh, Unit* /*passenger*/, int8 /*seatId*/)
    {
        if (Unit* base = veh->GetBase())
            if (Creature* cre = base->ToCreature())
                if (cre->AI())
                    cre->AI()->DoAction(ACTION_1);
    }

    struct npc_lighting_spear_float_stalkerAI : public ScriptedAI
    {
        npc_lighting_spear_float_stalkerAI(Creature* creature) : ScriptedAI(creature)
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
                    events.ScheduleEvent(EVENT_1, 1 * IN_MILLISECONDS);
                    break;
                case ACTION_2:
                    events.ScheduleEvent(EVENT_2, 1 * IN_MILLISECONDS);
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
                        MovementHelper(0);
                        break;
                    case EVENT_2:
                        MovementHelper(1);
                        Talk(0);
                        break;
                    default:
                        break;
                }
            }
        }

        void OnSpellClick(Unit* /*clicker*/)
        {
            if (Creature* sha = me->FindNearestCreature(0, 100.0f))
            {
                me->CastSpell(sha, SPELL_THROW_LANCE);
                me->DespawnOrUnsummon(2 * IN_MILLISECONDS);
            }
        }

        void MovementHelper(uint8 point)
        {
            me->SetSpeed(MOVE_RUN, 7.0f, true);
            me->SetSpeed(MOVE_PITCH_RATE, 3.14f, true);
            me->SetSpeed(MOVE_SWIM, 4.72f, true);
            me->SetSpeed(MOVE_FLIGHT, 4.5f, true);

            Movement::MoveSplineInit init(*me);
            init.Path().push_back(fStalkerPoints[point]);
            init.SetVelocity(7.0f);
            init.SetFall();
            init.SetSmooth();
            init.Launch();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lighting_spear_float_stalkerAI(creature);
    }
};

class go_thunder_forge_avnil_5 : public GameObjectScript
{
public:
    go_thunder_forge_avnil_5() : GameObjectScript("go_thunder_forge_avnil_5") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Creature* stalker = go->FindNearestCreature(NPC_ANVIL_STALKER, 7.0f))
                stalker->AI()->DoAction(ACTION_2);

            go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }
        return false;
    }
};

class npc_avnil_stalker : public CreatureScript
{
public:
    npc_avnil_stalker() : CreatureScript("npc_avnil_stalker") { }

    struct npc_avnil_stalkerAI : public CreatureAI
    {
        npc_avnil_stalkerAI(Creature* creature) : CreatureAI(creature) { }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    me->AddAura(SPELL_ANVIL_ACTIVATE_COSMETIC_DND, me);
                    break;
                case ACTION_2:
                    me->RemoveAura(SPELL_ANVIL_ACTIVATE_COSMETIC_DND);
                    me->CastSpell(me, SPELL_THUNDER_SURGE);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        { }

        void UpdateAI(uint32 diff)
        { }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_avnil_stalkerAI(creature);
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
            kill = false;
            stalker = false;
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_WRATHION)))
                cre->AI()->DoAction(ACTION_9);
        }

        void DamageTaken(Unit* /*attacker*/, uint32 &damage)
        {
            if (me->HealthBelowPctDamaged(20, damage) && !kill)
            {
                if (Creature* cre = me->FindNearestCreature(NPC_LIGHTING_SPEAR_FLOAT_STALKER, 100.0f))
                {
                    if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_WRATHION)))
                        cre->AI()->DoAction(ACTION_7);

                    cre->AI()->DoAction(ACTION_2);
                    kill = true;
                }
            }

            if (me->HealthBelowPctDamaged(50, damage) && !stalker)
            {
                if (Creature* cre = Creature::GetCreature(*me, instance->GetData64(DATA_WRATHION)))
                {
                    cre->AI()->DoAction(ACTION_8);
                    stalker = true;
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            //events.ScheduleEvent(EVENT_NSANITY, 1 * MINUTE * IN_MILLISECONDS + 30 * IN_MILLISECONDS);
            //events.ScheduleEvent(EVENT_SHADOW_BURST, urand(40, 50) * IN_MILLISECONDS);
            //events.ScheduleEvent(EVENT_DARK_BINDING, urand(15, 30) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_METEOR_STORM, 1 * MINUTE);
            //events.ScheduleEvent(EVENT_SHADOW_CRASH, 2 * MINUTE);
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
        bool kill;
        bool stalker;
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
            //if (Player* plr = caster->GetMap()->GetPlayers().begin()->getSource())
            //    caster->CastSpell(plr, SPELL_THUNDER_FORGE_CHARGE_TRIGGER);
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
            //Unit* caster = GetCaster();
            //if (!caster)
            //    return;

            //if (Unit* stalker = caster->FindNearestCreature(NPC_ANVIL_STALKER, 20.0f))
            //{
            //    stalker->CastSpell(stalker->GetPositionX(), stalker->GetPositionY(), stalker->GetPositionZ(), SPELL_THUNDER_SURGE);
            //    stalker->RemoveAllAuras();
            //}
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

    new npc_thunder_forge();
    new npc_thunder_forge_second();
    new npc_thunder_forge_third();
    new npc_invisible_stalker();
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
    new npc_lighting_spear_float_stalker();
    new go_thunder_forge_avnil_5();
    new npc_avnil_stalker();
    new npc_sha_beast();
    new npc_sha_fiend();
    new npc_sha_amalgamation();

    new spell_phase_shift_update();
    new spell_thundder_forge_charging();
    new spell_forging();
    new spell_avnil_click_dummy();
    new spell_spec_test();
}
