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
#include "throne_of_thunder.h"

enum eSpells
{
    SPELL_GAZE                    = 134029,
    SPELL_HARD_STARE              = 133765,
    SPELL_OBLITERATE              = 137747,
    SPELL_LINGERING_GAZE          = 138467,
    SPELL_LINGERING_GAZE_TR_M     = 133792, //Create AT
    SPELL_LINGERING_GAZE_DMG      = 134040,
    SPELL_LINGERING_GAZE_TARGET   = 134626,
    SPELL_ARTERIAL_CUT            = 133768,
    SPELL_FORCE_OF_WILL           = 136413,
    SPELL_DURUMU_EYE_TRAIL_ORANGE = 141006,
    SPELL_DURUMU_EYE_TRAIL_RED    = 141008,
    SPELL_DURUMU_EYE_TRAIL_BLUE   = 141009,
    SPELL_DURUMU_EYE_TRAIL_YELLOW = 141010,
    SPELL_FORCE_OF_WILL_2         = 136932,
    SPELL_BRIGHT_LIGHT_DURUMU     = 136121,

    //Red
    SPELL_INFRARED_LIGHT_P_T_AURA = 133731, //player target aura
    SPELL_INFRARED_LIGHT_C_T_AURA = 136120, //creature target aura
    SPELL_INFRARED_LIGHT_BEAM     = 134123, //channel beam
    SPELL_INFRARED_LIGHT_CONE     = 133734, //channel cone
    SPELL_INFRARED_LIGHT_CONE_DMG = 133732,
    SPELL_INFRARED_LIGHT_EXPLOSE  = 133733,
    SPELL_BURNING_EYE_FOUND       = 137002,
    SPELL_RED_FOG_INVISIBILITY    = 136116,
    SPELL_SUMMON_RED_FOG          = 136128,

    //Blue
    SPELL_BLUE_RAY_P_T_AURA       = 133675, //player target  aura
    SPELL_BLUE_RAY_C_T_AURA       = 136119, //creature target aura
    SPELL_BLUE_RAY_BEAM           = 134122, //channel beam
    SPELL_BLUE_RAY_CONE           = 133672, //channel cone
    SPELL_BLUE_RAY_CONE_DMG       = 133677,
    SPELL_BLUE_RAY_EXPLOSE        = 133678,
    SPELL_BLUE_FOG_INVISIBILITY   = 136118,
    SPELL_SUMMON_BLUE_FOG         = 136130,

    //Yellow
    SPELL_BRIGHT_LIGHT_P_T_AURA   = 133737, //target  aura
    SPELL_BRIGHT_LIGHT_C_T_AURA   = 136121,
    SPELL_BRIGHT_LIGHT_BEAM       = 134124, //channel beam
    SPELL_BRIGHT_LIGHT_CONE       = 133740, //channel cone
    SPELL_BRIGHT_LIGHT_CONE_DMG   = 133738,
    SPELL_BRIGHT_LIGHT_EXPLOSE    = 133739,
    SPELL_YELLOW_FOG_INVISIBILITY = 136117,

    //Crimson Fog
    SPELL_CAUSTIC_SPIKE           = 136154, //While revealed
    SPELL_CRIMSON_BLOOM           = 136122,

    //Azure Fog
    SPELL_ICY_GRASP               = 136177,
    SPELL_ICY_GRASP_AURA          = 136179,
    SPELL_FLASH_FREEZE            = 136124, //explose them die

    SPELL_MAZE_STARTS_HERE        = 140911,

    //136251
};

enum sEvents
{
    EVENT_ENRAGE                  = 1,
    EVENT_HARD_STARE              = 2,
    EVENT_FORCE_OF_WILL           = 3,
    EVENT_MOVE_TO_POINT           = 4,
    EVENT_START_MOVE              = 5,
    EVENT_LINGERING_GAZE_PREPARE  = 6,
    EVENT_LINGERING_GAZE          = 7,
    EVENT_RESTART_MOVING          = 8,
    EVENT_COLORBLIND              = 9,
    EVENT_PREPARE_BEAM            = 10,
    EVENT_CREATE_CONE             = 11,

