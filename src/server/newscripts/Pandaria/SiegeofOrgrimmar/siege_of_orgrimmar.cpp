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
#include "siege_of_orgrimmar.h"
#include "ScriptedEscortAI.h"
#include "CreatureTextMgr.h"

enum eSpells
{
};

//Lorewalker Cho
class npc_lorewalker_cho : public CreatureScript
{
public:
    npc_lorewalker_cho() : CreatureScript("npc_lorewalker_cho") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lorewalker_choAI (creature);
    }

    struct npc_lorewalker_choAI : public npc_escortAI
    {
        npc_lorewalker_choAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        uint32 state;
        bool on;

        enum eEvents
        {
            //imereus
            EVENT_IMMEREUS_1 = 1,
            EVENT_IMMEREUS_2,
            EVENT_IMMEREUS_3,
            EVENT_IMMEREUS_4,
            EVENT_IMMEREUS_5,
            EVENT_IMMEREUS_6,
            EVENT_IMMEREUS_END_1,
            EVENT_IMMEREUS_END_2,
            EVENT_IMMEREUS_END_3,
            EVENT_IMMEREUS_END_4,
            //falen champions
            EVENT_FC_1, 
            EVENT_FC_2,
            EVENT_FC_3,
            EVENT_FC_4,
            EVENT_FC_5,
            EVENT_FC_6,
            EVENT_FC_7,
            EVENT_FC_8,
            EVENT_FC_9,
            EVENT_FC_10,
            EVENT_FC_OUTRO_1,
            EVENT_FC_OUTRO_2,
        };

        void Reset()
        {
            state = 0;
            on = false;
            me->setActive(true);
        }

        void SetData(uint32 id, uint32 value)
        {
            switch(id)
            {
                case DATA_IMMERSEUS:
                {
                    uint32 t = 0;
                    if (value == DONE)
                    {
                        events.ScheduleEvent(EVENT_IMMEREUS_END_1, t += 100);  //07:04:14.000
                        events.ScheduleEvent(EVENT_IMMEREUS_END_2, t += 7000); //07:04:21.000
                        events.ScheduleEvent(EVENT_IMMEREUS_END_3, t += 7000); //07:04:28.000
                        events.ScheduleEvent(EVENT_IMMEREUS_END_4, t += 6000); //07:04:34.000
                    }else if (value == IN_PROGRESS)
                    {
                        events.ScheduleEvent(EVENT_IMMEREUS_3, t += 100);   //06:54:28.000
                        events.ScheduleEvent(EVENT_IMMEREUS_4, t += 10000); //06:54:38.000
                        events.ScheduleEvent(EVENT_IMMEREUS_5, t += 14000); //06:54:52.000
                        events.ScheduleEvent(EVENT_IMMEREUS_6, t += 8000);  //06:55:00.000
                    }
                    break;
                }
                case DATA_F_PROTECTORS:
                {
                    if (value == DONE)
                    {
                        uint32 t = 0;                                        //12:13:04.000
                        events.RescheduleEvent(EVENT_FC_OUTRO_1, t += 4000);   //12:13:08.000
                        events.RescheduleEvent(EVENT_FC_OUTRO_2, t += 5000);    //12:13:13.000
                    }
                }
                    break;
            }
            state = id;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (on || !state || who->GetTypeId() != TYPEID_PLAYER || !me->IsWithinDistInMap(who, 60.0f))
                return;

            on = true;

            if (state == DATA_F_PROTECTORS)
            {
                events.RescheduleEvent(EVENT_FC_1, 1000);
            }
            else
            {
                uint32 t = 0;
                events.ScheduleEvent(EVENT_IMMEREUS_1, t += 1000);  //06:53:34.000
                events.ScheduleEvent(EVENT_IMMEREUS_2, t += 11000); //06:53:45.000
                //06:54:10.000 blizz remove this and summon again. Posible at zone attack.
            }
        }

        void WaypointReached(uint32 pointId)
        {            
            switch(pointId)
            {
                case 9:
                    SetEscortPaused(true);
                    break;
                case 20:
                {
                    SetEscortPaused(true);
                    uint32 t = 0;
                    events.RescheduleEvent(EVENT_FC_2, t += 5000);   //07:05:32.000
                    events.RescheduleEvent(EVENT_FC_3, t += 6000);   //07:05:38.000
                    events.RescheduleEvent(EVENT_FC_4, t += 6000);   //07:05:44.000
                    events.RescheduleEvent(EVENT_FC_5, t += 9000);   //07:05:53.000
                    events.RescheduleEvent(EVENT_FC_6, t += 7000);   //07:06:00.000
                    events.RescheduleEvent(EVENT_FC_7, t += 10000);  //07:06:10.000
                    events.RescheduleEvent(EVENT_FC_8, t += 6000);   //07:06:16.000
                    events.RescheduleEvent(EVENT_FC_9, t += 13000);  //07:06:29.000
                    events.RescheduleEvent(EVENT_FC_10, t += 5000);  //07:06:34.000
                    break;
                }
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_IMMEREUS_END_4:
                        me->DespawnOrUnsummon();
                        break;
                    case EVENT_IMMEREUS_3:
                        Start(false, true);
                        //no break
                    case EVENT_IMMEREUS_1:
                    case EVENT_IMMEREUS_2:
                    case EVENT_IMMEREUS_4:
                    case EVENT_IMMEREUS_5:
                    case EVENT_IMMEREUS_6:
                    case EVENT_IMMEREUS_END_1:
                    case EVENT_IMMEREUS_END_2:
                    case EVENT_IMMEREUS_END_3:
                        sCreatureTextMgr->SendChat(me, eventId -1, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_1:
                        SetNextWaypoint(10, false, false);
                        Start(false, false, 0, NULL, false, false, false);
                        break;
                    case EVENT_FC_2:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_9, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_3:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_10, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_4:
                        if (Creature* rook = instance->instance->GetCreature(instance->GetData64(NPC_ROOK_STONETOE)))
                            sCreatureTextMgr->SendChat(rook, TEXT_GENERIC_0, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_5:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_11, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_6:
                        if (Creature* rook = instance->instance->GetCreature(instance->GetData64(NPC_ROOK_STONETOE)))
                            sCreatureTextMgr->SendChat(rook, TEXT_GENERIC_1, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_7:
                        if (Creature* rook = instance->instance->GetCreature(instance->GetData64(NPC_SUN_TENDERHEART)))
                            sCreatureTextMgr->SendChat(rook, TEXT_GENERIC_0, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_8:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_12, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_9:
                        if (Creature* rook = instance->instance->GetCreature(instance->GetData64(NPC_ROOK_STONETOE)))
                            sCreatureTextMgr->SendChat(rook, TEXT_GENERIC_2, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    case EVENT_FC_10:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_13, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        me->DespawnOrUnsummon(60000);
                        break;
                    case EVENT_FC_OUTRO_1:
                    case EVENT_FC_OUTRO_2:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_14+(eventId - TEXT_GENERIC_14), 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    default:
                        break;
                }
            }
        }
    };
};

class spell_self_absorbed: public SpellScriptLoader
{
    public:
        spell_self_absorbed() : SpellScriptLoader("spell_self_absorbed") { }

        class spell_self_absorbed_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_self_absorbed_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                uint32 count = 2;
                if (Unit* caster = GetCaster())
                    if(caster->GetMap() && (caster->GetMap()->GetSpawnMode() == MAN25_HEROIC_DIFFICULTY || caster->GetMap()->GetSpawnMode() == MAN25_DIFFICULTY))
                        count = 5;

                if (targets.size() > count)
                    targets.resize(count);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_self_absorbed_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_self_absorbed_SpellScript();
        }
};

class at_siege_of_orgrimmar_portal_to_orgrimmar : public AreaTriggerScript
{
    public:
        at_siege_of_orgrimmar_portal_to_orgrimmar() : AreaTriggerScript("at_siege_of_orgrimmar_portal_to_orgrimmar") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/, bool /*enter*/)
        {
            InstanceScript* instance = player->GetInstanceScript();
            if (!instance || instance->GetBossState(DATA_SHA_OF_PRIDE) != DONE)
                return true;

            player->CastSpell(player, player->GetTeam() == ALLIANCE ? SPELL_TP_ORGRIMMAR_2 : SPELL_TP_ORGRIMMAR_1);
            return true;
        }
};

void AddSC_siege_of_orgrimmar()
{
    new npc_lorewalker_cho();
    new spell_self_absorbed();
    new at_siege_of_orgrimmar_portal_to_orgrimmar();
}
