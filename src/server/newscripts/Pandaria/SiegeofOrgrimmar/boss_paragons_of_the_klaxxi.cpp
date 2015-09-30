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
    //Special spells
    //Palladin
    SPELL_SHIELD_OF_THE_RIGHTEOUS      = 132403,
    //Warrior
    SPELL_SHIELD_BLOCK                 = 132404,
    //Druid
    SPELL_SAVAGE_DEFENSE               = 132402,
    //Monk
    SPELL_ELUSIVE_BREW                 = 115308,
    //Death Knight
    SPELL_BLOOD_SHIELD                 = 77535,
    SPELL_READY_TO_FIGHT               = 143542,
    //

    //Hisek
    SPELL_MULTI_SHOT                   = 144839,

    //Skeer
    SPELL_HEVER_OF_FOES                = 143273,
    SPELL_BLODDLETTING                 = 143280,
    SPELL_BLOODLETTING_SUM             = 143280,
    SPELL_BLODDLETTING_BUFF            = 143320,

    //Rikkal
    SPELL_PREY                         = 144286,
    SPELL_MAD_SCIENTIST_AURA           = 143277,
    SPELL_INJECTION                    = 143339,
    SPELL_INJECTION_SUM                = 143340,
    SPELL_MUTATE                       = 143337,
    SPELL_FAULTY_MUTATION              = 148589,
    SPELL_FAULTY_MUTATION_KILL         = 148587,

    //Kilruk
    SPELL_RAZOR_SHARP_BLADES           = 142918,
    SPELL_GOUGE                        = 143939,
    SPELL_MUTILATE                     = 143941,
    SPELL_REAVE_PRE                    = 148677,
    SPELL_REAVE                        = 148681,

    //Xaril
    SPELL_TENDERIZING_STRIKES          = 142927,
    SPELL_TENDERIZING_STRIKES_DMG      = 142929,
    SPELL_CAUSTIC_BLOOD                = 142315,
    SPELL_BLOODY_EXPLOSION             = 142317,
    SPELL_TOXIC_INJECTION              = 142528,
    //Toxins
    SPELL_TOXIN_RED                    = 142533,
    SPELL_DELAYED_CATALYST_RED         = 142936,
    SPELL_TOXIN_BLUE                   = 142532,
    SPELL_DELAYED_CATALYST_BLUE        = 142935,
    SPELL_TOXIN_YELLOW                 = 142534,
    SPELL_DELAYED_CATALYST_YELLOW      = 142937,
    //Heroic Toxins
    SPELL_TOXIN_ORANGE                 = 142547,
    SPELL_DELAYED_CATALYST_ORANGE      = 142938,
    SPELL_TOXIN_PURPLE                 = 142548,
    SPELL_DELAYED_CATALYST_PURPLE      = 142939,
    SPELL_TOXIN_GREEN                  = 142549,
    SPELL_DELAYED_CATALYST_GREEN       = 142940,

    //Korven
    SPELL_SHIELD_BASH                  = 143974,
    SPELL_VICIOUS_ASSAULT              = 143980,
    SPELL_ENCASE_IN_AMBER              = 142564,
    SPELL_AMBER_VISUAL                 = 144120,

    //Karoz
    SPELL_FLASH                        = 143704,
    SPELL_HURL_AMBER                   = 143733,

    //Special
    SPELL_AURA_VISUAL_FS               = 143548, 
    SPELL_AURA_ENRAGE                  = 146983,
    SPELL_ENRAGE                       = 146982,
    SPELL_PARAGONS_PURPOSE_HEAL        = 143483,
    SPELL_PARAGONS_PURPOSE_DMG         = 143482,

    //Buffs
    //Rikkal
    SPELL_GENE_SPLICE                  = 143372,
    SPELL_MAD_SCIENTIST                = 141857,
    //Hisek
    SPELL_COMPOUND_EYE                 = 141852,
    //Xaril
    SPELL_VAST_APOTHECARIAL_KNOWLEDGE  = 141856,
    SPELL_APOTHECARY_VOLATILE_POULTICE = 142598, //caster 
    SPELL_VOLATILE_POULTICE            = 142877, //target
    SPELL_VOLATILE_POULTICE_HEAL       = 142897,
    //

    //Amber Parasite
    SPELL_FEED                         = 143362,
    SPELL_HUNGER                       = 143358,
    SPELL_REGENERATE                   = 143356,
    SPELL_GENETIC_MOD                  = 143355,
};

enum sEvents
{
    EVENT_START_KLAXXI                 = 1,
    EVENT_CHECK                        = 2,

    //Skeer
    EVENT_BLODDLETTING                 = 3,

    //Kilruk
    EVENT_GOUGE                        = 4,
    EVENT_REAVE                        = 5,

    //Hisek
    EVENT_MULTI_SHOT                   = 6,