    EVENT_SEARCHER                = 12,
};

enum sActions
{
    ACTION_RE_ATTACK              = 1,
    ACTION_LINGERING_GAZE         = 2,
    ACTION_CREATE_CONE            = 3,
};

enum Phase
{
    PHASE_NULL,
    PHASE_NORMAL,
    PHASE_COLORBLIND,
    PHASE_DISINTEGRATION_BEAM,
};

uint32 colorblindeyelist[3] =
{
    NPC_RED_EYE,
    NPC_BLUE_EYE,
    NPC_YELLOW_EYE,
};

Position Durumucenterpos = { 5895.52f, 4512.58f, -6.27f };

class _TankFilter
{
public:
    bool operator()(WorldObject* unit)
    {
        if (Player* target = unit->ToPlayer())
            if (target->GetRoleForGroup(target->GetSpecializationId(target->GetActiveSpec())) != ROLES_TANK)
                return false;
        return true;
    }
};

class VictimFilter
{
public:
    VictimFilter(uint64 victimguid) : _victimguid(victimguid){}

    bool operator()(WorldObject* unit)
    {
        if (unit->GetGUID() == _victimguid)
            return true;
        return false;
    }
private:
    uint64 _victimguid;
};

class LingeringGazeFilter
{
public:
    bool operator()(WorldObject* unit)
    {
        if (Player* player = unit->ToPlayer())
            if (player->HasAura(SPELL_LINGERING_GAZE_TARGET))
                return false;
        return true;
    }
};

class boss_durumu : public CreatureScript
{
public:
    boss_durumu() : CreatureScript("boss_durumu") {}

