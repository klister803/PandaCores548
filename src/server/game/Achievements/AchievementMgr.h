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
#ifndef __TRINITY_ACHIEVEMENTMGR_H
#define __TRINITY_ACHIEVEMENTMGR_H

#include <map>
#include <string>

#include "Common.h"
#include <ace/Singleton.h>
#include "DatabaseEnv.h"
#include "DBCEnums.h"
#include "DBCStores.h"
#include "MapUpdater.h"
#include "LockedMap.h"
#include "LockedQueue.h"

struct CriteriaTreeInfo
{
    uint32 criteriaTreeID = 0;
    uint32 parentID = 0;
    AchievementEntry const* achievement = NULL;
    CriteriaTreeEntry const* criteriaTree = NULL;
    CriteriaEntry const* criteria = NULL;
};

typedef std::vector<CriteriaTreeInfo> CriteriaTreeList;
typedef std::unordered_map<uint32, CriteriaTreeInfo> CriteriaTreeInfoMap;
typedef std::vector<AchievementEntry const*> AchievementEntryList;
typedef std::unordered_map<uint32, AchievementEntryList> AchievementListByReferencedId;

struct CriteriaTreeProgress
{
    CriteriaTreeProgress() : counter(0), CompletedGUID(0), date(0), changed(false), updated(false), completed(false), deactiveted(true)
    , achievement(NULL), criteriaTree(NULL), parent(NULL), criteria(NULL)    { }

    uint32 counter;
    time_t date;                                            // latest update time.
    uint64 CompletedGUID;                                   // GUID of the player that completed this criteria (guild achievements)
    bool changed;
    bool updated;
    bool completed;
    bool deactiveted;

    AchievementEntry const* achievement;
    CriteriaTreeEntry const* criteriaTree;
    CriteriaTreeEntry const* parent;
    CriteriaEntry const* criteria;
};

struct CriteriaProgressTree
{
    CriteriaProgressTree() : completed(false), achievement(NULL), criteriaTree(NULL) { }

    bool completed;
    AchievementEntry const* achievement = NULL;
    CriteriaTreeEntry const* criteriaTree = NULL;
    std::vector<CriteriaProgressTree *> ChildrenTree;
    std::vector<CriteriaTreeProgress const*> ChildrenCriteria;
};

enum AchievementCriteriaDataType
{                                                           // value1         value2        comment
    ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE                = 0, // 0              0
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE          = 1, // creature_id    0
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE = 2, // class_id       race_id
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH= 3, // health_percent 0
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA              = 5, // spell_id       effect_idx
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA              = 6, // area id        0
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA              = 7, // spell_id       effect_idx
    ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE               = 8, // minvalue                     value provided with achievement update must be not less that limit
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL             = 9, // minlevel                     minlevel of target
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER            = 10, // gender                       0=male; 1=female
    ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT              = 11, // scripted requirement
    // REUSE
    ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT    = 13, // count                        "with less than %u people in the zone"
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM              = 14, // team                         HORDE(67), ALLIANCE(469)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK             = 15, // drunken_state  0             (enum DrunkenState) of player
    ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY             = 16, // holiday_id     0             event in holiday time
    ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE  = 17, // min_score      max_score     player's team win bg and opposition team have team score in range
    ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT          = 18, // 0              0             maker instance script call for check current criteria requirements fit
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPED_ITEM      = 19, // item_level     item_quality  for equipped item in slot to check item level and quality
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE = 21, // class_id       race_id
};

#define MAX_ACHIEVEMENT_CRITERIA_DATA_TYPE               22 // maximum value in AchievementCriteriaDataType enum
class Player;
class Unit;
class WorldPacket;

