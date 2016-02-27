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
    //Garrosh
    SPELL_HELLSCREAM_WARSONG         = 144821,
    SPELL_DESECRETE                  = 144748,
    SPELL_EM_DESECRETE               = 144749,
    SPELL_DESECRATED                 = 144762,
    SPELL_EM_DESECRATED              = 144817,
    SPELL_DESECRATED_WEAPON_AT       = 144760,
    SPELL_EM_DESECRATED_WEAPON_AT    = 144818,
    SPELL_DESECRATED_WEAPON_AXE      = 145880,
    SPELL_WHIRLING_CORRUPTION        = 144985,
    SPELL_EM_WHIRLING_CORRUPTION     = 145037,
    SPELL_EM_WHIRLING_CORRUPTION_S   = 145023,
    SPELL_GRIPPING_DESPAIR           = 145183,
    SPELL_EM_GRIPPING_DESPAIR        = 145195,
    //Garrosh Special
    SPELL_TRANSITION_VISUAL_PHASE_2  = 144852,
    SPELL_TRANSITION_VISUAL_BASE     = 146756,
    SPELL_TRANSITION_VISUAL_ADVANCE  = 146845,
    SPELL_TRANSITION_VISUAL_PHASE_3  = 145222,
    SPELL_PHASE_TWO_TRANSFORM        = 144842,
    SPELL_PHASE_THREE_TRANSFORM      = 145246,
    //Embodied despair
    SPELL_CONSUMED_HOPE              = 149032,
    SPELL_HOPE_AT                    = 149003,
    SPELL_HOPE_BUFF                  = 149004,
    SPELL_EMBODIED_DESPAIR           = 145276,
    SPELL_COURAGE                    = 148983,

    //Iron Star
    SPELL_IRON_STAR_IMPACT_AT        = 144645,
    SPELL_IRON_STAR_IMPACT_DMG       = 144650,
    SPELL_EXPLODING_IRON_STAR        = 144798,

    //Engeneer
    SPELL_POWER_IRON_STAR            = 144616,

    //Warbringer
    SPELL_HAMSTRING                  = 144582,

    //Wolf Rider
    SPELL_ANCESTRAL_FURY             = 144585,
    SPELL_ANCESTRAL_CHAIN_HEAL       = 144583,
    SPELL_CHAIN_LIGHTNING            = 144584,
    //Embodied doubt
    SPELL_EMBODIED_DOUBT             = 145275,
    //Minion of Yshaarj
    SPELL_EMPOWERED                  = 145050,

    //Realm of Yshaarj
    SPELL_GARROSH_ENERGY             = 145801,
    SPELL_REMOVE_REALM_OF_YSHAARJ    = 145647,
    SPELL_REALM_OF_YSHAARJ           = 144954,
    SPELL_ANNIHILLATE                = 144969,
    SPELL_COSMETIC_CHANNEL           = 145431,

    //Special
    SPELL_SUMMON_ADDS                = 144489,
    SPELL_HEARTBEAT_SOUND            = 148591,
    SPELL_ENTER_REALM_OF_YSHAARJ     = 144867,
};

enum sEvents
{
    //Garrosh
    EVENT_DESECRATED_WEAPON          = 1,
    EVENT_HELLSCREAM_WARSONG         = 2,
    EVENT_SUMMON_WARBRINGERS         = 3,
    EVENT_SUMMON_WOLF_RIDER          = 4,
    EVENT_SUMMON_ENGINEER            = 5,
    EVENT_PHASE_TWO                  = 6,
    EVENT_WHIRLING_CORRUPTION        = 7,
    EVENT_GRIPPING_DESPAIR           = 8,
    EVENT_ENTER_REALM_OF_YSHAARJ     = 9,
    EVENT_RETURN_TO_REAL             = 10,
    //In Realm
    EVENT_ANNIHILLATE                = 11,
    //Desecrated weapon
    EVENT_REGENERATE                 = 12,
    //Summons
    EVENT_LAUNCH_STAR                = 13,
    EVENT_HAMSTRING                  = 14,
    EVENT_CHAIN_HEAL                 = 15,
    EVENT_CHAIN_LIGHTNING            = 16,
    //Iron Star
    EVENT_ACTIVE                     = 17,
    //Adds in realm
    EVENT_EMBODIED_DESPAIR           = 18,
    EVENT_EMBODIED_DOUBT             = 19,
};

enum Phase
{
    PHASE_NULL,
    PHASE_ONE,
    PHASE_PREPARE,
    PHASE_REALM_OF_YSHAARJ,
    PHASE_TWO,
    PHASE_LAST_PREPARE,
    PHASE_THREE,
};

enum sActions
{
    ACTION_LAUNCH                    = 1,
    ACTION_PHASE_PREPARE             = 2,
    ACTION_INTRO_REALM_OF_YSHAARJ    = 3,
    ACTION_INTRO_PHASE_THREE         = 4,
    ACTION_PHASE_THREE               = 5,
};

Position ironstarspawnpos[2] =
{
    {1087.05f, -5758.29f, -317.689f, 1.45992f},//R
    {1059.92f, -5520.2f, -317.689f, 4.64799f}, //L
};

