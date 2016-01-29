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
#include "heart_of_fear.h"

enum eSpells
{
    //Lord Meljarak
    SPELL_RECKLESSNESS          = 122354,
    SPELL_RAIN_OF_BLADES        = 122406,
    SPELL_WHIRLING_BLADE        = 121896,
    SPELL_WHIRLING_BLADE_PLR    = 124850,
    SPELL_WHIRLING_BLADE_SUM    = 124851,

    //Zarthik spells
    SPELL_HEAL                  = 122193,
    SPELL_HEAL_TR_EF            = 122147,
    SPELL_HASTE                 = 122149,

    //Korthik spells
    SPELL_KORTHIK_STRIKE        = 122409, //not work 
};

enum eEvents
{
    //Lord Meljarak
    EVENT_RAIN_BLADES           = 1,
    EVENT_WHIRLING_BLADE        = 2,
    EVENT_WHIRLING_BLADE_CAST   = 3,

    //Soldiers
    EVENT_HEAL                  = 1,
    EVENT_HASTE                 = 2,
};

const AuraType auratype[6] = 
{
    SPELL_AURA_MOD_STUN,
    SPELL_AURA_MOD_FEAR,
    SPELL_AURA_MOD_CHARM,
    SPELL_AURA_MOD_CONFUSE,
    SPELL_AURA_MOD_SILENCE,
    SPELL_AURA_TRANSFORM,
};

class boss_lord_meljarak : public CreatureScript
{
    public:
        boss_lord_meljarak() : CreatureScript("boss_lord_meljarak") {}

        struct boss_lord_meljarakAI : public BossAI
        {
            boss_lord_meljarakAI(Creature* creature) : BossAI(creature, DATA_MELJARAK)
            {
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_AGGRESSIVE);
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_RECKLESSNESS);
                me->RemoveAurasDueToSpell(SPELL_HASTE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_RAIN_BLADES, 50000);
                events.ScheduleEvent(EVENT_WHIRLING_BLADE, 35000); //19:05
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
                        case EVENT_RAIN_BLADES:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                                DoCast(target, SPELL_RAIN_OF_BLADES);
                            events.ScheduleEvent(EVENT_RAIN_BLADES, 60000);
                            break;
                        case EVENT_WHIRLING_BLADE:
                            DoCast(SPELL_WHIRLING_BLADE_PLR);
                            events.ScheduleEvent(EVENT_WHIRLING_BLADE_CAST, 500);
                            events.ScheduleEvent(EVENT_WHIRLING_BLADE, 35000);
                            break;
                        case EVENT_WHIRLING_BLADE_CAST:
                            DoCast(SPELL_WHIRLING_BLADE);
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lord_meljarakAI(creature);
        }
};

void SendDamageSoldiers(InstanceScript* instance, Creature* caller, uint32 callerEntry, uint64 callerGuid, uint32 damage)
{
    if (caller && instance)
    {
        switch (callerEntry)
        {
        case NPC_SRATHIK:
            for (uint32 n = NPC_SRATHIK_1; n <= NPC_SRATHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->SetHealth(soldier->GetHealth() - damage);
                }
            }
            break;
        case NPC_ZARTHIK:
            for (uint32 n = NPC_ZARTHIK_1; n <= NPC_ZARTHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->SetHealth(soldier->GetHealth() - damage);
                }
            }
            break;
        case NPC_KORTHIK:
            for (uint32 n = NPC_KORTHIK_1; n <= NPC_KORTHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->SetHealth(soldier->GetHealth() - damage);
                }
            }
            break;
        }
    }
}

void SendDiedSoldiers(InstanceScript* instance, Creature* caller, uint32 callerEntry, uint64 callerGuid)
{
    if (caller && instance)
    {
        switch (callerEntry)
        {
        case NPC_SRATHIK:
            for (uint32 n = NPC_SRATHIK_1; n <= NPC_SRATHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->Kill(soldier, true);
                }
            }
            break;
        case NPC_ZARTHIK:
            for (uint32 n = NPC_ZARTHIK_1; n <= NPC_ZARTHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->Kill(soldier, true);
                }
            }
            break;
        case NPC_KORTHIK:
            for (uint32 n = NPC_KORTHIK_1; n <= NPC_KORTHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->Kill(soldier, true);
                }
            }
            break;
        }

        if (Creature* meljarak = caller->GetCreature(*caller, instance->GetData64(NPC_MELJARAK)))
        {
            if (meljarak->isAlive())
                meljarak->AddAura(SPELL_RECKLESSNESS, meljarak);
        }
    }
}

