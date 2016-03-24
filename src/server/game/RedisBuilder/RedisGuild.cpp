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

#include "Guild.h"
#include "GuildMgr.h"
#include "GuildFinderMgr.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Config.h"
#include "SocialMgr.h"
#include "Log.h"
#include "AccountMgr.h"
#include "RedisBuilderMgr.h"

void Guild::SaveGuild()
{
    GuildData["m_leaderGuid"] = GUID_LOPART(m_leaderGuid);
    GuildData["m_name"] = m_name;
    GuildData["m_info"] = m_info;
    GuildData["m_motd"] = m_motd;
    GuildData["m_createdDate"] = m_createdDate;
    GuildData["_level"] = _level;
    GuildData["_experience"] = _experience;
    GuildData["_todayExperience"] = _todayExperience;
    GuildData["purchasedTabs"] = m_bankTabs.size();
    GuildData["m_style"] = m_emblemInfo.GetStyle();
    GuildData["m_color"] = m_emblemInfo.GetColor();
    GuildData["m_borderStyle"] = m_emblemInfo.GetBorderStyle();
    GuildData["m_borderColor"] = m_emblemInfo.GetBorderColor();
    GuildData["m_backgroundColor"] = m_emblemInfo.GetBackgroundColor();

    RefreshGuildRank();
    SaveGuildBankTab();
    SaveGuildMoney();

    std::string index = std::to_string(m_id);
    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetGuildKey(), index.c_str(), GuildData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuild guid %u", guid);
    });
}

void Guild::Member::SaveGuildMember()
{
    GuildMemberData["m_publicNote"] = m_publicNote;
    GuildMemberData["m_officerNote"] = m_officerNote;
    GuildMemberData["m_rankId"] = m_rankId;
    GuildMemberData["m_flags"] = m_flags;
    GuildMemberData["BankResetTimeMoney"] = m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime;
    GuildMemberData["BankRemMoney"] = m_bankRemaining[GUILD_BANK_MAX_TABS].value;
    GuildMemberData["BankResetTimeTab0"] = m_bankRemaining[0].resetTime;
    GuildMemberData["BankRemSlotsTab0"] = m_bankRemaining[0].value;
    GuildMemberData["BankResetTimeTab1"] = m_bankRemaining[1].resetTime;
    GuildMemberData["BankRemSlotsTab1"] = m_bankRemaining[1].value;
    GuildMemberData["BankResetTimeTab2"] = m_bankRemaining[2].resetTime;
    GuildMemberData["BankRemSlotsTab2"] = m_bankRemaining[2].value;
    GuildMemberData["BankResetTimeTab3"] = m_bankRemaining[3].resetTime;
    GuildMemberData["BankRemSlotsTab3"] = m_bankRemaining[3].value;
    GuildMemberData["BankResetTimeTab4"] = m_bankRemaining[4].resetTime;
    GuildMemberData["BankRemSlotsTab4"] = m_bankRemaining[4].value;
    GuildMemberData["BankResetTimeTab5"] = m_bankRemaining[5].resetTime;
    GuildMemberData["BankRemSlotsTab5"] = m_bankRemaining[5].value;
    GuildMemberData["BankResetTimeTab6"] = m_bankRemaining[6].resetTime;
    GuildMemberData["BankRemSlotsTab6"] = m_bankRemaining[6].value;
    GuildMemberData["BankResetTimeTab7"] = m_bankRemaining[7].resetTime;
    GuildMemberData["BankRemSlotsTab7"] = m_bankRemaining[7].value;
    GuildMemberData["m_name"] = m_name;
    GuildMemberData["m_level"] = m_level;
    GuildMemberData["m_class"] = m_class;
    GuildMemberData["m_zoneId"] = m_zoneId;
    GuildMemberData["m_gender"] = m_gender;
    GuildMemberData["m_accountId"] = m_accountId;
    GuildMemberData["m_achievementPoints"] = m_achievementPoints;
    GuildMemberData["profId1"] = m_professionInfo[0].skillId;
    GuildMemberData["profValue1"] = m_professionInfo[0].skillValue;
    GuildMemberData["profRank1"] = m_professionInfo[0].skillRank;
    GuildMemberData["recipesMask1"] = m_professionInfo[0].knownRecipes.GetMaskForSave();
    GuildMemberData["profId2"] = m_professionInfo[1].skillId;
    GuildMemberData["profValue2"] = m_professionInfo[1].skillValue;
    GuildMemberData["profRank2"] = m_professionInfo[1].skillRank;
    GuildMemberData["recipesMask2"] = m_professionInfo[1].knownRecipes.GetMaskForSave();
    GuildMemberData["m_logoutTime"] = m_logoutTime;
    GuildMemberData["m_totalActivity"] = m_totalActivity;
    GuildMemberData["m_weekActivity"] = m_weekActivity;
    GuildMemberData["m_weekReputation"] = m_weekReputation;
    GuildMemberData["m_totalReputation"] = m_totalReputation;

    std::string index = std::to_string(GUID_LOPART(m_guid));

    RedisDatabase.AsyncExecuteHSet("HSET", m_guild->GetGuildMemberKey(), index.c_str(), GuildMemberData, m_guid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildMember guid %u", guid);
    });
}

