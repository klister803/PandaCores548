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

uint32 bonusklaxxientry[6] =
{
    NPC_KAROZ,
    NPC_KORVEN,
    NPC_IYYOKYK,
    NPC_XARIL,
    NPC_KAZTIK,
    NPC_KILRUK,
};

uint32 removelist[29] =
{
    SPELL_HEWN,
    SPELL_ANGEL_OF_DEATH,
    SPELL_REAVE_PL,
    SPELL_PLAYER_REAVE,
    SPELL_COMPOUND_EYE,
    SPELL_SNIPE,
    SPELL_MAD_SCIENTIST,
    SPELL_GENETIC_ALTERATION,
    SPELL_GENE_SPLICE,
    SPELL_GENE_SPLICE_PLAYER,
    SPELL_TOXIN_RED,
    SPELL_TOXIN_BLUE,
    SPELL_TOXIN_YELLOW,
    SPELL_TOXIN_ORANGE,
    SPELL_TOXIN_PURPLE,
    SPELL_TOXIN_GREEN,
    SPELL_APOTHECARIAL_KNOWLEDGE,
    SPELL_VAST_APOTHECARIAL_KNOWLEDGE,
    SPELL_APOTHECARY_VOLATILE_POULTICE,
    SPELL_VOLATILE_POULTICE,
    SPELL_EXPOSED_VEINS,
    SPELL_TENDERIZING_STRIKES_DMG,
    SPELL_GENETIC_ALTERATION,
    SPELL_HUNGER,
    SPELL_INJECTION,
    SPELL_CAUSTIC_AMBER_AURA_DMG,
    SPELL_AIM_PLAYER,
    SPELL_MASTER_OF_AMBER,
    SPELL_MASTER_OF_AMBER2,
};

Position const Sha_of_pride_Norushe  = {797.357f, 880.5637f, 371.1606f, 1.786108f };

//Active Weapon Point Positions(Blackfuse)
Position spawnaweaponpos[3] =
{
    {1915.95f, -5711.31f, -302.8854f},
    {1924.21f, -5723.62f, -302.8854f},
    {1933.46f, -5736.51f, -302.8854f},
};

Position destapos[3] =
{
    {1866.87f, -5637.11f, -302.8854f},
    {1874.14f, -5649.20f, -302.8854f},
    {1882.69f, -5661.84f, -302.8854f},
};

