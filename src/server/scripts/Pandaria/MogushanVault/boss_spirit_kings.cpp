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
#include "mogu_shan_vault.h"

enum eSpells
{

    // Quiang
    SPELL_FLANKING_ORDERS       = 117910, // Also when vanquished
    SPELL_MASSIVE_ATTACKS       = 117921,
    SPELL_ANNIHILATE            = 117948,
    SPELL_IMPERVIOUS_SHIELD     = 117961, // Heroic

    // Subetai
    SPELL_PILLAGE               = 118047, // Also when vanquished
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

#define MAX_FLANKING_MOGU   48

Position flankingPos[MAX_FLANKING_MOGU] =
{
    {4195.25f, 1590.85f, 438.841f, 0.12350f},
    {4194.99f, 1586.06f, 438.841f, 0.26639f},
    {4193.99f, 1581.04f, 438.840f, 0.39482f},
    {4195.19f, 1576.21f, 438.840f, 0.52845f},
    {4196.93f, 1571.99f, 438.839f, 0.64866f},
    {4200.06f, 1568.31f, 438.839f, 0.77664f},
    {4203.17f, 1564.49f, 438.837f, 0.90416f},
    {4206.52f, 1561.59f, 438.837f, 1.01808f},
    {4209.94f, 1558.79f, 438.836f, 1.12821f},
    {4213.91f, 1556.73f, 438.837f, 1.23942f},
    {4217.99f, 1556.02f, 438.839f, 1.34239f},
    {4222.20f, 1555.52f, 438.839f, 1.44913f},
    {4226.90f, 1555.01f, 438.839f, 1.56812f},
    {4231.69f, 1555.00f, 438.839f, 1.68814f},
    {4236.49f, 1556.31f, 438.838f, 1.81247f},
    {4241.28f, 1558.22f, 438.837f, 1.94289f},
    {4245.75f, 1561.11f, 438.837f, 2.07869f},
    {4250.17f, 1563.56f, 438.837f, 2.20908f},
    {4253.08f, 1568.12f, 438.839f, 2.34491f},
    {4255.07f, 1573.32f, 438.840f, 2.48865f},
    {4257.26f, 1579.05f, 438.840f, 2.66189f},
    {4257.36f, 1584.80f, 438.841f, 2.82356f},
    {4258.93f, 1591.29f, 438.841f, 3.03247f},
    {4259.81f, 1597.36f, 438.841f, 3.21968f},
    {4265.56f, 1603.04f, 438.842f, 3.35247f},
    {4264.61f, 1608.74f, 438.843f, 3.49683f},
    {4263.33f, 1613.64f, 438.841f, 3.62029f},
    {4260.61f, 1618.46f, 438.842f, 3.75534f},
    {4257.08f, 1622.41f, 438.845f, 3.88442f},
    {4253.70f, 1625.77f, 438.845f, 4.00111f},
    {4250.06f, 1629.08f, 438.844f, 4.12039f},
    {4245.74f, 1631.66f, 438.843f, 4.24225f},
    {4241.14f, 1634.23f, 438.846f, 4.36825f},
    {4236.42f, 1635.51f, 438.847f, 4.48507f},
    {4231.43f, 1635.73f, 438.849f, 4.60476f},
    {4226.52f, 1635.65f, 438.851f, 4.72422f},
    {4221.75f, 1635.25f, 438.851f, 4.84165f},
    {4217.01f, 1633.81f, 438.853f, 4.96304f},
    {4211.88f, 1632.07f, 438.853f, 5.09767f},
    {4207.12f, 1629.80f, 438.851f, 5.22899f},
    {4202.68f, 1626.57f, 438.851f, 5.36565f},
    {4198.73f, 1623.16f, 438.850f, 5.49605f},
    {4195.37f, 1619.05f, 438.849f, 5.62901f},
    {4192.82f, 1614.57f, 438.849f, 5.75861f},
    {4190.84f, 1609.64f, 438.847f, 5.89364f},
    {4190.58f, 1604.28f, 438.850f, 6.02825f},
    {4192.61f, 1600.68f, 438.844f, 6.11362f},
    {4195.38f, 1595.63f, 438.841f, 6.25653f}
};

uint32 spiritKingsEntry[4] =
{
    NPC_QIANG,
    NPC_SUBETAI,
    NPC_ZIAN,
    NPC_MENG
};

Position spiritKingsPos[4] =
{
    {4226.33f, 1626.28f, 438.856f, 4.72348f},
    {4257.35f, 1591.36f, 438.841f, 3.13526f},
    {4226.97f, 1558.32f, 438.804f, 1.58495f},
    {4198.78f, 1590.29f, 438.841f, 6.26345f}
};

uint32 volleySpells[3] =
{
    SPELL_VOLLEY_1,
    SPELL_VOLLEY_2,
    SPELL_VOLLEY_3
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
            }