void Guild::SaveGuildEventLog()
{
    for (LogHolder::GuildLog::const_iterator itr = m_eventLog->GetLog()->begin(); itr != m_eventLog->GetLog()->end(); ++itr)
    {
        EventLogEntry* log = (EventLogEntry*)(*itr);
        std::string guid = std::to_string((*itr)->m_guid);
        GuildEventLogData[guid.c_str()]["m_timestamp"] = (*itr)->m_timestamp;
        GuildEventLogData[guid.c_str()]["m_eventType"] = log->m_eventType;
        GuildEventLogData[guid.c_str()]["m_playerGuid1"] = log->m_playerGuid1;
        GuildEventLogData[guid.c_str()]["m_playerGuid2"] = log->m_playerGuid2;
        GuildEventLogData[guid.c_str()]["m_newRank"] = log->m_newRank;
    }

    std::string index = std::to_string(m_id);

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "eventlog", GuildEventLogData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildEventLog guid %u", guid);
    });
}

void Guild::SaveGuildBankEventLog()
{
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
    {
        std::string tab = std::to_string(tabId);
        for (LogHolder::GuildLog::const_iterator itr = m_bankEventLog[tabId]->GetLog()->begin(); itr != m_bankEventLog[tabId]->GetLog()->end(); ++itr)
        {
            if (tabId < GetPurchasedTabsSize() || tabId == GUILD_BANK_MAX_TABS)
            {
                BankEventLogEntry* log = (BankEventLogEntry*)(*itr);
                std::string guid = std::to_string((*itr)->m_guid);
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_timestamp"] = (*itr)->m_timestamp;
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_eventType"] = log->m_eventType;
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_bankTabId"] = log->m_bankTabId;
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_playerGuid"] = log->m_playerGuid;
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_itemOrMoney"] = log->m_itemOrMoney;
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_itemStackCount"] = log->m_itemStackCount;
                GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_destTabId"] = log->m_destTabId;
            }
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "bankeventlog", GuildBankEventLogData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildBankEventLog guid %u", guid);
    });
}

void Guild::UpdateGuildEventLog(LogEntry* entry)
{
    EventLogEntry* log = (EventLogEntry*)(entry);
    std::string guid = std::to_string(entry->m_guid);
    GuildEventLogData[guid.c_str()]["m_timestamp"] = entry->m_timestamp;
    GuildEventLogData[guid.c_str()]["m_eventType"] = log->m_eventType;
    GuildEventLogData[guid.c_str()]["m_playerGuid1"] = log->m_playerGuid1;
    GuildEventLogData[guid.c_str()]["m_playerGuid2"] = log->m_playerGuid2;
    GuildEventLogData[guid.c_str()]["m_newRank"] = log->m_newRank;

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "eventlog", GuildBankEventLogData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildEventLog guid %u", guid);
    });
}

void Guild::UpdateGuildBankEventLog(LogEntry* entry)
{
    BankEventLogEntry* log = (BankEventLogEntry*)(entry);
    std::string tab = std::to_string(log->m_bankTabId);
    std::string guid = std::to_string(entry->m_guid);
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_timestamp"] = entry->m_timestamp;
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_eventType"] = log->m_eventType;
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_bankTabId"] = log->m_bankTabId;
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_playerGuid"] = log->m_playerGuid;
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_itemOrMoney"] = log->m_itemOrMoney;
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_itemStackCount"] = log->m_itemStackCount;
    GuildBankEventLogData[tab.c_str()][guid.c_str()]["m_destTabId"] = log->m_destTabId;

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "bankeventlog", GuildBankEventLogData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildBankEventLog guid %u", guid);
    });
}

void Guild::UpdateGuildEmblem()
{
    GuildData["m_style"] = m_emblemInfo.GetStyle();
    GuildData["m_color"] = m_emblemInfo.GetColor();
    GuildData["m_borderStyle"] = m_emblemInfo.GetBorderStyle();
    GuildData["m_borderColor"] = m_emblemInfo.GetBorderColor();
    GuildData["m_backgroundColor"] = m_emblemInfo.GetBackgroundColor();

    std::string index = std::to_string(m_id);

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "data", GuildData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildEmblem guid %u", guid);
    });
}

void Guild::UpdateGuildRank(RankInfo* info)
{
    std::string id = std::to_string(info->GetId());
    GuildRankData[id.c_str()]["m_name"] = info->GetName();
    GuildRankData[id.c_str()]["m_rights"] = info->GetRights();
    GuildRankData[id.c_str()]["m_bankMoneyPerDay"] = info->GetBankMoneyPerDay();
    for (uint8 j = 0; j < GetPurchasedTabsSize(); ++j)
    {
        std::string tab = std::to_string(j);
        GuildRankData[id.c_str()]["bank"][tab.c_str()]["rights"] = info->GetBankTabRights(j);
        GuildRankData[id.c_str()]["bank"][tab.c_str()]["slots"] = info->GetBankTabSlotsPerDay(j);
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "rank", GuildRankData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildRank guid %u", guid);
    });
}

