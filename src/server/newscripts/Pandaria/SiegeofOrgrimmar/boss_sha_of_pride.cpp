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
#include "ScriptedEscortAI.h"
#include "CreatureTextMgr.h"
#include "CreatureGroups.h"
#include "GameObjectAI.h"

// UPDATE `creature` SET `spawnMask` = '16632' WHERE map = 1136;
enum eSpells
{
    SPELL_SUBMERGE                  = 103742, //sha spawn
    SPELL_MANIFESTATION_SPAWN       = 144778, //Manifestation Spawn
    SPELL_SELF_REFLECTION_SPAWN     = 144784, //Self-Reflection
    SPELL_PRIDE                     = 144343,

    SPELL_CORRUPTED_PRISON_WEST     = 144574, //Corrupted Prison
    SPELL_CORRUPTED_PRISON_EAST     = 144636, //Corrupted Prison
    SPELL_CORRUPTED_PRISON_NORTH    = 144683, //Corrupted Prison 25ppl
    SPELL_CORRUPTED_PRISON_SOUTH    = 144684, //Corrupted Prison 25ppl
    SPELL_CORRUPTED_PRISON_KNOCK    = 144615, //Corrupted Prison

    SPELL_IMPRISON                  = 144563, //Imprison
    SPELL_MARK_OF_ARROGANCE         = 144351, //Mark of Arrogance
    SPELL_REACHING_ATTACK           = 144774, //Reaching Attack 119775
    SPELL_SELF_REFLECTION           = 144800, //Self-Reflection
    SPELL_WOUNDED_PRIDE             = 144358, //Wounded Pride

    SPELL_UNLEASHED                 = 144832, //Unleashed

    //Pride
    SPELL_SWELLING_PRIDE            = 144400, //Swelling Pride
    SPELL_BURSTING_PRIDE            = 144910, //Bursting Pride  25-49
    SPELL_PROJECTION                = 146822, //Projection      50-74
    SPELL_PROJECTION_MARKER         = 145066,
    SPELL_PROJECTION_DMG            = 145320,
    SPELL_AURA_OF_PRIDE             = 146817, //Aura of Pride   75-99
    SPELL_OVERCOME                  = 144843, //Overcome        100 
    SPELL_OVERCOME_MIND_CONTROL     = 144863,
    
    //Manifestation of Pride
    SPELL_MOCKING_BLAST             = 144379, //Mocking Blast
    SPELL_LAST_WORD                 = 144370, //Last Word

    //Norushen
    SPELL_DOOR_CHANNEL              = 145979, //Door Channel
    SPELL_FINAL_GIFT                = 144854, //Final Gift
    SPELL_GIFT_OF_THE_TITANS        = 144359, //Gift of the Titans
    SPELL_POWER_OF_THE_TITANS       = 144364, //Power of the Titans

    //Lingering Corruption
    SPELL_CORRUPTION_TOUCH          = 149207, //Corrupted Touch

    //Reflection
    SPELL_SELF_REFLECTION_CAST      = 144788, //Self-Reflection

    //Rift of Corruption
    SPELL_RIFT_OF_CORRUPTION        = 147199, //Rift of Corruption
    SPELL_UNSTABLE_CORRUPTION       = 147389, //Unstable Corruption
    SPELL_RIFT_OF_CORRUPTION_AT     = 147205,
    SPELL_RIFT_OF_CORRUPTION_DMG    = 147391,
    SPELL_WEAKENED_RESOLVE          = 147207, //Weakened Resolve

    //
    SPELL_ORB_OF_LIGHT              = 145345, //Orb of Light
};

Position const Sha_of_pride_taranzhu  = {748.1805f, 1058.264f, 356.1557f, 5.566918f };
Position const Sha_of_pride_finish_jaina  = {765.6154f, 1050.112f, 357.0135f, 1.514341f };
Position const Sha_of_pride_finish_teron  = {756.955f, 1048.71f, 357.0236f, 1.68638f };
Position const Sha_of_pride_portal[2]  = {
    {783.5452f, 1168.182f, 356.1551f, 4.221592f},
    {691.1077f, 1149.943f, 356.1552f, 5.842754f},
};

//Manifestation of Pride
Position const Sha_of_pride_manifestation[2]  = {
    {809.2969f, 1125.891f, 356.1557f, 3.349345f },
    {686.1302f, 1099.786f, 356.1556f, 0.1931868f },
};

uint32 const prison[4] = { GO_CORRUPTED_PRISON_WEST, GO_CORRUPTED_PRISON_EAST, GO_CORRUPTED_PRISON_NORTH, GO_CORRUPTED_PRISON_SOUTH };
uint32 const prison_spell[4] = { SPELL_CORRUPTED_PRISON_WEST, SPELL_CORRUPTED_PRISON_EAST, SPELL_CORRUPTED_PRISON_NORTH, SPELL_CORRUPTED_PRISON_SOUTH };
enum PhaseEvents
{
    EVENT_SPELL_MARK_OF_ARROGANCE       = 1,    
    EVENT_SPELL_WOUNDED_PRIDE           = 2,
    EVENT_SUMMON_MANIFESTATION_OF_PRIDE = 3,
    EVENT_SPELL_SELF_REFLECTION         = 4,
    EVENT_SPELL_CORRUPTED_PRISON        = 5,
    EVENT_SPELL_REACHING_ATTACK         = 6,
    EVENT_SPELL_GIFT_OF_THE_TITANS      = 7,
    EVENT_PRIDE_GENERATION              = 8,
    EVENT_SPELL_UNLEASHED               = 9,
    EVENT_RIFT_OF_CORRUPTION            =10,
};

enum Phases
{
    PHASE_BATTLE                    = 1,
};

void addPride(uint32 spellID, Unit* target)
{
    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (target->HasAura(SPELL_GIFT_OF_THE_TITANS))
        return;

    switch(spellID)
    {
        case 0:
        case SPELL_REACHING_ATTACK:
        case SPELL_MOCKING_BLAST:
        case SPELL_SELF_REFLECTION_CAST:
        case SPELL_RIFT_OF_CORRUPTION_DMG:
        case SPELL_MARK_OF_ARROGANCE:
            break;
        default:
            return;
    }

    uint32 incrasePride = 5;
    uint32 power = target->GetPower(POWER_ALTERNATE_POWER);
    if (power + incrasePride > 100)
        incrasePride = 100 - power;

    if (incrasePride && power < 100)
        target->SetPower(POWER_ALTERNATE_POWER, power + incrasePride);
}