struct AchievementCriteriaData
{
    AchievementCriteriaDataType dataType;
    union
    {
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE              = 0 (no data)
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE        = 1
        struct
        {
            uint32 id;
        } creature;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE = 2
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE = 21
        struct
        {
            uint32 class_id;
            uint32 race_id;
        } classRace;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH = 3
        struct
        {
            uint32 percent;
        } health;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA            = 5
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA            = 7
        struct
        {
            uint32 spell_id;
            uint32 effect_idx;
        } aura;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA            = 6
        struct
        {
            uint32 id;
        } area;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE             = 8
        struct
        {
            uint32 minvalue;
        } value;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL           = 9
        struct
        {
            uint32 minlevel;
        } level;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER          = 10
        struct
        {
            uint32 gender;
        } gender;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT            = 11 (no data)
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT  = 13
        struct
        {
            uint32 maxcount;
        } map_players;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM            = 14
        struct
        {
            uint32 team;
        } team;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK           = 15
        struct
        {
            uint32 state;
        } drunk;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY           = 16
        struct
        {
            uint32 id;
        } holiday;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE= 17
        struct
        {
            uint32 min_score;
            uint32 max_score;
        } bg_loss_team_score;
        // ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT        = 18 (no data)
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPED_ITEM    = 19
        struct
        {
            uint32 item_level;
            uint32 item_quality;
        } equipped_item;
        // raw
        struct
        {
            uint32 value1;
            uint32 value2;
        } raw;
    };
    uint32 ScriptId;

    AchievementCriteriaData() : dataType(ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE)
    {
        raw.value1 = 0;
        raw.value2 = 0;
        ScriptId = 0;
    }

    AchievementCriteriaData(uint32 _dataType, uint32 _value1, uint32 _value2, uint32 _scriptId) : dataType(AchievementCriteriaDataType(_dataType))
    {
        raw.value1 = _value1;
        raw.value2 = _value2;
        ScriptId = _scriptId;
    }

    bool IsValid(CriteriaEntry const* criteria);
    bool Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscvalue1 = 0) const;
};

struct AchievementCriteriaDataSet
{
        AchievementCriteriaDataSet() : criteria_id(0) {}
        typedef std::vector<AchievementCriteriaData> Storage;
        void Add(AchievementCriteriaData const& data) { storage.push_back(data); }
        bool Meets(Player const* source, Unit const* target, uint32 miscvalue = 0) const;
        void SetCriteriaId(uint32 id) {criteria_id = id;}
    private:
        uint32 criteria_id;
        Storage storage;
};

typedef std::unordered_map<uint32, AchievementCriteriaDataSet> AchievementCriteriaDataMap;

struct AchievementReward
{
    uint32 titleId[2];
    uint32 itemId;
    uint32 sender;
    std::string subject;
    std::string text;
    uint32 learnSpell;
    uint32 ScriptId;
};

typedef std::unordered_map<uint32, AchievementReward> AchievementRewards;

struct AchievementRewardLocale
{
    StringVector subject;
    StringVector text;
};

typedef std::unordered_map<uint32, AchievementRewardLocale> AchievementRewardLocales;

struct CompletedAchievementData
{
    CompletedAchievementData() : date(0), first_guid(0), changed(false), isAccountAchievement(false) { }

    time_t date;
    std::set<uint64> guids;
    uint64 first_guid;
    bool changed;
    bool isAccountAchievement;
};

struct AchievementCriteriaUpdateTask
{
    uint64 PlayerGUID;
    uint64 UnitGUID;
    std::function<void(uint64, uint64)> Task;
};

using LockedAchievementCriteriaTaskQueue = ACE_Based::LockedQueue<AchievementCriteriaUpdateTask, ACE_Thread_Mutex>;
using LockedPlayersAchievementCriteriaTask = ACE_Based::LockedMap<uint32, LockedAchievementCriteriaTaskQueue>;

using AchievementCriteriaTaskQueue = std::queue<AchievementCriteriaUpdateTask>;
using PlayersAchievementCriteriaTask = std::map<uint32, AchievementCriteriaTaskQueue>;

