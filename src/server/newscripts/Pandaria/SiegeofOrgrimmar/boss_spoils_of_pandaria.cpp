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
    //Big 
    SPELL_STRENGTH_OF_THE_STONE       = 145998,
    SPELL_SHADOW_VOLLEY               = 148515,
    SPELL_SHADOW_VOLLEY_D             = 148516,
    SPELL_MOLTEN_FIST                 = 148518,
    SPELL_MOLTEN_FIST_D               = 148517,
    SPELL_JADE_TEMPEST                = 148582,
    SPELL_JADE_TEMPEST_D              = 148583,
    SPELL_FRACTURE                    = 148513,
    SPELL_FRACTURE_D                  = 148514,
    SPELL_SET_TO_BLOW_DMG             = 145993,
    SPELL_SET_TO_BLOW_AURA            = 145987,
    SPELL_SET_TO_BLOW_AT              = 146365,
    SPELL_SET_TO_BLOW_VISUAL_1        = 142997,
    SPELL_SET_TO_BLOW_VISUAL_2        = 148054,
    SPELL_SET_TO_BLOW_VISUAL_3        = 148055,
    SPELL_SET_TO_BLOW_VISUAL_4        = 148056,
    SPELL_PHEROMONE_CLOUD             = 148762,
    SPELL_PHEROMONE_CLOUD_DMG         = 148760,

    //Medium
    SPELL_CRIMSON_RECOSTITUTION_AT    = 145270,
    SPELL_FORBIDDEN_MAGIC             = 145230,
    SPELL_MOGU_RUNE_OF_POWER_AT       = 145460,
    SPELL_RESIDUE                     = 145786,
    SPELL_RAGE_OF_THE_EMPRESS         = 145812,
    SPELL_RAGE_OF_THE_EMPRESS_AURA    = 145813,

    //Small
    SPELL_EARTHEN_SHARD               = 144923,
    SPELL_HARDEN_FLESH_DMG            = 145218,   
    SPELL_RUSH                        = 144904,
    SPELL_KW_ENRAGE                   = 145692,
    SPELL_MANTID_SWARM                = 145806,
    SPELL_MANTID_SWARM2               = 145807,
    SPELL_MANTID_SWARM3               = 145808,
    SPELL_THROW_EXPLOSIVES            = 145702,

    //Pandaren Relic Box
    SPELL_EMINENCE                    = 146189,
    SPELL_GUSTING_CRANE_KICK          = 146180,
    SPELL_GUSTING_CRANE_KICK_DMG      = 146182,
    SPELL_KEG_TOSS                    = 146214,
    SPELL_KEG_TOSS_DMG                = 146217,
    SPELL_BREATH_OF_FIRE              = 146222,
    SPELL_BREATH_OF_FIRE_DMG          = 146226,
    SPELL_BREATH_OF_FIRE_TR           = 146235,
    SPELL_PATH_OF_BLOSSOMS_AT         = 146255,
    SPELL_PATH_OF_BLOSSOMS_J          = 146256,
    SPELL_PATH_OF_BLOSSOMS_DMG        = 146257,
    SPELL_MASS_PARALYSIS              = 146289,

    //Special
    SPELL_GHOST_VISUAL                = 118205,
    SPELL_LIFT_HOOK_VISUAL            = 142721,
    SPELL_AURA_BAR                    = 144921,
    SPELL_AURA_BAR_S                  = 148505,
    SPELL_SOOPS_AT_VISUAL             = 145687,
    SPELL_BLADE_OF_THE_HUNDRED_STEPS  = 146068, //tank 
    SPELL_STAFF_OF_RESONATING_WATER   = 146099, //healer
    SPELL_CLAW_OF_BURNING_ANGER       = 146141, //dd
};

enum Events
{
    //Lift hook
    EVENT_START_LIFT                  = 1,
    EVENT_POINT                       = 2,
    EVENT_BACK                        = 3,
    EVENT_RESET                       = 4,

    //Summons
    //Big mogu
    EVENT_SPAWN                       = 5,
    EVENT_EARTHEN_SHARD               = 6, 
    EVENT_SHADOW_VOLLEY               = 7,
    EVENT_MOLTEN_FIST                 = 8,
    EVENT_JADE_TEMPEST                = 9,
    EVENT_FRACTURE                    = 10,
    EVENT_HARDEN_FLESH                = 11,
    EVENT_FIND_PLAYERS                = 12,
    EVENT_RUSH                        = 13,
    EVENT_KW_ENRAGE                   = 14,
    EVENT_IN_PROGRESS                 = 15,
    EVENT_KEG_TOSS                    = 16,
    EVENT_BREATH_OF_FIRE              = 17,
    EVENT_PATH_OF_BLOSSOMS            = 18,
    EVENT_PATH_OF_BLOSSOMS_PROGRESS   = 19,
    EVENT_MANTID_SWARM                = 20,
    EVENT_THROW_EXPLOSIVES            = 21,
    EVENT_FORBIDDEN_MAGIC             = 22,
    EVENT_MOGU_RUNE_OF_POWER          = 23,
    EVENT_GUSTING_CRANE_KICK          = 24,
    EVENT_CRIMSON_RECOSTITUTION       = 25,
    EVENT_SET_TO_BLOW                 = 26,
    EVENT_RESIDUE                     = 27,
    EVENT_RAGE_OF_THE_EMPRESS         = 28,
};

Position dpos[4] =
{
    { 1606.77f, -5124.41f, -263.3986f, 0.0f },
    { 1657.45f, -5127.57f, -263.3986f, 0.0f },
    { 1639.86f, -5101.63f, -263.3986f, 0.0f },
    { 1622.80f, -5148.56f, -263.3986f, 0.0f },
};