class boss_sha_of_pride : public CreatureScript
{
    public:
        boss_sha_of_pride() : CreatureScript("boss_sha_of_pride") {}

        struct boss_sha_of_prideAI : public BossAI
        {
            boss_sha_of_prideAI(Creature* creature) : BossAI(creature, DATA_SHA_OF_PRIDE)
            {
                instance = creature->GetInstanceScript();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                SetCombatMovement(false);
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PRIDE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OVERCOME);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MARK_OF_ARROGANCE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OVERCOME_MIND_CONTROL);

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 0);

                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            void SetData(uint32 id, uint32 value)
            {
                //event after killing all NPC_LINGERING_CORRUPTION. Appear of Sha.
                if (id == NPC_LINGERING_CORRUPTION)
                {
                    ZoneTalk(TEXT_GENERIC_0, 0);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    SetCombatMovement(false);
                    //me->SetUnitMovementFlags(1536);
                    //me->AddExtraUnitMovementFlag(15);
                    //me->GetMotionMaster()->MoveLand(0, *me);
                    me->AddAura(SPELL_SUBMERGE, me);
                    me->SetVisible(true);
                    DoCast(me, SPELL_SUBMERGE, false);
                }
            }
            void KilledUnit(Unit* /*victim*/) 
            {
                ZoneTalk(urand(TEXT_GENERIC_9, TEXT_GENERIC_10), 0);
            }
            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                ZoneTalk(TEXT_GENERIC_1, 0);
                events.SetPhase(PHASE_BATTLE);
                uint32 t = 0;
                
                if (IsHeroic())events.RescheduleEvent(EVENT_RIFT_OF_CORRUPTION, t += 2000, 0, PHASE_BATTLE);                 //19:02:02.000
                events.RescheduleEvent(EVENT_SPELL_GIFT_OF_THE_TITANS, t += 1000, 0, PHASE_BATTLE);           //19:02:03.000
                events.RescheduleEvent(EVENT_SPELL_WOUNDED_PRIDE, t += 3000, 0, PHASE_BATTLE);                //19:02:06.000
                events.RescheduleEvent(EVENT_SPELL_MARK_OF_ARROGANCE, t += 2000, 0, PHASE_BATTLE);            //19:02:08.000
                events.RescheduleEvent(EVENT_SPELL_SELF_REFLECTION, t += 13000, 0, PHASE_BATTLE);             //19:02:21.000
                events.RescheduleEvent(EVENT_SPELL_CORRUPTED_PRISON, t += 27000, 0, PHASE_BATTLE);            //19:02:48.000
                events.RescheduleEvent(EVENT_SUMMON_MANIFESTATION_OF_PRIDE, t += 9000, 0, PHASE_BATTLE);      //19:02:57.000
                events.RescheduleEvent(EVENT_PRIDE_GENERATION, 4000);                                         //first SPELL_SWELLING_PRIDE at 19:03:11.000
                events.RescheduleEvent(EVENT_SPELL_REACHING_ATTACK, 15000, 0, PHASE_BATTLE);                  //19:03:49.000. Cast only if no in attack range ppl.
                events.RescheduleEvent(EVENT_SPELL_UNLEASHED, RAID_MODE(10 * MINUTE * IN_MILLISECONDS, 10 * MINUTE * IN_MILLISECONDS, 20* MINUTE * IN_MILLISECONDS , 20 * MINUTE * IN_MILLISECONDS), 0, PHASE_BATTLE);                //19:21:11.000 

                Map::PlayerList const& PlayerList = instance->instance->GetPlayers();

                if (PlayerList.isEmpty())
                    return;

                for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
                {
                    Player* player = Itr->getSource();

                    if (!player)
                        continue;

                    DoCast(player, SPELL_PRIDE, true);
                }                
            }

            void DamageDealt(Unit* victim, uint32& /*damage*/, DamageEffectType damageType)
            {
                if (damageType != DIRECT_DAMAGE)
                    return;

                if (victim->HasAura(SPELL_WOUNDED_PRIDE))
                    addPride(0, victim);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;

                switch (spell->Id)
                {
                    case SPELL_SWELLING_PRIDE:
                    {
                        uint32 power = target->GetPower(POWER_ALTERNATE_POWER);
                        if (power >= 25 && power <= 49)
                            DoCast(target, SPELL_BURSTING_PRIDE, true);
                        else if (power >= 50 && power <= 74)
                            DoCast(target, SPELL_PROJECTION, true);
                        else if (power >= 75 && power <= 99)
                            DoCast(target, SPELL_AURA_OF_PRIDE, true);
                        else if (power == 100)
                        {
                            if (target->HasAura(SPELL_OVERCOME))
                                me->CastSpell(target, SPELL_OVERCOME_MIND_CONTROL, true);
                            else
                                target->CastSpell(target, SPELL_OVERCOME, true);
                        }
                        break;
                    }
                }
                addPride(spell->Id, target);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PRIDE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OVERCOME);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MARK_OF_ARROGANCE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OVERCOME_MIND_CONTROL);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                EnterEvadeIfOutOfCombatArea(diff);
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_SPELL_MARK_OF_ARROGANCE:
                            DoCast(me, SPELL_MARK_OF_ARROGANCE);
                            events.RescheduleEvent(EVENT_SPELL_MARK_OF_ARROGANCE, 20000, 0, PHASE_BATTLE);
                            break;
                        case EVENT_PRIDE_GENERATION:
                            me->SetPower(POWER_ENERGY, me->GetPower(POWER_ENERGY) + 5);
                            if (me->GetPower(POWER_ENERGY) == 100)
                            {
                                ZoneTalk(TEXT_GENERIC_12, 0);
                                ZoneTalk(urand(TEXT_GENERIC_5, TEXT_GENERIC_6), 0);
                                DoCast(me, SPELL_SWELLING_PRIDE, false);
                            }
                            events.RescheduleEvent(EVENT_PRIDE_GENERATION, 4000);
                            break;
                        case EVENT_SPELL_WOUNDED_PRIDE:
                            DoCastVictim(SPELL_WOUNDED_PRIDE);
                            events.RescheduleEvent(EVENT_SPELL_WOUNDED_PRIDE, 30*IN_MILLISECONDS, 0, PHASE_BATTLE);
                            break;
                        case EVENT_SPELL_SELF_REFLECTION:
                            ZoneTalk(TEXT_GENERIC_4, 0);
                            DoCast(me, SPELL_SELF_REFLECTION, false);
                            events.RescheduleEvent(EVENT_SPELL_SELF_REFLECTION, 78*IN_MILLISECONDS, 0, PHASE_BATTLE);
                            break;
                        case EVENT_SPELL_CORRUPTED_PRISON:
                        {
                            ZoneTalk(TEXT_GENERIC_11, 0);
                            ZoneTalk(TEXT_GENERIC_3, 0);
                            //Should be done by casting this spell, but this half-hack better check targets and cast spells by order.
                            DoCast(me, SPELL_IMPRISON, true);
                            uint8 count = Is25ManRaid() ? 4 : 2;
                            uint8 i = 0;
                            std::list<Unit*> targetList;
                            SelectTargetList(targetList, count, SELECT_TARGET_RANDOM, 0.0f, true);
                            for(std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                            {
                                if (GameObject* prisonGo = instance->instance->GetGameObject(instance->GetData64(prison[i])))
                                {
                                    me->CastSpell(prisonGo->GetPositionX(), prisonGo->GetPositionY(), prisonGo->GetPositionZ(), SPELL_CORRUPTED_PRISON_KNOCK);
                                    prisonGo->AI()->SetGUID((*itr)->GetGUID(), false);
                                    DoCast(*itr, prison_spell[i], true);
                                }
                                ++i;
                            }
                            events.RescheduleEvent(EVENT_SPELL_CORRUPTED_PRISON, 78*IN_MILLISECONDS, 0, PHASE_BATTLE);  //19:02:48.000
                            break;
                        }
                        case EVENT_SPELL_REACHING_ATTACK:
                            //Should be first
                            events.RescheduleEvent(EVENT_SPELL_REACHING_ATTACK, (urand(15, 20))*IN_MILLISECONDS, 0, PHASE_BATTLE);           //19:03:49.000

                            // only if in attack distance no target.
                            if(!me->getVictim() || me->IsWithinMeleeRange(me->getVictim()))
                                break;

                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 0.0f, true))
                                DoCast(target, SPELL_REACHING_ATTACK, false);
                            break;
                        case EVENT_SUMMON_MANIFESTATION_OF_PRIDE:
                        {
                            uint8 count = Is25ManRaid() ? 2 : 1;
                            for (uint8 i = 0; i < count; ++i)
                                me->SummonCreature(NPC_MANIFEST_OF_PRIDE, Sha_of_pride_manifestation[i], TEMPSUMMON_DEAD_DESPAWN);

                            events.RescheduleEvent(EVENT_SUMMON_MANIFESTATION_OF_PRIDE, 77*IN_MILLISECONDS, 0, PHASE_BATTLE);
                            break;
                        }
                        case EVENT_SPELL_UNLEASHED:
                        {
                            if (Creature* nor = instance->instance->GetCreature(instance->GetData64(NPC_SHA_NORUSHEN)))
                            {
                                ZoneTalk(TEXT_GENERIC_8, 0);
                                DoCast(nor, SPELL_UNLEASHED, false);
                                events.CancelEvent(EVENT_SPELL_GIFT_OF_THE_TITANS);
                            }
                            break;
                        }
                        case EVENT_SPELL_GIFT_OF_THE_TITANS:
                        {
                            if (Creature* nor = instance->instance->GetCreature(instance->GetData64(NPC_SHA_NORUSHEN)))
                                nor->AI()->SetData(SPELL_GIFT_OF_THE_TITANS, 0);

                            events.RescheduleEvent(EVENT_SPELL_GIFT_OF_THE_TITANS, 25000, 0, PHASE_BATTLE);
                            break;
                        }
                        case EVENT_RIFT_OF_CORRUPTION:
                        {
                            float x, y, z;
                            me->GetRandomPoint(*me, 50.0f, x, y, z);
                            me->SummonCreature(NPC_RIFT_OF_CORRUPTION, x, y, z, 0.0f);
                            events.RescheduleEvent(EVENT_RIFT_OF_CORRUPTION, urand(10000, 20000), 0, PHASE_BATTLE);
                            break;
                        }
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sha_of_prideAI(creature);
        }
};

