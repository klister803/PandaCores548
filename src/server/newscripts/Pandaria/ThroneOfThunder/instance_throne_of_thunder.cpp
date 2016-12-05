//UWoWCore
//Throne of Thunder

#include "NewScriptPCH.h"
#include "VMapFactory.h"
#include "throne_of_thunder.h"

const DoorData doorData[] =
{
    {GO_JINROKH_PRE_DOOR,    DATA_STORM_CALLER,      DOOR_TYPE_PASSAGE, 0},
    {GO_JINROKH_EX_DOOR,     DATA_JINROKH,           DOOR_TYPE_PASSAGE, 0},
    {GO_HORRIDON_PRE_DOOR,   DATA_STORMBRINGER,      DOOR_TYPE_PASSAGE, 0},
    {GO_HORRIDON_EX_DOOR,    DATA_HORRIDON,          DOOR_TYPE_PASSAGE, 0},
    {GO_COUNCIL_EX_DOOR,     DATA_COUNCIL_OF_ELDERS, DOOR_TYPE_PASSAGE, 0},
    {GO_COUNCIL_EX2_DOOR,    DATA_COUNCIL_OF_ELDERS, DOOR_TYPE_PASSAGE, 0},
    {GO_TORTOS_EX_DOOR,      DATA_TORTOS,            DOOR_TYPE_PASSAGE, 0},
    {GO_TORTOS_EX2_DOOR,     DATA_TORTOS,            DOOR_TYPE_PASSAGE, 0},
    {GO_MEGAERA_EX_DOOR,     DATA_MEGAERA,           DOOR_TYPE_PASSAGE, 0},
    {GO_JI_KUN_EX_DOOR,      DATA_JI_KUN,            DOOR_TYPE_PASSAGE, 0},
    {GO_DURUMU_EX_DOOR,      DATA_DURUMU,            DOOR_TYPE_PASSAGE, 0},
    {GO_PRIMORDIUS_EX_DOOR,  DATA_PRIMORDIUS,        DOOR_TYPE_PASSAGE, 0},
    {GO_DARK_ANIMUS_EX_DOOR, DATA_DARK_ANIMUS,       DOOR_TYPE_PASSAGE, 0},
    {GO_IRON_QON_EX_DOOR,    DATA_IRON_QON,          DOOR_TYPE_PASSAGE, 0},
    {GO_TWIN_EX_DOOR,        DATA_TWIN_CONSORTS,     DOOR_TYPE_PASSAGE, 0},
    {0,                      0,                      DOOR_TYPE_PASSAGE, 0},
};

uint32 HorridonAddGates[4] =
{
    GO_FARRAK_GATE,
    GO_GURUBASHI_GATE,
    GO_DRAKKARI_GATE,
    GO_AMANI_GATE,
};

class instance_throne_of_thunder : public InstanceMapScript
{
public:
    instance_throne_of_thunder() : InstanceMapScript("instance_throne_of_thunder", 1098) { }

    struct instance_throne_of_thunder_InstanceMapScript : public InstanceScript
    {
        instance_throne_of_thunder_InstanceMapScript(Map* map) : InstanceScript(map) {}

        //Special lists for Megaera heads mechanic
        std::vector<uint64>megaeralist;
        uint32 megaeraheadlist[3];
        uint32 lastdiedhead;

        //GameObjects
        uint64 jinrokhpredoorGuid;
        uint64 jinrokhentdoorGuid;
        uint64 jinrokhexdoorGuid;
        uint64 horridonpredoorGuid;
        uint64 horridonentdoorGuid;
        std::vector<uint64>horridonaddgateGuids;
        uint64 horridonexdoorGuid;
        uint64 councilexdoorGuid;
        uint64 councilex2doorGuid;
        uint64 tortosexdoorGuid;
        uint64 tortosex2doorGuid;
        uint64 megaeraexdoorGuid;
        uint64 jikunexdoorGuid;
        uint64 durumuexdoorGuid;
        uint64 primordiusentdoorGuid;
        uint64 secretradendoorGuid;
        uint64 primordiusexdoorGuid;
        uint64 danimusentdoorGuid;
        uint64 danimusexdoorGuid;
        uint64 ironqonentdoorGuid;
        uint64 ironqonexdoorGuid;
        uint64 twinentdoorGuid;
        uint64 twinexdoorGuid;
        uint64 radenentdoorGuid;
        
        //Creature
        uint64 stormcallerGuid;
        uint64 jinrokhGuid;
        uint64 stormbringerGuid;
        uint64 horridonGuid;
        uint64 hgatecontrollerGuid;
        uint64 jalakGuid;
        uint64 mallakGuid;
        uint64 marliGuid;
        uint64 kazrajinGuid;
        uint64 sulGuid;
        uint64 garajalsoulGuid;
        uint64 tortosGuid;
        uint64 megaeraGuid;
        uint64 nextmegaeraheadGuid;
        uint64 jikunGuid;
        uint64 durumuGuid;
        uint64 primordiusGuid;
        uint64 darkanimusGuid;
        uint64 ironqonGuid;
        uint64 roshakGuid;
        uint64 quetzalGuid;
        uint64 damrenGuid;
        uint64 sulinGuid;
        uint64 lulinGuid;
        uint64 leishenGuid;
        uint64 radenGuid;
        uint64 canimaGuid;
        uint64 cvitaGuid;