//Big
uint32 bigmoguentry[4] =
{
    NPC_JUN_WEI,
    NPC_ZU_YIN,
    NPC_XIANG_LIN,
    NPC_KUN_DA,
};

uint32 bigmantisentry[4] =
{
    NPC_COMMANDER_ZAKTAR,
    NPC_COMMANDER_IKTAL,
    NPC_COMMANDER_NAKAZ,
    NPC_COMMANDER_TIK,
};
//

//Mediom
uint32 mediummoguentry[2] =
{
    NPC_MODIFIED_ANIMA_GOLEM,
    NPC_MOGU_SHADOW_RITUALIST,
};

uint32 mediummantisentry[2] =
{
    NPC_ZARTHIK_AMBER_PRIEST,
    NPC_SETTHIK_WIND_WIELDER,
};
//

//Small
uint32 smallmoguentry[2] = 
{
    NPC_ANIMATED_STONE_MOGU,
    //NPC_BURIAL_URN, not found visual id and not work spells
    NPC_QUILEN_GUARDIANS,
};

uint32 smallmantisentry[3] =
{
    NPC_SRITHIK_BOMBARDIER,
    NPC_AMBER_ENCASED_KUNCHONG, //not found visual spell for Encapsulated Pheromones
    NPC_KORTHIK_WARCALLER,
};
//

uint32 mantidswarm[3] =
{
    SPELL_MANTID_SWARM,
    SPELL_MANTID_SWARM2,
    SPELL_MANTID_SWARM3,
};

//Pandaren Relic box
uint32 pandarenrelicentry[3] = 
{
    NPC_NAMELESS_WINDWALKER_SPIRIT,
    NPC_WISE_MISTWEAVER_SPIRIT,
    NPC_ANCIENT_BREWMASTER_SPIRIT,
};

enum sActions
{
    ACTION_SEND_AURA_BAR         = 1,
    ACTION_SECOND_ROOM           = 2,
};

enum sData
{
    DATA_BUFF_HEALER             = 1,
    DATA_BUFF_DD                 = 2,
    DATA_BUFF_TANK               = 3,
    DATA_UPDATE_POWER            = 4,
};

uint32 spellbuff[3] =
{
    SPELL_STAFF_OF_RESONATING_WATER,
    SPELL_CLAW_OF_BURNING_ANGER,
    SPELL_BLADE_OF_THE_HUNDRED_STEPS,   
};

uint32 spoilsentry[4] =
{
    NPC_MOGU_SPOILS,
    NPC_MOGU_SPOILS2,
    NPC_MANTIS_SPOILS,
    NPC_MANTIS_SPOILS2,
};

#define GOSSIP1 "We are ready"

//71889
class npc_ssop_spoils : public CreatureScript
{
public:
    npc_ssop_spoils() : CreatureScript("npc_ssop_spoils") {}

    struct npc_ssop_spoilsAI : public ScriptedAI
    {
        npc_ssop_spoilsAI(Creature* creature) : ScriptedAI(creature), _summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            lastcount = 270;
            newcount = 0;
        }
        
        InstanceScript* instance;
        SummonList _summons;
        EventMap events;
        uint32 lastcount;
        uint32 newcount;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void JustSummoned(Creature* sum)
        {
            _summons.Summon(sum);
        }
        
        void DoAction(int32 const action)
        {
            if (instance)
            {
                switch (action)
                {
                case ACTION_SSOPS_IN_PROGRESS:
                    DoCast(me, SPELL_SOOPS_AT_VISUAL, true);
                    events.ScheduleEvent(EVENT_IN_PROGRESS, 1000);
                    break;
                case ACTION_SSOPS_SECOND_ROOM:
                    events.Reset();
                    lastcount = 300;
                    events.ScheduleEvent(EVENT_IN_PROGRESS, 1000);
                    break;
                case ACTION_SSOPS_DONE:
                    events.Reset();
                    _summons.DespawnAll();
                    std::list<AreaTrigger*> atlist;
                    atlist.clear();
                    me->GetAreaTriggersWithEntryInRange(atlist, 5269, me->GetGUID(), 50.0f);
                    if (!atlist.empty())
                        for (std::list<AreaTrigger*>::const_iterator itr = atlist.begin(); itr != atlist.end(); itr++)
                            (*itr)->RemoveFromWorld();
                    OfflineWorldState();
                    break;
                }
            }
        }

        void OfflineWorldState()
        {
            Map::PlayerList const &players = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                if (Player* pl = i->getSource())
                    if (pl->isAlive())
                        pl->SendUpdateWorldState(8431, 0);
        }

