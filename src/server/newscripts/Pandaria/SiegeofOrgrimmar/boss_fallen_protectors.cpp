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
    SPELL_BOUND_OF_GOLDEN_LOTUS         = 143497, //Bond of the Golden Lotus

    //Rook
    SPELL_VENGEFUL_STRIKE               = 144396, //Vengeful Strikes
    SPELL_CORRUPTED_BREW                = 143019, //Corrupted Brew
    SPELL_CLASH                         = 143027, //Clash
    SPELL_CORRUPTION_KICK               = 143007, //Corruption Kick

    //He
    SPELL_GARROTE                       = 143198, //Garrote
    SPELL_GOUGE                         = 143301, //Gouge
    SPELL_NOXIOUS_POISON                = 143225, //Noxious Poison
    SPELL_INSTANT_POISON                = 143210, //Instant Poison

    //Sun
    SPELL_SHA_SEAR                      = 143423, //Sha Sear
    SPELL_SHADOW_WORD_BANE              = 143434, //Shadow Word: Bane
    SPELL_CALAMITY                      = 143491, //Calamity
};

enum Phases
{
    PHASE_BATTLE                    = 1,
    PHASE_DESPERATE_MEASURES        = 2,
    PHASE_BOND_GOLDEN_LOTUS         = 3
};


struct boss_fallen_protectors : public BossAI
{
    boss_fallen_protectors(Creature* creature) : BossAI(creature, DATA_F_PROTECTORS)
    {
        _healthPhase = 0;
    }

    int8 _healthPhase;

    void Reset()
    {
        
    }

    void EnterCombat(Unit* who)
    {

    }

    void HealReceived(Unit* /*done_by*/, uint32& addhealth)
    {
        float newpct = GetHealthPctWithHeal(addhealth);
        if ((_healthPhase == 0 && newpct <= 66.0f) ||
            (_healthPhase == 1 && newpct <= 33.0f))
            --_healthPhase;

        events.SetPhase(PHASE_BATTLE);
    }

    void DamageTaken(Unit* attacker, uint32 &damage)
    {
        if (me->GetHealth() <= 1 || int(me->GetHealth() - damage) <= 1)
        {
            damage = me->GetHealth() - damage;

            if (!events.IsInPhase(PHASE_BOND_GOLDEN_LOTUS))
            {
                me->InterruptNonMeleeSpells(false);
                DoCast(me, SPELL_BOUND_OF_GOLDEN_LOTUS, false);

                events.SetPhase(PHASE_BOND_GOLDEN_LOTUS);
                events.ScheduleEvent(EVENT_1, 1*IN_MILLISECONDS, 0, PHASE_BOND_GOLDEN_LOTUS);
            }
            return;
        }

        if ((_healthPhase == 0 && GetHealthPct(damage) <= 66.0f) ||
            (_healthPhase == 1 && GetHealthPct(damage) <= 33.0f))
        {
            ++_healthPhase;
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            events.SetPhase(PHASE_DESPERATE_MEASURES);
        }
    }

    void DoAction(int32 const action)
    {
    }

    void JustDied(Unit* /*killer*/)
    {
    }
};

//Rook Stonetoe
class boss_rook_stonetoe : public CreatureScript
{
    public:
        boss_rook_stonetoe() : CreatureScript("boss_rook_stonetoe") {}

        struct boss_rook_stonetoeAI : public boss_fallen_protectors
        {
            boss_rook_stonetoeAI(Creature* creature) : boss_fallen_protectors(creature)
            {
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
            }

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rook_stonetoeAI(creature);
        }
};

//He Softfoot
class boss_he_softfoot : public CreatureScript
{
    public:
        boss_he_softfoot() : CreatureScript("boss_he_softfoot") {}

        struct boss_he_softfootAI : public boss_fallen_protectors
        {
            boss_he_softfootAI(Creature* creature) : boss_fallen_protectors(creature)
            {
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
            }

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_he_softfootAI(creature);
        }
};

//Sun Tenderheart
class boss_sun_tenderheart : public CreatureScript
{
    public:
        boss_sun_tenderheart() : CreatureScript("boss_sun_tenderheart") {}

        struct boss_sun_tenderheartAI : public boss_fallen_protectors
        {
            boss_sun_tenderheartAI(Creature* creature) : boss_fallen_protectors(creature)
            {
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
            }

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sun_tenderheartAI(creature);
        }
};

//Golden Lotus
class npc_golden_lotus_control : public CreatureScript
{
public:
    npc_golden_lotus_control() : CreatureScript("npc_golden_lotus_control") { }

    struct npc_wind_vehicleAI : public ScriptedAI
    {        
        npc_wind_vehicleAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();            
        }

        enum data
        {
            SUMMON_MOVER        = 143705,
            SPELL_FACE_CHANNEL  = 116351,
        };

        InstanceScript* instance;
        EventMap events;

        //void JustSummoned(Creature* creature)
        //{
        //    if (creature->GetEntry() == NPC_GOLD_LOTOS_MOVER)
        //    {
        //        creature->SetDisplayId(48920);
        //        creature->GetMotionMaster()->MoveIdle();
        //        creature->LoadPath(creature->GetEntry());
        //        creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        //        creature->GetMotionMaster()->Initialize();
        //        if (creature->isAlive())                            // dead creature will reset movement generator at respawn
        //        {
        //            creature->setDeathState(JUST_DIED);
        //            creature->Respawn(true);
        //        }
        //    }
        //};

        void Reset()
        {
            //Use manual spawn.
            //me->CastSpell(me, SUMMON_MOVER, true);
            events.RescheduleEvent(EVENT_1, 1000);
        }
        
        void OnCharmed(bool /*apply*/)
        {
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (apply)
            {
                if (Creature* mover = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MOVER)))
                {
                    who->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                    if (!me->HasAura(SPELL_FACE_CHANNEL))
                        me->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {

            }
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wind_vehicleAI(creature);
    }
};

void AddSC_boss_fallen_protectors()
{
    new boss_rook_stonetoe();
    new boss_he_softfoot();
    new boss_sun_tenderheart();
    new npc_golden_lotus_control();
}