    struct boss_durumuAI : public BossAI
    {
        boss_durumuAI(Creature* creature) : BossAI(creature, DATA_DURUMU)
        {
            instance = creature->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* instance;
        Phase phase;
        uint32 checkvictim;

        void Reset()
        {
            RemoveDebuffFromPlayers();
            _Reset();
            phase = PHASE_NULL;
            //me->SetReactState(REACT_PASSIVE);
            me->SetReactState(REACT_DEFENSIVE);
            me->RemoveAurasDueToSpell(SPELL_BRIGHT_LIGHT_DURUMU);
            checkvictim = 3000;
        }

        float GetFogAngle(uint8 pos)
        {
            switch (pos)
            {
            case 0:
            {
                float mod = float(urand(0, 1));
                float mod2 = !mod ? float(urand(0, 9)) / 10 : float(urand(0, 3)) / 10;
                return mod + mod2;
            }
            case 1:
            {
                float mod = float(urand(1, 2));
                float mod2 = mod == 1 ? float(urand(6, 9)) / 10 : float(urand(0, 9)) / 10;
                return mod + mod2;
            }
            case 2:
            {
                float mod = float(urand(3, 4));
                float mod2 = mod == 3 ? float(urand(2, 9)) / 10 : float(urand(0, 3)) / 10;
                return mod + mod2;
            }
            case 3:
            {
                float mod = float(urand(4, 5));
                float mod2 = mod == 4 ? float(urand(6, 9)) / 10 : float(urand(0, 9)) / 10;
                return mod + mod2;
            }
            default:
                return 0;
            }
        }

        void SummonFogs()
        {
            float x, y;
            float ang = 0;
            uint32 summonspellentry = 0;
            uint8 bluefog = urand(0, 3);
            for (uint8 n = 0; n < 4; n++)
            {
                ang = GetFogAngle(n);
                summonspellentry = bluefog == n ? SPELL_SUMMON_BLUE_FOG : SPELL_SUMMON_RED_FOG;
                me->GetNearPoint2D(x, y, 12.0f, ang);
                me->CastSpell(x, y, -6.27f, summonspellentry, true);
            }
        }

        void RemoveDebuffFromPlayers()
        {
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_INFRARED_LIGHT_BEAM);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLUE_RAY_BEAM);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BRIGHT_LIGHT_BEAM);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_INFRARED_LIGHT_CONE_DMG);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLUE_RAY_CONE_DMG);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BRIGHT_LIGHT_CONE_DMG);
        }

        void EnterCombat(Unit* who)
        {
            _EnterCombat();
            phase = PHASE_NORMAL;
            //events.ScheduleEvent(EVENT_LINGERING_GAZE_PREPARE, 5000);
            //events.ScheduleEvent(EVENT_PREPARE_LINGERING_GAZE, 5000);
            events.ScheduleEvent(EVENT_ENRAGE, 600000);
            events.ScheduleEvent(EVENT_HARD_STARE, 12000);
            events.ScheduleEvent(EVENT_FORCE_OF_WILL, 20000);
            //events.ScheduleEvent(EVENT_COLORBLIND, 5000); //30sec after entercombat, and cooldawn 300000
        }

        void SetData(uint32 type, uint32 data)
        {
            uint32 explosespell = 0;
            switch (type)
            {
            case SPELL_INFRARED_LIGHT_CONE_DMG:
                explosespell = SPELL_INFRARED_LIGHT_EXPLOSE;
                break;
            case SPELL_BLUE_RAY_CONE_DMG:
                explosespell = SPELL_BLUE_RAY_EXPLOSE;
                break;
            case SPELL_BRIGHT_LIGHT_CONE_DMG:
                explosespell = SPELL_BRIGHT_LIGHT_EXPLOSE;
                break;
            }
            DoCast(me, explosespell, true);
        }

        void DoAction(int32 const action)
        {
            if (action == ACTION_RE_ATTACK)
                me->ReAttackWithZone();
        }

        void JustDied(Unit* /*killer*/)
        {
            RemoveDebuffFromPlayers();
            _JustDied();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (checkvictim <= diff)
            {
                if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                {
                    if (me->GetDistance(me->getVictim()) < 60.0f)
                        DoCastAOE(SPELL_GAZE);
                    else
                        EnterEvadeMode();
                }
                checkvictim = 3000;
            }
            else
                checkvictim -= diff;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                //Normal phase
                case EVENT_HARD_STARE:
                    if (me->getVictim())
                        DoCast(me->getVictim(), SPELL_HARD_STARE);
                    events.ScheduleEvent(EVENT_HARD_STARE, 12000);
                    break;
                case EVENT_ENRAGE:
                    DoCastAOE(SPELL_OBLITERATE);
                    break;
                case EVENT_FORCE_OF_WILL:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                    {
                        me->SetAttackStop(false);
                        me->SetFacingToObject(target);
                        DoCast(target, SPELL_FORCE_OF_WILL);
                    }
                    events.ScheduleEvent(EVENT_FORCE_OF_WILL, 20000);
                    break;
                case EVENT_LINGERING_GAZE_PREPARE:
                {
                    float x, y;
                    float mod = urand(0, 6);
                    float ang = mod <= 5 ? mod + float(urand(1, 9)) / 10 : mod;
                    me->GetNearPoint2D(x, y, -15.0f, ang);
                    me->SummonCreature(NPC_APPRAISING_EYE, x, y, 2.483505f, 0.0f);
                    events.ScheduleEvent(EVENT_LINGERING_GAZE, 5000);
                    break;
                }
                case EVENT_LINGERING_GAZE:
                {
                    uint8 maxnum = me->GetMap()->Is25ManRaid() ? 5 : 2;
                    uint8 num = 0;
                    std::list<Player*>pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, me, 100.0f);
                    if (!pllist.empty())
                    {
                        pllist.remove_if(_TankFilter());
                        if (!pllist.empty())
                        {
                            for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                            {
                                DoCast(*itr, SPELL_LINGERING_GAZE_TARGET, true);
                                num++;
                                if (num >= maxnum)
                                    break;
                            }
                        }
                    }
                    if (Creature* appraisingeye = me->FindNearestCreature(NPC_APPRAISING_EYE, 50.0f, true))
                        appraisingeye->AI()->DoAction(ACTION_LINGERING_GAZE);
                    break;
                }
                //Color Blind phase
                case EVENT_COLORBLIND:
                    events.Reset();
                    SummonFogs();
                    me->InterruptNonMeleeSpells(true);
                    phase = PHASE_COLORBLIND;

                    //For testing
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                        if (Creature* colorblindeye = me->SummonCreature(colorblindeyelist[0], Durumucenterpos))
                            colorblindeye->AI()->SetGUID(target->GetGUID(), 1);

                    /*std::list<Player*>pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, me, 150.0f);
                    if (!pllist.empty())
                    {
                        if (pllist.size() == 1) //WOD or Legion solo kill
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                                for (uint8 n = 0; n < 3; n++)
                                    if (Creature* colorblindeye = me->SummonCreature(colorblindeyelist[n], Durumucenterpos))
                                        colorblindeye->AI()->SetGUID(target->GetGUID(), 1);
                        }
                        else if (pllist.size() == 2)
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 150.0f, true)) 
                                for (uint8 n = 0; n < 3; n++)
                                    if (Creature* colorblindeye = me->SummonCreature(colorblindeyelist[n], Durumucenterpos))
                                        colorblindeye->AI()->SetGUID(target->GetGUID(), 1);          
                        }
                        else if (pllist.size() == 3)
                        {
                            std::list<Player*>::const_iterator Itr;
                            std::advance(Itr, urand(1, pllist.size() - 1));
                            for (uint8 n = 0; n < 3; n++)
                                if (Creature* colorblindeye = me->SummonCreature(colorblindeyelist[n], Durumucenterpos))
                                    colorblindeye->AI()->SetGUID((*Itr)->GetGUID(), 1);
                        }
                        else if (pllist.size() >= 4)
                        {
                            uint8 index = 0;
                            uint64 victimGuid = me->getVictim() ? me->getVictim()->GetGUID() : 0;
                            pllist.remove_if(VictimFilter(victimGuid));
                            for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                            {
                                if (Creature* colorblindeye = me->SummonCreature(colorblindeyelist[index], Durumucenterpos))
                                {
                                    index++;
                                    colorblindeye->AI()->SetGUID((*itr)->GetGUID(), 1);
                                    if (index >= 2)
                                        break;
                                }
                            }
                        }
                    }*/
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_durumuAI(creature);
    }
};

