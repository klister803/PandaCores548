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
#include "SerializePlayer.h"
#include "AchievementMgr.h"

void Player::InitSerializePlayer()
{
    if (!RedisDatabase.isConnected())
        return;

    SerializePlayer();
    SerializePlayerBG();
    SerializePlayerGroup();
    SerializePlayerLootCooldown();
    SerializePlayerCurrency();
    SerializePlayerBoundInstances();
    SerializePlayerInstanceTimes();
    SerializePlayerSkills();
    SerializePlayerTalents();
    SerializePlayerSpells();
    SerializePlayerGlyphs();
    SerializePlayerAuras();
    SerializePlayerQuestStatus();
    SerializePlayerQuestRewarded();
    SerializePlayerQuestDaily();
    SerializePlayerQuestWeekly();
    SerializePlayerQuestSeasonal();
    SerializePlayerBattlePets();
    SerializePlayerBattlePetSlots();
    SerializePlayerArchaeology();
    SerializePlayerReputation();
    SerializePlayerVoidStorage();
    SerializePlayerActions();
    SerializePlayerSocial();
    SerializePlayerSpellCooldowns();
    SerializePlayerKills();
    SerializePlayerDeclinedName();
    SerializePlayerEquipmentSets();
    SerializePlayerCUFProfiles();
    SerializePlayerVisuals();
    SerializePlayerAccountData();
    SerializePlayerHomeBind();
    SerializePlayerAchievement();
    SerializePlayerCriteria();
}

void Player::SerializePlayer()
{
    sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayer userKey %s", userKey);

    PlayerJson["realm"] = realmID;
    PlayerJson["guid"] = GetGUIDLow();
    PlayerJson["account"] = GetSession()->GetAccountId();
    PlayerJson["name"] = GetName();
    PlayerJson["race"] = getRace();
    PlayerJson["class"] = getClass();
    PlayerJson["gender"] = getGender();
    PlayerJson["level"] = getLevel();
    PlayerJson["xp"] = GetUInt32Value(PLAYER_XP);
    PlayerJson["money"] = GetMoney();
    PlayerJson["playerBytes"] = GetUInt32Value(PLAYER_BYTES);
    PlayerJson["playerBytes2"] = GetUInt32Value(PLAYER_BYTES_2);
    PlayerJson["playerFlags"] = GetUInt32Value(PLAYER_FLAGS);
    PlayerJson["map"] = GetMapId();
    PlayerJson["instance_id"] = GetInstanceId();
    PlayerJson["instance_mode_mask"] = (uint16(GetDungeonDifficulty()) | uint16(GetRaidDifficulty()) << 16);
    PlayerJson["position_x"] = finiteAlways(GetPositionX());
    PlayerJson["position_y"] = finiteAlways(GetPositionY());
    PlayerJson["position_z"] = finiteAlways(GetPositionZ());
    PlayerJson["orientation"] = finiteAlways(GetOrientation());
    std::ostringstream ss;
    ss << m_taxi;
    PlayerJson["taximask"] = ss.str();
    PlayerJson["cinematic"] = m_cinematic;
    PlayerJson["totaltime"] = m_Played_time[PLAYED_TIME_TOTAL];
    PlayerJson["leveltime"] = m_Played_time[PLAYED_TIME_LEVEL];
    PlayerJson["rest_bonus"] = finiteAlways(m_rest_bonus);
    PlayerJson["logout_time"] = uint32(time(NULL));
    PlayerJson["is_logout_resting"] = (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) ? 1 : 0);
    PlayerJson["resettalents_cost"] = GetTalentResetCost();
    PlayerJson["resettalents_time"] = GetTalentResetTime();
    PlayerJson["resetspecialization_cost"] = GetSpecializationResetCost();
    PlayerJson["resetspecialization_time"] = GetSpecializationResetTime();
    ss.str("");
    for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
        ss << uint32(0) << " ";
    PlayerJson["talentTree"] = ss.str();
    PlayerJson["extra_flags"] = m_ExtraFlags;
    PlayerJson["stable_slots"] = m_stableSlots;
    PlayerJson["at_login"] = m_atLoginFlags;
    PlayerJson["zone"] = m_zoneUpdateId;
    PlayerJson["death_expire_time"] = m_deathExpireTime;
    ss.str("");
    ss << m_taxi.SaveTaxiDestinationsToString();
    PlayerJson["taxi_path"] = ss.str();
    PlayerJson["totalKills"] = GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS);
    PlayerJson["todayKills"] = GetUInt16Value(PLAYER_FIELD_KILLS, 0);
    PlayerJson["yesterdayKills"] = GetUInt16Value(PLAYER_FIELD_KILLS, 1);
    PlayerJson["chosenTitle"] = GetUInt32Value(PLAYER_CHOSEN_TITLE);
    PlayerJson["watchedFaction"] = GetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX);
    PlayerJson["drunk"] = GetDrunkValue();
    PlayerJson["health"] = GetHealth();
    PlayerJson["power"] = GetPower(getPowerType());
    PlayerJson["speccount"] = GetSpecsCount();
    PlayerJson["activespec"] = GetActiveSpec();
    PlayerJson["specialization1"] = GetSpecializationId(0);
    PlayerJson["specialization2"] = GetSpecializationId(1);
        ss.str("");
    for (uint32 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; ++i)
        ss << GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + i) << ' ';
    PlayerJson["exploredZones"] = ss.str();
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
    PlayerJson["equipmentCache"] = ss.str();
    ss.str("");
    for (uint32 i = 0; i < KNOWN_TITLES_SIZE*2; ++i)
        ss << GetUInt32Value(PLAYER_FIELD_KNOWN_TITLES + i) << ' ';
    PlayerJson["knownTitles"] = ss.str();
    PlayerJson["actionBars"] = GetByteValue(PLAYER_FIELD_BYTES, 2);
    PlayerJson["currentpetnumber"] = m_currentPetNumber;
    ss.str("");
    for (uint32 i = 0; i < PET_SLOT_LAST; ++i)
        ss << m_PetSlots[i] << ' ';
    PlayerJson["petslot"] = m_currentPetNumber;
    PlayerJson["grantableLevels"] = m_grantableLevels;
    PlayerJson["lfgBonusFaction"] = GetLfgBonusFaction();

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "userdata", jsonBuilder.write(PlayerJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayer guid %u", guid);
    });
}