class npc_sha_of_pride_norushen : public CreatureScript
{
public:
    npc_sha_of_pride_norushen() : CreatureScript("npc_sha_of_pride_norushen") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_norushenAI (creature);
    }

    struct npc_sha_of_pride_norushenAI : public npc_escortAI
    {
        npc_sha_of_pride_norushenAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        std::set<uint64> _gift;
        InstanceScript* instance;
        EventMap events;
        bool start;

        void Reset()
        {
            start = false;
            _gift.clear();
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (start)return;
            start = true;

            ZoneTalk(TEXT_GENERIC_0, me->GetGUID());
            Start(false, false);
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_UNLEASHED)
            {
                DoCast(me, SPELL_FINAL_GIFT, false);
                ZoneTalk(TEXT_GENERIC_4, 0);
            }
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (target->GetTypeId() != TYPEID_PLAYER || spell->Id != SPELL_GIFT_OF_THE_TITANS)
                return;

            _gift.insert(target->GetGUID());
        }

        void SetGUID(uint64 guid, int32 /*id*/ = 0)
        {
            _gift.erase(guid);
        }

        void SetData(uint32 id, uint32 value)
        {
            //event after killing all NPC_LINGERING_CORRUPTION. Appear of Sha.
            if (id == NPC_LINGERING_CORRUPTION)
            {
                instance->SetData(DATA_SHA_PRE_EVENT, DONE);
                ZoneTalk(TEXT_GENERIC_2, 0);             //18:47:21.000 
                events.ScheduleEvent(EVENT_2, 11000);    //18:47:32.000
            }else if (id == SPELL_GIFT_OF_THE_TITANS)
            {
                ZoneTalk(TEXT_GENERIC_3, 0);
                me->CastSpell(me, SPELL_GIFT_OF_THE_TITANS, true);
            }else if (id == EVENT_SPELL_GIFT_OF_THE_TITANS)
            {
                bool good = _gift.size() == value;
                for(std::set<uint64>::iterator itr = _gift.begin(); itr != _gift.end(); ++itr)
                {
                    Player* target = ObjectAccessor::FindPlayer(*itr);
                    if (!target)
                        continue;

                    if (good)
                        target->AddAura(SPELL_POWER_OF_THE_TITANS, target);
                    else
                        target->RemoveAura(SPELL_POWER_OF_THE_TITANS);
                }
            }
        }

        void WaypointReached(uint32 i)
        {
            switch(i)
            {
                case 4:
                    SetEscortPaused(true);
                    DoCast(me, SPELL_DOOR_CHANNEL, false);
                    //
                    if (GameObject* door = instance->instance->GetGameObject(instance->GetData64(GO_NORUSHEN_EX_DOOR)))
                        door->SetGoState(GO_STATE_ACTIVE);
                    events.ScheduleEvent(EVENT_1, 2000);
                    break;
                case 5:
                    if (Creature* lo = instance->instance->GetCreature(instance->GetData64(NPC_LOREWALKER_CHO3)))
                        lo->AI()->DoAction(EVENT_1);
                    SetEscortPaused(true);
                    ZoneTalk(TEXT_GENERIC_1, me->GetGUID());
                    //Start pre-event 
                    instance->SetData(DATA_SHA_PRE_EVENT, IN_PROGRESS);

                    // WARNING!! T M P !!!!!!
                    //SetData(NPC_LINGERING_CORRUPTION, true);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        SetEscortPaused(false);
                        break;
                    case EVENT_2:
                        if (Creature* sha = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE)))
                        {
                            
                            sha->AI()->SetData(NPC_LINGERING_CORRUPTION, DONE); 
                        }
                        break;
                }
            }
        }
    };
};

