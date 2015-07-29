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

void CheckDone(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (!instance || !caller)
        return;

    if (Creature* council = caller->GetCreature(*caller, instance->GetData64(callerEntry)))
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, council);

    uint8 donecount = 0;
    for (uint8 n = 0; n < 4; n++)
    {
        if (Creature* pcouncil = caller->GetCreature(*caller, instance->GetData64(councilentry[n])))
        {
            if (!pcouncil->isAlive())
                donecount++;
        }
    }

    if (donecount == 4)
        instance->SetBossState(DATA_COUNCIL_OF_ELDERS, DONE);
    else
        caller->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
}

class boss_council_of_elders : public CreatureScript
{
    public:
        boss_council_of_elders() : CreatureScript("boss_council_of_elders") {}

        struct boss_council_of_eldersAI : public ScriptedAI
        {
            boss_council_of_eldersAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            Position chargepos;
            EventMap events;

            void Reset()
            {
                if (instance)
                    instance->SetBossState(DATA_COUNCIL_OF_ELDERS, NOT_STARTED);
                events.Reset();
                me->RemoveAurasDueToSpell(SPELL_R_CHARGE_VISUAL);
                if (me->GetEntry() == NPC_KAZRAJIN)
                    me->SetReactState(REACT_PASSIVE);
                else
                    me->SetReactState(REACT_DEFENSIVE);
            }

            void EnterCombat(Unit* who)
            {
                if (instance)
                    instance->SetBossState(DATA_COUNCIL_OF_ELDERS, IN_PROGRESS);

                switch (me->GetEntry())
                {
                case NPC_FROST_KING_MALAKK:
                    events.ScheduleEvent(EVENT_FRIGIT_ASSAULT, 15000);
                    events.ScheduleEvent(EVENT_BITTING_COLD,   30000);
                    break;
                case NPC_SUL_SANDCRAWLER:
                    events.ScheduleEvent(EVENT_SAND_BOLT,      35000);
                    break;
                case NPC_KAZRAJIN:
                    events.ScheduleEvent(EVENT_R_CHARGE,        6000);
                    break;
                default:
                    break;
                }
            }
            
            void JustDied(Unit* /*killer*/)
            {
                CheckDone(instance, me, me->GetEntry());
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
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

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
                    {
                        if (GetTarget()->GetAura(SPELL_FRIGIT_ASSAULT_D)->GetStackAmount() == 15)
                            GetTarget()->AddAura(SPELL_FRIGIT_ASSAULT_S, GetTarget());
                    }
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


void AddSC_boss_council_of_elders()
{
    new boss_council_of_elders();
    new spell_frigit_assault();
}