void Player::SerializePlayerBG()
{
    sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerBG userKey %s", userKey);

    PlayerBGJson["instanceId"] = m_bgData.bgInstanceID;
    PlayerBGJson["team"] = m_bgData.bgTeam;
    PlayerBGJson["joinX"] = m_bgData.joinPos.GetPositionX();
    PlayerBGJson["joinY"] = m_bgData.joinPos.GetPositionY();
    PlayerBGJson["joinZ"] = m_bgData.joinPos.GetPositionZ();
    PlayerBGJson["joinO"] = m_bgData.joinPos.GetOrientation();
    PlayerBGJson["joinMapId"] = m_bgData.joinPos.GetMapId();
    PlayerBGJson["taxiStart"] = m_bgData.taxiPath[0];
    PlayerBGJson["taxiEnd"] = m_bgData.taxiPath[1];
    PlayerBGJson["mountSpell"] = m_bgData.mountSpell;

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "BGdata", jsonBuilder.write(PlayerBGJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerBG guid %u", guid);
    });
}

void Player::SerializePlayerGroup()
{
    PlayerGroupJson["guid"] = GetGroup() ? GetGroup()->GetGUID() : 0;

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "group", jsonBuilder.write(PlayerGroupJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerGroup guid %u", guid);
    });
}