class npc_sha_of_pride_lowerwalker : public CreatureScript
{
public:
    npc_sha_of_pride_lowerwalker() : CreatureScript("npc_sha_of_pride_lowerwalker") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_lowerwalkerAI (creature);
    }

    struct npc_sha_of_pride_lowerwalkerAI : public npc_escortAI
    {
        npc_sha_of_pride_lowerwalkerAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
            group_member = sFormationMgr->CreateCustomFormation(me);
            end = false;
        }

        InstanceScript* instance;
        EventMap events;
        FormationInfo* group_member;
        bool end;

        void Reset()
        {

        }

        void DoAction(int32 const action)
        {
            switch(action)
            {
                case EVENT_1:
                    SetEscortPaused(false);
                    break;
                case EVENT_2:
                    end = true;     //19:34:01
                    uint32 t = 0;
                    events.ScheduleEvent(EVENT_7, t+= 2000);   //19:34:01
                    events.ScheduleEvent(EVENT_8, t+= 11000);  //19:34:13.000
                    events.ScheduleEvent(EVENT_9, t+= 10000);  //19:34:23.000
                    events.ScheduleEvent(EVENT_10, t+= 4000);  //19:34:27.000
                    events.ScheduleEvent(EVENT_16, t+= 3000);  //19:34:30.000
                    events.ScheduleEvent(EVENT_11, t+= 4000);  //19:34:33.000
                    events.ScheduleEvent(EVENT_12, t+= 9000);  //19:34:42.000
                    events.ScheduleEvent(EVENT_13, t+= 5000);  //19:34:47.000
                    events.ScheduleEvent(EVENT_17, t+= 12000);  //19:34:59.000
                    events.ScheduleEvent(EVENT_14, t+= 5000); //19:35:04.000
                    events.ScheduleEvent(EVENT_15, t+= 20000); //19:35:24.000
                    
                    if (Creature * c = instance->instance->SummonCreature(NPC_SHA_OF_PRIDE_END_LADY_JAINA, Sha_of_pride_finish_jaina))
                        c->GetMotionMaster()->MovePoint(c->GetGUIDLow(), 756.9792f, 1093.34f, 356.0723f);
                    if (Creature * c = instance->instance->SummonCreature(NPC_SHA_OF_PRIDE_END_THERON, Sha_of_pride_finish_teron))
                        c->GetMotionMaster()->MovePoint(c->GetGUIDLow(), 739.9184f, 1129.293f, 356.0723f);
                    break;
            }            
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!end)
                Start(false, true);
        }

        void WaypointReached(uint32 i)
        {
            switch(i)
            {
                case 5:
                    SetEscortPaused(true);
                    if (Creature * c = instance->instance->SummonCreature(NPC_SHA_TARAN_ZHU, Sha_of_pride_taranzhu))
                    {
                    }
                    break;
                case 6:
                {
                    SetEscortPaused(true);
                    uint32 t = 0;
                    events.ScheduleEvent(EVENT_1, t += 1000);    //18:45:18.000
                    events.ScheduleEvent(EVENT_3, t += 5000);    //18:45:23.000 
                    events.ScheduleEvent(EVENT_2, t += 1000);    //18:45:24.000
                    events.ScheduleEvent(EVENT_4, t += 12000);   //18:45:36.000
                    events.ScheduleEvent(EVENT_5, t += 11000);   //18:45:47.000
                    events.ScheduleEvent(EVENT_6, t += 1);
                    //18:45:47.000
                    break;
                }
                case 16:
                    if (Creature* taran = instance->instance->GetCreature(instance->GetData64(NPC_SHA_TARAN_ZHU)))
                    {
                        if (CreatureGroup* f = me->GetFormation())
                        {
                            f->RemoveMember(taran);
                            f->RemoveMember(me);
                            delete group_member;
                        }
                        taran->DespawnOrUnsummon();
                    }
                    me->DespawnOrUnsummon();
                    break;
            }
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
                        if (Creature* taran = instance->instance->GetCreature(instance->GetData64(NPC_SHA_TARAN_ZHU)))
                            taran->AI()->ZoneTalk(eventId - 1, me->GetGUID());
                        break;
                    case EVENT_3:
                    case EVENT_4:
                    case EVENT_5:
                        if (Creature* taran = instance->instance->GetCreature(instance->GetData64(NPC_SHA_TARAN_ZHU)))
                            ZoneTalk(eventId - 3, taran->GetGUID());
                        break;
                    case EVENT_6:
                        me->SetWalk(true);
                        if (Creature* taran = instance->instance->GetCreature(instance->GetData64(NPC_SHA_TARAN_ZHU)))
                            if (CreatureGroup* f = me->GetFormation())
                                f->AddMember(taran, group_member);
                        SetEscortPaused(false);
                        break;
                    case EVENT_7:
                        ZoneTalk(TEXT_GENERIC_3, 0);
                        //me->DespawnOrUnsummon(60000);
                        break;
                    case EVENT_8:
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                            jaina->AI()->ZoneTalk(TEXT_GENERIC_0, 0);
                        break;
                    case EVENT_9:
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                            jaina->GetMotionMaster()->MovePoint(jaina->GetGUIDLow(), 748.8203f, 1130.096f, 356.0723f);
                        if (Creature* teron = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_THERON)))
                            teron->AI()->ZoneTalk(TEXT_GENERIC_0, 0);
                        break;
                    case EVENT_10:
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                            jaina->AI()->ZoneTalk(TEXT_GENERIC_1, 0);
                        break;
                    case EVENT_11:
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                            jaina->AI()->ZoneTalk(TEXT_GENERIC_2, 0);
                        break;
                    case EVENT_12:
                        if (Creature* teron = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_THERON)))
                            teron->AI()->ZoneTalk(TEXT_GENERIC_2, 0);
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                            jaina->GetMotionMaster()->MovePoint(jaina->GetGUIDLow(), 748.5174f, 1131.481f, 356.0723f);
                        break;
                    case EVENT_13:
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                            jaina->AI()->ZoneTalk(TEXT_GENERIC_3, 0);
                        break;
                    case EVENT_17:
                        if (Creature* teron = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_THERON)))
                            teron->AI()->ZoneTalk(TEXT_GENERIC_3, 0);
                        break;
                    case EVENT_14:
                        for(uint32 i = 0; i < 2; ++i)
                            if (Creature * c = instance->instance->SummonCreature(NPC_PORTAL_TO_ORGRIMMAR, Sha_of_pride_portal[i]))
                            {
                                c->SetInt32Value(UNIT_FIELD_INTERACT_SPELL_ID, 148034);
                                c->SetDisplayId(51795);
                            }
                            
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                        {
                            jaina->AI()->ZoneTalk(TEXT_GENERIC_4, 0);
                            jaina->GetMotionMaster()->MovePoint(jaina->GetGUIDLow(), 783.2882f, 1167.352f, 356.0717f);
                        }
                        if (Creature* teron = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_THERON)))
                        {
                            teron->GetMotionMaster()->MovePoint(teron->GetGUIDLow(), 692.4531f, 1149.196f, 356.0718f);
                        }
                        break;
                    case EVENT_15:
                        if (Creature* jaina = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_LADY_JAINA)))
                        {
                            jaina->CastSpell(jaina, 51347, true);
                            jaina->DespawnOrUnsummon(5000);
                        }
                        if (Creature* teron = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_THERON)))
                        {
                            teron->CastSpell(teron, 51347, true);
                            teron->DespawnOrUnsummon(5000);
                        }
                        me->DespawnOrUnsummon(5000);
                        break;
                    case EVENT_16:
                        if (Creature* teron = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE_END_THERON)))
                            teron->AI()->ZoneTalk(TEXT_GENERIC_1, 0);
                        break;
                }
            }
        }
    };
};