Position engeneerspawnpos[2] =
{
    {1061.00f, -5755.13f, -304.4855f, 1.4820f},//R
    {1084.50f, -5524.25f, -304.4856f, 4.5215f},//L
};

Position rsspos[3] =
{
    {1015.77f, -5696.90f, -317.6967f, 6.1730f}, 
    {1014.27f, -5703.18f, -317.6998f, 6.1730f},
    {1012.70f, -5711.02f, -317.7060f, 6.1730f},
};

Position lsspos[3] = 
{
    {1028.11f, -5572.19f, -317.6992f, 6.1730f},
    {1028.52f, -5563.87f, -317.7022f, 6.1730f},
    {1029.31f, -5556.24f, -317.7059f, 6.1730f},
};

Position wspos[2] =
{
    {1020.22f, -5703.03f, -317.6922f, 6.1730f},
    {1034.52f, -5565.62f, -317.6930f, 6.1730f},
};

Position tppos[3] =
{   //1.1 scale sha vortex
    {1092.66f, -5453.67f, -354.902802f, 1.4436f},//Temple of the Jade Serpent
    {1084.72f, -5631.07f, -423.453369f, 3.0819f},//Terrace of Endless Spring
    {1055.22f, -5844.58f, -318.864105f, 4.6069f},//Temple of the Red Crane 
};

Position gspos[3] =
{
    {1105.88f, -5339.08f, -349.7873f, 4.5881f},  //Temple of the Jade Serpent
    {820.47f, -5601.41f, -397.7068f, 6.1840f},   //Terrace of Endless Spring
    {1056.55f, -5829.83f, -368.6667f, 4.6313f},  //Temple of the Red Crane
};

Position centerpos = {1073.09f, -5639.70f, -317.3894f};
Position realmtppos = {1073.14f, -5639.47f, -317.3893f, 3.0128f};

uint32 transformvisual[4] =
{
    SPELL_TRANSITION_VISUAL_PHASE_2,
    SPELL_TRANSITION_VISUAL_PHASE_3,
    SPELL_PHASE_TWO_TRANSFORM,
    SPELL_PHASE_THREE_TRANSFORM,
};

enum CreatureText
{
    //Real
    SAY_ENTERCOMBAT                 = 1,//Я, Гаррош, сын Грома, покажу вам, что значит быть Адским Криком! 38064
    SAY_HELLSCREAM_WARSONG          = 2,//Умрите с честью! 38075
    SAY_SUMMON_WOLF_RIDER           = 3,//Исцелите наши раны! 38072
    SAY_START_LAUNCH_IRON_STAR      = 4,//Узрите силу оружия Истинной Орды! 38068
    SAY_START_LAUNCH_IRON_STAR2     = 5,//Мы очистим этот мир сталью и пламенем! 38070
    SAY_PHASE_PREPARE               = 6,//Злоба. Ненависть. Страх! Вот орудия войны, вот слуги Вождя! 38048
    SAY_PHASE_REALM_OF_YSHAARJ      = 7,//Да... я вижу... вижу, какое будущее ждет этот мир... им будет править Орда... Моя Орда! 38051
    SAY_WHIRLING_CORRUPTION         = 8,//Я полон силы! 38076
    SAY_EM_WHIRLING_CORRUPTION      = 9,//Сила во мне уничтожит вас и весь ваш мир. 38077
    SAY_LAST_PHASE                  = 10,//Начнется правление Истинной Орды. Я видел это. Он показал мне горы черепов и реки крови. Мир... будет... моим! 38056
    SAY_KILL_PLAYER                 = 11,//Ничтожество! 38065
    SAY_DIE                         = 12,//Это не может... кончиться... так... Нет... я же видел... 38046
    //Realm of Yshaarj
    SAY_ENTER_REALM_OF_YSHAARJ      = 13,//Вы окажетесь в ловушке навеки! 38055
};

class boss_garrosh_hellscream : public CreatureScript
{
    public:
        boss_garrosh_hellscream() : CreatureScript("boss_garrosh_hellscream") {}

        struct boss_garrosh_hellscreamAI : public BossAI
        {
            boss_garrosh_hellscreamAI(Creature* creature) : BossAI(creature, DATA_GARROSH)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            uint32 realmofyshaarjtimer;
            uint32 updatepower;
            Phase phase;

