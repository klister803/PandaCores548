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

#include "Common.h"
#include "DatabaseEnv.h"
#include "DBCEnums.h"
#include "DB2Stores.h"
#include "Map.h"

#include <ting/shared_mutex.hpp>
#include <mutex>

static uint16 const MAX_ACHIEVEMENT = 9000;
static uint32 const MAX_CRITERIA = 36000;
static uint32 const MAX_CRITERIA_TREE = 36000;
static uint32 const MAX_MODIFIER_TREE = 15000;

struct ModifierTreeNode
{
    ModifierTreeEntry const* Entry;
    std::vector<ModifierTreeNode const*> Children;
};

typedef std::vector<AchievementEntry const*> AchievementEntryList;
typedef std::vector<AchievementEntryList> AchievementListByReferencedId;

struct Criteria
{
    uint32 ID = 0;
    CriteriaEntry const* Entry = nullptr;
    ModifierTreeNode const* Modifier = nullptr;
    uint32 FlagsCu = 0;
};

typedef std::vector<Criteria const*> CriteriaList;

struct CriteriaProgress
{
    CriteriaProgress() : Counter(0), PlayerGUID(), date(0), changed(false), updated(false), completed(false), deactiveted(true)
    , achievement(nullptr), criteriaTree(nullptr), parent(nullptr), criteria(nullptr)    { }

    uint64 Counter;
    time_t date;                                            // latest update time.
    uint64 PlayerGUID;                               // GUID of the player that completed this criteria (guild achievements)
    bool changed;
    bool updated;
    bool completed;
    bool deactiveted;

    AchievementEntry const* achievement;
    CriteriaTreeEntry const* criteriaTree;
    CriteriaTreeEntry const* parent;
    CriteriaEntry const* criteria;
    Criteria const* _criteria;
};

struct CriteriaTree
{
    uint32 ID = 0;
    uint32 CriteriaID = 0;
    CriteriaTreeEntry const* Entry = nullptr;
    AchievementEntry const* Achievement = nullptr;
    ScenarioStepEntry const* ScenarioStep = nullptr;
    struct Criteria const* Criteria = nullptr;
    std::vector<CriteriaTree const*> Children;
};

typedef std::vector<CriteriaTree const*> CriteriaTreeList;

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
    ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY      = 12,
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
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY    = 12
        struct
        {
            uint32 id;
        } difficulty;
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
typedef std::vector<AchievementCriteriaDataSet*> AchievementCriteriaDataVector;

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
typedef std::vector<AchievementReward*> AchievementRewardVector;

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
    GuidSet guids;
    uint64 first_guid;
    bool changed;
    bool isAccountAchievement;
};

