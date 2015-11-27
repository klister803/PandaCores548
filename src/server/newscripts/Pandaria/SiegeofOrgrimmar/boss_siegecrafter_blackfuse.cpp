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

#include "NewScriptPCH.h"
#include "siege_of_orgrimmar.h"

enum eSpells
{
    //Blackfuse
    SPELL_ELECTROSTATIC_CHARGE      = 143385,
    SPELL_AUTOMATIC_REPAIR_BEAM_AT  = 144212,

    SPELL_LAUNCH_SAWBLADE           = 143265,
    SPELL_LAUNCH_SAWBLADE_AT        = 143329, //1021
    SPELL_SERRATED_SLASH_DMG        = 143327,

    //Automated shredder
    SPELL_REACTIVE_ARMOR            = 143387,
    SPELL_DEATH_FROM_ABOVE          = 144208,
    SPELL_OVERLOAD                  = 145444,

    SPELL_ON_CONVEYOR               = 144287, 

    //Laser array trigger = 71910,
    SPELL_CONVEYOR_DEATH_BEAM_V     = 144284,
    SPELL_CONVEYOR_DEATH_BEAM_V2    = 149016,
    SPELL_CONVEYOR_DEATH_BEAM_AT    = 144282,
};

enum eEvents
{
    //Automated Shredder
    EVENT_DEATH_FROM_ABOVE          = 10,

    //Special
    EVENT_LAUNCH_BACK               = 11,
};

Position lapos[5] =
{
    { 1999.54f, -5537.81f, -302.9150f, 0.0f },
    { 2005.43f, -5534.02f, -302.9150f, 0.0f }, //1
    { 2011.31f, -5530.24f, -302.9150f, 0.0f },
    { 2017.20f, -5526.45f, -302.9150f, 0.0f },
    { 2023.09f, -5522.66f, -302.9150f, 0.0f },
};

Position lapos2[5] =
{
    { 2017.53f, -5563.97f, -303.5679f, 0.0f },
    { 2023.42f, -5560.20f, -303.5679f, 0.0f },
    { 2029.31f, -5556.41f, -303.5679f, 0.0f },
    { 2035.19f, -5552.62f, -303.5679f, 0.0f },//1
    { 2041.08f, -5548.85f, -303.5679f, 0.0f },
};

Position lapos3[5] =
{
    { 2033.35f, -5587.66f, -303.2579f, 0.0f },
    { 2039.25f, -5583.87f, -303.2579f, 0.0f },
    { 2045.14f, -5580.10f, -303.2579f, 0.0f },//1
    { 2051.03f, -5576.31f, -303.2579f, 0.0f },
    { 2056.91f, -5572.51f, -303.2579f, 0.0f },
};

struct WeaponWaveArray
{
    uint8  wavecount;
    uint32 firstentry;
    uint32 secondentry;
    uint32 thirdentry;
};

static WeaponWaveArray wavearray[5] =
{
    {0, NPC_DEACTIVATED_MISSILE_TURRET, NPC_DISASSEMBLED_CRAWLER_MINE, NPC_DEACTIVATED_LASER_TURRET},
    {1, NPC_DISASSEMBLED_CRAWLER_MINE, NPC_DEACTIVATED_LASER_TURRET, NPC_DEACTIVATED_MISSILE_TURRET},
    {2, NPC_DEACTIVATED_LASER_TURRET, NPC_DEACTIVATED_ELECTROMAGNET, NPC_DEACTIVATED_MISSILE_TURRET},
    {3, NPC_DEACTIVATED_LASER_TURRET, NPC_DEACTIVATED_MISSILE_TURRET, NPC_DISASSEMBLED_CRAWLER_MINE},
    {4, NPC_DEACTIVATED_MISSILE_TURRET, NPC_DEACTIVATED_ELECTROMAGNET, NPC_DISASSEMBLED_CRAWLER_MINE},
};

Position spawnweaponpos[3] =
{
    { 1973.65f, -5472.38f, -299.0f, 5.294743f },
    { 1958.50f, -5450.76f, -299.0f, 5.294743f },
    { 1941.65f, -5425.50f, -299.0f, 5.294743f },
};

Position destpos = { 2073.01f, -5620.12f, -302.8857f, 5.294743f };

//71504
class boss_siegecrafter_blackfuse : public CreatureScript
{
  public:
     boss_siegecrafter_blackfuse() : CreatureScript("boss_siegecrafter_blackfuse") {}
     
     struct boss_siegecrafter_blackfuseAI : public BossAI
     {
         boss_siegecrafter_blackfuseAI(Creature* creature) : BossAI(creature, DATA_BLACKFUSE)
         {
             instance = creature->GetInstanceScript();
         }
         
         InstanceScript* instance;
         uint8 weaponwavecount;
         
         void Reset()
         {
             _Reset();
             weaponwavecount = 0;
             me->RemoveAurasDueToSpell(SPELL_PROTECTIVE_FRENZY);
             me->RemoveAurasDueToSpell(SPELL_AUTOMATIC_REPAIR_BEAM_AT);
             //me->SetReactState(REACT_DEFENSIVE);
             me->SetReactState(REACT_PASSIVE);    //test only
             me->RemoveAurasDueToSpell(SPELL_AUTOMATIC_REPAIR_BEAM_AT);
         }

         void EnterCombat(Unit* who)
         {
             _EnterCombat();
             CreateLaserWalls();
             CreateWeaponWave(weaponwavecount);
             DoCast(me, SPELL_AUTOMATIC_REPAIR_BEAM_AT, true);
         }

         void EnterEvadeMode()
         {
             me->Kill(me, true);
             me->Respawn(true);
             me->GetMotionMaster()->MoveTargetedHome();
         }

