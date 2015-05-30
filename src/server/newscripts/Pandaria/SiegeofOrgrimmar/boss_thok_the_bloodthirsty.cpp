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
    //Phase 1
    SPELL_FEARSOME_ROAR      = 143426, 
    SPELL_DEAFENING_SCREECH  = 143343,
    SPELL_TAIL_LASH          = 143428, 
    SPELL_SHOCK_BLAST        = 143707,
    SPELL_BLOODIED           = 143452,
    SPELL_POWER_REGEN        = 143345,
    SPELL_ACCELERATION       = 143411,
    //
    SPELL_ACID_BREATH        = 143780,
    SPELL_CORROSIVE_BLOOD    = 143791,
    //
    SPELL_FREEZING_BREATH    = 143773,
    SPELL_ICY_BLOOD          = 143800,
    SPELL_SUMMON_ICE_TOMB    = 136929,
    SPELL_FROZEN_SOLID       = 143777,
    //
    SPELL_SCORCHING_BREATH   = 143767,
    SPELL_BURNING_BLOOD      = 143783,
    SPELL_BURNING_BLOOD_ADMG = 143784,

    //Phase 2
    SPELL_BLOOD_FRENZY       = 143440,
    SPELL_BLOOD_FRENZY_TE    = 143442,
    SPELL_BLOOD_FRENZY_KB    = 144067, 
    SPELL_FIXATE_PL          = 143445,
    SPELL_FIXATE_IM          = 146540,

    SPELL_ENRAGE_KJ          = 145974,
    SPELL_UNLOCKING          = 146589, 

    SPELL_ENRAGE             = 145974,

    SPELL_VAMPIRIC_FRENZY    = 147978,

    SPELL_CANNON_BALL        = 147906,
    SPELL_CANNON_BALL_ATDMG  = 147607,
    SPELL_CANNON_BALL_AT_A   = 147609,
    SPELL_CANNON_BALL_DESTD  = 147662,
};

enum Events
{
    //Default events
    EVENT_FEARSOME_ROAR      = 1,
    EVENT_TAIL_LASH          = 2,
    EVENT_SHOCK_BLAST        = 3,
    //Extra event
    EVENT_ACID_BREATH        = 4,
    EVENT_CORROSIVE_BLOOD    = 5,
    //
    EVENT_FREEZING_BREATH    = 6,
    //
    EVENT_SCORCHING_BREATH   = 7,
    EVENT_BURNING_BLOOD      = 8,

    //Summon events
    EVENT_ENRAGE_KJ          = 9,
    EVENT_MOVE_TO_CENTER     = 10,
    EVENT_MOVE_TO_THOK       = 11,
    EVENT_CHECK_TPLAYER      = 12,
    EVENT_Y_CHARGE           = 13,
    EVENT_PRE_Y_CHARGE       = 14,
    EVENT_CHECKER            = 15,
    EVENT_VAMPIRIC_FRENZY    = 16,
};

enum Action
{
    ACTION_PHASE_TWO         = 1,
    ACTION_PHASE_ONE_ACID    = 2, 
    ACTION_PHASE_ONE_FROST   = 3, 
    ACTION_PHASE_ONE_FIRE    = 4, 
    ACTION_FIXATE            = 5,

    ACTION_FREEDOM           = 6,
};

uint32 prisonersentry[3] =
{
    NPC_AKOLIK, 
    NPC_MONTAK, 
    NPC_WATERSPEAKER_GORAI, 
};

Position fpos[3] =
{
    {1293.5050f, -5127.1513f, -287.6911f, 2.9432f},
    {1116.9144f, -5096.3139f, -287.6315f, 6.1209f},
    {1223.3216f, -5026.2880f, -287.7276f, 4.5030f},
};

Position kjspawnpos = {1173.41f, -5130.74f, -289.9429f, 0.6028f};
Position cpos = {1208.61f, -5106.27f, -289.939f, 0.526631f};