void Player::SerializePlayerLootCooldown()
{
    for(int i = 0; i < MAX_LOOT_COOLDOWN_TYPE; i++)
    {
        std::string type = std::to_string(i);

        for (PlayerLootCooldownMap::iterator itr = m_playerLootCooldown[i].begin(); itr != m_playerLootCooldown[i].end(); ++itr)
        {
            std::string entry = std::to_string(itr->second.entry);
            PlayerLootCooldownJson[type.c_str()][entry.c_str()]["type"] = itr->second.type;
            PlayerLootCooldownJson[type.c_str()][entry.c_str()]["difficultyMask"] = itr->second.difficultyMask;
            PlayerLootCooldownJson[type.c_str()][entry.c_str()]["respawnTime"] = itr->second.respawnTime;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "lootCooldown", jsonBuilder.write(PlayerLootCooldownJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerLootCooldown guid %u", guid);
    });
}

void Player::SerializePlayerCurrency()
{
    for (PlayerCurrenciesMap::iterator itr = _currencyStorage.begin(); itr != _currencyStorage.end(); ++itr)
    {
        std::string currencyID = std::to_string(itr->first);
        PlayerCurrencyJson[currencyID.c_str()]["weekCount"] = itr->second.weekCount;
        PlayerCurrencyJson[currencyID.c_str()]["totalCount"] = itr->second.totalCount;
        PlayerCurrencyJson[currencyID.c_str()]["seasonTotal"] = itr->second.seasonTotal;
        PlayerCurrencyJson[currencyID.c_str()]["flags"] = itr->second.flags;
        PlayerCurrencyJson[currencyID.c_str()]["curentCap"] = itr->second.curentCap;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "currency", jsonBuilder.write(PlayerCurrencyJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerCurrency guid %u", guid);
    });
}

void Player::SerializePlayerInstanceTimes()
{
    for (InstanceTimeMap::const_iterator itr = _instanceResetTimes.begin(); itr != _instanceResetTimes.end(); ++itr)
    {
        std::string instanceId = std::to_string(itr->first);
        PlayerInstanceTimesJson[instanceId.c_str()]["releaseTime"] = itr->second;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "instancetime", jsonBuilder.write(PlayerInstanceTimesJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerInstanceTimes guid %u", guid);
    });
}

void Player::SerializePlayerSkills()
{
    for (SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        std::string skill = std::to_string(itr->first);
        uint16 field = itr->second.pos / 2;
        uint8 offset = itr->second.pos & 1;

        uint16 value = GetUInt16Value(PLAYER_SKILL_RANK_0 + field, offset);
        uint16 max = GetUInt16Value(PLAYER_SKILL_MAX_RANK_0 + field, offset);

        PlayerSkillsJson[skill.c_str()]["value"] = value;
        PlayerSkillsJson[skill.c_str()]["max"] = max;
        PlayerSkillsJson[skill.c_str()]["pos"] = itr->second.pos;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "skills", jsonBuilder.write(PlayerSkillsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerSkills guid %u", guid);
    });
}

void Player::SerializePlayerTalents()
{
    for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
    {
        std::string spec = std::to_string(i);

        for (PlayerTalentMap::iterator itr = GetTalentMap(i)->begin(); itr != GetTalentMap(i)->end(); ++itr)
        {
            std::string spell = std::to_string(itr->first);
            PlayerTalentsJson[spec.c_str()][spell.c_str()] = itr->second->spec;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "talents", jsonBuilder.write(PlayerTalentsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerTalents guid %u", guid);
    });
}

void Player::SerializePlayerSpells()
{
    for (PlayerSpellMap::iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        std::string spell = std::to_string(itr->first);
        if(itr->second->mount)
            PlayerMountsJson[spell.c_str()]["active"] = itr->second->active;
        else
        {
            PlayerSpellsJson[spell.c_str()]["active"] = itr->second->active;
            PlayerSpellsJson[spell.c_str()]["disabled"] = itr->second->disabled;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "spells", jsonBuilder.write(PlayerSpellsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerSpells Spells guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "mounts", jsonBuilder.write(PlayerMountsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerSpells mounts guid %u", guid);
    });
}

void Player::SerializePlayerGlyphs()
{
    for (uint8 spec = 0; spec < GetSpecsCount(); ++spec)
    {
        std::string specId = std::to_string(spec);
        PlayerGlyphsJson[specId.c_str()]["spec"] = spec;
        PlayerGlyphsJson[specId.c_str()]["glyph1"] = GetGlyph(spec, 0);
        PlayerGlyphsJson[specId.c_str()]["glyph2"] = GetGlyph(spec, 1);
        PlayerGlyphsJson[specId.c_str()]["glyph3"] = GetGlyph(spec, 2);
        PlayerGlyphsJson[specId.c_str()]["glyph4"] = GetGlyph(spec, 3);
        PlayerGlyphsJson[specId.c_str()]["glyph5"] = GetGlyph(spec, 4);
        PlayerGlyphsJson[specId.c_str()]["glyph6"] = GetGlyph(spec, 5);
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "glyphs", jsonBuilder.write(PlayerGlyphsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerGlyphs guid %u", guid);
    });
}

void Player::SerializePlayerAuras()
{
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
                PlayerAurasJson["effect"][slot.c_str()]["effect"] = i;
                PlayerAurasJson["effect"][slot.c_str()]["baseamount"] = effect->GetBaseAmount();
                PlayerAurasJson["effect"][slot.c_str()]["amount"] = effect->GetAmount();
                effMask |= 1 << i;
                if (effect->CanBeRecalculated())
                    recalculateMask |= 1 << i;
            }
        }

        PlayerAurasJson["aura"][slot.c_str()]["caster_guid"] = itr->second->GetCasterGUID();
        PlayerAurasJson["aura"][slot.c_str()]["item_guid"] = itr->second->GetCastItemGUID();
        PlayerAurasJson["aura"][slot.c_str()]["spell"] = itr->second->GetId();
        PlayerAurasJson["aura"][slot.c_str()]["effect_mask"] = effMask;
        PlayerAurasJson["aura"][slot.c_str()]["recalculate_mask"] = recalculateMask;
        PlayerAurasJson["aura"][slot.c_str()]["stackcount"] = itr->second->GetStackAmount();
        PlayerAurasJson["aura"][slot.c_str()]["maxduration"] = itr->second->GetMaxDuration();
        PlayerAurasJson["aura"][slot.c_str()]["remaintime"] = itr->second->GetDuration();
        PlayerAurasJson["aura"][slot.c_str()]["remaincharges"] = itr->second->GetCharges();
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "auras", jsonBuilder.write(PlayerAurasJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerAuras auras guid %u", guid);
    });
}

void Player::SerializePlayerQuestStatus()
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
            AccountQuestStatusJson[quest_id.c_str()]["status"] = int32(itr->second.Status);
            AccountQuestStatusJson[quest_id.c_str()]["explored"] = itr->second.Explored;
            AccountQuestStatusJson[quest_id.c_str()]["timer"] = itr->second.Timer;
            AccountQuestStatusJson[quest_id.c_str()]["playercount"] = itr->second.PlayerCount;
            AccountQuestStatusJson[quest_id.c_str()]["mobcount1"] = itr->second.CreatureOrGOCount[0];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount2"] = itr->second.CreatureOrGOCount[1];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount3"] = itr->second.CreatureOrGOCount[2];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount4"] = itr->second.CreatureOrGOCount[3];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount5"] = itr->second.CreatureOrGOCount[4];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount6"] = itr->second.CreatureOrGOCount[5];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount7"] = itr->second.CreatureOrGOCount[6];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount8"] = itr->second.CreatureOrGOCount[7];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount9"] = itr->second.CreatureOrGOCount[8];
            AccountQuestStatusJson[quest_id.c_str()]["mobcount10"] = itr->second.CreatureOrGOCount[9];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount1"] = itr->second.ItemCount[0];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount2"] = itr->second.ItemCount[1];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount3"] = itr->second.ItemCount[2];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount4"] = itr->second.ItemCount[3];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount5"] = itr->second.ItemCount[4];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount6"] = itr->second.ItemCount[5];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount7"] = itr->second.ItemCount[6];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount8"] = itr->second.ItemCount[7];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount9"] = itr->second.ItemCount[8];
            AccountQuestStatusJson[quest_id.c_str()]["itemcount10"] = itr->second.ItemCount[9];
        }
        else
        {
            PlayerQuestStatusJson[quest_id.c_str()]["status"] = int32(itr->second.Status);
            PlayerQuestStatusJson[quest_id.c_str()]["explored"] = itr->second.Explored;
            PlayerQuestStatusJson[quest_id.c_str()]["timer"] = itr->second.Timer;
            PlayerQuestStatusJson[quest_id.c_str()]["playercount"] = itr->second.PlayerCount;
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount1"] = itr->second.CreatureOrGOCount[0];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount2"] = itr->second.CreatureOrGOCount[1];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount3"] = itr->second.CreatureOrGOCount[2];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount4"] = itr->second.CreatureOrGOCount[3];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount5"] = itr->second.CreatureOrGOCount[4];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount6"] = itr->second.CreatureOrGOCount[5];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount7"] = itr->second.CreatureOrGOCount[6];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount8"] = itr->second.CreatureOrGOCount[7];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount9"] = itr->second.CreatureOrGOCount[8];
            PlayerQuestStatusJson[quest_id.c_str()]["mobcount10"] = itr->second.CreatureOrGOCount[9];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount1"] = itr->second.ItemCount[0];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount2"] = itr->second.ItemCount[1];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount3"] = itr->second.ItemCount[2];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount4"] = itr->second.ItemCount[3];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount5"] = itr->second.ItemCount[4];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount6"] = itr->second.ItemCount[5];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount7"] = itr->second.ItemCount[6];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount8"] = itr->second.ItemCount[7];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount9"] = itr->second.ItemCount[8];
            PlayerQuestStatusJson[quest_id.c_str()]["itemcount10"] = itr->second.ItemCount[9];
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "queststatus", jsonBuilder.write(PlayerQuestStatusJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestStatus player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "queststatus", jsonBuilder.write(AccountQuestStatusJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestStatus account guid %u", guid);
    });
}