typedef std::unordered_map<uint32, CriteriaTreeProgress> CriteriaProgressMap;
typedef std::unordered_map<uint32, CompletedAchievementData> CompletedAchievementMap;
typedef std::unordered_map<uint32, CriteriaProgressMap> AchievementProgressMap;
typedef std::unordered_map<uint32, CriteriaProgressTree> AchievementTreeProgressMap;

enum CriteriaSort
{
    PLAYER_CRITERIA     = 0,
    GUILD_CRITERIA      = 1,
    SCENARIO_CRITERIA   = 2,
};

template<class T>
class AchievementMgr
{
    public:
        AchievementMgr(T* owner);
        ~AchievementMgr();

        void Reset();
        static void DeleteFromDB(uint32 lowguid, uint32 accountId = 0);
        void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult achievementAccountResult = nullptr, PreparedQueryResult criteriaAccountResult = nullptr);
        void SaveToDB(SQLTransaction& trans);
        void ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, bool evenIfCriteriaComplete = false);
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, uint32 miscValue3 = 0, Unit const* unit = nullptr,
                                       Player* referencePlayer = nullptr, bool init = false, bool loginCheck = false);
        void CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer, CriteriaProgressMap* progressMap = nullptr, bool loginCheck = false);
        bool IsCompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer, CriteriaProgressMap* progressMap = nullptr);
        void CheckAllAchievementCriteria(Player* referencePlayer);
        void SendAllAchievementData(Player* receiver);
        void SendAllAccountCriteriaData(Player* receiver);
        void SendAchievementInfo(Player* receiver, uint32 achievementId = 0);
        bool HasAchieved(uint32 achievementId, uint64 guid = 0) const;
        bool HasAccountAchieved(uint32 achievementId) const;
        uint64 GetFirstAchievedCharacterOnAccount(uint32 achievementId) const;
        T* GetOwner() const { return _owner; }

        void UpdateTimedAchievements(uint32 timeDiff);
        void StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry, uint32 timeLost = 0);
        void RemoveTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry);   // used for quest and scripted timed achievements
        uint32 GetAchievementPoints() const { return _achievementPoints; }
        CriteriaSort GetCriteriaSort() const;
        bool IsCompletedCriteria(CriteriaTreeEntry const* criteriaTree, AchievementEntry const* achievement, CriteriaEntry const* criteria, CriteriaProgressMap* progressMap = NULL, CriteriaTreeProgress* progress = NULL);
        uint32 IsCompletedCriteriaTreeCounter(CriteriaTreeEntry const* criteriaTree, AchievementEntry const* achievement);
        bool IsCompletedCriteriaTree(CriteriaTreeEntry const* criteriaTree, AchievementEntry const* achievement, CriteriaProgressMap* progressMap, bool parent = false);
        bool IsCompletedScenarioTree(CriteriaTreeEntry const* criteriaTree);
        CriteriaProgressMap* GetCriteriaProgressMap(uint32 achievementId);
        CriteriaProgressTree* GetCriteriaTreeProgressMap(uint32 criteriaTreeId);
        void ClearProgressMap(CriteriaProgressMap* progressMap);

        const CompletedAchievementMap &GetCompletedAchievementsList() const { return m_completedAchievements; }
        ACE_Thread_Mutex &GetCompletedAchievementLock() { return m_CompletedAchievementsLock; }

    private:
        enum ProgressType { PROGRESS_SET, PROGRESS_ACCUMULATE, PROGRESS_HIGHEST };
        void SendAchievementEarned(AchievementEntry const* achievement) const;
        void SendCriteriaUpdate(CriteriaEntry const* criteria, CriteriaTreeProgress const* progress, uint32 timeElapsed, bool timedCompleted) const;
        void SendAccountCriteriaUpdate(CriteriaEntry const* criteria, CriteriaTreeProgress const* progress, uint32 timeElapsed, bool timedCompleted) const;

        CriteriaTreeProgress* GetCriteriaProgress(uint32 entry, uint32 achievementId, CriteriaProgressMap* progressMap = NULL);
        bool SetCriteriaProgress(AchievementEntry const* achievement, CriteriaTreeEntry const* treeEntry, CriteriaEntry const* criteria, uint32 changeValue, Player* referencePlayer, ProgressType ptype, CriteriaProgressMap*& progressMap, CriteriaTreeProgress*& progress);
        void RemoveCriteriaProgress(CriteriaTreeEntry const* criteriaTree, uint32 achievementId);
        bool CanCompleteCriteria(AchievementEntry const* achievement);

        bool CanUpdateCriteria(CriteriaTreeEntry const* treeEntry, CriteriaEntry const* criteria, AchievementEntry const* achievement, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer, CriteriaProgressMap* progressMap, CriteriaTreeProgress* progress);

        void SendPacket(WorldPacket* data) const;
        void GenerateAchievementProgressMap(AchievementEntry const* achievement, CriteriaProgressMap* progressMap, CriteriaTreeEntry const* parentTree, CriteriaProgressTree* parentTreeProgress);

        bool ConditionsSatisfied(CriteriaEntry const *criteria, Player* referencePlayer) const;
        bool RequirementsSatisfied(AchievementEntry const* achievement, CriteriaEntry const *criteria, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer) const;
        bool AdditionalRequirementsSatisfied(uint32 ModifyTree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer) const;

        T* _owner;
        AchievementProgressMap m_achievementProgress;
        AchievementTreeProgressMap m_achievementTreeProgress;
        CompletedAchievementMap m_completedAchievements;
        mutable ACE_Thread_Mutex m_CompletedAchievementsLock;
        typedef ACE_Based::LockedMap<uint32, uint32> TimedAchievementMap;
        TimedAchievementMap m_timedAchievements;      // Criteria id/time left in MS
        uint32 _achievementPoints;
};

