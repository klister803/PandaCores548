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
#include "ScriptedEscortAI.h"

enum eSpells
{
    SPELL_VISUAL_TELEPORT           = 149634,
    SPELL_VISUAL_TELEPORT_AC        = 145188,
    SPELL_EXTRACT_CORRUPTION        = 145143,
    SPELL_EXTRACT_CORRUPTION_S      = 145149,

    //Amalgam_of_Corruption
    SPELL_SPAWN_AMALGAM             = 145118,
    SPELL_CORRUPTION                = 144421, 
    SPELL_ICY_FEAR                  = 145733,
    SPELL_UNLEASHED_ANGER           = 145214,
    SPELL_UNLEASHED_ANGER_DMG       = 145212,
    SPELL_UNCHECKED_CORRUPTION      = 145679,
    SPELL_SELF_DOUBT                = 146124,
    SPELL_QUARANTINE_SAFETY         = 145779,
    SPELL_FRAYED                    = 146179,
    SPELL_UNLEASH_CORRUPTION        = 145769,

    //Blind Hatred
    SPELL_BLIND_HATRED              = 145571,
    SPELL_BLIND_HATRED_V            = 145226,
    SPELL_BLIND_HATRED_D            = 145227,

    // Frayed. Manifestation of corruption.
    SPELL_BURST_OF_ANGER            = 147082,

    //RESIDUAL_CORRUPTION
    SPELL_CORRUPTION_AREA           = 145052,

    //Manifestation of Corruption
    SPELL_TEAR_REALITY              = 144482,
    SPELL_RESIDUAL_CORRUPTION       = 145074,

    //Essence of Corruption
    SPELL_EXPEL_CORRUPTION_AT       = 144548, //Create areatrigger
    SPELL_EXPELLED_CORRUPTION       = 144480,

    // TC
    SPELL_TC_CLEANSE                = 147657,

    /*   C H A L L E N GE*/
    SPELL_PURIFIED_CHALLENGE        = 146022,
    SPELL_PURIFIED                  = 144452,

    //Test for players
    SPELL_TEST_OF_SERENITY          = 144849, //dd
    SPELL_TEST_OF_RELIANCE          = 144850, //heal
    SPELL_TEST_OF_CONFIDENCE        = 144851, //tank

    //Phase spells
    SPELL_LOOK_WITHIN_DD            = 146837,
    SPELL_LOOK_WITHIN_HEALER        = 144724,
    SPELL_LOOK_WITHIN_TANK          = 144727, //Look Within

    //DD
    SPELL_SPAWN_VICTORY_ORB_DD_BIG  = 144491, //Spawn Victory Orb summon NPC_MANIFESTATION_OF_CORRUPTION in world
    SPELL_SPAWN_VICTORY_ORB_DD_SML  = 145006/*144490*/, //Spawn Victory Orb summon NPC_ESSENCE_OF_CORRUPTION in world

    SPELL_SUM_ESSENCE_OF_CORRUPT_C  = 144733, //Summon NPC_ESSENCE_OF_CORRUPTION_C by Player
    SPELL_SUM_MANIFESTATION_OF_C    = 144739, //Summon NPC_MANIFESTATION_OF_CORRUPTION_C by Player

    //Tank
    SPELL_TITANIC_CORRUPTION        = 144848, //Titanic Corruption

    //Healer
    SPELL_GREATER_CORRUPTION        = 144980, //Greater Corruption
    SPELL_MELEE_COMBTANT            = 144975, //Melee Combatant
    SPELL_CASTER                    = 144977,
    SPELL_SUMMON_GUARDIAN           = 144973, //Summon Guardian
    SPELL_PROTECTORS_EXHAUSTED      = 148424,
    SPELL_PROTECTORS_DD             = 144521,
};

enum sEvents
{
    //Amalgam_of_Corruption
    EVENT_CHECK_VICTIM         = 1,
    EVENT_QUARANTINE_SAFETY    = 2, 
    EVENT_UNLEASHED_ANGER      = 3,
    EVENT_BLIND_HATRED         = 4,

    //Blind Hatred
    EVENT_GET_NEW_POS          = 5,

    //Manifestation of Corruption
    EVENT_TEAR_REALITY         = 6,

    //Titanic Corruption
    EVENT_BURST_OF_CORRUPTION  = 7,
    EVENT_CORRUPTION_TC        = 8,
    EVENT_HURL_CORRUPTION      = 9,
    EVENT_PIERCING_CORRUPTION  = 10,
    EVENT_TITANIC_SMASH        = 11,

    //Essence of Corruption
    EVENT_EXPELLED_CORRUPTION  = 12,
    
    EVENT_RE_ATTACK            = 13,
};

enum sData
{
    //Blind Hatred
    DATA_START_MOVING          = 1,
    DATA_FILL_MOVE_ORDER       = 2,
};

enum sAction
{
    //Blind Hatred
    ACTION_START_EVENT         = 1,
};

Position const plspos[5] =  //purifying light spawn pos
{
    {805.18f, 956.49f, 356.3400f},
    {760.49f, 946.19f, 356.3398f},
    {746.27f, 985.05f, 356.3398f},
    {771.05f, 1006.36f, 356.8000f},
    {805.67f, 991.16f, 356.3400f},
};

//Blind Hatred
Position const BlindHatred[4] =
{
    { 808.897f, 1023.77f, 356.3f}, //A
    { 728.585f, 1006.259f,356.3f}, //B
    { 748.132f, 911.165f, 356.3f}, //C
    { 828.936f, 929.101f, 356.3f}, //D
};                

typedef std::list<uint8> BlindOrderList;

void GenerateOrder(BlindOrderList &m)
{
    uint8 c = urand(0, 3);
    bool decrase = urand(0, 1);
    for(int8 i = 0; i < 4; ++i)
    {
        if (c > 3)      c = 0;

        if (decrase)    m.push_back(c);
        else            m.push_front(c);

        ++c;
    }
}

Position GetFirstRandPoin(BlindOrderList &m)
{
    BlindOrderList::iterator itr = m.begin();
    uint8 A = *itr;
    ++itr;
    uint8 B = *itr;
    return BlindHatred[A].GetRandPointBetween(BlindHatred[B]);
}

float const radius = 38.0f;

Position const Norushen  = {767.6754f, 1015.564f, 356.1747f, 4.922687f };
Position const Amalgan  = {777.3924f, 974.2292f, 356.3398f, 1.786108f };