void Player::SerializePlayerQuestRewarded()
{
    for (RewardedQuestSet::const_iterator itr = m_RewardedQuests.begin(); itr != m_RewardedQuests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
            AccountQuestRewardedJson[quest_id.c_str()] = quest_id;
        else
            PlayerQuestRewardedJson[quest_id.c_str()] = quest_id;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questrewarded", jsonBuilder.write(PlayerQuestRewardedJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestRewarded player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "questrewarded", jsonBuilder.write(AccountQuestRewardedJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestRewarded account guid %u", guid);
    });
}

void Player::SerializePlayerQuestDaily()
{
    for (QuestSet::const_iterator itr = m_dailyquests.begin(); itr != m_dailyquests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
            AccountQuestDailyJson[quest_id.c_str()] = uint64(m_lastDailyQuestTime);
        else
            PlayerQuestDailyJson[quest_id.c_str()] = uint64(m_lastDailyQuestTime);
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questdaily", jsonBuilder.write(PlayerQuestDailyJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestDaily player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "questdaily", jsonBuilder.write(AccountQuestDailyJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestDaily account guid %u", guid);
    });
}

void Player::SerializePlayerQuestWeekly()
{
    for (QuestSet::const_iterator itr = m_weeklyquests.begin(); itr != m_weeklyquests.end(); ++itr)
    {
        uint32 questId = *itr;
        std::string quest_id = std::to_string(questId);
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (quest->GetType() == QUEST_TYPE_ACCOUNT)
            AccountQuestWeeklyJson[quest_id.c_str()] = quest_id;
        else
            PlayerQuestWeeklyJson[quest_id.c_str()] = quest_id;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questweekly", jsonBuilder.write(PlayerQuestWeeklyJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestWeekly player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "questweekly", jsonBuilder.write(AccountQuestWeeklyJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestWeekly account guid %u", guid);
    });
}

void Player::SerializePlayerQuestSeasonal()
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
                AccountQuestSeasonalJson[quest_id.c_str()] = event_id;
            else
                PlayerQuestSeasonalJson[quest_id.c_str()] = event_id;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questseasonal", jsonBuilder.write(PlayerQuestSeasonalJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestSeasonal player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "questseasonal", jsonBuilder.write(AccountQuestSeasonalJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerQuestSeasonal account guid %u", guid);
    });
}

