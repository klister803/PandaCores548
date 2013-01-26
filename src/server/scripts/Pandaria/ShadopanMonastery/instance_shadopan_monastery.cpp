/*
    Dungeon : Shandopan Monastery 87-89
    Instance General Script
*/

#include "shadopan_monastery.h"
#include "InstanceScript.h"

DoorData const doorData[] =
{
    {GO_CLOUDSTRIKE_ENTRANCE,   DATA_GU_CLOUDSTRIKE,    DOOR_TYPE_ROOM,         BOUNDARY_SE  },
    {GO_CLOUDSTRIKE_EXIT,       DATA_GU_CLOUDSTRIKE,    DOOR_TYPE_PASSAGE,      BOUNDARY_S   },
    {GO_SNOWDRIFT_ENTRANCE,     NPC_MASTER_SNOWDRIFT,   DOOR_TYPE_ROOM,         BOUNDARY_SE  },
    {GO_SNOWDRIFT_FIRE_WALL,    NPC_MASTER_SNOWDRIFT,   DOOR_TYPE_ROOM,         BOUNDARY_SE  },
    {GO_SNOWDRIFT_DOJO_DOOR,    NPC_MASTER_SNOWDRIFT,   DOOR_TYPE_ROOM,         BOUNDARY_SE  },
    {GO_SNOWDRIFT_EXIT,         NPC_MASTER_SNOWDRIFT,   DOOR_TYPE_PASSAGE,      BOUNDARY_NW  },
    {GO_SHA_ENTRANCE,           NPC_SHA_VIOLENCE,       DOOR_TYPE_ROOM,         BOUNDARY_SW  },
    {GO_SHA_EXIT,               NPC_SHA_VIOLENCE,       DOOR_TYPE_PASSAGE,      BOUNDARY_S   },
    {0,                         0,                      DOOR_TYPE_ROOM,         BOUNDARY_NONE},// END
};

class instance_shadopan_monastery : public InstanceMapScript
{
public:
    instance_shadopan_monastery() : InstanceMapScript("instance_shadopan_monastery", 959) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_shadopan_monastery_InstanceMapScript(map);
    }

    struct instance_shadopan_monastery_InstanceMapScript : public InstanceScript
    {
        uint64 guCloudstikeGuid;
        uint64 masterSnowdriftGuid;
        uint64 shaViolenceGuid;
        uint64 taranZhuGuid;

        uint32 dataStorage[MAX_DATA];

        instance_shadopan_monastery_InstanceMapScript(Map* map) : InstanceScript(map)
        {}

        void Initialize()
        {
            SetBossNumber(EncounterCount);
            LoadDoorData(doorData);

            guCloudstikeGuid        = 0;
            masterSnowdriftGuid     = 0;
            shaViolenceGuid         = 0;
            taranZhuGuid            = 0;

            memset(dataStorage, 0, MAX_DATA * sizeof(uint32));
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_GU_CLOUDSTRIKE:    guCloudstikeGuid    = creature->GetGUID();  return;
                case NPC_MASTER_SNOWDRIFT:  masterSnowdriftGuid = creature->GetGUID();  return;
                case NPC_SHA_VIOLENCE:      shaViolenceGuid     = creature->GetGUID();  return;
                case NPC_TARAN_ZHU:         taranZhuGuid        = creature->GetGUID();  return;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_CLOUDSTRIKE_ENTRANCE:
                case GO_CLOUDSTRIKE_EXIT:
                case GO_SNOWDRIFT_ENTRANCE:
                case GO_SNOWDRIFT_FIRE_WALL:
                case GO_SNOWDRIFT_DOJO_DOOR:
                case GO_SNOWDRIFT_EXIT:
                case GO_SHA_ENTRANCE:
                case GO_SHA_EXIT:
                    AddDoor(go, true);
                    return;
                default:
                    return;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                default:
                    if (type < MAX_DATA)
                        dataStorage[type] = data;
                    break;
            }
        }

        uint32 GetData(uint32 type)
        {
            switch (type)
            {
                default:
                    if (type < MAX_DATA)
                        return dataStorage[type];
                    break;
            }

            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case NPC_GU_CLOUDSTRIKE:    return guCloudstikeGuid;
                case NPC_MASTER_SNOWDRIFT:  return masterSnowdriftGuid;
                case NPC_SHA_VIOLENCE:      return shaViolenceGuid;
                case NPC_TARAN_ZHU:         return taranZhuGuid;
            }

            return 0;
        }
    };

};

void AddSC_instance_shadopan_monastery()
{
    new instance_shadopan_monastery();
}