            InstanceScript* pInstance;
            uint64 flankingGuid[MAX_FLANKING_MOGU];

            bool fightInProgress;

            uint8 vanquishedCount;

            void Reset()
            {
                _Reset();
                fightInProgress = false;
                vanquishedCount = 0;

                for (uint8 i = 0; i < MAX_FLANKING_MOGU; ++i)
                    flankingGuid[i] = 0;

                spawnSpiritKings();
                me->SetReactState(REACT_PASSIVE);

                events.ScheduleEvent(EVENT_CHECK_WIPE, 2500);
            }

            void spawnSpiritKings()
            {
                for (uint8 i = 0; i < 4; ++i)
                    if (Creature* king = me->SummonCreature(spiritKingsEntry[i], spiritKingsPos[i].GetPositionX(), spiritKingsPos[i].GetPositionY(), spiritKingsPos[i].GetPositionZ(), spiritKingsPos[i].GetOrientation()))
                        if (i == 0) // No more random, qiang is alway the first one
                            king->AI()->DoAction(ACTION_FIRST_FIGHT);
            }

            void spawnFlankingMogu(uint8 moguNumber)
            {
                if (moguNumber >= MAX_FLANKING_MOGU)
                    return;

                if (Creature* flankingMogu = me->SummonCreature(NPC_FLANKING_MOGU, flankingPos[moguNumber].GetPositionX(), flankingPos[moguNumber].GetPositionY(), flankingPos[moguNumber].GetPositionZ(), flankingPos[moguNumber].GetOrientation()))
                {
                    flankingGuid[moguNumber] = flankingMogu->GetGUID();
                    flankingMogu->SetReactState(REACT_PASSIVE);
                    flankingMogu->AddAura(SPELL_GHOST_VISUAL, flankingMogu);
                }
            }