void SendHealSoldiers(InstanceScript* instance, Creature* caller, uint32 callerEntry, uint64 callerGuid, uint32 modhealth)
{
    if (caller && instance)
    {
        switch (callerEntry)
        {
        case NPC_SRATHIK:
            for (uint32 n = NPC_SRATHIK_1; n <= NPC_SRATHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->SetHealth(soldier->GetHealth() + modhealth);
                }
            }
            break;
        case NPC_ZARTHIK:
            for (uint32 n = NPC_ZARTHIK_1; n <= NPC_ZARTHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->SetHealth(soldier->GetHealth() + modhealth);
                }
            }
            break;
        case NPC_KORTHIK:
            for (uint32 n = NPC_KORTHIK_1; n <= NPC_KORTHIK_3; n++)
            {
                if (Creature* soldier = caller->GetCreature(*caller, instance->GetData64(n)))
                {
                    if (soldier->GetGUID() != callerGuid && soldier->isAlive())
                        soldier->SetHealth(soldier->GetHealth() + modhealth);
                }
            }
            break;
        }
    }
}

class npc_generic_soldier : public CreatureScript
{
    public:
        npc_generic_soldier() : CreatureScript("npc_generic_soldier") {}

        struct npc_generic_soldierAI : public ScriptedAI
        {
            npc_generic_soldierAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
                me->SetReactState(REACT_AGGRESSIVE);
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                me->RemoveAurasDueToSpell(SPELL_HASTE);
            }
            
            void EnterCombat(Unit* attacker)
            {
                if (pInstance)
                {
                    if (Creature* meljarak = me->GetCreature(*me, pInstance->GetData64(NPC_MELJARAK)))
                    {
                        if (meljarak->isAlive() && !meljarak->isInCombat())
                            meljarak->AI()->AttackStart(attacker);
                    }
                }

                switch (me->GetEntry())
                {
                case NPC_SRATHIK:
                    break;
                case NPC_ZARTHIK:
                    events.ScheduleEvent(EVENT_HEAL, urand(60000,  120000));
                    events.ScheduleEvent(EVENT_HASTE, urand(50000, 110000));
                    break;
                case NPC_KORTHIK:
                    break;
                }
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (damage >= me->GetHealth())
                    SendDiedSoldiers(pInstance, me, me->GetEntry(), me->GetGUID());
                else
                    SendDamageSoldiers(pInstance, me, me->GetEntry(), me->GetGUID(), damage);
            }

            bool CheckMeIsInControl()
            {
                for (uint8 n = 0; n < 6; n++)
                {
                    if (me->HasAuraType(auratype[n]))
                        return true;
                }
                return false;
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_HEAL_TR_EF)
                {
                    uint32 modhealth = me->GetMaxHealth()/4;
                    SendHealSoldiers(pInstance, me, me->GetEntry(), me->GetGUID(), modhealth);
                }
            }

            void FindSoldierWithLowHealt()
            {
                if (!CheckMeIsInControl())
                {
                    for (uint32 n = NPC_SRATHIK_1; n <= NPC_KORTHIK_3; n++)
                    {
                        if (Creature* soldier = me->GetCreature(*me, pInstance->GetData64(n)))
                        {
                            if (soldier->GetGUID() != me->GetGUID())
                            {
                                if (soldier->isAlive() && soldier->HealthBelowPct(75))
                                {
                                    DoCast(soldier, SPELL_HEAL);
                                    break;
                                }
                            }
                        }
                    }
                }
                events.ScheduleEvent(EVENT_HEAL, urand(60000, 120000));
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
                    case EVENT_HEAL:
                        if (pInstance)
                            FindSoldierWithLowHealt();
                        break;
                    case EVENT_HASTE:
                        if (!CheckMeIsInControl())
                            DoCast(me, SPELL_HASTE);
                        events.ScheduleEvent(EVENT_HASTE, urand(50000, 110000));
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_generic_soldierAI(creature);
        }
};

//121898
class spell_meljarak_whirling_blade : public SpellScriptLoader
{
    public:
        spell_meljarak_whirling_blade() : SpellScriptLoader("spell_meljarak_whirling_blade") { }

        class spell_meljarak_whirling_blade_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_meljarak_whirling_blade_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                AuraEffect const* aurEff = GetSpell()->GetTriggeredAuraEff();
                if (!aurEff)
                {
                    targets.clear();
                    return;
                }

                uint32 tick = aurEff->GetTickNumber();
                Aura* auraTrigger = aurEff->GetBase();
                Creature* target = caster->FindNearestCreature(63930, 60.0f);

                float distanceintick = 6.0f * tick;
                if (distanceintick > 40.0f)
                    distanceintick = (40.0f * 2) - distanceintick;

                if (distanceintick < 0.0f || !target)
                {
                    targets.clear();
                    return;
                }

                float angle = caster->GetAngle(target);

                // expload at tick
                float x = caster->GetPositionX() + (caster->GetObjectSize() + distanceintick) * std::cos(angle);
                float y = caster->GetPositionY() + (caster->GetObjectSize() + distanceintick) * std::sin(angle);
                Trinity::NormalizeMapCoord(x);
                Trinity::NormalizeMapCoord(y);

