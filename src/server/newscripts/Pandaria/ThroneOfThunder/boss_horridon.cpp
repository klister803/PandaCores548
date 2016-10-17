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
    //Horridon
    SPELL_RAMPAGE            = 136821,
    SPELL_TRIPLE_PUNCTURE    = 136767,
    SPELL_DOUBLE_SWIPE       = 136741,
    SPELL_HORRIDON_CHARGE    = 136769,
    //Jalak
    SPELL_BESTIAL_CRY        = 136817,

    //Gate Adds
    //Farrak
    SPELL_BLAZING_SUNLIGHT   = 136719,
    SPELL_SAND_TRAP          = 136724,
    SPELL_STONE_GAZE         = 136708,
    //Gurubashi
    SPELL_VENOM_BOLT_VOLLEY  = 136587,
    SPELL_LIVING_POISON      = 136645,
    SPELL_RENDING_CHARGE     = 136653,
    SPELL_RENDING_CHARGE_DMG = 136654,
    //Drakkari
    SPELL_MORTAL_STRIKE      = 136670,
    SPELL_FROZEN_ORB_SUM     = 136564,
    SPELL_FROZEN_BOLT_AURA   = 136572,
    SPELL_UNCONTROLLED_ABOM  = 136709, //Uncontrolled Abomination
    //Amani
    SPELL_SWIPE              = 136463,
    SPELL_CHAIN_LIGHTNING    = 136480,
    SPELL_LIGHTNING_NOVA_T_S = 136487,
    SPELL_LIGHTNING_NOVA     = 136489,
    SPELL_HEX_OF_CONFUSION   = 136512,
    SPELL_FIREBALL           = 136465,
};

enum sEvents
{
    //Horridon
    EVENT_TRIPLE_PUNCTURE    = 1,
    EVENT_DOUBLE_SWIPE       = 2,
    EVENT_CHARGES            = 3,
    EVENT_OPEN_GATE          = 4,
    //Jalak
    EVENT_INTRO              = 5,
    EVENT_BESTIAL_CRY        = 6,
    //Add events
    EVENT_BLAZING_SUNLIGHT   = 7,
    EVENT_SAND_TRAP          = 8,
    EVENT_STONE_GAZE         = 9,
    EVENT_VENOM_BOLT_VOLLEY  = 10,
    EVENT_RENDING_CHARGE     = 11,
    EVENT_MORTAL_STRIKE      = 12,
    EVENT_SUMMON_FROZEN_ORB  = 13,
    EVENT_SWIPE              = 14,
    EVENT_CHAIN_LIGHTNING    = 15,
    EVENT_SUMMON_TOTEM       = 16,
    EVENT_HEX_OF_CONFUSION   = 17,
    EVENT_FIREBALL           = 18,

    EVENT_RE_ATTACK          = 35,
};

enum sAction
{
    //Jalak
    ACTION_INTRO             = 1,
    ACTION_RE_ATTACK         = 2,
    ACTION_SHAMAN_DISMAUNT   = 3,
};

enum Phase
{
    PHASE_NULL,
    PHASE_ONE,
    PHASE_TWO,
};

class boss_horridon : public CreatureScript
{
public:
    boss_horridon() : CreatureScript("boss_horridon") {}
    
    struct boss_horridonAI : public BossAI
    {
        boss_horridonAI(Creature* creature) : BossAI(creature, DATA_HORRIDON)
        {
            instance = creature->GetInstanceScript();
        }
        InstanceScript* instance;
        Phase phase;

