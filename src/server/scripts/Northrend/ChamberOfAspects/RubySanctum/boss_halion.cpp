/*
* Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptPCH.h"
#include "ruby_sanctum.h"

enum
{
    SPELL_TWILIGHT_PRECISION         = 78243, // Increases Halion's chance to hit by 5% and decreases all players' chance to dodge by 20%
    SPELL_BERSERK                    = 26662, // Increases the caster's attack and movement speeds by 150% and all damage it deals by 500% for 5 min. Also grants immunity to Taunt effects.
    SPELL_START_PHASE2               = 74808, // Phases the caster into the Twilight realm, leaving behind a large rift.
    SPELL_TWILIGHT_ENTER             = 74807, // Phases the caster into the Twilight realm - phase 32
    SPELL_TWILIGHT_ENTER2            = 74812, //
    SPELL_SUMMON_TWILIGHT_PORTAL     = 74809, //

    SPELL_FIRE_PILLAR                = 76006, // Visual intro
    SPELL_FIERY_EXPLOSION            = 76010, // Visual intro

    //NEED SCRIPT
    SPELL_CLEAVE                     = 74524,
    SPELL_TAIL_LASH                  = 74531, // A sweeping tail strike hits all enemies behind the caster, inflicting 3063 to 3937 damage and stunning them for 2 sec.
    SPELL_TWILIGHT_DIVISION          = 75063, // Phases the caster, allowing him to exist and act simultaneously in both the material and Twilight realms.
    SPELL_TWILIGHT_CUTTER            = 77844, // Inflicts 13,875 to 16,125 Shadow damage every second to players touched by the shadow beam
    SPELL_TWILIGHT_CUTTER_CHANNEL    = 74768, // Channeling shadow cutter visual + trigger 74769
    
    //CORPOREALITY
    SPELL_CORPOREALITY_EVEN           = 74826, // Deals & receives normal damage
    SPELL_CORPOREALITY_20I            = 74827, // Damage dealt increased by 10% & Damage taken increased by 15%
    SPELL_CORPOREALITY_40I            = 74828, // Damage dealt increased by 30% & Damage taken increased by 50%
    SPELL_CORPOREALITY_60I            = 74829, // Damage dealt increased by 60% & Damage taken increased by 100%
    SPELL_CORPOREALITY_80I            = 74830, // Damage dealt increased by 100% & Damage taken increased by 200%
    SPELL_CORPOREALITY_100I           = 74831, // Damage dealt increased by 200% & Damage taken increased by 400%
    SPELL_CORPOREALITY_20D            = 74832, // Damage dealt reduced by 10% & Damage taken reduced by 15%
    SPELL_CORPOREALITY_40D            = 74833, // Damage dealt reduced by 30% & Damage taken reduced by 50%
    SPELL_CORPOREALITY_60D            = 74834, // Damage dealt reduced by 60% & Damage taken reduced by 100%
    SPELL_CORPOREALITY_80D            = 74835, // Damage dealt reduced by 100% & Damage taken reduced by 200%
    SPELL_CORPOREALITY_100D           = 74836, // Damage dealt reduced by 200% & Damage taken reduced by 400%
    //METEOR STRIKE
    SPELL_METEOR                      = 74637, // Script Start (summon NPC_METEOR_STRIKE)
    SPELL_METEOR_IMPACT               = 74641, // IMPACT ZONE FOR METEOR
    SPELL_METEOR_FLAME                = 74718, // FLAME FROM METEOR
    //N10
    SPELL_FLAME_BREATH                = 74525, // Inflicts 17,500 to 22,500 Fire damage to players in front of Halion
    SPELL_DARK_BREATH                 = 74806, // Inflicts 17,500 to 22,500 Shadow damage to players in front of Halion
    SPELL_DUSK_SHROUD                 = 75484, // Inflicts 3,000 Shadow damage every 2 seconds to everyone in the Twilight Realm
    //Combustion
    NPC_COMBUSTION                    = 40001,
    SPELL_MARK_OF_COMBUSTION          = 74567, // Dummy effect only
    SPELL_FIERY_COMBUSTION            = 74562, // Inflicts 4,000 Fire damage every 2 seconds for 30 seconds to a random raider. Every time Fiery Combustion does damage, it applies a stackable Mark of Combustion.
    SPELL_COMBUSTION_EXPLODE          = 74607,
    SPELL_COMBUSTION_AURA             = 74629,
    //Consumption
    NPC_CONSUMPTION                  = 40135,
    SPELL_MARK_OF_CONSUMPTION        = 74795, // Dummy effect only
    SPELL_SOUL_CONSUMPTION           = 74792, // Inflicts 4,000 Shadow damage every 2 seconds for 30 seconds to a random raider. Every time Soul Consumption does damage, it applies a stackable Mark of Consumption.
    SPELL_CONSUMPTION_EXPLODE        = 74799,
    SPELL_CONSUMPTION_AURA           = 74803,
    SPELL_GROW_UP                    = 36300,
    //Summons
    NPC_METEOR_STRIKE                = 40029, //casts "impact zone" then meteor
    NPC_METEOR_STRIKE_1              = 40041,
    NPC_METEOR_STRIKE_2              = 40042,
    NPC_ORB_CUTTER                   = 40081,
    
    FR_RADIUS                        = 45,

    //SAYS
    SAY_HALION_SPAWN                 = -1666100, //17499 Meddlesome insects, you're too late! The Ruby Sanctum is lost.
    SAY_HALION_AGGRO                 = -1666101, //17500 Your world teeters on the brink of annihilation. You will all bear witness to the coming of a new age of destruction!
    SAY_HALION_SLAY_1                = -1666102, //17501 Another hero falls.
    SAY_HALION_SLAY_2                = -1666103, //17502 Ha Ha Ha!
    SAY_HALION_DEATH                 = -1666104, //17503 Relish this victory mortals, for it will be your last. This world will burn with the Master's return!
    SAY_HALION_BERSERK               = -1666105, //17504 Not good enough!
    SAY_HALION_SPECIAL_1             = -1666106, //17505 The heavens burn!
    SAY_HALION_SPECIAL_2             = -1666107, //17506 Beware the shadow!
    SAY_HALION_PHASE_2               = -1666108, //17507 You will find only suffering within the realm of Twilight. Enter if you dare.
    SAY_HALION_PHASE_3               = -1666109, //17508 I am the light AND the darkness! Cower mortals before the Herald of Deathwing!
    EMOTE_WARNING                    = -1666110, //orbs charge warning
    EMOTE_REAL_PUSH                  = -1666111, // Out of real world message
    EMOTE_REAL_PULL                  = -1666112, // To real world message
    EMOTE_TWIL_PUSH                  = -1666113, // Out of twilight world message
    EMOTE_TWIL_PULL                  = -1666114, // To twilight world message
};

enum Events
{
    EVENT_INTRO_ONE_PHASE   = 1,
    EVENT_FIERY_COMBUSTION  = 2,
    EVENT_SOUL_CONSUMPTION  = 3,
    EVENT_CLEAVE            = 4, 
    EVENT_TAILLASH          = 5,
    EVENT_FLAME             = 6,
    EVENT_METEOR            = 7,
    EVENT_CHECK_EVADE       = 8,
};

enum Phase
{
    PHASE_NONE        = 0,
    PHASE_NULL        = 1,
    PHASE_ONE         = 2,
    PHASE_TWO         = 3,
    PHASE_THREE       = 4,
    PHASE_INTRO_TWO   = 5,
    PHASE_INTRO_THREE = 6,
};

enum Actions
{
    ACTION_INTRO_THREE_PHASE,
    ACTION_INTRO_TWO_PHASE,
    ACTION_EVADE,
};

struct Locations
{
    float x, y, z;
};

const Position portalpos[2] =
{
    {3152.4760f, 507.4405f, 72.8887f},
    {3140.2194f, 549.6845f, 72.8887f},
};

const Position Center = {3154.99f, 535.637f, 72.8887f};

static Locations SpawnLoc[]=
{
    {3154.99f, 535.637f, 72.8887f}, // 0 - Halion spawn point (center)
};

enum MovementPoints
{
    POINT_START                 = 0,
};

#define SPELL_METEOR_STRIKE RAID_MODE(74648, 74648, 75878, 75878) 

/*######
## boss_halion_real (Physical version)
######*/
float Rdmg, Tdmg;

