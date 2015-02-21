//UWoWCore
//Siege of Orgrimmar

#include "NewScriptPCH.h"
#include "VMapFactory.h"
#include "siege_of_orgrimmar.h"
#include "AccountMgr.h"
#include "Transport.h"

Position const LorewalkerChoSpawn[5]  = {
    {1448.236f, 312.6528f, 289.2837f, 4.652967f},
    {1441.406f, 988.1795f, 340.1876f, 1.985304f},   //fallen
    {806.067f,  841.3726f, 371.2589f, 1.791488f},   //norushen
    {805.7786f, 879.8768f, 371.0946f, 1.911932f},   //sha
    {761.5104f, 1048.512f, 357.2339f, 1.767873f},   //sha finish
};

Position const Sha_of_pride_Norushe  = {797.357f, 880.5637f, 371.1606f, 1.786108f };

DoorData const doorData[] =
{
    {GO_IMMERSEUS_EX_DOOR,                   DATA_IMMERSEUS,              DOOR_TYPE_PASSAGE,    BOUNDARY_NONE   },
    {GO_SHA_FIELD,                           DATA_F_PROTECTORS,           DOOR_TYPE_PASSAGE,    BOUNDARY_NONE   },
    {GO_NORUSHEN_EX_DOOR,                    DATA_SHA_OF_PRIDE,           DOOR_TYPE_PASSAGE,    BOUNDARY_NONE   },
    {0,                                      0,                           DOOR_TYPE_ROOM,       BOUNDARY_NONE   }, // END
};

class instance_siege_of_orgrimmar : public InstanceMapScript
{
public:
    instance_siege_of_orgrimmar() : InstanceMapScript("instance_siege_of_orgrimmar", 1136) { }

    struct instance_siege_of_orgrimmar_InstanceMapScript : public InstanceScript
    {
        instance_siege_of_orgrimmar_InstanceMapScript(Map* map) : InstanceScript(map) {}

        std::map<uint32, uint64> easyGUIDconteiner;
        //Misc
        uint32 TeamInInstance;
        uint32 _teamInInstance;
        uint32 EventfieldOfSha;
        uint32 lingering_corruption_count;

        //Galakras worldstate
        uint16 ShowCannon;
        uint16 CannonCount;
        uint32 ShowSouthTower;
        uint32 ShowNorthTower;
        uint32 SouthTowerCount;
        uint32 NorthTowerCount;

        //GameObjects
        uint64 immerseusexdoorGUID;
        uint64 chestShaVaultOfForbiddenTreasures;
        std::vector<uint64> lightqGUIDs;
        
        //Creature
        std::set<uint64> shaSlgGUID;
        uint64 LorewalkerChoGUIDtmp;
        uint64 fpGUID[3];
        uint64 WrynOrLorthemarGUID;
        uint64 JainaOrSylvanaGUID;
        uint64 VereesaOrAethasGUID;
        uint64 sExpertGUID;
        uint64 nExpertGUID;

        EventMap Events;

        bool onInitEnterState;

        Transport* transport;

        ~instance_siege_of_orgrimmar_InstanceMapScript()
        {
            delete transport;
        }

        void Initialize()
        {
            SetBossNumber(DATA_MAX);
            LoadDoorData(doorData);

            TeamInInstance = 0;
            _teamInInstance = 0;
            lingering_corruption_count = 0;

            //GameObject
            immerseusexdoorGUID     = 0;
            chestShaVaultOfForbiddenTreasures = 0;
            lightqGUIDs.clear();
           
            //Creature
            LorewalkerChoGUIDtmp    = 0;
            memset(fpGUID, 0, 3 * sizeof(uint64));
            EventfieldOfSha     = 0;
            WrynOrLorthemarGUID     = 0;
            JainaOrSylvanaGUID      = 0;
            VereesaOrAethasGUID     = 0;
            sExpertGUID             = 0;
            nExpertGUID             = 0;

            onInitEnterState = false;

            transport = NULL;
            
            //Galakras WorldState
            ShowCannon            = NOT_STARTED;
            ShowSouthTower        = NOT_STARTED;
            ShowNorthTower        = NOT_STARTED;

            CannonCount     = 0;
            SouthTowerCount = 0;
            NorthTowerCount = 0;
        }