        void Reset()
        {
            _Reset();
            ResetJalak();
            phase = PHASE_NULL;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void ResetJalak()
        {
            if (Creature* jalak = me->GetCreature(*me, instance->GetData64(NPC_JALAK)))
            {
                if (!jalak->isAlive())
                {
                    jalak->Respawn();
                    jalak->GetMotionMaster()->MoveTargetedHome();
                }
                else if (jalak->isInCombat())
                    jalak->AI()->EnterEvadeMode();
            }
        }

        void EnterCombat(Unit* who)
        {
            _EnterCombat();
            phase = PHASE_ONE;
            events.ScheduleEvent(EVENT_TRIPLE_PUNCTURE, urand(11000, 15000));
            events.ScheduleEvent(EVENT_DOUBLE_SWIPE, urand(17000, 20000));
            //events.ScheduleEvent(EVENT_OPEN_GATE, 31000);
            events.ScheduleEvent(EVENT_CHARGES, urand(50000, 60000));
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (HealthBelowPct(30) && phase == PHASE_ONE)
            {
                phase = PHASE_TWO;
                if (Creature* jalak = me->GetCreature(*me, instance->GetData64(NPC_JALAK)))
                    jalak->AI()->DoAction(ACTION_INTRO);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Creature* jalak = me->GetCreature(*me, instance->GetData64(NPC_JALAK)))
            {
                if (jalak->isAlive())
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                else
                    instance->SetBossState(DATA_HORRIDON, DONE);
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
                case EVENT_TRIPLE_PUNCTURE:
                    if (me->getVictim())
                        DoCast(me->getVictim(), SPELL_TRIPLE_PUNCTURE);
                    events.ScheduleEvent(EVENT_TRIPLE_PUNCTURE, urand(11000, 15000));
                    break;
                case EVENT_DOUBLE_SWIPE:
                    DoCast(me, SPELL_DOUBLE_SWIPE);
                    events.ScheduleEvent(EVENT_DOUBLE_SWIPE, urand(17000, 20000));
                    break;
                case EVENT_CHARGES:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                        DoCast(target, SPELL_HORRIDON_CHARGE);
                    events.ScheduleEvent(EVENT_CHARGES, urand(50000, 60000));
                    break;
                case EVENT_OPEN_GATE:
                    if (GameObject* gate = instance->instance->GetGameObject(instance->GetData64(DATA_GET_NEXT_GATE)))
                        gate->UseDoorOrButton(10);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_horridonAI(creature);
    }
};

class boss_jalak : public CreatureScript
{
public:
    boss_jalak() : CreatureScript("boss_jalak") {}
    
    struct boss_jalakAI : public CreatureAI
    {
        boss_jalakAI(Creature* creature) : CreatureAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
            ResetHorridon();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void ResetHorridon()
        {
            if (Creature* horridon = me->GetCreature(*me, instance->GetData64(NPC_HORRIDON)))
            {
                if (!horridon->isAlive())
                {
                    horridon->Respawn();
                    horridon->GetMotionMaster()->MoveTargetedHome();
                }
            }
        }

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_BESTIAL_CRY, 5000);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE && type != EFFECT_MOTION_TYPE)
                return;

            if (pointId == 0)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_AGGRESSIVE);
                DoZoneInCombat(me, 150.0f);
            }
        }

        void DoAction(int32 const action)
        {
            if (action == ACTION_INTRO)
                me->GetMotionMaster()->MoveJump(5433.32f, 5745.32f, 129.6066f, 25.0f, 25.0f, 0);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Creature* horridon = me->GetCreature(*me, instance->GetData64(NPC_HORRIDON)))
            {
                if (horridon->isAlive())
                    horridon->AddAura(SPELL_RAMPAGE, horridon);
                else
                {
                    horridon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    instance->SetBossState(DATA_HORRIDON, DONE);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_BESTIAL_CRY)
                {
                    DoCast(me, SPELL_BESTIAL_CRY);
                    events.ScheduleEvent(EVENT_BESTIAL_CRY, 10000);
                }
            }
            DoMeleeAttackIfReady();
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_jalakAI(creature);
    }
};

//Farrak: big - 69175, small - 69172.
//Gurubashi: big - 69164, 69314, small - 69167.
//Drakkari: big - 69178, special summons - 69268, small - 69185.
//Amani: big - 69177, 69176, small - 69168.
class npc_generic_gate_add: public CreatureScript
{
public:
    npc_generic_gate_add() : CreatureScript("npc_generic_gate_add") {}

