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
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_E   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_N   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_S   },
    {GO_KIPTILAK_WALLS,                     DATA_KIPTILAK,              DOOR_TYPE_ROOM,         BOUNDARY_W   },
    {GO_KIPTILAK_EXIT_DOOR,                 DATA_KIPTILAK,              DOOR_TYPE_PASSAGE,      BOUNDARY_N   },
    {GO_RIMAK_AFTER_DOOR,                   DATA_RIMOK,                 DOOR_TYPE_ROOM,         BOUNDARY_S   },
    {GO_RAIGONN_DOOR,                       DATA_RAIGONN,               DOOR_TYPE_ROOM,         BOUNDARY_NE  },
    {GO_RAIGONN_AFTER_DOOR,                 DATA_RAIGONN,               DOOR_TYPE_PASSAGE,      BOUNDARY_E   },
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
        uint64 raigonWeakGuid;

        uint64 firstDoorGuid;
        uint64 fireSignalGuid;

        uint32 cinematicTimer;
        uint8 cinematicEventProgress;

        std::vector<uint64> bombarderGuids;
        std::vector<uint64> bombStalkerGuids;
        std::vector<uint64> mantidBombsGUIDs;
        std::vector<uint64> rimokAddGenetarorsGUIDs;
        std::vector<uint64> artilleryGUIDs;

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
            raigonWeakGuid  = 0;
            
            firstDoorGuid   = 0;

            cinematicTimer = 0;
            cinematicEventProgress = 0;

            memset(dataStorage, 0, MAX_DATA * sizeof(uint32));

            bombarderGuids.clear();
            bombStalkerGuids.clear();
            mantidBombsGUIDs.clear();
            rimokAddGenetarorsGUIDs.clear();
            artilleryGUIDs.clear();
        }

        void OnPlayerEnter(Player* player)
        {
            if (GetData(DATA_BRASIER_CLICKED) == NOT_STARTED)
                player->SetPhaseMask(1, true);
            else
                player->SetPhaseMask(2, true);
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_KIPTILAK:          kiptilakGuid    = creature->GetGUID();                  return;
                case NPC_GADOK:             gadokGuid       = creature->GetGUID();                  return;
                case NPC_RIMOK:             rimokGuid       = creature->GetGUID();                  return;
                case NPC_RAIGONN:           raigonnGuid     = creature->GetGUID();                  return;
                case NPC_KRITHUK_BOMBARDER: bombarderGuids.push_back(creature->GetGUID());          return;
                case NPC_BOMB_STALKER:      bombStalkerGuids.push_back(creature->GetGUID());        return;
                case NPC_ADD_GENERATOR:     rimokAddGenetarorsGUIDs.push_back(creature->GetGUID()); return;
                case NPC_ARTILLERY:         artilleryGUIDs.push_back(creature->GetGUID());          return;
                default:                                                                            return;
            }
        }

        virtual void OnCreatureRemove(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_KRITHUK_BOMBARDER:
                    for (std::vector<uint64>::iterator it = bombarderGuids.begin(); it != bombarderGuids.end(); ++it)
                        if (*it == creature->GetGUID())
                            bombarderGuids.erase(it);
                    break;
                default:                                                                            return;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_KIPTILAK_ENTRANCE_DOOR:
                    firstDoorGuid = go->GetGUID();
                    break;
                case GO_SIGNAL_FIRE:
                    fireSignalGuid = go->GetGUID();
                    break;
                case GO_KIPTILAK_WALLS:
                case GO_KIPTILAK_EXIT_DOOR:
                case GO_RIMAK_AFTER_DOOR:
                case GO_RAIGONN_AFTER_DOOR:
                    AddDoor(go, true);
                    return;
                case GO_KIPTILAK_MANTID_BOMBS:
                    mantidBombsGUIDs.push_back(go->GetGUID());
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
                        for (auto itr: mantidBombsGUIDs)
                            if (GameObject* bomb = instance->GetGameObject(itr))
                                bomb->SetPhaseMask(32768, true); // Set Invisible
                    break;
                }
                case DATA_RIMOK:
                {
                    uint8 generatorsCount = 0;

                    for (auto itr: rimokAddGenetarorsGUIDs)
                    {
                        if (Creature* generator = instance->GetCreature(itr))
                        {
                            if (generator->AI())
                            {
                                // There is 7 add generators, the middle one spawn saboteur
                                if (state == IN_PROGRESS && (++generatorsCount == 4))
                                    generator->AI()->DoAction(SPECIAL);
                                else
                                    generator->AI()->DoAction(state);
                            }
                        }
                    }
                    break;
                }
                case DATA_RAIGONN:
                {
                    if (state == IN_PROGRESS)
                    {
                        for (auto itr: artilleryGUIDs)
                            if (Creature* artillery = instance->GetCreature(itr))
                                artillery->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    }
                    else
                    {
                        for (auto itr: artilleryGUIDs)
                            if (Creature* artillery = instance->GetCreature(itr))
                                artillery->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
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
            switch (type)
            {
                case DATA_OPEN_FIRST_DOOR:
                {
                    if (dataStorage[type] == DONE)
                        return;

                    HandleGameObject(firstDoorGuid, true);

                    dataStorage[type] = data;
                    
                    if (GameObject* firstDoor = instance->GetGameObject(firstDoorGuid))
                    {

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
                case DATA_BRASIER_CLICKED:
                {
                    if (dataStorage[type] == DONE)
                        return;

                    Map::PlayerList const &PlayerList = instance->GetPlayers();
                    for (Map::PlayerList::const_iterator it = PlayerList.begin(); it != PlayerList.end(); ++it)
                    {
                        if (Player* player = it->getSource())
                        {
                            player->SendCinematicStart(CINEMATIC_SETTING_SUN);
                            player->SetPhaseMask(2, true);
                        }
                    }

                    cinematicTimer = 100;
                    dataStorage[type] = data;
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

        void SetData64(uint32 type, uint64 value)
        {
            switch (type)
            {
                case NPC_WEAK_SPOT:     raigonWeakGuid = value;     break;
                default:                                            break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case NPC_KIPTILAK:              return kiptilakGuid;
                case NPC_GADOK:                 return gadokGuid;
                case NPC_RIMOK:                 return rimokGuid;
                case NPC_RAIGONN:               return raigonnGuid;
                case NPC_WEAK_SPOT:             return raigonWeakGuid;
                case DATA_RANDOM_BOMBARDER:     return Trinity::Containers::SelectRandomContainerElement(bombarderGuids);
                case DATA_RANDOM_BOMB_STALKER:  return Trinity::Containers::SelectRandomContainerElement(bombStalkerGuids);
            }

            return 0;
        }

        void doEventCinematic()
        {
            switch(cinematicEventProgress)
            {
                case 0:
                    // On allume le brasier & la meche
                    cinematicTimer = 6000;
                    break;
                case 1:
                    if (GameObject* go = instance->GetGameObject(fireSignalGuid))
                        go->UseDoorOrButton();
                    cinematicTimer = 0;
                    break;
            }

            ++cinematicEventProgress;
        }

        void Update(uint32 diff)
        {
            if (cinematicTimer)
            {
                if (cinematicTimer <= diff)
                    doEventCinematic();
                else
                    cinematicTimer -= diff;
            }
        }
    };

};

void AddSC_instance_gate_setting_sun()
{
    new instance_gate_setting_sun();
}