    //Rikkal
    EVENT_MUTATE                       = 7,
    EVENT_INJECTION                    = 8,

    //Xaril
    EVENT_TOXIC_INJECTION              = 9,
    EVENT_CATALYST                     = 10,

    //Karoz
    EVENT_HURL_AMBER                   = 11,

    //Amber Parasite
    EVENT_FEED                         = 12,
    EVENT_REGENERATE                   = 13,

    //Blood
    EVENT_FIND_LOW_HP_KLAXXI           = 14,
    EVENT_CHECK_DIST_TO_KLAXXI         = 15,
    EVENT_CHECK_PLAYER                 = 16,
    EVENT_RE_ATTACK                    = 17,
};

enum sActions
{
    ACTION_KLAXXI_START                = 1,
    ACTION_ENCASE_IN_AMBER             = 2,
    ACTION_RE_ATTACK                   = 3,
};

uint32 removeaurasentry[4] =
{
    SPELL_READY_TO_FIGHT,
    SPELL_AURA_ENRAGE,
    SPELL_ENRAGE,
    SPELL_PARAGONS_PURPOSE_DMG,
};

uint32 EvadeSpells[5] =
{
    SPELL_SHIELD_OF_THE_RIGHTEOUS,
    SPELL_SHIELD_BLOCK,
    SPELL_SAVAGE_DEFENSE,
    SPELL_ELUSIVE_BREW,
    SPELL_BLOOD_SHIELD,
};

Position bloodsumpos[3] =
{
    { 1591.96f, -5650.64f, -314.7395f, 4.4687f },
    { 1619.42f, -5682.49f, -314.7208f, 3.1806f },
    { 1549.81f, -5705.55f, -314.6497f, 0.5652f },
};

uint32 klaxxientry[9] =
{
    NPC_KILRUK,
    NPC_XARIL,
    NPC_KAZTIK,
    NPC_KORVEN,
    NPC_IYYOKYK,
    NPC_KAROZ,
    NPC_SKEER,
    NPC_RIKKAL,
    NPC_HISEK,
};

uint32 toxinlist[6] =
{
    SPELL_TOXIN_BLUE,
    SPELL_TOXIN_RED,
    SPELL_TOXIN_YELLOW,
    SPELL_TOXIN_ORANGE,
    SPELL_TOXIN_PURPLE,
    SPELL_TOXIN_GREEN,
};

uint32 catalystlist[6] =
{
    SPELL_DELAYED_CATALYST_RED,
    SPELL_DELAYED_CATALYST_BLUE,
    SPELL_DELAYED_CATALYST_YELLOW,
    SPELL_DELAYED_CATALYST_ORANGE,
    SPELL_DELAYED_CATALYST_PURPLE,
    SPELL_DELAYED_CATALYST_GREEN,
};

//71628
class npc_amber_piece : public CreatureScript
{
public:
    npc_amber_piece() : CreatureScript("npc_amber_piece") {}

    struct npc_amber_pieceAI : public ScriptedAI
    {
        npc_amber_pieceAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;

        void Reset()
        {
            DoCast(me, SPELL_AURA_VISUAL_FS, true);
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}

        void OnSpellClick(Unit* clicker)
        {
            if (me->HasAura(SPELL_AURA_VISUAL_FS))
            {
                me->RemoveAurasDueToSpell(SPELL_AURA_VISUAL_FS);
                if (Creature* ck = me->GetCreature(*me, instance->GetData64(NPC_KLAXXI_CONTROLLER)))
                    ck->AI()->DoAction(ACTION_KLAXXI_START);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amber_pieceAI(creature);
    }
};

//71592
class npc_klaxxi_controller : public CreatureScript
{
public:
    npc_klaxxi_controller() : CreatureScript("npc_klaxxi_controller") {}

    struct npc_klaxxi_controllerAI : public ScriptedAI
    {
        npc_klaxxi_controllerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            if (instance && instance->GetBossState(DATA_KLAXXI) != DONE)
                instance->SetBossState(DATA_KLAXXI, NOT_STARTED);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void DespawnSummons()
        {
            std::list<Creature*> sumlist;
            sumlist.clear();
            GetCreatureListWithEntryInGrid(sumlist, me, NPC_AMBER_PARASITE, 150.0f);
            if (!sumlist.empty())
                for (std::list<Creature*>::const_iterator itr = sumlist.begin(); itr != sumlist.end(); itr++)
                    (*itr)->DespawnOrUnsummon();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
            case ACTION_KLAXXI_START:
                events.ScheduleEvent(EVENT_CHECK, 1000);
                events.ScheduleEvent(EVENT_START_KLAXXI, 5000);
                break;
            case ACTION_KLAXXI_DONE:
                DespawnSummons();
                events.Reset();
                if (Creature* ap = me->GetCreature(*me, instance->GetData64(NPC_AMBER_PIECE)))
                    me->Kill(ap, true);
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
                case EVENT_START_KLAXXI:
                    instance->SetBossState(DATA_KLAXXI, IN_PROGRESS);
                    break;
                case EVENT_CHECK:
                    if (instance->IsWipe())
                    {
                        DespawnSummons();
                        instance->SetBossState(DATA_KLAXXI, NOT_STARTED);
                    }
                    else
                        events.ScheduleEvent(EVENT_CHECK, 1000);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_klaxxi_controllerAI(creature);
    }
};

class boss_paragons_of_the_klaxxi : public CreatureScript
{
    public:
        boss_paragons_of_the_klaxxi() : CreatureScript("boss_paragons_of_the_klaxxi") {}