    struct npc_generic_gate_addAI : public ScriptedAI
    {
        npc_generic_gate_addAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            if (me->GetEntry() == NPC_RISEN_DRAKKARI_CHAMPION)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            }
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
            switch (me->GetEntry())
            {
            case NPC_FROZEN_ORB:
                me->SetDisplayId(11686);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                DoZoneInCombat(me, 100.0f);
                DoCast(me, SPELL_FROZEN_BOLT_AURA, true);
                break;
            case NPC_RISEN_DRAKKARI_CHAMPION:
                FindAndAttackRandomPlayer();
                break;
            case NPC_AMANISHI_BEAST_SHAMAN:
                me->SetFlag(UNIT_FIELD_FLAGS,  UNIT_FLAG_NON_ATTACKABLE);
                break;
            }
        }

        void FindAndAttackRandomPlayer()
        {
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f, true))
            {
                me->AddThreat(target, 50000000.0f);
                me->SetReactState(REACT_AGGRESSIVE);
                me->Attack(target, true);
                me->GetMotionMaster()->MoveChase(target);
            }
        }

        void EnterCombat(Unit* who)
        {
            switch (me->GetEntry())
            {
            //Farrak gate
            case NPC_FARRAKI_WASTEWALKER:
                events.ScheduleEvent(EVENT_BLAZING_SUNLIGHT, 10000);
                events.ScheduleEvent(EVENT_SAND_TRAP, 7000);
                break;
            case NPC_SULLITHUZ_STONEGAZER:
                events.ScheduleEvent(EVENT_STONE_GAZE, 10000);
                break;
            //Gurubashi
            case NPC_GURUBASHI_VENOM_PRIEST:
                events.ScheduleEvent(EVENT_VENOM_BOLT_VOLLEY, 10000);
                break;
            case NPC_GURUBASHI_BLOODLORD:
                events.ScheduleEvent(EVENT_RENDING_CHARGE, 10000);
                break;
            //Drakkari
            case NPC_DRAKKARI_FROZEN_WARLORD:
                events.ScheduleEvent(EVENT_MORTAL_STRIKE, 6000);
                events.ScheduleEvent(EVENT_SUMMON_FROZEN_ORB, 15000);
                break;
            case NPC_RISEN_DRAKKARI_CHAMPION:
                DoCast(me, SPELL_UNCONTROLLED_ABOM, true);
                break;
            //Amani
            case NPC_AMANI_WARBEAR:
                events.ScheduleEvent(EVENT_SWIPE, 5000);
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 10000);
                events.ScheduleEvent(EVENT_SUMMON_TOTEM, 20000);
                events.ScheduleEvent(EVENT_HEX_OF_CONFUSION, 30000);
                break;
            case NPC_AMANISHI_FLAME_CASTER:
                events.ScheduleEvent(EVENT_FIREBALL, 4000);
                break;
            }
        }

        void JustDied(Unit* killer)
        {
            if (me->GetEntry() == NPC_AMANI_WARBEAR)
                if (Vehicle* vehicle = me->GetVehicleKit())
                    if (Unit* passenger = vehicle->GetPassenger(0))
                        passenger->ToCreature()->AI()->DoAction(ACTION_SHAMAN_DISMAUNT);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
            case ACTION_RE_ATTACK:
                events.ScheduleEvent(EVENT_RE_ATTACK, 1500);
                break;
            case ACTION_SHAMAN_DISMAUNT:
                me->ExitVehicle();
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_AGGRESSIVE);
                DoZoneInCombat(me, 100.0f);
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 10000);
                events.ScheduleEvent(EVENT_SUMMON_TOTEM, 20000);
                events.ScheduleEvent(EVENT_HEX_OF_CONFUSION, 30000);
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
                //Farrak
                case EVENT_BLAZING_SUNLIGHT:
                    DoCast(me, SPELL_BLAZING_SUNLIGHT);
                    events.ScheduleEvent(EVENT_BLAZING_SUNLIGHT, 10000);
                    break;
                case EVENT_SAND_TRAP:
                    if (Creature* horridon = me->GetCreature(*me, instance->GetData64(NPC_HORRIDON)))
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f, true))
                            horridon->SummonCreature(NPC_SAND_TRAP, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                    events.ScheduleEvent(EVENT_SAND_TRAP, 7000);
                    break;
                case EVENT_STONE_GAZE:
                    me->SetAttackStop(true);
                    DoCast(me, SPELL_STONE_GAZE);
                    break;
                //Gurubashi
                case EVENT_VENOM_BOLT_VOLLEY:
                    DoCast(me, SPELL_VENOM_BOLT_VOLLEY);
                    events.ScheduleEvent(EVENT_VENOM_BOLT_VOLLEY, 20000);
                    break;
                case EVENT_RENDING_CHARGE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 55.0f, true))
                    {
                        me->SetAttackStop(true);
                        DoCast(target, SPELL_RENDING_CHARGE);
                    }
                    break;
                //Drakkari
                case EVENT_MORTAL_STRIKE:
                    if (me->getVictim())
                        DoCast(me->getVictim(), SPELL_MORTAL_STRIKE);
                    events.ScheduleEvent(EVENT_MORTAL_STRIKE, 10000);
                    break;
                case EVENT_SUMMON_FROZEN_ORB:
                    DoCast(me, SPELL_FROZEN_ORB_SUM);
                    events.ScheduleEvent(EVENT_SUMMON_FROZEN_ORB, 20000);
                    break;
                //Amani
                case EVENT_SWIPE:
                    if (me->getVictim())
                        DoCast(me->getVictim(), SPELL_SWIPE);
                    events.ScheduleEvent(EVENT_SWIPE, 5000);
                    break;
                //Amani Beast Shaman
                case EVENT_CHAIN_LIGHTNING:
                    if (me->GetEntry() == NPC_AMANI_WARBEAR)
                    {
                        if (Vehicle* vehicle = me->GetVehicleKit())
                            if (Unit* passenger = vehicle->GetPassenger(0))
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                                    passenger->CastSpell(target, SPELL_CHAIN_LIGHTNING);
                    }
                    else
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                            DoCast(target, SPELL_CHAIN_LIGHTNING);
                    }
                    events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 10000);
                    break;
                case EVENT_SUMMON_TOTEM:
                    if (me->GetEntry() == NPC_AMANI_WARBEAR)
                    {
                        if (Vehicle* vehicle = me->GetVehicleKit())
                            if (Unit* passenger = vehicle->GetPassenger(0))
                                passenger->CastSpell(passenger, SPELL_LIGHTNING_NOVA_T_S);
                    }
                    else
                        DoCast(me, SPELL_LIGHTNING_NOVA_T_S);
                    events.ScheduleEvent(EVENT_SUMMON_TOTEM, 20000);
                    break;
                case EVENT_HEX_OF_CONFUSION:
                    if (me->GetEntry() == NPC_AMANI_WARBEAR)
                    {
                        if (Vehicle* vehicle = me->GetVehicleKit())
                            if (Unit* passenger = vehicle->GetPassenger(0))
                                passenger->CastSpell(passenger, SPELL_HEX_OF_CONFUSION);
                    }
                    else
                        DoCast(me, SPELL_HEX_OF_CONFUSION);
                    events.ScheduleEvent(EVENT_HEX_OF_CONFUSION, 30000);
                    break;
                //
                case EVENT_FIREBALL:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 45.0f, true))
                        DoCast(target, SPELL_FIREBALL);
                    events.ScheduleEvent(EVENT_FIREBALL, 4000);
                    break;
                //Special
                case EVENT_RE_ATTACK:
                    me->ReAttackWithZone();
                    switch (me->GetEntry())
                    {
                    case NPC_SULLITHUZ_STONEGAZER:
                        events.ScheduleEvent(EVENT_STONE_GAZE, 10000);
                        break;
                    case NPC_GURUBASHI_BLOODLORD:
                        events.ScheduleEvent(EVENT_RENDING_CHARGE, 10000);
                        break;
                    default:
                        break;
                    }
                    break;
                }
            }
            if (me->GetEntry() != NPC_AMANISHI_FLAME_CASTER)
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_generic_gate_addAI(creature);
    }
};

