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


/*######
## npc_gnomeregan_survivor
######*/

class npc_gnomeregan_survivor : public CreatureScript
{
public:
    npc_gnomeregan_survivor() : CreatureScript("npc_gnomeregan_survivor") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_gnomeregan_survivorAI (creature);
    }

    struct npc_gnomeregan_survivorAI : public ScriptedAI
    {
        npc_gnomeregan_survivorAI(Creature* creature) : ScriptedAI(creature) {}
        
        void Reset() {}

        void SpellHit(Unit* Caster, const SpellInfo* Spell)
        {
            if (Spell->Id == 86264)
            {
                if (Caster->ToPlayer())
                    Caster->ToPlayer()->KilledMonsterCredit(46268, 0);

                me->ForcedDespawn(1000);
                me->SetRespawnDelay(15);
            }
    };
};

/*######
## npc_flying_target_machin
######*/

class npc_flying_target_machin : public CreatureScript
{
public:
    npc_flying_target_machin() : CreatureScript("npc_flying_target_machin") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_flying_target_machinAI (creature);
    }

    struct npc_flying_target_machinAI : public ScriptedAI
    {
        npc_flying_target_machinAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            me->SetSpeed(MOVE_FLIGHT, 0.5f);
        }

        void EnterCombat(Unit* /*who*/)
        {
            return;
        }
    };
};

void AddSC_dun_morogh()
{
    new npc_gnomeregan_survivor();
    new npc_flying_target_machin();
}
