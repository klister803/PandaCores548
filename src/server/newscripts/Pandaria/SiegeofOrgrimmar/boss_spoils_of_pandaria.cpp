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
    //Big Mogu
    SPELL_EARTHEN_SHARD          = 144923,
    SPELL_STRENGTH_OF_THE_STONE  = 145998,
    SPELL_SHADOW_VOLLEY          = 148515,
    SPELL_SHADOW_VOLLEY_D        = 148516,
    SPELL_MOLTEN_FIST            = 148518,
    SPELL_MOLTEN_FIST_D          = 148517,
    SPELL_JADE_TEMPEST           = 148582,
    SPELL_JADE_TEMPEST_D         = 148583,
    SPELL_FRACTURE               = 148513,
    SPELL_FRACTURE_D             = 148514,
    //Small mogu
    SPELL_HARDEN_FLESH_V         = 144922,
    SPELL_HARDEN_FLESH_DMG       = 145218,    
    //Special
    SPELL_LIFT_HOOK_VISUAL       = 142721,
    SPELL_AURA_BAR               = 144921,
    SPELL_AURA_BAR2              = 148505,
};

enum Events
{
    //Lift hook
    EVENT_START_LIFT             = 1,
    EVENT_POINT                  = 2,
    EVENT_BACK                   = 3,
    EVENT_RESET                  = 4,
    //Summons
    //Big mogu
    EVENT_SPAWN                  = 5,
    EVENT_EARTHEN_SHARD          = 6, 
    EVENT_SHADOW_VOLLEY          = 7,
    EVENT_MOLTEN_FIST            = 8,
    EVENT_JADE_TEMPEST           = 9,
    EVENT_FRACTURE               = 10,
    EVENT_HARDEN_FLESH           = 11,

    //144281
    //146529 from death mob
};

Position dpos[4] =
{
    { 1606.77f, -5124.41f, -263.3986f, 0.0f },
    { 1657.45f, -5127.57f, -263.3986f, 0.0f },
    { 1639.86f, -5101.63f, -263.3986f, 0.0f },
    { 1622.80f, -5148.56f, -263.3986f, 0.0f },
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

        //for safe
        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

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
                }
                break;
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
            //me->SetReactState(REACT_PASSIVE);
            //me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        InstanceScript* instance;
        EventMap events;

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
        }


        void IsSummonedBy(Unit* summoner)
        {
            events.ScheduleEvent(EVENT_SPAWN, 1500);
        }

        void EnterCombat(Unit* who)
        {
            switch (me->GetEntry())
            {
                //Big mogu
                case NPC_JUN_WEI:
                    events.ScheduleEvent(EVENT_SHADOW_VOLLEY, 5000);
                    break;
                case NPC_ZU_YIN:
                    events.ScheduleEvent(EVENT_MOLTEN_FIST, 5000);
                    break;
                case NPC_XIANG_LIN:
                    events.ScheduleEvent(EVENT_JADE_TEMPEST, 5000);
                    break;
                case NPC_KUN_DA:
                    events.ScheduleEvent(EVENT_FRACTURE, 5000);
                    break;
                //Small mogu
                case NPC_ANIMATED_STONE_MOGU:
                    events.ScheduleEvent(EVENT_EARTHEN_SHARD, 5000);
                    break;
                default:
                    break;
            }
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
                //default events;
                case EVENT_SPAWN:
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    DoZoneInCombat(me, 100.0f);
                    break;
                case EVENT_EARTHEN_SHARD:
                    DoCastVictim(SPELL_EARTHEN_SHARD);
                    events.ScheduleEvent(EVENT_EARTHEN_SHARD, 5000);
                    break;
                //
                case EVENT_MOLTEN_FIST:
                    DoCast(me, SPELL_MOLTEN_FIST);
                    events.ScheduleEvent(EVENT_MOLTEN_FIST, 5000);
                    break;
                case EVENT_SHADOW_VOLLEY:
                    DoCast(me, SPELL_SHADOW_VOLLEY);
                    events.ScheduleEvent(EVENT_SHADOW_VOLLEY, 5000);
                    break;
                case EVENT_JADE_TEMPEST:
                    DoCast(me, SPELL_JADE_TEMPEST);
                    events.ScheduleEvent(EVENT_JADE_TEMPEST, 3000);
                    break;
                case EVENT_FRACTURE:
                    DoCast(me, SPELL_FRACTURE);
                    events.ScheduleEvent(EVENT_FRACTURE, 5000);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_generic_sop_unitsAI(creature);
    }
};

//GO_SMALL_MOGU_BOX      = 221906,
//GO_MEDIUM_MOGU_BOX     = 221893,
//GO_BIG_MOGU_BOX        = 221885,
//GO_SMALL_MANTIS_BOX    = 221816,
//GO_MEDIUM_MANTIS_BOX   = 221820,
//GO_BIG_MANTIS_BOX      = 221804,
//GO_PANDAREN_RELIC_BOX  = 221878,

uint32 bigmoguentry[4] =
{
    NPC_JUN_WEI,
    NPC_ZU_YIN,
    NPC_XIANG_LIN,
    NPC_KUN_DA,
};

uint32 mediummoguentry[2] =
{
    NPC_MODIFIED_ANIMA_GOLEM,
    NPC_MOGU_SHADOW_RITUALIST,
};

class go_generic_sop_box : public GameObjectScript
{
public:
    go_generic_sop_box() : GameObjectScript("go_generic_sop_box") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        Position pos;
        go->GetPosition(&pos);

        switch (go->GetEntry())
        {
        case GO_BIG_MOGU_BOX:
        {
            uint8 pos = urand(0, 3);
            go->SummonCreature(bigmoguentry[pos], pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            break;
        }
        case GO_MEDIUM_MOGU_BOX:
        {
            uint8 pos = urand(0, 1);
            go->SummonCreature(mediummoguentry[pos], pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            break;
        }
        case GO_SMALL_MOGU_BOX:
            go->SummonCreature(NPC_ANIMATED_STONE_MOGU, pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            break;
        default:
            break;
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


void AddSC_boss_spoils_of_pandaria()
{
    new npc_lift_hook();
    new npc_generic_sop_units();
    new go_generic_sop_box();
    new spell_shadow_volley();
    new spell_molten_fist();
    new spell_fracture();
}
