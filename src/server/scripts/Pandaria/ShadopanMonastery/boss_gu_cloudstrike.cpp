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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "shadopan_monastery.h"

enum eSpells
{
    // Gu
    SPELL_KILL_GUARDIANS    = 114927,

    SPELL_INVOKE_LIGHTNING  = 106984,
    SPELL_CHARGING_SOUL     = 110945,

    SPELL_OVERCHARGED_SOUL  = 110852,

    // Azure Serpent
    SPELL_LIGHTNING_SHIELD  = 123496,
    SPELL_STATIC_FIELD      = 106923,

    SPELL_LIGHTNING_BREATH  = 102573,
    SPELL_MAGNETIC_SHROUD   = 107140, // TODO
};

enum eEvents
{
    // Gu
    EVENT_INVOKE_LIGHTNING  = 1,

    // Azure Serpent
    EVENT_STATIC_FIELD      = 2,
    EVENT_LIGHTNING_BREATH  = 3,
    EVENT_MAGNETIC_SHROUD   = 4,
};

enum eActions
{
    // Gu
    ACTION_INTRO                = 1,
    ACTION_GU_P_3               = 2,
    
    // Azure Serpent
    ACTION_AZURE_SERPENT_P_1    = 3,
    ACTION_AZURE_SERPENT_P_2    = 4,
    ACTION_AZURE_SERPENT_RESET  = 5,
};

enum ePhases
{
    PHASE_ONE   = 1,
    PHASE_TWO   = 2,
    PHASE_THREE = 3
};

class boss_gu_cloudstrike : public CreatureScript
{
    public:
        boss_gu_cloudstrike() : CreatureScript("boss_gu_cloudstrike") {}

        struct boss_gu_cloudstrikeAI : public BossAI
        {
            boss_gu_cloudstrikeAI(Creature* creature) : BossAI(creature, DATA_GU_CLOUDSTRIKE)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            bool introDone;

            uint8 phase;

            void Reset()
            {
                _Reset();

                phase = 1;
                events.ScheduleEvent(EVENT_INVOKE_LIGHTNING, urand(5000, 10000), PHASE_ONE);

                introDone = false;
            }

            Creature* GetAzureSerpent()
            {
                return pInstance->instance->GetCreature(pInstance->GetData64(NPC_AZURE_SERPENT));
            }

            void JustReachedHome()
            {
                pInstance->SetBossState(DATA_GU_CLOUDSTRIKE, FAIL);
                summons.DespawnAll();
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (phase == 1 && me->HealthBelowPctDamaged(80, damage))
                {
                    phase = 2;
                    events.CancelEventGroup(PHASE_ONE);
                    
                    me->SetReactState(REACT_PASSIVE);
                    me->CastSpell(me, SPELL_CHARGING_SOUL, false);

                    if (Creature* azureSerpent = GetAzureSerpent())
                        if (azureSerpent->AI())
                            azureSerpent->AI()->DoAction(ACTION_AZURE_SERPENT_P_2);
                }
            }

            void DoAction(const int32 action)
            {
                if (action == ACTION_INTRO)
                {
                    if (!introDone)
                    {
                        // Say machin
                        me->CastSpell(me, SPELL_KILL_GUARDIANS);
                        introDone = true;
                    }
                }
                else if (action == ACTION_GU_P_3)
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->CastSpell(me, SPELL_OVERCHARGED_SOUL, true);
                }
            }

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

            void EnterCombat(Unit* who)
            {
                if (Creature* azureSerpent = GetAzureSerpent())
                    if (azureSerpent->AI())
                        azureSerpent->AI()->DoAction(ACTION_AZURE_SERPENT_P_1);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    case EVENT_INVOKE_LIGHTNING:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            me->CastSpell(target, SPELL_INVOKE_LIGHTNING, false);

                        events.ScheduleEvent(EVENT_INVOKE_LIGHTNING, urand(5000, 10000), PHASE_ONE);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_gu_cloudstrikeAI(creature);
        }
};

Position azureSerpentPositions[4] =
{
    {3835.01f, 2906.63f, 753.33f},
    {3850.37f, 2738.14f, 814.84f},
    {3758.79f, 2692.08f, 778.60f},
    {3736.37f, 2680.89f, 772.98f}
};

class npc_azure_serpent : public CreatureScript
{
    public:
        npc_azure_serpent() :  CreatureScript("npc_azure_serpent") { }

        struct npc_azure_serpentAI : public Scripted_NoMovementAI
        {
            npc_azure_serpentAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            uint8 phase;

            void Reset()
            {
                phase = 0;

                me->setActive(true);
                me->SetReactState(REACT_PASSIVE);

                events.Reset();

                me->AddAura(SPELL_LIGHTNING_SHIELD, me);
            }

            Creature* GetGu()
            {
                return pInstance->instance->GetCreature(pInstance->GetData64(NPC_AZURE_SERPENT));
            }