class boss_halion_real : public CreatureScript
{
public:
    boss_halion_real() : CreatureScript("boss_halion_real") { }
    
    struct boss_halion_realAI : public BossAI
    {
        boss_halion_realAI(Creature* pCreature) : BossAI(pCreature, BOSS_HALION_REAL), summons(me)
        {
            me->SetVisible(false);
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;

        Phase phase;
        SummonList summons;

        uint32 enragetimer;
        uint32 HealingTimer;
        uint32 ChangeVictim;
        bool introphasethree;
        bool introphasetwo;
        bool MovementStarted;

        void Reset()
        {
            if (!pInstance)
                return;

            me->SetFullHealth();
            pInstance->SetData(TYPE_HALION, NOT_STARTED);
            pInstance->SetData(TYPE_HALION_EVENT, FAIL);
            if (GameObject* Ring = me->FindNearestGameObject(203007, 80.0f))
            {
                if (Ring)
                    Ring->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
            }
            events.Reset();
            events.SetPhase(PHASE_NULL);
            phase = PHASE_NULL;
            me->SetDisplayId(31952);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
            me->SetReactState(REACT_DEFENSIVE);
            introphasethree = false;
            introphasetwo = false;
            enragetimer = 0;
            ChangeVictim = 0;
            HealingTimer = 0;
            Rdmg = 0;
            SetCombatMovement(true);
            me->RemoveAurasDueToSpell(SPELL_TWILIGHT_ENTER);
            me->LowerPlayerDamageReq(me->GetMaxHealth());

            if (GameObject* pGoPortal = me->FindNearestGameObject(GO_HALION_PORTAL_1, 50.0f))
                pGoPortal->Delete();
            if (GameObject* pGoPortal = me->FindNearestGameObject(GO_HALION_PORTAL_2, 50.0f))
                pGoPortal->Delete();
            if (GameObject* pGoPortal = me->FindNearestGameObject(GO_HALION_PORTAL_3, 50.0f))
                pGoPortal->Delete();
        }
        
        void JustReachedHome()
        {
            if (!pInstance)
                return;

            if (pInstance->GetData(TYPE_HALION_EVENT) != FAIL)
                return;
                
            ScriptedAI::JustReachedHome();
        }

        void EnterEvadeMode()
        {
            if (!pInstance)
                return;
            
            if (phase == PHASE_ONE)
            {
                summons.DespawnAll();
                ScriptedAI::EnterEvadeMode();
            }

        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
        }

        void JustDied(Unit* pKiller)
        {
            if (!pInstance)
                return;

            DoScriptText(-1666104, me);
            pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_ENTER);
            if (GameObject* Ring = me->FindNearestGameObject(203007, 80.0f))
            {
                if (Ring)
                    Ring->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
            }
            summons.DespawnAll();
            pInstance->SetData(TYPE_HALION, DONE);
            pInstance->SetData(TYPE_COUNTER, COUNTER_OFF);
        }

        void KilledUnit(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
            {
                switch (urand(0,1))
                {
                case 0:
                    DoScriptText(-1666106, me, who);
                    break;
                case 1:
                    DoScriptText(-1666107, me, who);
                    break;
                }
            }
        }

        void EnterCombat(Unit* pWho)
        {
            if (!pInstance)
                return;

            if (GameObject* Ring = me->FindNearestGameObject(203007, 80.0f))
            {
                if (Ring)
                {
                    Ring->SetGoState(GO_STATE_READY);
                    Ring->SetPhaseMask(65535, true);
                }
            }
            else if (GameObject* Ring = me->SummonGameObject(203007, 3154.99f, 535.637f, 72.8887f, 3.14159f, 0, 0, 0, 1, 300))
            {
                if (Ring)
                {
                    Ring->SetGoState(GO_STATE_READY);
                    Ring->SetPhaseMask(65535, true);
                }
            }
            events.SetPhase(PHASE_ONE);
            phase = PHASE_ONE;
            DoCast(SPELL_TWILIGHT_PRECISION);
            me->SetInCombatWithZone();
            pInstance->SetData(TYPE_HALION, IN_PROGRESS);
            DoScriptText(-1666101, me);
            enragetimer = 480000;
            events.RescheduleEvent(EVENT_FLAME, urand(10000, 18000));
            events.RescheduleEvent(EVENT_FIERY_COMBUSTION, urand(30000, 40000));
            events.RescheduleEvent(EVENT_METEOR, urand(30000, 35000));
            events.RescheduleEvent(EVENT_TAILLASH, urand(15000, 25000));
            events.RescheduleEvent(EVENT_CLEAVE, urand(10000, 15000));
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 0 && phase == PHASE_INTRO_TWO)
            {
                me->GetMotionMaster()->MovementExpired();
                DoScriptText(-1666108, me);
                DoCast(me, SPELL_SUMMON_TWILIGHT_PORTAL);
                if (GameObject* pGoPortal = pInstance->instance->GetGameObject(pInstance->GetData64(GO_HALION_PORTAL_1)))
                {
                    pGoPortal->SetPhaseMask(31,true);
                    me->SetDisplayId(11686);
                }

               if (Creature* pTwilight = me->SummonCreature(NPC_HALION_TWILIGHT, Center, TEMPSUMMON_MANUAL_DESPAWN, 1000))
               {
                   pTwilight->SetPhaseMask(32, true);
                   pTwilight->SetHealth(me->GetHealth());
               }
            }
        }
        
        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (!pInstance)
                return;

            if (attacker->GetTypeId() != TYPEID_PLAYER)
                return;

            Rdmg += damage;

            Creature * pHalionTwilight = me->GetCreature(*me, pInstance->GetData64(NPC_HALION_TWILIGHT));
            
            if (!pHalionTwilight)
                return;

            if (pHalionTwilight->GetHealth() <= 1 || !pHalionTwilight->isAlive())
                return;

