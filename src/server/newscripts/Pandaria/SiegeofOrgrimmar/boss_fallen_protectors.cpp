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
#include "CreatureTextMgr.h"

enum eSpells
{
    SPELL_BOUND_OF_GOLDEN_LOTUS             = 143497, //Bond of the Golden Lotus
    SPELL_EJECT_ALL_PASSANGERS              = 68576,
    SPELL_DESPAWN_AT                        = 138175,

    //Rook
    SPELL_VENGEFUL_STRIKE                   = 144396, //Vengeful Strikes
    SPELL_CORRUPTED_BREW                    = 143019, //Corrupted Brew
    SPELL_CLASH                             = 143027, //Clash   cast 143028
    SPELL_CORRUPTION_KICK                   = 143007, //Corruption Kick
    SPELL_MISERY_SORROW_GLOOM               = 143955, //Misery, Sorrow, and Gloom

    //He
    SPELL_GARROTE                           = 143198, //Garrote
    SPELL_GOUGE                             = 143301, //Gouge
    SPELL_NOXIOUS_POISON                    = 143225, //Noxious Poison
    SPELL_INSTANT_POISON                    = 143210, //Instant Poison
    SPELL_MARK_OF_ANGUISH_MEDITATION        = 143812, //Mark of Anguish

    //Sun
    SPELL_SHA_SEAR                          = 143423, //Sha Sear
    SPELL_SHADOW_WORD_BANE                  = 143434, //Shadow Word: Bane
    SPELL_CALAMITY                          = 143491, //Calamity
    SPELL_DARK_MEDITATION                   = 143546,
    SPELL_DARK_MEDITATION_JUMP              = 143730, //Prock after jump 143546
    SPELL_DARK_MEDITATION_SHARE_HEALTH_P    = 143745, //
    SPELL_DARK_MEDITATION_SHARE_HEALTH      = 143723,

    SPELL_CLEAR_ALL_DEBUFS                  = 34098,  //ClearAllDebuffs
    SPELL_SHA_CORRUPTION                    = 143708, // begore 34098 and 17683
    SPELL_FULL_HEALTH                       = 17683,

    //meashures of sun
    SPELL_MANIFEST_DESPERATION              = 144504, //Manifest Desperation of 71482
    SPELL_MANIFEST_DESPAIR                  = 143746, //Manifest Despair of 71474
    SPELL_SHA_CORRUPTION_OF_SUN             = 142891,

    //meashures of he
    SPELL_SHA_CORRUPTION_SUMMONED           = 142885,
    SPELL_MARK_OF_ANGUISH_JUMP              = 143808, //Mark of Anguish
    SPELL_MARK_OF_ANGUISH_SELECT_TARGET     = 143822, //Mark of Anguish by 143840
    SPELL_MARK_OF_ANGUISH_STAN              = 143840,
    SPELL_MARK_OF_ANGUISH_DAMAGE            = 144365,
    SPELL_MARK_OF_ANGUISH_GIVE_A_FRIEND     = 143842,
    SPELL_SHADOW_WEAKNES_PROC               = 144079, //prock spell on hit or something else
    SPELL_DEBILITATION                      = 147383, //Debilitation
    SPELL_SHADOW_WEAKNESS                   = 144176, //charges targetGUID: Full: 0x70000000695B52A Type: Player Low: 110474538 
    SPELL_SHADOW_WEAKNES_MASS               = 144081,

    //measures of rook
    SPELL_SHA_CORRUPTION_MIS_OF_ROOK        = 142892,
    SPELL_SHA_CORRUPTION_GLOOM_OF_ROOK      = 142889,
    SPELL_SHA_CORRUPTION_SOR_OF_ROOK        = 142893,
    SPELL_MISERY_SORROW_GLOOM_SUMON         = 143948,
    SPELL_DEFILED_GROUND                    = 143961, //Defiled Ground apply 143959
    SPELL_DEFILE_GROUND_PROC                = 143959,
    SPELL_INFERNO_STRIKE                    = 143962, //Inferno Strike
    SPELL_CORRUPTION_SHOCK                  = 143958, //Corruption Shock
};

enum Phases
{
    PHASE_BATTLE                    = 1,
    PHASE_DESPERATE_MEASURES        = 2,
    PHASE_BOND_GOLDEN_LOTUS         = 3
};

enum PhaseEvents
{
    EVENT_LOTUS                     = 1,    
    EVENT_DESPERATE_MEASURES        = 2,//Desperate Measures
};

enum data
{
    BATTLE_AREA                    = 6798,

    DATA_SHADOW_WORD_DAMAGE        = 1,
    DATA_SHADOW_WORD_REMOVED       = 2,
    DATA_CALAMITY_HIT              = 3,
};

uint32 const protectors[3] = { NPC_ROOK_STONETOE, NPC_SUN_TENDERHEART, NPC_HE_SOFTFOOT };

struct boss_fallen_protectors : public BossAI
{
    boss_fallen_protectors(Creature* creature) : BossAI(creature, DATA_F_PROTECTORS)
    {
        measureVeh = 0;
        measureSummonedCount = 0;
    }

    int8 _healthPhase;
    uint32 measureVeh;      //inut on subclas.
    uint32 measureSummonedCount;

    void Reset()
    {
        _Reset();
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        _healthPhase = 0;
        me->CastSpell(me, SPELL_DESPAWN_AT, true);
        //me->RemoveAllAreaObjects();
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
    }

