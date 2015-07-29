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

#include "Common.h"
#include "DBCEnums.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiersImpl.h"
#include "Guild.h"
#include "Language.h"
#include "Player.h"
#include "SpellMgr.h"
#include "DisableMgr.h"
#include "ScriptMgr.h"
#include "MapManager.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "BattlegroundAB.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Group.h"
#include "Bracket.h"
#include "ScenarioMgr.h"
#include "Formulas.h"

namespace Trinity
{
    class AchievementChatBuilder
    {
        public:
            AchievementChatBuilder(Player const& player, ChatMsg msgtype, int32 textId, uint32 ach_id)
                : i_player(player), i_msgtype(msgtype), i_textId(textId), i_achievementId(ach_id) {}
            void operator()(WorldPacket& data, LocaleConstant loc_idx)
            {
                Trinity::ChatData c;
                c.message = sObjectMgr->GetTrinityString(i_textId, loc_idx);
                c.sourceGuid = i_player.GetGUID();
                c.targetGuid = i_player.GetGUID();
                c.chatType = i_msgtype;
                c.achievementId = i_achievementId;

                Trinity::BuildChatPacket(data, c);
            }

        private:
            Player const& i_player;
            ChatMsg i_msgtype;
            int32 i_textId;
            uint32 i_achievementId;
    };
}                                                           // namespace Trinity

bool AchievementCriteriaData::IsValid(CriteriaEntry const* criteria)
{
    if (dataType >= MAX_ACHIEVEMENT_CRITERIA_DATA_TYPE)
    {
        sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` for criteria (Entry: %u) has wrong data type (%u), ignored.", criteria->ID, dataType);
        return false;
    }

    switch (criteria->type)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:          // only hardcoded list
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:    // only Children's Week achievements
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:                // only Children's Week achievements
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            break;
        default:
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` has data for non-supported criteria type (Entry: %u Type: %u), ignored.", criteria->ID, criteria->type);
                return false;
            }
            break;
    }

    switch (dataType)
    {
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE:
        case ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT:
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE:
            if (!creature.id || !sObjectMgr->GetCreatureTemplate(creature.id))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_CREATURE (%u) has non-existing creature id in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, creature.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE:
            if (classRace.class_id && ((1 << (classRace.class_id-1)) & CLASSMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE (%u) has non-existing class in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id-1)) & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE (%u) has non-existing race in value2 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, classRace.race_id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH:
            if (health.percent < 1 || health.percent > 100)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_PLAYER_LESS_HEALTH (%u) has wrong percent value in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, health.percent);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA:
        {
            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(aura.spell_id);
            if (!spellEntry)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type %s (%u) has wrong spell id in value1 (%u), ignored.",
                    criteria->ID, criteria->type, (dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA?"ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA":"ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA"), dataType, aura.spell_id);
                return false;
            }
            if (aura.effect_idx >= 3)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type %s (%u) has wrong spell effect index in value2 (%u), ignored.",
                    criteria->ID, criteria->type, (dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA?"ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA":"ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA"), dataType, aura.effect_idx);
                return false;
            }
            if (!spellEntry->Effects[aura.effect_idx].ApplyAuraName)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type %s (%u) has non-aura spell effect (ID: %u Effect: %u), ignores.",
                    criteria->ID, criteria->type, (dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA?"ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA":"ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA"), dataType, aura.spell_id, aura.effect_idx);
                return false;
            }
            return true;
        }
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA:
            if (!GetAreaEntryByAreaID(area.id))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA (%u) has wrong area id in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, area.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL:
            if (level.minlevel > STRONG_MAX_LEVEL)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL (%u) has wrong minlevel in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, level.minlevel);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER:
            if (gender.gender > GENDER_NONE)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER (%u) has wrong gender in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, gender.gender);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT:
            if (!ScriptId)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT (%u) does not have ScriptName set, ignored.",
                    criteria->ID, criteria->type, dataType);
                return false;
            }
            return true;
        /*Todo:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY:
        */
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT:
            if (map_players.maxcount <= 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT (%u) has wrong max players count in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, map_players.maxcount);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM:
            if (team.team != ALLIANCE && team.team != HORDE)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM (%u) has unknown team in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, team.team);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK:
            if (drunk.state >= MAX_DRUNKEN)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK (%u) has unknown drunken state in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, drunk.state);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY:
            if (!sHolidaysStore.LookupEntry(holiday.id))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY (%u) has unknown holiday in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, holiday.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE:
            return true;                                    // not check correctness node indexes
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPED_ITEM:
            if (equipped_item.item_quality >= MAX_ITEM_QUALITY)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPED_ITEM (%u) has unknown quality state in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, equipped_item.item_quality);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE:
            if (!classRace.class_id && !classRace.race_id)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE (%u) must not have 0 in either value field, ignored.",
                    criteria->ID, criteria->type, dataType);
                return false;
            }
            if (classRace.class_id && ((1 << (classRace.class_id-1)) & CLASSMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE (%u) has non-existing class in value1 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id-1)) & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE (%u) has non-existing race in value2 (%u), ignored.",
                    criteria->ID, criteria->type, dataType, classRace.race_id);
                return false;
            }
            return true;
        default:
            sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) has data for non-supported data type (%u), ignored.", criteria->ID, criteria->type, dataType);
            return false;
    }
}

bool AchievementCriteriaData::Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscValue1 /*= 0*/) const
{
    switch (dataType)
    {
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE:
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE:
            if (!target || target->GetTypeId() != TYPEID_UNIT)
                return false;
            return target->GetEntry() == creature.id;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            if (classRace.class_id && classRace.class_id != target->ToPlayer()->getClass())
                return false;
            if (classRace.race_id && classRace.race_id != target->ToPlayer()->getRace())
                return false;
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE:
            if (!source || source->GetTypeId() != TYPEID_PLAYER)
                return false;
            if (classRace.class_id && classRace.class_id != source->ToPlayer()->getClass())
                return false;
            if (classRace.race_id && classRace.race_id != source->ToPlayer()->getRace())
                return false;
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            return !target->HealthAbovePct(health.percent);
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA:
            return source->HasAuraEffect(aura.spell_id, aura.effect_idx);
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA:
            return area.id == source->GetZoneId() || area.id == source->GetAreaId();
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA:
            return target && target->HasAuraEffect(aura.spell_id, aura.effect_idx);
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE:
           return miscValue1 >= value.minvalue;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL:
            if (!target)
                return false;
            return target->getLevel() >= level.minlevel;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER:
            if (!target)
                return false;
            return target->getGender() == gender.gender;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT:
            return sScriptMgr->OnCriteriaCheck(this, const_cast<Player*>(source), const_cast<Unit*>(target));
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT:
            return source->GetMap()->GetPlayersCountExceptGMs() <= map_players.maxcount;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            return target->ToPlayer()->GetTeam() == team.team;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK:
            return Player::GetDrunkenstateByValue(source->GetDrunkValue()) >= DrunkenState(drunk.state);
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY:
            return IsHolidayActive(HolidayIds(holiday.id));
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE:
        {
            Battleground* bg = source->GetBattleground();
            if (!bg)
                return false;
            return bg->IsTeamScoreInRange(source->GetTeam() == ALLIANCE ? HORDE : ALLIANCE, bg_loss_team_score.min_score, bg_loss_team_score.max_score);
        }
        case ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT:
        {
            if (!source->IsInWorld())
                return false;
            Map* map = source->GetMap();
            if (!map->IsDungeon())
            {
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Achievement system call ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT (%u) for achievement criteria %u for non-dungeon/non-raid map %u",
                    ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT, criteria_id, map->GetId());
                    return false;
            }
            InstanceScript* instance = ((InstanceMap*)map)->GetInstanceScript();
            if (!instance)
            {
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Achievement system call ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT (%u) for achievement criteria %u for map %u but map does not have a instance script",
                    ACHIEVEMENT_CRITERIA_DATA_INSTANCE_SCRIPT, criteria_id, map->GetId());
                return false;
            }
            return instance->CheckAchievementCriteriaMeet(criteria_id, source, target, miscValue1);
        }
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPED_ITEM:
        {
            ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(miscValue1);
            if (!pProto)
                return false;
            return pProto->ItemLevel >= equipped_item.item_level && pProto->Quality >= equipped_item.item_quality;
        }
        default:
            break;
    }
    return false;
}

bool AchievementCriteriaDataSet::Meets(Player const* source, Unit const* target, uint32 miscvalue /*= 0*/) const
{
    for (Storage::const_iterator itr = storage.begin(); itr != storage.end(); ++itr)
        if (!itr->Meets(criteria_id, source, target, miscvalue))
            return false;

    return true;
}

template<class T>
AchievementMgr<T>::AchievementMgr(T* owner): _owner(owner), _achievementPoints(0) 
{
}

template<class T>
AchievementMgr<T>::~AchievementMgr()
{
}

template<class T>
void AchievementMgr<T>::SendPacket(WorldPacket* data) const
{
}

template<>
void AchievementMgr<ScenarioProgress>::SendPacket(WorldPacket* data) const
{
    // FIXME
}

template<>
void AchievementMgr<Guild>::SendPacket(WorldPacket* data) const
{
    GetOwner()->BroadcastPacket(data);
}

template<>
void AchievementMgr<Player>::SendPacket(WorldPacket* data) const
{
    GetOwner()->GetSession()->SendPacket(data);
}

template<class T>
void AchievementMgr<T>::RemoveCriteriaProgress(const CriteriaTreeEntry* criteriaTree)
{
    CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

    if (!progressMap)
        return;

    CriteriaProgressMap::iterator criteriaProgress = progressMap->find(criteriaTree->ID);
    if (criteriaProgress == progressMap->end())
        return;

    WorldPacket data(SMSG_CRITERIA_DELETED, 4);
    data << uint32(criteriaTree->criteria);
    SendPacket(&data);

    progressMap->erase(criteriaProgress);
}

template<>
void AchievementMgr<ScenarioProgress>::RemoveCriteriaProgress(const CriteriaTreeEntry* criteriaTree)
{
    // FIXME
}

template<>
void AchievementMgr<Guild>::RemoveCriteriaProgress(const CriteriaTreeEntry* criteriaTree)
{
    CriteriaProgressMap::iterator criteriaProgress = GetCriteriaProgressMap()->find(criteriaTree->ID);
    if (criteriaProgress == GetCriteriaProgressMap()->end())
        return;

    ObjectGuid guid = GetOwner()->GetGUID();

    WorldPacket data(SMSG_GUILD_CRITERIA_DELETED, 4 + 8 + 1);
    data << uint32(criteriaTree->criteria);
    data.WriteGuidMask<0, 3, 5, 6, 4, 1, 7, 2>(guid);
    data.WriteGuidBytes<7, 0, 3, 5, 6, 2, 4, 1>(guid);

    SendPacket(&data);

    GetCriteriaProgressMap()->erase(criteriaProgress);
}

template<class T>
void AchievementMgr<T>::ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1, uint32 miscValue2, bool evenIfCriteriaComplete)
{
    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::ResetAchievementCriteria(%u, %u, %u)", type, miscValue1, miscValue2);

    // disable for gamemasters with GM-mode enabled
    if (GetOwner()->isGameMaster())
        return;

    CriteriaTreeList const& criteriaTreeList = sAchievementMgr->GetCriteriaTreeByType(type, GetCriteriaSort());
    for (CriteriaTreeList::const_iterator i = criteriaTreeList.begin(); i != criteriaTreeList.end(); ++i)
    {
        CriteriaTreeEntry const* criteriaTree = i->criteriaTree;
        CriteriaEntry const* criteria = i->criteria;
        AchievementEntry const* achievement = i->achievement;
        if (!achievement)
            continue;

        // don't update already completed criteria if not forced or achievement already complete
        if ((IsCompletedCriteria(criteriaTree, achievement, criteria) && !evenIfCriteriaComplete)/* || HasAchieved(achievement->ID)*/)
            continue;

        if (criteria->timedCriteriaStartType == miscValue1 &&
            (!criteria->timedCriteriaMiscId ||
            criteria->timedCriteriaMiscId == miscValue2))
        {
            RemoveCriteriaProgress(criteriaTree);
            break;
        }

        if (criteria->timedCriteriaFailType == miscValue1 &&
            (!criteria->timedCriteriaMiscFailId ||
            criteria->timedCriteriaMiscFailId == miscValue2))
        {
            RemoveCriteriaProgress(criteriaTree);
            break;
        }
    }
}

template<>
void AchievementMgr<ScenarioProgress>::ResetAchievementCriteria(AchievementCriteriaTypes /*type*/, uint32 /*miscValue1*/, uint32 /*miscValue2*/, bool /*evenIfCriteriaComplete*/)
{
    // Not needed
}

template<>
void AchievementMgr<Guild>::ResetAchievementCriteria(AchievementCriteriaTypes /*type*/, uint32 /*miscValue1*/, uint32 /*miscValue2*/, bool /*evenIfCriteriaComplete*/)
{
    // Not needed
}

template<class T>
void AchievementMgr<T>::DeleteFromDB(uint32 /*lowguid*/, uint32 /*accountId*/)
{
}

template<>
void AchievementMgr<ScenarioProgress>::DeleteFromDB(uint32 lowguid, uint32 accountId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_SCENARIO_CRITERIAPROGRESS);
    stmt->setUInt32(0, lowguid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

template<>
void AchievementMgr<Player>::DeleteFromDB(uint32 lowguid, uint32 accountId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT);
    stmt->setUInt32(0, lowguid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

template<>
void AchievementMgr<Guild>::DeleteFromDB(uint32 lowguid, uint32 accountId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GUILD_ACHIEVEMENTS);
    stmt->setUInt32(0, lowguid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GUILD_ACHIEVEMENT_CRITERIA);
    stmt->setUInt32(0, lowguid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

template<class T>
void AchievementMgr<T>::SaveToDB(SQLTransaction& /*trans*/)
{
}

template<>
void AchievementMgr<Player>::SaveToDB(SQLTransaction& trans)
{
    if (!m_completedAchievements.empty())
    {
        bool need_execute = false;
        bool need_execute_acc = false;

        std::ostringstream ssAccIns;
        std::ostringstream ssCharIns;

        for (CompletedAchievementMap::iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            /// first new/changed record prefix
            if (!need_execute)
            {
                ssCharIns << "REPLACE INTO character_achievement (guid, achievement, date) VALUES ";
                need_execute = true;
            }
            /// next new/changed record prefix
            else
                ssCharIns << ',';

            if (!need_execute_acc)
            {
                ssAccIns << "REPLACE INTO account_achievement (account, first_guid, achievement, date) VALUES ";
                need_execute_acc = true;
            }
            else
                ssAccIns << ',';

            // new/changed record data
            ssAccIns << '(' << GetOwner()->GetSession()->GetAccountId() << ',' << iter->second.first_guid << ',' << iter->first << ',' << iter->second.date << ')';
            ssCharIns << '(' << GetOwner()->GetGUIDLow() << ',' << iter->first << ',' << iter->second.date << ')';

            /// mark as saved in db
            iter->second.changed = false;
        }

        if (need_execute)
            trans->Append(ssCharIns.str().c_str());

        if (need_execute_acc)
            trans->Append(ssAccIns.str().c_str());
    }

    CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

    if (!progressMap)
        return;

    if (!progressMap->empty())
    {
        /// prepare deleting and insert
        bool need_execute_ins       = false;
        bool need_execute_account   = false;

        bool isAccountAchievement   = false;

        bool alreadyOneCharInsLine  = false;
        bool alreadyOneAccInsLine   = false;

        std::ostringstream ssAccins;
        std::ostringstream ssCharins;
        
        uint64 guid      = GetOwner()->GetGUIDLow();
        uint32 accountId = GetOwner()->GetSession()->GetAccountId();

        for (CriteriaProgressMap::iterator iter = progressMap->begin(); iter != progressMap->end(); ++iter)
        {
            if (!iter->second.changed && !iter->second.updated)
                continue;

            //disable? active before test achivement system
            AchievementEntry const* achievement = iter->second.achievement;
            if (!achievement)
                continue;

            if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
            {
                isAccountAchievement = true;
                need_execute_account = true;
            }
            else
                isAccountAchievement = false;

            // store data only for real progress
            bool hasAchieve = CanDeleteOrUpdateCreteria(achievement->ID, GetOwner()->GetGUIDLow()) || (achievement->parent && !HasAchieved(achievement->parent));
            if (iter->second.counter != 0 && !hasAchieve)
            {
                uint32 achievID = iter->second.achievement ? iter->second.achievement->ID : 0;
                if(iter->second.changed)
                {
                    /// first new/changed record prefix
                    if (!need_execute_ins)
                    {
                        ssAccins  << "REPLACE INTO account_achievement_progress   (account, criteria, counter, date, achievID) VALUES ";
                        ssCharins << "REPLACE INTO character_achievement_progress (guid,    criteria, counter, date, achievID) VALUES ";
                        need_execute_ins = true;
                    }
                    /// next new/changed record prefix
                    else
                    {
                        if (isAccountAchievement)
                        {
                            if (alreadyOneAccInsLine)
                                ssAccins  << ',';
                        }
                        else
                        {
                            if (alreadyOneCharInsLine)
                                ssCharins << ',';
                        }
                    }

                    // new/changed record data
                    if (isAccountAchievement)
                    {
                        ssAccins  << '(' << accountId << ',' << iter->first << ',' << iter->second.counter << ',' << iter->second.date <<  ',' << achievID << ')';
                        alreadyOneAccInsLine  = true;
                    }
                    else
                    {
                        ssCharins << '(' << guid      << ',' << iter->first << ',' << iter->second.counter << ',' << iter->second.date << ',' << achievID << ')';
                        alreadyOneCharInsLine = true;
                    }
                }
                if(iter->second.updated)
                {
                    std::ostringstream ssUpd;
                    if (isAccountAchievement)
                        ssUpd  << "UPDATE account_achievement_progress SET counter = " << iter->second.counter << ", date = " << iter->second.date << ", achievID = " << achievID << " WHERE account = " << accountId << " AND criteria = " << iter->first << ';';
                    else
                        ssUpd  << "UPDATE character_achievement_progress SET counter = " << iter->second.counter << ", date = " << iter->second.date << ", achievID = " << achievID << " WHERE guid = " << guid << " AND criteria = " << iter->first << ';';
                    trans->Append(ssUpd.str().c_str());
                }
            }

            /// mark as updated in db
            iter->second.changed = false;
            iter->second.updated = false;
        }


        if (need_execute_ins)
        {
            if (need_execute_account && alreadyOneAccInsLine)
                trans->Append(ssAccins.str().c_str());

            if (alreadyOneCharInsLine)
                trans->Append(ssCharins.str().c_str());
        }
    }
}

template<>
void AchievementMgr<Guild>::SaveToDB(SQLTransaction& trans)
{
    PreparedStatement* stmt;
    std::ostringstream guidstr;
    for (CompletedAchievementMap::iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        if (!itr->second.changed)
            continue;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT);
        stmt->setUInt32(0, GetOwner()->GetId());
        stmt->setUInt32(1, itr->first);
        stmt->setUInt32(2, itr->second.date);
        for (std::set<uint64>::const_iterator gItr = itr->second.guids.begin(); gItr != itr->second.guids.end(); ++gItr)
            guidstr << GUID_LOPART(*gItr) << ',';

        stmt->setString(3, guidstr.str());
        trans->Append(stmt);
        itr->second.changed = false;

        guidstr.str("");
    }

    CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

    if (!progressMap)
        return;

    for (CriteriaProgressMap::iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
    {
        if (itr->second.changed)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT_CRITERIA);
            stmt->setUInt32(0, GetOwner()->GetId());
            stmt->setUInt32(1, itr->first);
            stmt->setUInt32(2, itr->second.counter);
            stmt->setUInt32(3, itr->second.date);
            stmt->setUInt32(4, GUID_LOPART(itr->second.CompletedGUID));
            stmt->setUInt32(5, itr->second.achievement ? itr->second.achievement->ID : 0);
            trans->Append(stmt);
            itr->second.changed = false;
        }

        if (itr->second.updated)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_ACHIEVEMENT_CRITERIA);
            stmt->setUInt32(0, itr->second.counter);
            stmt->setUInt32(1, itr->second.date);
            stmt->setUInt32(2, itr->second.achievement ? itr->second.achievement->ID : 0);
            stmt->setUInt32(3, GetOwner()->GetId());
            stmt->setUInt32(4, itr->first);
            trans->Append(stmt);
            itr->second.updated = false;
        }
    }
}

