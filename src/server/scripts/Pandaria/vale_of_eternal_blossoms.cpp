/*
    Dungeon : Template of the Jade Serpent 85-87
    Wise mari
    Jade servers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum eZhaoSpells
{
    SPELL_LAVA_BURST        = 75068,
    SPELL_LIGHTNING_SPEAR   = 116570,
};

enum eZhaoEvents
{
    EVENT_LAVA_BURST        = 1,
    EVENT_LIGHTNING_SPEAR   = 2,
};

class mob_zhao_jin : public CreatureScript
{
    public:
        mob_zhao_jin() : CreatureScript("mob_zhao_jin") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_zhao_jinAI(creature);
        }

        struct mob_zhao_jinAI : public ScriptedAI
        {
            mob_zhao_jinAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void Reset()
            {
                events.Reset();
                
                events.ScheduleEvent(EVENT_LAVA_BURST,      10000);
                events.ScheduleEvent(EVENT_LIGHTNING_SPEAR, 15000);
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
                        case EVENT_LAVA_BURST:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_LAVA_BURST, false);
                            events.ScheduleEvent(EVENT_LAVA_BURST,      10000);
                            break;
                        case EVENT_LIGHTNING_SPEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                                me->CastSpell(target, SPELL_LIGHTNING_SPEAR, false);
                            events.ScheduleEvent(EVENT_LIGHTNING_SPEAR, 15000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_vale_of_eternal_blossoms()
{
    new mob_zhao_jin();
}
