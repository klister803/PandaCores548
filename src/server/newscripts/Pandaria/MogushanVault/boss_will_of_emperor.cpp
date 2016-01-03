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
#include "mogu_shan_vault.h"

enum eSpells
{
    // Jan Xi && Qin Xi
    SPELL_STOMP                 = 116969,
    SPELL_DEVASTATING_ARC       = 117006,
    SPELL_MAGNETIC_ARMOR        = 116815,
    // Woi controller
    SPELL_TITAN_GAS             = 116779,
    // On players if evade death attack from boss
    SPELL_OPPORTUNISTIC_STRIKE  = 116808,
    // Rage
    SPELL_FOCALISED_ASSAULT     = 116525,
    SPELL_WITHOUT_ARMOR         = 116535,
    // Courage
    SPELL_FOCALISED_DEFENSE     = 116778,
    SPELL_IMPEDING_THRUST       = 117485,
    SPELL_HALF_PLATE            = 116537,
    // Force
    SPELL_ENERGIZING_SMASH      = 116550,
    SPELL_FULL_PLATE            = 116540,
    // Titan Spark
    SPELL_SUMMON_TITAN_SPARK    = 117746,
    SPELL_FOCALISED_ENERGY      = 116829,
    SPELL_ENERGY_OF_CREATION    = 116805
};

enum eEvents
{
    // Imperators
    EVENT_DEATH_ATTACKS_START       = 8,
    EVENT_DEATH_ATTACKS_END         = 9,
    // All adds
    EVENT_CHECK_TARGET              = 10,
    // Courage
    EVENT_IMPEDING_THRUST           = 12,
    // Strenght
    EVENT_ENERGIZING_SMASH          = 13,
    // Woi Controller
    EVENT_CHECK_WIPE                = 14,
    EVENT_SPAWN_IMPERATOR           = 15,
    EVENT_TITAN_GAS                 = 16,
    EVENT_SPAWN_RANDOM_ADD          = 17,
    EVENT_SPAWN_RAGE                = 60396,
    EVENT_SPAWN_FORCE               = 60397,
    EVENT_SPAWN_COURAGE             = 60398,
};

enum esAction
{
    // Jan Xi && Qin Xi
    ACTION_DEATH_ATTACK         = 1,
    // Woi controller
    ACTION_DONE                 = 2,
};

Position const janxipos  = {3829.48f, 1523.41f, 362.26f, 0.6683f};
Position const qinxipos  = {3828.49f, 1576.28f, 362.26f, 5.9148f};

Position const ragepos = {3821.39f, 1550.37f, 362.26f, 0.0471f};

Position const couragepos[2] =
{
    {3894.50f, 1498.40f, 362.26f, 1.6925f},
    {3895.52f, 1602.42f, 362.26f, 4.8458f},
};

Position const forcepos[2] = 
{
    {3841.71f, 1591.97f, 362.27f, 5.5487f},
    {3841.46f, 1508.87f, 362.28f, 0.8599f},
};

uint32 imperators[2] = 
{
    NPC_QIN_XI,
    NPC_JAN_XI,  
};

class npc_woi_controller : public CreatureScript
{
    public:
        npc_woi_controller() : CreatureScript("npc_woi_controller") {}

        struct npc_woi_controllerAI : public BossAI
        {
            npc_woi_controllerAI(Creature* creature) : BossAI(creature, DATA_WILL_OF_EMPEROR)
            {
                pInstance = creature->GetInstanceScript();
                me->SetDisplayId(11686);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
            }

            InstanceScript* pInstance;
            uint32 addentry[3]; //Array for summons entry

            void Reset()
            {
                if (pInstance)
                {
                    _Reset();
                    me->RemoveAurasDueToSpell(SPELL_TITAN_GAS);
                    RemovePursuitAuraOnPlayers();
                    for (uint8 n = 0; n < 3 ; n++)
                        addentry[n] = 0;
                }
            }

