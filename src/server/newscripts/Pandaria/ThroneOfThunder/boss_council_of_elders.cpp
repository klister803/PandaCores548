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
#include "throne_of_thunder.h"

enum eSpells
{
    //Mallak
    SPELL_FRIGIT_ASSAULT     = 136904,
    SPELL_FRIGIT_ASSAULT_S   = 136910,
    SPELL_FRIGIT_ASSAULT_D   = 136903,
    SPELL_BITING_COLD        = 136992,
    //Sul
    SPELL_SAND_BOLT          = 136189,
    SPELL_QUICK_SAND_VISUAL  = 136851,
    //Marli
    SPELL_WRATH_OF_THE_LOA   = 137344,
    //Kazrajin
    SPELL_R_CHARGE_DMG       = 137133,
    SPELL_R_CHARGE_POINT_T   = 138026,
    SPELL_R_CHARGE_VISUAL    = 137117,
    SPELL_R_CHARGE_POINT_DMG = 137122,

    SPELL_LINGERING_PRESENCE = 136467, //after soul gone
    SPELL_DARK_POWER         = 136507,
    SPELL_POSSESSED          = 136442,
    SPELL_GARAJAL_SOUL_V     = 136423,
};

enum SsAction
{
    ACTION_RESET,
    ACTION_CHANGE_COUNCIL,
    ACTION_GARAJAL_SOUL_ACTIVE,
};

enum eEvents
{
    //Mallak
    EVENT_BITTING_COLD       = 1,
    EVENT_FRIGIT_ASSAULT     = 2,
    //Sul
    EVENT_SAND_BOLT          = 3,
    //Kazrajin
    EVENT_R_CHARGE           = 4,
    EVENT_MOVING             = 5,
};

//Spells summon loa spirit
uint32 spell_loa_spirit[3] =
{
    137200,
    137201,
    137202,
};

uint32 councilentry[4] =
{
    NPC_FROST_KING_MALAKK,
    NPC_PRINCESS_MARLI,
    NPC_KAZRAJIN,
    NPC_SUL_SANDCRAWLER,
};

struct council_of_eldersAI : public ScriptedAI
{
    council_of_eldersAI(Creature* creature) : ScriptedAI(creature)
    {
        instance = creature->GetInstanceScript();
    }
    InstanceScript* instance;

    void CouncilsReset()
    {
        for (int32 i = 0; i < 4; i++)
        {
            if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[i])))
            {
                if (me->GetEntry() != council->GetEntry())
                {
                    if (council->isAlive() && council->isInCombat())
                        council->AI()->EnterEvadeMode();
                    else
                    {
                        council->Respawn();
                        council->GetMotionMaster()->MoveTargetedHome();
                    }
                }
            }
        }
        if (instance->GetBossState(DATA_COUNCIL_OF_ELDERS != NOT_STARTED))
            instance->SetBossState(DATA_COUNCIL_OF_ELDERS, NOT_STARTED);
    }

    void CouncilsEnterCombat()
    {
        for (int32 i = 0; i < 4; i++)
            if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[i])))
                if (me->GetEntry() != council->GetEntry())
                    if (council->isAlive() && !council->isInCombat())
                        DoZoneInCombat(council, 150.0f);
        if (instance->GetBossState(DATA_COUNCIL_OF_ELDERS) != IN_PROGRESS)
            instance->SetBossState(DATA_COUNCIL_OF_ELDERS, IN_PROGRESS);
    }
};

class boss_council_of_elders : public CreatureScript
{
public:
    boss_council_of_elders() : CreatureScript("boss_council_of_elders") {}

    struct boss_council_of_eldersAI : public council_of_eldersAI
    {
        boss_council_of_eldersAI(Creature* creature) : council_of_eldersAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
        InstanceScript* instance;
        Position chargepos;
        EventMap events;
        uint32 donehppct;

        void Reset()
        {
            CouncilsReset();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            //RemovePassenger(true);
            events.Reset();
            me->RemoveAurasDueToSpell(SPELL_R_CHARGE_VISUAL);
            if (me->GetEntry() == NPC_KAZRAJIN)
                me->SetReactState(REACT_PASSIVE);
            else
                me->SetReactState(REACT_DEFENSIVE);
            //if (me->GetEntry() == NPC_FROST_KING_MALAKK)
                //ResetGarajalSoul();
            me->setPowerType(POWER_ENERGY);
            me->SetPower(POWER_ENERGY, 0);
            donehppct = 100; //default value
        }

