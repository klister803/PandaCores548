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

#include "BracketMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Bracket.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "RedisBuilderMgr.h"

void Bracket::SaveBracket()
{
    std::string type = std::to_string(m_Type);
    BracketData[type.c_str()]["rating"] = getRating();
    BracketData[type.c_str()]["rating_best"] = GetBracketInfo(BRACKET_BEST);
    BracketData[type.c_str()]["rating_best_week"] = GetBracketInfo(BRACKET_WEEK_BEST);
    BracketData[type.c_str()]["mmv"] = getMMV();
    BracketData[type.c_str()]["games"] = GetBracketInfo(BRACKET_SEASON_GAMES);
    BracketData[type.c_str()]["wins"] = GetBracketInfo(BRACKET_SEASON_WIN);
    BracketData[type.c_str()]["week_games"] = GetBracketInfo(BRACKET_WEEK_GAMES);
    BracketData[type.c_str()]["week_wins"] = GetBracketInfo(BRACKET_WEEK_WIN);

    std::string index = std::to_string(GUID_LOPART(m_owner));

    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetBracketKey(), index.c_str(), BracketData, m_owner, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Bracket::SaveBracket guid %u", guid);
    });
}

void BracketMgr::LoadBrackets()
{
    uint32 oldMSTime = getMSTime();

    RedisValue bracket = RedisDatabase.Execute("HGETALL", sRedisBuilderMgr->GetBracketKey());

    std::vector<RedisValue> bracketVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(&bracket, bracketVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "BracketMgr::LoadBrackets guild not found");
        return;
    }

    uint32 count = 0;
    for (auto itr = bracketVector.begin(); itr != bracketVector.end();)
    {
        uint32 guid = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        Json::Value data;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), data))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "BracketMgr::LoadBrackets not parse guid %i", guid);
            continue;
        }
        else
            ++itr;

        for (auto iter = data.begin(); iter != data.end(); ++iter)
        {
            BracketType type = (BracketType)atoi(iter.memberName());
            auto dataValue = *iter;

            Bracket * bracket = TryGetOrCreateBracket(guid, type);
            bracket->InitStats(dataValue["rating"].asUInt(), dataValue["mmv"].asUInt(), dataValue["games"].asUInt(), dataValue["wins"].asUInt(), dataValue["week_games"].asUInt(), dataValue["week_wins"].asUInt(), dataValue["rating_best_week"].asUInt(), dataValue["rating_best"].asUInt());
        }
        ++count;
    }
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u brackets data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}