    void EnterCombat(Unit* who)
    {
        InitBattle();

        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
        DoZoneInCombat(me, 150.0f);
        
        for (int32 i = 0; i < 3; ++i)
        {
            if (me->GetEntry() == protectors[i])
                continue;

            if (Creature* prot = ObjectAccessor::GetCreature(*me, instance->GetData64(protectors[i])))
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, prot);
                DoZoneInCombat(prot, 150.0f);
            }
        }
    }

    virtual void InitBattle() { 
        events.SetPhase(PHASE_BATTLE);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    // remove from PHASE_BOND_GOLDEN_LOTUS
    void HealReceived(Unit* /*done_by*/, uint32& addhealth)
    {
        float newpct = GetHealthPctWithHeal(addhealth);
        if ((_healthPhase == 0 && newpct <= 66.0f) ||
            (_healthPhase == 1 && newpct <= 33.0f))
            --_healthPhase;

        events.SetPhase(PHASE_BATTLE);
    }

    bool CheckLotus()
    {
        for (int32 i = 0; i < 3; ++i)
        {
            if (Creature* prot = ObjectAccessor::GetCreature(*me, instance->GetData64(protectors[i])))
            {
                if (prot->GetHealth() != 1)
                    return false;
            }
        }
        return true;
    }

    void DamageTaken(Unit* attacker, uint32 &damage)
    {
        if (me->GetHealth() <= damage)
        {
            damage = me->GetHealth() - 1;

            if (!events.IsInPhase(PHASE_BOND_GOLDEN_LOTUS))
            {
                me->InterruptNonMeleeSpells(false);
                DoCast(me, SPELL_BOUND_OF_GOLDEN_LOTUS, false);

                events.SetPhase(PHASE_BOND_GOLDEN_LOTUS);
                events.RescheduleEvent(EVENT_LOTUS, 1*IN_MILLISECONDS, 0, PHASE_BOND_GOLDEN_LOTUS);   //BreakIfAny
            }else if (CheckLotus())
            {
                //END EVENT
                damage = me->GetHealth();
                _JustDied();

                for (int32 i = 0; i < 3; ++i)
                {
                    if (me->GetEntry() == protectors[i])
                        continue;

                    if (Creature* prot = ObjectAccessor::GetCreature(*me, instance->GetData64(protectors[i])))
                        attacker->Kill(prot, true);
                }
            }
            return;
        }

        if ((_healthPhase == 0 && GetHealthPct(damage) <= 66.0f) ||
            (_healthPhase == 1 && GetHealthPct(damage) <= 33.0f))
        {
            ++_healthPhase;
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
            me->GetMotionMaster()->MovementExpired();
            events.SetPhase(PHASE_DESPERATE_MEASURES);
            events.RescheduleEvent(EVENT_DESPERATE_MEASURES, 1*IN_MILLISECONDS, 0, PHASE_DESPERATE_MEASURES);   //BreakIfAny
            me->InterruptNonMeleeSpells(false);
        }
    }

    void JustDied(Unit* /*killer*/)
    {
        events.Reset();
        summons.DespawnAll();

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
    }

    void summonDesperation()
    {
        Creature* lotos = instance->instance->GetCreature(instance->GetData64(measureVeh));
        if (!lotos)
        {
            sLog->outError(LOG_FILTER_GENERAL, " >> Script boss_fallen_protectors::summonDesperation con't get %u for %u", measureVeh, me->GetEntry());
            return;
        }

        Vehicle * vehicle = lotos->GetVehicleKit();
        if (!vehicle)
        {
            sLog->outError(LOG_FILTER_GENERAL, " >> Script boss_fallen_protectors::summonDesperation unit %u of %u is not vehicle", measureVeh, me->GetEntry());
            return;
        }

        //lotos->CastSpell(lotos, SPELL_EJECT_ALL_PASSANGERS, true);

        SeatMap tempSeatMap = vehicle->Seats;
        for (SeatMap::iterator itr = tempSeatMap.begin(); itr != tempSeatMap.end(); ++itr)
        {
            if (!itr->second.Passenger)
                continue;

            Unit* passenger = ObjectAccessor::FindUnit(itr->second.Passenger);
            if (!passenger)
                continue;

            TempSummon* summon = passenger->ToTempSummon();
            if (!summon)
            {
                sLog->outError(LOG_FILTER_GENERAL, " >> Script boss_fallen_protectors::summonDesperation unit %u has not tempSummon passanger entry %u", measureVeh, passenger->GetEntry());
                continue;
            }

            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, summon);
            summon->ExitVehicle();
            ++measureSummonedCount;
        }
    }

    void DoAction(int32 const action)
    {
        switch(action)
        {
            //Check if all boses have 1 pct. If == true -> setbossstate(DONE)
            case EVENT_1:
                events.RescheduleEvent(EVENT_1, 1*IN_MILLISECONDS, 0, PHASE_BOND_GOLDEN_LOTUS);   //BreakIfAny
                break;
            case EVENT_DESPERATE_MEASURES:
                summonDesperation();
                break;
            // caled at measures die from instance::CreatureDies
            case NPC_EMBODIED_ANGUISH_OF_HE:
            case NPC_EMBODIED_DESPERATION_OF_SUN:
            case NPC_EMBODIED_DESPIRE_OF_SUN:
            case NPC_EMBODIED_MISERY_OF_ROOK:
            case NPC_EMBODIED_GLOOM_OF_ROOK:
            case NPC_EMBODIED_SORROW_OF_ROOK:
            {
                --measureSummonedCount;
                // END EVENT_DESPERATE_MEASURES. Countinue attacking.
                if (!measureSummonedCount)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    InitBattle();
                    me->RemoveAllAreaObjects();
                    me->RemoveAurasDueToSpell(SPELL_DARK_MEDITATION);   //sun
                    me->RemoveAurasDueToSpell(SPELL_MARK_OF_ANGUISH_MEDITATION);   //he
                    me->RemoveAurasDueToSpell(SPELL_MISERY_SORROW_GLOOM);   //rook                    
                    me->SetInCombatWithZone();
                }
                break;
            }
        }
    }
};

