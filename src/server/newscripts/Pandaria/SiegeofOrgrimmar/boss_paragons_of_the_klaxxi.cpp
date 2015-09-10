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
    SPELL_READY_TO_FIGHT        = 143542,

    //Skeer
    SPELL_HEVER_OF_FOES         = 143273,
    SPELL_BLODDLETTING          = 143280,
    SPELL_BLODDLETTING_BUFF     = 143320,

    //Rikkal
    SPELL_MAD_SCIENTIST_AURA    = 143277,
    SPELL_INJECTION             = 143339,


    //Kilruk
    SPELL_RAZOR_SHARP_BLADES    = 142918,
    SPELL_GOUGE                 = 143939,
    SPELL_MUTILATE              = 143941,
    SPELL_REAVE_PRE             = 148677,
    SPELL_REAVE                 = 148681,

    //Xaril
    SPELL_TENDERIZING_STRIKES   = 142927,

    //Special
    SPELL_AURA_VISUAL_FS        = 143548, 
    SPELL_AURA_ENRAGE           = 146983,
    SPELL_ENRAGE                = 146982,
    SPELL_PARAGONS_PURPOSE_HEAL = 143483,
    SPELL_PARAGONS_PURPOSE_DMG  = 143482,

    //Buffs
    //Rikkal
    SPELL_GENE_SPLICE           = 143372,
    SPELL_MAD_SCIENTIST         = 141857,
    //Hisek
    SPELL_COMPOUND_EYE          = 141852,
    //
};

enum sEvents
{
    EVENT_START_KLAXXI          = 1,
    EVENT_CHECK                 = 2,
};

enum sActions
{
    ACTION_KLAXXI_START         = 1,
};

uint32 removeaurasentry[4] =
{
    SPELL_READY_TO_FIGHT,
    SPELL_AURA_ENRAGE,
    SPELL_ENRAGE,
    SPELL_PARAGONS_PURPOSE_DMG,
};

//71628
class npc_amber_piece : public CreatureScript
{
public:
    npc_amber_piece() : CreatureScript("npc_amber_piece") {}

    struct npc_amber_pieceAI : public ScriptedAI
    {
        npc_amber_pieceAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_AURA_VISUAL_FS, true);
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}

        void OnSpellClick(Unit* clicker)
        {
            if (me->HasAura(SPELL_AURA_VISUAL_FS))
            {
                me->RemoveAurasDueToSpell(SPELL_AURA_VISUAL_FS);
                if (Creature* ck = me->GetCreature(*me, instance->GetData64(NPC_KLAXXI_CONTROLLER)))
                    ck->AI()->DoAction(ACTION_KLAXXI_START);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amber_pieceAI(creature);
    }
};

//71592
class npc_klaxxi_controller : public CreatureScript
{
public:
    npc_klaxxi_controller() : CreatureScript("npc_klaxxi_controller") {}

    struct npc_klaxxi_controllerAI : public ScriptedAI
    {
        npc_klaxxi_controllerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            if (instance && instance->GetBossState(DATA_KLAXXI) != DONE)
                instance->SetBossState(DATA_KLAXXI, NOT_STARTED);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void DoAction(int32 const action)
        {
            switch (action)
            {
            case ACTION_KLAXXI_START:
                events.ScheduleEvent(EVENT_CHECK, 1000);
                events.ScheduleEvent(EVENT_START_KLAXXI, 5000);
                break;
            case ACTION_KLAXXI_DONE:
                events.Reset();
                if (Creature* ap = me->GetCreature(*me, instance->GetData64(NPC_AMBER_PIECE)))
                    me->Kill(ap, true);
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
                case EVENT_START_KLAXXI:
                    instance->SetBossState(DATA_KLAXXI, IN_PROGRESS);
                    break;
                case EVENT_CHECK:
                    if (instance->IsWipe())
                        instance->SetBossState(DATA_KLAXXI, NOT_STARTED);
                    else
                        events.ScheduleEvent(EVENT_CHECK, 1000);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_klaxxi_controllerAI(creature);
    }
};

class boss_paragons_of_the_klaxxi : public CreatureScript
{
    public:
        boss_paragons_of_the_klaxxi() : CreatureScript("boss_paragons_of_the_klaxxi") {}

