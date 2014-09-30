//UWoWCore
//Siege of Orgrimmar

#include "NewScriptPCH.h"
#include "VMapFactory.h"
#include "siege_of_orgrimmar.h"
#include "AccountMgr.h"

Position const LorewalkerChoSpawn[2]  = {
    {1448.236f, 312.6528f, 289.2837f, 4.652967f},
    {1441.406f, 988.1795f, 340.1876f, 1.985304f},   //fallen
};

DoorData const doorData[] =
{
    {GO_IMMERSEUS_EX_DOOR,                   DATA_IMMERSEUS,              DOOR_TYPE_PASSAGE,    BOUNDARY_NONE   },
    {GO_SHA_FIELD,                           DATA_F_PROTECTORS,           DOOR_TYPE_PASSAGE,    BOUNDARY_NONE   },
    {0,                                      0,                           DOOR_TYPE_ROOM,       BOUNDARY_NONE}, // END
};

class instance_siege_of_orgrimmar : public InstanceMapScript
{
public:
    instance_siege_of_orgrimmar() : InstanceMapScript("instance_siege_of_orgrimmar", 1136) { }

    struct instance_siege_of_orgrimmar_InstanceMapScript : public InstanceScript
    {
        instance_siege_of_orgrimmar_InstanceMapScript(Map* map) : InstanceScript(map) {}

        //Misc
        uint32 TeamInInstance;

        //GameObjects
        uint64 immerseusexdoorGuid;
        uint64 norushenexdoorGuid;
        std::vector<uint64> lightqGuids;
        
        //Creature
        uint64 LorewalkerChoGUIDtmp;
        uint64 immerseusGuid;
        uint64 npcpointGuid;
        uint64 rookGuid;
        uint64 sunGuid;
        uint64 heGuid;
        uint64 noryshenGuid;
        uint64 bhcGuid;
        uint64 bhGuid;
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
        uint64 npcGoldenLotosMoverGUID;
        uint64 npcGoldenLotosMainGUID;
        uint64 npcGoldenLotosGUID[3];
        uint64 npcEmbodiesGUID[6];

        void Initialize()
        {
            SetBossNumber(DATA_MAX);
            LoadDoorData(doorData);

            TeamInInstance = 0;

            //GameObject
            immerseusexdoorGuid  = 0;
            norushenexdoorGuid   = 0;
            lightqGuids.clear();
           
            //Creature
            npcGoldenLotosMoverGUID = 0;
            npcGoldenLotosMainGUID  = 0;
            LorewalkerChoGUIDtmp= 0;
            immerseusGuid       = 0;
            npcpointGuid        = 0;
            rookGuid            = 0;
            sunGuid             = 0;
            heGuid              = 0;
            noryshenGuid        = 0;
            bhcGuid             = 0;
            bhGuid              = 0;
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

            memset(npcGoldenLotosGUID, 0, 3 * sizeof(uint64));
            memset(npcEmbodiesGUID, 0, 6 * sizeof(uint64));
        }

