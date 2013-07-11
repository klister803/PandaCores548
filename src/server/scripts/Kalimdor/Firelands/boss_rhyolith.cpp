/*
* Copyright (C) 2005 - 2011 MaNGOS <http://www.getmangos.org/>
*
* Copyright (C) 2008 - 2011 TrinityCore <http://www.trinitycore.org/>
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

/**********
* Script Coded by Naios
* Script Complete 1% (or less)
**********/

/* TODO 

*/



#include "ScriptPCH.h"
#include "firelands.h"

enum Yells
{
    SAY_AGGRO                                    = -1999971,
    SAY_SOFT_ENRAGE                                 = -1999972, //TODO Add Sound
    SAY_ON_DOGS_FALL                             = -1999973, //TODO Add Sound
    SAY_ON_DEAD                                     = -1999974, //TODO Add Sound
};

enum Spells
{

};

enum Events
{

};

// ######### Lord Rhyolith #########

class boss_rhyolith : public CreatureScript
{
public:
    boss_rhyolith() : CreatureScript("boss_rhyolith"){}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_rhyolithAI(creature);
    }

    struct boss_rhyolithAI : public BossAI
    {
        boss_rhyolithAI(Creature* c) : BossAI(c, DATA_SHANNOX)
        {
            instance = me->GetInstanceScript();
        }

        InstanceScript* instance;
        bool enrage;
        
        void Reset()
        {
            instance->SetBossState(DATA_RHYOLITH, NOT_STARTED);
            events.Reset();

            me->GetMotionMaster()->MoveTargetedHome();

        }

        /*void JustSummoned(Creature* summon)
        {
        summons.Summon(summon);
        summon->setActive(true);
        }*/

        void KilledUnit(Unit * /*victim*/)
        {
        }

        void JustDied(Unit * /*victim*/)
        {

            instance->SetBossState(DATA_RHYOLITH, DONE);
            DoScriptText(SAY_ON_DEAD, me);
            _JustDied();
        }

        void EnterCombat(Unit* who)
        {

            _EnterCombat();
        }

        void UpdateAI(const uint32 diff)
        {
            
            
        }
    };
};

void AddSC_boss_rhyolith()
{
    new boss_rhyolith();
}