//Rook Stonetoe
class boss_rook_stonetoe : public CreatureScript
{
    public:
        boss_rook_stonetoe() : CreatureScript("boss_rook_stonetoe") {}

        struct boss_rook_stonetoeAI : public boss_fallen_protectors
        {
            boss_rook_stonetoeAI(Creature* creature) : boss_fallen_protectors(creature)
            {
                measureVeh = NPC_GOLD_LOTOS_ROOK;
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
            }

            enum local
            {
                EVENT_VENGEFUL_STRIKE   = 5,
                EVENT_CORRUPTED_BREW    = 6,
                EVENT_CLASH             = 7,
            };

            void InitBattle()
            {
                boss_fallen_protectors::InitBattle();

                events.RescheduleEvent(EVENT_VENGEFUL_STRIKE, urand(10*IN_MILLISECONDS, 20*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_CORRUPTED_BREW, urand(IN_MILLISECONDS, 5*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_CLASH, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
            }

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);
                sCreatureTextMgr->SendChat(me, TEXT_GENERIC_3, me->GetGUID());
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
            }

            void UpdateAI(uint32 diff)
            {
                if (events.IsInPhase(PHASE_BATTLE) && !UpdateVictim() ||
                    !events.IsInPhase(PHASE_BOND_GOLDEN_LOTUS) && me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                EnterEvadeIfOutOfCombatArea(diff);
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    boss_fallen_protectors::DoAction(eventId);
                    switch (eventId)
                    {
                        case EVENT_VENGEFUL_STRIKE:
                            DoCastVictim(SPELL_VENGEFUL_STRIKE);
                            events.RescheduleEvent(EVENT_VENGEFUL_STRIKE, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_CORRUPTED_BREW:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 0.0f, true))
                                DoCast(target, SPELL_CORRUPTED_BREW, true);
                            events.RescheduleEvent(EVENT_CORRUPTED_BREW, urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_CLASH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_CLASH);
                            //DoCastVictim(SPELL_CLASH);
                            events.RescheduleEvent(EVENT_CLASH, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_DESPERATE_MEASURES:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_4, me->GetGUID());
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_5, me->GetGUID());
                            DoCast(me, SPELL_MISERY_SORROW_GLOOM, false);
                            break;
                        case EVENT_LOTUS:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_6, me->GetGUID());
                            break;
                        default:
                            break;
                    }
                }

                if (events.IsInPhase(PHASE_BATTLE))
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rook_stonetoeAI(creature);
        }
};

//He Softfoot
class boss_he_softfoot : public CreatureScript
{
    public:
        boss_he_softfoot() : CreatureScript("boss_he_softfoot") {}

        struct boss_he_softfootAI : public boss_fallen_protectors
        {
            boss_he_softfootAI(Creature* creature) : boss_fallen_protectors(creature)
            {
                measureVeh = NPC_GOLD_LOTOS_HE;
            }

            void Reset()
            {
                boss_fallen_protectors::Reset();
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GARROTE);
            }

            enum local
            {
                EVENT_GARROTE                   = 5,
                EVENT_GOUGE                     = 6,
                EVENT_POISON_NOXIOUS            = 7,
                EVENT_POISON_INSTANT            = 8,
            };

            void InitBattle()
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHADOW_WEAKNESS);

                boss_fallen_protectors::InitBattle();

                events.RescheduleEvent(EVENT_GARROTE, 5*IN_MILLISECONDS, 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_GOUGE, urand(IN_MILLISECONDS, 5*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_POISON_NOXIOUS, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
            }

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);
                DoCast(who, SPELL_INSTANT_POISON, false);
            }

            void AttackStart(Unit* target)
            {
                if (!events.IsInPhase(PHASE_BATTLE))
                    return;

                BossAI::AttackStart(target);
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GARROTE);
            }

            bool AllowSelectNextVictim(Unit* target)
            {
                // Go next raid member.
                return !target->HasUnitState(UNIT_STATE_STUNNED);
            }

            //bool CanAIAttack(Unit const* target) const
            //{
            //    return const_cast<EventMap*>(&events)->IsInPhase(PHASE_BATTLE);
            //}

            void UpdateAI(uint32 diff)
            {
                if (events.IsInPhase(PHASE_BATTLE) && !UpdateVictim() ||
                    !events.IsInPhase(PHASE_BOND_GOLDEN_LOTUS) && me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);
                EnterEvadeIfOutOfCombatArea(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    boss_fallen_protectors::DoAction(eventId);
                    switch (eventId)
                    {
                        case EVENT_GARROTE:
                            if (Unit* target = me->getVictim())
                                if (!target->HasAura(SPELL_GARROTE))
                                    DoCast(target, SPELL_GARROTE, false);
                            events.RescheduleEvent(EVENT_GARROTE, 5*IN_MILLISECONDS, 0, PHASE_BATTLE);
                            break;
                        case EVENT_GOUGE:
                            DoCastVictim(SPELL_GOUGE);
                            events.RescheduleEvent(EVENT_GOUGE, urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_POISON_NOXIOUS:
                            events.RescheduleEvent(EVENT_POISON_INSTANT, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            DoCastVictim(SPELL_NOXIOUS_POISON);
                            break;
                        case EVENT_POISON_INSTANT:
                            DoCastVictim( SPELL_INSTANT_POISON);
                            events.RescheduleEvent(EVENT_POISON_NOXIOUS, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            me->RemoveAllAreaObjects();
                            break;
                        case EVENT_DESPERATE_MEASURES:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_1, me->GetGUID());
                            DoCast(me, SPELL_MARK_OF_ANGUISH_MEDITATION, false);
                            break;
                        case EVENT_LOTUS:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_2, me->GetGUID());
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_3, me->GetGUID());
                            break;
                    }
                }
                if (events.IsInPhase(PHASE_BATTLE))
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_he_softfootAI(creature);
        }
};

