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
#include "ScriptPCH.h"

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

    switch (criteria->Type)
    {
        case CRITERIA_TYPE_KILL_CREATURE:
        case CRITERIA_TYPE_WIN_BG:
        case CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
        case CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case CRITERIA_TYPE_COMPLETE_QUEST:          // only hardcoded list
        case CRITERIA_TYPE_CAST_SPELL:
        case CRITERIA_TYPE_WIN_RATED_ARENA:
        case CRITERIA_TYPE_DO_EMOTE:
        case CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case CRITERIA_TYPE_WIN_DUEL:
        case CRITERIA_TYPE_LOOT_TYPE:
        case CRITERIA_TYPE_CAST_SPELL2:
        case CRITERIA_TYPE_BE_SPELL_TARGET:
        case CRITERIA_TYPE_BE_SPELL_TARGET2:
        case CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
        case CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case CRITERIA_TYPE_HONORABLE_KILL:
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST:    // only Children's Week achievements
        case CRITERIA_TYPE_USE_ITEM:                // only Children's Week achievements
        case CRITERIA_TYPE_GET_KILLING_BLOWS:
        case CRITERIA_TYPE_REACH_LEVEL:
        case CRITERIA_TYPE_EXPLORE_AREA:
            break;
        default:
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` has data for non-supported criteria type (Entry: %u Type: %u), ignored.", criteria->ID, criteria->Type);
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
                    criteria->ID, criteria->Type, dataType, creature.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE:
            if (classRace.class_id && ((1 << (classRace.class_id-1)) & CLASSMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE (%u) has non-existing class in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id-1)) & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE (%u) has non-existing race in value2 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, classRace.race_id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH:
            if (health.percent < 1 || health.percent > 100)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_PLAYER_LESS_HEALTH (%u) has wrong percent value in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, health.percent);
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
                    criteria->ID, criteria->Type, (dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA?"ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA":"ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA"), dataType, aura.spell_id);
                return false;
            }
            if (aura.effect_idx >= 3)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type %s (%u) has wrong spell effect index in value2 (%u), ignored.",
                    criteria->ID, criteria->Type, (dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA?"ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA":"ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA"), dataType, aura.effect_idx);
                return false;
            }
            if (!spellEntry->Effects[aura.effect_idx]->ApplyAuraName)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type %s (%u) has non-aura spell effect (ID: %u Effect: %u), ignores.",
                    criteria->ID, criteria->Type, (dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA?"ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA":"ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA"), dataType, aura.spell_id, aura.effect_idx);
                return false;
            }
            return true;
        }
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA:
            if (!GetAreaEntryByAreaID(area.id))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA (%u) has wrong area id in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, area.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL:
            if (level.minlevel > STRONG_MAX_LEVEL)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL (%u) has wrong minlevel in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, level.minlevel);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER:
            if (gender.gender > GENDER_NONE)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER (%u) has wrong gender in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, gender.gender);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT:
            if (!ScriptId)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT (%u) does not have ScriptName set, ignored.",
                    criteria->ID, criteria->Type, dataType);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY:
            if (!difficulty.id)
                return false;
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT:
            if (map_players.maxcount <= 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT (%u) has wrong max players count in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, map_players.maxcount);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM:
            if (team.team != ALLIANCE && team.team != HORDE)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM (%u) has unknown team in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, team.team);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK:
            if (drunk.state >= MAX_DRUNKEN)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK (%u) has unknown drunken state in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, drunk.state);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY:
            if (!sHolidaysStore.LookupEntry(holiday.id))
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY (%u) has unknown holiday in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, holiday.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE:
            return true;                                    // not check correctness node indexes
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPED_ITEM:
            if (equipped_item.item_quality >= MAX_ITEM_QUALITY)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPED_ITEM (%u) has unknown quality state in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, equipped_item.item_quality);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE:
            if (!classRace.class_id && !classRace.race_id)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE (%u) must not have 0 in either value field, ignored.",
                    criteria->ID, criteria->Type, dataType);
                return false;
            }
            if (classRace.class_id && ((1 << (classRace.class_id-1)) & CLASSMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE (%u) has non-existing class in value1 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id-1)) & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) for data type ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE (%u) has non-existing race in value2 (%u), ignored.",
                    criteria->ID, criteria->Type, dataType, classRace.race_id);
                return false;
            }
            return true;
        default:
            sLog->outError(LOG_FILTER_SQL, "Table `achievement_criteria_data` (Entry: %u Type: %u) has data for non-supported data type (%u), ignored.", criteria->ID, criteria->Type, dataType);
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
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY:
            return (1 << source->GetMap()->GetDifficulty()) & difficulty.id;
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
AchievementMgr<T>::AchievementMgr(T* owner) : _owner(owner), _achievementPoints(0)
{
    _completedAchievementsArr.assign(MAX_ACHIEVEMENT, NULL);
    _criteriaProgressArr.assign(MAX_CRITERIA_TREE, NULL);
    _timeCriteriaTreesArr.assign(MAX_CRITERIA_TREE, NULL);

    if (GetCriteriaSort() == SCENARIO_CRITERIA)
        m_canUpdateAchiev = 1;
}

template<class T>
AchievementMgr<T>::~AchievementMgr()
{ }

template<class T>
void AchievementMgr<T>::SendPacket(WorldPacket* data) const
{ }

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
void AchievementMgr<T>::RemoveCriteriaProgress(CriteriaTree const* criteriaTree)
{
    CriteriaProgress* criteria = _criteriaProgressArr[criteriaTree->ID];
    if (!criteria)
        return;

    WorldPacket data(SMSG_CRITERIA_DELETED, 4);
    data << uint32(criteriaTree->CriteriaID);
    SendPacket(&data);

    criteria->Counter = 0;
    criteria->deactiveted = true;
}

template<>
void AchievementMgr<ScenarioProgress>::RemoveCriteriaProgress(const CriteriaTree* /*criteriaTree*/)
{
    // FIXME
}

template<>
void AchievementMgr<Guild>::RemoveCriteriaProgress(const CriteriaTree* criteriaTree)
{
    CriteriaProgress* criteria = _criteriaProgressArr[criteriaTree->ID];
    if (!criteria)
        return;

    _criteriaProgressArr[criteriaTree->ID] = NULL;
    ObjectGuid guid = GetOwner()->GetGUID();

    WorldPacket data(SMSG_GUILD_CRITERIA_DELETED, 4 + 8 + 1);
    data << uint32(criteriaTree->CriteriaID);
    data.WriteGuidMask<0, 3, 5, 6, 4, 1, 7, 2>(guid);
    data.WriteGuidBytes<7, 0, 3, 5, 6, 2, 4, 1>(guid);
    SendPacket(&data);

    criteria->Counter = 0;
    criteria->deactiveted = true;
}

template<class T>
void AchievementMgr<T>::ResetAchievementCriteria(CriteriaTypes type, uint32 miscValue1, uint32 miscValue2, bool evenIfCriteriaComplete)
{
    // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::ResetAchievementCriteria(%u, %u, %u)", type, miscValue1, miscValue2);

    // disable for gamemasters with GM-mode enabled
    if (GetOwner()->isGameMaster())
        return;

    CriteriaTreeList const& criteriaTreeList = sAchievementMgr->GetCriteriaTreeByType(type, GetCriteriaSort());
    for (CriteriaTree const* criteriaTree : criteriaTreeList)
    {
        if (!criteriaTree->Criteria)
            continue;

        CriteriaEntry const* criteria = criteriaTree->Criteria->Entry;
        AchievementEntry const* achievement = criteriaTree->Achievement;
        CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(criteriaTree->ID);
        if (!achievement || !tree)
            continue;

        // don't update already completed criteria if not forced or achievement already complete
        if ((IsCompletedCriteriaTree(criteriaTree) && !evenIfCriteriaComplete)/* || HasAchieved(achievement->ID, GetOwner()->GetGUIDLow())*/)
            continue;

        if (criteria->StartEvent == miscValue1 &&
            (!criteria->StartAsset ||
            criteria->StartAsset == miscValue2))
        {
            RemoveCriteriaProgress(tree);
            break;
        }

        if (criteria->FailEvent == miscValue1 &&
            (!criteria->FailAsset ||
            criteria->FailAsset == miscValue2))
        {
            RemoveCriteriaProgress(tree);
            break;
        }
    }
}

template<>
void AchievementMgr<ScenarioProgress>::ResetAchievementCriteria(CriteriaTypes /*type*/, uint32 /*miscValue1*/, uint32 /*miscValue2*/, bool /*evenIfCriteriaComplete*/)
{
    // Not needed
}

template<>
void AchievementMgr<Guild>::ResetAchievementCriteria(CriteriaTypes /*type*/, uint32 /*miscValue1*/, uint32 /*miscValue2*/, bool /*evenIfCriteriaComplete*/)
{
    // Not needed
}

template<class T>
void AchievementMgr<T>::DeleteFromDB(uint64 /*lowguid*/, uint32 /*accountId*/)
{ }

template<>
void AchievementMgr<Player>::DeleteFromDB(uint64 guid, uint32 accountId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT);
    stmt->setUInt32(0, guid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

template<>
void AchievementMgr<Guild>::DeleteFromDB(uint64 guid, uint32 accountId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GUILD_ACHIEVEMENTS);
    stmt->setUInt32(0, guid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GUILD_ACHIEVEMENT_CRITERIA);
    stmt->setUInt32(0, guid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

template<class T>
void AchievementMgr<T>::SaveToDB(SQLTransaction& /*trans*/)
{ }

template<>
void AchievementMgr<Player>::SaveToDB(SQLTransaction& trans)
{
    if (!_completedAchievements.empty())
    {
        bool need_execute = false;
        bool need_execute_acc = false;

        std::ostringstream ssAccIns;
        std::ostringstream ssCharIns;

        for (CompletedAchievementMap::iterator iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
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

    if (_criteriaProgress.empty())
        return;

    {
        uint64 guid      = GetOwner()->GetGUIDLow();
        uint32 accountId = GetOwner()->GetSession()->GetAccountId();

        // ReadGuardType guard(i_criteriaProgressLock);
        for (CriteriaProgressMap::iterator iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
        {
            CriteriaProgress* progress = &iter->second;
            if (!progress || progress->deactiveted || (!progress->changed && !progress->updated))
                continue;

            /// prepare deleting and insert
            bool need_execute_ins       = false;
            bool need_execute_account   = false;

            bool isAccountAchievement   = false;

            bool alreadyOneCharInsLine  = false;
            bool alreadyOneAccInsLine   = false;

            std::ostringstream ssAccins;
            std::ostringstream ssCharins;

            //disable? active before test achivement system
            AchievementEntry const* achievement = progress->achievement;
            if (!achievement)
                continue;

            if (achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT)
            {
                isAccountAchievement = true;
                need_execute_account = true;
            }
            else
                isAccountAchievement = false;

            // store data only for real progress
            bool hasAchieve = HasAchieved(achievement->ID, GetOwner()->GetGUIDLow()) || (achievement->ParentAchievement && !HasAchieved(achievement->ParentAchievement, GetOwner()->GetGUIDLow()));
            if (progress->Counter != 0 && !hasAchieve)
            {
                uint32 achievID = progress->achievement ? progress->achievement->ID : 0;
                if(progress->changed)
                {
                    /// first new/changed record prefix
                    if (!need_execute_ins)
                    {
                        ssAccins  << "REPLACE INTO account_achievement_progress   (account, criteria, counter, date, achievID, completed) VALUES ";
                        ssCharins << "REPLACE INTO character_achievement_progress (guid,    criteria, counter, date, achievID, completed) VALUES ";
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
                        ssAccins  << '(' << accountId << ',' << iter->first << ',' << progress->Counter << ',' << progress->date <<  ',' << achievID << ',' << progress->completed <<')';
                        alreadyOneAccInsLine  = true;
                    }
                    else
                    {
                        ssCharins << '(' << guid      << ',' << iter->first << ',' << progress->Counter << ',' << progress->date << ',' << achievID << ',' << progress->completed << ')';
                        alreadyOneCharInsLine = true;
                    }
                }
                else if(progress->updated)
                {
                    std::ostringstream ssUpd;
                    if (isAccountAchievement)
                        ssUpd << "UPDATE account_achievement_progress SET counter = " << progress->Counter << ", date = " << progress->date << ", achievID = " << achievID << ", completed = " << progress->completed << " WHERE account = " << accountId << " AND criteria = " << iter->first << ';';
                    else
                        ssUpd << "UPDATE character_achievement_progress SET counter = " << progress->Counter << ", date = " << progress->date << ", achievID = " << achievID << ", completed = " << progress->completed << " WHERE guid = " << guid << " AND criteria = " << iter->first << ';';
                    trans->Append(ssUpd.str().c_str());
                }
            }

            /// mark as updated in db
            progress->changed = false;
            progress->updated = false;
            if (need_execute_ins)
            {
                if (need_execute_account && alreadyOneAccInsLine)
                    trans->Append(ssAccins.str().c_str());

                if (alreadyOneCharInsLine)
                    trans->Append(ssCharins.str().c_str());
            }
        }
    }
}

template<>
void AchievementMgr<Guild>::SaveToDB(SQLTransaction& trans)
{
    PreparedStatement* stmt;
    std::ostringstream guidstr;
    for (CompletedAchievementMap::iterator itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        if (!itr->second.changed)
            continue;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT);
        stmt->setUInt32(0, GetOwner()->GetId());
        stmt->setUInt32(1, itr->first);
        stmt->setUInt32(2, itr->second.date);
        for (GuidSet::const_iterator gItr = itr->second.guids.begin(); gItr != itr->second.guids.end(); ++gItr)
            guidstr << GUID_LOPART(*gItr) << ',';

        stmt->setString(3, guidstr.str());
        trans->Append(stmt);
        itr->second.changed = false;

        guidstr.str("");
    }

    {
        // ReadGuardType guard(i_criteriaProgressLock);
        for (CriteriaProgressMap::iterator itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
        {
            if (itr->second.deactiveted)
                  continue;

            if (itr->second.changed)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT_CRITERIA);
                stmt->setUInt32(0, GetOwner()->GetId());
                stmt->setUInt32(1, itr->first);
                stmt->setUInt32(2, itr->second.Counter);
                stmt->setUInt32(3, itr->second.date);
                stmt->setUInt32(4, GUID_LOPART(itr->second.PlayerGUID));
                stmt->setUInt32(5, itr->second.achievement ? itr->second.achievement->ID : 0);
                stmt->setUInt32(6, itr->second.completed);
                trans->Append(stmt);
                itr->second.changed = false;
            }

            if (itr->second.updated)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_ACHIEVEMENT_CRITERIA);
                stmt->setUInt32(0, itr->second.Counter);
                stmt->setUInt32(1, itr->second.date);
                stmt->setUInt32(2, itr->second.achievement ? itr->second.achievement->ID : 0);
                stmt->setUInt32(3, itr->second.completed);
                stmt->setUInt32(4, GetOwner()->GetId());
                stmt->setUInt32(5, itr->first);
                trans->Append(stmt);
                itr->second.updated = false;
            }
        }
    }
}

template<class T>
void AchievementMgr<T>::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult achievementAccountResult, PreparedQueryResult criteriaAccountResult)
{ }

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
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = _completedAchievements[achievementid];
            ca.date = time_t(fields[2].GetUInt32());
            ca.changed = false;
            ca.first_guid = first_guid;
            ca.isAccountAchievement = achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT;
            _completedAchievementsArr[achievementid] = &ca;

            _achievementPoints += achievement->RewardPoints;

            // title achievement rewards are retroactive
            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
            {
                if (uint32 titleId = reward->titleId[Player::TeamForRace(GetOwner()->getRace()) == ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        GetOwner()->SetTitle(titleEntry);
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
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            // not added on account?
            if (!_completedAchievementsArr[achievementid])
            {
                CompletedAchievementData& ca = _completedAchievements[achievementid];
                ca.changed = true;
                ca.first_guid = GetOwner()->GetGUIDLow();
                ca.date = time_t(fields[1].GetUInt32());
                _achievementPoints += achievement->RewardPoints;
                _completedAchievementsArr[achievementid] = &ca;
            }
            else
            {
                CompletedAchievementData* ca = _completedAchievementsArr[achievementid];
                ca->changed = false;
                ca->first_guid = GetOwner()->GetGUIDLow();
                ca->date = time_t(fields[1].GetUInt32());
            }

            // title achievement rewards are retroactive
            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
            {
                if (uint32 titleId = reward->titleId[Player::TeamForRace(GetOwner()->getRace()) == ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        GetOwner()->SetTitle(titleEntry);
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
            bool completed               = fields[4].GetUInt32();
            bool update = false;

            CriteriaTreeEntry const* criteriaTree = sCriteriaTreeStore.LookupEntry(char_criteria_id);
            if (!criteriaTree)
            {
                // we will remove not existed criteriaTree for all characters
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteriaTree %u data removed from table `character_achievement_progress`.", char_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, char_criteria_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            Criteria const* criteria = sAchievementMgr->GetCriteria(criteriaTree->CriteriaID);
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
                uint32 parent = sAchievementMgr->GetParantTreeId(criteriaTree->Parent);
                achievement = GetsAchievementByTreeList(parent);
                update = true;
            }
            else
                achievement = sAchievementStore.LookupEntry(achievementID);

            bool hasAchieve = !achievement || HasAchieved(achievement->ID, GetOwner()->GetGUIDLow());
            if (hasAchieve)
            {
                // we will remove already completed criteria
                sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "Achievement %s with progress char_criteria_id %u data removed from table `character_achievement_progress` ", achievement ? "completed" : "not exist", char_criteria_id);
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, char_criteria_id);
                stmt->setUInt32(1, GetOwner()->GetGUIDLow());
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[char_criteria_id];
            _criteriaProgressArr[char_criteria_id] = &progress;
            progress.Counter = fields[1].GetUInt32();
            progress.date    = date;
            progress.changed = false;
            progress.updated = update;
            progress.completed = completed;
            progress.deactiveted = false;
            progress.achievement = achievement;
            progress.criteriaTree = criteriaTree;
            progress._criteria = criteria;
            progress.criteria = criteria->Entry;
            progress.parent = sCriteriaTreeStore.LookupEntry(criteriaTree->Parent);
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
            uint32 achievementID        = fields[3].GetUInt32();
            bool completed              = fields[4].GetUInt32();
            bool update = false;

            CriteriaTreeEntry const* criteriaTree = sCriteriaTreeStore.LookupEntry(acc_criteria_id);
            if (!criteriaTree)
            {
                // we will remove not existed criteria for all characters
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `character_achievement_progress`.", acc_criteria_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, acc_criteria_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            Criteria const* criteria = sAchievementMgr->GetCriteria(criteriaTree->CriteriaID);
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
                uint32 parent = sAchievementMgr->GetParantTreeId(criteriaTree->Parent);
                achievement = GetsAchievementByTreeList(parent);
                update = true;
            }
            else
                achievement = sAchievementStore.LookupEntry(achievementID);

            bool hasAchieve = !achievement || HasAchieved(achievement->ID);
            if (hasAchieve)
            {
                // we will remove already completed criteria
                sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "Achievement %s with progress acc_criteria_id %u data removed from table `account_achievement_progress` ", achievement ? "completed" : "not exist", acc_criteria_id);
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ACC_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, acc_criteria_id);
                stmt->setUInt32(1, GetOwner()->GetSession()->GetAccountId());
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            // Achievement in both account & characters achievement_progress, problem
            if (_criteriaProgressArr[acc_criteria_id] != NULL)
            {
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Achievement '%u' in both account & characters achievement_progress", acc_criteria_id);
                continue;
            }
            CriteriaProgress& progress = _criteriaProgress[acc_criteria_id];
            _criteriaProgressArr[acc_criteria_id] = &progress;
            progress.Counter = fields[1].GetUInt32();
            progress.date    = date;
            progress.changed = false;
            progress.updated = update;
            progress.completed = completed;
            progress.deactiveted = false;
            progress.achievement = achievement;
            progress.criteriaTree = criteriaTree;
            progress._criteria = criteria;
            progress.criteria = criteria->Entry;
            progress.parent = sCriteriaTreeStore.LookupEntry(criteriaTree->Parent);
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

            CompletedAchievementData& ca = _completedAchievements[achievementid];
            ca.date = time_t(fields[1].GetUInt32());
            Tokenizer guids(fields[2].GetString(), ' ');
            for (size_t i = 0; i < guids.size(); ++i)
                 ca.guids.insert(MAKE_NEW_GUID(atol(guids[i]), 0, HIGHGUID_PLAYER));

            ca.changed = false;
            _achievementPoints += achievement->RewardPoints;
            _completedAchievementsArr[achievementid] = &ca;

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
            bool completed                = fields[5].GetUInt32();
            bool update = false;

            CriteriaTreeEntry const* criteriaTree = sCriteriaTreeStore.LookupEntry(guild_criteriaTree_id);
            if (!criteriaTree)
            {
                // we will remove not existed criteria for all guilds
                sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement criteria %u data removed from table `guild_achievement_progress`.", guild_criteriaTree_id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, guild_criteriaTree_id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            Criteria const* criteria = sAchievementMgr->GetCriteria(criteriaTree->CriteriaID);
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
                uint32 parent = sAchievementMgr->GetParantTreeId(criteriaTree->Parent);
                achievement = GetsAchievementByTreeList(parent);
                update = true;
            }
            else
                achievement = sAchievementStore.LookupEntry(achievementID);

            bool hasAchieve = !achievement || HasAchieved(achievement->ID);
            if (hasAchieve)
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, guild_criteriaTree_id);
                stmt->setUInt32(1, GetOwner()->GetId());
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[guild_criteriaTree_id];
            _criteriaProgressArr[guild_criteriaTree_id] = &progress;
            progress.Counter = fields[1].GetUInt32();
            progress.date    = date;
            progress.PlayerGUID = MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER);
            progress.changed = false;
            progress.completed = completed;
            progress.deactiveted = false;
            progress.updated = update;
            progress.achievement = achievement;
            progress.criteriaTree = criteriaTree;
            progress._criteria = criteria;
            progress.criteria = criteria->Entry;
            progress.parent = sCriteriaTreeStore.LookupEntry(criteriaTree->Parent);
        } while (criteriaResult->NextRow());
    }
    m_canUpdateAchiev = 1;
}

template<class T>
void AchievementMgr<T>::Reset()
{ }

template<>
void AchievementMgr<ScenarioProgress>::Reset()
{
    // FIXME
}

template<>
void AchievementMgr<Player>::Reset()
{
    for (CompletedAchievementMap::const_iterator iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_ACHIEVEMENT_DELETED, 4 + 4);
        data << uint32(iter->first);
        data << uint32(0);
        SendPacket(&data);
    }

    _completedAchievements.clear();
    _achievementPoints = 0;
    DeleteFromDB(GetOwner()->GetGUID());

    if (_criteriaProgress.empty())
        return;

    _criteriaProgressArr.assign(MAX_CRITERIA_TREE, NULL);

    {
        // WriteGuardType guard(i_criteriaProgressLock);
        for (CriteriaProgressMap::iterator iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
        {
            WorldPacket data(SMSG_CRITERIA_DELETED, 4);
            data << uint32(iter->second.criteriaTree->CriteriaID);
            SendPacket(&data);
        }
        _criteriaProgress.clear();
    }
    // re-fill data
    CheckAllAchievementCriteria(GetOwner());
}

template<class T>
void AchievementMgr<T>::ClearMap()
{
    m_canUpdateAchiev = 0;

    _completedAchievementsArr.clear();
    _completedAchievements.clear();

    {
        // WriteGuardType guard(i_criteriaProgressLock);
        _criteriaProgressArr.clear();
        _criteriaProgress.clear();
    }
}

template<>
void AchievementMgr<Guild>::Reset()
{
    ObjectGuid guid = GetOwner()->GetGUID();
    for (CompletedAchievementMap::const_iterator iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
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

    _completedAchievements.clear();
    DeleteFromDB(guid);

    if (_criteriaProgress.empty())
        return;

    {
        // WriteGuardType guard(i_criteriaProgressLock);
        for (CriteriaProgressMap::iterator iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
            if (CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(iter->second.criteriaTree->ID))
                RemoveCriteriaProgress(tree);
        _criteriaProgress.clear();
    }
    _achievementPoints = 0;
}

template<class T>
void AchievementMgr<T>::SendAchievementEarned(AchievementEntry const* achievement) const
{
    // Don't send for achievements with ACHIEVEMENT_FLAG_HIDDEN
    if (achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    if (Guild* guild = sGuildMgr->GetGuildById(GetOwner()->GetGuildId()))
    {
        Trinity::AchievementChatBuilder say_builder(*GetOwner(), CHAT_MSG_GUILD_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED, achievement->ID);
        Trinity::LocalizedPacketDo<Trinity::AchievementChatBuilder> say_do(say_builder);
        guild->BroadcastWorker(say_do);
    }

    ObjectGuid ownerGuid = GetOwner()->GetGUID();

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        std::string name = GetOwner()->GetName();
        WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, strlen(GetOwner()->GetName()) + 1 + 8 + 4 + 4);
        data.WriteGuidMask<6, 4, 0, 1, 3, 7, 5>(ownerGuid);
        data.WriteBits(name.size(), 7);
        data.WriteBit(1);                                   // 0=link supplied string as player name, 1=display plain string
        data.WriteGuidMask<2>(ownerGuid);
        data.WriteGuidBytes<6, 1, 5>(ownerGuid);
        data.WriteString(name);
        data.WriteGuidBytes<3, 7, 4, 0, 2>(ownerGuid);
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
        cell.Visit(p, Trinity::makeWorldVisitor(say_worker), *GetOwner()->GetMap(), *GetOwner(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
    }

    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    ObjectGuid thisPlayerGuid = GetOwner()->GetGUID();
    ObjectGuid firstPlayerOnAccountGuid = ownerGuid;

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
void AchievementMgr<T>::SendCriteriaUpdate(CriteriaProgress const* /*progress*/, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{ }

template<class T>
void AchievementMgr<T>::SendAccountCriteriaUpdate(CriteriaProgress const* /*progress*/, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{ }

template<>
void AchievementMgr<Player>::SendCriteriaUpdate(CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const
{
    if (!GetOwner() || !progress || !progress->criteria)
        return;

    WorldPacket data(SMSG_CRITERIA_UPDATE, 8 + 4 + 8);
    data << uint32(progress->criteria->ID);
    data << uint32(secsToTimeBitFields(progress->date)); // Date
    data << uint32(timeElapsed);                    // TimeFromStart / TimeFromCreate, normally even 0
    data << uint64(progress->Counter);

    if (!progress->criteria->StartTimer)
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
void AchievementMgr<Player>::SendAccountCriteriaUpdate(CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const
{
    if (!GetOwner() || !progress || !progress->criteria)
        return;

    WorldPacket data(SMSG_ACCOUNT_CRITERIA_UPDATE);
    ObjectGuid guid = GetOwner()->GetGUID();         // needed send first completer criteria guid or else not found - then current player guid

    data.WriteGuidMask<6, 7>(progress->Counter);
    data.WriteGuidMask<6>(guid);
    data.WriteGuidMask<1>(progress->Counter);
    data.WriteBits(0, 4);                         // Flags
    data.WriteGuidMask<4, 5>(guid);
    data.WriteGuidMask<2, 5, 0>(progress->Counter);
    data.WriteGuidMask<0, 3, 1, 2>(guid);
    data.WriteGuidMask<4, 3>(progress->Counter);
    data.WriteGuidMask<7>(guid);

    data.WriteGuidBytes<0, 5, 3>(guid);
    data.WriteGuidBytes<7>(progress->Counter);
    data.WriteGuidBytes<6, 1, 2>(guid);
    data.WriteGuidBytes<1>(progress->Counter);
    data << uint32(0);
    data << uint32(0);
    data.WriteGuidBytes<2, 3>(progress->Counter);
    data.WriteGuidBytes<4>(guid);
    data.WriteGuidBytes<6, 0, 4>(progress->Counter);
    data << uint32(secsToTimeBitFields(progress->date));   // Date
    data.WriteGuidBytes<7>(guid);
    data << uint32(progress->criteria->ID);
    data.WriteGuidBytes<5>(progress->Counter);

    SendPacket(&data);
}

template<>
void AchievementMgr<ScenarioProgress>::SendCriteriaUpdate(CriteriaProgress const* progress, uint32 /*timeElapsed*/, bool timedCompleted) const
{
    // FIXME
    GetOwner()->SendCriteriaUpdate(progress);
    if (timedCompleted)
        GetOwner()->UpdateCurrentStep(false);
}

template<>
void AchievementMgr<Guild>::SendCriteriaUpdate(CriteriaProgress const* progress, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{
    if (!GetOwner() || !progress || !progress->criteria)
        return;

    //will send response to criteria progress request
    WorldPacket data(SMSG_GUILD_CRITERIA_UPDATE, 3 + 1 + 1 + 8 + 8 + 4 + 4 + 4 + 4 + 4);

    ObjectGuid Counter = progress->Counter; // for accessing every byte individually
    ObjectGuid guid = progress->PlayerGUID;

    data.WriteBits(1, 19);                 // Criteria count
    data.WriteGuidMask<4, 2, 6>(guid);
    data.WriteGuidMask<1, 5>(Counter);
    data.WriteGuidMask<3>(guid);
    data.WriteGuidMask<2>(Counter);
    data.WriteGuidMask<0, 5>(guid);
    data.WriteGuidMask<3>(Counter);
    data.WriteGuidMask<1>(guid);
    data.WriteGuidMask<7>(Counter);
    data.WriteGuidMask<7>(guid);
    data.WriteGuidMask<0, 6, 4>(Counter);

    data.WriteGuidBytes<0>(Counter);
    data << uint32(secsToTimeBitFields(progress->date)); // DateUpdated?
    data.WriteGuidBytes<2>(guid);
    data.WriteGuidBytes<1>(Counter);
    data << uint32(0);                      // Flags
    data.WriteGuidBytes<7, 6>(Counter);
    data.WriteGuidBytes<0>(guid);
    data << uint32(progress->date);         // DateStarted / DateCreated
    data.WriteGuidBytes<6, 7>(guid);
    data.WriteGuidBytes<4>(Counter);
    data.WriteGuidBytes<5>(guid);
    data << uint32(progress->criteria->ID);
    data.WriteGuidBytes<4, 1>(guid);
    data << uint32(::time(NULL) - progress->date); // DateStarted / DateCreated
    data.WriteGuidBytes<5, 2>(Counter);
    data.WriteGuidBytes<3>(guid);
    data.WriteGuidBytes<3>(Counter);

    SendPacket(&data);
}

/**
 * called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
 */
template<class T>
void AchievementMgr<T>::CheckAllAchievementCriteria(Player* referencePlayer)
{
    // suppress sending packets
    for (uint32 i = 0; i < CRITERIA_TYPE_TOTAL; ++i)
        UpdateAchievementCriteria(CriteriaTypes(i), 0, 0, 0, NULL, referencePlayer, true);

    m_canUpdateAchiev = 1;
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
void AchievementMgr<T>::UpdateAchievementCriteria(CriteriaTypes type, uint32 miscValue1 /*= 0*/, uint32 miscValue2 /*= 0*/, uint32 miscValue3 /*= 0*/,Unit const* unit /*= NULL*/, Player* referencePlayer /*= NULL*/, bool init /*=false*/)
{
    // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "UpdateAchievementCriteria type %u (%u, %u, %u) CriteriaSort %u", type, miscValue1, miscValue2, miscValue3, GetCriteriaSort());

    // Prevent update if player not loading
    if (!CanUpdate())
        return;

    // disable for gamemasters with GM-mode enabled
    if (referencePlayer->isGameMaster())
        return;

     // Lua_GetGuildLevelEnabled() is checked in achievement UI to display guild tab
    if (GetCriteriaSort() == GUILD_CRITERIA && !sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        return;

    CriteriaTreeList const& criteriaList = sAchievementMgr->GetCriteriaTreeByType(type, GetCriteriaSort());

    // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "UpdateAchievementCriteria type %u criteriaList %u", type, criteriaList.size());

    if(criteriaList.empty())
        return;

    for (CriteriaTree const* tree : criteriaList)
    {
        CriteriaTreeEntry const* criteriaTree = tree->Entry;
        CriteriaEntry const* criteria = tree->Criteria ? tree->Criteria->Entry : NULL;
        AchievementEntry const* achievement = tree->Achievement;

        if(!criteriaTree || !criteria)
            continue;

        bool canComplete = false;
        if (!CanUpdateCriteria(tree, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
            continue;

        // requirements not found in the dbc
        if (AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(tree))
            if (!data->Meets(referencePlayer, unit, miscValue1))
                continue;

        switch (type)
        {
            // std. case: increment at 1
            case CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
            case CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            case CRITERIA_TYPE_LOSE_DUEL:
            case CRITERIA_TYPE_CREATE_AUCTION:
            case CRITERIA_TYPE_WON_AUCTIONS:    /* FIXME: for online player only currently */
            case CRITERIA_TYPE_ROLL_NEED:
            case CRITERIA_TYPE_ROLL_GREED:
            case CRITERIA_TYPE_QUEST_ABANDONED:
            case CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            case CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
            case CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            case CRITERIA_TYPE_LOOT_EPIC_ITEM:
            case CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            case CRITERIA_TYPE_DEATH:
            case CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            case CRITERIA_TYPE_DEATH_AT_MAP:
            case CRITERIA_TYPE_DEATH_IN_DUNGEON:
            case CRITERIA_TYPE_KILLED_BY_CREATURE:
            case CRITERIA_TYPE_KILLED_BY_PLAYER:
            case CRITERIA_TYPE_DEATHS_FROM:
            case CRITERIA_TYPE_BE_SPELL_TARGET:
            case CRITERIA_TYPE_BE_SPELL_TARGET2:
            case CRITERIA_TYPE_CAST_SPELL:
            case CRITERIA_TYPE_CAST_SPELL2:
            case CRITERIA_TYPE_WIN_RATED_ARENA:
            case CRITERIA_TYPE_USE_ITEM:
            case CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            case CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            case CRITERIA_TYPE_DO_EMOTE:
            case CRITERIA_TYPE_USE_GAMEOBJECT:
            case CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            case CRITERIA_TYPE_WIN_DUEL:
            case CRITERIA_TYPE_HK_CLASS:
            case CRITERIA_TYPE_HK_RACE:
            case CRITERIA_TYPE_HONORABLE_KILL:
            case CRITERIA_TYPE_SPECIAL_PVP_KILL:
            case CRITERIA_TYPE_GET_KILLING_BLOWS:
            case CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            case CRITERIA_TYPE_INSTANSE_MAP_ID:
            case CRITERIA_TYPE_WIN_ARENA:
            case CRITERIA_TYPE_SCRIPT_EVENT:
            case CRITERIA_TYPE_SCRIPT_EVENT_2:
            case CRITERIA_TYPE_COMPLETE_RAID:
            case CRITERIA_TYPE_PLAY_ARENA:
            case CRITERIA_TYPE_OWN_RANK:
            //case CRITERIA_TYPE_EARNED_PVP_TITLE:
            case CRITERIA_TYPE_KILL_CREATURE_TYPE:
            case CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
            case CRITERIA_TYPE_CATCH_FROM_POOL:
            case CRITERIA_TYPE_BUY_GUILD_EMBLEM:
            case CRITERIA_TYPE_OBTAIN_BATTLEPET:
            case CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
            case CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
            case CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
            case CRITERIA_TYPE_BATTLEPET_WIN:
            case CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
            case CRITERIA_TYPE_COMPLETE_SCENARIO:
                canComplete = SetCriteriaProgress(tree, init ? 0 : 1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            // std case: increment at miscValue1
            case CRITERIA_TYPE_MONEY_FROM_VENDORS:
            case CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            case CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            case CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            case CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            case CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            case CRITERIA_TYPE_LOOT_MONEY:
            case CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:/* FIXME: for online player only currently */
            case CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            case CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
            case CRITERIA_TYPE_WIN_BG:
            case CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
            case CRITERIA_TYPE_DAMAGE_DONE:
            case CRITERIA_TYPE_HEALING_DONE:
            case CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
            case CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
            case CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
                canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            case CRITERIA_TYPE_KILL_CREATURE:
            case CRITERIA_TYPE_LOOT_TYPE:
            case CRITERIA_TYPE_OWN_ITEM:
            case CRITERIA_TYPE_LOOT_ITEM:
            case CRITERIA_TYPE_CURRENCY:
            case CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            case CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
            case CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
                canComplete = SetCriteriaProgress(tree, miscValue2, referencePlayer, PROGRESS_ACCUMULATE);
                break;
                // std case: high value at miscValue1
            case CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            case CRITERIA_TYPE_HIGHEST_AUCTION_SOLD: /* FIXME: for online player only currently */
            case CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            case CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            case CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            case CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
            case CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
            case CRITERIA_TYPE_REACH_RBG_RATING:
                canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_HIGHEST);
                break;
            case CRITERIA_TYPE_REACH_LEVEL:
                canComplete = SetCriteriaProgress(tree, referencePlayer->getLevel(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_REACH_SKILL_LEVEL:
                if (uint32 skillvalue = referencePlayer->GetBaseSkillValue(criteria->Asset))
                    canComplete = SetCriteriaProgress(tree, skillvalue, referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_LEARN_SKILL_LEVEL:
                if (uint32 maxSkillvalue = referencePlayer->GetPureMaxSkillValue(criteria->Asset))
                    canComplete = SetCriteriaProgress(tree, maxSkillvalue, referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetRewardedQuestCount(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            {
                time_t nextDailyResetTime = sWorld->GetNextDailyQuestsResetTime();
                CriteriaProgress* progress = GetCriteriaProgress(criteriaTree->ID);

                if (!miscValue1) // Login case.
                {
                    // reset if player missed one day.
                    if (progress && progress->date < (nextDailyResetTime - 2 * DAY))
                        canComplete = SetCriteriaProgress(tree, 0, referencePlayer, PROGRESS_SET);
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

                canComplete = SetCriteriaProgress(tree, 1, referencePlayer, progressType);
                break;
            }
            case CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            {
                uint64 Counter = 0;
                const RewardedQuestSet &rewQuests = referencePlayer->getRewardedQuests();
                for (RewardedQuestSet::const_iterator itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
                {
                    Quest const* quest = sObjectMgr->GetQuestTemplate(*itr);
                    if (quest && quest->GetZoneOrSort() >= 0 && uint32(quest->GetZoneOrSort()) == criteria->Asset)
                        ++Counter;
                }
                canComplete = SetCriteriaProgress(tree, Counter, referencePlayer, PROGRESS_SET);
                break;
            }
            case CRITERIA_TYPE_FALL_WITHOUT_DYING:
                // miscValue1 is the ingame fallheight*100 as stored in dbc
                canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_COMPLETE_QUEST:
            case CRITERIA_TYPE_LEARN_SPELL:
            case CRITERIA_TYPE_EXPLORE_AREA:
            case CRITERIA_TYPE_VISIT_BARBER_SHOP:
            case CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            case CRITERIA_TYPE_EQUIP_ITEM:
            case CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                canComplete = SetCriteriaProgress(tree, 1, referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_BUY_BANK_SLOT:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetBankBagSlotCount(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_GAIN_REPUTATION:
                {
                    int32 reputation = referencePlayer->GetReputationMgr().GetReputation(criteria->Asset);
                    if (reputation > 0)
                        canComplete = SetCriteriaProgress(tree, reputation, referencePlayer, PROGRESS_SET);
                    break;
                }
            case CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetReputationMgr().GetExaltedFactionCount(), referencePlayer, PROGRESS_SET);
                  break;
            case CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            case CRITERIA_TYPE_LEARN_SKILL_LINE:
            {
                uint32 spellCount = 0;
                for (PlayerSpellMap::const_iterator spellIter = referencePlayer->GetSpellMap().begin();
                    spellIter != referencePlayer->GetSpellMap().end();
                    ++spellIter)
                {
                    SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellIter->first);
                    for (SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                    {
                        if (skillIter->second->skillId == criteria->Asset)
                            spellCount++;
                    }
                }
                canComplete = SetCriteriaProgress(tree, spellCount, referencePlayer, PROGRESS_SET);
                break;
            }
            case CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetReputationMgr().GetReveredFactionCount(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetReputationMgr().GetHonoredFactionCount(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_KNOWN_FACTIONS:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetReputationMgr().GetVisibleFactionCount(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_EARN_HONORABLE_KILL:
                if (!miscValue1)
                    canComplete = SetCriteriaProgress(tree, referencePlayer->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS), referencePlayer, PROGRESS_HIGHEST);
                else
                    canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            case CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetMoney(), referencePlayer, PROGRESS_HIGHEST);
                break;
            case CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            case CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                if (!miscValue1)
                    canComplete = SetCriteriaProgress(tree, _achievementPoints, referencePlayer, PROGRESS_SET);
                else
                    canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_ACCUMULATE);
                break;
            case CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                {
                    if (miscValue1)
                    {
                        if (miscValue2 != criteria->Asset)
                            continue;
                        canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_HIGHEST);
                    }
                    else // login case
                    {
                        for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
                        {
                            Bracket* bracket = referencePlayer->getBracket(BracketType(arena_slot));
                            if (!bracket || arena_slot != criteria->Asset)
                                continue;
                            canComplete = SetCriteriaProgress(tree, bracket->getRating(), referencePlayer, PROGRESS_HIGHEST);
                            break;
                        }
                    }
                }
                break;
            case CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            {
                if (miscValue1)
                {
                    if (miscValue2 != criteria->Asset)
                        continue;
                    canComplete = SetCriteriaProgress(tree, miscValue1, referencePlayer, PROGRESS_HIGHEST);
                }
                else // login case
                {
                    for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
                    {
                        Bracket* bracket = referencePlayer->getBracket(BracketType(arena_slot));
                        if (!bracket || arena_slot != criteria->Asset)
                            continue;
                        canComplete = SetCriteriaProgress(tree, bracket->getRating(), referencePlayer, PROGRESS_HIGHEST);
                    }
                }
                break;
            }
            case CRITERIA_TYPE_REACH_GUILD_LEVEL:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetGuildLevel(), referencePlayer, PROGRESS_SET);
                break;
            case CRITERIA_TYPE_COLLECT_BATTLEPET:
                canComplete = SetCriteriaProgress(tree, referencePlayer->GetBattlePetMgr()->GetPetJournal().size(), referencePlayer, PROGRESS_SET);
                break;
            // FIXME: not triggered in code as result, need to implement
            case CRITERIA_TYPE_COMPLETED_LFG_DUNGEONS:
            case CRITERIA_TYPE_INITIATED_KICK_IN_LFG:
            case CRITERIA_TYPE_VOTED_KICK_IN_LFG:
            case CRITERIA_TYPE_BEING_KICKED_IN_LFG:
            case CRITERIA_TYPE_ABANDONED_LFG_DUNGEONS:
            case CRITERIA_TYPE_UNK137:
            case CRITERIA_TYPE_COMPLETE_GUILD_DUNGEON_CHALLENGES:
            case CRITERIA_TYPE_COMPLETE_GUILD_CHALLENGES:
            case CRITERIA_TYPE_UNK140:
            case CRITERIA_TYPE_UNK141:
            case CRITERIA_TYPE_UNK142:
            case CRITERIA_TYPE_UNK143:
            case CRITERIA_TYPE_UNK144:
            case CRITERIA_TYPE_COMPLETED_LFR_DUNGEONS:
            case CRITERIA_TYPE_ABANDONED_LFR_DUNGEONS:
            case CRITERIA_TYPE_INITIATED_KICK_IN_LFR:
            case CRITERIA_TYPE_VOTED_KICK_IN_LFR:
            case CRITERIA_TYPE_BEING_KICKED_IN_LFR:
            case CRITERIA_TYPE_COUNT_OF_LFR_QUEUE_BOOSTS_BY_TANK:
            case CRITERIA_TYPE_COMPLETE_SCENARIO_COUNT:
            case CRITERIA_TYPE_REACH_SCENARIO_BOSS:
            case CRITERIA_TYPE_UNK154:
            case CRITERIA_TYPE_UNK159:
            case CRITERIA_TYPE_LEVEL_BATTLE_PET_CREDIT:
                break;                                   // Not implemented yet :(
        }

        if (!achievement || !canComplete)
            continue;

        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "UpdateAchievementCriteria criteriaTree %u, achievement %u, criteria %u canComplete %u", criteriaTree->ID, achievement->ID, criteria->ID, canComplete);
        // Counter can never complete
        if (IsCompletedAchievement(achievement, referencePlayer))
            CompletedAchievement(achievement, referencePlayer);

        if (AchievementEntryList const* achRefList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
        {
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
            {
                // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "UpdateAchievementCriteria achievement %u achRef %u", achievement->ID, (*itr)->ID);
                if (IsCompletedAchievement(*itr, referencePlayer))
                    CompletedAchievement(*itr, referencePlayer);
            }
        }
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
    if (achievement && achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
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
bool AchievementMgr<T>::CanUpdateCriteriaTree(CriteriaTree const* tree, Player* referencePlayer) const
{
    if ((tree->Entry->Flags & CRITERIA_TREE_FLAG_HORDE_ONLY && referencePlayer->GetTeam() != HORDE) ||
        (tree->Entry->Flags & CRITERIA_TREE_FLAG_ALLIANCE_ONLY && referencePlayer->GetTeam() != ALLIANCE))
    {
        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteriaTree tree %u", tree->ID);
        return false;
    }

    return true;
}

template<class T>
bool AchievementMgr<T>::IsCompletedCriteria(CriteriaTree const* tree, uint64 requiredAmount)
{
    if (!tree->Criteria)
        return false;

    CriteriaProgress* progress = GetCriteriaProgress(tree->ID);
    if (!progress)
        return false;

    switch (tree->Criteria->Entry->Type)
    {
        case CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            progress->completed = progress->Counter >= (requiredAmount * 75); // skillLevel * 75
            break;
        case CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
        case CRITERIA_TYPE_COMPLETE_QUEST:
        case CRITERIA_TYPE_LEARN_SPELL:
        case CRITERIA_TYPE_EXPLORE_AREA:
            progress->completed = progress->Counter >= 1;
            break;
        case CRITERIA_TYPE_WIN_ARENA:
            progress->completed = requiredAmount && progress->Counter >= requiredAmount;
            break;
        case CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            progress->completed = progress->Counter >= 9000;
            break;
        case CRITERIA_TYPE_EARNED_PVP_TITLE:
            progress->completed = true;
            break;
        case CRITERIA_TYPE_WIN_BG:
        case CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
        case CRITERIA_TYPE_COMPLETE_RAID:
        case CRITERIA_TYPE_PLAY_ARENA:
        case CRITERIA_TYPE_OWN_RANK:
        case CRITERIA_TYPE_KILL_CREATURE_TYPE:
        case CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
        case CRITERIA_TYPE_CATCH_FROM_POOL:
        case CRITERIA_TYPE_BUY_GUILD_EMBLEM:
        case CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
        case CRITERIA_TYPE_KILL_CREATURE:
        case CRITERIA_TYPE_REACH_LEVEL:
        case CRITERIA_TYPE_REACH_GUILD_LEVEL:
        case CRITERIA_TYPE_REACH_SKILL_LEVEL:
        case CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
        case CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
        case CRITERIA_TYPE_DAMAGE_DONE:
        case CRITERIA_TYPE_HEALING_DONE:
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
        case CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case CRITERIA_TYPE_BE_SPELL_TARGET:
        case CRITERIA_TYPE_BE_SPELL_TARGET2:
        case CRITERIA_TYPE_CAST_SPELL:
        case CRITERIA_TYPE_CAST_SPELL2:
        case CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
        case CRITERIA_TYPE_HONORABLE_KILL:
        case CRITERIA_TYPE_EARN_HONORABLE_KILL:
        case CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
        case CRITERIA_TYPE_OWN_ITEM:
        case CRITERIA_TYPE_WIN_RATED_ARENA:
        case CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
        case CRITERIA_TYPE_USE_ITEM:
        case CRITERIA_TYPE_LOOT_ITEM:
        case CRITERIA_TYPE_BUY_BANK_SLOT:
        case CRITERIA_TYPE_GAIN_REPUTATION:
        case CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
        case CRITERIA_TYPE_VISIT_BARBER_SHOP:
        case CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
        case CRITERIA_TYPE_HK_CLASS:
        case CRITERIA_TYPE_HK_RACE:
        case CRITERIA_TYPE_DO_EMOTE:
        case CRITERIA_TYPE_EQUIP_ITEM:
        case CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
        case CRITERIA_TYPE_LOOT_MONEY:
        case CRITERIA_TYPE_USE_GAMEOBJECT:
        case CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
        case CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
        case CRITERIA_TYPE_WIN_DUEL:
        case CRITERIA_TYPE_LOOT_TYPE:
        case CRITERIA_TYPE_LEARN_SKILL_LINE:
        case CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
        case CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
        case CRITERIA_TYPE_GET_KILLING_BLOWS:
        case CRITERIA_TYPE_CURRENCY:
        case CRITERIA_TYPE_INSTANSE_MAP_ID:
        case CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
        case CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
        case CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
        case CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
        case CRITERIA_TYPE_REACH_RBG_RATING:
        case CRITERIA_TYPE_SCRIPT_EVENT:
        case CRITERIA_TYPE_SCRIPT_EVENT_2:
        case CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
        case CRITERIA_TYPE_OBTAIN_BATTLEPET:
        case CRITERIA_TYPE_COLLECT_BATTLEPET:
        case CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE:
        case CRITERIA_TYPE_BATTLEPET_WIN:
        case CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT:
        case CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
        case CRITERIA_TYPE_COMPLETE_SCENARIO:
            progress->completed = progress->Counter >= requiredAmount;
            break;
        // handle all statistic-only criteria here
        case CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case CRITERIA_TYPE_DEATH_AT_MAP:
        case CRITERIA_TYPE_DEATH:
        case CRITERIA_TYPE_DEATH_IN_DUNGEON:
        case CRITERIA_TYPE_KILLED_BY_CREATURE:
        case CRITERIA_TYPE_KILLED_BY_PLAYER:
        case CRITERIA_TYPE_DEATHS_FROM:
        case CRITERIA_TYPE_HIGHEST_TEAM_RATING:
        case CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case CRITERIA_TYPE_LOSE_DUEL:
        case CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case CRITERIA_TYPE_CREATE_AUCTION:
        case CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case CRITERIA_TYPE_WON_AUCTIONS:
        case CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case CRITERIA_TYPE_KNOWN_FACTIONS:
        case CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case CRITERIA_TYPE_ROLL_NEED:
        case CRITERIA_TYPE_ROLL_GREED:
        case CRITERIA_TYPE_QUEST_ABANDONED:
        case CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
        default:
            break;
    }

    return progress->completed;
}

template<class T>
bool AchievementMgr<T>::IsCompletedCriteriaTree(CriteriaTree const* tree)
{
    // if (!CanCompleteCriteriaTree(tree))
        // return false;

    uint64 requiredCount = tree->Entry->Amount;
    switch (tree->Entry->Operator)
    {
        case CRITERIA_TREE_OPERATOR_SINGLE:
            return tree->Criteria && IsCompletedCriteria(tree, requiredCount);
        case CRITERIA_TREE_OPERATOR_SINGLE_NOT_COMPLETED:
            return !tree->Criteria || !IsCompletedCriteria(tree, requiredCount);
        case CRITERIA_TREE_OPERATOR_ALL:
            for (CriteriaTree const* node : tree->Children)
                if (!IsCompletedCriteriaTree(node))
                    return false;
            return true;
        case CRITERIA_TREE_OPERAROR_SUM_CHILDREN:
        {
            uint64 progress = 0;
            AchievementGlobalMgr::WalkCriteriaTree(tree, [this, &progress](CriteriaTree const* criteriaTree)
            {
                if (criteriaTree->Criteria)
                    if (CriteriaProgress const* criteriaProgress = GetCriteriaProgress(criteriaTree->ID))
                        progress += criteriaProgress->Counter;
            });
            return progress >= requiredCount;
        }
        case CRITERIA_TREE_OPERATOR_SCENARIO:
        {
            uint64 progress = 0;
            AchievementGlobalMgr::WalkCriteriaTree(tree, [this, &progress](CriteriaTree const* criteriaTree)
            {
                if (criteriaTree->Criteria)
                    if (CriteriaProgress const* criteriaProgress = GetCriteriaProgress(criteriaTree->ID))
                        progress += criteriaProgress->Counter * criteriaTree->Entry->Amount;
            });
            return progress >= requiredCount;
        }
        case CRITERIA_TREE_OPERATOR_MAX_CHILD:
        {
            uint64 progress = 0;
            AchievementGlobalMgr::WalkCriteriaTree(tree, [this, &progress](CriteriaTree const* criteriaTree)
            {
                if (criteriaTree->Criteria)
                    if (CriteriaProgress const* criteriaProgress = GetCriteriaProgress(criteriaTree->ID))
                        if (criteriaProgress->Counter > progress)
                            progress = criteriaProgress->Counter;
            });
            return progress >= requiredCount;
        }
        case CRITERIA_TREE_OPERATOR_COUNT_DIRECT_CHILDREN:
        {
            uint64 progress = 0;
            for (CriteriaTree const* node : tree->Children)
                if (node->Criteria)
                    if (CriteriaProgress const* criteriaProgress = GetCriteriaProgress(node->ID))
                        if (criteriaProgress->Counter >= 1)
                            if (++progress >= requiredCount)
                                return true;

            return false;
        }
        case CRITERIA_TREE_OPERATOR_ANY:
        {
            uint64 progress = 0;
            for (CriteriaTree const* node : tree->Children)
                if (IsCompletedCriteriaTree(node))
                    if (++progress >= requiredCount)
                        return true;

            return false;
        }
        default:
            break;
    }

    return false;
}

template<class T>
bool AchievementMgr<T>::IsCompletedScenarioTree(CriteriaTreeEntry const* criteriaTree)
{
    CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(criteriaTree->ID);
    if (!tree)
        return false;

    return IsCompletedCriteriaTree(tree);
}

template<class T>
bool AchievementMgr<T>::IsCompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    // Counter can never complete
    if ((achievement->Flags & ACHIEVEMENT_FLAG_COUNTER) || HasAchieved(achievement->ID, referencePlayer->GetGUIDLow()))
        return false;

    CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(achievement->CriteriaTree);
    if (!tree)
        return false;

    if (!CanCompleteCriteria(achievement))
        return false;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT contained in the achievement, but in each individual criteria
    if (achievement->Flags & ACHIEVEMENT_FLAG_SUMM)
    {
        uint64 progress = 0;
        AchievementGlobalMgr::WalkCriteriaTree(tree, [this, &progress](CriteriaTree const* criteriaTree)
        {
            if (CriteriaProgress const* progress = this->GetCriteriaProgress(criteriaTree->ID))
                progress += progress->Counter;
        });
        return progress >= tree->Entry->Amount;
    }

    return IsCompletedCriteriaTree(tree);
}

template<class T>
CriteriaProgress* AchievementMgr<T>::GetCriteriaProgress(uint32 entry, bool create)
{
    CriteriaProgress* progress = _criteriaProgressArr[entry];
    if(progress)
        return progress;

    if (create)
    {
        WriteGuardType guard(i_criteriaProgressLock);
        progress = &_criteriaProgress[entry];
        _criteriaProgressArr[entry] = progress;
    }

    return progress;
}

template<class T>
bool AchievementMgr<T>::SetCriteriaProgress(CriteriaTree const* tree, uint64 changeValue, Player* referencePlayer, ProgressType ptype)
{
    AchievementEntry const* achievement = tree->Achievement;

    // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "SetCriteriaProgress tree %u CriteriaID %u achievement %u", tree->ID, tree->CriteriaID, achievement ? achievement->ID : 0);

    if (!CanUpdate())
        return false;

    Criteria const* criteria = tree->Criteria;
    if (!criteria)
        return false;

    auto timedIter = _timeCriteriaTreesArr[tree->ID];
    // Don't allow to cheat - doing timed achievements without timer active
    if (criteria->Entry->StartTimer && timedIter == NULL)
        return false;

    CriteriaProgress* progress = NULL;
    {
        // ReadGuardType guard(i_criteriaProgressLock);
        progress = _criteriaProgressArr[tree->ID];
    }
    if (!progress)
    {
        WriteGuardType guard(i_criteriaProgressLock);
        progress = &_criteriaProgress[tree->ID];
        _criteriaProgressArr[tree->ID] = progress;
        // not create record for 0 counter but allow it for timed achievements
        // we will need to send 0 progress to client to start the timer
        if (changeValue == 0 && !criteria->Entry->StartTimer)
            return false;

        #ifdef _MSC_VER
        sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "SetCriteriaProgress(%u, %u%u) new CriteriaSort %u achievement %u treeEntry %u", tree->ID, changeValue, GetCriteriaSort(), achievement ? achievement->ID : 0, criteria->ID);
        #endif

        progress->Counter = changeValue;
        progress->changed = true;
        progress->deactiveted = false;
        progress->completed = false;
        progress->achievement = achievement;
        progress->criteriaTree = tree->Entry;
        progress->_criteria = criteria;
        progress->criteria = criteria->Entry;
        progress->parent = sCriteriaTreeStore.LookupEntry(tree->Entry->Parent);
    }
    else
    {
        #ifdef _MSC_VER
        sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "SetCriteriaProgress(%u, %u%u) old CriteriaSort %u achievement %u", criteria->ID, changeValue, GetCriteriaSort(), achievement ? achievement->ID : 0);
        #endif

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
                newValue = max_value - progress->Counter > changeValue ? progress->Counter + changeValue : max_value;
                break;
            }
            case PROGRESS_HIGHEST:
                newValue = progress->Counter < changeValue ? changeValue : progress->Counter;
                break;
        }

        // not update (not mark as changed) if Counter will have same value
        if (progress->Counter == newValue && !criteria->Entry->StartTimer)
            return false;

        progress->Counter = newValue;
        progress->updated = true;
        progress->deactiveted = false;
    }

    progress->date = time(NULL); // set the date to the latest update.

    //if (!achievement)
        //return;

    uint32 timeElapsed = 0;
    bool timedCompleted = IsCompletedCriteriaTree(tree) || progress->completed;
    if (criteria->Entry->StartTimer)
    {
        // Client expects this in packet
        timeElapsed = criteria->Entry->StartTimer - ((*timedIter)/IN_MILLISECONDS);

        // Remove the timer, we wont need it anymore
        if (progress->completed)
        {
            _timeCriteriaTrees.erase(tree->ID);
            _timeCriteriaTreesArr[tree->ID] = NULL;
        }
    }

    #ifdef _MSC_VER
    sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "SetCriteriaProgress criteria %u achievement %u treeEntry %u completed %i timeElapsed %i StartTimer %i timedCompleted %u",
    criteria->ID, achievement ? achievement->ID : 0, criteria->ID, progress->completed, timeElapsed, criteria->Entry->StartTimer, timedCompleted);
    #endif

    if (progress->completed && achievement && achievement->Flags & ACHIEVEMENT_FLAG_SHOW_CRITERIA_MEMBERS && !progress->PlayerGUID)
        progress->PlayerGUID = referencePlayer->GetGUID();

    if (achievement && achievement->ParentAchievement && !HasAchieved(achievement->ParentAchievement, referencePlayer->GetGUIDLow())) //Don`t send update criteria to client if parent achievment not complete
        return false;

    if (achievement && achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT)
        SendAccountCriteriaUpdate(progress, timeElapsed, progress->completed);
    else
        SendCriteriaUpdate(progress, timeElapsed, timedCompleted);

    return timedCompleted;
}

