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
#include "throne_of_thunder.h"

enum eSpells
{
    SPELL_PRIMORDIAL_STRIKE   = 136037,
    SPELL_MALFORMED_BLOOD     = 136050,

    //Special spells
    SPELL_ACIDIC_EXPLOSION    = 136219,
    SPELL_VOLATILE_POTHOGEN   = 136228,
    SPELL_VOLATILE_POTHOGEN_D = 136225,
    SPELL_CAUSTIC_GAS         = 136216,
    SPELL_CAUSTIC_GAS_D       = 136215,
};

enum eEvents
{
    EVENT_PRIMORDIAL_STRIKE   = 1,
    EVENT_MALFORMED_BLOOD     = 2,
    EVENT_SPECIAL_STRIKE      = 3,
};

class boss_primordius : public CreatureScript
{
    public:
        boss_primordius() : CreatureScript("boss_primordius") {}

        struct boss_primordiusAI : public BossAI
        {
            boss_primordiusAI(Creature* creature) : BossAI(creature, DATA_PRIMORDIUS)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                me->AddAura(SPELL_VOLATILE_POTHOGEN_D, me);
                me->AddAura(SPELL_CAUSTIC_GAS_D, me);
                events.ScheduleEvent(EVENT_PRIMORDIAL_STRIKE, 22000);
                events.ScheduleEvent(EVENT_MALFORMED_BLOOD,   50000);
                events.ScheduleEvent(EVENT_SPECIAL_STRIKE,    60000);
            }
            
            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
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
                    case EVENT_PRIMORDIAL_STRIKE:
                        if (me->getVictim())
                        {
                            me->SetFacingToObject(me->getVictim());
                            DoCast(me->getVictim(), SPELL_PRIMORDIAL_STRIKE);
                        }
                        events.ScheduleEvent(EVENT_PRIMORDIAL_STRIKE, 22000);
                        break;
                    case EVENT_MALFORMED_BLOOD:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_MALFORMED_BLOOD);
                        events.ScheduleEvent(EVENT_MALFORMED_BLOOD, 50000);
                        break;
                    case EVENT_SPECIAL_STRIKE:
                        {
                            uint8 pos = urand(0, 2);
                            switch (pos)
                            {
                            case 0:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                                    DoCast(target, SPELL_ACIDIC_EXPLOSION);
                                break;
                            case 1:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                                    DoCast(target, SPELL_VOLATILE_POTHOGEN);
                                break;
                            case 2:
                                DoCastAOE(SPELL_CAUSTIC_GAS);
                                break;
                            }
                        }
                        events.ScheduleEvent(EVENT_SPECIAL_STRIKE, 60000);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_primordiusAI(creature);
        }
};

void AddSC_boss_primordius()
{
    new boss_primordius();
}