void Guild::RefreshGuildRank()
{
    if (!GuildRankData.empty())
        GuildRankData.clear();

    for (uint32 i = 0; i < m_ranks.size(); ++i)
    {
        std::string id = std::to_string(m_ranks[i].GetId());
        GuildRankData[id.c_str()]["m_name"] = m_ranks[i].GetName();
        GuildRankData[id.c_str()]["m_rights"] = m_ranks[i].GetRights();
        GuildRankData[id.c_str()]["m_bankMoneyPerDay"] = m_ranks[i].GetBankMoneyPerDay();
        for (uint8 j = 0; j < GetPurchasedTabsSize(); ++j)
        {
            std::string tab = std::to_string(j);
            GuildRankData[id.c_str()]["bank"][tab.c_str()]["rights"] = m_ranks[i].GetBankTabRights(j);
            GuildRankData[id.c_str()]["bank"][tab.c_str()]["slots"] = m_ranks[i].GetBankTabSlotsPerDay(j);
        }
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "rank", GuildRankData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::RefreshGuildRank guid %u", guid);
    });
}

void Guild::SaveGuildBankTab()
{
    if (!GuildTabData.empty())
        GuildTabData.clear();

    for (uint8 i = 0; i < GetPurchasedTabsSize(); ++i)
    {
        std::string id = std::to_string(i);
        GuildTabData[id.c_str()]["m_name"] = m_bankTabs[i]->GetName();
        GuildTabData[id.c_str()]["m_icon"] = m_bankTabs[i]->GetIcon();
        GuildTabData[id.c_str()]["m_text"] = m_bankTabs[i]->GetText();
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "banktab", GuildTabData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildBankTab guid %u", guid);
    });
}

void Guild::UpdateGuildBankTab(BankTab* pTab)
{
    std::string id = std::to_string(pTab->GetTabId());
    GuildTabData[id.c_str()]["m_name"] = pTab->GetName();
    GuildTabData[id.c_str()]["m_icon"] = pTab->GetIcon();
    GuildTabData[id.c_str()]["m_text"] = pTab->GetText();

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "banktab", GuildTabData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildBankTab guid %u", guid);
    });
}

void Guild::SaveGuildMoney()
{
    GuildMoney = m_bankMoney;

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "money", GuildMoney, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildMoney guid %u", guid);
    });
}

void Guild::DeleteMemberGuild(uint32 lowguid)
{
    std::string userKey = "r{" + std::to_string(realmID) + "}u{" + std::to_string(lowguid) + "}";
    RedisDatabase.AsyncExecuteH("HDEL", userKey.c_str(), "guild", lowguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteMemberGuild guid %u", guid);
    });
}

void Guild::DeleteMembers()
{
    std::string _key = "r{" + std::to_string(realmID) + "}g{" + std::to_string(m_id) + "}member";
    RedisDatabase.AsyncExecute("DEL", _key.c_str(), m_id, [&](const RedisValue &v, uint64 guid) {});
}

void Guild::SaveGuildNewsLog()
{
    for (GuildNewsLogMap::const_iterator it = _newsLog.GetNewsLog()->begin(); it != _newsLog.GetNewsLog()->end(); ++it)
    {
        std::string id = std::to_string(it->first);
        GuildNewsLogData[id.c_str()]["EventType"] = it->second.EventType;
        GuildNewsLogData[id.c_str()]["PlayerGuid"] = it->second.PlayerGuid;
        GuildNewsLogData[id.c_str()]["Data"] = it->second.Data;
        GuildNewsLogData[id.c_str()]["Flags"] = it->second.Flags;
        GuildNewsLogData[id.c_str()]["Date"] = it->second.Date;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "newslog", GuildNewsLogData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildNewsLog guid %u", guid);
    });
}

void Guild::UpdateGuildNewsLog(GuildNewsEntry* log, uint32 id)
{
    std::string index = std::to_string(id);
    GuildNewsLogData[index.c_str()]["EventType"] = log->EventType;
    GuildNewsLogData[index.c_str()]["PlayerGuid"] = log->PlayerGuid;
    GuildNewsLogData[index.c_str()]["Data"] = log->Data;
    GuildNewsLogData[index.c_str()]["Flags"] = log->Flags;
    GuildNewsLogData[index.c_str()]["Date"] = log->Date;

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "newslog", GuildNewsLogData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildNewsLog guid %u", guid);
    });
}