//67858
class npc_appraising_eye : public CreatureScript
{
public:
    npc_appraising_eye() : CreatureScript("npc_appraising_eye") {}

    struct npc_appraising_eyeAI : public ScriptedAI
    {
        npc_appraising_eyeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
        }
        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            DoZoneInCombat(me, 100.0f);
            DoCast(me, SPELL_DURUMU_EYE_TRAIL_ORANGE, true);
            events.ScheduleEvent(EVENT_START_MOVE, 250);
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void DoAction(int32 const action)
        {
            if (action == ACTION_LINGERING_GAZE)
            {
                events.Reset();
                me->GetMotionMaster()->Clear(false);
                DoCast(me, SPELL_LINGERING_GAZE);
                events.ScheduleEvent(EVENT_START_MOVE, 4000);
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_START_MOVE)
                {
                    if (Creature* durumu = me->GetCreature(*me, pInstance->GetData64(NPC_DURUMU)))
                    {
                        float x, y;
                        float ang = durumu->GetAngle(me) + 0.5f;                      
                        durumu->GetNearPoint2D(x, y, -15.0f, ang);
                        me->GetMotionMaster()->Clear(false);
                        me->GetMotionMaster()->MoveCharge(x, y, 2.483505f, 10.0f, 1);
                    }
                    events.ScheduleEvent(EVENT_START_MOVE, 250);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_appraising_eyeAI(creature);
    }
};

//67855, 67854, 67856
class npc_colorblind_eye : public CreatureScript
{
public:
    npc_colorblind_eye() : CreatureScript("npc_colorblind_eye") { }

