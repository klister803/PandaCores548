//UWoWCore
//Heart of Fear

#include "ScriptPCH.h"
#include "VMapFactory.h"
#include "heart_of_fear.h"

class instance_heart_of_fear : public InstanceMapScript
{
public:
    instance_heart_of_fear() : InstanceMapScript("instance_heart_of_fear", 1009) { }


    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_heart_of_fear_InstanceMapScript(map);
    }

    struct instance_heart_of_fear_InstanceMapScript : public InstanceScript
    {
        instance_heart_of_fear_InstanceMapScript(Map* map) : InstanceScript(map) {}

        //GameObjects
        uint64 vizierentdoorGuid;
        uint64 vizierexdoorGuid;
        uint64 tayakexdoorGuid;

        //Creature
        uint64 zorlokGuid;
        uint64 gascontrollerGuid;
        uint64 tayakGuid;
        uint64 garalonGuid;
        uint64 merjalakGuid;
        uint64 unsokGuid;
        uint64 shekzeerGuid;
        
        //Arrays
       
        void Initialize()
        {
            SetBossNumber(7);

            //GameObject
            vizierentdoorGuid = 0;
            vizierexdoorGuid  = 0;
            tayakexdoorGuid   = 0;

            //Creature
            zorlokGuid        = 0;
            gascontrollerGuid = 0;
            tayakGuid         = 0;
            garalonGuid       = 0;
            merjalakGuid      = 0;
            unsokGuid         = 0;
            shekzeerGuid      = 0;

            //Arrays
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            { 
            case NPC_VIZIER_ZORLOK:
                zorlokGuid = creature->GetGUID();
                break;
            case NPC_GAS_CONTROLLER:
                gascontrollerGuid = creature->GetGUID();
                break;
            case NPC_LORD_TAYAK:
                tayakGuid = creature->GetGUID();
                break;
            case NPC_GAROLON:
                garalonGuid = creature->GetGUID();
            case NPC_MERJALAK:
                merjalakGuid = creature->GetGUID();
                break;
            case NPC_UNSOK:
                unsokGuid = creature->GetGUID();
                break;
            case NPC_SHEKZEER:
                shekzeerGuid = creature->GetGUID();
                break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
            case GO_VIZIER_ENT_DOOR:
                vizierentdoorGuid = go->GetGUID();
                break;
            case GO_VIZIER_EX_DOOR:
                vizierexdoorGuid = go->GetGUID();
                break;
            case GO_TAYAK_EX_DOOR:
                tayakexdoorGuid = go->GetGUID();
                break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
            case DATA_VIZIER_ZORLOK:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        HandleGameObject(vizierentdoorGuid, true);
                        break;
                    case DONE:
                        HandleGameObject(vizierentdoorGuid, true);
                        HandleGameObject(vizierexdoorGuid, true); 
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(vizierentdoorGuid, false);
                        break;
                    }
                    break;
                }
            case DATA_LORD_TAYAK:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        HandleGameObject(vizierexdoorGuid, true);
                        break;
                    case DONE:
                        HandleGameObject(vizierexdoorGuid, true);
                        //HandleGameObject(tayakexdoorGuid, true); next boss not ready...
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(vizierexdoorGuid, false);
                        break;
                    }
                    break;
                }
            }
            return true;
        }

        void SetData(uint32 type, uint32 data){}

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case NPC_VIZIER_ZORLOK:
                    return zorlokGuid;
                case NPC_GAS_CONTROLLER:
                    return gascontrollerGuid;
                case NPC_LORD_TAYAK:
                    return tayakGuid;
                case NPC_GAROLON:
                    return garalonGuid;
                case NPC_MERJALAK:
                    return merjalakGuid;
                case NPC_UNSOK:
                    return unsokGuid;
                case NPC_SHEKZEER:
                    return shekzeerGuid;
            }
            return 0;
        }

        bool IsWipe()
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();

            if (PlayerList.isEmpty())
                return true;

            for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
            {
                Player* player = Itr->getSource();

                if (!player)
                    continue;

                if (player->isAlive() && !player->isGameMaster() && !player->HasAura(115877)) // Aura 115877 = Totaly Petrified
                    return false;
            }

            return true;
        }

        std::string GetSaveData()
        {
            std::ostringstream saveStream;
            saveStream << GetBossSaveData() << " ";
            return saveStream.str();
        }

        void Load(const char* data)
        {
            std::istringstream loadStream(LoadBossState(data));
            uint32 buff;
            for (uint32 i = 0; i < 7; ++i)
                loadStream >> buff;
        }
    };
};

void AddSC_instance_heart_of_fear()
{
    new instance_heart_of_fear();
}