Position ccbatspawnpos[7] =
{
    {1257.04f, -5169.60f, -280.0894f, 2.2238f},
    {1251.81f, -5162.37f, -280.0894f, 2.2238f},
    {1251.15f, -5174.29f, -280.0894f, 2.2238f},
    {1245.83f, -5166.56f, -280.0894f, 2.2238f},
    {1238.66f, -5181.14f, -280.0894f, 2.2238f},
    {1233.69f, -5172.25f, -280.0894f, 2.2238f},
    {1244.47f, -5158.36f, -280.0894f, 2.2238f},
};

Position sumyetipos[4] = 
{
    {1217.50f, -5041.30f, -290.4328f, 4.4818f},
    {1272.52f, -5124.51f, -290.4575f, 2.8089f},
    {1218.18f, -5181.76f, -290.4609f, 1.7683f},
    {1135.86f, -5098.79f, -290.4617f, 6.0448f},
};

enum CreatureText
{
    SAY_PULL
};

class boss_thok_the_bloodthirsty : public CreatureScript
{
    public:
        boss_thok_the_bloodthirsty() : CreatureScript("boss_thok_the_bloodthirsty") {}

        struct boss_thok_the_bloodthirstyAI : public BossAI
        {
            boss_thok_the_bloodthirstyAI(Creature* creature) : BossAI(creature, DATA_THOK)
            {
                instance = creature->GetInstanceScript();
            }
            
            InstanceScript* instance;
            std::list<Player*> plist;
            uint32 meleecheck, enrage;
            uint8 phasecount;

