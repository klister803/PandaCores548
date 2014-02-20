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
#include "mogu_shan_vault.h"

enum eSpells
{

    // Quiang
    SPELL_FLANKING_ORDERS       = 117910, // Also when vanquished
    SPELL_MASSIVE_ATTACKS       = 117921,
    SPELL_ANNIHILATE            = 117948,
    SPELL_IMPERVIOUS_SHIELD     = 117961, // Heroic

    // Subetai
    SPELL_PILLAGE               = 118049, 
    SPELL_VOLLEY_VISUAL         = 118100,
    SPELL_VOLLEY_1              = 118094,
    SPELL_VOLLEY_2              = 118105,
    SPELL_VOLLEY_3              = 118106,
    SPELL_RAIN_OF_ARROWS        = 118122,
    SPELL_SLEIGHT_OF_HAND       = 118162, // Heroic

    // Zian
    SPELL_UNDYING_SHADOWS       = 117506, // Also when vanquished
    SPELL_FIXATE                = 118303,
    SPELL_UNDYING_SHADOW_DOT    = 117514,
    SPELL_COALESCING_SHADOW_DOT = 117539,

    SPELL_SHADOW_BLAST          = 117628,
    SPELL_CHARGED_SHADOWS       = 117685,
    SPELL_SHIELD_OF_DARKNESS    = 117697, // Heroic

    // Meng
    SPELL_MADDENING_SHOUT       = 117708, // Also when vanquished
    SPELL_CRAZED                = 117737,
    SPELL_COWARDICE             = 117756,
    SPELL_CRAZY_TOUGHT          = 117833,
    SPELL_DELIRIOUS             = 117837, // Heroic

    // Shared
    SPELL_INACTIVE              = 118205,
    SPELL_INACTIVE_STUN         = 118319,
    SPELL_BERSERK               = 120207,
    SPELL_NEXT_SPIRIT_VISUAL    = 118861,

    // Flanking Mogu
    SPELL_GHOST_VISUAL          = 117904,
    SPELL_TRIGGER_ATTACK        = 117917,
};

enum eEvents
{
    // Controler
    EVENT_CHECK_WIPE            = 1,

    // Quiang
    EVENT_FLANKING_MOGU         = 2,
    EVENT_MASSIVE_ATTACK        = 3,
    EVENT_ANNIHILATE            = 4,

    // Subetai
    EVENT_PILLAGE               = 5,
    EVENT_VOLLEY_1              = 6,
    EVENT_VOLLEY_2              = 7,
    EVENT_VOLLEY_3              = 8,
    EVENT_RAIN_OF_ARROWS        = 9,

    // Zian
    EVENT_UNDYING_SHADOWS       = 10,
    EVENT_SHADOW_BLAST          = 11,
    EVENT_CHARGED_SHADOWS       = 12,

    // Meng
    EVENT_MADDENING_SHOUT       = 13,
    EVENT_CRAZED                = 14,
    EVENT_CRAZY_TOUGHT          = 15
};

// This array need for remove some auras
uint32 spiritKingsEntry[4] =
{
    NPC_QIANG,
    NPC_SUBETAI,
    NPC_ZIAN,
    NPC_MENG
};

class boss_spirit_kings_controler : public CreatureScript
{
    public:
        boss_spirit_kings_controler() : CreatureScript("boss_spirit_kings_controler") {}

        struct boss_spirit_kings_controlerAI : public BossAI
        {
            boss_spirit_kings_controlerAI(Creature* creature) : BossAI(creature, DATA_SPIRIT_KINGS)
            {
                pInstance = creature->GetInstanceScript();
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetDisplayId(11686);
            }

            InstanceScript* pInstance;
            bool fightInProgress;
            uint32 spiritkings[3]; //Need for Event
            uint32 spiritkingsvirtual[3]; //Need for finish Event

            void Reset()
            {
                if (pInstance)
                {
                    pInstance->SetBossState(DATA_SPIRIT_KINGS, NOT_STARTED);
                    fightInProgress = false;
                    
                    for (uint8 n = 0; n < 3; n++)
                        spiritkings[n] = 0;
                    
                    for (uint8 n = 0; n < 3; n++)
                        spiritkingsvirtual[n] = 0;
                }
            }
            