template<class T>
void AchievementMgr<T>::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult achievementAccountResult, PreparedQueryResult criteriaAccountResult)
{
}

template<>
void AchievementMgr<Player>::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult achievementAccountResult, PreparedQueryResult criteriaAccountResult)
{
    if (achievementAccountResult)
    {
        do
        {
            Field* fields = achievementAccountResult->Fetch();
            uint32 first_guid    = fields[0].GetUInt64();
            uint32 achievementid = fields[1].GetUInt32();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementMgr->GetAchievement(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievementid];
            ca.date = time_t(fields[2].GetUInt32());
            ca.changed = false;
            ca.first_guid = first_guid;
            ca.isAccountAchievement = achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT;

            _achievementPoints += achievement->points;

            // title achievement rewards are retroactive
            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
            {
                if (uint32 titleId = reward->titleId[Player::TeamForRace(GetOwner()->getRace()) == ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        GetOwner()->SetTitle(titleEntry);

                if (reward->learnSpell && !GetOwner()->HasSpell(reward->learnSpell))
                    GetOwner()->learnSpell(reward->learnSpell, true);
            }

        } 
        while (achievementAccountResult->NextRow());
    }

    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].GetUInt32();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementMgr->GetAchievement(achievementid);
            if (!achievement)
                continue;

            // not added on account?
            if (m_completedAchievements.find(achievementid) == m_completedAchievements.end())
            {
                CompletedAchievementData& ca = m_completedAchievements[achievementid];
                ca.changed = true;
                ca.first_guid = GetOwner()->GetGUIDLow();
                ca.date = time_t(fields[1].GetUInt32());
                _achievementPoints += achievement->points;
            }
            else
            {
                CompletedAchievementData& ca = m_completedAchievements[achievementid];
                ca.changed = false;
                ca.first_guid = GetOwner()->GetGUIDLow();
                ca.date = time_t(fields[1].GetUInt32());
            }

            // title achievement rewards are retroactive
            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
            {
                if (uint32 titleId = reward->titleId[Player::TeamForRace(GetOwner()->getRace()) == ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        GetOwner()->SetTitle(titleEntry);

                if (reward->learnSpell && !GetOwner()->HasSpell(reward->learnSpell))
                    GetOwner()->learnSpell(reward->learnSpell, true);
            }

        }
        while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 char_criteria_id      = fields[0].GetUInt32();
            time_t date                  = time_t(fields[2].GetUInt32());
            uint32 achievementID         = fields[3].GetUInt32();
            bool update = false;

            CriteriaTreeEntry const* criteriaTree = sAchievementMgr->GetAchievementCriteriaTree(char_criteria_id);
            if (!criteriaTree)
            {
                // we will remove not existed criteriaTree for all characters
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteriaTree %u data removed from table `character_achievement_progress`.", char_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, char_criteria_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(criteriaTree->criteria);
            if (!criteria)
            {
                // we will remove not existed criteria for all characters
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `character_achievement_progress`.", char_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, char_criteria_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            AchievementEntry const* achievement = NULL;
            if(!achievementID)
            {
                uint32 parent = sAchievementMgr->GetParantTreeId(criteriaTree->parent);
                achievement = GetsAchievementByTreeList(parent);
                update = true;
            }
            else
                achievement = sAchievementMgr->GetAchievement(achievementID);

            bool hasAchieve = !achievement || CanDeleteOrUpdateCreteria(achievement->ID, GetOwner()->GetGUIDLow());
            if (hasAchieve)
            {
                // we will remove already completed criteria
                //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "Achievement %s with progress char_criteria_id %u data removed from table `character_achievement_progress` ", achievement ? "completed" : "not exist", char_criteria_id);
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, char_criteria_id);
                stmt->setUInt32(1, GetOwner()->GetGUIDLow());
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->timeLimit && time_t(date + criteria->timeLimit) < now)
                continue;

            CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

            if (!progressMap)
                continue;

            CriteriaTreeProgress& progress = (*progressMap)[char_criteria_id];
            progress.counter = fields[1].GetUInt32();
            progress.date    = date;
            progress.changed = false;
            progress.updated = update;
            progress.achievement = achievement;
            progress.criteriaTree = criteriaTree;
            progress.criteria = criteria;
        }
        while (criteriaResult->NextRow());
    }

    if (criteriaAccountResult)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = criteriaAccountResult->Fetch();
            uint32 acc_criteria_id      = fields[0].GetUInt32();
            time_t date                 = time_t(fields[2].GetUInt32());
            uint32 achievementID         = fields[3].GetUInt32();
            bool update = false;

            CriteriaTreeEntry const* criteriaTree = sAchievementMgr->GetAchievementCriteriaTree(acc_criteria_id);
            if (!criteriaTree)
            {
                // we will remove not existed criteria for all characters
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `character_achievement_progress`.", acc_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, acc_criteria_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(criteriaTree->criteria);
            if (!criteria)
            {
                // we will remove not existed criteria for all characters
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `character_achievement_progress`.", acc_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, acc_criteria_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            AchievementEntry const* achievement = NULL;
            if(!achievementID)
            {
                uint32 parent = sAchievementMgr->GetParantTreeId(criteriaTree->parent);
                achievement = GetsAchievementByTreeList(parent);
                update = true;
            }
            else
                achievement = sAchievementMgr->GetAchievement(achievementID);

            bool hasAchieve = !achievement || HasAchieved(achievement->ID);
            if (hasAchieve)
            {
                // we will remove already completed criteria
                //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "Achievement %s with progress acc_criteria_id %u data removed from table `account_achievement_progress` ", achievement ? "completed" : "not exist", acc_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ACC_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, acc_criteria_id);
                stmt->setUInt32(1, GetOwner()->GetSession()->GetAccountId());
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->timeLimit && time_t(date + criteria->timeLimit) < now)
                continue;

            CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

            if (!progressMap)
                continue;
            
            // Achievement in both account & characters achievement_progress, problem
            if (progressMap->find(acc_criteria_id) != progressMap->end())
            {
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Achievement '%u' in both account & characters achievement_progress", acc_criteria_id);
                continue;
            }
            
            CriteriaTreeProgress& progress = (*progressMap)[acc_criteria_id];
            progress.counter = fields[1].GetUInt32();
            progress.date    = date;
            progress.changed = false;
            progress.updated = update;
            progress.achievement = achievement;
            progress.criteriaTree = criteriaTree;
            progress.criteria = criteria;
        }
        while (criteriaAccountResult->NextRow());
    }
}

template<>
void AchievementMgr<Guild>::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult achievementAccountResult, PreparedQueryResult criteriaAccountResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].GetUInt32();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievementid];
            ca.date = time_t(fields[1].GetUInt32());
            Tokenizer guids(fields[2].GetString(), ' ');
            for (uint32 i = 0; i < guids.size(); ++i)
                ca.guids.insert(MAKE_NEW_GUID(atol(guids[i]), 0, HIGHGUID_PLAYER));

            ca.changed = false;
            _achievementPoints += achievement->points;

        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 guild_criteriaTree_id  = fields[0].GetUInt32();
            time_t date                   = time_t(fields[2].GetUInt32());
            uint64 guid                   = fields[3].GetUInt32();
            uint64 achievementID          = fields[4].GetUInt32();
            bool update = false;

            CriteriaTreeEntry const* criteriaTree = sAchievementMgr->GetAchievementCriteriaTree(guild_criteriaTree_id);
            if (!criteriaTree)
            {
                // we will remove not existed criteria for all guilds
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `guild_achievement_progress`.", guild_criteriaTree_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, guild_criteriaTree_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(criteriaTree->criteria);
            if (!criteria)
            {
                // we will remove not existed criteria for all guilds
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `guild_achievement_progress`.", guild_criteriaTree_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, guild_criteriaTree_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            AchievementEntry const* achievement = NULL;
            if(!achievementID)
            {
                uint32 parent = sAchievementMgr->GetParantTreeId(criteriaTree->parent);
                achievement = GetsAchievementByTreeList(parent);
                update = true;
            }
            else
                achievement = sAchievementMgr->GetAchievement(achievementID);

            bool hasAchieve = !achievement || HasAchieved(achievement->ID);
            if (hasAchieve)
                continue;

            if (criteria->timeLimit && time_t(date + criteria->timeLimit) < now)
                continue;

            CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

            if (!progressMap)
                continue;
            
            CriteriaTreeProgress& progress = (*progressMap)[guild_criteriaTree_id];
            progress.counter = fields[1].GetUInt32();
            progress.date    = date;
            progress.CompletedGUID = MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER);
            progress.changed = false;
            progress.updated = update;
            progress.achievement = achievement;
            progress.criteriaTree = criteriaTree;
            progress.criteria = criteria;
        } while (criteriaResult->NextRow());
    }
}

template<class T>
void AchievementMgr<T>::Reset()
{
}

template<>
void AchievementMgr<ScenarioProgress>::Reset()
{
    // FIXME
}

template<>
void AchievementMgr<Player>::Reset()
{
    for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_ACHIEVEMENT_DELETED, 4 + 4);
        data << uint32(iter->first);
        data << uint32(0);
        SendPacket(&data);
    }

    CriteriaProgressMap* criteriaProgress = GetCriteriaProgressMap();

    if (!criteriaProgress)
        return;

    for (CriteriaProgressMap::const_iterator iter = criteriaProgress->begin(); iter != criteriaProgress->end(); ++iter)
    {
        WorldPacket data(SMSG_CRITERIA_DELETED, 4);
        data << uint32(iter->second.criteriaTree->criteria);
        SendPacket(&data);
    }

    m_completedAchievements.clear();
    _achievementPoints = 0;
    criteriaProgress->clear();
    DeleteFromDB(GetOwner()->GetGUIDLow());

    // re-fill data
    CheckAllAchievementCriteria(GetOwner());
}

template<>
void AchievementMgr<Guild>::Reset()
{
    ObjectGuid guid = GetOwner()->GetGUID();
    for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DELETED, 4 + 4 + 8 + 1);
        data.WriteGuidMask<1, 3, 4, 2, 0, 5, 7, 6>(guid);

        data.WriteGuidBytes<0>(guid);
        data << uint32(iter->first);
        data.WriteGuidBytes<3, 4, 7, 1, 2>(guid);
        data << uint32(secsToTimeBitFields(iter->second.date));
        data.WriteGuidBytes<5, 6>(guid);
        SendPacket(&data);
    }

    CriteriaProgressMap* criteriaProgressMap = GetCriteriaProgressMap();

    if (!criteriaProgressMap)
        return;

    while (!criteriaProgressMap->empty())
        RemoveCriteriaProgress(criteriaProgressMap->begin()->second.criteriaTree);

    _achievementPoints = 0;
    m_completedAchievements.clear();
    DeleteFromDB(GetOwner()->GetId());
}

template<class T>
void AchievementMgr<T>::SendAchievementEarned(AchievementEntry const* achievement) const
{
    // Don't send for achievements with ACHIEVEMENT_FLAG_HIDDEN
    if (achievement->flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    if (Guild* guild = sGuildMgr->GetGuildById(GetOwner()->GetGuildId()))
    {
        Trinity::AchievementChatBuilder say_builder(*GetOwner(), CHAT_MSG_GUILD_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED, achievement->ID);
        Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> say_do(say_builder);
        guild->BroadcastWorker(say_do);
    }

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        ObjectGuid guid = GetOwner()->GetGUID();
        std::string name = GetOwner()->GetName();
        WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, strlen(GetOwner()->GetName()) + 1 + 8 + 4 + 4);
        data.WriteGuidMask<6, 4, 0, 1, 3, 7, 5>(guid);
        data.WriteBits(name.size(), 7);
        data.WriteBit(1);                                   // 0=link supplied string as player name, 1=display plain string
        data.WriteGuidMask<2>(guid);
        data.WriteGuidBytes<6, 1, 5>(guid);
        data.WriteString(name);
        data.WriteGuidBytes<3, 7, 4, 0, 2>(guid);
        data << uint32(achievement->ID);

        sWorld->SendGlobalMessage(&data);
    }
    // if player is in world he can tell his friends about new achievement
    else if (GetOwner()->IsInWorld())
    {
        Trinity::AchievementChatBuilder say_builder(*GetOwner(), CHAT_MSG_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED, achievement->ID);

        CellCoord p = Trinity::ComputeCellCoord(GetOwner()->GetPositionX(), GetOwner()->GetPositionY());

        Cell cell(p);
        cell.SetNoCreate();

        Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> say_do(say_builder);
        Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> > say_worker(GetOwner(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), say_do);
        TypeContainerVisitor<Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> >, WorldTypeMapContainer > message(say_worker);
        cell.Visit(p, message, *GetOwner()->GetMap(), *GetOwner(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
    }

    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    ObjectGuid thisPlayerGuid = GetOwner()->GetGUID();
    ObjectGuid firstPlayerOnAccountGuid = GetOwner()->GetGUID();

    if (HasAccountAchieved(achievement->ID))
        firstPlayerOnAccountGuid = GetFirstAchievedCharacterOnAccount(achievement->ID);

    data.WriteGuidMask<5, 1>(thisPlayerGuid);
    data.WriteGuidMask<1, 5, 7>(firstPlayerOnAccountGuid);
    data.WriteGuidMask<0, 6, 2>(thisPlayerGuid);
    data.WriteGuidMask<0>(firstPlayerOnAccountGuid);
    data.WriteGuidMask<7>(thisPlayerGuid);
    data.WriteGuidMask<3, 4, 2>(firstPlayerOnAccountGuid);
    data.WriteBit(0);                                            // Initial, some special effect with bit 0
    data.WriteGuidMask<4, 3>(thisPlayerGuid);
    data.WriteGuidMask<6>(firstPlayerOnAccountGuid);

    data.WriteGuidBytes<5>(thisPlayerGuid);
    data << uint32(realmID);
    data.WriteGuidBytes<2>(firstPlayerOnAccountGuid);
    data.WriteGuidBytes<3>(thisPlayerGuid);
    data.WriteGuidBytes<4>(firstPlayerOnAccountGuid);
    data << uint32(realmID);
    data.WriteGuidBytes<1, 0>(firstPlayerOnAccountGuid);
    data.WriteGuidBytes<7>(thisPlayerGuid);
    data << uint32(achievement->ID);
    data.WriteGuidBytes<2>(thisPlayerGuid);
    data.WriteGuidBytes<5>(firstPlayerOnAccountGuid);
    data << uint32(secsToTimeBitFields(time(NULL)));
    data.WriteGuidBytes<4>(thisPlayerGuid);
    data.WriteGuidBytes<7>(firstPlayerOnAccountGuid);
    data.WriteGuidBytes<0>(thisPlayerGuid);
    data.WriteGuidBytes<6, 3>(firstPlayerOnAccountGuid);
    data.WriteGuidBytes<1, 6>(thisPlayerGuid);

    GetOwner()->SendMessageToSetInRange(&data, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), true);
}

