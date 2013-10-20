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
};

enum eEvents
{
};

Position middlePos = { 4022.00f, 1908.00f, 358.00f, 0.0f };

class boss_elegon : public CreatureScript
{
    public:
        boss_elegon() : CreatureScript("boss_elegon") {}

        struct boss_elegonAI : public BossAI
        {
            boss_elegonAI(Creature* creature) : BossAI(creature, DATA_ELEGON)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();
            }

            void DoAction(const int32 action)
            {
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_elegonAI(creature);
        }
};

enum eCelestialProtectorSpells
{
    SPELL_TOTAL_ANNIHILATION        = 129711,
    SPELL_ARCING_ENERGY             = 117945,
    SPELL_STABILITY_FLUX            = 117911,
    SPELL_TOUCH_OF_THE_TITANS       = 117870,
    SPELL_ECLIPSE                   = 117885,
};

enum eCelestialProtectorEvents
{
    EVENT_ARCING_ENERGY             = 1,
    EVENT_TOUCH_OF_THE_TITANS       = 2,
    EVENT_KILLED                    = 3,
};

class mob_celestial_protector : public CreatureScript
{
    public:
        mob_celestial_protector() : CreatureScript("mob_celestial_protector") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_celestial_protectorAI(creature);
        }

        struct mob_celestial_protectorAI : public ScriptedAI
        {
            mob_celestial_protectorAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;
            bool stabilityFluxCasted;
            bool totalAnnihilationCasted;

            void Reset()
            {
                events.Reset();
                
                events.ScheduleEvent(EVENT_ARCING_ENERGY, 30000);

                stabilityFluxCasted = false;
                totalAnnihilationCasted = false;
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                if (!stabilityFluxCasted)
                    if (me->HealthBelowPctDamaged(25, damage))
                    {
                        me->CastSpell(me, SPELL_STABILITY_FLUX, false);
                        stabilityFluxCasted = true;
                    }

                if (me->GetHealth() < damage)
                {
                    damage = 0;
                    
                    if (!totalAnnihilationCasted)
                    {
                        me->CastSpell(me, SPELL_TOTAL_ANNIHILATION, false);
                        totalAnnihilationCasted = true;
                        events.ScheduleEvent(EVENT_KILLED , 4500);
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
                
                if (me->GetHealthPct() > 25.0f && me->GetDistance(middlePos) >= 40)
                {
                    me->CastSpell(me, SPELL_ECLIPSE, false);
                }
                
                events.Update(diff);
                
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARCING_ENERGY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                me->CastSpell(target, SPELL_ARCING_ENERGY, false);
                            events.ScheduleEvent(EVENT_ARCING_ENERGY,      30000);
                            break;
                        
                        case EVENT_KILLED:
                            me->Kill(me);
                            break;

                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };
};

enum eCosmicSparkSpells
{
    SPELL_ICE_TRAP        = 135382,
};

enum eCosmicSparkEvents
{
    EVENT_ICE_TRAP             = 1,
};

class mob_cosmic_spark : public CreatureScript
{
    public:
        mob_cosmic_spark() : CreatureScript("mob_cosmic_spark") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_cosmic_sparkAI(creature);
        }

        struct mob_cosmic_sparkAI : public ScriptedAI
        {
            mob_cosmic_sparkAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void Reset()
            {
                events.Reset();
                
                events.ScheduleEvent(EVENT_ICE_TRAP, 60000);
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
                        case EVENT_ICE_TRAP:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                me->CastSpell(target, SPELL_ICE_TRAP, false);
                            events.ScheduleEvent(EVENT_ICE_TRAP,      60000);
                            break;
                        
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_boss_elegon()
{
    new boss_elegon();
    new mob_celestial_protector();
    new mob_cosmic_spark();
}