//Sun Tenderheart
class boss_sun_tenderheart : public CreatureScript
{
    public:
        boss_sun_tenderheart() : CreatureScript("boss_sun_tenderheart") {}

        struct boss_sun_tenderheartAI : public boss_fallen_protectors
        {
            boss_sun_tenderheartAI(Creature* creature) : boss_fallen_protectors(creature)
            {
                SetCombatMovement(false);
                measureVeh = NPC_GOLD_LOTOS_SUN;
            }

            uint32 shadow_word_count;
            void Reset()
            {
                boss_fallen_protectors::Reset();
                shadow_word_count = 0;

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHADOW_WORD_BANE);
            }

            enum local
            {
                EVENT_SHA_SEAR            = 5,
                EVENT_SHADOW_WORD_BANE    = 6,
                EVENT_CALAMITY            = 7,
            };

            void InitBattle()
            {
                boss_fallen_protectors::InitBattle();

                events.RescheduleEvent(EVENT_SHA_SEAR, IN_MILLISECONDS, 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_SHADOW_WORD_BANE, urand(15*IN_MILLISECONDS, 25*IN_MILLISECONDS), 0, PHASE_BATTLE);
                events.RescheduleEvent(EVENT_CALAMITY, urand(60*IN_MILLISECONDS, 70*IN_MILLISECONDS), 0, PHASE_BATTLE);
            }

            void EnterCombat(Unit* who)
            {
                boss_fallen_protectors::EnterCombat(who);
                sCreatureTextMgr->SendChat(me, TEXT_GENERIC_1, me->GetGUID());
            }

            void DoAction(int32 const action)
            {
                boss_fallen_protectors::DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                boss_fallen_protectors::JustDied(NULL);
                sCreatureTextMgr->SendChat(me, TEXT_GENERIC_6, me->GetGUID());
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHADOW_WORD_BANE);
            }

            void SetData(uint32 type, uint32 value)
            {
                switch(type)
                {
                    case DATA_SHADOW_WORD_DAMAGE:
                        events.ScheduleEvent(EVENT_SHADOW_WORD_BANE, 1, 0, PHASE_BATTLE);
                        break;
                    case DATA_SHADOW_WORD_REMOVED:
                        --shadow_word_count;
                        break;
                    // calamity hit caled every hit on target and it's right.
                    case DATA_CALAMITY_HIT:
                        //remove shadow word bane
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHADOW_WORD_BANE);     //this call DATA_SHADOW_WORD_REMOVED
                        events.RescheduleEvent(EVENT_SHADOW_WORD_BANE, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE); //reschedal remove curent events.
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (events.IsInPhase(PHASE_BATTLE) && !UpdateVictim() ||
                    !events.IsInPhase(PHASE_BOND_GOLDEN_LOTUS) && me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);
                EnterEvadeIfOutOfCombatArea(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    boss_fallen_protectors::DoAction(eventId);
                    switch (eventId)
                    {
                        case EVENT_SHA_SEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 0.0f, true))
                                DoCast(target, SPELL_SHA_SEAR, true);
                            events.RescheduleEvent(EVENT_SHA_SEAR, urand(5*IN_MILLISECONDS, 10*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_SHADOW_WORD_BANE:
                            if (shadow_word_count < 3){
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, -SPELL_SHADOW_WORD_BANE))
                                    DoCast(target, SPELL_SHADOW_WORD_BANE, true);
                                ++shadow_word_count;
                            }
                            events.RescheduleEvent(EVENT_SHADOW_WORD_BANE, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_CALAMITY:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_2, 0);
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_3, 0);
                            DoCastVictim(SPELL_CALAMITY);
                            events.RescheduleEvent(EVENT_CALAMITY, urand(60*IN_MILLISECONDS, 70*IN_MILLISECONDS), 0, PHASE_BATTLE);
                            break;
                        case EVENT_DESPERATE_MEASURES:
                            if (Creature* lotos = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MAIN)))
                                DoCast(lotos, SPELL_DARK_MEDITATION_JUMP, true);
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_4, me->GetGUID());
                            break;
                        case EVENT_LOTUS:
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_5, me->GetGUID());
                            break;
                    }
                }
                //DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sun_tenderheartAI(creature);
        }
};

//Golden Lotus
class npc_golden_lotus_control : public CreatureScript
{
public:
    npc_golden_lotus_control() : CreatureScript("npc_golden_lotus_control") { }

    struct npc_golden_lotus_controlAI : public ScriptedAI
    {
        npc_golden_lotus_controlAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();            
        }

        enum data
        {
            SUMMON_MOVER        = 143705,
            SPELL_FACE_CHANNEL  = 116351,
        };

        InstanceScript* instance;
        EventMap events;

        //void JustSummoned(Creature* creature)
        //{
        //    if (creature->GetEntry() == NPC_GOLD_LOTOS_MOVER)
        //    {
        //        creature->SetDisplayId(48920);
        //        creature->GetMotionMaster()->MoveIdle();
        //        creature->LoadPath(creature->GetEntry());
        //        creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        //        creature->GetMotionMaster()->Initialize();
        //        if (creature->isAlive())                            // dead creature will reset movement generator at respawn
        //        {
        //            creature->setDeathState(JUST_DIED);
        //            creature->Respawn(true);
        //        }
        //    }
        //};

        void Reset()
        {
            //Use manual spawn.
            //me->CastSpell(me, SUMMON_MOVER, true);
            events.RescheduleEvent(EVENT_1, 1000);
        }
        
        void OnCharmed(bool /*apply*/)
        {
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (apply)
            {
                if (Creature* mover = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MOVER)))
                {
                    who->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                    if (!me->HasAura(SPELL_FACE_CHANNEL))
                        me->CastSpell(mover, SPELL_FACE_CHANNEL, true);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {

            }
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_golden_lotus_controlAI(creature);
    }
};