                std::list<uint64> saveTargets = auraTrigger->GetEffectTargets();

                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    uint64 guid = (*itr)->GetGUID();
                    bool find = false;
                    if(!saveTargets.empty())
                    {
                        for (std::list<uint64>::iterator itrGuid = saveTargets.begin(); itrGuid != saveTargets.end();)
                        {
                            if(guid == (*itrGuid))
                            {
                                find = true;
                                break;
                            }
                            ++itrGuid;
                        }
                    }
                    if(find || ((*itr)->GetDistance2d(x, y) > 4.0f))
                        targets.erase(itr++);
                    else
                    {
                        auraTrigger->AddEffectTarget(guid);
                        ++itr;
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_meljarak_whirling_blade_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_meljarak_whirling_blade_SpellScript();
        }
};

//121896
class spell_meljarak_whirling_blade_visual : public SpellScriptLoader
{
    public:
        spell_meljarak_whirling_blade_visual() : SpellScriptLoader("spell_meljarak_whirling_blade_visual") { }

        class spell_meljarak_whirling_blade_visual_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_meljarak_whirling_blade_visual_AuraScript)

            Position pos, _ownPos;
            bool check;

            bool Load()
            {
                pos.Relocate(0, 0, 0, 0);
                _ownPos.Relocate(0, 0, 0, 0);
                check = false;
                return true;
            }

            void OnPereodic(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                Unit* caster = GetCaster();
                if (!caster || !check)
                    return;

                uint32 tick = aurEff->GetTickNumber() - 1;

                if (tick == 7)
                {
                    caster->SendMissileCancel(GetSpellInfo()->Effects[2].TriggerSpell);
                    caster->SendMissileCancel(GetSpellInfo()->Effects[3].TriggerSpell);
                    GetAura()->ClearEffectTarget();
                }

                float distanceintick = 6.0f * tick;
                if (distanceintick > 40.0f)
                    distanceintick = (40.0f * 2) - distanceintick;

                if (distanceintick < 0.0f)
                    return;

                // expload at tick
                float x = _ownPos.GetPositionX() + (caster->GetObjectSize() + distanceintick) * std::cos(_ownPos.GetOrientation());
                float y = _ownPos.GetPositionY() + (caster->GetObjectSize() + distanceintick) * std::sin(_ownPos.GetOrientation());
                Trinity::NormalizeMapCoord(x);
                Trinity::NormalizeMapCoord(y);

                caster->CastSpell(x, y, _ownPos.GetPositionZ(), GetSpellInfo()->Effects[0].TriggerSpell, true, NULL, aurEff);
            }

            void HandleApplyEffect(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Creature* target = caster->FindNearestCreature(63930, 60.0f);
                if (!target)
                    return;

                check = true;
                float x, y;
                float angle = caster->GetAngle(target);
                
                caster->GetNearPoint2D(x, y, 40.0f, angle);
                pos.Relocate(x, y, caster->GetPositionZ(), angle);

                GetAura()->SetDuration(uint32(1000.0f * 4));
                _ownPos.Relocate(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), angle);
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->SendMissileCancel(GetSpellInfo()->Effects[2].TriggerSpell);
                    caster->SendMissileCancel(GetSpellInfo()->Effects[3].TriggerSpell);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_meljarak_whirling_blade_visual_AuraScript::OnPereodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                OnEffectApply += AuraEffectApplyFn(spell_meljarak_whirling_blade_visual_AuraScript::HandleApplyEffect, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_meljarak_whirling_blade_visual_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        class spell_meljarak_whirling_blade_visual_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_meljarak_whirling_blade_visual_SpellScript);
 
            void HandleTriggerEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Creature* target = caster->FindNearestCreature(63930, 60.0f);
                if (!target)
                    return;

                float x, y;
                float angle = caster->GetAngle(target);
                caster->GetNearPoint2D(x, y, 40.0f, angle);

                caster->CastSpell(x, y, caster->GetPositionZ(), GetSpellInfo()->Effects[2].TriggerSpell, true);
                caster->CastSpell(x, y, caster->GetPositionZ(), GetSpellInfo()->Effects[3].TriggerSpell, true);
            }
            
            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_meljarak_whirling_blade_visual_SpellScript::HandleTriggerEffect, EFFECT_2, SPELL_EFFECT_TRIGGER_SPELL);
                OnEffectLaunch += SpellEffectFn(spell_meljarak_whirling_blade_visual_SpellScript::HandleTriggerEffect, EFFECT_3, SPELL_EFFECT_TRIGGER_SPELL);
            }
        };
        
        AuraScript* GetAuraScript() const
        {
            return new spell_meljarak_whirling_blade_visual_AuraScript();
        }

        SpellScript* GetSpellScript() const
        {
            return new spell_meljarak_whirling_blade_visual_SpellScript();
        }
};

void AddSC_boss_lord_meljarak()
{
    new boss_lord_meljarak();
    new npc_generic_soldier();
    new spell_meljarak_whirling_blade();
    new spell_meljarak_whirling_blade_visual();
}
