/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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


#include "ChallengeMgr.h"
#include "QueryResult.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "ObjectAccessor.h"

ChallengeMgr::~ChallengeMgr()
{
    for(ChallengeMap::iterator itr = m_ChallengeMap.begin(); itr != m_ChallengeMap.end(); ++itr)
        delete itr->second;

    m_ChallengeMap.clear();
    m_ChallengesOfMember.clear();
    m_BestForMap.clear();
}

void ChallengeMgr::CheckBestMapId(Challenge *c)
{
    if (!m_BestForMap[c->mapID] || m_BestForMap[c->mapID]->recordTime > c->recordTime)
        m_BestForMap[c->mapID] = c;
}

void ChallengeMgr::CheckBestGuildMapId(Challenge *c)
{
    if (!c->guildId)
        return;

    if (!m_GuildBest[c->guildId][c->mapID] || m_GuildBest[c->guildId][c->mapID]->recordTime > c->recordTime)
        m_GuildBest[c->guildId][c->mapID] = c;
}

void ChallengeMgr::CheckBestMemberMapId(uint64 guid, Challenge *c)
{
    if (!m_ChallengesOfMember[guid][c->mapID] || m_ChallengesOfMember[guid][c->mapID]->recordTime > c->recordTime)
        m_ChallengesOfMember[guid][c->mapID] = c;
}

void ChallengeMgr::SaveChallengeToDB(Challenge *c)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHALLENGE);
    stmt->setUInt32(0, c->Id);
    stmt->setUInt32(1, c->guildId);
    stmt->setUInt16(2, c->mapID);
    stmt->setUInt32(3, c->recordTime);
    stmt->setUInt32(4, c->date);
    stmt->setUInt8(5, c->medal);
    trans->Append(stmt);

    for(ChallengeMemberList::const_iterator itr = c->member.begin(); itr != c->member.end(); ++itr)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHALLENGE_MEMBER);
        stmt->setUInt32(0, c->Id);
        stmt->setUInt64(1, (*itr).guid);
        stmt->setUInt16(2, (*itr).specId);
        trans->Append(stmt);
    }

    CharacterDatabase.CommitTransaction(trans);
}

void ChallengeMgr::LoadFromDB()
{
    uint32 count = 0;

    if (QueryResult result = CharacterDatabase.Query("SELECT `id`, `guildId`, `mapID`, `recordTime`, `date`, `medal` FROM `challenge`"))
    {
        Field* fields = NULL;
        do
        {
            Field* fields = result->Fetch();

            Challenge *c = new Challenge;
            c->Id = fields[0].GetUInt32();
            c->guildId = fields[1].GetUInt32();
            c->mapID = fields[2].GetUInt16();
            c->recordTime = fields[3].GetUInt32();
            c->date = fields[4].GetUInt32();
            c->medal = fields[5].GetUInt8();

            m_ChallengeMap[c->Id] = c;
            CheckBestMapId(c);
            CheckBestGuildMapId(c);

            // sync guid generator
            if (c->Id >= challengeGUID)
                challengeGUID = ++c->Id;

        }while (result->NextRow());
    }

    if (QueryResult result = CharacterDatabase.Query("SELECT `id`, `member`, `specID` FROM `challenge_member`"))
    {
        do
        {
            Field* fields = result->Fetch();
            ChallengeMember member;
            member.guid = fields[1].GetUInt64();
            member.specId = fields[2].GetUInt16();

            ChallengeMap::iterator itr = m_ChallengeMap.find(fields[0].GetUInt32());
            if (itr == m_ChallengeMap.end())
            {
                TC_LOG_ERROR("sql", "Tabble challenge_member. Challenge %u for member " UI64FMTD " does not exist!", fields[0].GetUInt32(), member.guid);
                continue;
            }
            itr->second->member.insert(member);
            CheckBestMemberMapId(member.guid, itr->second);
        }while (result->NextRow());
    }

    /// @init quest complete
    m_reward.clear();
    if (QueryResult result = WorldDatabase.Query("SELECT `mapId`, `questCredit` FROM `chellenge_reward`"))
    {
        do
        {
            Field* fields = result->Fetch();
            m_reward[fields[0].GetUInt16()] = fields[1].GetUInt32();
        }while (result->NextRow());
    }

    /// @init reward for medals
    m_valorPoints[CHALLENGE_MEDAL_NONE] = sWorld->getIntConfig(CONFIG_CURRENCY_VALOR_FOR_CHALLENGE_NO_MEDAL);    //defauld mod 10500
    m_valorPoints[CHALLENGE_MEDAL_BRONZE] = sWorld->getIntConfig(CONFIG_CURRENCY_VALOR_FOR_CHALLENGE_BRONZ);     // +2000
    m_valorPoints[CHALLENGE_MEDAL_SILVER] = sWorld->getIntConfig(CONFIG_CURRENCY_VALOR_FOR_CHALLENGE_SILVER);    // +4000
    m_valorPoints[CHALLENGE_MEDAL_GOLD] = sWorld->getIntConfig(CONFIG_CURRENCY_VALOR_FOR_CHALLENGE_GOLD);        // +6000
    m_valorPoints[CHALLENGE_MEDAL_PLAT] = sWorld->getIntConfig(CONFIG_CURRENCY_VALOR_FOR_CHALLENGE_PLATINUM);    // only for scenario
}