void Player::SerializePlayerBoundInstances()
{
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        //std::string diff = std::to_string(i);

        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if (itr->second.perm)
            {
                InstanceSave* save = itr->second.save;

                std::string data;
                uint32 challenge = 0;
                Map* map = sMapMgr->FindMap(save->GetMapId(), save->GetInstanceId());
                if (map && map->IsDungeon())
                {
                    if (InstanceScript* instanceScript = ((InstanceMap*)map)->GetInstanceScript())
                    {
                        data = instanceScript->GetSaveData();
                        challenge = instanceScript->GetChallengeProgresTime();
                    }
                }

                std::string instanceID = std::to_string(save->GetInstanceId());
                PlayerBoundInstancesJson[instanceID.c_str()]["perm"] = itr->second.perm;
                PlayerBoundInstancesJson[instanceID.c_str()]["map"] = save->GetMapId();
                PlayerBoundInstancesJson[instanceID.c_str()]["difficulty"] = save->GetDifficulty();
                PlayerBoundInstancesJson[instanceID.c_str()]["challenge"] = challenge;
                PlayerBoundInstancesJson[instanceID.c_str()]["data"] = data.c_str();
                PlayerBoundInstancesJson[instanceID.c_str()]["completedEncounters"] = save->GetCompletedEncounterMask();
            }
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "boundinstances", jsonBuilder.write(PlayerBoundInstancesJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerBoundInstances guid %u", guid);
    });
}

void Player::SerializePlayerBattlePets()
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
        AccountBattlePetsJson[petGuid.c_str()]["customName"] = pjInfo->GetCustomName();
        AccountBattlePetsJson[petGuid.c_str()]["creatureEntry"] = pjInfo->GetCreatureEntry();
        AccountBattlePetsJson[petGuid.c_str()]["speciesID"] = pjInfo->GetSpeciesID();
        AccountBattlePetsJson[petGuid.c_str()]["spell"] = pjInfo->GetSummonSpell();
        AccountBattlePetsJson[petGuid.c_str()]["level"] = pjInfo->GetLevel();
        AccountBattlePetsJson[petGuid.c_str()]["displayID"] = pjInfo->GetDisplayID();
        AccountBattlePetsJson[petGuid.c_str()]["power"] = pjInfo->GetPower();
        AccountBattlePetsJson[petGuid.c_str()]["speed"] = pjInfo->GetSpeed();
        AccountBattlePetsJson[petGuid.c_str()]["health"] = pjInfo->GetHealth();
        AccountBattlePetsJson[petGuid.c_str()]["maxHealth"] = pjInfo->GetMaxHealth();
        AccountBattlePetsJson[petGuid.c_str()]["quality"] = pjInfo->GetQuality();
        AccountBattlePetsJson[petGuid.c_str()]["xp"] = pjInfo->GetXP();
        AccountBattlePetsJson[petGuid.c_str()]["flags"] = pjInfo->GetFlags();
        AccountBattlePetsJson[petGuid.c_str()]["breedID"] = pjInfo->GetBreedID();
    }

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "battlepets", jsonBuilder.write(AccountBattlePetsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerBattlePets guid %u", guid);
    });
}

void Player::SerializePlayerBattlePetSlots()
{
    for (int i = 0; i < 3; ++i)
    {
        std::string index = std::to_string(i);
        PetBattleSlot* slot = GetBattlePetMgr()->GetPetBattleSlot(i);
        AccountBattlePetSlotsJson[index.c_str()] = slot ? slot->GetPet() : 0;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "battlepetslots", jsonBuilder.write(AccountBattlePetSlotsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerBattlePetSlots guid %u", guid);
    });
}

void Player::SerializePlayerArchaeology()
{
    if (!sWorld->getBoolConfig(CONFIG_ARCHAEOLOGY_ENABLED))
        return;

    if (!GetSkillValue(SKILL_ARCHAEOLOGY))
        return;

    std::stringstream ss;
    for (ResearchSiteSet::const_iterator itr = _researchSites.begin(); itr != _researchSites.end(); ++itr)
        ss << (*itr) << " ";

    PlayerArchaeologyJson["sites"] = ss.str().c_str();
    ss.str("");
    for (uint8 j = 0; j < MAX_RESEARCH_SITES; ++j)
        ss << uint32(_digSites[j].count) << " ";
    PlayerArchaeologyJson["counts"] = ss.str().c_str();

    ss.str("");
    for (uint32 i = 0; i < MAX_RESEARCH_PROJECTS; ++i)
        if (uint16 val = GetUInt16Value(PLAYER_FIELD_RESEARCHING_1 + i / 2, i % 2))
            ss << val << " ";
    PlayerArchaeologyJson["projects"] = ss.str().c_str();

    for (CompletedProjectList::iterator itr = _completedProjects.begin(); itr != _completedProjects.end(); ++itr)
    {
        std::string ID = std::to_string(itr->entry->ID);
        PlayerArchaeologyJson["finds"][ID.c_str()]["count"] = itr->count;
        PlayerArchaeologyJson["finds"][ID.c_str()]["date"] = itr->date;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "archaeology", jsonBuilder.write(PlayerArchaeologyJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerArchaeology guid %u", guid);
    });
}

void Player::SerializePlayerReputation()
{
    for (FactionStateList::const_iterator itr = m_reputationMgr.GetStateList().begin(); itr != m_reputationMgr.GetStateList().end(); ++itr)
    {
        std::string faction = std::to_string(itr->second.ID);
        PlayerReputationJson[faction.c_str()]["standing"] = itr->second.Standing;
        PlayerReputationJson[faction.c_str()]["flags"] = itr->second.Flags;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "reputation", jsonBuilder.write(PlayerReputationJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerReputation player guid %u", guid);
    });
}

