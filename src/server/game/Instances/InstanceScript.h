/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_INSTANCE_DATA_H
#define TRINITY_INSTANCE_DATA_H

#include "ZoneScript.h"
#include "World.h"
#include "ObjectMgr.h"
#include "CreatureAIImpl.h"
//#include "GameObject.h"
//#include "Map.h"

#define OUT_SAVE_INST_DATA             TC_LOG_DEBUG("server", "Saving Instance Data for Instance %s (Map %d, Instance Id %d)", instance->GetMapName(), instance->GetId(), instance->GetInstanceId())
#define OUT_SAVE_INST_DATA_COMPLETE    TC_LOG_DEBUG("server", "Saving Instance Data for Instance %s (Map %d, Instance Id %d) completed.", instance->GetMapName(), instance->GetId(), instance->GetInstanceId())
#define OUT_LOAD_INST_DATA(a)          TC_LOG_DEBUG("server", "Loading Instance Data for Instance %s (Map %d, Instance Id %d). Input is '%s'", instance->GetMapName(), instance->GetId(), instance->GetInstanceId(), a)
#define OUT_LOAD_INST_DATA_COMPLETE    TC_LOG_DEBUG("server", "Instance Data Load for Instance %s (Map %d, Instance Id: %d) is complete.", instance->GetMapName(), instance->GetId(), instance->GetInstanceId())
#define OUT_LOAD_INST_DATA_FAIL        TC_LOG_ERROR("server", "Unable to load Instance Data for Instance %s (Map %d, Instance Id: %d).", instance->GetMapName(), instance->GetId(), instance->GetInstanceId())

class Map;
class Unit;
class Player;
class GameObject;
class Creature;

typedef std::set<GameObject*> DoorSet;
typedef std::set<Creature*> MinionSet;

enum EncounterFrameType
{
    ENCOUNTER_FRAME_SET_COMBAT_RES_LIMIT    = 0,
    ENCOUNTER_FRAME_RESET_COMBAT_RES_LIMIT  = 1,
    ENCOUNTER_FRAME_ENGAGE                  = 2,
    ENCOUNTER_FRAME_DISENGAGE               = 3,
    ENCOUNTER_FRAME_UPDATE_PRIORITY         = 4,
    ENCOUNTER_FRAME_ADD_TIMER               = 5,
    ENCOUNTER_FRAME_ENABLE_OBJECTIVE        = 6,
    ENCOUNTER_FRAME_UPDATE_OBJECTIVE        = 7,
    ENCOUNTER_FRAME_DISABLE_OBJECTIVE       = 8,
    ENCOUNTER_FRAME_UNK7                    = 9,    // Seems to have something to do with sorting the encounter units
    ENCOUNTER_FRAME_ADD_COMBAT_RES_LIMIT    = 10,
};

enum EncounterState
{
    NOT_STARTED   = 0,
    IN_PROGRESS   = 1,
    FAIL          = 2,
    DONE          = 3,
    SPECIAL       = 4,
    TO_BE_DECIDED = 5,
};

struct MinionData
{
    uint32 entry, bossId;
};

struct BossInfo
{
    BossInfo() : state(TO_BE_DECIDED) {}
    EncounterState state;
    DoorSet door[MAX_DOOR_TYPES];
    MinionSet minion;
    BossBoundaryMap boundary;
};

struct DoorInfo
{
    explicit DoorInfo(BossInfo* _bossInfo, DoorType _type, BoundaryType _boundary)
        : bossInfo(_bossInfo), type(_type), boundary(_boundary) {}
    BossInfo* bossInfo;
    DoorType type;
    BoundaryType boundary;
};

struct MinionInfo
{
    explicit MinionInfo(BossInfo* _bossInfo) : bossInfo(_bossInfo) {}
    BossInfo* bossInfo;
};

typedef std::multimap<uint32 /*entry*/, DoorInfo> DoorInfoMap;
typedef std::map<uint32 /*entry*/, MinionInfo> MinionInfoMap;