            void JustDied(Unit* /*killer*/)
            {
                if (Creature* gu = GetGu())
                    if (gu->AI())
                        gu->AI()->DoAction(ACTION_GU_P_3);
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_AZURE_SERPENT_P_1:
                        me->GetMotionMaster()->MovePoint(1, azureSerpentPositions[0].GetPositionX(), azureSerpentPositions[0].GetPositionY(), azureSerpentPositions[0].GetPositionZ());
                        break;
                    case ACTION_AZURE_SERPENT_P_2:
                        events.CancelEventGroup(PHASE_ONE);
                        events.ScheduleEvent(EVENT_MAGNETIC_SHROUD, urand(10000, 15000), PHASE_TWO);
                        events.ScheduleEvent(EVENT_LIGHTNING_BREATH, urand (2500, 7500), PHASE_TWO);
                        
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->GetMotionMaster()->MovePoint(4, azureSerpentPositions[4].GetPositionX(), azureSerpentPositions[4].GetPositionY(), azureSerpentPositions[4].GetPositionZ());
                        me->RemoveAurasDueToSpell(SPELL_LIGHTNING_SHIELD);

                        DoZoneInCombat();
                        break;
                    case ACTION_AZURE_SERPENT_RESET:
                        if (!me->isAlive())
                            me->Respawn();

                        me->CombatStop();
                        Reset();
                        me->NearTeleportTo(3829.19f, 2927.29f, 705.71f, 0.0f);
                        break;
                }
            }

            void MovementInform(uint32 uiType, uint32 id)
            {
                if (!id || uiType != POINT_MOTION_TYPE)
                    return;

                if (id < 3)
                {
                    me->GetMotionMaster()->MovePoint(id + 1, azureSerpentPositions[id].GetPositionX(), azureSerpentPositions[id].GetPositionY(), azureSerpentPositions[id].GetPositionZ());
                }
                else if (id == 3)
                {
                    // Movement Finished, stop move start event
                    phase = 1;
                    events.ScheduleEvent(EVENT_STATIC_FIELD, 10000, PHASE_ONE);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!phase)
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    case EVENT_STATIC_FIELD:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            me->CastSpell(target, SPELL_STATIC_FIELD, false);
                        events.ScheduleEvent(EVENT_STATIC_FIELD, 10000, PHASE_ONE);
                        break;
                    case EVENT_MAGNETIC_SHROUD:
                        me->CastSpell(me, SPELL_MAGNETIC_SHROUD, false);
                        events.ScheduleEvent(EVENT_MAGNETIC_SHROUD, urand(10000, 15000), PHASE_TWO);
                        break;
                    case EVENT_LIGHTNING_BREATH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0, 0, true))
                            me->CastSpell(target, SPELL_LIGHTNING_BREATH, false);
                        events.ScheduleEvent(EVENT_LIGHTNING_BREATH, urand(7500, 12500), PHASE_TWO);
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_azure_serpentAI(creature);
        }
};

class AreaTrigger_at_gu_intro : public AreaTriggerScript
{
    public:
        AreaTrigger_at_gu_intro() : AreaTriggerScript("at_gu_intro") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
        {
            if (InstanceScript* pInstance = player->GetInstanceScript())
                if (Creature* gu = pInstance->instance->GetCreature(pInstance->GetData64(NPC_GU_CLOUDSTRIKE)))
                    if (gu->AI())
                        gu->AI()->DoAction(ACTION_INTRO);

            return false;
        }
};

class OnlyGuardianPredicate
{
    public:
        OnlyGuardianPredicate() {}

        bool operator()(WorldObject* target)
        {
            return target->GetEntry() != NPC_GUARDIAN;
        }
};

class spell_kill_guardians : public SpellScriptLoader
{
    public:
        spell_kill_guardians() : SpellScriptLoader("spell_kill_guardians") { }

        class spell_kill_guardians_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_kill_guardians_SpellScript);

            bool Validate(SpellInfo const* spell)
            {
                return true;
            }

            // set up initial variables and check if caster is creature
            // this will let use safely use ToCreature() casts in entire script
            bool Load()
            {
                return true;
            }

            void SelectTarget(std::list<WorldObject*>& targetList)
            {
                targetList.remove_if(OnlyGuardianPredicate());
                _targetList = targetList;
            }

            void KillTarget(std::list<WorldObject*>& targetList)
            {
                targetList = _targetList;
                for (auto itr: targetList)
                {
                    if (Creature* target = itr->ToCreature())
                    {
                        target->Kill(target);
                        target->DespawnOrUnsummon(2000);
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kill_guardians_SpellScript::SelectTarget, EFFECT_0, TARGET_SRC_CASTER);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kill_guardians_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kill_guardians_SpellScript::KillTarget, EFFECT_1, TARGET_SRC_CASTER);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kill_guardians_SpellScript::KillTarget, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            }

            std::list<WorldObject*> _targetList;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_kill_guardians_SpellScript();
        }
};

void AddSC_boss_gu_cloudstrike()
{
    new boss_gu_cloudstrike();
    new npc_azure_serpent();
    new AreaTrigger_at_gu_intro();
    new spell_kill_guardians();
}