            void Reset()
            {
                _Reset();
                plist.clear();
                DespawnObjects();
                me->SetReactState(REACT_DEFENSIVE);
                me->RemoveAurasDueToSpell(SPELL_POWER_REGEN);
                me->RemoveAurasDueToSpell(SPELL_ACCELERATION);
                me->RemoveAurasDueToSpell(SPELL_FIXATE_PL);
                me->RemoveAurasDueToSpell(SPELL_BLOOD_FRENZY);
                me->RemoveAurasDueToSpell(SPELL_BLOOD_FRENZY_TE);
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 0);
                phasecount = 0;
                meleecheck = 0;
                enrage = 0;
                if (instance)
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FIXATE_PL);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOODIED);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNLOCKING);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BURNING_BLOOD_ADMG);
                }
            }

            void DespawnObjects()
            {
                std::list<AreaTrigger*> atlist;
                atlist.clear();
                me->GetAreaTriggersWithEntryInRange(atlist, 4890, me->GetGUID(), 200.0f);
                if (!atlist.empty())
                {
                    for (std::list<AreaTrigger*>::const_iterator itr = atlist.begin(); itr != atlist.end(); itr++)
                        (*itr)->RemoveFromWorld();
                }
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                enrage = 600000;
                DoCast(me, SPELL_POWER_REGEN, true);
                events.ScheduleEvent(EVENT_SHOCK_BLAST, 1000);
                events.ScheduleEvent(EVENT_TAIL_LASH, 12000);
                events.ScheduleEvent(EVENT_FEARSOME_ROAR, 15000);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_PHASE_ONE_ACID: 
                case ACTION_PHASE_ONE_FROST: 
                case ACTION_PHASE_ONE_FIRE:
                    events.Reset();
                    meleecheck = 0;
                    me->StopMoving();
                    me->getThreatManager().resetAllAggro();
                    me->RemoveAurasDueToSpell(SPELL_BLOOD_FRENZY);
                    me->RemoveAurasDueToSpell(SPELL_BLOOD_FRENZY_TE);
                    me->InterruptNonMeleeSpells(true);
                    me->RemoveAurasDueToSpell(SPELL_FIXATE_PL);
                    if (instance)
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FIXATE_PL);
                    DoCast(me, SPELL_POWER_REGEN, true);
                    DoZoneInCombat(me, 200.0f);
                    events.ScheduleEvent(EVENT_TAIL_LASH, 12000);
                    switch (action)
                    {
                    case ACTION_PHASE_ONE_ACID: 
                        events.ScheduleEvent(EVENT_ACID_BREATH, 15000);
                        events.ScheduleEvent(EVENT_CORROSIVE_BLOOD, 3500);
                        break;
                    case ACTION_PHASE_ONE_FROST: 
                        events.ScheduleEvent(EVENT_FREEZING_BREATH, 15000);
                        break;
                    case ACTION_PHASE_ONE_FIRE:
                        events.ScheduleEvent(EVENT_SCORCHING_BREATH, 15000);
                        events.ScheduleEvent(EVENT_BURNING_BLOOD, urand(3000, 4000));
                        break;
                    }
                    if (me->GetMap()->IsHeroic())
                    {
                        phasecount++;
                        me->SetHealth(me->GetHealth() + me->CountPctFromMaxHealth(8));
                        switch (phasecount)
                        {
                        case 1:
                            for (uint8 n = 0; n < 7; n++)
                                if (Creature* bat = me->SummonCreature(NPC_CAPTIVE_CAVE_BAT, ccbatspawnpos[n]))
                                    bat->AI()->DoZoneInCombat(bat, 200.0f);
                            break;
                        case 2:
                            me->SummonCreature(NPC_STARVED_YETI, sumyetipos[urand(0, 3)]);
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case ACTION_PHASE_TWO:
                    events.Reset();
                    events.ScheduleEvent(EVENT_SHOCK_BLAST, urand(2000, 3000));
                    if (instance)
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOODIED);
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    me->getThreatManager().resetAllAggro();
                    me->RemoveAurasDueToSpell(SPELL_POWER_REGEN);
                    me->RemoveAurasDueToSpell(SPELL_ACCELERATION);
                    me->SetPower(POWER_ENERGY, 0);
                    DoCast(me, SPELL_BLOOD_FRENZY_KB, true);
                    DoCast(me, SPELL_BLOOD_FRENZY, true);
                    if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 150.0f, true))
                        DoCast(target, SPELL_FIXATE_PL);
                    if (Creature* kj = me->SummonCreature(NPC_KORKRON_JAILER, kjspawnpos))
                        kj->AI()->DoZoneInCombat(kj, 300.0f);
                    meleecheck = 4000;
                    break;
                case ACTION_FIXATE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 150.0f, true))
                        DoCast(target, SPELL_FIXATE_PL);
                    break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (enrage)
                {
                    if (enrage <= diff)
                    {
                        DoCast(me, SPELL_ENRAGE, true);
                        enrage = 0;
                    }
                    else
                        enrage -= diff;
                }

                if (meleecheck)
                {
                    if (meleecheck <= diff)
                    {
                        plist.clear();
                        GetPlayerListInGrid(plist, me, 5.0f);
                        if (!plist.empty())
                        {
                            for (std::list<Player*>::const_iterator itr = plist.begin(); itr != plist.end(); itr++)
                                if ((*itr)->isInFront(me))
                                    (*itr)->Kill(*itr, true);
                        }
                        meleecheck = 1000;
                    }
                    else
                        meleecheck -= diff;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    //Default events
                    case EVENT_SHOCK_BLAST:
                        DoCastAOE(SPELL_SHOCK_BLAST, true);
                        events.ScheduleEvent(EVENT_SHOCK_BLAST, urand(3000, 4000));
                        break;
                    case EVENT_TAIL_LASH:
                        DoCast(me, SPELL_TAIL_LASH, true);
                        events.ScheduleEvent(EVENT_TAIL_LASH, 12000);
                        break;
                    case EVENT_FEARSOME_ROAR:
                        DoCast(me, SPELL_FEARSOME_ROAR, true);
                        events.ScheduleEvent(EVENT_FEARSOME_ROAR, 15000);
                        break;
                    //Extra events
                    //
                    case EVENT_ACID_BREATH:
                        DoCast(me, SPELL_ACID_BREATH, true);
                        events.ScheduleEvent(EVENT_ACID_BREATH, 15000);
                        break;
                    case EVENT_CORROSIVE_BLOOD:
                        DoCastAOE(SPELL_CORROSIVE_BLOOD, true);
                        events.ScheduleEvent(EVENT_CORROSIVE_BLOOD, 3500);
                        break;
                    //
                    case EVENT_FREEZING_BREATH:
                        DoCast(me, SPELL_FREEZING_BREATH, true);
                        events.ScheduleEvent(EVENT_FREEZING_BREATH, 15000);
                        break;
                    //
                    case EVENT_SCORCHING_BREATH:
                        DoCast(me, SPELL_SCORCHING_BREATH, true);
                        events.ScheduleEvent(EVENT_SCORCHING_BREATH, 15000);
                        break;
                    case EVENT_BURNING_BLOOD:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 150.0f, true))
                            DoCast(target, SPELL_BURNING_BLOOD);
                        events.ScheduleEvent(EVENT_BURNING_BLOOD, urand(3000, 4000));
                        break;               
                    }
                }
                if (!meleecheck)
                    DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                DespawnObjects();
                if (instance)
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FIXATE_PL);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOODIED);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNLOCKING);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BURNING_BLOOD_ADMG);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_thok_the_bloodthirstyAI(creature);
        }
};