void ChallengeMgr::GroupReward(Map *instance, uint32 recordTime, ChallengeMode medal)
{
    Map::PlayerList const& players = instance->GetPlayers();
    if (players.isEmpty())
        return;

    uint32 challengeID = GenerateChallengeID();

    Challenge *c = new Challenge;
    c->Id = challengeID;
    c->mapID = instance->GetId();
    c->recordTime = recordTime;
    c->date = time(NULL);
    c->medal = medal;

    // Finish quest's for complete challenge (without medal too)
    uint32 npcRewardCredit = m_reward[instance->GetId()];
    uint32 valorReward = m_valorPoints[medal];

    std::map<uint32/*guild*/, uint32> guildCounter;
    for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
        if (Player* player = i->getSource())
        {
            ChallengeMember member;
            member.guid = player->GetGUID();
            member.specId = player->GetActiveSpec();

            if (player->GetGuildId())
                guildCounter[player->GetGuildId()] += 1;

            c->member.insert(member);
            CheckBestMemberMapId(member.guid, c);

            /// @there is achieve just for complete challenge with no medal
            player->UpdateAchievementCriteria(CRITERIA_TYPE_INSTANSE_MAP_ID, instance->GetId(), medal);

            /// @quest reward for finish challenge. daily
            if (npcRewardCredit)    //should never happend
                player->KilledMonsterCredit(npcRewardCredit, 0);

            /// @reward for get medal
            if (valorReward)
                player->ModifyCurrency(CURRENCY_TYPE_VALOR_POINTS, valorReward);
        }

    // not save if no medal
    if (medal == CHALLENGE_MEDAL_NONE)
    {
        delete c;
        return;
    }

    // Stupid group guild check.
    for(std::map<uint32/*guild*/, uint32>::const_iterator itr = guildCounter.begin(); itr != guildCounter.end(); ++itr)
    {
        //only full guild group could be defined
        if(itr->second == 5)
            c->guildId = itr->first;
    }


    m_ChallengeMap[c->Id] = c;
    CheckBestMapId(c);
    CheckBestGuildMapId(c);

    SaveChallengeToDB(c);
}

Challenge * ChallengeMgr::BestServerChallenge(uint16 map)
{
    ChallengeByMap::iterator itr = m_BestForMap.find(map);
    if (itr == m_BestForMap.end())
        return NULL;

    return itr->second;
}

Challenge * ChallengeMgr::BestGuildChallenge(uint32 guildId, uint16 map)
{
    if (!guildId)
        return NULL;

    GuildBestRecord::iterator itr = m_GuildBest.find(guildId);
    if (itr == m_GuildBest.end())
        return NULL;

    ChallengeByMap::iterator itr2 = itr->second.find(map);
    if (itr2 == itr->second.end())
        return NULL;

    return itr2->second;
}