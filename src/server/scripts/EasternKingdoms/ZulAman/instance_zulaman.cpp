#include "ScriptPCH.h"
#include "zulaman.h"

#define MAX_ENCOUNTER     6

struct SHostageInfo
{
    uint32 npc, go; // FIXME go Not used
    float x, y, z, o;
};

static SHostageInfo HostageInfo[] =
{
    {23790, 186648, -57, 1343, 40.77f, 3.2f}, // bear
    {23999, 187021, 400, 1414, 74.36f, 3.3f}, // eagle
    {24001, 186672, -35, 1134, 18.71f, 1.9f}, // dragonhawk
    {24024, 186667, 413, 1117,  6.32f, 3.1f}  // lynx
};

static const DoorData doordata[] = 
{
    {GO_AKILZON_EXIT,           DATA_AKILZON,            DOOR_TYPE_ROOM,     BOUNDARY_NONE},
    {GO_HALAZZI_ENTRANCE,       DATA_HALAZZI,            DOOR_TYPE_ROOM,     BOUNDARY_NONE},
    {GO_HALAZZI_EXIT,           DATA_HALAZZI,            DOOR_TYPE_PASSAGE,  BOUNDARY_NONE},
    {GO_MALACRASS_EXIT,         DATA_HEX_LORD_MALACRASS, DOOR_TYPE_PASSAGE,  BOUNDARY_NONE},
    {GO_DAAKARA_EXIT,           DATA_DAAKARA,            DOOR_TYPE_ROOM,     BOUNDARY_NONE},
    {0,                         0,                       DOOR_TYPE_ROOM,     BOUNDARY_NONE},
};