         void CreateWeaponWave(uint8 wavecount)
         {
             wavecount = wavecount > 4 ? 0 : wavecount;
             if (Creature* firstdweapon = me->SummonCreature(wavearray[wavecount].firstentry, spawnweaponpos[0]))
                 firstdweapon->GetMotionMaster()->MoveCharge(destpos.GetPositionX(), destpos.GetPositionY(), destpos.GetPositionZ(), 5.0f, false);
             if (Creature* seconddweapon = me->SummonCreature(wavearray[wavecount].secondentry, spawnweaponpos[1]))
                 seconddweapon->GetMotionMaster()->MoveCharge(destpos.GetPositionX(), destpos.GetPositionY(), destpos.GetPositionZ(), 5.0f, false);
             if (Creature* thirddweapon = me->SummonCreature(wavearray[wavecount].thirdentry, spawnweaponpos[2]))
                 thirddweapon->GetMotionMaster()->MoveCharge(destpos.GetPositionX(), destpos.GetPositionY(), destpos.GetPositionZ(), 5.0f, false);
             weaponwavecount++;
         }

         void CreateLaserWalls()
         {
             uint8 mod = urand(0, 4);
             switch (mod)
             {
             case 0:
                 for (uint8 n = 0; n < 5; ++n) 
                     if (n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n) 
                     if (n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n) 
                     if (n != 2)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 1:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 1 && n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 2:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 1 && n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 3:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 1 && n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 4:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             }
         }

         void DoAction(int32 const action){}
         
         void JustDied(Unit* killer)
         {
             if (killer != me)
                 _JustDied();
         }

         void UpdateAI(uint32 diff)
         {
             if (!UpdateVictim())
                 return;
             
             DoMeleeAttackIfReady();
         }
     };
     
     CreatureAI* GetAI(Creature* creature) const
     {
         return new boss_siegecrafter_blackfuseAI(creature);
     }
};

//71532(0), 72694(1)
class npc_blackfuse_passenger : public CreatureScript
{
public:
    npc_blackfuse_passenger() : CreatureScript("npc_blackfuse_passenger") {}

    struct npc_blackfuse_passengerAI : public ScriptedAI
    {
        npc_blackfuse_passengerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        InstanceScript* instance;

        void Reset()
        {
            me->SetVisible(true);
        }

        void EnterCombat(Unit* who){}

        void SpellHit(Unit* caster, SpellInfo const *spell)
        {
            if (spell->Id == SPELL_LAUNCH_SAWBLADE_AT)
                LaunchSawBlade();
        }

        void LaunchSawBlade()
        {
            if (!me->ToTempSummon())
                return;
            
            if (Unit* blackfuse = me->ToTempSummon()->GetSummoner())
            {
                me->SetVisible(false);
                Position pos;
                blackfuse->GetNearPosition(pos, 7.0f, 5.5f);
                if (Creature* sawblade = blackfuse->SummonCreature(NPC_BLACKFUSE_SAWBLADE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + 2.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 10000))
                    if (Unit* target = blackfuse->ToCreature()->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                        sawblade->GetMotionMaster()->MoveCharge(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ() + 2.0f, 15.0f);
            }
        }

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_blackfuse_passengerAI(creature);
    }
};

//71591
class npc_automated_shredder : public CreatureScript
{
public:
    npc_automated_shredder() : CreatureScript("npc_automated_shredder") {}

    struct npc_automated_shredderAI : public ScriptedAI
    {
        npc_automated_shredderAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->ModifyAuraState(AURA_STATE_CONFLAGRATE, true);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset(){}

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_DEATH_FROM_ABOVE, 5000);
        }

        void JustDied(Unit* killer){}

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_DEATH_FROM_ABOVE:
                    DoCast(me, SPELL_DEATH_FROM_ABOVE);
                    events.ScheduleEvent(EVENT_DEATH_FROM_ABOVE, 40000);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_automated_shredderAI(creature);
    }
};

//71790, 71751, 71694, 71606, 71788, 71752, 71696, 71638
class npc_blackfuse_weapon : public CreatureScript
{
public:
    npc_blackfuse_weapon() : CreatureScript("npc_blackfuse_weapon") {}

    struct npc_blackfuse_weaponAI : public ScriptedAI
    {
        npc_blackfuse_weaponAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
        }

        InstanceScript* instance;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void JustDied(Unit* killer)
        {
            if (!instance)
                return;

            instance->SetData(DATA_SAFE_WEAPONS, me->GetEntry());
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch (pointId)
                {
                //offline weapon in dest point
                case 0: 
                    instance->SetData(DATA_D_WEAPON_IN_DEST_POINT, 0);
                    me->DespawnOrUnsummon();
                    break;
                //online weapon in dest point
                case 1: 
                    me->SetFacingTo(0.435088f);
                    break;
                }
            }
        }

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_blackfuse_weaponAI(creature);
    }
};

//71910, 71740, 72710
class npc_blackfuse_trigger : public CreatureScript
{
public:
    npc_blackfuse_trigger() : CreatureScript("npc_blackfuse_trigger") {}

    struct npc_blackfuse_triggerAI : public ScriptedAI
    {
        npc_blackfuse_triggerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            if (me->GetEntry() == NPC_LASER_ARRAY)
                me->AddAura(SPELL_CONVEYOR_DEATH_BEAM_V, me);
        }

        InstanceScript* instance;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_blackfuse_triggerAI(creature);
    }
};

void AddSC_boss_siegecrafter_blackfuse()
{
    new boss_siegecrafter_blackfuse();
    new npc_blackfuse_passenger();
    new npc_automated_shredder();
    new npc_blackfuse_weapon();
    new npc_blackfuse_trigger();
}
