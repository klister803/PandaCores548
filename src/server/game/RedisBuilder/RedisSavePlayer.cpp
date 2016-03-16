/*
 * Copyright (C) 2005-2016 Uwow <http://uwow.biz/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Containers.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "Group.h"
#include "Spell.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "BattlePetMgr.h"
#include "ReputationMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "AuctionHouseMgr.h"
#include "GameEventMgr.h"
#include "SocialMgr.h"
#include "RedisLoadPlayer.h"
#include "AchievementMgr.h"
#include "RedisBuilderMgr.h"
#include "QuestDef.h"

void Player::InitSavePlayer()
{
    if (!RedisDatabase.isConnected())
        return;

    if (!GetSession()->AccountDatas.empty())
        initAcData = false;

    SavePlayer();
    SavePlayerGold();
    SavePlayerBG();
    SavePlayerGroup();
    InitPlayerLootCooldown();
    InitPlayerCurrency();
    InitPlayerBoundInstances();
    InitPlayerSkills();
    InitPlayerTalents();
    InitPlayerSpells();
    InitPlayerGlyphs();
    SavePlayerAuras();
    InitPlayerQuestStatus();
    InitPlayerQuestRewarded();
    InitPlayerQuestDaily();
    InitPlayerQuestWeekly();
    InitPlayerQuestSeasonal();
    SavePlayerArchaeology();
    InitPlayerReputation();
    InitPlayerVoidStorage();
    InitPlayerActions();
    InitPlayerSocial();
    InitPlayerSpellCooldowns();
    InitPlayerKills();
    SavePlayerDeclinedName();
    InitPlayerEquipmentSets();
    SavePlayerCUFProfiles();
    SavePlayerVisuals();
    SavePlayerHomeBind();
    InitAchievement();
    InitCriteria();
    InitPlayerPets();
    InitPlayerMails();
    if (initAcData)
    {
        InitPlayerBattlePets();
        SavePlayerBattlePetSlots();
        InitPlayerAccountData();
        SaveAccountTutorials();
    }
}

void Player::SavePlayer()
{
    m_nextSave = sWorld->getIntConfig(CONFIG_INTERVAL_SAVE) / 10;

    //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayer userKey %s", userKey);

    PlayerData["data"]["realm"] = realmID;
    PlayerData["data"]["guid"] = GetGUIDLow();
    PlayerData["data"]["account"] = GetSession()->GetAccountId();
    PlayerData["data"]["name"] = GetName();
    PlayerData["data"]["race"] = getRace();
    PlayerData["data"]["class"] = getClass();
    PlayerData["data"]["gender"] = getGender();
    PlayerData["data"]["level"] = getLevel();
    PlayerData["data"]["xp"] = GetUInt32Value(PLAYER_XP);
    PlayerData["data"]["playerBytes"] = GetUInt32Value(PLAYER_BYTES);
    PlayerData["data"]["playerBytes2"] = GetUInt32Value(PLAYER_BYTES_2);
    PlayerData["data"]["playerFlags"] = GetUInt32Value(PLAYER_FLAGS);
    PlayerData["data"]["map"] = GetMapId();
    PlayerData["data"]["instance_id"] = GetInstanceId();
    PlayerData["data"]["instance_mode_mask"] = (uint16(GetDungeonDifficulty()) | uint16(GetRaidDifficulty()) << 16);
    PlayerData["data"]["position_x"] = finiteAlways(GetPositionX());
    PlayerData["data"]["position_y"] = finiteAlways(GetPositionY());
    PlayerData["data"]["position_z"] = finiteAlways(GetPositionZ());
    PlayerData["data"]["orientation"] = finiteAlways(GetOrientation());
    std::ostringstream ss;
    ss << m_taxi;
    PlayerData["data"]["taximask"] = ss.str();
    PlayerData["data"]["cinematic"] = m_cinematic;
    PlayerData["data"]["totaltime"] = m_Played_time[PLAYED_TIME_TOTAL];
    PlayerData["data"]["leveltime"] = m_Played_time[PLAYED_TIME_LEVEL];
    PlayerData["data"]["rest_bonus"] = finiteAlways(m_rest_bonus);
    PlayerData["data"]["logout_time"] = uint32(time(NULL));
    PlayerData["data"]["is_logout_resting"] = (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) ? 1 : 0);
    PlayerData["data"]["resettalents_cost"] = GetTalentResetCost();
    PlayerData["data"]["resettalents_time"] = GetTalentResetTime();
    PlayerData["data"]["resetspecialization_cost"] = GetSpecializationResetCost();
    PlayerData["data"]["resetspecialization_time"] = GetSpecializationResetTime();
    ss.str("");
    for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
        ss << uint32(0) << " ";
    PlayerData["data"]["talentTree"] = ss.str();
    PlayerData["data"]["extra_flags"] = m_ExtraFlags;
    PlayerData["data"]["stable_slots"] = m_stableSlots;
    PlayerData["data"]["at_login"] = m_atLoginFlags;
    PlayerData["data"]["zone"] = m_zoneUpdateId;
    PlayerData["data"]["death_expire_time"] = m_deathExpireTime;
    ss.str("");
    ss << m_taxi.SaveTaxiDestinationsToString();
    PlayerData["data"]["taxi_path"] = ss.str();
    PlayerData["data"]["totalKills"] = GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS);
    PlayerData["data"]["todayKills"] = GetUInt16Value(PLAYER_FIELD_KILLS, 0);
    PlayerData["data"]["yesterdayKills"] = GetUInt16Value(PLAYER_FIELD_KILLS, 1);
    PlayerData["data"]["chosenTitle"] = GetUInt32Value(PLAYER_CHOSEN_TITLE);
    PlayerData["data"]["watchedFaction"] = GetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX);
    PlayerData["data"]["drunk"] = GetDrunkValue();
    PlayerData["data"]["health"] = GetHealth();
    PlayerData["data"]["power"] = GetPower(getPowerType());
    PlayerData["data"]["speccount"] = GetSpecsCount();
    PlayerData["data"]["activespec"] = GetActiveSpec();
    PlayerData["data"]["specialization1"] = GetSpecializationId(0);
    PlayerData["data"]["specialization2"] = GetSpecializationId(1);
        ss.str("");
    for (uint32 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; ++i)
        ss << GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + i) << ' ';
    PlayerData["data"]["exploredZones"] = ss.str();
    ss.str("");
    // cache equipment...
    for (uint32 i = 0; i < EQUIPMENT_SLOT_END * 2; ++i)
        ss << GetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + i) << ' ';

    // ...and bags for enum opcode
    for (uint32 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            ss << item->GetEntry();
        else
            ss << '0';
        ss << " 0 ";
    }
    PlayerData["data"]["equipmentCache"] = ss.str();
    ss.str("");
    for (uint32 i = 0; i < KNOWN_TITLES_SIZE*2; ++i)
        ss << GetUInt32Value(PLAYER_FIELD_KNOWN_TITLES + i) << ' ';
    PlayerData["data"]["knownTitles"] = ss.str();
    PlayerData["data"]["actionBars"] = GetByteValue(PLAYER_FIELD_BYTES, 2);
    PlayerData["data"]["currentpetnumber"] = m_currentPetNumber;
    ss.str("");
    for (uint32 i = 0; i < PET_SLOT_LAST; ++i)
        ss << m_PetSlots[i] << ' ';
    PlayerData["data"]["petslot"] = ss.str();
    PlayerData["data"]["grantableLevels"] = m_grantableLevels;
    PlayerData["data"]["lfgBonusFaction"] = GetLfgBonusFaction();

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "data", PlayerData["data"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayer guid %u", guid);
    });
}

void Player::SavePlayerBG()
{
    if (m_bgData.joinPos.m_mapId == MAPID_INVALID)
        return;

    sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerBG userKey %s", GetUserKey());

    PlayerData["bg"]["instanceId"] = m_bgData.bgInstanceID;
    PlayerData["bg"]["team"] = m_bgData.bgTeam;
    PlayerData["bg"]["joinX"] = m_bgData.joinPos.GetPositionX();
    PlayerData["bg"]["joinY"] = m_bgData.joinPos.GetPositionY();
    PlayerData["bg"]["joinZ"] = m_bgData.joinPos.GetPositionZ();
    PlayerData["bg"]["joinO"] = m_bgData.joinPos.GetOrientation();
    PlayerData["bg"]["joinMapId"] = m_bgData.joinPos.GetMapId();
    PlayerData["bg"]["taxiStart"] = m_bgData.taxiPath[0];
    PlayerData["bg"]["taxiEnd"] = m_bgData.taxiPath[1];
    PlayerData["bg"]["mountSpell"] = m_bgData.mountSpell;

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "bg", PlayerData["bg"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerBG guid %u", guid);
    });
}

void Player::SavePlayerGroup()
{
    PlayerData["group"] = GetGroup() ? GetGroup()->GetLowGUID() : 0;

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "group", PlayerData["group"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerGroup guid %u", guid);
    });
}

void Player::InitPlayerLootCooldown()
{
    for(int i = 0; i < MAX_LOOT_COOLDOWN_TYPE; i++)
    {
        std::string type = std::to_string(i);

        for (PlayerLootCooldownMap::iterator itr = m_playerLootCooldown[i].begin(); itr != m_playerLootCooldown[i].end(); ++itr)
        {
            std::string entry = std::to_string(itr->second.entry);
            PlayerData["lootcooldown"][type.c_str()][entry.c_str()]["type"] = itr->second.type;
            PlayerData["lootcooldown"][type.c_str()][entry.c_str()]["difficultyMask"] = itr->second.difficultyMask;
            PlayerData["lootcooldown"][type.c_str()][entry.c_str()]["respawnTime"] = itr->second.respawnTime;
        }
    }

    SavePlayerLootCooldown();
}

void Player::SavePlayerLootCooldown()
{
    if (!PlayerData["lootcooldown"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "lootcooldown", PlayerData["lootcooldown"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerCurrency()
{
    for (PlayerCurrenciesMap::iterator itr = _currencyStorage.begin(); itr != _currencyStorage.end(); ++itr)
    {
        std::string currencyID = std::to_string(itr->first);
        PlayerData["currency"][currencyID.c_str()]["weekCount"] = itr->second.weekCount;
        PlayerData["currency"][currencyID.c_str()]["totalCount"] = itr->second.totalCount;
        PlayerData["currency"][currencyID.c_str()]["seasonTotal"] = itr->second.seasonTotal;
        PlayerData["currency"][currencyID.c_str()]["flags"] = itr->second.flags;
        PlayerData["currency"][currencyID.c_str()]["curentCap"] = itr->second.curentCap;
    }

    SavePlayerCurrency();
}

void Player::SavePlayerCurrency()
{
    if (!PlayerData["currency"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "currency", PlayerData["currency"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerSkills()
{
    for (SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        std::string skill = std::to_string(itr->first);
        uint16 field = itr->second.pos / 2;
        uint8 offset = itr->second.pos & 1;

        uint16 value = GetUInt16Value(PLAYER_SKILL_RANK_0 + field, offset);
        uint16 max = GetUInt16Value(PLAYER_SKILL_MAX_RANK_0 + field, offset);

        PlayerData["skills"][skill.c_str()]["value"] = value;
        PlayerData["skills"][skill.c_str()]["max"] = max;
        PlayerData["skills"][skill.c_str()]["pos"] = itr->second.pos;
    }

    SavePlayerSkills();
}

void Player::SavePlayerSkills()
{
    if (!PlayerData["skills"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "skills", PlayerData["skills"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerTalents()
{
    for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
    {
        std::string spec = std::to_string(i);

        for (PlayerTalentMap::iterator itr = GetTalentMap(i)->begin(); itr != GetTalentMap(i)->end();)
        {
            if (itr->second->state == PLAYERSPELL_REMOVED)
            {
                delete itr->second;
                GetTalentMap(i)->erase(itr++);
            }
            else
            {
                itr->second->state = PLAYERSPELL_UNCHANGED;
                std::string spell = std::to_string(itr->first);
                PlayerData["talents"][spec.c_str()][spell.c_str()] = itr->second->spec;
                ++itr;
            }
        }
    }

    SavePlayerTalents();
}

void Player::SavePlayerTalents()
{
    if (!PlayerData["talents"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "talents", PlayerData["talents"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerSpells()
{
    for (PlayerSpellMap::iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (!itr->second || itr->second->state == PLAYERSPELL_REMOVED)
        {
            if (itr->second)
                delete itr->second;
            m_spells.erase(itr++);
            continue;
        }
        if (!itr->second->dependent)
        {
            std::string spell = std::to_string(itr->first);
            if(itr->second->mount)
            {
                if (initAcData)
                    GetSession()->AccountDatas["mounts"][spell.c_str()]["active"] = itr->second->active;
            }
            else
            {
                PlayerData["spells"][spell.c_str()]["active"] = itr->second->active;
                PlayerData["spells"][spell.c_str()]["disabled"] = itr->second->disabled;
            }
        }
    }

    if (!PlayerData["spells"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "spells", PlayerData["spells"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["mounts"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "mounts", GetSession()->AccountDatas["mounts"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    //Update data skill
    SavePlayerSkills();
}

void Player::InitPlayerGlyphs()
{
    for (uint8 spec = 0; spec < GetSpecsCount(); ++spec)
    {
        std::string specId = std::to_string(spec);
        PlayerData["glyphs"][specId.c_str()]["spec"] = spec;
        for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
        {
            std::string index = std::to_string(i);
            PlayerData["glyphs"][specId.c_str()][index.c_str()] = GetGlyph(spec, i);
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "glyphs", PlayerData["glyphs"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::InitPlayerGlyphs guid %u", guid);
    });
}

void Player::SavePlayerGlyphs()
{
    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "glyphs", PlayerData["glyphs"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerGlyphs guid %u", guid);
    });
}

void Player::SavePlayerAuras()
{
    if (isBeingLoaded())
        return;
    PlayerData["auras"].clear();
    for (AuraMap::const_iterator itr = m_ownedAuras.begin(); itr != m_ownedAuras.end(); ++itr)
    {
        if (!itr->second->CanBeSaved())
            continue;

        Aura* aura = itr->second;
        AuraApplication * foundAura = GetAuraApplication(aura->GetId(), aura->GetCasterGUID(), aura->GetCastItemGUID());

        if(!foundAura)
            continue;

        std::string slot = std::to_string(foundAura->GetSlot());
        uint32 effMask = 0;
        uint32 recalculateMask = 0;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (AuraEffect const* effect = aura->GetEffect(i))
            {
                PlayerData["auras"]["effect"][slot.c_str()]["effect"] = i;
                PlayerData["auras"]["effect"][slot.c_str()]["baseamount"] = effect->GetBaseAmount();
                PlayerData["auras"]["effect"][slot.c_str()]["amount"] = effect->GetAmount();
                effMask |= 1 << i;
                if (effect->CanBeRecalculated())
                    recalculateMask |= 1 << i;
            }
        }

        PlayerData["auras"]["aura"][slot.c_str()]["caster_guid"] = itr->second->GetCasterGUID();
        PlayerData["auras"]["aura"][slot.c_str()]["item_guid"] = itr->second->GetCastItemGUID();
        PlayerData["auras"]["aura"][slot.c_str()]["spell"] = itr->second->GetId();
        PlayerData["auras"]["aura"][slot.c_str()]["effect_mask"] = effMask;
        PlayerData["auras"]["aura"][slot.c_str()]["recalculate_mask"] = recalculateMask;
        PlayerData["auras"]["aura"][slot.c_str()]["stackcount"] = itr->second->GetStackAmount();
        PlayerData["auras"]["aura"][slot.c_str()]["maxduration"] = itr->second->GetMaxDuration();
        PlayerData["auras"]["aura"][slot.c_str()]["remaintime"] = itr->second->GetDuration();
        PlayerData["auras"]["aura"][slot.c_str()]["remaincharges"] = itr->second->GetCharges();
    }

    if (!PlayerData["auras"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "auras", PlayerData["auras"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerQuestStatus()
{
    for (QuestStatusMap::const_iterator itr = m_QuestStatus.begin(); itr != m_QuestStatus.end(); ++itr)
    {
        uint32 questId = itr->first;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
        {
            if (initAcData)
            {
                GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["status"] = int32(itr->second.Status);
                GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["explored"] = itr->second.Explored;
                GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["timer"] = itr->second.Timer;
                GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["playercount"] = itr->second.PlayerCount;
                for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
                {
                    std::string index = std::to_string(i);
                    GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["mobcount"][index.c_str()] = itr->second.CreatureOrGOCount[i];
                    GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["itemcount"][index.c_str()] = itr->second.ItemCount[i];
                }
            }
        }
        else
        {
            PlayerData["queststatus"][quest_id.c_str()]["status"] = int32(itr->second.Status);
            PlayerData["queststatus"][quest_id.c_str()]["explored"] = itr->second.Explored;
            PlayerData["queststatus"][quest_id.c_str()]["timer"] = itr->second.Timer;
            PlayerData["queststatus"][quest_id.c_str()]["playercount"] = itr->second.PlayerCount;
            for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            {
                std::string index = std::to_string(i);
                PlayerData["queststatus"][quest_id.c_str()]["mobcount"][index.c_str()] = itr->second.CreatureOrGOCount[i];
                PlayerData["queststatus"][quest_id.c_str()]["itemcount"][index.c_str()] = itr->second.ItemCount[i];
            }
        }
    }

    if (!PlayerData["queststatus"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "queststatus", PlayerData["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["queststatus"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "queststatus", GetSession()->AccountDatas["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerQuestRewarded()
{
    for (RewardedQuestSet::const_iterator itr = m_RewardedQuests.begin(); itr != m_RewardedQuests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
        {
            if (initAcData)
                GetSession()->AccountDatas["questrewarded"][quest_id.c_str()] = quest_id;
        }
        else
            PlayerData["questrewarded"][quest_id.c_str()] = quest_id;
    }

    if (!PlayerData["questrewarded"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questrewarded", PlayerData["questrewarded"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["questrewarded"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questrewarded", GetSession()->AccountDatas["questrewarded"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerQuestDaily()
{
    for (QuestSet::const_iterator itr = m_dailyquests.begin(); itr != m_dailyquests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
        {
            if (initAcData)
                GetSession()->AccountDatas["questdaily"][quest_id.c_str()] = uint64(m_lastDailyQuestTime);
        }
        else
            PlayerData["questdaily"][quest_id.c_str()] = uint64(m_lastDailyQuestTime);
    }

    for (DFQuestsDoneList::const_iterator itr = m_DFQuests.begin(); itr != m_DFQuests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
        {
            if (initAcData)
                GetSession()->AccountDatas["questdaily"][quest_id.c_str()] = uint64(m_lastDailyQuestTime);
        }
        else
            PlayerData["questdaily"][quest_id.c_str()] = uint64(m_lastDailyQuestTime);
    }

    if (!PlayerData["questdaily"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questdaily", PlayerData["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["questdaily"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questdaily", GetSession()->AccountDatas["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerQuestWeekly()
{
    for (QuestSet::const_iterator itr = m_weeklyquests.begin(); itr != m_weeklyquests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
        {
            if (initAcData)
                GetSession()->AccountDatas["questweekly"][quest_id.c_str()] = questId;
        }
        else
            PlayerData["questweekly"][quest_id.c_str()] = questId;
    }

    if (!PlayerData["questweekly"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questweekly", PlayerData["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["questweekly"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questweekly", GetSession()->AccountDatas["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerQuestSeasonal()
{
    for (SeasonalEventQuestMap::const_iterator iter = m_seasonalquests.begin(); iter != m_seasonalquests.end(); ++iter)
    {
        std::string event_id = std::to_string(iter->first);
        for (SeasonalQuestSet::const_iterator itr = iter->second.begin(); itr != iter->second.end(); ++itr)
        {
            uint32 questId = *itr;
            std::string quest_id = std::to_string(questId);
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;
            if (quest->GetType() == QUEST_TYPE_ACCOUNT)
            {
                if (initAcData)
                    GetSession()->AccountDatas["questseasonal"][event_id.c_str()][quest_id.c_str()] = questId;
            }
            else
                PlayerData["questseasonal"][event_id.c_str()][quest_id.c_str()] = questId;
        }
    }

    if (!PlayerData["questseasonal"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questseasonal", PlayerData["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["questseasonal"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questseasonal", GetSession()->AccountDatas["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerBoundInstances()
{
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        //std::string diff = std::to_string(i);

        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if (itr->second.perm)
            {
                InstanceSave* save = itr->second.save;

                Map* map = sMapMgr->FindMap(save->GetMapId(), save->GetInstanceId());
                if (map && map->IsDungeon())
                {
                    if (InstanceScript* instanceScript = ((InstanceMap*)map)->GetInstanceScript())
                    {
                        save->SetData(instanceScript->GetSaveData());
                        save->SetCompletedEncountersMask(instanceScript->GetCompletedEncounterMask());
                        save->SetChallenge(instanceScript->GetChallengeProgresTime());
                    }
                }

                std::string instanceID = std::to_string(save->GetInstanceId());
                PlayerData["boundinstances"][instanceID.c_str()]["perm"] = itr->second.perm;
                PlayerData["boundinstances"][instanceID.c_str()]["map"] = save->GetMapId();
                PlayerData["boundinstances"][instanceID.c_str()]["difficulty"] = save->GetDifficulty();
                PlayerData["boundinstances"][instanceID.c_str()]["challenge"] = save->GetChallenge();
                PlayerData["boundinstances"][instanceID.c_str()]["data"] = save->GetData();
                PlayerData["boundinstances"][instanceID.c_str()]["completedEncounters"] = save->GetCompletedEncounterMask();
                PlayerData["boundinstances"][instanceID.c_str()]["saveTime"] = save->GetSaveTime();
            }
        }
    }

    SavePlayerBoundInstances();
}

void Player::SavePlayerBoundInstances()
{
    if (!PlayerData["boundinstances"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "boundinstances", PlayerData["boundinstances"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::UpdateInstance(InstanceSave* save)
{
    std::string instanceID = std::to_string(save->GetInstanceId());

    Map* map = sMapMgr->FindMap(save->GetMapId(), save->GetInstanceId());
    if (map && map->IsDungeon())
    {
        if (InstanceScript* instanceScript = ((InstanceMap*)map)->GetInstanceScript())
        {
            save->SetData(instanceScript->GetSaveData());
            save->SetCompletedEncountersMask(instanceScript->GetCompletedEncounterMask());
            save->SetChallenge(instanceScript->GetChallengeProgresTime());
        }
    }

    PlayerData["boundinstances"][instanceID.c_str()]["perm"] = save->GetPerm();
    PlayerData["boundinstances"][instanceID.c_str()]["map"] = save->GetMapId();
    PlayerData["boundinstances"][instanceID.c_str()]["difficulty"] = save->GetDifficulty();
    PlayerData["boundinstances"][instanceID.c_str()]["challenge"] = save->GetChallenge();
    PlayerData["boundinstances"][instanceID.c_str()]["data"] = save->GetData();
    PlayerData["boundinstances"][instanceID.c_str()]["completedEncounters"] = save->GetCompletedEncounterMask();
    PlayerData["boundinstances"][instanceID.c_str()]["saveTime"] = save->GetSaveTime();

    SavePlayerBoundInstances();
}

void Player::InitPlayerBattlePets()
{
    PetJournal journal = GetBattlePetMgr()->GetPetJournal();
    // nothing to save
    if (journal.empty())
        return;

    for (PetJournal::const_iterator pet = journal.begin(); pet != journal.end(); ++pet)
    {
        PetJournalInfo* pjInfo = pet->second;
        if (!pjInfo)
            continue;

        std::string petGuid = std::to_string(pet->first);
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["customName"] = pjInfo->GetCustomName();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["creatureEntry"] = pjInfo->GetCreatureEntry();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["speciesID"] = pjInfo->GetSpeciesID();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["spell"] = pjInfo->GetSummonSpell();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["level"] = pjInfo->GetLevel();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["displayID"] = pjInfo->GetDisplayID();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["power"] = pjInfo->GetPower();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["speed"] = pjInfo->GetSpeed();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["health"] = pjInfo->GetHealth();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["maxHealth"] = pjInfo->GetMaxHealth();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["quality"] = pjInfo->GetQuality();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["xp"] = pjInfo->GetXP();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["flags"] = pjInfo->GetFlags();
        GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["breedID"] = pjInfo->GetBreedID();
    }

    SavePlayerBattlePets();
}

void Player::SavePlayerBattlePets()
{
    if (!GetSession()->AccountDatas["battlepets"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "battlepets", GetSession()->AccountDatas["battlepets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerBattlePetSlots()
{
    for (int i = 0; i < 3; ++i)
    {
        std::string index = std::to_string(i);
        PetBattleSlot* slot = GetBattlePetMgr()->GetPetBattleSlot(i);
        GetSession()->AccountDatas["battlepetslots"][index.c_str()] = slot ? slot->GetPet() : 0;
    }

    if (!PlayerData["battlepetslots"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "battlepetslots", GetSession()->AccountDatas["battlepetslots"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerArchaeology()
{
    if (!sWorld->getBoolConfig(CONFIG_ARCHAEOLOGY_ENABLED))
        return;

    if (!GetSkillValue(SKILL_ARCHAEOLOGY))
        return;

    if (isBeingLoaded())
        return;
    std::stringstream ss;
    for (ResearchSiteSet::const_iterator itr = _researchSites.begin(); itr != _researchSites.end(); ++itr)
        ss << (*itr) << " ";

    PlayerData["archaeology"]["sites"] = ss.str().c_str();
    ss.str("");
    for (uint8 j = 0; j < MAX_RESEARCH_SITES; ++j)
        ss << uint32(_digSites[j].count) << " ";
    PlayerData["archaeology"]["counts"] = ss.str().c_str();

    ss.str("");
    for (uint32 i = 0; i < MAX_RESEARCH_PROJECTS; ++i)
        if (uint16 val = GetUInt16Value(PLAYER_FIELD_RESEARCHING_1 + i / 2, i % 2))
            ss << val << " ";
    PlayerData["archaeology"]["projects"] = ss.str().c_str();

    PlayerData["archaeology"].clear();
    for (CompletedProjectList::iterator itr = _completedProjects.begin(); itr != _completedProjects.end(); ++itr)
    {
        std::string ID = std::to_string(itr->entry->ID);
        PlayerData["archaeology"]["finds"][ID.c_str()]["count"] = itr->count;
        PlayerData["archaeology"]["finds"][ID.c_str()]["date"] = itr->date;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "archaeology", PlayerData["archaeology"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerArchaeology guid %u", guid);
    });
}

void Player::InitPlayerReputation()
{
    for (FactionStateList::const_iterator itr = m_reputationMgr.GetStateList().begin(); itr != m_reputationMgr.GetStateList().end(); ++itr)
    {
        std::string faction = std::to_string(itr->second.ID);
        PlayerData["reputation"][faction.c_str()]["standing"] = itr->second.Standing;
        PlayerData["reputation"][faction.c_str()]["flags"] = itr->second.Flags;
    }

    SavePlayerReputation();
}

void Player::SavePlayerReputation()
{
    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "reputation", PlayerData["reputation"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerReputation player guid %u", guid);
    });
}

void Player::InitPlayerVoidStorage()
{
    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
    {
        if (_voidStorageItems[i]) // unused item
        {
            std::string index = std::to_string(i);
            PlayerData["voidstorage"][index.c_str()]["itemId"] = _voidStorageItems[i]->ItemId;
            PlayerData["voidstorage"][index.c_str()]["itemEntry"] = _voidStorageItems[i]->ItemEntry;
            PlayerData["voidstorage"][index.c_str()]["creatorGuid"] = _voidStorageItems[i]->CreatorGuid;
            PlayerData["voidstorage"][index.c_str()]["randomProperty"] = _voidStorageItems[i]->ItemRandomPropertyId;
            PlayerData["voidstorage"][index.c_str()]["suffixFactor"] = _voidStorageItems[i]->ItemSuffixFactor;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "voidstorage", PlayerData["voidstorage"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::InitPlayerVoidStorage player guid %u", guid);
    });
}

void Player::InitPlayerActions()
{
    for (uint8 i = 0; i < MAX_SPEC_COUNT; ++i)
    {
        for (ActionButtonList::iterator itr = m_actionButtons[i].begin(); itr != m_actionButtons[i].end(); ++itr)
        {
            std::string spec = std::to_string(i);
            std::string index = std::to_string(itr->first);
            PlayerData["actions"][spec.c_str()][index.c_str()]["action"] = itr->second.GetAction();
            PlayerData["actions"][spec.c_str()][index.c_str()]["type"] = itr->second.GetType();
        }
    }

    SavePlayerActions();
}

void Player::SavePlayerActions()
{
    if (!PlayerData["actions"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "actions", PlayerData["actions"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerSocial()
{
    PlayerSocial* social = sSocialMgr->GetPlayerSocial(GetGUIDLow());
    for (PlayerSocialMap::const_iterator itr = social->m_playerSocialMap.begin(); itr != social->m_playerSocialMap.end(); ++itr)
    {
        std::string friend_guid = std::to_string(itr->first);
        PlayerData["social"][friend_guid.c_str()]["Flags"] = itr->second.Flags;
        PlayerData["social"][friend_guid.c_str()]["Note"] = itr->second.Note.c_str();
    }

    if (!PlayerData["social"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "social", PlayerData["social"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerSpellCooldowns()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    PlayerData["spellcooldowns"].clear();
    // remove outdated and save active
    for (SpellCooldowns::iterator itr = m_spellCooldowns.begin(); itr != m_spellCooldowns.end();)
    {
        if (itr->second.end <= curTime)
            m_spellCooldowns.erase(itr++);
        else if (itr->second.end <= infTime)                 // not save locked cooldowns, it will be reset or set at reload
        {
            std::string spell = std::to_string(itr->first);
            PlayerData["spellcooldowns"][spell.c_str()]["item"] = itr->second.itemid;
            PlayerData["spellcooldowns"][spell.c_str()]["time"] = uint64(itr->second.end);
            ++itr;
        }
        else
            ++itr;
    }

    if (!PlayerData["spellcooldowns"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "spellcooldowns", PlayerData["spellcooldowns"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerKills()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    // remove outdated and save active
    for(KillInfoMap::iterator itr = m_killsPerPlayer.begin(); itr != m_killsPerPlayer.end(); ++itr)
    {
        std::string victim = std::to_string(itr->first);
        PlayerData["kills"][victim.c_str()] = itr->second.count;
    }

    if (!PlayerData["kills"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "kills", PlayerData["kills"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerDeclinedName()
{
    if (m_declinedname)
    {
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            std::string index = std::to_string(i);
            PlayerData["declinedname"][index.c_str()] = m_declinedname->name[i].c_str();
        }
    }

    if (!PlayerData["declinedname"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "declinedname", PlayerData["declinedname"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerEquipmentSets()
{
    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end();++itr)
    {
        std::string index = std::to_string(itr->first);
        EquipmentSet& eqset = itr->second;
        PlayerData["equipmentsets"][index.c_str()]["setguid"] = eqset.Guid;
        PlayerData["equipmentsets"][index.c_str()]["name"] = eqset.Name.c_str();
        PlayerData["equipmentsets"][index.c_str()]["iconname"] = eqset.IconName.c_str();
        PlayerData["equipmentsets"][index.c_str()]["ignore_mask"] = eqset.IgnoreMask;
        PlayerData["equipmentsets"][index.c_str()]["item0"] = eqset.Items[0];
        PlayerData["equipmentsets"][index.c_str()]["item1"] = eqset.Items[1];
        PlayerData["equipmentsets"][index.c_str()]["item2"] = eqset.Items[2];
        PlayerData["equipmentsets"][index.c_str()]["item3"] = eqset.Items[3];
        PlayerData["equipmentsets"][index.c_str()]["item4"] = eqset.Items[4];
        PlayerData["equipmentsets"][index.c_str()]["item5"] = eqset.Items[5];
        PlayerData["equipmentsets"][index.c_str()]["item6"] = eqset.Items[6];
        PlayerData["equipmentsets"][index.c_str()]["item7"] = eqset.Items[7];
        PlayerData["equipmentsets"][index.c_str()]["item8"] = eqset.Items[8];
        PlayerData["equipmentsets"][index.c_str()]["item9"] = eqset.Items[9];
        PlayerData["equipmentsets"][index.c_str()]["item10"] = eqset.Items[10];
        PlayerData["equipmentsets"][index.c_str()]["item11"] = eqset.Items[11];
        PlayerData["equipmentsets"][index.c_str()]["item12"] = eqset.Items[12];
        PlayerData["equipmentsets"][index.c_str()]["item13"] = eqset.Items[13];
        PlayerData["equipmentsets"][index.c_str()]["item14"] = eqset.Items[14];
        PlayerData["equipmentsets"][index.c_str()]["item15"] = eqset.Items[15];
        PlayerData["equipmentsets"][index.c_str()]["item16"] = eqset.Items[16];
        PlayerData["equipmentsets"][index.c_str()]["item17"] = eqset.Items[17];
        PlayerData["equipmentsets"][index.c_str()]["item18"] = eqset.Items[18];
    }

    if (!PlayerData["equipmentsets"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "equipmentsets", PlayerData["equipmentsets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerCUFProfiles()
{
    for (uint8 i = 0; i < MAX_CUF_PROFILES; ++i)
    {
        std::string index = std::to_string(i);
        if (_CUFProfiles[i])
        {
            PlayerData["cufprofiles"][index.c_str()]["profileName"] = _CUFProfiles[i]->profileName;
            PlayerData["cufprofiles"][index.c_str()]["frameHeight"] = _CUFProfiles[i]->frameHeight;
            PlayerData["cufprofiles"][index.c_str()]["frameWidth"] = _CUFProfiles[i]->frameWidth;
            PlayerData["cufprofiles"][index.c_str()]["sortBy"] = _CUFProfiles[i]->sortBy;
            PlayerData["cufprofiles"][index.c_str()]["showHealthText"] = _CUFProfiles[i]->showHealthText;
            PlayerData["cufprofiles"][index.c_str()]["options"] = _CUFProfiles[i]->options;
            PlayerData["cufprofiles"][index.c_str()]["Unk146"] = _CUFProfiles[i]->Unk146;
            PlayerData["cufprofiles"][index.c_str()]["Unk147"] = _CUFProfiles[i]->Unk147;
            PlayerData["cufprofiles"][index.c_str()]["Unk148"] = _CUFProfiles[i]->Unk148;
            PlayerData["cufprofiles"][index.c_str()]["Unk150"] = _CUFProfiles[i]->Unk150;
            PlayerData["cufprofiles"][index.c_str()]["Unk152"] = _CUFProfiles[i]->Unk152;
            PlayerData["cufprofiles"][index.c_str()]["Unk154"] = _CUFProfiles[i]->Unk154;
        }
        else if(PlayerData["cufprofiles"].isMember(index.c_str()))
            PlayerData["cufprofiles"].removeMember(index.c_str());
    }

    if (!PlayerData["cufprofiles"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "cufprofiles", PlayerData["cufprofiles"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerVisuals()
{
    if (m_vis)
    {
        PlayerData["visuals"]["head"] = m_vis->m_visHead;
        PlayerData["visuals"]["shoulders"] = m_vis->m_visShoulders;
        PlayerData["visuals"]["chest"] = m_vis->m_visChest;
        PlayerData["visuals"]["waist"] = m_vis->m_visWaist;
        PlayerData["visuals"]["legs"] = m_vis->m_visLegs;
        PlayerData["visuals"]["feet"] = m_vis->m_visFeet;
        PlayerData["visuals"]["wrists"] = m_vis->m_visWrists;
        PlayerData["visuals"]["hands"] = m_vis->m_visHands;
        PlayerData["visuals"]["back"] = m_vis->m_visBack;
        PlayerData["visuals"]["main"] = m_vis->m_visMainhand;
        PlayerData["visuals"]["off"] = m_vis->m_visOffhand;
        PlayerData["visuals"]["ranged"] = m_vis->m_visRanged;
        PlayerData["visuals"]["tabard"] = m_vis->m_visTabard;
        PlayerData["visuals"]["shirt"] = m_vis->m_visShirt;
    }

    if (!PlayerData["visuals"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "visuals", PlayerData["visuals"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerAccountData()
{
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
    {
        if (PER_CHARACTER_CACHE_MASK & (1 << i))
        {
            std::string index = std::to_string(i);
            PlayerData["accountdata"][index.c_str()]["Time"] = GetSession()->m_accountData[i].Time;
            PlayerData["accountdata"][index.c_str()]["Data"] = GetSession()->m_accountData[i].Data.c_str();
        }
        if (GLOBAL_CACHE_MASK & (1 << i))
        {
            std::string index = std::to_string(i);
            GetSession()->AccountDatas["data"][index.c_str()]["Time"] = GetSession()->m_accountData[i].Time;
            GetSession()->AccountDatas["data"][index.c_str()]["Data"] = GetSession()->m_accountData[i].Data.c_str();
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "accountdata", PlayerData["accountdata"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::InitPlayerAccountData player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "accountdata", GetSession()->AccountDatas["data"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::InitPlayerAccountData player guid %u", guid);
    });
}

void Player::SavePlayerHomeBind()
{
    PlayerData["homebind"]["mapId"] = m_homebindMapId;
    PlayerData["homebind"]["zoneId"] = m_homebindAreaId;
    PlayerData["homebind"]["posX"] = m_homebindX;
    PlayerData["homebind"]["posY"] = m_homebindY;
    PlayerData["homebind"]["posZ"] = m_homebindZ;

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "homebind", PlayerData["homebind"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerHomeBind player guid %u", guid);
    });
}

void Player::InitAchievement()
{
    for (auto iter = GetAchievementMgr().GetCompletedAchievementsList().begin(); iter != GetAchievementMgr().GetCompletedAchievementsList().end(); ++iter)
    {
        std::string achievementId = std::to_string(iter->first);
        if (iter->second.first_guid_real != iter->second.first_guid || iter->second.first_guid_real == GetGUIDLow())
        {
            PlayerData["achievement"][achievementId.c_str()]["date"] = iter->second.date;
            PlayerData["achievement"][achievementId.c_str()]["changed"] = iter->second.changed;
        }

        if (initAcData)
        {
            GetSession()->AccountDatas["achievement"][achievementId.c_str()]["first_guid"] = iter->second.first_guid_real;
            GetSession()->AccountDatas["achievement"][achievementId.c_str()]["date"] = iter->second.date;
            GetSession()->AccountDatas["achievement"][achievementId.c_str()]["changed"] = iter->second.changed;
        }
    }

    if (!PlayerData["achievement"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "achievement", PlayerData["achievement"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["achievement"].empty() && initAcData)
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "achievement", GetSession()->AccountDatas["achievement"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitCriteria()
{
    for (auto itr = GetAchievementMgr().GetAchievementProgress().begin(); itr != GetAchievementMgr().GetAchievementProgress().end(); ++itr)
    {
        std::string achievID = std::to_string(itr->first);
        bool save_pl = false;
        bool save_ac = false;
        Json::Value CriteriaPl;
        Json::Value CriteriaAc;

        if(auto progressMap = &itr->second)
        {
            for (auto iter = progressMap->begin(); iter != progressMap->end(); ++iter)
            {
                if (iter->second.deactiveted)
                    continue;

                //disable? active before test achivement system
                AchievementEntry const* achievement = iter->second.achievement;
                if (!achievement)
                    continue;

                // store data only for real progress
                bool hasAchieve = HasAchieved(achievement->ID) || (achievement->parent && !HasAchieved(achievement->parent));
                if (iter->second.counter != 0 && !hasAchieve)
                {
                    std::string criteriaId = std::to_string(iter->first);
                    if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
                    {
                        if (initAcData)
                        {
                            save_ac = true;
                            CriteriaAc[criteriaId.c_str()]["counter"] = iter->second.counter;
                            CriteriaAc[criteriaId.c_str()]["date"] = iter->second.date;
                            CriteriaAc[criteriaId.c_str()]["completed"] = iter->second.completed;
                        }
                    }
                    else
                    {
                        save_pl = true;
                        CriteriaPl[criteriaId.c_str()]["counter"] = iter->second.counter;
                        CriteriaPl[criteriaId.c_str()]["date"] = iter->second.date;
                        CriteriaPl[criteriaId.c_str()]["completed"] = iter->second.completed;
                    }
                }
            }
        }

        if (save_pl)
        {
            PlayerData["criteria"][achievID.c_str()] = CriteriaPl;
            RedisDatabase.AsyncExecuteHSet("HSET", GetCriteriaPlKey(), achievID.c_str(), CriteriaPl, GetGUID(), [&](const RedisValue &v, uint64 guid) {});
        }

        if (save_ac)
        {
            GetSession()->AccountDatas["criteria"][achievID.c_str()] = CriteriaAc;
            RedisDatabase.AsyncExecuteHSet("HSET", GetCriteriaAcKey(), achievID.c_str(), CriteriaAc, GetGUID(), [&](const RedisValue &v, uint64 guid) {});
        }
    }
}

void Player::InitPlayerPets()
{
    QueryResult result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, name, renamed, curhealth, curmana, abdata, savetime, CreatedBySpell, PetType, specialization FROM character_pet WHERE owner = '%u'", GetGUIDLow());
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            std::string id = std::to_string(fields[0].GetUInt32());
            PlayerData["pets"][id.c_str()]["entry"] = fields[1].GetUInt32();
            PlayerData["pets"][id.c_str()]["modelid"] = fields[3].GetUInt32();
            PlayerData["pets"][id.c_str()]["level"] = fields[4].GetUInt16();
            PlayerData["pets"][id.c_str()]["exp"] = fields[5].GetUInt32();
            PlayerData["pets"][id.c_str()]["Reactstate"] = fields[6].GetUInt8();
            PlayerData["pets"][id.c_str()]["name"] = fields[7].GetString();
            PlayerData["pets"][id.c_str()]["renamed"] = fields[8].GetBool();
            PlayerData["pets"][id.c_str()]["curhealth"] = fields[9].GetUInt32();
            PlayerData["pets"][id.c_str()]["curmana"] = fields[10].GetUInt32();
            PlayerData["pets"][id.c_str()]["abdata"] = fields[11].GetString();
            PlayerData["pets"][id.c_str()]["savetime"] = fields[12].GetUInt32();
            PlayerData["pets"][id.c_str()]["CreatedBySpell"] = fields[13].GetUInt32();
            PlayerData["pets"][id.c_str()]["PetType"] = fields[14].GetUInt8();
            PlayerData["pets"][id.c_str()]["specialization"] = fields[15].GetUInt32();
        }
        while (result->NextRow());
    }

    if (!PlayerData["pets"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "pets", PlayerData["pets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::CreatedPlayerPets()
{
    if (getClass() == CLASS_HUNTER || getClass() == CLASS_WARLOCK)
    {
        m_currentPetNumber = sObjectMgr->GenerateLowGuid(HIGHGUID_PET);
        std::string petId = std::to_string(m_currentPetNumber);

        if (getClass() == CLASS_WARLOCK)
        {
            PlayerData["pets"][petId.c_str()]["entry"] = 416;
            PlayerData["pets"][petId.c_str()]["modelid"] = 4449;
            PlayerData["pets"][petId.c_str()]["level"] = 1;
            PlayerData["pets"][petId.c_str()]["exp"] = 0;
            PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
            PlayerData["pets"][petId.c_str()]["name"] = "Imp";
            PlayerData["pets"][petId.c_str()]["renamed"] = true;
            PlayerData["pets"][petId.c_str()]["curhealth"] = 36;
            PlayerData["pets"][petId.c_str()]["curmana"] = 200;
            PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 4 129 3110 1 0 1 0 1 0 6 2 6 3 6 0 ";
            PlayerData["pets"][petId.c_str()]["savetime"] = 1409586285;
            PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 0;
            PlayerData["pets"][petId.c_str()]["PetType"] = 0;
            PlayerData["pets"][petId.c_str()]["specialization"] = 0;
        }else
        {
            switch (getRace())
            {
                case RACE_HUMAN:
                    PlayerData["pets"][petId.c_str()]["entry"] = 299;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 903;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Wolf";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727347;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_DWARF:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42713;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 822;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Bear";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 212;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727650;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_ORC:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42719;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 744;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Boar";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 212;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727175;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_NIGHTELF:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42718;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 17090;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Cat";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727501;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_UNDEAD_PLAYER:
                    PlayerData["pets"][petId.c_str()]["entry"] = 51107;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 368;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Spider";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 202;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727821;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_TAUREN:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42720;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 29057;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Tallstrider";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727912;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_TROLL:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42721;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 23518;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Raptor";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 50498 129 16827 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295727987;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_GOBLIN:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42715;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 27692;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Crab";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 212;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295720595;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_BLOODELF:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42710;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 23515;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Dragonhawk";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 202;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295728068;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_DRAENEI:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42712;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 29056;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = 'Moth';
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 49966 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295728128;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_WORGEN:
                    PlayerData["pets"][petId.c_str()]["entry"] = 42722;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 30221;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Dog";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295728219;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                case RACE_PANDAREN_NEUTRAL:
                    PlayerData["pets"][petId.c_str()]["entry"] = 57239;
                    PlayerData["pets"][petId.c_str()]["modelid"] = 42656;
                    PlayerData["pets"][petId.c_str()]["CreatedBySpell"] = 13481;
                    PlayerData["pets"][petId.c_str()]["PetType"] = 1;
                    PlayerData["pets"][petId.c_str()]["level"] = 1;
                    PlayerData["pets"][petId.c_str()]["exp"] = 0;
                    PlayerData["pets"][petId.c_str()]["Reactstate"] = 0;
                    PlayerData["pets"][petId.c_str()]["name"] = "Turtle";
                    PlayerData["pets"][petId.c_str()]["renamed"] = false;
                    PlayerData["pets"][petId.c_str()]["curhealth"] = 192;
                    PlayerData["pets"][petId.c_str()]["curmana"] = 0;
                    PlayerData["pets"][petId.c_str()]["abdata"] = "7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ";
                    PlayerData["pets"][petId.c_str()]["savetime"] = 1295728219;
                    PlayerData["pets"][petId.c_str()]["specialization"] = 0;
                    break;
                default:
                    break;
            }
        }
        SetOnAnyFreeSlot(m_currentPetNumber);
        SetTemporaryUnsummonedPetNumber(m_currentPetNumber);
    }

    if (!PlayerData["pets"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "pets", PlayerData["pets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::InitPlayerMails()
{
    //load players mails, and mailed items
    if (!m_mailsLoaded)
        _LoadMail();

    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        Mail* m = (*itr);
        std::string messageID = std::to_string(m->messageID);
        m->MailJson["messageType"] = m->messageType;
        m->MailJson["sender"] = m->sender;
        m->MailJson["receiver"] = m->receiver;
        m->MailJson["subject"] = m->subject;
        m->MailJson["body"] = m->body;
        m->MailJson["has_items"] = m->HasItems();
        m->MailJson["expire_time"] = m->expire_time;
        m->MailJson["deliver_time"] = m->deliver_time;
        m->MailJson["money"] = m->money;
        m->MailJson["COD"] = m->COD;
        m->MailJson["checked"] = m->checked;
        m->MailJson["stationery"] = m->stationery;
        m->MailJson["mailTemplateId"] = m->mailTemplateId;

        PlayerMailData["mails"][messageID.c_str()] = m->MailJson;
        PlayerMailData["mails"][messageID.c_str()]["items"] = m->MailItemJson;

        RedisDatabase.AsyncExecuteHSet("HSET", mailKey, messageID.c_str(), m->MailJson, GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::InitPlayerMails player guid %u", guid);
        });

        Json::Value expireStr = m->expire_time;
        RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetMailsKey(), messageID.c_str(), expireStr, GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    }
}

void Player::SavePlayerGold()
{
    PlayerData["gold"] = GetMoney();

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "gold", PlayerData["gold"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerGold guid %u", guid);
    });
}

void Player::SaveAccountTutorials()
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
    {
        std::string index = std::to_string(i);
        GetSession()->AccountDatas["tutorials"][index.c_str()] = GetSession()->GetTutorialInt(i);
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "tutorials", GetSession()->AccountDatas["tutorials"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerGold guid %u", guid);
    });
}

void Player::SavePlayerGuild()
{
    PlayerData["guild"] = GetGuildId();

    RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "guild", PlayerData["guild"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerGuild guid %u", guid);
    });
}

void Player::SavePlayerPetitions()
{
    if (!PlayerData["petitions"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "petitions", PlayerData["petitions"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerSpells()
{
    if (!PlayerData["spells"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "spells", PlayerData["spells"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!GetSession()->AccountDatas["mounts"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "mounts", GetSession()->AccountDatas["mounts"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
}

void Player::SavePlayerDataAll()
{
    if (!PlayerData["data"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "data", PlayerData["data"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["gold"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "gold", PlayerData["gold"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["bg"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "bg", PlayerData["bg"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["group"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "group", PlayerData["group"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    SavePlayerLootCooldown();
    SavePlayerCurrency();
    SavePlayerSkills();
    SavePlayerTalents();
    SavePlayerSpells();
    SavePlayerGlyphs();

    if (!PlayerData["auras"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "auras", PlayerData["auras"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["queststatus"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "queststatus", PlayerData["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["queststatus"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "queststatus", GetSession()->AccountDatas["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["questrewarded"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questrewarded", PlayerData["questrewarded"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["questrewarded"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questrewarded", GetSession()->AccountDatas["questrewarded"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["questdaily"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questdaily", PlayerData["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["questdaily"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questdaily", GetSession()->AccountDatas["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["questweekly"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questweekly", PlayerData["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["questweekly"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questweekly", GetSession()->AccountDatas["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["questseasonal"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "questseasonal", PlayerData["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["questseasonal"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questseasonal", GetSession()->AccountDatas["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    SavePlayerBoundInstances();
    SavePlayerBattlePets();

    if (!GetSession()->AccountDatas["battlepetslots"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "battlepetslots", GetSession()->AccountDatas["battlepetslots"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["archaeology"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "archaeology", PlayerData["archaeology"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    SavePlayerReputation();

    if (!PlayerData["voidstorage"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "voidstorage", PlayerData["voidstorage"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    SavePlayerActions();

    if (!PlayerData["social"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "social", PlayerData["social"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["spellcooldowns"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "spellcooldowns", PlayerData["spellcooldowns"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["kills"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "kills", PlayerData["kills"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["declinedname"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "declinedname", PlayerData["declinedname"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["equipmentsets"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "equipmentsets", PlayerData["equipmentsets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["cufprofiles"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "cufprofiles", PlayerData["cufprofiles"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["visuals"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "visuals", PlayerData["visuals"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["accountdata"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "accountdata", PlayerData["accountdata"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["data"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "data", GetSession()->AccountDatas["data"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["homebind"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "homebind", PlayerData["homebind"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    if (!PlayerData["achievement"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "achievement", PlayerData["achievement"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});
    if (!GetSession()->AccountDatas["achievement"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "achievement", GetSession()->AccountDatas["achievement"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    InitCriteria();

    if (!PlayerData["pets"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetUserKey(), "pets", PlayerData["pets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    InitPlayerMails();
    SavePlayerGold();

    if (!GetSession()->AccountDatas["tutorials"].empty())
        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "tutorials", GetSession()->AccountDatas["tutorials"], GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    SavePlayerGuild();
    SavePlayerPetitions();
}