        void SendWipe()
        {
            if (instance)
            {
                events.Reset();
                Map::PlayerList const &players = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                    if (Player* pl = i->getSource())
                        if (pl->isAlive())
                            pl->Kill(pl, true);
                
                std::list<AreaTrigger*> atlist;
                atlist.clear();
                me->GetAreaTriggersWithEntryInRange(atlist, 5269, me->GetGUID(), 50.0f);
                if (!atlist.empty())
                    for (std::list<AreaTrigger*>::const_iterator itr = atlist.begin(); itr != atlist.end(); itr++)
                        (*itr)->RemoveFromWorld();
                OfflineWorldState();
                _summons.DespawnAll();
                instance->SetBossState(DATA_SPOILS_OF_PANDARIA, NOT_STARTED);
                lastcount = 270;
                newcount = 0;
            }
        }
        
        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_IN_PROGRESS && instance)
                {
                    if (instance->IsWipe())
                    {
                        events.Reset();
                        std::list<AreaTrigger*> atlist;
                        atlist.clear();
                        me->GetAreaTriggersWithEntryInRange(atlist, 5269, me->GetGUID(), 50.0f);
                        if (!atlist.empty())
                            for (std::list<AreaTrigger*>::const_iterator itr = atlist.begin(); itr != atlist.end(); itr++)
                                (*itr)->RemoveFromWorld();
                        OfflineWorldState();
                        _summons.DespawnAll();
                        lastcount = 270;
                        newcount = 0;
                        instance->SetBossState(DATA_SPOILS_OF_PANDARIA, NOT_STARTED);
                    }
                    else
                    {
                        newcount = lastcount ? lastcount - 1 : 0;
                        Map::PlayerList const &players = me->GetMap()->GetPlayers();
                        for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                        {
                            if (Player* pl = i->getSource())
                            {
                                pl->SendUpdateWorldState(8431, 1);
                                pl->SendUpdateWorldState(8381, newcount);
                            }
                        }
                        lastcount = newcount;

                        if (!newcount)
                            SendWipe();
                        else
                            events.ScheduleEvent(EVENT_IN_PROGRESS, 1000);
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ssop_spoilsAI(creature);
    }
};

//73720, 73722, 71512, 73721
class npc_generic_spoil : public CreatureScript
{
public:
    npc_generic_spoil() : CreatureScript("npc_generic_spoil") {}

    struct npc_generic_spoilAI : public ScriptedAI
    {
        npc_generic_spoilAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;
        SummonList summons;
        EventMap events;
        uint64 mspoilGuid;      //main spoil 
        uint64 othermspoilguid; //other main spoil 
        uint8 power;

        void Reset()
        {
            power = 0;
            mspoilGuid = 0;
        }

        void EnterCombat(Unit* who){}