Position const LotusJumpPosition[6]   =
{
    {1214.094f, 1006.571f, 418.0658f, 0.0f}, //NPC_EMBODIED_DESPIRE_OF_SUN
    {1212.148f, 1057.528f, 417.1646f, 0.0f}, //NPC_EMBODIED_DESPERATION_OF_SUN
    {1204.724f, 1055.807f, 417.6278f, 0.0f}, //NPC_EMBODIED_ANGUISH_OF_HE
    {1202.516f, 1011.427f, 418.1869f, 0.0f}, //NPC_EMBODIED_MISERY_OF_ROOK
    {1208.924f, 1014.313f, 452.1267f, 0.0f}, //NPC_EMBODIED_GLOOM_OF_ROOK
    {1228.698f, 1038.337f, 418.0633f, 0.0f}, //NPC_EMBODIED_SORROW_OF_ROOK
};

class ExitVexMeasure : public BasicEvent
{
    public:
        explicit ExitVexMeasure(Creature *c) : creature(c) { }

        bool Execute(uint64 /*currTime*/, uint32 /*diff*/)
        {
            uint8 _idx = 0;
            switch(creature->GetEntry())
            {
                case NPC_EMBODIED_DESPIRE_OF_SUN:       _idx = 0; break;
                case NPC_EMBODIED_DESPERATION_OF_SUN:   _idx = 1; break;
                case NPC_EMBODIED_ANGUISH_OF_HE:        _idx = 2; break;
                case NPC_EMBODIED_MISERY_OF_ROOK:       _idx = 3; break;
                case NPC_EMBODIED_GLOOM_OF_ROOK:        _idx = 4; break;
                case NPC_EMBODIED_SORROW_OF_ROOK:       _idx = 5; break;
                default:
                    sLog->outError(LOG_FILTER_GENERAL, " >> Script: OO:ExitVexMeasure no position for fall down for entry %u", creature->GetEntry());
                    return true;
            }
            creature->GetMotionMaster()->MoveJump(LotusJumpPosition[_idx].m_positionX, LotusJumpPosition[_idx].m_positionY, LotusJumpPosition[_idx].m_positionZ, 20.0f, 20.0f);
            creature->AI()->DoAction(EVENT_1);
            return true;
        }

    private:
        Creature *creature;
};

class vehicle_golden_lotus_conteiner : public VehicleScript
{
    public:
        vehicle_golden_lotus_conteiner() : VehicleScript("vehicle_golden_lotus_conteiner") {}

        void OnAddPassenger(Vehicle* veh, Unit* passenger, int8 /*seatId*/)
        {

        }

        void OnRemovePassenger(Vehicle* veh, Unit* passenger)
        {
            Unit* own = veh->GetBase();
            if (!own)
                return;
            InstanceScript* instance = own->GetInstanceScript();
            if (!instance)
                return;

            Creature* lotos = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MAIN));
            if (!lotos)
                return;

            passenger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            passenger->m_Events.AddEvent(new ExitVexMeasure(passenger->ToCreature()), passenger->m_Events.CalculateTime(1000));
        }
};

struct npc_measure : public ScriptedAI
{
    npc_measure(Creature* creature) : ScriptedAI(creature), summons(creature)
    {
        instance = creature->GetInstanceScript();
        ownVehicle = 0;
        ownSummoner = 0;
    }

    InstanceScript* instance;
    SummonList summons;
    EventMap events;
    uint32 ownVehicle;
    uint32 ownSummoner;

    void Reset()
    {
        summons.DespawnAll();
        events.Reset();
    }

    //return back
    void EnterEvadeMode()
    {    
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MARK_OF_ANGUISH_STAN); //he
        me->InterruptNonMeleeSpells(false);
        goBack();
        instance->SetData(DATA_FP_EVADE, true);
    }

    void goBack()
    {
        if (me->GetVehicle())
            return;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        summons.DespawnAll();
        me->CastSpell(me, SPELL_DESPAWN_AT, true);

        if (Creature* owner = instance->instance->GetCreature(instance->GetData64(ownSummoner)))
            owner->AI()->DoAction(me->GetEntry());
        else
            sLog->outError(LOG_FILTER_GENERAL, " >> Script boss_fallen_protectors::npc_measure can't find owner %u", ownSummoner);

        if (Creature* lotos = instance->instance->GetCreature(instance->GetData64(ownVehicle)))
        {
            me->CastSpell(me, SPELL_CLEAR_ALL_DEBUFS, true);
            me->CastSpell(me, SPELL_FULL_HEALTH, true);
            me->RemoveAura(SPELL_SHA_CORRUPTION_SUMMONED);          //he
            me->RemoveAura(SPELL_SHA_CORRUPTION_MIS_OF_ROOK);       //rook
            me->RemoveAura(SPELL_SHA_CORRUPTION_GLOOM_OF_ROOK);     //rook
            me->RemoveAura(SPELL_SHA_CORRUPTION_SOR_OF_ROOK);       //rook
            me->RemoveAura(SPELL_SHA_CORRUPTION_OF_SUN);            //sun
            
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->EnterVehicle(lotos, vehSlotForMeasures(me->GetEntry()));
            me->CastSpell(me, SPELL_SHA_CORRUPTION, true);
        }
        else
            sLog->outError(LOG_FILTER_GENERAL, " >> Script boss_fallen_protectors::npc_measure can't find vehowner %u", ownVehicle);
    }

    void DamageTaken(Unit* attacker, uint32 &damage)
    {
        if (damage >= me->GetHealth())
        {
            goBack();
            damage = 0;
        }
    }

    void JustSummoned(Creature* summon)
    {
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature* summon)
    {
        summons.Despawn(summon);
    }

    void DoAction(int32 const action)
    {
        //Start measure event. onExit from veh.
        if (action == EVENT_1)
        {
            me->RemoveAura(SPELL_SHA_CORRUPTION);
            events.ScheduleEvent(EVENT_1, 4000);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetInCombatWithZone();
        }
    }

    void OnCharmed(bool /*apply*/)
    {
    }

    void UpdateAI(uint32 diff)
    {
        events.Update(diff);
        EnterEvadeIfOutOfCombatArea(diff);
    }
};

