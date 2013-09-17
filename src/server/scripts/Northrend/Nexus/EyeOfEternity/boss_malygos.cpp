/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

/* Script Data Start
SDName: Boss malygos
Script Data End */

// TO-DOs:
// Implement a better pathing for Malygos.
// Find sniffed spawn position for chest
// Implement a better way to disappear the gameobjects
// Implement achievements
// Remove hack that re-adds targets to the aggro list after they enter to a vehicle when it works as expected
// Improve whatever can be improved :)

#include "ScriptPCH.h"
#include "eye_of_eternity.h"
#include "ScriptedEscortAI.h"

enum Achievements
{
    ACHIEV_TIMED_START_EVENT                      = 20387,
};

enum Events
{
    // Phase One
    EVENT_ARCANE_BREATH       = 1,
    EVENT_ARCANE_STORM        = 2,
    EVENT_VORTEX_START        = 3,
    EVENT_VORTEX_PROGRESS     = 4,
    EVENT_VORTEX_END          = 5,
    EVENT_POWER_SPARKS        = 6,
    // Phase Two
    EVENT_START_PHASE         = 7,
    EVENT_BREATH_TWO          = 8,
    EVENT_ARCANE_PULSE        = 9,
    EVENT_SURGE_POWER         = 10, 
    EVENT_SUMMON_ARCANE       = 11,
    // Phase Three
    EVENT_SURGE_POWER_PHASE_3 = 12,
    EVENT_STATIC_FIELD        = 13,
    EVENT_YELL_0              = 14,
    EVENT_YELL_1              = 15,
    EVENT_YELL_2              = 16,
    EVENT_YELL_3              = 17,
    EVENT_YELL_4              = 18,
    EVENT_BERSERK             = 19,
    EVENT_CALL_DRAKE          = 20,
    //Alexstrasza events
    EVENT_YELL_5              = 21,
    EVENT_YELL_6              = 22,
    EVENT_YELL_7              = 23,
    EVENT_YELL_8              = 24,
};

enum Phase
{
    PHASE_NULL  = 0,
    PHASE_ONE   = 1,
    PHASE_TWO   = 2, 
    PHASE_THREE = 3, 
};

enum Spells
{
    SPELL_ARCANE_BREATH   = 56272,
    SPELL_ARCANE_BREATH_2 = 60072,
    SPELL_ARCANE_STORM    = 57459,
    SPELL_BERSERK         = 47008,

    SPELL_VORTEX_1 = 56237, // seems that frezze object animation
    SPELL_VORTEX_2 = 55873, // visual effect
    SPELL_VORTEX_3 = 56105, // this spell must handle all the script - casted by the boss and to himself
    SPELL_VORTEX_6 = 73040, // teleport - (casted to all raid) | caster 30090 | target player

    SPELL_PORTAL_VISUAL_CLOSED = 55949,
    SPELL_SUMMON_POWER_PARK = 56142,
    SPELL_POWER_SPARK_DEATH = 55852,
    SPELL_POWER_SPARK_MALYGOS = 56152,

    SPELL_SURGE_POWER = 56505, // used in phase 2
    SPELL_SUMMON_ARCANE_BOMB = 56429,
    SPELL_ARCANE_OVERLOAD = 56432,
    SPELL_SUMMOM_RED_DRAGON = 56070,
    SPELL_SURGE_POWER_PHASE_3 = 57407,
    SPELL_STATIC_FIELD = 57430,

    //Add's Spells
    SPELL_HASTE = 57060,
    SPELL_ARCANE_BARRAGE = 49705,
};

#define SPELL_ARCANE_SHOCK   RAID_MODE(57058, 60073, 57058, 60073)

enum Lights
{
    LIGHT_GET_DEFAULT_FOR_MAP        = 0,
    LIGHT_ARCANE_RUNES               = 1824,
    LIGHT_OBSCURE_ARCANE_RUNES       = 1825,
};

