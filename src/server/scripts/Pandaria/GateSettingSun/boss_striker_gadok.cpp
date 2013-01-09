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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "gate_setting_sun.h"

struct StrafPointStruct
{
    uint8 pointIdBegin;
    Position begin;
    
    uint8 pointIdEnd;
    Position end;
    
    uint8 pointIdOutside;
    Position outside;
};

enum eMovements
{
    POINT_NORTH_START       = 1,
    POINT_SOUTH_START       = 2,
    POINT_WEST_START        = 3,
    POINT_EAST_START        = 4,

    POINT_NORTH_END         = 5,
    POINT_SOUTH_END         = 6,
    POINT_WEST_END          = 7,
    POINT_EAST_END          = 8,

    POINT_NORTH_OUTSIDE     = 9,
    POINT_SOUTH_OUTSIDE     = 10,
    POINT_WEST_OUTSIDE      = 11,
    POINT_EAST_OUTSIDE      = 12,

    MOV_NORTH_SOUTH     = 0,
    MOV_SOUTH_NORTH     = 1,
    MOV_WEST_EAST       = 2,
    MOV_EAST_WEST       = 3
};

StrafPointStruct StrafPoints[4] =
{
    { POINT_NORTH_START, {1238.007f, 2304.644f, 435.0f, 0.0f}, POINT_NORTH_END, {1153.398f, 2304.578f, 435.0f, 0.0f}, POINT_NORTH_OUTSIDE, {1133.4f, 2304.578f, 438.0f, 0.0f} }, // North -> South
    { POINT_SOUTH_START, {1153.398f, 2304.578f, 435.0f, 0.0f}, POINT_SOUTH_END, {1238.007f, 2304.644f, 435.0f, 0.0f}, POINT_SOUTH_OUTSIDE, {1258.0f, 2304.644f, 438.0f, 0.0f} }, // South -> North
    { POINT_WEST_START,  {1195.299f, 2348.941f, 435.0f, 0.0f}, POINT_WEST_END,  {1195.392f, 2263.441f, 435.0f, 0.0f}, POINT_WEST_OUTSIDE,  {1195.4f, 2243.441f, 438.0f, 0.0f} }, // West  -> East
    { POINT_EAST_START,  {1195.392f, 2263.441f, 435.0f, 0.0f}, POINT_EAST_END,  {1195.299f, 2348.941f, 435.0f, 0.0f}, POINT_EAST_OUTSIDE,  {1195.3f, 2366.941f, 438.0f, 0.0f} }  // East  -> West
};

enum eSpells
{
    SPELL_PREY_TIME         = 106933,
    SPELL_IMPALING_STRIKE   = 107047,

    SPELL_STRAFING_RUN      = 107342,
    SPELL_STRAFIND_RUN_DMG  = 116298,
};

enum eEvents
{
    EVENT_PREY_TIME         = 1,
    EVENT_IMPALING_STRIKE   = 2
};

enum ePhases
{
    PHASE_NONE          = 0,
    PHASE_NORTH_SOUTH   = 1,
    PHASE_WEST_EAST     = 2
};

enum eStrafing
{
    STRAF_NONE    = 0,
    STRAF_70      = 1,
    STRAF_30      = 2
};

class boss_striker_gadok : public CreatureScript
{
    public:
        boss_striker_gadok() : CreatureScript("boss_striker_gadok") {}

        struct boss_striker_gadokAI : public BossAI
        {
            boss_striker_gadokAI(Creature* creature) : BossAI(creature, DATA_GADOK)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            bool isStrafing;

            uint32 strafingTimer;

            uint8 strafingEventCount;
            uint8 strafingEventProgress;

            uint8 move;