            void Reset()
            {
                updatepower = 0;
                realmofyshaarjtimer = 0;
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 0);
                if (me->ToTempSummon()) //Realm of Yshaarj
                {
                    updatepower = 1500;
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                    me->SetReactState(REACT_PASSIVE);
                    DoCast(me, SPELL_YSHAARJ_PROTECTION);
                }
                else
                {                       //Main
                    _Reset();
                    for (uint8 n = 0; n < 4; n++)
                        me->RemoveAurasDueToSpell(transformvisual[n]);
                    //me->SetReactState(REACT_PASSIVE);//test only
                    me->SetReactState(REACT_DEFENSIVE);
                    phase = PHASE_NULL;
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GARROSH_ENERGY);
                }
            }

            void SpawnIronStar()
            {
                for (uint8 n = 0; n < 2; n++)
                    me->SummonCreature(NPC_KORKRON_IRON_STAR, ironstarspawnpos[n]);
            }

            void EnterCombat(Unit* who)
            {
                if (!me->ToTempSummon())
                {
                    /*if (!instance->GetData(DATA_CHECK_INSTANCE_PROGRESS))
                    {
                    EnterEvadeMode();
                    return;
                    }*/
                    Talk(SAY_ENTERCOMBAT, 0);
                    _EnterCombat();
                    SpawnIronStar();
                    phase = PHASE_ONE;
                    events.ScheduleEvent(EVENT_SUMMON_WARBRINGERS, 1000);
                    events.ScheduleEvent(EVENT_DESECRATED_WEAPON, 12000);
                    events.ScheduleEvent(EVENT_HELLSCREAM_WARSONG, 18000);
                    events.ScheduleEvent(EVENT_SUMMON_WOLF_RIDER, 30000);
                    events.ScheduleEvent(EVENT_SUMMON_ENGINEER, 20000);
                }
            }

            void OnUnitDeath(Unit* unit)
            {
                if (unit->ToPlayer())
                    Talk(SAY_KILL_PLAYER);
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                //Garrosh in realm of yshaarj
                if (me->ToTempSummon()) 
                    if (damage >= me->GetHealth())
                        damage = 0;

                //Garrosh in real
                if (HealthBelowPct(10) && phase == PHASE_ONE)
                {   //phase two
                    phase = PHASE_PREPARE;
                    DoAction(ACTION_PHASE_PREPARE);
                }
                else if (HealthBelowPct(10) && phase == PHASE_TWO)
                {   //phase three
                    phase = PHASE_LAST_PREPARE;
                    realmofyshaarjtimer = 0;
                    DoAction(ACTION_INTRO_PHASE_THREE);
                }                                            //protect from exploit
                else if (phase == PHASE_PREPARE || phase == PHASE_REALM_OF_YSHAARJ)
                    if (damage >= me->GetHealth())
                        damage = 0;
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_PHASE_PREPARE:
                    Talk(SAY_PHASE_PREPARE, 0);
                    events.Reset();
                    me->SetAttackStop(false);
                    me->GetMotionMaster()->MoveCharge(centerpos.GetPositionX(), centerpos.GetPositionY(), centerpos.GetPositionZ(), 15.0f, 1);
                    break;
                case ACTION_INTRO_REALM_OF_YSHAARJ:
                    Talk(SAY_PHASE_REALM_OF_YSHAARJ, 0);
                    me->ReAttackWithZone();
                    events.ScheduleEvent(EVENT_ENTER_REALM_OF_YSHAARJ, 12000);
                    break;
                case ACTION_LAUNCH_ANNIHILLATE:
                    updatepower = 0;
                    events.ScheduleEvent(EVENT_ANNIHILLATE, 4000);
                    break;
                case ACTION_INTRO_PHASE_THREE:
                    events.Reset();
                    me->SetAttackStop(false);
                    me->GetMotionMaster()->MoveCharge(centerpos.GetPositionX(), centerpos.GetPositionY(), centerpos.GetPositionZ(), 15.0f, 2);
                    break;
                case ACTION_PHASE_THREE:
                    phase = PHASE_THREE;
                    me->SetHealth(me->CountPctFromMaxHealth(28));
                    me->SetPower(POWER_ENERGY, 100);
                    me->ReAttackWithZone();
                    events.ScheduleEvent(EVENT_GRIPPING_DESPAIR, 2000);
                    events.ScheduleEvent(EVENT_DESECRATED_WEAPON, 12000);
                    events.ScheduleEvent(EVENT_WHIRLING_CORRUPTION, 30000);
                    break;
                }
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type == POINT_MOTION_TYPE)
                {
                    switch (pointId)
                    {
                    case 1:
                        me->RemoveAurasDueToSpell(SPELL_TRANSITION_VISUAL_PHASE_2);
                        DoCast(me, SPELL_TRANSITION_VISUAL_BASE);
                        DoCast(me, SPELL_TRANSITION_VISUAL_PHASE_2);
                        break;
                    case 2:
                        Talk(SAY_LAST_PHASE, 0);
                        me->RemoveAurasDueToSpell(SPELL_TRANSITION_VISUAL_PHASE_2);
                        DoCast(me, SPELL_TRANSITION_VISUAL_BASE);
                        DoCast(me, SPELL_TRANSITION_VISUAL_ADVANCE);
                        DoCast(me, SPELL_TRANSITION_VISUAL_PHASE_3);
                        break;
                    default:
                        break;
                    }
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DIE, 0);
                _JustDied();
            }

            bool IfTargetHavePlayersInRange(Player* target, uint8 count)
            {
                count++;
                std::list<Player*>pllist;
                GetPlayerListInGrid(pllist, target, 15.0f);
                if (pllist.size() >= count)
                    return true;
                return false;
            }

            void UpdateAI(uint32 diff)
            {
                //Garrosh from realm yshaarj(update power)
                if (updatepower)
                {
                    if (updatepower <= diff)
                    {
                        if (me->GetPower(POWER_ENERGY) <= 99)
                        {
                            uint8 power = me->GetPower(POWER_ENERGY) + 1;
                            me->SetPower(POWER_ENERGY, power);
                            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                            if (!PlayerList.isEmpty())
                                for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
                                    if (Player* player = Itr->getSource())
                                        if (player->isAlive())
                                            if (player->HasAura(SPELL_GARROSH_ENERGY))
                                                player->SetPower(POWER_ALTERNATE_POWER, power);
                            updatepower = 1500;
                        }
                    }
                    else
                        updatepower -= diff;
                }
                //

                //Garrosh real
                if (realmofyshaarjtimer)
                {
                    if (realmofyshaarjtimer <= diff)
                    {
                        realmofyshaarjtimer = 0;
                        phase = PHASE_PREPARE;
                        DoAction(ACTION_PHASE_PREPARE);
                    }
                    else
                        realmofyshaarjtimer -= diff;
                }
                //

                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    //In Realm
                    //Phase one
                    case EVENT_ANNIHILLATE:
                    {
                        float mod = urand(0, 6);
                        float orientation = mod <= 5 ? mod + float(urand(1, 9)) / 10 : mod;
                        me->SetFacingTo(orientation);
                        DoCast(me, SPELL_ANNIHILLATE);
                        events.ScheduleEvent(EVENT_ANNIHILLATE, 4000);
                        break;
                    }
                    //
                    case EVENT_SUMMON_WARBRINGERS:
                        instance->SetData(DATA_OPEN_SOLDIER_FENCH, 0);
                        for (uint8 n = 0; n < 3; n++)
                            if (Creature* rsoldier = me->SummonCreature(NPC_WARBRINGER, rsspos[n]))
                                rsoldier->AI()->DoZoneInCombat(rsoldier, 200.0f);
                        for (uint8 n = 0; n < 3; n++)
                            if (Creature* lsoldier = me->SummonCreature(NPC_WARBRINGER, lsspos[n]))
                                lsoldier->AI()->DoZoneInCombat(lsoldier, 200.0f);
                        events.ScheduleEvent(EVENT_SUMMON_WARBRINGERS, 40000);
                        break;
                    case EVENT_HELLSCREAM_WARSONG:
                        Talk(SAY_HELLSCREAM_WARSONG, 0);
                        DoCast(me, SPELL_HELLSCREAM_WARSONG);
                        events.ScheduleEvent(EVENT_HELLSCREAM_WARSONG, 38000);
                        break;
                    case EVENT_SUMMON_WOLF_RIDER:
                    {
                        Talk(SAY_SUMMON_WOLF_RIDER, 0);
                        uint8 pos = urand(0, 1);
                        if (Creature* wrider = me->SummonCreature(NPC_WOLF_RIDER, wspos[pos]))
                            wrider->AI()->DoZoneInCombat(wrider, 200.0f);
                        events.ScheduleEvent(EVENT_SUMMON_WOLF_RIDER, 50000);
                    }
                    break;
                    case EVENT_SUMMON_ENGINEER:
                        for (uint8 n = 0; n < 2; n++)
                            me->SummonCreature(NPC_SIEGE_ENGINEER, engeneerspawnpos[n]);
                        Talk(urand(SAY_START_LAUNCH_IRON_STAR, SAY_START_LAUNCH_IRON_STAR2), 0);
                        events.ScheduleEvent(EVENT_SUMMON_ENGINEER, 40000);
                        break;
                    case EVENT_DESECRATED_WEAPON:
                    {
                        uint8 count = Is25ManRaid() ? 7 : 3;
                        bool havetarget = false;
                        std::list<Player*> pllist;
                        pllist.clear();
                        GetPlayerListInGrid(pllist, me, 150.0f);
                        if (!pllist.empty())
                        {
                            for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                            {
                                if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && me->GetExactDist(*itr) >= 15.0f)
                                {
                                    if (IfTargetHavePlayersInRange(*itr, count))
                                    {
                                        havetarget = true;
                                        DoCast(*itr, me->GetPower(POWER_ENERGY) >= 75 ? SPELL_EM_DESECRETE : SPELL_DESECRETE);
                                        break;
                                    }
                                }
                            }
                            //If no target in range, take melee
                            if (!havetarget)
                            {
                                for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                                {
                                    if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK)
                                    {
                                        if (IfTargetHavePlayersInRange(*itr, count))
                                        {
                                            havetarget = true;
                                            DoCast(*itr, me->GetPower(POWER_ENERGY) >= 75 ? SPELL_EM_DESECRETE : SPELL_DESECRETE);
                                            break;
                                        }
                                    }
                                }
                            }
                            //If still no target, take random include tank
                            if (!havetarget)
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                    DoCast(target, me->GetPower(POWER_ENERGY) >= 75 ? SPELL_EM_DESECRETE : SPELL_DESECRETE);
                        }
                        events.ScheduleEvent(EVENT_DESECRATED_WEAPON, 40000);
                        break;
                    }
                    case EVENT_ENTER_REALM_OF_YSHAARJ:
                    {
                        me->SetAttackStop(false);
                        uint8 mod = instance->GetData(DATA_GET_REALM_OF_YSHAARJ);
                        if (Creature* garroshrealm = me->SummonCreature(NPC_GARROSH, gspos[mod]))
                        {
                            uint32 hp = me->GetHealth();
                            uint32 power = me->GetPower(POWER_ENERGY);
                            garroshrealm->SetHealth(hp);
                            garroshrealm->SetPower(POWER_ENERGY, power);
                            std::list<Player*>pllist;
                            GetPlayerListInGrid(pllist, me, 150.0f);
                            if (!pllist.empty())
                            {
                                for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                                {
                                    (*itr)->NearTeleportTo(tppos[mod].GetPositionX(), tppos[mod].GetPositionY(), tppos[mod].GetPositionZ(), tppos[mod].GetOrientation());
                                    (*itr)->AddAura(SPELL_REALM_OF_YSHAARJ, *itr);
                                    (*itr)->CastSpell(*itr, SPELL_GARROSH_ENERGY);
                                }
                            }
                            garroshrealm->AI()->Talk(SAY_ENTER_REALM_OF_YSHAARJ, 0);
                            phase = PHASE_REALM_OF_YSHAARJ;
                            events.ScheduleEvent(EVENT_RETURN_TO_REAL, 60000);
                        }
                        break;
                    }
                    case EVENT_RETURN_TO_REAL:
                    {
                        Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                        if (!PlayerList.isEmpty())
                            for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
                                if (Player* player = Itr->getSource())
                                    if (player->isAlive())
                                        if (player->HasAura(SPELL_REALM_OF_YSHAARJ))
                                            player->RemoveAura(SPELL_REALM_OF_YSHAARJ, AURA_REMOVE_BY_EXPIRE);
                        events.ScheduleEvent(EVENT_PHASE_TWO, 2000);
                    }
                    break;
                    //Phase Two
                    case EVENT_PHASE_TWO:
                        if (Creature* garroshrealm = me->GetCreature(*me, instance->GetData64(DATA_GARROSH_REALM)))
                        {
                            int32 power = garroshrealm->GetPower(POWER_ENERGY);
                            uint32 hp = garroshrealm->GetHealth();
                            garroshrealm->DespawnOrUnsummon();
                            instance->SetData(DATA_UPDATE_GARROSH_REALM, 0);
                            me->SetPower(POWER_ENERGY, power);
                            me->SetHealth(hp);
                            phase = PHASE_TWO;
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->ReAttackWithZone();
                            events.ScheduleEvent(EVENT_GRIPPING_DESPAIR, 2000);
                            events.ScheduleEvent(EVENT_DESECRATED_WEAPON, 12000);
                            events.ScheduleEvent(EVENT_WHIRLING_CORRUPTION, 30000); 
                            realmofyshaarjtimer = 150000;
                        }
                        break;
                    case EVENT_GRIPPING_DESPAIR:
                        if (me->getVictim())
                            DoCastVictim(SPELL_GRIPPING_DESPAIR);
                        events.ScheduleEvent(EVENT_GRIPPING_DESPAIR, 4000);
                        break;
                    case EVENT_WHIRLING_CORRUPTION:
                        Talk(me->GetPower(POWER_ENERGY) >= 25 ? SAY_WHIRLING_CORRUPTION : SAY_EM_WHIRLING_CORRUPTION, 0);
                        DoCast(me, me->GetPower(POWER_ENERGY) >= 25 ? SPELL_EM_WHIRLING_CORRUPTION : SPELL_WHIRLING_CORRUPTION);
                        events.ScheduleEvent(EVENT_WHIRLING_CORRUPTION, 40000);
                        break;
                    }
                }
                if (!me->ToTempSummon())
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_garrosh_hellscreamAI(creature);
        }
};

