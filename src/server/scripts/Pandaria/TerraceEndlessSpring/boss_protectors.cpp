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
#include "terrace_of_endless_spring.h"

enum eSpells
{
    //All
    SPELL_SHA_CORRUPTION                = 117052,
    //Kaolan
    SPELL_TOUCH_OF_SHA                  = 117519,
    //Regail
    SPELL_LIGHTNING_BOLT                = 117187,
    SPELL_OVERWHELMING_CORRUPTION       = 117351,
    SPELL_OVERWHELMING_CORRUPTION_TR_EF = 117353,
    //Asani
    SPELL_WATER_BOLT                    = 118312,
};

enum eEvents
{
    //Kaolan
    EVENT_TOUCH_OF_SHA                  = 1,
    //Regail
    EVENT_LIGHTNING_BOLT                = 2,
    //Asani
    EVENT_WATER_BOLT                    = 3,
};

uint32 protectorsEntry[3] =
{
    NPC_PROTECTOR_KAOLAN,
    NPC_ELDER_REGAIL,
    NPC_ELDER_ASANI,
};

void ResetBosses(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* protector = caller->GetCreature(*caller, instance->GetData64(protectorsEntry[n])))
            {
                if (protector->GetEntry() != callerEntry)
                {
                    if (!protector->isAlive())
                    {
                        protector->Respawn();
                        protector->GetMotionMaster()->MoveTargetedHome();
                    }
                    else if (protector->isAlive() && protector->isInCombat())
                        protector->AI()->EnterEvadeMode();
                }
            }
        }
    }
}

void CallBosses(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* protector = caller->GetCreature(*caller, instance->GetData64(protectorsEntry[n])))
            {
                if (protector->GetEntry() != callerEntry)
                {
                    if (protector->isAlive() && !protector->isInCombat())
                        protector->AI()->DoZoneInCombat(protector, 150.0f);
                }
            }
        }
    }
}

void CallDieControl(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* protector = caller->GetCreature(*caller, instance->GetData64(protectorsEntry[n])))
            {
                if (protector->GetEntry() != callerEntry)
                {
                    if (protector->isAlive())
                    {
                        protector->SetFullHealth();
                        protector->CastSpell(protector, SPELL_SHA_CORRUPTION);
                    }
                }
            }
        }
        if (callerEntry == NPC_PROTECTOR_KAOLAN)
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TOUCH_OF_SHA);
    }
}

uint8 CalcAliveBosses(InstanceScript* instance, Creature* caller)
{
    uint8 abossval = 0;

    if (caller && instance)
    {
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* protector = caller->GetCreature(*caller, instance->GetData64(protectorsEntry[n])))
            {
                if (protector->isAlive())
                    abossval++;
            }
        }
    }
    return abossval;
}

class boss_protectors : public CreatureScript
{
    public:
        boss_protectors() : CreatureScript("boss_protectors") {}

        struct boss_protectorsAI : public ScriptedAI
        {
            boss_protectorsAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
                me->SetReactState(REACT_DEFENSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->RemoveAurasDueToSpell(SPELL_SHA_CORRUPTION);
                me->RemoveAurasDueToSpell(SPELL_OVERWHELMING_CORRUPTION);
                if (instance)
                {
                    ResetBosses(instance, me, me->GetEntry());
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TOUCH_OF_SHA);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OVERWHELMING_CORRUPTION_TR_EF);
                }
            }

            void EnterCombat(Unit* who)
            {
                if (instance)
                    CallBosses(instance, me, me->GetEntry());

                switch (me->GetEntry())
                {
                case NPC_PROTECTOR_KAOLAN:
                    events.ScheduleEvent(EVENT_TOUCH_OF_SHA,   urand(30000, 60000));
                    break;
                case NPC_ELDER_REGAIL:
                    events.ScheduleEvent(EVENT_LIGHTNING_BOLT, urand(10000, 20000));
                    break;
                case NPC_ELDER_ASANI:
                    events.ScheduleEvent(EVENT_WATER_BOLT,     urand(10000, 20000));
                    break;
                }
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_SHA_CORRUPTION)
                {
                    if (me->HasAura(SPELL_SHA_CORRUPTION))//for safe
                    {
                        if (uint8 stack = me->GetAura(SPELL_SHA_CORRUPTION)->GetStackAmount())
                        {
                            switch (me->GetEntry())
                            {
                            case NPC_PROTECTOR_KAOLAN:
                                {
                                    switch (stack) //Empty case because spells not works, in future must be fix
                                    {
                                    case 1:
                                        break;
                                    case 2:
                                        break;
                                    default:
                                        break;
                                    }
                                }
                                break;
                            case NPC_ELDER_REGAIL:
                                {
                                    switch (stack)
                                    {
                                    case 1:
                                        break;
                                    case 2:
                                        me->AddAura(SPELL_OVERWHELMING_CORRUPTION, me);
                                        break;
                                    default:
                                        break;
                                    }
                                }
                                break;
                            case NPC_ELDER_ASANI:
                                {
                                    switch (stack)
                                    {
                                    case 1:
                                        break;
                                    case 2:
                                        me->AddAura(SPELL_OVERWHELMING_CORRUPTION, me);
                                        break;
                                    default:
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            void SetProtectorsDone()
            {
                  Map::PlayerList const& players = me->GetMap()->GetPlayers();
                  if (!players.isEmpty())
                  {
                      for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                      {
                          if (Player* pPlayer = itr->getSource())
                              me->GetMap()->ToInstanceMap()->PermBindAllPlayers(pPlayer);
                      }
                  }
                  
                  if (Creature* tsulong = me->GetCreature(*me, instance->GetData64(NPC_TSULONG)))
                  {
                      tsulong->SetVisible(true);
                      tsulong->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                  }
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (instance)
                {
                    if (damage >= me->GetHealth())
                    {
                        CallDieControl(instance, me, me->GetEntry());
                        if (CalcAliveBosses(instance, me) > 1) //Only last boss must be looted
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        else if (CalcAliveBosses(instance, me) == 1) //last boss died - active tsulong
                            SetProtectorsDone();
                    }
                }
            }

            void JustDied(Unit* killer)
            {
                if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    me->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventid = events.ExecuteEvent())
                {
                    switch (eventid)
                    {
                    case EVENT_TOUCH_OF_SHA:
                        {
                            Map* pMap = me->GetMap();
                            if (pMap && pMap->IsDungeon())
                            {
                                Map::PlayerList const &players = pMap->GetPlayers();
                                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                                {
                                    if (Player* pPlayer = i->getSource())
                                    {
                                        if (pPlayer->isAlive() && !pPlayer->HasAura(SPELL_TOUCH_OF_SHA))
                                        {
                                            DoCast(pPlayer, SPELL_TOUCH_OF_SHA);
                                            break;
                                        }
                                    }
                                }
                            }
                            events.ScheduleEvent(EVENT_TOUCH_OF_SHA, urand(30000, 60000));
                            break;
                        }
                    case EVENT_LIGHTNING_BOLT:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_LIGHTNING_BOLT);
                        events.ScheduleEvent(EVENT_LIGHTNING_BOLT, urand(10000, 20000));
                        break;
                    case EVENT_WATER_BOLT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 45.0f, true))
                            DoCast(target, SPELL_WATER_BOLT);
                        events.ScheduleEvent(EVENT_WATER_BOLT, urand(10000, 20000));
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_protectorsAI(creature);
        }
};

void AddSC_boss_protectors()
{
    new boss_protectors();
}
