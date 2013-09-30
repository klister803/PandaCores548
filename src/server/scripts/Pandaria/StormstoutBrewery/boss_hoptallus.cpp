/*
    Dungeon : Template of the Jade Serpent 85-87
    Wise mari first boss
    Jade servers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "stormstout_brewery.h"

enum Spells
{
    SPELL_CARROT_BREATH  = 112944,
    SPELL_FURL_WIND      = 112992, 

};

class boss_hoptallus : public CreatureScript
{
    public:
        boss_hoptallus() : CreatureScript("boss_hoptallus") { }
        
        struct boss_hoptallus_AI : public BossAI
        {
            boss_hoptallus_AI(Creature* creature) : BossAI(creature, DATA_HOPTALLUS)
            {
                InstanceScript* instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            uint32 breathtimer;
            uint32 windtimer;
         
            void Reset()
            {
                _Reset();
                breathtimer = 0;
                windtimer = 0;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                breathtimer = 5000;
                windtimer = 12000;
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
                
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (breathtimer <= diff)
                {
                    DoCast(me, SPELL_CARROT_BREATH);
                    breathtimer = 8000;
                }
                else
                    breathtimer -= diff;

                if (windtimer <= diff)
                {
                    DoCast(me, SPELL_FURL_WIND);
                    windtimer = 12000;
                }
                else
                    windtimer -= diff;

                DoMeleeAttackIfReady();
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hoptallus_AI(creature);
        }
};


void AddSC_boss_hoptallus()
{
    new boss_hoptallus();
}
