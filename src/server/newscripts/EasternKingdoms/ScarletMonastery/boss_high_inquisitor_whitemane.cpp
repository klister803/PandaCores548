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

/* ScriptData
SDName: boss_high_inquisitor_whitemane
SD%Complete: 90
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scarlet_monastery.h"

enum Says
{
    //Whitemane says
    SAY_WH_INTRO                 = -1189008,
    SAY_WH_KILL                  = -1189009,
    SAY_WH_RESSURECT             = -1189010,
};

enum Spells
{
    //Whitemanes Spells
    SPELL_DEEPSLEEP              = 9256,
    SPELL_SCARLETRESURRECTION    = 9232,
    SPELL_HOLYSMITE              = 114848,
    SPELL_HEAL                   = 12039,
    SPELL_POWERWORDSHIELD        = 127399,

    SPELL_ACHIEV_CREDIT          = 132022,
};

class boss_high_inquisitor_whitemane : public CreatureScript
{
public:
    boss_high_inquisitor_whitemane() : CreatureScript("boss_high_inquisitor_whitemane") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_high_inquisitor_whitemaneAI (creature);
    }

    struct boss_high_inquisitor_whitemaneAI : public ScriptedAI
    {
        boss_high_inquisitor_whitemaneAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        uint32 Heal_Timer;
        uint32 PowerWordShield_Timer;
        uint32 HolySmite_Timer;
        uint32 Wait_Timer;

        bool _bCanResurrectCheck;
        bool _bCanResurrect;

        void Reset()
        {
            Wait_Timer = 7000;
            Heal_Timer = 10000;
            PowerWordShield_Timer = 15000;
            HolySmite_Timer = 6000;

            _bCanResurrectCheck = false;
            _bCanResurrect = false;

            if (instance)
                if (me->isAlive())
                    instance->SetData(TYPE_MOGRAINE_AND_WHITE_EVENT, NOT_STARTED);
        }

        /*void AttackStart(Unit* who)
        {
            if (instance && instance->GetData(TYPE_MOGRAINE_AND_WHITE_EVENT) == NOT_STARTED)
                return;

            ScriptedAI::AttackStart(who);
        }*/

        void EnterCombat(Unit* /*who*/)
        {
            DoScriptText(SAY_WH_INTRO, me);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            DoScriptText(SAY_WH_KILL, me);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            if (!_bCanResurrectCheck && damage >= me->GetHealth())
                damage = me->GetHealth() - 1;
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_ACHIEV_CREDIT, 0, me);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (_bCanResurrect)
            {
                //When casting resuruction make sure to delay so on rez when reinstate battle deepsleep runs out
                if (instance && Wait_Timer <= diff)
                {
                    if (Unit* Mograine = Unit::GetUnit(*me, instance->GetData64(DATA_MOGRAINE)))
                    {
                        DoCast(Mograine, SPELL_SCARLETRESURRECTION);
                        DoScriptText(SAY_WH_RESSURECT, me);
                        _bCanResurrect = false;
                    }
                }
                else Wait_Timer -= diff;
            }

            //Cast Deep sleep when health is less than 50%
            if (!_bCanResurrectCheck && !HealthAbovePct(50))
            {
                //if (me->IsNonMeleeSpellCasted(false))
                //    me->InterruptNonMeleeSpells(false);

                DoCast(me->getVictim(), SPELL_DEEPSLEEP);
                _bCanResurrectCheck = true;
                //_bCanResurrect = true;
                return;
            }

            //while in "resurrect-mode", don't do anything
            if (_bCanResurrect)
                return;

            //If we are <75% hp cast healing spells at self or Mograine
            if (Heal_Timer <= diff)
            {
                Creature* target = NULL;

                if (!HealthAbovePct(75))
                    target = me;

                if (instance)
                {
                    if (Creature* mograine = Unit::GetCreature((*me), instance->GetData64(DATA_MOGRAINE)))
                    {
                        // checking _bCanResurrectCheck prevents her healing Mograine while he is "faking death"
                        if (_bCanResurrectCheck && mograine->isAlive() && !mograine->HealthAbovePct(75))
                            target = mograine;
                    }
                }

                if (target)
                    DoCast(target, SPELL_HEAL);

                Heal_Timer = 13000;
            }
            else Heal_Timer -= diff;

            //PowerWordShield_Timer
            if (PowerWordShield_Timer <= diff)
            {
                DoCast(me, SPELL_POWERWORDSHIELD);
                PowerWordShield_Timer = 15000;
            }
            else PowerWordShield_Timer -= diff;

            //HolySmite_Timer
            if (HolySmite_Timer <= diff)
            {
                DoCast(me->getVictim(), SPELL_HOLYSMITE);
                HolySmite_Timer = 6000;
            }
            else HolySmite_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_high_inquisitor_whitemane()
{
    new boss_high_inquisitor_whitemane();
}