class AchievementGlobalMgr
{
        friend class ACE_Singleton<AchievementGlobalMgr, ACE_Null_Mutex>;
        AchievementGlobalMgr() {}
        ~AchievementGlobalMgr() {}

    public:
        static char const* GetCriteriaTypeString(AchievementCriteriaTypes type);
        static char const* GetCriteriaTypeString(uint32 type);

        CriteriaTreeList const& GetCriteriaTreeByType(AchievementCriteriaTypes type, CriteriaSort sort) const
        {
            if (sort == PLAYER_CRITERIA)
                return m_CriteriaTreesByType[type];
            else if (sort == GUILD_CRITERIA)
                return m_GuildCriteriaTreesByType[type];
            else
                return m_ScenarioCriteriaTreesByType[type];
        }

        CriteriaTreeList const& GetTimedCriteriaTreeByType(AchievementCriteriaTimedTypes type) const
        {
            return m_CriteriaTreesByTimedType[type];
        }

        CriteriaTreeInfo const* GetCriteriaTreeInfoById(uint32 id) const
        {
            CriteriaTreeInfoMap::const_iterator itr = m_CriteriaTreeInfoById.find(id);
            return itr != m_CriteriaTreeInfoById.end() ? &itr->second : NULL;
        }

        AchievementEntryList const* GetAchievementByReferencedId(uint32 id) const
        {
            AchievementListByReferencedId::const_iterator itr = m_AchievementListByReferencedId.find(id);
            return itr != m_AchievementListByReferencedId.end() ? &itr->second : NULL;
        }

        AchievementReward const* GetAchievementReward(AchievementEntry const* achievement) const
        {
            AchievementRewards::const_iterator iter = m_achievementRewards.find(achievement->ID);
            return iter != m_achievementRewards.end() ? &iter->second : NULL;
        }

        AchievementRewardLocale const* GetAchievementRewardLocale(AchievementEntry const* achievement) const
        {
            AchievementRewardLocales::const_iterator iter = m_achievementRewardLocales.find(achievement->ID);
            return iter != m_achievementRewardLocales.end() ? &iter->second : NULL;
        }