            void RemovePursuitAuraOnPlayers()
            {
               pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FOCALISED_ASSAULT);
               pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FOCALISED_DEFENSE);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                PushAddArray();
            }

            void PushAddArray()
            {
                uint8 pos = urand(0, 3);
                switch (pos)
                {
                case 0:
                    addentry[0] = NPC_RAGE;
                    addentry[1] = NPC_FORCE;                     
                    addentry[2] = NPC_COURAGE;
                    break;
                case 1:
                    addentry[0] = NPC_FORCE;
                    addentry[1] = NPC_RAGE;                    
                    addentry[2] = NPC_COURAGE;
                    break;
                case 2:
                    addentry[0] = NPC_COURAGE;
                    addentry[1] = NPC_FORCE;                    
                    addentry[2] = NPC_RAGE; 
                    break;
                case 3:
                    addentry[0] = NPC_RAGE;
                    addentry[1] = NPC_COURAGE;                      
                    addentry[2] = NPC_FORCE;
                    break;
                }
                StartEvent();
            }

            void StartEvent()
            {
                events.ScheduleEvent(EVENT_CHECK_WIPE,       1500);
                events.ScheduleEvent(addentry[0],            1000);
                events.ScheduleEvent(addentry[1],            30000);
                events.ScheduleEvent(addentry[2],            60000);
                events.ScheduleEvent(EVENT_SPAWN_IMPERATOR,  90000);
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                case ACTION_DONE:
                    if (pInstance)
                    {
                        me->RemoveAurasDueToSpell(SPELL_TITAN_GAS);
                        RemovePursuitAuraOnPlayers();
                        pInstance->SetBossState(DATA_WILL_OF_EMPEROR, DONE);
                        me->DespawnOrUnsummon(1000);
                    }
                    break;
                default:
                    break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CHECK_WIPE:
                        if (pInstance->IsWipe())
                            EnterEvadeMode();
                        events.ScheduleEvent(EVENT_CHECK_WIPE, 1500);
                        break;
                    case EVENT_SPAWN_RAGE:
                        me->SummonCreature(NPC_RAGE, ragepos, TEMPSUMMON_CORPSE_DESPAWN);
                        break;
                    case EVENT_SPAWN_FORCE:
                        me->SummonCreature(NPC_FORCE, forcepos[rand()%2], TEMPSUMMON_CORPSE_DESPAWN);
                        break;
                    case EVENT_SPAWN_COURAGE:
                        me->SummonCreature(NPC_COURAGE, couragepos[rand()%2], TEMPSUMMON_CORPSE_DESPAWN);
                        break;
                    case EVENT_SPAWN_RANDOM_ADD:
                        {
                            uint8 pos = urand(0, 2);
                            switch (pos)
                            {
                            case 0:
                                events.ScheduleEvent(EVENT_SPAWN_RAGE, 500);
                                break;
                            case 1:
                                events.ScheduleEvent(EVENT_SPAWN_FORCE, 500);
                                break;
                            case 2:
                                events.ScheduleEvent(EVENT_SPAWN_COURAGE, 500);
                                break;
                            }
                            events.ScheduleEvent(EVENT_SPAWN_RANDOM_ADD, 30000);
                            break;
                        }
                    case EVENT_SPAWN_IMPERATOR:
                        me->SummonCreature(NPC_QIN_XI, qinxipos);
                        me->SummonCreature(NPC_JAN_XI, janxipos);
                        events.ScheduleEvent(EVENT_SPAWN_RANDOM_ADD, 500);
                        events.ScheduleEvent(EVENT_TITAN_GAS,     120000);
                        break;
                    case EVENT_TITAN_GAS:
                        me->AddAura(SPELL_TITAN_GAS, me);
                        events.ScheduleEvent(EVENT_TITAN_GAS, 120000);
                        break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_woi_controllerAI(creature);
        }
};

void SendDied(InstanceScript* pInstance, Creature* caller, uint32 callerentry)
{
    if (caller && pInstance)
    {
        for (uint8 n = 0; n < 2; n++)
        {
            if (Creature* imperator = caller->GetCreature(*caller, pInstance->GetData64(imperators[n])))
            {
                if (imperator->isAlive() && imperator->GetEntry() != callerentry)
                {
                    imperator->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    imperator->Kill(imperator, true);
                    imperator->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                }
            }
        }
        if (Creature* controller = caller->GetCreature(*caller, pInstance->GetData64(NPC_WOI_CONTROLLER)))
            controller->AI()->DoAction(ACTION_DONE);
    }
}

