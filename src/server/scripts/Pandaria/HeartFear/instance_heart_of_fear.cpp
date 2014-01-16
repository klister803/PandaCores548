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
        uint64 garalonentdoorGuid;
        uint64 meljarakexdoorGuid;
        uint64 unsokendoorGuid;
        uint64 unsokexdoorGuid;

        std::vector<uint64> vizierarenadoorGuids;
        std::vector<uint64> garaloncdoorGuids;
        std::vector<uint64> garalonexdoorGuids;

        //Creature
        uint64 zorlokGuid;
        uint64 gascontrollerGuid;
        uint64 tayakGuid;
        uint64 garalonGuid;
        uint64 meljarakGuid;
        uint64 unsokGuid;
        uint64 ambermonsterGuid;
        uint64 shekzeerGuid;

        uint64 srathik[3];
        uint64 zarthik[3];
        uint64 korthik[3];
        std::vector<uint64> meljaraksoldiersGuids;
        
        void Initialize()
        {
            SetBossNumber(7);

            //GameObject
            vizierentdoorGuid   = 0;
            vizierexdoorGuid    = 0;
            tayakexdoorGuid     = 0;
            garalonentdoorGuid  = 0;
            meljarakexdoorGuid  = 0;
            unsokendoorGuid     = 0;
            unsokexdoorGuid     = 0;

            vizierarenadoorGuids.clear();
            garaloncdoorGuids.clear();
            garalonexdoorGuids.clear();

            //Creature
            zorlokGuid          = 0;
            gascontrollerGuid   = 0;
            tayakGuid           = 0;
            garalonGuid         = 0;
            meljarakGuid        = 0;
            unsokGuid           = 0;
            ambermonsterGuid    = 0;
            shekzeerGuid        = 0;

            for (uint8 n = 0; n < 3; n++)
            {
                srathik[n] = 0;
                zarthik[n] = 0;
                korthik[n] = 0;
            }
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
            case NPC_GARALON:
                garalonGuid = creature->GetGUID();
                break;
            case NPC_MELJARAK:
                meljarakGuid = creature->GetGUID();
                break;
            case NPC_SRATHIK:
                for (uint8 n = 0; n < 3; n++)
                {
                    if (srathik[n] == 0)
                    {
                        srathik[n] = creature->GetGUID();
                        meljaraksoldiersGuids.push_back(srathik[n]);
                        break;
                    }
                }
                break;
            case NPC_ZARTHIK:
                for (uint8 n = 0; n < 3; n++)
                {
                    if (zarthik[n] == 0)
                    {
                        zarthik[n] = creature->GetGUID();
                        meljaraksoldiersGuids.push_back(zarthik[n]);
                        break;
                    }
                }
                break;
            case NPC_KORTHIK:
                for (uint8 n = 0; n < 3; n++)
                {
                    if (korthik[n] == 0)
                    {
                        korthik[n] = creature->GetGUID();
                        meljaraksoldiersGuids.push_back(korthik[n]);
                        break;
                    }
                }
                break;
            case NPC_UNSOK:
                unsokGuid = creature->GetGUID();
                break;
            case NPC_AMBER_MONSTER:
                ambermonsterGuid = creature->GetGUID();
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
            case GO_VIZIER_ARENA_DOOR:
                vizierarenadoorGuids.push_back(go->GetGUID());
                break;
            case GO_VIZIER_EX_DOOR:
                vizierexdoorGuid = go->GetGUID();
                break;
            case GO_TAYAK_EX_DOOR:
                tayakexdoorGuid = go->GetGUID();
                break;
            case GO_GARALON_ENT_DOOR:
                garalonentdoorGuid = go->GetGUID();
                break;
            case GO_GARALON_COMBAT_DOOR:
                garaloncdoorGuids.push_back(go->GetGUID());
                break;
            case GO_GARALON_EX_DOOR:
                garalonexdoorGuids.push_back(go->GetGUID());
                break;
            case GO_MELJARAK_EX_DOOR:
                meljarakexdoorGuid = go->GetGUID();
                break;
            case GO_UNSOK_EN_DOOR:
                unsokendoorGuid = go->GetGUID();
                break;
            case GO_UNSOK_EX_DOOR:
                unsokexdoorGuid = go->GetGUID();
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
                        for (std::vector<uint64>::const_iterator guid = vizierarenadoorGuids.begin(); guid != vizierarenadoorGuids.end(); guid++)
                            HandleGameObject(*guid, true);
                        break;
                    case DONE:
                        HandleGameObject(vizierentdoorGuid, true);
                        HandleGameObject(vizierexdoorGuid, true); 
                        for (std::vector<uint64>::const_iterator guid = vizierarenadoorGuids.begin(); guid != vizierarenadoorGuids.end(); guid++)
                            HandleGameObject(*guid, true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(vizierentdoorGuid, false);
                        for (std::vector<uint64>::const_iterator guid = vizierarenadoorGuids.begin(); guid != vizierarenadoorGuids.end(); guid++)
                            HandleGameObject(*guid, false);
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
                        HandleGameObject(tayakexdoorGuid, true);
                        HandleGameObject(garalonentdoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(vizierexdoorGuid, false);
                        break;
                    }
                    break;
                }
            case DATA_GARALON:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector<uint64>::const_iterator guid = garaloncdoorGuids.begin(); guid != garaloncdoorGuids.end(); guid++)
                            HandleGameObject(*guid, true);
                        break;
                    case DONE:
                        for (std::vector<uint64>::const_iterator guid = garaloncdoorGuids.begin(); guid != garaloncdoorGuids.end(); guid++)
                            HandleGameObject(*guid, true);
                        
                        for (std::vector<uint64>::const_iterator guids = garalonexdoorGuids.begin(); guids != garalonexdoorGuids.end(); guids++)
                            HandleGameObject(*guids, true);
                        break;
                    case IN_PROGRESS:
                        for (std::vector<uint64>::const_iterator guid = garaloncdoorGuids.begin(); guid != garaloncdoorGuids.end(); guid++)
                            HandleGameObject(*guid, false);
                        break;
                    }
                    break;
                }
            case DATA_MELJARAK:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector<uint64>::const_iterator guids = garalonexdoorGuids.begin(); guids != garalonexdoorGuids.end(); guids++)
                            HandleGameObject(*guids, true);

                        for (std::vector<uint64>::const_iterator guid = meljaraksoldiersGuids.begin(); guid != meljaraksoldiersGuids.end(); guid++)
                        {
                            if (Creature* soldier = instance->GetCreature(*guid))
                            {
                                if (!soldier->isAlive())
                                {
                                    soldier->Respawn();
                                    soldier->GetMotionMaster()->MoveTargetedHome();
                                }
                                else if (soldier->isAlive() && soldier->isInCombat())
                                    soldier->AI()->EnterEvadeMode();
                            }
                        }
                        break;
                    case IN_PROGRESS:
                        for (std::vector<uint64>::const_iterator guids = garalonexdoorGuids.begin(); guids != garalonexdoorGuids.end(); guids++)
                            HandleGameObject(*guids, false);

                        for (std::vector<uint64>::const_iterator guid = meljaraksoldiersGuids.begin(); guid != meljaraksoldiersGuids.end(); guid++)
                        {
                            if (Creature* soldier = instance->GetCreature(*guid))
                            {
                                if (soldier->isAlive() && !soldier->isInCombat())
                                    soldier->AI()->DoZoneInCombat(soldier, 100.0f);
                            }
                        }
                        break;
                    case DONE:
                        HandleGameObject(meljarakexdoorGuid, true);
                        HandleGameObject(unsokendoorGuid, true);
                        for (std::vector<uint64>::const_iterator guids = garalonexdoorGuids.begin(); guids != garalonexdoorGuids.end(); guids++)
                            HandleGameObject(*guids, true);
                        break;
                    }
                    break;
                }
            case DATA_UNSOK:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        HandleGameObject(unsokendoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(unsokendoorGuid, false);
                        break;
                    case DONE:
                        HandleGameObject(unsokendoorGuid, true);
                        //HandleGameObject(unsokexdoorGuid, true); next boss not ready
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
                case NPC_GARALON:
                    return garalonGuid;
                case NPC_MELJARAK:
                    return meljarakGuid;
                //Merjalak Soldiers
                case NPC_SRATHIK_1:
                    return srathik[0];
                case NPC_SRATHIK_2:
                    return srathik[1];
                case NPC_SRATHIK_3:
                    return srathik[2];
                case NPC_ZARTHIK_1:
                    return zarthik[0];
                case NPC_ZARTHIK_2:
                    return zarthik[1];
                case NPC_ZARTHIK_3:
                    return zarthik[2];
                case NPC_KORTHIK_1:
                    return korthik[0];
                case NPC_KORTHIK_2:
                    return korthik[1];
                case NPC_KORTHIK_3:
                    return korthik[2];
                //
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