//71658
class npc_korkron_jailer : public CreatureScript
{
public:
    npc_korkron_jailer() : CreatureScript("npc_korkron_jailer") {}

    struct npc_korkron_jailerAI : public ScriptedAI
    {
        npc_korkron_jailerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            done = false;
            click = true;
        }

        InstanceScript* instance;
        EventMap events;
        bool done, click;

        void Reset(){}
        
        void EnterCombat(Unit* who)
        {
            Talk(SAY_PULL);
            events.ScheduleEvent(EVENT_ENRAGE_KJ, 1000);
        }

        void OnSpellClick(Unit* clicker)
        {
            if (click && done)
            {
                click = false;
                clicker->CastSpell(clicker, SPELL_UNLOCKING);
                me->DespawnOrUnsummon(1000);
            }
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth() && !done)
            {
                damage = 0;
                done = true;
                events.Reset();
                me->StopMoving();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);
                me->SetStandState(UNIT_STAND_STATE_DEAD);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_ENRAGE_KJ)
                {
                    DoCast(me, SPELL_ENRAGE_KJ, true);
                    events.ScheduleEvent(EVENT_ENRAGE_KJ, urand(15000, 20000));
                }
            }
            if (!done)
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_korkron_jailerAI(creature);
    }
};

//71742, 71763, 71749
class npc_generic_prisoner : public CreatureScript
{
public:
    npc_generic_prisoner() : CreatureScript("npc_generic_prisoner") {}

    struct npc_generic_prisonerAI : public ScriptedAI
    {
        npc_generic_prisonerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset(){}

        void DoAction(int32 const action)
        {
            if (action == ACTION_FREEDOM)
                events.ScheduleEvent(EVENT_MOVE_TO_CENTER, 2000);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE && instance)
            {
                switch (pointId)
                {
                case 0:
                    events.ScheduleEvent(EVENT_MOVE_TO_THOK, 500);
                    break;
                case 1:
                    if (Creature* thok = me->GetCreature(*me, instance->GetData64(NPC_THOK)))
                    {
                        if (thok->isAlive())
                        {
                            me->Kill(me, true);
                            switch (me->GetEntry())
                            {
                            case NPC_AKOLIK:
                                thok->AI()->DoAction(ACTION_PHASE_ONE_ACID);
                                break;
                            case NPC_MONTAK:
                                thok->AI()->DoAction(ACTION_PHASE_ONE_FIRE);
                                break;
                            case NPC_WATERSPEAKER_GORAI:
                                thok->AI()->DoAction(ACTION_PHASE_ONE_FROST);
                                break;
                            }
                        }
                    }
                    break;
                }
            }               
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_MOVE_TO_CENTER:
                    me->GetMotionMaster()->MoveCharge(cpos.GetPositionX(), cpos.GetPositionY(), cpos.GetPositionZ(), 20.0f, 0);
                    break;
                case EVENT_MOVE_TO_THOK:
                    if (Creature* thok = me->GetCreature(*me, instance->GetData64(NPC_THOK)))
                        if (thok->isAlive())
                            me->GetMotionMaster()->MoveCharge(thok->GetPositionX(), thok->GetPositionY(), thok->GetPositionZ(), 20.0f, 1);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_generic_prisonerAI(creature);
    }
};