void CheckCorruptionForCleanse(Player* p)
{
    if (!p->HasAura(SPELL_CORRUPTION) || p->HasAura(SPELL_PURIFIED))
        return;

    if (!p->GetPower(POWER_ALTERNATE_POWER))
        p->CastSpell(p, SPELL_PURIFIED);
}

//If plr take corruption with purifie remove it.
void onAddCorruption(Player* p)
{
    if (p->HasAura(SPELL_PURIFIED))
        p->RemoveAurasDueToSpell(SPELL_PURIFIED);
}

class boss_norushen : public CreatureScript
{
    public:
        boss_norushen() : CreatureScript("boss_norushen") {}

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        { 
            player->CLOSE_GOSSIP_MENU();
            if (action)
                creature->AI()->DoAction(true);

            return true;
        }

        struct boss_norushenAI : public ScriptedAI
        {
            boss_norushenAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                //me->SetUInt32Value(UNIT_NPC_FLAGS, 1);
            }

            void DoAction(int32 const action)
            {
                me->SetUInt32Value(UNIT_NPC_FLAGS, 0);
                //continue from EVENT_13
                uint32 t = 0;
                events.ScheduleEvent(EVENT_1, t += 0);          //18:23:14.000
                events.ScheduleEvent(EVENT_2, t += 8000);       //18:23:22.000
                events.ScheduleEvent(EVENT_3, t += 11000);      //18:23:32.000
                events.ScheduleEvent(EVENT_4, t += 2000);       //18:23:34.000
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_1:
                            me->CastSpell(me, SPELL_VISUAL_TELEPORT, false);
                            me->CastSpell(me, SPELL_VISUAL_TELEPORT_AC, true);
                            ZoneTalk(eventId + 6, me->GetGUID());
                            break;
                        case EVENT_2:
                            me->SetFacingTo(1.791488f);
                            instance->SetBossState(DATA_NORUSHEN, IN_PROGRESS);
                            ZoneTalk(eventId + 6, me->GetGUID());
                            me->CastSpell(Amalgan.GetPositionX(), Amalgan.GetPositionY(), Amalgan.GetPositionZ(), SPELL_EXTRACT_CORRUPTION);
                            break;
                        case EVENT_3:
                            me->CastSpell(Amalgan.GetPositionX(), Amalgan.GetPositionY(), Amalgan.GetPositionZ(), SPELL_EXTRACT_CORRUPTION_S);
                            break;
                        case EVENT_4:
                            ZoneTalk(TEXT_GENERIC_9, me->GetGUID());
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_norushenAI(creature);
        }
};

class npc_norushen_lowerwalker : public CreatureScript
{
public:
    npc_norushen_lowerwalker() : CreatureScript("npc_norushen_lowerwalker") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_norushen_lowerwalkerAI (creature);
    }

    enum phases
    {
        PHASE_EVENT     = 1,
    };

    struct npc_norushen_lowerwalkerAI : public npc_escortAI
    {
        npc_norushen_lowerwalkerAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        uint64 norushGUID;

        void Reset()
        {
            norushGUID = 0;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (events.IsInPhase(PHASE_EVENT))
                return;

            Start(false, false);
            events.SetPhase(PHASE_EVENT);
            uint32 t = 0;
            events.ScheduleEvent(EVENT_1, t += 1000);    //18:20:50.000
            events.ScheduleEvent(EVENT_2, t += 6000);    //18:20:56.000
            events.ScheduleEvent(EVENT_3, t += 8000);    //18:21:04.000
            events.ScheduleEvent(EVENT_4, t += 8000);    //18:21:11.000
            events.ScheduleEvent(EVENT_5, t += 2000);    //18:21:13.000
            events.ScheduleEvent(EVENT_6, t += 7000);    //18:21:20.000
            events.ScheduleEvent(EVENT_7, t += 7000);    //18:21:27.000
            events.ScheduleEvent(EVENT_8, t += 8000);    //18:21:35.000
            events.ScheduleEvent(EVENT_9, t += 5000);    //18:21:40.000
            events.ScheduleEvent(EVENT_10, t += 3000);   //18:21:43.000
            events.ScheduleEvent(EVENT_11, t += 14000);  //18:21:56.000
            events.ScheduleEvent(EVENT_12, t += 8000);   //18:22:04.000
            events.ScheduleEvent(EVENT_13, t += 10000);  //18:22:14.000
            events.ScheduleEvent(EVENT_14, t += 1);
        }

        void WaypointReached(uint32 i)
        {
            if (i == 2)
                SetEscortPaused(true);
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                    case EVENT_2:
                    case EVENT_3:
                        ZoneTalk(eventId - 1, me->GetGUID());
                        break;                    
                    case EVENT_4:
                        if (Creature* norush = instance->instance->SummonCreature(NPC_NORUSHEN, Norushen))
                        {
                            norush->AI()->ZoneTalk(TEXT_GENERIC_0, me->GetGUID());
                            norushGUID = norush->GetGUID();
                        }
                        break;
                    case EVENT_5:
                        ZoneTalk(TEXT_GENERIC_3, me->GetGUID());
                        break;
                    case EVENT_6:
                        if (Creature* norush = instance->instance->GetCreature(norushGUID))
                            norush->AI()->ZoneTalk(TEXT_GENERIC_1, me->GetGUID());
                        break;
                    case EVENT_7:
                        ZoneTalk(TEXT_GENERIC_4, me->GetGUID());
                        break;
                    case EVENT_8:
                        if (Creature* norush = instance->instance->GetCreature(norushGUID))
                            norush->AI()->ZoneTalk(TEXT_GENERIC_2, me->GetGUID());
                        break;
                    case EVENT_9:
                        ZoneTalk(TEXT_GENERIC_5, me->GetGUID());
                        break;
                    case EVENT_10:
                    case EVENT_11:
                    case EVENT_12:
                    case EVENT_13:
                        if (Creature* norush = instance->instance->GetCreature(norushGUID))
                            norush->AI()->ZoneTalk(eventId - 7, me->GetGUID());
                        break;
                    case EVENT_14:
                        if (Creature* norush = instance->instance->GetCreature(norushGUID))
                            norush->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                }
            }
        }
    };
};

class boss_amalgam_of_corruption : public CreatureScript
{
    public:
        boss_amalgam_of_corruption() : CreatureScript("boss_amalgam_of_corruption") {}

        struct boss_amalgam_of_corruptionAI : public BossAI
        {
            boss_amalgam_of_corruptionAI(Creature* creature) : BossAI(creature, DATA_NORUSHEN)
            {
                instance = creature->GetInstanceScript();
                SetCombatMovement(false);
            }

            InstanceScript* instance;
            uint8 FrayedCounter;
            std::map<uint32, uint32> challengeCounter;

