/*
 * Copyright © 2008-2012 Holystone Productions>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "firelands.h"

enum Spells
{
    Spell_Blaze_of_Glory    = 99252,
    Spell_Incendiary_Soul   = 99369,
    Spell_Shards_of_Torment = 99259,
    Spell_Torment           = 99256,
    Spell_Torment_Visual    = 99255,
    Spell_Tormented         = 99257,
    Spell_Wave_of_Torment   = 99261,
    Spell_Vital_Spark       = 99262,
    Spell_Vital_Flame       = 99263,
    Spell_Decimation_Blade  = 99352,
    Spell_Inferno_Blade     = 99350,
};

enum Events
{
    Event_Blaze_of_Glory    = 1,
    Event_Incendiary_Soul   = 2,
    Event_Shards_of_Torment = 3,
    Event_Torment           = 4,
    Event_Tormented         = 5,
    Event_Wave_of_Torment   = 6,
    Event_Vital_Spark       = 7,
    Event_Vital_Flame       = 8,
    Event_Blades_of_Baleroc = 9,
    Event_Change_Blade      = 10,
    // Spawn Shards
    Event_Spawn_Shards      = 11,
};

Position const SummonMelleRange[3] =
{
    // Melle dps's Range
    {66.5319f, -73.8099f, 54.6200f, 0.5214f},
    {67.8785f, -47.4203f, 55.1227f, 4.8764f},
    {64.8196f, -61.0175f, 54.1135f, 6.2077f},
};

Position const SummonRangeRange[3] =
{
    // Range dps's Range
    {40.8419f, -41.7327f, 54.9782f, 5.9249f},
    {55.3210f, -22.2373f, 56.3639f, 5.0099f},
    {44.5859f, -86.5407f, 54.8949f, 0.6274f},
};

class boss_baleroc : public CreatureScript
{
    public:
        boss_baleroc() : CreatureScript("boss_baleroc") { }

        struct boss_balerocAI : public BossAI
        {
            boss_balerocAI(Creature* creature) : BossAI(creature, DATA_BALEROC)
            {
            }

            void reset()
            {
                events.ScheduleEvent(Event_Blaze_of_Glory, 30000);
                events.ScheduleEvent(Event_Shards_of_Torment, 40000);
                events.ScheduleEvent(Event_Blades_of_Baleroc, 20000);
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(Event_Blaze_of_Glory, 30000);
                events.ScheduleEvent(Event_Shards_of_Torment, 40000);
                events.ScheduleEvent(Event_Blades_of_Baleroc, 20000);
            }

            void SummShard()
            {
                me->SummonCreature(NPC_Crystal_Shard, SummonMelleRange[urand(1, 3)]);
                me->SummonCreature(NPC_Crystal_Shard, SummonRangeRange[urand(1, 3)]);
            }

            void UpdateAI(uint32 const diff)
            {
                while (uint32 eventId = events.ExecuteEvent())
                {
                     switch (eventId)
                    {
                     case Event_Blaze_of_Glory:
                         DoCast(Spell_Blaze_of_Glory);
                         events.ScheduleEvent(Event_Incendiary_Soul, 1000);
                         break;
                     case Event_Incendiary_Soul:
                         if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                             DoCast(target, Spell_Incendiary_Soul);
                         events.ScheduleEvent(Event_Blaze_of_Glory, 30000);
                         break;
                     case Event_Shards_of_Torment:
                         DoCast(Spell_Shards_of_Torment);
                         events.ScheduleEvent(Event_Spawn_Shards, 40000);
                         break;
                     case Event_Spawn_Shards:
                         SummShard();
                         break;
                     case Event_Blades_of_Baleroc:
                         DoCastVictim(Spell_Decimation_Blade);
                         events.ScheduleEvent(Event_Change_Blade, 20000);
                         break;
                     case Event_Change_Blade:
                         DoCastVictim(Spell_Inferno_Blade);
                         events.ScheduleEvent(Event_Blades_of_Baleroc, 20000);
                    }
                }

            DoMeleeAttackIfReady();
            }
    };
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_balerocAI(creature);
    }
};

class Crystal_Shard : public CreatureScript
{
    public:
        Crystal_Shard() : CreatureScript("Crystal_Shard") { }

        struct Crystal_ShardAI : public BossAI
        {
            Crystal_ShardAI(Creature* creature) : BossAI(creature, DATA_Crystal_Shard)
            {
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(Event_Torment, 1500);
            }

            void UpdateAI(uint32 const diff)
            {
                while (uint32 eventId = events.ExecuteEvent())
                {
                     switch (eventId)
                    {
                     case Event_Torment:
                         if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 4))
                         {
                             me->AddAura(Spell_Torment, target);
                         }
                         break;
                     }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new Crystal_ShardAI(creature);
        }
};

void AddSC_boss_baleroc()
{
    new boss_baleroc();
    new Crystal_Shard();
}