//69398
class npc_thok_ice_tomb : public CreatureScript
{
public:
    npc_thok_ice_tomb() : CreatureScript("npc_thok_ice_tomb") {}

    struct npc_thok_ice_tombAI : public ScriptedAI
    {
        npc_thok_ice_tombAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;
        EventMap events;
        uint64 sGuid;

        void Reset()
        {
            events.Reset();
            sGuid = 0;
        }

        void IsSummonedBy(Unit* summoner)
        {
            sGuid = summoner->GetGUID();
            events.ScheduleEvent(EVENT_CHECK_TPLAYER, 1000);
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void JustDied(Unit* killer)
        {
            if (Player* pl = me->GetPlayer(*me, sGuid))
            {
                pl->RemoveAurasDueToSpell(SPELL_FROZEN_SOLID);
                if (GameObject* it = me->FindNearestGameObject(GO_ICE_TOMB, 3.0f))
                    it->Delete();
            }
            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_CHECK_TPLAYER)
                {
                    if (Player* pl = me->GetPlayer(*me, sGuid))
                    {
                        if (!pl->isAlive())
                        {
                            if (GameObject* it = me->FindNearestGameObject(GO_ICE_TOMB, 3.0f))
                                it->Delete();
                            me->DespawnOrUnsummon();
                        }
                        else
                            events.ScheduleEvent(EVENT_CHECK_TPLAYER, 1000);
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_thok_ice_tombAI(creature);
    }
};

//73522
class npc_captive_cave_bat : public CreatureScript
{
public:
    npc_captive_cave_bat() : CreatureScript("npc_captive_cave_bat") {}

    struct npc_captive_cave_batAI : public ScriptedAI
    {
        npc_captive_cave_batAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset(){}

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_VAMPIRIC_FRENZY, urand(10000, 20000));
        }

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_VAMPIRIC_FRENZY)
                {
                    DoCastAOE(SPELL_VAMPIRIC_FRENZY, true);
                    events.ScheduleEvent(EVENT_VAMPIRIC_FRENZY, urand(10000, 20000));
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_captive_cave_batAI(creature);
    }
};

//71787
class npc_body_stalker : public CreatureScript
{
public:
    npc_body_stalker() : CreatureScript("npc_body_stalker") {}

    struct npc_body_stalkerAI : public ScriptedAI
    {
        npc_body_stalkerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE |UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;

        void Reset(){}
        
        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_body_stalkerAI(creature);
    }
};

//71645
class npc_shock_collar : public CreatureScript
{
public:
    npc_shock_collar() : CreatureScript("npc_shock_collar") {}

    struct npc_shock_collarAI : public ScriptedAI
    {
        npc_shock_collarAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->AddAura(SPELL_CANNON_BALL_DESTD, me);
        }

        InstanceScript* instance;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_shock_collarAI(creature);
    }
};

//73184
class npc_starved_yeti : public CreatureScript
{
public:
    npc_starved_yeti() : CreatureScript("npc_starved_yeti") {}

    struct npc_starved_yetiAI : public ScriptedAI
    {
        npc_starved_yetiAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        InstanceScript* instance;
        EventMap events;
        SummonList summons;
        float x, y;

        void Reset(){}