//All summons, include realm of yshaarj
class npc_garrosh_soldier : public CreatureScript
{
public:
    npc_garrosh_soldier() : CreatureScript("npc_garrosh_soldier") {}

    struct npc_garrosh_soldierAI : public ScriptedAI
    {
        npc_garrosh_soldierAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        InstanceScript* instance;
        EventMap events;
        bool firstengeneerdied;

        void Reset()
        {
            firstengeneerdied = false;
            switch (me->GetEntry())
            {
            case NPC_SIEGE_ENGINEER:
                me->SetReactState(REACT_PASSIVE);
                events.ScheduleEvent(EVENT_LAUNCH_STAR, 500);
                break;
            case NPC_EMBODIED_DESPAIR:
                me->SetReactState(REACT_AGGRESSIVE);
                DoCast(me, SPELL_CONSUMED_HOPE, true);
                break;
            case NPC_MINION_OF_YSHAARJ:
                me->SetReactState(REACT_AGGRESSIVE);
                DoCast(me, SPELL_EMPOWERED);
                break;
            case NPC_EMBODIED_DOUBT:
                me->SetReactState(REACT_AGGRESSIVE);
                break;
            default:
                me->AddAura(SPELL_SUMMON_ADDS, me);
                break;
            }
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (me->GetEntry() == NPC_SIEGE_ENGINEER)
                if (firstengeneerdied)
                    damage = 0;
        }