            void EnterCombat(Unit* who)
            {
                if (pInstance)
                {
                    PushArrayBoss();
                    pInstance->SetBossState(DATA_SPIRIT_KINGS, IN_PROGRESS);
                    fightInProgress = true;
                    events.ScheduleEvent(EVENT_CHECK_WIPE, 1500);
                }
            }
            
            void PushArrayBoss()
            {
                uint8 pos = urand(0, 5);
                switch (pos)
                {
                case 0:
                    spiritkings[0] = 60710;
                    spiritkings[1] = 60701;
                    spiritkings[2] = 60708;
                    break;
                case 1:
                    spiritkings[0] = 60710;
                    spiritkings[1] = 60708;
                    spiritkings[2] = 60701;
                    break;
                case 2:
                    spiritkings[0] = 60708;
                    spiritkings[1] = 60701;
                    spiritkings[2] = 60710;
                    break;
                case 3:
                    spiritkings[0] = 60708;
                    spiritkings[1] = 60710;
                    spiritkings[2] = 60701;
                    break;
                case 4:
                    spiritkings[0] = 60701;
                    spiritkings[1] = 60708;
                    spiritkings[2] = 60710;
                    break;
                case 5:
                    spiritkings[0] = 60701;
                    spiritkings[1] = 60710;
                    spiritkings[2] = 60708;
                    break;
                }
                spiritkingsvirtual[0] = spiritkings[0];;
                spiritkingsvirtual[1] = spiritkings[1];
                spiritkingsvirtual[2] = NPC_QIANG;
                RestartEvent();
            }

            void RestartEvent()
            {   // Qiang always first
                if (Creature* qiang = me->GetCreature(*me, pInstance->GetData64(NPC_QIANG)))
                {
                    if (!qiang->isAlive())
                        qiang->Respawn();
                    
                    qiang->AI()->DoAction(ACTION_START_FIGHT);                   
                }

                for (uint8 n = 0; n < 3; n++)
                {
                    if (Creature* kings = me->GetCreature(*me, pInstance->GetData64(spiritkings[n])))
                    {
                        if (!kings->isAlive())
                            kings->Respawn();
                    }
                }

                if (Creature* nspirit = me->GetCreature(*me, pInstance->GetData64(spiritkings[0])))
                        nspirit->AddAura(SPELL_NEXT_SPIRIT_VISUAL, nspirit);
            }

            void DoAction(const int32 action)
            {
                if (!pInstance)
                    return;

                switch (action)
                {
                case ACTION_SPIRIT_KILLED:
                    {
                        uint32 nextspirit = 0;
                        for (uint8 n = 0; n < 3; n++)
                        {
                            if (spiritkings[n] != 0)
                            {
                                nextspirit = spiritkings[n];
                                if (nextspirit == spiritkings[2])
                                {
                                    if (Creature* sp = me->GetCreature(*me, pInstance->GetData64(nextspirit)))
                                        sp->AI()->DoAction(ACTION_SPIRIT_LOW_HEALTH);
                                }
                                else
                                {
                                    if (Creature* nspirit = me->GetCreature(*me, pInstance->GetData64(spiritkings[n+1])))
                                        nspirit->AddAura(SPELL_NEXT_SPIRIT_VISUAL, nspirit);
                                }
                                spiritkings[n] = 0;
                                break;
                            }
                        }
                    
                        if (nextspirit)
                        {
                            if (Creature* king = me->GetCreature(*me, pInstance->GetData64(nextspirit)))
                                king->AI()->DoAction(ACTION_START_FIGHT);
                        }                   
                    }
                    break;
                case ACTION_SPIRIT_DONE:
                    for (uint8 i = 0; i < 3; i++)
                    {
                        if (Creature* king = me->GetCreature(*me, pInstance->GetData64(spiritkingsvirtual[i])))
                        {
                            if (king->isAlive())
                                me->Kill(king, true);
                        }
                    }
                    me->DespawnOrUnsummon();
                    pInstance->SetBossState(DATA_SPIRIT_KINGS, DONE);
                    break;
                }
            }
            