        void JustSummoned(Creature* sum)
        {
            summons.Summon(sum);
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
            case DATA_BUFF_DD:
            case DATA_BUFF_TANK:
            case DATA_BUFF_HEALER:
            {
                std::list<Player*> pllist;
                pllist.clear();
                GetPlayerListInGrid(pllist, me, 50.0f);
                if (!pllist.empty())
                {
                    for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                        if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) == type)
                            (*itr)->CastSpell(*itr, spellbuff[type-1], true);
                }
                break;
            }
            case DATA_UPDATE_POWER:
                power = power + data > 50 ? 50 : power + data;
                break;
            default:
                break;
            }
        }

        void ActivateOrOfflineBoxes(bool state)
        {
            std::list<GameObject*> boxlist;
            boxlist.clear();
            switch (me->GetEntry())
            {
            case NPC_MOGU_SPOILS:
            case NPC_MOGU_SPOILS2:
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_SMALL_MOGU_BOX, 50.0f);
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_MEDIUM_MOGU_BOX, 50.0f);
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_BIG_MOGU_BOX, 50.0f);
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_PANDAREN_RELIC_BOX, 50.0f);
                if (!boxlist.empty())
                {
                    for (std::list<GameObject*>::const_iterator itr = boxlist.begin(); itr != boxlist.end(); itr++)
                    {
                        if (state)
                            (*itr)->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else
                            (*itr)->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    }

                }
                break;
            case NPC_MANTIS_SPOILS:
            case NPC_MANTIS_SPOILS2:
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_SMALL_MANTIS_BOX, 50.0f);
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_MEDIUM_MANTIS_BOX, 50.0f);
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_BIG_MANTIS_BOX, 50.0f);
                GetGameObjectListWithEntryInGrid(boxlist, me, GO_PANDAREN_RELIC_BOX, 50.0f);
                if (!boxlist.empty())
                {
                    for (std::list<GameObject*>::const_iterator itr = boxlist.begin(); itr != boxlist.end(); itr++)
                    {
                        if (state)
                            (*itr)->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else
                            (*itr)->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    }
                }
                break;
            }
        }

        void SetGUID(uint64 guid, int32 type)
        {
            if (type == 1)
            {
                mspoilGuid = guid; 
                switch (me->GetEntry())
                {
                case NPC_MOGU_SPOILS:
                case NPC_MOGU_SPOILS2:
                    othermspoilguid = instance->GetData64(DATA_SPOIL_MANTIS);
                    break;
                case NPC_MANTIS_SPOILS:
                case NPC_MANTIS_SPOILS2:
                    othermspoilguid = instance->GetData64(DATA_SPOIL_MOGU);
                    break;
                }
                ActivateOrOfflineBoxes(true);
                events.ScheduleEvent(EVENT_FIND_PLAYERS, 1000);
            }
        }
        
        void DoAction(int32 const action)
        {
            if (instance)
            {
                switch (action)
                {
                case ACTION_RESET:
                    events.Reset();
                    summons.DespawnAll();
                    power = 0;
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    me->RemoveAurasDueToSpell(SPELL_AURA_BAR_S);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_AURA_BAR);
                    break;
                case ACTION_IN_PROGRESS:
                    if (uint32(me->GetPositionZ()) == -271)
                        me->CastSpell(me, SPELL_AURA_BAR_S, true);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    if (Creature* spoiltrigger = me->GetCreature(*me, instance->GetData64(me->GetEntry())))
                        spoiltrigger->AI()->SetGUID(me->GetGUID(), 1);
                    break;
                case ACTION_SECOND_ROOM:
                    ActivateOrOfflineBoxes(false);
                    if (GameObject* go = me->GetMap()->GetGameObject(instance->GetData64(me->GetEntry() == NPC_MOGU_SPOILS2 ? GO_LEVER_R : GO_LEVER_L)))
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_FIND_PLAYERS:
                    if (!instance)
                        return;

                    std::list<Player*> pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, me, 55.0f);
                    if (!pllist.empty())
                    {
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                            if (uint32((*itr)->GetPositionZ()) <= -280 && me->GetDistance(*itr) <= 55.0f)
                                if (!(*itr)->HasAura(SPELL_AURA_BAR))
                                    (*itr)->CastCustomSpell(SPELL_AURA_BAR, SPELLVALUE_BASE_POINT0, power, *itr, true);
                                else
                                    (*itr)->SetPower(POWER_ALTERNATE_POWER, power, true);

                        //Send new power in frame
                        if (Creature* mspoil = me->GetCreature(*me, mspoilGuid))
                            if (mspoil->HasAura(SPELL_AURA_BAR_S))
                                mspoil->SetPower(POWER_ALTERNATE_POWER, power, true);
                    }
                    //Check spoils, if power 50, go second room
                    if (power == 50)
                    {
                        if (Creature* omspoil = me->GetCreature(*me, othermspoilguid))
                        {
                            if (omspoil->HasAura(SPELL_AURA_BAR_S) && omspoil->GetPower(POWER_ALTERNATE_POWER) == 50)
                            {
                                if (instance->GetBossState(DATA_SPOILS_OF_PANDARIA) == SPECIAL)
                                {
                                    instance->SetBossState(DATA_SPOILS_OF_PANDARIA, DONE);
                                    break;
                                }
                                else
                                {
                                    DoAction(ACTION_SECOND_ROOM);
                                    break;
                                }
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_FIND_PLAYERS, 1000);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_generic_spoilAI(creature);
    }
};

//72281
class npc_lever : public CreatureScript
{
public:
    npc_lever() : CreatureScript("npc_lever") {}

    struct npc_leverAI : public ScriptedAI
    {
        npc_leverAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_leverAI(creature);
    }
};

//72972
class npc_lift_hook : public CreatureScript
{
public:
    npc_lift_hook() : CreatureScript("npc_lift_hook") {}

    struct npc_lift_hookAI : public ScriptedAI
    {
        npc_lift_hookAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(SPELL_LIFT_HOOK_VISUAL, me);
            count = 4;
        }

        InstanceScript* instance;
        EventMap events;
        uint8 count;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void OnSpellClick(Unit* clicker)
        {
            if (int32(me->GetPositionX()) == 1598)
                count = 0;
            else if (int32(me->GetPositionX()) == 1665)
                count = 1;
            else if (int32(me->GetPositionX()) == 1652)
                count = 2;
            else if (int32(me->GetPositionX()) == 1613)
                count = 3;

            if (count > 3)
                return;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            clicker->EnterVehicle(me, 0);
            events.ScheduleEvent(EVENT_START_LIFT, 1000);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch (pointId)
                {
                case 0:
                    if (Vehicle* base = me->GetVehicleKit())
                        if (Unit* passenger = base->GetPassenger(0))
                            passenger->RemoveAurasDueToSpell(SPELL_AURA_BAR);
                    events.ScheduleEvent(EVENT_POINT, 500);
                    break;
                case 1:
                    if (Vehicle* base = me->GetVehicleKit())
                        if (Unit* passenger = base->GetPassenger(0))
                            passenger->ExitVehicle();
                    events.ScheduleEvent(EVENT_BACK, 1000);
                    break;
                case 2:
                    events.ScheduleEvent(EVENT_RESET, 9000);
                    break;
                case 3:
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_START_LIFT:
                    me->GetMotionMaster()->MoveCharge(me->GetPositionX(), me->GetPositionY(), -263.3986f, 20.0f, 0);
                    break;
                case EVENT_POINT:      
                    me->GetMotionMaster()->MoveCharge(dpos[count].GetPositionX(), dpos[count].GetPositionY(), -263.3986f, 20.0f, 1);
                    break;
                case EVENT_BACK:
                {
                    Position pos = me->GetHomePosition();
                    me->GetMotionMaster()->MoveCharge(pos.GetPositionX(), pos.GetPositionY(), -263.3986f, 20.0f, 2);
                    break;
                }
                case EVENT_RESET:
                {
                    Position pos = me->GetHomePosition();
                    me->GetMotionMaster()->MoveCharge(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 20.0f, 3);
                }
                break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lift_hookAI(creature);
    }
};

class npc_generic_sop_units : public CreatureScript
{
public:
    npc_generic_sop_units() : CreatureScript("npc_generic_sop_units") {}

    struct npc_generic_sop_unitsAI : public ScriptedAI
    {
        npc_generic_sop_unitsAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            spawn = 0;
            switch (me->GetEntry())
            {
            case NPC_NAMELESS_WINDWALKER_SPIRIT:
            case NPC_WISE_MISTWEAVER_SPIRIT:
            case NPC_ANCIENT_BREWMASTER_SPIRIT:
                DoCast(me, SPELL_GHOST_VISUAL, true);
                break;
            default:
                break;
            }
        }

        InstanceScript* instance;
        EventMap events;
        uint32 spawn;

        void Reset()
        {
            switch (me->GetEntry())
            {
            case NPC_JUN_WEI:
            case NPC_ZU_YIN:
            case NPC_XIANG_LIN:
            case NPC_KUN_DA:
                DoCast(me, SPELL_STRENGTH_OF_THE_STONE, true);
                break;
            default:
                break;
            }
            spawn = 1500;
        }

        void EnterCombat(Unit* who)
        {
            switch (me->GetEntry())
            {
            //Big 
            case NPC_JUN_WEI:
                events.ScheduleEvent(EVENT_SHADOW_VOLLEY, 8000);
                break;
            case NPC_ZU_YIN:
                events.ScheduleEvent(EVENT_MOLTEN_FIST, 8000);
                break;
            case NPC_XIANG_LIN:
                events.ScheduleEvent(EVENT_JADE_TEMPEST, 8000);
                break;
            case NPC_KUN_DA:
                events.ScheduleEvent(EVENT_FRACTURE, 8000);
                break;
            case NPC_COMMANDER_ZAKTAR:
            case NPC_COMMANDER_IKTAL:
            case NPC_COMMANDER_NAKAZ:
            case NPC_COMMANDER_TIK:
                DoCast(me, SPELL_PHEROMONE_CLOUD, true);
                events.ScheduleEvent(EVENT_SET_TO_BLOW, 5000);
                break;
            //Medoum
            case NPC_MOGU_SHADOW_RITUALIST:
                events.ScheduleEvent(EVENT_FORBIDDEN_MAGIC, 10000);
                events.ScheduleEvent(EVENT_MOGU_RUNE_OF_POWER, 5000);
                break;
            case NPC_MODIFIED_ANIMA_GOLEM:
                events.ScheduleEvent(EVENT_CRIMSON_RECOSTITUTION, 8000);
                break;
            case NPC_ZARTHIK_AMBER_PRIEST:
                events.ScheduleEvent(EVENT_RESIDUE, 1000);
                events.ScheduleEvent(EVENT_MANTID_SWARM, 5000);
                break;
            case NPC_SETTHIK_WIND_WIELDER:
                events.ScheduleEvent(EVENT_RAGE_OF_THE_EMPRESS, 1000);
                break;
            //Small 
            case NPC_QUILEN_GUARDIANS:
                events.ScheduleEvent(EVENT_RUSH, 1000);
                break;
            case NPC_ANIMATED_STONE_MOGU:
                events.ScheduleEvent(EVENT_EARTHEN_SHARD, 5000);
                events.ScheduleEvent(EVENT_HARDEN_FLESH, 10000);
                break;
            case NPC_KORTHIK_WARCALLER:
                events.ScheduleEvent(EVENT_KW_ENRAGE, 4000);
                break;
            case NPC_SRITHIK_BOMBARDIER:
                events.ScheduleEvent(EVENT_THROW_EXPLOSIVES, 8000);
                break;
            //Pandaren Relic box
            case NPC_NAMELESS_WINDWALKER_SPIRIT:
                events.ScheduleEvent(EVENT_PATH_OF_BLOSSOMS, 8000);
                break;
            case NPC_ANCIENT_BREWMASTER_SPIRIT:
                events.ScheduleEvent(EVENT_KEG_TOSS, 5000);
                events.ScheduleEvent(EVENT_BREATH_OF_FIRE, 10000);
                break;
            case NPC_WISE_MISTWEAVER_SPIRIT:
                DoCast(me, SPELL_EMINENCE, true);
                events.ScheduleEvent(EVENT_GUSTING_CRANE_KICK, 10000);
                break;
            default:
                break;
            }
        }

        void JustSummoned(Creature* summon)
        {
            if (summon->GetEntry() == NPC_ZARTHIK_SWARMER)
            {
                if (me->ToTempSummon())
                    if (Unit* summoner = me->ToTempSummon()->GetSummoner())
                        summoner->ToCreature()->AI()->JustSummoned(summon);
                summon->setFaction(16);
                summon->SetReactState(REACT_AGGRESSIVE);
                summon->AI()->DoZoneInCombat(summon, 50.0f);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (spawn)
            {
                if (spawn <= diff)
                {
                    spawn = 0;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 60.0f);
                }
                else 
                    spawn -= diff;
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                //Big                      
                case EVENT_MOLTEN_FIST:
                    DoCast(me, SPELL_MOLTEN_FIST);
                    events.ScheduleEvent(EVENT_MOLTEN_FIST, 15000);
                    break;
                case EVENT_SHADOW_VOLLEY:
                    DoCast(me, SPELL_SHADOW_VOLLEY);
                    events.ScheduleEvent(EVENT_SHADOW_VOLLEY, 15000);
                    break;
                case EVENT_JADE_TEMPEST:
                    DoCast(me, SPELL_JADE_TEMPEST);
                    events.ScheduleEvent(EVENT_JADE_TEMPEST, 15000);
                    break;
                case EVENT_FRACTURE:
                    DoCast(me, SPELL_FRACTURE);
                    events.ScheduleEvent(EVENT_FRACTURE, 15000);
                    break;
                case EVENT_SET_TO_BLOW:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_SET_TO_BLOW_AURA, true);
                    events.ScheduleEvent(EVENT_SET_TO_BLOW, 12000);
                    break;
                //Medium
                case EVENT_CRIMSON_RECOSTITUTION:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_CRIMSON_RECOSTITUTION_AT);
                    events.ScheduleEvent(EVENT_CRIMSON_RECOSTITUTION, 15000);
                    break;
                case EVENT_MOGU_RUNE_OF_POWER:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_MOGU_RUNE_OF_POWER_AT);
                    events.ScheduleEvent(EVENT_MOGU_RUNE_OF_POWER, 15000);
                    break;
                case EVENT_FORBIDDEN_MAGIC:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 10.0f, true))
                        DoCast(target, SPELL_FORBIDDEN_MAGIC);
                    events.ScheduleEvent(EVENT_FORBIDDEN_MAGIC, 15000);
                    break;
                case EVENT_RESIDUE:
                    DoCast(me, SPELL_RESIDUE, false);
                    events.ScheduleEvent(EVENT_RESIDUE, 15000);
                    break;
                case EVENT_RAGE_OF_THE_EMPRESS:
                    DoCast(me, SPELL_RAGE_OF_THE_EMPRESS, false);
                    events.ScheduleEvent(EVENT_RAGE_OF_THE_EMPRESS, 15000);
                    break;
                //Small
                case EVENT_KW_ENRAGE:
                    DoCast(me, SPELL_KW_ENRAGE, true);
                    events.ScheduleEvent(EVENT_KW_ENRAGE, 20000);
                    break;
                case EVENT_EARTHEN_SHARD:
                    DoCastVictim(SPELL_EARTHEN_SHARD);
                    events.ScheduleEvent(EVENT_EARTHEN_SHARD, 10000);
                    break;
                case EVENT_HARDEN_FLESH:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_HARDEN_FLESH_DMG);
                    events.ScheduleEvent(EVENT_HARDEN_FLESH, 15000);
                    break;
                case EVENT_RUSH:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60, true))
                        DoCast(target, SPELL_RUSH, true);
                    events.ScheduleEvent(EVENT_RUSH, 10000);
                    break;
                case EVENT_MANTID_SWARM:
                    for (uint8 n = 0; n < 3; n++)
                        DoCast(me, mantidswarm[n]);
                    events.ScheduleEvent(EVENT_MANTID_SWARM, 20000);
                    break;
                case EVENT_THROW_EXPLOSIVES:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_THROW_EXPLOSIVES);
                    events.ScheduleEvent(EVENT_THROW_EXPLOSIVES, 15000);
                    break;
                //Pandaren Relic box 
                case EVENT_GUSTING_CRANE_KICK:
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    DoCast(me, SPELL_GUSTING_CRANE_KICK);
                    events.ScheduleEvent(EVENT_GUSTING_CRANE_KICK, 15000); 
                    break;
                case EVENT_KEG_TOSS:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_KEG_TOSS);
                    events.ScheduleEvent(EVENT_KEG_TOSS, 10000);
                    break;
                case EVENT_BREATH_OF_FIRE:
                    if (me->getVictim())
                        DoCastVictim(SPELL_BREATH_OF_FIRE);
                    events.ScheduleEvent(EVENT_BREATH_OF_FIRE, 15000);
                    break;
                case EVENT_PATH_OF_BLOSSOMS:
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_PATH_OF_BLOSSOMS_J);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            if (me->ToTempSummon())
            {
                uint8 data = 0;
                switch (me->GetEntry())
                {
                //Big box
                case NPC_JUN_WEI:
                case NPC_ZU_YIN:
                case NPC_XIANG_LIN:
                case NPC_KUN_DA:
                case NPC_COMMANDER_ZAKTAR:
                case NPC_COMMANDER_IKTAL:
                case NPC_COMMANDER_NAKAZ:
                case NPC_COMMANDER_TIK:
                    data = 14;
                    break;
                //Medium box
                case NPC_MODIFIED_ANIMA_GOLEM:
                case NPC_MOGU_SHADOW_RITUALIST:
                case NPC_ZARTHIK_AMBER_PRIEST:
                case NPC_SETTHIK_WIND_WIELDER:
                    data = 4;
                    break;
                //Small box
                case NPC_ANIMATED_STONE_MOGU:
                case NPC_BURIAL_URN:
                case NPC_QUILEN_GUARDIANS:
                case NPC_SRITHIK_BOMBARDIER:
                case NPC_AMBER_ENCASED_KUNCHONG:
                case NPC_KORTHIK_WARCALLER:
                    data = 1;
                    break;
                //Pandaren Relic box
                case NPC_NAMELESS_WINDWALKER_SPIRIT:
                    for (uint8 n = 0; n < 4; n++)
                    {
                        if (Creature* nearspoils = me->FindNearestCreature(spoilsentry[n], 50.0f, true))
                        {
                            nearspoils->ToCreature()->AI()->SetData(DATA_BUFF_DD, 0);
                            break;
                        }
                    }
                    break;
                case NPC_WISE_MISTWEAVER_SPIRIT:
                    for (uint8 n = 0; n < 4; n++)
                    {
                        if (Creature* nearspoils = me->FindNearestCreature(spoilsentry[n], 50.0f, true))
                        {
                            nearspoils->ToCreature()->AI()->SetData(DATA_BUFF_HEALER, 0);
                            break;
                        }
                    }
                    break;
                case NPC_ANCIENT_BREWMASTER_SPIRIT:
                    for (uint8 n = 0; n < 4; n++)
                    {
                        if (Creature* nearspoils = me->FindNearestCreature(spoilsentry[n], 50.0f, true))
                        {
                            nearspoils->ToCreature()->AI()->SetData(DATA_BUFF_TANK, 0);
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
                if (data)
                    if (Unit* summoner = me->ToTempSummon()->GetSummoner())
                        summoner->ToCreature()->AI()->SetData(DATA_UPDATE_POWER, data);
            }
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == EFFECT_MOTION_TYPE)
            {
                if (pointId == SPELL_PATH_OF_BLOSSOMS_J)
                {
                    DoCast(me, SPELL_PATH_OF_BLOSSOMS_DMG, true);
                    DoCast(me, SPELL_MASS_PARALYSIS, true);
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 50.0f);
                    events.ScheduleEvent(EVENT_PATH_OF_BLOSSOMS, 15000);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_generic_sop_unitsAI(creature);
    }
};

//220823
class go_ssop_spoils : public GameObjectScript
{
public:
    go_ssop_spoils() : GameObjectScript("go_ssop_spoils") { }

    bool OnGossipSelect(Player* pl, GameObject* go, uint32 /*uiSender*/, uint32 uiAction)
    {
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            if (instance->GetBossState(DATA_SPOILS_OF_PANDARIA) == NOT_STARTED)
            {
                if (uiAction == GOSSIP_ACTION_INFO_DEF)
                {
                    pl->PlayerTalkClass->ClearMenus();
                    pl->CLOSE_GOSSIP_MENU();
                    instance->SetBossState(DATA_SPOILS_OF_PANDARIA, IN_PROGRESS);
                }
            }
        }
        return true;
    }

    bool OnGossipHello(Player* pl, GameObject* go)
    {
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            if (instance->GetBossState(DATA_SPOILS_OF_PANDARIA) == NOT_STARTED)
            {
                pl->PrepareGossipMenu(go);
                pl->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                pl->SEND_GOSSIP_MENU(pl->GetGossipTextId(go), go->GetGUID());
            }
        }
        return true;
    }
};