        void JustSummoned(Creature* sum)
        {
            summons.Summon(sum);
            events.ScheduleEvent(EVENT_Y_CHARGE, 2000);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (pointId == 1)
                {
                    events.CancelEvent(EVENT_CHECKER);
                    summons.DespawnAll();
                    DoCast(me, SPELL_CANNON_BALL, true);
                    if (Creature* bs = me->FindNearestCreature(NPC_BODY_STALKER, 100.0f, true))
                    {
                        float ang1 = me->GetAngle(bs);
                        float ang = ang1 + GetAngleMod();
                        me->SetFacingTo(ang);
                        GetPositionWithDistInOrientation(me, 135.0f, ang, x, y);
                        events.ScheduleEvent(EVENT_PRE_Y_CHARGE, 15000);
                    }
                }
            }
        }

        float GetAngleMod()
        {
            float mod = float(urand(0, 1));
            mod = !mod ? -1 : 1;
            float mod2 = float(urand(1, 5))/10;
            float modangle = mod*mod2;
            return modangle;
        }

        void IsSummonedBy(Unit* summoner)
        {
            GetPositionWithDistInOrientation(me, 135.0f, me->GetOrientation(), x, y);
            me->SummonCreature(NPC_SHOCK_COLLAR, x, y, me->GetPositionZ());
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECKER:
                {
                    std::list<Player*> plist;
                    plist.clear();
                    GetPlayerListInGrid(plist, me, 10.0f);
                    if (!plist.empty())
                    {
                        for (std::list<Player*>::const_iterator itr = plist.begin(); itr != plist.end(); itr++)
                            if ((*itr)->isInFront(me))
                                DoCast(*itr, SPELL_CANNON_BALL_ATDMG, true);
                    }
                    events.ScheduleEvent(EVENT_CHECKER, 750);
                    break;
                }
                case EVENT_PRE_Y_CHARGE:
                    me->SummonCreature(NPC_SHOCK_COLLAR, x, y, me->GetPositionZ());
                    break;
                case EVENT_Y_CHARGE:
                    if (Creature* sc = me->FindNearestCreature(NPC_SHOCK_COLLAR, 135.0f, true))
                        me->SetFacingToObject(sc);
                    me->GetMotionMaster()->MoveCharge(x, y, me->GetPositionZ(), 30.0f, 1);
                    events.ScheduleEvent(EVENT_CHECKER, 750);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_starved_yetiAI(creature);
    }
};

//143345
class spell_power_regen : public SpellScriptLoader
{
public:
    spell_power_regen() : SpellScriptLoader("spell_power_regen") { }

    class spell_power_regen_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_power_regen_AuraScript);

        void OnPeriodic(AuraEffect const* aurEff)
        {
            if (GetCaster())
            {
                if (GetCaster()->GetPower(POWER_ENERGY) == 100)
                {
                    GetCaster()->SetPower(POWER_ENERGY, 0);
                    GetCaster()->CastSpell(GetCaster(), SPELL_DEAFENING_SCREECH);
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_power_regen_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_power_regen_AuraScript();
    }
};

//143430
class spell_clump_check : public SpellScriptLoader
{
public:
    spell_clump_check() : SpellScriptLoader("spell_clump_check") { }

    class spell_clump_check_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_clump_check_SpellScript);

        void HandleOnHit()
        {
            if (GetHitUnit() && GetCaster())
            {
                if (!GetHitUnit()->HasAura(SPELL_BLOODIED))
                {
                    if (GetHitUnit()->HealthBelowPct(50))
                    {
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            if (!instance->GetData(DATA_THOK))
                                return;

                        GetHitUnit()->AddAura(SPELL_BLOODIED, GetHitUnit());
                    }
                }
                else
                {
                    /*//Test Only (For testing Solo)
                    if (GetCaster()->HasAura(SPELL_POWER_REGEN))
                    {
                        GetCaster()->RemoveAurasDueToSpell(SPELL_POWER_REGEN);
                        GetCaster()->ToCreature()->AI()->DoAction(ACTION_PHASE_TWO);
                    }*/
                    std::list<Player*>pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, GetHitUnit(), 10.0f);
                    if (!pllist.empty())
                    {
                        //uint8 count = GetCaster()->GetMap()->Is25ManRaid() ? 15 : 5; Test Only
                        if (pllist.size() >= 2)//count)
                        {
                            if (GetCaster()->HasAura(SPELL_POWER_REGEN))
                            {
                                GetCaster()->RemoveAurasDueToSpell(SPELL_POWER_REGEN);
                                GetCaster()->ToCreature()->AI()->DoAction(ACTION_PHASE_TWO);
                            }
                        }
                    }
                }
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_clump_check_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_clump_check_SpellScript();
    }
};