        std::vector <uint64> councilGuids;
        std::vector <uint64> mogufontsGuids;
        std::vector <uint64> councilentdoorGuids;
        std::vector <uint64> jikunfeatherGuids;
        std::vector <uint64> massiveanimagolemGuids;
        std::vector <uint64> twinfencedoorGuids;
        
        void Initialize()
        {
            SetBossNumber(16);
            LoadDoorData(doorData);

            //GameObject
            jinrokhentdoorGuid    = 0;
            jinrokhexdoorGuid     = 0;
            horridonpredoorGuid   = 0;
            horridonentdoorGuid   = 0;
            horridonaddgateGuids.clear();
            horridonexdoorGuid    = 0;
            councilexdoorGuid     = 0;
            councilex2doorGuid    = 0;
            tortosexdoorGuid      = 0;
            tortosex2doorGuid     = 0;
            megaeraexdoorGuid     = 0;
            jikunexdoorGuid       = 0;
            durumuexdoorGuid      = 0;
            primordiusentdoorGuid = 0;
            secretradendoorGuid   = 0;
            primordiusexdoorGuid  = 0;
            danimusentdoorGuid    = 0;
            danimusexdoorGuid     = 0;
            ironqonentdoorGuid    = 0;
            ironqonexdoorGuid     = 0;
            twinentdoorGuid       = 0;
            twinexdoorGuid        = 0;
            radenentdoorGuid      = 0;
           
            //Creature
            stormcallerGuid       = 0;
            jinrokhGuid           = 0;
            stormbringerGuid      = 0;
            horridonGuid          = 0;
            hgatecontrollerGuid   = 0;
            jalakGuid             = 0;
            mallakGuid            = 0;
            marliGuid             = 0;
            kazrajinGuid          = 0;
            sulGuid               = 0;
            garajalsoulGuid       = 0;
            tortosGuid            = 0;
            megaeraGuid           = 0;
            lastdiedhead          = 0;
            nextmegaeraheadGuid   = 0;
            jikunGuid             = 0;
            durumuGuid            = 0;
            primordiusGuid        = 0;
            darkanimusGuid        = 0;
            ironqonGuid           = 0;
            roshakGuid            = 0;
            quetzalGuid           = 0;
            damrenGuid            = 0;
            sulinGuid             = 0;
            lulinGuid             = 0;
            leishenGuid           = 0;
            radenGuid             = 0;
            canimaGuid            = 0;
            cvitaGuid             = 0;

            councilGuids.clear();
            mogufontsGuids.clear();
            councilentdoorGuids.clear();
            jikunfeatherGuids.clear();
            massiveanimagolemGuids.clear();
            twinfencedoorGuids.clear();
            megaeralist.clear();
            for (uint8 n = 0; n < 3; n++)
                megaeraheadlist[n] = 0;
            CreateMegaeraHeads();
        }

        void CreateMegaeraHeads()
        {
            uint8 mod = urand(0, 5);
            switch (mod)
            {
            case 0:
                megaeraheadlist[0] = NPC_FLAMING_HEAD_MELEE;
                megaeraheadlist[1] = NPC_FROZEN_HEAD_MELEE;
                megaeraheadlist[2] = NPC_VENOMOUS_HEAD_RANGE;
                break;
            case 1:
                megaeraheadlist[0] = NPC_FROZEN_HEAD_MELEE;
                megaeraheadlist[1] = NPC_FLAMING_HEAD_MELEE;
                megaeraheadlist[2] = NPC_VENOMOUS_HEAD_RANGE;
                break;
            case 2:
                megaeraheadlist[0] = NPC_FROZEN_HEAD_MELEE;
                megaeraheadlist[1] = NPC_VENOMOUS_HEAD_MELEE;
                megaeraheadlist[2] = NPC_FLAMING_HEAD_RANGE;
                break;
            case 3:
                megaeraheadlist[0] = NPC_VENOMOUS_HEAD_MELEE;
                megaeraheadlist[1] = NPC_FROZEN_HEAD_MELEE;
                megaeraheadlist[2] = NPC_FLAMING_HEAD_RANGE;
                break;
            case 4:
                megaeraheadlist[0] = NPC_VENOMOUS_HEAD_MELEE;
                megaeraheadlist[1] = NPC_FLAMING_HEAD_MELEE;
                megaeraheadlist[2] = NPC_FROZEN_HEAD_RANGE;
                break;
            case 5:
                megaeraheadlist[0] = NPC_FLAMING_HEAD_MELEE;
                megaeraheadlist[1] = NPC_VENOMOUS_HEAD_MELEE;
                megaeraheadlist[2] = NPC_FROZEN_HEAD_RANGE;
                break;
            }
            for (uint8 n = 0; n < 3; n++)
                instance->SummonCreature(megaeraheadlist[n], megaeraspawnpos[n]);
        }