enum ACTION_CORUPTED_PRISON
{
    ACTION_CORUPTED_PRISON_ACTIVATE_KEY     = 1,
    ACTION_CORUPTED_PRISON_DEACTIVATE_KEY   = 2,
    ACTION_CORUPTED_PRISON_ENABLE           = 3,
};

class go_sha_of_pride_corupted_prison : public GameObjectScript
{
    public:
        go_sha_of_pride_corupted_prison() : GameObjectScript("go_sha_of_pride_corupted_prison") { }

        struct go_sha_of_pride_corupted_prisonAI : public GameObjectAI
        {
            go_sha_of_pride_corupted_prisonAI(GameObject* go) : 
                GameObjectAI(go), _enableKeyCount(0), _plrPrisonerGUID(0)
            {
                instance = go->GetInstanceScript();
                switch(go->GetEntry())
                {
                    case GO_CORRUPTED_PRISON_WEST:
                        _key[0] = GO_CORRUPTED_BUTTON_WEST_1;
                        _key[1] = GO_CORRUPTED_BUTTON_WEST_2;
                        _key[2] = GO_CORRUPTED_BUTTON_WEST_3;
                        break;
                    case GO_CORRUPTED_PRISON_EAST:
                        _key[0] = GO_CORRUPTED_BUTTON_EAST_1;
                        _key[1] = GO_CORRUPTED_BUTTON_EAST_2;
                        _key[2] = GO_CORRUPTED_BUTTON_EAST_3;
                        break;
                    case GO_CORRUPTED_PRISON_NORTH:
                        _key[0] = GO_CORRUPTED_BUTTON_NORTH_1;
                        _key[1] = GO_CORRUPTED_BUTTON_NORTH_2;
                        _key[2] = GO_CORRUPTED_BUTTON_NORTH_3;
                        break;
                    case GO_CORRUPTED_PRISON_SOUTH:
                        _key[0] = GO_CORRUPTED_BUTTON_SOUTH_1;
                        _key[1] = GO_CORRUPTED_BUTTON_SOUTH_2;
                        _key[2] = GO_CORRUPTED_BUTTON_SOUTH_3;
                        break;
                }
            }

            void SetGUID(const uint64& guid, int32 /*id = 0 */)
            {
                _enableKeyCount = 0;
                _plrPrisonerGUID =guid;
                go->EnableOrDisableGo(true, false);
                for(uint8 i = 0; i < 2; ++i)
                {
                    if (GameObject* buttons = instance->instance->GetGameObject(instance->GetData64(_key[i])))
                        buttons->AI()->DoAction(ACTION_CORUPTED_PRISON_ACTIVATE_KEY);
                }
                //last one should be activated
                if (GameObject* buttons = instance->instance->GetGameObject(instance->GetData64(_key[2])))
                    buttons->AI()->DoAction(ACTION_CORUPTED_PRISON_ENABLE);
            }