//221771, 221773
class go_generic_lever : public GameObjectScript
{
public:
    go_generic_lever() : GameObjectScript("go_generic_lever") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            instance->HandleGameObject(instance->GetData64(go->GetEntry() == GO_LEVER_R ? GO_IRON_DOOR_R : GO_IRON_DOOR_L), true);
            if (instance->GetBossState(DATA_SPOILS_OF_PANDARIA) == IN_PROGRESS)
                instance->SetBossState(DATA_SPOILS_OF_PANDARIA, SPECIAL);
        }
        return false;
    }
};

//221906, 221893, 221885, 221816, 221820, 221804, 221878
class go_generic_sop_box : public GameObjectScript
{
public:
    go_generic_sop_box() : GameObjectScript("go_generic_sop_box"){}

    bool OnGossipHello(Player* player, GameObject* go)
    {
        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        Position pos;
        go->GetPosition(&pos);
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            if (Creature* summoner = instance->instance->GetCreature(instance->GetData64(go->GetEntry())))
            {
                switch (go->GetEntry())
                {
                case GO_BIG_MOGU_BOX:
                    summoner->SummonCreature(bigmoguentry[urand(0, 3)], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                case GO_BIG_MANTIS_BOX:
                    summoner->SummonCreature(bigmantisentry[urand(0, 3)], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                case GO_MEDIUM_MOGU_BOX:
                    summoner->SummonCreature(mediummoguentry[urand(0, 1)], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                case GO_MEDIUM_MANTIS_BOX:
                    summoner->SummonCreature(mediummantisentry[urand(0, 1)], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                case GO_SMALL_MOGU_BOX:
                {
                    uint8 entry = urand(0, 1);
                    if (entry)
                    {
                        for (uint8 n = 0; n < 3; n++)
                            go->SummonCreature(smallmoguentry[entry], pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                        summoner->SummonCreature(smallmoguentry[entry], pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    }
                    else
                        summoner->SummonCreature(smallmoguentry[entry], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                }
                case GO_SMALL_MANTIS_BOX:
                    summoner->SummonCreature(smallmantisentry[urand(0, 2)], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                case GO_PANDAREN_RELIC_BOX:
                    summoner->SummonCreature(pandarenrelicentry[urand(0, 2)], pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                default:
                    break;
                }
            }
        }
        return false;
    }
};

//148515
class spell_shadow_volley : public SpellScriptLoader
{
public:
    spell_shadow_volley() : SpellScriptLoader("spell_shadow_volley") { }

    class spell_shadow_volley_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_shadow_volley_SpellScript);

        void HandleOnHit()
        {
            if (GetHitUnit()->ToPlayer())
                GetCaster()->CastCustomSpell(SPELL_SHADOW_VOLLEY_D, SPELLVALUE_BASE_POINT0, GetSpellInfo()->Effects[EFFECT_0].BasePoints, GetHitUnit());
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_shadow_volley_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_shadow_volley_SpellScript();
    }
};

//148518
class spell_molten_fist : public SpellScriptLoader
{
public:
    spell_molten_fist() : SpellScriptLoader("spell_molten_fist") { }

    class spell_molten_fist_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_molten_fist_SpellScript);

        void HandleOnHit()
        {
            if (GetCaster())
            {
                int32 dmg = GetSpellInfo()->Effects[EFFECT_0].BasePoints;
                std::list<Player*> pllist;
                pllist.clear();
                GetPlayerListInGrid(pllist, GetCaster(), 30.0f);
                if (!pllist.empty())
                {
                    for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                        GetCaster()->CastCustomSpell(SPELL_MOLTEN_FIST_D, SPELLVALUE_BASE_POINT0, dmg, *itr, true);
                }
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_molten_fist_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_molten_fist_SpellScript();
    }
};

//148513
class spell_fracture : public SpellScriptLoader
{
public:
    spell_fracture() : SpellScriptLoader("spell_fracture") { }

    class spell_fracture_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_fracture_SpellScript);

        void HandleOnHit()
        {
            if (GetCaster())
            {
                int32 dmg = GetSpellInfo()->Effects[EFFECT_0].BasePoints;
                std::list<Player*> pllist;
                pllist.clear();
                GetPlayerListInGrid(pllist, GetCaster(), 30.0f);
                if (!pllist.empty())
                {
                    for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                        GetCaster()->CastCustomSpell(SPELL_FRACTURE_D, SPELLVALUE_BASE_POINT0, dmg, *itr, true);
                }
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_fracture_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_fracture_SpellScript();
    }
};

//146222
class spell_breath_of_fire : public SpellScriptLoader
{
public:
    spell_breath_of_fire() : SpellScriptLoader("spell_breath_of_fire") { }

    class spell_breath_of_fire_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_breath_of_fire_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetCaster() && GetTarget())
            {
                GetCaster()->CastSpell(GetTarget(), SPELL_BREATH_OF_FIRE_DMG, true);
                if (GetTarget()->HasAura(SPELL_KEG_TOSS_DMG))
                    GetCaster()->CastSpell(GetTarget(), SPELL_BREATH_OF_FIRE_TR, true);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_breath_of_fire_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_breath_of_fire_AuraScript();
    }
};

//146180
class spell_gusting_crane_kick : public SpellScriptLoader
{
public:
    spell_gusting_crane_kick() : SpellScriptLoader("spell_gusting_crane_kick") { }

    class spell_gusting_crane_kick_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gusting_crane_kick_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            if (GetCaster())
                GetCaster()->CastSpell(GetCaster(), SPELL_GUSTING_CRANE_KICK_DMG, true);
        }

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                {
                    GetCaster()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    GetCaster()->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                    GetCaster()->ToCreature()->AI()->DoZoneInCombat(GetCaster()->ToCreature(), 50.0f);
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gusting_crane_kick_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_gusting_crane_kick_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_gusting_crane_kick_AuraScript();
    }
};

//145987
class spell_set_to_blow : public SpellScriptLoader
{
public:
    spell_set_to_blow() : SpellScriptLoader("spell_set_to_blow") { }

    class spell_set_to_blow_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_set_to_blow_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (Aura* aura = aurEff->GetBase())
            {
                uint8 stack = aura->GetStackAmount();
                if (!stack)
                {
                    caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_1);
                    return;
                }

                switch (stack)
                {
                    case 1:
                        caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_2);
                        caster->CastSpell(caster, SPELL_SET_TO_BLOW_VISUAL_1, true);
                        break;
                    case 2:
                        caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_3);
                        caster->CastSpell(caster, SPELL_SET_TO_BLOW_VISUAL_2, true);
                        break;
                    case 3:
                        caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_4);
                        caster->CastSpell(caster, SPELL_SET_TO_BLOW_VISUAL_3, true);
                        break;
                    case 4:
                        caster->CastSpell(caster, SPELL_SET_TO_BLOW_VISUAL_4, true);
                        break;
                }
            }
        }

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if (Aura* aura = aurEff->GetBase())
                aura->SetStackAmount(4);
        }

        void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
        {
            if (Unit* caster = GetCaster())
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                {
                    if (Aura* aura = aurEff->GetBase())
                    {
                        uint8 stack = aura->GetStackAmount();
                        if (!stack)
                            return;

                        for (uint8 i = 0; i < stack; i++)
                            caster->CastSpell(caster, SPELL_SET_TO_BLOW_DMG, true);
                    }
                }
                caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_1);
                caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_2);
                caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_3);
                caster->RemoveAura(SPELL_SET_TO_BLOW_VISUAL_4);
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_set_to_blow_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            OnEffectApply += AuraEffectApplyFn(spell_set_to_blow_AuraScript::OnApply, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_set_to_blow_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_set_to_blow_AuraScript();
    }
};