        void DoAction(int32 const action)
        {
            if (me->GetEntry() == NPC_SIEGE_ENGINEER)
                if (action == ACTION_FIRST_ENGENEER_DIED)
                    firstengeneerdied = true;
        }

        void JustDied(Unit* killer)
        {
            if (me->GetEntry() == NPC_SIEGE_ENGINEER)
                instance->SetData(DATA_FIRST_ENGENEER_DIED, 1);
            else if (me->GetEntry() == NPC_WARBRINGER || me->GetEntry() == NPC_WOLF_RIDER)
                me->DespawnOrUnsummon();
        }

        void EnterCombat(Unit* who)
        {
            switch (me->GetEntry())
            {
            case NPC_WARBRINGER:
                events.ScheduleEvent(EVENT_HAMSTRING, 3000);
                break;
            case NPC_WOLF_RIDER:
                DoCast(me, SPELL_ANCESTRAL_FURY);
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 15000);
                events.ScheduleEvent(EVENT_CHAIN_HEAL, 21500);
                break;
            case NPC_EMBODIED_DESPAIR:
                events.ScheduleEvent(EVENT_EMBODIED_DESPAIR, 10000);
                break;
            case NPC_EMBODIED_DOUBT:
                events.ScheduleEvent(EVENT_EMBODIED_DOUBT, urand(8000, 13000));
                break;
            default:
                break;
            }
        }

