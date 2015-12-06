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
    SPELL_AUTOMATIC_REPAIR_BEAM     = 144213,
    SPELL_LAUNCH_SAWBLADE           = 143265,
    SPELL_LAUNCH_SAWBLADE_AT        = 143329, //1021
    SPELL_SERRATED_SLASH_DMG        = 143327,
    //Automated shredder
    SPELL_REACTIVE_ARMOR            = 143387,
    SPELL_DEATH_FROM_ABOVE          = 144208,
    SPELL_OVERLOAD                  = 145444,
    //Crawler Mine
    SPELL_DETONATE                  = 149146,
    SPELL_READY_TO_GO               = 145580,
    SPELL_BREAK_IN_PERIOD           = 145269,
    SPELL_CRAWLER_MINE_FIXATE       = 144010,
    SPELL_CRAWLER_MINE_FIXATE_D     = 144009,
    //Special
    SPELL_ON_CONVEYOR               = 144287, 
    SPELL_PATTERN_RECOGNITION       = 144236,
    //Laser array trigger = 71910,
    SPELL_CONVEYOR_DEATH_BEAM_V     = 144284,
    SPELL_CONVEYOR_DEATH_BEAM_V2    = 149016,
    SPELL_CONVEYOR_DEATH_BEAM_AT    = 144282,
    SPELL_PURIFICATION_BEAM         = 144335,
    //Create conveyer trigger 145272
    //Laser turret
    SPELL_DISINTEGRATION_LASER_V    = 143867,
    SPELL_PURSUIT_LASER             = 143828,
    SPELL_LASER_GROUND_PERIODIC_AT  = 143829,
    SPELL_SUPERHEATER               = 144040,
};

enum eEvents
{
    EVENT_SAWBLADE                  = 1,
    EVENT_ELECTROSTATIC_CHARGE      = 2,
    //Weapon
    EVENT_ACTIVE                    = 3,
    //Crawler Mine
    EVENT_PURSUE                    = 4,
    EVENT_CHECK_DISTANCE            = 5,
    //Automated Shredder
    EVENT_DEATH_FROM_ABOVE          = 10,
    //Special
    EVENT_LAUNCH_BACK               = 11,
};

enum _ATentry
{
    ENTER                           = 9250,
    ENTER_2                         = 9251,
    ENTER_3                         = 9252,
    LEAVE                           = 9253,
    LEAVE_2                         = 9371,
    LEAVE_3                         = 9240,
    ENTER_CONVEYOR                  = 9189,
    ENTER_CONVEYOR_2                = 9190,
    LEAVE_CONVEYOR                  = 9493,
    LEAVE_CONVEYOR_2                = 9194,
    LEAVE_CONVEYOR_3                = 9238,
    LEAVE_CONVEYOR_4                = 9239,
};