//69346
class npc_sand_trap : public CreatureScript
{
public:
    npc_sand_trap() : CreatureScript("npc_sand_trap") {}

    struct npc_sand_trapAI : public ScriptedAI
    {
        npc_sand_trapAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }
        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_SAND_TRAP, true);
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
        return new npc_sand_trapAI(creature);
    }
};

//69313
class npc_living_poison : public CreatureScript
{
public:
    npc_living_poison() : CreatureScript("npc_living_poison") {}

    struct npc_living_poisonAI : public ScriptedAI
    {
        npc_living_poisonAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }
        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_LIVING_POISON, true);
            me->GetMotionMaster()->MoveRandom(10.0f);
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
        return new npc_living_poisonAI(creature);
    }
};

//69215
class npc_lightning_nova_totem : public CreatureScript
{
public:
    npc_lightning_nova_totem() : CreatureScript("npc_lightning_nova_totem") {}

    struct npc_lightning_nova_totemAI : public ScriptedAI
    {
        npc_lightning_nova_totemAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }
        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_LIGHTNING_NOVA, true);
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
        return new npc_lightning_nova_totemAI(creature);
    }
};

class spell_horridon_charge : public SpellScriptLoader
{
public:
    spell_horridon_charge() : SpellScriptLoader("spell_horridon_charge") { }
    
