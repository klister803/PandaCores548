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
#include "thunder_forge.h"

class instance_thunder_forge : public InstanceMapScript
{
public:
    instance_thunder_forge() : InstanceMapScript("instance_thunder_forge", 1126) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_thunder_forge_InstanceMapScript(map);
    }

    struct instance_thunder_forge_InstanceMapScript : public InstanceScript
    {
        instance_thunder_forge_InstanceMapScript(Map* map) : InstanceScript(map)
        { }

        void Initialize()
        {
            playerRole = ROLES_DEFAULT;

            crucibleGUID = 0;
            doorGUID = 0;
            wrathionGUID = 0;
            forgemasterGUID = 0;
            celectialBlacksmithGUID = 0;
            celectialDefenderGUID = 0;

            stageData = STAGE_1;
            waveCounter = 0;
            s1p2 = 0;
            s2p1 = 0;
            s2p2 = 0;
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
                case GO_INVISIBLE_WALL:
                    invisibleWallGUID = go->GetGUID();
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
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    wrathionGUID = creature->GetGUID();
                    break;
                case NPC_SHADO_PAN_WARRIOR:
                    warriorGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SHADO_PAN_DEFENDER:
                    defenderGUID = creature->GetGUID();
                    break;
                case NPC_FORGEMASTER_VULKON:
                    forgemasterGUID = creature->GetGUID();
                    break;
                case NPC_CELESTIAL_BLACKSMITH:
                    celectialBlacksmithGUID = creature->GetGUID();
                    break;
                case NPC_CELESTIAL_DEFENDER:
                    celectialDefenderGUID = creature->GetGUID();
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
                        case 0:
                            if (Creature* cre = instance->GetCreature(defenderGUID))
                                cre->AI()->DoAction(ACTION_1);
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
                        {
                            // HandleGameObject(GetData64(doorGUID), true); EVIL
                            Map::PlayerList const& players = instance->GetPlayers();
                            if (Player* plr = players.begin()->getSource())
                            {
                                if (GameObject* obj = GameObject::GetGameObject(*plr, invisibleWallGUID))
                                    obj->DestroyForPlayer(plr);

                                if (GameObject* obj = GameObject::GetGameObject(*plr, doorGUID))
                                    obj->DestroyForPlayer(plr);
                            }

                            if (Creature* cre = instance->GetCreature(wrathionGUID))
                                cre->AI()->DoAction(ACTION_3);
                            break;
                        }
                    }
                case DATA_SECOND_STAGE_FIRST_STEP:
                {
                    s2p1 = data;
                    switch (data)
                    {
                        case FAIL:
                            if (Creature* cre = instance->GetCreature(wrathionGUID))
                                cre->AI()->DoAction(ACTION_1);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case DATA_COMPLETE_SECOND_STAGE_SECOND_STEP:
                    s2p2 = data;
                    if (data == DONE)
                    {
                        Map::PlayerList const& players = instance->GetPlayers();
                        if (Player* plr = players.begin()->getSource())
                            plr->KilledMonsterCredit(70094, 0); //< set quest 32593 completed

                        if (Creature* cre = instance->GetCreature(wrathionGUID))
                        {
                            cre->AI()->DoAction(ACTION_9);
                            cre->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        }
                    }
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
                case DATA_CELESTIAL_BLACKSMITH:
                    return celectialBlacksmithGUID;
                case DATA_CELESTIAL_DEFENDER:
                    return celectialDefenderGUID;
                default:
                    break;
            }
            return 0;
        }

    private:
        uint32 playerRole;

        uint64 doorGUID;
        uint64 invisibleWallGUID;
        uint64 crucibleGUID;
        uint64 wrathionGUID;
        uint64 defenderGUID;
        uint64 forgemasterGUID;
        uint64 celectialBlacksmithGUID;
        uint64 celectialDefenderGUID;

        std::vector<uint64> warriorGUIDs;

        uint32 stageData;
        uint32 waveCounter;
        uint32 s1p2;
        uint32 s2p1;
        uint32 s2p2;
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

void AddSC_instance_thunder_forge()
{
    new instance_thunder_forge();
}