            void Reset()
            {
                _Reset();
                
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PURIFIED);

                me->RemoveAurasDueToSpell(SPELL_FRAYED);
                me->RemoveAurasDueToSpell(SPELL_ICY_FEAR);
                me->SetReactState(REACT_DEFENSIVE);
                me->ModifyAuraState(AURA_STATE_UNKNOWN22, true);
                ApplyOrRemoveBar(false);
                FrayedCounter = 0;
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                challengeCounter.clear();
            }

            //
            void SetGUID(uint64 guid, int32 id)
            {
                if (id > 0)
                    ++challengeCounter[uint32(guid)];
                else
                    --challengeCounter[uint32(guid)];
            }

            uint32 GetData(uint32 guid)
            {  
                return challengeCounter[guid];
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetInCombatWithZone();
                //me->CastSpell(me, SPELL_SPAWN_AMALGAM, true);
            }

            void ApplyOrRemoveBar(bool state)
            {
                Map* pMap = me->GetMap();
                if (!pMap || !pMap->IsDungeon())
                    return;

                Map::PlayerList const &players = pMap->GetPlayers();
                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                {
                    if (Player* pl = i->getSource())
                    {
                        if (pl->isAlive() && state)
                            pl->AddAura(SPELL_CORRUPTION, pl);
                        else
                            pl->RemoveAurasDueToSpell(SPELL_CORRUPTION);
                    }
                }
            }

            void EnterCombat(Unit* who)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                _EnterCombat();
                me->AddAura(SPELL_ICY_FEAR, me);
                ApplyOrRemoveBar(true);
                events.ScheduleEvent(EVENT_CHECK_VICTIM, 2000);
                events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);     //18:23:51.000 - 18:24:02.000
                events.ScheduleEvent(EVENT_QUARANTINE_SAFETY, 420000);
                events.ScheduleEvent(EVENT_BLIND_HATRED, 12000);
                me->CastSpell(me, SPELL_SPAWN_AMALGAM, true);

                //Summon Purifying light.
                for(uint8 i = 0; i < 5; ++i)
                    me->SummonCreature(NPC_PURIFYING_LIGHT, plspos[i].GetPositionX(), plspos[i].GetPositionY(), plspos[i].GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN); 
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (!attacker->HasAura(SPELL_CORRUPTION))
                {
                    damage = 0;
                    return;
                }

                //deal less damage if plr have corruptuin. Default value - 75%
                damage = CalculatePct(damage, 100 - attacker->GetPower(POWER_ALTERNATE_POWER));

                //Frayed summon manifestation of corruption every 10% after 50 pct.
                if (HealthBelowPct(50) && damage < me->GetHealth())
                {
                    if (HealthBelowPct(50 - 10*FrayedCounter))
                    {
                        if (!me->HasAura(SPELL_FRAYED))
                            me->AddAura(SPELL_FRAYED, me);
                        ++FrayedCounter;
                        DoCast(me, SPELL_UNLEASH_CORRUPTION);
                    }
                }
            }

            void DoAction(int32 const action)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                ApplyOrRemoveBar(false);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_VICTIM:
                            if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                                DoCastAOE(SPELL_UNCHECKED_CORRUPTION);
                            events.ScheduleEvent(EVENT_CHECK_VICTIM, 2000);
                            break;
                        case EVENT_QUARANTINE_SAFETY:
                            DoCastAOE(SPELL_QUARANTINE_SAFETY);
                            break;
                        case EVENT_UNLEASHED_ANGER:
                            if (me->getVictim())
                                DoCast(me->getVictim(), SPELL_UNLEASHED_ANGER);
                            events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);
                            break;
                        case EVENT_BLIND_HATRED:
                        {
                            DoCast(me, SPELL_BLIND_HATRED, false);
                            events.ScheduleEvent(EVENT_BLIND_HATRED, 40000);
                            break;
                        }
                    }

                }
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_amalgam_of_corruptionAI(creature);
        }
};

//72565 
class npc_blind_hatred : public CreatureScript
{
public:
    npc_blind_hatred() : CreatureScript("npc_blind_hatred") { }

    struct npc_blind_hatredAI : public ScriptedAI
    {
        npc_blind_hatredAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
        }

        InstanceScript* pInstance;
        EventMap events;
        BlindOrderList order;

        void Reset()
        {
            events.Reset();
        }

        void EnterEvadeMode(){}

        void EnterCombat(Unit* who){}

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_FILL_MOVE_ORDER)
                order.push_back(data);

            if (type == DATA_START_MOVING)
            {
                order.pop_front();  //remove first point as it was source rand generation.
                Position p = BlindHatred[*order.begin()];
                me->GetMotionMaster()->MoveCharge(p.GetPositionX(), p.GetPositionY(), me->GetPositionZ(), 5.0f, 0);
                order.pop_front();  //remove secont point as we just move from it.
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE && !order.empty())
                events.ScheduleEvent(DATA_START_MOVING, 1);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                Position p = BlindHatred[*order.begin()];
                me->GetMotionMaster()->MoveCharge(p.GetPositionX(), p.GetPositionY(), me->GetPositionZ(), 5.0f, 0);
                order.pop_front();  //remove secont point as we just move from it.
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blind_hatredAI(pCreature);
    }
};


class npc_norushen_purifying_light : public CreatureScript
{
public:
    npc_norushen_purifying_light() : CreatureScript("npc_norushen_purifying_light") { }

    struct npc_norushen_purifying_lightAI : public CreatureAI
    {
        npc_norushen_purifying_lightAI(Creature* pCreature) : CreatureAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;

        void Reset()
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void OnSpellClick(Unit* clicker)
        {
            Player* p = clicker->ToPlayer();
            if (!p)
                return;

            uint8 role = p->GetRoleForGroup(p->GetSpecializationId(p->GetActiveSpec()));
            switch(role)
            {
                case ROLES_TANK:
                    p->CastSpell(p, SPELL_TEST_OF_CONFIDENCE, false);
                    break;
                case ROLES_DPS:
                    p->CastSpell(p, SPELL_TEST_OF_SERENITY, false);
                    break;
                case ROLES_HEALER:
                    p->CastSpell(p, SPELL_TEST_OF_RELIANCE, false);
                    break;
                default:
                    p->CastSpell(p, SPELL_TEST_OF_SERENITY, false);
                    sLog->outError(LOG_FILTER_PLAYER, "Script::npc_norushen_purifying_light: Player %s has not localized role specID.", p->ToString().c_str(), role);
                    break;
            }
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}     
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_purifying_lightAI(pCreature);
    }
};

