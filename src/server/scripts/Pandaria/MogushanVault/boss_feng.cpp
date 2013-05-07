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
#include "mogu_shan_vault.h"

enum eSpells
{
    // Shared
    SPELL_SPIRIT_BOLT           = 118530,
    SPELL_STRENGHT_OF_SPIRIT    = 116363,

    // Spirit of the Fist
    SPELL_LIGHTNING_LASH        = 131788,
    SPELL_LIGHTNING_FISTS       = 116157,
    SPELL_EPICENTER             = 116040,

    // Spirit of the Spear
    SPELL_FLAMING_SPEAR         = 116942,
    SPELL_WILDFIRE_SPARK        = 116784,
    SPELL_DRAW_FLAME            = 116711,

    // Spirit of the Staff 
    SPELL_ARCANE_SHOCK          = 131790,
    SPELL_ARCANE_VELOCITY       = 116365,
    SPELL_ARCANE_RESONANCE      = 116417,

    // Spirit of the Shield ( Heroic )
    SPELL_SHADOWBURN            = 17877,
    SPELL_SIPHONING_SHIELD      = 118071,
    SPELL_CHAINS_OF_SHADOW      = 118783,

    // Stolen Essences of Stone
    SPELL_NULLIFICATION_BARRIER = 115817,
    SPELL_SHROUD_OF_REVERSAL    = 115911
};

enum eEvents
{
};

enum ePhases
{
    PHASE_NONE      = 0,
    PHASE_FIST      = 1,
    PHASE_SPEAR     = 2,
    PHASE_STAFF     = 3,
    PHASE_SHIELD    = 4
};

class boss_feng : public CreatureScript
{
    public:
        boss_feng() : CreatureScript("boss_feng") {}

        struct boss_fengAI : public BossAI
        {
            boss_fengAI(Creature* creature) : BossAI(creature, DATA_FENG)
            {
                pInstance = creature->GetInstanceScript();

                std::list<uint8> phaseList;
                for (uint8 i = 1; i <= 4; ++i) phaseList.push_back(i);
            }

            InstanceScript* pInstance;

            uint8 actualPhase;

            void Reset()
            {
                _Reset();

                actualPhase = PHASE_NONE;
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_fengAI(creature);
        }
};

void AddSC_boss_feng()
{
    new boss_feng();
}