class npc_measure_of_sun : public CreatureScript
{
public:
    npc_measure_of_sun() : CreatureScript("npc_measure_of_sun") { }

    struct npc_measure_of_sunAI : public npc_measure
    {
        npc_measure_of_sunAI(Creature* creature) : npc_measure(creature)
        {
            SetCombatMovement(false);
            ownVehicle = NPC_GOLD_LOTOS_SUN;
            ownSummoner = NPC_SUN_TENDERHEART;
        }

        uint32 _spell;

        void Reset()
        {
            _spell = 0;
            npc_measure::Reset();
        }

        void DoAction(int32 const action)
        {
            npc_measure::DoAction(action);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, SPELL_SHA_CORRUPTION_OF_SUN, true);
        }
        void JustSummoned(Creature* summon)
        {
            npc_measure::JustSummoned(summon);
            DoCast(summon, SPELL_DARK_MEDITATION_SHARE_HEALTH, true);
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                summon->AI()->AttackStart(target);
        }

        void UpdateAI(uint32 diff)
        {
            npc_measure::UpdateAI(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        _spell = me->GetEntry() == NPC_EMBODIED_DESPERATION_OF_SUN ? SPELL_MANIFEST_DESPERATION : SPELL_MANIFEST_DESPAIR;
                        DoCast(me, _spell, true);
                        if (Creature* lotos = instance->instance->GetCreature(instance->GetData64(NPC_GOLD_LOTOS_MAIN)))
                            me->SetFacingToObject(lotos);
                        events.ScheduleEvent(EVENT_2, 4000);
                        break;
                    case EVENT_2:
                    {
                        const Map::PlayerList &PlayerList = me->GetMap()->GetPlayers();
                        if (PlayerList.isEmpty())
                        {
                            EnterEvadeMode();
                            return;
                        }
                        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                            if (Player* player = i->getSource())
                            {
                                if (player->GetAreaId() != BATTLE_AREA)
                                {
                                    EnterEvadeMode();
                                    return;
                                }
                            }
                        events.ScheduleEvent(EVENT_2, 4000);
                        break;
                    }
                }
            }

            if (!_spell || !UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            DoCast(me, _spell, false);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_measure_of_sunAI(creature);
    }
};

class npc_measure_of_he : public CreatureScript
{
public:
    npc_measure_of_he() : CreatureScript("npc_measure_of_he") { }

    struct npc_measure_of_heAI : public npc_measure
    {
        npc_measure_of_heAI(Creature* creature) : npc_measure(creature)
        {
            ownVehicle = NPC_GOLD_LOTOS_HE;
            ownSummoner = NPC_HE_SOFTFOOT;
            _target = 0;
        }

        uint64 _target;
        void SetGUID(uint64 guid, int32 /*id*/ = 0)
        {
            _target = guid;
            if (Unit* target = ObjectAccessor::FindUnit(guid))
            {
                if (Creature* owner = instance->instance->GetCreature(instance->GetData64(ownSummoner)))
                    sCreatureTextMgr->SendChat(owner, TEXT_GENERIC_0, 0);

                me->CastSpell(target, SPELL_MARK_OF_ANGUISH_JUMP, true);
                target->CastSpell(target, SPELL_DEBILITATION, true);
                me->CastSpell(target, SPELL_MARK_OF_ANGUISH_STAN, false);
                AttackStart(target);
            }
        }

        void AttackStart(Unit* target)
        {
            if (target->GetGUID() ==_target)
                npc_measure::AttackStart(target);
            else if (_target)
            {
                //If player leave from game or isDead find new target.
                Unit* target = ObjectAccessor::FindUnit(_target);
                if (!target || !target->isAlive())
                    events.ScheduleEvent(EVENT_1, 100);
            }
        }

        void DoAction(int32 const action)
        {
            npc_measure::DoAction(action);
            me->CastSpell(me, SPELL_SHA_CORRUPTION_SUMMONED, true); //dark aura
        }

        bool AllowSelectNextVictim(Unit* target)
        {
            // Only our aura target could be.
            return target->GetGUID() == _target;
        }

        void UpdateAI(uint32 diff)
        {
            UpdateVictim();

            npc_measure::UpdateAI(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                me->InterruptNonMeleeSpells(false);
                DoCast(me, SPELL_SHADOW_WEAKNES_PROC, true);
                DoCast(me, SPELL_MARK_OF_ANGUISH_SELECT_TARGET, true);
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_measure_of_heAI(creature);
    }
};

class npc_measure_of_rook : public CreatureScript
{
public:
    npc_measure_of_rook() : CreatureScript("npc_measure_of_rook") { }

    struct npc_measure_of_rookAI : public npc_measure
    {
        npc_measure_of_rookAI(Creature* creature) : npc_measure(creature)
        {
            ownVehicle = NPC_GOLD_LOTOS_ROOK;
            ownSummoner = NPC_ROOK_STONETOE;
            switch(creature->GetEntry())
            {
                case NPC_EMBODIED_MISERY_OF_ROOK: _spell = SPELL_DEFILED_GROUND; break;
                case NPC_EMBODIED_GLOOM_OF_ROOK: _spell = SPELL_CORRUPTION_SHOCK; break;
                case NPC_EMBODIED_SORROW_OF_ROOK: _spell = SPELL_INFERNO_STRIKE; break;
                default:
                    break;
            }
        }
        uint32 _spell;