//72550
class npc_norushen_residual_corruption : public CreatureScript
{
public:
    npc_norushen_residual_corruption() : CreatureScript("npc_norushen_residual_corruption") { }

    struct npc_norushen_residual_corruptionAI : public ScriptedAI
    {
        npc_norushen_residual_corruptionAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
        }

        void Reset() { }

        void MoveInLineOfSight(Unit* target)
        {
            if (target->GetTypeId() != TYPEID_PLAYER ||
                me->GetDistance(target) > 1.0f ||
                !target->HasAura(SPELL_CORRUPTION))
                return;

            uint32 power = target->GetPower(POWER_ALTERNATE_POWER) + 25;
            if (power > 100)
                return;

            onAddCorruption(target->ToPlayer());
            target->SetPower(POWER_ALTERNATE_POWER, power);
            me->DespawnOrUnsummon();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_norushen_residual_corruptionAI(creature);
    }
};

struct npc_norushenChallengeAI : public ScriptedAI
{
    npc_norushenChallengeAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        instance = (InstanceScript*)pCreature->GetInstanceScript();
        switch(pCreature->GetEntry())
        {
            case NPC_ESSENCE_OF_CORRUPTION_C:
            case NPC_MANIFESTATION_OF_CORRUPTION_C:
                challenge = true;
                break;
            default:
                break;
        }
        cleanseSpellID = 0;
    }

    uint32 cleanseSpellID;
    InstanceScript* instance;
    EventMap events;
    bool challenge;

    void Reset()
    {
    }

    void IsSummonedBy(Unit* summoner)
    {
        if (!instance)
            return;

        me->SetPhaseId(summoner->GetGUID(), true);
        me->AddPlayerInPersonnalVisibilityList(summoner->GetGUID());

        if (challenge)
        {
            events.RescheduleEvent(EVENT_1, 60000);
            if (Creature* amalgam = instance->instance->GetCreature(instance->GetData64(NPC_AMALGAM_OF_CORRUPTION)))
                amalgam->AI()->SetGUID(summoner->GetGUID(), 1);
        }
        AttackStart(summoner);
    }

    void EnterEvadeMode()
    {
        if (Creature* amalgam = instance->instance->GetCreature(instance->GetData64(NPC_AMALGAM_OF_CORRUPTION)))
        {
            summonInRealWorld(amalgam);

            if (TempSummon const* sum = me->ToTempSummon())
            {
                amalgam->AI()->SetGUID(sum->GetSummonerGUID(), -1);
            }
        }
        me->DespawnOrUnsummon();
    }

    void JustDied(Unit* killer)
    {
        if (!challenge)
            return;

        Player* plr = killer->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (!plr)
            return;

        if (cleanseSpellID)
            plr->CastSpell(plr, cleanseSpellID, false);
        CheckCorruptionForCleanse(plr);

        if (Creature* amalgam = instance->instance->GetCreature(instance->GetData64(NPC_AMALGAM_OF_CORRUPTION)))
        {
            summonInRealWorld(amalgam);
            amalgam->AI()->SetGUID(plr->GetGUID(), -1);
            if (!amalgam->AI()->GetData(plr->GetGUID()))
                plr->RemoveAurasDueToSpell(SPELL_TEST_OF_SERENITY);
        }
        me->DespawnOrUnsummon();
    }

    void summonInRealWorld(Creature* amalgan)
    {
        switch(me->GetEntry())
        {
            case NPC_MANIFESTATION_OF_CORRUPTION_C:
                amalgan->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_SPAWN_VICTORY_ORB_DD_BIG);
                break;
            case NPC_ESSENCE_OF_CORRUPTION_C:
            {
                amalgan->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_SPAWN_VICTORY_ORB_DD_SML);
                break;
            }
        }
    }

    void ActionEvent(uint32 _event)
    {
        switch(_event)
        {
            case EVENT_1:
                if (Creature* amalgam = instance->instance->GetCreature(instance->GetData64(NPC_AMALGAM_OF_CORRUPTION)))
                    summonInRealWorld(amalgam);
                me->DespawnOrUnsummon();
                break;
        }
    }
};

//71976 for dd
class npc_essence_of_corruption_challenge : public CreatureScript
{
public:
    npc_essence_of_corruption_challenge() : CreatureScript("npc_essence_of_corruption_challenge") { }

    struct npc_essence_of_corruptionAI : public npc_norushenChallengeAI
    {
        npc_essence_of_corruptionAI(Creature* pCreature) : npc_norushenChallengeAI(pCreature)
        {
            cleanseSpellID = SPELL_CLEANSE;
            SetCombatMovement(false);
        }
       
        enum spell
        {
            SPELL_ESSENCE_OF_CORUPTION          = 148452,   //Essence of Corruption
            SPELL_STEALTH_DETECTION             = 8279,     //Stealth Detection
            SPELL_EXPEL_CORRUPTUIN              = 144479,   //Expel Corruption
            SPELL_EXPELED_CORRUPTION            = 144480,
            SPELL_CLEANSE                       = 144449,
        };

        void IsSummonedBy(Unit* summoner)
        {
            events.RescheduleEvent(EVENT_2, urand(3000, 6000));
            me->CastSpell(me, SPELL_ESSENCE_OF_CORUPTION, false);
            me->CastSpell(me, SPELL_STEALTH_DETECTION, true);
            npc_norushenChallengeAI::IsSummonedBy(summoner);
        }

        void UpdateAI(uint32 diff)
        {
            UpdateVictim();

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_2:
                        DoCastVictim(SPELL_EXPEL_CORRUPTUIN);
                        events.RescheduleEvent(EVENT_2, 9*IN_MILLISECONDS);
                        break;
                }
                ActionEvent(eventId);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_essence_of_corruptionAI(pCreature);
    }
};

//71977 for dd
class npc_norushen_manifestation_of_corruption_challenge : public CreatureScript
{
public:
    npc_norushen_manifestation_of_corruption_challenge() : CreatureScript("npc_norushen_manifestation_of_corruption_challenge") { }

    struct npc_norushen_manifestation_of_corruption_challengeAI : public npc_norushenChallengeAI
    {
        npc_norushen_manifestation_of_corruption_challengeAI(Creature* pCreature) : npc_norushenChallengeAI(pCreature)
        {
            cleanseSpellID = SPELL_CLEANSE;
        }

