/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "gate_setting_sun.h"
#include "Vehicle.h"
#include "Spline.h"

enum eSpells
{
    SPELL_IMPERVIOUS_CARAPACE   = 107118,

    SPELL_BATTERING_HEADBUTT    = 118685,
    SPELL_BATTERING_STUN        = 130772,
};

enum eEvents
{
};

enum eMovements
{
    POINT_MAIN_DOOR     = 1,
    POINT_HERSE         = 2
};

Position pos[2] =
{
    { 958.33f, 2241.68f, 296.10f, 0.0f },
    { 958.26f, 2330.15f, 296.18f, 0.0f }
};

class boss_raigonn : public CreatureScript
{
    public:
        boss_raigonn() : CreatureScript("boss_raigonn") {}

        struct boss_raigonnAI : public BossAI
        {
            boss_raigonnAI(Creature* creature) : BossAI(creature, DATA_RAIGONN)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;

            uint8  eventChargeProgress;
            uint32 eventChargeTimer;

            void Reset()
            {
                _Reset();

                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_IMPERVIOUS_CARAPACE, me);

                eventChargeProgress = 0;
                eventChargeTimer    = 1000;
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();

                if (Creature* weakPoint = me->SummonCreature(NPC_WEAK_POINT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                    weakPoint->EnterVehicle(me, 1);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (pointId)
                {
                    case POINT_MAIN_DOOR:
                    case POINT_HERSE:
                        DoEventCharge();
                        break;
                }
            }

            void JustReachedHome()
            {
                instance->SetBossState(DATA_RAIGONN, FAIL);
                summons.DespawnAll();
            }

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

            void DoEventCharge()
            {
                switch (eventChargeProgress)
                {
                    case 0:
                        // Emote Wazzaaaa
                        eventChargeTimer = 1000;
                        ++eventChargeProgress;
                        break;
                    case 1:
                        me->GetMotionMaster()->MoveCharge(pos[0].GetPositionX(), pos[0].GetPositionY(), pos[0].GetPositionZ(), 42.0f, POINT_HERSE);
                        ++eventChargeProgress;
                        break;
                    case 2:
                        // Todo : Remove passengers
                        eventChargeTimer = 3000;
                        ++eventChargeProgress;
                        break;
                    case 3:
                        // We are going back to main door, restart
                        me->GetMotionMaster()->MoveBackward(POINT_MAIN_DOOR, pos[1].GetPositionX(), pos[1].GetPositionY(), pos[1].GetPositionZ());
                        eventChargeProgress = 0;
                        break;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (eventChargeTimer)
                {
                    if (eventChargeTimer <= diff)
                    {
                        eventChargeTimer = 0;
                        DoEventCharge();
                    }
                    else
                        eventChargeTimer -= diff;
                }

                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_raigonnAI(creature);
        }
};

class vehicle_artillery : public VehicleScript
{
    public:
        vehicle_artillery() : VehicleScript("vehicle_artillery") {}

        void OnAddPassenger(Vehicle* veh, Unit* /*passenger*/, int8 /*seatId*/)
        {
            if (veh->GetBase())
                if (veh->GetBase()->ToCreature())
                    if (veh->GetBase()->ToCreature()->AI())
                        veh->GetBase()->ToCreature()->AI()->DoAction(0);
        }

        struct vehicle_artilleryAI : public ScriptedAI
        {
            vehicle_artilleryAI(Creature* creature) : ScriptedAI(creature)
            {
               pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
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
                    if (Creature* weakPoint = pInstance->instance->GetCreature(pInstance->GetData64(NPC_WEAK_POINT)))
                    {
                        if (weakPoint->GetVehicle())
                        {
                            if (me->GetVehicle())
                            {
                                if (Unit* passenger = me->GetVehicle()->GetPassenger(0))
                                {
                                    passenger->ExitVehicle();

                                    const uint32 maxSeatCount = 2;
                                    uint32 availableSeatCount = weakPoint->GetVehicle()->GetAvailableSeatCount();
                                    weakPoint->GetVehicle()->AddPassenger(passenger, maxSeatCount - availableSeatCount);
                                }
                            }
                        }
                    }

                    launchEventTimer = 0;
                }
                else launchEventTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new vehicle_artilleryAI(creature);
        }
};



void AddSC_boss_raigonn()
{
    new boss_raigonn();
    new vehicle_artillery();
}