void GuildMgr::LoadGuildsRedis()
{
    // 1. Load all guilds
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading guilds...");
    {
        uint32 oldMSTime = getMSTime();

        RedisValue guildV = RedisDatabase.Execute("HGETALL", sRedisBuilderMgr->GetGuildKey());

        std::vector<RedisValue> guildVector;
        if (!sRedisBuilderMgr->LoadFromRedisArray(&guildV, guildVector))
        {
            sLog->outInfo(LOG_FILTER_REDIS, "GuildMgr::LoadGuildsRedis guild not found");
            return;
        }

        uint32 count = 0;
        for (auto itr = guildVector.begin(); itr != guildVector.end();)
        {
            std::string id = itr->toString();

            uint32 guildId = atoi(itr->toString().c_str());
            ++itr;
            if (itr->isInt())
            {
                ++itr;
                continue;
            }

            Json::Value guildDB;
            if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), guildDB))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "GuildMgr::LoadGuildsRedis not parse guildId %i", guildId);
                continue;
            }
            else
                ++itr;

            Guild* guild = new Guild();

            if (!guild->LoadFromDB(guildId, guildDB))
            {
                delete guild;
                continue;
            }
            AddGuild(guild);

            ++count;
        }

        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u guild definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    // 2. Update Guild Known Recipes
    for (GuildContainer::iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
    {
        Guild* guild = itr->second;
        if (!guild)
            continue;

        guild->UpdateGuildRecipes();
    }

    // 3. Validate loaded guild data
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Validating data of loaded guilds...");
    {
        uint32 oldMSTime = getMSTime();

        for (GuildContainer::iterator itr = GuildStore.begin(); itr != GuildStore.end();)
        {
            Guild* guild = itr->second;
            if (guild)
            {
                guild->Validate();
                /*if (!guild->Validate())
                {
                    volatile uint32 _guildId = guild->GetId();
                    GuildStore.erase(itr++);
                    delete guild;
                }
                else*/
                    ++itr;
            }
            else
                ++itr;
        }

        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Validated data of loaded guilds in %u ms", GetMSTimeDiffToNow(oldMSTime));
    }
}

bool Guild::LoadFromDB(uint32 guildId, Json::Value guildData)
{
    m_id            = guildId;
    m_name          = guildData["m_name"].asString();
    m_leaderGuid    = MAKE_NEW_GUID(guildData["m_leaderGuid"].asUInt(), 0, HIGHGUID_PLAYER);
    m_emblemInfo.LoadFromDB(guildData);
    m_info          = guildData["m_info"].asString();
    m_motd          = guildData["m_motd"].asString();
    m_createdDate   = time_t(guildData["m_createdDate"].asUInt());

    _level          = guildData["_level"].asUInt();
    _experience     = guildData["_experience"].asUInt64();
    _todayExperience = guildData["_todayExperience"].asUInt64();

    uint8 purchasedTabs = guildData["purchasedTabs"].asUInt();
    if (purchasedTabs > GUILD_BANK_MAX_TABS)
        purchasedTabs = GUILD_BANK_MAX_TABS;

    m_bankTabs.resize(purchasedTabs);
    for (uint8 i = 0; i < purchasedTabs; ++i)
        m_bankTabs[i] = new BankTab(m_id, i);

    _CreateLogHolders();
    CreateKey();

    //Load Guild data
    uint32 oldMSTime = getMSTime();
    RedisValue g = RedisDatabase.Execute("HGETALL", guildKey);
    std::vector<RedisValue> data;
    if (sRedisBuilderMgr->LoadFromRedisArray(&g, data))
    {
        for (auto itr = data.begin(); itr != data.end();)
        {
            std::string index = itr->toString();
            ++itr;

            Json::Value guildField;
            if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), guildField))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "Guild::LoadFromDB not parse guildId %i", guildId);
                continue;
            }
            else
                ++itr;

            if (!strcmp(index.c_str(), "money"))
                m_bankMoney = guildField.asUInt64();
            else if (!strcmp(index.c_str(), "rank"))
                    LoadRankFromDB(guildField);
            else if (!strcmp(index.c_str(), "eventlog"))
                    LoadEventLogFromDB(guildField);
            else if (!strcmp(index.c_str(), "bankeventlog"))
                    LoadBankEventLogFromDB(guildField);
            else if (!strcmp(index.c_str(), "banktab"))
                    LoadBankTabFromDB(guildField);
            else if (!strcmp(index.c_str(), "newslog"))
                    GetNewsLog().LoadFromDB(guildField);
            else if (!strcmp(index.c_str(), "achievement"))
                    LoadAchievement(guildField);
        }
    }
    else
        return false;

    //Load Member data
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading guild members...");
    uint32 count = 0;
    RedisValue m = RedisDatabase.Execute("HGETALL", GetGuildMemberKey());
    std::vector<RedisValue> memberVector;
    if (sRedisBuilderMgr->LoadFromRedisArray(&m, memberVector))
    {
        for (auto itr = memberVector.begin(); itr != memberVector.end();)
        {
            uint32 memberId = atoi(itr->toString().c_str());
            ++itr;

            Json::Value memberData;
            if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), memberData))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "Guild::LoadFromDB not parse memberId %i", memberId);
                continue;
            }
            else
                ++itr;

            LoadMemberFromDB(memberId, memberData);
            ++count;
        }
    }
    else
        return false;

    //Load Criteria data
    RedisValue cri = RedisDatabase.Execute("HGETALL", GetCriteriaKey());
    std::vector<RedisValue> criteriaVector;
    if (sRedisBuilderMgr->LoadFromRedisArray(&cri, criteriaVector))
    {
        for (auto itr = criteriaVector.begin(); itr != criteriaVector.end();)
        {
            uint32 achievementID = atoi(itr->toString().c_str());
            std::string achievID = std::to_string(achievementID);
            ++itr;

            if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), GuildCriteriaData[achievID.c_str()]))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "Guild::LoadFromDB not parse achievementID %i", achievementID);
                continue;
            }
            else
                ++itr;

            for (auto iter = GuildCriteriaData[achievID.c_str()].begin(); iter != GuildCriteriaData[achievID.c_str()].end(); ++iter)
            {
                uint32 char_criteria_id = atoi(iter.memberName());
                auto dataValue = *iter;


                time_t date    = time_t(dataValue["date"].asUInt());
                uint32 counter = dataValue["counter"].asUInt();
                bool completed = dataValue["completed"].asBool();
                uint32 guid = dataValue["CompletedGUID"].asUInt();

                m_achievementMgr.AddGuildCriteriaProgress(achievementID, char_criteria_id, date, counter, completed, guid);
            }
        }

        m_achievementMgr.GenerateProgressMap();
    }

    //Load items
    RedisValue item = RedisDatabase.Execute("HGETALL", GetItemKey());
    std::vector<RedisValue> itemVector;
    if (sRedisBuilderMgr->LoadFromRedisArray(&item, itemVector))
    {
        for (auto itr = itemVector.begin(); itr != itemVector.end();)
        {
            uint32 itemGuid = atoi(itr->toString().c_str());
            ++itr;

            Json::Value itemData;
            if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), itemData))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "Guild::LoadFromDB not parse itemGuid %i", itemGuid);
                continue;
            }
            else
                ++itr;

            uint8 tabId = itemData["m_tabId"].asUInt();
            if (tabId >= GetPurchasedTabsSize())
            {
                sLog->outError(LOG_FILTER_GUILD, "Invalid tab for item (GUID: %u, id: #%u) in guild bank, skipped.",
                    itemGuid, itemData["itemEntry"].asUInt());
                continue;
            }
            m_bankTabs[tabId]->LoadItemFromDB(itemData);
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded  guild data init %u ms, members %u", GetMSTimeDiffToNow(oldMSTime), count);

    return true;
}