            void DoAction(const int32 param)
            {
                switch (param)
                {
                    case ACTION_CORUPTED_PRISON_DEACTIVATE_KEY:
                        --_enableKeyCount;
                        break;
                    case ACTION_CORUPTED_PRISON_ACTIVATE_KEY:
                        ++_enableKeyCount;
                        if (_enableKeyCount >= 3)
                        {
                            if (Player* player = ObjectAccessor::FindPlayer(_plrPrisonerGUID))
                            {
                                _plrPrisonerGUID = 0;

                                player->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_WEST);
                                player->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_EAST);
                                player->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_NORTH);
                                player->RemoveAurasDueToSpell(SPELL_CORRUPTED_PRISON_SOUTH);
                                go->EnableOrDisableGo(false, false);
                                for(uint8 i = 0; i < 3; ++i)
                                {
                                    if (GameObject* buttons = instance->instance->GetGameObject(instance->GetData64(_key[i])))
                                        buttons->AI()->DoAction(ACTION_CORUPTED_PRISON_DEACTIVATE_KEY);
                                }
                            }
                        }
                        break;
                }
            }

        private:
            InstanceScript* instance;
            uint64 _plrPrisonerGUID;
            uint32 _enableKeyCount;
            uint32 _key[3];
        };

        GameObjectAI* GetAI(GameObject* go) const
        {
            return new go_sha_of_pride_corupted_prisonAI(go);
        }
};

class go_sha_of_pride_corupted_prison_button : public GameObjectScript
{
    public:
        go_sha_of_pride_corupted_prison_button() : GameObjectScript("go_sha_of_pride_corupted_prison_button") { }

        struct go_sha_of_pride_corupted_prison_buttonAI : public GameObjectAI
        {
            go_sha_of_pride_corupted_prison_buttonAI(GameObject* go) : 
                GameObjectAI(go), ownerEntry(0)
            {
                go->EnableOrDisableGo(true, true);
                instance = go->GetInstanceScript();
                switch(go->GetEntry())
                {
                    case GO_CORRUPTED_BUTTON_WEST_1:
                    case GO_CORRUPTED_BUTTON_WEST_2:
                    case GO_CORRUPTED_BUTTON_WEST_3:
                        ownerEntry = GO_CORRUPTED_PRISON_WEST;
                        break;
                    case GO_CORRUPTED_BUTTON_EAST_1:
                    case GO_CORRUPTED_BUTTON_EAST_2:
                    case GO_CORRUPTED_BUTTON_EAST_3:
                        ownerEntry = GO_CORRUPTED_PRISON_EAST;
                        break;
                    case GO_CORRUPTED_BUTTON_NORTH_1:
                    case GO_CORRUPTED_BUTTON_NORTH_2:
                    case GO_CORRUPTED_BUTTON_NORTH_3:
                        ownerEntry = GO_CORRUPTED_PRISON_NORTH;
                        break;
                    case GO_CORRUPTED_BUTTON_SOUTH_1:
                    case GO_CORRUPTED_BUTTON_SOUTH_2:
                    case GO_CORRUPTED_BUTTON_SOUTH_3:
                        ownerEntry = GO_CORRUPTED_PRISON_SOUTH;
                        break;
                }

                //TMP
                //events.ScheduleEvent(ACTION_CORUPTED_PRISON_DEACTIVATE_KEY, 1000);
            }

            void DoAction(const int32 param)
            {
                switch (param)
                {
                    case ACTION_CORUPTED_PRISON_DEACTIVATE_KEY:
                        events.Reset();
                        break;
                    case ACTION_CORUPTED_PRISON_ACTIVATE_KEY:
                        events.ScheduleEvent(ACTION_CORUPTED_PRISON_DEACTIVATE_KEY, 1000);
                        break;
                    case ACTION_CORUPTED_PRISON_ENABLE:
                        if (go->GetGoState() == GO_STATE_ACTIVE_ALTERNATIVE)
                        {
                            go->EnableOrDisableGo(false, false);
                            if (GameObject* pris = instance->instance->GetGameObject(instance->GetData64(ownerEntry)))
                                pris->AI()->DoAction(ACTION_CORUPTED_PRISON_ACTIVATE_KEY);
                            return;
                        }
                        break;
                }

                if (go->GetGoState() != GO_STATE_ACTIVE_ALTERNATIVE)
                    go->EnableOrDisableGo(true, true);
            }

            void UpdateAI(uint32 diff) 
            {
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    //should be first
                    events.ScheduleEvent(ACTION_CORUPTED_PRISON_DEACTIVATE_KEY, 100);

                    // Possible Blizard do check like this
                    std::list<Player*> playerList;
                    go->GetPlayerListInGrid(playerList, 20.0f);

                    bool find = false;
                    for(std::list<Player*>::iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                    {
                        if (go->GetDistance(*itr) > 5.5f && go->GetDistance(*itr) < 7.5f 
                            && go->IsInDegreesRange((*itr)->GetPositionX(), (*itr)->GetPositionY(), 170.0f, 250.0f, true))
                            find = true;
                    }

                    if (find)
                    {
                        if (go->GetGoState() == GO_STATE_ACTIVE_ALTERNATIVE)
                        {
                            go->EnableOrDisableGo(false, false);
                            if (GameObject* pris = instance->instance->GetGameObject(instance->GetData64(ownerEntry)))
                                pris->AI()->DoAction(ACTION_CORUPTED_PRISON_ACTIVATE_KEY);
                        }
                    }else if (go->GetGoState() == GO_STATE_READY)
                    {
                        go->EnableOrDisableGo(true, true);
                        if (GameObject* pris = instance->instance->GetGameObject(instance->GetData64(ownerEntry)))
                            pris->AI()->DoAction(ACTION_CORUPTED_PRISON_DEACTIVATE_KEY);
                    }
                }
            }

        private:
            InstanceScript* instance;
            uint32 ownerEntry;
            EventMap events;
        };

        GameObjectAI* GetAI(GameObject* go) const
        {
            return new go_sha_of_pride_corupted_prison_buttonAI(go);
        }
};