template<>
void AchievementMgr<ScenarioProgress>::SendAchievementEarned(AchievementEntry const* achievement) const
{
    // FIXME
}

template<>
void AchievementMgr<Guild>::SendAchievementEarned(AchievementEntry const* achievement) const
{
    ObjectGuid guid = GetOwner()->GetGUID();

    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_EARNED, 8 + 4 + 4 + 1);
    data.WriteGuidMask<6, 2, 7, 0, 1, 3, 5, 4>(guid);

    data.WriteGuidBytes<5, 2, 0, 6, 3>(guid);
    data << uint32(achievement->ID);
    data.WriteGuidBytes<1, 7, 4>(guid);
    data << uint32(secsToTimeBitFields(time(NULL)));

    SendPacket(&data);
}

template<class T>
void AchievementMgr<T>::SendCriteriaUpdate(CriteriaEntry const* /*entry*/, CriteriaTreeProgress const* /*progress*/, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{
}

template<class T>
void AchievementMgr<T>::SendAccountCriteriaUpdate(CriteriaEntry const* /*entry*/, CriteriaTreeProgress const* /*progress*/, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{
}

template<>
void AchievementMgr<Player>::SendCriteriaUpdate(CriteriaEntry const* criteria, CriteriaTreeProgress const* progress, uint32 timeElapsed, bool timedCompleted) const
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8 + 4 + 8);
    data << uint32(criteria->ID);
    data << uint32(secsToTimeBitFields(progress->date)); // Date
    data << uint32(timeElapsed);                    // TimeFromStart / TimeFromCreate, normally even 0
    data << uint64(progress->counter);

    if (!criteria->timeLimit)
        data << uint32(0);
    else
        data << uint32(timedCompleted ? 0 : 1);     // Flags, 1 is for keeping the counter at 0 in client
    data << uint32(timeElapsed);                    // TimeFromStart / TimeFromCreate, normally even 0

    ObjectGuid guid = GetOwner()->GetGUID();
    data.WriteGuidMask<3, 0, 7, 6, 2, 1, 4, 5>(guid);
    data.WriteGuidBytes<4, 7, 1, 2, 6, 0, 5, 3>(guid);

    SendPacket(&data);
}

template<>
void AchievementMgr<Player>::SendAccountCriteriaUpdate(CriteriaEntry const* criteria, CriteriaTreeProgress const* progress, uint32 timeElapsed, bool timedCompleted) const
{
    WorldPacket data(SMSG_ACCOUNT_CRITERIA_UPDATE);
    ObjectGuid guid = GetOwner()->GetGUID();         // needed send first completer criteria guid or else not found - then current player guid

    data.WriteGuidMask<6, 7>(progress->counter);
    data.WriteGuidMask<6>(guid);
    data.WriteGuidMask<1>(progress->counter);
    data.WriteBits(0, 4);                         // Flags
    data.WriteGuidMask<4, 5>(guid);
    data.WriteGuidMask<2, 5, 0>(progress->counter);
    data.WriteGuidMask<0, 3, 1, 2>(guid);
    data.WriteGuidMask<4, 3>(progress->counter);
    data.WriteGuidMask<7>(guid);

    data.WriteGuidBytes<0, 5, 3>(guid);
    data.WriteGuidBytes<7>(progress->counter);
    data.WriteGuidBytes<6, 1, 2>(guid);
    data.WriteGuidBytes<1>(progress->counter);
    data << uint32(0);
    data << uint32(0);
    data.WriteGuidBytes<2, 3>(progress->counter);
    data.WriteGuidBytes<4>(guid);
    data.WriteGuidBytes<6, 0, 4>(progress->counter);
    data << uint32(secsToTimeBitFields(progress->date));   // Date
    data.WriteGuidBytes<7>(guid);
    data << uint32(criteria->ID);
    data.WriteGuidBytes<5>(progress->counter);

    SendPacket(&data);
}

template<>
void AchievementMgr<ScenarioProgress>::SendCriteriaUpdate(CriteriaEntry const* entry, CriteriaTreeProgress const* progress, uint32 /*timeElapsed*/, bool timedCompleted) const
{
    // FIXME
    GetOwner()->SendCriteriaUpdate(entry->ID, progress->counter, progress->date);
    if (timedCompleted)
        GetOwner()->UpdateCurrentStep(false);
}

template<>
void AchievementMgr<Guild>::SendCriteriaUpdate(CriteriaEntry const* entry, CriteriaTreeProgress const* progress, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{
    //will send response to criteria progress request
    WorldPacket data(SMSG_GUILD_CRITERIA_UPDATE, 3 + 1 + 1 + 8 + 8 + 4 + 4 + 4 + 4 + 4);

    ObjectGuid counter = progress->counter; // for accessing every byte individually
    ObjectGuid guid = progress->CompletedGUID;

    data.WriteBits(1, 19);                 // Criteria count
    data.WriteGuidMask<4, 2, 6>(guid);
    data.WriteGuidMask<1, 5>(counter);
    data.WriteGuidMask<3>(guid);
    data.WriteGuidMask<2>(counter);
    data.WriteGuidMask<0, 5>(guid);
    data.WriteGuidMask<3>(counter);
    data.WriteGuidMask<1>(guid);
    data.WriteGuidMask<7>(counter);
    data.WriteGuidMask<7>(guid);
    data.WriteGuidMask<0, 6, 4>(counter);

    data.WriteGuidBytes<0>(counter);
    data << uint32(secsToTimeBitFields(progress->date)); // DateUpdated?
    data.WriteGuidBytes<2>(guid);
    data.WriteGuidBytes<1>(counter);
    data << uint32(0);                      // Flags
    data.WriteGuidBytes<7, 6>(counter);
    data.WriteGuidBytes<0>(guid);
    data << uint32(progress->date);         // DateStarted / DateCreated
    data.WriteGuidBytes<6, 7>(guid);
    data.WriteGuidBytes<4>(counter);
    data.WriteGuidBytes<5>(guid);
    data << uint32(entry->ID);
    data.WriteGuidBytes<4, 1>(guid);
    data << uint32(::time(NULL) - progress->date); // DateStarted / DateCreated
    data.WriteGuidBytes<5, 2>(counter);
    data.WriteGuidBytes<3>(guid);
    data.WriteGuidBytes<3>(counter);

    SendPacket(&data);
}

template<class T>
CriteriaProgressMap* AchievementMgr<T>::GetCriteriaProgressMap()
{
    return &m_criteriaProgress;
}

/**
 * called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
 */
template<class T>
void AchievementMgr<T>::CheckAllAchievementCriteria(Player* referencePlayer)
{
    // suppress sending packets
    for (uint32 i = 0; i < ACHIEVEMENT_CRITERIA_TYPE_TOTAL; ++i)
        UpdateAchievementCriteria(AchievementCriteriaTypes(i), 0, 0, 0, NULL, referencePlayer, true);
}

static const uint32 achievIdByArenaSlot[MAX_ARENA_SLOT] = {1057, 1107, 1108};
static const uint32 achievIdForDungeon[][4] =
{
    // ach_cr_id, is_dungeon, is_raid, is_heroic_dungeon
    { 321,       true,      true,   true  },
    { 916,       false,     true,   false },
    { 917,       false,     true,   false },
    { 918,       true,      false,  false },
    { 2219,      false,     false,  true  },
    { 0,         false,     false,  false }
};

/**
 * this function will be called whenever the user might have done a criteria relevant action
 */
template<class T>
void AchievementMgr<T>::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 /*= 0*/, uint32 miscValue2 /*= 0*/, uint32 miscValue3 /*= 0*/,Unit const* unit /*= NULL*/, Player* referencePlayer /*= NULL*/, bool init /*=false*/)
{
    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "UpdateAchievementCriteria(%u, %u, %u) CriteriaSort %u", type, miscValue1, miscValue2, miscValue3, GetCriteriaSort());

    // disable for gamemasters with GM-mode enabled
    if (referencePlayer->isGameMaster())
        return;

     // Lua_GetGuildLevelEnabled() is checked in achievement UI to display guild tab
    if (GetCriteriaSort() == GUILD_CRITERIA && !sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        return;

    CriteriaTreeList const& criteriaTreeList = sAchievementMgr->GetCriteriaTreeByType(type, GetCriteriaSort());
    if(criteriaTreeList.empty())
        return;

    for (CriteriaTreeList::const_iterator i = criteriaTreeList.begin(); i != criteriaTreeList.end(); ++i)
    {
        bool canComplete = false;
        CriteriaTreeEntry const* criteriaTree = i->criteriaTree;
        CriteriaEntry const* criteria = i->criteria;
        AchievementEntry const* achievement = i->achievement;
        if(!criteriaTree || !criteria)
            continue;

        if (GetCriteriaSort() != SCENARIO_CRITERIA && !achievement)
        {
            sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "UpdateAchievementCriteria: Achievement for criteriaTree->ID %u not found!", criteriaTree->ID);
            continue;
        }

        if (!CanUpdateCriteria(criteriaTree, criteria, achievement, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
            continue;

        // requirements not found in the dbc
        if (AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(criteria))
            if (!data->Meets(referencePlayer, unit, miscValue1))
                continue;

        switch (type)
        {
            // std. case: increment at 1
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
            case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
            case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
            case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:    /* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
            case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
            case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
            case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
            case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            case ACHIEVEMENT_CRITERIA_TYPE_INSTANSE_MAP_ID:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT:
            case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
            case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            //case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
            case ACHIEVEMENT_CRITERIA_TYPE_OBTAIN_BATTLEPET:
            case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
            case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
            case ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
            case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN:
            case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, init ? 0 : 1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            // std case: increment at miscValue1
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:/* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
            case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
            case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue2, referencePlayer, PROGRESS_ACCUMULATE);
                break;
                // std case: high value at miscValue1
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD: /* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer, PROGRESS_HIGHEST);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->getLevel(), referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:                
                if (uint32 skillvalue = referencePlayer->GetBaseSkillValue(criteria->reach_skill_level.skillID))
                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, skillvalue, referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:             
                if (uint32 maxSkillvalue = referencePlayer->GetPureMaxSkillValue(criteria->learn_skill_level.skillID))
                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, maxSkillvalue, referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetRewardedQuestCount(), referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            {
                time_t nextDailyResetTime = sWorld->GetNextDailyQuestsResetTime();
                CriteriaTreeProgress *progress = GetCriteriaProgress(criteriaTree);

                if (!miscValue1) // Login case.
                {
                    // reset if player missed one day.
                    if (progress && progress->date < (nextDailyResetTime - 2 * DAY))
                        canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, 0, referencePlayer, PROGRESS_SET);
                    continue;
                }

                ProgressType progressType;
                if (!progress)
                    // 1st time. Start count.
                    progressType = PROGRESS_SET;
                else if (progress->date < (nextDailyResetTime - 2 * DAY))
                   // last progress is older than 2 days. Player missed 1 day => Restart count.
                    progressType = PROGRESS_SET;
                else if (progress->date < (nextDailyResetTime - DAY))
                    // last progress is between 1 and 2 days. => 1st time of the day.
                    progressType = PROGRESS_ACCUMULATE;
                else
                    // last progress is within the day before the reset => Already counted today.
                    continue;

                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, 1, referencePlayer, progressType);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            {

                uint32 counter = 0;

                const RewardedQuestSet &rewQuests = referencePlayer->getRewardedQuests();
                for (RewardedQuestSet::const_iterator itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
                {
                    Quest const* quest = sObjectMgr->GetQuestTemplate(*itr);
                    if (quest && quest->GetZoneOrSort() >= 0 && uint32(quest->GetZoneOrSort()) == criteria->complete_quests_in_zone.zoneID)
                        ++counter;
                }
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, counter, referencePlayer);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                // miscValue1 is the ingame fallheight*100 as stored in dbc
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, 1, referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetBankBagSlotCount(), referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
                {
                    int32 reputation = referencePlayer->GetReputationMgr().GetReputation(criteria->gain_reputation.factionID);
                    if (reputation > 0)
                        canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, reputation, referencePlayer);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetReputationMgr().GetExaltedFactionCount(), referencePlayer);
                  break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            {
                    uint32 spellCount = 0;
                    for (PlayerSpellMap::const_iterator spellIter = referencePlayer->GetSpellMap().begin();
                        spellIter != referencePlayer->GetSpellMap().end();
                        ++spellIter)

                    {
                        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellIter->first);
                        for (SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                        {
                            if (skillIter->second->skillId == criteria->learn_skillline_spell.skillLine)
                                spellCount++;
                        }
                    }
                   
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, spellCount, referencePlayer);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetReputationMgr().GetReveredFactionCount(), referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetReputationMgr().GetHonoredFactionCount(), referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetReputationMgr().GetVisibleFactionCount(), referencePlayer);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            {
                    uint32 spellCount = 0;
                    for (PlayerSpellMap::const_iterator spellIter = referencePlayer->GetSpellMap().begin();
                        spellIter != referencePlayer->GetSpellMap().end();
                        ++spellIter)
                    {
                        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellIter->first);
                        for (SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                            if (skillIter->second->skillId == criteria->learn_skill_line.skillLine)
                                spellCount++;   
                    }

                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, spellCount, referencePlayer);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                if (!miscValue1)
                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS), referencePlayer, PROGRESS_HIGHEST);
                else
                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetMoney(), referencePlayer, PROGRESS_HIGHEST);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                if (!miscValue1)
                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, _achievementPoints, referencePlayer, PROGRESS_SET);
                else
                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                {
                    uint32 reqTeamType = criteria->highest_team_rating.teamtype;

                    if (miscValue1)
                    {
                        if (miscValue2 != reqTeamType)
                            continue;
                        canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer, PROGRESS_HIGHEST);
                    }
                    else // login case
                    {
                        for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
                        {
                            Bracket* bracket = referencePlayer->getBracket(BracketType(arena_slot));
                            if (!bracket || arena_slot != reqTeamType)
                                continue;
                            canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, bracket->getRating(), referencePlayer, PROGRESS_HIGHEST);
                            break;
                        }
                    }
                }
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            {
                uint32 reqTeamType = criteria->highest_personal_rating.teamtype;

                if (miscValue1)
                {
                    if (miscValue2 != reqTeamType)
                        continue;

                    canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, miscValue1, referencePlayer, PROGRESS_HIGHEST);
                }
                
                else // login case
                {
                    for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
                    {
                        Bracket* bracket = referencePlayer->getBracket(BracketType(arena_slot));
                        if (!bracket || arena_slot != reqTeamType)
                            continue;
                        canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, bracket->getRating(), referencePlayer, PROGRESS_HIGHEST);
                    }
                }

                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            {
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetGuildLevel(), referencePlayer, PROGRESS_SET);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET:
                canComplete = SetCriteriaProgress(achievement, criteriaTree, criteria, referencePlayer->GetBattlePetMgr()->GetPetJournal().size(), referencePlayer);
                break;
            // FIXME: not triggered in code as result, need to implement
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETED_LFG_DUNGEONS:
            case ACHIEVEMENT_CRITERIA_TYPE_INITIATED_KICK_IN_LFG:
            case ACHIEVEMENT_CRITERIA_TYPE_VOTED_KICK_IN_LFG:
            case ACHIEVEMENT_CRITERIA_TYPE_BEING_KICKED_IN_LFG:
            case ACHIEVEMENT_CRITERIA_TYPE_ABANDONED_LFG_DUNGEONS:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK137:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_GUILD_DUNGEON_CHALLENGES:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_GUILD_CHALLENGES:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK140:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK141:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK142:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK143:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK144:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETED_LFR_DUNGEONS:
            case ACHIEVEMENT_CRITERIA_TYPE_ABANDONED_LFR_DUNGEONS:
            case ACHIEVEMENT_CRITERIA_TYPE_INITIATED_KICK_IN_LFR:
            case ACHIEVEMENT_CRITERIA_TYPE_VOTED_KICK_IN_LFR:
            case ACHIEVEMENT_CRITERIA_TYPE_BEING_KICKED_IN_LFR:
            case ACHIEVEMENT_CRITERIA_TYPE_COUNT_OF_LFR_QUEUE_BOOSTS_BY_TANK:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_SCENARIOS:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_SCENARIOS_SATURDAY:
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SCENARIO_BOSS:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK154:
            case ACHIEVEMENT_CRITERIA_TYPE_UNK159:
            case ACHIEVEMENT_CRITERIA_TYPE_LEVEL_BATTLE_PET_CREDIT:
                break;                                   // Not implemented yet :(
        }

        if (!achievement || !canComplete)
            continue;

        if (IsCompletedCriteria(criteriaTree, achievement, criteria))
            CompletedCriteriaFor(achievement, referencePlayer);

        // check again the completeness for SUMM and REQ COUNT achievements,
        // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
        if (achievement->flags & ACHIEVEMENT_FLAG_SUMM)
            if (IsCompletedAchievement(achievement, referencePlayer))
                CompletedAchievement(achievement, referencePlayer);

        if (AchievementEntryList const* achRefList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (IsCompletedAchievement(*itr, referencePlayer))
                    CompletedAchievement(*itr, referencePlayer);
    }
}

template<class T>
bool AchievementMgr<T>::CanCompleteCriteria(AchievementEntry const* achievement)
{
    return true;
}

template<>
bool AchievementMgr<Player>::CanCompleteCriteria(AchievementEntry const* achievement)
{
    if (achievement && achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr->IsRealmCompleted(achievement))
            return false;

        if (GetOwner())
            if (GetOwner()->GetSession())
                if (GetOwner()->GetSession()->GetSecurity())
                    return false;
    }

    return true;
}

template<class T>
bool AchievementMgr<T>::IsCompletedCriteria(CriteriaTreeEntry const* criteriaTree, AchievementEntry const* achievement, CriteriaEntry const* criteria)
{
    // counter can never complete
    if (achievement && achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (!CanCompleteCriteria(achievement))
        return false;

    if(CriteriaTreeEntry const* pTree = sCriteriaTreeStore.LookupEntry(criteriaTree->parent))
        if((pTree->flags & ACHIEVEMENT_CRITERIA_FLAG_SHOW_PROGRESS_BAR) || (criteriaTree->flags & ACHIEVEMENT_CRITERIA_FLAG_HIDDEN))
            return IsCompletedCriteriaTree(pTree, achievement);
    /*if(CriteriaTreeEntry const* pTree = sCriteriaTreeStore.LookupEntry(sAchievementMgr->GetParantTreeId(criteriaTree->parent)))
    {
        if(pTree->requirement_count > 0)
        {
            uint32 checkCount = 0;
            checkCount += IsCompletedCriteriaTreeCounter(pTree, achievement);
            //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteria pTree %u, requirement_count %u checkCount %i check %u criteriaTree %i criteria %i", pTree->ID, pTree->requirement_count, checkCount, checkCount >= pTree->requirement_count, criteriaTree->ID, criteria->ID);
            return checkCount >= pTree->requirement_count;
        }
    }*/

    if (criteriaTree->criteria == 0 || !criteria)
    {
        if(criteriaTree->parent == 0)
            return false;

        //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteria criteriaTree %u, achievement %u ", criteriaTree->ID, achievement ? achievement->ID : 0);

        return IsCompletedCriteriaTree(criteriaTree, achievement);
    }

    CriteriaTreeProgress const* progress = GetCriteriaProgress(criteriaTree);
    if (!progress)
        return false;

    switch (criteria->type)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            return progress->counter >= (criteriaTree->requirement_count * 75); // skillLevel * 75
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
        case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
        //case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
        case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
        case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
        case ACHIEVEMENT_CRITERIA_TYPE_INSTANSE_MAP_ID:
        case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
        case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT:
        case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2:
        case ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
        case ACHIEVEMENT_CRITERIA_TYPE_OBTAIN_BATTLEPET:
        case ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET:
        case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
        case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN:
        case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
        case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
            return progress->counter >= criteriaTree->requirement_count;
        // handle all statistic-only criteria here
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
        default:
            break;
    }
    return false;
}

template<class T>
uint32 AchievementMgr<T>::IsCompletedCriteriaTreeCounter(CriteriaTreeEntry const* criteriaTree, AchievementEntry const* achievement)
{
    uint32 count = 0;
    int32 saveType = -1;
    bool saveCheck = false;
    std::vector<CriteriaTreeEntry const*> const* cTreeList = GetCriteriaTreeList(criteriaTree->ID);
    if(!cTreeList)
        return count;

    for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cTreeList->begin(); itr != cTreeList->end(); ++itr)
    {
        CriteriaTreeEntry const* cTree = *itr;
        if(cTree->criteria == 0)
            return IsCompletedCriteriaTreeCounter(cTree, achievement);
        else
        {
            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(cTree->criteria);
            CriteriaTreeProgress const* progress = GetCriteriaProgress(cTree);
            if(!progress || !criteria)
                continue;

            bool check = false;
            int32 const reqType = criteria->type;
            if(saveType == reqType || saveType == -1)
                count += progress->counter;
            else
                count = progress->counter;

            //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTreeCounter cTree %u, count %u ", cTree->ID, count);
            switch (criteria->type)
            {
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
                    check = count >= (criteriaTree->requirement_count * 75); // skillLevel * 75
                    break;
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
                    check = count >= 1;
                    break;
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
                case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
                case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
                case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
                case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
                case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
                case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
                case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
                case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
                case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
                case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
                case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
                case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
                case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
                case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
                case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
                case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
                case ACHIEVEMENT_CRITERIA_TYPE_INSTANSE_MAP_ID:
                case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT:
                case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2:
                case ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
                case ACHIEVEMENT_CRITERIA_TYPE_OBTAIN_BATTLEPET:
                case ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET:
                case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
                case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN:
                case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
                case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
                    check = count >= criteriaTree->requirement_count;
                    break;
                // handle all statistic-only criteria here
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
                case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
                case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
                case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
                case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
                case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
                case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
                    break;
                default:
                    break;
            }
            if(saveType == -1)
            {
                if(check)
                    saveType = reqType;
                saveCheck = check;
            }
            else if(saveType != reqType)
            {
                if(!saveCheck || !check)
                {
                    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTreeCounter criteriaTree %u, check %u saveCheck %i count %i requirement_count %i", criteriaTree->ID, check, saveCheck, count, criteriaTree->requirement_count);
                    return 0;
                }
            }
            else if(saveType == reqType)
            {
                if(saveCheck || check)
                {
                    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTreeCounter criteriaTree %u, check %u saveCheck %i count %i requirement_count %i", criteriaTree->ID, check, saveCheck, count, criteriaTree->requirement_count);
                    return 1;
                }
            }
        }
    }

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTreeCounter criteriaTree %u, achievement %u saveCheck %i count %i requirement_count %i", criteriaTree->ID, achievement ? achievement->ID : 0, saveCheck, count, criteriaTree->requirement_count);
    return saveCheck ? 1 : 0;
}

template<class T>
bool AchievementMgr<T>::IsCompletedCriteriaTree(CriteriaTreeEntry const* criteriaTree, AchievementEntry const* achievement)
{
    uint32 count = 0;
    int32 saveType = -1;
    bool saveCheck = false;
    std::vector<CriteriaTreeEntry const*> const* cTreeList = GetCriteriaTreeList(criteriaTree->ID);
    if(!cTreeList)
        return false;

    for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cTreeList->begin(); itr != cTreeList->end(); ++itr)
    {
        CriteriaTreeEntry const* cTree = *itr;
        if(cTree->criteria == 0)
        {
            if(!IsCompletedCriteriaTree(cTree, achievement))
                return false;
        }
        else
        {
            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(cTree->criteria);
            CriteriaTreeProgress const* progress = GetCriteriaProgress(cTree);
            if(!progress || !criteria)
                continue;

            bool check = false;
            int32 const reqType = criteria->type;
            if(saveType == reqType || saveType == -1)
                count += progress->counter;
            else
                count = progress->counter;

            //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTree cTree %u, count %u ", cTree->ID, count);
            switch (criteria->type)
            {
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
                    check = count >= (criteriaTree->requirement_count * 75); // skillLevel * 75
                    break;
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
                    check = count >= 1;
                    break;
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
                case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
                case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
                case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
                case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
                case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
                case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
                case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
                case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
                case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
                case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
                case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
                case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
                case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
                case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
                case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
                case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
                case ACHIEVEMENT_CRITERIA_TYPE_INSTANSE_MAP_ID:
                case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT:
                case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2:
                case ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
                case ACHIEVEMENT_CRITERIA_TYPE_OBTAIN_BATTLEPET:
                case ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET:
                case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
                case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN:
                case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
                case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
                    check = count >= criteriaTree->requirement_count;
                    break;
                // handle all statistic-only criteria here
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
                case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
                case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
                case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
                case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
                case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
                case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
                    break;
                default:
                    break;
            }
            if(saveType == -1)
            {
                if(check)
                    saveType = reqType;
                saveCheck = check;
            }
            else if(saveType != reqType)
            {
                if(!saveCheck || !check)
                {
                    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTree criteriaTree %u, check %u saveCheck %i count %i requirement_count %i", criteriaTree->ID, check, saveCheck, count, criteriaTree->requirement_count);
                    return false;
                }
            }
            else if(saveType == reqType)
            {
                if(saveCheck || check)
                {
                    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTree criteriaTree %u, check %u saveCheck %i count %i requirement_count %i", criteriaTree->ID, check, saveCheck, count, criteriaTree->requirement_count);
                    return true;
                }
            }
        }
    }

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedCriteriaTree criteriaTree %u, achievement %u saveCheck %i count %i requirement_count %i", criteriaTree->ID, achievement ? achievement->ID : 0, saveCheck, count, criteriaTree->requirement_count);
    return saveCheck;
}