        void FillInitialWorldStates(WorldPacket& data)
        {
            data << uint32(ShowSouthTower == IN_PROGRESS)        << uint32(WS_SHOW_SOUTH_TOWER);
            data << uint32(ShowNorthTower == IN_PROGRESS)        << uint32(WS_SHOW_NORTH_TOWER);
            data << uint32(ShowSouthTower == SPECIAL)            << uint32(WS_SHOW_CAPTURE_SOUTH_TOWER);
            data << uint32(ShowNorthTower == SPECIAL)            << uint32(WS_SHOW_CAPTURE_NORTH_TOWER);
            data << uint32(SouthTowerCount)                      << uint32(WS_SOUTH_TOWER);
            data << uint32(NorthTowerCount)                      << uint32(WS_NORTH_TOWER);
            data << uint32(SouthTowerCount)                      << uint32(WS_CAPTURE_SOUTH_TOWER);
            data << uint32(NorthTowerCount)                      << uint32(WS_CAPTURE_NORTH_TOWER);
            data << uint32(ShowCannon == IN_PROGRESS)            << uint32(WS_SHOW_KORKRON_CANNON);
            data << uint32(CannonCount)                          << uint32(WS_KORKRON_CANNON_COUNT);
        }

        void OnPlayerEnter(Player* player)
        {
            if (!TeamInInstance)
                TeamInInstance = player->GetTeam();
            
            if (GetBossState(DATA_SHA_OF_PRIDE) == IN_PROGRESS)
                player->CastSpell(player, 144343, true);
            
            if (GetBossState(DATA_NORUSHEN) == IN_PROGRESS)
                player->CastSpell(player, 144421, true);

            //Custom check.
            CustomSpellCheck(player);

            //not handle lorewalker summon if already done.
            if (onInitEnterState)
                return;
            onInitEnterState = true;

            DoSummoneEventCreatures();

            if (!transport)
                transport = CreateTransport(TeamInInstance == HORDE ? GO_SHIP_HORDE : GO_SHIP_ALLIANCE, TRANSPORT_PERIOD);

            SendTransportInit(player);
        }

        //Some auras should not stay after relog. If player out of dung whey remove automatically
        //but if player on dungeon he could use it.
        void CustomSpellCheck(Player* player)
        {
            if (GetBossState(DATA_SHA_OF_PRIDE) != IN_PROGRESS)
            {
                //Sha of pride: SPELL_OVERCOME
                if (player->HasAura(144843))
                    player->RemoveAura(144843);

                //Sha of pride: SPELL_PRIDE
                if (player->HasAura(144343))
                    player->RemoveAura(144343);
            }
            if (GetBossState(DATA_NORUSHEN) != IN_PROGRESS)
            {
                //Norushen: Coruption
                if (player->HasAura(144421))
                    player->RemoveAura(144421);

                //Norushen: PURIFIED
                if (player->HasAura(144452))
                    player->RemoveAura(144452);
            }
        }

        void DoSummoneEventCreatures()
        {
            if (GetBossState(DATA_IMMERSEUS) != DONE)
            {
                if (Creature* cho = instance->SummonCreature(NPC_LOREWALKER_CHO, LorewalkerChoSpawn[0]))
                {
                    cho->setActive(true);
                    LorewalkerChoGUIDtmp = cho->GetGUID();
                    cho->AI()->SetData(DATA_IMMERSEUS, NOT_STARTED);
                }
            }else if (GetBossState(DATA_F_PROTECTORS) != DONE)
            {
                if (Creature* cho = instance->SummonCreature(NPC_LOREWALKER_CHO, LorewalkerChoSpawn[1]))
                {
                    cho->setActive(true);
                    LorewalkerChoGUIDtmp = cho->GetGUID();
                    cho->AI()->SetData(DATA_F_PROTECTORS, NOT_STARTED);
                }
            }else if (GetBossState(DATA_NORUSHEN) != DONE)
            {
                if (Creature* cho = instance->SummonCreature(NPC_LOREWALKER_CHO2, LorewalkerChoSpawn[2]))
                {
                    cho->setActive(true);
                    LorewalkerChoGUIDtmp = cho->GetGUID();
                }
            }else if (GetBossState(DATA_SHA_OF_PRIDE) != DONE)
            {
                if (Creature * c = instance->SummonCreature(NPC_SHA_NORUSHEN, Sha_of_pride_Norushe))
                    c->setActive(true);
                if (Creature * c = instance->SummonCreature(NPC_LOREWALKER_CHO3, LorewalkerChoSpawn[3]))
                {
                    LorewalkerChoGUIDtmp = c->GetGUID();
                    c->setActive(true);
                }
            }else if (GetBossState(DATA_GALAKRAS) != DONE)
            {
                if (Creature * c = instance->SummonCreature(NPC_LOREWALKER_CHO3, LorewalkerChoSpawn[4]))
                {
                    LorewalkerChoGUIDtmp = c->GetGUID();
                    c->setActive(true);
                    c->AI()->DoAction(EVENT_2);
                }
            }
        }