        void ResetMegaera()
        {
            if (Creature* megaera = instance->GetCreature(megaeraGuid))
                megaera->AI()->DoAction(ACTION_MEGAERA_RESET);
            if (!megaeralist.empty())
            {
                for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                {
                    if (Creature* mh = instance->GetCreature(*itr))
                    {
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, mh);
                        mh->DespawnOrUnsummon();
                    }
                }
            }
            lastdiedhead = 0;
            megaeralist.clear();
            if (Creature* megaera = instance->GetCreature(megaeraGuid))
                for (uint8 n = 0; n < 3; n++)
                    megaera->SummonCreature(megaeraheadlist[n], megaeraspawnpos[n]);
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
            case NPC_STORM_CALLER:
                stormcallerGuid = creature->GetGUID();
                break;
            case NPC_JINROKH:
                jinrokhGuid = creature->GetGUID();
                break;
            case NPC_STORMBRINGER:
                stormbringerGuid = creature->GetGUID();
                break;
            case NPC_HORRIDON: 
                horridonGuid = creature->GetGUID();
                break;
            case NPC_JALAK:
                jalakGuid = creature->GetGUID();
                break;
            case NPC_H_GATE_CONTROLLER:
                hgatecontrollerGuid = creature->GetGUID();
                break;
            //Council of Elders
            case NPC_FROST_KING_MALAKK:
                mallakGuid = creature->GetGUID();
                councilGuids.push_back(creature->GetGUID());
                break;
            case NPC_PRINCESS_MARLI:
                marliGuid = creature->GetGUID();
                councilGuids.push_back(creature->GetGUID());
                break;  
            case NPC_KAZRAJIN:  
                kazrajinGuid = creature->GetGUID();
                councilGuids.push_back(creature->GetGUID());
                break;
            case NPC_SUL_SANDCRAWLER: 
                sulGuid = creature->GetGUID();
                councilGuids.push_back(creature->GetGUID());
                break;
            case NPC_GARAJAL_SOUL:
                garajalsoulGuid = creature->GetGUID();
                break;
            //
            case NPC_TORTOS: 
                tortosGuid = creature->GetGUID();
                break;
            //Megaera
            case NPC_MEGAERA:
                megaeraGuid = creature->GetGUID();
                break;
            case NPC_FLAMING_HEAD_MELEE:
            case NPC_FLAMING_HEAD_RANGE:
            case NPC_VENOMOUS_HEAD_MELEE:
            case NPC_VENOMOUS_HEAD_RANGE:
            case NPC_FROZEN_HEAD_MELEE:
            case NPC_FROZEN_HEAD_RANGE:
                megaeralist.push_back(creature->GetGUID());
                break;
            case NPC_JI_KUN:  
                jikunGuid = creature->GetGUID();
                break;
            case NPC_DURUMU:  
                durumuGuid = creature->GetGUID();
                break;
            case NPC_PRIMORDIUS: 
                primordiusGuid = creature->GetGUID();
                break;
            case NPC_MASSIVE_ANIMA_GOLEM:
                massiveanimagolemGuids.push_back(creature->GetGUID());
                break;
            case NPC_DARK_ANIMUS:  
                darkanimusGuid = creature->GetGUID();
                break;
            case NPC_IRON_QON:
                ironqonGuid = creature->GetGUID();
                break;
            case NPC_ROSHAK:
                roshakGuid = creature->GetGUID();
                break;
            case NPC_QUETZAL:
                quetzalGuid = creature->GetGUID();
                break;
            case NPC_DAMREN:
                damrenGuid = creature->GetGUID();
                break;
            //Twin consorts
            case NPC_SULIN:   
                sulinGuid = creature->GetGUID();
                break;
            case NPC_LULIN: 
                lulinGuid = creature->GetGUID();
                break;
            //
            case NPC_LEI_SHEN:
                leishenGuid = creature->GetGUID();
                break;
            case NPC_RA_DEN:
                radenGuid = creature->GetGUID();
                break;
            case NPC_CORRUPTED_ANIMA:
                canimaGuid = creature->GetGUID();
                break;
            case NPC_CORRUPTED_VITA:
                cvitaGuid = creature->GetGUID();
                break;
            }