class npc_sha_of_pride_manifest_of_pride : public CreatureScript
{
public:
    npc_sha_of_pride_manifest_of_pride() : CreatureScript("npc_sha_of_pride_manifest_of_pride") { }

    enum localEvent
    {
        EVENT_SPAWN                 = 1,
        EVENT_SPELL_MOCKING_BLAST   = 2,
    };

    struct npc_sha_of_pride_manifest_of_prideAI : public ScriptedAI
    {
        npc_sha_of_pride_manifest_of_prideAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            onSpawn = true;
        }

        void Reset()
        {
            events.RescheduleEvent(EVENT_SPAWN, 1000);
            events.RescheduleEvent(EVENT_SPELL_MOCKING_BLAST, 6000);
            me->AddAura(SPELL_MANIFESTATION_SPAWN, me);
        }

        bool onSpawn;
        EventMap events;
        InstanceScript* instance;

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            addPride(spell->Id, target);
        }

        void JustDied(Unit* /*killer*/)
        {
            DoCast(me, SPELL_LAST_WORD, false);
        }

        void UpdateAI(uint32 diff)
        {
            if (!onSpawn && !UpdateVictim())
                return;

            EnterEvadeIfOutOfCombatArea(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SPAWN:
                        me->RemoveAurasDueToSpell(SPELL_MANIFESTATION_SPAWN);
                        me->SetInCombatWithZone();
                        onSpawn = false;
                        break;
                    case EVENT_SPELL_MOCKING_BLAST:
                        DoCastVictim(SPELL_MOCKING_BLAST);
                        events.RescheduleEvent(EVENT_SPELL_MOCKING_BLAST, 6000);
                        break;
                }
            }         

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_manifest_of_prideAI(creature);
    }
};

class npc_sha_of_pride_reflection : public CreatureScript
{
public:
    npc_sha_of_pride_reflection() : CreatureScript("npc_sha_of_pride_reflection") { }

    enum localEvent
    {
        EVENT_SPAWN                        = 1,
        EVENT_SPELL_SELF_REFLECTION_CAST   = 2,
    };

    struct npc_sha_of_pride_reflectionAI : public ScriptedAI
    {
        npc_sha_of_pride_reflectionAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            onSpawn = true;
            SetCombatMovement(false);
        }

        void Reset()
        {
            events.RescheduleEvent(EVENT_SPAWN, 3000);
            events.RescheduleEvent(EVENT_SPELL_SELF_REFLECTION_CAST, 6000);
            me->AddAura(SPELL_SELF_REFLECTION_SPAWN, me);
        }

        bool onSpawn;
        EventMap events;
        InstanceScript* instance;

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            addPride(spell->Id, target);
        }

        void UpdateAI(uint32 diff)
        {
            if (!onSpawn && !UpdateVictim())
                return;

            EnterEvadeIfOutOfCombatArea(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SPAWN:
                        me->RemoveAurasDueToSpell(SPELL_SELF_REFLECTION_SPAWN);
                        me->SetInCombatWithZone();
                        DoCastVictim(SPELL_SELF_REFLECTION_CAST);
                        onSpawn = false;
                        SetCombatMovement(true);
                        break;
                    default:
                        break;
                }
            }

            if (!onSpawn)
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_reflectionAI(creature);
    }
};

class npc_sha_of_pride_rift_of_corruption : public CreatureScript
{
public:
    npc_sha_of_pride_rift_of_corruption() : CreatureScript("npc_sha_of_pride_rift_of_corruption") { }

    enum localEvent
    {
        EVENT_SPELL_RIFT_OF_CORRUPTION_AT   = 1,
        EVENT_SPELL_RIFT_OF_CORRUPTION_DMG  = 2,
    };

    struct npc_sha_of_pride_rift_of_corruptionAI : public ScriptedAI
    {
        npc_sha_of_pride_rift_of_corruptionAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            SetCombatMovement(false);
        }

        void Reset()    //18:37:42.000
        {
            me->CastSpell(me, SPELL_RIFT_OF_CORRUPTION, true);                //18:37:42.000
            me->CastSpell(me, SPELL_UNSTABLE_CORRUPTION, true);               //18:37:42.000 /for check where is player.
            events.RescheduleEvent(EVENT_SPELL_RIFT_OF_CORRUPTION_AT, 2000);  //18:37:44.000
            events.RescheduleEvent(EVENT_SPELL_RIFT_OF_CORRUPTION_DMG, 5000); //18:37:47.000
        }

        bool onSpawn;
        EventMap events;
        InstanceScript* instance;

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            addPride(spell->Id, target);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->GetDistance(who) > 2.0f || who->HasAura(SPELL_WEAKENED_RESOLVE))
                return;

            who->CastSpell(who, SPELL_WEAKENED_RESOLVE, true);
            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SPELL_RIFT_OF_CORRUPTION_AT:
                        me->CastSpell(me, SPELL_RIFT_OF_CORRUPTION_AT, true);
                        break;
                    case EVENT_SPELL_RIFT_OF_CORRUPTION_DMG:
                        if (Creature * sha = instance->instance->GetCreature(instance->GetData64(NPC_SHA_OF_PRIDE)))
                        {
                            if (Unit* target = sha->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                me->CastSpell(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), SPELL_RIFT_OF_CORRUPTION_DMG);
                        }
                        events.RescheduleEvent(EVENT_SPELL_RIFT_OF_CORRUPTION_DMG, 5000); //18:37:47.000
                        break;
                    default:
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sha_of_pride_rift_of_corruptionAI(creature);
    }
};

class spell_sha_of_pride_imprison : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_imprison() : SpellScriptLoader("spell_sha_of_pride_imprison") { }

        class spell_sha_of_pride_imprison_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_of_pride_imprison_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                //Better work is on EVENT_SPELL_CORRUPTED_PRISON. No need use it.
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_of_pride_imprison_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_of_pride_imprison_SpellScript();
        }
};