        void OnPlayerEnter(Player* player)
        {
            if (!TeamInInstance)
                TeamInInstance = player->GetTeam();

            //not handle lorewalker summon if already done.
            if (LorewalkerChoGUIDtmp)
                return;

            if (GetBossState(DATA_IMMERSEUS) != DONE)
            {
                if (Creature* cho = instance->SummonCreature(NPC_LOREWALKER_CHO, LorewalkerChoSpawn[0]))
                {
                    LorewalkerChoGUIDtmp = cho->GetGUID();
                    cho->AI()->SetData(DATA_IMMERSEUS, NOT_STARTED);
                }
            }else if (GetBossState(DATA_F_PROTECTORS) != DONE)
            {
                if (Creature* cho = instance->SummonCreature(NPC_LOREWALKER_CHO, LorewalkerChoSpawn[1]))
                {
                    LorewalkerChoGUIDtmp = cho->GetGUID();
                    cho->AI()->SetData(DATA_F_PROTECTORS, NOT_STARTED);
                }
            }
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
            case NPC_GOLD_LOTOS_MOVER:
                npcGoldenLotosMoverGUID = creature->GetGUID();
                break;
            case NPC_GOLD_LOTOS_MAIN:
                npcGoldenLotosMainGUID = creature->GetGUID();
                break;
            case NPC_GOLD_LOTOS_HE:
                npcGoldenLotosGUID[0] = creature->GetGUID();
                break;
            case NPC_GOLD_LOTOS_SUN:
                npcGoldenLotosGUID[1] = creature->GetGUID();
                break;
            case NPC_GOLD_LOTOS_ROOK:
                npcGoldenLotosGUID[2] = creature->GetGUID();
                break;
            case NPC_EMBODIED_ANGUISH_OF_HE:
                npcEmbodiesGUID[0] = creature->GetGUID();
                break;
            case NPC_EMBODIED_DESPERATION_OF_SUN:
                npcEmbodiesGUID[1] = creature->GetGUID();
                break;
            case NPC_EMBODIED_DESPIRE_OF_SUN:
                npcEmbodiesGUID[2] = creature->GetGUID();
                break;
            case NPC_EMBODIED_MISERY_OF_ROOK:
                npcEmbodiesGUID[3] = creature->GetGUID();
                break;
            case NPC_EMBODIED_GLOOM_OF_ROOK:
                npcEmbodiesGUID[4] = creature->GetGUID();
                break;
            case NPC_EMBODIED_SORROW_OF_ROOK:
                npcEmbodiesGUID[5] = creature->GetGUID();
                break;
                
            //  
            case NPC_NORUSHEN:  
                noryshenGuid = creature->GetGUID();
                break;
            case NPC_B_H_CONTROLLER:
                bhcGuid = creature->GetGUID();
                break;
            case NPC_BLIND_HATRED:
                bhGuid = creature->GetGUID();
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
            case GO_SHA_FIELD:
                AddDoor(go, true);
                break;
            case GO_LIGTH_QUARANTINE:
            case GO_LIGTH_QUARANTINE_2:
            case GO_LIGTH_QUARANTINE_3:
            case GO_LIGTH_QUARANTINE_4:
            case GO_LIGTH_QUARANTINE_5:
            case GO_LIGTH_QUARANTINE_6:
                lightqGuids.push_back(go->GetGUID());
                break;
            case GO_NORUSHEN_EX_DOOR:
                norushenexdoorGuid = go->GetGUID();
                break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
            case DATA_IMMERSEUS:
                if (state == DONE)
                {
                    if (Creature* bq = instance->GetCreature(LorewalkerChoGUIDtmp))
                        bq->AI()->SetData(DATA_IMMERSEUS, DONE);
                    //Protectors intro
                    if (Creature* cho = instance->SummonCreature(NPC_LOREWALKER_CHO, LorewalkerChoSpawn[1]))
                    {
                        LorewalkerChoGUIDtmp = cho->GetGUID();
                        cho->AI()->SetData(DATA_F_PROTECTORS, NOT_STARTED);
                    }
                }
                break;
            case DATA_F_PROTECTORS:
            {
                if (state == DONE)
                {
                    sLog->outU(">>>>>>>>>>>>>>>>>>>> DONE");
                    if (Creature* bq = instance->GetCreature(LorewalkerChoGUIDtmp))
                        bq->AI()->SetData(DATA_F_PROTECTORS, DONE);
                }
                break;
            }
            case DATA_NORUSHEN:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector<uint64>::const_iterator guid = lightqGuids.begin(); guid != lightqGuids.end(); guid++)
                            HandleGameObject(*guid, true);
                        break;
                    case IN_PROGRESS:
                        for (std::vector<uint64>::const_iterator guid = lightqGuids.begin(); guid != lightqGuids.end(); guid++)
                            HandleGameObject(*guid, false);
                        break;
                    case DONE:
                        for (std::vector<uint64>::const_iterator guid = lightqGuids.begin(); guid != lightqGuids.end(); guid++)
                            HandleGameObject(*guid, true);
                        HandleGameObject(norushenexdoorGuid, true);
                        break;
                    }
                }
                break;
            }
            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
        }

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
                case NPC_GOLD_LOTOS_MOVER:
                    return npcGoldenLotosMoverGUID;
                case NPC_GOLD_LOTOS_MAIN:
                    return npcGoldenLotosMainGUID;
                case NPC_GOLD_LOTOS_HE:
                    return npcGoldenLotosGUID[0];
                case NPC_GOLD_LOTOS_SUN:
                    return npcGoldenLotosGUID[1];
                case NPC_GOLD_LOTOS_ROOK:
                    return npcGoldenLotosGUID[2];
                case NPC_EMBODIED_ANGUISH_OF_HE:
                    return npcEmbodiesGUID[0];
                case NPC_EMBODIED_DESPERATION_OF_SUN:
                    return npcEmbodiesGUID[1];
                case NPC_EMBODIED_DESPIRE_OF_SUN:
                    return npcEmbodiesGUID[2];
                case NPC_EMBODIED_MISERY_OF_ROOK:
                    return npcEmbodiesGUID[3];
                case NPC_EMBODIED_GLOOM_OF_ROOK:
                    return npcEmbodiesGUID[4];
                case NPC_EMBODIED_SORROW_OF_ROOK:
                    return npcEmbodiesGUID[5];

                //  
                case NPC_NORUSHEN:  
                    return noryshenGuid;
                case NPC_B_H_CONTROLLER:
                    return bhcGuid;
                case NPC_BLIND_HATRED:
                    return bhGuid;
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
                case NPC_LOREWALKER_CHO:
                    return LorewalkerChoGUIDtmp;
            }
            return 0;
        }

        void CreatureDies(Creature* creature, Unit* /*killer*/)
        {
            //switch(creature->GetEntry())
            //{

            //}
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

        
        bool CheckRequiredBosses(uint32 bossId, Player const* player = NULL) const
        {
            // Only on win build no check for complete boses.
            #ifdef WIN32
            return true;
            #endif

            if (player && AccountMgr::IsGMAccount(player->GetSession()->GetSecurity()))
                return true;

            switch (bossId)
            {
                case DATA_IMMERSEUS:
                    return true;
                    //no break
                case DATA_F_PROTECTORS:
                    return GetBossState(DATA_IMMERSEUS) == DONE;
                    //no break
                case DATA_NORUSHEN:
                    return GetBossState(DATA_F_PROTECTORS) == DONE;
            }

            return true;
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