void SendDamage(InstanceScript* pInstance, Creature* caller, uint32 callerentry, uint32 damage)
{
    if (caller && pInstance)
    {
        for (uint8 n = 0; n < 2; n++)
        {
            if (Creature* imperator = caller->GetCreature(*caller, pInstance->GetData64(imperators[n])))
            {
                if (imperator->isAlive() && imperator->GetEntry() != callerentry)
                    imperator->SetHealth(imperator->GetHealth() - damage);
            }
        }
    }
}

class boss_generic_imperator : public CreatureScript
{
    public:
        boss_generic_imperator() : CreatureScript("boss_generic_imperator") {}

        struct boss_generic_imperatorAI : public ScriptedAI
        {
            boss_generic_imperatorAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            uint32 attackspell[2];

            void Reset()
            {
                me->LowerPlayerDamageReq(me->GetMaxHealth());
                me->setPowerType(POWER_ENERGY);
                me->SetPower(POWER_ENERGY, 0);
                DoZoneInCombat(me, 150.0f);
            }

            void RegeneratePower(Powers power, float &value)
            {
                if (!me->isInCombat())
                {
                    value = 0;
                    return;
                }

                if (me->GetPower(POWER_ENERGY) == 98)
                    DoAction(ACTION_DEATH_ATTACK);
                value = 2;
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                case ACTION_DEATH_ATTACK:
                    for (uint8 n = 0; n < 2; n++)
                        attackspell[n] = 0;

                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();

                    uint8 pos = urand(0, 1);
                    switch (pos)
                    {
                    case 0:
                        attackspell[0] = SPELL_STOMP;
                        attackspell[1] = SPELL_DEVASTATING_ARC;
                        break;
                    case 1:
                        attackspell[0] = SPELL_DEVASTATING_ARC;
                        attackspell[1] = SPELL_STOMP;
                        break;
                    }
                    DoCast(me, attackspell[0]);
                    events.ScheduleEvent(EVENT_DEATH_ATTACKS_START, 3500); 
                    break;
                }
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (damage >= me->GetHealth())
                    SendDied(pInstance, me, me->GetEntry());
                else
                    SendDamage(pInstance, me, me->GetEntry(), damage);
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_DEATH_ATTACKS_START:
                        DoCast(me, attackspell[1]);
                        events.ScheduleEvent(EVENT_DEATH_ATTACKS_END, 3500);
                        break;
                    case EVENT_DEATH_ATTACKS_END:
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f);
                        if (me->getVictim())
                            me->GetMotionMaster()->MoveChase(me->getVictim());
                        me->SetPower(POWER_ENERGY, 0);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_generic_imperatorAI(creature);
        }
};

class mob_woi_add_generic : public CreatureScript
{
    public:
        mob_woi_add_generic() : CreatureScript("mob_woi_add_generic") {}

        struct mob_woi_add_genericAI : public ScriptedAI
        {
            mob_woi_add_genericAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;
            uint32 focusspell;
            uint64 targetguid;