void Guild::LoadRankFromDB(Json::Value rankData)
{
    for (auto itr = rankData.begin(); itr != rankData.end(); ++itr)
    {
        uint32 rankId = atoi(itr.memberName());
        auto rankValue = (*itr);

        RankInfo rankInfo(m_id);
        rankInfo.LoadFromDB(rankId, rankValue);
        m_ranks.push_back(rankInfo);
        for (auto iter = rankValue["bank"].begin(); iter != rankValue["bank"].end(); ++iter)
        {
            uint32 tabIdId = atoi(iter.memberName());
            auto bankValue = (*iter);

            GuildBankRightsAndSlots rightsAndSlots(tabIdId, bankValue["rights"].asUInt(), bankValue["rights"].asUInt());
            _SetRankBankTabRightsAndSlots(rankId, rightsAndSlots);
        }
    }
}

// RankInfo
void Guild::RankInfo::LoadFromDB(uint32 rankId, Json::Value rankValue)
{
    m_rankId            = rankId;
    m_name              = rankValue["m_name"].asString();
    m_rights            = rankValue["m_rights"].asUInt();
    m_bankMoneyPerDay   = rankValue["m_bankMoneyPerDay"].asUInt64();
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        m_rights |= GR_RIGHT_ALL;
}

bool Guild::LoadEventLogFromDB(Json::Value eventValue)
{
    for (auto itr = eventValue.begin(); itr != eventValue.end(); ++itr)
    {
        uint32 guid = atoi(itr.memberName());
        auto event = (*itr);

        if (m_eventLog->CanInsert())
        {
            m_eventLog->LoadEvent(new EventLogEntry(
                m_id,                                                // guild id
                guid,                                                // guid
                time_t(event["m_timestamp"].asUInt()),               // timestamp
                GuildEventLogTypes(event["m_eventType"].asUInt()),   // event type
                event["m_playerGuid1"].asUInt(),                     // player guid 1
                event["m_playerGuid2"].asUInt(),                     // player guid 2
                event["m_newRank"].asUInt()));                       // rank
            return true;
        }
    }
    return false;
}