enum MalygosSays
{
    SAY_AGGRO_P_ONE,
    SAY_KILLED_PLAYER_P_ONE,
    SAY_END_P_ONE,
    SAY_AGGRO_P_TWO,
    SAY_ANTI_MAGIC_SHELL, // not sure when execute it
    SAY_MAGIC_BLAST,  // not sure when execute it
    SAY_KILLED_PLAYER_P_TWO,
    SAY_END_P_TWO,
    SAY_INTRO_P_THREE,
    SAY_AGGRO_P_THREE,
    SAY_SURGE_POWER,  // not sure when execute it
    SAY_BUFF_SPARK,
    SAY_KILLED_PLAYER_P_THREE,
    SAY_SPELL_CASTING_P_THREE,
    SAY_DEATH
};

const Position sparkpos[4] =
{
    {718.8471f, 1265.5639f, 268.2312f},
    {720.0969f, 1337.5793f, 268.2313f},
    {789.6120f, 1338.2320f, 268.2312f},
    {790.6192f, 1264.8442f, 268.2312f},

};

const Position scionpos[4] = 
{
    {738.7973f, 1285.6856f, 268.2312f},
    {771.4146f, 1317.3347f, 268.2313f},
    {770.3053f, 1287.1090f, 268.2312f},
    {737.9934f, 1317.5926f, 268.2312f},

};