            void UpdateAI(const uint32 diff)
            {
                if (!fightInProgress)
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_CHECK_WIPE:
                            {
                                if (pInstance->IsWipe())
                                {
                                    for (uint8 n = 0; n < 4; n++)
                                    {
                                        if (Creature* king = me->GetCreature(*me, pInstance->GetData64(spiritKingsEntry[n])))
                                        {
                                            if (king->isAlive() && king->HasAura(SPELL_NEXT_SPIRIT_VISUAL))
                                                king->RemoveAurasDueToSpell(SPELL_NEXT_SPIRIT_VISUAL);
                                        }
                                    }
                                    EnterEvadeMode();
                                }
                                else
                                    events.ScheduleEvent(EVENT_CHECK_WIPE, 1500);
                            }
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_spirit_kings_controlerAI(creature);
        }
};

class boss_spirit_kings : public CreatureScript
{
    public:
        boss_spirit_kings() : CreatureScript("boss_spirit_kings") {}

        struct boss_spirit_kingsAI : public ScriptedAI
        {
            boss_spirit_kingsAI(Creature* creature) : ScriptedAI(creature), summons(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap   events;
            SummonList summons;
            bool vanquished, lastboss;
            uint8 shadowCount;
            uint8 maxShadowCount;

            void Reset()
            {
                if (me->HasAura(SPELL_NEXT_SPIRIT_VISUAL))
                    me->RemoveAurasDueToSpell(SPELL_NEXT_SPIRIT_VISUAL);
                me->SetReactState(REACT_PASSIVE);
                shadowCount = 0;
                maxShadowCount = 3;
                vanquished = false;
                lastboss = false;
                DoCast(me, SPELL_INACTIVE, true);
                if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE))
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->RemoveAurasDueToSpell(SPELL_CRAZED);
                me->RemoveAurasDueToSpell(SPELL_COWARDICE);
            }

            Creature* GetControler()
            {
                if (pInstance) return pInstance->instance->GetCreature(pInstance->GetData64(NPC_SPIRIT_GUID_CONTROLER)); else return NULL;
            }

