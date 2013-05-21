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
    SPELL_BANISHMENT            = 116272,
    SPELL_VOODOO_DOLL           = 122151,
    SPELL_SUMMON_SPIRIT_TOTEM   = 116174,

    // attaques ombreuses
    SPELL_RIFHT_CROSS           = 117215,
    SPELL_LEFT_HOOK             = 117218,
    SPELL_HAMMER_FIST           = 117219,
    SPELL_SWEEPING_KICK         = 117222,

    SPELL_FRENESIE              = 117752,

    // Shadowy Minion
    SPELL_SHADOW_BOLT           = 122118,
    SPELL_SPIRITUAL_GRASP       = 118421,

    // Misc
    SPELL_CLONE                 = 119051,
    SPELL_CLONE_VISUAL          = 119053,
    SPELL_LIFE_FRAGILE_THREAD   = 116227,
    SPELL_CROSSED_OVER          = 116161, // Todo : changer la phase
};

enum eEvents
{
    EVENT_SECONDARY_ATTACK      = 1,

    // Shadowy Minion
    EVENT_SHADOW_BOLT           = 2,
    EVENT_SPIRITUAL_GRASP       = 3,
};

class boss_garajal : public CreatureScript
{
    public:
        boss_garajal() : CreatureScript("boss_garajal") {}

        struct boss_garajalAI : public BossAI
        {
            boss_garajalAI(Creature* creature) : BossAI(creature, DATA_GARAJAL)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();

                events.ScheduleEvent(EVENT_SECONDARY_ATTACK, urand(5000, 10000));
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {}

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (!pInstance)
                    return;

                if (!me->HasAura(SPELL_FRENESIE))
                    if (me->HealthBelowPctDamaged(20, damage))
                        me->CastSpell(me, SPELL_FRENESIE, true);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_SECONDARY_ATTACK:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                            {
                                uint32 spellId = RAND(SPELL_RIFHT_CROSS, SPELL_LEFT_HOOK, SPELL_HAMMER_FIST, SPELL_SWEEPING_KICK);
                                me->CastSpell(target, spellId, true);
                            }
                            events.ScheduleEvent(EVENT_SECONDARY_ATTACK, urand(5000, 10000));
                            break;
                        }
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_garajalAI(creature);
        }
};

class mob_spirit_totem : public CreatureScript
{
    public:
        mob_spirit_totem() : CreatureScript("mob_spirit_totem") {}

        struct mob_spirit_totemAI : public ScriptedAI
        {
            mob_spirit_totemAI(Creature* creature) : ScriptedAI(creature)
            {}

            void Reset()
            {}

            void JustDied(Unit* attacker)
            {
                std::list<Player*> playerList;
                GetPlayerListInGrid(playerList, me, 3.0f);

                uint8 count = 0;
                for (auto player: playerList)
                {
                    if (++count > 3)
                        break;

                    if (Creature* clone = me->SummonCreature(56405, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation()))
                    {
                        player->CastSpell(player, SPELL_CLONE_VISUAL, true);
                        player->CastSpell(player, SPELL_CROSSED_OVER, true);

                        player->CastSpell(clone,  SPELL_CLONE, true);

                        clone->CastSpell(clone, SPELL_LIFE_FRAGILE_THREAD, true);

                        player->AddAura(SPELL_LIFE_FRAGILE_THREAD, player);
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_spirit_totemAI(creature);
        }
};

class mob_shadowy_minion : public CreatureScript
{
    public:
        mob_shadowy_minion() : CreatureScript("mob_shadowy_minion") {}

        struct mob_shadowy_minionAI : public ScriptedAI
        {
            mob_shadowy_minionAI(Creature* creature) : ScriptedAI(creature)
            {}

            uint64 spiritGuid;
            EventMap events;

            void Reset()
            {
                events.Reset();
                spiritGuid = 0;

                if (me->GetEntry() == NPC_SHADOWY_MINION_REAL)
                {
                    if (Creature* spirit = me->SummonCreature(NPC_SHADOWY_MINION_SPIRIT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ, me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN))
                    {
                        spiritGuid = spirit->GetGUID();
                        spirit->SetPhaseMask(2, true);
                    }

                    events.ScheduleEvent(EVENT_SPIRITUAL_GRASP, urand(2000, 5000));
                }
                else
                    events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(2000, 5000));

                DoZoneInCombat();
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                if (summon->GetEntry() == NPC_SHADOWY_MINION_SPIRIT)
                    me->DespawnOrUnsummon();
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Spirit World
                        case EVENT_SHADOW_BOLT:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                me->CastSpell(target, SPELL_SHADOW_BOLT, false);

                            events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(2000, 5000));
                            break;
                        }
                        // Real World
                        case EVENT_SPIRITUAL_GRASP:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                me->CastSpell(target, SPELL_SPIRITUAL_GRASP, false);

                            events.ScheduleEvent(EVENT_SPIRITUAL_GRASP, urand(5000, 8000));
                            break;
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_shadowy_minionAI(creature);
        }
};

void AddSC_boss_garajal()
{
    new boss_garajal();
    new mob_spirit_totem();
    new mob_shadowy_minion();
}