            if (damage < me->GetHealth())
            {
                pHalionTwilight->SetHealth(me->GetHealth()- damage);
            }
        }

        void DoAction(const int32 action)
        {
            switch(action)
            {
            case ACTION_INTRO_TWO_PHASE:
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                me->InterruptNonMeleeSpells(true);
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->GetMotionMaster()->MovePoint(0, Center);
                break;
            case ACTION_INTRO_THREE_PHASE:
                if (GameObject* pGoPortal = me->FindNearestGameObject(GO_HALION_PORTAL_1, 50.0f))
                    pGoPortal->Delete();

                Position pos;
                me->GetPosition(&pos);
                if (Creature* summoner = me->SummonCreature(40157, pos, TEMPSUMMON_MANUAL_DESPAWN))// For Cata 40157, Lk 
                {
                    summoner->SetPhaseMask(31, true);
                    
                    for (int32 i = 0; i < 2; i++)
                    {
                        summoner->SummonGameObject(GO_HALION_PORTAL_2, portalpos[i].m_positionX, portalpos[i].m_positionY, portalpos[i].m_positionZ, 4.47206f, 0, 0, 0.786772f, -0.617243f, DAY);
                        if (GameObject* portal = me->GetGameObject(GO_HALION_PORTAL_3))
                            portal->SetPhaseMask(31, true);
                    }
                }
                if (Creature* Control = me->SummonCreature(40146, Center, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Control->SetPhaseMask(65535, true);
                }
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetDisplayId(31952);
                me->SetReactState(REACT_AGGRESSIVE);
                SetCombatMovement(true);
                events.SetPhase(PHASE_THREE);
                phase = PHASE_THREE;
                introphasethree = true;
                events.RescheduleEvent(EVENT_FLAME, urand(10000, 18000));
                events.RescheduleEvent(EVENT_FIERY_COMBUSTION, urand(30000, 40000));
                events.RescheduleEvent(EVENT_METEOR, urand(30000, 35000));
                events.RescheduleEvent(EVENT_TAILLASH, urand(15000, 25000));
                events.RescheduleEvent(EVENT_CLEAVE, urand(10000, 15000));
                break;
            case ACTION_EVADE:
                summons.DespawnAll();
                ScriptedAI::EnterEvadeMode();
                break;
            case ACTION_INTRO_HALION:
                DoCast(me, 76006);
                me->SetVisible(true);
                DoScriptText(-1666100, me);
                break;
            }
                
        }

        void UpdateAI(const uint32 diff)
        {
            if (!pInstance)
                return;
            
            if (ChangeVictim && phase == PHASE_THREE)
            {
                if (ChangeVictim <= diff)
                {
                    ChangeVictim = 0;
                    me->GetMotionMaster()->MovePoint(0, Center);
                    me->SetReactState(REACT_AGGRESSIVE);
                }
                else ChangeVictim -= diff;
            }
            
            if (me->getVictim() && phase == PHASE_THREE)
            {
                if (me->getVictim()->GetPhaseMask() != 1)
                {
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    ChangeVictim = 3000;
                }
            }

            if (!UpdateVictim() && phase == PHASE_ONE)
                EnterEvadeMode();
            else if (!UpdateVictim() && phase == PHASE_THREE && !HealingTimer)
            {
                if (Creature* THalion = me->GetCreature(*me, pInstance->GetData64(NPC_HALION_TWILIGHT)))
                {
                    if (THalion && THalion->isAlive())
                    {
                        if (THalion->getVictim())
                            HealingTimer = 5000;
                    }
                }
            }
            
            if (HealthBelowPct(76) && phase == PHASE_ONE && !introphasetwo)
            {
                introphasetwo = true;
                me->MonsterTextEmote("Phase 2", 0, true);
                phase = PHASE_INTRO_TWO;
                events.SetPhase(PHASE_INTRO_TWO);
                DoAction(ACTION_INTRO_TWO_PHASE);
            }
            
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
            
            if (HealingTimer && phase == PHASE_THREE)
            {
                if (HealingTimer <= diff)
                {
                    if (!me->getVictim())
                    {
                        if (Creature* Thalion = me->GetCreature(*me, pInstance->GetData64(NPC_HALION_TWILIGHT)))
                        {
                            if (Thalion && Thalion->isAlive())
                            {
                                if (IsHeroic())
                                {
                                    me->SetHealth(me->GetHealth()+ 306790);
                                    Thalion->SetHealth(Thalion->GetHealth()+ 306790);
                                }
                                else
                                {
                                    me->SetHealth(me->GetHealth()+ 223123);
                                    Thalion->SetHealth(Thalion->GetHealth()+ 223123);
                                }
                                HealingTimer = 0;
                            }
                        }
                    }
                    else
                        HealingTimer = 0;
                }
                else HealingTimer -= diff;
            }

            if (enragetimer)
            {
                if (enragetimer <= diff)
                {
                    enragetimer = 0;
                    DoScriptText(-1666105, me);
                    me->AddAura(SPELL_BERSERK, me);
                }
                else enragetimer -= diff;
            }
            
            events.Update(diff);
            
            if (me->getVictim() && (phase == PHASE_ONE || phase == PHASE_THREE))
            {
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_FLAME:
                        DoCast(SPELL_FLAME_BREATH);
                        events.RescheduleEvent(EVENT_FLAME, urand(10000, 18000));
                        break;
                    case EVENT_FIERY_COMBUSTION:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, true))
                            DoCast(target, SPELL_FIERY_COMBUSTION);
                        else if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true)) 
                            DoCast(pTarget, SPELL_FIERY_COMBUSTION);
                        events.RescheduleEvent(EVENT_FIERY_COMBUSTION, urand(30000, 40000));
                        break;
                    case EVENT_METEOR:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60, true))
                        {
                            Position pos;
                            target->GetPosition(&pos);
                            me->SummonCreature(40029, pos, TEMPSUMMON_CORPSE_DESPAWN);
                        }
                        events.RescheduleEvent(EVENT_METEOR, urand(30000, 35000));
                        break;
                    case EVENT_TAILLASH:
                        DoCast(SPELL_TAIL_LASH);
                        events.RescheduleEvent(EVENT_TAILLASH, urand(15000, 25000));
                        break;
                    case EVENT_CLEAVE:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_CLEAVE);
                        events.RescheduleEvent(EVENT_CLEAVE, urand(10000, 15000));
                        break;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }
    };
    
    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_halion_realAI(pCreature);
    }
};

/*######
## boss_halion_twilight (Twilight version)
######*/

