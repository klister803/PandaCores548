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
    SPELL_FURIOUS_SWIPE     = 122735, //Cleave
    SPELL_PHEROMONES        = 122835,
    SPELL_FURY              = 122754,
    SPELL_CRUSH             = 122774,
};

enum eEvents
{
    EVENT_FURIOUS_SWIPE     = 1,
    EVENT_CHECK_HIT_SWIPE   = 2,
};

class boss_garalon : public CreatureScript
{
    public:
        boss_garalon() : CreatureScript("boss_garalon") {}

        struct boss_garalonAI : public BossAI
        {
            boss_garalonAI(Creature* creature) : BossAI(creature, DATA_GARALON)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            uint8 hitcount;

            void Reset()
            {
                _Reset();
                hitcount = 0;
                me->SetReactState(REACT_DEFENSIVE);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_FURIOUS_SWIPE, urand(20000, 30000));
            }
            
            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (target->GetTypeId() == TYPEID_PLAYER && spell->Id == SPELL_FURIOUS_SWIPE)
                    hitcount++;
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
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
                    case EVENT_FURIOUS_SWIPE:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_FURIOUS_SWIPE);
                        events.ScheduleEvent(EVENT_CHECK_HIT_SWIPE, 3000);
                        events.ScheduleEvent(EVENT_FURIOUS_SWIPE, urand(20000, 30000));
                        break;
                    case EVENT_CHECK_HIT_SWIPE:
                        if (hitcount < 2)
                            DoCast(me, SPELL_FURY);
                        hitcount = 0;
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_garalonAI(creature);
        }
};

void AddSC_boss_garalon()
{
    new boss_garalon();
}