class boss_malygos : public CreatureScript
{
public:
    boss_malygos() : CreatureScript("boss_malygos") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_malygosAI(creature);
    }

    struct boss_malygosAI : public BossAI
    {
        boss_malygosAI(Creature* creature) : BossAI(creature, BOSS_MALYGOS), summons(me)
        {
            InstanceScript* pInstance = creature->GetInstanceScript();
            firstpull = false;
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), 295.00f, 10, 10);
            if (Is25ManRaid())
                scionval = 8;
            else
                scionval = 4;
        }

        InstanceScript* pInstance;

        SummonList summons;
        Phase phase;
        bool firstpull;
        bool phasetwo, phasethree;
        uint8 scioncount, scionval;

        void Reset()
        {
            if (!instance)
                return;

            if (GameObject* portal = me->FindNearestGameObject(193908, 100.0f))
                portal->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            _Reset();
            me->SetReactState(REACT_DEFENSIVE);
            events.SetPhase(PHASE_NULL);
            phase = PHASE_NULL;
            phasetwo = false;
            phasethree = false;
            scioncount = 0;
            instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
            if (!firstpull)
                me->GetMotionMaster()->MovePoint(0, 710.9990f, 1269.2036f, 295.00f);
            else
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }
        
        void SendLightOverride(uint32 overrideId, uint32 fadeInTime) const
        {
            WorldPacket data(SMSG_OVERRIDE_LIGHT, 12);
            data << uint32(1773);       // Light.dbc entry (map default)
            data << uint32(overrideId); // Light.dbc entry (override)
            data << uint32(fadeInTime);
            SendPacketToPlayers(&data);
        }
        
        void SendPacketToPlayers(WorldPacket const* data) const
        {
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (!players.isEmpty())
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (Player* player = itr->getSource())
                        if (player->GetAreaId() == 4500)
                            player->GetSession()->SendPacket(data);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch(id)
                {
                case 0:
                    if (Creature* portal = me->FindNearestCreature(30118, 100.0f, true))
                        me->SetFacingToObject(portal);
                    break;
                case 1:
                    me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->SetCanFly(false);
                    me->SetDisableGravity(false);
                    me->GetMotionMaster()->MoveFall();
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    if (phase == PHASE_ONE)
                    {
                        me->GetMotionMaster()->Clear(false);
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat();
                        if (me->getVictim())
                            me->GetMotionMaster()->MoveChase(me->getVictim());
                        events.ScheduleEvent(EVENT_VORTEX_START, 60000, 0, PHASE_ONE);
                    }
                    if (!firstpull)
                        firstpull = true;
                    break;
                case 2:
                    me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->SetCanFly(false);
                    me->SetDisableGravity(false);
                    me->SetFacingTo(2.3308f);
                    break;
                case 3:
                    if (Creature* trigger = me->FindNearestCreature(30334, 60.0f, true))
                        trigger->AddAura(SPELL_VORTEX_2, trigger); //Visual Vortex
                    break;
                case 4:
                    events.ScheduleEvent(EVENT_YELL_1, 26000, 0, PHASE_TWO);
                    events.ScheduleEvent(EVENT_START_PHASE, 10000, 0, PHASE_TWO);
                    break;
                case 5:
                    for (uint8 i = 0; i < RAID_MODE(2, 4); i++)
                    {
                        if (Creature* add = me->SummonCreature(30249, scionpos[rand()%4]))//Caster
                            add->SetInCombatWithZone();
                        if (Creature* add = me->SummonCreature(30245, scionpos[rand()%4]))//Melee
                            add->SetInCombatWithZone();
                    }
                    SendLightOverride(LIGHT_ARCANE_RUNES, 1000);
                    events.ScheduleEvent(EVENT_SURGE_POWER, urand(60000, 70000), 0, PHASE_TWO);
                    events.ScheduleEvent(EVENT_SUMMON_ARCANE, urand(2000, 5000), 0, PHASE_TWO);
                    me->SetFacingTo(4.0814f);
                    break;
                case 6:
                    SendLightOverride(LIGHT_OBSCURE_ARCANE_RUNES, 1000);
                    if (GameObject* platform = me->FindNearestGameObject(193070, 80.0f))
                        platform->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    events.ScheduleEvent(EVENT_CALL_DRAKE, 1500, 0, PHASE_THREE);
                    break;
                }
            }
        }
        
        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
        }
        
        void DoAction(const int32 action)
        {
            switch(action)
            {
            case ACTION_START_MALYGOS:
                me->GetMotionMaster()->MovePoint(1, 754.395f, 1302.03f, 275.17f);
                break;
            case ACTION_START_PHASE_TWO:
                summons.DespawnAll();
                events.SetPhase(PHASE_TWO);
                phase = PHASE_TWO;
                events.ScheduleEvent(EVENT_YELL_0, 1000, 0, PHASE_TWO);
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                me->SetCanFly(true);
                me->SetDisableGravity(true);
                me->GetMotionMaster()->MovePoint(4, 754.395f, 1302.03f, 290.17f);
                break;
            case ACTION_START_PHASE_THREE:
                events.SetPhase(PHASE_THREE);
                phase = PHASE_THREE;
                summons.DespawnAll(); //Despawn Disk
                events.ScheduleEvent(EVENT_YELL_2, 2000);
                events.ScheduleEvent(EVENT_YELL_3, 8000);
                events.ScheduleEvent(EVENT_YELL_4, 16000);
                break;
            case ACTION_DEATH_SCION:
                scioncount++;
                if (scioncount == scionval)
                    DoAction(ACTION_START_PHASE_THREE);
                break;
            }
        }

        void EnterEvadeMode()
        {
           if (phase == PHASE_THREE)
           {               
               if (GameObject* platform = me->FindNearestGameObject(193070, 80.0f))
               {
                   platform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                   platform->Respawn();
               }
           }
           summons.DespawnAll();
           ScriptedAI::EnterEvadeMode();
           SendLightOverride(LIGHT_GET_DEFAULT_FOR_MAP, 1000);
           me->GetMotionMaster()->MovePoint(2, 754.395f, 1302.03f, 266.56f);  
        }

        void EnterCombat(Unit* who)
        {
            if (!instance)
                return;

            if (GameObject* portal = me->FindNearestGameObject(193908, 100.0f))
                portal->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            _EnterCombat();
            Talk(SAY_AGGRO_P_ONE);
            phase = PHASE_ONE;
            events.SetPhase(PHASE_ONE);
            events.ScheduleEvent(EVENT_ARCANE_BREATH, urand(15000, 20000), 0, PHASE_ONE);
            events.ScheduleEvent(EVENT_BERSERK, 600000);
            //events.ScheduleEvent(EVENT_ARCANE_STORM, urand(5000, 10000));//Not found Visual spell...
            events.ScheduleEvent(EVENT_VORTEX_START, urand(30000, 40000), 0, PHASE_ONE);
            events.ScheduleEvent(EVENT_POWER_SPARKS, urand(30000, 35000), 0, PHASE_ONE);
            instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
        }

        void KilledUnit(Unit* who)
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            switch (phase)
            {
                case PHASE_ONE:
                    Talk(SAY_KILLED_PLAYER_P_ONE);
                    break;
                case PHASE_TWO:
                    Talk(SAY_KILLED_PLAYER_P_TWO);
                    break;
                case PHASE_THREE:
                    Talk(SAY_KILLED_PLAYER_P_THREE);
                    break;
            }
        }
        
        void JustDied(Unit* killer)
        {
            if (GameObject* portal = me->FindNearestGameObject(193908, 100.0f))
                portal->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            SendLightOverride(LIGHT_GET_DEFAULT_FOR_MAP, 1000);
            Talk(SAY_DEATH);
            _JustDied();
        }

        void UpdateAI(uint32 const diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (HealthBelowPct(50) && !phasetwo)
            {
                phasetwo = true;
                DoAction(ACTION_START_PHASE_TWO);
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_YELL_2:
                        Talk(SAY_END_P_TWO);
                        break;
                    case EVENT_YELL_3:
                        Talk(SAY_INTRO_P_THREE);
                        me->GetMotionMaster()->MovePoint(6, 754.395f, 1302.03f, 290.2312f);
                        break;
                    case EVENT_YELL_4:
                        Talk(SAY_AGGRO_P_THREE);
                        me->SetReactState(REACT_PASSIVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        DoZoneInCombat();
                        events.ScheduleEvent(EVENT_SURGE_POWER_PHASE_3, urand(7000, 10000), 0, PHASE_THREE);
                        events.ScheduleEvent(EVENT_STATIC_FIELD, urand(10000, 15000), 0, PHASE_THREE);
                        break;
                    case EVENT_YELL_0:
                        Talk(SAY_END_P_ONE);
                        break;
                    case EVENT_YELL_1:
                        Talk(SAY_AGGRO_P_TWO);
                        break;
                    case EVENT_ARCANE_BREATH:
                        DoCast(me->getVictim(), SPELL_ARCANE_BREATH);
                        events.ScheduleEvent(EVENT_ARCANE_BREATH, urand(35000, 60000), 0, PHASE_ONE);
                        break;
                    case EVENT_ARCANE_STORM: //Not found Visual spell...
                        DoCast(me->getVictim(), SPELL_ARCANE_STORM);
                        events.ScheduleEvent(EVENT_ARCANE_STORM, urand(5000, 10000), 0, PHASE_ONE);
                        break;
                    case EVENT_VORTEX_START:
                        me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        events.DelayEvents(17000);
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        me->SetCanFly(true);
                        me->SetDisableGravity(true);
                        me->GetMotionMaster()->MovePoint(3, 754.395f, 1302.03f, 285.17f);
                        events.ScheduleEvent(EVENT_VORTEX_PROGRESS, 3000, 0, PHASE_ONE);
                        events.ScheduleEvent(EVENT_VORTEX_END, 17000, 0, PHASE_ONE);
                        break;
                    case EVENT_VORTEX_PROGRESS:
                        {
                            events.CancelEvent(EVENT_VORTEX_PROGRESS);
                            Map* pMap = me->GetMap();
                            if (pMap && pMap->IsDungeon())
                            {
                                Map::PlayerList const &players = pMap->GetPlayers();
                                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                                {
                                    if (Player* pPlayer = i->getSource())
                                    {
                                        if (pPlayer->isAlive())
                                        {
                                            if (Creature* vortex = pPlayer->SummonCreature(30090, 754.733f, 1301.51f, 283.379f, 0, TEMPSUMMON_TIMED_DESPAWN, 15000))
                                            {
                                                pPlayer->CastSpell(vortex, 55853);
                                                pPlayer->EnterVehicle(vortex, 0);
                                            }
                                        }
                                    }
                                }
                            }
                        break;
                        }
                    case EVENT_VORTEX_END:
                        me->GetMotionMaster()->MovePoint(1, 754.395f, 1302.03f, 275.17f);
                        events.CancelEvent(EVENT_VORTEX_END);
                        break;
                    case EVENT_POWER_SPARKS:
                        me->SummonCreature(30084, sparkpos[rand()%4], TEMPSUMMON_MANUAL_DESPAWN);
                        events.ScheduleEvent(EVENT_POWER_SPARKS, urand(30000, 35000), 0, PHASE_ONE);
                        break;
                    case EVENT_SURGE_POWER:
                        DoCast(SPELL_SURGE_POWER);
                        events.ScheduleEvent(EVENT_SURGE_POWER, urand(60000, 70000), 0, PHASE_TWO);
                        break;
                    case EVENT_SUMMON_ARCANE:
                        DoCast(SPELL_SUMMON_ARCANE_BOMB);
                        events.ScheduleEvent(EVENT_SUMMON_ARCANE, urand(20000, 30000), 0, PHASE_TWO);
                        break;
                    case EVENT_SURGE_POWER_PHASE_3:
                        {
                            uint64 lasttarget = 0;

                            Map* pMap = me->GetMap();
                            if (pMap && pMap->IsDungeon())
                            {
                                Map::PlayerList const &players = pMap->GetPlayers();
                                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                                {
                                    if (Player* pPlayer = i->getSource())
                                    {
                                        if (pPlayer->isAlive() && pPlayer->GetVehicle())
                                        {
                                            if (Unit* target = pPlayer->GetVehicleBase())
                                            {
                                                DoCast(target, SPELL_SURGE_POWER_PHASE_3);
                                                lasttarget = target->GetGUID();
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (!lasttarget)
                                    EnterEvadeMode();
                            }
                        events.ScheduleEvent(EVENT_SURGE_POWER_PHASE_3, urand(10000, 20000), 0, PHASE_THREE);
                        break;
                        }
                    case EVENT_STATIC_FIELD:
                        {
                            Map* pMap = me->GetMap();
                            if (pMap && pMap->IsDungeon())
                            {
                                Map::PlayerList const &players = pMap->GetPlayers();
                                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                                {
                                    if (Player* pPlayer = i->getSource())
                                    {
                                        if (pPlayer->isAlive() && pPlayer->GetVehicle())
                                        {
                                            Position pos;
                                            pPlayer->GetPosition(&pos);
                                            me->SummonCreature(40005, pos, TEMPSUMMON_TIMED_DESPAWN, 30000);
                                            break;
                                        }
                                    }
                                }
                            }
                        events.ScheduleEvent(EVENT_STATIC_FIELD, urand(10000, 30000), 0, PHASE_THREE);
                        break;
                        }
                    case EVENT_START_PHASE:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        me->GetMotionMaster()->MovePoint(5, 782.9380f, 1337.2382f, 290.2312f);
                        break;
                    case EVENT_CALL_DRAKE:
                        {
                            events.CancelEvent(EVENT_CALL_DRAKE);
                            Map* pMap = me->GetMap();
                            if (pMap && pMap->IsDungeon())
                            {
                                Map::PlayerList const &players = pMap->GetPlayers();
                                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                                {
                                    if (Player* pPlayer = i->getSource())
                                    {
                                        if (pPlayer->isAlive())
                                        {
                                            if (Creature* drake = me->SummonCreature(30161, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ()- 5, 0, TEMPSUMMON_CORPSE_DESPAWN))
                                                pPlayer->EnterVehicle(drake, 0);
                                        }
                                    }
                                }
                            }
                        break;
                        }
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        events.CancelEvent(EVENT_BERSERK);
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

class npc_portal_eoe: public CreatureScript
{
public:
    npc_portal_eoe() : CreatureScript("npc_portal_eoe") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_portal_eoeAI(creature);
    }

    struct npc_portal_eoeAI : public ScriptedAI
    {
        npc_portal_eoeAI(Creature* creature) : ScriptedAI(creature){}
        
        uint32 bufftimer;
        
        void Reset()
        {
            if (me->GetEntry() == 40050)//for LK 40002
            {
                me->AddAura(SPELL_PORTAL_OPENED, me);
                bufftimer = 24000;
            }
            else
            {
                me->AddAura(SPELL_PORTAL_VISUAL_CLOSED, me);
                bufftimer = 0;
            }
        }

        void EnterEvadeMode(){}
        
        void UpdateAI(uint32 const diff)
        {
            if (bufftimer)
            {
                if (bufftimer <= diff)
                {
                    me->AddAura(SPELL_PORTAL_OPENED, me);
                    bufftimer = 24000;
                }
                else bufftimer -= diff;
            }
        }
       
    };
};

class npc_power_spark: public CreatureScript
{
public:
    npc_power_spark() : CreatureScript("npc_power_spark") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_power_sparkAI(creature);
    }

    struct npc_power_sparkAI : public ScriptedAI
    {
        npc_power_sparkAI(Creature* creature) : ScriptedAI(creature)
        {
            InstanceScript* pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            buff = false;
            me->AddAura(55845, me);
        }

        InstanceScript* pInstance;

        uint32 CheckDist;
        bool buff;

        void Reset()
        {
            if (Creature* malygos = me->FindNearestCreature(28859, 150.0f, true))
            {
                if (malygos->isInCombat())
                {
                    me->GetMotionMaster()->MoveFollow(malygos, 0.0f, 0.0f);
                    CheckDist = 1000;
                        
                }
            }
            else
                me->DespawnOrUnsummon();
        }

        void EnterEvadeMode(){}
        
        void UpdateAI(uint32 const diff)
        {
            if (CheckDist)
            {
                if (CheckDist <= diff)
                {
                    if (Creature* malygos = me->ToTempSummon()->GetSummoner()->ToCreature())
                    {
                        if (malygos && malygos->isAlive())
                        {
                            me->GetMotionMaster()->MoveFollow(malygos, 0.0f, 0.0f);
                            if (me->GetDistance(malygos) <= 1.0f && !buff)
                            {
                                buff = true;
                                CheckDist = 0;
                                DoCast(me, 52776); //Visual Despawn
                                malygos->AddAura(SPELL_POWER_SPARK_MALYGOS, malygos);
                                me->DespawnOrUnsummon(1000);

                            }
                        }
                    }
                }
                else CheckDist -= diff;
            }
        }
        
        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetMaxHealth() && !buff)
            {
                damage = 0;
                buff = true;
                CheckDist = 0;
                me->RemoveAllAuras();
                me->SetFullHealth();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveIdle();
                me->SetStandState(UNIT_STAND_STATE_DEAD);
                me->CastSpell(me, SPELL_POWER_SPARK_DEATH);
            }
        }
    };
};

class npc_scion_of_eternity  : public CreatureScript
{
public:
    npc_scion_of_eternity() : CreatureScript("npc_scion_of_eternity") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scion_of_eternityAI(creature);
    }

    struct npc_scion_of_eternityAI : public ScriptedAI
    {
        npc_scion_of_eternityAI(Creature* creature) : ScriptedAI(creature)
        {
            _instance = creature->GetInstanceScript();
        }

        uint32 BarrageTimer;
        uint32 ShockTimer;
        uint32 HasteTimer;

        void Reset()
        {
            BarrageTimer = 0;
            ShockTimer = 0;
            HasteTimer = 0;

            if (me->GetEntry() == 30249)
                BarrageTimer = 6000;
            else
            {
                ShockTimer = 6000;
                HasteTimer = 10000;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (HasteTimer)
            {
                if (HasteTimer <= diff)
                {
                    DoCast(me, SPELL_HASTE);
                    HasteTimer = 10000;
                }
                else HasteTimer -= diff;
            }

            if (ShockTimer)
            {
                if (ShockTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                    {
                        DoCast(target, SPELL_ARCANE_SHOCK);
                        ShockTimer = 8000; 
                    }
                }
                else ShockTimer -= diff;
            }

            if (BarrageTimer)
            {
                if (BarrageTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                    {
                        DoCast(target, SPELL_ARCANE_BARRAGE);
                        BarrageTimer = 8000;
                    }
                }
                else BarrageTimer -= diff;
            }
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* attacker)
        {
            Position pos;
            me->GetPosition(&pos);
            if (Creature* malygos = me->ToTempSummon()->GetSummoner()->ToCreature())
            {
                if (Creature* disk = malygos->SummonCreature(30234, pos, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    disk->GetMotionMaster()->MoveFall();
                    malygos->AI()->DoAction(ACTION_DEATH_SCION);
                }
            }
        }

    private:
        InstanceScript* _instance;
    };
};

// The reason of this AI is to make the creature able to enter in combat otherwise the spell casting of SPELL_ARCANE_OVERLOAD fails.
class npc_arcane_overload : public CreatureScript
{
public:
    npc_arcane_overload() : CreatureScript("npc_arcane_overload") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arcane_overloadAI (creature);
    }

    struct npc_arcane_overloadAI : public ScriptedAI
    {
        npc_arcane_overloadAI(Creature* creature) : ScriptedAI(creature) {}

        void AttackStart(Unit* who)
        {
            DoStartNoMovement(who);
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            DoCast(me, SPELL_ARCANE_OVERLOAD, false);
        }

        void UpdateAI(uint32 const /*diff*/)
        {
            // we dont do melee damage!
        }

    };
};

class npc_static_field : public CreatureScript
{
public:
    npc_static_field() : CreatureScript("npc_static_field") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_static_fieldAI (creature);
    }

    struct npc_static_fieldAI : public ScriptedAI
    {
        npc_static_fieldAI(Creature* creature) : ScriptedAI(creature){}
        
        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            DoCast(me, 57428);
        }
        
        void AttackStart(Unit* who)
        {
            DoStartNoMovement(who);
        }
        
        void EnterEvadeMode(){}
        
        void UpdateAI(uint32 const /*diff*/){}
    };
};

class ExactDistanceCheck
{
    public:
        ExactDistanceCheck(WorldObject* source, float dist) : _source(source), _dist(dist) {}

        bool operator()(WorldObject* unit)
        {
            return _source->GetExactDist2d(unit) > _dist;
        }

    private:
        WorldObject* _source;
        float _dist;
};

class spell_arcane_overload : public SpellScriptLoader
{
    public:
        spell_arcane_overload() : SpellScriptLoader("spell_arcane_overload") { }

        class spell_arcane_overload_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_arcane_overload_SpellScript);

            void ScaleRange(std::list<WorldObject*>& targets)
            {
                targets.remove_if(ExactDistanceCheck(GetCaster(), 12.0f - (GetCaster()->GetAura(56435)->GetStackAmount()/3.75)));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_arcane_overload_SpellScript::ScaleRange, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_arcane_overload_SpellScript();
        }
};