            void EnterCombat(Unit* attacker)
            {
                switch (me->GetEntry())
                {
                    case NPC_QIANG:
                        //events.ScheduleEvent(EVENT_FLANKING_MOGU,       30000);
                        events.ScheduleEvent(EVENT_MASSIVE_ATTACK,      3500);
                        events.ScheduleEvent(EVENT_ANNIHILATE,          urand(15000, 20000));
                        break;
                    case NPC_SUBETAI:
                        events.ScheduleEvent(EVENT_PILLAGE,             30000);
                        events.ScheduleEvent(EVENT_VOLLEY_1,            urand(15000, 20000));
                        //events.ScheduleEvent(EVENT_RAIN_OF_ARROWS,      45000);
                        break;
                    case NPC_ZIAN:
                        events.ScheduleEvent(EVENT_UNDYING_SHADOWS,     30000);
                        events.ScheduleEvent(EVENT_SHADOW_BLAST,        15000);
                        events.ScheduleEvent(EVENT_CHARGED_SHADOWS,     10000);
                        break;
                    case NPC_MENG:
                        events.ScheduleEvent(EVENT_MADDENING_SHOUT,     30000);
                        events.ScheduleEvent(EVENT_CRAZED,              5000);
                        events.ScheduleEvent(EVENT_CRAZY_TOUGHT,        10000);
                        break;
                }

                if (me->GetEntry() == NPC_MENG)
                    me->AddAura(SPELL_CRAZED, me);
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_START_FIGHT:
                        if (me->HasAura(SPELL_NEXT_SPIRIT_VISUAL))
                            me->RemoveAurasDueToSpell(SPELL_NEXT_SPIRIT_VISUAL);
                        me->RemoveAurasDueToSpell(SPELL_INACTIVE);
                        me->setFaction(16);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 100.0f);
                        break;
                    case ACTION_SPIRIT_LOW_HEALTH: //Last king in event - need JustDied
                        lastboss = true;
                        break;
                }
            }

            void JustDied(Unit* killer)
            {
                if (Creature* con = GetControler())
                    con->AI()->DoAction(ACTION_SPIRIT_DONE);
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (me->HealthBelowPctDamaged(5, damage) && !lastboss)
                {
                    vanquished = true;
                    damage = 0;

                    if (Creature* controler = GetControler())
                        controler->AI()->DoAction(ACTION_SPIRIT_KILLED);

                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->AddAura(SPELL_INACTIVE, me);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    // We reschedule only the vanquished spell
                  /*events.Reset();
                    switch (me->GetEntry())
                    {
                        case NPC_QIANG:
                            events.ScheduleEvent(EVENT_FLANKING_MOGU, 30000);
                            break;
                        case NPC_SUBETAI:
                            events.ScheduleEvent(EVENT_PILLAGE, 30000);
                            break;
                        case NPC_ZIAN:
                            events.ScheduleEvent(EVENT_UNDYING_SHADOWS, 30000);
                            break;
                        case NPC_MENG:
                            break;
                        default:
                            break;
                    }*/
                }

                if (me->HasAura(SPELL_COWARDICE))
                    if (Aura* aura = me->GetAura(SPELL_COWARDICE))
                    {
                        float charges = aura->GetCharges();
                        me->DealDamage(attacker, damage * (charges / 100));
                    } 
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        // Qiang
                        case EVENT_FLANKING_MOGU: //Not work
                            if (Creature* controler = GetControler())
                                DoCast(me, SPELL_FLANKING_ORDERS);
                                //controler->AI()->DoAction(ACTION_FLANKING_MOGU);
                            events.ScheduleEvent(EVENT_FLANKING_MOGU, 30000);
                            break;
                        case EVENT_MASSIVE_ATTACK:
                            if (me->getVictim())
                                DoCast(me->getVictim(), SPELL_MASSIVE_ATTACKS);
                            events.ScheduleEvent(EVENT_MASSIVE_ATTACK, 3500);
                            break;
                        case EVENT_ANNIHILATE:
                            if (me->getVictim())
                                DoCast(me->getVictim(), SPELL_ANNIHILATE);
                            events.ScheduleEvent(EVENT_ANNIHILATE, urand(15000, 20000));
                            break;
                        // Subetai
                        case EVENT_PILLAGE:
                            if (me->getVictim())
                                DoCast(me->getVictim(), SPELL_PILLAGE);
                            events.ScheduleEvent(EVENT_PILLAGE, 30000);
                            break;
                        case EVENT_VOLLEY_1:
                            DoCast(me, SPELL_VOLLEY_3);
                            events.ScheduleEvent(EVENT_VOLLEY_1, urand(15000, 20000));
                            break;
                        case EVENT_RAIN_OF_ARROWS:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f , true))
                                DoCast(target, SPELL_RAIN_OF_ARROWS);
                            events.ScheduleEvent(EVENT_RAIN_OF_ARROWS, 45000);
                            break;
                        // Zian
                        case EVENT_UNDYING_SHADOWS:
                            if (shadowCount < maxShadowCount) // Max 3 undying shadow during the fight
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 30.0f, true))
                                    DoCast(target, SPELL_UNDYING_SHADOWS);
                            events.ScheduleEvent(EVENT_UNDYING_SHADOWS, 45000);
                            break;
                        case EVENT_SHADOW_BLAST:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                DoCast(target, SPELL_SHADOW_BLAST);
                            events.ScheduleEvent(EVENT_SHADOW_BLAST, 15000);
                            break;
                        case EVENT_CHARGED_SHADOWS:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 30.0f, true))
                                DoCast(target, SPELL_CHARGED_SHADOWS);
                            events.ScheduleEvent(EVENT_CHARGED_SHADOWS, 15000);
                            break;
                        // Meng
                        case EVENT_MADDENING_SHOUT:
                            DoCast(me, SPELL_MADDENING_SHOUT);
                            events.ScheduleEvent(EVENT_MADDENING_SHOUT, 30000);
                            break;
                        case EVENT_CRAZED:
                            DoCast(me, SPELL_CRAZED);
                            break;
                        case EVENT_CRAZY_TOUGHT:
                            DoCast(me, SPELL_CRAZY_TOUGHT);
                            events.ScheduleEvent(EVENT_CRAZY_TOUGHT, 10000);
                            break;
                        default:
                            break;
                    }
                }
                if (!vanquished)
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_spirit_kingsAI(creature);
        }
};

