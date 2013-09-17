/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptPCH.h"
#include "naxxramas.h"

enum Yells
{
    EMOTE_AURA_BLOCKING                                     = -1533143,
    EMOTE_AURA_WANE                                         = -1533144,
    EMOTE_AURA_FADING                                       = -1533145
};
enum Spells
{
    SPELL_NECROTIC_AURA                                    = 55593,
    SPELL_SUMMON_SPORE                                     = 29234,
    SPELL_DEATHBLOOM                                       = 29865,
    H_SPELL_DEATHBLOOM                                     = 55053,
    SPELL_INEVITABLE_DOOM                                  = 29204,
    H_SPELL_INEVITABLE_DOOM                                = 55052,
    SPELL_FUNGAL_CREEP                                     = 29232
};

enum Events
{
    EVENT_NONE,
    EVENT_AURA,
    EVENT_BLOOM,
    EVENT_DOOM,
    EVENT_EMOTE_WANE,
    EVENT_EMOTE_FADE
};

enum Achievements
{
    ACHIEVEMENT_SPORE_LOSER_10            =2182,
    ACHIEVEMENT_SPORE_LOSER_25            =2183,
};
class boss_loatheb : public CreatureScript
{
public:
    boss_loatheb() : CreatureScript("boss_loatheb") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_loathebAI (creature);
    }

    struct boss_loathebAI : public BossAI
    {
        boss_loathebAI(Creature* c) : BossAI(c, BOSS_LOATHEB) {}

        bool bSporeKilled;

        void Reset()
        {
            _Reset();
            bSporeKilled = false;
        }

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_AURA, 10000);
            events.ScheduleEvent(EVENT_BLOOM, 5000);
            events.ScheduleEvent(EVENT_DOOM, 120000);
        }

        void DoAction(int32 const /*param*/)
        {
            bSporeKilled = true;
        }

        void JustDied(Unit* /*Killer*/)
        {
            _JustDied();

            if (instance && !bSporeKilled)
            {
                instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_SPORE_LOSER_10, ACHIEVEMENT_SPORE_LOSER_25));
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FUNGAL_CREEP);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_AURA:
                        DoCastAOE(SPELL_NECROTIC_AURA);
                        DoScriptText(EMOTE_AURA_BLOCKING, me);
                        events.ScheduleEvent(EVENT_AURA, 20000);
                        events.ScheduleEvent(EVENT_EMOTE_WANE, 12000);
                        events.ScheduleEvent(EVENT_EMOTE_FADE, 17000);
                        break;
                    case EVENT_BLOOM:
                        // TODO : Add missing text
                        DoCastAOE(SPELL_SUMMON_SPORE, true);
                        DoCastAOE(RAID_MODE(SPELL_DEATHBLOOM, H_SPELL_DEATHBLOOM));
                        events.ScheduleEvent(EVENT_BLOOM, 30000);
                        break;
                    case EVENT_DOOM:
                        DoCastAOE(RAID_MODE(SPELL_INEVITABLE_DOOM, H_SPELL_INEVITABLE_DOOM));
                        events.ScheduleEvent(EVENT_DOOM, events.GetTimer() < 5*60000 ? 30000 : 15000);
                        break;
                    case EVENT_EMOTE_WANE:
                        DoScriptText(EMOTE_AURA_WANE, me);
                        break;
                    case EVENT_EMOTE_FADE:
                        DoScriptText(EMOTE_AURA_FADING, me);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

};

class mob_loatheb_spore : public CreatureScript
{
public:
    mob_loatheb_spore() : CreatureScript("mob_loatheb_spore") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_loatheb_sporeAI (pCreature);
    }

    struct mob_loatheb_sporeAI : public ScriptedAI
    {
        mob_loatheb_sporeAI(Creature *c) : ScriptedAI(c) 
        {
            deathTimer = 1000;
            dying = false;
        }

        uint32 deathTimer;
        bool dying;

        void DamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            if (damage < me->GetHealth())
                return;

            DoCast(SPELL_FUNGAL_CREEP);
            me->SetHealth(10);
            damage = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            dying = true;
        }

        void UpdateAI(uint32 const diff)
        {
            if (dying)
                deathTimer -= diff;

            if (deathTimer <= diff)
               me->Kill(me);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            if (InstanceScript* pInstance = me->GetInstanceScript())
                if (Creature* pLoatheb = Creature::GetCreature(*me, pInstance->GetData64(DATA_LOATHEB)))
                    if (pLoatheb->isAlive())
                        pLoatheb->AI()->DoAction(0);
        }
    };

};

void AddSC_boss_loatheb()
{
    new boss_loatheb();
    new mob_loatheb_spore();
}