        void EnterEvadeMode(){}

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
                    //Engineer
                case EVENT_LAUNCH_STAR:
                    if (Creature* ironstar = me->FindNearestCreature(NPC_KORKRON_IRON_STAR, 30.0f, true))
                    {
                        me->SetFacingToObject(ironstar);
                        DoCast(me, SPELL_POWER_IRON_STAR);
                    }
                    break;
                    //Warbringer
                case EVENT_HAMSTRING:
                    if (me->getVictim())
                        DoCastVictim(SPELL_HAMSTRING);
                    events.ScheduleEvent(EVENT_HAMSTRING, 3000);
                    break;
                    //Wolf Rider
                case EVENT_CHAIN_LIGHTNING:
                {
                    std::list<Player*> pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, me, 100.0f);
                    if (!pllist.empty())
                    {
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                        {
                            if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK)
                            {
                                DoCast(*itr, SPELL_CHAIN_LIGHTNING);
                                break;
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 15000);
                    break;
                }
                case EVENT_CHAIN_HEAL:
                    if (Unit* ftarget = DoSelectLowestHpFriendly(60.0f))
                        if (ftarget->HealthBelowPct(90))
                            DoCast(ftarget, SPELL_ANCESTRAL_CHAIN_HEAL);
                    events.ScheduleEvent(EVENT_CHAIN_HEAL, 21500);
                    break;
                //Realm of Yshaarj
                //Embodied Despair
                case EVENT_EMBODIED_DESPAIR:
                    DoCast(me, SPELL_EMBODIED_DESPAIR);
                    events.ScheduleEvent(EVENT_EMBODIED_DESPAIR, 20000);
                    break;
                //Embodied doubt
                case EVENT_EMBODIED_DOUBT:
                    DoCast(me, SPELL_EMBODIED_DOUBT);
                    events.ScheduleEvent(EVENT_EMBODIED_DOUBT, urand(8000, 13000));
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_garrosh_soldierAI(creature);
    }
};

//71985
class npc_iron_star : public CreatureScript
{
public:
    npc_iron_star() : CreatureScript("npc_iron_star") {}

    struct npc_iron_starAI : public ScriptedAI
    {
        npc_iron_starAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset(){}

        void DoAction(int32 const action)
        {
            if (action == ACTION_LAUNCH)
            {
                float x, y;
                DoCast(me, SPELL_IRON_STAR_IMPACT_AT, true);
                GetPositionWithDistInOrientation(me, 200.0f, me->GetOrientation(), x, y);
                me->GetMotionMaster()->MoveJump(x, y, -317.4815f, 25.0f, 0.0f, 1);
            }
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == EFFECT_MOTION_TYPE && pointId == 1)
            {
                DoCast(me, SPELL_EXPLODING_IRON_STAR, true);
                me->Kill(me, true);
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_ACTIVE)
                {
                    float x, y;
                    DoCast(me, SPELL_IRON_STAR_IMPACT_AT, true);
                    GetPositionWithDistInOrientation(me, 200.0f, me->GetOrientation(), x, y);
                    me->GetMotionMaster()->MoveJump(x, y, -317.4815f, 25.0f, 0.0f, 1);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_iron_starAI(creature);
    }
};

//72154
class npc_desecrated_weapon : public CreatureScript
{
public:
    npc_desecrated_weapon() : CreatureScript("npc_desecrated_weapon") {}

    struct npc_desecrated_weaponAI : public ScriptedAI
    {
        npc_desecrated_weaponAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            lastpct = 90;
        }

        InstanceScript* instance;
        uint8 lastpct;

        void Reset()
        {
            DoCast(me, SPELL_DESECRATED_WEAPON_AXE);
            DoCast(me, SPELL_DESECRATED_WEAPON_AT);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (lastpct && HealthBelowPct(lastpct))
            {
                float scale = float(lastpct)/100;
                if (AreaTrigger* at = me->GetAreaObject(SPELL_DESECRATED_WEAPON_AT))
                    at->SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
                lastpct = lastpct - 10;
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_desecrated_weaponAI(creature);
    }
};

//72198
class npc_empowered_desecrated_weapon : public CreatureScript
{
public:
    npc_empowered_desecrated_weapon() : CreatureScript("npc_empowered_desecrated_weapon") {}

