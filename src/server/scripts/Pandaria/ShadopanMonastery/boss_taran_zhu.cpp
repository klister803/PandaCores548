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
    SPELL_RISING_HATE               = 107356,
    SPELL_RING_OF_MALICE            = 131521,
    SPELL_SHA_BLAST                 = 114999,
    SPELL_SUMMON_GRIPPING_HATRED    = 115002,

    // Gripping Hatred
    SPELL_GRIP_OF_HATE              = 115010,
    SPELL_POOL_OF_SHADOWS           = 112929,
};

enum eEvents
{
    EVENT_RISING_HATE               = 1,
    EVENT_RING_OF_MALICE            = 2,
    EVENT_SHA_BLAST                 = 3,
    EVENT_SUMMON_GRIPPING_HATRED    = 4,

    EVENT_GRIP_OF_HATE              = 5,
};

class boss_taran_zhu : public CreatureScript
{
    public:
        boss_taran_zhu() : CreatureScript("boss_taran_zhu") {}

        struct boss_taran_zhuAI : public BossAI
        {
            boss_taran_zhuAI(Creature* creature) : BossAI(creature, DATA_TARAN_ZHU)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            bool enrageDone;

            void Reset()
            {
                _Reset();
                
                events.ScheduleEvent(EVENT_RISING_HATE,             urand(25000, 35000));
                events.ScheduleEvent(EVENT_RING_OF_MALICE,          urand(10000, 20000));
                events.ScheduleEvent(EVENT_SUMMON_GRIPPING_HATRED,  urand(20000, 30000));
            }

            void JustReachedHome()
            {
                pInstance->SetBossState(DATA_SHA_VIOLENCE, FAIL);
                summons.DespawnAll();
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (!enrageDone && me->HealthBelowPctDamaged(20, damage))
                    me->CastSpell(me, SPELL_ENRAGE, true);
            }

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);

                if (summoned->GetEntry() == NPC_LESSER_VOLATILE_ENERGY)
                    summoned->CastSpell(summoned, SPELL_ICE_TRAP, true);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);

                if (summon->GetEntry() == NPC_LESSER_VOLATILE_ENERGY)
                    summon->CastSpell(summon, SPELL_EXPLOSION, true);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    case EVENT_SMOKE_BLADES:
                        me->CastSpell(me, SPELL_SMOKE_BLADES, false);
                        events.ScheduleEvent(EVENT_SMOKE_BLADES,        urand(25000, 35000));
                        break;
                    case EVENT_SHA_SPIKE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            me->CastSpell(target, SPELL_SHA_SPIKE, false);
                        
                        events.ScheduleEvent(EVENT_SHA_SPIKE,           urand(10000, 20000));
                        break;
                    case EVENT_DISORIENTING_SMASH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            me->CastSpell(target, SPELL_DISORIENTING_SMASH, false);
                        
                        events.ScheduleEvent(EVENT_DISORIENTING_SMASH,  urand(20000, 30000));
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_taran_zhuAI(creature);
        }
};

void AddSC_boss_taran_zhu()
{
    new boss_taran_zhuAI();
}