//143445
class spell_fixate : public SpellScriptLoader
{
public:
    spell_fixate() : SpellScriptLoader("spell_fixate") { }

    class spell_fixate_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_fixate_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget() && GetCaster())
            {
                GetCaster()->CastSpell(GetCaster(), SPELL_FIXATE_IM, true);
                GetCaster()->AddThreat(GetTarget(), 50000000.0f);
                GetCaster()->ClearUnitState(UNIT_STATE_CASTING);
                GetCaster()->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                GetCaster()->Attack(GetTarget(), true);
            }
        }

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTarget() && GetCaster())
            {
                if (GetCaster()->isAlive() && GetCaster()->HasAura(SPELL_BLOOD_FRENZY))
                {
                    GetCaster()->ToCreature()->SetReactState(REACT_PASSIVE);
                    GetCaster()->StopMoving();
                    GetCaster()->getThreatManager().resetAllAggro();
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_FIXATE);
                }
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_fixate_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_POSSESS_PET, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_fixate_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_POSSESS_PET, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_fixate_AuraScript();
    }
};

//146589
class spell_unlocking : public SpellScriptLoader
{
public:
    spell_unlocking() : SpellScriptLoader("spell_unlocking") { }

    class spell_unlocking_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_unlocking_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                for (uint8 n = 0; n < 3; n++)
                {
                    if (Creature* p = GetTarget()->FindNearestCreature(prisonersentry[n], 40.0f, true))
                    {
                        p->NearTeleportTo(fpos[n].GetPositionX(), fpos[n].GetPositionY(), fpos[n].GetPositionZ(), fpos[n].GetOrientation());
                        p->AI()->DoAction(ACTION_FREEDOM);
                        break;
                    }
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_unlocking_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_unlocking_AuraScript();
    }
};

//143800
class spell_icy_blood : public SpellScriptLoader
{
public:
    spell_icy_blood() : SpellScriptLoader("spell_icy_blood") { }

    class spell_icy_blood_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_icy_blood_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            if (GetTarget() && GetTarget()->HasAura(SPELL_ICY_BLOOD))
            {
                if (GetTarget()->GetAura(SPELL_ICY_BLOOD)->GetStackAmount() >= 5 && !GetTarget()->HasAura(SPELL_FROZEN_SOLID))
                {
                    GetTarget()->AddAura(SPELL_FROZEN_SOLID, GetTarget());
                    GetTarget()->RemoveAurasDueToSpell(SPELL_ICY_BLOOD);
                    GetTarget()->CastSpell(GetTarget(), SPELL_SUMMON_ICE_TOMB, true);
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_icy_blood_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_icy_blood_AuraScript();
    }
};


void AddSC_boss_thok_the_bloodthirsty()
{
    new boss_thok_the_bloodthirsty();
    new npc_korkron_jailer();
    new npc_generic_prisoner();
    new npc_thok_ice_tomb();
    new npc_captive_cave_bat();
    new npc_body_stalker();
    new npc_shock_collar();
    new npc_starved_yeti();
    new spell_power_regen();
    new spell_clump_check();
    new spell_fixate();
    new spell_unlocking();
    new spell_icy_blood();
}