        enum spells
        {
            SPELL_STEALTH_AND_INVISIBILITY_DETECT        = 141048,
            SPELL_CLEANSE                                = 144450,
        };

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_TEAR_REALITY, 8500);
        }

        void IsSummonedBy(Unit* summoner)
        {
            //events.RescheduleEvent(EVENT_2, 3000);
            me->CastSpell(me, SPELL_STEALTH_AND_INVISIBILITY_DETECT, true);
            npc_norushenChallengeAI::IsSummonedBy(summoner);
        }

        void UpdateAI(uint32 diff)
        {
            UpdateVictim();

            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {   
                switch (eventId)
                {
                    case EVENT_TEAR_REALITY:
                        me->AttackStop();
                        DoCastAOE(SPELL_TEAR_REALITY);
                        events.ScheduleEvent(EVENT_TEAR_REALITY, 8500);
                        break;
                }
                ActionEvent(eventId);
            }
            if (!me->HasUnitState(UNIT_STATE_CASTING))
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_manifestation_of_corruption_challengeAI(pCreature);
    }
};

//72264
class npc_norushen_manifestation_of_corruption_released : public CreatureScript
{
public:
    npc_norushen_manifestation_of_corruption_released() : CreatureScript("npc_norushen_manifestation_of_corruption_released") { }

    struct npc_norushen_manifestation_of_corruption_releasedAI : public ScriptedAI
    {
        npc_norushen_manifestation_of_corruption_releasedAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;

        enum sp
        {
            SPELL_UNLEASHED                              = 146173,
        };

        void Reset()
        {
            events.Reset();

            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY, 100);
            me->SetPower(POWER_ENERGY, 100);
        }
        
        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_1, 5000);
        }

        void IsSummonedBy(Unit* /*summoner*/)
        {
            me->CastSpell(me, SPELL_UNLEASHED, false);
            me->SetInCombatWithZone();
        }

        void JustDied(Unit* killer)
        {
            //Summon NPC_RESIDUAL_CORRUPTION by not existent spell 145522
            Unit* owner = me->GetAnyOwner();
            if (!owner)
                return;

            if (Creature* rc = owner->SummonCreature(NPC_RESIDUAL_CORRUPTION, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
            {
                rc->CastSpell(rc, 145074, true);
            }
        }
       
        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {   
                switch (eventId)
                {
                    case EVENT_1:
                        DoCastAOE(SPELL_BURST_OF_ANGER);
                        events.ScheduleEvent(EVENT_1, 5000);
                        break;
                    default:
                        break;
                }
            }
            if (!me->HasUnitState(UNIT_STATE_CASTING))
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_manifestation_of_corruption_releasedAI(pCreature);
    }
};

class npc_essence_of_corruption_released : public CreatureScript
{
public:
    npc_essence_of_corruption_released() : CreatureScript("npc_essence_of_corruption_released") { }

    struct npc_essence_of_corruption_releasedAI : public ScriptedAI
    {
        npc_essence_of_corruption_releasedAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            instance = (InstanceScript*)pCreature->GetInstanceScript();
            SetCombatMovement(false);
        }
       
        enum spell
        {
            SPELL_UNLEASHED                     = 146174,
            SPELL_STEALTH_DETECTION             = 8279,     //Stealth Detection
            SPELL_EXPEL_CORRUPTION              = 145064,   //145132 on friend | 145134 on enemy
        };

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            //events.Reset();
        }

        void IsSummonedBy(Unit* /*summoner*/)
        {
            me->CastSpell(me, SPELL_UNLEASHED, false);          //18:38:25.000 
            me->CastSpell(me, SPELL_STEALTH_DETECTION, false);

            if (Creature* amalgam = instance->instance->GetCreature(instance->GetData64(NPC_AMALGAM_OF_CORRUPTION)))
                me->SetFacingToObject(amalgam);

            me->CastSpell(me, SPELL_EXPEL_CORRUPTION, true);
            events.RescheduleEvent(EVENT_1, 6*IN_MILLISECONDS);                  //18:38:25.000
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        if (Creature* amalgam = instance->instance->GetCreature(instance->GetData64(NPC_AMALGAM_OF_CORRUPTION)))
                            me->SetFacingToObject(amalgam);
                         me->CastSpell(me, SPELL_EXPEL_CORRUPTION, false);
                        events.RescheduleEvent(EVENT_1, 6*IN_MILLISECONDS);
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_essence_of_corruption_releasedAI(pCreature);
    }
};

//72051 for tank
class npc_titanic_corruption : public CreatureScript
{
public:
    npc_titanic_corruption() : CreatureScript("npc_titanic_corruption") { }

    struct npc_titanic_corruptionAI : public ScriptedAI
    {
        npc_titanic_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            instance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        enum spells
        {
            SPELL_BURST_OF_CORRUPTION       = 144654,
            SPELL_CORRUPTION_TC             = 144639,
            SPELL_HURL_CORRUPTION           = 144649,
            SPELL_PIERCING_CORRUPTION       = 144657,
            SPELL_TITANIC_SMASH             = 144628,
        };

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        void IsSummonedBy(Unit* summoner)
        {
            if (!instance)
                return;

            me->SetPhaseId(summoner->GetGUID(), true);
            me->AddPlayerInPersonnalVisibilityList(summoner->GetGUID());
            me->DespawnOrUnsummon(60000);
            me->SetInCombatWithZone();
        }

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_PIERCING_CORRUPTION, 14000);
            events.ScheduleEvent(EVENT_TITANIC_SMASH, 16000);
            events.ScheduleEvent(EVENT_HURL_CORRUPTION, 20000);
            events.ScheduleEvent(EVENT_BURST_OF_CORRUPTION, 35000);
        }
        
        void UpdateAI(uint32 diff)
        {
            UpdateVictim();

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            { 
                switch (eventId)
                {
                    case EVENT_PIERCING_CORRUPTION:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_PIERCING_CORRUPTION);
                        events.ScheduleEvent(EVENT_PIERCING_CORRUPTION, 14000);
                        break;
                    case EVENT_TITANIC_SMASH:
                        me->AttackStop();
                        me->SetReactState(REACT_PASSIVE);
                        DoCastAOE(SPELL_TITANIC_SMASH);
                        events.ScheduleEvent(EVENT_RE_ATTACK, 1000);
                        events.ScheduleEvent(EVENT_TITANIC_SMASH, 16000);
                        break;
                    case EVENT_HURL_CORRUPTION:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_HURL_CORRUPTION);
                        events.ScheduleEvent(EVENT_HURL_CORRUPTION, 20000);
                        break;
                    case EVENT_BURST_OF_CORRUPTION:
                        DoCastAOE(SPELL_BURST_OF_CORRUPTION);
                        events.ScheduleEvent(EVENT_BURST_OF_CORRUPTION, 35000);
                        break;
                    case EVENT_RE_ATTACK:
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 75.0f);
                        break;
                }
            }

            if (!me->HasUnitState(UNIT_STATE_CASTING))
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_titanic_corruptionAI(pCreature);
    }
};