bool Guild::LoadBankEventLogFromDB(Json::Value eventBankValue)
{
    for (auto itr = eventBankValue.begin(); itr != eventBankValue.end(); ++itr)
    {
        uint8 dbTabId = atoi(itr.memberName());
        auto eventBank = (*itr);

        for (auto itr = eventBank.begin(); itr != eventBank.end(); ++itr)
        {
            uint32 guid = atoi(itr.memberName());
            auto event = (*itr);

            bool isMoneyTab = (dbTabId == GUILD_BANK_MONEY_LOGS_TAB);
            if (dbTabId < GetPurchasedTabsSize() || isMoneyTab)
            {
                uint8 tabId = isMoneyTab ? uint8(GUILD_BANK_MAX_TABS) : dbTabId;
                LogHolder* pLog = m_bankEventLog[tabId];
                if (pLog->CanInsert())
                {
                    GuildBankEventLogTypes eventType = GuildBankEventLogTypes(event["m_eventType"].asUInt());
                    if (BankEventLogEntry::IsMoneyEvent(eventType))
                    {
                        if (!isMoneyTab)
                        {
                            sLog->outError(LOG_FILTER_GUILD, "GuildBankEventLog ERROR: MoneyEvent(LogGuid: %u, Guild: %u) does not belong to money tab (%u), ignoring...", guid, m_id, dbTabId);
                            return false;
                        }
                    }
                    else if (isMoneyTab)
                    {
                        sLog->outError(LOG_FILTER_GUILD, "GuildBankEventLog ERROR: non-money event (LogGuid: %u, Guild: %u) belongs to money tab, ignoring...", guid, m_id);
                        return false;
                    }
                    pLog->LoadEvent(new BankEventLogEntry(
                        m_id,                                   // guild id
                        guid,                                   // guid
                        time_t(event["m_eventType"].asUInt()),           // timestamp
                        dbTabId,                                         // tab id
                        eventType,                                       // event type
                        event["m_playerGuid"].asUInt(),                  // player guid
                        event["m_itemOrMoney"].asUInt(),                 // item or money
                        event["m_itemStackCount"].asUInt(),              // itam stack count
                        event["m_destTabId"].asUInt()));                 // dest tab id
                }
            }
        }
    }
    return true;
}

void Guild::LoadBankTabFromDB(Json::Value bankTabValue)
{
    for (auto itr = bankTabValue.begin(); itr != bankTabValue.end(); ++itr)
    {
        uint8 tabId = atoi(itr.memberName());
        auto bankTab = (*itr);

        if (tabId >= GetPurchasedTabsSize())
        {
            sLog->outError(LOG_FILTER_GUILD, "Invalid tab (tabId: %u) in guild bank, skipped.", tabId);
            continue;
        }
        m_bankTabs[tabId]->LoadFromDB(bankTab);
    }
}

// BankTab
void Guild::BankTab::LoadFromDB(Json::Value bankTab)
{
    m_name = bankTab["m_name"].asString();
    m_icon = bankTab["m_icon"].asString();
    m_text = bankTab["m_text"].asString();
}

// EmblemInfo
void EmblemInfo::LoadFromDB(Json::Value emblemData)
{
    m_style             = emblemData["m_style"].asUInt();
    m_color             = emblemData["m_color"].asUInt();
    m_borderStyle       = emblemData["m_borderStyle"].asUInt();
    m_borderColor       = emblemData["m_borderColor"].asUInt();
    m_backgroundColor   = emblemData["m_backgroundColor"].asUInt();
}

void Guild::GuildNewsLog::LoadFromDB(Json::Value guildNews)
{
    for (auto itr = guildNews.begin(); itr != guildNews.end(); ++itr)
    {
        uint8 id = atoi(itr.memberName());
        auto news = (*itr);

        GuildNewsEntry& log = _newsLog[id];
        log.EventType = GuildNews(news["EventType"].asUInt());
        log.PlayerGuid = news["PlayerGuid"].asUInt();
        log.Data = news["Data"].asUInt();
        log.Flags = news["Flags"].asUInt();
        log.Date = time_t(news["Date"].asUInt());
    }

    Shrink();
}

bool Guild::LoadMemberFromDB(uint32 memberId, Json::Value memberData)
{
    Member *member = new Member(this, MAKE_NEW_GUID(memberId, 0, HIGHGUID_PLAYER), memberData["m_rankId"].asUInt());
    if (!member->LoadFromDB(memberData))
    {
        DeleteMemberGuild(memberId);
        delete member;
        return false;
    }

    sLog->outInfo(LOG_FILTER_REDIS, "Guild::LoadMemberFromDB memberId %i", memberId);
    m_members[memberId] = member;
    return true;
}

