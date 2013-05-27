/*
    Dungeon : Template of Mogu'shan Palace 87-89
    Instance General Script
    Jade servers
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
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

        uint32 actualPetrifierEntry;
        int8   randomDespawnStoneGuardian;

        uint32 StoneGuardPetrificationTimer;

        uint64 stoneGuardControlerGuid;
        uint64 fengGuid;
        uint64 inversionGobGuid;
        uint64 cancelGobGuid;
        std::vector<uint64> stoneGuardGUIDs;
        std::vector<uint64> fengStatuesGUIDs;

        void Initialize()
        {
            SetBossNumber(DATA_MAX_BOSS_DATA);
            LoadDoorData(doorData);

            actualPetrifierEntry = 0;
            StoneGuardPetrificationTimer = 10000;
            stoneGuardControlerGuid = 0;

            stoneGuardGUIDs.clear();
            fengStatuesGUIDs.clear();

            randomDespawnStoneGuardian = urand(1,4);
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
                    creature->Respawn(true);

                    if (--randomDespawnStoneGuardian == 0)
                    {
                        creature->DespawnOrUnsummon();
                        randomDespawnStoneGuardian = -1;
                    }
                    break;
                case NPC_FENG:
                    fengGuid = creature->GetGUID();
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
                case GOB_STONE_GUARD_DOOR_EXIT:
                case GOB_FENG_DOOR_FENCE:
                case GOB_FENG_DOOR_EXIT:
                case GOB_GARAJAL_FENCE:
                case GOB_GARAJAL_EXIT:
                case GOB_SPIRIT_KINGS_WIND_WALL:
                case GOB_SPIRIT_KINGS_EXIT:
                    AddDoor(go, true);
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
                        case IN_PROGRESS:
                        {
                            if (Creature* stoneGuardControler = instance->GetCreature(stoneGuardControlerGuid))
                                stoneGuardControler->AI()->DoAction(ACTION_ENTER_COMBAT);

                            for (auto stoneGuardGuid: stoneGuardGUIDs)
                                if (Creature* stoneGuard = instance->GetCreature(stoneGuardGuid))
                                    stoneGuard->AI()->DoAction(ACTION_ENTER_COMBAT);
                            break;
                        }
                        case FAIL:
                        {
                            for (auto stoneGuardGuid: stoneGuardGUIDs)
                                if (Creature* stoneGuard = instance->GetCreature(stoneGuardGuid))
                                    stoneGuard->AI()->DoAction(ACTION_FAIL);
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
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
                // Creature
                case NPC_STONE_GUARD_CONTROLER:
                    return stoneGuardControlerGuid;
                case NPC_JASPER:
                case NPC_JADE:
                case NPC_AMETHYST:
                case NPC_COBALT:
                {
                    for (auto guid: stoneGuardGUIDs)
                        if (Creature* stoneGuard = instance->GetCreature(guid))
                            if (stoneGuard->GetEntry() == type)
                                return guid;
                    break;
                }
                case NPC_FENG:
                    return fengGuid;
                // Gameobject
                case GOB_SPEAR_STATUE:
                case GOB_FIST_STATUE:
                case GOB_SHIELD_STATUE:
                case GOB_STAFF_STATUE:
                {
                    for (auto guid: fengStatuesGUIDs)
                        if (GameObject* fengStatue = instance->GetGameObject(guid))
                            if (fengStatue->GetEntry() == type)
                                return guid;
                    break;
                }
                case GOB_INVERSION: return inversionGobGuid;
                case GOB_CANCEL:    return cancelGobGuid;
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
    };

};

void AddSC_instance_mogu_shan_vault()
{
    new instance_mogu_shan_vault();
}