class spell_sha_of_pride_self_reflection : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_self_reflection() : SpellScriptLoader("spell_sha_of_pride_self_reflection") { }


        class spell_sha_of_pride_self_reflection_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_pride_self_reflection_AuraScript);
            std::set<uint64> alreadyHitGUID;

            void OnTick(AuraEffect const* aurEff)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                std::list<HostileReference*> const& threatlist = caster->getThreatManager().getThreatList();
                if (threatlist.empty())
                    return;

                Unit *target = NULL;
                int32 current_power = 0;
                Unit *selected = NULL;

                for (std::list<HostileReference*>::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
                {
                    target = (*itr)->getTarget();
                    if (target->GetTypeId() != TYPEID_PLAYER || alreadyHitGUID.find(target->GetGUID()) != alreadyHitGUID.end())
                        continue;

                    if (target->GetPower(POWER_ALTERNATE_POWER) >= current_power)
                    {
                        current_power = target->GetPower(POWER_ALTERNATE_POWER);
                        selected = target;
                    }
                }

                if (selected)
                {
                    alreadyHitGUID.insert(selected->GetGUID());
                    caster->SummonCreature(NPC_REFLECTION, *selected, TEMPSUMMON_DEAD_DESPAWN);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_of_pride_self_reflection_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_pride_self_reflection_AuraScript();
        }
};

class player_spell_sha_of_pride_self_overcome_mcAI : public PlayerAI
{
    public:
        player_spell_sha_of_pride_self_overcome_mcAI(Player* player, Creature* c) : PlayerAI(player, c)
        {
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;
            DoMeleeAttackIfReady();
        }
};

class spell_sha_of_pride_self_overcome_mc : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_self_overcome_mc() : SpellScriptLoader("spell_sha_of_pride_self_overcome_mc") { }

        class spell_sha_of_pride_self_overcome_mc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_pride_self_overcome_mc_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                    return;

                oldAI = GetTarget()->GetAI();
                GetTarget()->SetAI(new player_spell_sha_of_pride_self_overcome_mcAI(GetTarget()->ToPlayer(), GetCaster()->ToCreature()));
                oldAIState = GetTarget()->IsAIEnabled;
                GetTarget()->IsAIEnabled = true;
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                    return;

                GetTarget()->SetDisabledCurrentAI();
                GetTarget()->SetAI(oldAI);
                GetTarget()->IsAIEnabled = oldAIState;
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_sha_of_pride_self_overcome_mc_AuraScript::OnApply, EFFECT_0, SPELL_AURA_AOE_CHARM, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_of_pride_self_overcome_mc_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_AOE_CHARM, AURA_EFFECT_HANDLE_REAL);
            }

            UnitAI* oldAI;
            bool oldAIState;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_pride_self_overcome_mc_AuraScript();
        }
};

class spell_sha_of_pride_projection : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_projection() : SpellScriptLoader("spell_sha_of_pride_projection") { }

        class spell_sha_of_pride_projection_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_pride_projection_AuraScript);

            float x, y ,z;
            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                    return;

                GetTarget()->GetRandomPoint(*GetTarget(), 15.0f, x, y, z);
                GetTarget()->CastSpell(x, y, z, SPELL_PROJECTION_MARKER, false);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetDistance(x, y, z) > 2.0f)
                    GetTarget()->CastSpell(GetTarget(), SPELL_PROJECTION_DMG, false);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_sha_of_pride_projection_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_of_pride_projection_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_pride_projection_AuraScript();
        }
};

class spell_sha_of_pride_gift_of_titans : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_gift_of_titans() : SpellScriptLoader("spell_sha_of_pride_gift_of_titans") { }

        class spell_sha_of_pride_gift_of_titans_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_pride_gift_of_titans_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster() && GetCaster()->ToCreature())
                    GetCaster()->ToCreature()->AI()->SetGUID(GetTarget()->GetGUID());
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_of_pride_gift_of_titans_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_MOD_SCALE_2, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_pride_gift_of_titans_AuraScript();
        }
};

class spell_sha_of_pride_gift_of_titans_ckecker : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_gift_of_titans_ckecker() : SpellScriptLoader("spell_sha_of_pride_gift_of_titans_ckecker") { }

        class spell_sha_of_pride_gift_of_titans_ckecker_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_of_pride_gift_of_titans_ckecker_SpellScript);

            //! Should work by different way.
            void SelectTarget(std::list<WorldObject*>& unitList)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                InstanceScript* instance = caster->GetInstanceScript();
                if (!instance)
                    return;

                uint8 c = 0;
                for(std::list<WorldObject*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                {
                    if (caster->GetDistance(*itr) <= 8.0f)
                        ++c;
                }
                if (Creature * norush = instance->instance->GetCreature(instance->GetData64(NPC_SHA_NORUSHEN)))
                    norush->AI()->SetData(EVENT_SPELL_GIFT_OF_THE_TITANS, c);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_of_pride_gift_of_titans_ckecker_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_of_pride_gift_of_titans_ckecker_SpellScript();
        }
};

//Mark of Arrogance
class spell_sha_of_pride_mark_of_arrogance : public SpellScriptLoader
{
    public:
        spell_sha_of_pride_mark_of_arrogance() : SpellScriptLoader("spell_sha_of_pride_mark_of_arrogance") { }

        class spell_sha_of_pride_mark_of_arrogance_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_pride_mark_of_arrogance_AuraScript);

            void HandleDispel(DispelInfo* dispelInfo)
            {
                if (Unit* dispeller = dispelInfo->GetDispeller())
                    addPride(GetSpellInfo()->Id, dispeller);
            }

            void Register()
            {
                AfterDispel += AuraDispelFn(spell_sha_of_pride_mark_of_arrogance_AuraScript::HandleDispel);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_pride_mark_of_arrogance_AuraScript();
        }
};

void AddSC_boss_sha_of_pride()
{
    new boss_sha_of_pride();
    new npc_sha_of_pride_norushen();
    new npc_sha_of_pride_lowerwalker();
    new go_sha_of_pride_corupted_prison();
    new go_sha_of_pride_corupted_prison_button();
    new npc_sha_of_pride_manifest_of_pride();
    new npc_sha_of_pride_reflection();
    new npc_sha_of_pride_rift_of_corruption();
    new spell_sha_of_pride_imprison();
    new spell_sha_of_pride_self_reflection();
    new spell_sha_of_pride_self_overcome_mc();
    new spell_sha_of_pride_projection();
    new spell_sha_of_pride_gift_of_titans();
    new spell_sha_of_pride_gift_of_titans_ckecker();
    new spell_sha_of_pride_mark_of_arrogance();
}