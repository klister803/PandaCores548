#include "ScriptMgr.h"
#include "ScriptedCreature.h"

class mob_tushui_trainee : public CreatureScript
{
    public:
        mob_tushui_trainee() : CreatureScript("mob_tushui_trainee") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_tushui_trainee_AI(creature);
        }

        struct mob_tushui_trainee_AI : public ScriptedAI
        {
            mob_tushui_trainee_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_DEFENSIVE);
                me->setFaction(2068);
            }
            
            EventMap events;
            
            void EnterCombat(Unit* unit) { }

            void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
            {
                if((me->GetHealth() - uiDamage)*100/me->GetMaxHealth() < 20)
                {
                    uiDamage = 0;
                    me->SetHealth(1);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                //while (uint32 eventId = events.ExecuteEvent())
                //{
                //    if(eventId == 1) //on ne sais jamais :D
                //    {
                //        me->setFaction(2101);
                //    }
                //}
                
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
                
                if(me->GetHealthPct() < 20)
                {
                    if(me->getVictim() && me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                        ((Player*)me->getVictim())->KilledMonsterCredit(54586, 0);
                    me->CombatStop();
                    me->SetHealth(me->GetMaxHealth());
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                    me->setFaction(2101);
                    me->ToCreature()->DespawnOrUnsummon(3000);
                    //me->setFaction(7);
                    //events.ScheduleEvent(1, 20000);
                }
            }
        };
};

void AddSC_WanderingIsland()
{
    new mob_tushui_trainee();
}