void Item::SerializeItem()
{
    if (!RedisDatabase.isConnected())
        return;

    ItemsJson["state"] = "normal";
    ItemsJson["itemGuid"] = GetGUIDLow();
    ItemsJson["slot"] = m_slot;
    ItemsJson["bagGuid"] = m_container ? m_container->GetGUIDLow() : 0;
    ItemsJson["owner_guid"] = GetOwnerGUID();
    ItemsJson["itemEntry"] = GetEntry();
    ItemsJson["creatorGuid"] = GetUInt64Value(ITEM_FIELD_CREATOR);
    ItemsJson["giftCreatorGuid"] = GetUInt64Value(ITEM_FIELD_GIFTCREATOR);
    ItemsJson["count"] = GetCount();
    ItemsJson["duration"] = GetUInt32Value(ITEM_FIELD_DURATION);
    std::ostringstream ssSpells;
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        ssSpells << GetSpellCharges(i) << ' ';
    ItemsJson["charges"] = ssSpells.str().c_str();
    ItemsJson["flags"] = GetUInt32Value(ITEM_FIELD_FLAGS);
    std::ostringstream ssEnchants;
    for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
    {
        ssEnchants << GetEnchantmentId(EnchantmentSlot(i)) << ' ';
        ssEnchants << GetEnchantmentDuration(EnchantmentSlot(i)) << ' ';
        ssEnchants << GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
    }
    ItemsJson["enchantments"] = ssEnchants.str().c_str();
    ItemsJson["randomPropertyId"] = GetItemRandomPropertyId();
    ItemsJson["reforgeId"] = GetReforge();
    ItemsJson["transmogrifyId"] = GetTransmogrification();
    ItemsJson["upgradeId"] = GetUpgradeId();
    ItemsJson["durability"] = GetUInt32Value(ITEM_FIELD_DURABILITY);
    ItemsJson["playedTime"] = GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME);
    ItemsJson["text"] = GetText().c_str();
    ItemsJson["uState"] = GetState();
    ItemsJson["paidMoney"] = m_paidMoney;
    ItemsJson["paidExtendedCost"] = m_paidExtendedCost;
    ItemsJson["paidGuid"] = m_refundRecipient;
    std::ostringstream ss;
    AllowedLooterSet::const_iterator itr = allowedGUIDs.begin();
    ss << *itr;
    for (++itr; itr != allowedGUIDs.end(); ++itr)
        ss << ' ' << *itr;
    ItemsJson["allowedGUIDs"] = ss.str().c_str();

    std::string index = std::to_string(GetGUIDLow());

    RedisDatabase.AsyncExecuteHSet("HSET", itemKey, index.c_str(), jsonBuilder.write(ItemsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Item::SerializeItem guid %u", guid);
    });
}

void Player::SerializePlayerVoidStorage()
{
    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
    {
        if (_voidStorageItems[i]) // unused item
        {
            if(_voidStorageItems[i]->deleted)
            {
                delete _voidStorageItems[i];
                _voidStorageItems[i] = NULL;
                continue;
            }

            std::string index = std::to_string(i);
            PlayerVoidStorageJson[index.c_str()]["itemId"] = _voidStorageItems[i]->ItemId;
            PlayerVoidStorageJson[index.c_str()]["itemEntry"] = _voidStorageItems[i]->ItemEntry;
            PlayerVoidStorageJson[index.c_str()]["creatorGuid"] = _voidStorageItems[i]->CreatorGuid;
            PlayerVoidStorageJson[index.c_str()]["randomProperty"] = _voidStorageItems[i]->ItemRandomPropertyId;
            PlayerVoidStorageJson[index.c_str()]["suffixFactor"] = _voidStorageItems[i]->ItemSuffixFactor;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "voidstorage", jsonBuilder.write(PlayerVoidStorageJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerVoidStorage player guid %u", guid);
    });
}

void Player::SerializePlayerActions()
{
    for (ActionButtonList::iterator itr = m_actionButtons.begin(); itr != m_actionButtons.end(); ++itr)
    {
        std::string index = std::to_string(itr->first);
        PlayerActionsJson[index.c_str()]["spec"] = GetActiveSpec();
        PlayerActionsJson[index.c_str()]["action"] = itr->second.GetAction();
        PlayerActionsJson[index.c_str()]["type"] = itr->second.GetType();
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "actions", jsonBuilder.write(PlayerActionsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerActions player guid %u", guid);
    });
}

void Player::SerializePlayerSocial()
{
    PlayerSocial* social = sSocialMgr->GetPlayerSocial(GetGUIDLow());
    for (PlayerSocialMap::const_iterator itr = social->m_playerSocialMap.begin(); itr != social->m_playerSocialMap.end(); ++itr)
    {
        std::string friend_guid = std::to_string(itr->first);
        PlayerSocialJson[friend_guid.c_str()]["Flags"] = itr->second.Flags;
        PlayerSocialJson[friend_guid.c_str()]["Note"] = itr->second.Note.c_str();
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "social", jsonBuilder.write(PlayerSocialJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerSocial player guid %u", guid);
    });
}

void Player::SerializePlayerSpellCooldowns()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    // remove outdated and save active
    for (SpellCooldowns::iterator itr = m_spellCooldowns.begin(); itr != m_spellCooldowns.end();)
    {
        if (itr->second.end <= curTime)
            m_spellCooldowns.erase(itr++);
        else if (itr->second.end <= infTime)                 // not save locked cooldowns, it will be reset or set at reload
        {
            std::string spell = std::to_string(itr->first);
            PlayerSpellCooldownsJson[spell.c_str()]["item"] = itr->second.itemid;
            PlayerSpellCooldownsJson[spell.c_str()]["time"] = uint64(itr->second.end);
            ++itr;
        }
        else
            ++itr;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "spellcooldowns", jsonBuilder.write(PlayerSpellCooldownsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerSpellCooldowns player guid %u", guid);
    });
}