template<class T>
bool AchievementMgr<T>::IsCompletedScenarioTree(CriteriaTreeEntry const* criteriaTree)
{
    bool check = false;
    std::vector<CriteriaTreeEntry const*> const* cTreeList = GetCriteriaTreeList(criteriaTree->ID);
    if(!cTreeList)
        return false;

    for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cTreeList->begin(); itr != cTreeList->end(); ++itr)
    {
        CriteriaTreeEntry const* cTree = *itr;
        if(cTree->criteria == 0)
        {
            if(!IsCompletedScenarioTree(cTree))
                return false;
        }
        else
        {
            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(cTree->criteria);
            CriteriaTreeProgress const* progress = GetCriteriaProgress(cTree);
            if(!progress || !criteria)
                return false;

            switch (criteria->type)
            {
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
                    check = progress->counter >= (criteriaTree->requirement_count * 75); // skillLevel * 75
                    break;
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
                    check = progress->counter >= 1;
                    break;
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
                case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
                case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
                case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
                case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
                case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
                case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
                case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
                case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
                case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
                case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
                case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
                case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
                case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
                case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
                case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
                case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
                case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
                case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
                case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
                case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
                case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
                case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
                case ACHIEVEMENT_CRITERIA_TYPE_INSTANSE_MAP_ID:
                case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
                case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
                case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT:
                case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2:
                case ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
                case ACHIEVEMENT_CRITERIA_TYPE_OBTAIN_BATTLEPET:
                case ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET:
                case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
                case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN:
                case ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
                case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
                    check = progress->counter >= criteriaTree->requirement_count;
                    break;
                // handle all statistic-only criteria here
                case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
                case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
                case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
                case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
                case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
                case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
                case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
                case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
                case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
                case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
                    break;
                default:
                    break;
            }

            //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedScenarioTree cTree %u, criteria %u check %i counter %i requirement_count %i", cTree->ID, cTree->criteria, check, progress->counter, criteriaTree->requirement_count);
            if(!check)
                return check;
        }
    }

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "IsCompletedScenarioTree criteriaTree %u, check %i", criteriaTree->ID, check);
    return check;
}

template<class T>
void AchievementMgr<T>::CompletedCriteriaFor(AchievementEntry const* achievement, Player* referencePlayer)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    if (IsCompletedAchievement(achievement, referencePlayer))
        CompletedAchievement(achievement, referencePlayer);
}

template<class T>
bool AchievementMgr<T>::IsCompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    // already completed and stored
    if (CanDeleteOrUpdateCreteria(achievement->ID, referencePlayer->GetGUIDLow()))
        return false;

    std::vector<CriteriaTreeEntry const*> const* cList = GetCriteriaTreeList(achievement->criteriaTree);
    if (!cList)
        return false;
    uint32 count = 0;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT contained in the achievement, but in each individual criteria
    if (achievement->flags & ACHIEVEMENT_FLAG_SUMM)
    {
        for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
        {
            CriteriaTreeEntry const* criteriaTree = *itr;

            CriteriaTreeProgress const* progress = GetCriteriaProgress(criteriaTree);
            if (!progress)
                continue;

            count += progress->counter;

            // for counters, field4 contains the main count requirement
            if (count >= criteriaTree->requirement_count)
                return true;
        }
        return false;
    }

    // Default case - need complete all or
    bool completed_all = true;
    for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = *itr;

        bool completed = IsCompletedCriteria(criteriaTree, achievement, criteriaTree->criteria ? sCriteriaStore.LookupEntry(criteriaTree->criteria) : NULL);

        // found an uncompleted criteria, but DONT return false yet - there might be a completed criteria with ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL
        if (completed)
        {
            ++count;
            if (achievement->flags & ACHIEVEMENT_FLAG_BAR) //achievement complete if completed one criteria
                return true;
        }
        else
            completed_all = false;

        // completed as have req. count of completed criterias
        if (achievement->count > 0 && achievement->count <= count)
           return true;
    }

    // all criterias completed requirement
    if (completed_all && achievement->count == 0)
        return true;

    return false;
}

template<class T>
CriteriaTreeProgress* AchievementMgr<T>::GetCriteriaProgress(uint32 entry)
{
    CriteriaProgressMap* criteriaProgressMap = GetCriteriaProgressMap();

    if (!criteriaProgressMap)
        return NULL;

    CriteriaProgressMap::iterator iter = criteriaProgressMap->find(entry);

    if (iter == criteriaProgressMap->end())
        return NULL;

    return &(iter->second);
}

template<class T>
CriteriaTreeProgress* AchievementMgr<T>::GetCriteriaProgress(CriteriaTreeEntry const* entry)
{
   CriteriaProgressMap* criteriaProgressMap = GetCriteriaProgressMap();

    if (!criteriaProgressMap)
        return NULL;

    CriteriaProgressMap::iterator iter = criteriaProgressMap->find(entry->ID);

    if (iter == criteriaProgressMap->end())
        return NULL;

    return &(iter->second);
}