        void OnCreatureCreate(Creature* creature)
        {
            if (!_teamInInstance)
            {
                Map::PlayerList const& players = instance->GetPlayers();
                if (!players.isEmpty())
                    if (Player* player = players.begin()->getSource())
                        _teamInInstance = player->GetTeam();
            }

            switch (creature->GetEntry())
            {
                case NPC_IMMERSEUS:
                case NPC_PUDDLE_POINT:
                case NPC_GOLD_LOTOS_MOVER:
                case NPC_GOLD_LOTOS_MAIN:
                case NPC_GOLD_LOTOS_HE:
                case NPC_GOLD_LOTOS_SUN:
                case NPC_GOLD_LOTOS_ROOK:
                case NPC_EMBODIED_ANGUISH_OF_HE:
                case NPC_EMBODIED_DESPIRE_OF_SUN:
                case NPC_EMBODIED_MISERY_OF_ROOK:
                case NPC_EMBODIED_GLOOM_OF_ROOK:
                case NPC_EMBODIED_SORROW_OF_ROOK:
                case NPC_SHA_NORUSHEN:
                case NPC_SHA_TARAN_ZHU:
                case NPC_SHA_OF_PRIDE_END_LADY_JAINA:
                case NPC_SHA_OF_PRIDE_END_THERON:
                case NPC_NORUSHEN:
                case NPC_AMALGAM_OF_CORRUPTION:
                case NPC_B_H_CONTROLLER:
                case NPC_BLIND_HATRED:
                case NPC_GALAKRAS:
                case NPC_WARLORD_ZAELA:
                case NPC_TOWER_SOUTH:
                case NPC_TOWER_NORTH:
                case NPC_ANTIAIR_TURRET:
                case NPC_IRON_JUGGERNAUT:
                case NPC_KORKRON_D_SHAMAN:
                case NPC_GENERAL_NAZGRIM:
                case NPC_MALKOROK:
                case NPC_THOK:
                case NPC_BLACKFUSE:
                case NPC_KILRUK:
                case NPC_XARIL:
                case NPC_KAZTIK:
                case NPC_KORVEN: 
                case NPC_IYYOKYK:
                case NPC_KAROZ:
                case NPC_SKEER:
                case NPC_RIKKAL:
                case NPC_HISEK:
                case NPC_GARROSH:
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
                break;
           
                //Fallen Protectors
                case NPC_ROOK_STONETOE: 
                    fpGUID[0] = creature->GetGUID();
                    break;
                case NPC_SUN_TENDERHEART:
                    fpGUID[1] = creature->GetGUID();
                    break;
                case NPC_HE_SOFTFOOT:
                    fpGUID[2] = creature->GetGUID();
                    break;

                //Sha
                case NPC_SHA_OF_PRIDE:
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
                    creature->SetVisible(false);
                    break;
                case NPC_LINGERING_CORRUPTION:
                    ++lingering_corruption_count;
                    if (!creature->isAlive())
                        creature->Respawn(true);
                    break;
                case NPC_SLG_GENERIC_MOP:
                    shaSlgGUID.insert(creature->GetGUID());
                    break;
                case NPC_PORTAL_TO_ORGRIMMAR:
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
                    creature->SetVisible((GetBossState(DATA_SHA_OF_PRIDE)==DONE) ? false : true);
                    break;

                //Galakras:
                case NPC_KING_VARIAN_WRYNN_A:
                case NPC_LORTHEMAR_THERON_H:
                    WrynOrLorthemarGUID = creature->GetGUID();
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
                    break;
                case NPC_LADY_JAINA_PROUDMOORE_A:
                case NPC_LADY_SYLVANAS_WINDRUNNER_H:
                    JainaOrSylvanaGUID = creature->GetGUID();
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
                    break;
                case NPC_VEREESA_WINDRUNNER_A:
                case NPC_ARCHMAGE_AETHAS_SUNREAVER_H:
                    VereesaOrAethasGUID = creature->GetGUID();
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
                    break;
                case NPC_DEMOLITIONS_EXPERT_S_A:
                case NPC_DEMOLITIONS_EXPERT_S_H:
                    sExpertGUID = creature->GetGUID();
                    break;
                case NPC_DEMOLITIONS_EXPERT_N_A:
                case NPC_DEMOLITIONS_EXPERT_N_H:
                    nExpertGUID = creature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_NORUSHEN_EX_DOOR:
                case GO_CORRUPTED_PRISON_WEST:
                case GO_CORRUPTED_BUTTON_WEST_1:
                case GO_CORRUPTED_BUTTON_WEST_2:
                case GO_CORRUPTED_BUTTON_WEST_3:
                case GO_CORRUPTED_PRISON_EAST:
                case GO_CORRUPTED_BUTTON_EAST_1:
                case GO_CORRUPTED_BUTTON_EAST_2:
                case GO_CORRUPTED_BUTTON_EAST_3:
                case GO_CORRUPTED_PRISON_NORTH:
                case GO_CORRUPTED_BUTTON_NORTH_1:
                case GO_CORRUPTED_BUTTON_NORTH_2:
                case GO_CORRUPTED_BUTTON_NORTH_3:
                case GO_CORRUPTED_PRISON_SOUTH:
                case GO_CORRUPTED_BUTTON_SOUTH_1:
                case GO_CORRUPTED_BUTTON_SOUTH_2:
                case GO_CORRUPTED_BUTTON_SOUTH_3:
                case GO_SOUTH_DOOR:
                case GO_NORTH_DOOR:
                    easyGUIDconteiner[go->GetEntry()] = go->GetGUID();
                    break;
                case GO_VAULT_OF_FORBIDDEN_TREASURES_1:
                case GO_VAULT_OF_FORBIDDEN_TREASURES_2:
                case GO_VAULT_OF_FORBIDDEN_TREASURES_3:
                case GO_VAULT_OF_FORBIDDEN_TREASURES_4:
                case GO_VAULT_OF_FORBIDDEN_TREASURES_5:
                case GO_VAULT_OF_FORBIDDEN_TREASURES_6:
                case GO_VAULT_OF_FORBIDDEN_TREASURES_7:
                    chestShaVaultOfForbiddenTreasures = go->GetGUID();
                    break;

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
                case GO_LIGHT_RAY_01:
                case GO_LIGHT_RAY_02:
                case GO_LIGHT_RAY_03:
                case GO_LIGHT_RAY_04:
                case GO_LIGHT_RAY_05:
                case GO_LIGHT_RAY_06:
                case GO_LIGHT_RAY_07:
                case GO_LIGHT_RAY_08:
                case GO_LIGHT_RAY_09:
                case GO_LIGHT_RAY_10:
                case GO_LIGHT_RAY_11:
                case GO_LIGHT_RAY_12:
                case GO_LIGHT_RAY_13:
                case GO_LIGHT_RAY_14:
                case GO_LIGHT_RAY_15:
                case GO_LIGHT_RAY_16:
                    go->setIgnorePhaseIdCheck(true);
                    lightqGUIDs.push_back(go->GetGUID());
                    break;
                case GO_SHA_ENERGY_WALL:
                    easyGUIDconteiner[go->GetEntry()] = go->GetGUID();
                    if (EventfieldOfSha >= 3)
                        HandleGameObject(go->GetGUID(), true, go);
                    break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            //Privent overwrite state.
            if (GetBossState(id) == DONE)
                return false;

            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
            case DATA_IMMERSEUS:
                if (state == DONE)
                {
                    if (Creature* bq = instance->GetCreature(LorewalkerChoGUIDtmp))
                        bq->AI()->SetData(DATA_IMMERSEUS, DONE);
                }
                break;
            case DATA_F_PROTECTORS:
            {
                if (state == DONE)
                {
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
                        for (std::vector<uint64>::const_iterator guid = lightqGUIDs.begin(); guid != lightqGUIDs.end(); guid++)
                            HandleGameObject(*guid, true);
                        break;
                    case IN_PROGRESS:
                        for (std::vector<uint64>::const_iterator guid = lightqGUIDs.begin(); guid != lightqGUIDs.end(); guid++)
                            HandleGameObject(*guid, false);
                        break;
                    case DONE:
                        for (std::vector<uint64>::const_iterator guid = lightqGUIDs.begin(); guid != lightqGUIDs.end(); guid++)
                            HandleGameObject(*guid, true);
                        if (Creature* norush = instance->GetCreature(GetData64(NPC_NORUSHEN)))
                            norush->DespawnOrUnsummon();
                        if (Creature* bq = instance->GetCreature(LorewalkerChoGUIDtmp))
                            bq->DespawnOrUnsummon();
                        break;
                    }
                }
                break;
            case DATA_SHA_OF_PRIDE:
                if(state == DONE)
                {
                    if (GameObject* pChest = instance->GetGameObject(chestShaVaultOfForbiddenTreasures))
                        pChest->SetRespawnTime(pChest->GetRespawnDelay());
                }
                break;
            case DATA_GALAKRAS:
                {
                    switch (state)
                    {
                        case NOT_STARTED:
                            SetData(DATA_SOUTH_TOWER, NOT_STARTED);
                            SetData(DATA_NORTH_TOWER, NOT_STARTED);
                            if (GameObject* SouthDoor = instance->GetGameObject(GetData64(GO_SOUTH_DOOR)))
                                SouthDoor->SetGoState(GO_STATE_READY);
                            if (GameObject* NorthDoor = instance->GetGameObject(GetData64(GO_NORTH_DOOR)))
                                NorthDoor->SetGoState(GO_STATE_READY);
                            if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                            {
                                Galakras->AI()->Reset();
                                Galakras->AI()->EnterEvadeMode();
                            }
                            break;
                        case IN_PROGRESS:
                            if (Creature* JainaOrSylvana = instance->GetCreature(JainaOrSylvanaGUID))
                                JainaOrSylvana->AI()->DoAction(ACTION_FRIENDLY_BOSS);
                            if (Creature* VereesOrAethas = instance->GetCreature(VereesaOrAethasGUID))
                                VereesOrAethas->AI()->DoAction(ACTION_FRIENDLY_BOSS);
                            break;
                        case DONE:
                            break;
                    }
                }
                break;
            }

            if (state == DONE)
                DoSummoneEventCreatures();
            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_FIELD_OF_SHA)
            {
                ++EventfieldOfSha;
                if (EventfieldOfSha >= 3)
                {
                    HandleGameObject(GetData64(GO_SHA_ENERGY_WALL), true);
                    SaveToDB();
                }
            }else if (type == DATA_FP_EVADE)
            {
                //for(uint32 i = 0; i < 6; ++i)
                //    if (Creature* me = instance->GetCreature(npcEmbodiesGUID[i]))
                //        me->AI()->EnterEvadeMode();
                for (uint32 i = 0; i < 3; ++i)
                {
                    if (Creature* me = instance->GetCreature(fpGUID[i]))
                        me->AI()->EnterEvadeMode();
                }
            }else if (type == DATA_SHA_PRE_EVENT)
            {
                for(std::set<uint64>::iterator itr =  shaSlgGUID.begin(); itr != shaSlgGUID.end(); ++itr)
                {
                    if (Creature* slg = instance->GetCreature(*itr))
                    {
                        if (data == IN_PROGRESS) slg->AddAura(SPELL_SHA_VORTEX, slg);
                        else slg->RemoveAura(SPELL_SHA_VORTEX);
                    }
                }
            }
            switch (type)
            {
                case DATA_GALAKRAS_PRE_EVENT:
                {
                    switch (data)
                    {
                        case IN_PROGRESS:
                            ShowCannon = data;
                            DoUpdateWorldState(WS_SHOW_KORKRON_CANNON, ShowCannon);
                            break;
                        case DONE:
                            ShowCannon = data;
                            DoUpdateWorldState(WS_SHOW_KORKRON_CANNON, 0);
                            DoUpdateWorldState(WS_KORKRON_CANNON_COUNT, 0);
                            if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                                Galakras->AI()->DoAction(ACTION_PRE_EVENT_FINISH);
                            break;
                    }
                    break;
                }
                case DATA_GALAKRAS_PRE_EVENT_COUNT:
                {
                    CannonCount = data;
                    DoUpdateWorldState(WS_KORKRON_CANNON_COUNT, CannonCount);
                    
                    if (CannonCount > 7)
                        CannonCount = 7;
                    
                    if (CannonCount == 0)
                        SetData(DATA_GALAKRAS_PRE_EVENT, DONE);
                    break;
                }
                case DATA_GALAKRAS:
                {
                    if (data == DONE)
                    {
                        if (TeamInInstance == HORDE)
                            Events.ScheduleEvent(EVENT_FINISH_1_H, 3000);
                        else
                            Events.ScheduleEvent(EVENT_FINISH_1_A, 3000);
                    }
                    break;
                }
                case DATA_SOUTH_TOWER:
                {
                    switch (data)
                    {
                        case IN_PROGRESS:
                            ShowSouthTower = data;
                            DoUpdateWorldState(WS_SHOW_SOUTH_TOWER, 1);
                            if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                                Galakras->AI()->DoAction(ACTION_GRUNT_SOUTH);
                            break;
                        case NOT_STARTED:
                            ShowSouthTower = data;
                            DoUpdateWorldState(WS_SHOW_SOUTH_TOWER, 0);
                            DoUpdateWorldState(WS_SHOW_CAPTURE_SOUTH_TOWER, 0);
                            DoUpdateWorldState(WS_SOUTH_TOWER, SouthTowerCount = 0);
                            break;
                        case SPECIAL:
                            ShowSouthTower = data;
                            DoUpdateWorldState(WS_SHOW_SOUTH_TOWER, 0);
                            DoUpdateWorldState(WS_SHOW_CAPTURE_SOUTH_TOWER, 1);
                            if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                                Galakras->AI()->DoAction(ACTION_GRUNT_SOUTH_FINISH);
                            break;
                    }
                    break;
                }
                case DATA_NORTH_TOWER:
                {
                    switch (data)
                    {
                        case IN_PROGRESS:
                            ShowNorthTower = data;
                            DoUpdateWorldState(WS_SHOW_NORTH_TOWER, 1);
                            if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                                Galakras->AI()->DoAction(ACTION_GRUNT_NORTH);
                            break;
                        case NOT_STARTED:
                            ShowNorthTower = data;
                            DoUpdateWorldState(WS_SHOW_NORTH_TOWER, 0);
                            DoUpdateWorldState(WS_SHOW_CAPTURE_NORTH_TOWER, 0);
                            DoUpdateWorldState(WS_NORTH_TOWER, NorthTowerCount = 0);
                            break;
                        case SPECIAL:
                            ShowNorthTower = data;
                            DoUpdateWorldState(WS_SHOW_NORTH_TOWER, 0);
                            DoUpdateWorldState(WS_SHOW_CAPTURE_NORTH_TOWER, 1);
                            if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                                Galakras->AI()->DoAction(ACTION_GRUNT_NORTH_FINISH);
                            break;
                    }
                    break;
                }
                case DATA_SOUTH_COUNT:
                {
                    SouthTowerCount = data;
                    DoUpdateWorldState(WS_SOUTH_TOWER, SouthTowerCount);
                    DoUpdateWorldState(WS_CAPTURE_SOUTH_TOWER, SouthTowerCount);

                    if (SouthTowerCount >= 100)
                    {
                        if (GameObject* SouthDoor = instance->GetGameObject(GetData64(GO_SOUTH_DOOR)))
                            SouthDoor->SetGoState(GO_STATE_ACTIVE);
                        if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                            Galakras->AI()->DoAction(ACTION_DEMOLITIONS_NORTH);
                        if (Creature* STower = instance->GetCreature(GetData64(NPC_TOWER_SOUTH)))
                            STower->AI()->DoAction(ACTION_TOWER_GUARDS);
                        if (Creature* sDemo = instance->GetCreature(sExpertGUID))
                            sDemo->AI()->DoAction(ACTION_DEMOLITIONS_COMPLETE);
                        SetData(DATA_SOUTH_TOWER, SPECIAL);
                        SetData(DATA_NORTH_TOWER, IN_PROGRESS);
                    }
                    if (SouthTowerCount == 0)
                    {
                        SetData(DATA_SOUTH_TOWER, NOT_STARTED);
                        if (Creature* STower = instance->GetCreature(GetData64(NPC_TOWER_SOUTH)))
                            STower->AI()->DoAction(ACTION_TOWER_TURRET);
                    }
                    break;
                }
                case DATA_NORTH_COUNT:
                {
                    NorthTowerCount = data;
                    DoUpdateWorldState(WS_NORTH_TOWER, NorthTowerCount);
                    DoUpdateWorldState(WS_CAPTURE_NORTH_TOWER, NorthTowerCount);

                    if (NorthTowerCount >= 100)
                    {
                        if (GameObject* NorthDoor = instance->GetGameObject(GetData64(GO_NORTH_DOOR)))
                            NorthDoor->SetGoState(GO_STATE_ACTIVE);
                        if (Creature* NTower = instance->GetCreature(GetData64(NPC_TOWER_NORTH)))
                            NTower->AI()->DoAction(ACTION_TOWER_GUARDS);
                        if (Creature* nDemo = instance->GetCreature(nExpertGUID))
                            nDemo->AI()->DoAction(ACTION_DEMOLITIONS_COMPLETE);
                        SetData(DATA_NORTH_TOWER, SPECIAL);
                    }
                    if (NorthTowerCount == 0)
                    {
                        SetData(DATA_NORTH_TOWER, NOT_STARTED);
                        if (Creature* NTower = instance->GetCreature(GetData64(NPC_TOWER_NORTH)))
                            NTower->AI()->DoAction(ACTION_TOWER_TURRET);
                    }
                    break;
                }
                break;
            }
        }

