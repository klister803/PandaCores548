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

#include "ScriptPCH.h"
#include "heart_of_fear.h"

enum eSpells
{
    //Lord Meljarak
    SPELL_RECKLESSNESS    = 122354,

    //Zarthik spells
    SPELL_HEAL            = 122193,
    SPELL_HEAL_TR_EF      = 122147,
    SPELL_HASTE           = 122149,

    //Korthik spells
    SPELL_KORTHIK_STRIKE  = 122409,
};

enum eEvents
{
    //Soldiers
    EVENT_HEAL            = 1,
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
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_RECKLESSNESS);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

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
            npc_generic_soldierAI(Creature* creature) : ScriptedAI(creature), summons(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            SummonList summons;

            void Reset()
            {
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
                    events.ScheduleEvent(EVENT_HEAL, urand(60000, 120000));
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
                events.ScheduleEvent(EVENT_HEAL, urand(60000, 120000));
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
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

void AddSC_boss_lord_meljarak()
{
    new boss_lord_meljarak();
    new npc_generic_soldier();
}
