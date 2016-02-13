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
    RedisDatabase.AsyncExecuteHSet("HSET", sGuildMgr->GetGuildKey(), index.c_str(), sRedisBuilder->BuildString(GuildData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    std::string index = std::to_string(GUID_LOPART(m_guid));

    RedisDatabase.AsyncExecuteHSet("HSET", guildMemberKey, index.c_str(), sRedisBuilder->BuildString(GuildMemberData).c_str(), m_guid, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "eventlog", sRedisBuilder->BuildString(GuildEventLogData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "bankeventlog", sRedisBuilder->BuildString(GuildBankEventLogData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "eventlog", sRedisBuilder->BuildString(GuildBankEventLogData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "bankeventlog", sRedisBuilder->BuildString(GuildBankEventLogData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "data", sRedisBuilder->BuildString(GuildData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "rank", sRedisBuilder->BuildString(GuildRankData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "rank", sRedisBuilder->BuildString(GuildRankData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "banktab", sRedisBuilder->BuildString(GuildTabData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildBankTab guid %u", guid);
    });
}

void Guild::UpdateGuildBankTab(BankTab* pTab)
{
    std::string id = std::to_string(pTab->GetTabId());
    GuildTabData[id.c_str()]["m_name"] = pTab->GetName();
    GuildTabData[id.c_str()]["m_icon"] = pTab->GetIcon();
    GuildTabData[id.c_str()]["m_text"] = pTab->GetText();

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "banktab", sRedisBuilder->BuildString(GuildTabData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::UpdateGuildBankTab guid %u", guid);
    });
}

void Guild::SaveGuildMoney()
{
    GuildMoney = m_bankMoney;

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "money", sRedisBuilder->BuildString(GuildMoney).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildMoney guid %u", guid);
    });
}

void Guild::DeleteMemberGuild(uint32 lowguid)
{
    char* userKey = new char[18];
    sprintf(userKey, "r{%u}u{%u}", realmID, lowguid);

    RedisDatabase.AsyncExecuteH("HDEL", userKey, "guild", lowguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteMemberGuild guid %u", guid);
    });
}

void Guild::DeleteMembers()
{
    char* _key = new char[32];
    sprintf(_key, "r{%u}g{%u}member", realmID, m_id);
    RedisDatabase.AsyncExecute("DEL", _key, m_id, [&](const RedisValue &v, uint64 guid) {});
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "newslog", sRedisBuilder->BuildString(GuildNewsLogData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildBankTab guid %u", guid);
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

    RedisDatabase.AsyncExecuteHSet("HSET", GetGuildKey(), "newslog", sRedisBuilder->BuildString(GuildNewsLogData).c_str(), m_id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Guild::SaveGuildBankTab guid %u", guid);
    });
}

void GuildMgr::LoadGuildsRedis()
{
    // 1. Load all guilds
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading guilds definitions...");
    {
        uint32 oldMSTime = getMSTime();

        RedisValue guildV = RedisDatabase.Execute("HGETALL", guildKey);

        std::vector<RedisValue> guildVector;
        if (!sRedisBuilder->LoadFromRedisArray(&guildV, guildVector))
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

            Json::Value guildData;
            if (!sRedisBuilder->LoadFromRedis(&(*itr), guildData))
            {
                ++itr;
                sLog->outInfo(LOG_FILTER_REDIS, "GuildMgr::LoadGuildsRedis not parse guildId %i", guildId);
                continue;
            }
            else
                ++itr;

            char* guildKeyId = new char[32];
            sprintf(guildKeyId, "r{%u}g{%u}", realmID, guildId);

            RedisValue v = RedisDatabase.Execute("HGETALL", guildKeyId);

            std::vector<RedisValue> guildDataV;
            if (!sRedisBuilder->LoadFromRedisArray(&v, guildDataV))
            {
                sLog->outInfo(LOG_FILTER_REDIS, "GuildMgr::LoadGuildsRedis group not found");
                continue;
            }

            Guild* guild = new Guild();

            if (!guild->LoadFromDB(guildId, guildData, &guildDataV))
            {
                delete guild;
                continue;
            }
            AddGuild(guild);

            ++count;
        }

        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u guild definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    // 3. Load all guild members
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading guild members...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete orphaned guild member entries before loading the valid ones
        CharacterDatabase.DirectExecute("DELETE gm FROM guild_member gm LEFT JOIN guild g ON gm.guildId = g.guildId WHERE g.guildId IS NULL");

                                                     //          0        1        2     3      4        5                   6
        QueryResult result = CharacterDatabase.Query("SELECT gm.guildid, gm.guid, rank, pnote, offnote, BankResetTimeMoney, BankRemMoney, "
                                                     //   7                  8                 9                  10                11                 12
                                                     "BankResetTimeTab0, BankRemSlotsTab0, BankResetTimeTab1, BankRemSlotsTab1, BankResetTimeTab2, BankRemSlotsTab2, "
                                                     //   13                 14                15                 16                17                 18
                                                     "BankResetTimeTab3, BankRemSlotsTab3, BankResetTimeTab4, BankRemSlotsTab4, BankResetTimeTab5, BankRemSlotsTab5, "
                                                     //   19                 20                21                 22
                                                     "BankResetTimeTab6, BankRemSlotsTab6, BankResetTimeTab7, BankRemSlotsTab7, "
                                                     //   23      24       25       26      27         28             29          30           31          32       33         34
                                                     "c.name, c.level, c.class, c.zone, c.account, c.logout_time, re.standing, XpContrib, XpContribWeek, RepWeek, AchPoint, c.gender, "
                                                     //   35   36          37         38            39       40          41         42
                                                     "profId1, profValue1, profRank1, recipesMask1, profId2, profValue2, profRank2, recipesMask2 "
                                                     "FROM guild_member gm LEFT JOIN characters c ON c.guid = gm.guid LEFT JOIN character_reputation re ON re.guid = gm.guid AND re.faction = 1168 ORDER BY guildid ASC");

        if (!result)
        {
            sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 guild members. DB table `guild_member` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadMemberFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u guild members int %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // 9. Load guild achievements
    {
        PreparedQueryResult achievementResult;
        PreparedQueryResult criteriaResult;
        for (GuildContainer::const_iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_ACHIEVEMENT);
            stmt->setUInt32(0, itr->first);
            achievementResult = CharacterDatabase.Query(stmt);
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_ACHIEVEMENT_CRITERIA);
            stmt->setUInt32(0, itr->first);
            criteriaResult = CharacterDatabase.Query(stmt);

            itr->second->GetAchievementMgr().LoadFromDB(achievementResult, criteriaResult);
        }
    }

    // 11. Update Guild Known Recipes
    for (GuildContainer::iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
    {
        Guild* guild = itr->second;
        if (!guild)
            continue;

        guild->UpdateGuildRecipes();
    }

    // 12. Validate loaded guild data
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Validating data of loaded guilds...");
    {
        uint32 oldMSTime = getMSTime();

        for (GuildContainer::iterator itr = GuildStore.begin(); itr != GuildStore.end();)
        {
            Guild* guild = itr->second;
            if (guild)
            {
                guild->Validate();
                guild->SaveGuild();
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

bool Guild::LoadFromDB(uint32 guildId, Json::Value guildData, std::vector<RedisValue>* guildDataV)
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

    for (auto itr = guildDataV->begin(); itr != guildDataV->end();)
    {
        std::string index = itr->toString();
        ++itr;

        Json::Value guildData2;
        if (!sRedisBuilder->LoadFromRedis(&(*itr), guildData2))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "GuildMgr::LoadFromDB not parse guildId %i", guildId);
            continue;
        }
        else
            ++itr;

        if (!strcmp(index.c_str(), "money"))
            m_bankMoney = guildData2.asUInt64();
        else if (!strcmp(index.c_str(), "rank"))
                LoadRankFromDB(guildData2);
        else if (!strcmp(index.c_str(), "eventlog"))
                LoadEventLogFromDB(guildData2);
        else if (!strcmp(index.c_str(), "bankeventlog"))
                LoadBankEventLogFromDB(guildData2);
        else if (!strcmp(index.c_str(), "banktab"))
                LoadBankTabFromDB(guildData2);
        else if (!strcmp(index.c_str(), "newslog"))
                GetNewsLog().LoadFromDB(guildData2);
    }
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
    m_bankMoneyPerDay   = rankValue["m_bankMoneyPerDay"].asUInt();
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