        uint32 GetData(uint32 type)
        {
            switch (type)
            {
                case DATA_TEAM_IN_INSTANCE:
                    if (!_teamInInstance)
                    {
                        Map::PlayerList const& players = instance->GetPlayers();
                        if (!players.isEmpty())
                            if (Player* player = players.begin()->getSource())
                                _teamInInstance = player->GetTeam();
                    }
                    return _teamInInstance;

                case DATA_GALAKRAS_PRE_EVENT:
                    return ShowCannon;
                case DATA_GALAKRAS_PRE_EVENT_COUNT:
                    return CannonCount;
                case DATA_SOUTH_TOWER:
                    return ShowSouthTower;
                case DATA_SOUTH_COUNT:
                    return SouthTowerCount;
                case DATA_NORTH_TOWER:
                    return ShowNorthTower;
                case DATA_NORTH_COUNT:
                    return NorthTowerCount;
            }
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                //Fallen Protectors
                case NPC_ROOK_STONETOE: 
                    return fpGUID[0];
                case NPC_SUN_TENDERHEART:
                    return fpGUID[1];
                case NPC_HE_SOFTFOOT:
                    return fpGUID[2];
                //Galakras
                case DATA_JAINA_OR_SYLVANA:
                    return JainaOrSylvanaGUID;
                case DATA_VEREESA_OR_AETHAS:
                    return VereesaOrAethasGUID;
                case DATA_DEMOLITIONS_EXPERT_S:
                    return sExpertGUID;
                case DATA_DEMOLITIONS_EXPERT_N:
                    return nExpertGUID;
                //Paragons of the Klaxxi
                case NPC_LOREWALKER_CHO:
                case NPC_LOREWALKER_CHO3:
                    return LorewalkerChoGUIDtmp;
            }