Position atdestpos[4] =
{
    { 1927.78f, -5566.85f, -309.3259f, 5.2955f },
    { 1889.66f, -5511.84f, -294.8935f, 2.1186f },
    { 1994.60f, -5503.39f, -302.8841f, 2.1813f },
    { 2008.93f, -5600.63f, -309.3268f, 3.6947f },
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

static WeaponWaveArray wavearray[6] =
{
    {0, NPC_DEACTIVATED_MISSILE_TURRET, NPC_DISASSEMBLED_CRAWLER_MINE, NPC_DEACTIVATED_LASER_TURRET},
    {1, NPC_DISASSEMBLED_CRAWLER_MINE, NPC_DEACTIVATED_LASER_TURRET, NPC_DEACTIVATED_MISSILE_TURRET},
    {2, NPC_DEACTIVATED_LASER_TURRET, NPC_DEACTIVATED_ELECTROMAGNET, NPC_DEACTIVATED_MISSILE_TURRET},
    {3, NPC_DEACTIVATED_LASER_TURRET, NPC_DEACTIVATED_MISSILE_TURRET, NPC_DISASSEMBLED_CRAWLER_MINE},
    {4, NPC_DEACTIVATED_MISSILE_TURRET, NPC_DEACTIVATED_ELECTROMAGNET, NPC_DISASSEMBLED_CRAWLER_MINE},
    {5, NPC_DISASSEMBLED_CRAWLER_MINE, NPC_DEACTIVATED_LASER_TURRET, NPC_DISASSEMBLED_CRAWLER_MINE}
};

Position spawnweaponpos[3] =
{
    { 1973.65f, -5472.38f, -299.0f, 5.294743f },
    { 1958.50f, -5450.76f, -299.0f, 5.294743f },
    { 1941.65f, -5425.50f, -299.0f, 5.294743f },
};

Position destpos = { 2073.01f, -5620.12f, -302.2553f};
Position cmdestpos = { 1905.39f, -5631.86f, -309.3265f };

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
             events.ScheduleEvent(EVENT_ELECTROSTATIC_CHARGE, 2000);
             events.ScheduleEvent(EVENT_SAWBLADE, 7000);
         }

         void EnterEvadeMode()
         {
             me->Kill(me, true);
             me->Respawn(true);
             me->GetMotionMaster()->MoveTargetedHome();
         }

         void CreateWeaponWave(uint8 wavecount)
         {
             wavecount = wavecount >= 5 ? 0 : wavecount;
             if (Creature* firstdweapon = me->SummonCreature(wavearray[wavecount].firstentry, spawnweaponpos[0]))
                 firstdweapon->GetMotionMaster()->MoveCharge(destpos.GetPositionX(), destpos.GetPositionY(), destpos.GetPositionZ(), 7.0f, false);
             if (Creature* seconddweapon = me->SummonCreature(wavearray[wavecount].secondentry, spawnweaponpos[1]))
                 seconddweapon->GetMotionMaster()->MoveCharge(destpos.GetPositionX(), destpos.GetPositionY(), destpos.GetPositionZ(), 7.0f, false);
             if (Creature* thirddweapon = me->SummonCreature(wavearray[wavecount].thirdentry, spawnweaponpos[2]))
                 thirddweapon->GetMotionMaster()->MoveCharge(destpos.GetPositionX(), destpos.GetPositionY(), destpos.GetPositionZ(), 7.0f, false);
             weaponwavecount++;
         }

         void CreateLaserWalls()
         {
             uint8 mod = urand(0, 4);
             switch (mod)
             {
             case 0:
                 for (uint8 n = 0; n < 5; ++n) 
                     if (n != 1 && n != 2)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n) 
                     if (n != 3 && n != 4)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n) 
                     if (n != 0 && n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 1:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2 && n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 0 && n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3 && n != 4)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 2:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3 && n != 4)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2 && n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 0 && n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 3:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 0 && n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3 && n != 4)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2 && n != 3)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos3[n]);
                 break;
             case 4:
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 3 && n != 4)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 0 && n != 1)
                         me->SummonCreature(NPC_LASER_ARRAY, lapos2[n]);
                 for (uint8 n = 0; n < 5; ++n)
                     if (n != 2 && n != 3)
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

             events.Update(diff);

             if (me->HasUnitState(UNIT_STATE_CASTING))
                 return;

             while (uint32 eventId = events.ExecuteEvent())
             {
                 switch (eventId)
                 {
                 case EVENT_SAWBLADE:
                 {
                     std::list<Player*> pllist;
                     pllist.clear();
                     GetPlayerListInGrid(pllist, me, 150.0f);
                     if (!pllist.empty())
                     {
                         for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                         {
                             if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && !(*itr)->HasAura(SPELL_PATTERN_RECOGNITION))
                             {
                                 DoCast(*itr, SPELL_LAUNCH_SAWBLADE);
                                 break;
                             }
                         }
                     }
                     events.ScheduleEvent(EVENT_SAWBLADE, 12000);
                     break;
                 }
                 case EVENT_ELECTROSTATIC_CHARGE:
                     if (me->getVictim())
                         DoCastVictim(SPELL_ELECTROSTATIC_CHARGE);
                     events.ScheduleEvent(EVENT_ELECTROSTATIC_CHARGE, 2000);
                     break;
                 }
             }
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

        void Reset(){}

        void EnterCombat(Unit* who){}

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
            DoCast(me, SPELL_REACTIVE_ARMOR, true);
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
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->SetReactState(REACT_PASSIVE);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            targetGuid = 0;
            done = false;
        }

        InstanceScript* instance;
        EventMap events;
        uint64 targetGuid;
        bool done;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void JustDied(Unit* killer)
        {
            if (instance)
                instance->SetData(DATA_SAFE_WEAPONS, me->GetEntry());
            me->DespawnOrUnsummon(1000);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            switch (type)
            {
            case POINT_MOTION_TYPE:
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
                    switch (me->GetEntry())
                    {
                    case NPC_BLACKFUSE_CRAWLER_MINE:
                        events.ScheduleEvent(EVENT_ACTIVE, 4000);
                        break;
                    case NPC_ACTIVATED_LASER_TURRET:
                        events.ScheduleEvent(EVENT_ACTIVE, 3000);
                        break;
                    case NPC_ACTIVATED_ELECTROMAGNET:
                        break;
                    case NPC_ACTIVATED_MISSILE_TURRET:
                        break;
                    }
                    break;
                }
                break;
            }
            case EFFECT_MOTION_TYPE:
                if (pointId == 3)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    events.ScheduleEvent(EVENT_PURSUE, 500);
                }
                break;
            }
        }

        void StartDisentegrationLaser()
        {
            if (me->ToTempSummon())
            {
                if (Unit* blackfuse = me->ToTempSummon()->GetSummoner())
                {
                    std::list<Player*>pllist;
                    GetPlayerListInGrid(pllist, me, 150.0f);
                    if (!pllist.empty())
                    {
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                        {
                            if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && !(*itr)->HasAura(SPELL_PATTERN_RECOGNITION))
                            {
                                if (Creature* laser = blackfuse->SummonCreature(NPC_LASER_TARGET, (*itr)->GetPositionX() + 10.0f, (*itr)->GetPositionY(), blackfuse->GetPositionZ()))
                                {
                                    laser->CastSpell(*itr, SPELL_PURSUIT_LASER);
                                    DoCast(laser, SPELL_DISINTEGRATION_LASER_V);
                                    laser->CastSpell(laser, SPELL_LASER_GROUND_PERIODIC_AT);
                                    laser->AddThreat(*itr, 50000000.0f);
                                    laser->SetReactState(REACT_AGGRESSIVE);
                                    laser->TauntApply(*itr);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        void DamageTaken(Unit* attacker, uint32 &dmg)
        {
            me->getThreatManager().addThreat(attacker, 0.0f);
        }

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_ACTIVE:
                {
                    switch (me->GetEntry())
                    {
                    case NPC_BLACKFUSE_CRAWLER_MINE:
                        me->GetMotionMaster()->MoveJump(cmdestpos.GetPositionX(), cmdestpos.GetPositionY(), cmdestpos.GetPositionZ(), 15.0f, 15.0f, 3);
                        break;
                    case NPC_ACTIVATED_LASER_TURRET:
                        StartDisentegrationLaser();
                        break;
                    }
                }
                break;
                case EVENT_PURSUE:
                {
                    std::list<Player*>pllist;
                    GetPlayerListInGrid(pllist, me, 150.0f);
                    if (!pllist.empty())
                    {
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                        {
                            if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && !(*itr)->HasAura(SPELL_PATTERN_RECOGNITION) && !(*itr)->HasAura(SPELL_PURSUIT_LASER))
                            {
                                DoCast(*itr, SPELL_CRAWLER_MINE_FIXATE, true);
                                targetGuid = (*itr)->GetGUID();
                                DoCast(me, SPELL_BREAK_IN_PERIOD, true);
                                me->AddThreat(*itr, 50000000.0f);
                                me->SetReactState(REACT_AGGRESSIVE);
                                me->TauntApply(*itr);
                                break;
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_CHECK_DISTANCE, 1000);
                }
                break;
                case EVENT_CHECK_DISTANCE:
                {
                    if (Player* pl = me->GetPlayer(*me, targetGuid))
                    {
                        if (pl->isAlive())
                        {
                            if (me->GetDistance(pl) <= 6.0f && !done)
                            {
                                done = true;
                                pl->RemoveAurasDueToSpell(SPELL_CRAWLER_MINE_FIXATE);
                                me->GetMotionMaster()->Clear(false);
                                DoCast(pl, SPELL_DETONATE, true);
                                me->DespawnOrUnsummon(1000);
                                return;
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_CHECK_DISTANCE, 1000);
                }
                break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_blackfuse_weaponAI(creature);
    }
};

//71910, 71740, 72710, 72052
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
            {
                me->AddAura(SPELL_CONVEYOR_DEATH_BEAM_V, me);
                DoCast(me, SPELL_CONVEYOR_DEATH_BEAM_AT, true);
            }
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

//143265
class spell_blackfuse_launch_sawblade : public SpellScriptLoader
{
public:
    spell_blackfuse_launch_sawblade() : SpellScriptLoader("spell_blackfuse_launch_sawblade") { }

    class spell_blackfuse_launch_sawblade_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blackfuse_launch_sawblade_SpellScript);

        void HandleHit()
        {
            if (GetCaster() && GetHitUnit())
            { 
                Position pos;
                GetCaster()->GetNearPosition(pos, 7.0f, 5.5f);
                if (Creature* sawblade = GetCaster()->SummonCreature(NPC_BLACKFUSE_SAWBLADE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + 2.0f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    sawblade->AddAura(SPELL_LAUNCH_SAWBLADE_AT, sawblade);
                    sawblade->GetMotionMaster()->MoveCharge(GetHitUnit()->GetPositionX(), GetHitUnit()->GetPositionY(), GetHitUnit()->GetPositionZ() + 2.0f, 15.0f);
                }
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_blackfuse_launch_sawblade_SpellScript::HandleHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_blackfuse_launch_sawblade_SpellScript();
    }
};

//145269
class spell_break_in_period : public SpellScriptLoader
{
public:
    spell_break_in_period() : SpellScriptLoader("spell_break_in_period") { }

    class spell_break_in_period_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_break_in_period_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                GetTarget()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                GetTarget()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                GetTarget()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                GetTarget()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                GetTarget()->CastSpell(GetTarget(), SPELL_READY_TO_GO);
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_break_in_period_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_break_in_period_AuraScript();
    }
};

//143867
class spell_disintegration_laser : public SpellScriptLoader
{
public:
    spell_disintegration_laser() : SpellScriptLoader("spell_disintegration_laser") { }

    class spell_disintegration_laser_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_disintegration_laser_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                if (GetTarget()->ToCreature())
                    GetTarget()->ToCreature()->DespawnOrUnsummon();
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_disintegration_laser_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_disintegration_laser_AuraScript();
    }
};

//9250, 9251, 9252, 9253, 9371, 9240, 9189, 9190, 9493, 9194, 9238, 9239, 
class at_blackfuse_pipe : public AreaTriggerScript
{
public:
    at_blackfuse_pipe() : AreaTriggerScript("at_blackfuse_pipe") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger, bool enter)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (enter)
            {
                switch (areaTrigger->id)
                {
                case ENTER:
                case ENTER_2:
                case ENTER_3:
                    if (instance->GetBossState(DATA_BLACKFUSE) != IN_PROGRESS)
                        player->NearTeleportTo(atdestpos[0].GetPositionX(), atdestpos[0].GetPositionY(), atdestpos[0].GetPositionZ(), atdestpos[0].GetOrientation());
                    break;
                case LEAVE:
                case LEAVE_2:
                case LEAVE_3:
                    if (instance->GetBossState(DATA_BLACKFUSE) != IN_PROGRESS)
                        player->NearTeleportTo(atdestpos[1].GetPositionX(), atdestpos[1].GetPositionY(), atdestpos[1].GetPositionZ(), atdestpos[1].GetOrientation());
                    break;
                case ENTER_CONVEYOR:
                case ENTER_CONVEYOR_2:
                    if (instance->GetBossState(DATA_BLACKFUSE) == IN_PROGRESS)
                    {
                        if (!player->HasAura(SPELL_PATTERN_RECOGNITION))
                        {
                            player->CastSpell(player, SPELL_PATTERN_RECOGNITION, true);
                            player->NearTeleportTo(atdestpos[2].GetPositionX(), atdestpos[2].GetPositionY(), atdestpos[2].GetPositionZ(), atdestpos[2].GetOrientation());
                        }
                    }
                    break;
                case LEAVE_CONVEYOR:
                case LEAVE_CONVEYOR_2:
                case LEAVE_CONVEYOR_3:
                case LEAVE_CONVEYOR_4:
                    player->NearTeleportTo(atdestpos[3].GetPositionX(), atdestpos[3].GetPositionY(), atdestpos[3].GetPositionZ(), atdestpos[3].GetOrientation());
                    break;
                }
            }
        }
        return true;
    }
};

void AddSC_boss_siegecrafter_blackfuse()
{
    new boss_siegecrafter_blackfuse();
    new npc_blackfuse_passenger();
    new npc_automated_shredder();
    new npc_blackfuse_weapon();
    new npc_blackfuse_trigger();
    new spell_blackfuse_launch_sawblade();
    new spell_break_in_period();
    new spell_disintegration_laser();
    new at_blackfuse_pipe();
}