        AchievementCriteriaDataSet const* GetCriteriaDataSet(CriteriaEntry const* achievementCriteria) const
        {
            AchievementCriteriaDataMap::const_iterator iter = m_criteriaDataMap.find(achievementCriteria->ID);
            return iter != m_criteriaDataMap.end() ? &iter->second : NULL;
        }

        bool IsRealmCompleted(AchievementEntry const* achievement) const
        {
            return m_allCompletedAchievements.find(achievement->ID) != m_allCompletedAchievements.end();
        }

        void SetRealmCompleted(AchievementEntry const* achievement)
        {
            m_allCompletedAchievements.insert(achievement->ID);
        }

        bool IsGroupCriteriaType(AchievementCriteriaTypes type) const
        {
            switch (type)
            {
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:         // NYI
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:        // NYI
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:  // NYI
                    return true;
                default:
                    break;
            }

            return false;
        }

        void LoadAchievementCriteriaList();
        void LoadAchievementCriteriaData();
        void LoadAchievementReferenceList();
        void LoadCompletedAchievements();
        void LoadRewards();
        void LoadRewardLocales();
        AchievementEntry const* GetAchievement(uint32 achievementId) const;
        CriteriaEntry const* GetAchievementCriteria(uint32 criteriaId) const;
        CriteriaTreeEntry const* GetAchievementCriteriaTree(uint32 criteriaId) const;
        uint32 GetParantTreeId(uint32 parent);

        void PrepareCriteriaUpdateTaskThread();

        void AddCriteriaUpdateTask(AchievementCriteriaUpdateTask const& p_Task)
        {
            _lockedPlayersAchievementCriteriaTask[p_Task.PlayerGUID].add(p_Task);
        }

        PlayersAchievementCriteriaTask const& GetPlayersCriteriaTask() const
        {
            return _playersAchievementCriteriaTask;
        }

        void ClearPlayersCriteriaTask()
        {
            _playersAchievementCriteriaTask.clear();
        }

    private:
        AchievementCriteriaDataMap m_criteriaDataMap;

        // store achievement criterias by type to speed up lookup
        CriteriaTreeList m_CriteriaTreesByType[ACHIEVEMENT_CRITERIA_TYPE_TOTAL];
        CriteriaTreeList m_GuildCriteriaTreesByType[ACHIEVEMENT_CRITERIA_TYPE_TOTAL];
        CriteriaTreeList m_ScenarioCriteriaTreesByType[ACHIEVEMENT_CRITERIA_TYPE_TOTAL];

        CriteriaTreeList m_CriteriaTreesByTimedType[ACHIEVEMENT_TIMED_TYPE_MAX];
        CriteriaTreeInfoMap m_CriteriaTreeInfoById;

        // store achievements by referenced achievement id to speed up lookup
        AchievementListByReferencedId m_AchievementListByReferencedId;

        typedef std::set<uint32> AllCompletedAchievements;
        AllCompletedAchievements m_allCompletedAchievements;

        AchievementRewards m_achievementRewards;
        AchievementRewardLocales m_achievementRewardLocales;

        LockedPlayersAchievementCriteriaTask _lockedPlayersAchievementCriteriaTask;  /// All criteria update task are first storing
        PlayersAchievementCriteriaTask       _playersAchievementCriteriaTask;        /// Before thread process, all task stored 
};

#define sAchievementMgr ACE_Singleton<AchievementGlobalMgr, ACE_Null_Mutex>::instance()

class MapUpdater;
class AchievementCriteriaUpdateRequest : public MapUpdaterTask
{
    public:
        AchievementCriteriaUpdateRequest(MapUpdater *updater, AchievementCriteriaTaskQueue taskQueue);
        virtual void call() override;

    private:
        AchievementCriteriaTaskQueue _criteriaUpdateTasks;
};

#endif
