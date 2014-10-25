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
    SPELL_VISUAL_TELEPORT      = 149634,
    SPELL_VISUAL_TELEPORT_AC   = 145188,
    SPELL_EXTRACT_CORRUPTION   = 145143,
    SPELL_EXTRACT_CORRUPTION_S = 145149,
    //SPELL_PURIFIED             = 146022,

    //Amalgam_of_Corruption
    SPELL_SPAWN_AMALGAM        = 145118,
    SPELL_CORRUPTION           = 144421, 
    SPELL_PURIFIED             = 144452,
    SPELL_ICY_FEAR             = 145733,
    SPELL_UNLEASHED_ANGER      = 145214,
    SPELL_UNLEASHED_ANGER_DMG  = 145212,
    SPELL_UNCHECKED_CORRUPTION = 145679,
    SPELL_SELF_DOUBT           = 146124,
    SPELL_QUARANTINE_SAFETY    = 145779,
    SPELL_FRAYED               = 146179,

    //Phase spells
    SPELL_LOOK_WITHIN          = 146837,

    //Blind Hatred
    SPELL_BLIND_HATRED_V       = 145226,
    SPELL_BLIND_HATRED_D       = 145227,

    SPELL_CLEANSE              = 147657,

    //Titanic Corruption
    SPELL_BURST_OF_CORRUPTION  = 144654,
    SPELL_CORRUPTION_TC        = 144639,
    SPELL_HURL_CORRUPTION      = 144649,
    SPELL_PIERCING_CORRUPTION  = 144657,
    SPELL_TITANIC_SMASH        = 144628,

    //Greater Corruption
    SPELL_BOTTOMLESS_PIT       = 146703,
    SPELL_DISHEARTENING_LAUGH  = 146707,
    SPELL_LINGERING_CORRUPTION = 144514,

    //Manifestation of Corruption
    SPELL_TEAR_REALITY         = 144482,
    SPELL_RESIDUAL_CORRUPTION  = 145074,

    //Essence of Corruption
    SPELL_EXPEL_CORRUPTION_AT  = 144548, //Create areatrigger
    SPELL_EXPELLED_CORRUPTION  = 144480,


    //Test for players
    SPELL_TEST_OF_SERENITY     = 144849, //dd
    SPELL_TEST_OF_RELIANCE     = 144850, //heal
    SPELL_TEST_OF_CONFIDENCE   = 144851, //tank
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
                me->SetUInt32Value(UNIT_NPC_FLAGS, 1);
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
                            me->CastSpell(me, SPELL_VISUAL_TELEPORT, true);
                            me->CastSpell(me, SPELL_VISUAL_TELEPORT_AC, true);
                            ZoneTalk(eventId + 6, me->GetGUID());
                            break;
                        case EVENT_2:
                            instance->SetBossState(DATA_NORUSHEN, IN_PROGRESS);
                            me->SetFacingTo(1.791488f);
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

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_ICY_FEAR);
                me->SetReactState(REACT_DEFENSIVE);
                me->ModifyAuraState(AURA_STATE_UNKNOWN22, true);
                ApplyOrRemoveBar(false);
                FrayedCounter = 0;
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
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
                //events.ScheduleEvent(EVENT_CHECK_VICTIM, 2000);
                //events.ScheduleEvent(EVENT_UNLEASHED_ANGER, 11000);     //18:23:51.000 - 18:24:02.000
                //events.ScheduleEvent(EVENT_QUARANTINE_SAFETY, 420000);
                events.ScheduleEvent(EVENT_BLIND_HATRED, 1/*12000*/);
                me->CastSpell(me, SPELL_SPAWN_AMALGAM, true);
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
                if (HealthBelowPct(50) && damage > me->GetHealth())
                {
                    if (!me->HasAura(SPELL_FRAYED))
                        me->AddAura(SPELL_FRAYED, me);

                    if (HealthBelowPct(50 + 10*FrayedCounter))
                    {
                        ++FrayedCounter;
                        SummonManifestationofCorruption();
                    }
                }
            }

            void SummonManifestationofCorruption()
            {
                if (Creature* moc = me->SummonCreature(NPC_MANIFESTATION_OF_CORRUPTION, me->GetPositionX()+ 5.0f, me->GetPositionY(), me->GetPositionZ()))
                    DoZoneInCombat(moc, 100.0f);
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
                if (!UpdateVictim()/* || me->HasUnitState(UNIT_STATE_CASTING)*/)
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
                            BlindOrderList m;
                            GenerateOrder(m);
                            Position p = GetFirstRandPoin(m);
                            if (Creature* bh = me->SummonCreature(NPC_BLIND_HATRED, p.GetPositionX(), p.GetPositionY(), p.GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                            {
                                DoCast(bh, SPELL_BLIND_HATRED_V, false);
                                for(BlindOrderList::iterator itr = m.begin(); itr != m.end(); ++itr)
                                    bh->AI()->SetData(DATA_FILL_MOVE_ORDER, *itr);
                                bh->AI()->SetData(DATA_START_MOVING, 0);
                            }
                            events.ScheduleEvent(EVENT_BLIND_HATRED, 40000);
                            break;
                        }
                    }

                }
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

