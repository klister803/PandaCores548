#include "ScriptMgr.h"
#include "ScriptedCreature.h"



class mob_tushui_trainee : public CreatureScript
{
    public:
        mob_tushui_trainee() : CreatureScript("mob_glintrok_oracle") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_tushui_trainee_AI(creature);
        }

        struct mob_tushui_trainee_AI : public ScriptedAI
        {
            mob_tushui_trainee_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
                me->setFaction(2101);
            }
            
            EventMap events;
            
            void EnterCombat(Unit* unit) { }

            void UpdateAI(const uint32 diff)
            {
                while (uint32 eventId = events.ExecuteEvent())
                {
                    if(eventId == 1) //on ne sais jamais :D
                    {
                        me->setFaction(2101);
                    }
                }
                
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
                
                if(me->GetHealthPct() < 0.20)
                {
                    if (Player* plr = me->getVictim()->ToPlayer())
                        plr->KilledMonsterCredit(me->GetEntry(), 0);
                    me->CombatStop();
                    me->SetHealth(me->GetMaxHealth());
                    me->setFaction(14);
                    events.ScheduleEvent(1, 20000);
                }
            }
        };
};

void AddSC_WanderingIsland()
{
    new mob_tushui_trainee();
}