class boss_halion_twilight : public CreatureScript
{
public:
    boss_halion_twilight() : CreatureScript("boss_halion_twilight") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_halion_twilightAI(pCreature);
    }

    struct boss_halion_twilightAI : public BossAI
    {
        boss_halion_twilightAI(Creature* pCreature) : BossAI(pCreature, BOSS_HALION_TWILIGHT), summons(me)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;

        Phase phase;
        SummonList summons;

        uint32 HealingTimer;
        uint32 ChangeVictim;
        uint32 CheckEvade;
        bool introthreephase;
        bool enrage;
        
        void Reset()
        {
            if (!pInstance)
                return;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
            me->SetReactState(REACT_AGGRESSIVE);
            introthreephase = false;
            enrage = false;
            CheckEvade = 15000;
            HealingTimer = 0;
            ChangeVictim = 0;
            phase = PHASE_INTRO_TWO;
            events.SetPhase(PHASE_INTRO_TWO);
            DoCast(me, 75476);
            Tdmg = 0;
            
            Creature* pFocus = me->GetMap()->GetCreature(pInstance->GetData64(NPC_ORB_ROTATION_FOCUS));
            if (!pFocus )
                pFocus = me->SummonCreature(NPC_ORB_ROTATION_FOCUS, SpawnLoc[0].x, SpawnLoc[0].y, SpawnLoc[0].z, 0, TEMPSUMMON_MANUAL_DESPAWN);
            else if (!pFocus->isAlive())
                pFocus->Respawn();
 
            if (!me->HasAura(SPELL_TWILIGHT_ENTER))
                 DoCast(me, SPELL_TWILIGHT_ENTER);
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
        }
        
        void JustReachedHome()
        {
            if (!pInstance)
                return;

            if (pInstance->GetData(TYPE_HALION_EVENT) != FAIL)
                return;

            ScriptedAI::JustReachedHome();
        }

        void EnterEvadeMode(){}
        
        void JustDied(Unit* pKiller)
        {
            if (!pInstance)
                return;

            summons.DespawnAll();
            pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_ENTER);

            if (Unit* Rhalion = me->ToTempSummon()->GetSummoner())
            {
                if (Rhalion && Rhalion->isAlive())
                {
                    me->Kill(Rhalion, true);
                }
            }
        }

        void KilledUnit(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
            {
                switch (urand(0,1))
                {
                case 0:
                    DoScriptText(-1666106, me, who);
                    break;
                case 1:
                    DoScriptText(-1666107, me, who);
                    break;
                }
            }
        }

        void EnterCombat(Unit* pWho)
        {
            if (!pInstance)
                return;

            CheckEvade = 0;
            events.SetPhase(PHASE_TWO);
            phase = PHASE_TWO;
            DoZoneInCombat();
            DoCast(SPELL_TWILIGHT_PRECISION);
            events.RescheduleEvent(EVENT_FLAME, urand(10000, 18000));
            events.RescheduleEvent(EVENT_SOUL_CONSUMPTION, urand(30000, 40000));
            events.RescheduleEvent(EVENT_TAILLASH, urand(15000, 25000));
            events.RescheduleEvent(EVENT_CLEAVE, urand(10000, 15000));

        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (!pInstance)
                return;

            if (attacker->GetTypeId() != TYPEID_PLAYER)
                return;

            Tdmg += damage;
            Creature * pHalionReal = me->GetCreature(*me, pInstance->GetData64(NPC_HALION_REAL));

            if (!pHalionReal)
                return;

            if (pHalionReal->GetHealth() <= 1 || !pHalionReal->isAlive())
                return;

            if (damage < me->GetHealth())
            {
                pHalionReal->SetHealth(me->GetHealth()-damage);
            }
        }
        
        void UpdateAI(const uint32 diff)
        {
            if (ChangeVictim && phase == PHASE_THREE)
            {
                if (ChangeVictim <= diff)
                {
                    ChangeVictim = 0;
                    me->GetMotionMaster()->MovePoint(0, Center);
                    me->SetReactState(REACT_AGGRESSIVE);
                }
                else ChangeVictim -= diff;
            }

            if (me->getVictim() && phase == PHASE_THREE)
            {
                if (me->getVictim()->GetPhaseMask() != 32)
                {
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    ChangeVictim = 3000;
                }
            }

            if (CheckEvade)
            {
                if (CheckEvade <= diff)
                {
                    if (Creature* Rhalion = me->ToTempSummon()->GetSummoner()->ToCreature())
                    {
                        if (phase == PHASE_INTRO_TWO || PHASE_TWO)
                        {
                            summons.DespawnAll();
                            Rhalion->AI()->DoAction(ACTION_EVADE);
                            CheckEvade = 0;
                        }
                        else if (phase == PHASE_THREE)
                        {
                            if (Rhalion && Rhalion->isAlive())
                            {
                                if (!Rhalion->getVictim())
                                {
                                    summons.DespawnAll();
                                    Rhalion->AI()->DoAction(ACTION_EVADE);
                                    CheckEvade = 0;
                                }
                                else
                                    CheckEvade = 0;
                            }
                        }
                    }
                }
                else CheckEvade -= diff;
            }
            
            if (!UpdateVictim()) 
            {
                if (phase == PHASE_INTRO_TWO || phase == PHASE_TWO)
                {
                    if (!CheckEvade)
                        CheckEvade = 6000;
                }
                else if (phase == PHASE_THREE)
                {
                    if (Creature* Rhalion = me->ToTempSummon()->GetSummoner()->ToCreature())
                    {
                        if (Rhalion && Rhalion->isAlive())
                        {
                            if (!Rhalion->getVictim())
                            {
                                if (!CheckEvade)
                                    CheckEvade = 6000;
                            }
                            else if (Rhalion->getVictim() && !HealingTimer)
                                HealingTimer = 5000;
                        }
                    }
                }
            }
            
            if (HealthBelowPct(51) && phase == PHASE_TWO && !introthreephase)
            {
                introthreephase = true;
                me->MonsterTextEmote("Phase 3", 0, true);
                DoScriptText(-1666109, me);
                events.SetPhase(PHASE_THREE);
                phase = PHASE_THREE;
                SetCombatMovement(true);
                if (Unit * halionreal = me->ToTempSummon()->GetSummoner())
                {
                    if (halionreal && halionreal->isAlive())
                    {
                        halionreal->ToCreature()->AI()->DoAction(ACTION_INTRO_THREE_PHASE);
                        halionreal->SetHealth(me->GetHealth());
                    }

                    Position pos;
                    me->GetPosition(&pos);
                    if (Creature* summoner = me->SummonCreature(40157, pos, TEMPSUMMON_MANUAL_DESPAWN)) // For Cata 40157, Lk 33004
                    {
                        summoner->SetPhaseMask(32, true);

                        for (int32 i = 0; i < 2; i++)
                        {
                            summoner->SummonGameObject(GO_HALION_PORTAL_3, portalpos[i].m_positionX, portalpos[i].m_positionY, portalpos[i].m_positionZ, 4.47206f, 0, 0, 0.786772f, -0.617243f, DAY);
                            if (GameObject* portal = pInstance->instance->GetGameObject(GO_HALION_PORTAL_3))
                                portal->SetPhaseMask(32, true);
                            
                        }
                    }
                }
            }
            
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
            
            if (HealingTimer && phase == PHASE_THREE)
            {
                if (HealingTimer <= diff)
                {
                    if (!me->getVictim())
                    {
                        if (Creature* Rhalion = me->ToTempSummon()->GetSummoner()->ToCreature())
                        {
                            if (Rhalion && Rhalion->isAlive())
                            {
                                if (IsHeroic())
                                {
                                    me->SetHealth(me->GetHealth()+ 306790);
                                    Rhalion->SetHealth(Rhalion->GetHealth()+ 306790);
                                }
                                else
                                {
                                    me->SetHealth(me->GetHealth()+ 223123);
                                    Rhalion->SetHealth(Rhalion->GetHealth()+ 223123);
                                }
                                HealingTimer = 0;
                            }
                        }
                    }
                    else
                        HealingTimer = 0;
                }
                else HealingTimer -= diff;
            }
            
            if (!enrage)
            {
                if (Creature* RHalion = me->ToTempSummon()->GetSummoner()->ToCreature())
                {
                    if (RHalion && RHalion->isAlive())
                    {
                        if (RHalion->HasAura(SPELL_BERSERK))
                        {
                            enrage = true;
                            me->AddAura(SPELL_BERSERK, me);
                        }
                    }
                }
            }

            events.Update(diff);

            if (me->getVictim() && (phase == PHASE_TWO || phase == PHASE_THREE))
            {
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                    case EVENT_FLAME:
                        DoCast(SPELL_DARK_BREATH);
                        events.RescheduleEvent(EVENT_FLAME, urand(10000, 18000));
                        break;
                    case EVENT_SOUL_CONSUMPTION:
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, true))
                            DoCast(pTarget, SPELL_SOUL_CONSUMPTION);
                        else if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true))
                            DoCast(target, SPELL_SOUL_CONSUMPTION);
                        events.RescheduleEvent(EVENT_SOUL_CONSUMPTION, urand(30000, 40000));
                        break;
                    case EVENT_TAILLASH:
                        DoCast(SPELL_TAIL_LASH);
                        events.RescheduleEvent(EVENT_TAILLASH, urand(15000, 25000));
                        break;
                    case EVENT_CLEAVE:
                        if (me->getVictim())
                            DoCast(me->getVictim(),SPELL_CLEAVE);
                        events.RescheduleEvent(EVENT_CLEAVE, urand(10000, 15000));
                        break;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};


/*######
## mob_halion_control
######*/

struct HalionBuffLine
{
    uint32 real, twilight;  // Buff pair
    float d_diff;           // Displayed Corporeality
    uint32 corp_diff;       // Corporeality diff
};