    struct npc_empowered_desecrated_weaponAI : public ScriptedAI
    {
        npc_empowered_desecrated_weaponAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            lastpct = 90;
        }

        InstanceScript* instance;
        EventMap events;
        uint32 hppctmod;
        uint8 lastpct;

        void Reset()
        {
            events.Reset();
            hppctmod = me->CountPctFromMaxHealth(10);
            DoCast(me, SPELL_DESECRATED_WEAPON_AXE);
            DoCast(me, SPELL_EM_DESECRATED_WEAPON_AT);
            events.ScheduleEvent(EVENT_REGENERATE, 10000);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;

            if (lastpct && HealthBelowPct(lastpct))
            {
                float scale = float(lastpct) / 100;
                if (AreaTrigger* at = me->GetAreaObject(SPELL_EM_DESECRATED_WEAPON_AT))
                    at->SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
                lastpct = lastpct - 10;
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_REGENERATE)
                {
                    if (me->GetHealthPct() < 100)
                    {
                        float scale = 0;
                        if (me->GetHealth() + hppctmod >= me->GetMaxHealth())
                        {
                            me->SetFullHealth();
                            lastpct = 100;
                            scale = 1;
                        }
                        else
                        {
                            me->SetHealth(me->GetHealth() + hppctmod);
                            lastpct = uint8(floor(me->GetHealthPct()));
                            scale = float(lastpct) / 100;
                        }
                        if (AreaTrigger* at = me->GetAreaObject(SPELL_EM_DESECRATED_WEAPON_AT))
                            at->SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
                    }
                    events.ScheduleEvent(EVENT_REGENERATE, 10000);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_empowered_desecrated_weaponAI(creature);
    }
};

//72215 
class npc_heart_of_yshaarj : public CreatureScript
{
public:
    npc_heart_of_yshaarj() : CreatureScript("npc_heart_of_yshaarj") {}

    struct npc_heart_of_yshaarjAI : public ScriptedAI
    {
        npc_heart_of_yshaarjAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_HEARTBEAT_SOUND, true);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_heart_of_yshaarjAI(creature);
    }
};

//72228
class npc_heart_of_yshaarj_realm : public CreatureScript
{
public:
    npc_heart_of_yshaarj_realm() : CreatureScript("npc_heart_of_yshaarj_realm") {}

    struct npc_heart_of_yshaarj_realmAI : public ScriptedAI
    {
        npc_heart_of_yshaarj_realmAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        InstanceScript* instance;

        void Reset(){}

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_heart_of_yshaarj_realmAI(creature);
    }
};

//72239
class npc_sha_vortex : public CreatureScript
{
public:
    npc_sha_vortex() : CreatureScript("npc_sha_vortex") {}

    struct npc_sha_vortexAI : public ScriptedAI
    {
        npc_sha_vortexAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_YSHAARJ_PROTECTION_AT, true);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_vortexAI(creature);
    }
};

//144798
class spell_exploding_iron_star : public SpellScriptLoader
{
public:
    spell_exploding_iron_star() : SpellScriptLoader("spell_exploding_iron_star") { }

    class spell_exploding_iron_star_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_exploding_iron_star_SpellScript);

        void DealDamage()
        {
            if (GetCaster() && GetHitUnit())
            {
                float distance = GetCaster()->GetExactDist2d(GetHitUnit());
                if (distance >= 0 && distance < 300)
                    SetHitDamage(GetHitDamage() * (1 - (distance / 300)));
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_exploding_iron_star_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_exploding_iron_star_SpellScript();
    }
};

//144989
class spell_whirling_corruption : public SpellScriptLoader
{
public:
    spell_whirling_corruption() : SpellScriptLoader("spell_whirling_corruption") { }

    class spell_whirling_corruption_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_whirling_corruption_SpellScript);

        void DealDamage()
        {
            if (GetCaster() && GetHitUnit())
            {
                float distance = GetCaster()->GetExactDist2d(GetHitUnit());
                if (distance >= 0 && distance < 300)
                    SetHitDamage(GetHitDamage() * (1 - (distance / 300)));
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_whirling_corruption_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_whirling_corruption_SpellScript();
    }
};

//145037
class spell_empovered_whirling_corruption : public SpellScriptLoader
{
public:
    spell_empovered_whirling_corruption() : SpellScriptLoader("spell_empovered_whirling_corruption") { }

    class spell_empovered_whirling_corruption_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_empovered_whirling_corruption_AuraScript);

        void OnPeriodic(AuraEffect const*aurEff)
        {
            if (GetCaster())
                GetCaster()->CastSpell(GetCaster(), SPELL_EM_WHIRLING_CORRUPTION_S, true);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_empovered_whirling_corruption_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_empovered_whirling_corruption_AuraScript();

    }
};

//144616
class spell_power_iron_star : public SpellScriptLoader
{
public:
    spell_power_iron_star() : SpellScriptLoader("spell_power_iron_star") { }