            void Reset()
            {
                if (!instance)
                    return;

                _Reset();
                isStrafing = false;

                strafingTimer = 0;

                strafingEventCount = 0;
                strafingEventProgress = 0;
                move = 0;

                instance->SetData(DATA_GADOK, PHASE_NONE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
            }

            void JustReachedHome()
            {
                if (!instance)
                    return;

                instance->SetBossState(DATA_GADOK, FAIL);
                summons.DespawnAll();
            }

            void MovementInform(uint32 type, uint32 id)
            {
                switch (id)
                {
                    case POINT_NORTH_START:   case POINT_SOUTH_START:   case POINT_WEST_START:   case POINT_EAST_START:
                    case POINT_NORTH_END:     case POINT_SOUTH_END:     case POINT_WEST_END:     case POINT_EAST_END:
                    case POINT_NORTH_OUTSIDE: case POINT_SOUTH_OUTSIDE: case POINT_WEST_OUTSIDE: case POINT_EAST_OUTSIDE:
                        DoStrafingEvent();
                        break;
                    default:
                        break;
                }
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                float nextHealthPct = ((float(me->GetHealth()) - damage)  / float(me->GetMaxHealth())) * 100;

                if (!isStrafing)
                {
                    if ((strafingEventCount < STRAF_70 && nextHealthPct <= 70.0f) ||
                        (strafingEventCount < STRAF_30 && nextHealthPct <= 30.0f))
                    {
                        DoStrafingEvent();
                        ++strafingEventCount;
                    }
                }
            }

            uint8 SelectNextStartPoint()
            {
                // In videos, Gadok follow the opposite direction of clockwise, to confirm

                switch (move)
                {
                    case MOV_NORTH_SOUTH:   return MOV_WEST_EAST;
                    case MOV_SOUTH_NORTH:   return MOV_EAST_WEST;
                    case MOV_WEST_EAST:     return MOV_NORTH_SOUTH;
                    case MOV_EAST_WEST:     return MOV_SOUTH_NORTH;
                    default:                break;
                }

                return MOV_NORTH_SOUTH;
            }

            void DoStrafingEvent()
            {
                if (!instance)
                    return;

                switch (strafingEventProgress)
                {
                    case 0: // Begin, Gadok is 70% or 30% health, he go to the first POINT_START
                        isStrafing = true;
                        me->SetReactState(REACT_PASSIVE);

                        move = urand(MOV_NORTH_SOUTH, MOV_EAST_WEST);

                        if (instance)
                            instance->SetData(DATA_GADOK, move <= MOV_SOUTH_NORTH ? PHASE_NORTH_SOUTH: PHASE_WEST_EAST);

                        me->GetMotionMaster()->MovePoint(StrafPoints[move].pointIdBegin, StrafPoints[move].begin.GetPositionX(), StrafPoints[move].begin.GetPositionY(), StrafPoints[move].begin.GetPositionZ());

                        ++strafingEventProgress;
                        strafingTimer = 0;
                        break;
                    case 1: // We are a POINT_START, wait 2 sec then continue
                        ++strafingEventProgress;
                        strafingTimer = 2000;
                        break;
                    case 2: // 2 sec passed, move to POINT_END with the spell
                        // Todo : set speed to 2
                        me->GetMotionMaster()->MovePoint(StrafPoints[move].pointIdEnd, StrafPoints[move].end.GetPositionX(), StrafPoints[move].end.GetPositionY(), StrafPoints[move].end.GetPositionZ());
                        me->CastSpell(me, SPELL_STRAFING_RUN, true);

                        ++strafingEventProgress;
                        break;
                    case 3: // First strafing finished, we are at a POINT_END and go to POINT_OUTSIDE
                        if (instance)
                            instance->SetData(DATA_GADOK, PHASE_NONE);

                        me->GetMotionMaster()->MovePoint(StrafPoints[move].pointIdOutside, StrafPoints[move].outside.GetPositionX(), StrafPoints[move].outside.GetPositionY(), StrafPoints[move].outside.GetPositionZ());
                        ++strafingEventProgress;
                        break;
                    case 4: // We are POINT_OUTSIDE, go to the next POINT_START
                        move = SelectNextStartPoint();

                        if (instance)
                            instance->SetData(DATA_GADOK, move <= MOV_SOUTH_NORTH ? PHASE_NORTH_SOUTH: PHASE_WEST_EAST);
                        
                        me->GetMotionMaster()->MovePoint(StrafPoints[move].pointIdBegin, StrafPoints[move].begin.GetPositionX(), StrafPoints[move].begin.GetPositionY(), StrafPoints[move].begin.GetPositionZ());

                        ++strafingEventProgress;
                        break;
                    case 5: // Just arrived to second POINT_START, wait 2 sec
                        ++strafingEventProgress;
                        strafingTimer = 2000;
                        break;
                    case 6: // 2 sec passed, move to POINT_END with the spell
                        // Todo : set speed to 2
                        me->GetMotionMaster()->MovePoint(StrafPoints[move].pointIdEnd, StrafPoints[move].end.GetPositionX(), StrafPoints[move].end.GetPositionY(), StrafPoints[move].end.GetPositionZ());
                        me->CastSpell(me, SPELL_STRAFING_RUN, true);

                        ++strafingEventProgress;
                        break;
                    case 7: // POINT_END, End Strafing Event, go back to fight
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                        {
                            me->GetMotionMaster()->MoveCharge(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                            me->AI()->AttackStart(target);
                        }

                        if (instance)
                            instance->SetData(DATA_GADOK, PHASE_NONE);

                        move = 0;
                        me->SetReactState(REACT_AGGRESSIVE);
                        strafingTimer = 0;
                        strafingEventProgress = 0;
                        isStrafing = false;
                        break;
                    default:
                        break;
                }
            }

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (strafingTimer)
                {
                    if (strafingTimer <= diff)
                    {
                        strafingTimer = 0;
                        DoStrafingEvent();
                    }
                    else strafingTimer -= diff;
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_striker_gadokAI(creature);
        }
};

class spell_gadok_strafing : public SpellScriptLoader
{
    public:
        spell_gadok_strafing() :  SpellScriptLoader("spell_gadok_strafing") { }

        class spell_gadok_strafing_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gadok_strafing_SpellScript);
            
            void HandleScript()
            {
                if (Unit* caster = GetCaster())
                {
                    if (InstanceScript* instance = caster->GetInstanceScript())
                    {
                        uint8 actualStrafPhase = instance->GetData(DATA_GADOK);

                        if (!actualStrafPhase)
                            return;

                        uint32 stalkerEntry = actualStrafPhase == PHASE_NORTH_SOUTH ? NPC_STALKER_NORTH_SOUTH: NPC_STALKER_WEST_EAST;

                        std::list<Creature*> stalkerList;
                        GetCreatureListWithEntryInGrid(stalkerList, caster, stalkerEntry, 10.0f);

                        for (auto itr: stalkerList)
                            if (!itr->HasAura(SPELL_STRAFIND_RUN_DMG))
                                itr->CastSpell(itr, SPELL_STRAFIND_RUN_DMG, true);
                    }
                }
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_gadok_strafing_SpellScript::HandleScript);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gadok_strafing_SpellScript();
        }
};

void AddSC_boss_striker_gadok()
{
    new boss_striker_gadok();
}