static HalionBuffLine Buff[]=
{                                                                   
    {SPELL_CORPOREALITY_100D , SPELL_CORPOREALITY_100I, 10.0f, 0}, 
    {SPELL_CORPOREALITY_80D  , SPELL_CORPOREALITY_80I, 8.0f, 10},  
    {SPELL_CORPOREALITY_60D  , SPELL_CORPOREALITY_60I, 6.0f, 20},   
    {SPELL_CORPOREALITY_40D  , SPELL_CORPOREALITY_40I, 4.0f, 30},  
    {SPELL_CORPOREALITY_20D  , SPELL_CORPOREALITY_20I, 2.0f, 40},  
    {SPELL_CORPOREALITY_EVEN , SPELL_CORPOREALITY_EVEN, 1.0f, 50}, 
    {SPELL_CORPOREALITY_20I  , SPELL_CORPOREALITY_20D,  -2.0f, 60}, 
    {SPELL_CORPOREALITY_40I  , SPELL_CORPOREALITY_40D, -4.0f, 70}, 
    {SPELL_CORPOREALITY_60I  , SPELL_CORPOREALITY_60D, -6.0f, 80},  
    {SPELL_CORPOREALITY_80I  , SPELL_CORPOREALITY_80D, -8.0f, 90},  
    {SPELL_CORPOREALITY_100I , SPELL_CORPOREALITY_100D, -10.0f, 100},
};


class mob_halion_control : public CreatureScript
{
public:
    mob_halion_control() : CreatureScript("mob_halion_control") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_halion_controlAI(pCreature);
    }

    struct mob_halion_controlAI : public ScriptedAI
    {
        mob_halion_controlAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        uint32 lastbuffnum, m_lastBuffReal, m_lastBuffTwilight;
        uint32 m_uiCorporealityTimer;
        float m_diff;
        bool firstbuff;

        void Reset()
        {
            if (!pInstance) 
                return;

            m_uiCorporealityTimer = 5*IN_MILLISECONDS;
            me->SetDisplayId(11686);
            me->SetPhaseMask(65535, true);
            SetCombatMovement(false);
            firstbuff = false;
            m_lastBuffReal = 0;
            m_lastBuffTwilight = 0;
            lastbuffnum = 0;
            pInstance->SetData(TYPE_COUNTER, COUNTER_OFF);
        }

        void EnterEvadeMode(){}

        bool doSearchPlayerAtRange(float range)
        {
            Map* pMap = me->GetMap();
            if (pMap && pMap->IsDungeon())
            {
                Map::PlayerList const &PlayerList = pMap->GetPlayers();
                if (!PlayerList.isEmpty())
                   for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                   {
                       if (!i->getSource()->IsInMap(me)) 
                           continue;

                       if (i->getSource()->isAlive() && i->getSource()->IsWithinDistInMap(me, range))
                           return true;
                   }
            }
            return false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!pInstance)
            {
                me->DespawnOrUnsummon();
                return;
            }

            if (m_uiCorporealityTimer <= diff)
            {
                uint8 buffnum;
                buffnum = 0;

                if (!firstbuff)
                {
                    firstbuff = true;
                    buffnum = 5;
                }
                else if (Rdmg > Tdmg)
                {
                    if (lastbuffnum <= 0)
                        buffnum = 0;
                    else
                        buffnum = lastbuffnum - 1;
                }
                else if (Tdmg > Rdmg)
                {
                    if (lastbuffnum >= 10)
                        buffnum = 10;
                    else
                        buffnum = lastbuffnum + 1;
                }
                else if (Rdmg == Tdmg)
                    buffnum = lastbuffnum;
                
                Tdmg = 0;
                Rdmg = 0;
                
                if (Creature* pHalionReal = me->GetCreature(*me, pInstance->GetData64(NPC_HALION_REAL)))
                {
                    if (pHalionReal && pHalionReal->isAlive())
                    {
                        if (m_lastBuffReal)
                           pHalionReal->RemoveAurasDueToSpell(m_lastBuffReal);
                        me->AddAura(Buff[buffnum].real, pHalionReal);
                        m_lastBuffReal = Buff[buffnum].real;
                    }
                }
                if (Creature* pHalionTwilight = me->GetCreature(*me, pInstance->GetData64(NPC_HALION_TWILIGHT)))
                {
                    if (pHalionTwilight && pHalionTwilight->isAlive())
                    {
                        if (m_lastBuffTwilight)
                            pHalionTwilight->RemoveAurasDueToSpell(m_lastBuffTwilight);
                        me->AddAura(Buff[buffnum].twilight, pHalionTwilight);
                        m_lastBuffTwilight = Buff[buffnum].twilight;
                    }
                }
                lastbuffnum = buffnum;
                pInstance->SetData(TYPE_COUNTER, Buff[buffnum].corp_diff);
                m_uiCorporealityTimer = 15*IN_MILLISECONDS;
            } 
            else m_uiCorporealityTimer -= diff;

        }

    };
};

