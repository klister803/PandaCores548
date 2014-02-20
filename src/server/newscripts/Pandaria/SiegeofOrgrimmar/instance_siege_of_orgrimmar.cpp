//UWoWCore
//Siege of Orgrimmar

#include "NewScriptPCH.h"
#include "VMapFactory.h"
#include "siege_of_orgrimmar.h"

class instance_siege_of_orgrimmar : public InstanceMapScript
{
public:
    instance_siege_of_orgrimmar() : InstanceMapScript("instance_siege_of_orgrimmar", 1136) { }

    struct instance_siege_of_orgrimmar_InstanceMapScript : public InstanceScript
    {
        instance_siege_of_orgrimmar_InstanceMapScript(Map* map) : InstanceScript(map) {}

        //GameObjects
        
        //Creature
        
        void Initialize()
        {
            SetBossNumber(14);

            //GameObject
           
            //Creature
        }

        void OnCreatureCreate(Creature* creature)
        {
        }

        void OnGameObjectCreate(GameObject* go)
        {    
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            return true;
        }

        void SetData(uint32 type, uint32 data){}

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
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

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_siege_of_orgrimmar_InstanceMapScript(map);
    }
};

void AddSC_instance_siege_of_orgrimmar()
{
    new instance_siege_of_orgrimmar();
}
