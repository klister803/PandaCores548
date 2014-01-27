//UWoWCore
//Terrace of Endless Spring

#include "ScriptPCH.h"
#include "VMapFactory.h"
#include "terrace_of_endless_spring.h"

class instance_terrace_of_endless_spring : public InstanceMapScript
{
public:
    instance_terrace_of_endless_spring() : InstanceMapScript("instance_terrace_of_endless_spring", 996) { }


    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_terrace_of_endless_spring_InstanceMapScript(map);
    }

    struct instance_terrace_of_endless_spring_InstanceMapScript : public InstanceScript
    {
        instance_terrace_of_endless_spring_InstanceMapScript(Map* map) : InstanceScript(map) {}

        //GameObjects

        //Creature
        uint64 kaolanGuid;
        uint64 regailGuid;
        uint64 asaniGuid;
        uint64 tsulongGuid;
        uint64 leishiGuid;
        uint64 shaGuid;
        
        void Initialize()
        {
            SetBossNumber(5);

            //GameObject
           
            //Creature
            kaolanGuid    = 0;
            regailGuid    = 0;
            asaniGuid     = 0;
            tsulongGuid   = 0;
            leishiGuid    = 0;
            shaGuid       = 0;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            { 
            case NPC_PROTECTOR_KAOLAN:
                kaolanGuid = creature->GetGUID();
                break;
            case NPC_ELDER_REGAIL:
                regailGuid = creature->GetGUID();
                break;
            case NPC_ELDER_ASANI:
                asaniGuid = creature->GetGUID();
                break;
            case NPC_TSULONG:
                tsulongGuid = creature->GetGUID();
                break;
            case NPC_LEI_SHI:
                leishiGuid = creature->GetGUID();
                break;
            case NPC_SHA_OF_FEAR:
                shaGuid = creature->GetGUID();
                break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
           /* switch (go->GetEntry())
            {
            }*/
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;
            
           /* switch (id)
            {
            }*/
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
            //Protectors
            case NPC_PROTECTOR_KAOLAN:
                return kaolanGuid;
            case NPC_ELDER_REGAIL:
                return regailGuid;
            case NPC_ELDER_ASANI:
                return asaniGuid;
            //
            case NPC_TSULONG:
                return tsulongGuid;
            case NPC_LEI_SHI:
                return leishiGuid;
            case NPC_SHA_OF_FEAR:
                return shaGuid;
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

void AddSC_instance_terrace_of_endless_spring()
{
    new instance_terrace_of_endless_spring();
}