class mob_orb_rotation_focus : public CreatureScript
{
public:
    mob_orb_rotation_focus() : CreatureScript("mob_orb_rotation_focus") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_orb_rotation_focusAI(pCreature);
    }

    struct mob_orb_rotation_focusAI : public ScriptedAI
    {
        mob_orb_rotation_focusAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
			me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
			me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        }

        InstanceScript* pInstance;

        uint32 m_timer;
		uint32 m_launchPulsarTimer;
		uint32 m_launchTriggerTimer;
		uint32 m_launchRaysTimerEnd;
        float m_direction, m_nextdirection;
        bool m_warning;
		bool m_raysActive;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            me->SetPhaseMask(65535, true);
            SetCombatMovement(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_direction = 0.0f;
            m_nextdirection = 0.0f;
            m_timer = 30000;
			m_launchPulsarTimer = 0;
			m_launchTriggerTimer = 0;
			m_launchRaysTimerEnd = 0;
            m_warning = false;
			m_raysActive = false;

            Creature* pPulsar1 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_N));
            if (!pPulsar1 )
            {
                float x,y;
                me->GetNearPoint2D(x, y, FR_RADIUS, m_direction);
                pPulsar1 = me->SummonCreature(NPC_SHADOW_PULSAR_N, x, y, me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
            } 
			else if (!pPulsar1->isAlive())
				pPulsar1->Respawn();

            Creature* pPulsar2 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_S));
            if (!pPulsar2)
            {
                float x,y;
                me->GetNearPoint2D(x, y, FR_RADIUS, m_direction + M_PI);
                pPulsar2 = me->SummonCreature(NPC_SHADOW_PULSAR_S, x, y, me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
            } 
			else if (!pPulsar2->isAlive())
				pPulsar2->Respawn();

            if (me->GetMap()->IsHeroic())
            {
                 Creature* pPulsar3 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_I));
                 if (!pPulsar3)
                 {
                     float x,y;
                     me->GetNearPoint2D(x, y, FR_RADIUS, m_direction + 1.570796326795f);
                     pPulsar3 = me->SummonCreature(NPC_SHADOW_PULSAR_I, x, y, me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                 } 
                 else if (!pPulsar3->isAlive())
                     pPulsar3->Respawn();
                 
                 Creature* pPulsar4 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_W));
                 if (!pPulsar4)
                 {
                     float x,y;
                     me->GetNearPoint2D(x, y, FR_RADIUS, m_direction + M_PI + 1.570796326795f);
                     pPulsar4 = me->SummonCreature(NPC_SHADOW_PULSAR_W, x, y, me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                 } 
                 else if (!pPulsar4->isAlive())
                     pPulsar4->Respawn();
            }
        }

        void EnterEvadeMode(){}
        
        void SearchPlayersAndDamage()
        {
            Map* pMap = me->GetMap();
            if (pMap && pMap->IsDungeon())
            {
				Creature* pPulsar1 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_N));
				Creature* pPulsar2 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_S));
                Creature* pPulsar3 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_I));
                Creature* pPulsar4 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_W));

                Map::PlayerList const &PlayerList = pMap->GetPlayers();
                if (!PlayerList.isEmpty())
				{
                   for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                   {
					   if (!i->getSource())
						   continue;

                       if (!i->getSource()->IsInMap(me)) 
						   continue;

                       if (i->getSource()->isGameMaster()) 
						   continue;

                       if (i->getSource()->isAlive() && i->getSource()->GetPhaseMask() == 32)
					   {
                          if (pPulsar1 && pPulsar2)
                          {
                              float x1 = pPulsar1->GetPositionX();
                              float y1 = pPulsar1->GetPositionY();
                              float x2 = pPulsar2->GetPositionX();
                              float y2 = pPulsar2->GetPositionY();
                              float x3 = i->getSource()->GetPositionX();
                              float y3 = i->getSource()->GetPositionY();

                              float rt = abs((x3-x1) * (y2-y1) - (y3-y1) * (x2-x1));
                              if (rt < 150.0f)//Lk 150.0f
                              {
                                  uint32 damage = (IsHeroic()) ? urand(41625, 48376) : urand(13875, 16126);
                                  i->getSource()->DealDamage(i->getSource(), damage, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                                  pPulsar1->SendSpellNonMeleeDamageLog(i->getSource(), 74769, damage, SPELL_SCHOOL_MASK_SHADOW, 0, 0, false, 0, false); 
                              }
                          }

                          if (pPulsar3 && pPulsar4)
                          {
                              float x3 = pPulsar3->GetPositionX();
                              float y3 = pPulsar3->GetPositionY();
                              float x4 = pPulsar4->GetPositionX();
                              float y4 = pPulsar4->GetPositionY();
                              float x5 = i->getSource()->GetPositionX();
                              float y5 = i->getSource()->GetPositionY();

                              float rt = abs((x5-x3) * (y4-y3) - (y5-y3) * (x4-x3));
                              if (rt < 150.0f)//Lk 150.0f
                              {
                                  uint32 damage = (IsHeroic()) ? urand(41625, 48376) : urand(13875, 16126);
                                  i->getSource()->DealDamage(i->getSource(), damage, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW);
                                  pPulsar1->SendSpellNonMeleeDamageLog(i->getSource(), 74769, damage, SPELL_SCHOOL_MASK_SHADOW, 0, 0, false, 0, false); 
                              }
                          }
					   }
                   }
				}
            }

            return;
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!pInstance) 
                me->DespawnOrUnsummon();

            if (me->GetMap()->IsHeroic())
            {
                uint8 dataorb = 0;

                for (uint32 data = DATA_ORB_S; data <= DATA_ORB_W; data++)
                {
                    if (pInstance->GetData(data) == DONE)
                        dataorb++;
                    else
                        break;
                }
                
                if (dataorb == 4)
                {
                    m_direction = m_nextdirection;
                    m_nextdirection = (m_direction - M_PI/64.0f);
                    if (m_nextdirection < 0.0f )
                        m_nextdirection = m_nextdirection + 2.0f*M_PI;
                    pInstance->SetData(DATA_ORB_DIRECTION, (uint32)(m_nextdirection*1000));
                    pInstance->SetData(DATA_ORB_N, SPECIAL);
                    pInstance->SetData(DATA_ORB_S, SPECIAL);
                    pInstance->SetData(DATA_ORB_I, SPECIAL);
                    pInstance->SetData(DATA_ORB_W, SPECIAL);
                    sLog->outDebug(LOG_FILTER_TSCR, "EventMGR: creature %u send direction %u ",me->GetEntry(),pInstance->GetData(DATA_ORB_DIRECTION));
                }
            }
            else
            {
                if (pInstance->GetData(DATA_ORB_S) == DONE && pInstance->GetData(DATA_ORB_N) == DONE)
                {
                    m_direction = m_nextdirection;
                    m_nextdirection = (m_direction - M_PI/64.0f);
                    if (m_nextdirection < 0.0f )
                        m_nextdirection = m_nextdirection + 2.0f*M_PI;
                    pInstance->SetData(DATA_ORB_DIRECTION, (uint32)(m_nextdirection*1000));
                    pInstance->SetData(DATA_ORB_N, SPECIAL);
                    pInstance->SetData(DATA_ORB_S, SPECIAL);
                    sLog->outDebug(LOG_FILTER_TSCR, "EventMGR: creature %u send direction %u ",me->GetEntry(),pInstance->GetData(DATA_ORB_DIRECTION));
                }
            }
            
            if (m_timer - 6000 <= uiDiff && !m_warning)
            {
                DoScriptText(-1666110,me);
                m_warning = true;
				m_launchPulsarTimer = 5000;
			}

			if (m_warning && m_launchPulsarTimer != 0)
			{
				if (m_launchPulsarTimer <= uiDiff)
				{
					Creature* pPulsar1 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_N));
					Creature* pPulsar2 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_S));
                    Creature* pPulsar3 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_I));
                    Creature* pPulsar4 = me->GetMap()->GetCreature(pInstance->GetData64(NPC_SHADOW_PULSAR_W));

					if (pPulsar1 && pPulsar2)
					{
                        if (pPulsar3 && pPulsar4)
                        {
                            pPulsar3->CastSpell(pPulsar4, 74768, true);
                            pPulsar4->CastSpell(pPulsar3, 74768, true);
                        }
						pPulsar1->CastSpell(pPulsar2, 74768, true);
						pPulsar2->CastSpell(pPulsar1, 74768, true);
						m_launchTriggerTimer = 300;
						m_launchRaysTimerEnd = 9000;
						m_raysActive = true;
					}

					m_launchPulsarTimer = 0;
				}
				else
					m_launchPulsarTimer -= uiDiff;
			}

			if (m_raysActive && m_launchRaysTimerEnd != 0)
			{
				if (m_launchTriggerTimer <= uiDiff)
				{
					SearchPlayersAndDamage();
					
					m_launchTriggerTimer = 100;
				}
				else
					m_launchTriggerTimer -= uiDiff;


				if (m_launchRaysTimerEnd <= uiDiff)
				{
					m_raysActive = false;
					m_launchRaysTimerEnd = 0;
					m_launchTriggerTimer = 0;
				}
				else
					m_launchRaysTimerEnd -= uiDiff;
			}

            if (m_timer <= uiDiff)
            {
                float x,y;
                me->GetNearPoint2D(x, y, FR_RADIUS, m_nextdirection);
                m_timer = 30000;
                m_warning = false;
            }   
			else 
				m_timer -= uiDiff;
        }
    };
};