            std::map<uint32, uint64>::iterator itr = easyGUIDconteiner.find(type);
            if (itr != easyGUIDconteiner.end())
                return itr->second;

            return 0;
        }

        void CreatureDies(Creature* creature, Unit* /*killer*/)
        {
            switch(creature->GetEntry())
            {
                case NPC_ZEAL:
                case NPC_ARROGANCE:
                case NPC_VANITY:
                    SetData(DATA_FIELD_OF_SHA, true);
                    break;
                case NPC_LINGERING_CORRUPTION:
                    --lingering_corruption_count;
                    if (!lingering_corruption_count)
                    {
                        if (Creature* Norushen = instance->GetCreature(GetData64(NPC_SHA_NORUSHEN)))
                            Norushen->AI()->SetData(NPC_LINGERING_CORRUPTION, DONE);
                    }
                    break;
            }
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
            saveStream << "S O " << GetBossSaveData() << " " << EventfieldOfSha;
            return saveStream.str();
        }

        void Load(const char* data)
        {
            if (!data)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(data);

            char dataHead1, dataHead2;

            std::istringstream loadStream(data);
            loadStream >> dataHead1 >> dataHead2;

            if (dataHead1 == 'S' && dataHead2 == 'O')
            {
                for (uint32 i = 0; i < DATA_MAX; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;
                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;
                    SetBossState(i, EncounterState(tmpState));
                }
                loadStream >> EventfieldOfSha;
            }
            else
                OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        
        bool CheckRequiredBosses(uint32 bossId, uint32 entry, Player const* player = NULL) const
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

            void Update(uint32 diff)
            {
                Events.Update(diff);

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Galakras finish event. Horde
                        case EVENT_FINISH_1_H:
                            if (Creature* Lorthemar = instance->GetCreature(GetData64(NPC_LORTHEMAR_THERON_H)))
                                Lorthemar->AI()->Talk(7);
                            Events.ScheduleEvent(EVENT_FINISH_2_H, 2000);
                            break;
                        case EVENT_FINISH_2_H:
                            if (Creature* Sylvana = instance->GetCreature(GetData64(NPC_LADY_SYLVANAS_WINDRUNNER_H)))
                                Sylvana->AI()->Talk(5);
                            Events.ScheduleEvent(EVENT_FINISH_3_H, 4000);
                            break;
                        case EVENT_FINISH_3_H:
                            if (Creature* Lorthemar = instance->GetCreature(GetData64(NPC_LORTHEMAR_THERON_H)))
                                Lorthemar->AI()->Talk(8);
                            break;
                        // Galakras finish event. Alliance
                        case EVENT_FINISH_1_A:
                            if (Creature* Jaina = instance->GetCreature(GetData64(NPC_LADY_JAINA_PROUDMOORE_A)))
                                Jaina->AI()->Talk(5);
                            Events.ScheduleEvent(EVENT_FINISH_2_A, 2000);
                            break;
                        case EVENT_FINISH_2_A:
                            if (Creature* Varian = instance->GetCreature(GetData64(NPC_KING_VARIAN_WRYNN_A)))
                                Varian->AI()->Talk(7);
                            Events.ScheduleEvent(EVENT_FINISH_3_A, 4000);
                            break;
                        case EVENT_FINISH_3_A:
                            if (Creature* Jaina = instance->GetCreature(GetData64(NPC_LADY_JAINA_PROUDMOORE_A)))
                                Jaina->AI()->Talk(6);
                            break;
                    }
                }
            }

            Transport* CreateTransport(uint32 goEntry, uint32 period)
            {
                Transport* t = new Transport(period, 0);

                GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(goEntry);
                if (!goinfo)
                {
                    sLog->outError(LOG_FILTER_SQL, "Transport ID: %u will not be loaded, gameobject_template missing", goEntry);
                    delete t;
                    return NULL;
                }

                std::set<uint32> mapsUsed;
                if (!t->GenerateWaypoints(goinfo->moTransport.taxiPathId, mapsUsed))
                    // skip transports with empty waypoints list
                {
                    sLog->outError(LOG_FILTER_SQL, "Transport (path id %u) path size = 0. Transport ignored, check DBC files or transport GO data0 field.", goinfo->moTransport.taxiPathId);
                    delete t;
                    return NULL;
                }

                uint32 mapid = t->m_WayPoints[0].mapid;
                float x = t->m_WayPoints[0].x;
                float y = t->m_WayPoints[0].y;
                float z = t->m_WayPoints[0].z;
                float o = 1;

                // creates the Gameobject
                if (!t->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_MO_TRANSPORT), goEntry, mapid, x, y, z, o, 255, 0))
                {
                    delete t;
                    return NULL;
                }

                //If we someday decide to use the grid to track transports, here:
                t->SetMap(instance);

                //for (uint8 i = 0; i < 5; ++i)
                //    t->AddNPCPassenger(0, (goEntry == GO_HORDE_GUNSHIP ? NPC_HORDE_GUNSHIP_CANNON : NPC_ALLIANCE_GUNSHIP_CANNON), (goEntry == GO_HORDE_GUNSHIP ? hordeGunshipPassengers[i].GetPositionX() : allianceGunshipPassengers[i].GetPositionX()), (goEntry == GO_HORDE_GUNSHIP ? hordeGunshipPassengers[i].GetPositionY() : allianceGunshipPassengers[i].GetPositionY()), (goEntry == GO_HORDE_GUNSHIP ? hordeGunshipPassengers[i].GetPositionZ() : allianceGunshipPassengers[i].GetPositionZ()), (goEntry == GO_HORDE_GUNSHIP ? hordeGunshipPassengers[i].GetOrientation() : allianceGunshipPassengers[i].GetOrientation()));

                return t;
            }

            void SendTransportInit(Player* player)
            {
                if (!transport)
                    return;

                UpdateData transData(player->GetMapId());
                transport->BuildCreateUpdateBlockForPlayer(&transData, player);

                WorldPacket packet;

                transData.BuildPacket(&packet);
                player->GetSession()->SendPacket(&packet);
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
