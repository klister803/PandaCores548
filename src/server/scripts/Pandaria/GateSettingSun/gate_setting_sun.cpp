/*
    Dungeon : Gate of the Setting Sun 90 Heroic
    Instance General Script
*/

#include "gate_setting_sun.h"

enum spells
{
    SPELL_MANTID_MUNITION_EXPLOSION     = 107153,
    SPELL_EXPLOSE_GATE                  = 115456,
};

class mob_serpent_spine_defender : public CreatureScript
{
public:
    mob_serpent_spine_defender() : CreatureScript("mob_serpent_spine_defender") { }

    struct mob_serpent_spine_defenderAI : public Scripted_NoMovementAI
    {
        mob_serpent_spine_defenderAI(Creature* creature) : Scripted_NoMovementAI(creature) {}

        uint32 attackTimer;

        void Reset()
        {
            attackTimer = 2500;
        }

        void DamageDealt(Unit* /*target*/, uint32& damage, DamageEffectType /*damageType*/)
        {
            damage = 0;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!me->isInCombat())
            {
                if (attackTimer <= diff)
                {
                    if (Unit* target = me->SelectNearestTarget(5.0f))
                        if (!target->IsFriendlyTo(me))
                            AttackStart(target);
                }
                else
                    attackTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_serpent_spine_defenderAI(creature);
    }
};

//8359
class AreaTrigger_at_first_door : public AreaTriggerScript
{
    public:
        AreaTrigger_at_first_door() : AreaTriggerScript("at_first_door") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
        {
            if (!player->GetInstanceScript())
                return false;

            if (player->GetInstanceScript()->GetData(DATA_OPEN_FIRST_DOOR))
                return false;

            player->GetInstanceScript()->SetData(DATA_OPEN_FIRST_DOOR, 1);
            return false;
        }
};

void AddSC_gate_setting_sun()
{
    new mob_serpent_spine_defender();
    new AreaTrigger_at_first_door();
}