/*######
## mob_halion_orb
######*/
class mob_halion_orb : public CreatureScript
{
public:
    mob_halion_orb() : CreatureScript("mob_halion_orb") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_halion_orbAI(pCreature);
    }

    struct mob_halion_orbAI : public ScriptedAI
    {
        mob_halion_orbAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
			me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
			me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        }

        InstanceScript* pInstance;

        float m_direction,m_delta;
        uint32 m_flag;
        uint32 m_flag1;
        bool MovementStarted;
        Creature* focus;
        uint32 nextPoint;

        void Reset()
        {
            if (!pInstance)
                return;

            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(32754);
            me->SetPhaseMask(32, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            if (me->GetEntry() == NPC_SHADOW_PULSAR_N)
            {
                m_flag = DATA_ORB_N;
                m_delta = 0.0f;
            } 
			else if (me->GetEntry() == NPC_SHADOW_PULSAR_S)
            {
                m_flag = DATA_ORB_S;
                m_delta = M_PI;
            }
            else if (me->GetEntry() == NPC_SHADOW_PULSAR_I)// For Cata 40155, Lk 40131
            {
                m_flag = DATA_ORB_I;
                m_delta = 1.570796326795f;
            }
            else if (me->GetEntry() == NPC_SHADOW_PULSAR_W)//For Cata 40156, Lk 40132
            {
                m_flag = DATA_ORB_W;
                m_delta = M_PI + 1.570796326795f;
            }
            
            m_direction = 0.0f;
            nextPoint = 0;
            MovementStarted = false;
            pInstance->SetData(m_flag, DONE);
            sLog->outDebug(LOG_FILTER_TSCR, "EventMGR: creature %u assume m_flag %u ",me->GetEntry(),m_flag);
        }

        void EnterEvadeMode(){}

        void MovementInform(uint32 type, uint32 id)
        {
            if (!pInstance)
                return;

            if (type != POINT_MOTION_TYPE || !MovementStarted)
                return;

            if (id == nextPoint) 
			{
				me->GetMotionMaster()->MovementExpired();
				MovementStarted = false;
				pInstance->SetData(m_flag, DONE);
			}
        }

        void StartMovement(uint32 id)
        {
            if (!pInstance)
                return;

            nextPoint = id;
            float x,y;
            pInstance->SetData(m_flag, IN_PROGRESS);
            MovementStarted = true;
            m_direction = ((float)pInstance->GetData(DATA_ORB_DIRECTION)/1000 + m_delta);
            if (m_direction > 2.0f*M_PI)
                m_direction = m_direction - 2.0f*M_PI;
            if (focus = me->GetMap()->GetCreature(pInstance->GetData64(NPC_ORB_ROTATION_FOCUS)))
            {
                focus->GetNearPoint2D(x, y, FR_RADIUS, m_direction);
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePoint(id, x, y,  me->GetPositionZ());
            }
			else 
                me->DespawnOrUnsummon();
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!pInstance) 
                me->DespawnOrUnsummon();

            if (pInstance->GetData(TYPE_HALION) != IN_PROGRESS)
                me->DespawnOrUnsummon();

            if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 3.0f, true))
                DoCast(pTarget, SPELL_TWILIGHT_CUTTER);
            else if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 3.0f, true))
                DoCast(pTarget, SPELL_TWILIGHT_CUTTER);

            if (!MovementStarted && pInstance->GetData(m_flag) == SPECIAL)
                StartMovement(1);
        }
    };
};


/*######
## mob_soul_consumption
######*/
class mob_soul_consumption : public CreatureScript
{
public:
    mob_soul_consumption() : CreatureScript("mob_soul_consumption") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_soul_consumptionAI(pCreature);
    }

    struct mob_soul_consumptionAI : public ScriptedAI
    {
        mob_soul_consumptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (InstanceScript*)pCreature->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
			me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->SetReactState(REACT_PASSIVE);
        }
        
        InstanceScript* m_pInstance;

        void Reset()
        {
            if (!IsHeroic())
                me->SetPhaseMask(32,true);
            else me->SetPhaseMask(65535,true);
 
            SetCombatMovement(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            DoCast(SPELL_CONSUMPTION_AURA);
        }

        void AttackStart(Unit *pWho)
        {
            return;
        }

        void EnterEvadeMode() {}
    };
};

/*######
## mob_fiery_combustion
######*/
class mob_fiery_combustion : public CreatureScript
{
public:
    mob_fiery_combustion() : CreatureScript("mob_fiery_combustion") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_fiery_combustionAI(pCreature);
    }

    struct mob_fiery_combustionAI : public ScriptedAI
    {
        mob_fiery_combustionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (InstanceScript*)pCreature->GetInstanceScript();
			me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
			me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->SetReactState(REACT_PASSIVE);      
        }

        InstanceScript* m_pInstance;

        void Reset()
        {
            if (!IsHeroic())
                me->SetPhaseMask(31, true);
            else
                me->SetPhaseMask(65535, true);

            SetCombatMovement(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            DoCast(SPELL_COMBUSTION_AURA);
        }

        void AttackStart(Unit *pWho)
        {
            return;
        }

        void EnterEvadeMode() {}
    };
};

/*######
## mob_halion_meteor
######*/
#define TARGETS_10 5
#define TARGETS_25 7
class mob_halion_meteor : public CreatureScript
{
public:
    mob_halion_meteor() : CreatureScript("mob_halion_meteor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_halion_meteorAI(pCreature);
    }

    struct mob_halion_meteorAI : public ScriptedAI
    {
        mob_halion_meteorAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            Reset();
        }

        float direction;
        uint8 stage;
        uint32 m_uiMeteorImpactTimer;
        uint32 m_uiMeteorStrikeTimer;

        void setStage(uint8 phase)
        {
            stage = phase;
        }

        uint8 getStage()
        {
            return stage;
        }

        void Reset()
        {
            me->SetDisplayId(11686);
            me->SetRespawnDelay(7*DAY);
            SetCombatMovement(false);
            m_uiMeteorImpactTimer = 0.5*IN_MILLISECONDS;
            m_uiMeteorStrikeTimer = 6*IN_MILLISECONDS;
            setStage(0);
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            switch (getStage())
            {
                case 0:
                    if (m_uiMeteorImpactTimer <= uiDiff)
                    {
                        DoCast(74637);
                        DoCast(SPELL_METEOR_IMPACT);
                        m_uiMeteorImpactTimer = 0.5*IN_MILLISECONDS;
                        setStage(1);
                    } else m_uiMeteorImpactTimer -= uiDiff;
                    break;
                case 1:
                    if (m_uiMeteorStrikeTimer <= uiDiff)
                    {
                        DoCast(SPELL_METEOR_STRIKE);
                        m_uiMeteorStrikeTimer = 6*IN_MILLISECONDS;
                        setStage(2);
                    } else m_uiMeteorStrikeTimer -= uiDiff;
                    break;
                case 2:
                    // Place summon flames there
                    {
                        direction = 2.0f*M_PI*((float)urand(0,15)/16.0f);
                        float x, y, radius;
                        radius = 0.0f;
                        for(uint8 i = 0; i < RAID_MODE(TARGETS_10,TARGETS_25,TARGETS_10,TARGETS_25); ++i)
                        {
                            radius = radius + 5.0f;
                            me->GetNearPoint2D(x, y, radius, direction);
                            me->SummonCreature(NPC_METEOR_STRIKE_1, x, y, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 20000);
                            me->GetNearPoint2D(x, y, radius, direction+M_PI);
                            me->SummonCreature(NPC_METEOR_STRIKE_1, x, y, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 20000);
                        }
                    };
                    {
                        direction = direction + M_PI/4;
                        float x, y, radius;
                        radius = 0.0f;
                        for(uint8 i = 0; i < RAID_MODE(TARGETS_10,TARGETS_25,TARGETS_10,TARGETS_25); ++i)
                        {
                            radius = radius + 5.0f;
                            me->GetNearPoint2D(x, y, radius, direction);
                            me->SummonCreature(NPC_METEOR_STRIKE_1, x, y, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 20000);
                            me->GetNearPoint2D(x, y, radius, direction+M_PI);
                            me->SummonCreature(NPC_METEOR_STRIKE_1, x, y, me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 20000);
                        }
                    };
                    setStage(3);
                    break;
                case 3:
                    if (m_uiMeteorImpactTimer <= uiDiff)
                    {
                        DoCast(SPELL_METEOR_IMPACT);
                        me->DespawnOrUnsummon();
                        m_uiMeteorImpactTimer = 0.5*IN_MILLISECONDS;
                    } else m_uiMeteorImpactTimer -= uiDiff;
                    break;
                default:
                     break;
            }
        }
    };
};


