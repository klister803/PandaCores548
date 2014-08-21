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
        uint64 immerseusexdoorGuid;
        
        //Creature
        uint64 immerseusGuid;
        uint64 npcpointGuid;
        uint64 rookGuid;
        uint64 sunGuid;
        uint64 heGuid;
        uint64 noryshenGuid;
        uint64 shaGuid;
        uint64 galakrasGuid;
        uint64 juggernautGuid;
        uint64 korkronGuid;
        uint64 nazgrimGuid;
        uint64 malkorokGuid;
        uint64 thokGuid;
        uint64 blackfuseGuid;
        uint64 kilrukGuid;
        uint64 xarilGuid;
        uint64 kaztikGuid;
        uint64 korvenGuid;
        uint64 iyyokykGuid;
        uint64 karozGuid;
        uint64 skeerGuid;
        uint64 rikkalGuid;
        uint64 hisekGuid;
        uint64 garroshGuid;
        
        void Initialize()
        {
            SetBossNumber(15);

            //GameObject
            immerseusexdoorGuid  = 0;
           
            //Creature
            immerseusGuid       = 0;
            npcpointGuid        = 0;
            rookGuid            = 0;
            sunGuid             = 0;
            heGuid              = 0;
            noryshenGuid        = 0;
            shaGuid             = 0;
            galakrasGuid        = 0;
            juggernautGuid      = 0;
            korkronGuid         = 0;
            nazgrimGuid         = 0;
            malkorokGuid        = 0;
            thokGuid            = 0;
            blackfuseGuid       = 0;
            kilrukGuid          = 0;
            xarilGuid           = 0;
            kaztikGuid          = 0;
            korvenGuid          = 0;
            iyyokykGuid         = 0;
            karozGuid           = 0;
            skeerGuid           = 0;
            rikkalGuid          = 0;
            hisekGuid           = 0;
            garroshGuid         = 0;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
            case NPC_IMMERSEUS:
                immerseusGuid = creature->GetGUID();
                break;
            case NPC_PUDDLE_POINT:
                npcpointGuid = creature->GetGUID();
                break;
            //Fallen Protectors
            case NPC_ROOK_STONETOE: 
                rookGuid = creature->GetGUID();
                break;
            case NPC_SUN_TENDERHEART:
                sunGuid = creature->GetGUID();
                break;
            case NPC_HE_SOFTFOOT:
                heGuid = creature->GetGUID();
                break;
            //  
            case NPC_NORUSHEN:  
                noryshenGuid = creature->GetGUID();
                break;
            case NPC_SHA_OF_PRIDE: 
                shaGuid = creature->GetGUID();
                break;
            case NPC_GALAKRAS: 
                galakrasGuid = creature->GetGUID();
                break;
            case NPC_IRON_JUGGERNAUT: 
                juggernautGuid = creature->GetGUID();
                break;
            case NPC_KORKRON_D_SHAMAN:
                korkronGuid = creature->GetGUID();
                break;
            case NPC_GENERAL_NAZGRIM:
                nazgrimGuid = creature->GetGUID();
                break;
            case NPC_MALKOROK:  
                malkorokGuid = creature->GetGUID();
                break;
            case NPC_THOK:  
                thokGuid = creature->GetGUID();
                break;
            case NPC_BLACKFUSE: 
                blackfuseGuid = creature->GetGUID();
                break;
            //Paragons of the Klaxxi
            case NPC_KILRUK:  
                kilrukGuid = creature->GetGUID();
                break;
            case NPC_XARIL:
                xarilGuid = creature->GetGUID();
                break;
            case NPC_KAZTIK:   
                kaztikGuid = creature->GetGUID();
                break;
            case NPC_KORVEN: 
                korvenGuid = creature->GetGUID();
                break;
            case NPC_IYYOKYK:
                iyyokykGuid = creature->GetGUID();
                break;
            case NPC_KAROZ:
                karozGuid = creature->GetGUID();
                break;
            case NPC_SKEER:
                skeerGuid = creature->GetGUID();
                break;
            case NPC_RIKKAL:
                rikkalGuid = creature->GetGUID();
                break;
            case NPC_HISEK:
                hisekGuid = creature->GetGUID();
                break;
            //
            case NPC_GARROSH:
                garroshGuid = creature->GetGUID();
                break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {    
            switch (go->GetEntry())
            {
            case GO_IMMERSEUS_EX_DOOR:
                immerseusexdoorGuid = go->GetGUID();
                break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
            case DATA_IMMEREUS:
               // if (state == DONE)
                   // HandleGameObject(immerseusexdoorGuid, true);
                break;
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
                case NPC_IMMERSEUS:
                    return immerseusGuid;
                case NPC_PUDDLE_POINT:
                    return npcpointGuid;
                //Fallen Protectors
                case NPC_ROOK_STONETOE: 
                    return rookGuid;
                case NPC_SUN_TENDERHEART:
                    return sunGuid;
                case NPC_HE_SOFTFOOT:
                    return heGuid;
                //  
                case NPC_NORUSHEN:  
                    return noryshenGuid;
                case NPC_SHA_OF_PRIDE: 
                    return shaGuid;
                case NPC_GALAKRAS: 
                    return galakrasGuid;
                case NPC_IRON_JUGGERNAUT: 
                    return juggernautGuid;
                case NPC_KORKRON_D_SHAMAN:
                    return korkronGuid;
                case NPC_GENERAL_NAZGRIM:
                    return nazgrimGuid;
                case NPC_MALKOROK:  
                    return malkorokGuid;
                case NPC_THOK:  
                    return thokGuid;
                case NPC_BLACKFUSE: 
                    return blackfuseGuid;
                //Paragons of the Klaxxi
                case NPC_KILRUK:  
                    return kilrukGuid;
                case NPC_XARIL:
                    return xarilGuid;
                case NPC_KAZTIK:   
                    return kaztikGuid;
                case NPC_KORVEN: 
                    return korvenGuid;
                case NPC_IYYOKYK:
                    return iyyokykGuid;
                case NPC_KAROZ:
                    return karozGuid;
                case NPC_SKEER:
                    return skeerGuid;
                case NPC_RIKKAL:
                    return rikkalGuid;
                case NPC_HISEK:
                    return hisekGuid;
                //
                case NPC_GARROSH:
                    return garroshGuid;
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
            for (uint32 i = 0; i < 15; ++i)
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