enum AlexstraszaYells
{
    SAY_ONE   = 0,
    SAY_TWO   = 1,
    SAY_THREE = 2,
    SAY_FOUR  = 3,
};

class npc_alexstrasza_eoe : public CreatureScript
{
public:
    npc_alexstrasza_eoe() : CreatureScript("npc_alexstrasza_eoe") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_alexstrasza_eoeAI (creature);
    }

    struct npc_alexstrasza_eoeAI : public ScriptedAI
    {
        npc_alexstrasza_eoeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
        }

        EventMap events;

        void Reset()
        {
            events.ScheduleEvent(EVENT_YELL_5, 1000);
        }

        void UpdateAI(uint32 const diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_YELL_5:
                        Talk(SAY_ONE);
                        events.ScheduleEvent(EVENT_YELL_6, 4000);
                        break;
                    case EVENT_YELL_6:
                        Talk(SAY_TWO);
                        events.ScheduleEvent(EVENT_YELL_7, 4000);
                        break;
                    case EVENT_YELL_7:
                        Talk(SAY_THREE);
                        events.ScheduleEvent(EVENT_YELL_8, 7000);
                        break;
                    case EVENT_YELL_8:
                        Talk(SAY_FOUR);
                        break;
                }
            }
        }
    };
};

class achievement_denyin_the_scion : public AchievementCriteriaScript
{
    public:
        achievement_denyin_the_scion() : AchievementCriteriaScript("achievement_denyin_the_scion") {}

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            if (Unit* disk = source->GetVehicleBase())
                if (disk->GetEntry() == NPC_HOVER_DISK_CASTER || disk->GetEntry() == NPC_HOVER_DISK_MELEE)
                    return true;

            return false;
        }
};

void AddSC_boss_malygos()
{
    new boss_malygos();
    new npc_portal_eoe();
    new npc_power_spark();
    new npc_scion_of_eternity();
    new npc_arcane_overload();
    new npc_static_field();
    new spell_arcane_overload();
    new npc_alexstrasza_eoe();
    new achievement_denyin_the_scion();
}