    class spell_horridon_charge_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_horridon_charge_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetCaster())
                GetCaster()->CastSpell(GetCaster(), SPELL_DOUBLE_SWIPE);
        }

        void Register()
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_horridon_charge_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        }
    };
    
    AuraScript* GetAuraScript() const
    {
        return new spell_horridon_charge_AuraScript();
    }
};

//136719
class spell_blazing_sunlight : public SpellScriptLoader
{
public:
    spell_blazing_sunlight() : SpellScriptLoader("spell_blazing_sunlight") { }

    class spell_blazing_sunlight_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blazing_sunlight_SpellScript);

        void _FilterTarget(std::list<WorldObject*>&targets)
        {
            if (targets.size() > 1)
                targets.resize(1);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blazing_sunlight_SpellScript::_FilterTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blazing_sunlight_SpellScript::_FilterTarget, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_blazing_sunlight_SpellScript();
    }
};

//136708
class spell_stone_gaze : public SpellScriptLoader
{
public:
    spell_stone_gaze() : SpellScriptLoader("spell_stone_gaze") { }

    class spell_stone_gaze_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_stone_gaze_SpellScript);

        void HandleAfterCast()
        {
            if (GetCaster() && GetCaster()->ToCreature())
                GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_stone_gaze_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_stone_gaze_SpellScript();
    }
};

//136653
class spell_rending_charge : public SpellScriptLoader
{
public:
    spell_rending_charge() : SpellScriptLoader("spell_rending_charge") { }

    class spell_rending_charge_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_rending_charge_SpellScript);

        uint64 targetGuid;

        bool Load()
        {
            targetGuid = 0;
            return true;
        }

        SpellCastResult CheckTarget()
        {
            if (GetExplTargetUnit())
                targetGuid = GetExplTargetUnit()->GetGUID();
            return SPELL_CAST_OK;
        }

        void HandleAfterCast()
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (Unit* _target = GetCaster()->GetUnit(*GetCaster(), targetGuid))
                    if (_target->isAlive())
                        GetCaster()->CastSpell(_target, SPELL_RENDING_CHARGE_DMG, true);
                GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
            }
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_rending_charge_SpellScript::CheckTarget);
            AfterCast += SpellCastFn(spell_rending_charge_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_rending_charge_SpellScript();
    }
};

void AddSC_boss_horridon()
{
    new boss_horridon();
    new boss_jalak();
    new npc_generic_gate_add();
    new npc_sand_trap();
    new npc_living_poison();
    new npc_lightning_nova_totem();
    new spell_horridon_charge();
    new spell_blazing_sunlight();
    new spell_stone_gaze();
    new spell_rending_charge();
}