bool Guild::Member::LoadFromDB(Json::Value memberData)
{
    m_publicNote    = memberData["m_publicNote"].asString();
    m_officerNote   = memberData["m_officerNote"].asString();
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime  = memberData["BankResetTimeMoney"].asUInt();
    m_bankRemaining[GUILD_BANK_MAX_TABS].value      = memberData["BankRemMoney"].asUInt();

    m_bankRemaining[0].resetTime                = memberData["BankResetTimeTab0"].asUInt();
    m_bankRemaining[0].value                    = memberData["BankRemSlotsTab0"].asUInt();
    m_bankRemaining[1].resetTime                = memberData["BankResetTimeTab1"].asUInt();
    m_bankRemaining[1].value                    = memberData["BankRemSlotsTab1"].asUInt();
    m_bankRemaining[2].resetTime                = memberData["BankResetTimeTab2"].asUInt();
    m_bankRemaining[2].value                    = memberData["BankRemSlotsTab2"].asUInt();
    m_bankRemaining[3].resetTime                = memberData["BankResetTimeTab3"].asUInt();
    m_bankRemaining[3].value                    = memberData["BankRemSlotsTab3"].asUInt();
    m_bankRemaining[4].resetTime                = memberData["BankResetTimeTab4"].asUInt();
    m_bankRemaining[4].value                    = memberData["BankRemSlotsTab4"].asUInt();
    m_bankRemaining[5].resetTime                = memberData["BankResetTimeTab5"].asUInt();
    m_bankRemaining[5].value                    = memberData["BankRemSlotsTab5"].asUInt();
    m_bankRemaining[6].resetTime                = memberData["BankResetTimeTab6"].asUInt();
    m_bankRemaining[6].value                    = memberData["BankRemSlotsTab6"].asUInt();
    m_bankRemaining[7].resetTime                = memberData["BankResetTimeTab7"].asUInt();
    m_bankRemaining[7].value                    = memberData["BankRemSlotsTab7"].asUInt();

    SetStats(memberData["m_name"].asString(),                       // characters.name
            memberData["m_level"].asUInt(),                         // characters.level
            memberData["m_class"].asUInt(),                         // characters.class
            memberData["m_zoneId"].asUInt(),                        // characters.zone
            memberData["m_accountId"].asUInt(),                     // characters.account
            memberData["m_totalReputation"].asUInt(),               // character_reputation.standing
            memberData["m_gender"].asUInt(),                        // characters.gender
            memberData["m_achievementPoints"].asUInt(),             // achievement points
            memberData["profId1"].asUInt(),                         // prof id 1
            memberData["profValue1"].asUInt(),                      // prof value 1
            memberData["profRank1"].asUInt(),                       // prof rank 1
            memberData["recipesMask1"].asString(),                  // prof recipes mask 1
            memberData["profId2"].asUInt(),                         // prof id 2
            memberData["profValue2"].asUInt(),                      // prof value 2
            memberData["profRank2"].asUInt(),                       // prof rank 2
            memberData["recipesMask2"].asString()                   // prof recipes mask 2
            );
    m_logoutTime    = memberData["m_logoutTime"].asUInt();           // characters.logout_time
    m_totalActivity = memberData["m_totalActivity"].asUInt64();
    m_weekActivity = memberData["m_weekActivity"].asUInt64();
    m_weekReputation = memberData["m_weekReputation"].asUInt();

    if (!CheckStats())
        return false;

    if (!m_zoneId)
        m_zoneId = 5736; //Hack

    return true;
}

void Guild::InitAchievement()
{
    for (auto iter = GetAchievementMgr().GetCompletedAchievementsList().begin(); iter != GetAchievementMgr().GetCompletedAchievementsList().end(); ++iter)
    {
        std::string achievementId = std::to_string(iter->first);
        GuildAchievementData[achievementId.c_str()]["date"] = iter->second.date;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "achievement", GuildAchievementData, m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::InitAchievement guid %u", guid);
    });
}

void Guild::DeleteCriteriaProgress(AchievementEntry const* achievement)
{
    std::string achievID = std::to_string(achievement->ID);

    GuildCriteriaData.removeMember(achievID.c_str());

    RedisDatabase.AsyncExecuteH("HDEL", criteriaKey, achievID.c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteCriteriaProgress guid %u", guid);
    });
}

void Guild::InitCriteria()
{
    for (auto itr = GetAchievementMgr().GetAchievementProgress().begin(); itr != GetAchievementMgr().GetAchievementProgress().end(); ++itr)
    {
        std::string achievID = std::to_string(itr->first);
        bool save_pl = false;
        bool save_ac = false;
        Json::Value Criteria;

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
                bool hasAchieve = m_achievementMgr.HasAchieved(achievement->ID) || (achievement->parent && !m_achievementMgr.HasAchieved(achievement->parent));
                if (iter->second.counter != 0 && !hasAchieve)
                {
                    std::string criteriaId = std::to_string(iter->first);
                    Criteria[criteriaId.c_str()]["counter"] = iter->second.counter;
                    Criteria[criteriaId.c_str()]["date"] = iter->second.date;
                    Criteria[criteriaId.c_str()]["completed"] = iter->second.completed;
                    Criteria[criteriaId.c_str()]["CompletedGUID"] = GUID_LOPART(iter->second.CompletedGUID);
                }
            }
        }

        GuildCriteriaData[achievID.c_str()] = Criteria;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaKey, achievID.c_str(), Criteria, m_id, [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Guild::InitCriteria account guid %u", guid);
        });
    }
}

void Guild::UpdateCriteriaProgress(AchievementEntry const* achievement, CriteriaProgressMap* progressMap)
{
    std::string achievID = std::to_string(achievement->ID);
    Json::Value Criteria;

    for (auto iter = progressMap->begin(); iter != progressMap->end(); ++iter)
    {
        if (iter->second.deactiveted)
            continue;

        // store data only for real progress
        if (iter->second.counter != 0)
        {
            std::string criteriaId = std::to_string(iter->first);
            Criteria[criteriaId.c_str()]["counter"] = iter->second.counter;
            Criteria[criteriaId.c_str()]["date"] = iter->second.date;
            Criteria[criteriaId.c_str()]["completed"] = iter->second.completed;
            Criteria[criteriaId.c_str()]["CompletedGUID"] = GUID_LOPART(iter->second.CompletedGUID);
        }
    }

    GuildCriteriaData[achievID.c_str()] = Criteria;
    RedisDatabase.AsyncExecuteHSet("HSET", criteriaKey, achievID.c_str(), Criteria, m_id, [&](const RedisValue &v, uint64 guid) {
        //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerCriteria player guid %u", guid);
    });
}