//148762
class spell_pheromone_cloud : public SpellScriptLoader
{
    public:
        spell_pheromone_cloud() : SpellScriptLoader("spell_pheromone_cloud") { }

        class spell_pheromone_cloud_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pheromone_cloud_AuraScript);

            void HandlePeriodicTick(AuraEffect const* /*aurEff*/)
            {
                if (Creature* caster = GetCaster()->ToCreature())
                    if (Unit* pTarget = caster->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        caster->CastSpell(pTarget, SPELL_PHEROMONE_CLOUD_DMG, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pheromone_cloud_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pheromone_cloud_AuraScript();
        }
};

// 145812
class spell_rage_of_the_empress : public SpellScriptLoader
{
    public:
        spell_rage_of_the_empress() : SpellScriptLoader("spell_rage_of_the_empress") { }

        class spell_rage_of_the_empress_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rage_of_the_empress_SpellScript);

            int32 bp0;
            bool Load()
            {
                bp0 = 0;
                return true;
            }

            void FilterTargetsDamage(std::list<WorldObject*>& targets)
            {
                targets.remove(GetCaster());

                if (targets.empty())
                    return;

                SpellInfo const* Spell = sSpellMgr->GetSpellInfo(SPELL_RAGE_OF_THE_EMPRESS_AURA);
                bp0 = Spell->Effects[EFFECT_0].BasePoints / targets.size();
            }

            void HandleOnHit()
            {
                if (!GetCaster())
                    return;

                if (!GetHitUnit())
                    return;

                GetCaster()->CastCustomSpell(GetHitUnit(), SPELL_RAGE_OF_THE_EMPRESS_AURA, &bp0, NULL, NULL, false);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rage_of_the_empress_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
                OnHit += SpellHitFn(spell_rage_of_the_empress_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rage_of_the_empress_SpellScript();
        }
};

void AddSC_boss_spoils_of_pandaria()
{
    new npc_ssop_spoils();
    new npc_generic_spoil();
    new npc_lever();
    new npc_lift_hook();
    new npc_generic_sop_units();
    new go_ssop_spoils();
    new go_generic_lever();
    new go_generic_sop_box();
    new spell_shadow_volley();
    new spell_molten_fist();
    new spell_fracture();
    new spell_breath_of_fire();
    new spell_gusting_crane_kick();
    new spell_set_to_blow();
    new spell_pheromone_cloud();
    new spell_rage_of_the_empress();
}