/*######
## mob_halion_flame
######*/
class mob_halion_flame : public CreatureScript
{
public:
    mob_halion_flame() : CreatureScript("mob_halion_flame") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_halion_flameAI(pCreature);
    }

    struct mob_halion_flameAI : public ScriptedAI
    {
        mob_halion_flameAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            Reset();
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetDisplayId(11686);
            me->SetRespawnDelay(7*DAY);
            SetCombatMovement(false);
            me->SetInCombatWithZone();
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!me->HasAura(SPELL_METEOR_FLAME))
                DoCast(SPELL_METEOR_FLAME);
        }

    };
};


class go_halion_portal_twilight : public GameObjectScript
{
    public:

        go_halion_portal_twilight() : GameObjectScript("go_halion_portal_twilight") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            InstanceScript* pInstance = (InstanceScript*)go->GetInstanceScript();
            if (!pInstance)
                return false;

            player->CastSpell(player,SPELL_TWILIGHT_ENTER,false);
            return true;
        }
};

class go_halion_portal_real : public GameObjectScript
{
    public:

        go_halion_portal_real() : GameObjectScript("go_halion_portal_real") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            InstanceScript* pInstance = (InstanceScript*)go->GetInstanceScript();
            if (!pInstance) 
                return false;

            player->RemoveAurasDueToSpell(SPELL_TWILIGHT_ENTER);
            return true;
        }
};


/*######
## spell_halion_fiery_combustion 74562
## DELETE FROM `spell_script_names` WHERE `spell_id`=74562 AND `ScriptName`='spell_halion_fiery_combustion';
## INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (74562,'spell_halion_fiery_combustion');
######*/
class spell_halion_fiery_combustion : public SpellScriptLoader
{
    public:
        spell_halion_fiery_combustion() : SpellScriptLoader("spell_halion_fiery_combustion") { }

        class spell_halion_fiery_combustion_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_halion_fiery_combustion_AuraScript)
            enum Spells
            {
                SPELL_MARK_OF_COMBUSTION = 74567,
                SPELL_COMBUSTION_EXPLODE = 74607
            };

            bool Validate(SpellEntry const* /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_MARK_OF_COMBUSTION))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_COMBUSTION_EXPLODE))
                    return false;
                return true;
            }

            void HandlePeriodicTick(AuraEffect const * /*aurEff*/)
            {
                if (Unit* pTarget = GetTarget())
                    pTarget->AddAura(SPELL_MARK_OF_COMBUSTION, pTarget);
            }

            void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* pTarget = GetTarget())
                {
                    if (Aura *mark = pTarget->GetAura(SPELL_MARK_OF_COMBUSTION))
                    {
                        int32 bp = 2000 * mark->GetStackAmount();
                        if (GetCaster())
							if (Creature * combust = GetCaster()->SummonCreature(40001, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), pTarget->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 60000))
                            {
                                combust->SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f * mark->GetStackAmount());
                                if (pTarget->GetMap()->IsHeroic())
                                    combust->CastCustomSpell(pTarget, SPELL_COMBUSTION_EXPLODE, &bp, 0, 0, true);
                            }
                        pTarget->RemoveAura(SPELL_MARK_OF_COMBUSTION, pTarget->GetGUID());
                    }
                    
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_halion_fiery_combustion_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectRemove += AuraEffectRemoveFn(spell_halion_fiery_combustion_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_halion_fiery_combustion_AuraScript();
        }
};

class ExactDistanceCheck
{
    public:
        ExactDistanceCheck(WorldObject* source, float dist) : _source(source), _dist(dist) {}

        bool operator()(WorldObject* unit)
        {
            return _source->GetExactDist2d(unit) > _dist;
        }

    private:
        WorldObject* _source;
        float _dist;
};

class spell_combustion : public SpellScriptLoader
{
    public:
        spell_combustion() : SpellScriptLoader("spell_combustion") { }

        class spell_combustion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_combustion_SpellScript);

            void ScaleRange(std::list<WorldObject*>& targets)
            {
                targets.remove_if(ExactDistanceCheck(GetCaster(), 7.0f * GetCaster()->GetFloatValue(OBJECT_FIELD_SCALE_X)));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_combustion_SpellScript::ScaleRange, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);//TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_combustion_SpellScript::ScaleRange, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);//TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_combustion_SpellScript();
        }
};

class spell_consumption : public SpellScriptLoader
{
    public:
        spell_consumption() : SpellScriptLoader("spell_consumption") { }

        class spell_consumption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_consumption_SpellScript);

            void ScaleRange(std::list<WorldObject*>& targets)
            {
                targets.remove_if(ExactDistanceCheck(GetCaster(), 7.0f * GetCaster()->GetFloatValue(OBJECT_FIELD_SCALE_X)));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_consumption_SpellScript::ScaleRange, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);//TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_consumption_SpellScript::ScaleRange, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);//TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_consumption_SpellScript();
        }
};

/*######
## spell_halion_soul_consumption 74792
## DELETE FROM `spell_script_names` WHERE `spell_id`=74792 AND `ScriptName`='spell_halion_soul_consumption';
## INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (74792,'spell_halion_soul_consumption');
######*/
class spell_halion_soul_consumption : public SpellScriptLoader
{
    public:
        spell_halion_soul_consumption() : SpellScriptLoader("spell_halion_soul_consumption") { }

        class spell_halion_soul_consumption_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_halion_soul_consumption_AuraScript)
            enum Spells
            {
                SPELL_MARK_OF_CONSUMPTION = 74795,
                SPELL_CONSUMPTION_EXPLODE = 74799
            };

            bool Validate(SpellEntry const* /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_MARK_OF_CONSUMPTION))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_CONSUMPTION_EXPLODE))
                    return false;
                return true;
            }

            void HandlePeriodicTick(AuraEffect const * /*aurEff*/)
            {
                if (Unit* pTarget = GetTarget())
                    pTarget->AddAura(SPELL_MARK_OF_CONSUMPTION, pTarget);
            }

            void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* pTarget = GetTarget())
                {
                    if (Aura *mark = pTarget->GetAura(SPELL_MARK_OF_CONSUMPTION))
                    {
                        int32 bp = 2000 * mark->GetStackAmount();
                        if (GetCaster())
                            if (Creature * comsumpt = GetCaster()->SummonCreature(40135, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), pTarget->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 60000))
                            {
                                comsumpt->SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f * mark->GetStackAmount());
                                if (pTarget->GetMap()->IsHeroic())
                                    comsumpt->CastCustomSpell(pTarget, SPELL_CONSUMPTION_EXPLODE, &bp, 0, 0, true);
                            }
                        pTarget->RemoveAura(SPELL_MARK_OF_CONSUMPTION, pTarget->GetGUID());
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_halion_soul_consumption_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectRemove += AuraEffectRemoveFn(spell_halion_soul_consumption_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_halion_soul_consumption_AuraScript();
        }
};

void AddSC_boss_halion()
{
    new boss_halion_real();
    new boss_halion_twilight();
    new mob_halion_meteor();
    new mob_halion_flame();
    new mob_halion_orb();
    new mob_halion_control();
    new mob_orb_rotation_focus();
    new mob_soul_consumption();
    new mob_fiery_combustion();
    new go_halion_portal_twilight();
    new go_halion_portal_real();
    new spell_combustion();
    new spell_consumption();
    new spell_halion_fiery_combustion();
    new spell_halion_soul_consumption();
}