DoorData const doorData[] =
{
    {GO_IMMERSEUS_EX_DOOR,                   DATA_IMMERSEUS,              DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_SHA_FIELD,                           DATA_F_PROTECTORS,           DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_NORUSHEN_EX_DOOR,                    DATA_SHA_OF_PRIDE,           DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_ORGRIMMAR_GATE,                      DATA_IRON_JUGGERNAUT,        DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_RUSTY_BARS,                          DATA_KORKRON_D_SHAMAN,       DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_NAZGRIM_EX_DOOR,                     DATA_GENERAL_NAZGRIM,        DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_PRE_ENT_KLAXXI_DOOR,                 DATA_THOK,                   DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GO_SP_EX_DOOR,                          DATA_SPOILS_OF_PANDARIA,     DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {0,                                      0,                           DOOR_TYPE_ROOM,       BOUNDARY_NONE},
};

static uint32 _wavearray[6][4] =
{
    { 0, NPC_ACTIVATED_MISSILE_TURRET, NPC_BLACKFUSE_CRAWLER_MINE, NPC_ACTIVATED_LASER_TURRET },
    { 1, NPC_BLACKFUSE_CRAWLER_MINE, NPC_ACTIVATED_LASER_TURRET, NPC_ACTIVATED_MISSILE_TURRET },
    { 2, NPC_ACTIVATED_LASER_TURRET, NPC_ACTIVATED_ELECTROMAGNET, NPC_ACTIVATED_MISSILE_TURRET },
    { 3, NPC_ACTIVATED_LASER_TURRET, NPC_ACTIVATED_MISSILE_TURRET, NPC_BLACKFUSE_CRAWLER_MINE },
    { 4, NPC_ACTIVATED_MISSILE_TURRET, NPC_ACTIVATED_ELECTROMAGNET, NPC_BLACKFUSE_CRAWLER_MINE },
    { 5, NPC_BLACKFUSE_CRAWLER_MINE, NPC_ACTIVATED_LASER_TURRET, NPC_BLACKFUSE_CRAWLER_MINE },
};

uint32 _toxinlist[6] =
{
    SPELL_TOXIN_BLUE,
    SPELL_TOXIN_RED,
    SPELL_TOXIN_YELLOW,
    SPELL_TOXIN_ORANGE,
    SPELL_TOXIN_PURPLE,
    SPELL_TOXIN_GREEN,
};

class instance_siege_of_orgrimmar : public InstanceMapScript
{
public:
    instance_siege_of_orgrimmar() : InstanceMapScript("instance_siege_of_orgrimmar", 1136) { }

    struct instance_siege_of_orgrimmar_InstanceMapScript : public InstanceScript
    {
        instance_siege_of_orgrimmar_InstanceMapScript(Map* map) : InstanceScript(map) {}

        std::map<uint32, uint64> easyGUIDconteiner;
        //count killed klaxxi
        uint8 klaxxidiecount;
        //count weapons finish move
        uint8 weaponsdone;
        uint8 crawlerminenum;
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
        uint64 winddoorGuid;
        uint64 orgrimmargateGuid;
        uint64 orgrimmargate2Guid;
        uint64 rustybarsGuid;
        uint64 nazgrimdoorGuid;
        uint64 nazgrimexdoorGuid;
        std::vector<uint64> malkorokfenchGuids;
        std::vector<uint64> roomgateGuids;
        std::vector<uint64> roomdoorGuids;
        std::vector<uint64> irondoorGuids;
        std::vector<uint64> leverGuids;
        std::vector<uint64> sopboxGuids;
        std::vector<uint64> spoilsGuids;  //for send frames
        std::vector<uint64> spoils2Guids; //find players and send aura bar in radius
        uint64 gossopsGuid;
        uint64 gonsopsGuid;
        uint64 spentdoorGuid;
        uint64 klaxxientdoorGuid;
        uint64 spexdoorGuid;
        uint64 thokentdoorGuid;
        std::vector<uint64> jaillistGuids;
        std::vector<uint64> klaxxiarenagateGuid;
        
        //Creature
        std::set<uint64> shaSlgGUID;
        std::vector<uint64> PortalOrgrimmarGUID;
        std::vector<uint64> MeasureGUID;
        uint64 LorewalkerChoGUIDtmp;
        uint64 fpGUID[3];
        uint64 WrynOrLorthemarGUID;
        uint64 JainaOrSylvanaGUID;
        uint64 VereesaOrAethasGUID;
        uint64 sExpertGUID;
        uint64 nExpertGUID;
        uint64 ironjuggGuid;
        uint64 kardrisGuid;
        uint64 harommGuid;
        uint64 bloodclawGuid;
        uint64 darkfangGuid;
        uint64 gnazgrimGuid;
        uint64 amGuid;
        uint64 npcssopsGuid;
        std::vector<uint64> npcleverlistGuids;
        std::vector<uint64> klaxxilist; //all klaxxi
        uint64 amberpieceGuid;
        uint64 klaxxicontrollerGuid;
        uint64 thokGuid;
        std::vector<uint64> prisonerGuids;
        std::vector<uint64> dweaponGuids;
        std::vector<uint32> aweaponentry;
        uint64 bsGuid;
        uint64 blackfuseGuid;
        std::vector<uint64> crawlermineGuids;
        uint64 swmstalkerGuid;

        EventMap Events;

        bool onInitEnterState;
        
        bool STowerFull;
        bool NTowerFull;
        bool STowerNull;
        bool NTowerNull;

        Transport* transport;

        ~instance_siege_of_orgrimmar_InstanceMapScript()
        {
            delete transport;
        }

        void Initialize()
        {
            SetBossNumber(DATA_MAX);
            LoadDoorData(doorData);

            klaxxidiecount = 0;
            weaponsdone = 0;
            TeamInInstance = 0;
            _teamInInstance = 0;
            lingering_corruption_count = 0;
            crawlerminenum = 0;

            //GameObject
            immerseusexdoorGUID     = 0;
            chestShaVaultOfForbiddenTreasures = 0;
            lightqGUIDs.clear();
            winddoorGuid            = 0;
            orgrimmargateGuid       = 0;
            orgrimmargate2Guid      = 0;
            rustybarsGuid           = 0;
            nazgrimdoorGuid         = 0;
            nazgrimexdoorGuid       = 0;
            malkorokfenchGuids.clear();
            roomgateGuids.clear();
            roomdoorGuids.clear();
            irondoorGuids.clear();
            leverGuids.clear();
            sopboxGuids.clear();
            spoilsGuids.clear();
            spoils2Guids.clear();
            gossopsGuid             = 0;
            gonsopsGuid             = 0;
            klaxxientdoorGuid       = 0;
            spentdoorGuid           = 0;
            spexdoorGuid            = 0;
            thokentdoorGuid         = 0;
            jaillistGuids.clear();
            klaxxiarenagateGuid.clear();
           
            //Creature
            LorewalkerChoGUIDtmp    = 0;
            memset(fpGUID, 0, 3 * sizeof(uint64));
            EventfieldOfSha     = 0;
            WrynOrLorthemarGUID     = 0;
            JainaOrSylvanaGUID      = 0;
            VereesaOrAethasGUID     = 0;
            sExpertGUID             = 0;
            nExpertGUID             = 0;
            ironjuggGuid            = 0;
            kardrisGuid             = 0;
            harommGuid              = 0;
            bloodclawGuid           = 0;
            darkfangGuid            = 0;
            gnazgrimGuid            = 0;
            amGuid                  = 0;
            npcssopsGuid            = 0;
            npcleverlistGuids.clear();
            klaxxilist.clear();
            amberpieceGuid          = 0;
            klaxxicontrollerGuid    = 0;
            thokGuid                = 0;
            prisonerGuids.clear();
            bsGuid                  = 0;
            dweaponGuids.clear();
            aweaponentry.clear();
            blackfuseGuid           = 0;
            crawlermineGuids.clear();
            swmstalkerGuid = 0;

            onInitEnterState = false;
            STowerFull = false;
            NTowerFull = false;
            STowerNull = false;
            NTowerNull = false;
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
            data << uint32(ShowSouthTower == IN_PROGRESS)    << uint32(WS_SHOW_SOUTH_TOWER);
            data << uint32(ShowNorthTower == IN_PROGRESS)    << uint32(WS_SHOW_NORTH_TOWER);
            data << uint32(ShowSouthTower == SPECIAL)        << uint32(WS_SHOW_CAPTURE_SOUTH_TOWER);
            data << uint32(ShowNorthTower == SPECIAL)        << uint32(WS_SHOW_CAPTURE_NORTH_TOWER);
            data << uint32(SouthTowerCount)                  << uint32(WS_SOUTH_TOWER);
            data << uint32(NorthTowerCount)                  << uint32(WS_NORTH_TOWER);
            data << uint32(SouthTowerCount)                  << uint32(WS_CAPTURE_SOUTH_TOWER);
            data << uint32(NorthTowerCount)                  << uint32(WS_CAPTURE_NORTH_TOWER);
            data << uint32(ShowCannon == IN_PROGRESS)        << uint32(WS_SHOW_KORKRON_CANNON);
            data << uint32(CannonCount)                      << uint32(WS_KORKRON_CANNON_COUNT);
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

            //Crash or disconnect when Klaxxi in progress
            for (uint8 n = 0; n < 6; n++)
                player->RemoveAurasDueToSpell(_toxinlist[n]);
        }

        void OnPlayerLeave(Player* player)
        {
            for (uint8 n = 0; n < 6; n++)
                player->RemoveAurasDueToSpell(_toxinlist[n]);
        }

        void RemoveDebuffFromPlayers()
        {
            for (uint8 n = 0; n < 29; ++n)
                DoRemoveAurasDueToSpellOnPlayers(removelist[n]);
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
                case NPC_BLACKFUSE:
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
                case NPC_EMBODIED_ANGUISH_OF_HE:
                case NPC_EMBODIED_DESPERATION_OF_SUN:
                case NPC_EMBODIED_DESPIRE_OF_SUN:
                case NPC_EMBODIED_MISERY_OF_ROOK:
                case NPC_EMBODIED_GLOOM_OF_ROOK:
                case NPC_EMBODIED_SORROW_OF_ROOK:
                    MeasureGUID.push_back(creature->GetGUID());
                    easyGUIDconteiner[creature->GetEntry()] =creature->GetGUID();
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
                    PortalOrgrimmarGUID.push_back(creature->GetGUID());
                    creature->SetVisible((GetBossState(DATA_SHA_OF_PRIDE)==DONE) ? true : false);
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
                case NPC_IRON_JUGGERNAUT:
                    ironjuggGuid = creature->GetGUID();
                    break;
                //Korkron Dark Shamans
                case NPC_WAVEBINDER_KARDRIS:
                    kardrisGuid = creature->GetGUID();
                    break;
                case NPC_EARTHBREAKER_HAROMM:
                    harommGuid = creature->GetGUID();
                    break;
                case NPC_BLOODCLAW:
                    bloodclawGuid = creature->GetGUID();
                    break;
                case NPC_DARKFANG:
                    darkfangGuid = creature->GetGUID();
                    break;
                //
                //General Nazgrim
                case NPC_GENERAL_NAZGRIM:
                    gnazgrimGuid = creature->GetGUID();
                    break;
                //Malkorok
                case NPC_ANCIENT_MIASMA:
                    amGuid = creature->GetGUID();
                    break;
                //Spoils of Pandaria
                case NPC_SSOP_SPOILS:
                    npcssopsGuid = creature->GetGUID();
                    break;
                case NPC_MOGU_SPOILS:
                case NPC_MOGU_SPOILS2:
                case NPC_MANTIS_SPOILS:
                case NPC_MANTIS_SPOILS2:
                    if (uint32(creature->GetPositionZ()) == -271)
                        spoilsGuids.push_back(creature->GetGUID());
                    else
                        spoils2Guids.push_back(creature->GetGUID());
                    break;
                case NPC_LEVER:
                    npcleverlistGuids.push_back(creature->GetGUID());
                    break;
                //Paragons of the Klaxxi
                case NPC_KILRUK:
                case NPC_XARIL:
                case NPC_KAZTIK:
                case NPC_KORVEN:
                case NPC_IYYOKYK:
                case NPC_KAROZ:
                case NPC_SKEER:
                case NPC_RIKKAL:
                case NPC_HISEK:
                    klaxxilist.push_back(creature->GetGUID());
                    break;
                case NPC_AMBER_PIECE:
                    amberpieceGuid = creature->GetGUID();
                    break;
                case NPC_KLAXXI_CONTROLLER:
                    klaxxicontrollerGuid = creature->GetGUID();
                    break;
                //Thok
                case NPC_THOK:
                    thokGuid = creature->GetGUID();
                    break;
                case NPC_BODY_STALKER:
                    bsGuid = creature->GetGUID();
                    break;
                //Prisoners
                case NPC_AKOLIK:
                case NPC_MONTAK:
                case NPC_WATERSPEAKER_GORAI:
                    prisonerGuids.push_back(creature->GetGUID());
                    break;
                //BlackFuse
                case NPC_BLACKFUSE_MAUNT:
                    blackfuseGuid = creature->GetGUID();
                    break;
                case NPC_DISASSEMBLED_CRAWLER_MINE:
                case NPC_DEACTIVATED_LASER_TURRET:
                case NPC_DEACTIVATED_ELECTROMAGNET:
                case NPC_DEACTIVATED_MISSILE_TURRET:
                    dweaponGuids.push_back(creature->GetGUID());
                    break;
                case NPC_BLACKFUSE_CRAWLER_MINE:
                    crawlermineGuids.push_back(creature->GetGUID());
                    break;
                case NPC_SHOCKWAVE_MISSILE_STALKER:
                    swmstalkerGuid = creature->GetGUID();
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
                case GO_WIND_DOOR:
                    winddoorGuid = go->GetGUID();
                    break;
                case GO_ORGRIMMAR_GATE:
                    AddDoor(go, true);
                    orgrimmargateGuid = go->GetGUID();
                    break;
                case GO_ORGRIMMAR_GATE2:
                    orgrimmargate2Guid = go->GetGUID();
                    break;
                case GO_RUSTY_BARS:
                    rustybarsGuid = go->GetGUID();
                    break;
                case GO_NAZGRIM_DOOR:
                    nazgrimdoorGuid = go->GetGUID();
                    break;
                case GO_NAZGRIM_EX_DOOR:
                    AddDoor(go, true);
                    nazgrimexdoorGuid = go->GetGUID();
                    break;
                case GO_MALKOROK_FENCH:
                case GO_MALKOROK_FENCH_2:
                    malkorokfenchGuids.push_back(go->GetGUID());
                    break;
                case GO_ENT_GATE:
                    spentdoorGuid = go->GetGUID();
                    break;
                case GO_SP_EX_DOOR:
                    AddDoor(go, true);
                    spexdoorGuid = go->GetGUID();
                    break;
                //Thok
                case GO_THOK_ENT_DOOR:
                    thokentdoorGuid = go->GetGUID();
                    break;
                case GO_JINUI_JAIL:
                case GO_JINUI_JAIL2:
                case GO_SAUROK_JAIL:
                case GO_SAUROK_JAIL2:
                case GO_YAUNGOLIAN_JAIL:
                case GO_YAUNGOLIAN_JAIL2:
                    jaillistGuids.push_back(go->GetGUID());
                    break;
                //Spoils of pandaria
                case GO_SSOP_SPOILS:
                    if (GetBossState(DATA_SPOILS_OF_PANDARIA) != DONE)
                    {
                        SetBossState(DATA_SPOILS_OF_PANDARIA, NOT_STARTED);
                        gossopsGuid = go->GetGUID();
                    }
                    else if ((GetBossState(DATA_SPOILS_OF_PANDARIA) == DONE))
                        go->Delete();
                    break;
                case GO_NSOP_SPOILS:
                    gonsopsGuid = go->GetGUID();
                    break;
                case GO_SMALL_MOGU_BOX:
                case GO_MEDIUM_MOGU_BOX:
                case GO_BIG_MOGU_BOX:
                case GO_SMALL_MANTIS_BOX:
                case GO_MEDIUM_MANTIS_BOX:
                case GO_BIG_MANTIS_BOX:
                case GO_PANDAREN_RELIC_BOX:
                    sopboxGuids.push_back(go->GetGUID());
                    break;
                case GO_ENT_DOOR_LEFT:
                case GO_ENT_DOOR_RIGHT:
                case GO_EX_DOOR_RIGHT:
                case GO_EX_DOOR_LEFT:
                    roomdoorGuids.push_back(go->GetGUID());
                    break;
                case GO_ROOM_GATE:
                case GO_ROOM_GATE2:
                case GO_ROOM_GATE3:
                case GO_ROOM_GATE4:
                    roomgateGuids.push_back(go->GetGUID());
                    break;
                case GO_IRON_DOOR_R:
                case GO_IRON_DOOR_L:
                    irondoorGuids.push_back(go->GetGUID());
                    break;
                case GO_LEVER_R:
                case GO_LEVER_L:
                    leverGuids.push_back(go->GetGUID());
                    break;
                //Paragons of Klaxxi
                case GO_PRE_ENT_KLAXXI_DOOR:
                    AddDoor(go, true);
                    klaxxientdoorGuid = go->GetGUID();
                    break;
                case GO_ARENA_WALL:
                    klaxxiarenagateGuid.push_back(go->GetGUID());
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
                    if (Creature* bq = instance->GetCreature(LorewalkerChoGUIDtmp))
                        bq->AI()->SetData(DATA_IMMERSEUS, DONE);
                break;
            case DATA_F_PROTECTORS:
            {
                switch (state)
                {
                case NOT_STARTED:
                    for (std::vector<uint64>::iterator itr = MeasureGUID.begin(); itr != MeasureGUID.end(); ++itr)
                        if (Creature* mes = instance->GetCreature(*itr))
                            mes->DespawnOrUnsummon();
                    SetData(DATA_FP_EVADE, true);
                    break;
                case DONE:
                    if (Creature* bq = instance->GetCreature(LorewalkerChoGUIDtmp))
                        bq->AI()->SetData(DATA_F_PROTECTORS, DONE);

                    for (std::vector<uint64>::iterator itr = MeasureGUID.begin(); itr != MeasureGUID.end(); ++itr)
                        if (Creature* mes = instance->GetCreature(*itr))
                            mes->DespawnOrUnsummon();
                    break;
                default:
                    break;
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
                break;
            }
            case DATA_SHA_OF_PRIDE:
                if (state == DONE)
                {
                    if (!instance->IsLfr())
                        if (GameObject* pChest = instance->GetGameObject(chestShaVaultOfForbiddenTreasures))
                            pChest->SetRespawnTime(pChest->GetRespawnDelay());
                    if (GetData(DATA_GALAKRAS_PRE_EVENT) != IN_PROGRESS)
                    {
                        if (Creature* Galakras = instance->GetCreature(GetData64(NPC_GALAKRAS)))
                            Galakras->AI()->DoAction(ACTION_PRE_EVENT);
                        SetData(DATA_GALAKRAS_PRE_EVENT, IN_PROGRESS);
                    }
                }
                break;
            case DATA_GALAKRAS:
            {
                switch (state)
                {
                case NOT_STARTED:
                    SetData(DATA_SOUTH_TOWER, NOT_STARTED);
                    SetData(DATA_NORTH_TOWER, NOT_STARTED);
                    STowerFull = false;
                    STowerNull = false;
                    NTowerFull = false;
                    NTowerNull = false;
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
                default:
                    break;
                }
                break;
            }
            case DATA_IRON_JUGGERNAUT:
            {
                switch (state)
                {
                case NOT_STARTED:
                    if (Creature* ij = instance->GetCreature(ironjuggGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, ij);
                    HandleGameObject(winddoorGuid, true);
                    break;
                case DONE:
                    if (Creature* ij = instance->GetCreature(ironjuggGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, ij);
                    HandleGameObject(winddoorGuid, true);
                    HandleGameObject(orgrimmargateGuid, true);
                    break;
                case IN_PROGRESS:
                    if (Creature* ij = instance->GetCreature(ironjuggGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, ij);
                    HandleGameObject(winddoorGuid, false);
                    break;
                }
                break;
            }
            case DATA_KORKRON_D_SHAMAN:
            {
                switch (state)
                {
                case NOT_STARTED:
                    for (uint32 n = NPC_WAVEBINDER_KARDRIS; n <= NPC_EARTHBREAKER_HAROMM; n++)
                        if (Creature* shaman = instance->GetCreature(GetData64(n)))
                            SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, shaman);
                    HandleGameObject(orgrimmargate2Guid, true);
                    break;
                case DONE:
                    for (uint32 n = NPC_WAVEBINDER_KARDRIS; n <= NPC_EARTHBREAKER_HAROMM; n++)
                        if (Creature* shaman = instance->GetCreature(GetData64(n)))
                            SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, shaman);
                    HandleGameObject(orgrimmargate2Guid, true);
                    HandleGameObject(rustybarsGuid, true);
                    break;
                case IN_PROGRESS:
                    for (uint32 n = NPC_WAVEBINDER_KARDRIS; n <= NPC_EARTHBREAKER_HAROMM; n++)
                        if (Creature* shaman = instance->GetCreature(GetData64(n)))
                            SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, shaman);
                    HandleGameObject(orgrimmargate2Guid, false);
                    break;
                }
                break;
            }
            case DATA_GENERAL_NAZGRIM:
            {
                switch (state)
                {
                case NOT_STARTED:
                    if (Creature* nazgrim = instance->GetCreature(gnazgrimGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, nazgrim);
                    HandleGameObject(nazgrimdoorGuid, true);
                    break;
                case IN_PROGRESS:
                    if (Creature* nazgrim = instance->GetCreature(gnazgrimGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, nazgrim);
                    HandleGameObject(nazgrimdoorGuid, false);
                    break;
                case DONE:
                    if (Creature* nazgrim = instance->GetCreature(gnazgrimGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, nazgrim);
                    HandleGameObject(nazgrimdoorGuid, true);
                    HandleGameObject(nazgrimexdoorGuid, true);
                    break;
                }
                break;
            }
            case DATA_MALKOROK:
            {
                switch (state)
                {
                case NOT_STARTED:
                    for (std::vector<uint64>::const_iterator itr = malkorokfenchGuids.begin(); itr != malkorokfenchGuids.end(); itr++)
                        HandleGameObject(*itr, true);
                    break;
                case IN_PROGRESS:
                    for (std::vector<uint64>::const_iterator itr = malkorokfenchGuids.begin(); itr != malkorokfenchGuids.end(); itr++)
                        HandleGameObject(*itr, false);
                    break;
                case DONE:
                    for (std::vector<uint64>::const_iterator itr = malkorokfenchGuids.begin(); itr != malkorokfenchGuids.end(); itr++)
                        HandleGameObject(*itr, true);
                    break;
                }
                break;
            }
            case DATA_SPOILS_OF_PANDARIA:
            {
                switch (state)
                {
                case NOT_STARTED:
                    //Remove Combat
                    for (std::vector<uint64>::const_iterator itr = npcleverlistGuids.begin(); itr != npcleverlistGuids.end(); itr++)
                        if (Creature* npclever = instance->GetCreature(*itr))
                            npclever->AI()->EnterEvadeMode();
                    //Clear Frames
                    for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            spoil->AI()->DoAction(ACTION_RESET);
                    //Reset Spoils
                    for (std::vector<uint64>::const_iterator itr = spoils2Guids.begin(); itr != spoils2Guids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            spoil->AI()->DoAction(ACTION_RESET);
                    //Reset All levers
                    for (std::vector<uint64>::const_iterator itr = leverGuids.begin(); itr != leverGuids.end(); itr++)
                    {
                        if (GameObject* lever = instance->GetGameObject(*itr))
                        {
                            lever->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                            lever->SetGoState(GO_STATE_READY);
                            lever->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                        }
                    }
                    //Close All Room's Gates
                    for (std::vector<uint64>::const_iterator itr = roomgateGuids.begin(); itr != roomgateGuids.end(); itr++)
                        HandleGameObject(*itr, false);
                    //Close All Room's Doors
                    for (std::vector<uint64>::const_iterator itr = irondoorGuids.begin(); itr != irondoorGuids.end(); itr++)
                        HandleGameObject(*itr, false);
                    //Reset All Boxes
                    for (std::vector<uint64>::const_iterator itr = sopboxGuids.begin(); itr != sopboxGuids.end(); itr++)
                    {
                        if (GameObject* box = instance->GetGameObject(*itr))
                        {
                            box->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                            box->SetGoState(GO_STATE_READY);
                            box->SetLootState(GO_READY);
                            box->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                        }
                    }
                    HandleGameObject(spentdoorGuid, true);
                    //Remove all buffs
                    DoRemoveAurasDueToSpellOnPlayers(146068);
                    DoRemoveAurasDueToSpellOnPlayers(146099);
                    DoRemoveAurasDueToSpellOnPlayers(146141);
                    break;
                case IN_PROGRESS:
                    if (Creature* ssops = instance->GetCreature(npcssopsGuid))
                        ssops->AI()->DoAction(ACTION_SSOPS_IN_PROGRESS);
                    for (std::vector<uint64>::const_iterator itr = npcleverlistGuids.begin(); itr != npcleverlistGuids.end(); itr++)
                        if (Creature* npclever = instance->GetCreature(*itr))
                            npclever->AI()->DoZoneInCombat(npclever, 100.0f);
                    HandleGameObject(spentdoorGuid, false);
                    break;
                case DONE:
                    //Clear Frames
                    for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            spoil->AI()->DoAction(ACTION_RESET);
                    //Reset Spoils
                    for (std::vector<uint64>::const_iterator itr = spoils2Guids.begin(); itr != spoils2Guids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            spoil->AI()->DoAction(ACTION_RESET);
                    //Remove Combat
                    for (std::vector<uint64>::const_iterator itr = npcleverlistGuids.begin(); itr != npcleverlistGuids.end(); itr++)
                        if (Creature* npclever = instance->GetCreature(*itr))
                            npclever->AI()->EnterEvadeMode();
                    //Open Room's Doors
                    for (std::vector<uint64>::const_iterator itr = roomdoorGuids.begin(); itr != roomdoorGuids.end(); itr++)
                        HandleGameObject(*itr, true);
                    
                    if (Creature* ssops = instance->GetCreature(npcssopsGuid))
                        ssops->AI()->DoAction(ACTION_SSOPS_DONE);

                    if (GameObject* _ssops = instance->GetGameObject(gossopsGuid))
                        _ssops->Delete();

                    //Open All Gates (for safe)
                    for (std::vector<uint64>::const_iterator itr = roomgateGuids.begin(); itr != roomgateGuids.end(); itr++)
                        HandleGameObject(*itr, true);

                    HandleGameObject(spentdoorGuid, true);
                    HandleGameObject(spexdoorGuid, true);
                    //Remove all buffs
                    DoRemoveAurasDueToSpellOnPlayers(146068);
                    DoRemoveAurasDueToSpellOnPlayers(146099);
                    DoRemoveAurasDueToSpellOnPlayers(146141);
                    break;
                case SPECIAL: //first room done, start second
                    if (Creature* ssops = instance->GetCreature(npcssopsGuid))
                        ssops->AI()->DoAction(ACTION_SSOPS_SECOND_ROOM);
                    //Open Next Gates In Room
                    for (std::vector<uint64>::const_iterator itr = roomgateGuids.begin(); itr != roomgateGuids.end(); itr++)
                        if (GameObject* gate = instance->GetGameObject(*itr))
                            if (gate->GetEntry() == GO_ROOM_GATE || gate->GetEntry() == GO_ROOM_GATE3)
                                gate->SetGoState(GO_STATE_ACTIVE);
                    //Clear Frames
                    for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            spoil->AI()->DoAction(ACTION_RESET);
                    //Reset Spoils
                    for (std::vector<uint64>::const_iterator itr = spoils2Guids.begin(); itr != spoils2Guids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            spoil->AI()->DoAction(ACTION_RESET);
                    //Send Next Frames
                    for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                        if (Creature* spoil = instance->GetCreature(*itr))
                            if (spoil->GetEntry() == NPC_MOGU_SPOILS || spoil->GetEntry() == NPC_MANTIS_SPOILS)
                                spoil->AI()->DoAction(ACTION_IN_PROGRESS);
                    break;
                }
                break;
            }
            case DATA_KLAXXI:
            {
                switch (state)
                {
                case NOT_STARTED:
                    RemoveDebuffFromPlayers();
                    klaxxidiecount = 0;
                    for (std::vector<uint64>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                    {
                        if (Creature* klaxxi = instance->GetCreature(*itr))
                        {
                            SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, klaxxi);
                            klaxxi->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            klaxxi->StopMoving();
                            klaxxi->GetMotionMaster()->Clear(false);
                            klaxxi->Kill(klaxxi, true);
                            klaxxi->Respawn();
                            klaxxi->GetMotionMaster()->Clear(false);
                            klaxxi->NearTeleportTo(klaxxi->GetHomePosition().GetPositionX(), klaxxi->GetHomePosition().GetPositionY(), klaxxi->GetHomePosition().GetPositionZ(), klaxxi->GetHomePosition().GetOrientation());
                        }
                    }

                    for (std::vector<uint64>::const_iterator itr = klaxxiarenagateGuid.begin(); itr != klaxxiarenagateGuid.end(); itr++)
                        HandleGameObject(*itr, true);

                    if (Creature* kc = instance->GetCreature(klaxxicontrollerGuid))
                        kc->AI()->Reset();

                    if (Creature* ap = instance->GetCreature(amberpieceGuid))
                        ap->AI()->Reset();
                    break;
                case IN_PROGRESS:
                    for (std::vector<uint64>::const_iterator itr = klaxxiarenagateGuid.begin(); itr != klaxxiarenagateGuid.end(); itr++)
                        HandleGameObject(*itr, false);
                    break;
                case DONE:
                    RemoveDebuffFromPlayers();
                    if (Creature* kc = instance->GetCreature(klaxxicontrollerGuid))
                        kc->AI()->DoAction(ACTION_KLAXXI_DONE);

                    for (std::vector<uint64>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                    {
                        if (Creature* klaxxi = instance->GetCreature(*itr))
                        {
                            SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, klaxxi);
                            klaxxi->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        }
                    }

                    for (std::vector<uint64>::const_iterator itr = klaxxiarenagateGuid.begin(); itr != klaxxiarenagateGuid.end(); itr++)
                        HandleGameObject(*itr, true);
                    break;
                }
                break;
            }
            case DATA_THOK:
            {
                switch (state)
                {
                case NOT_STARTED:
                    for (std::vector<uint64>::const_iterator Itr = prisonerGuids.begin(); Itr != prisonerGuids.end(); Itr++)
                    {
                        if (Creature* p = instance->GetCreature(*Itr))
                        {
                            if (!p->isAlive())
                                p->Respawn();
                            p->AI()->DoAction(ACTION_RESET);
                            p->NearTeleportTo(p->GetHomePosition().GetPositionX(), p->GetHomePosition().GetPositionY(), p->GetHomePosition().GetPositionZ(), p->GetHomePosition().GetOrientation());
                        }
                    }
                    for (std::vector<uint64>::const_iterator itr = jaillistGuids.begin(); itr != jaillistGuids.end(); itr++)
                    {
                        if (GameObject* jail = instance->GetGameObject(*itr))
                        {
                            jail->SetGoState(GO_STATE_READY);
                            jail->SetLootState(GO_READY);
                            jail->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                        }
                    }
                    if (Creature* thok = instance->GetCreature(thokGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, thok);
                    HandleGameObject(thokentdoorGuid, true);
                    break;
                case IN_PROGRESS:
                    if (Creature* thok = instance->GetCreature(thokGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, thok);
                    HandleGameObject(thokentdoorGuid, false);
                    break;
                case DONE:
                    if (Creature* thok = instance->GetCreature(thokGuid))
                        SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, thok);
                    HandleGameObject(thokentdoorGuid, true);
                    HandleGameObject(klaxxientdoorGuid, true);
                    break;
                }
                break;
            }
            case DATA_BLACKFUSE:
            {
                switch (state)
                {
                case NOT_STARTED:
                    weaponsdone = 0;
                    break;
                case IN_PROGRESS:
                    crawlerminenum = instance->Is25ManRaid() ? 7 : 3;
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
            switch (type)
            {
            case DATA_FIELD_OF_SHA:
                ++EventfieldOfSha;
                if (EventfieldOfSha >= 3)
                {
                    HandleGameObject(GetData64(GO_SHA_ENERGY_WALL), true);
                    SaveToDB();
                }
                break;
            case DATA_FP_EVADE:
                for (uint32 i = 0; i < 3; ++i)
                    if (Creature* me = instance->GetCreature(fpGUID[i]))
                        me->AI()->EnterEvadeMode();
                break;
            case DATA_SHA_PRE_EVENT:
                for (std::set<uint64>::iterator itr = shaSlgGUID.begin(); itr != shaSlgGUID.end(); ++itr)
                    if (Creature* slg = instance->GetCreature(*itr))
                        if (data == IN_PROGRESS)
                            slg->AddAura(SPELL_SHA_VORTEX, slg);
                        else
                            slg->RemoveAura(SPELL_SHA_VORTEX);
                break;
            case DATA_SHA_OF_PRIDE:
                if (data == DONE)
                    for (std::vector<uint64>::iterator itr = PortalOrgrimmarGUID.begin(); itr != PortalOrgrimmarGUID.end(); ++itr)
                        if (Creature* c = instance->GetCreature(*itr))
                            c->SetVisible(true);
                break;
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
                if (SouthTowerCount < 0)
                    SouthTowerCount = 0;
                DoUpdateWorldState(WS_SOUTH_TOWER, SouthTowerCount);
                DoUpdateWorldState(WS_CAPTURE_SOUTH_TOWER, SouthTowerCount);

                if (SouthTowerCount >= 100 && !STowerFull)
                {
                    STowerFull = true;
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
                if (SouthTowerCount == 0 && !STowerNull)
                {
                    STowerNull = true;
                    SetData(DATA_SOUTH_TOWER, NOT_STARTED);
                    if (Creature* STower = instance->GetCreature(GetData64(NPC_TOWER_SOUTH)))
                        STower->AI()->DoAction(ACTION_TOWER_TURRET);
                }
                break;
            }
            case DATA_NORTH_COUNT:
            {
                NorthTowerCount = data;
                if (NorthTowerCount < 0)
                    NorthTowerCount = 0;
                DoUpdateWorldState(WS_NORTH_TOWER, NorthTowerCount);
                DoUpdateWorldState(WS_CAPTURE_NORTH_TOWER, NorthTowerCount);

                if (NorthTowerCount >= 100 && !NTowerFull)
                {
                    NTowerFull = true;
                    if (GameObject* NorthDoor = instance->GetGameObject(GetData64(GO_NORTH_DOOR)))
                        NorthDoor->SetGoState(GO_STATE_ACTIVE);
                    if (Creature* NTower = instance->GetCreature(GetData64(NPC_TOWER_NORTH)))
                        NTower->AI()->DoAction(ACTION_TOWER_GUARDS);
                    if (Creature* nDemo = instance->GetCreature(nExpertGUID))
                        nDemo->AI()->DoAction(ACTION_DEMOLITIONS_COMPLETE);
                    SetData(DATA_NORTH_TOWER, SPECIAL);
                }
                if (NorthTowerCount == 0 && !NTowerNull)
                {
                    NTowerNull = true;
                    SetData(DATA_NORTH_TOWER, NOT_STARTED);
                    if (Creature* NTower = instance->GetCreature(GetData64(NPC_TOWER_NORTH)))
                        NTower->AI()->DoAction(ACTION_TOWER_TURRET);
                }
                break;
            }
            case DATA_SOP_START:
                //Open Gates In Room
                for (std::vector<uint64>::const_iterator itr = roomgateGuids.begin(); itr != roomgateGuids.end(); itr++)
                    if (GameObject* gate = instance->GetGameObject(*itr))
                        if (gate->GetEntry() == GO_ROOM_GATE2 || gate->GetEntry() == GO_ROOM_GATE4)
                            gate->SetGoState(GO_STATE_ACTIVE);
                //Send Frames
                for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                    if (Creature* spoil = instance->GetCreature(*itr))
                        if (spoil->GetEntry() == NPC_MOGU_SPOILS2 || spoil->GetEntry() == NPC_MANTIS_SPOILS2)
                            spoil->AI()->DoAction(ACTION_IN_PROGRESS);
                break;
            case DATA_KLAXXI_START:
                for (std::vector<uint64>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                {
                    if (Creature* klaxxi = instance->GetCreature(*itr))
                    {
                        klaxxi->CastSpell(klaxxi, 146983, true); //Aura Enrage
                        if (klaxxi->HasAura(143542))
                            klaxxi->AI()->DoAction(ACTION_KLAXXI_IN_PROGRESS);
                    }
                }
                break;
            case DATA_BUFF_NEXT_KLAXXI:
                if (klaxxidiecount < 6)
                    if (Creature* klaxxi = instance->GetCreature(GetData64(bonusklaxxientry[klaxxidiecount])))
                        klaxxi->CastSpell(klaxxi, 143542, true); //Ready to Fight 
                break;
            case DATA_INTRO_NEXT_KLAXXI:
                if (klaxxidiecount < 6)
                    if (Creature* klaxxi = instance->GetCreature(GetData64(bonusklaxxientry[klaxxidiecount])))
                        klaxxi->AI()->DoAction(ACTION_KLAXXI_IN_PROGRESS);
                klaxxidiecount++;
                for (std::vector<uint64>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                    if (Creature* klaxxi = instance->GetCreature(*itr))
                        if (klaxxi->isAlive())
                            klaxxi->CastSpell(klaxxi, 143483, true); //Paragons Purpose Heal
                break;
            case DATA_SAFE_WEAPONS:
                if (!dweaponGuids.empty())
                {
                    uint32 entry = 0;
                    for (std::vector<uint64>::const_iterator itr = dweaponGuids.begin(); itr != dweaponGuids.end(); itr++)
                    {
                        if (Creature* dw = instance->GetCreature(*itr))
                        {
                            if (dw->isAlive())
                            {
                                dw->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                switch (dw->GetEntry())
                                {
                                case NPC_DISASSEMBLED_CRAWLER_MINE:
                                    entry = NPC_BLACKFUSE_CRAWLER_MINE;
                                    break;
                                case NPC_DEACTIVATED_LASER_TURRET:
                                    entry = NPC_ACTIVATED_LASER_TURRET;
                                    break;
                                case NPC_DEACTIVATED_ELECTROMAGNET:
                                    entry = NPC_ACTIVATED_ELECTROMAGNET;
                                    break;
                                case NPC_DEACTIVATED_MISSILE_TURRET:
                                    entry = NPC_ACTIVATED_MISSILE_TURRET;
                                    break;
                                }
                                aweaponentry.push_back(entry);
                                entry = 0;
                            }
                        }
                    }
                    if (Creature* blackfuse = instance->GetCreature(blackfuseGuid))
                        blackfuse->CastSpell(blackfuse, SPELL_PROTECTIVE_FRENZY, true);
                }
                break;
            case DATA_D_WEAPON_IN_DEST_POINT:
                weaponsdone++;
                if (Creature* blackfuse = instance->GetCreature(blackfuseGuid))
                {
                    if (weaponsdone == 2 && !aweaponentry.empty())
                    {
                        for (uint8 n = 0; n < 2; n++)
                        {
                            if (aweaponentry[n] == NPC_BLACKFUSE_CRAWLER_MINE)
                            {
                                for (uint8 b = crawlerminenum; b > 0; b--)
                                {
                                    if (Creature* aw = blackfuse->SummonCreature(aweaponentry[n], spawnaweaponpos[n].GetPositionX() + float(b + 2), spawnaweaponpos[n].GetPositionY() + float(b + 2), spawnaweaponpos[n].GetPositionZ(), 0.0f))
                                    {
                                        aw->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                        aw->GetMotionMaster()->MoveCharge(destapos[n].GetPositionX() + float(b + 2), destapos[n].GetPositionY() + float(b + 2), destapos[n].GetPositionZ(), 10.0f, 1, false);
                                    }
                                }
                            }
                            else
                            {
                                if (Creature* aw = blackfuse->SummonCreature(aweaponentry[n], spawnaweaponpos[n]))
                                {
                                    aw->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                    aw->GetMotionMaster()->MoveCharge(destapos[n].GetPositionX(), destapos[n].GetPositionY(), destapos[n].GetPositionZ(), 10.0f, 1, false);
                                }
                            }
                        }
                        EjectPlayersFromConveyor();
                        weaponsdone = 0;
                    }
                    else if (weaponsdone == 3 && aweaponentry.empty())
                    {
                        blackfuse->CastSpell(blackfuse, SPELL_ENERGIZED_DEFENSIVE_MATRIX, true);
                        uint8 num = (blackfuse->AI()->GetData(DATA_GET_WEAPON_WAVE_INDEX) - 1);
                        for (uint8 n = 1; n < 4; n++)
                        {
                            if (_wavearray[num][n] == NPC_BLACKFUSE_CRAWLER_MINE)
                            {
                                for (uint8 b = crawlerminenum; b > 0; b--)
                                {
                                    if (Creature* weapon = blackfuse->SummonCreature(_wavearray[num][n], spawnaweaponpos[n - 1].GetPositionX() + float(b + 2), spawnaweaponpos[n - 1].GetPositionY() + float(b + 2), spawnaweaponpos[n - 1].GetPositionZ(), 0.0f))
                                    {
                                        weapon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                        weapon->GetMotionMaster()->MoveCharge(destapos[n - 1].GetPositionX() + float(b + 2), destapos[n - 1].GetPositionY() + float(b + 2), destapos[n - 1].GetPositionZ(), 10.0f, 1, false);
                                    }
                                }
                            }
                            else
                            {
                                if (Creature* weapon = blackfuse->SummonCreature(_wavearray[num][n], spawnaweaponpos[n - 1]))
                                {
                                    weapon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                    weapon->GetMotionMaster()->MoveCharge(destapos[n - 1].GetPositionX(), destapos[n - 1].GetPositionY(), destapos[n - 1].GetPositionZ(), 10.0f, 1, false);
                                }
                            }
                        }
                        EjectPlayersFromConveyor();
                        weaponsdone = 0;
                    }
                }
                break;
            case DATA_CRAWLER_MINE_READY:
                crawlerminenum--;
                if (!crawlerminenum)
                {
                    for (uint8 m = 0; m < crawlermineGuids.size(); m++)
                        if (Creature* cm = instance->GetCreature(crawlermineGuids[m]))
                            cm->AI()->SetData(DATA_CRAWLER_MINE_ENTERCOMBAT, uint32(m));
                    crawlermineGuids.clear();
                    crawlerminenum = instance->Is25ManRaid() ? 7 : 3;
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
                case DATA_SEND_KLAXXI_DIE_COUNT:
                    return klaxxidiecount;
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
                case NPC_LOREWALKER_CHO:
                case NPC_LOREWALKER_CHO3:
                    return LorewalkerChoGUIDtmp;
                //Korkron Dark Shaman
                case NPC_WAVEBINDER_KARDRIS:
                    return kardrisGuid;
                case NPC_EARTHBREAKER_HAROMM:
                    return harommGuid;
                case NPC_BLOODCLAW:
                    return bloodclawGuid;
                case NPC_DARKFANG:
                    return darkfangGuid;
                //Malkorok
                case NPC_ANCIENT_MIASMA:
                    return amGuid;
                //Spoils of Pandaria
                case GO_LEVER_R:
                case GO_LEVER_L:
                    for (std::vector<uint64>::const_iterator itr = leverGuids.begin(); itr != leverGuids.end(); itr++)
                        if (GameObject* lever = instance->GetGameObject(*itr))
                            if (lever->GetEntry() == type)
                                return lever->GetGUID();
                case GO_IRON_DOOR_R:
                case GO_IRON_DOOR_L:
                    for (std::vector<uint64>::const_iterator itr = irondoorGuids.begin(); itr != irondoorGuids.end(); itr++)
                        if (GameObject* door = instance->GetGameObject(*itr))
                            if (door->GetEntry() == type)
                                return door->GetGUID();
                case DATA_SPOIL_MANTIS: 
                    for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                    {
                        if (Creature* spoil = instance->GetCreature(*itr))
                        {
                            if (GetBossState(DATA_SPOILS_OF_PANDARIA) == IN_PROGRESS)
                            {
                                if (spoil->GetEntry() == NPC_MANTIS_SPOILS2)
                                    return spoil->GetGUID();
                            }
                            else if (GetBossState(DATA_SPOILS_OF_PANDARIA) == SPECIAL)
                                if (spoil->GetEntry() == NPC_MANTIS_SPOILS)
                                    return spoil->GetGUID();
                        }
                    }
                case DATA_SPOIL_MOGU: 
                    for (std::vector<uint64>::const_iterator itr = spoilsGuids.begin(); itr != spoilsGuids.end(); itr++)
                    {
                        if (Creature* spoil = instance->GetCreature(*itr))
                        {
                            if (GetBossState(DATA_SPOILS_OF_PANDARIA) == IN_PROGRESS)
                            {
                                if (spoil->GetEntry() == NPC_MOGU_SPOILS2)
                                    return spoil->GetGUID();
                            }
                            else if (GetBossState(DATA_SPOILS_OF_PANDARIA) == SPECIAL)
                                if (spoil->GetEntry() == NPC_MOGU_SPOILS)
                                    return spoil->GetGUID();
                        }
                    }
                case NPC_MOGU_SPOILS:    
                case NPC_MOGU_SPOILS2:   
                case NPC_MANTIS_SPOILS:
                case NPC_MANTIS_SPOILS2: 
                    for (std::vector<uint64>::const_iterator itr = spoils2Guids.begin(); itr != spoils2Guids.end(); itr++)
                        if (Creature* spoil2 = instance->GetCreature(*itr))
                            if (spoil2->GetEntry() == type)
                                return spoil2->GetGUID();
                //Mogu
                case GO_SMALL_MOGU_BOX:
                case GO_MEDIUM_MOGU_BOX:
                case GO_BIG_MOGU_BOX:
                    for (std::vector<uint64>::const_iterator itr = spoils2Guids.begin(); itr != spoils2Guids.end(); itr++)
                    {
                        if (Creature* spoil = instance->GetCreature(*itr))
                        {
                            if (GetBossState(DATA_SPOILS_OF_PANDARIA) == IN_PROGRESS)
                            {
                                if (spoil->GetEntry() == NPC_MOGU_SPOILS2)
                                    return spoil->GetGUID();
                            }
                            else if (GetBossState(DATA_SPOILS_OF_PANDARIA) == SPECIAL)
                                if (spoil->GetEntry() == NPC_MOGU_SPOILS)
                                    return spoil->GetGUID();
                        }
                    }
                //Mantis
                case GO_SMALL_MANTIS_BOX:
                case GO_MEDIUM_MANTIS_BOX:
                case GO_BIG_MANTIS_BOX:
                    for (std::vector<uint64>::const_iterator itr = spoils2Guids.begin(); itr != spoils2Guids.end(); itr++)
                    {
                        if (Creature* spoil = instance->GetCreature(*itr))
                        {
                            if (GetBossState(DATA_SPOILS_OF_PANDARIA) == IN_PROGRESS)
                            {
                                if (spoil->GetEntry() == NPC_MANTIS_SPOILS2)
                                    return spoil->GetGUID();
                            }
                            else if (GetBossState(DATA_SPOILS_OF_PANDARIA) == SPECIAL)
                                if (spoil->GetEntry() == NPC_MANTIS_SPOILS)
                                    return spoil->GetGUID();
                        }
                    }
                case NPC_SSOP_SPOILS:
                case GO_PANDAREN_RELIC_BOX:
                    return npcssopsGuid;
                //Paragons of the Klaxxi
                case NPC_KILRUK:
                case NPC_XARIL:
                case NPC_KAZTIK:
                case NPC_KORVEN:
                case NPC_IYYOKYK:
                case NPC_KAROZ:
                case NPC_SKEER:
                case NPC_RIKKAL:
                case NPC_HISEK:
                    for (std::vector<uint64>::const_iterator itr = klaxxilist.begin(); itr != klaxxilist.end(); itr++)
                        if (Creature* klaxxi = instance->GetCreature(*itr))
                            if (klaxxi->GetEntry() == type)
                                return klaxxi->GetGUID();
                case NPC_AMBER_PIECE:
                    return amberpieceGuid;
                case NPC_KLAXXI_CONTROLLER:
                    return klaxxicontrollerGuid;
                //Thok
                case NPC_THOK:
                    return thokGuid;
                case NPC_BODY_STALKER:
                    return bsGuid;
                //Blackfuse
                case NPC_BLACKFUSE_MAUNT:
                    return blackfuseGuid;
                case NPC_SHOCKWAVE_MISSILE_STALKER:
                    return swmstalkerGuid;
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

        void EjectPlayersFromConveyor()
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();
            if (!PlayerList.isEmpty())
            {
                for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
                {
                    if (Player* player = Itr->getSource())
                    {
                        if (player->isAlive() && player->HasAura(SPELL_ON_CONVEYOR))
                        {
                            player->RemoveAurasDueToSpell(SPELL_ON_CONVEYOR);
                            player->GetMotionMaster()->MoveJump(2008.93f, -5600.63f, -309.3268f, 15.0f, 15.0f);
                        }
                    }
                }
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
            if (player && AccountMgr::IsGMAccount(player->GetSession()->GetSecurity()))
                return true;

            switch (bossId)
            {
                case DATA_IMMERSEUS:
                    return true;
                case DATA_F_PROTECTORS:
                    return GetBossState(DATA_IMMERSEUS) == DONE;
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
                if (!t->GenerateWaypoints(goinfo->moTransport.taxiPathID, mapsUsed))
                    // skip transports with empty waypoints list
                {
                    sLog->outError(LOG_FILTER_SQL, "Transport (path id %u) path size = 0. Transport ignored, check DBC files or transport GO data0 field.", goinfo->moTransport.taxiPathID);
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