//72001 for healers
class npc_norushen_heal_ch_greater_corruption : public CreatureScript
{
public:
    npc_norushen_heal_ch_greater_corruption() : CreatureScript("npc_norushen_heal_ch_greater_corruption") { }

    struct npc_norushen_heal_ch_greater_corruptionAI : public ScriptedAI
    {
        npc_norushen_heal_ch_greater_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        enum spells
        {
            SPELL_BOTTOMLESS_PIT             = 146705,
            SPELL_DISHEARTENING_LAUGH        = 146707,
            SPELL_LINGERING_CORRUPTION       = 144514,
            SPELL_CLEANSE                    = 147657,
        };

        enum events
        {
            EVENT_SPELL_BOTTOMLESS_PIT       = 1,
            EVENT_SPELL_DISHEARTENING_LAUGH  = 2,
            EVENT_SPELL_LINGERING_CORRUPTION = 3,
            EVENT_END                        = 4,
        };

        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        void IsSummonedBy(Unit* summoner)
        {
            me->SetPhaseId(summoner->GetGUID(), true);
            me->AddPlayerInPersonnalVisibilityList(summoner->GetGUID());
        }

        void EnterCombat(Unit* who)
        {
            //20:10:05.000
            events.ScheduleEvent(EVENT_SPELL_BOTTOMLESS_PIT, 17000);         //20:10:22.000
            events.ScheduleEvent(EVENT_SPELL_DISHEARTENING_LAUGH, 12000);    //20:10:17.000 20:10:29.000
            events.ScheduleEvent(EVENT_SPELL_LINGERING_CORRUPTION, 14000);   //20:10:19.000
            if (me->GetPhaseId()) events.ScheduleEvent(EVENT_END, 60000);
        }

        void JustDied(Unit* killer)
        {
            Unit* owner = me->GetAnyOwner();
            if (!owner)
                return;

            Player * plr = owner->ToPlayer();
            if (!plr)
                return;

            if (!me->GetPhaseId())
            {
                me->DespawnOrUnsummon();
                return;
            }

            plr->CastSpell(plr, SPELL_CLEANSE, false);
            CheckCorruptionForCleanse(plr);
            plr->RemoveAurasDueToSpell(SPELL_TEST_OF_RELIANCE);
            me->DespawnOrUnsummon();
        }

        void spawnOnRealWorld()
        {
            me->SetPhaseId(0, true);
            me->SetInCombatWithZone();
            me->SetFullHealth();
        }

        void EnterEvadeMode()
        {
            if (me->GetPhaseId())
                spawnOnRealWorld();
        }

        void UpdateAI(uint32 diff)
        {
            UpdateVictim();
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SPELL_BOTTOMLESS_PIT:
                        DoCast(me, SPELL_BOTTOMLESS_PIT);
                        events.ScheduleEvent(EVENT_SPELL_BOTTOMLESS_PIT, 17000);
                        break;
                    case EVENT_SPELL_DISHEARTENING_LAUGH:
                        DoCast(me, SPELL_DISHEARTENING_LAUGH);
                        events.ScheduleEvent(EVENT_SPELL_DISHEARTENING_LAUGH, 12000);
                        break;
                    case EVENT_SPELL_LINGERING_CORRUPTION:
                        DoCastVictim(SPELL_LINGERING_CORRUPTION);
                        events.ScheduleEvent(EVENT_SPELL_LINGERING_CORRUPTION, 14000);
                        break;
                    case EVENT_END:
                        spawnOnRealWorld();
                        //me->DespawnOrUnsummon();
                        break;
                }
            }
            if (!me->HasUnitState(UNIT_STATE_CASTING))
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_heal_ch_greater_corruptionAI(pCreature);
    }
};

struct npc_norushen_heal_chAI : public ScriptedAI
{
    npc_norushen_heal_chAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
    }

    EventMap events;

    void Reset()
    {
        events.Reset();
    }

    void DamageTaken(Unit* attacker, uint32 &damage)
    {
        if (damage >= me->GetHealth())
        {
            if (!me->HasAura(SPELL_PROTECTORS_EXHAUSTED))
                DoCast(me, SPELL_PROTECTORS_EXHAUSTED);
            damage = 0;
        }
    }

    void EnterCombat(Unit* who)
    {

    }

    void HealReceived(Unit* /*done_by*/, uint32& addhealth)
    {
        float newpct = GetHealthPctWithHeal(addhealth);
        if (newpct > 30.0f && me->HasAura(SPELL_PROTECTORS_EXHAUSTED))
            me->RemoveAurasDueToSpell(SPELL_PROTECTORS_EXHAUSTED);
    }

    void IsSummonedBy(Unit* summoner)
    {
        me->SetPhaseId(summoner->GetGUID(), true);
        me->AddPlayerInPersonnalVisibilityList(summoner->GetGUID());
        if (Creature* corruption = me->FindNearestCreature(NPC_GREATER_CORRUPTION, 50.0f, true))
        {
            corruption->AddPlayerInPersonnalVisibilityList(me->GetGUID());
            me->AddPlayerInPersonnalVisibilityList(corruption->GetGUID());
            AttackStart(corruption);
        }
    }

    virtual void DoEvent(uint32 eventId)
    {

    }

    void UpdateAI(uint32 diff)
    {
        UpdateVictim();

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
            DoEvent(eventId);

        DoMeleeAttackIfReady();
    }
};

//71996
class npc_norushen_heal_ch_melee_combtant : public CreatureScript
{
public:
    npc_norushen_heal_ch_melee_combtant() : CreatureScript("npc_norushen_heal_ch_melee_combtant") { }

    struct npc_norushen_heal_ch_melee_combtantAI : public npc_norushen_heal_chAI
    {
        npc_norushen_heal_ch_melee_combtantAI(Creature* pCreature) : npc_norushen_heal_chAI(pCreature)
        {
        }

        void IsSummonedBy(Unit* summoner)
        {
            DoCast(me, SPELL_PROTECTORS_DD);
            npc_norushen_heal_chAI::IsSummonedBy(summoner);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_heal_ch_melee_combtantAI(pCreature);
    }
};

//72000
class npc_norushen_heal_ch_caster : public CreatureScript
{
public:
    npc_norushen_heal_ch_caster() : CreatureScript("npc_norushen_heal_ch_caster") { }

