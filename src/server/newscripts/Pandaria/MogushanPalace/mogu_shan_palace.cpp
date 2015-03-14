#include "NewScriptPCH.h"
#include "mogu_shan_palace.h"

class npc_jade_quilen : public CreatureScript
{
    public:
        npc_jade_quilen() : CreatureScript("npc_jade_quilen") {}

        struct npc_jade_quilenAI : public Scripted_NoMovementAI
        {
            npc_jade_quilenAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
                instance = creature->GetInstanceScript();
                OneClick = false;
            }

            InstanceScript* instance;
            bool OneClick;
            
            void OnSpellClick(Unit* /*clicker*/)
            {
                if (instance && !OneClick)
                {
                    OneClick = true;
                    uint32 JadeCount = instance->GetData(TYPE_JADECOUNT) + 1;
                    instance->SetData(TYPE_JADECOUNT, JadeCount);
                    me->DespawnOrUnsummon();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_jade_quilenAI(creature);
        }
};

void AddSC_mogu_shan_palace()
{
    new npc_jade_quilen();
}