        void RemovePassenger(bool action)
        {
            if (Vehicle* ij = me->GetVehicleKit())
            {
                if (Unit* p = ij->GetPassenger(0))
                {
                    if (p->ToCreature() && p->ToCreature()->AI())
                    {
                        if (action)
                            p->ToCreature()->AI()->DoAction(ACTION_RESET);
                        else
                            p->ToCreature()->AI()->DoAction(ACTION_CHANGE_COUNCIL);
                    }
                }
            }
        }

        void ResetGarajalSoul()
        {
            if (Creature* gs = me->GetCreature(*me, instance->GetData64(NPC_GARAJAL_SOUL)))
                gs->AI()->DoAction(ACTION_RESET);
        }

        void ActiveGarajalSoul()
        {
            if (Creature* gs = me->GetCreature(*me, instance->GetData64(NPC_GARAJAL_SOUL)))
                gs->AI()->DoAction(ACTION_GARAJAL_SOUL_ACTIVE);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            /*if (donehppct < 100)
            {
                if (HealthBelowPct(donehppct))
                {
                    donehppct = 100;
                    RemovePassenger(false);
                }
            }*/
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (apply)
            {
                donehppct = uint32(me->GetHealthPct()) - 25 < 0 ? 1 : uint32(me->GetHealthPct() - 25);
                DoCast(me, SPELL_POSSESSED, true);
                //Create special events
            }
            else
            {
                me->RemoveAurasDueToSpell(SPELL_POSSESSED);
                me->SetPower(POWER_ENERGY, 0);
                DoCast(me, SPELL_LINGERING_PRESENCE, true);
                //Offline special events
            }
        }

        void EnterCombat(Unit* who)
        {
            CouncilsEnterCombat();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            switch (me->GetEntry())
            {
            case NPC_FROST_KING_MALAKK:
                //ActiveGarajalSoul();
                events.ScheduleEvent(EVENT_FRIGIT_ASSAULT, 15000);
                events.ScheduleEvent(EVENT_BITTING_COLD, 30000);
                break;
            case NPC_SUL_SANDCRAWLER:
                events.ScheduleEvent(EVENT_SAND_BOLT, 35000);
                break;
            case NPC_KAZRAJIN:
                events.ScheduleEvent(EVENT_R_CHARGE, 6000);
                break;
            default:
                break;
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_R_CHARGE_VISUAL)
                me->GetMotionMaster()->MoveCharge(chargepos.GetPositionX(), chargepos.GetPositionY(), chargepos.GetPositionZ(), 35.0f, 1);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (pointId == 1)
                {
                    me->RemoveAurasDueToSpell(SPELL_R_CHARGE_VISUAL);
                    DoCastAOE(SPELL_R_CHARGE_POINT_DMG);
                }
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
                //Mallak
                case EVENT_BITTING_COLD:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                        DoCast(target, SPELL_BITING_COLD);
                    events.ScheduleEvent(EVENT_BITTING_COLD, 45000);
                    break;
                case EVENT_FRIGIT_ASSAULT:
                    DoCast(me, SPELL_FRIGIT_ASSAULT);
                    events.ScheduleEvent(EVENT_FRIGIT_ASSAULT, 30000);
                    break;
                //Sul
                case EVENT_SAND_BOLT:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                        DoCast(target, SPELL_SAND_BOLT);
                    events.ScheduleEvent(EVENT_SAND_BOLT, 35000);
                    break;
                //Kazrajin
                case EVENT_R_CHARGE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 80.0f, true))
                    {
                        me->SetFacingToObject(target);
                        target->GetPosition(&chargepos);
                        target->CastSpell(target, SPELL_R_CHARGE_POINT_T);
                        DoCast(me, SPELL_R_CHARGE_VISUAL);
                    }
                    events.ScheduleEvent(EVENT_R_CHARGE, 6000);
                    break;
                }
            }
            if (me->GetEntry() != NPC_PRINCESS_MARLI)
                DoMeleeAttackIfReady();
            else
                DoSpellAttackIfReady(SPELL_WRATH_OF_THE_LOA);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_council_of_eldersAI(creature);
    }
};

//69182
class npc_garajal_soul : public CreatureScript
{
public:
    npc_garajal_soul() : CreatureScript("npc_garajal_soul") {}

    struct npc_garajal_soulAI : public ScriptedAI
    {
        npc_garajal_soulAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }
        InstanceScript* instance;
        EventMap events;
        uint32 lastcouncil;
        bool donecouncil;
        uint8 councilcount;