template<class T>
bool AchievementMgr<T>::SetCriteriaProgress(AchievementEntry const* achievement, CriteriaTreeEntry const* treeEntry, CriteriaEntry const* criteria, uint32 changeValue, Player* referencePlayer, ProgressType ptype)
{
    // Don't allow to cheat - doing timed achievements without timer active
    TimedAchievementMap::iterator timedIter = m_timedAchievements.find(treeEntry->ID);
    if (criteria->timeLimit && timedIter == m_timedAchievements.end())
        return false;

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "SetCriteriaProgress(%u, %u) CriteriaSort %u achievement %u", criteria->ID, changeValue, GetCriteriaSort(), achievement ? achievement->ID : 0);

    CriteriaTreeProgress* progress = GetCriteriaProgress(treeEntry);
    if (!progress)
    {
        // not create record for 0 counter but allow it for timed achievements
        // we will need to send 0 progress to client to start the timer
        if (changeValue == 0 && !criteria->timeLimit)
            return false;

        CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

        if (!progressMap)
            return false;

        progress = &(*progressMap)[treeEntry->ID];
        progress->counter = changeValue;
        progress->changed = true;
        progress->achievement = achievement;
        progress->criteriaTree = treeEntry;
        progress->criteria = criteria;
    }
    else
    {
        uint32 newValue = 0;
        switch (ptype)
        {
            case PROGRESS_SET:
                newValue = changeValue;
                break;
            case PROGRESS_ACCUMULATE:
            {
                // avoid overflow
                uint32 max_value = std::numeric_limits<uint32>::max();
                newValue = max_value - progress->counter > changeValue ? progress->counter + changeValue : max_value;
                break;
            }
            case PROGRESS_HIGHEST:
                newValue = progress->counter < changeValue ? changeValue : progress->counter;
                break;
        }

        // not update (not mark as changed) if counter will have same value
        if (progress->counter == newValue && !criteria->timeLimit)
            return false;

        progress->counter = newValue;
        progress->updated = true;
    }

    progress->date = time(NULL); // set the date to the latest update.

    //if (!achievement)
        //return;

    uint32 timeElapsed = 0;
    bool criteriaComplete = IsCompletedCriteria(treeEntry, achievement, criteria);

    if (criteria->timeLimit)
    {
        // Client expects this in packet
        timeElapsed = criteria->timeLimit - (timedIter->second/IN_MILLISECONDS);

        // Remove the timer, we wont need it anymore
        if (criteriaComplete)
            m_timedAchievements.erase(timedIter);
    }

    if (criteriaComplete && achievement && achievement->flags & ACHIEVEMENT_FLAG_SHOW_CRITERIA_MEMBERS && !progress->CompletedGUID)
        progress->CompletedGUID = referencePlayer->GetGUID();

    if (achievement && achievement->parent && !HasAchieved(achievement->parent)) //Don`t send update criteria to client if parent achievment not complete
        return false;

    if (achievement && achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
        SendAccountCriteriaUpdate(criteria, progress, timeElapsed, criteriaComplete);
    else
        SendCriteriaUpdate(criteria, progress, timeElapsed, criteriaComplete);

    return true;
}

template<class T>
void AchievementMgr<T>::UpdateTimedAchievements(uint32 timeDiff)
{
    if (!m_timedAchievements.empty())
    {
        for (TimedAchievementMap::iterator itr = m_timedAchievements.begin(); itr != m_timedAchievements.end();)
        {
            // Time is up, remove timer and reset progress
            if (itr->second <= timeDiff)
            {
                CriteriaTreeEntry const* entry = sAchievementMgr->GetAchievementCriteriaTree(itr->first);
                RemoveCriteriaProgress(entry);
                m_timedAchievements.erase(itr++);
            }
            else
            {
                itr->second -= timeDiff;
                ++itr;
            }
        }
    }
}

template<class T>
void AchievementMgr<T>::StartTimedAchievement(AchievementCriteriaTimedTypes /*type*/, uint32 /*entry*/, uint32 /*timeLost = 0*/)
{
}

template<>
void AchievementMgr<Player>::StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry, uint32 timeLost /* = 0 */)
{
    CriteriaTreeList const& criteriaTreeList = sAchievementMgr->GetTimedCriteriaTreeByType(type);
    for (CriteriaTreeList::const_iterator i = criteriaTreeList.begin(); i != criteriaTreeList.end(); ++i)
    {
        CriteriaTreeEntry const* criteriaTree = i->criteriaTree;
        CriteriaEntry const* criteria = i->criteria;
        AchievementEntry const* achievement = i->achievement;

        if (criteria->timedCriteriaMiscId != entry)
            continue;

        if (m_timedAchievements.find(criteriaTree->ID) == m_timedAchievements.end() && !IsCompletedCriteria(criteriaTree, achievement, criteria))
        {
            // Start the timer
            if (criteria->timeLimit * IN_MILLISECONDS > timeLost)
            {
                m_timedAchievements[criteriaTree->ID] = criteria->timeLimit * IN_MILLISECONDS - timeLost;

                // and at client too
                SetCriteriaProgress(achievement, criteriaTree, criteria, 0, GetOwner(), PROGRESS_SET);
            }
        }
    }
}

template<class T>
void AchievementMgr<T>::RemoveTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry)
{
    CriteriaTreeList const& criteriaTreeList = sAchievementMgr->GetTimedCriteriaTreeByType(type);
    for (CriteriaTreeList::const_iterator i = criteriaTreeList.begin(); i != criteriaTreeList.end(); ++i)
    {
        CriteriaTreeEntry const* criteriaTree = i->criteriaTree;
        CriteriaEntry const* achievementCriteria = i->criteria;

        if (achievementCriteria->timedCriteriaMiscId != entry)
            continue;

        TimedAchievementMap::iterator timedIter = m_timedAchievements.find(criteriaTree->ID);
        // We don't have timer for this achievement
        if (timedIter == m_timedAchievements.end())
            continue;

        // remove progress
        RemoveCriteriaProgress(criteriaTree);

        // Remove the timer
        m_timedAchievements.erase(timedIter);
    }
}

template<class T>
void AchievementMgr<T>::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    // disable for gamemasters with GM-mode enabled
    if (GetOwner()->isGameMaster())
        return;

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = sGuildMgr->GetGuildById(referencePlayer->GetGuildId()))
            guild->GetNewsLog().AddNewEvent(GUILD_NEWS_PLAYER_ACHIEVEMENT, time(NULL), referencePlayer->GetGUID(), achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    if (!GetOwner()->GetSession()->PlayerLoading())
        SendAchievementEarned(achievement);

    CompletedAchievementData& ca = m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.first_guid = GetOwner()->GetGUIDLow();
    ca.changed = true;

    // don't insert for ACHIEVEMENT_FLAG_REALM_FIRST_KILL since otherwise only the first group member would reach that achievement
    // TODO: where do set this instead?
    if (!(achievement->flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    _achievementPoints += achievement->points;

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, 0, NULL, referencePlayer);
    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->points, 0, 0, NULL, referencePlayer);

    // reward items and titles if any
    AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement);

    // no rewards
    if (!reward)
        return;

    //! Custom reward handlong
    sScriptMgr->OnRewardCheck(reward, GetOwner());

    // titles
    //! Currently there's only one achievement that deals with gender-specific titles.
    //! Since no common attributes were found, (not even in titleRewardFlags field)
    //! we explicitly check by ID. Maybe in the future we could move the achievement_reward
    //! condition fields to the condition system.
    if (uint32 titleId = reward->titleId[achievement->ID == 1793 ? GetOwner()->getGender() : (GetOwner()->GetTeam() == ALLIANCE ? 0 : 1)])
        if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            GetOwner()->SetTitle(titleEntry);

    // mail
    if (reward->sender)
    {
        uint32 itemID = sScriptMgr->OnSelectItemReward(reward, GetOwner());
        if (!itemID)
            itemID = reward->itemId;   //OnRewardSelectItem return 0 if no script

        Item* item = itemID ? Item::CreateItem(itemID, 1, GetOwner()) : NULL;

        int loc_idx = GetOwner()->GetSession()->GetSessionDbLocaleIndex();

        // subject and text
        std::string subject = reward->subject;
        std::string text = reward->text;
        if (loc_idx >= 0)
        {
            if (AchievementRewardLocale const* loc = sAchievementMgr->GetAchievementRewardLocale(achievement))
            {
                ObjectMgr::GetLocaleString(loc->subject, loc_idx, subject);
                ObjectMgr::GetLocaleString(loc->text, loc_idx, text);
            }
        }

        MailDraft draft(subject, text);

        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        if (item)
        {
            // save new item before send
            item->SaveToDB(trans);                               // save for prevent lost at next mail load, if send fail then item will deleted

            // item
            draft.AddItem(item);
            if(item->GetEntry() == 38186)
                sLog->outDebug(LOG_FILTER_EFIR, "CompletedAchievement - CreateItem of item %u; count = %u playerGUID %u, itemGUID %u", item->GetEntry(), 1, GetOwner()->GetGUID(), item->GetGUID());
        }

        draft.SendMailTo(trans, GetOwner(), MailSender(MAIL_CREATURE, reward->sender));
        CharacterDatabase.CommitTransaction(trans);
    }

    if (reward->learnSpell)
        GetOwner()->learnSpell(reward->learnSpell, false);
}

template<>
void AchievementMgr<ScenarioProgress>::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    // not needed
}

template<>
void AchievementMgr<Guild>::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = sGuildMgr->GetGuildById(referencePlayer->GetGuildId()))
            guild->GetNewsLog().AddNewEvent(GUILD_NEWS_GUILD_ACHIEVEMENT, time(NULL), 0, achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    SendAchievementEarned(achievement);
    CompletedAchievementData& ca = m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.changed = true;

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_GUILD_MEMBERS)
    {
        if (referencePlayer->GetGuildId() == GetOwner()->GetId())
            ca.guids.insert(referencePlayer->GetGUID());

        if (Group const* group = referencePlayer->GetGroup())
            for (GroupReference const* ref = group->GetFirstMember(); ref != NULL; ref = ref->next())
                if (Player const* groupMember = ref->getSource())
                    if (groupMember->GetGuildId() == GetOwner()->GetId())
                        ca.guids.insert(groupMember->GetGUID());
    }

    sAchievementMgr->SetRealmCompleted(achievement);

    _achievementPoints += achievement->points;

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, 0, NULL, referencePlayer);
    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->points, 0, 0, NULL, referencePlayer);
    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS, achievement->points, 0, 0, NULL, referencePlayer);
}

struct VisibleAchievementPred
{
    bool operator()(CompletedAchievementMap::value_type const& val) const
    {
        AchievementEntry const* achievement = sAchievementMgr->GetAchievement(val.first);
        return achievement && !(achievement->flags & ACHIEVEMENT_FLAG_HIDDEN);
    }
};

template<class T>
void AchievementMgr<T>::SendAllAchievementData(Player* /*receiver*/)
{
    const CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

    if (!progressMap)
        return;

    VisibleAchievementPred isVisible;
    size_t numAchievements = std::count_if(m_completedAchievements.begin(), m_completedAchievements.end(), isVisible);
    uint32 criteriaCount = 0;
    ObjectGuid guid = GetOwner()->GetGUID();

    WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA);

    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 19);
    data.WriteBits(numAchievements, 20);

    for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;
        if (!criteriaTree)
            continue;

        AchievementEntry const* achievement = itr->second.achievement;

        // account criteria send in other packet
        if (!achievement || (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT))
            continue;

        ObjectGuid counter = uint64(itr->second.counter);

        data.WriteGuidMask<5>(counter);
        data.WriteBits(0, 4);            // Flags
        data.WriteGuidMask<6>(guid);
        data.WriteGuidMask<2>(counter);
        data.WriteGuidMask<3, 2>(guid);
        data.WriteGuidMask<4>(counter);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<3, 7, 0, 1>(counter);
        data.WriteGuidMask<5, 0, 7>(guid);
        data.WriteGuidMask<6>(counter);
        data.WriteGuidMask<4>(guid);
    }

    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        if (!isVisible(*itr))
            continue;

        data.WriteGuidMask<3, 0, 6, 5, 2, 7, 4, 1>(itr->second.first_guid);
    }

    data.FlushBits();

    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        if (!isVisible(*itr))
            continue;

        ObjectGuid firstAccountGuid = itr->second.first_guid;

        data << uint32(secsToTimeBitFields(itr->second.date));
        data.WriteGuidBytes<0>(firstAccountGuid);
        data << uint32(itr->first);
        data.WriteGuidBytes<4, 2, 3, 5>(firstAccountGuid);
        data << uint32(realmID);
        data.WriteGuidBytes<6>(firstAccountGuid);
        data << uint32(realmID);
        data.WriteGuidBytes<7, 1>(firstAccountGuid);
    }

    for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;
        if (!criteriaTree)
            continue;

        AchievementEntry const* achievement = itr->second.achievement;

        // account criteria send in other packet
        if (!achievement || (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT))
            continue;

        ObjectGuid counter = uint64(itr->second.counter);

        data.WriteGuidBytes<5, 7>(counter);
        data.WriteGuidBytes<3, 4>(guid);
        data.WriteGuidBytes<1>(counter);
        data << uint32(secsToTimeBitFields(itr->second.date));       // Date
        data.WriteGuidBytes<0>(guid);
        data.WriteGuidBytes<2>(counter);
        data.WriteGuidBytes<6>(guid);
        data.WriteGuidBytes<6>(counter);
        data << uint32(criteriaTree->criteria);         // CriteriaID
        data.WriteGuidBytes<4>(counter);
        data.WriteGuidBytes<2>(guid);
        data.WriteGuidBytes<3>(counter);
        data.WriteGuidBytes<7>(guid);
        data.WriteGuidBytes<0>(counter);
        data << uint32(0);                              // TimeFromStart / TimeFromCreate - set NOT NULL if flags == 1, need research...
        data << uint32(0);                              // TimeFromStart / TimeFromCreate
        data.WriteGuidBytes<1, 5>(guid);

        ++criteriaCount;
    }

    data.PutBits(bit_pos, criteriaCount, 19);

    SendPacket(&data);
}

template<>
void AchievementMgr<ScenarioProgress>::SendAllAchievementData(Player* receiver)
{
    // not needed
}

template<>
void AchievementMgr<Guild>::SendAllAchievementData(Player* receiver)
{
    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DATA, m_completedAchievements.size() * (4 + 4) + 3);
    data.WriteBits(m_completedAchievements.size(), 20);
    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
        data.WriteBits(0, 8);
    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        data << uint32(secsToTimeBitFields(itr->second.date));
        data << uint32(realmID);                                     // VirtualRealmAddress / NativeRealmAddress
        data << uint32(itr->first);
        data << uint32(realmID);                                     // VirtualRealmAddress / NativeRealmAddress
    }

    receiver->GetSession()->SendPacket(&data);
}

template<class T>
void AchievementMgr<T>::SendAllAccountCriteriaData(Player* /*receiver*/)
{
    const CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

    if (!progressMap)
        return;

    ObjectGuid guid = GetOwner()->GetGUID();
    uint32 criteriaCount = 0;
    ByteBuffer criteriaData;

    WorldPacket data(SMSG_ALL_ACCOUNT_CRITERIA_DATA);

    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 19);

    for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;

        if (!criteriaTree)
            continue;

        AchievementEntry const* achievement = itr->second.achievement;

        if (!achievement || !(achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT))
            continue;

        ObjectGuid counter = uint64(itr->second.counter);

        data.WriteBits(0, 4);            // Flags
        data.WriteGuidMask<4>(counter);
        data.WriteGuidMask<3>(guid);
        data.WriteGuidMask<6, 0, 3>(counter);
        data.WriteGuidMask<2, 5>(guid);
        data.WriteGuidMask<5>(counter);
        data.WriteGuidMask<0, 1>(guid);
        data.WriteGuidMask<2>(counter);
        data.WriteGuidMask<4>(guid);
        data.WriteGuidMask<1>(counter);
        data.WriteGuidMask<6, 7>(guid);
        data.WriteGuidMask<7>(counter);

        criteriaData.WriteGuidBytes<0>(guid);
        criteriaData.WriteGuidBytes<7>(counter);
        criteriaData << uint32(0);
        criteriaData << uint32(0);
        criteriaData.WriteGuidBytes<2>(guid);
        criteriaData.WriteGuidBytes<2>(counter);
        criteriaData << uint32(secsToTimeBitFields(itr->second.date));
        criteriaData.WriteGuidBytes<4>(counter);
        criteriaData.WriteGuidBytes<6>(guid);
        criteriaData.WriteGuidBytes<0>(counter);
        criteriaData.WriteGuidBytes<4, 1>(guid);
        criteriaData.WriteGuidBytes<1>(counter);
        criteriaData.WriteGuidBytes<3, 5>(guid);
        criteriaData.WriteGuidBytes<6, 5, 3>(counter);
        criteriaData.WriteGuidBytes<7>(guid);
        criteriaData << uint32(criteriaTree->criteria);

        ++criteriaCount;
    }

    data.FlushBits();

    if (!criteriaData.empty())
        data.append(criteriaData);

    data.PutBits(bit_pos, criteriaCount, 19);

    SendPacket(&data);
}

template<>
void AchievementMgr<ScenarioProgress>::SendAllAccountCriteriaData(Player* /*receiver*/)
{
}

template<class T>
void AchievementMgr<T>::SendAchievementInfo(Player* receiver, uint32 achievementId /*= 0*/)
{
}

