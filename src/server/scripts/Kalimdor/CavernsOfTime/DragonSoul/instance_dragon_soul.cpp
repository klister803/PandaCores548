#include "ScriptPCH.h"
#include "dragon_soul.h"

#define MAX_ENCOUNTER 8

class instance_dragon_soul : public InstanceMapScript
{
public:
    instance_dragon_soul() : InstanceMapScript("instance_dragon_soul", 967) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_dragon_soul_InstanceMapScript(map);
    }

    struct instance_dragon_soul_InstanceMapScript : public InstanceScript
    {
        instance_dragon_soul_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetBossNumber(MAX_ENCOUNTER);

            uiMorchokGUID           = 0;
            uiKohcromGUID           = 0;
            uiZonozzGUID            = 0;
            uiValeeraGUID           = 0;
            uiEiendormiGUID         = 0;
            uiHagaraGUID            = 0;
            uiUltraxionGUID         = 0;
            uiBlackhornGUID         = 0;
            uiAllianceShipGUID      = 0;
            uiSwayzeGUID            = 0;
            uiReevsGUID             = 0;
            uiDeckGUID              = 0;
            uiMaelstormGUID         = 0;
            uiSpineGuid             = 0;

            DeathwingGUID           = 0;
            Maelstrom_trall         = 0;
            Maelstrom_kalecgos      = 0;
            Maelstrom_ysera         = 0;
            Maelstrom_nozdormy      = 0;
            Maelstrom_alexstrasza   = 0;

            memset(uiLesserCacheofTheAspects, 0, sizeof(uiLesserCacheofTheAspects));
            memset(uiBackPlates, 0, sizeof(uiBackPlates));
            memset(uiGreaterCacheofTheAspects, 0, sizeof(uiGreaterCacheofTheAspects));

            bHagaraEvent = 0;
        }

        void OnPlayerEnter(Player* pPlayer)
        {
            if (!uiTeamInInstance)
                uiTeamInInstance = pPlayer->GetTeam();
        }

        void OnCreatureCreate(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
            case NPC_MORCHOK:
                uiMorchokGUID = pCreature->GetGUID();
                break;
            case NPC_KOHCROM:
                uiKohcromGUID = pCreature->GetGUID();
                break;
            case NPC_VALEERA:
                uiValeeraGUID = pCreature->GetGUID();
                break;
            case NPC_EIENDORMI:
                uiEiendormiGUID = pCreature->GetGUID();
                break;
            case NPC_ZONOZZ:
                uiZonozzGUID = pCreature->GetGUID();
                break;
            case NPC_HAGARA:
                uiHagaraGUID = pCreature->GetGUID();
                break;
            case NPC_ULTRAXION:
                uiUltraxionGUID = pCreature->GetGUID();
                break;
            case NPC_SKY_CAPTAIN_SWAYZE:
                if (pCreature->GetPositionZ() > 200.0f)
                {
                    uiSwayzeGUID = pCreature->GetGUID();
                    if (GetBossState(DATA_ULTRAXION) == DONE)
                        pCreature->SetVisible(true);
                    else
                        pCreature->SetVisible(false);
                }
                break;
            case NPC_KAANU_REEVS:
                if (pCreature->GetPositionZ() > 200.0f)
                {
                    uiReevsGUID = pCreature->GetGUID();
                    if (GetBossState(DATA_ULTRAXION) == DONE)
                        pCreature->SetVisible(true);
                    else
                        pCreature->SetVisible(false);
                }
                break;
            case NPC_BLACKHORN:
                uiBlackhornGUID = pCreature->GetGUID();
                break;
            case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
            case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
            case NPC_TRAVEL_TO_WYRMREST_BASE:
            case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
                teleportGUIDs.push_back(pCreature->GetGUID());
                break;
            case NPC_TRAVEL_TO_DECK:
                if (GetBossState(DATA_BLACKHORN) == DONE)
                    pCreature->SetVisible(true);
                else
                    pCreature->SetVisible(false);
                uiDeckGUID = pCreature->GetGUID();
                teleportGUIDs.push_back(pCreature->GetGUID());
                break;
            case NPC_TRAVEL_TO_MAELSTORM:
                if (GetBossState(DATA_SPINE) == DONE)
                    pCreature->SetVisible(true);
                else
                    pCreature->SetVisible(false);
                uiMaelstormGUID = pCreature->GetGUID();
                teleportGUIDs.push_back(pCreature->GetGUID());
                break;
            case DATA_SPINE:
                uiSpineGuid = pCreature->GetGUID();
                break;
            case NPC_DEATHWING_1:
                DeathwingGUID = pCreature->GetGUID();
                break;
            case NPC_THRALL_2: 
                Maelstrom_trall = pCreature->GetGUID(); 
                break;
            case NPC_KALECGOS_DRAGON: 
                Maelstrom_kalecgos = pCreature->GetGUID(); 
                break;
            case NPC_YSERA_DRAGON: 
                Maelstrom_ysera = pCreature->GetGUID(); 
                break;
            case NPC_NOZDORMU_DRAGON: 
                Maelstrom_nozdormy = pCreature->GetGUID(); 
                break;
            case NPC_ALEXSTRASZA_DRAGON: 
                Maelstrom_alexstrasza = pCreature->GetGUID(); 
                break;
            default:
                break;
            }
        }

        void OnCreatureRemove(Creature* pCreature)
        {
            if (pCreature->GetEntry() == NPC_ULTRAXION)
                uiUltraxionGUID = 0;
            else if (pCreature->GetEntry() == NPC_BLACKHORN)
                uiBlackhornGUID = 0;
        }

        void OnGameObjectCreate(GameObject* pGo)
        {
            switch (pGo->GetEntry())
            {
            case GO_LESSER_CACHE_OF_THE_ASPECTS_10N:
                uiLesserCacheofTheAspects[0] = pGo->GetGUID();
                break;
            case GO_LESSER_CACHE_OF_THE_ASPECTS_25N:
                uiLesserCacheofTheAspects[1] = pGo->GetGUID();
                break;
            case GO_LESSER_CACHE_OF_THE_ASPECTS_10H:
                uiLesserCacheofTheAspects[2] = pGo->GetGUID();
                break;
            case GO_LESSER_CACHE_OF_THE_ASPECTS_25H:
                uiLesserCacheofTheAspects[3] = pGo->GetGUID();
                break;
            case GO_ALLIANCE_SHIP:
                uiAllianceShipGUID = pGo->GetGUID();
                if (GetBossState(DATA_ULTRAXION) == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                pGo->UpdateObjectVisibility();
                break;
            case GO_DEATHWING_BACK_PLATE_1:
                uiBackPlates[0] = pGo->GetGUID();
                break;
            case GO_DEATHWING_BACK_PLATE_2:
                uiBackPlates[1] = pGo->GetGUID();
                break;
            case GO_DEATHWING_BACK_PLATE_3:
                uiBackPlates[2] = pGo->GetGUID();
                break;
            case GO_GREATER_CACHE_OF_THE_ASPECTS_10N:
                uiGreaterCacheofTheAspects[0] = pGo->GetGUID();
                break;
            case GO_GREATER_CACHE_OF_THE_ASPECTS_25N:
                uiGreaterCacheofTheAspects[1] = pGo->GetGUID();
                break;
            case GO_GREATER_CACHE_OF_THE_ASPECTS_10H:
                uiGreaterCacheofTheAspects[2] = pGo->GetGUID();
                break;
            case GO_GREATER_CACHE_OF_THE_ASPECTS_25H:
                uiGreaterCacheofTheAspects[3] = pGo->GetGUID();
                break;
            default:
                break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
            case DATA_MORCHOK:              return uiMorchokGUID;
            case DATA_KOHCROM:              return uiKohcromGUID;
            case NPC_VALEERA:               return uiValeeraGUID;
            case NPC_EIENDORMI:             return uiEiendormiGUID;
            case DATA_ZONOZZ:               return uiZonozzGUID;
            case DATA_HAGARA:               return uiHagaraGUID;
            case DATA_ULTRAXION:            return uiUltraxionGUID;
            case DATA_BLACKHORN:            return uiBlackhornGUID;
            case DATA_LESSER_CACHE_10N:     return uiLesserCacheofTheAspects[0];
            case DATA_LESSER_CACHE_25N:     return uiLesserCacheofTheAspects[1];
            case DATA_LESSER_CACHE_10H:     return uiLesserCacheofTheAspects[2];
            case DATA_LESSER_CACHE_25H:     return uiLesserCacheofTheAspects[3];
            case DATA_SWAYZE:               return uiSwayzeGUID;
            case DATA_REEVS:                return uiReevsGUID;
            case DATA_BACK_PLATE_1:         return uiBackPlates[0];
            case DATA_BACK_PLATE_2:         return uiBackPlates[1];
            case DATA_BACK_PLATE_3:         return uiBackPlates[2];
            case DATA_GREATER_CACHE_10N:    return uiGreaterCacheofTheAspects[0];
            case DATA_GREATER_CACHE_25N:    return uiGreaterCacheofTheAspects[1];
            case DATA_GREATER_CACHE_10H:    return uiGreaterCacheofTheAspects[2];
            case DATA_GREATER_CACHE_25H:    return uiGreaterCacheofTheAspects[3];
            case DATA_ALLIANCE_SHIP:        return uiAllianceShipGUID;
            case NPC_THRALL_2:              return Maelstrom_trall;
            case DATA_SPINE:                return uiSpineGuid;
            case DATA_DAMAGE_DEATHWING:     return DeathwingGUID;
            default: return 0;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {   
            if (type == DATA_HAGARA_EVENT)
            {
                bHagaraEvent = data;
                SaveToDB();
            }
        }

        uint32 GetData(uint32 type)
        {
            if (type == DATA_HAGARA_EVENT)
                return bHagaraEvent;

            return 0;
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            if (type == DATA_BLACKHORN)
                if (Creature* pDeck = instance->GetCreature(uiDeckGUID))
                    pDeck->SetVisible(state == DONE ? true : false);

            if (type == DATA_SPINE)
                if (Creature* pMaelstorm = instance->GetCreature(uiMaelstormGUID))
                    pMaelstorm->SetVisible(state == DONE ? true : false);

            if (state == IN_PROGRESS)
            {
                if (!teleportGUIDs.empty())
                    for (std::vector<uint64>::const_iterator itr = teleportGUIDs.begin(); itr != teleportGUIDs.end(); ++itr)
                        if (Creature* pTeleport = instance->GetCreature((*itr)))
                        {
                            pTeleport->RemoveAura(SPELL_TELEPORT_VISUAL_ACTIVE);
                            pTeleport->CastSpell(pTeleport, SPELL_TELEPORT_VISUAL_DISABLED, true);
                        }
            }
            else
            {
                if (!teleportGUIDs.empty())
                    for (std::vector<uint64>::const_iterator itr = teleportGUIDs.begin(); itr != teleportGUIDs.end(); ++itr)
                        if (Creature* pTeleport = instance->GetCreature((*itr)))
                        {
                            pTeleport->RemoveAura(SPELL_TELEPORT_VISUAL_DISABLED);
                            pTeleport->CastSpell(pTeleport, SPELL_TELEPORT_VISUAL_ACTIVE, true);
                        }
            }


            return true;
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::string str_data;

            std::ostringstream saveStream;
            saveStream << "D S " << GetBossSaveData() << bHagaraEvent << " ";

            str_data = saveStream.str();

            OUT_SAVE_INST_DATA_COMPLETE;
            return str_data;
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

            if (dataHead1 == 'D' && dataHead2 == 'S')
            {
                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;
                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;
                    SetBossState(i, EncounterState(tmpState));
                }

                uint32 tmpEvent;
                loadStream >> tmpEvent;
                if (tmpEvent != DONE) 
                    tmpEvent = NOT_STARTED;
                bHagaraEvent = tmpEvent;

            }
            else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        uint32 uiTeamInInstance;

        uint64 uiMorchokGUID;
        uint64 uiKohcromGUID;
        uint64 uiZonozzGUID;
        uint64 uiHagaraGUID;
        uint64 uiValeeraGUID;
        uint64 uiEiendormiGUID;
        uint64 uiUltraxionGUID;
        uint64 uiAllianceShipGUID;
        uint64 uiSwayzeGUID;
        uint64 uiReevsGUID;
        uint64 uiBlackhornGUID;
        uint64 uiDeckGUID;
        uint64 uiMaelstormGUID;

        uint64 uiLesserCacheofTheAspects[4];
        uint64 uiBackPlates[3];
        uint64 uiGreaterCacheofTheAspects[4];
        uint64 uiSpineGuid;

        uint64 DeathwingGUID;

        uint64 Maelstrom_trall;
        uint64 Maelstrom_kalecgos;
        uint64 Maelstrom_ysera;
        uint64 Maelstrom_nozdormy;
        uint64 Maelstrom_alexstrasza;

        std::vector<uint64> teleportGUIDs;

        uint32 bHagaraEvent;

    };
};

void WhisperToAllPlayerInZone(int32 TextId, Creature* sender)
{
    Map::PlayerList const &players = sender->GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
        if (Player* player = i->getSource())
            DoScriptText(TextId, sender, player);
}

void AddSC_instance_dragon_soul()
{
    new instance_dragon_soul();
}