/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#ifndef TRINITY_SCENARIOMGR_H
#define TRINITY_SCENARIOMGR_H

#include "Common.h"
#include <ace/Singleton.h>
#include "DatabaseEnv.h"
#include "AchievementMgr.h"

class ScenarioProgress;
struct ScenarioStepEntry;

typedef std::map<uint8, ScenarioStepEntry const*> ScenarioSteps;

namespace lfg{
struct LFGDungeonData;
}

enum ScenarioType
{
    SCENARIO_TYPE_NORMAL            = 0,
    SCENARIO_TYPE_CHALLENGE         = 1,
    SCENARIO_TYPE_PROVING_GROUNDS   = 2,
};

class ScenarioProgress
{
public:
    ScenarioProgress() : instanceId(0), dungeonData(NULL), type(SCENARIO_TYPE_NORMAL), m_achievementMgr(this), currentStep(0) { }
    ScenarioProgress(uint32 _instanceId, lfg::LFGDungeonData const* _dungeonData);

    void SaveToDB(SQLTransaction& trans);
    void LoadFromDB();
    void DeleteFromDB();

    uint32 GetInstanceId() const { return instanceId; }
    ScenarioType GetType() const { return type; }
    uint32 GetScenarioId() const;
    uint32 GetCurrentStep() const { return currentStep; }

    bool IsCompleted(bool bonus) const;
    uint8 GetBonusStepCount() const;
    bool HasBonusStep() const;
    uint8 GetStepCount(bool withBonus) const;
    uint8 UpdateCurrentStep(bool loading);

    AchievementMgr<ScenarioProgress>& GetAchievementMgr() { return m_achievementMgr; }
    AchievementMgr<ScenarioProgress> const& GetAchievementMgr() const { return m_achievementMgr; }

    void SendStepUpdate(Player* player = NULL, bool full = false);
    void SendCriteriaUpdate(uint32 criteriaId, uint32 counter, time_t date);
    void BroadCastPacket(WorldPacket& data);

    bool CanUpdateCriteria(uint32 criteriaTreeId) const;

private:
    uint32 instanceId;
    lfg::LFGDungeonData const* dungeonData;
    AchievementMgr<ScenarioProgress> m_achievementMgr;

    uint8 currentStep;
    ScenarioSteps steps;

    ScenarioType type;
};

typedef UNORDERED_MAP<uint32 /*instance_id*/, ScenarioProgress> ScenarioProgressMap;
typedef UNORDERED_MAP<uint32, ScenarioSteps> ScenarioStepsByScenarioMap;

class ScenarioMgr
{
    friend class ACE_Singleton<ScenarioMgr, ACE_Null_Mutex>;

    ScenarioMgr();
    ~ScenarioMgr();

public:
    void Initialize();

    void Update(uint32 diff);

    void LoadFromDB();
    void SaveToDB(SQLTransaction& trans);

    static ScenarioType GetScenarioType(uint32 scenarioId);

    void AddScenarioProgress(uint32 instanceId, lfg::LFGDungeonData const* _dungeonData, bool loading);
    void RemoveScenarioProgress(uint32 instanceId);
    ScenarioProgress* GetScenarioProgress(uint32 instanceId);

    ScenarioSteps const* GetScenarioSteps(uint32 scenarioId);

private:
    ScenarioProgressMap m_scenarioProgressMap;

    ScenarioStepsByScenarioMap m_stepMap;

    uint32 updateDiff;

};

#define sScenarioMgr ACE_Singleton<ScenarioMgr, ACE_Null_Mutex>::instance()

#endif
