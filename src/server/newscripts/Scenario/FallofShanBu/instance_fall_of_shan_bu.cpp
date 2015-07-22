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

        void Initialize()
        {
            playerRole = 0;
            crucibleGUID = 0;
            doorGUID = 0;
            wrathionGUID = 0;
            thunderForgeGUID = 0;
            shaBeastGUIDs.clear();
            shaFiendGUIDs.clear();
            firstPhaseTrashGUIDs.clear();

            stageData = STAGE_1;
            waveCounter = 0;
            waveCounter2 = 0;


            eventStage = 0;
            eventStage2 = 0;
            eventStage3 = 0;
            counter = 0;
            jumpPos = 0;
            lrStage2 = 0;
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_MOGU_CRICUBLE:
                    crucibleGUID = go->GetGUID();
                    break;
                case GO_THUNDER_FORGE_DOOR:
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
                case NPC_WRATHION:
                    wrathionGUID = creature->GetGUID();
                    break;
                case NPC_THUNDER_FORGE2:
                case NPC_THUNDER_FORGE_3:
                case NPC_THUNDER_FORGE_CRUCIBLE:
                case NPC_INVISIBLE_STALKER:
                case NPC_LIGHTING_PILAR_BEAM_STALKER:
                    creature->SetVisible(false);
                    break;
                case NPC_SHA_BEAST:
                    creature->SetVisible(false);
                    shaBeastGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHA_FIEND:
                    creature->SetVisible(false);
                    shaFiendGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHANZE_SHADOWCASTER:
                case NPC_SHANZE_WARRIOR:
                case NPC_SHANZE_BATTLEMASTER:
                case NPC_SHANZE_ELECTRO_CUTIONER:
                case NPC_SHANZE_ELECTRO_CUTIONER2:
                case NPC_SHANZE_PYROMANCER:
                    creature->SetVisible(false);
                    firstPhaseTrashGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_THUNDER_FORGE:
                    creature->SetVisible(false);
                    thunderForgeGUID = creature->GetGUID();
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
                case NPC_SHANZE_ELECTRO_CUTIONER:
                case NPC_SHANZE_PYROMANCER:
                    SetData(DATA_WAVE_COUNTER, GetData(DATA_WAVE_COUNTER) - 1);
                    break;
                case NPC_FORGEMASTER_VULKON:
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_PLAYER_ROLE:
                    playerRole = data;
                    switch(data)
                    {
                        case ROLES_DEFAULT:
                        case ROLES_HEALER:
                            break;
                        case ROLES_DPS:
                            break;
                        case ROLES_TANK:
                            break;
                        default:
                            break;
                    }
                    break;
                case DATA_ALLOWED_STAGE:
                    stageData = data;
                    switch(data)
                    {
                        case STAGE_3:
                            if (Creature* cre = instance->GetCreature(wrathionGUID))
                                cre->AI()->DoAction(ACTION_1);
                            break;
                        default:
                            break;
                    }
                    break;
                case DATA_WAVE_COUNTER:
                    waveCounter = data;
                    switch(data)
                    {
                        case 0:
                            if (waveCounter2 = 0)
                            {
                                if (Creature* cre = instance->GetCreature(wrathionGUID))
                                    cre->AI()->DoAction(ACTION_2);
                                break;
                            }
                            else
                            {
                                uint8 count = 0;
                                uint8 amount = urand(1, 3);
                                for (std::vector<uint64>::const_iterator itr = firstPhaseTrashGUIDs.begin(); itr != firstPhaseTrashGUIDs.end(); itr++)
                                    if (Creature* cre = instance->GetCreature(*itr))
                                    {
                                        firstPhaseTrashGUIDs.erase(itr);
                                        cre->SetVisible(true);
                                        count++;
                                        if (count > amount)
                                            break;
                                    }
                            }
                            break;
                        case 1:
                        {
                            waveCounter = 0;
                            waveCounter2 = 1;
                            uint8 count = 0;
                            for (std::vector<uint64>::const_iterator itr = firstPhaseTrashGUIDs.begin(); itr != firstPhaseTrashGUIDs.end(); itr++)
                                if (Creature* cre = instance->GetCreature(*itr))
                                {
                                    firstPhaseTrashGUIDs.erase(itr);
                                    cre->SetVisible(true);
                                    count++;
                                    if (count > 3)
                                        break;
                                }
                            break;
                        }
                        default:
                            break;
                    }
                    break;



                case DATA_START_EVENT:
                    eventStage = data;
                    break;
                case DATA_EVENT_PART_1:
                    eventStage2 = data;
                    break;
                case DATA_EVENT_PART_2:
                    eventStage3 = data;
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

        uint32 GetData(uint32 type)
        {
            switch (type)
            {
                case DATA_PLAYER_ROLE:
                    return playerRole;
                case DATA_ALLOWED_STAGE:
                    return stageData;
                case DATA_WAVE_COUNTER:
                    return waveCounter;

                case DATA_START_EVENT:
                    return eventStage;
                case DATA_EVENT_PART_1:
                    return eventStage2;
                case DATA_EVENT_PART_2:
                    return eventStage3;
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

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_MOGU_CRUCIBLE:
                    return crucibleGUID;
                case DATA_THUNDER_FORGE:
                    return thunderForgeGUID;
                case DATA_TRUNDER_FORGE_DOOR:
                    return doorGUID;
                default:
                    break;
            }
            return 0;
        }

    private:
        uint32 playerRole;
        uint64 doorGUID;
        uint64 crucibleGUID;
        uint64 wrathionGUID;
        uint64 thunderForgeGUID;

        std::vector<uint64> shaBeastGUIDs;
        std::vector<uint64> shaFiendGUIDs;
        std::vector<uint64> firstPhaseTrashGUIDs;

        uint32 stageData;
        uint32 waveCounter;
        uint32 waveCounter2;



        int8 eventStage;
        int8 eventStage2;
        int8 eventStage3;
        uint8 counter;
        uint8 jumpPos;
        uint8 lrStage2;
    };
};

bool Helper::IsNextStageAllowed(InstanceScript* instance, uint8 stage)
{
    switch (stage)
    {
        case STAGE_2:
            if (instance->GetData(DATA_ALLOWED_STAGE) == STAGE_1)
            {
                instance->SetData(DATA_ALLOWED_STAGE, STAGE_2);
                return true;
            }
            return false;
        case STAGE_3:
            if (instance->GetData(DATA_ALLOWED_STAGE) == STAGE_2)
            {
                instance->SetData(DATA_ALLOWED_STAGE, STAGE_3);
                return true;
            }
            return false;
        default:
            return false;
    }
    return false;
}

void doAction(Unit* unit, uint32 creature, uint8 action, float range /*= 200.0f*/)
{
    std::list<Creature*> creatures;
    GetCreatureListWithEntryInGrid(creatures, unit, creature, range);
    if (!creatures.empty())
        for (std::list<Creature*>::iterator creature = creatures.begin(); creature != creatures.end(); ++creature)
            (*creature)->AI()->DoAction(action);
}

void AddSC_instance_fall_of_shan_bu()
{
    new instance_fall_of_shan_bu();
}