template<>
void AchievementMgr<Player>::SendAchievementInfo(Player* receiver, uint32 /*achievementId = 0 */)
{
    CriteriaProgressMap* progressMap = GetCriteriaProgressMap();

    if (!progressMap)
        return;

    ObjectGuid guid = GetOwner()->GetGUID();
    ObjectGuid counter;

    VisibleAchievementPred isVisible;
    size_t numCriteria = progressMap->size();
    size_t numAchievements = std::count_if(m_completedAchievements.begin(), m_completedAchievements.end(), isVisible);

    WorldPacket data(SMSG_RESPOND_INSPECT_ACHIEVEMENTS);

    data.WriteGuidMask<4, 7, 6>(guid);
    data.WriteBits(numCriteria, 19);
    data.WriteGuidMask<2, 3>(guid);

    for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
    {
        counter = itr->second.counter;

        data.WriteGuidMask<5>(guid);
        data.WriteGuidMask<0>(counter);
        data.WriteGuidMask<2>(guid);
        data.WriteGuidMask<7, 2, 6>(counter);
        data.WriteGuidMask<4>(guid);
        data.WriteBits(0, 4);                    // Flags
        data.WriteGuidMask<7, 3, 6>(guid);
        data.WriteGuidMask<5>(counter);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<1, 3, 4>(counter);
        data.WriteGuidMask<0>(guid);
    }

    data.WriteGuidMask<0>(guid);
    data.WriteBits(numAchievements, 20);

    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        if (!isVisible(*itr))
            continue;

        ObjectGuid firstGuid = (*itr).second.first_guid;
        data.WriteGuidMask<2, 3, 4, 1, 0, 5, 7, 6>(firstGuid);
    }

    data.WriteGuidMask<5, 1>(guid);

    data.FlushBits();

    for (CriteriaProgressMap::const_iterator itr = progressMap->begin(); itr != progressMap->end(); ++itr)
    {
        counter = itr->second.counter;
        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;

        data.WriteGuidBytes<1, 3, 0>(guid);
        data.WriteGuidBytes<5, 7>(counter);
        data << uint32(criteriaTree->criteria);     // ID
        data.WriteGuidBytes<2>(counter);
        data << uint32(0);                          // TimeFromStart / TimeFromCreate
        data.WriteGuidBytes<4>(guid);
        data.WriteGuidBytes<4, 0>(counter);
        data << uint32(secsToTimeBitFields(itr->second.date));  // Date
        data.WriteGuidBytes<6, 3>(counter);
        data << uint32(0);                          // TimeFromStart / TimeFromCreate
        data.WriteGuidBytes<2, 5>(guid);
        data.WriteGuidBytes<1>(counter);
        data.WriteGuidBytes<7, 6>(guid);
    }

    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        if (!isVisible(*itr))
            continue;

        ObjectGuid firstGuid = (*itr).second.first_guid;

        data.WriteGuidBytes<6>(firstGuid);
        data << uint32(realmID);                                   // VirtualRealmAddress / NativeRealmAddress
        data.WriteGuidBytes<7, 5>(firstGuid);
        data << uint32(secsToTimeBitFields((*itr).second.date));   // Date
        data << uint32(realmID);                                   // VirtualRealmAddress / NativeRealmAddress
        data << uint32(itr->first);                                // ID
        data.WriteGuidBytes<4, 2, 3, 0, 1>(firstGuid);
    }

    data.WriteGuidBytes<2, 5, 3, 1, 6, 4, 0, 7>(guid);

    receiver->GetSession()->SendPacket(&data);
}

template<>
void AchievementMgr<Guild>::SendAchievementInfo(Player* receiver, uint32 achievementId /*= 0*/)
{
    //will send response to criteria progress request
    AchievementEntry const* entry = sAchievementMgr->GetAchievement(achievementId);
    uint32 criteriaTree = entry ? entry->criteriaTree : 0;

    std::vector<CriteriaTreeEntry const*> const* cTree = GetCriteriaTreeList(criteriaTree);
    CriteriaProgressMap* progressMap = GetCriteriaProgressMap();
    if (!cTree || !progressMap)
    {
        // send empty packet
        WorldPacket data(SMSG_GUILD_CRITERIA_UPDATE, 3);
        data.WriteBits(0, 19);
        receiver->GetSession()->SendPacket(&data);
        return;
    }

    uint32 numCriteria = 0;
    ByteBuffer criteriaData(cTree->size() * (8 + 8 + 4 + 4 + 4));

    WorldPacket data(SMSG_GUILD_CRITERIA_UPDATE, 3 + cTree->size());
    data.WriteBits(numCriteria, 19);

    for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cTree->begin(); itr != cTree->end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = *itr;
        CriteriaTreeProgress* progress = GetCriteriaProgress(criteriaTree->ID);
        if (!progress)
            continue;

        ++numCriteria;

        ObjectGuid counter = progress->counter;
        ObjectGuid guid = progress->CompletedGUID;

        data.WriteGuidMask<4, 2, 6>(guid);
        data.WriteGuidMask<1, 5>(counter);
        data.WriteGuidMask<3>(guid);
        data.WriteGuidMask<2>(counter);
        data.WriteGuidMask<0, 5>(guid);
        data.WriteGuidMask<3>(counter);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<7>(counter);
        data.WriteGuidMask<7>(guid);
        data.WriteGuidMask<0, 6, 4>(counter);

        criteriaData.WriteGuidBytes<0>(counter);
        criteriaData << uint32(secsToTimeBitFields(progress->date)); // DateUpdated
        criteriaData.WriteGuidBytes<2>(guid);
        criteriaData.WriteGuidBytes<1>(counter);
        criteriaData << uint32(0);                      // Flags
        criteriaData.WriteGuidBytes<7, 6>(counter);
        criteriaData.WriteGuidBytes<0>(guid);
        criteriaData << uint32(progress->date);         // DateStarted / DateCreated
        criteriaData.WriteGuidBytes<6, 7>(guid);
        criteriaData.WriteGuidBytes<4>(counter);
        criteriaData.WriteGuidBytes<5>(guid);
        criteriaData << uint32(criteriaTree->criteria);
        criteriaData.WriteGuidBytes<4, 1>(guid);
        criteriaData << uint32(::time(NULL) - progress->date); // DateStarted / DateCreated
        criteriaData.WriteGuidBytes<5, 2>(counter);
        criteriaData.WriteGuidBytes<3>(guid);
        criteriaData.WriteGuidBytes<3>(counter);
    }

    data.FlushBits();

    if (!criteriaData.empty())
        data.append(criteriaData);

    data.PutBits(0, numCriteria, 19);

    receiver->GetSession()->SendPacket(&data);
}

template<class T>
bool AchievementMgr<T>::HasAchieved(uint32 achievementId) const
{
    if (m_completedAchievements.empty())
        return false;
    
    return m_completedAchievements.find(achievementId) != m_completedAchievements.end();
}

template<class T>
bool AchievementMgr<T>::HasAccountAchieved(uint32 achievementId) const
{
    if (m_completedAchievements.empty())
        return false;

    return m_completedAchievements.find(achievementId) != m_completedAchievements.end();
}

template<class T>
uint64 AchievementMgr<T>::GetFirstAchievedCharacterOnAccount(uint32 achievementId) const
{
    CompletedAchievementMap::const_iterator itr = m_completedAchievements.find(achievementId);

    if (itr == m_completedAchievements.end())
        return 0LL;

    return (*itr).second.first_guid;
}

template<class T>
bool AchievementMgr<T>::CanDeleteOrUpdateCreteria(uint32 achievementId, uint64 guid) const
{
    CompletedAchievementMap::const_iterator itr = m_completedAchievements.find(achievementId);

    if (itr == m_completedAchievements.end())
        return false;

    if ((*itr).second.isAccountAchievement)
        return true;

    return GetCriteriaSort() == PLAYER_CRITERIA ? (*itr).second.first_guid == guid : true;
}

