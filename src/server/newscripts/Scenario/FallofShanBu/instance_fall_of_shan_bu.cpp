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
#include "fall_of_shan_bu.h"

const DoorData doorData[] =
{
    {   GO_THUNDER_FORGE_DOOR,  DATA_TRUNDER_FORGE_DOOR,    DOOR_TYPE_PASSAGE,  0},
    {   0,                      0,                          DOOR_TYPE_PASSAGE,  0},
};

class instance_fall_of_shan_bu : public InstanceMapScript
{
public:
    instance_fall_of_shan_bu() : InstanceMapScript("instance_fall_of_shan_bu", 1126) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_fall_of_shan_bu_InstanceMapScript(map);
    }

    struct instance_fall_of_shan_bu_InstanceMapScript : public InstanceScript
    {
        instance_fall_of_shan_bu_InstanceMapScript(Map* map) : InstanceScript(map)
        { }

        int8 eventStage;
        int8 eventStage2;
        int8 eventStage3;
        int8 completeEventStage_1;
        int64 doorGUID;
        uint8 counter;
        uint8 jumpPos;
        uint8 lrStage2;

        void Initialize()
        {
            LoadDoorData(doorData);

            eventStage = 0;
            eventStage2 = 0;
            eventStage3 = 0;
            completeEventStage_1 = 0;
            doorGUID = 0;
            counter = 0;
            jumpPos = 0;
            lrStage2 = 0;
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_THUNDER_FORGE_DOOR:
                    AddDoor(go, true);
                    doorGUID = go->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_SHANZE_SHADOWCASTER:
                case NPC_SHANZE_WARRIOR:
                case NPC_SHANZE_BATTLEMASTER:
                case NPC_SHANZE_ELECTRO_COUTIONER:
                case NPC_SHANZE_PYROMANCER:
                    ++counter;
                    break;
                default:
                    break;
            }
        }

        void CreatureDies(Creature* creature, Unit* /*killer*/)
        {
            switch (creature->GetEntry())
            {
                case NPC_SHANZE_SHADOWCASTER:
                case NPC_SHANZE_WARRIOR:
                case NPC_SHANZE_BATTLEMASTER:
                case NPC_SHANZE_ELECTRO_COUTIONER:
                case NPC_SHANZE_PYROMANCER:
                    --counter;
                    break;
                case NPC_FORGEMASTER_VULKON:
                    DoUseDoorOrButton(GetData64(DATA_TRUNDER_FORGE_DOOR));
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_START_EVENT:
                    eventStage = data;
                    break;
                case DATA_EVENT_PART_1:
                    eventStage2 = data;
                    break;
                case DATA_EVENT_PART_2:
                    eventStage3 = data;
                    break;
                case DATA_COMPLETE_EVENT_STAGE_1:
                    completeEventStage_1 = data;
                    break;
                case DATA_SUMMONS_COUNTER:
                    counter = data;
                    break;
                case DATA_JUMP_POS:
                    jumpPos = data;
                    break;
                case DATA_LR_STAGE_2:
                    lrStage2 = data;
                    break;
                default:
                    break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_TRUNDER_FORGE_DOOR:
                    return doorGUID;
                default:
                    break;
            }
            return 0;
        }

        uint32 GetData(uint32 type)
        {
            switch (type)
            {
                case DATA_START_EVENT:
                    return eventStage;
                case DATA_EVENT_PART_1:
                    return eventStage2;
                case DATA_EVENT_PART_2:
                    return eventStage3;
                case DATA_COMPLETE_EVENT_STAGE_1:
                    return completeEventStage_1;
                case DATA_SUMMONS_COUNTER:
                    return counter;
                case DATA_JUMP_POS:
                    return jumpPos;
                case DATA_LR_STAGE_2:
                    return lrStage2;
                default:
                    break;
            }
            return 0;
        }
    };
};

void AttakersCounter(Creature* me, InstanceScript* instance)
{
    instance->SetData(DATA_SUMMONS_COUNTER, instance->GetData(DATA_SUMMONS_COUNTER) - 1);

    if (instance->GetData(DATA_SUMMONS_COUNTER) > 0)
        return;

    std::list<Creature*> creatures;
    GetCreatureListWithEntryInGrid(creatures, me, NPC_SHADO_PAN_WARRIOR, 200.0f);
    if (!creatures.empty())
        for (std::list<Creature*>::iterator forge = creatures.begin(); forge != creatures.end(); ++forge)
            (*forge)->AI()->DoAction(ACTION_EVADE);

    if (Creature* defender = me->FindNearestCreature(NPC_SHADO_PAN_DEFENDER, 100.0f))
        if (!defender->getVictim())
            defender->AI()->DoAction(ACTION_EVADE);
}

void AddSC_instance_fall_of_shan_bu()
{
    new instance_fall_of_shan_bu();
}