class InstanceScript : public ZoneScript
{
    public:
        explicit InstanceScript(Map* map) : instance(map), completedEncounters(0), challenge_timer(0){ m_type = ZONE_TYPE_INSTANCE; }

        virtual ~InstanceScript() {}

        Map* instance;

        //On creation, NOT load.
        virtual void Initialize() {}

        //On load
        virtual void Load(char const* data) { LoadBossState(data); }

        //When save is needed, this function generates the data
        virtual std::string GetSaveData() { return GetBossSaveData(); }

        void SaveToDB();

        virtual void Update(uint32 /*diff*/);

        //Used by the map's CanEnter function.
        //This is to prevent players from entering during boss encounters.
        virtual bool IsEncounterInProgress() const;
        bool IsInstanceEmpty();
        void ResetEncounters();
        //Called when a player successfully enters the instance.
        virtual void OnPlayerEnter(Player* /*player*/) {}
        virtual void OnPlayerLeave(Player* /*player*/) {}

        virtual void CreatureDies(Creature* /*creature*/, Unit* /*killer*/) {}
        void OnCreatureCreate(Creature* /*creature*/) {}
        void OnCreatureRemove(Creature* /*creature*/) {}
        void OnGameObjectCreate(GameObject* go)
        {
            //TC_LOG_DEBUG("server", "InstanceScript::OnGameObjectCreate go %u", go->GetEntry());
            if(sObjectMgr->GetCreatureAIInstaceGoData(go->GetEntry()))
                AddDoor(go, true);
        }
        void OnGameObjectRemove(GameObject* go)
        {
            //TC_LOG_DEBUG("server", "InstanceScript::OnGameObjectRemove go %u", go->GetEntry());
            if(sObjectMgr->GetCreatureAIInstaceGoData(go->GetEntry()))
                AddDoor(go, false);
        }

        //Handle open / close objects
        //use HandleGameObject(0, boolen, GO); in OnObjectCreate in instance scripts
        //use HandleGameObject(GUID, boolen, NULL); in any other script
        void HandleGameObject(uint64 guid, bool open, GameObject* go = NULL);

        //change active state of doors or buttons
        void DoUseDoorOrButton(uint64 guid, uint32 withRestoreTime = 0, bool useAlternativeState = false);

        //Respawns a GO having negative spawntimesecs in gameobject-table
        void DoRespawnGameObject(uint64 guid, uint32 timeToDespawn = MINUTE);

        //sends world state update to all players in instance
        void DoUpdateWorldState(uint32 worldstateId, uint32 worldstateValue);

        // Send Notify to all players in instance
        void DoSendNotifyToInstance(char const* format, ...);

        // Reset Achievement Criteria
        void DoResetAchievementCriteria(CriteriaTypes type, uint64 miscValue1 = 0, uint64 miscValue2 = 0, bool evenIfCriteriaComplete = false);

        // Complete Achievement for all players in instance
        DECLSPEC_DEPRECATED void DoCompleteAchievement(uint32 achievement) ATTR_DEPRECATED;

        // Update Achievement Criteria for all players in instance
        void DoUpdateAchievementCriteria(CriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, uint32 miscValue3 = 0, Unit* unit = NULL);

        // Start/Stop Timed Achievement Criteria for all players in instance
        void DoStartTimedAchievement(CriteriaTimedTypes type, uint32 entry);
        void DoStopTimedAchievement(CriteriaTimedTypes type, uint32 entry);

        // Remove Auras due to Spell on all players in instance
        void DoRemoveAurasDueToSpellOnPlayers(uint32 spell);

        // Remove aura from stack on all players in instance
        void DoRemoveAuraFromStackOnPlayers(uint32 spell, uint64 casterGUID = 0, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT, uint32 num = 1);

        // Cast spell on all players in instance
        void DoCastSpellOnPlayers(uint32 spell);

        void DoSetAlternatePowerOnPlayers(int32 value);

        void DoNearTeleportPlayers(const Position pos, bool casting = false);
        
        void DoStartMovie(uint32 movieId);