        struct boss_paragons_of_the_klaxxiAI : public ScriptedAI
        {
            boss_paragons_of_the_klaxxiAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                instance = creature->GetInstanceScript();
                me->SetDisableGravity(true);
                me->SetCanFly(true);
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            uint32 checkklaxxi, healcooldown;
            bool healready;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
                for (uint8 n = 0; n < 4; n++)
                    me->RemoveAurasDueToSpell(removeaurasentry[n]);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                checkklaxxi = 0;
                healcooldown = 0;
                healready = true;
                switch (me->GetEntry())
                {
                case NPC_SKEER:
                case NPC_RIKKAL:
                case NPC_HISEK:
                    DoCast(me, SPELL_READY_TO_FIGHT, true); 
                    break;
                default:
                    break;
                }
            }

            void JustSummoned(Creature* sum)
            {
                summons.Summon(sum);
            }

            void EnterCombat(Unit* who)
            {
                switch (me->GetEntry())
                {
                case NPC_KILRUK:
                    DoCast(me, SPELL_RAZOR_SHARP_BLADES, true);
                    events.ScheduleEvent(EVENT_GOUGE, 10000);
                    events.ScheduleEvent(EVENT_REAVE, 33000);
                    break;
                case NPC_XARIL:
                    DoCast(me, SPELL_TENDERIZING_STRIKES, true);
                    DoCast(me, SPELL_TOXIC_INJECTION, true);
                    break;
                case NPC_SKEER:
                    DoCast(me, SPELL_HEVER_OF_FOES, true);
                    events.ScheduleEvent(EVENT_BLODDLETTING, 10000);
                    break;
                case NPC_RIKKAL:
                    DoCast(me, SPELL_MAD_SCIENTIST_AURA, true);
                    events.ScheduleEvent(EVENT_MUTATE, 10000); //test
                    events.ScheduleEvent(EVENT_INJECTION, 10000); //test
                    break;
                case NPC_KAZTIK:
                    break;
                case NPC_KORVEN:
                    checkklaxxi = 2000;
                    break;
                case NPC_IYYOKYK:
                    break;
                case NPC_KAROZ:
                    events.ScheduleEvent(EVENT_HURL_AMBER, 5000);
                    break;
                case NPC_HISEK:
                    events.ScheduleEvent(EVENT_MULTI_SHOT, 2000);
                    break;
                }
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                case ACTION_KLAXXI_IN_PROGRESS:
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    me->GetMotionMaster()->MoveJump(1582.4f, -5684.9f, -313.635f, 15.0f, 15.0f, 1);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    break;
                case ACTION_ENCASE_IN_AMBER:
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    break;
                case ACTION_RE_ATTACK:
                    me->SetFullHealth();
                    if (me->GetEntry() != NPC_HISEK)
                        me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 150.0f);
                    break;
                }
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type == EFFECT_MOTION_TYPE)
                {
                    switch (pointId)
                    {
                    case 1:
                        me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        me->RemoveAurasDueToSpell(SPELL_READY_TO_FIGHT);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        if (me->GetEntry() != NPC_HISEK) //Test
                            me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f); 
                        instance->SetData(DATA_BUFF_NEXT_KLAXXI, 0);
                        break;
                    case 2:
                    {
                        std::list<Player*>pllist;
                        GetPlayerListInGrid(pllist, me, 150.0f);
                        if (!pllist.empty())
                        {
                            for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                            {
                                if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK)
                                {
                                    me->SetFacingToObject(*itr);
                                    DoCast(*itr, SPELL_HURL_AMBER, true);
                                }
                            }
                        }
                        events.ScheduleEvent(EVENT_RE_ATTACK, 2000);
                        break;
                    }
                    case 3:
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f);
                        break;
                    }
                }
            }

            void JustDied(Unit* killer)
            {
                if (killer->ToPlayer() && instance)
                {
                    if (instance->GetData(DATA_SEND_KLAXXI_DIE_COUNT) < 8)
                    {
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        me->SetLootRecipient(NULL);
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        instance->SetData(DATA_INTRO_NEXT_KLAXXI, 0);
                    }
                    else
                        instance->SetBossState(DATA_KLAXXI, DONE);
                }
            }

            void OnSpellClick(Unit* clicker)
            {
                if (Player* pl = clicker->ToPlayer())
                {
                    switch (me->GetEntry())
                    {
                    case NPC_RIKKAL:
                        pl->CastSpell(pl, SPELL_MAD_SCIENTIST, true);
                        break;
                    case NPC_HISEK:
                        if (pl->GetRoleForGroup(pl->GetSpecializationId(pl->GetActiveSpec())) == ROLES_DPS)
                            pl->CastSpell(pl, SPELL_COMPOUND_EYE, true);
                        break;
                    case NPC_XARIL:
                        if (pl->GetRoleForGroup(pl->GetSpecializationId(pl->GetActiveSpec())) == ROLES_HEALER)
                            pl->CastSpell(pl, SPELL_VAST_APOTHECARIAL_KNOWLEDGE, true);
                        break;
                    default:
                        break;
                    }
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (healcooldown)
                {
                    if (healcooldown <= diff)
                    {
                        healcooldown = 0;
                        healready = true;
                    }
                    else
                        healcooldown -= diff;
                }

                events.Update(diff);

                if (me->HasAura(SPELL_ENCASE_IN_AMBER) || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (checkklaxxi <= diff)
                {
                    if (healready)
                    {
                        for (uint8 n = 0; n < 9; n++)
                        {
                            if (Creature* klaxxi = me->FindNearestCreature(klaxxientry[n], 150.0f, true))
                            {
                                if (klaxxi->isInCombat())
                                {
                                    if (klaxxi->HealthBelowPct(50))
                                    {
                                        healready = false;
                                        DoCast(klaxxi, SPELL_ENCASE_IN_AMBER, true);
                                        healcooldown = 90000;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    checkklaxxi = 2000;
                }
                else
                    checkklaxxi -= diff;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    //Hisek
                    case EVENT_MULTI_SHOT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                            DoCast(target, SPELL_MULTI_SHOT);
                        events.ScheduleEvent(EVENT_MULTI_SHOT, 4000);
                        break;
                    //Skeer
                    case EVENT_BLODDLETTING:
                        if (me->getVictim())
                            DoCastVictim(SPELL_BLODDLETTING);
                        events.ScheduleEvent(EVENT_BLODDLETTING, 30000);
                        break;
                    //Kilruk
                    case EVENT_GOUGE:
                        if (me->getVictim())
                            DoCastVictim(SPELL_GOUGE);
                        events.ScheduleEvent(EVENT_GOUGE, 10000);
                        break;
                    case EVENT_REAVE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, /*1,*/0, 75.0f, true))
                        {
                            me->AttackStop();
                            me->SetReactState(REACT_PASSIVE);
                            DoCast(target, SPELL_REAVE);
                        }
                        events.ScheduleEvent(EVENT_REAVE, 33000);
                        break;
                    //Rikkal
                    case EVENT_MUTATE: //transform scorpion
                    {
                        std::list<Player*> pllist;
                        pllist.clear();
                        GetPlayerListInGrid(pllist, me, 100.0f);
                        uint8 count = 0;
                        uint8 maxcount = me->GetMap()->Is25ManRaid() ? 3 : 1;                   
                        if (!pllist.empty() && maxcount)
                        {
                            for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                            {
                                if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && !(*itr)->HasAura(SPELL_MUTATE))
                                {
                                    count++;
                                    DoCast(*itr, SPELL_MUTATE, true);
                                }

                                if (count >= maxcount)
                                    break;
                            }
                        }
                        events.ScheduleEvent(EVENT_MUTATE, 30000); //test
                        break;
                    }
                    case EVENT_INJECTION:
                        if (me->getVictim())
                        {
                            for (uint8 n = 0; n < 5; n++)
                            {
                                if (me->getVictim()->HasAura(EvadeSpells[n]))
                                {
                                    events.ScheduleEvent(EVENT_INJECTION, 10000); //test
                                    return;
                                }
                            }
                            DoCast(me->getVictim(), SPELL_INJECTION);
                        }  
                        events.ScheduleEvent(EVENT_INJECTION, 10000); //test
                        break;
                    //Xaril
                    case EVENT_TOXIC_INJECTION:
                    {
                        uint8 listcount = 0;
                        uint8 maxbuffcount = 0;
                        uint8 buffcount = 0;
                        std::list<Player*>_pllist;
                        std::vector<Player*>pllist;
                        _pllist.clear();
                        pllist.clear();
                        me->GetPlayerListInGrid(_pllist, 150.0f);
                        if (!_pllist.empty())
                        {
                            for (std::list<Player*>::const_iterator itr = _pllist.begin(); itr != _pllist.end(); ++itr)
                                pllist.push_back(*itr);

                            uint8 bluecount = me->GetMap()->Is25ManRaid() ? 3 : 2;

                            if (me->GetMap()->IsHeroic())
                            {
                                for (uint8 n = 0; n < 3; ++n)
                                {
                                    for (std::vector<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                                    {
                                        if (!(*itr)->HasAura(toxinlist[n + 3]))
                                        {
                                            me->CastSpell(*itr, toxinlist[n + 3], true);
                                            break;
                                        }
                                    }
                                }
                                listcount = me->GetMap()->Is25ManRaid() ? 22 : 7;
                                maxbuffcount = listcount == 22 ? urand(7, 8) : urand(2, 3);
                                for (uint8 b = 0; b < 3; ++b)
                                {
                                    for (std::vector<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                                    {
                                        if (!(*itr)->HasAura(toxinlist[3]) && !(*itr)->HasAura(toxinlist[4]) && !(*itr)->HasAura(toxinlist[5]))
                                        {
                                            me->CastSpell(*itr, toxinlist[b + 3], true);
                                            buffcount++;
                                            if (buffcount == maxbuffcount)
                                            {
                                                switch (maxbuffcount)
                                                {
                                                case 2:
                                                    maxbuffcount = 3;
                                                    break;
                                                case 3:
                                                    maxbuffcount = 2;
                                                    break;
                                                case 7:
                                                    maxbuffcount = 8;
                                                    break;
                                                case 8:
                                                    maxbuffcount = 7;
                                                    break;
                                                }
                                                maxbuffcount = urand(2, 3);
                                                buffcount = 0;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                //Blue toxin
                                for (uint8 b = 0; b < bluecount; ++b)
                                    me->CastSpell(pllist[b], toxinlist[0], true);

                                listcount = me->GetMap()->Is25ManRaid() ? 22 : 8;
                                maxbuffcount = listcount == 22 ? urand(10, 12) : urand(3, 5);
                                for (uint8 b = 1; b < 3; ++b)
                                {
                                    for (std::vector<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                                    {
                                        if (!(*itr)->HasAura(toxinlist[0]) && !(*itr)->HasAura(toxinlist[1]) && !(*itr)->HasAura(toxinlist[2]))
                                        {
                                            me->CastSpell(*itr, toxinlist[b], true);
                                            buffcount++;
                                            if (buffcount == maxbuffcount)
                                            {
                                                switch (maxbuffcount)
                                                {
                                                case 3:
                                                    maxbuffcount = 5;
                                                    break;
                                                case 4:
                                                    maxbuffcount = 4;
                                                    break;
                                                case 5:
                                                    maxbuffcount = 3;
                                                    break;
                                                case 10:
                                                    maxbuffcount = 12;
                                                    break;
                                                case 11:
                                                    maxbuffcount = 11;
                                                    break;
                                                case 12:
                                                    maxbuffcount = 10;
                                                    break;
                                                }
                                                buffcount = 0;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case EVENT_CATALYST:
                    {
                        uint32 n = me->GetMap()->IsHeroic() ? urand(0, 2) : urand(3, 5);
                        DoCast(me, catalystlist[n], true);
                        break;
                    }
                    case EVENT_HURL_AMBER:
                        me->AttackStop();
                        me->SetReactState(REACT_PASSIVE);
                        if (Creature* ab = me->FindNearestCreature(NPC_AMBER_BOMB, 110.0f, true))
                            me->GetMotionMaster()->MoveJump(ab->GetPositionX(), ab->GetPositionY(), ab->GetPositionZ() + 5.0f, 15.0f, 15.0f, 2);
                        break;
                    case EVENT_RE_ATTACK:
                        me->GetMotionMaster()->MoveJump(1582.4f, -5684.9f, -313.635f, 15.0f, 15.0f, 3);
                        break;
                    }
                }
                if (me->GetEntry() != NPC_HISEK)
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_paragons_of_the_klaxxiAI(creature);
        }
};

//71542
class npc_klaxxi_blood : public CreatureScript
{
public:
    npc_klaxxi_blood() : CreatureScript("npc_klaxxi_blood") {}

    struct npc_klaxxi_bloodAI : public ScriptedAI
    {
        npc_klaxxi_bloodAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            klaxxiGuid = 0;
        }

        InstanceScript* instance;
        EventMap events;
        uint64 klaxxiGuid;
        uint32 checktarget;

        void Reset()
        {
            events.ScheduleEvent(EVENT_FIND_LOW_HP_KLAXXI, 500);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            me->getThreatManager().addThreat(attacker, 0.0f);
        }

        uint64 GetLowestHpKlaxxi()
        {     
            std::list<Creature*> klaxxilist;
            klaxxilist.clear();
            float lasthppct = 100;
            for (uint8 n = 0; n < 9; n++)
            {   //only active Klaxxi
                if (Creature* klaxxi = me->FindNearestCreature(klaxxientry[n], 150.0f, true))
                    if (klaxxi->isInCombat())
                        klaxxilist.push_back(klaxxi);
            }

            if (!klaxxilist.empty())
            {   //get lowest hp value from Klaxxi
                for (std::list<Creature*>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                {
                    if ((*itr)->GetHealthPct() < lasthppct)
                        lasthppct = (*itr)->GetHealthPct();
                }
                //find Klaxxi with this value
                for (std::list<Creature*>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                    if ((*itr)->GetHealthPct() <= lasthppct)
                        return ((*itr)->GetGUID());
            }
            return 0;
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (pointId == 0)
                {
                    events.Reset();
                    me->StopMoving();
                    me->GetMotionMaster()->Clear(false);
                    if (Creature* klaxxi = me->GetCreature(*me, klaxxiGuid))
                    {
                        if (klaxxi->isAlive())
                        {
                            klaxxi->SetHealth(klaxxi->GetHealth() + me->GetHealth());
                            me->DespawnOrUnsummon();
                        }
                    }
                }
            }
        }
        
        void CheckDistToKlaxxi()
        {
            if (Creature* klaxxi = me->GetCreature(*me, klaxxiGuid))
            {
                if (klaxxi->isAlive())
                {
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MoveCharge(klaxxi->GetPositionX(), klaxxi->GetPositionY(), me->GetPositionZ(), 4.0f, 0);
                    events.ScheduleEvent(EVENT_CHECK_DIST_TO_KLAXXI, 1000);
                    return;
                }
            }
            me->StopMoving();
            me->GetMotionMaster()->Clear(false);
            events.ScheduleEvent(EVENT_FIND_LOW_HP_KLAXXI, 1000);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_FIND_LOW_HP_KLAXXI:
                    if (Creature* klaxxi = me->GetCreature(*me, GetLowestHpKlaxxi()))
                    {
                        klaxxiGuid = klaxxi->GetGUID();
                        me->GetMotionMaster()->MoveCharge(klaxxi->GetPositionX(), klaxxi->GetPositionY(), me->GetPositionZ(), 4.0f, 0);
                        events.ScheduleEvent(EVENT_CHECK_DIST_TO_KLAXXI, 1000);
                    }
                    else
                        events.ScheduleEvent(EVENT_FIND_LOW_HP_KLAXXI, 1000);
                    break;
                case EVENT_CHECK_DIST_TO_KLAXXI:
                    CheckDistToKlaxxi();
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_klaxxi_bloodAI(creature);
    }
};

//71578
class npc_amber_parasite : public CreatureScript
{
public:
    npc_amber_parasite() : CreatureScript("npc_amber_parasite") {}

    struct npc_amber_parasiteAI : public ScriptedAI
    {
        npc_amber_parasiteAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
        }

        InstanceScript* instance;
        EventMap events;
        uint64 targetGuid;
        uint32 checktarget;
        bool fullhealth;

        void Reset()
        {
            DoCast(me, SPELL_GENETIC_MOD, true);
            targetGuid  = 0;
            checktarget = 0;
            fullhealth = true;
            SetHungerTarget();
        }

        void EnterCombat(Unit* who)
        {
            events.ScheduleEvent(EVENT_FEED, 5000);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (attacker->GetGUID() != targetGuid)
                me->getThreatManager().addThreat(attacker, 0.0f);

            if (fullhealth && HealthBelowPct(50))
            {
                fullhealth = false;
                events.ScheduleEvent(EVENT_REGENERATE, 1000);
            }
        }

        void SpellHit(Unit* caster, SpellInfo const *spell)
        {
            if (spell->Id == SPELL_PREY && caster->HasAura(SPELL_FAULTY_MUTATION))
                caster->RemoveAurasDueToSpell(SPELL_FAULTY_MUTATION);
        }

        void JustDied(Unit* killer)
        {
            if (Player* pl = me->GetPlayer(*me, targetGuid))
                if (pl->isAlive())
                    pl->RemoveAurasDueToSpell(SPELL_HUNGER);
        }

        void SetHungerTarget()
        {
            std::list<Player*> pllist;
            pllist.clear();
            GetPlayerListInGrid(pllist, me, 150.0f);
            if (!pllist.empty())
            {
                for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                {
                    if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && !(*itr)->HasAura(SPELL_HUNGER))
                    {
                        DoCast(*itr, SPELL_HUNGER, true);
                        targetGuid = (*itr)->GetGUID();
                        me->AddThreat((*itr), 50000000.0f);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->TauntApply((*itr));
                        break;
                    }
                }
            }
            checktarget = 1500;
        }

        void UpdateAI(uint32 diff)
        {
            if (checktarget)
            {
                if (checktarget <= diff)
                {
                    Player* pl = me->GetPlayer(*me, targetGuid);
                    if (!pl || !pl->isAlive())
                    {
                        checktarget = 0;
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        SetHungerTarget();
                    }
                }
                else
                    checktarget -= diff;
            }
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_FEED:
                    if (me->getVictim())
                        DoCastVictim(SPELL_FEED);
                    events.ScheduleEvent(EVENT_FEED, 15000);
                    break;
                case EVENT_REGENERATE:
                    DoCast(me, SPELL_REGENERATE);
                    events.ScheduleEvent(EVENT_REGENERATE, 10000);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amber_parasiteAI(creature);
    }
};

//71407
class npc_amber : public CreatureScript
{
public:
    npc_amber() : CreatureScript("npc_amber") {}

    struct npc_amberAI : public ScriptedAI
    {
        npc_amberAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            DoCast(me, SPELL_AMBER_VISUAL, true);
            targetGuid = 0;
        }

        InstanceScript* instance;
        uint64 targetGuid;

        void Reset(){}

        void IsSummonedBy(Unit* summoner)
        {
            targetGuid = summoner->GetGUID();
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void JustDied(Unit* killer)
        {
            if (Creature* klaxxi = me->GetCreature(*me, targetGuid))
            {
                if (GameObject* it = me->FindNearestGameObject(GO_ICE_TOMB, 3.0f))
                    it->Delete();
                klaxxi->RemoveAurasDueToSpell(SPELL_ENCASE_IN_AMBER, 0, 0, AURA_REMOVE_BY_EXPIRE);
                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amberAI(creature);
    }
};

//71413
class npc_amber_player : public CreatureScript
{
public:
    npc_amber_player() : CreatureScript("npc_amber_player") {}

    struct npc_amber_playerAI : public ScriptedAI
    {
        npc_amber_playerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
            DoCast(me, SPELL_AMBER_VISUAL, true);
        }

        InstanceScript* instance;

        void Reset(){}

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amber_playerAI(creature);
    }
};

//143939
class spell_klaxxi_gouge: public SpellScriptLoader
{
public:
    spell_klaxxi_gouge() : SpellScriptLoader("spell_klaxxi_gouge") { }

    class spell_klaxxi_gouge_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_klaxxi_gouge_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetCaster() && GetTarget())
                GetCaster()->CastSpell(GetTarget(), SPELL_MUTILATE, false);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_klaxxi_gouge_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_POSSESS_PET, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_klaxxi_gouge_AuraScript();
    }
};

//143373
class spell_gene_splice : public SpellScriptLoader
{
public:
    spell_gene_splice() : SpellScriptLoader("spell_gene_splice") { }

    class spell_gene_splice_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gene_splice_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                GetTarget()->RemoveAurasDueToSpell(SPELL_GENE_SPLICE);
                GetTarget()->RemoveAurasDueToSpell(SPELL_MAD_SCIENTIST);
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gene_splice_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_gene_splice_AuraScript();
    }
};

//143217
class spell_snipe : public SpellScriptLoader
{
public:
    spell_snipe() : SpellScriptLoader("spell_snipe") { }

    class spell_snipe_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_snipe_SpellScript);

        void DealDamage()
        {
            if (GetCaster() && GetHitUnit())
            {
                float distance = GetCaster()->GetExactDist2d(GetHitUnit());
                if (distance && distance <= 100)
                    SetHitDamage(GetHitDamage() * (distance / 100));
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_snipe_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_snipe_SpellScript();
    }
};

//148676
class spell_klaxxi_reave : public SpellScriptLoader
{
public:
    spell_klaxxi_reave() : SpellScriptLoader("spell_klaxxi_reave") { }

    class spell_klaxxi_reave_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_klaxxi_reave_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
                {
                    GetCaster()->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                    GetCaster()->ToCreature()->AI()->DoZoneInCombat(GetCaster()->ToCreature(), 80.0f);
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_klaxxi_reave_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_klaxxi_reave_AuraScript();
    }
};

//143339 
class spell_klaxxi_injection : public SpellScriptLoader
{
public:
    spell_klaxxi_injection() : SpellScriptLoader("spell_klaxxi_injection") { }

    class spell_klaxxi_injection_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_klaxxi_injection_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH && GetCaster() && GetCaster()->isAlive())
                GetTarget()->CastSpell(GetTarget(), SPELL_INJECTION_SUM, true);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_klaxxi_injection_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_klaxxi_injection_AuraScript();
    }
};

//143337
class spell_klaxxi_mutate : public SpellScriptLoader
{
public:
    spell_klaxxi_mutate() : SpellScriptLoader("spell_klaxxi_mutate") { }

    class spell_klaxxi_mutate_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_klaxxi_mutate_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget() && GetTarget()->GetMap()->IsHeroic())
                GetTarget()->CastSpell(GetTarget(), SPELL_FAULTY_MUTATION, true);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_klaxxi_mutate_AuraScript::OnApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_klaxxi_mutate_AuraScript();
    }
};

//148589
class spell_faulty_mutation : public SpellScriptLoader
{
public:
    spell_faulty_mutation() : SpellScriptLoader("spell_faulty_mutation") { }

    class spell_faulty_mutation_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_faulty_mutation_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                    GetTarget()->CastSpell(GetTarget(), SPELL_FAULTY_MUTATION_KILL, true);
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_faulty_mutation_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_faulty_mutation_AuraScript();
    }
};

//143280
class spell_klaxxi_bloodletting : public SpellScriptLoader
{
public:
    spell_klaxxi_bloodletting() : SpellScriptLoader("spell_klaxxi_bloodletting") { }

    class spell_klaxxi_bloodletting_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_klaxxi_bloodletting_SpellScript);

        void HandleAfterCast()
        {
            if (GetCaster())
            {
                uint8 n = GetCaster()->GetMap()->Is25ManRaid() ? 3 : 2;
                for (uint8 b = 0; b < n; b++)
                    GetCaster()->SummonCreature(NPC_BLOOD, bloodsumpos[b]);
            }
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_klaxxi_bloodletting_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_klaxxi_bloodletting_SpellScript();
    }
};

//142929
class spell_tenderizing_strikes_dmg : public SpellScriptLoader
{
public:
    spell_tenderizing_strikes_dmg() : SpellScriptLoader("spell_tenderizing_strikes_dmg") { }

    class spell_tenderizing_strikes_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_tenderizing_strikes_dmg_SpellScript);

        void HandleHit()
        {
            if (GetCaster() && GetHitUnit())
            {
                for (uint8 n = 0; n < 5; n++)
                    if (GetHitUnit()->HasAura(EvadeSpells[n]))
                        return;
                GetCaster()->CastSpell(GetHitUnit(), SPELL_CAUSTIC_BLOOD);
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_tenderizing_strikes_dmg_SpellScript::HandleHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_tenderizing_strikes_dmg_SpellScript();
    }
};

//142315
class spell_caustic_blood : public SpellScriptLoader
{
public:
    spell_caustic_blood() : SpellScriptLoader("spell_caustic_blood") { }

    class spell_caustic_blood_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_caustic_blood_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            if (GetTarget() && GetTarget()->HasAura(SPELL_CAUSTIC_BLOOD))
            {
                if (GetTarget()->GetAura(SPELL_CAUSTIC_BLOOD)->GetStackAmount() >= 10)
                {
                    GetTarget()->RemoveAurasDueToSpell(SPELL_CAUSTIC_BLOOD);
                    GetTarget()->CastSpell(GetTarget(), SPELL_BLOODY_EXPLOSION, true);
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_caustic_blood_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_caustic_blood_AuraScript();
    }
};

//142564
class spell_encase_in_amber : public SpellScriptLoader
{
public:
    spell_encase_in_amber() : SpellScriptLoader("spell_encase_in_amber") { }

    class spell_encase_in_amber_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_encase_in_amber_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget() && GetTarget()->ToCreature())
                GetTarget()->ToCreature()->AI()->DoAction(ACTION_ENCASE_IN_AMBER);
        }

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                    if (GetTarget()->ToCreature())
                        GetTarget()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_encase_in_amber_AuraScript::OnApply, EFFECT_4, SPELL_AURA_DAMAGE_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_encase_in_amber_AuraScript::HandleEffectRemove, EFFECT_4, SPELL_AURA_DAMAGE_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_encase_in_amber_AuraScript();
    }
};

//143666
class spell_diminish : public SpellScriptLoader
{
public:
    spell_diminish() : SpellScriptLoader("spell_diminish") { }

    class spell_diminish_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_diminish_SpellScript);

        void HandleHit()
        {
            if (GetCaster() && GetHitUnit())
            {
                if (GetHitUnit()->HealthBelowPct(34))
                    GetHitUnit()->Kill(GetHitUnit(), true);
                else
                {
                    int32 dmg = GetHitUnit()->CountPctFromMaxHealth(34);
                    SetHitDamage(dmg);
                }
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_diminish_SpellScript::HandleHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_diminish_SpellScript();
    }
};

void AddSC_boss_paragons_of_the_klaxxi()
{
    new npc_amber_piece();
    new npc_klaxxi_controller();
    new boss_paragons_of_the_klaxxi();
    new npc_klaxxi_blood();
    new npc_amber_parasite();
    new npc_amber();
    new spell_klaxxi_gouge();
    new spell_gene_splice();
    new spell_snipe();
    new spell_klaxxi_reave();
    new spell_klaxxi_injection();
    new spell_klaxxi_mutate();
    new spell_faulty_mutation();
    new spell_klaxxi_bloodletting();
    new spell_tenderizing_strikes_dmg();
    new spell_caustic_blood();
    new spell_encase_in_amber();
    new spell_diminish();
}