class instance_zulaman : public InstanceMapScript
{
    public:
        instance_zulaman() : InstanceMapScript("instance_zulaman", 568){}
        
        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_zulaman_InstanceMapScript(map);
        }

        struct instance_zulaman_InstanceMapScript : public InstanceScript
        {
            instance_zulaman_InstanceMapScript(Map* map) : InstanceScript(map) 
            {
                SetBossNumber(MAX_ENCOUNTER);
                LoadDoorData(doordata);

                HarkorsSatchelGUID = 0;
                TanzarsTrunkGUID = 0;
                AshlisBagGUID = 0;
                KrazsPackageGUID = 0;

                HexLordGateGUID = 0;
                MainGateGUID    = 0;
                StrangeGongGUID = 0;

                QuestTimer = 0;
                QuestMinute = 21;
                uiMainGate = 0;
                uiVendor1 = 0;
                uiVendor2 = 0;
            }

            uint64 HarkorsSatchelGUID;
            uint64 TanzarsTrunkGUID;
            uint64 AshlisBagGUID;
            uint64 KrazsPackageGUID;

            uint64 HexLordGateGUID;
            uint64 MainGateGUID;
            uint64 StrangeGongGUID;
            uint64 AmanishiTempestGUID;

            uint32 uiMainGate;
            uint32 uiVendor1;
            uint32 uiVendor2;
            uint32 QuestTimer;
            uint16 QuestMinute;
            
            void OnCreatureCreate(Creature* pCreature)
            {
                switch (pCreature->GetEntry())
                {
                    case NPC_AMANISHI_TEMPEST:
                        AmanishiTempestGUID = pCreature->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* pGo)
            {
                switch (pGo->GetEntry())
                {
                    case GO_STRANGE_GONG:
                        StrangeGongGUID = pGo->GetGUID();
                        if (uiMainGate == 1)
                            pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                        break;
                    case GO_MAIN_GATE:
                        MainGateGUID = pGo->GetGUID();
                        if (uiMainGate == 1)
                            HandleGameObject(MainGateGUID, true);
                        break;
                    case GO_AKILZON_EXIT:
                        AddDoor(pGo, true);
                        break;
                    case GO_HALAZZI_ENTRANCE:
                        AddDoor(pGo, true);
                        break;
                    case GO_HALAZZI_EXIT:
                        AddDoor(pGo, true);
                        break;
                    case GO_MALACRASS_ENTRANCE:
                        HexLordGateGUID = pGo->GetGUID(); 
                        break;
                    case GO_MALACRASS_EXIT:
                        AddDoor(pGo, true);
                        break;
                    case GO_DAAKARA_EXIT:
                        AddDoor(pGo, true);
                        break;
                    case 187021: 
                        HarkorsSatchelGUID  = pGo->GetGUID();
                        break;
                    case 186648: 
                        TanzarsTrunkGUID = pGo->GetGUID();
                        break;
                    case 186672: 
                        AshlisBagGUID = pGo->GetGUID();
                        break;
                    case 186667: 
                        KrazsPackageGUID  = pGo->GetGUID();
                        break;
                }
            }

            std::string GetSaveData()
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream ss;
                ss << "ZA " << GetBossSaveData() << uiMainGate << " " << QuestMinute << " " << uiVendor1 << " " << uiVendor2 << " ";

                OUT_SAVE_INST_DATA_COMPLETE;
                return ss.str();
            }

            void Load(const char* in)
            {
                if (!in)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(in);

                char dataHead1, dataHead2;

                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2;

                if (dataHead1 == 'Z' && dataHead2 == 'A')
                {
                    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
				    {
					    uint32 tmpState;
					    loadStream >> tmpState;
					    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
						    tmpState = NOT_STARTED;
					    SetBossState(i, EncounterState(tmpState));
				    }
                    loadStream >> uiMainGate;
                    SetData(DATA_MAIN_GATE, uiMainGate);
                    loadStream >> QuestMinute;
                    DoUpdateWorldState(3104, QuestMinute);
                    loadStream >> uiVendor1;
                    loadStream >> uiVendor2;

                } else OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }
            
            bool SetBossState(uint32 id, EncounterState state)
            {
                if (!InstanceScript::SetBossState(id, state))
                    return false;

                switch (id)
                {
                    case DATA_AKILZON:
                        if (state == DONE)
                        {
                            if (GetBossState(DATA_AKILZON) == DONE && GetBossState(DATA_NALORAKK) == DONE && GetBossState(DATA_JANALAI) == DONE && GetBossState(DATA_HALAZZI) == DONE)
                            {
                                HandleGameObject(HexLordGateGUID, true);
                                if (QuestMinute)
                                {
                                    QuestMinute = 0;
                                    DoUpdateWorldState(3104, 0);
                                }
                            }
                            else if (QuestMinute)
                            {
                                QuestMinute += 15;
                                DoUpdateWorldState(3106, QuestMinute);
                            }
                        }
                        break;
                    case DATA_NALORAKK:
                        if (state == DONE)
                        {
                            if (GetBossState(DATA_AKILZON) == DONE && GetBossState(DATA_NALORAKK) == DONE && GetBossState(DATA_JANALAI) == DONE && GetBossState(DATA_HALAZZI) == DONE)
                            {
                                HandleGameObject(HexLordGateGUID, true);
                                if (QuestMinute)
                                {
                                    QuestMinute = 0;
                                    DoUpdateWorldState(3104, 0);
                                }
                            }    
                            else if (QuestMinute)
                            {
                                QuestMinute += 10;
                                DoUpdateWorldState(3106, QuestMinute);
                            }
                        }
                        break;
                    case DATA_JANALAI:
                        if (state == DONE)
                        {
                            if (GetBossState(DATA_AKILZON) == DONE && GetBossState(DATA_NALORAKK) == DONE && GetBossState(DATA_JANALAI) == DONE && GetBossState(DATA_HALAZZI) == DONE)
                            {
                                HandleGameObject(HexLordGateGUID, true);
                                if (QuestMinute)
                                {
                                    QuestMinute = 0;
                                    DoUpdateWorldState(3104, 0);
                                }
                            }
                        }
                        break;
                    case DATA_HALAZZI:
                        if (state == DONE)
                        {
                            if (GetBossState(DATA_AKILZON) == DONE && GetBossState(DATA_NALORAKK) == DONE && GetBossState(DATA_JANALAI) == DONE && GetBossState(DATA_HALAZZI) == DONE)
                            {
                                HandleGameObject(HexLordGateGUID, true);
                                if (QuestMinute)
                                {
                                    QuestMinute = 0;
                                    DoUpdateWorldState(3104, 0);
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
                return true;
            }

            void SetData(uint32 type, uint32 data)
            {
                switch (type)
                {
                    case DATA_MAIN_GATE:
                        uiMainGate = data;
                        if (data == 1)
                        {
                            HandleGameObject(MainGateGUID, true);
                            if (GameObject* pGo = instance->GetGameObject(StrangeGongGUID))
                                pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                            SaveToDB();
                        }
                        break;
                    case DATA_VENDOR_1:
                        uiVendor1 = data;
                        SaveToDB();
                        break;
                    case DATA_VENDOR_2:
                        uiVendor2 = data;
                        SaveToDB();
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type)
            {
                switch (type)
                {
                    case DATA_MAIN_GATE:
                        return MainGateGUID;
                    case DATA_TEMPEST:
                        return AmanishiTempestGUID;
                    case DATA_VENDOR_1:
                        return uiVendor1;
                    case DATA_VENDOR_2:
                        return uiVendor2;
                    default: 
                        return 0;
                }
            }

            void Update(uint32 diff)
            {
                if (QuestMinute)
                {
                    if (QuestTimer <= diff)
                    {
                        QuestMinute--;
                        SaveToDB();
                        QuestTimer += 60000;
                        if (QuestMinute)
                        {
                            DoUpdateWorldState(3104, 1);
                            DoUpdateWorldState(3106, QuestMinute);
                        } else DoUpdateWorldState(3104, 0);
                    }
                    QuestTimer -= diff;
                }
            }
        };
};

void AddSC_instance_zulaman()
{
    new instance_zulaman();
}