            void Reset()
            {
                targetguid = 0;
                focusspell = 0;
                DoZoneInCombat(me, 150.0f);
                switch (me->GetEntry())
                {
                    case NPC_RAGE:
                        focusspell = SPELL_FOCALISED_ASSAULT;
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                        {
                            target->AddAura(focusspell, target);
                            targetguid = target->GetGUID();
                            AttackStart(target);
                            me->getThreatManager().addThreat(target, 1000000.0f);
                        }
                        me->AddAura(SPELL_WITHOUT_ARMOR, me);
                        break;
                    case NPC_COURAGE:
                        focusspell = SPELL_FOCALISED_DEFENSE;
                        if (Unit* randomBoss = pInstance->instance->GetCreature(pInstance->GetData64(urand(0, 1) ? NPC_QIN_XI: NPC_JAN_XI)))
                        {
                            if (Unit* tank = randomBoss->getVictim())
                            {
                                tank->AddAura(focusspell, tank);
                                targetguid = tank->GetGUID();
                                AttackStart(tank);
                                me->getThreatManager().addThreat(tank, 1000000.0f);
                            }
                        }
                        else if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                        {
                            target->AddAura(focusspell, target);
                            targetguid = target->GetGUID();
                            AttackStart(target);
                            me->getThreatManager().addThreat(target, 1000000.0f);                           
                        }
                        me->AddAura(SPELL_HALF_PLATE, me);
                        events.ScheduleEvent(EVENT_IMPEDING_THRUST, 5000);
                        break;
                    case NPC_FORCE:
                        me->AddAura(SPELL_FULL_PLATE, me);
                        events.ScheduleEvent(EVENT_ENERGIZING_SMASH, urand(5000, 7000));
                        break;
                }
                events.ScheduleEvent(EVENT_CHECK_TARGET, 1500);
            }
            
            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (me->GetEntry() == NPC_COURAGE)
                    if (me->isInFront(attacker))
                        damage = 0;

                if (damage >= me->GetHealth())
                {
                   if (me->GetEntry() == NPC_RAGE || me->GetEntry() == NPC_COURAGE)
                   {
                       if (Unit* target = me->GetUnit(*me, targetguid))
                       {
                           if (target->isAlive())
                               target->RemoveAurasDueToSpell(focusspell);
                       }
                   }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_TARGET: //All adds, check pursuit target
                            if (!me->getVictim() || !me->getVictim()->isAlive())
                            {
                                switch (me->GetEntry())
                                {
                                case NPC_RAGE:
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                                    {
                                        target->AddAura(focusspell, target);
                                        targetguid = target->GetGUID();
                                        AttackStart(target);
                                        me->getThreatManager().addThreat(target, 1000000.0f);
                                    }
                                    break;
                                case NPC_FORCE:
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                                    {
                                        AttackStart(target);
                                        me->getThreatManager().addThreat(target, 1000000.0f);
                                    }
                                    break;
                                case NPC_COURAGE:
                                    if (Unit* randomBoss = pInstance->instance->GetCreature(pInstance->GetData64(urand(0, 1) ? NPC_QIN_XI: NPC_JAN_XI)))
                                    {
                                        if (Unit* tank = randomBoss->getVictim())
                                        {
                                            tank->AddAura(focusspell, tank);
                                            targetguid = tank->GetGUID();
                                            AttackStart(tank);
                                            me->getThreatManager().addThreat(tank, 1000000.0f);
                                        }
                                    }
                                    else if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                                    {
                                        target->AddAura(focusspell, target);
                                        targetguid = target->GetGUID();
                                        AttackStart(target);
                                        me->getThreatManager().addThreat(target, 1000000.0f);
                                    }
                                    events.ScheduleEvent(EVENT_IMPEDING_THRUST, 2000);
                                    break;
                                }
                            }
                            events.ScheduleEvent(EVENT_CHECK_TARGET, 2000);
                            break;
                        case EVENT_IMPEDING_THRUST: // Courage
                            if (me->getVictim())
                            {
                                if (me->IsWithinMeleeRange(me->getVictim()))
                                {
                                    DoCast(me->getVictim(), SPELL_IMPEDING_THRUST);
                                    events.ScheduleEvent(EVENT_IMPEDING_THRUST, 10000);
                                }
                                else
                                    events.ScheduleEvent(EVENT_IMPEDING_THRUST, 3000);
                            }
                            break;
                        case EVENT_ENERGIZING_SMASH: // Strenght
                            DoCast(me, SPELL_ENERGIZING_SMASH);
                            events.ScheduleEvent(EVENT_ENERGIZING_SMASH, urand(5000, 7000));
                            break;
                        
                    }
                }
                if (me->GetEntry() != NPC_FORCE)
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_woi_add_genericAI(creature);
        }
};

void AddSC_boss_will_of_emperor()
{
    new npc_woi_controller();
    new boss_generic_imperator();
    new mob_woi_add_generic();
}