        void Reset()
        {
            events.Reset();
            councilcount = 0;
            lastcouncil = 0;
            donecouncil = false;
            DoCast(me, SPELL_GARAJAL_SOUL_V, true);
        }

        uint8 GetCouncilCount(uint32 entry)
        {
            switch (entry)
            {
            case NPC_FROST_KING_MALAKK:
                return 0;
            case NPC_PRINCESS_MARLI:
                return 1;
            case NPC_KAZRAJIN:
                return 2;
            case NPC_SUL_SANDCRAWLER:
                return 3;
            }
            return 0;
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
            case ACTION_RESET:
                events.Reset();
                councilcount = 0;
                donecouncil = false;
                lastcouncil = 0;
                me->ExitVehicle();
                me->GetMotionMaster()->Clear(false);
                me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
                break;
            case ACTION_CHANGE_COUNCIL:
                me->ExitVehicle();
                councilcount++;
                if ((councilcount > 3 && !donecouncil) || donecouncil)
                {
                    if (!donecouncil)
                        donecouncil = true;
                    std::vector<uint64> councillistGuids;
                    councillistGuids.clear();
                    for (uint8 n = 0; n < 4; n++)
                        if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[n])))
                            if (council->GetEntry() != lastcouncil)
                                if (council->isAlive() && council->isInCombat())
                                    councillistGuids.push_back(council->GetGUID());

                    if (!councillistGuids.empty())
                    {
                        std::vector<uint64>::const_iterator Itr = councillistGuids.begin();
                        std::advance(Itr, urand(0, councillistGuids.size() - 1));
                        if (Creature* council = me->GetCreature(*me, *Itr))
                            councilcount = GetCouncilCount(council->GetEntry());
                    }
                }
                if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[councilcount])))
                {
                    lastcouncil = council->GetEntry();
                    me->GetMotionMaster()->MoveCharge(council->GetPositionX(), council->GetPositionY(), council->GetPositionZ(), 10.0f, 1);
                }
                events.ScheduleEvent(EVENT_MOVING, 1000);
                break;
            case ACTION_GARAJAL_SOUL_ACTIVE:
                if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[councilcount])))
                {
                    lastcouncil = council->GetEntry();
                    me->GetMotionMaster()->MoveCharge(council->GetPositionX(), council->GetPositionY(), council->GetPositionZ(), 10.0f, 1);
                    events.ScheduleEvent(EVENT_MOVING, 1000);
                }
                break;
            default:
                break;
            }
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE && pointId)
            {
                events.Reset();
                if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[councilcount])))
                    me->EnterVehicle(council, 0);
            }
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_MOVING)
                {
                    if (Creature* council = me->GetCreature(*me, instance->GetData64(councilentry[councilcount])))
                    {
                        me->GetMotionMaster()->Clear(false);
                        me->GetMotionMaster()->MoveCharge(council->GetPositionX(), council->GetPositionY(), council->GetPositionZ(), 10.0f, 1);
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_garajal_soulAI(creature);
    }
};


class spell_frigit_assault : public SpellScriptLoader
{
public:
    spell_frigit_assault() : SpellScriptLoader("spell_frigit_assault") { }

    class spell_frigit_assault_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_frigit_assault_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
            {
                if (GetTarget()->HasAura(SPELL_FRIGIT_ASSAULT_D) && !GetTarget()->HasAura(SPELL_FRIGIT_ASSAULT_S))
                    if (GetTarget()->GetAura(SPELL_FRIGIT_ASSAULT_D)->GetStackAmount() == 15)
                        GetTarget()->AddAura(SPELL_FRIGIT_ASSAULT_S, GetTarget());
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_frigit_assault_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        }
    };
    
    AuraScript* GetAuraScript() const
    {
        return new spell_frigit_assault_AuraScript();
    }
};

//136442
class spell_possessed : public SpellScriptLoader
{
public:
    spell_possessed() : SpellScriptLoader("spell_possessed") { }

    class spell_possessed_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_possessed_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (GetCaster()->GetPower(POWER_ENERGY) + 1 < 100)
                    GetCaster()->SetPower(POWER_ENERGY, GetCaster()->GetPower(POWER_ENERGY) + 1);
                else if (GetCaster()->GetPower(POWER_ENERGY) == 100)
                    GetCaster()->CastSpell(GetCaster(), SPELL_DARK_POWER);
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_possessed_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_possessed_AuraScript();
    }
};

void AddSC_boss_council_of_elders()
{
    new boss_council_of_elders();
    new npc_garajal_soul();
    new spell_frigit_assault();
    new spell_possessed();
}
