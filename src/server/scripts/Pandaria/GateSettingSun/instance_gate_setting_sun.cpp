/*
    Dungeon : Gate of the Setting Sun 90-90
    Instance General Script
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "gate_setting_sun.h"

DoorData const doorData[] =
{
    {GO_KIPTILAK_ENTRANCE_DOOR,             DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_E   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_E   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_N   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_S   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_W   },
    {GO_KIPTILAK_EXIT_DOOR,                 DATA_KIPTILAK,              DOOR_TYPE_PASSAGE,      BOUNDARY_N   },
    {0,                                     0,                          DOOR_TYPE_ROOM,         BOUNDARY_NONE},// END
};

class instance_gate_setting_sun : public InstanceMapScript
{
public:
    instance_gate_setting_sun() : InstanceMapScript("instance_gate_setting_sun", 962) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_gate_setting_sun_InstanceMapScript(map);
    }

    struct instance_gate_setting_sun_InstanceMapScript : public InstanceScript
    {
        uint64 kiptilakGuid;
        uint64 gadokGuid;
        uint64 rimokGuid;
        uint64 raigonnGuid;

        uint64 firstDoorGuid;
        std::vector<uint64> mantidBombsGUID;

        uint32 dataStorage[MAX_DATA];

        instance_gate_setting_sun_InstanceMapScript(Map* map) : InstanceScript(map)
        {}

        void Initialize()
        {
            SetBossNumber(EncounterCount);
            LoadDoorData(doorData);

            kiptilakGuid    = 0;
            gadokGuid       = 0;
            rimokGuid       = 0;
            raigonnGuid     = 0;
            
            firstDoorGuid   = 0;

            memset(dataStorage, 0, MAX_DATA * sizeof(uint32));
            mantidBombsGUID.clear();
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_KIPTILAK:  kiptilakGuid    = creature->GetGUID();  return;
                case NPC_GADOK:     gadokGuid       = creature->GetGUID();  return;
                case NPC_RIMOK:     rimokGuid       = creature->GetGUID();  return;
                case NPC_RAIGONN:   raigonnGuid     = creature->GetGUID();  return;

                // Spawn in center of the Ga'Dok tower, we must make them move around the tower
                case NPC_KRIKTHIK_STRIKER:
                case NPC_KRIKTHIK_DISRUPTOR:
                default:                                                    return;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_KIPTILAK_ENTRANCE_DOOR:
                    firstDoorGuid = go->GetGUID();
                    break;
                case GO_KIPTILAK_WALLS:
                case GO_KIPTILAK_EXIT_DOOR:
                    AddDoor(go, true);
                    return;
                case GO_KIPTILAK_MANTID_BOMBS:
                    mantidBombsGUID.push_back(go->GetGUID());
                    return;
                default:
                    return;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
                case DATA_KIPTILAK:
                {
                    if (state == DONE)
                        for (auto itr: mantidBombsGUID)
                            if (GameObject* bomb = instance->GetGameObject(itr))
                                bomb->SetPhaseMask(32768, true); // Set Invisible

                    break;
                }
                default:
                    break;
            }

            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_OPEN_FIRST_DOOR:
                {
                    HandleGameObject(firstDoorGuid, true);
                    
                    if (GameObject* firstDoor = instance->GetGameObject(firstDoorGuid))
                    {
                        dataStorage[type] = data;

                        if (Creature* trigger = firstDoor->SummonTrigger(firstDoor->GetPositionX(), firstDoor->GetPositionY(), firstDoor->GetPositionZ(), 0, 500))
                        {
                            std::list<Creature*> defensorList;
                            GetCreatureListWithEntryInGrid(defensorList, trigger, 65337, 20.0f);

                            trigger->CastSpell(trigger, 115456); // Explosion

                            for (auto itr: defensorList)
                            {
                                uint8 random = rand() % 2;

                                float posX = random ? 814.0f:  640.0f;
                                float posY = random ? 2102.0f: 2112.0f;
                                itr->KnockbackFrom(posX, posY, 25.0f, 20.0f);
                                itr->DespawnOrUnsummon(1000);
                            }
                        }
                    }
                    break;
                }
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
                case DATA_OPEN_FIRST_DOOR:
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
                case NPC_KIPTILAK:  return kiptilakGuid;
                case NPC_GADOK:     return gadokGuid;
                case NPC_RIMOK:     return rimokGuid;
                case NPC_RAIGONN:   return raigonnGuid;
            }

            return 0;
        }
    };

};

void AddSC_instance_gate_setting_sun()
{
    new instance_gate_setting_sun();
}
