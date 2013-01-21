/*
    Dungeon : Gate of the Setting Sun 90 Heroic
    Instance General Script
*/

#include "gate_setting_sun.h"
#include "Vehicle.h"

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
            if (player->GetInstanceScript())
                player->GetInstanceScript()->SetData(DATA_OPEN_FIRST_DOOR, DONE);

            return false;
        }
};

class go_setting_sun_brasier : public GameObjectScript
{
public:
    go_setting_sun_brasier() : GameObjectScript("go_setting_sun_brasier") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (player->GetInstanceScript())
            player->GetInstanceScript()->SetData(DATA_BRASIER_CLICKED, DONE);

        return false;
    }
};

class vehicle_artillery_to_wall : public VehicleScript
{
    public:
        vehicle_artillery_to_wall() : VehicleScript("vehicle_artillery_to_wall") {}

        void OnAddPassenger(Vehicle* veh, Unit* /*passenger*/, int8 /*seatId*/)
        {
            if (veh->GetBase())
                if (veh->GetBase()->ToCreature())
                    if (veh->GetBase()->ToCreature()->AI())
                        veh->GetBase()->ToCreature()->AI()->DoAction(0);
        }

        struct vehicle_artillery_to_wallAI : public ScriptedAI
        {
            vehicle_artillery_to_wallAI(Creature* creature) : ScriptedAI(creature)
            {}

            uint32 launchEventTimer;

            void Reset()
            {
                launchEventTimer = 0;
            }

            void DoAction(int32 const action)
            {
                launchEventTimer = 2500;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!launchEventTimer)
                    return;

                if (launchEventTimer <= diff)
                {
                    if (me->GetVehicle())
                    {
                        if (Unit* passenger = me->GetVehicle()->GetPassenger(0))
                        {
                            passenger->ExitVehicle();
                            passenger->GetMotionMaster()->MoveJump(1100.90f, 2304.58f, 381.23f, 20.0f, 10.0f);
                        }
                    }

                    launchEventTimer = 0;
                }
                else launchEventTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new vehicle_artillery_to_wallAI(creature);
        }
};

void AddSC_gate_setting_sun()
{
    new mob_serpent_spine_defender();
    new AreaTrigger_at_first_door();
    new go_setting_sun_brasier();
    new vehicle_artillery_to_wall();
}