        void DoAction(int32 const action)
        {
            npc_measure::DoAction(action);
            switch(me->GetEntry())
            {
                case NPC_EMBODIED_MISERY_OF_ROOK:
                    me->CastSpell(me, SPELL_SHA_CORRUPTION_MIS_OF_ROOK, true);
                    break;
                case NPC_EMBODIED_GLOOM_OF_ROOK:
                    me->CastSpell(me, SPELL_SHA_CORRUPTION_GLOOM_OF_ROOK, true);
                    break;
                case NPC_EMBODIED_SORROW_OF_ROOK:
                    me->CastSpell(me, SPELL_SHA_CORRUPTION_SOR_OF_ROOK, true);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_measure::UpdateAI(diff);

            if (!_spell || !UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                DoCastVictim(_spell);
                events.ScheduleEvent(EVENT_1, urand(10000, 15000));
            }         

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_measure_of_rookAI(creature);
    }
};

class spell_clash : public SpellScriptLoader
{
    public:
        spell_clash() : SpellScriptLoader("spell_OO_clash") { }

        class spell_clash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_clash_SpellScript);

            enum proc
            {
                SPELL_PROCK     = 143028,
            };
            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        caster->CastSpell(target, SPELL_PROCK, false);
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_clash_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_clash_SpellScript();
        }
};

//Corrupted Brew
class spell_corrupted_brew : public SpellScriptLoader
{
    public:
        spell_corrupted_brew() : SpellScriptLoader("spell_OO_corrupted_brew") { }

        class spell_corrupted_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_corrupted_brew_SpellScript);

            enum proc
            {
                SPELL_PROCK     = 143021,
            };

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        caster->CastSpell(target, SPELL_PROCK, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_corrupted_brew_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_corrupted_brew_SpellScript();
        }
};

class spell_gouge : public SpellScriptLoader
{
    public:
        spell_gouge() : SpellScriptLoader("spell_OO_gouge") { }

        class spell_gouge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gouge_SpellScript);

            void HandleEffect(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();
                if (!target)
                    return;

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (target->HasInArc(static_cast<float>(M_PI), caster))
                {
                    caster->getThreatManager().modifyThreatPercent(target, -100);
                    target->DeleteFromThreatList(caster);
                }else
                {
                    PreventHitAura();
                }
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_gouge_SpellScript::HandleEffect, EFFECT_1, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gouge_SpellScript();
        }
};

class spell_dark_meditation : public SpellScriptLoader
{
    public:
        spell_dark_meditation() : SpellScriptLoader("spell_OO_dark_meditation") { }

        enum proc
        {
            SPELL_PROCK     = 143559,
        };

        class spell_dark_meditation_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dark_meditation_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if(Unit* caster = GetCaster())
                    caster->CastSpell(caster, SPELL_PROCK, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_dark_meditation_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_dark_meditation_AuraScript();
        }
};

//Shadow Word: Bane
class spell_fallen_protectors_shadow_word_bane : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_shadow_word_bane() :  SpellScriptLoader("spell_fallen_protectors_shadow_word_bane") { }

        class spell_fallen_protectors_shadow_word_bane_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_fallen_protectors_shadow_word_bane_AuraScript);

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if(caster->GetAI())
                        caster->GetAI()->SetData(DATA_SHADOW_WORD_REMOVED, true);
            }

            void OnPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                    if(caster->GetAI())
                        caster->GetAI()->SetData(DATA_SHADOW_WORD_DAMAGE, true);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_fallen_protectors_shadow_word_bane_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_fallen_protectors_shadow_word_bane_AuraScript::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_fallen_protectors_shadow_word_bane_AuraScript();
        }
};

class spell_fallen_protectors_calamity : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_calamity() : SpellScriptLoader("spell_fallen_protectors_calamity") { }

        class spell_fallen_protectors_calamity_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fallen_protectors_calamity_SpellScript);

            enum proc
            {
                SPELL_PROCK     = 143493,
            };

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        target->CastSpell(target, SPELL_PROCK, false);

                    if(caster->GetAI())
                        caster->GetAI()->SetData(DATA_CALAMITY_HIT, true);
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_fallen_protectors_calamity_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_fallen_protectors_calamity_SpellScript();
        }
};

//Mark of Anguish select target.
class spell_fallen_protectors_mark_of_anguish_select_first_target : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_mark_of_anguish_select_first_target() :  SpellScriptLoader("spell_fallen_protectors_mark_of_anguish_select_first_target") { }

        class spell_fallen_protectors_mark_of_anguish_select_first_target_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fallen_protectors_mark_of_anguish_select_first_target_SpellScript);

            void SelectTarget(std::list<WorldObject*>& unitList)
            {
                if (unitList.empty())
                    return;

                unitList.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                if (unitList.size() < 1)
                    return;

                unitList.resize(1);
            }

            void HandleOnHit()
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Unit* target = GetHitUnit();
                if (!target)
                    return;

                if(caster->GetAI())
                    caster->GetAI()->SetGUID(target->GetGUID(), true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_fallen_protectors_mark_of_anguish_select_first_target_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnHit += SpellHitFn(spell_fallen_protectors_mark_of_anguish_select_first_target_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_fallen_protectors_mark_of_anguish_select_first_target_SpellScript();
        }

};

//Shadow Weakness
class spell_fallen_protectors_shadow_weakness_prock : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_shadow_weakness_prock() : SpellScriptLoader("spell_fallen_protectors_shadow_weakness_prock") { }

        class spell_fallen_protectors_shadow_weakness_prock_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_fallen_protectors_shadow_weakness_prock_AuraScript);

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                if (!aurEff)
                    return;

                Unit* _caster = GetCaster();
                if (!_caster)
                    return;

                Unit* _target = eventInfo.GetProcTarget();
                if (!_target)
                    return;

                _caster->CastSpell(_target, SPELL_SHADOW_WEAKNESS, false);
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_fallen_protectors_shadow_weakness_prock_AuraScript::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_fallen_protectors_shadow_weakness_prock_AuraScript();
        }
};

