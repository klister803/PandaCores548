#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"

enum eShadowOfDoubtSpells
{
    SPELL_DEAFENED        = 108918,
    SPELL_SHA_BOLT        = 126139,
    SPELL_GROWING_DOUBT   = 126147,
};

enum eShadowOfDoubtEvents
{
    EVENT_DEAFENED              = 1,
    EVENT_GROWING_DOUBT         = 2,
    EVENT_SHA_BOLT              = 3,
};

class mob_shadow_of_doubt : public CreatureScript
{
    public:
        mob_shadow_of_doubt() : CreatureScript("mob_shadow_of_doubt") 
		{ 
		}

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_shadow_of_doubtAI(creature);
        }

        struct mob_shadow_of_doubtAI : public ScriptedAI
        {
            mob_shadow_of_doubtAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void Reset()
            {
                events.Reset();
                
                events.ScheduleEvent(EVENT_DEAFENED,      30000);
                events.ScheduleEvent(EVENT_GROWING_DOUBT,  9000);
                events.ScheduleEvent(EVENT_SHA_BOLT,	  15000);
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
                        case EVENT_DEAFENED:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_DEAFENED, false);
                            events.ScheduleEvent(EVENT_DEAFENED,      30000);
                            break;
                        case EVENT_GROWING_DOUBT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_GROWING_DOUBT, false);
                            events.ScheduleEvent(EVENT_GROWING_DOUBT, 9000);
                            break;
                        case EVENT_SHA_BOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_SHA_BOLT, false);
                            events.ScheduleEvent(EVENT_SHA_BOLT, 15000);
                            break;

                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_jade_forest()
{
    new mob_shadow_of_doubt();
}