    struct npc_norushen_heal_ch_casterAI : public npc_norushen_heal_chAI
    {
        npc_norushen_heal_ch_casterAI(Creature* pCreature) : npc_norushen_heal_chAI(pCreature)
        {
        }

        enum spells
        {
            SPELL_FIREBALL      = 144522,
        };

        void IsSummonedBy(Unit* summoner)
        {
            DoCast(me, SPELL_PROTECTORS_DD);
            npc_norushen_heal_chAI::IsSummonedBy(summoner);
        }

        void EnterCombat(Unit* who)
        {
            npc_norushen_heal_chAI::EnterCombat(who);
            events.ScheduleEvent(EVENT_2, 1500);
        }

        void DoEvent(uint32 eventId)
        {
            npc_norushen_heal_chAI::DoEvent(eventId);
            if (eventId == EVENT_2)
            {
                DoCastVictim(SPELL_FIREBALL);
                events.ScheduleEvent(EVENT_2, 2500);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_heal_ch_casterAI(pCreature);
    }
};

//71995
class npc_norushen_heal_ch_guardian : public CreatureScript
{
public:
    npc_norushen_heal_ch_guardian() : CreatureScript("npc_norushen_heal_ch_guardian") { }

    struct npc_norushen_heal_ch_guardianAI : public npc_norushen_heal_chAI
    {
        npc_norushen_heal_ch_guardianAI(Creature* pCreature) : npc_norushen_heal_chAI(pCreature)
        {
        }

        enum spells
        {
            SPELL_THREATENING_STRIKE            = 144527,//Threatening Strike 20:10:06.000 20:10:12.000
        };

        void EnterCombat(Unit* who)
        {
            npc_norushen_heal_chAI::EnterCombat(who);
            events.ScheduleEvent(EVENT_2, 1000);
        }

        void DoEvent(uint32 eventId)
        {
            npc_norushen_heal_chAI::DoEvent(eventId);
            if (eventId == EVENT_2)
            {
                DoCastVictim(SPELL_THREATENING_STRIKE);
                events.ScheduleEvent(EVENT_2, 6000);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_norushen_heal_ch_guardianAI(pCreature);
    }
};

//145571 Blind Hatred
class spell_norushen_blind_hatred : public SpellScriptLoader
{
    public:
        spell_norushen_blind_hatred() : SpellScriptLoader("spell_norushen_blind_hatred") { }

        class spell_norushen_blind_hatred_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_norushen_blind_hatred_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                BlindOrderList m;
                GenerateOrder(m);
                Position p = GetFirstRandPoin(m);
                if (Creature* bh = caster->SummonCreature(NPC_BLIND_HATRED, p.GetPositionX(), p.GetPositionY(), p.GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                {
                    for(BlindOrderList::iterator itr = m.begin(); itr != m.end(); ++itr)
                        bh->AI()->SetData(DATA_FILL_MOVE_ORDER, *itr);
                    bh->AI()->SetData(DATA_START_MOVING, 0);
                    caster->CastSpell(bh, SPELL_BLIND_HATRED_V, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_norushen_blind_hatred_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_norushen_blind_hatred_SpellScript();
        }
};

class BlindHatredDmgSelector
{
public:
    BlindHatredDmgSelector(Unit* caster, Creature* blindhatred) : _caster(caster), _blindhatred(blindhatred) {}
    
    bool operator()(WorldObject* target)
    {
        Unit* unit = target->ToUnit();
        
        if (!unit)
            return true;

        if (unit->IsInBetween(_caster, _blindhatred))
            return false;
        
        return true;
    }
private:
    Unit* _caster;
    Creature* _blindhatred;
};

//145227
class spell_norushen_blind_hatred_prock : public SpellScriptLoader
{
    public:
        spell_norushen_blind_hatred_prock() : SpellScriptLoader("spell_blind_hatred") { }

        class spell_norushen_blind_hatred_prock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_norushen_blind_hatred_prock_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                InstanceScript* instance = GetCaster()->GetInstanceScript();
                if (!instance)
                {
                    unitList.clear();
                    return;
                }

                Creature *bh = instance->instance->GetCreature(instance->GetData64(NPC_BLIND_HATRED));
                if (!bh)
                {
                    unitList.clear();
                    return;
                }

                unitList.remove_if (BlindHatredDmgSelector(GetCaster(), bh));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_norushen_blind_hatred_prock_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_norushen_blind_hatred_prock_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_norushen_blind_hatred_prock_SpellScript();
        }
};

//145216
class spell_unleashed_anger : public SpellScriptLoader
{
    public:
        spell_unleashed_anger() : SpellScriptLoader("spell_unleashed_anger") { }

        class spell_unleashed_anger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_unleashed_anger_SpellScript);

            void DealDamage()
            {
                if (!GetCaster() || !GetHitUnit())
                    return;

                GetCaster()->CastSpell(GetHitUnit(), SPELL_UNLEASHED_ANGER_DMG, true);
                GetCaster()->AddAura(SPELL_SELF_DOUBT, GetHitUnit());
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_unleashed_anger_SpellScript::DealDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_unleashed_anger_SpellScript();
        }
};

//145735
class spell_icy_fear_dmg : public SpellScriptLoader
{
    public:
        spell_icy_fear_dmg() : SpellScriptLoader("spell_icy_fear_dmg") { }

        class spell_icy_fear_dmg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icy_fear_dmg_SpellScript);

            void DealDamage()
            {
                if (GetCaster() && GetHitUnit())
                {
                    uint32 pre_mod = GetHitDamage()/100;
                    uint32 pct_mod = 100 - (uint32)floor(GetCaster()->GetHealthPct());
                    if ((pre_mod*pct_mod) > 0)
                        SetHitDamage(GetHitDamage()+(pre_mod*pct_mod));
                    else
                        SetHitDamage(GetHitDamage());
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_icy_fear_dmg_SpellScript::DealDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icy_fear_dmg_SpellScript();
        }
};

//145074 Residual Corruption
class spell_norushen_residual_corruption : public SpellScriptLoader
{
    public:
        spell_norushen_residual_corruption() : SpellScriptLoader("spell_norushen_residual_corruption") { }

        class spell_norushen_residual_corruption_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_norushen_residual_corruption_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                PreventDefaultAction();

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                AreaTrigger * areaTrigger = new AreaTrigger;
                if (!areaTrigger->CreateAreaTrigger(sObjectMgr->GenerateLowGuid(HIGHGUID_AREATRIGGER), 5022, caster, GetSpellInfo(), *caster))
                {
                    delete areaTrigger;
                    return;
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_norushen_residual_corruption_AuraScript::OnApply, EFFECT_0, SPELL_AURA_CREATE_AREATRIGGER, AURA_EFFECT_HANDLE_REAL);
            }

        };

