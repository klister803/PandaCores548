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
            forgemasterGUID = 0;

            shaBeastGUIDs.clear();
            shaFiendGUIDs.clear();
            firstPhaseNormalTrashGUIDs.clear();

            stageData = STAGE_1;
            waveCounter = 0;
            s1p2 = 0;

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
                case NPC_THUNDER_FORGE:
                    thunderForgeGUID = creature->GetGUID();
                    creature->SetVisible(false);
                    break;
                case NPC_THUNDER_FORGE_2:
                case NPC_THUNDER_FORGE_3:
                case NPC_THUNDER_FORGE_CRUCIBLE:
                case NPC_INVISIBLE_STALKER:
                case NPC_LIGHTING_PILAR_BEAM_STALKER:
                case NPC_LIGHTING_PILAR_SPARK_STALKER:
                    creature->SetVisible(false);
                    break;
                case NPC_SHA_BEAST:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetPhaseMask(12, true);
                    shaBeastGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHA_FIEND:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetPhaseMask(12, true);
                    shaFiendGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHANZE_BATTLEMASTER:
                case NPC_SHANZE_SHADOWCASTER:
                case NPC_SHANZE_WARRIOR:
                case NPC_SHANZE_ELECTRO_CUTIONER:
                case NPC_SHANZE_ELECTRO_CUTIONER2:
                case NPC_SHANZE_PYROMANCER:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetPhaseMask(12, true);
                    firstPhaseNormalTrashGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHADO_PAN_WARRIOR:
                    warriorGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHADO_PAN_DEFENDER:
                    defenderGUID = creature->GetGUID();
                    break;
                case NPC_FORGEMASTER_VULKON:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetPhaseMask(12, true);
                    forgemasterGUID = creature->GetGUID();
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
                case NPC_SHANZE_ELECTRO_CUTIONER2:
                case NPC_SHANZE_PYROMANCER:
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
                    switch (data)
                    {
                        case ROLES_DEFAULT:
                            break;
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
                    switch (data)
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
                    switch (data)
                    {
                        case DONE:
                            AddCreaturesToBattle();
                            break;
                        case SPECIAL:
                            break;
                        default:
                            break;
                    }
                    break;
                case DATA_STAGE1_P2:
                    s1p2 = data;
                    switch (data)
                    {
                        case IN_PROGRESS:
                            if (Creature* cre = instance->GetCreature(forgemasterGUID))
                            {
                                cre->SetReactState(REACT_AGGRESSIVE);
                                cre->SetPhaseMask(1, true);
                                cre->AI()->DoAction(ACTION_1);
                            }
                            break;
                        case DONE:
                            HandleGameObject(GetData64(doorGUID), true);

                            if (Creature* cre = instance->GetCreature(wrathionGUID))
                                cre->AI()->DoAction(ACTION_3);
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

        void AddCreaturesToBattle()
        {
            uint8 count = 0;
            uint8 amount = firstPhaseNormalTrashGUIDs.size() / 4;
            for (std::vector<uint64>::const_iterator itr = firstPhaseNormalTrashGUIDs.begin(); itr != firstPhaseNormalTrashGUIDs.end(); itr++)
                if (Creature* cre = instance->GetCreature(*itr))
                {
                    cre->SetPhaseMask(1, true);
                    cre->SetReactState(REACT_AGGRESSIVE);
                    cre->AI()->AttackStart(instance->GetCreature(defenderGUID));

                    firstPhaseNormalTrashGUIDs.erase(itr);

                    count++;
                    if (count > amount)
                        return;
                }

            SetData(DATA_WAVE_COUNTER, SPECIAL);
            return;
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
                case DATA_STAGE1_P2:
                    return s1p2;
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
                case DATA_WARRIOR_1:
                    return warriorGUIDs[0];
                case DATA_WARRIOR_2:
                    return warriorGUIDs[1];
                case DATA_DEFENDER:
                    return defenderGUID;
                case DATA_TRUNDER_FORGE_DOOR:
                    return doorGUID;
                case DATA_WRATHION:
                    return wrathionGUID;
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
        uint64 defenderGUID;
        uint64 forgemasterGUID;

        std::vector<uint64> shaBeastGUIDs;
        std::vector<uint64> shaFiendGUIDs;
        std::vector<uint64> firstPhaseNormalTrashGUIDs;
        std::vector<uint64> warriorGUIDs;

        uint32 stageData;
        uint32 waveCounter;
        uint32 s1p2;
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