            void DoAction(const int32 action)
            {
                if (!pInstance)
                    return;

                switch (action)
                {
                    case ACTION_ENTER_COMBAT:
                    {
                        for (uint8 i = 0; i < MAX_FLANKING_MOGU; ++i)
                            spawnFlankingMogu(i);

                        fightInProgress = true;
                        break;
                    }
                    case ACTION_FLANKING_MOGU:
                    {
                        uint8 moguBegin = urand(0, 47);
                        uint8 middleMogu = (moguBegin + 3) > 47 ? (moguBegin + 3) - 48: (moguBegin + 3);
                        float orientation = 0.0f;

                        if (Creature* flankingMogu = pInstance->instance->GetCreature(flankingGuid[middleMogu]))
                            orientation = flankingMogu->GetOrientation();

                        for (uint8 i = moguBegin; i < moguBegin + 6; ++i)
                        {
                            uint8 j = i;

                            if (j >= 48)
                                j -= 48;

                            if (Creature* flankingMogu = pInstance->instance->GetCreature(flankingGuid[j]))
                            {
                                flankingMogu->RemoveAurasDueToSpell(SPELL_GHOST_VISUAL);
                                flankingMogu->AddAura(SPELL_TRIGGER_ATTACK, flankingMogu);

                                float x = 0.0f, y = 0.0f;
                                GetPositionWithDistInOrientation(flankingMogu, 100.0f, orientation, x, y);

                                if (!x && !y)
                                    continue;

                                flankingMogu->GetMotionMaster()->MovePoint(1, x, y, flankingMogu->GetPositionZ());
                                flankingMogu->DespawnOrUnsummon(15000);

                                // Spawn a new one to remplace the old one
                                spawnFlankingMogu(j);
                            }
                        }
                        break;
                    }
                    case ACTION_SPIRIT_LOW_HEALTH:
                    {
                        uint8 nextSpirit = vanquishedCount + 1;
                        if (nextSpirit >= 4)
                            break;

                        if (Creature* spirit = pInstance->instance->GetCreature(pInstance->GetData64(spiritKingsEntry[nextSpirit])))
                            spirit->AI()->DoAction(ACTION_SPIRIT_LOW_HEALTH);

                        break;
                    }
                    case ACTION_SPIRIT_KILLED:
                    {
                        uint8 nextSpirit = ++vanquishedCount;
                        if (nextSpirit >= 4)
                        {
                            pInstance->SetBossState(DATA_SPIRIT_KINGS, DONE);
                            summons.DespawnEntry(NPC_FLANKING_MOGU);

                            for (auto entry: spiritKingsEntry)
                            {
                                if (Creature* spirit = pInstance->instance->GetCreature(pInstance->GetData64(entry)))
                                {
                                    spirit->LowerPlayerDamageReq(spirit->GetMaxHealth());
                                    me->Kill(spirit);
                                }
                            }
                        }
                        else
                        {
                            if (Creature* spirit = pInstance->instance->GetCreature(pInstance->GetData64(spiritKingsEntry[nextSpirit])))
                                spirit->AI()->DoAction(ACTION_ENTER_COMBAT);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void UpdateAI(const uint32 diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

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
                                pInstance->SetBossState(DATA_SPIRIT_KINGS, FAIL);
                                Reset();
                            }
                            else
                                events.ScheduleEvent(EVENT_CHECK_WIPE, 2500);
                            break;
                        }
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
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

            bool vanquished;

            uint8 shadowCount;
            uint8 maxShadowCount;

            void Reset()
            {
                shadowCount = 0;
                maxShadowCount = 3;

                vanquished = false;
                me->CastSpell(me, SPELL_INACTIVE, true);
                me->CastSpell(me, SPELL_INACTIVE_STUN, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                
                me->RemoveAurasDueToSpell(SPELL_CRAZED);
                me->RemoveAurasDueToSpell(SPELL_COWARDICE);

                switch (me->GetEntry())
                {
                    case NPC_QIANG:
                        DoAction(ACTION_FIRST_FIGHT);
                        events.ScheduleEvent(EVENT_FLANKING_MOGU,       30000);
                        events.ScheduleEvent(EVENT_MASSIVE_ATTACK,      3500);
                        events.ScheduleEvent(EVENT_ANNIHILATE,          urand(15000, 20000));
                        break;
                    case NPC_SUBETAI:
                        events.ScheduleEvent(EVENT_PILLAGE,             30000);
                        events.ScheduleEvent(EVENT_VOLLEY_1,            urand(15000, 20000));
                        events.ScheduleEvent(EVENT_RAIN_OF_ARROWS,      45000);
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
                    default:
                        break;
                }
            }

            Creature* GetControler()
            {
                if (pInstance) return pInstance->instance->GetCreature(pInstance->GetData64(NPC_SPIRIT_GUID_CONTROLER)); else return NULL;
            }

            void EnterCombat(Unit* attacker)
            {
                if (pInstance)
                    pInstance->SetBossState(DATA_SPIRIT_KINGS, IN_PROGRESS);

                if (me->GetEntry() == NPC_MENG)
                    me->AddAura(SPELL_CRAZED, me);
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_ENTER_COMBAT:
                        if (!me->isInCombat())
                            if (Player* victim = me->SelectNearestPlayerNotGM(50.0f))
                                AttackStart(victim);
                    // No Break
                    case ACTION_FIRST_FIGHT:
                        me->RemoveAurasDueToSpell(SPELL_INACTIVE);
                        me->RemoveAurasDueToSpell(SPELL_INACTIVE_STUN);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        me->setFaction(14);
                        break;
                    case ACTION_SPIRIT_LOW_HEALTH: // Previous spirit is low hp, add preparation aura
                    {
                        //me->AddAura(SPELL_SPIRIT_PREPARATION, me);
                        break;
                    }
                    default:
                        break;
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);

                if (summon->GetEntry() == NPC_UNDYING_SHADOW)
                    ++shadowCount;
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);

                if (summon->GetEntry() == NPC_UNDYING_SHADOW)
                    --shadowCount;
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (me->HealthBelowPctDamaged(5, damage))
                {
                    vanquished = true;
                    damage = 0;

                    if (Creature* controler = GetControler())
                        controler->AI()->DoAction(ACTION_SPIRIT_KILLED);

                    me->AddAura(SPELL_INACTIVE, me);
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                    me->getThreatManager().resetAllAggro();
                    me->SetSpeed(MOVE_RUN, 0.0f, true);
                    me->SetSpeed(MOVE_WALK, 0.0f, true);

                    // We reschedule only the vanquished spell
                    events.Reset();
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
                    }
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
                        case EVENT_FLANKING_MOGU:
                        {
                            if (Creature* controler = GetControler())
                            {
                                me->CastSpell(me, SPELL_FLANKING_ORDERS, false);
                                controler->AI()->DoAction(ACTION_FLANKING_MOGU);
                            }
                            events.ScheduleEvent(EVENT_FLANKING_MOGU, 30000);
                            break;
                        }
                        case EVENT_MASSIVE_ATTACK:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_MASSIVE_ATTACKS, false);

                            events.ScheduleEvent(EVENT_MASSIVE_ATTACK, 3500);
                            break;
                        }
                        case EVENT_ANNIHILATE:
                        {
                            me->CastSpell(me, SPELL_ANNIHILATE, false);
                            events.ScheduleEvent(EVENT_ANNIHILATE, urand(15000, 20000));
                            break;
                        }
                        // Subetai
                        case EVENT_PILLAGE:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                me->CastSpell(target, SPELL_PILLAGE, false);

                            events.ScheduleEvent(EVENT_PILLAGE, 30000);
                            break;
                        }
                        case EVENT_VOLLEY_1:
                        case EVENT_VOLLEY_2:
                        case EVENT_VOLLEY_3:
                        {
                            me->CastSpell(me, volleySpells[eventId - EVENT_VOLLEY_1], false);
                            events.ScheduleEvent(eventId == EVENT_VOLLEY_3 ? EVENT_VOLLEY_1: eventId + 1, eventId == EVENT_VOLLEY_3 ? urand(15000, 20000): 50);
                            break;
                        }
                        case EVENT_RAIN_OF_ARROWS:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 2))
                                me->CastSpell(target, SPELL_RAIN_OF_ARROWS, false);

                            events.ScheduleEvent(EVENT_RAIN_OF_ARROWS, 45000);
                            break;
                        }
                        // Zian
                        case EVENT_UNDYING_SHADOWS:
                        {
                            if (shadowCount < maxShadowCount) // Max 3 undying shadow during the fight
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 2))
                                    me->CastSpell(target, SPELL_UNDYING_SHADOWS, false);

                            events.ScheduleEvent(EVENT_UNDYING_SHADOWS, 45000);
                            break;
                        }
                        case EVENT_SHADOW_BLAST:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 2))
                                me->CastSpell(target, SPELL_SHADOW_BLAST, false);

                            events.ScheduleEvent(EVENT_SHADOW_BLAST, 15000);
                            break;
                        }
                        case EVENT_CHARGED_SHADOWS:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 2))
                                me->CastSpell(target, SPELL_CHARGED_SHADOWS, false);

                            events.ScheduleEvent(EVENT_CHARGED_SHADOWS, 15000);
                            break;
                        }
                        // Meng
                        case EVENT_MADDENING_SHOUT:
                        {
                            me->CastSpell(me, SPELL_MADDENING_SHOUT, false);
                            events.ScheduleEvent(EVENT_MADDENING_SHOUT, 30000);
                            break;
                        }
                        case EVENT_CRAZED:
                        {
                            me->CastSpell(me, SPELL_CRAZED, false);
                            break;
                        }
                        case EVENT_CRAZY_TOUGHT:
                        {
                            me->CastSpell(me, SPELL_CRAZY_TOUGHT, false);
                            events.ScheduleEvent(EVENT_CRAZY_TOUGHT, 10000);
                            break;
                        }
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