        AuraScript* GetAuraScript() const
        {
            return new spell_norushen_residual_corruption_AuraScript();
        }
};

//144849 144850 144851
class spell_norushen_challenge : public SpellScriptLoader
{
    public:
        spell_norushen_challenge() : SpellScriptLoader("spell_norushen_challenge") { }

        class spell_norushen_challenge_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_norushen_challenge_AuraScript);

            uint64 eventGUID;

            uint32 getPhaseSpell()
            {
                switch(GetId())
                {
                    case SPELL_TEST_OF_SERENITY:    //dd
                        return SPELL_LOOK_WITHIN_DD;
                    case SPELL_TEST_OF_RELIANCE:    //heal
                        return SPELL_LOOK_WITHIN_HEALER;
                    case SPELL_TEST_OF_CONFIDENCE:  //tank
                        return SPELL_LOOK_WITHIN_TANK;
                }
                return 0;
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                PreventDefaultAction();

                Unit* target = GetTarget();
                if (!target || target->GetTypeId() != TYPEID_PLAYER)
                    return;

                InstanceScript* instance = target->GetInstanceScript();
                if (!instance)
                    return;

                //enter phase
                target->CastSpell(target, getPhaseSpell(), true);

                if (Creature* norush = instance->instance->GetCreature(instance->GetData64(NPC_NORUSHEN)))
                    norush->AI()->ZoneTalk(TEXT_GENERIC_10, target->GetGUID());

                target->SetPhaseId(target->GetGUID(), true);

                //target->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_ONLY_OWN_TEMP_CREATRES, ONLY_OWN_TEMP_CREATRES_VISIBILITY_TYPE);

                switch(GetId())
                {
                    case SPELL_TEST_OF_SERENITY:    //dd
                    {
                        target->CastSpell(777.5012f, 974.7348f, 356.3398f, SPELL_SUM_MANIFESTATION_OF_C);
                        target->CastSpell(761.4254f, 982.592f, 356.3398f, SPELL_SUM_ESSENCE_OF_CORRUPT_C);
                        target->CastSpell(766.5504f, 960.6927f, 356.3398f, SPELL_SUM_ESSENCE_OF_CORRUPT_C);
                        target->CastSpell(787.5417f, 987.2986f, 356.3398f, SPELL_SUM_ESSENCE_OF_CORRUPT_C);
                        break;
                    }
                    case SPELL_TEST_OF_RELIANCE:    //heal
                        target->CastSpell(777.5012f, 974.7348f, 356.3398f, SPELL_GREATER_CORRUPTION);
                        target->CastSpell(789.889f, 958.021f, 356.34f, SPELL_MELEE_COMBTANT);
                        target->CastSpell(772.854f, 947.467f, 356.34f, SPELL_CASTER);
                        target->CastSpell(780.8785f, 974.7535f, 356.34f, SPELL_SUMMON_GUARDIAN);
                        break;
                    case SPELL_TEST_OF_CONFIDENCE:  //tank
                        target->CastSpell(777.5012f, 974.7348f, 356.3398f, SPELL_TITANIC_CORRUPTION);
                        break;
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (!target || target->GetTypeId() != TYPEID_PLAYER)
                    return;

                //remove phase
                target->RemoveAurasDueToSpell(getPhaseSpell());
                target->CastSpell(target, SPELL_PURIFIED_CHALLENGE, true);

                switch(GetId())
                {
                    case SPELL_TEST_OF_CONFIDENCE:  //tank
                        target->CastSpell(target, SPELL_TC_CLEANSE, true);
                        CheckCorruptionForCleanse(target->ToPlayer());
                        break;
                    case SPELL_TEST_OF_RELIANCE:
                        target->RemoveAllMinionsByEntry(NPC_NN_HEAL_EVENT_PROTECTOR_1);
                        target->RemoveAllMinionsByEntry(NPC_NN_HEAL_EVENT_PROTECTOR_2);
                        target->RemoveAllMinionsByEntry(NPC_NN_HEAL_EVENT_PROTECTOR_3);
                        break;
                    default:
                        break;                    
                }
                target->SetPhaseId(0, true);
                //target->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_ONLY_OWN_TEMP_CREATRES, 0);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_norushen_challenge_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_norushen_challenge_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_norushen_challenge_AuraScript();
        }
};

//144521
class spell_norushen_heal_test_dd : public SpellScriptLoader
{
    public:
        spell_norushen_heal_test_dd() : SpellScriptLoader("spell_norushen_heal_test_dd") { }

        class spell_norushen_heal_test_dd_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_norushen_heal_test_dd_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                //ToDo: posible it's dinamically damage modificator.
                if (AuraEffect* aurEff = caster->GetAuraEffect(GetSpellInfo()->Id, EFFECT_2))
                {
                    if (aurEff->GetAmount() == 300)
                        return;

                    if (AuraApplication * aurApp = GetAura()->GetApplicationOfTarget(GetCasterGUID()))
                    {
                        aurEff->HandleEffect(aurApp, AURA_EFFECT_HANDLE_REAL, false);
                        aurEff->SetAmount(300);
                        aurEff->HandleEffect(aurApp, AURA_EFFECT_HANDLE_REAL, true);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_norushen_heal_test_dd_AuraScript::OnTick, EFFECT_3, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_norushen_heal_test_dd_AuraScript();
        }
};

void AddSC_boss_norushen()
{
    new boss_norushen();
    new npc_norushen_lowerwalker();
    new boss_amalgam_of_corruption();
    new npc_blind_hatred();
    new npc_norushen_purifying_light();
    new npc_norushen_residual_corruption();
    new npc_essence_of_corruption_challenge();
    new npc_norushen_manifestation_of_corruption_challenge();
    new npc_norushen_manifestation_of_corruption_released();
    new npc_essence_of_corruption_released();
    new npc_titanic_corruption();
    new npc_norushen_heal_ch_greater_corruption();
    new npc_norushen_heal_ch_melee_combtant();
    new npc_norushen_heal_ch_caster();
    new npc_norushen_heal_ch_guardian();
    new spell_norushen_blind_hatred();
    new spell_norushen_blind_hatred_prock();
    new spell_unleashed_anger();
    new spell_icy_fear_dmg();
    new spell_norushen_residual_corruption();
    new spell_norushen_challenge();
    new spell_norushen_heal_test_dd();
}