template<class T>
bool AchievementMgr<T>::CanUpdateCriteria(CriteriaTreeEntry const* treeEntry, CriteriaEntry const* criteria, AchievementEntry const* achievement, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer)
{
    if(!achievement && GetCriteriaSort() != SCENARIO_CRITERIA)
        return false;

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_ACHIEVEMENT_CRITERIA, criteria->ID, NULL))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Disabled",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    if (achievement && achievement->mapID != -1 && referencePlayer->GetMapId() != uint32(achievement->mapID))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Wrong map",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        InstanceTemplate const* mInstance = sObjectMgr->GetInstanceTemplate(referencePlayer->GetMapId());
        if(mInstance)
        {
            if(mInstance->Parent != uint32(achievement->mapID))
                return false;
        }
        else
            return false;
    }

    if (achievement && (achievement->requiredFaction == ACHIEVEMENT_FACTION_HORDE && referencePlayer->GetTeam() != HORDE ||
        achievement->requiredFaction == ACHIEVEMENT_FACTION_ALLIANCE && referencePlayer->GetTeam() != ALLIANCE))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Wrong faction",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    if (IsCompletedCriteria(treeEntry, achievement, criteria))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Is Completed",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    if (!RequirementsSatisfied(achievement, criteria, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Requirements not satisfied",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    if (!AdditionalRequirementsSatisfied(criteria->ModifyTree, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Additional requirements not satisfied",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    if (!ConditionsSatisfied(criteria, referencePlayer))
    {
        // sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Conditions not satisfied",
            // treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    if (!GetOwner()->CanUpdateCriteria(treeEntry->ID))
    {
        //sLog->outTrace(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: %s (Id: %u Type %s) Scenario condition can not be updated at current scenario stage",
            //treeEntry->name, criteria->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->type));
        return false;
    }

    return true;
}

template<class T>
bool AchievementMgr<T>::ConditionsSatisfied(CriteriaEntry const* criteria, Player* referencePlayer) const
{
    if (criteria->timedCriteriaStartType)
    {
        switch (criteria->timedCriteriaStartType)
        {
            case ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP:
                if (referencePlayer->GetMapId() != criteria->timedCriteriaMiscId)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_CONDITION_NOT_IN_GROUP:
                if (referencePlayer->GetGroup())
                    return false;
                break;
            default:
                break;
        }
    }

    if (criteria->timedCriteriaFailType)
    {
        switch (criteria->timedCriteriaFailType)
        {
            case ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP:
                if (referencePlayer->GetMapId() != criteria->timedCriteriaMiscFailId && criteria->timedCriteriaMiscFailId != 0)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_CONDITION_NOT_IN_GROUP:
                if (referencePlayer->GetGroup())
                    return false;
                break;
            default:
                break;
        }
    }

    return true;
}

template<class T>
bool AchievementMgr<T>::RequirementsSatisfied(AchievementEntry const* achievement, CriteriaEntry const* criteria, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const *unit, Player* referencePlayer) const
{
    //if(criteria->worldStateId && GetMap()->getWorldState(criteria->worldStateId) != criteria->worldStateValue)
        //return false;

    switch (AchievementCriteriaTypes(criteria->type))
    {
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
        case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
            if (!miscValue1)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
        case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
        case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
        case ACHIEVEMENT_CRITERIA_TYPE_OBTAIN_BATTLEPET:
        case ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET:
        case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN:
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            if (m_completedAchievements.find(criteria->complete_achievement.linkedAchievement) == m_completedAchievements.end())
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            if (!miscValue1 || criteria->win_bg.bgMapID != referencePlayer->GetMapId())
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            if (!miscValue1 || criteria->kill_creature.creatureID != miscValue1)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            // update at loading or specific skill update
            if (miscValue1 && miscValue1 != criteria->reach_skill_level.skillID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            // update at loading or specific skill update
            if (miscValue1 && miscValue1 != criteria->learn_skill_level.skillID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            if (miscValue1 && miscValue1 != criteria->complete_quests_in_zone.zoneID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
            if (!miscValue1 || referencePlayer->GetMapId() != criteria->complete_battleground.mapID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
            if (!miscValue1 || referencePlayer->GetMapId() != criteria->death_at_map.mapID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
            {
                if (!miscValue1)
                    return false;
                // skip wrong arena achievements, if not achievIdByArenaSlot then normal total death counter
                bool notfit = false;
                for (int j = 0; j < MAX_ARENA_SLOT; ++j)
                {
                    if (achievement && achievIdByArenaSlot[j] == achievement->ID)
                    {
                        Battleground* bg = referencePlayer->GetBattleground();
                        if (!bg || !bg->isArena() || BattlegroundMgr::BracketByJoinType(bg->GetJoinType()) != j)
                            notfit = true;
                        break;
                    }
                }
                if (notfit)
                    return false;
                break;
            }
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
            {
                if (!miscValue1 || !achievement)
                    return false;

                Map const* map = referencePlayer->IsInWorld() ? referencePlayer->GetMap() : sMapMgr->FindMap(referencePlayer->GetMapId(), referencePlayer->GetInstanceId());
                if (!map || !map->IsDungeon())
                    return false;

                // search case
                bool found = false;
                for (int j = 0; achievIdForDungeon[j][0]; ++j)
                {
                    if (achievIdForDungeon[j][0] == achievement->ID)
                    {
                        if (map->IsRaid())
                        {
                            // if raid accepted (ignore difficulty)
                            if (!achievIdForDungeon[j][2])
                                break;                      // for
                        }
                        else if (referencePlayer->GetDungeonDifficulty() == REGULAR_DIFFICULTY)
                        {
                            // dungeon in normal mode accepted
                            if (!achievIdForDungeon[j][1])
                                break;                      // for
                        }
                        else
                        {
                            // dungeon in heroic mode accepted
                            if (!achievIdForDungeon[j][3])
                                break;                      // for
                        }

                        found = true;
                        break;                              // for
                    }
                }
                if (!found)
                    return false;

                //FIXME: work only for instances where max == min for players
                if (((InstanceMap*)map)->GetMaxPlayers() != criteria->death_in_dungeon.manLimit)
                    return false;
                break;
            }
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
            if (!miscValue1 || miscValue1 != criteria->killed_by_creature.creatureEntry)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
            if (!miscValue1)
                return false;

            // if team check required: must kill by opposition faction
            if (achievement->ID == 318 && miscValue2 == referencePlayer->GetTeam())
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
            if (!miscValue1 || miscValue2 != criteria->death_from.type)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            {
                // if miscValues != 0, it contains the questID.
                if (miscValue1)
                {
                    if (miscValue1 != criteria->complete_quest.questID)
                        return false;
                }
                else
                {
                    // login case.
                    if (!referencePlayer->GetQuestRewardStatus(criteria->complete_quest.questID))
                        return false;
                }

                if (AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(criteria))
                    if (!data->Meets(referencePlayer, unit))
                        return false;
                break;
            }
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            if (!miscValue1 || miscValue1 != criteria->be_spell_target.spellID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            if (!miscValue1 || miscValue1 != criteria->cast_spell.spellID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            if (miscValue1 && miscValue1 != criteria->learn_spell.spellID)
                return false;

            if (!referencePlayer->HasSpell(criteria->learn_spell.spellID))
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            // miscValue1 = loot_type (note: 0 = LOOT_CORPSE and then it ignored)
            // miscValue2 = count of item loot
            if (!miscValue1 || !miscValue2 || miscValue1 != criteria->loot_type.lootType)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            if (miscValue1 && criteria->own_item.itemID != miscValue1)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            if (!miscValue1 || criteria->use_item.itemID != miscValue1)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            if (!miscValue1 || miscValue1 != criteria->own_item.itemID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            {
                WorldMapOverlayEntry const* worldOverlayEntry = sWorldMapOverlayStore.LookupEntry(criteria->explore_area.areaReference);
                if (!worldOverlayEntry)
                    break;

                    // those requirements couldn't be found in the dbc
                if(AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(criteria))
                    if (data->Meets(referencePlayer, unit))
                        return true;

                bool matchFound = false;
                for (int j = 0; j < MAX_WORLD_MAP_OVERLAY_AREA_IDX; ++j)
                {
                    uint32 area_id = worldOverlayEntry->areatableID[j];
                    if (!area_id)                            // array have 0 only in empty tail
                        break;

                    int32 exploreFlag = GetAreaFlagByAreaID(area_id);
                    if (exploreFlag < 0)
                        continue;

                    uint32 playerIndexOffset = uint32(exploreFlag) / 32;
                    uint32 mask = 1 << (uint32(exploreFlag) % 32);

                    if (referencePlayer->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + playerIndexOffset) & mask)
                    {
                        matchFound = true;
                        break;
                    }
                }

                if (!matchFound)
                    return false;
                break;
            }
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            if (miscValue1 && miscValue1 != criteria->gain_reputation.factionID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            // miscValue1 = itemid miscValue2 = itemSlot
            if (!miscValue1 || miscValue2 != criteria->equip_epic_item.itemSlot)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            {
                // miscValue1 = itemid miscValue2 = diced value
                if (!miscValue1 || miscValue2 != criteria->roll_greed_on_loot.rollValue)
                    return false;

                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                if (!proto)
                    return false;
                break;
            }
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            if (!miscValue1 || miscValue1 != criteria->do_emote.emoteID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            if (!miscValue1)
                return false;

            if (criteria->timedCriteriaStartType == ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP)
            {
                if (referencePlayer->GetMapId() != criteria->timedCriteriaMiscId)
                    return false;

                // map specific case (BG in fact) expected player targeted damage/heal
                if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
                    return false;
            }
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            // miscValue1 = item_id
            if (!miscValue1 || miscValue1 != criteria->equip_item.itemID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            if (!miscValue1 || miscValue1 != criteria->use_gameobject.goEntry)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            if (!miscValue1 || miscValue1 != criteria->fish_in_gameobject.goEntry)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            if (miscValue1 && miscValue1 != criteria->learn_skillline_spell.skillLine)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            {
                if (!miscValue1)
                    return false;
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(miscValue1);
                if (!proto || proto->Quality < ITEM_QUALITY_EPIC)
                    return false;
                break;
            }
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            if (miscValue1 && miscValue1 != criteria->learn_skill_line.skillLine)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            if (!miscValue1 || miscValue1 != criteria->hk_class.classID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            if (!miscValue1 || miscValue1 != criteria->hk_race.raceID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            if (!miscValue1 || miscValue1 != criteria->bg_objective.objectiveId)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            if (!miscValue1 || miscValue1 != criteria->honorable_kill_at_area.areaID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
            if (!miscValue1 || !miscValue2 || int64(miscValue2) < 0
                || miscValue1 != criteria->currencyGain.currency)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_INSTANSE_MAP_ID:
            if (!miscValue1 || miscValue1 != criteria->finish_instance.mapID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            if (!miscValue1 || miscValue1 != criteria->win_arena.mapID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
            if (!miscValue1 || miscValue1 != criteria->complete_raid.groupSize)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
            if (!miscValue1 || miscValue1 != criteria->play_arena.mapID)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            if (!miscValue1 || miscValue1 != criteria->own_rank.rank)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT:
        case ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2:
            if (!miscValue1 || miscValue1 != criteria->script_event.unkValue)
                return false;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
            if (!miscValue1 || miscValue1 != criteria->battle_pet_journal.add_pet)
                return false;
        case ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
            if (!miscValue1 || miscValue1 != criteria->battlepet_level.level_up)
                return false;
            break;
        default:
            break;
    }
    return true;
}

template<class T>
bool AchievementMgr<T>::AdditionalRequirementsSatisfied(uint32 ModifyTree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer) const
{
    if(!ModifyTree)
        return true;

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied start ModifyTree %i, miscValue1 %u miscValue2 %u", ModifyTree, miscValue1, miscValue2);

    int32 saveReqType = -1;
    bool saveCheck = false;
    if(std::vector<ModifierTreeEntry const*> const* modifierList = GetModifierTreeList(ModifyTree))
    for (std::vector<ModifierTreeEntry const*>::const_iterator itr = modifierList->begin(); itr != modifierList->end(); ++itr)
    {
        ModifierTreeEntry const* modifier = *itr;
        uint32 const reqType = modifier->additionalConditionType;
        uint32 const reqValue = modifier->additionalConditionValue;
        uint32 const reqCount = modifier->additionalConditionCount;
        bool check = true;

        //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied cheker start Modify %i, reqType %i reqValue %i reqCount %i saveCheck %i saveReqType %i operatorFlags %i check %i", (*itr), reqType, reqValue, reqCount, saveCheck, saveReqType, modifier->operatorFlags, check);

        if(modifier->operatorFlags & (MODIFIERTREE_FLAG_MAIN | MODIFIERTREE_FLAG_PARENT))
        {
            if(!AdditionalRequirementsSatisfied(modifier->ID, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
                return false;
        }
        else
        {
            switch (AchievementCriteriaAdditionalCondition(reqType))
            {
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_DRUNK_VALUE: // 1
                {
                    if(referencePlayer->GetDrunkValue() < reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_ITEM_LEVEL: // 3
                {
                    // miscValue1 is itemid
                    ItemTemplate const * item = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                    if (!item || item->ItemLevel < reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_CREATURE_ENTRY: // 4
                    if (!unit || unit->GetEntry() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_PLAYER: // 5
                    if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_DEAD: // 6
                    if (!unit || unit->isAlive())
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_ENEMY: // 7
                    if (!unit)
                        check = false;
                    else if (const Player* player = unit->ToPlayer())
                        if (player->GetTeam() == referencePlayer->GetTeam())
                            check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_HAS_AURA: // 8
                    if (!referencePlayer->HasAura(reqValue))
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_HAS_AURA: // 10
                    if (!unit || !unit->HasAura(reqValue))
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_MOUNTED: // 11
                    if (!unit || !unit->IsMounted())
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_ITEM_QUALITY_MIN: // 14
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_ITEM_QUALITY_EQUALS: // 15
                {
                    // miscValue1 is itemid
                    ItemTemplate const * item = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                    if (!item || item->Quality < reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_AREA_OR_ZONE: // 17
                {
                    if (referencePlayer->GetZoneId() != reqValue && referencePlayer->GetAreaId() != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_AREA_OR_ZONE: // 18
                {
                    if (!unit)
                        check = false;
                    else
                    {
                        if (unit->GetZoneId() != reqValue && unit->GetAreaId() != reqValue)
                            check = false;
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_MAP_DIFFICULTY: // 20
                    if (sObjectMgr->GetDiffFromSpawn(referencePlayer->GetMap()->GetDifficulty()) != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_CREATURE_YIELDS_XP: // 21
                {
                    if (!unit)
                        check = false;
                    else
                    {
                        uint32 _xp = Trinity::XP::Gain(referencePlayer, const_cast<Unit*>(unit));
                        if (!_xp)
                            check = false;
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_ARENA_TEAM_SIZE: // 24
                {
                    Battleground* bg = referencePlayer->GetBattleground();
                    if (!bg || !bg->isArena() || !bg->isRated() || bg->GetJoinType() != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_RACE: // 25
                    if (referencePlayer->getRace() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_CLASS: // 26
                    if (referencePlayer->getClass() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_RACE: // 27
                    if (!unit || unit->GetTypeId() != TYPEID_PLAYER || unit->getRace() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_CLASS: // 28
                    if (!unit || unit->GetTypeId() != TYPEID_PLAYER || unit->getClass() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_MAX_GROUP_MEMBERS: // 29
                    if (referencePlayer->GetGroup() && referencePlayer->GetGroup()->GetMembersCount() >= reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_CREATURE_TYPE: // 30
                {
                    if (miscValue1 != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_MAP: // 32
                    if (referencePlayer->GetMapId() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_LEVEL_IN_SLOT: // 34
                {
                    for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
                    {
                        if (PetBattleSlot* _slot = referencePlayer->GetBattlePetMgr()->GetPetBattleSlot(i))
                        {
                            if (!_slot->IsEmpty())
                            {
                                if (PetJournalInfo* petInfo = referencePlayer->GetBattlePetMgr()->GetPetInfoByPetGUID(_slot->GetPet()))
                                {
                                    if(petInfo->GetLevel() < reqValue)
                                    {
                                        check = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_WITHOUT_GROUP: // 35
                    if (Group const* group = referencePlayer->GetGroup())
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_MIN_PERSONAL_RATING: // 37
                {
                    Battleground* bg = referencePlayer->GetBattleground();
                    if (!bg || !bg->isArena() || !bg->isRated())
                        check = false;
                    else
                    {
                        Bracket* bracket = referencePlayer->getBracket(BattlegroundMgr::BracketByJoinType(bg->GetJoinType()));
                        if (!bracket || bracket->getRating() < reqValue)
                            check = false;
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TITLE_BIT_INDEX: // 38
                    // miscValue1 is title's bit index
                    if (miscValue1 != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_SOURCE_LEVEL: // 39
                    if (referencePlayer->getLevel() != reqValue)
                        check = false;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_LEVEL: // 40
                    if (!unit || unit->getLevel() != reqValue)
                        check = false;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_ZONE: // 41
                    if (referencePlayer->GetZoneId() != reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_HEALTH_PERCENT_BELOW: // 46
                    if (!unit || unit->GetHealthPct() >= reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_MIN_ACHIEVEMENT_POINTS: // 56
                    if (_achievementPoints < reqValue)
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_REQUIRES_LFG_GROUP: // 58
                    if (!referencePlayer->GetGroup() || !referencePlayer->GetGroup()->isLFGGroup())
                        check = false;
                    break;
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_REQUIRES_GUILD_GROUP: // 61
                {
                    Map* map = referencePlayer->GetMap();
                    if (!referencePlayer->GetGroup() || !referencePlayer->GetGuildId() || !map)
                        check = false;
                    else
                    {
                        Group const* group = referencePlayer->GetGroup();
                        uint32 guildId = referencePlayer->GetGuildId();
                        uint32 count = 0;
                        uint32 size = 0;

                        for (GroupReference const* ref = group->GetFirstMember(); ref != NULL; ref = ref->next())
                        {
                            size = group->GetMembersCount();
                            if (Player const* groupMember = ref->getSource())
                                if (groupMember->GetGuildId() != guildId)
                                    count++;
                        }
                        if(map->GetInstanceId() && sObjectMgr->GetCountFromDifficulty(map->GetDifficulty()) > count)
                            check = false;
                        else if(!size || size != count)
                            check = false;
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_GUILD_REPUTATION: // 62
                {
                    if (referencePlayer->GetReputationMgr().GetReputation(REP_GUILD) < reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_RATED_BATTLEGROUND: // 63
                {
                    Battleground* bg = referencePlayer->GetBattleground();
                    if (!bg || !bg->IsRBG())
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_PROJECT_RARITY: // 65
                {
                    if (!miscValue1)
                    {
                        check = false;
                        break;
                    }

                    ResearchProjectEntry const* rp = sResearchProjectStore.LookupEntry(miscValue1);
                    if (!rp)
                        check = false;
                    else
                    {
                        if (rp->rare != reqValue)
                            check = false;

                        if (referencePlayer->IsCompletedProject(rp->ID, false))
                            check = false;
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_PROJECT_RACE: // 66
                {
                    if (!miscValue1)
                    {
                        check = false;
                        break;
                    }

                    ResearchProjectEntry const* rp = sResearchProjectStore.LookupEntry(miscValue1);
                    if (!rp)
                        check = false;
                    else if (rp->branchId != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_DUNGEON_FIFFICULTY: // 68
                {
                    if (!unit || !unit->GetMap() || unit->GetMap()->GetSpawnMode() != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_MIN_LEVEL: // 70
                {
                    if (!unit || unit->getLevel() >= reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_TARGET_MAX_LEVEL: // 71
                {
                    if (!unit || unit->getLevel() < reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_FEMALY: // 78
                {
                    if (!miscValue3 || miscValue3 != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_HP_LOW_THAT: // 79
                {
                    PetBattleWild* petBattle = referencePlayer->GetBattlePetMgr()->GetPetBattleWild();
                    if (!petBattle || petBattle->GetFrontPet(TEAM_ENEMY)->GetHealthPct() >= reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_MASTER_PET_TAMER: // 81
                {
                    if (!miscValue2 || miscValue2 != reqValue)
                        check = false;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_CHALANGER_RATE: // 83
                {
                    if (!miscValue2)
                       check = false;
                    else if (reqValue > miscValue2)            // Medal check
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_QUALITY: // 89
                {
                    uint8 qPet = 0;
                    for (uint32 i = 0; i < sBattlePetBreedQualityStore.GetNumRows(); ++i)
                    {
                        BattlePetBreedQualityEntry const* qEntry = sBattlePetBreedQualityStore.LookupEntry(i);
                        if (!qEntry)
                            continue;

                        if (miscValue2 == qEntry->quality)
                            qPet = qEntry->ID;

                        if (qPet != reqValue)
                            check = false;
                        else
                            check = true;
                    }
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_WIN_IN_PVP: // 90
                {
                    if (!miscValue1 || miscValue1 != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_SPECIES: // 91
                {
                    if (!miscValue1 || miscValue1 != reqValue)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_EXPANSION_LESS: // 92
                {
                    if (reqValue >= (int32)sWorld->getIntConfig(CONFIG_EXPANSION))
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_REPUTATION: // 95
                {
                    if (referencePlayer->GetReputationMgr().GetReputation(reqValue) < reqCount)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_ITEM_CLASS_AND_SUBCLASS: // 96
                {
                    // miscValue1 is itemid
                    ItemTemplate const * item = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                    if (!item || item->Class != reqValue || item->SubClass != reqCount)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_CURRENCY: // 121
                {
                    if (referencePlayer->GetCurrency(reqValue) <= reqCount)
                        check = false;
                    break;
                }
                case ACHIEVEMENT_CRITERIA_ADDITIONAL_CONDITION_ARENA_SEASON: // 125
                {
                    if (sWorld->getIntConfig(CONFIG_ARENA_SEASON_ID) != reqValue)
                        check = false;
                    break;
                }
                default:
                    break;
            }
        }

        //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied cheker end Modify %i, reqType %i reqValue %i reqCount %i saveCheck %i saveReqType %i operatorFlags %i check %i", (*itr), reqType, reqValue, reqCount, saveCheck, saveReqType, modifier->operatorFlags, check);

        if(saveReqType == -1)
        {
            if(check && reqType) //don`t save if false
                saveReqType = reqType;
            saveCheck = check;
        }
        else if(saveReqType != reqType)
        {
            if(!saveCheck || !check)
                return false;
        }
        else if(saveReqType == reqType)
        {
            if(saveCheck || check)
                return true;
        }
    }

    //sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied end ModifyTree %i, saveCheck %u saveReqType %i", ModifyTree, saveCheck, saveReqType);
    return saveCheck;
}

template<class T>
CriteriaSort AchievementMgr<T>::GetCriteriaSort() const
{
    return PLAYER_CRITERIA;
}

template<>
CriteriaSort AchievementMgr<Guild>::GetCriteriaSort() const
{
    return GUILD_CRITERIA;
}

template<>
CriteriaSort AchievementMgr<ScenarioProgress>::GetCriteriaSort() const
{
    return SCENARIO_CRITERIA;
}

char const* AchievementGlobalMgr::GetCriteriaTypeString(uint32 type)
{
    return GetCriteriaTypeString(AchievementCriteriaTypes(type));
}

char const* AchievementGlobalMgr::GetCriteriaTypeString(AchievementCriteriaTypes type)
{
    switch (type)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            return "KILL_CREATURE";
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            return "TYPE_WIN_BG";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
            return "COMPLETE_RESEARCH";
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            return "REACH_LEVEL";
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            return "REACH_SKILL_LEVEL";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return "COMPLETE_ACHIEVEMENT";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            return "COMPLETE_QUEST_COUNT";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            return "COMPLETE_DAILY_QUEST_DAILY";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            return "COMPLETE_QUESTS_IN_ZONE";
        case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY:
            return "CURRENCY";
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
            return "DAMAGE_DONE";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            return "COMPLETE_DAILY_QUEST";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
            return "COMPLETE_BATTLEGROUND";
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
            return "DEATH_AT_MAP";
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
            return "DEATH";
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
            return "DEATH_IN_DUNGEON";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
            return "COMPLETE_RAID";
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
            return "KILLED_BY_CREATURE";
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
            return "KILLED_BY_PLAYER";
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            return "FALL_WITHOUT_DYING";
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
            return "DEATHS_FROM";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            return "COMPLETE_QUEST";
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            return "BE_SPELL_TARGET";
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            return "CAST_SPELL";
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            return "BG_OBJECTIVE_CAPTURE";
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            return "HONORABLE_KILL_AT_AREA";
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            return "WIN_ARENA";
        case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
            return "PLAY_ARENA";
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            return "LEARN_SPELL";
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            return "HONORABLE_KILL";
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            return "OWN_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
            return "WIN_RATED_ARENA";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
            return "HIGHEST_TEAM_RATING";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            return "HIGHEST_PERSONAL_RATING";
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            return "LEARN_SKILL_LEVEL";
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            return "USE_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            return "LOOT_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            return "EXPLORE_AREA";
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            return "OWN_RANK";
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            return "BUY_BANK_SLOT";
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            return "GAIN_REPUTATION";
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            return "GAIN_EXALTED_REPUTATION";
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            return "VISIT_BARBER_SHOP";
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            return "EQUIP_EPIC_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            return "ROLL_NEED_ON_LOOT";
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            return "GREED_ON_LOOT";
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            return "HK_CLASS";
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            return "HK_RACE";
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            return "DO_EMOTE";
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            return "HEALING_DONE";
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            return "GET_KILLING_BLOWS";
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            return "EQUIP_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
            return "MONEY_FROM_VENDORS";
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            return "GOLD_SPENT_FOR_TALENTS";
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            return "NUMBER_OF_TALENT_RESETS";
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            return "MONEY_FROM_QUEST_REWARD";
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            return "GOLD_SPENT_FOR_TRAVELLING";
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            return "GOLD_SPENT_AT_BARBER";
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            return "GOLD_SPENT_FOR_MAIL";
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            return "LOOT_MONEY";
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            return "USE_GAMEOBJECT";
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            return "BE_SPELL_TARGET2";
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            return "SPECIAL_PVP_KILL";
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            return "FISH_IN_GAMEOBJECT";
        case ACHIEVEMENT_CRITERIA_TYPE_EARNED_PVP_TITLE:
            return "EARNED_PVP_TITLE";
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            return "LEARN_SKILLLINE_SPELLS";
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
            return "WIN_DUEL";
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
            return "LOSE_DUEL";
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
            return "KILL_CREATURE_TYPE";
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
            return "GOLD_EARNED_BY_AUCTIONS";
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
            return "CREATE_AUCTION";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            return "HIGHEST_AUCTION_BID";
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
            return "WON_AUCTIONS";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
            return "HIGHEST_AUCTION_SOLD";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
            return "HIGHEST_GOLD_VALUE_OWNED";
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
            return "GAIN_REVERED_REPUTATION";
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
            return "GAIN_HONORED_REPUTATION";
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
            return "KNOWN_FACTIONS";
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
            return "LOOT_EPIC_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            return "RECEIVE_EPIC_ITEM";
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
            return "ROLL_NEED";
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
            return "ROLL_GREED";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            return "HIT_DEALT";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            return "HIT_RECEIVED";
        case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            return "TOTAL_DAMAGE_RECEIVED";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            return "HIGHEST_HEAL_CASTED";
        case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
            return "TOTAL_HEALING_RECEIVED";
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
            return "HIGHEST_HEALING_RECEIVED";
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
            return "QUEST_ABANDONED";
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            return "FLIGHT_PATHS_TAKEN";
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            return "LOOT_TYPE";
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            return "CAST_SPELL2";
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            return "LEARN_SKILL_LINE";
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
            return "EARN_HONORABLE_KILL";
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
            return "ACCEPTED_SUMMONINGS";
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            return "EARN_ACHIEVEMENT_POINTS";
        case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            return "USE_LFD_TO_GROUP_WITH_PLAYERS";
        case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
            return "SPENT_GOLD_GUILD_REPAIRS";
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            return "REACH_GUILD_LEVEL";
        case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
            return "CRAFT_ITEMS_GUILD";
        case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
            return "CATCH_FROM_POOL";
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
            return "BUY_GUILD_BANK_SLOTS";
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
            return "EARN_GUILD_ACHIEVEMENT_POINTS";
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
            return "WIN_RATED_BATTLEGROUND";
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_RBG_RATING:
            return "REACH_BG_RATING";
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM:
            return "BUY_GUILD_TABARD";
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
            return "COMPLETE_QUESTS_GUILD";
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
            return "HONORABLE_KILLS_GUILD";
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
            return "KILL_CREATURE_TYPE_GUILD";
        default:
            return "MISSING_TYPE";
    }
    return "";
}

template class AchievementMgr<ScenarioProgress>;
template class AchievementMgr<Guild>;
template class AchievementMgr<Player>;

//==========================================================
void AchievementGlobalMgr::LoadAchievementCriteriaList()
{
    uint32 oldMSTime = getMSTime();

    if (sCriteriaStore.GetNumRows() == 0)
    {
        sLog->outError(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 achievement criteria.");
        return;
    }

    volatile uint32 criterias = 0;
    volatile uint32 guildCriterias = 0;
    volatile uint32 scenarioCriterias = 0;
    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        AchievementEntry const* achievement = sAchievementMgr->GetAchievement(entryId);
        if (!achievement)
            continue;

        if(std::vector<CriteriaTreeEntry const*> const* criteriaTreeList = GetCriteriaTreeList(achievement->criteriaTree))
        for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = criteriaTreeList->begin(); itr != criteriaTreeList->end(); ++itr)
        {
            CriteriaTreeEntry const* criteriaTree = *itr;
            CriteriaTreeInfo cTreeInfo;

            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(criteriaTree->criteria);
            if (!criteria)
            {
                if(criteriaTree->criteria != 0 || criteriaTree->parent == 0)
                    continue;

                if(std::vector<CriteriaTreeEntry const*> const* cTreeList = GetCriteriaTreeList(criteriaTree->ID))
                for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cTreeList->begin(); itr != cTreeList->end(); ++itr)
                {
                    CriteriaTreeEntry const* cTree = *itr;
                    if(!cTree)
                        continue;
                    CriteriaEntry const* crite = sAchievementMgr->GetAchievementCriteria(cTree->criteria);
                    if (!crite)
                        continue;

                    cTreeInfo.criteriaTreeID = cTree->ID;
                    cTreeInfo.parentID = sAchievementMgr->GetParantTreeId(cTree->parent);
                    cTreeInfo.criteriaTree = cTree;
                    cTreeInfo.criteria = crite;
                    cTreeInfo.achievement = achievement;

                    if (achievement->flags & ACHIEVEMENT_FLAG_GUILD)
                        ++guildCriterias, m_GuildCriteriaTreesByType[crite->type].push_back(cTreeInfo);
                    else
                        ++criterias, m_CriteriaTreesByType[crite->type].push_back(cTreeInfo);

                    if (crite->timeLimit)
                        m_CriteriaTreesByTimedType[crite->timedCriteriaStartType].push_back(cTreeInfo);
                }
                continue;
            }

            cTreeInfo.criteriaTreeID = criteriaTree->ID;
            cTreeInfo.parentID = sAchievementMgr->GetParantTreeId(criteriaTree->parent);
            cTreeInfo.criteriaTree = criteriaTree;
            cTreeInfo.criteria = criteria;
            cTreeInfo.achievement = achievement;

            if (achievement->flags & ACHIEVEMENT_FLAG_GUILD)
                ++guildCriterias, m_GuildCriteriaTreesByType[criteria->type].push_back(cTreeInfo);
            else
                ++criterias, m_CriteriaTreesByType[criteria->type].push_back(cTreeInfo);

            if (criteria->timeLimit)
                m_CriteriaTreesByTimedType[criteria->timedCriteriaStartType].push_back(cTreeInfo);
        }
    }

    for (std::set<uint32>::const_iterator itr = sScenarioCriteriaTreeStore.begin(); itr != sScenarioCriteriaTreeStore.end(); ++itr)
    {
        uint32 criteriaTree = *itr;
        std::vector<CriteriaTreeEntry const*> const* criteriaTreeList = GetCriteriaTreeList(criteriaTree);
        if (!criteriaTreeList)
            continue;

        for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = criteriaTreeList->begin(); itr != criteriaTreeList->end(); ++itr)
        {
            CriteriaTreeEntry const* criteriaTree = *itr;
            if(!criteriaTree)
                continue;

            CriteriaTreeInfo cTreeInfo;
            CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(criteriaTree->criteria);
            if (!criteria)
            {
                if(criteriaTree->criteria != 0 || criteriaTree->parent == 0)
                    continue;

                if(std::vector<CriteriaTreeEntry const*> const* cTreeList = GetCriteriaTreeList(criteriaTree->ID))
                for (std::vector<CriteriaTreeEntry const*>::const_iterator itr = cTreeList->begin(); itr != cTreeList->end(); ++itr)
                {
                    CriteriaTreeEntry const* cTree = *itr;
                    if(!cTree)
                        continue;
                    CriteriaEntry const* crite = sAchievementMgr->GetAchievementCriteria(cTree->criteria);
                    if (!crite)
                        continue;

                    cTreeInfo.criteriaTreeID = cTree->ID;
                    cTreeInfo.parentID = sAchievementMgr->GetParantTreeId(cTree->parent);
                    cTreeInfo.criteriaTree = cTree;
                    cTreeInfo.criteria = crite;
                    cTreeInfo.achievement = NULL;

                    ++scenarioCriterias, m_ScenarioCriteriaTreesByType[crite->type].push_back(cTreeInfo);

                    if (crite->timeLimit)
                        m_CriteriaTreesByTimedType[crite->timedCriteriaStartType].push_back(cTreeInfo);
                }
                continue;
            }

            cTreeInfo.criteriaTreeID = criteriaTree->ID;
            cTreeInfo.parentID = sAchievementMgr->GetParantTreeId(criteriaTree->parent);
            cTreeInfo.criteriaTree = criteriaTree;
            cTreeInfo.criteria = criteria;
            cTreeInfo.achievement = NULL;

            ++scenarioCriterias, m_ScenarioCriteriaTreesByType[criteria->type].push_back(cTreeInfo);
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u achievement criteria, %u guild and %u scenario criterias in %u ms", criterias, guildCriterias, scenarioCriterias, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    uint32 oldMSTime = getMSTime();

    if (sAchievementStore.GetNumRows() == 0)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 achievement references.");
        return;
    }

    uint32 count = 0;

    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
         AchievementEntry const* achievement = sAchievementMgr->GetAchievement(entryId);
        if (!achievement || !achievement->refAchievement)
            continue;

        m_AchievementListByReferencedId[achievement->refAchievement].push_back(achievement);
        ++count;
    }

    // Once Bitten, Twice Shy (10 player) - Icecrown Citadel
    if (AchievementEntry const* achievement = sAchievementMgr->GetAchievement(4539))
        const_cast<AchievementEntry*>(achievement)->mapID = 631;    // Correct map requirement (currently has Ulduar)

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u achievement references in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadAchievementCriteriaData()
{
    uint32 oldMSTime = getMSTime();

    m_criteriaDataMap.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT criteria_id, type, value1, value2, ScriptName FROM achievement_criteria_data");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 additional achievement criteria data. DB table `achievement_criteria_data` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 criteria_id = fields[0].GetUInt32();

        CriteriaEntry const* criteria = sAchievementMgr->GetAchievementCriteria(criteria_id);

        if (!criteria)
        {
            sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` has data for non-existing criteria (Entry: %u), ignore.", criteria_id);
            WorldDatabase.PExecute("DELETE FROM `achievement_criteria_data` WHERE criteria_id = %u", criteria_id);
            continue;
        }

        uint32 dataType = fields[1].GetUInt8();
        const char* scriptName = fields[4].GetCString();
        uint32 scriptId = 0;
        if (strcmp(scriptName, "")) // not empty
        {
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` has ScriptName set for non-scripted data type (Entry: %u, type %u), useless data.", criteria_id, dataType);
            else
                scriptId = sObjectMgr->GetScriptId(scriptName);
        }

        AchievementCriteriaData data(dataType, fields[2].GetUInt32(), fields[3].GetUInt32(), scriptId);

        if (!data.IsValid(criteria))
            continue;

        // this will allocate empty data set storage
        AchievementCriteriaDataSet& dataSet = m_criteriaDataMap[criteria_id];
        dataSet.SetCriteriaId(criteria_id);

        // add real data only for not NONE data types
        if (data.dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE)
            dataSet.Add(data);

        // counting data by and data types
        ++count;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u additional achievement criteria data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT achievement FROM character_achievement GROUP BY achievement");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 completed achievements. DB table `character_achievement` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 achievementId = fields[0].GetUInt32();
        const AchievementEntry* achievement = sAchievementMgr->GetAchievement(achievementId);
        if (!achievement)
        {
            // Remove non existent achievements from all characters
            sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement %u data removed from table `character_achievement`.", achievementId);

            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEVMENT);

            stmt->setUInt32(0, uint32(achievementId));

            CharacterDatabase.Execute(stmt);

            continue;
        }
        else if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
            m_allCompletedAchievements.insert(achievementId);
    } 
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %lu completed achievements in %u ms", (unsigned long)m_allCompletedAchievements.size(), GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    m_achievementRewards.clear();                           // need for reload case

    //                                               0      1        2        3     4       5        6          7        8
    QueryResult result = WorldDatabase.Query("SELECT entry, title_A, title_H, item, sender, subject, text, learnSpell, ScriptName FROM achievement_reward");

    if (!result)
    {
        sLog->outError(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 achievement rewards. DB table `achievement_reward` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        const AchievementEntry* pAchievement = GetAchievement(entry);
        if (!pAchievement)
        {
            sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` has wrong achievement (Entry: %u), ignored.", entry);
            continue;
        }

        AchievementReward reward;
        reward.titleId[0] = fields[1].GetUInt32();
        reward.titleId[1] = fields[2].GetUInt32();
        reward.itemId     = fields[3].GetUInt32();
        reward.sender     = fields[4].GetUInt32();
        reward.subject    = fields[5].GetString();
        reward.text       = fields[6].GetString();
        reward.learnSpell = fields[7].GetUInt32();

        const char* scriptName = fields[8].GetCString();
        uint32 scriptId = 0;

        if (strcmp(scriptName, "")) // not empty
            scriptId = sObjectMgr->GetScriptId(scriptName);

        reward.ScriptId = scriptId;

        // must be title or mail at least
        if (!reward.titleId[0] && !reward.titleId[1] && !reward.sender && !reward.learnSpell && !reward.ScriptId)
        {
            sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) does not have title or item reward data, ignored.", entry);
            continue;
        }

        if (pAchievement->requiredFaction == ACHIEVEMENT_FACTION_ANY && ((reward.titleId[0] == 0) != (reward.titleId[1] == 0)))
            sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) has title (A: %u H: %u) for only one team.", entry, reward.titleId[0], reward.titleId[1]);

        if (reward.titleId[0])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[0]);
            if (!titleEntry)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_A`, set to 0", entry, reward.titleId[0]);
                reward.titleId[0] = 0;
            }
        }

        if (reward.titleId[1])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[1]);
            if (!titleEntry)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_H`, set to 0", entry, reward.titleId[1]);
                reward.titleId[1] = 0;
            }
        }

        //check mail data before item for report including wrong item case
        if (reward.sender)
        {
            if (!sObjectMgr->GetCreatureTemplate(reward.sender))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) has invalid creature entry %u as sender, mail reward skipped.", entry, reward.sender);
                reward.sender = 0;
            }
        }
        else if (reward.learnSpell)
        {
            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(reward.learnSpell);
            if (!spellEntry)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) have not existent learn spell %i.", entry, reward.learnSpell);
                continue;
            }
        }else
        {
            if (reward.itemId)
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) does not have sender data but has item reward, item will not be rewarded.", entry);

            if (!reward.subject.empty())
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) does not have sender data but has mail subject.", entry);

            if (!reward.text.empty())
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) does not have sender data but has mail text.", entry);
        }

        if (reward.itemId)
        {
            if (!sObjectMgr->GetItemTemplate(reward.itemId))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_reward` (Entry: %u) has invalid item id %u, reward mail will not contain item.", entry, reward.itemId);
                reward.itemId = 0;
            }
        }

        m_achievementRewards[entry] = reward;
        ++count;

    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u achievement rewards in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadRewardLocales()
{
    uint32 oldMSTime = getMSTime();

    m_achievementRewardLocales.clear();                       // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, subject_loc1, text_loc1, subject_loc2, text_loc2, subject_loc3, text_loc3, subject_loc4, text_loc4, "
                                             "subject_loc5, text_loc5, subject_loc6, text_loc6, subject_loc7, text_loc7, subject_loc8, text_loc8, subject_loc9, text_loc9,"
                                             "subject_loc10, text_loc10 FROM locales_achievement_reward");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 achievement reward locale strings.  DB table `locales_achievement_reward` is empty");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (m_achievementRewards.find(entry) == m_achievementRewards.end())
        {
            sLog->outError(LOG_FILTER_SQL, "Table `locales_achievement_reward` (Entry: %u) has locale strings for non-existing achievement reward.", entry);
            continue;
        }

        AchievementRewardLocale& data = m_achievementRewardLocales[entry];

        for (int i = 1; i < TOTAL_LOCALES; ++i)
        {
            LocaleConstant locale = (LocaleConstant) i;
            ObjectMgr::AddLocaleString(fields[1 + 2 * (i - 1)].GetString(), locale, data.subject);
            ObjectMgr::AddLocaleString(fields[1 + 2 * (i - 1) + 1].GetString(), locale, data.text);
        }
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %lu achievement reward locale strings in %u ms", (unsigned long)m_achievementRewardLocales.size(), GetMSTimeDiffToNow(oldMSTime));
}

AchievementEntry const* AchievementGlobalMgr::GetAchievement(uint32 achievementId) const
{
    return sAchievementStore.LookupEntry(achievementId);
}

CriteriaEntry const* AchievementGlobalMgr::GetAchievementCriteria(uint32 criteriaId) const
{
    return sCriteriaStore.LookupEntry(criteriaId);
}

CriteriaTreeEntry const* AchievementGlobalMgr::GetAchievementCriteriaTree(uint32 criteriaId) const
{
    return sCriteriaTreeStore.LookupEntry(criteriaId);
}
uint32 AchievementGlobalMgr::GetParantTreeId(uint32 parent)
{
    if(CriteriaTreeEntry const* pTree = sCriteriaTreeStore.LookupEntry(parent))
    {
        if(pTree->parent == 0)
            return pTree->ID;
        else
            return GetParantTreeId(pTree->parent);
    }
    return parent;
}
