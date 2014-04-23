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

void ChallengeMgr::LoadFromDB()
{
    QueryResult result = CharacterDatabase.Query("SELECT `id`, `mapID`, `recordTime`, `date`, `medal` FROM `challenge`");

    if (!result)
        return;

    uint32 count = 0;

    Field* fields = NULL;
    do
    {
        fields = result->Fetch();

        Challenge *c = new Challenge;
        c->Id = fields[0].GetUInt32();
        c->mapID = fields[1].GetUInt16();
        c->recordTime = fields[2].GetUInt32();
        c->date = fields[3].GetUInt32();
        c->medal = fields[4].GetUInt8();

        m_ChallengeMap[c->Id] = c;
        if (!m_BestForMap[c->mapID] || m_BestForMap[c->mapID]->recordTime > c->recordTime)
            m_BestForMap[c->mapID] = c;

    }while (result->NextRow());

    result = CharacterDatabase.Query("SELECT `id`, `member`, `specID` FROM `challenge_member`");
    do
    {
        fields = result->Fetch();
        ChallengeMember member;
        member.guid = fields[1].GetUInt64();
        member.specId = fields[2].GetUInt16();

        ChallengeMap::iterator itr = m_ChallengeMap.find(fields[0].GetUInt32());
        if (itr == m_ChallengeMap.end())
        {
            sLog->outError(LOG_FILTER_SQL, "Tabble challenge_member. Challenge %u for member " UI64FMTD " does not exist!", fields[0].GetUInt32(), member.guid);
            continue;
        }
        itr->second->member.insert(member);
        m_ChallengesOfMember[member.guid].insert(itr->second);
    }while (result->NextRow());
}