    struct npc_colorblind_eyeAI : public ScriptedAI
    {
        npc_colorblind_eyeAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            instance = pCreature->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            targetGuid = 0;
        }
        InstanceScript* instance;
        EventMap events;
        uint64 targetGuid;

        void Reset(){}

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void DoAction(int32 const action)
        {
            if (action == ACTION_CREATE_CONE)
                events.ScheduleEvent(EVENT_PREPARE_BEAM, 100);
        }

        uint32 GetEyeBeamTargetEntry()
        {
            switch (me->GetEntry())
            {
            case NPC_RED_EYE:
                return NPC_RED_EYEBEAM_TARGET;
            case NPC_BLUE_EYE:
                return NPC_BLUE_EYEBEAM_TARGET;
            default:
                return 0;
            }
        }

        uint32 GetBeamSpellEntry()
        {
            switch (me->GetEntry())
            {
            case NPC_RED_EYE:
                return SPELL_INFRARED_LIGHT_BEAM;
            case NPC_BLUE_EYE:
                return SPELL_BLUE_RAY_BEAM;
            case NPC_YELLOW_EYE:
                return SPELL_BRIGHT_LIGHT_BEAM;
            default:
                return 0;
            }
        }

        uint32 GetConeSpellEntry()
        {
            switch (me->GetEntry())
            {
            case NPC_RED_EYE:
                return SPELL_INFRARED_LIGHT_CONE;
            case NPC_BLUE_EYE:
                return SPELL_BLUE_RAY_CONE;
            case NPC_YELLOW_EYE:
                return SPELL_BRIGHT_LIGHT_CONE;
            default:
                return 0;
            }
        }

        uint32 GetLightPlayerAura()
        {
            switch (me->GetEntry())
            {
            case NPC_RED_EYE:
                return SPELL_INFRARED_LIGHT_P_T_AURA;
            case NPC_BLUE_EYE:
                return SPELL_BLUE_RAY_P_T_AURA;
            case NPC_YELLOW_EYE:
                return SPELL_BRIGHT_LIGHT_P_T_AURA;
            default:
                return 0;
            }
        }

        uint32 GetLightFogAura()
        {
            switch (me->GetEntry())
            {
            case NPC_RED_EYE:
                return SPELL_INFRARED_LIGHT_C_T_AURA;
            case NPC_BLUE_EYE:
                return SPELL_BLUE_RAY_C_T_AURA;
            case NPC_YELLOW_EYE:
                return 0;
            default:
                return 0;
            }
        }

        uint32 GetSearcheFogEntry()
        {
            switch (me->GetEntry())
            {
            case NPC_RED_EYE:
                return NPC_CRIMSON_FOG;
            case NPC_BLUE_EYE:
                return NPC_AZURE_FOG;
            case NPC_YELLOW_EYE:
                return 0;
            default:
                return 0;
            }
        }