    class spell_power_iron_star_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_power_iron_star_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetCaster() && GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                if (Creature* ironstar = GetCaster()->FindNearestCreature(NPC_KORKRON_IRON_STAR, 30.0f, true))
                {
                    if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                        instance->SetData(DATA_FIRST_ENGENEER_DIED, 0);
                    if (GetCaster()->ToCreature())
                        GetCaster()->ToCreature()->DespawnOrUnsummon();
                    ironstar->AI()->DoAction(ACTION_LAUNCH);
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_power_iron_star_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_power_iron_star_AuraScript();
    }
};

//144867
class spell_enter_realm_of_yshaarj : public SpellScriptLoader
{
public:
    spell_enter_realm_of_yshaarj() : SpellScriptLoader("spell_enter_realm_of_yshaarj") { }

    class spell_enter_realm_of_yshaarj_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_enter_realm_of_yshaarj_AuraScript);

        void HandleEffectApply(AuraEffect const * aurEff, AuraEffectHandleModes mode)
        {
            if (InstanceScript* instance = GetTarget()->GetInstanceScript())
            {
                uint8 mod = instance->GetData(DATA_GET_REALM_OF_YSHAARJ);
                GetTarget()->NearTeleportTo(tppos[mod].GetPositionX(), tppos[mod].GetPositionY(), tppos[mod].GetPositionZ(), tppos[mod].GetOrientation());
                GetTarget()->AddAura(SPELL_REALM_OF_YSHAARJ, GetTarget());
                GetTarget()->CastSpell(GetTarget(), SPELL_GARROSH_ENERGY);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectRemoveFn(spell_enter_realm_of_yshaarj_AuraScript::HandleEffectApply, EFFECT_2, SPELL_AURA_SCREEN_EFFECT, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_enter_realm_of_yshaarj_AuraScript();
    }
};

//144954
class spell_realm_of_yshaarj : public SpellScriptLoader
{
public:
    spell_realm_of_yshaarj() : SpellScriptLoader("spell_realm_of_yshaarj") { }
    
    class spell_realm_of_yshaarj_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_realm_of_yshaarj_AuraScript);

        void HandleEffectRemove(AuraEffect const * aurEff, AuraEffectHandleModes mode)
        {
            if (GetTarget() && GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            {
                GetTarget()->NearTeleportTo(realmtppos.GetPositionX(), realmtppos.GetPositionY(), realmtppos.GetPositionZ(), realmtppos.GetOrientation());
                GetTarget()->RemoveAurasDueToSpell(SPELL_HOPE_BUFF);
                GetTarget()->RemoveAurasDueToSpell(SPELL_COURAGE);
                GetTarget()->RemoveAurasDueToSpell(SPELL_GARROSH_ENERGY);
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_realm_of_yshaarj_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_SCREEN_EFFECT, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_realm_of_yshaarj_AuraScript();

    }
};

//144852, 145222
class spell_transition_visual : public SpellScriptLoader
{
public:
    spell_transition_visual() : SpellScriptLoader("spell_transition_visual") { }

    class spell_transition_visual_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_transition_visual_AuraScript);

        void OnPeriodic(AuraEffect const*aurEff)
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                switch (GetSpellInfo()->Id)
                {
                case SPELL_TRANSITION_VISUAL_PHASE_2:
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_INTRO_REALM_OF_YSHAARJ);
                    break;
                case SPELL_TRANSITION_VISUAL_PHASE_3:
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_PHASE_THREE);
                    break;
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_transition_visual_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_transition_visual_AuraScript();

    }
};

class EmpoweringCorruptionFilter
{
public:
    bool operator()(WorldObject* unit) const
    {
        if (unit->ToCreature() && unit->GetEntry() == NPC_MINION_OF_YSHAARJ)
            return false;
        return true;
    }
};

//145043,  149536
class spell_empowering_corruption : public SpellScriptLoader
{
public:
    spell_empowering_corruption() : SpellScriptLoader("spell_empowering_corruption") { }

    class spell_empowering_corruption_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_empowering_corruption_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (!targets.empty())
                targets.remove_if(EmpoweringCorruptionFilter());
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_empowering_corruption_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_empowering_corruption_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_empowering_corruption_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_empowering_corruption_SpellScript::FilterTargets, EFFECT_3, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_empowering_corruption_SpellScript::FilterTargets, EFFECT_4, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_empowering_corruption_SpellScript();
    }
};

void AddSC_boss_garrosh_hellscream()
{
    new boss_garrosh_hellscream();
    new npc_garrosh_soldier();
    new npc_iron_star();
    new npc_desecrated_weapon();
    new npc_empowered_desecrated_weapon();
    new npc_sha_vortex();
    new npc_heart_of_yshaarj();
    new npc_heart_of_yshaarj_realm();
    new spell_exploding_iron_star();
    new spell_whirling_corruption();
    new spell_empovered_whirling_corruption();
    new spell_power_iron_star();
    new spell_enter_realm_of_yshaarj();
    new spell_realm_of_yshaarj();
    new spell_transition_visual();
    new spell_empowering_corruption();
}