        // Add aura on all players in instance
        void DoAddAuraOnPlayers(uint32 spell);

        // Return wether server allow two side groups or not
        bool ServerAllowsTwoSideGroups() { return sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP); }

        virtual bool SetBossState(uint32 id, EncounterState state);
        EncounterState GetBossState(uint32 id) const { return id < bosses.size() ? bosses[id].state : TO_BE_DECIDED; }
        BossBoundaryMap const* GetBossBoundary(uint32 id) const { return id < bosses.size() ? &bosses[id].boundary : NULL; }

        // Achievement criteria additional requirements check
        // NOTE: not use this if same can be checked existed requirement types from AchievementCriteriaRequirementType
        virtual bool CheckAchievementCriteriaMeet(uint32 /*criteria_id*/, Player const* /*source*/, Unit const* /*target*/ = NULL, uint32 /*miscvalue1*/ = 0);

        // Checks boss requirements (one boss required to kill other)
        virtual bool CheckRequiredBosses(uint32 bossId, uint32 entry, Player const* player = NULL) const
        {
            if(CreatureAIInstance const* aiinstdata = sObjectMgr->GetCreatureAIInstaceData(entry))
            {
                if(aiinstdata->bossidactivete != 0)
                    if (GetBossState(aiinstdata->bossidactivete) != DONE)
                        return false;
            }
            return true;
        }

        virtual bool IsRaidBoss(uint32 creature_entry)
        {
            return false;
        }

        // Checks encounter state at kill/spellcast
        //void UpdateEncounterState(EncounterCreditType type, uint32 creditEntry, Unit* source);

        // Used only during loading
        void SetCompletedEncountersMask(uint32 newMask) { completedEncounters = newMask; }

        // Returns completed encounters mask for packets
        uint32 GetCompletedEncounterMask() const { return completedEncounters; }

        void SetResurectSpell() { ResurectCount++; }

        bool GetResurectSpell()
        {
            if (instance->IsNonRaidDungeon())
                return true;
            if (instance->Is25ManRaid() && ResurectCount < 3)
                return true;
            if (ResurectCount < 1)
                return true;

            return false;
        }

        void SendEncounterUnit(uint32 type, Unit* unit = NULL, uint8 param1 = 0, uint8 param2 = 0);

        // Check if all players are dead (except gamemasters)
        virtual bool IsWipe();

        void RemoveCombatFromPlayers();
        void RemoveSatedAuraFromPlayers();
        void RemoveSelfResFieldFromPlayers();

        virtual void FillInitialWorldStates(WorldPacket& /*data*/) {}

        void UpdatePhasing();
        void SetBossNumber(uint32 number)
        {
            //TC_LOG_DEBUG("server", "InstanceScript::SetBossNumber number %u", number);
            if(bosses.size() < number) bosses.resize(number);
        }
        void LoadDoorDataBase(std::vector<DoorData> const* data);

        void BroadcastPacket(WorldPacket& data) const;

        // Challenge
        void FillInitialWorldTimers(WorldPacket& data);
        void StartChallenge();
        void StopChallenge();
        uint32 GetChallengeProgresTime();
        void SetChallengeProgresInSec(uint32 timer);
        uint32 GetChallengeTime() { return challenge_timer; }

    protected:
        void LoadDoorData(DoorData const* data);
        void LoadMinionData(MinionData const* data);

        void AddDoor(GameObject* door, bool add);
        void AddMinion(Creature* minion, bool add);

        void UpdateDoorState(GameObject* door);
        void UpdateMinionState(Creature* minion, EncounterState state);

        std::string LoadBossState(char const* data);
        std::string GetBossSaveData();
    private:
        std::vector<BossInfo> bosses;
        DoorInfoMap doors;
        MinionInfoMap minions;
        uint32 completedEncounters;         // completed encounter mask, bit indexes are DungeonEncounter.dbc boss numbers, used for packets
        uint32 ResurectCount;
        uint32 challenge_timer;             // time in ms of start challenge.
        EventMap _events;
};
#endif