class mob_pinning_arrow : public CreatureScript
{
    public:
        mob_pinning_arrow() : CreatureScript("mob_pinning_arrow") {}

        struct mob_pinning_arrowAI : public ScriptedAI
        {
            mob_pinning_arrowAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            uint64 playerGuid;

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                playerGuid = 0;
            }

            void SetGUID(uint64 guid, int32 /*id*/ = 0)
            {
                playerGuid = guid;

                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                {
                    me->EnterVehicle(player);
                    me->AddAura(118141, me); // Pinnig arrow visual
                }
            }

            void JustDied(Unit* attacker)
            {
                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                    player->RemoveAurasDueToSpell(118135); // DOT
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_pinning_arrowAI(creature);
        }
};

#define PHASE_UNDYING_SHADOW    true
#define PHASE_COALESCING_SHADOW false

class mob_undying_shadow : public CreatureScript
{
    public:
        mob_undying_shadow() : CreatureScript("mob_undying_shadow") {}

        struct mob_undying_shadowAI : public ScriptedAI
        {
            mob_undying_shadowAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            bool phase;
            uint32 switchPhaseTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_UNDYING_SHADOW_DOT, true);
                DoZoneInCombat();

                if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST))
                {
                    me->CastSpell(target, SPELL_FIXATE, true);
                    me->GetMotionMaster()->MoveChase(target);
                }
                switchPhaseTimer = 0;

                phase = PHASE_UNDYING_SHADOW;
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (phase == PHASE_UNDYING_SHADOW)
                {
                    if (damage >= me->GetHealth())
                    {
                        me->RemoveAurasDueToSpell(SPELL_UNDYING_SHADOW_DOT);
                        me->AddAura(SPELL_COALESCING_SHADOW_DOT, me);
                        me->AddUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        phase = PHASE_COALESCING_SHADOW;
                        switchPhaseTimer = 30000;
                        damage = 0;
                    }
                }
                else
                    damage = 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (switchPhaseTimer)
                {
                    if (switchPhaseTimer <= diff)
                    {
                        me->RemoveAurasDueToSpell(SPELL_COALESCING_SHADOW_DOT);
                        me->AddAura(SPELL_UNDYING_SHADOW_DOT, me);
                        me->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        phase = PHASE_UNDYING_SHADOW;
                        switchPhaseTimer = 0;
                        DoZoneInCombat();

                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        {
                            me->CastSpell(target, SPELL_FIXATE, true);
                            me->GetMotionMaster()->MoveChase(target);
                        }
                    }
                    else switchPhaseTimer -= diff;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_undying_shadowAI(creature);
        }
};

class spell_massive_attacks : public SpellScriptLoader
{
    public:
        spell_massive_attacks() : SpellScriptLoader("spell_massive_attacks") { }

        class spell_massive_attacks_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_massive_attacks_SpellScript);

            uint8 targetsCount;

            void CheckTargets(std::list<WorldObject*>& targets)
            {
                targetsCount = targets.size();
            }

            void RecalculateDamage(SpellEffIndex /*effIndex*/)
            {
                SetHitDamage(GetHitDamage() / targetsCount);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_massive_attacks_SpellScript::CheckTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_54);
                OnEffectHitTarget += SpellEffectFn(spell_massive_attacks_SpellScript::RecalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_massive_attacks_SpellScript();
        }
};

class spell_volley : public SpellScriptLoader
{
    public:
        spell_volley() : SpellScriptLoader("spell_volley") { }

