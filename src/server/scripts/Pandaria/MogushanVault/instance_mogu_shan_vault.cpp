/*
    Dungeon : Template of Mogu'shan Palace 87-89
    Instance General Script
    Jade servers
*/

#include "ScriptPCH.h"
#include "VMapFactory.h"
#include "mogu_shan_vault.h"

DoorData const doorData[] =
{
    {GOB_STONE_GUARD_DOOR_ENTRANCE,          DATA_STONE_GUARD,          DOOR_TYPE_ROOM,       BOUNDARY_S   },
    {GOB_STONE_GUARD_DOOR_EXIT,              DATA_STONE_GUARD,          DOOR_TYPE_PASSAGE,    BOUNDARY_N   },
    {GOB_FENG_DOOR_FENCE,                    DATA_FENG,                 DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_FENG_DOOR_EXIT,                     DATA_FENG,                 DOOR_TYPE_PASSAGE,    BOUNDARY_N   },
    {GOB_GARAJAL_FENCE,                      DATA_GARAJAL,              DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    //{GOB_GARAJAL_EXIT,                       DATA_GARAJAL,              DOOR_TYPE_PASSAGE,    BOUNDARY_W   },
    //{GOB_SPIRIT_KINGS_WIND_WALL,             DATA_SPIRIT_KINGS          DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    //{GOB_SPIRIT_KINGS_EXIT,                  DATA_SPIRIT_KINGS,         DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    //{GOB_ELEGON_DOOR_ENTRANCE,               DATA_SPIRIT_KINGS,         DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    //{GOB_ELEGON_CELESTIAL_DOOR,              DATA_ELEGON,               DOOR_TYPE_ROOM,       BOUNDARY_E   },
    //{GOB_WILL_OF_EMPEROR_ENTRANCE,           DATA_ELEGON,               DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {0,                                      0,                         DOOR_TYPE_ROOM,       BOUNDARY_NONE},// END
};

class instance_mogu_shan_vault : public InstanceMapScript
{
public:
    instance_mogu_shan_vault() : InstanceMapScript("instance_mogu_shan_vault", 1008) { }


    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_mogu_shan_vault_InstanceMapScript(map);
    }

    struct instance_mogu_shan_vault_InstanceMapScript : public InstanceScript
    {
        instance_mogu_shan_vault_InstanceMapScript(Map* map) : InstanceScript(map) {}

        int8   randomDespawnStoneGuardian;
        uint8  willOfEmperorPhase;
        uint32 actualPetrifierEntry;
        uint32 StoneGuardPetrificationTimer;
        uint32 willOfEmperorTimer;

        //GameObject
        uint64 stoneexitdoorGuid;
        uint64 stoneentrdoorGuid;
        uint64 fengexitdoorGuid;
        uint64 garajalexitdoorGuid;

        //Creature
        uint64 stoneGuardControlerGuid;
        uint64 fengGuid;
        uint64 inversionGobGuid;
        uint64 cancelGobGuid;
        uint64 spiritKingsControlerGuid;
        uint64 qiangGuid;
        uint64 subetaiGuid;
        uint64 zianGuid;
        uint64 mengGuid;
        uint64 janxiGuid;
        uint64 qinxiGuid;

        std::vector<uint64> stoneGuardGUIDs;
        std::vector<uint64> fengdoorGUIDs;
        std::vector<uint64> garajaldoorGUIDs;
        std::vector<uint64> fengStatuesGUIDs;
        //std::vector<uint64> spiritKingsGUIDs;
        std::vector<uint64> kingsdoorGUIDs;

        
        void Initialize()
        {
            SetBossNumber(DATA_MAX_BOSS_DATA);
            LoadDoorData(doorData);
            randomDespawnStoneGuardian      = urand(1,4);
            willOfEmperorPhase              = 0;
            actualPetrifierEntry            = 0;
            StoneGuardPetrificationTimer    = 10000;
            willOfEmperorTimer  = 0;

            //GameObject
            stoneexitdoorGuid = 0;
            stoneentrdoorGuid = 0;
            fengexitdoorGuid = 0;
            garajalexitdoorGuid = 0;

            //Creature
            stoneGuardControlerGuid         = 0;
            fengGuid                        = 0;
            inversionGobGuid                = 0;
            cancelGobGuid                   = 0;
            spiritKingsControlerGuid        = 0;
            qiangGuid                       = 0;
            subetaiGuid                     = 0;
            zianGuid                        = 0;
            mengGuid                        = 0;

            stoneGuardGUIDs.clear();
            fengStatuesGUIDs.clear();
            garajaldoorGUIDs.clear();
            fengdoorGUIDs.clear();
            //spiritKingsGUIDs.clear();
            kingsdoorGUIDs.clear();
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_STONE_GUARD_CONTROLER:
                    stoneGuardControlerGuid = creature->GetGUID();
                    break;
                case NPC_JASPER:
                case NPC_JADE:
                case NPC_AMETHYST:
                case NPC_COBALT:
                    stoneGuardGUIDs.push_back(creature->GetGUID());

                    if (GetBossState(DATA_STONE_GUARD) != DONE)
                    {
                        creature->Respawn(true);

                        if (!creature->GetMap()->Is25ManRaid())
                        {
                            if (--randomDespawnStoneGuardian == 0)
                            {
                                creature->DespawnOrUnsummon();
                                randomDespawnStoneGuardian = -1;
                            }
                        }
                    }
                    break;
                case NPC_FENG:
                    fengGuid = creature->GetGUID();
                    break;
                case NPC_SPIRIT_GUID_CONTROLER:
                    spiritKingsControlerGuid = creature->GetGUID();
                    break;
                case NPC_QIANG:
                    qiangGuid = creature->GetGUID();
                    break;
                case NPC_ZIAN:
                    zianGuid = creature->GetGUID();
                    break;
                case NPC_MENG:
                    mengGuid = creature->GetGUID();
                    break;
                case NPC_SUBETAI:
                    subetaiGuid = creature->GetGUID();
                    break;
                case NPC_QIN_XI:
                    qinxiGuid = creature->GetGUID();
                    break;
                case NPC_JAN_XI:
                    janxiGuid = creature->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GOB_STONE_GUARD_DOOR_ENTRANCE:
                    stoneentrdoorGuid = go->GetGUID();
                    break;
                case GOB_STONE_GUARD_DOOR_EXIT:
                    stoneexitdoorGuid = go->GetGUID();
                    break;
                case GOB_FENG_DOOR_FENCE:
                    fengdoorGUIDs.push_back(go->GetGUID());
                    break;
                case GOB_FENG_DOOR_EXIT:
                    fengexitdoorGuid = go->GetGUID();
                    break;
                case GOB_GARAJAL_FENCE:
                    garajaldoorGUIDs.push_back(go->GetGUID());
                    break;
                case GOB_GARAJAL_EXIT:
                    garajalexitdoorGuid = go->GetGUID();
                    break;
                case GOB_SPIRIT_KINGS_WIND_WALL:
                    kingsdoorGUIDs.push_back(go->GetGUID());
                    break;
                case GOB_SPEAR_STATUE:
                case GOB_FIST_STATUE:
                case GOB_SHIELD_STATUE:
                case GOB_STAFF_STATUE:
                    fengStatuesGUIDs.push_back(go->GetGUID());
                    break;
                case GOB_INVERSION:
                    inversionGobGuid = go->GetGUID();
                    break;
                case GOB_CANCEL:
                    cancelGobGuid = go->GetGUID();
                    break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
                case DATA_STONE_GUARD:
                {
                    switch (state)
                    {
                        case FAIL:
                        {
                            for (std::vector<uint64>::const_iterator guid = stoneGuardGUIDs.begin(); guid != stoneGuardGUIDs.end(); ++guid)
                                if (Creature* stoneGuard = instance->GetCreature(*guid))
                                    stoneGuard->AI()->DoAction(ACTION_FAIL);

                            HandleGameObject(stoneentrdoorGuid, true);
                        }
                        break;
                        case IN_PROGRESS:
                            HandleGameObject(stoneentrdoorGuid, false);
                            break;
                        case DONE:
                            HandleGameObject(stoneexitdoorGuid, true);
                            HandleGameObject(stoneentrdoorGuid, true);
                            break;
                    }
                    break;
                }
                case DATA_FENG:
                    {
                        switch (state)
                        {
                        case NOT_STARTED:
                            for (std::vector<uint64>::const_iterator guid = fengdoorGUIDs.begin(); guid != fengdoorGUIDs.end(); guid++)
                                HandleGameObject(*guid, true);
                            break;
                        case DONE:
                            for (std::vector<uint64>::const_iterator guid = fengdoorGUIDs.begin(); guid != fengdoorGUIDs.end(); guid++)
                                HandleGameObject(*guid, true);
                            HandleGameObject(fengexitdoorGuid, true);
                            break;
                        case IN_PROGRESS:
                            for (std::vector<uint64>::const_iterator guid = fengdoorGUIDs.begin(); guid != fengdoorGUIDs.end(); guid++)
                                HandleGameObject(*guid, false);
                            break;
                        }
                        break;
                    }
                case DATA_GARAJAL:
                    {
                        switch (state)
                        {
                        case NOT_STARTED:
                            for (std::vector<uint64>::const_iterator guid = garajaldoorGUIDs.begin(); guid != garajaldoorGUIDs.end(); guid++)
                                HandleGameObject(*guid, true);
                            break;
                        case IN_PROGRESS:
                            for (std::vector<uint64>::const_iterator guid = garajaldoorGUIDs.begin(); guid != garajaldoorGUIDs.end(); guid++)
                                HandleGameObject(*guid, false);
                            break;
                        case DONE:
                            for (std::vector<uint64>::const_iterator guid = garajaldoorGUIDs.begin(); guid != garajaldoorGUIDs.end(); guid++)
                                HandleGameObject(*guid, true);
                            HandleGameObject(garajalexitdoorGuid, true);
                            break;
                        }
                        break;
                    }
                case DATA_SPIRIT_KINGS:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector<uint64>::const_iterator guid = kingsdoorGUIDs.begin(); guid != kingsdoorGUIDs.end(); guid++)
                            HandleGameObject(*guid, true);
                        break;
                    case IN_PROGRESS:
                        for (std::vector<uint64>::const_iterator guid = kingsdoorGUIDs.begin(); guid != kingsdoorGUIDs.end(); guid++)
                            HandleGameObject(*guid, false);
                        break;
                    case DONE:
                        for (std::vector<uint64>::const_iterator guid = kingsdoorGUIDs.begin(); guid != kingsdoorGUIDs.end(); guid++)
                            HandleGameObject(*guid, true);
                        //TODO: here must be door for next boss
                        break;
                    }
                    break;
                }
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
                case NPC_STONE_GUARD_CONTROLER:
                    return stoneGuardControlerGuid;
                case NPC_JASPER:
                case NPC_JADE:
                case NPC_AMETHYST:
                case NPC_COBALT:
                {
                    for (std::vector<uint64>::const_iterator guid = stoneGuardGUIDs.begin(); guid != stoneGuardGUIDs.end(); ++guid)
                        if (Creature* stoneGuard = instance->GetCreature(*guid))
                            if (stoneGuard->GetEntry() == type)
                                return *guid;
                    break;
                }
                case NPC_FENG:
                    return fengGuid;
                case NPC_SPIRIT_GUID_CONTROLER:
                    return spiritKingsControlerGuid;
                case NPC_QIANG:
                    return qiangGuid;
                case NPC_ZIAN:
                    return zianGuid;
                case NPC_MENG:
                    return mengGuid;
                case NPC_SUBETAI:
                    return subetaiGuid;
                case NPC_QIN_XI:
                    return qinxiGuid;
                case NPC_JAN_XI:
                    return janxiGuid;
                case GOB_SPEAR_STATUE:
                case GOB_FIST_STATUE:
                case GOB_SHIELD_STATUE:
                case GOB_STAFF_STATUE:
                {
                    for (std::vector<uint64>::const_iterator guid = fengStatuesGUIDs.begin(); guid != fengStatuesGUIDs.end(); ++guid)
                        if (GameObject* fengStatue = instance->GetGameObject(*guid))
                            if (fengStatue->GetEntry() == type)
                                return *guid;
                    break;
                }
                case GOB_INVERSION: 
                    return inversionGobGuid;
                case GOB_CANCEL: 
                    return cancelGobGuid;
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
            for (uint32 i=0; i < DATA_MAX_BOSS_DATA; ++i)
                loadStream >> buff;
        }

        void Update(uint32 diff)
        {
            if (GetBossState(DATA_WILL_OF_EMPEROR) != IN_PROGRESS)
                return;

            if (willOfEmperorTimer)
            {
                if (willOfEmperorTimer <= diff)
                {
                    switch (willOfEmperorPhase)
                    {
                        case PHASE_WOE_RAGE:
                            break;
                        case PHASE_WOE_COURAGE:
                            break;
                        case PHASE_WOE_STRENGHT:
                            break;
                        case PHASE_WOE_GAZ:
                            break;
                    }
                }
                else
                    willOfEmperorTimer -= diff;
            }
        }
    };

};

void AddSC_instance_mogu_shan_vault()
{
    new instance_mogu_shan_vault();
}