void Player::SerializePlayerKills()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    // remove outdated and save active
    for(KillInfoMap::iterator itr = m_killsPerPlayer.begin(); itr != m_killsPerPlayer.end(); ++itr)
    {
        std::string victim = std::to_string(itr->first);
        PlayerKillsJson[victim.c_str()] = itr->second.count;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "kills", jsonBuilder.write(PlayerKillsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerKills player guid %u", guid);
    });
}

void Player::SerializePlayerDeclinedName()
{
    if (m_declinedname)
    {
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            std::string index = std::to_string(i);
            PlayerDeclinedNameJson[index.c_str()] = m_declinedname->name[i].c_str();
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "declinedname", jsonBuilder.write(PlayerDeclinedNameJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerDeclinedName player guid %u", guid);
    });
}

void Player::SerializePlayerEquipmentSets()
{
    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end();++itr)
    {
        std::string index = std::to_string(itr->first);
        EquipmentSet& eqset = itr->second;
        PlayerEquipmentSetsJson[index.c_str()]["setguid"] = eqset.Guid;
        PlayerEquipmentSetsJson[index.c_str()]["name"] = eqset.Name.c_str();
        PlayerEquipmentSetsJson[index.c_str()]["iconname"] = eqset.IconName.c_str();
        PlayerEquipmentSetsJson[index.c_str()]["ignore_mask"] = eqset.IgnoreMask;
        PlayerEquipmentSetsJson[index.c_str()]["item0"] = eqset.Items[0];
        PlayerEquipmentSetsJson[index.c_str()]["item1"] = eqset.Items[1];
        PlayerEquipmentSetsJson[index.c_str()]["item2"] = eqset.Items[2];
        PlayerEquipmentSetsJson[index.c_str()]["item3"] = eqset.Items[3];
        PlayerEquipmentSetsJson[index.c_str()]["item4"] = eqset.Items[4];
        PlayerEquipmentSetsJson[index.c_str()]["item5"] = eqset.Items[5];
        PlayerEquipmentSetsJson[index.c_str()]["item6"] = eqset.Items[6];
        PlayerEquipmentSetsJson[index.c_str()]["item7"] = eqset.Items[7];
        PlayerEquipmentSetsJson[index.c_str()]["item8"] = eqset.Items[8];
        PlayerEquipmentSetsJson[index.c_str()]["item9"] = eqset.Items[9];
        PlayerEquipmentSetsJson[index.c_str()]["item10"] = eqset.Items[10];
        PlayerEquipmentSetsJson[index.c_str()]["item11"] = eqset.Items[11];
        PlayerEquipmentSetsJson[index.c_str()]["item12"] = eqset.Items[12];
        PlayerEquipmentSetsJson[index.c_str()]["item13"] = eqset.Items[13];
        PlayerEquipmentSetsJson[index.c_str()]["item14"] = eqset.Items[14];
        PlayerEquipmentSetsJson[index.c_str()]["item15"] = eqset.Items[15];
        PlayerEquipmentSetsJson[index.c_str()]["item16"] = eqset.Items[16];
        PlayerEquipmentSetsJson[index.c_str()]["item17"] = eqset.Items[17];
        PlayerEquipmentSetsJson[index.c_str()]["item18"] = eqset.Items[18];
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "equipmentsets", jsonBuilder.write(PlayerEquipmentSetsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerEquipmentSets player guid %u", guid);
    });
}