        struct boss_paragons_of_the_klaxxiAI : public ScriptedAI
        {
            boss_paragons_of_the_klaxxiAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->SetDisableGravity(true);
                me->SetCanFly(true);
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
                for (uint8 n = 0; n < 4; n++)
                    me->RemoveAurasDueToSpell(removeaurasentry[n]);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                switch (me->GetEntry())
                {
                case NPC_SKEER:
                case NPC_RIKKAL:
                case NPC_HISEK:
                    DoCast(me, SPELL_READY_TO_FIGHT, true); 
                    break;
                default:
                    break;
                }
            }

            void EnterCombat(Unit* who)
            {
                switch (me->GetEntry())
                {
                case NPC_KILRUK:
                    DoCast(me, SPELL_RAZOR_SHARP_BLADES, true);
                    break;
                case NPC_XARIL:
                    DoCast(me, SPELL_TENDERIZING_STRIKES, true);
                    break;
                case NPC_SKEER:
                    DoCast(me, SPELL_HEVER_OF_FOES, true);
                    break;
                case NPC_RIKKAL:
                    DoCast(me, SPELL_MAD_SCIENTIST_AURA, true);
                    break;
                case NPC_KAZTIK:
                case NPC_KORVEN:
                case NPC_IYYOKYK:
                case NPC_KAROZ:
                case NPC_HISEK:
                    break;
                }
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_KLAXXI_IN_PROGRESS:
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    me->GetMotionMaster()->MoveJump(1582.4f, -5684.9f, -313.635f, 15.0f, 15.0f, 1);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    break;
                }
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type == EFFECT_MOTION_TYPE)
                {
                    if (pointId == 1)
                    {
                        me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        me->RemoveAurasDueToSpell(SPELL_READY_TO_FIGHT);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f); 
                        instance->SetData(DATA_BUFF_NEXT_KLAXXI, 0);
                    }
                }
            }

            void JustDied(Unit* killer)
            {
                if (killer->ToPlayer() && instance)
                {
                    if (instance->GetData(DATA_SEND_KLAXXI_DIE_COUNT) < 8)
                    {
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        me->SetLootRecipient(NULL);
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        instance->SetData(DATA_INTRO_NEXT_KLAXXI, 0);
                    }
                    else
                        instance->SetBossState(DATA_KLAXXI, DONE);
                }
            }

            void OnSpellClick(Unit* clicker)
            {
                if (Player* pl = clicker->ToPlayer())
                {
                    switch (me->GetEntry())
                    {
                    case NPC_RIKKAL:
                        pl->CastSpell(pl, SPELL_MAD_SCIENTIST, true);
                        break;
                    case NPC_HISEK:
                        if (pl->GetRoleForGroup(pl->GetSpecializationId(pl->GetActiveSpec())) == ROLES_DPS)
                            pl->CastSpell(pl, SPELL_COMPOUND_EYE, true);
                        break;
                    default:
                        break;
                    }
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                }
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_paragons_of_the_klaxxiAI(creature);
        }
};

//143939
class spell_klaxxi_gouge: public SpellScriptLoader
{
public:
    spell_klaxxi_gouge() : SpellScriptLoader("spell_klaxxi_gouge") { }

    class spell_klaxxi_gouge_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_klaxxi_gouge_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetCaster() && GetTarget())
                GetCaster()->CastSpell(GetTarget(), SPELL_MUTILATE, false);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_klaxxi_gouge_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_POSSESS_PET, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_klaxxi_gouge_AuraScript();
    }
};

//143373
class spell_gene_splice : public SpellScriptLoader
{
public:
    spell_gene_splice() : SpellScriptLoader("spell_gene_splice") { }

    class spell_gene_splice_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gene_splice_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTarget())
            {
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
                {
                    GetTarget()->RemoveAurasDueToSpell(SPELL_GENE_SPLICE);
                    GetTarget()->RemoveAurasDueToSpell(SPELL_MAD_SCIENTIST);
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gene_splice_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_gene_splice_AuraScript();
    }
};

//143217
class spell_snipe : public SpellScriptLoader
{
public:
    spell_snipe() : SpellScriptLoader("spell_snipe") { }

    class spell_snipe_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_snipe_SpellScript);

        void DealDamage()
        {
            if (GetCaster() && GetHitUnit())
            {
                float distance = GetCaster()->GetExactDist2d(GetHitUnit());
                if (distance && distance <= 100)
                    SetHitDamage(GetHitDamage() * (distance / 100));
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_snipe_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_snipe_SpellScript();
    }
};

void AddSC_boss_paragons_of_the_klaxxi()
{
    new npc_amber_piece();
    new npc_klaxxi_controller();
    new boss_paragons_of_the_klaxxi();
    new spell_klaxxi_gouge();
    new spell_gene_splice();
    new spell_snipe();
}
