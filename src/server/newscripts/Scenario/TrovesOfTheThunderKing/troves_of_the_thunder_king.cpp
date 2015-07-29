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

#include "troves_of_the_thunder_king.h"
#include "ScriptedCreature.h"

class npc_taoshi : public CreatureScript
{
public:
    npc_taoshi() : CreatureScript("npc_taoshi") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = creature->GetInstanceScript())
        {
            if (instance->GetData(DATA_EVENT_STARTED) == NOT_STARTED)
                player->ADD_GOSSIP_ITEM_DB(15571, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            else
                player->ADD_GOSSIP_ITEM_DB(15572, 1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            return true;
        }
        return false;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (!action || !instance)
            return false;

        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            instance->SetData(DATA_EVENT_STARTED, DONE);
            player->CastSpell(player, SPELL_TIMED_RUN_STARTED_SPELL); // REQ item 94222 - Key to the Palace of Lei Shen
            player->AddAura(SPELL_LIMITED_TIME, player);
            player->AddAura(SPELL_TROVES_OF_THE_THUNDER_KING, player);
            creature->AI()->DoAction(ACTION_1);
        }

        if (action == GOSSIP_ACTION_INFO_DEF + 2)
            player->TeleportTo(1064, 6889.38f, 5517.79f, 2.08655f, 2.407245f); //< via 140010 but core dont support this :(

        creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        player->CLOSE_GOSSIP_MENU();

        return true;
    }

    struct npc_taoshiAI : public ScriptedAI
    {
        npc_taoshiAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            events.Reset();
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
                case ACTION_1:
                    events.ScheduleEvent(EVENT_1, 1 * IN_MILLISECONDS);
                    break;
                default:
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
                    case EVENT_1:
                        events.ScheduleEvent(EVENT_1, 5 * MINUTE * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_taoshiAI(creature);
    }
};


void AddSC_troves_of_the_thunder_king()
{
    new npc_taoshi();
}
