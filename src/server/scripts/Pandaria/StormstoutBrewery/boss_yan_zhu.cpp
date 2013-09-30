/*
    Dungeon : Template of the Jade Serpent 85-87
    Wise mari first boss
    Jade servers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "stormstout_brewery.h"

class boss_yan_zhu : public CreatureScript
{
    public:
        boss_yan_zhu() : CreatureScript("boss_yan_zhu") { }

        struct boss_yan_zhu_AI : public BossAI
        {
            boss_yan_zhu_AI(Creature* creature) : BossAI(creature, DATA_YAN_ZHU)
            {
                InstanceScript* instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
            }

            void UpdateAI(const uint32 diff)
            {
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_yan_zhu_AI(creature);
        }
};

void AddSC_boss_yan_zhu()
{
    new boss_yan_zhu();
}