//72065
class npc_purifying_light : public CreatureScript
{
public:
    npc_purifying_light() : CreatureScript("npc_purifying_light") { }

    struct npc_purifying_lightAI : public CreatureAI
    {
        npc_purifying_lightAI(Creature* pCreature) : CreatureAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;

        void Reset(){}

        void OnSpellClick(Unit* clicker)
        {
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}     
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_purifying_lightAI(pCreature);
    }
};

//71976 for dd
class npc_essence_of_corruption : public CreatureScript
{
public:
    npc_essence_of_corruption() : CreatureScript("npc_essence_of_corruption") { }

    struct npc_essence_of_corruptionAI : public ScriptedAI
    {
        npc_essence_of_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* who)
        {
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (me->ToTempSummon())
            {
                if (Unit* aoc = me->ToTempSummon()->GetSummoner())
                {
                    if (aoc->isAlive() && aoc->isInCombat())
                    {
                        if (aoc->GetHealth() >= damage)
                            aoc->SetHealth(aoc->GetHealth() - damage);
                    }
                }
            }
        }
        
        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_essence_of_corruptionAI(pCreature);
    }
};

//72264 for dd
class npc_manifestation_of_corruption : public CreatureScript
{
public:
    npc_manifestation_of_corruption() : CreatureScript("npc_manifestation_of_corruption") { }

    struct npc_manifestation_of_corruptionAI : public ScriptedAI
    {
        npc_manifestation_of_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_TEAR_REALITY, 8500);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (me->ToTempSummon())
            {
                if (Unit* aoc = me->ToTempSummon()->GetSummoner())
                {
                    if (aoc->isAlive() && aoc->isInCombat())
                    {
                        if (aoc->GetHealth() >= damage)
                            aoc->SetHealth(aoc->GetHealth() - damage);
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
                case EVENT_TEAR_REALITY:
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    DoCastAOE(SPELL_TEAR_REALITY);
                    events.ScheduleEvent(EVENT_RE_ATTACK, 1000);
                    events.ScheduleEvent(EVENT_TEAR_REALITY, 8500);
                    break;
                case EVENT_RE_ATTACK:
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 75.0f);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_manifestation_of_corruptionAI(pCreature);
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
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            events.Reset();
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
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

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
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_titanic_corruptionAI(pCreature);
    }
};

//72001 for healers
class npc_greater_corruption : public CreatureScript
{
public:
    npc_greater_corruption() : CreatureScript("npc_greater_corruption") { }

    struct npc_greater_corruptionAI : public ScriptedAI
    {
        npc_greater_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = (InstanceScript*)pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* who){}


        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

          /*  while (uint32 eventId = events.ExecuteEvent())
            {                
            }*/
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_greater_corruptionAI(pCreature);
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
class spell_blind_hatred : public SpellScriptLoader
{
    public:
        spell_blind_hatred() : SpellScriptLoader("spell_blind_hatred") { }

        class spell_blind_hatred_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blind_hatred_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                if (Creature* bh = GetCaster()->FindNearestCreature(NPC_BLIND_HATRED, 50.0f, true))
                {
                    unitList.remove_if (BlindHatredDmgSelector(GetCaster(), bh));
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blind_hatred_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blind_hatred_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blind_hatred_SpellScript();
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

void AddSC_boss_norushen()
{
    new boss_norushen();
    new npc_norushen_lowerwalker();
    new boss_amalgam_of_corruption();
    new npc_blind_hatred();
    new npc_purifying_light();
    new npc_essence_of_corruption();
    new npc_manifestation_of_corruption();
    new npc_titanic_corruption();
    new npc_greater_corruption();
    new spell_unleashed_anger();
    new spell_blind_hatred();
    new spell_icy_fear_dmg();
}