template<class T>
void AchievementMgr<T>::UpdateTimedAchievements(uint32 timeDiff)
{
    if (!_timeCriteriaTrees.empty())
    {
        for (auto itr = _timeCriteriaTrees.begin(); itr != _timeCriteriaTrees.end();)
        {
            // Time is up, remove timer and reset progress
            if (itr->second <= timeDiff)
            {
                CriteriaTree const* criteriaTree = sAchievementMgr->GetCriteriaTree(itr->first);
                if (criteriaTree->Criteria)
                    RemoveCriteriaProgress(criteriaTree);

                _timeCriteriaTreesArr[itr->first] = NULL;
                itr = _timeCriteriaTrees.erase(itr);
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
void AchievementMgr<T>::StartTimedAchievement(CriteriaTimedTypes /*type*/, uint32 /*entry*/, uint16 /*timeLost = 0*/)
{
}

template<>
void AchievementMgr<Player>::StartTimedAchievement(CriteriaTimedTypes type, uint32 entry, uint16 timeLost /*= 0*/)
{
    CriteriaTreeList const& criteriaList = sAchievementMgr->GetTimedCriteriaByType(type);
    for (CriteriaTree const* tree : criteriaList)
    {
        if (tree->Criteria->Entry->StartAsset != entry)
            continue;

        bool canStart = false;
        if (_timeCriteriaTreesArr[tree->ID] == NULL && !IsCompletedCriteriaTree(tree))
        {
            // Start the timer
            if (tree->Criteria->Entry->StartTimer * uint32(IN_MILLISECONDS) > timeLost)
            {
                _timeCriteriaTrees[tree->ID] = tree->Criteria->Entry->StartTimer * IN_MILLISECONDS - timeLost;
                _timeCriteriaTreesArr[tree->ID] = &_timeCriteriaTrees[tree->ID];
                canStart = true;
            }
        }

        if (!canStart)
            continue;

        // and at client too
        SetCriteriaProgress(tree, 0, nullptr, PROGRESS_SET);
    }
}

template<class T>
void AchievementMgr<T>::RemoveTimedAchievement(CriteriaTimedTypes type, uint32 entry)
{
    CriteriaTreeList const& criteriaList = sAchievementMgr->GetTimedCriteriaByType(type);
    for (CriteriaTree const* tree : criteriaList)
    {
        if (tree->Criteria->Entry->StartAsset != entry)
            continue;

        _timeCriteriaTrees.erase(tree->ID);
        _timeCriteriaTreesArr[tree->ID] = NULL;

        // remove progress
        RemoveCriteriaProgress(tree);
    }
}

template<class T>
void AchievementMgr<T>::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    // disable for gamemasters with GM-mode enabled
    if (GetOwner()->isGameMaster())
        return;

    CompletedAchievementData* ca = NULL;
    {
        WriteGuardType guard(i_completedAchievementsLock);

        if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
            if (Guild* guild = sGuildMgr->GetGuildById(referencePlayer->GetGuildId()))
                guild->GetNewsLog().AddNewEvent(GUILD_NEWS_PLAYER_ACHIEVEMENT, time(NULL), referencePlayer->GetGUID(), achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

        if (!GetOwner()->GetSession()->PlayerLoading())
            SendAchievementEarned(achievement);

        ca = &_completedAchievements[achievement->ID];
        ca->date = time(NULL);
        ca->first_guid = GetOwner()->GetGUIDLow();
        ca->changed = true;
        _completedAchievementsArr[achievement->ID] = ca;
    }

    // don't insert for ACHIEVEMENT_FLAG_REALM_FIRST_KILL since otherwise only the first group member would reach that achievement
    // TODO: where do set this instead?
    if (!(achievement->Flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    _achievementPoints += achievement->RewardPoints;

    UpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, 0, NULL, referencePlayer);
    UpdateAchievementCriteria(CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->RewardPoints, 0, 0, NULL, referencePlayer);

    sAchievementMgr->AchievementScriptLoaders(achievement, GetOwner());

    if (sWorld->getBoolConfig(CONFIG_CUSTOM_X20))
    {
       if (GetOwner()->getLevel() > 1 && GetOwner()->getLevel() != 55) // on 1 level accounts achievment
       {
          QueryResult result = CharacterDatabase.PQuery("SELECT * FROM account_achievement WHERE account = '%u' and achievement = '%u' ", GetOwner()->GetSession()->GetAccountId(), achievement->ID);
          QueryResult result_1 = CharacterDatabase.PQuery("SELECT * FROM custom_account_checker WHERE account = '%u' and type = '4' and count = '%u';", GetOwner()->GetSession()->GetAccountId(), achievement->ID); 
          if (!result) //if achiev was
           if (!result_1) //if achiew was complete now
             {
                uint32 id = 37711;
                ChatHandler chH = ChatHandler(GetOwner()); 
                uint32 count = achievement->RewardPoints;       

                //Adding items
                uint32 noSpaceForCount = 0;

                // check space and find places
                ItemPosCountVec dest;
                InventoryResult msg = GetOwner()->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count, &noSpaceForCount);
                if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
                    count -= noSpaceForCount;

                if (count == 0 || dest.empty())                         // can't add any
                {
                    chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                    return;
                }

                Item* item = GetOwner()->StoreNewItem(dest, id, true, Item::GenerateItemRandomPropertyId(id));

                if (count > 0 && item)
                    GetOwner()->SendNewItem(item, count, false, true);

                if (noSpaceForCount > 0)
                {
                    chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                    CharacterDatabase.PExecute("INSERT INTO `custom_account_checker` (`type`, `count`, `account`) VALUES ('2', '%u', '%u');", noSpaceForCount, GetOwner()->GetSession()->GetAccountId()); 
                } 
                CharacterDatabase.PExecute("INSERT INTO `custom_account_checker` (`type`, `count`, `account`) VALUES ('4', '%u', '%u');", achievement->ID, GetOwner()->GetSession()->GetAccountId()); 
             }
        }
    }

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

        LocaleConstant localeConstant = GetOwner()->GetSession()->GetSessionDbLocaleIndex();

        // subject and text
        std::string subject = reward->subject;
        std::string text = reward->text;
        if (localeConstant >= 0)
        {
            if (AchievementRewardLocale const* loc = sAchievementMgr->GetAchievementRewardLocale(achievement))
            {
                ObjectMgr::GetLocaleString(loc->subject, localeConstant, subject);
                ObjectMgr::GetLocaleString(loc->text, localeConstant, text);
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

        draft.SendMailTo(trans, GetOwner(), MailSender(MAIL_CREATURE, uint64(reward->sender)));
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
    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    CompletedAchievementData* ca = NULL;
    {
        WriteGuardType guard(i_completedAchievementsLock);

        if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
            if (Guild* guild = sGuildMgr->GetGuildById(referencePlayer->GetGuildId()))
                guild->GetNewsLog().AddNewEvent(GUILD_NEWS_GUILD_ACHIEVEMENT, time(NULL), 0, achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

        SendAchievementEarned(achievement);

        ca = &_completedAchievements[achievement->ID];
        ca->date = time(NULL);
        ca->changed = true;
        _completedAchievementsArr[achievement->ID] = ca;
    }

    sAchievementMgr->SetRealmCompleted(achievement);

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_GUILD_MEMBERS)
    {
        if (referencePlayer->GetGuildId() == GetOwner()->GetId())
            ca->guids.insert(referencePlayer->GetGUID());

        if (Group const* group = referencePlayer->GetGroup())
            for (GroupReference const* ref = group->GetFirstMember(); ref != NULL; ref = ref->next())
                if (Player const* groupMember = ref->getSource())
                    if (groupMember->GetGuildId() == GetOwner()->GetId())
                        ca->guids.insert(groupMember->GetGUID());
    }

    _achievementPoints += achievement->RewardPoints;

    UpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, 0, NULL, referencePlayer);
    UpdateAchievementCriteria(CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->RewardPoints, 0, 0, NULL, referencePlayer);
    UpdateAchievementCriteria(CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS, achievement->RewardPoints, 0, 0, NULL, referencePlayer);
}

struct VisibleAchievementPred
{
    bool operator()(CompletedAchievementMap::value_type const& val) const
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(val.first);
        return achievement && !(achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN);
    }
};

template<class T>
void AchievementMgr<T>::SendAllAchievementData(Player* /*receiver*/)
{
    if (_criteriaProgress.empty())
        return;

    VisibleAchievementPred isVisible;
    size_t numAchievements = std::count_if(_completedAchievements.begin(), _completedAchievements.end(), isVisible);
    uint32 criteriaCount = 0;
    ObjectGuid guid = GetOwner()->GetGUID();

    WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA);

    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 19);
    data.WriteBits(numAchievements, 20);

    for (auto itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        ObjectGuid Counter = uint64(itr->second.Counter);

        data.WriteGuidMask<5>(Counter);
        data.WriteBits(0, 4);            // Flags
        data.WriteGuidMask<6>(guid);
        data.WriteGuidMask<2>(Counter);
        data.WriteGuidMask<3, 2>(guid);
        data.WriteGuidMask<4>(Counter);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<3, 7, 0, 1>(Counter);
        data.WriteGuidMask<5, 0, 7>(guid);
        data.WriteGuidMask<6>(Counter);
        data.WriteGuidMask<4>(guid);
    }

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        if (!isVisible(*itr))
            continue;

        data.WriteGuidMask<3, 0, 6, 5, 2, 7, 4, 1>(itr->second.first_guid);
    }

    data.FlushBits();

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
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

    for (auto itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;
        if (!criteriaTree)
            continue;

        ObjectGuid Counter = uint64(itr->second.Counter);

        data.WriteGuidBytes<5, 7>(Counter);
        data.WriteGuidBytes<3, 4>(guid);
        data.WriteGuidBytes<1>(Counter);
        data << uint32(secsToTimeBitFields(itr->second.date));       // Date
        data.WriteGuidBytes<0>(guid);
        data.WriteGuidBytes<2>(Counter);
        data.WriteGuidBytes<6>(guid);
        data.WriteGuidBytes<6>(Counter);
        data << uint32(criteriaTree->CriteriaID);         // CriteriaID
        data.WriteGuidBytes<4>(Counter);
        data.WriteGuidBytes<2>(guid);
        data.WriteGuidBytes<3>(Counter);
        data.WriteGuidBytes<7>(guid);
        data.WriteGuidBytes<0>(Counter);
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
    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DATA, _completedAchievements.size() * (4 + 4) + 3);
    data.WriteBits(_completedAchievements.size(), 20);
    
    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
        data.WriteBits(0, 8);

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        data << uint32(secsToTimeBitFields(itr->second.date));
        data << uint32(realmID);                                     // VirtualRealmAddress / NativeRealmAddress
        data << uint32(itr->first);
        data << uint32(realmID);     
    }

    receiver->GetSession()->SendPacket(&data);
}