        uint32 GetData(uint32 type)
        {
            if (type == DATA_IS_CONE_TARGET_CREATURE)
            {
                if (Unit* conetarget = me->GetUnit(*me, targetGuid))
                {
                    if (conetarget->ToCreature())
                        return 1;
                    else
                        return 0;
                }
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            //remove focus cone from trigger, and despawn him
            if (type == DATA_DESPAWN_CREATURE_CONE_TARGET)
            {
                if (Creature* conetarget = me->GetCreature(*me, targetGuid))
                {
                    uint32 conespell = GetConeSpellEntry();
                    conetarget->RemoveAurasDueToSpell(conespell);//for safe
                    conetarget->DespawnOrUnsummon();
                }
            }
        }

        void SetGUID(uint64 guid, int32 id)
        {
            targetGuid = guid;
            switch (id)
            {
            case 1:
            {
                uint32 beamspell = GetBeamSpellEntry();
                if (Player* pl = me->GetPlayer(*me, targetGuid))
                    DoCast(pl, beamspell, true);
                break;
            }
            //focus cone player died, create cone with trigger
            case 2:
                if (Creature* durumu = me->GetCreature(*me, instance->GetData64(NPC_DURUMU)))
                {
                    if (Player* pl = me->GetPlayer(*me, targetGuid))
                    {
                        Position pos;
                        pl->GetPosition(&pos);
                        uint32 conespell = GetConeSpellEntry();
                        uint32 npceyebeamtarget = GetEyeBeamTargetEntry();
                        if (Creature* conetarget = durumu->SummonCreature(npceyebeamtarget, pos))
                        {
                            targetGuid = conetarget->GetGUID();
                            DoCast(conetarget, conespell, true);
                        }
                    }
                }
                break;
            //create focus cone on new player
            case 3:
            {
                uint32 conespell = GetConeSpellEntry();
                if (Player* pl = me->GetPlayer(*me, targetGuid))
                    DoCast(pl, conespell, true);
            }
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
                case EVENT_PREPARE_BEAM: //this event create for fix rotate lag, before launch cone spell
                    if (Player* pl = me->GetPlayer(*me, targetGuid))
                    {
                        me->AddThreat(pl, 50000000.0f);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->Attack(pl, true);
                    }
                    events.ScheduleEvent(EVENT_CREATE_CONE, 250);
                    break;
                case EVENT_CREATE_CONE:
                    if (Player* pl = me->GetPlayer(*me, targetGuid))
                    {
                        uint32 conespell = GetConeSpellEntry();
                        if (Player* pl = me->GetPlayer(*me, targetGuid))
                        {
                            if (me->GetEntry() == NPC_YELLOW_EYE)
                            {
                                float x, y;
                                float ang = me->GetAngle(pl);
                                me->GetNearPoint2D(x, y, 64.0f, ang);
                                if (Creature* durumu = me->GetCreature(*me, instance->GetData64(NPC_DURUMU)))
                                {
                                    if (Creature* conetarget = durumu->SummonCreature(NPC_YELLOW_EYEBEAM_TARGET, x, y, me->GetPositionZ(), 0.0f))
                                    {
                                        durumu->CastSpell(durumu, SPELL_BRIGHT_LIGHT_DURUMU, true); //visual
                                        DoCast(conetarget, conespell, true);
                                        conetarget->AI()->SetGUID(me->GetGUID(), 2);
                                    }
                                }
                            }
                            else
                                DoCast(pl, conespell, true);
                        }
                        events.ScheduleEvent(EVENT_SEARCHER, 1000);
                    }
                    break;
                case EVENT_SEARCHER:
                {
                    //Searche players in cone
                    uint32 lightaura = GetLightPlayerAura();
                    std::list<Player*>pllist;
                    GetPlayerListInGrid(pllist, me, 100.0f);
                    if (!pllist.empty())
                    {
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                        {
                            if (me->isInFront(*itr, M_PI / 6))
                            {
                                if (!(*itr)->HasAura(lightaura))
                                    (*itr)->CastSpell(*itr, lightaura, true);
                            }
                            else
                                if ((*itr)->HasAura(lightaura))
                                    (*itr)->RemoveAurasDueToSpell(lightaura);
                        }
                    }
                    //Searche fogs in cone
                    if (me->GetEntry() != NPC_YELLOW_EYE)
                    {
                        uint32 _lightaura = GetLightFogAura();
                        uint32 fogentry = GetSearcheFogEntry();
                        std::list<Creature*>foglist;
                        GetCreatureListWithEntryInGrid(foglist, me, fogentry, 150.0f);
                        if (!foglist.empty())
                        {
                            for (std::list<Creature*>::const_iterator Itr = foglist.begin(); Itr != foglist.end(); Itr++)
                            {
                                if (me->isInFront(*Itr, M_PI / 6))
                                {
                                    if (!(*Itr)->HasAura(_lightaura))
                                        (*Itr)->CastSpell(*Itr, _lightaura, true);
                                }
                                else
                                    if ((*Itr)->HasAura(_lightaura))
                                        (*Itr)->RemoveAurasDueToSpell(_lightaura);
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_SEARCHER, 1000);
                }
                break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_colorblind_eyeAI(pCreature);
    }
};

//67851, 67829, 67852
class npc_colorblind_eye_beam_target : public CreatureScript
{
public:
    npc_colorblind_eye_beam_target() : CreatureScript("npc_colorblind_eye_beam_target") { }

    struct npc_colorblind_eye_beam_targetAI : public ScriptedAI
    {
        npc_colorblind_eye_beam_targetAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->SetDisplayId(11686);
            colorblindeyeGuid = 0;
        }
        EventMap events;
        uint64 colorblindeyeGuid;

        void Reset(){}

        void SetGUID(uint64 guid, int32 id)
        {
            if (id == 2)
            {
                colorblindeyeGuid = guid;
                events.ScheduleEvent(EVENT_START_MOVE, 1000);
            }
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_START_MOVE)
                {
                    if (Creature* colorblindeye = me->GetCreature(*me, colorblindeyeGuid))
                    {
                        float x, y;
                        float ang = colorblindeye->GetAngle(me) - 0.5f;
                        colorblindeye->GetNearPoint2D(x, y, 64.0f, ang);
                        me->GetMotionMaster()->Clear(false);
                        me->GetMotionMaster()->MoveCharge(x, y, me->GetPositionZ(), 5.0f, 1);
                    }
                    events.ScheduleEvent(EVENT_START_MOVE, 250);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_colorblind_eye_beam_targetAI(pCreature);
    }
};

//69050, 69052
class npc_durumu_fog : public CreatureScript
{
public:
    npc_durumu_fog() : CreatureScript("npc_durumu_fog") {}

    struct npc_durumu_fogAI : public ScriptedAI
    {
        npc_durumu_fogAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            explose = false;
            switch (me->GetEntry())
            {
                case NPC_CRIMSON_FOG:
                    DoCast(me, SPELL_RED_FOG_INVISIBILITY, true);
                    break;
                case NPC_AZURE_FOG:
                    DoCast(me, SPELL_BLUE_FOG_INVISIBILITY, true);
                    break;
                default:
                    break;
            }
        }
        InstanceScript* instance;
        EventMap events;
        bool explose;

        void Reset(){}

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (me->GetEntry() != NPC_AZURE_FOG)
                return;

            if (damage >= me->GetHealth() && !explose)
            {
                explose = true;
                DoCast(me, SPELL_FLASH_FREEZE, true);
            }
        }

        void SpellHit(Unit* caster, SpellInfo const *spell)
        {
            if (spell->Id == SPELL_INFRARED_LIGHT_C_T_AURA)
            {
                me->RemoveAurasDueToSpell(SPELL_RED_FOG_INVISIBILITY);
                DoCast(me, SPELL_BURNING_EYE_FOUND, true);
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
        return new npc_durumu_fogAI(creature);
    }
};

//133768
class spell_arterial_cut : public SpellScriptLoader
{
public:
    spell_arterial_cut() : SpellScriptLoader("spell_arterial_cut") { }

    class spell_arterial_cut_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_arterial_cut_AuraScript)

        void HandlePeriodicTick(AuraEffect const * /*aurEff*/)
        {
            if (GetTarget())
                if (GetTarget()->GetHealth() == GetTarget()->GetMaxHealth())
                    GetTarget()->RemoveAurasDueToSpell(SPELL_ARTERIAL_CUT);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_arterial_cut_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_arterial_cut_AuraScript();
    }
};

//136413
class spell_force_of_will : public SpellScriptLoader
{
public:
    spell_force_of_will() : SpellScriptLoader("spell_force_of_will") { }

    class spell_force_of_will_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_force_of_will_SpellScript);

        void OnAfterCast()
        {
            if (GetCaster() && GetCaster()->ToCreature())
                GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_force_of_will_SpellScript::OnAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_force_of_will_SpellScript();
    }
};

//138467
class spell_lingering_gaze : public SpellScriptLoader
{
public:
    spell_lingering_gaze() : SpellScriptLoader("spell_lingering_gaze") { }

    class spell_lingering_gaze_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_lingering_gaze_SpellScript);

        void HandleScript(SpellEffIndex effIndex)
        {
            if (GetCaster())
            {
                std::list<Player*>pllist;
                pllist.clear();
                GetPlayerListInGrid(pllist, GetCaster(), 100.0f);
                if (!pllist.empty())
                {
                    pllist.remove_if(LingeringGazeFilter());
                    if (!pllist.empty())
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                            GetCaster()->CastSpell(*itr, SPELL_LINGERING_GAZE_TR_M);
                }
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_lingering_gaze_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_lingering_gaze_SpellScript();
    }
};

//134123, 134122, 134124
class spell_durumu_color_beam : public SpellScriptLoader
{
public:
    spell_durumu_color_beam() : SpellScriptLoader("spell_durumu_color_beam") { }

    class spell_durumu_color_beam_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_durumu_color_beam_AuraScript);

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                if (GetCaster() && GetCaster()->ToCreature())
                    GetCaster()->ToCreature()->AI()->DoAction(ACTION_CREATE_CONE);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_durumu_color_beam_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_durumu_color_beam_AuraScript();
    }
};

//133734, 133672, 133740
class spell_durumu_color_cone : public SpellScriptLoader
{
public:
    spell_durumu_color_cone() : SpellScriptLoader("spell_durumu_color_cone") { }