void Guild::LoadAchievement(Json::Value achievData)
{
    for (auto itr = achievData.begin(); itr != achievData.end(); ++itr)
    {
        uint32 achievID = atoi(itr.memberName());
        auto achievValue = (*itr);

        uint32 date    = achievValue["date"].asInt();

        m_achievementMgr.AddAchievements(achievID, date);
    }
}

bool Guild::BankTab::LoadItemFromDB(Json::Value itemData)
{
    uint8 slotId = itemData["slot"].asUInt();
    uint32 itemGuid = itemData["itemGuid"].asUInt();
    uint32 itemEntry = itemData["itemEntry"].asUInt();
    if (slotId >= GUILD_BANK_MAX_SLOTS)
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid slot for item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
    if (!proto)
    {
        sLog->outError(LOG_FILTER_GUILD, "Unknown item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    Item* pItem = NewItemOrBag(proto);
    pItem->SetItemKey(ITEM_KEY_GUILD, m_guildId);

    if (!pItem->LoadFromDB(itemGuid, 0, itemData, itemEntry))
    {
        delete pItem;
        return false;
    }

    pItem->SetTabId(m_tabId);
    pItem->SetSlot(slotId);
    pItem->AddToWorld();
    m_items[slotId] = pItem;
    return true;
}

void GuildFinderMgr::GuildFinderSave(std::string index, Json::Value finderData)
{
    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetGuildFKey(), index.c_str(), finderData, 0, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::GuildFinderSave guid %u", guid);
    });
}

void GuildFinderMgr::GuildFinderMemberSave()
{
    for (auto itr = FinderMemberData.begin(); itr != FinderMemberData.end(); ++itr)
    {
        RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetGuildFMKey(), itr.memberName(), (*itr), 0, [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::GuildFinderMemberSave guid %u", guid);
        });
    }
}

void GuildFinderMgr::UpdateFinderMember(std::string index)
{
    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetGuildFMKey(), index.c_str(), FinderMemberData[index.c_str()], 0, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::GuildFinderMemberSave guid %u", guid);
    });
}

void GuildFinderMgr::DeleteFinderGuild(std::string index)
{
    RedisDatabase.AsyncExecuteH("HDEL", sRedisBuilderMgr->GetGuildFMKey(), index.c_str(), 0, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteFinderGuild guid %u", guid);
    });
    RedisDatabase.AsyncExecuteH("HDEL", sRedisBuilderMgr->GetGuildFKey(), index.c_str(), 0, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteFinderGuild guid %u", guid);
    });
}

void GuildFinderMgr::LoadFromRedis()
{
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading guild finder guild-related settings...");

    uint32 oldMSTime = getMSTime();

    RedisValue finder = RedisDatabase.Execute("HGETALL", sRedisBuilderMgr->GetGuildFKey());

    std::vector<RedisValue> finderVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(&finder, finderVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::LoadFromRedis guild not found");
        return;
    }

    uint32 count = 0;
    for (auto itr = finderVector.begin(); itr != finderVector.end();)
    {
        uint32 guildId = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        Json::Value _data;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), _data))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::LoadFromRedis not parse guildId %i", guildId);
            continue;
        }
        else
            ++itr;

        uint8  availability = _data["availability"].asUInt();
        uint8  classRoles   = _data["classRoles"].asUInt();
        uint8  interests    = _data["interests"].asUInt();
        uint8  level        = _data["level"].asUInt();
        bool   listed       = (_data["listed"].asUInt() != 0);
        std::string comment = _data["comment"].asString();
        TeamId guildTeam = TeamId(_data["guildTeam"].asUInt());

        LFGuildSettings settings(listed, guildTeam, guildId, classRoles, availability, interests, level, comment);
        _guildSettings[guildId] = settings;

        ++count;
    }
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u guild finder guild-related settings in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));


    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading guild finder membership requests...");

    RedisValue finderM = RedisDatabase.Execute("HGETALL", sRedisBuilderMgr->GetGuildFMKey());

    std::vector<RedisValue> finderMVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(&finderM, finderVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::LoadFromRedis guild not found");
        return;
    }

    count = 0;
    for (auto itr = finderMVector.begin(); itr != finderMVector.end();)
    {
        uint32 guildId = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        Json::Value _data;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), _data))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "GuildFinderMgr::LoadFromRedis not parse guildId %i", guildId);
            continue;
        }
        else
            ++itr;

        for (auto itr = _data.begin(); itr != _data.end(); ++itr)
        {
            uint32 playerId = atoi(itr.memberName());
            auto dataValue = *itr;

            uint8  availability = dataValue["availability"].asUInt();
            uint8  classRoles   = dataValue["classRoles"].asUInt();
            uint8  interests    = dataValue["interests"].asUInt();
            uint32  submitTime    = dataValue["submitTime"].asUInt();
            std::string comment = dataValue["comment"].asString();

            MembershipRequest request(playerId, guildId, availability, classRoles, interests, comment, time_t(submitTime));
            _membershipRequests[guildId].push_back(request);

            ++count;
        }
    }
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u guild finder membership requests in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}