void Player::SerializePlayerCUFProfiles()
{
    for (uint8 i = 0; i < MAX_CUF_PROFILES; ++i)
    {
        if (_CUFProfiles[i])
        {
            std::string index = std::to_string(i);
            PlayerCUFProfilesJson[index.c_str()]["profileName"] = _CUFProfiles[i]->profileName;
            PlayerCUFProfilesJson[index.c_str()]["frameHeight"] = _CUFProfiles[i]->frameHeight;
            PlayerCUFProfilesJson[index.c_str()]["frameWidth"] = _CUFProfiles[i]->frameWidth;
            PlayerCUFProfilesJson[index.c_str()]["sortBy"] = _CUFProfiles[i]->sortBy;
            PlayerCUFProfilesJson[index.c_str()]["showHealthText"] = _CUFProfiles[i]->showHealthText;
            PlayerCUFProfilesJson[index.c_str()]["options"] = _CUFProfiles[i]->options;
            PlayerCUFProfilesJson[index.c_str()]["Unk146"] = _CUFProfiles[i]->Unk146;
            PlayerCUFProfilesJson[index.c_str()]["Unk147"] = _CUFProfiles[i]->Unk147;
            PlayerCUFProfilesJson[index.c_str()]["Unk148"] = _CUFProfiles[i]->Unk148;
            PlayerCUFProfilesJson[index.c_str()]["Unk150"] = _CUFProfiles[i]->Unk150;
            PlayerCUFProfilesJson[index.c_str()]["Unk152"] = _CUFProfiles[i]->Unk152;
            PlayerCUFProfilesJson[index.c_str()]["Unk154"] = _CUFProfiles[i]->Unk154;
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "cufprofiles", jsonBuilder.write(PlayerCUFProfilesJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerCUFProfiles player guid %u", guid);
    });
}

void Player::SerializePlayerVisuals()
{
    if (m_vis)
    {
        PlayerVisualsJson["head"] = m_vis->m_visHead;
        PlayerVisualsJson["shoulders"] = m_vis->m_visShoulders;
        PlayerVisualsJson["chest"] = m_vis->m_visChest;
        PlayerVisualsJson["waist"] = m_vis->m_visWaist;
        PlayerVisualsJson["legs"] = m_vis->m_visLegs;
        PlayerVisualsJson["feet"] = m_vis->m_visFeet;
        PlayerVisualsJson["wrists"] = m_vis->m_visWrists;
        PlayerVisualsJson["hands"] = m_vis->m_visHands;
        PlayerVisualsJson["back"] = m_vis->m_visBack;
        PlayerVisualsJson["main"] = m_vis->m_visMainhand;
        PlayerVisualsJson["off"] = m_vis->m_visOffhand;
        PlayerVisualsJson["ranged"] = m_vis->m_visRanged;
        PlayerVisualsJson["tabard"] = m_vis->m_visTabard;
        PlayerVisualsJson["shirt"] = m_vis->m_visShirt;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "visuals", jsonBuilder.write(PlayerVisualsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerVisuals player guid %u", guid);
    });
}

void Player::SerializePlayerAccountData()
{
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
    {
        if (PER_CHARACTER_CACHE_MASK & (1 << i))
        {
            std::string index = std::to_string(i);
            PlayerAccountDataJson[index.c_str()]["Time"] = GetSession()->m_accountData[i].Time;
            PlayerAccountDataJson[index.c_str()]["Data"] = GetSession()->m_accountData[i].Data.c_str();
        }
    }

    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
    {
        if (GLOBAL_CACHE_MASK & (1 << i))
        {
            std::string index = std::to_string(i);
            AccountDataJson[index.c_str()]["Time"] = GetSession()->m_accountData[i].Time;
            AccountDataJson[index.c_str()]["Data"] = GetSession()->m_accountData[i].Data.c_str();
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "playeraccountdata", jsonBuilder.write(PlayerAccountDataJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerAccountData player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "accountdata", jsonBuilder.write(AccountDataJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerAccountData player guid %u", guid);
    });
}

void Player::SerializePlayerHomeBind()
{
    PlayerHomeBindJson["mapId"] = m_homebindMapId;
    PlayerHomeBindJson["zoneId"] = m_homebindAreaId;
    PlayerHomeBindJson["posX"] = m_homebindX;
    PlayerHomeBindJson["posY"] = m_homebindY;
    PlayerHomeBindJson["posZ"] = m_homebindZ;

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "homebind", jsonBuilder.write(PlayerHomeBindJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerHomeBind player guid %u", guid);
    });
}

void Player::SerializePlayerAchievement()
{
    for (auto iter = GetAchievementMgr().GetCompletedAchievementsList().begin(); iter != GetAchievementMgr().GetCompletedAchievementsList().end(); ++iter)
    {
        std::string achievementId = std::to_string(iter->first);
        PlayerAchievementJson[achievementId.c_str()] = iter->second.date;

        AccountAchievementJson[achievementId.c_str()]["first_guid"] = iter->second.first_guid;
        AccountAchievementJson[achievementId.c_str()]["date"] = iter->second.date;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "achievement", jsonBuilder.write(PlayerAchievementJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerAchievement player guid %u", guid);
    });

    RedisDatabase.AsyncExecuteHSet("HSET", accountKey, "achievement", jsonBuilder.write(AccountAchievementJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerAchievement account guid %u", guid);
    });
}

void Player::SerializePlayerCriteria()
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
            uint64 guid      = GetGUIDLow();

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
                        save_ac = true;
                        CriteriaAc[criteriaId.c_str()]["counter"] = iter->second.counter;
                        CriteriaAc[criteriaId.c_str()]["date"] = iter->second.date;
                        CriteriaAc[criteriaId.c_str()]["completed"] = iter->second.completed;
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
            PlayerCriteriaJson[achievID.c_str()] = CriteriaPl;
            RedisDatabase.AsyncExecuteHSet("HSET", criteriaPlKey, achievID.c_str(), jsonBuilder.write(CriteriaPl).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerCriteria player guid %u", guid);
            });
        }

        if (save_ac)
        {
            AccountCriteriaJson[achievID.c_str()] = CriteriaAc;
            RedisDatabase.AsyncExecuteHSet("HSET", criteriaAcKey, achievID.c_str(), jsonBuilder.write(CriteriaAc).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerCriteria account guid %u", guid);
            });
        }
    }
}