template<class T>
void AchievementMgr<T>::SendAllAccountCriteriaData(Player* /*receiver*/)
{
    if (_criteriaProgress.empty())
        return;

    ObjectGuid guid = GetOwner()->GetGUID();
    uint32 criteriaCount = 0;
    ByteBuffer criteriaData;

    WorldPacket data(SMSG_ALL_ACCOUNT_CRITERIA_DATA);

    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 19);

    for (CriteriaProgressMap::const_iterator itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;
        if (!criteriaTree)
            continue;

        AchievementEntry const* achievement = itr->second.achievement;

        if (!achievement || !(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
            continue;

        ObjectGuid Counter = uint64(itr->second.Counter);

        data.WriteBits(0, 4);            // Flags
        data.WriteGuidMask<4>(Counter);
        data.WriteGuidMask<3>(guid);
        data.WriteGuidMask<6, 0, 3>(Counter);
        data.WriteGuidMask<2, 5>(guid);
        data.WriteGuidMask<5>(Counter);
        data.WriteGuidMask<0, 1>(guid);
        data.WriteGuidMask<2>(Counter);
        data.WriteGuidMask<4>(guid);
        data.WriteGuidMask<1>(Counter);
        data.WriteGuidMask<6, 7>(guid);
        data.WriteGuidMask<7>(Counter);

        criteriaData.WriteGuidBytes<0>(guid);
        criteriaData.WriteGuidBytes<7>(Counter);
        criteriaData << uint32(0);
        criteriaData << uint32(0);
        criteriaData.WriteGuidBytes<2>(guid);
        criteriaData.WriteGuidBytes<2>(Counter);
        criteriaData << uint32(secsToTimeBitFields(itr->second.date));
        criteriaData.WriteGuidBytes<4>(Counter);
        criteriaData.WriteGuidBytes<6>(guid);
        criteriaData.WriteGuidBytes<0>(Counter);
        criteriaData.WriteGuidBytes<4, 1>(guid);
        criteriaData.WriteGuidBytes<1>(Counter);
        criteriaData.WriteGuidBytes<3, 5>(guid);
        criteriaData.WriteGuidBytes<6, 5, 3>(Counter);
        criteriaData.WriteGuidBytes<7>(guid);
        criteriaData << uint32(criteriaTree->CriteriaID);

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
{ }

template<class T>
void AchievementMgr<T>::SendAchievementInfo(Player* receiver, uint32 achievementId /*= 0*/)
{ }

template<>
void AchievementMgr<Player>::SendAchievementInfo(Player* receiver, uint32 /*achievementId = 0 */)
{
    if (_criteriaProgress.empty())
        return;

    ObjectGuid guid = GetOwner()->GetGUID();
    ObjectGuid Counter;

    size_t numCriteria = _criteriaProgress.size();

    VisibleAchievementPred isVisible;
    size_t numAchievements = std::count_if(_completedAchievements.begin(), _completedAchievements.end(), isVisible);

    WorldPacket data(SMSG_RESPOND_INSPECT_ACHIEVEMENTS);

    data.WriteGuidMask<4, 7, 6>(guid);
    data.WriteBits(numCriteria, 19);
    data.WriteGuidMask<2, 3>(guid);

    for (CriteriaProgressMap::const_iterator iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
    {
        Counter = iter->second.Counter;

        data.WriteGuidMask<5>(guid);
        data.WriteGuidMask<0>(Counter);
        data.WriteGuidMask<2>(guid);
        data.WriteGuidMask<7, 2, 6>(Counter);
        data.WriteGuidMask<4>(guid);
        data.WriteBits(0, 4);                    // Flags
        data.WriteGuidMask<7, 3, 6>(guid);
        data.WriteGuidMask<5>(Counter);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<1, 3, 4>(Counter);
        data.WriteGuidMask<0>(guid);
    }

    data.WriteGuidMask<0>(guid);
    data.WriteBits(numAchievements, 20);

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        if (!isVisible(*itr))
            continue;

        ObjectGuid firstGuid = (*itr).second.first_guid;
        data.WriteGuidMask<2, 3, 4, 1, 0, 5, 7, 6>(firstGuid);
    }

    data.WriteGuidMask<5, 1>(guid);

    data.FlushBits();

    for (CriteriaProgressMap::const_iterator iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
    {
        CriteriaTreeEntry const* criteriaTree = iter->second.criteriaTree;
        if (!criteriaTree)
            continue;
        Counter = iter->second.Counter;

        data.WriteGuidBytes<1, 3, 0>(guid);
        data.WriteGuidBytes<5, 7>(Counter);
        data << uint32(criteriaTree->CriteriaID);     // ID
        data.WriteGuidBytes<2>(Counter);
        data << uint32(0);                          // TimeFromStart / TimeFromCreate
        data.WriteGuidBytes<4>(guid);
        data.WriteGuidBytes<4, 0>(Counter);
        data << uint32(secsToTimeBitFields(iter->second.date));  // Date
        data.WriteGuidBytes<6, 3>(Counter);
        data << uint32(0);                          // TimeFromStart / TimeFromCreate
        data.WriteGuidBytes<2, 5>(guid);
        data.WriteGuidBytes<1>(Counter);
        data.WriteGuidBytes<7, 6>(guid);
    }

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
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
    AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId);
    if (!achievement)
    {
        WorldPacket data(SMSG_GUILD_CRITERIA_UPDATE, 3);
        data.WriteBits(0, 19);
        receiver->GetSession()->SendPacket(&data);
        return;
    }

    uint32 numCriteria = 0;

    for (CriteriaProgressMap::const_iterator itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        if (!itr->second.achievement || itr->second.achievement->ID != achievementId)
            continue;

        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;
        if (!criteriaTree)
            continue;

        numCriteria++;
    }

    ByteBuffer criteriaData(numCriteria * (8 + 8 + 4 + 4 + 4));

    WorldPacket data(SMSG_GUILD_CRITERIA_UPDATE, 3 + numCriteria);
    data.WriteBits(numCriteria, 19);

    for (CriteriaProgressMap::const_iterator itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        if (!itr->second.achievement || itr->second.achievement->ID != achievementId)
            continue;

        CriteriaTreeEntry const* criteriaTree = itr->second.criteriaTree;
        if (!criteriaTree)
            continue;

        ObjectGuid Counter = itr->second.Counter;
        ObjectGuid guid = itr->second.PlayerGUID;

        data.WriteGuidMask<4, 2, 6>(guid);
        data.WriteGuidMask<1, 5>(Counter);
        data.WriteGuidMask<3>(guid);
        data.WriteGuidMask<2>(Counter);
        data.WriteGuidMask<0, 5>(guid);
        data.WriteGuidMask<3>(Counter);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<7>(Counter);
        data.WriteGuidMask<7>(guid);
        data.WriteGuidMask<0, 6, 4>(Counter);

        criteriaData.WriteGuidBytes<0>(Counter);
        criteriaData << uint32(secsToTimeBitFields(itr->second.date)); // DateUpdated
        criteriaData.WriteGuidBytes<2>(guid);
        criteriaData.WriteGuidBytes<1>(Counter);
        criteriaData << uint32(0);                      // Flags
        criteriaData.WriteGuidBytes<7, 6>(Counter);
        criteriaData.WriteGuidBytes<0>(guid);
        criteriaData << uint32(itr->second.date);         // DateStarted / DateCreated
        criteriaData.WriteGuidBytes<6, 7>(guid);
        criteriaData.WriteGuidBytes<4>(Counter);
        criteriaData.WriteGuidBytes<5>(guid);
        criteriaData << uint32(criteriaTree->CriteriaID);
        criteriaData.WriteGuidBytes<4, 1>(guid);
        criteriaData << uint32(::time(NULL) - itr->second.date); // DateStarted / DateCreated
        criteriaData.WriteGuidBytes<5, 2>(Counter);
        criteriaData.WriteGuidBytes<3>(guid);
        criteriaData.WriteGuidBytes<3>(Counter);
    }

    data.FlushBits();

    if (!criteriaData.empty())
        data.append(criteriaData);

    data.PutBits(0, numCriteria, 19);

    receiver->GetSession()->SendPacket(&data);
}

template<class T>
bool AchievementMgr<T>::HasAchieved(uint32 achievementId, uint64 guid /*= 0*/) const
{
    CompletedAchievementData* ca = _completedAchievementsArr[achievementId];
    if (!ca)
        return false;

    if (ca->isAccountAchievement)
        return true;

    return GetCriteriaSort() == PLAYER_CRITERIA ? ca->first_guid == guid : true;
}

template<class T>
bool AchievementMgr<T>::HasAccountAchieved(uint32 achievementId) const
{
    return _completedAchievementsArr[achievementId] != NULL;
}

template<class T>
uint64 AchievementMgr<T>::GetFirstAchievedCharacterOnAccount(uint32 achievementId) const
{
    CompletedAchievementData* ca = _completedAchievementsArr[achievementId];
    if (!ca)
        return 0LL;

    return ca->first_guid;
}

template<class T>
bool AchievementMgr<T>::CanUpdateCriteria(CriteriaTree const* tree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer)
{
    CriteriaEntry const* criteria = tree->Criteria->Entry;

    if (!criteria)
    {
        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria !criteria tree %u", tree->ID);
        return false;
    }

    if (!CanUpdateCriteriaTree(tree, referencePlayer))
        return false;

    if (!RequirementsSatisfied(tree, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
    {
        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: (Id: %u Type %s) Requirements not satisfied",
            // tree->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->Type));
        return false;
    }

    if (tree->Criteria->Modifier && !AdditionalRequirementsSatisfied(tree->Criteria->Modifier, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
    {
        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: (Id: %u Type %s) Additional requirements not satisfied",
            // tree->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->Type));
        return false;
    }

    if (!ConditionsSatisfied(tree->Criteria, referencePlayer))
    {
        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: (Id: %u Type %s) Conditions not satisfied",
            // tree->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->Type));
        return false;
    }

    if (!GetOwner()->CanUpdateCriteria(tree->ID))
    {
        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "CanUpdateCriteria: (Id: %u Type %s) Scenario condition can not be updated at current scenario stage",
            // tree->ID, AchievementGlobalMgr::GetCriteriaTypeString(criteria->Type));
        return false;
    }

    return true;
}

template<class T>
bool AchievementMgr<T>::ConditionsSatisfied(Criteria const* criteria, Player* referencePlayer) const
{
    if (criteria->Entry->StartEvent)
    {
        switch (criteria->Entry->StartEvent)
        {
            case CRITERIA_CONDITION_BG_MAP:
                if (referencePlayer->GetMapId() != criteria->Entry->StartAsset)
                    return false;
                break;
            case CRITERIA_CONDITION_NOT_IN_GROUP:
                if (referencePlayer->GetGroup())
                    return false;
                break;
            default:
                break;
        }
    }

    if (criteria->Entry->FailEvent)
    {
        switch (criteria->Entry->FailEvent)
        {
            case CRITERIA_CONDITION_BG_MAP:
                if (referencePlayer->GetMapId() != criteria->Entry->FailAsset && criteria->Entry->FailAsset != 0)
                    return false;
                break;
            case CRITERIA_CONDITION_NOT_IN_GROUP:
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
bool AchievementMgr<T>::RequirementsSatisfied(CriteriaTree const* tree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const *unit, Player* referencePlayer) const
{
    CriteriaEntry const* criteria = tree->Criteria->Entry;
    AchievementEntry const* achievement = tree->Achievement;

    switch (CriteriaTypes(criteria->Type))
    {
        case CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
        case CRITERIA_TYPE_CREATE_AUCTION:
        case CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case CRITERIA_TYPE_GET_KILLING_BLOWS:
        case CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
        case CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
        case CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
        case CRITERIA_TYPE_HIGHEST_HIT_DEALT:
        case CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
        case CRITERIA_TYPE_HONORABLE_KILL:
        case CRITERIA_TYPE_LOOT_MONEY:
        case CRITERIA_TYPE_LOSE_DUEL:
        case CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
        case CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case CRITERIA_TYPE_QUEST_ABANDONED:
        case CRITERIA_TYPE_ROLL_GREED:
        case CRITERIA_TYPE_ROLL_NEED:
        case CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
        case CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
        case CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
        case CRITERIA_TYPE_VISIT_BARBER_SHOP:
        case CRITERIA_TYPE_WIN_DUEL:
        case CRITERIA_TYPE_WIN_RATED_ARENA:
        case CRITERIA_TYPE_WON_AUCTIONS:
        case CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
        case CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
        case CRITERIA_TYPE_REACH_RBG_RATING:
        case CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
        case CRITERIA_TYPE_CATCH_FROM_POOL:
        case CRITERIA_TYPE_EARNED_PVP_TITLE:
            if (!miscValue1)
                return false;
            break;
        case CRITERIA_TYPE_BUY_BANK_SLOT:
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
        case CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
        case CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
        case CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
        case CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
        case CRITERIA_TYPE_HIGHEST_TEAM_RATING:
        case CRITERIA_TYPE_KNOWN_FACTIONS:
        case CRITERIA_TYPE_REACH_LEVEL:
        case CRITERIA_TYPE_KILL_CREATURE_TYPE:
        case CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
        case CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
        case CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
        case CRITERIA_TYPE_BUY_GUILD_EMBLEM:
        case CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
        case CRITERIA_TYPE_OBTAIN_BATTLEPET:
        case CRITERIA_TYPE_COLLECT_BATTLEPET:
        case CRITERIA_TYPE_BATTLEPET_WIN:
            break;
        case CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                if (_completedAchievementsArr[criteria->Asset] == NULL)
                return false;
            break;
        case CRITERIA_TYPE_WIN_BG:
            if (!miscValue1 || criteria->Asset != referencePlayer->GetMapId())
                return false;
            break;
        case CRITERIA_TYPE_KILL_CREATURE:
        case CRITERIA_TYPE_USE_ITEM:
            if (!miscValue1 || criteria->Asset != miscValue1)
                return false;
            break;
        case CRITERIA_TYPE_REACH_SKILL_LEVEL:
        case CRITERIA_TYPE_LEARN_SKILL_LEVEL:
        case CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
        case CRITERIA_TYPE_GAIN_REPUTATION:
        case CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
        case CRITERIA_TYPE_LEARN_SKILL_LINE:
            if (miscValue1 && miscValue1 != criteria->Asset)
                return false;
            break;
        case CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case CRITERIA_TYPE_DEATH_AT_MAP:
            if (!miscValue1 || referencePlayer->GetMapId() != criteria->Asset)
                return false;
            break;
        case CRITERIA_TYPE_DEATH:
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
        case CRITERIA_TYPE_DEATH_IN_DUNGEON:
            {
                if (!miscValue1/* || !achievement*/)
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
                if (((InstanceMap*)map)->GetMaxPlayers() != criteria->Asset)
                    return false;
                break;
            }
        case CRITERIA_TYPE_KILLED_BY_PLAYER:
            if (!miscValue1)
                return false;

            // if team check required: must kill by opposition faction
            if (miscValue2 == referencePlayer->GetTeam())
                return false;
            break;
        case CRITERIA_TYPE_DEATHS_FROM:
        case CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            if (!miscValue1 || miscValue2 != criteria->Asset)
                return false;
            break;
        case CRITERIA_TYPE_COMPLETE_QUEST:
            {
                // if miscValues != 0, it contains the questID.
                if (miscValue1)
                {
                    if (miscValue1 != criteria->Asset)
                        return false;
                }
                else
                {
                    // login case.
                    if (!referencePlayer->GetQuestRewardStatus(criteria->Asset))
                        return false;
                }

                if (AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(tree))
                    if (!data->Meets(referencePlayer, unit))
                        return false;
                break;
            }
        case CRITERIA_TYPE_KILLED_BY_CREATURE:
        case CRITERIA_TYPE_BE_SPELL_TARGET:
        case CRITERIA_TYPE_BE_SPELL_TARGET2:
        case CRITERIA_TYPE_CAST_SPELL:
        case CRITERIA_TYPE_CAST_SPELL2:
        case CRITERIA_TYPE_LOOT_ITEM:
        case CRITERIA_TYPE_DO_EMOTE:
        case CRITERIA_TYPE_EQUIP_ITEM:
        case CRITERIA_TYPE_USE_GAMEOBJECT:
        case CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
        case CRITERIA_TYPE_HK_CLASS:
        case CRITERIA_TYPE_HK_RACE:
        case CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
        case CRITERIA_TYPE_INSTANSE_MAP_ID:
        case CRITERIA_TYPE_WIN_ARENA:
        case CRITERIA_TYPE_COMPLETE_RAID:
        case CRITERIA_TYPE_PLAY_ARENA:
        case CRITERIA_TYPE_OWN_RANK:
        case CRITERIA_TYPE_SCRIPT_EVENT:
        case CRITERIA_TYPE_SCRIPT_EVENT_2:
        case CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL:
        case CRITERIA_TYPE_BATTLEPET_LEVEL_UP:
        case CRITERIA_TYPE_COMPLETE_SCENARIO:
            if (!miscValue1 || miscValue1 != criteria->Asset)
                return false;
            break;
        case CRITERIA_TYPE_LEARN_SPELL:
            if (miscValue1 && miscValue1 != criteria->Asset)
                return false;

            if (!referencePlayer->HasSpell(criteria->Asset))
                return false;
            break;
        case CRITERIA_TYPE_LOOT_TYPE:
            // miscValue1 = loot_type (note: 0 = LOOT_CORPSE and then it ignored)
            // miscValue2 = count of item loot
            if (!miscValue1 || !miscValue2 || miscValue1 != criteria->Asset)
                return false;
            break;
        case CRITERIA_TYPE_OWN_ITEM:
            if (miscValue1 && criteria->Asset != miscValue1)
                return false;
            break;
        case CRITERIA_TYPE_EXPLORE_AREA:
            {
                WorldMapOverlayEntry const* worldOverlayEntry = sWorldMapOverlayStore.LookupEntry(criteria->Asset);
                if (!worldOverlayEntry)
                    break;

                    // those requirements couldn't be found in the dbc
                if(AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(tree))
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
        case CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            {
                // miscValue1 = itemid miscValue2 = diced value
                if (!miscValue1 || miscValue2 != criteria->Asset)
                    return false;

                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                if (!proto)
                    return false;
                break;
            }
        case CRITERIA_TYPE_DAMAGE_DONE:
        case CRITERIA_TYPE_HEALING_DONE:
            if (!miscValue1)
                return false;

            if (criteria->StartEvent == CRITERIA_CONDITION_BG_MAP)
            {
                if (referencePlayer->GetMapId() != criteria->StartAsset)
                    return false;

                // map specific case (BG in fact) expected player targeted damage/heal
                if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
                    return false;
            }
            break;
        case CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            {
                if (!miscValue1)
                    return false;
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(miscValue1);
                if (!proto || proto->Quality < ITEM_QUALITY_EPIC)
                    return false;
                break;
            }
        case CRITERIA_TYPE_CURRENCY:
            if (!miscValue1 || !miscValue2 || int64(miscValue2) < 0 || miscValue1 != criteria->Asset)
                return false;
            break;
        default:
            break;
    }
    return true;
}

template<class T>
bool AchievementMgr<T>::AdditionalRequirementsSatisfied(ModifierTreeNode const* tree, uint64 miscValue1, uint64 miscValue2, uint64 miscValue3, Unit const* unit, Player* referencePlayer) const
{

    // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied start miscValue1 %u%u, miscValue2 %u%u miscValue3 %u%u ModifierTree %u", miscValue1, miscValue2, miscValue3, tree->Entry->ID);

    if (m_canUpdateAchiev != 1)
        return false;

    int32 saveReqType = -1;
    int32 count = 0;
    bool saveCheck = false;

    for (ModifierTreeNode const* node : tree->Children)
    {
        if (!node->Children.empty())
            if (!AdditionalRequirementsSatisfied(node, miscValue1, miscValue2, miscValue3, unit, referencePlayer))
                return false;

        uint32 reqType = node->Entry->Type;
        if (!reqType)
            return true;

        uint32 reqValue = node->Entry->Asset;
        uint32 reqCount = node->Entry->SecondaryAsset;
        bool check = true;

        switch (CriteriaAdditionalCondition(reqType))
        {
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_DRUNK_VALUE: // 1
            {
                if(referencePlayer->GetDrunkValue() < reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_ITEM_LEVEL: // 3
            {
                // miscValue1 is itemid
                ItemTemplate const * item = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                if (!item || (int32)item->ItemLevel < reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_CREATURE_ENTRY: // 4
                if (!unit || unit->GetEntry() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_PLAYER: // 5
                if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_DEAD: // 6
                if (!unit || unit->isAlive())
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_ENEMY: // 7
                if (!unit)
                    check = false;
                else if (const Player* player = unit->ToPlayer())
                    if (player->GetTeam() == referencePlayer->GetTeam())
                        check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_HAS_AURA: // 8
                if (!referencePlayer->HasAura(reqValue))
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_HAS_AURA: // 10
                if (!unit || !unit->HasAura(reqValue))
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_MUST_BE_MOUNTED: // 11
                if (!unit || !unit->IsMounted())
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_ITEM_QUALITY_MIN: // 14
            case CRITERIA_ADDITIONAL_CONDITION_ITEM_QUALITY_EQUALS: // 15
            {
                // miscValue1 is itemid
                ItemTemplate const * item = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                if (!item || (int32)item->Quality < reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_AREA_OR_ZONE: // 17
            {
                if (referencePlayer->GetZoneId() != reqValue && referencePlayer->GetAreaId() != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_AREA_OR_ZONE: // 18
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
            case CRITERIA_ADDITIONAL_CONDITION_ITEM_ENTRY: // 19
            {
                if (miscValue1 != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_MAP_DIFFICULTY: // 20
                if (sObjectMgr->GetDiffFromSpawn(referencePlayer->GetMap()->GetDifficulty()) != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_CREATURE_YIELDS_XP: // 21
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
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_ARENA_TEAM_SIZE: // 24
            {
                Battleground* bg = referencePlayer->GetBattleground();
                if (!bg || !bg->isArena() || !bg->isRated() || bg->GetJoinType() != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_RACE: // 25
                if (referencePlayer->getRace() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_CLASS: // 26
                if (referencePlayer->getClass() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_RACE: // 27
                if (!unit || unit->GetTypeId() != TYPEID_PLAYER || unit->getRace() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_CLASS: // 28
                if (!unit || unit->GetTypeId() != TYPEID_PLAYER || unit->getClass() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_MAX_GROUP_MEMBERS: // 29
                if (referencePlayer->GetGroup() && (int32)referencePlayer->GetGroup()->GetMembersCount() >= reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_CREATURE_TYPE: // 30
            {
                if (miscValue1 != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_MAP: // 32
                if (referencePlayer->GetMapId() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_BUILD_VERSION: // 33
                if (reqValue >= 50399) // Current version
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_LEVEL_IN_SLOT: // 34
                if (!referencePlayer)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_WITHOUT_GROUP: // 35
                if (Group const* group = referencePlayer->GetGroup())
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_MIN_PERSONAL_RATING: // 37
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
            case CRITERIA_ADDITIONAL_CONDITION_TITLE_BIT_INDEX: // 38
                // miscValue1 is title's bit index
                if (miscValue1 != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_LEVEL: // 39
                if (referencePlayer->getLevel() != reqValue)
                    check = false;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_LEVEL: // 40
                if (!unit || unit->getLevel() != reqValue)
                    check = false;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_ZONE: // 41
                if (!unit || unit->GetZoneId() != reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_SOURCE_NOT_ZONE: // 42
                if (referencePlayer->GetZoneId() == reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_HEALTH_PERCENT_BELOW: // 46
                if (!unit || unit->GetHealthPct() >= reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_MIN_ACHIEVEMENT_POINTS: // 56
                if ((int32)_achievementPoints < reqValue)
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_REQUIRES_LFG_GROUP: // 58
                if (!referencePlayer->GetGroup() || !referencePlayer->GetGroup()->isLFGGroup())
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_REQUIRES_GUILD_GROUP: // 61
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
            case CRITERIA_ADDITIONAL_CONDITION_GUILD_REPUTATION: // 62
            {
                if (referencePlayer->GetReputationMgr().GetReputation(REP_GUILD) < int32(reqValue))
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_RATED_BATTLEGROUND: // 63
            {
                Battleground* bg = referencePlayer->GetBattleground();
                if (!bg || !bg->IsRBG())
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_PROJECT_RARITY: // 65
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
            case CRITERIA_ADDITIONAL_CONDITION_PROJECT_RACE: // 66
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
            case CRITERIA_ADDITIONAL_CONDITION_SPEC_EVENT:  // 67
            {
                switch(reqValue)
                {
                    case 7091:  // Ahieve 3636
                    case 14662: // celebrate 10-th wow
                    case 12671: // celebrate 9-th wow
                    case 9906:  // celebrate 8-th wow
                    case 9463:  // celebrate 7-th wow
                    case 8831:  // celebrate 6-th wow
                    case 7427:  // celebrate 5-th wow
                    case 6577:  // celebrate 4-th wow
                        check = false;
                    default:
                        break;
                }
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_DUNGEON_FIFFICULTY: // 68
            {
                if (!unit || !unit->GetMap() || unit->GetMap()->GetSpawnMode() != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_MIN_LEVEL: // 69
            {
                if (referencePlayer->getLevel() >= reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_MIN_LEVEL: // 70
            {
                if (!unit || unit->getLevel() >= reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_TARGET_MAX_LEVEL: // 71
            {
                if (!unit || unit->getLevel() < reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_ACTIVE_SCENARIO: // 74
            {
                if (!referencePlayer)
                {
                    check = false;
                    break;
                }

                // InstanceMap* inst = referencePlayer->GetMap()->ToInstanceMap();
                // if (uint32 instanceId = inst ? inst->GetInstanceId() : 0)
                    // if (Scenario* progress = sScenarioMgr->GetScenario(instanceId))
                        // if (progress->GetScenarioId() != reqValue)
                            // check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_ACHIEV_POINTS: // 76
            {
                if ((int32)_achievementPoints < reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_FEMALY: // 78
            {
                if (!miscValue3 || miscValue3 != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_HP_LOW_THAT: // 79
            {
                check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_COUNT_OF_GUILD_MEMBER_IN_GROUP:     // 80
            {
                if (!referencePlayer)
                {
                    check = false;
                    break;
                }

                Group* group   = referencePlayer->GetGroup();
                uint32 guildId = referencePlayer->GetGuildId();
                if (!guildId || !group)
                {
                    check = false;
                    break;
                }

                uint32 counter = 0;
                for (GroupReference* groupRef = group->GetFirstMember(); groupRef != NULL; groupRef = groupRef->next())
                {
                    if (Player* player = groupRef->getSource())
                    {
                        if (player->GetGuildId() == guildId)
                            ++counter;
                    }
                }

                if (counter < reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_MASTER_PET_TAMER: // 81
            {
                if (!miscValue2 || miscValue2 != reqValue)
                    check = false;
            }
            case CRITERIA_ADDITIONAL_CONDITION_CHALANGER_RATE: // 83
            {
                if (!miscValue2)
                   check = false;
                else if (reqValue > miscValue2)            // Medal check
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_INCOMPLETE_QUEST: // 84
            {
                if (!referencePlayer)
                {
                    check = false;
                    break;
                }

                if (referencePlayer->GetQuestStatus(reqValue) != QUEST_STATUS_INCOMPLETE)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_COMPLETE_ACHIEVEMENT: // 86
            case CRITERIA_ADDITIONAL_CONDITION_NOT_COMPLETE_ACHIEVEMENT: // 87
            {
                if (_completedAchievementsArr[reqValue] == NULL)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_REPUTATION_UNK: // 88
            {
                if (!referencePlayer)
                    check = false;
                else if (referencePlayer->GetReputationMgr().GetReputation(miscValue1) < int32(reqValue))
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_QUALITY: // 89
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
                        return false;
                }
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_WIN_IN_PVP: // 90
            case CRITERIA_ADDITIONAL_CONDITION_BATTLEPET_SPECIES: // 91
            {
                if (!miscValue1 || miscValue1 != reqValue)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_EXPANSION_LESS: // 92
                if (reqValue >= (int32)sWorld->getIntConfig(CONFIG_EXPANSION))
                    check = false;
                break;
            case CRITERIA_ADDITIONAL_CONDITION_REPUTATION: // 95
            {
                if (referencePlayer->GetReputationMgr().GetReputation(reqValue) < int32(reqCount))
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_ITEM_CLASS_AND_SUBCLASS: // 96
            {
                // miscValue1 is itemid
                ItemTemplate const * item = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                if (!item || item->Class != reqValue || item->SubClass != reqCount)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_REACH_SKILL_LEVEL: // 99
            {
                if (referencePlayer->GetBaseSkillValue(reqValue) < reqCount)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_NOT_HAVE_ACTIVE_SPELL: // 104
            {
                if (!referencePlayer)
                    check = false;
                else if (referencePlayer->HasActiveSpell(reqValue))
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_HAS_ITEM_COUNT: // 105
            case CRITERIA_ADDITIONAL_CONDITION_ITEM_COUNT: // 114
            {
                if (!referencePlayer)
                    check = false;
                else if (referencePlayer->GetItemCount(reqValue) != reqCount)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_REQ_ADDON: // 106
            {
                if (reqValue != 5) // mop 5.0
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_START_QUEST: // 110
            {
                if (!referencePlayer)
                {
                    check = false;
                    break;
                }

                if (referencePlayer->GetQuestStatus(reqValue) == QUEST_STATUS_NONE)
                   check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_CURRENCY_COUNT: // 119
            {
                if ((int32)referencePlayer->GetCurrency(reqValue) != reqCount)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_CURRENCY_ON_SEASON: // 121
            {
                if ((int32)referencePlayer->GetCurrencyOnSeason(reqValue) <= reqCount)
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_DEATH_COUNTER:   // 122
            {
                if (!referencePlayer || !referencePlayer->GetInstanceId() || referencePlayer->isAlive())
                    check = false;
                break;
            }
            case CRITERIA_ADDITIONAL_CONDITION_ARENA_SEASON: // 125
            {
                if (sWorld->getIntConfig(CONFIG_ARENA_SEASON_ID) != reqValue)
                    check = false;
                break;
            }
            default:
                break;
        }

        // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied cheker end Modify %i, reqType %i reqValue %i reqCount %i saveCheck %i saveReqType %i check %i count %u Amount %u",
        // node->Entry->ID, reqType, reqValue, reqCount, saveCheck, saveReqType, check, count, tree->Entry->Amount);

        if(saveReqType == -1)
        {
            if(check && reqType) //don`t save if false
            {
                saveReqType = reqType;
                count++;
            }
            saveCheck = count >= tree->Entry->Amount;
        }
        else if(saveReqType != reqType)
        {
            if(!saveCheck || !check)
                return false;
        }
        else if(saveReqType == reqType)
        {
            if (check)
                count++;
            if(count >= tree->Entry->Amount)
                return true;
        }
    }

    // sLog->outDebug(LOG_FILTER_ACHIEVEMENTSYS, "AchievementMgr::AdditionalRequirementsSatisfied end ModifierTreeId %i, saveCheck %u saveReqType %i count %u Amount %u", tree->Entry->ID, saveCheck, saveReqType, count, tree->Entry->Amount);
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
    return GetCriteriaTypeString(CriteriaTypes(type));
}

char const* AchievementGlobalMgr::GetCriteriaTypeString(CriteriaTypes type)
{
    switch (type)
    {
        case CRITERIA_TYPE_KILL_CREATURE:
            return "KILL_CREATURE";
        case CRITERIA_TYPE_WIN_BG:
            return "TYPE_WIN_BG";
        case CRITERIA_TYPE_COMPLETE_ARCHAEOLOGY_PROJECTS:
            return "COMPLETE_RESEARCH";
        case CRITERIA_TYPE_REACH_LEVEL:
            return "REACH_LEVEL";
        case CRITERIA_TYPE_REACH_SKILL_LEVEL:
            return "REACH_SKILL_LEVEL";
        case CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return "COMPLETE_ACHIEVEMENT";
        case CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            return "COMPLETE_QUEST_COUNT";
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            return "COMPLETE_DAILY_QUEST_DAILY";
        case CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            return "COMPLETE_QUESTS_IN_ZONE";
        case CRITERIA_TYPE_CURRENCY:
            return "CURRENCY";
        case CRITERIA_TYPE_DAMAGE_DONE:
            return "DAMAGE_DONE";
        case CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            return "COMPLETE_DAILY_QUEST";
        case CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
            return "COMPLETE_BATTLEGROUND";
        case CRITERIA_TYPE_DEATH_AT_MAP:
            return "DEATH_AT_MAP";
        case CRITERIA_TYPE_DEATH:
            return "DEATH";
        case CRITERIA_TYPE_DEATH_IN_DUNGEON:
            return "DEATH_IN_DUNGEON";
        case CRITERIA_TYPE_COMPLETE_RAID:
            return "COMPLETE_RAID";
        case CRITERIA_TYPE_KILLED_BY_CREATURE:
            return "KILLED_BY_CREATURE";
        case CRITERIA_TYPE_KILLED_BY_PLAYER:
            return "KILLED_BY_PLAYER";
        case CRITERIA_TYPE_FALL_WITHOUT_DYING:
            return "FALL_WITHOUT_DYING";
        case CRITERIA_TYPE_DEATHS_FROM:
            return "DEATHS_FROM";
        case CRITERIA_TYPE_COMPLETE_QUEST:
            return "COMPLETE_QUEST";
        case CRITERIA_TYPE_BE_SPELL_TARGET:
            return "BE_SPELL_TARGET";
        case CRITERIA_TYPE_CAST_SPELL:
            return "CAST_SPELL";
        case CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            return "BG_OBJECTIVE_CAPTURE";
        case CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            return "HONORABLE_KILL_AT_AREA";
        case CRITERIA_TYPE_WIN_ARENA:
            return "WIN_ARENA";
        case CRITERIA_TYPE_PLAY_ARENA:
            return "PLAY_ARENA";
        case CRITERIA_TYPE_LEARN_SPELL:
            return "LEARN_SPELL";
        case CRITERIA_TYPE_HONORABLE_KILL:
            return "HONORABLE_KILL";
        case CRITERIA_TYPE_OWN_ITEM:
            return "OWN_ITEM";
        case CRITERIA_TYPE_WIN_RATED_ARENA:
            return "WIN_RATED_ARENA";
        case CRITERIA_TYPE_HIGHEST_TEAM_RATING:
            return "HIGHEST_TEAM_RATING";
        case CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            return "HIGHEST_PERSONAL_RATING";
        case CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            return "LEARN_SKILL_LEVEL";
        case CRITERIA_TYPE_USE_ITEM:
            return "USE_ITEM";
        case CRITERIA_TYPE_LOOT_ITEM:
            return "LOOT_ITEM";
        case CRITERIA_TYPE_EXPLORE_AREA:
            return "EXPLORE_AREA";
        case CRITERIA_TYPE_OWN_RANK:
            return "OWN_RANK";
        case CRITERIA_TYPE_BUY_BANK_SLOT:
            return "BUY_BANK_SLOT";
        case CRITERIA_TYPE_GAIN_REPUTATION:
            return "GAIN_REPUTATION";
        case CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            return "GAIN_EXALTED_REPUTATION";
        case CRITERIA_TYPE_VISIT_BARBER_SHOP:
            return "VISIT_BARBER_SHOP";
        case CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            return "EQUIP_EPIC_ITEM";
        case CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            return "ROLL_NEED_ON_LOOT";
        case CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            return "GREED_ON_LOOT";
        case CRITERIA_TYPE_HK_CLASS:
            return "HK_CLASS";
        case CRITERIA_TYPE_HK_RACE:
            return "HK_RACE";
        case CRITERIA_TYPE_DO_EMOTE:
            return "DO_EMOTE";
        case CRITERIA_TYPE_HEALING_DONE:
            return "HEALING_DONE";
        case CRITERIA_TYPE_GET_KILLING_BLOWS:
            return "GET_KILLING_BLOWS";
        case CRITERIA_TYPE_EQUIP_ITEM:
            return "EQUIP_ITEM";
        case CRITERIA_TYPE_MONEY_FROM_VENDORS:
            return "MONEY_FROM_VENDORS";
        case CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            return "GOLD_SPENT_FOR_TALENTS";
        case CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            return "NUMBER_OF_TALENT_RESETS";
        case CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            return "MONEY_FROM_QUEST_REWARD";
        case CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            return "GOLD_SPENT_FOR_TRAVELLING";
        case CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            return "GOLD_SPENT_AT_BARBER";
        case CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            return "GOLD_SPENT_FOR_MAIL";
        case CRITERIA_TYPE_LOOT_MONEY:
            return "LOOT_MONEY";
        case CRITERIA_TYPE_USE_GAMEOBJECT:
            return "USE_GAMEOBJECT";
        case CRITERIA_TYPE_BE_SPELL_TARGET2:
            return "BE_SPELL_TARGET2";
        case CRITERIA_TYPE_SPECIAL_PVP_KILL:
            return "SPECIAL_PVP_KILL";
        case CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            return "FISH_IN_GAMEOBJECT";
        case CRITERIA_TYPE_EARNED_PVP_TITLE:
            return "EARNED_PVP_TITLE";
        case CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            return "LEARN_SKILLLINE_SPELLS";
        case CRITERIA_TYPE_WIN_DUEL:
            return "WIN_DUEL";
        case CRITERIA_TYPE_LOSE_DUEL:
            return "LOSE_DUEL";
        case CRITERIA_TYPE_KILL_CREATURE_TYPE:
            return "KILL_CREATURE_TYPE";
        case CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
            return "GOLD_EARNED_BY_AUCTIONS";
        case CRITERIA_TYPE_CREATE_AUCTION:
            return "CREATE_AUCTION";
        case CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            return "HIGHEST_AUCTION_BID";
        case CRITERIA_TYPE_WON_AUCTIONS:
            return "WON_AUCTIONS";
        case CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
            return "HIGHEST_AUCTION_SOLD";
        case CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
            return "HIGHEST_GOLD_VALUE_OWNED";
        case CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
            return "GAIN_REVERED_REPUTATION";
        case CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
            return "GAIN_HONORED_REPUTATION";
        case CRITERIA_TYPE_KNOWN_FACTIONS:
            return "KNOWN_FACTIONS";
        case CRITERIA_TYPE_LOOT_EPIC_ITEM:
            return "LOOT_EPIC_ITEM";
        case CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            return "RECEIVE_EPIC_ITEM";
        case CRITERIA_TYPE_ROLL_NEED:
            return "ROLL_NEED";
        case CRITERIA_TYPE_ROLL_GREED:
            return "ROLL_GREED";
        case CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            return "HIT_DEALT";
        case CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            return "HIT_RECEIVED";
        case CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            return "TOTAL_DAMAGE_RECEIVED";
        case CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            return "HIGHEST_HEAL_CASTED";
        case CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
            return "TOTAL_HEALING_RECEIVED";
        case CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
            return "HIGHEST_HEALING_RECEIVED";
        case CRITERIA_TYPE_QUEST_ABANDONED:
            return "QUEST_ABANDONED";
        case CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            return "FLIGHT_PATHS_TAKEN";
        case CRITERIA_TYPE_LOOT_TYPE:
            return "LOOT_TYPE";
        case CRITERIA_TYPE_CAST_SPELL2:
            return "CAST_SPELL2";
        case CRITERIA_TYPE_LEARN_SKILL_LINE:
            return "LEARN_SKILL_LINE";
        case CRITERIA_TYPE_EARN_HONORABLE_KILL:
            return "EARN_HONORABLE_KILL";
        case CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
            return "ACCEPTED_SUMMONINGS";
        case CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            return "EARN_ACHIEVEMENT_POINTS";
        case CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            return "USE_LFD_TO_GROUP_WITH_PLAYERS";
        case CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
            return "SPENT_GOLD_GUILD_REPAIRS";
        case CRITERIA_TYPE_REACH_GUILD_LEVEL:
            return "REACH_GUILD_LEVEL";
        case CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
            return "CRAFT_ITEMS_GUILD";
        case CRITERIA_TYPE_CATCH_FROM_POOL:
            return "CATCH_FROM_POOL";
        case CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
            return "BUY_GUILD_BANK_SLOTS";
        case CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
            return "EARN_GUILD_ACHIEVEMENT_POINTS";
        case CRITERIA_TYPE_WIN_RATED_BATTLEGROUND:
            return "WIN_RATED_BATTLEGROUND";
        case CRITERIA_TYPE_REACH_RBG_RATING:
            return "REACH_BG_RATING";
        case CRITERIA_TYPE_BUY_GUILD_EMBLEM:
            return "BUY_GUILD_TABARD";
        case CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
            return "COMPLETE_QUESTS_GUILD";
        case CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
            return "HONORABLE_KILLS_GUILD";
        case CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
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
template<typename T>
T GetEntry(std::unordered_map<uint32, T> const& map, CriteriaTreeEntry const* tree)
{
    CriteriaTreeEntry const* cur = tree;
    auto itr = map.find(tree->ID);
    while (itr == map.end())
    {
        if (!cur->Parent)
            break;

        cur = sCriteriaTreeStore.LookupEntry(cur->Parent);
        if (!cur)
            break;

        itr = map.find(cur->ID);
    }

    if (itr == map.end())
        return nullptr;

    return itr->second;
};

void AchievementGlobalMgr::LoadCriteriaList()
{
    uint32 oldMSTime = getMSTime();

    if (sCriteriaStore.GetNumRows() == 0)
    {
        sLog->outError(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 achievement criteria.");
        return;
    }

    _criteriaModifiers.assign(MAX_MODIFIER_TREE, NULL);

    if (sModifierTreeStore.GetNumRows() != 0)
    {
        // Load modifier tree nodes
        for (uint32 i = 0; i < sModifierTreeStore.GetNumRows(); ++i)
        {
            ModifierTreeEntry const* tree = sModifierTreeStore.LookupEntry(i);
            if (!tree)
                continue;

            ModifierTreeNode* node = new ModifierTreeNode();
            node->Entry = tree;
            _criteriaModifiers[node->Entry->ID] = node;
        }

        // Build tree
        for (auto itr = _criteriaModifiers.begin(); itr != _criteriaModifiers.end(); ++itr)
        {
            if ((*itr) == NULL || !(*itr)->Entry->Parent)
                continue;

            auto parent = _criteriaModifiers[(*itr)->Entry->Parent];
            if (parent != NULL)
                parent->Children.push_back((*itr));
        }
    }

    _criteriaTreeByCriteriaVector.assign(MAX_CRITERIA, NULL);
    _criteriaTrees.assign(MAX_CRITERIA_TREE, NULL);
    _criteria.assign(MAX_CRITERIA, NULL);

    std::unordered_map<uint32 /*criteriaTreeID*/, AchievementEntry const*> achievementCriteriaTreeIds;
    for (uint32 i = 0; i < sAchievementStore.GetNumRows(); ++i)
    {
        if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(i))
            if (achievement->CriteriaTree)
                achievementCriteriaTreeIds[achievement->CriteriaTree] = achievement;
    }

    std::unordered_map<uint32 /*criteriaTreeID*/, ScenarioStepEntry const*> scenarioCriteriaTreeIds;
    for (uint32 i = 0; i < sScenarioStepStore.GetNumRows(); ++i)
    {
        if (ScenarioStepEntry const* scenarioStep = sScenarioStepStore.LookupEntry(i))
            if (scenarioStep->m_criteriaTreeId)
                scenarioCriteriaTreeIds[scenarioStep->m_criteriaTreeId] = scenarioStep;
    }

    uint32 criterias = 0;
    uint32 guildCriterias = 0;
    uint32 scenarioCriterias = 0;

    // Load criteria tree nodes
    for (uint32 i = 0; i < sCriteriaTreeStore.GetNumRows(); ++i)
    {
        CriteriaTreeEntry const* tree = sCriteriaTreeStore.LookupEntry(i);
        if (!tree)
            continue;

        // Find linked achievement
        AchievementEntry const* achievement = GetEntry(achievementCriteriaTreeIds, tree);
        ScenarioStepEntry const* scenarioStep = GetEntry(scenarioCriteriaTreeIds, tree);
        if (!achievement && !scenarioStep)
            continue;

        CriteriaTree* criteriaTree = new CriteriaTree();
        criteriaTree->ID = tree->ID;
        criteriaTree->Achievement = achievement;
        criteriaTree->ScenarioStep = scenarioStep;
        criteriaTree->Entry = tree;

        _criteriaTrees[criteriaTree->Entry->ID] = criteriaTree;

        if (CriteriaEntry const* criteriaEntry = sCriteriaStore.LookupEntry(tree->CriteriaID))
        {
            if (achievement)
            {
                if (achievement->Flags & ACHIEVEMENT_FLAG_GUILD)
                    ++guildCriterias, _guildCriteriasByType[criteriaEntry->Type].push_back(criteriaTree);
                else
                    ++criterias, _criteriasByType[criteriaEntry->Type].push_back(criteriaTree);
            }
            else if (scenarioStep)
                ++scenarioCriterias, _scenarioCriteriasByType[criteriaEntry->Type].push_back(criteriaTree);

            if (criteriaEntry->StartTimer)
                _criteriasByTimedType[criteriaEntry->StartEvent].push_back(criteriaTree);
        }
    }

    // Build tree
    for (auto itr = _criteriaTrees.begin(); itr != _criteriaTrees.end(); ++itr)
    {
        if ((*itr) == NULL || !(*itr)->Entry->Parent)
            continue;

        auto parent = _criteriaTrees[(*itr)->Entry->Parent];
        if (parent != NULL)
        {
            parent->Children.push_back((*itr));
            while (parent != NULL)
            {
                auto cur = parent;
                parent = _criteriaTrees[parent->Entry->Parent];
                if (parent != NULL)
                {
                    if (sCriteriaStore.LookupEntry((*itr)->Entry->CriteriaID))
                    {
                        _criteriaTreeByCriteria[(*itr)->Entry->CriteriaID].push_back(cur);
                        _criteriaTreeByCriteriaVector[(*itr)->Entry->CriteriaID] = &_criteriaTreeByCriteria[(*itr)->Entry->CriteriaID];
                    }
                }
            }
        }
        else if (sCriteriaStore.LookupEntry((*itr)->Entry->CriteriaID))
        {
            _criteriaTreeByCriteria[(*itr)->Entry->CriteriaID].push_back((*itr));
            _criteriaTreeByCriteriaVector[(*itr)->Entry->CriteriaID] = &_criteriaTreeByCriteria[(*itr)->Entry->CriteriaID];
        }
    }

    // Load criteria
    for (uint32 i = 0; i < sCriteriaStore.GetNumRows(); ++i)
    {
        CriteriaEntry const* criteriaEntry = sCriteriaStore.LookupEntry(i);
        if (!criteriaEntry)
            continue;

        Criteria* criteria = new Criteria();
        criteria->ID = criteriaEntry->ID;
        criteria->Entry = criteriaEntry;
        auto mod = _criteriaModifiers[criteriaEntry->ModifyTree];
        if (mod != NULL)
            criteria->Modifier = mod;

        _criteria[criteria->ID] = criteria;
    }

    uint32 criter = 0;
    for (auto& p : _criteriaTrees)
    {
        if (!p)
            continue;

        if (Criteria const* criteria = GetCriteria(p->Entry->CriteriaID))
        {
            p->Criteria = criteria;
            p->CriteriaID = p->Entry->CriteriaID;
            criter++;
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u criteria, %u guild and %u scenario criter %u in %u ms", criterias, guildCriterias, scenarioCriterias, criter, GetMSTimeDiffToNow(oldMSTime));
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
    m_AchievementListByReferencedId.resize(MAX_ACHIEVEMENT);

    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->RefAchievement)
            continue;

        m_AchievementListByReferencedId[achievement->RefAchievement].push_back(achievement);
        ++count;
    }

    // Once Bitten, Twice Shy (10 player) - Icecrown Citadel
    if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(4539))
        const_cast<AchievementEntry*>(achievement)->MapID = 631;    // Correct map requirement (currently has Ulduar)

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u achievement references in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadAchievementCriteriaData()
{
    uint32 oldMSTime = getMSTime();

    m_criteriaDataMap.clear();                              // need for reload case
    m_criteriaDataVector.assign(MAX_CRITERIA_TREE, NULL);

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

        CriteriaEntry const* criteria = sCriteriaStore.LookupEntry(criteria_id);

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
        m_criteriaDataVector[criteria_id] = &dataSet;

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
    m_allCompletedAchievements.assign(MAX_ACHIEVEMENT, false);

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
        const AchievementEntry* achievement = sAchievementStore.LookupEntry(achievementId);
        if (!achievement)
        {
            // Remove non existent achievements from all characters
            sLog->outError(LOG_FILTER_ACHIEVEMENTSYS, "Non-existing achievement %u data removed from table `character_achievement`.", achievementId);

            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEVMENT);

            stmt->setUInt32(0, uint32(achievementId));

            CharacterDatabase.Execute(stmt);

            continue;
        }
        else if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
            m_allCompletedAchievements[achievementId] = true;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %lu completed achievements in %u ms", (unsigned long)m_allCompletedAchievements.size(), GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    m_achievementRewards.clear();                           // need for reload case
    m_achievementRewardVector.assign(MAX_ACHIEVEMENT, NULL);

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
        const AchievementEntry* pAchievement = sAchievementStore.LookupEntry(entry);
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

        if (pAchievement->FactionFlag == ACHIEVEMENT_FACTION_ANY && ((reward.titleId[0] == 0) != (reward.titleId[1] == 0)))
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
        m_achievementRewardVector[entry] = &m_achievementRewards[entry];
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

        if (m_achievementRewardVector[entry] == NULL)
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

uint32 AchievementGlobalMgr::GetParantTreeId(uint32 parent)
{
    if(CriteriaTreeEntry const* pTree = sCriteriaTreeStore.LookupEntry(parent))
    {
        if (pTree->Parent == 0)
            return pTree->ID;
        else
            return GetParantTreeId(pTree->Parent);
    }
    return parent;
}

CriteriaTree const* AchievementGlobalMgr::GetCriteriaTree(uint32 criteriaTreeId) const
{
    return _criteriaTrees[criteriaTreeId];
}

Criteria const* AchievementGlobalMgr::GetCriteria(uint32 criteriaId) const
{
    return _criteria[criteriaId];
}

ModifierTreeNode const* AchievementGlobalMgr::GetModifierTree(uint32 modifierTreeId) const
{
    return _criteriaModifiers[modifierTreeId];
}

void AchievementGlobalMgr::AchievementScriptLoaders(AchievementEntry const* achievement, Player* source)
{
    AchievementScriptsBounds bounds = sObjectMgr->GetAchievementScriptsBounds(achievement->ID);

    for (AchievementScriptsContainer::iterator itr = bounds.first; itr != bounds.second; ++itr)
        sScriptMgr->OnCompletedAchievement(achievement, source, itr->second);
}
