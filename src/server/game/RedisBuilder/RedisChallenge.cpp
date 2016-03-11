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

#include "ChallengeMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "RedisBuilderMgr.h"

void ChallengeMgr::SaveChallenges()
{
    for(ChallengeMap::iterator itr = m_ChallengeMap.begin(); itr != m_ChallengeMap.end(); ++itr)
    {
        if (Challenge* challenge = itr->second)
        {
            std::string id = std::to_string(itr->first);
            ChallengeData[id.c_str()]["guildId"] = challenge->guildId;
            ChallengeData[id.c_str()]["mapID"] = challenge->mapID;
            ChallengeData[id.c_str()]["recordTime"] = challenge->recordTime;
            ChallengeData[id.c_str()]["date"] = challenge->date;
            ChallengeData[id.c_str()]["medal"] = challenge->medal;

            for(ChallengeMemberList::const_iterator iter = challenge->member.begin(); iter != challenge->member.end(); ++iter)
            {
                std::string guid = std::to_string((*iter).guid);
                ChallengeData[id.c_str()]["members"][guid.c_str()] = (*iter).specId;
            }

            RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetChallengeKey(), id.c_str(), ChallengeData[id.c_str()], itr->first, [&](const RedisValue &v, uint64 guid) {});
        }
    }
}

void ChallengeMgr::SaveChallengeToDB(Challenge* challenge)
{
    std::string id = std::to_string(challenge->Id);
    ChallengeData[id.c_str()]["guildId"] = challenge->guildId;
    ChallengeData[id.c_str()]["mapID"] = challenge->mapID;
    ChallengeData[id.c_str()]["recordTime"] = challenge->recordTime;
    ChallengeData[id.c_str()]["date"] = challenge->date;
    ChallengeData[id.c_str()]["medal"] = challenge->medal;

    for(ChallengeMemberList::const_iterator itr = challenge->member.begin(); itr != challenge->member.end(); ++itr)
    {
        std::string guid = std::to_string((*itr).guid);
        ChallengeData[id.c_str()]["members"][guid.c_str()] = (*itr).specId;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetChallengeKey(), id.c_str(), ChallengeData[id.c_str()], challenge->Id, [&](const RedisValue &v, uint64 guid) {});
}

void ChallengeMgr::LoadChallenges()
{
    uint32 oldMSTime = getMSTime();

    RedisValue challenge = RedisDatabase.Execute("HGETALL", sRedisBuilderMgr->GetChallengeKey());

    std::vector<RedisValue> challengeVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(&challenge, challengeVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "ChallengeMgr::LoadChallenges guild not found");
        return;
    }

    uint32 count = 0;
    for (auto itr = challengeVector.begin(); itr != challengeVector.end();)
    {
        uint32 id = atoi(itr->toString().c_str());
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
            sLog->outInfo(LOG_FILTER_REDIS, "ChallengeMgr::LoadChallenges not parse guid %i", id);
            continue;
        }
        else
            ++itr;

        Challenge *c = new Challenge;
        c->Id = id;
        c->guildId = data["guildId"].asUInt();
        c->mapID = data["mapID"].asUInt();
        c->recordTime = data["recordTime"].asUInt();
        c->date = data["date"].asUInt();
        c->medal = data["medal"].asUInt();

        m_ChallengeMap[id] = c;
        CheckBestMapId(c);
        CheckBestGuildMapId(c);

        // sync guid generator
        if (c->Id >= challengeGUID)
            challengeGUID = ++c->Id;

        for (auto iter = data["members"].begin(); iter != data["members"].end(); ++iter)
        {
            uint64 guid = atoi(iter.memberName());
            auto dataValue = *iter;

            ChallengeMember member;
            member.guid = guid;
            member.specId = dataValue.asUInt();

            ChallengeMap::iterator itr = m_ChallengeMap.find(id);
            if (itr == m_ChallengeMap.end())
                continue;

            itr->second->member.insert(member);
            CheckBestMemberMapId(member.guid, itr->second);
        }
        ++count;
    }
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u Challenge data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}