            /*if (IsRaidBoss(creature->GetEntry())) online in future(need script bosses)
                if (creature->isAlive())
                    creature->CastSpell(creature, SPELL_SHADO_PAN_ONSLAUGHT, true);*/ //Patch 5.4
        }

        void OnGameObjectCreate(GameObject* go)
        {    
            switch (go->GetEntry())
            {
            case GO_JINROKH_PRE_DOOR:
                AddDoor(go, true);
                jinrokhpredoorGuid = go->GetGUID();
                break;
            case GO_JINROKH_ENT_DOOR:
                jinrokhentdoorGuid = go->GetGUID();
                break;
            //Mogu Fonts
            case GO_MOGU_SR:
            case GO_MOGU_NR:
            case GO_MOGU_NL:
            case GO_MOGU_SL:
                mogufontsGuids.push_back(go->GetGUID());
                break;
            //
            case GO_JINROKH_EX_DOOR:
                AddDoor(go, true);
                jinrokhexdoorGuid = go->GetGUID();
                break;
            case GO_HORRIDON_PRE_DOOR:
                AddDoor(go, true);
                horridonpredoorGuid = go->GetGUID();
                break;
            case GO_HORRIDON_ENT_DOOR:
                horridonentdoorGuid = go->GetGUID();
                break;
            case GO_FARRAK_GATE:
            case GO_GURUBASHI_GATE:
            case GO_DRAKKARI_GATE:
            case GO_AMANI_GATE:
                horridonaddgateGuids.push_back(go->GetGUID());
                break;
            case GO_HORRIDON_EX_DOOR:
                AddDoor(go, true);
                horridonexdoorGuid = go->GetGUID();
                break;
            case GO_COUNCIL_LENT_DOOR:
                councilentdoorGuids.push_back(go->GetGUID());
                break;
            case GO_COUNCIL_RENT_DOOR:
                councilentdoorGuids.push_back(go->GetGUID());
                break;
            case GO_COUNCIL_EX_DOOR:
                AddDoor(go, true);
                councilexdoorGuid = go->GetGUID();
                break;
            case GO_COUNCIL_EX2_DOOR:
                AddDoor(go, true);
                councilex2doorGuid = go->GetGUID();
                break;
            case GO_TORTOS_EX_DOOR:
                AddDoor(go, true);
                tortosexdoorGuid = go->GetGUID();
                break;
            case GO_TORTOS_EX2_DOOR:
                AddDoor(go, true);
                tortosex2doorGuid = go->GetGUID();
                break;
            case GO_MEGAERA_EX_DOOR:
                AddDoor(go, true);
                megaeraexdoorGuid = go->GetGUID();
                break;
            case GO_JI_KUN_FEATHER:
                jikunfeatherGuids.push_back(go->GetGUID());
                break;
            case GO_JI_KUN_EX_DOOR:
                AddDoor(go, true);
                jikunexdoorGuid = go->GetGUID();
                break;
            case GO_DURUMU_EX_DOOR:
                AddDoor(go, true);
                durumuexdoorGuid = go->GetGUID();
                break;
            case GO_PRIMORDIUS_ENT_DOOR:
                primordiusentdoorGuid = go->GetGUID();
                break;
            case GO_S_RA_DEN_ENT_DOOR:
                LoadSecretRaDenDoor(go);
                secretradendoorGuid = go->GetGUID();
            case GO_PRIMORDIUS_EX_DOOR:
                AddDoor(go, true);
                primordiusexdoorGuid = go->GetGUID();
                break;
            case GO_DARK_ANIMUS_ENT_DOOR:
                danimusentdoorGuid = go->GetGUID();
                break;
            case GO_DARK_ANIMUS_EX_DOOR:
                AddDoor(go, true);
                danimusexdoorGuid = go->GetGUID();
                break;
            case GO_IRON_QON_ENT_DOOR:
                ironqonentdoorGuid = go->GetGUID();
                break;
            case GO_IRON_QON_EX_DOOR:
                AddDoor(go, true);
                ironqonexdoorGuid = go->GetGUID();
                break;
            case GO_TWIN_ENT_DOOR:
                twinentdoorGuid = go->GetGUID();
                break;
            case GO_TWIN_FENCE_DOOR:
                twinfencedoorGuids.push_back(go->GetGUID());
                break;
            case GO_TWIN_FENCE_DOOR_2:
                twinfencedoorGuids.push_back(go->GetGUID());
                break;
            case GO_TWIN_EX_DOOR:
                AddDoor(go, true);
                twinexdoorGuid = go->GetGUID();
                break;
            case GO_RA_DEN_ENT_DOOR:
                radenentdoorGuid = go->GetGUID();
                break;
            default:
                break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
            case DATA_STORM_CALLER:
                if (state == DONE)
                    HandleGameObject(jinrokhpredoorGuid, true);
                break;
            case DATA_JINROKH:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector<uint64>::const_iterator guid = mogufontsGuids.begin(); guid != mogufontsGuids.end(); guid++)
                            HandleGameObject(*guid, false);
                        HandleGameObject(jinrokhentdoorGuid, true);
                        SetData(DATA_RESET_MOGU_FONTS, 0);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(jinrokhentdoorGuid, false);
                        break;
                    case DONE:
                        HandleGameObject(jinrokhentdoorGuid, true);
                        HandleGameObject(jinrokhexdoorGuid, true); 
                        break;
                    }
                }
                break;
            case DATA_STORMBRINGER:
                if (state == DONE)
                    HandleGameObject(horridonpredoorGuid, true);
                break;
            case DATA_HORRIDON:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        ResetHorridonAddGates();
                        HandleGameObject(horridonentdoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        ResetHorridonAddGates();
                        HandleGameObject(horridonentdoorGuid, false);
                        break;
                    case DONE:
                        HandleGameObject(horridonentdoorGuid, true);
                        HandleGameObject(horridonexdoorGuid, true);
                        break;
                    }
                }
                break;
            case DATA_COUNCIL_OF_ELDERS:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector <uint64>::const_iterator guids = councilentdoorGuids.begin(); guids != councilentdoorGuids.end(); guids++)
                            HandleGameObject(*guids, true);
                        break;
                    case IN_PROGRESS:
                        for (std::vector <uint64>::const_iterator guids = councilentdoorGuids.begin(); guids != councilentdoorGuids.end(); guids++)
                            HandleGameObject(*guids, false);
                        break;
                    case DONE:
                        for (std::vector <uint64>::const_iterator guids = councilentdoorGuids.begin(); guids != councilentdoorGuids.end(); guids++)
                            HandleGameObject(*guids, true);
                        if (Creature* gs = instance->GetCreature(garajalsoulGuid))
                            gs->DespawnOrUnsummon();
                        HandleGameObject(councilexdoorGuid, true);
                        HandleGameObject(councilex2doorGuid, true);
                        break;
                    }
                }
                break;
            case DATA_TORTOS:
                if (state == DONE)
                {
                    HandleGameObject(tortosexdoorGuid, true);
                    HandleGameObject(tortosex2doorGuid, true);
                }
                break;
            case DATA_MEGAERA:
                switch (state)
                {
                case NOT_STARTED:
                    if (Creature* megaera = instance->GetCreature(megaeraGuid))
                        megaera->AI()->DoAction(ACTION_MEGAERA_RESET);
                    break;
                case IN_PROGRESS:
                    if (Creature* megaera = instance->GetCreature(megaeraGuid))
                        megaera->AI()->DoAction(ACTION_MEGAERA_IN_PROGRESS);
                    if (!megaeralist.empty())
                        for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                            if (Creature* mh = instance->GetCreature(*itr))
                                if (mh->GetEntry() == NPC_FLAMING_HEAD_MELEE || mh->GetEntry() == NPC_FROZEN_HEAD_MELEE || mh->GetEntry() == NPC_VENOMOUS_HEAD_MELEE)
                                    SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, mh);        
                    break;
                case DONE:
                    if (Creature* megaera = instance->GetCreature(megaeraGuid))
                    {
                        if (!megaeralist.empty())
                            for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                                if (Creature* mh = instance->GetCreature(*itr))
                                    mh->AI()->DoAction(ACTION_UNSUMMON);
                        lastdiedhead = 0;
                        megaeralist.clear();
                        megaera->setFaction(35);
                        if (!instance->IsLfr())
                            if (GameObject* chest = megaera->SummonGameObject(218805, 6415.06f, 4527.67f, -209.1780f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 604800))
                                chest->SetObjectScale(3.0f);
                    }
                    HandleGameObject(megaeraexdoorGuid, true);
                    break;
                case FAIL:
                    ResetMegaera();
                    break;
                default:
                    break;
                }
                break;
            case DATA_JI_KUN:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                    case DONE:
                        HandleGameObject(megaeraexdoorGuid, true);
                        HandleGameObject(jikunexdoorGuid, true);
                        for (std::vector <uint64>::const_iterator guid = jikunfeatherGuids.begin(); guid != jikunfeatherGuids.end(); guid++)
                            if (GameObject* feather = instance->GetGameObject(*guid))
                                feather->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(megaeraexdoorGuid, false);
                        for (std::vector <uint64>::const_iterator guid = jikunfeatherGuids.begin(); guid != jikunfeatherGuids.end(); guid++)
                            if (GameObject* feather = instance->GetGameObject(*guid))
                                feather->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    }
                }
                break;
            case DATA_DURUMU: 
                if (state == DONE)
                    HandleGameObject(durumuexdoorGuid, true);
                break;
            case DATA_PRIMORDIUS:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        HandleGameObject(primordiusentdoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(primordiusentdoorGuid, false);
                        break;
                    case DONE:
                        HandleGameObject(primordiusentdoorGuid, true);
                        HandleGameObject(primordiusexdoorGuid, true);
                        break;
                    }
                }
                break;
            case DATA_DARK_ANIMUS:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        for (std::vector<uint64>::const_iterator guid = massiveanimagolemGuids.begin(); guid != massiveanimagolemGuids.end(); guid++)
                        {
                            if (Creature* mag = instance->GetCreature(*guid))
                            {
                                if (mag->isAlive() && mag->isInCombat())
                                    mag->AI()->EnterEvadeMode();
                                else if (!mag->isAlive())
                                {
                                    mag->Respawn();
                                    mag->GetMotionMaster()->MoveTargetedHome();
                                }
                            }
                        }
                        HandleGameObject(danimusentdoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        if (Creature* animus = instance->GetCreature(darkanimusGuid))
                        {
                            if (animus->isAlive() && !animus->isInCombat())
                                animus->AI()->DoZoneInCombat(animus, 150.0f);
                        }

                        for (std::vector<uint64>::const_iterator guid = massiveanimagolemGuids.begin(); guid != massiveanimagolemGuids.end(); guid++)
                        {
                            if (Creature* mag = instance->GetCreature(*guid))
                            {
                                if (mag->isAlive() && !mag->isInCombat())
                                    mag->AI()->DoZoneInCombat(mag, 150.0f);
                            }
                        }
                        HandleGameObject(danimusentdoorGuid, false);
                        break;
                    case DONE:
                        HandleGameObject(danimusentdoorGuid, true);
                        HandleGameObject(danimusexdoorGuid, true);
                        break;
                    }
                }
                break;
            case DATA_IRON_QON:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                        HandleGameObject(ironqonentdoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(ironqonentdoorGuid, false);
                        break;
                    case DONE:
                        HandleGameObject(ironqonentdoorGuid, true);
                        HandleGameObject(ironqonexdoorGuid, true);
                        break;
                    }
                }
                break;
            case DATA_TWIN_CONSORTS:
                {
                    switch (state)
                    {
                        case NOT_STARTED:
                            for (std::vector<uint64>::const_iterator guid = twinfencedoorGuids.begin(); guid != twinfencedoorGuids.end(); guid++)
                                HandleGameObject(*guid, true);
                            HandleGameObject(twinentdoorGuid, true);
                            break;
                        case IN_PROGRESS:
                            for (std::vector<uint64>::const_iterator guid = twinfencedoorGuids.begin(); guid != twinfencedoorGuids.end(); guid++)
                                HandleGameObject(*guid, false);
                            HandleGameObject(twinentdoorGuid, false);
                            break;
                        case DONE:
                            for (std::vector<uint64>::const_iterator guid = twinfencedoorGuids.begin(); guid != twinfencedoorGuids.end(); guid++)
                                HandleGameObject(*guid, true);
                            HandleGameObject(twinentdoorGuid, true);
                            HandleGameObject(twinexdoorGuid, true);
                            break;                         
                    }
                }
                break;
            case DATA_RA_DEN:
                {
                    switch (state)
                    {
                    case NOT_STARTED:
                    case DONE:
                        HandleGameObject(radenentdoorGuid, true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(radenentdoorGuid, false);
                        break;
                    }
                }
                break;
            default:
                break;
            }
           
            if (state == DONE && id != DATA_RA_DEN)
            {
                if (GameObject* go = instance->GetGameObject(secretradendoorGuid))
                    LoadSecretRaDenDoor(go);
            }
            return true;
        }

        void ResetHorridonAddGates()
        {
            if (!horridonaddgateGuids.empty())
                for (std::vector<uint64>::const_iterator itr = horridonaddgateGuids.begin(); itr != horridonaddgateGuids.end(); itr++)
                    if (GameObject* gate = instance->GetGameObject(*itr))
                        gate->SetGoState(GO_STATE_READY);
        }

        void LoadSecretRaDenDoor(GameObject* go)
        {
            if (!go || !instance->IsHeroic())
                return;

            if (InstanceMap* im = instance->ToInstanceMap())
            {
                InstanceScript* pinstance = im->GetInstanceScript();
                for (uint8 n = DATA_STORM_CALLER; n <= DATA_LEI_SHEN; n++)
                {
                    if (pinstance->GetBossState(n) != DONE)
                        return;
                }
                go->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
            case DATA_RESET_MOGU_FONTS:
                for (std::vector<uint64>::const_iterator itr = mogufontsGuids.begin(); itr != mogufontsGuids.end(); itr++)
                    if (GameObject* mogufont = instance->GetGameObject(*itr))
                        mogufont->SetGoState(GO_STATE_READY);
                break;
            case DATA_SEND_LAST_DIED_HEAD:
                lastdiedhead = data;
                if (!megaeralist.empty())
                {
                    for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                    {
                        if (Creature* mh = instance->GetCreature(*itr))
                        {
                            if (mh->GetEntry() != lastdiedhead && (mh->GetEntry() == NPC_FLAMING_HEAD_MELEE || mh->GetEntry() == NPC_FROZEN_HEAD_MELEE || mh->GetEntry() == NPC_VENOMOUS_HEAD_MELEE))
                            {
                                mh->SetFullHealth();
                                mh->CastSpell(mh, SPELL_HYDRA_FRENZY, true);
                            }
                        }
                    }
                }
                break;
            }
        }

        uint32 GetData(uint32 type)
        {
            switch (type)
            {
            case DATA_CHECK_VALIDATE_THUNDERING_THROW:
                for (std::vector<uint64>::const_iterator itr = mogufontsGuids.begin(); itr != mogufontsGuids.end(); itr++)
                    if (GameObject* mogufont = instance->GetGameObject(*itr))
                        if (mogufont->GetGoState() == GO_STATE_READY)
                            return 1;
            case DATA_CHECK_COUNCIL_PROGRESS:
                if (!councilGuids.empty())
                {
                    for (std::vector<uint64>::const_iterator itr = councilGuids.begin(); itr != councilGuids.end(); itr++)
                        if (Creature* council = instance->GetCreature(*itr))
                            if (council->isAlive())
                                return 1;
                    return 0;
                }
            case DATA_CHECK_PROGRESS_MEGAERA:
                if (Creature* megaera = instance->GetCreature(megaeraGuid))
                {
                    if (megaera->GetHealthPct() <= 14.2)//Done
                    {
                        megaera->Kill(megaera);
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, megaera);
                        SetBossState(DATA_MEGAERA, DONE);
                        return 0;
                    }
                    else
                        return 1; //Still in progress
                }
            case DATA_GET_COUNT_RANGE_HEADS:
                if (!megaeralist.empty())
                {
                    uint8 count = 0;
                    for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                    {
                        if (Creature* mh = instance->GetCreature(*itr))
                            if (mh->GetEntry() == NPC_FLAMING_HEAD_RANGE || mh->GetEntry() == NPC_FROZEN_HEAD_RANGE || mh->GetEntry() == NPC_VENOMOUS_HEAD_RANGE)
                                count++;
                    }
                    return count;
                }
            }
            return 0;
        }

        void CreatureDies(Creature* creature, Unit* /*killer*/)
        {
            switch (creature->GetEntry())
            {
            case NPC_FROST_KING_MALAKK:
            case NPC_PRINCESS_MARLI:
            case NPC_KAZRAJIN:
            case NPC_SUL_SANDCRAWLER:
                if (!councilGuids.empty())
                {
                    for (std::vector<uint64>::const_iterator itr = councilGuids.begin(); itr != councilGuids.end(); itr++)
                        if (Creature* council = instance->GetCreature(*itr))
                            if (council->isAlive())
                                return;

                    SetBossState(DATA_COUNCIL_OF_ELDERS, DONE);
                }
                break;
            default:
                break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
            case NPC_JINROKH:
                return jinrokhGuid;
            case NPC_HORRIDON: 
                return horridonGuid;
            case NPC_JALAK:
                return jalakGuid;
            case NPC_H_GATE_CONTROLLER:
                return hgatecontrollerGuid;
            case GO_FARRAK_GATE:
            case GO_GURUBASHI_GATE:
            case GO_DRAKKARI_GATE:
            case GO_AMANI_GATE:
                for (std::vector<uint64>::const_iterator itr = horridonaddgateGuids.begin(); itr != horridonaddgateGuids.end(); itr++)
                    if (GameObject* gate = instance->GetGameObject(*itr))
                        if (gate->GetEntry() == type)
                            return gate->GetGUID();
            //Council of Elders
            case NPC_FROST_KING_MALAKK:
                return mallakGuid;
            case NPC_PRINCESS_MARLI:
                return marliGuid;
            case NPC_KAZRAJIN:  
                return kazrajinGuid;
            case NPC_SUL_SANDCRAWLER: 
                return sulGuid;
            case NPC_GARAJAL_SOUL:
                return garajalsoulGuid;
            //
            //Megaera
            case NPC_MEGAERA:
                return megaeraGuid;
            case NPC_FLAMING_HEAD_MELEE:
            case NPC_FLAMING_HEAD_RANGE:
            case NPC_VENOMOUS_HEAD_MELEE:
            case NPC_VENOMOUS_HEAD_RANGE:
            case NPC_FROZEN_HEAD_MELEE:
            case NPC_FROZEN_HEAD_RANGE:
                for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                    if (Creature* mh = instance->GetCreature(*itr))
                        if (mh->GetEntry() == type)
                            return mh->GetGUID();

            case DATA_GET_NEXT_HEAD:
                if (!megaeralist.empty())
                    for (std::vector<uint64>::const_iterator itr = megaeralist.begin(); itr != megaeralist.end(); itr++)
                        if (Creature* mh = instance->GetCreature(*itr))
                            if (mh->GetEntry() != lastdiedhead && (mh->GetEntry() == NPC_FLAMING_HEAD_RANGE || mh->GetEntry() == NPC_FROZEN_HEAD_RANGE || mh->GetEntry() == NPC_VENOMOUS_HEAD_RANGE))
                                return mh->GetGUID();

            case NPC_TORTOS: 
                return tortosGuid;
            case NPC_JI_KUN:  
                return jikunGuid;
            case NPC_DURUMU:  
                return durumuGuid;
            case NPC_PRIMORDIUS: 
                return primordiusGuid;
            case NPC_DARK_ANIMUS:  
                return darkanimusGuid;
            case NPC_IRON_QON:
                return ironqonGuid;
            //Iron Qon Maunts
            case NPC_ROSHAK:
                return roshakGuid;
            case NPC_QUETZAL:
                return quetzalGuid;
            case NPC_DAMREN:
                return damrenGuid;
            //
            //Twin consorts
            case NPC_SULIN:   
                return sulinGuid;
            case NPC_LULIN: 
                return lulinGuid;
            //
            case NPC_LEI_SHEN:
                return leishenGuid;
            case NPC_RA_DEN:
                return radenGuid;
            case NPC_CORRUPTED_ANIMA:
                return canimaGuid;
            case NPC_CORRUPTED_VITA:
                return cvitaGuid;
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

        bool IsRaidBoss(uint32 creature_entry)
        {
            switch (creature_entry)
            {
            case NPC_JINROKH:
            case NPC_HORRIDON:
            case NPC_JALAK:
            case NPC_FROST_KING_MALAKK:
            case NPC_PRINCESS_MARLI:
            case NPC_KAZRAJIN:
            case NPC_SUL_SANDCRAWLER:
            case NPC_TORTOS:
            case NPC_FLAMING_HEAD_MELEE:
            case NPC_FROZEN_HEAD_MELEE:
            case NPC_VENOMOUS_HEAD_MELEE:
            case NPC_JI_KUN:
            case NPC_DURUMU:
            case NPC_PRIMORDIUS:
            case NPC_DARK_ANIMUS:
            case NPC_IRON_QON:
            case NPC_ROSHAK:
            case NPC_QUETZAL:
            case NPC_DAMREN:
            case NPC_SULIN:
            case NPC_LULIN:
            case NPC_LEI_SHEN:
            case NPC_RA_DEN:
                return true;
            }
            return false;
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
            for (uint32 i = 0; i < 16; ++i)
                loadStream >> buff;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_throne_of_thunder_InstanceMapScript(map);
    }
};

enum sSpells
{
    SPELL_STORM_WEAPON   = 139319,
    SPELL_STORM_ENERGY   = 139322,
    SPELL_CHAIN_LIGHTNIG = 139903,
    SPELL_STORMCLOUD     = 139900,
};

enum sEvent
{
    EVENT_STORM_ENERGY   = 1,
    EVENT_CHAIN_LIGHTNIG = 2,
};

//Mini boss, guard Jinrokh entrance
class npc_storm_caller : public CreatureScript
{
    public:
        npc_storm_caller() : CreatureScript("npc_storm_caller") { }
        
        struct npc_storm_callerAI : public BossAI
        {
            npc_storm_callerAI(Creature* pCreature) : BossAI(pCreature, DATA_STORM_CALLER)
            {
                pInstance = pCreature->GetInstanceScript();
            }
            
            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_STORM_WEAPON);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                me->AddAura(SPELL_STORM_WEAPON, me);
                events.ScheduleEvent(EVENT_STORM_ENERGY, urand(15000, 20000));
            }

            void JustDied(Unit* killer)
            {
                _JustDied();
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    if (eventId == EVENT_STORM_ENERGY)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                            DoCast(target, SPELL_STORM_ENERGY);
                        events.ScheduleEvent(EVENT_STORM_ENERGY, urand(15000, 20000));
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
        
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_storm_callerAI(pCreature);
        }
};