        class spell_volley_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_volley_SpellScript);

            void HandleDummyLaunch(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                float coneAngle = 0.0f;

                switch (GetSpellInfo()->Id)
                {
                    case 118094:
                        coneAngle = M_PI / (1.5f);
                        break;
                    case 118105:
                        coneAngle = M_PI / 4;
                        break;
                    case 118106:
                        coneAngle = M_PI / 6;
                        break;
                }

                float startAngle = caster->GetOrientation() - (coneAngle / 2);
                float maxAngle   = caster->GetOrientation() + (coneAngle / 2);

                for (float actualAngle = startAngle; actualAngle <= maxAngle; actualAngle += 0.1f)
                {
                    float x = 0.0f, y = 0.0f;
                    GetPositionWithDistInOrientation(caster, 100.0f, actualAngle, x, y);

                    caster->CastSpell(x, y, caster->GetPositionZ(), SPELL_VOLLEY_VISUAL, true);
                }
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_volley_SpellScript::HandleDummyLaunch, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_volley_SpellScript();
        }
};

class spell_pinned_down : public SpellScriptLoader
{
    public:
        spell_pinned_down() : SpellScriptLoader("spell_pinned_down") { }

        class spell_pinned_down_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pinned_down_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* target = GetHitUnit())
                    if (Creature* pinningArrow = target->SummonCreature(NPC_PINNING_ARROW, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN))
                        pinningArrow->AI()->SetGUID(target->GetGUID());
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_pinned_down_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pinned_down_SpellScript();
        }
};

class spell_maddening_shout : public SpellScriptLoader
{
    public:
        spell_maddening_shout() : SpellScriptLoader("spell_maddening_shout") { }

        class spell_maddening_shout_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_maddening_shout_AuraScript);

            void OnAbsorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                if (Unit* attacker = dmgInfo.GetAttacker())
                    if (attacker->GetTypeId() != TYPEID_PLAYER)
                        absorbAmount = 0;
            }

            void Register()
            {
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_maddening_shout_AuraScript::OnAbsorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_maddening_shout_AuraScript();
        }
};

class spell_crazed_cowardice : public SpellScriptLoader
{
    public:
        spell_crazed_cowardice() : SpellScriptLoader("spell_crazed_cowardice") { }

        class spell_crazed_cowardice_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_crazed_cowardice_AuraScript);

            void HandlePeriodic(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                
                if (Unit* caster = GetCaster())
                {
                    if (Aura* aura = GetAura())
                    {
                        if (aura->GetId() == SPELL_CRAZED)
                            aura->SetStackAmount(aura->GetStackAmount() + 1);
                        else
                            aura->SetCharges(aura->GetCharges() + 1);

                        if (aura->GetStackAmount() >= 100 || aura->GetCharges() >= 100)
                        {
                            caster->CastSpell(caster, aura->GetId() == SPELL_CRAZED ? SPELL_COWARDICE: SPELL_CRAZED, true);
                            aura->Remove();
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_crazed_cowardice_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_crazed_cowardice_AuraScript();
        }
};

class spell_crazy_tought : public SpellScriptLoader
{
    public:
        spell_crazy_tought() : SpellScriptLoader("spell_crazy_tought") { }

        class spell_crazy_tought_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_crazy_tought_SpellScript);

            void HandleEffect(SpellEffIndex effIndex)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Aura* aura = caster->GetAura(SPELL_CRAZED))
                        aura->SetStackAmount(aura->GetStackAmount() + 10);
                    else if (Aura* aura = caster->GetAura(SPELL_COWARDICE))
                        aura->SetCharges(aura->GetCharges() + 10);
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_crazy_tought_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_ENERGIZE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_crazy_tought_SpellScript();
        }
};

void AddSC_boss_spirit_kings()
{
    new boss_spirit_kings_controler();
    new boss_spirit_kings();
    new mob_pinning_arrow();
    new mob_undying_shadow();
    new spell_massive_attacks();
    new spell_volley();
    new spell_pinned_down();
    new spell_maddening_shout();
    new spell_crazed_cowardice();
    new spell_crazy_tought();
}