//Mark of Anguish
class spell_fallen_protectors_mark_of_anguish : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_mark_of_anguish() : SpellScriptLoader("spell_fallen_protectors_mark_of_anguish") { }

        class spell_fallen_protectors_mark_of_anguish_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_fallen_protectors_mark_of_anguish_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                Unit* target = GetTarget();
                if (!target)
                    return;

                if(Unit* caster = GetCaster())
                {
                    target->CastSpell(target, SPELL_MARK_OF_ANGUISH_DAMAGE, true, NULL, NULL, caster->GetGUID());

                    //By normal way should prock from our proc system... but where is caster and target is channel target... so this is custom prock reason.
                    if (SpellInfo const* m_spellInfo = sSpellMgr->GetSpellInfo(SPELL_MARK_OF_ANGUISH_DAMAGE))
                    {
                        DamageInfo dmgInfoProc = DamageInfo(caster, target, 1, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE);
                        caster->ProcDamageAndSpell(target, PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG, 0, PROC_EX_NORMAL_HIT, &dmgInfoProc, BASE_ATTACK, m_spellInfo, aurEff->GetSpellInfo());
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_fallen_protectors_mark_of_anguish_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_fallen_protectors_mark_of_anguish_AuraScript();
        }
};

//SPELL_MARK_OF_ANGUISH_GIVE_A_FRIEND     = 143842,
class spell_fallen_protectors_mark_of_anguish_transfer : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_mark_of_anguish_transfer() : SpellScriptLoader("spell_fallen_protectors_mark_of_anguish_transfer") { }

        class spell_fallen_protectors_mark_of_anguish_transfer_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fallen_protectors_mark_of_anguish_transfer_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                InstanceScript* instance = caster->GetInstanceScript();
                if (!instance)
                    return;

                Unit* target = GetHitUnit();
                if (!target)
                    return;

                Creature* mesOfHe = instance->instance->GetCreature(instance->GetData64(NPC_EMBODIED_ANGUISH_OF_HE));
                if (!mesOfHe)
                    return;

                mesOfHe->CastSpell(mesOfHe, SPELL_SHADOW_WEAKNES_MASS, true);
                caster->RemoveAurasDueToSpell(SPELL_MARK_OF_ANGUISH_STAN);
                mesOfHe->GetAI()->SetGUID(target->GetGUID(), true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_fallen_protectors_mark_of_anguish_transfer_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_fallen_protectors_mark_of_anguish_transfer_SpellScript();
        }
};

//SPELL_INFERNO_STRIKE                    = 143962, //Inferno Strike
class spell_fallen_protectors_inferno_strike : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_inferno_strike() :  SpellScriptLoader("spell_fallen_protectors_inferno_strike") { }

        class spell_fallen_protectors_inferno_strike_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fallen_protectors_inferno_strike_SpellScript);

            void SelectTarget(std::list<WorldObject*>& unitList)
            {
                SpellValue const* val = GetSpellValue();
                if (!val || unitList.empty())
                    return;

                uint32 count = val->EffectBasePoints[EFFECT_0];
                unitList.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                if (unitList.size() < count)
                    return;

                unitList.resize(count);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_fallen_protectors_inferno_strike_SpellScript::SelectTarget, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_fallen_protectors_inferno_strike_SpellScript();
        }
};

class spell_fallen_protectors_defile_ground : public SpellScriptLoader
{
    public:
        spell_fallen_protectors_defile_ground() : SpellScriptLoader("spell_fallen_protectors_defile_ground") { }

        class spell_fallen_protectors_defile_ground_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fallen_protectors_defile_ground_SpellScript);
 
            enum data
            {
                AT_ENTRY    = 4906,
            };
            void HandleTriggerEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_1);

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Unit* target = GetExplTargetUnit();
                if (!target)
                    return;

                AreaTrigger * areaTrigger = new AreaTrigger;
                if (!areaTrigger->CreateAreaTrigger(sObjectMgr->GenerateLowGuid(HIGHGUID_AREATRIGGER), AT_ENTRY, caster, GetSpellInfo(), *target, GetSpell()))
                {

                    delete areaTrigger;
                    return;
                }
                areaTrigger->SetSpellId(GetSpellInfo()->Effects[EFFECT_1].TriggerSpell);
            }
            
            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_fallen_protectors_defile_ground_SpellScript::HandleTriggerEffect, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
            }
        };
        
        SpellScript* GetSpellScript() const
        {
            return new spell_fallen_protectors_defile_ground_SpellScript();
        }
};

void AddSC_boss_fallen_protectors()
{
    new boss_rook_stonetoe();
    new boss_he_softfoot();
    new boss_sun_tenderheart();
    new npc_golden_lotus_control();
    new vehicle_golden_lotus_conteiner();
    new npc_measure_of_sun();
    new npc_measure_of_he();
    new npc_measure_of_rook();
    new spell_clash();
    new spell_corrupted_brew();
    new spell_gouge();
    new spell_dark_meditation();
    new spell_fallen_protectors_shadow_word_bane();
    new spell_fallen_protectors_calamity();
    new spell_fallen_protectors_mark_of_anguish_select_first_target();
    new spell_fallen_protectors_shadow_weakness_prock();
    new spell_fallen_protectors_mark_of_anguish();
    new spell_fallen_protectors_mark_of_anguish_transfer();
    new spell_fallen_protectors_inferno_strike();
    new spell_fallen_protectors_defile_ground();
}