//Mini boss, guard Horridon entrance
class npc_stormbringer : public CreatureScript
{
    public:
        npc_stormbringer() : CreatureScript("npc_stormbringer") { }
        
        struct npc_stormbringerAI : public BossAI
        {
            npc_stormbringerAI(Creature* pCreature) : BossAI(pCreature, DATA_STORMBRINGER)
            {
                pInstance = pCreature->GetInstanceScript();
            }
            
            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_STORMCLOUD);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                DoCast(me, SPELL_STORMCLOUD);
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNIG, urand(15000, 20000));
            }

            void JustDied(Unit* killer)
            {
                _JustDied();
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    if (eventId == EVENT_CHAIN_LIGHTNIG)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                            DoCast(target, SPELL_CHAIN_LIGHTNIG);
                        events.ScheduleEvent(EVENT_STORM_ENERGY, urand(15000, 20000));
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
        
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_stormbringerAI(pCreature);
        }
};

Position const onbridge    = {6045.42f, 5163.28f, 148.1146f, 1.548f};
Position const underbridge = {6042.31f, 5088.96f,  -43.152f, 4.654f};

//Te;eport to Tortos, and back
class npc_teleporter : public CreatureScript
{
    public:
        npc_teleporter() : CreatureScript("npc_teleporter") {}

        struct npc_teleporterAI : public CreatureAI
        {
            npc_teleporterAI(Creature* creature) : CreatureAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->AddAura(126493, me); //Visual
            }

            InstanceScript* instance;

            void Reset(){}
            
            void OnSpellClick(Unit* clicker)
            {
                if (instance)
                {
                   if (clicker->GetTypeId() == TYPEID_PLAYER)
                   {
                       if (me->GetPositionZ() > 140.0f)
                           clicker->NearTeleportTo(underbridge.GetPositionX(), underbridge.GetPositionY(), underbridge.GetPositionZ(), underbridge.GetOrientation());
                       else
                           clicker->NearTeleportTo(onbridge.GetPositionX(), onbridge.GetPositionY(), onbridge.GetPositionZ(), onbridge.GetOrientation());
                   }
                }
            }
            
            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void UpdateAI(uint32 diff){}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_teleporterAI(creature);
        }
};

void AddSC_instance_throne_of_thunder()
{
    new instance_throne_of_thunder();
    new npc_storm_caller();
    new npc_stormbringer();
    new npc_teleporter();
}