    class spell_durumu_color_cone_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_durumu_color_cone_AuraScript);

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            //if focus cone player died, need drop cone zone
            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEATH)
                if (GetSpellInfo()->Id == SPELL_INFRARED_LIGHT_CONE || GetSpellInfo()->Id == SPELL_BLUE_RAY_CONE)
                    if (GetTarget() && GetCaster() && GetCaster()->ToCreature())
                        GetCaster()->ToCreature()->AI()->SetGUID(GetTarget()->GetGUID(), 2);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_durumu_color_cone_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_durumu_color_cone_AuraScript();
    }
};

//133732, 133677, 133738
class spell_durumu_color_cone_dmg : public SpellScriptLoader
{
public:
    spell_durumu_color_cone_dmg() : SpellScriptLoader("spell_durumu_color_cone_dmg") { }

    class spell_durumu_color_cone_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_durumu_color_cone_dmg_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (targets.empty())
                {
                    if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                        if (Creature* durumu = GetCaster()->GetCreature(*GetCaster(), instance->GetData64(NPC_DURUMU)))
                            durumu->AI()->SetData(GetSpellInfo()->Id, 1);
                }
                else
                {
                    if (GetSpellInfo()->Id != SPELL_BRIGHT_LIGHT_CONE_DMG)
                    {
                        //if focus cone on creature (it means player drop it), who entrance in cone pick up him
                        if (GetCaster()->ToCreature()->AI()->GetData(DATA_IS_CONE_TARGET_CREATURE))
                        {
                            GetCaster()->ToCreature()->AI()->SetData(DATA_DESPAWN_CREATURE_CONE_TARGET, 0);
                            std::list<WorldObject*>::const_iterator itr = targets.begin();
                            std::advance(itr, urand(0, targets.size() - 1));
                            GetCaster()->ToCreature()->AI()->SetGUID((*itr)->GetGUID(), 3);
                        }
                    }
                }
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_durumu_color_cone_dmg_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_110);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_durumu_color_cone_dmg_SpellScript();
    }
};

//8897
class at_durumu_entrance : public AreaTriggerScript
{
public:
    at_durumu_entrance() : AreaTriggerScript("at_durumu_entrance") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger, bool enter)
    {
        if (enter)
            player->RemoveAurasDueToSpell(SPELL_JIKUN_FLY);
        return true;
    }
};

void AddSC_boss_durumu()
{
    new boss_durumu();
    new npc_appraising_eye();
    new npc_colorblind_eye();
    new npc_colorblind_eye_beam_target();
    new npc_durumu_fog();
    new spell_arterial_cut();
    new spell_force_of_will();
    new spell_lingering_gaze();
    new spell_durumu_color_beam();
    new spell_durumu_color_cone();
    new spell_durumu_color_cone_dmg();
    new at_durumu_entrance();
}
