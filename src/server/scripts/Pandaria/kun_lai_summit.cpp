#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"

enum eNessosSpells
{
    SPELL_VICIOUS_REND        = 125624,
    SPELL_GRAPPLING_HOOK      = 125623,
    SPELL_VANISH              = 125632,
    SPELL_SMOKED_BLADE        = 125633,
};

enum eNessosEvents
{
    EVENT_VICIOUS_REND        = 1,
    EVENT_GRAPPLING_HOOK      = 2,
    EVENT_VANISH              = 3,
    EVENT_SMOKED_BLADE        = 4,
};

class mob_nessos_the_oracle : public CreatureScript
{
    public:
        mob_nessos_the_oracle() : CreatureScript("mob_nessos_the_oracle") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_nessos_the_oracleAI(creature);
        }

        struct mob_nessos_the_oracleAI : public ScriptedAI
        {
            mob_nessos_the_oracleAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void Reset()
            {
                events.Reset();
                
                events.ScheduleEvent(EVENT_VICIOUS_REND,      7000);
                events.ScheduleEvent(EVENT_GRAPPLING_HOOK, 17000);
                events.ScheduleEvent(EVENT_VANISH, 12000);
            }

            void JustDied(Unit* /*killer*/)
            {
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
                        case EVENT_VICIOUS_REND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_VICIOUS_REND, false);
                            events.ScheduleEvent(EVENT_VICIOUS_REND,      7000);
                            break;
                        case EVENT_GRAPPLING_HOOK:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_GRAPPLING_HOOK, false);
                            events.ScheduleEvent(EVENT_GRAPPLING_HOOK, 17000);
                            break;
                        case EVENT_VANISH:
                            me->CastSpell(me, SPELL_VANISH, false);
                            events.ScheduleEvent(EVENT_VANISH, 20000);
                            events.ScheduleEvent(EVENT_SMOKED_BLADE, urand(0, 8000));
                            break;
                        case EVENT_SMOKED_BLADE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_SMOKED_BLADE, false);

                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_kun_lai_summit()
{
    new mob_nessos_the_oracle();
}