typedef std::unordered_map<uint32, CriteriaProgress> CriteriaProgressMap;
typedef std::unordered_map<uint32, CompletedAchievementData> CompletedAchievementMap;

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
        void ClearMap();
        static void DeleteFromDB(uint64 lowguid, uint32 accountId = 0);
        void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult achievementAccountResult = nullptr, PreparedQueryResult criteriaAccountResult = nullptr);
        void SaveToDB(SQLTransaction& trans);
        void ResetAchievementCriteria(CriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, bool evenIfCriteriaComplete = false);
        void UpdateAchievementCriteria(CriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, uint32 miscValue3 = 0,Unit const* unit = nullptr, Player* referencePlayer = nullptr, bool init = false);
        void CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer);
        bool IsCompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer);
        void CheckAllAchievementCriteria(Player* referencePlayer);
        void SendAllAchievementData(Player* receiver);
        void SendAllAccountCriteriaData(Player* receiver);
        void SendAchievementInfo(Player* receiver, uint32 achievementId = 0);
        bool HasAchieved(uint32 achievementId, uint64 guid = 0) const;
        bool HasAccountAchieved(uint32 achievementId) const;
        uint64 GetFirstAchievedCharacterOnAccount(uint32 achievementId) const;
        T* GetOwner() const { return _owner; }

        void UpdateTimedAchievements(uint32 timeDiff);
        void StartTimedAchievement(CriteriaTimedTypes type, uint32 entry, uint16 timeLost = 0);
        void RemoveTimedAchievement(CriteriaTimedTypes type, uint32 entry);   // used for quest and scripted timed achievements
        uint32 GetAchievementPoints() const { return _achievementPoints; }
        CriteriaSort GetCriteriaSort() const;
        bool IsCompletedCriteria(CriteriaTree const* tree, uint64 requiredAmount);
        bool IsCompletedCriteriaTree(CriteriaTree const* tree);
        bool CanUpdateCriteriaTree(CriteriaTree const* tree, Player* referencePlayer) const;
        bool IsCompletedScenarioTree(CriteriaTreeEntry const* criteriaTree);
        bool CanUpdate() { return m_canUpdateAchiev == 1; }

        CompletedAchievementMap const* GetCompletedAchievementsList()
        {
            return &_completedAchievements;
        }

        CriteriaProgressMap const* GetCriteriaProgressMap()
        {
            return &_criteriaProgress;
        }

    private:
        enum ProgressType { PROGRESS_SET, PROGRESS_ACCUMULATE, PROGRESS_HIGHEST };
        void SendAchievementEarned(AchievementEntry const* achievement) const;
        void SendCriteriaUpdate(CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const;
        void SendAccountCriteriaUpdate(CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const;

        CriteriaProgress* GetCriteriaProgress(uint32 entry, bool create = false);
        bool SetCriteriaProgress(CriteriaTree const* tree, uint64 changeValue, Player* referencePlayer, ProgressType ptype);
        void RemoveCriteriaProgress(CriteriaTree const* criteriaTree);
        bool CanCompleteCriteria(AchievementEntry const* achievement);

        bool CanUpdateCriteria(CriteriaTree const* tree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer);

        void SendPacket(WorldPacket* data) const;

        bool ConditionsSatisfied(Criteria const *criteria, Player* referencePlayer) const;
        bool RequirementsSatisfied(CriteriaTree const* tree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer) const;
        bool AdditionalRequirementsSatisfied(ModifierTreeNode const* parent, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer) const;

        T* _owner;
        CriteriaProgressMap _criteriaProgress;
        CompletedAchievementMap _completedAchievements;

        std::mutex _completedAchievementsLock;
        typedef std::unordered_map<uint32, uint32> TimedAchievementMap;
        TimedAchievementMap _timeCriteriaTrees;      // Criteria id/time left in MS
        uint32 _achievementPoints;

        typedef ting::shared_mutex LockType;
        typedef std::lock_guard<LockType> WriteGuardType;
        typedef ting::shared_lock<LockType> ReadGuardType;

        LockType i_criteriaProgressLock;
        LockType i_completedAchievementsLock;

        uint64 m_canUpdateAchiev = 0;

        std::vector<CompletedAchievementData*> _completedAchievementsArr;
        std::vector<CriteriaProgress*> _criteriaProgressArr;
        std::vector<uint32*> _timeCriteriaTreesArr;
};

class AchievementGlobalMgr
{
        AchievementGlobalMgr() { }
        ~AchievementGlobalMgr() { }

    public:
        static char const* GetCriteriaTypeString(CriteriaTypes type);
        static char const* GetCriteriaTypeString(uint32 type);

        static AchievementGlobalMgr* instance()
        {
            static AchievementGlobalMgr instance;
            return &instance;
        }

        CriteriaTreeList const& GetCriteriaTreeByType(CriteriaTypes type, CriteriaSort sort) const
        {
            if (sort == PLAYER_CRITERIA)
                return _criteriasByType[type];
            else if (sort == GUILD_CRITERIA)
                return _guildCriteriasByType[type];
            else
                return _scenarioCriteriasByType[type];
        }

        CriteriaTreeList const* GetCriteriaTreesByCriteria(uint32 criteriaId) const
        {
            return _criteriaTreeByCriteriaVector[criteriaId];
        }

        CriteriaTreeList const& GetTimedCriteriaByType(CriteriaTimedTypes type) const
        {
            return _criteriasByTimedType[type];
        }

        AchievementEntryList const* GetAchievementByReferencedId(uint32 id) const
        {
            return &m_AchievementListByReferencedId[id];
        }

        AchievementReward const* GetAchievementReward(AchievementEntry const* achievement) const
        {
            return m_achievementRewardVector[achievement->ID];
        }

        AchievementRewardLocale const* GetAchievementRewardLocale(AchievementEntry const* achievement) const
        {
            AchievementRewardLocales::const_iterator iter = m_achievementRewardLocales.find(achievement->ID);
            return iter != m_achievementRewardLocales.end() ? &iter->second : nullptr;
        }

        AchievementCriteriaDataSet const* GetCriteriaDataSet(CriteriaTree const* Criteria) const
        {
            return m_criteriaDataVector[Criteria->ID];
        }

        bool IsRealmCompleted(AchievementEntry const* achievement) const
        {
            return m_allCompletedAchievements[achievement->ID];
        }

        void SetRealmCompleted(AchievementEntry const* achievement)
        {
            m_allCompletedAchievements[achievement->ID] = true;
        }

        bool IsGroupCriteriaType(CriteriaTypes type) const
        {
            switch (type)
            {
                case CRITERIA_TYPE_KILL_CREATURE:
                case CRITERIA_TYPE_WIN_BG:
                case CRITERIA_TYPE_BE_SPELL_TARGET:         // NYI
                case CRITERIA_TYPE_WIN_RATED_ARENA:
                case CRITERIA_TYPE_BE_SPELL_TARGET2:        // NYI
                case CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:  // NYI
                    return true;
                default:
                    break;
            }

            return false;
        }

        template<typename Func>
        static void WalkCriteriaTree(CriteriaTree const* tree, Func const& func)
        {
            for (CriteriaTree const* node : tree->Children)
                WalkCriteriaTree(node, func);

            func(tree);
        }

        void LoadCriteriaList();
        void LoadAchievementCriteriaData();
        void LoadAchievementReferenceList();
        void LoadCompletedAchievements();
        void LoadRewards();
        void LoadRewardLocales();
        uint32 GetParantTreeId(uint32 parent);

        void AchievementScriptLoaders(AchievementEntry const* achievement, Player* source);

        CriteriaTree const* GetCriteriaTree(uint32 criteriaTreeId) const;
        Criteria const* GetCriteria(uint32 criteriaId) const;
        ModifierTreeNode const* GetModifierTree(uint32 modifierTreeId) const;

    private:
        AchievementCriteriaDataMap m_criteriaDataMap;
        AchievementCriteriaDataVector m_criteriaDataVector;

        std::vector<CriteriaTree*> _criteriaTrees;
        std::vector<Criteria*> _criteria;
        std::vector<ModifierTreeNode*> _criteriaModifiers;

        std::unordered_map<uint32, CriteriaTreeList> _criteriaTreeByCriteria;
        std::vector<CriteriaTreeList*> _criteriaTreeByCriteriaVector;

        // store achievement criterias by type to speed up lookup
        CriteriaTreeList _criteriasByType[CRITERIA_TYPE_TOTAL];
        CriteriaTreeList _guildCriteriasByType[CRITERIA_TYPE_TOTAL];
        CriteriaTreeList _scenarioCriteriasByType[CRITERIA_TYPE_TOTAL];

        CriteriaTreeList _criteriasByTimedType[CRITERIA_TIMED_TYPE_MAX];

        // store achievements by referenced achievement id to speed up lookup
        AchievementListByReferencedId m_AchievementListByReferencedId;

        typedef std::vector<bool>  AllCompletedAchievements;
        AllCompletedAchievements m_allCompletedAchievements;

        AchievementRewards m_achievementRewards;
        AchievementRewardVector m_achievementRewardVector;
        AchievementRewardLocales m_achievementRewardLocales;
};

#define sAchievementMgr AchievementGlobalMgr::instance()

#endif
