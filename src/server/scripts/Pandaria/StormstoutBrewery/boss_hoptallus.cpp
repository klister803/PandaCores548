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
    SPELL_EXPLOSIVE_BREW = 114291,

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
            uint32 summonadd;
         
            void Reset()
            {
                _Reset();
                breathtimer = 0;
                windtimer = 0;
                summonadd = 0;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                breathtimer = 13000;
                windtimer = 8000;
                summonadd = 10000;
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
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
                    breathtimer = 13000;
                }
                else
                    breathtimer -= diff;

                if (windtimer <= diff)
                {
                    DoCast(me, SPELL_FURL_WIND);
                    windtimer = 8000;
                }
                else
                    windtimer -= diff;

                if (summonadd <= diff)
                {
                    for (uint8 i = 0; i < 3; i++) 
                       if (Creature* hopper =  me->SummonCreature(59464, me->GetPositionX() +5, me->GetPositionY(), me->GetPositionZ(), TEMPSUMMON_CORPSE_DESPAWN))
                           hopper->SetInCombatWithZone();
                       summonadd = 10000;
                }
                else
                    summonadd -= diff;

                DoMeleeAttackIfReady();
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hoptallus_AI(creature);
        }
};

class npc_hopper : public CreatureScript
{
    public:
        npc_hopper() : CreatureScript("npc_hopper") {}

        struct npc_hopperAI : public ScriptedAI
        {
            npc_hopperAI(Creature* creature) : ScriptedAI(creature){}

            bool explose;

            void Reset()
            {
                explose = false;
            }

            void UpdateAI(const uint32 diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (me->getVictim())
                {
                    if (me->IsWithinMeleeRange(me->getVictim()))
                    {
                        if (me->GetDistance(me->getVictim()) <= 0.5f && !explose)
                        {
                            explose = true;
                            DoCast(me->getVictim(), SPELL_EXPLOSIVE_BREW);
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hopperAI(creature);
        }
};


void AddSC_boss_hoptallus()
{
    new boss_hoptallus();
    new npc_hopper();
}
