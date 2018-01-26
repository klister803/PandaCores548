/*
uwow.biz
*/

#include "BracketMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"

BracketMgr::~BracketMgr()
{
    for (BracketContainer::iterator itr = m_conteiner.begin(); itr != m_conteiner.end(); ++itr)
        for (BracketList::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            delete itr2->second;
}

void BracketMgr::LoadCharacterBrackets()
{
    uint32 oldMSTime = getMSTime();

    //                                                      0           1       2       3           4       5       6       7               8        9
    QueryResult result = CharacterDatabase.Query("SELECT `bracket`, `rating`, `best`, `bestWeek`, `mmr`, `games`, `wins`, `weekGames`, `weekWins`, `guid` FROM `character_brackets_info`");
    if (!result)
    {
        TC_LOG_INFO("server", ">> Loaded 0 bracket info. DB table `character_brackets_info` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        BracketType bType = (BracketType)fields[0].GetUInt8();
        uint64 owner = fields[9].GetUInt64();

        Bracket * bracket = TryGetOrCreateBracket(owner, bType);

        uint16 rating = fields[1].GetUInt16();
        uint16 rating_best = fields[2].GetUInt16();
        uint16 rating_best_week = fields[3].GetUInt16();
        uint16 mmv = fields[4].GetUInt16();
        uint32 games      = fields[5].GetUInt32();
        uint32 wins       = fields[6].GetUInt32();
        uint32 week_games = fields[7].GetUInt16();
        uint32 week_wins  = fields[8].GetUInt16();

        bracket->InitStats(rating, mmv, games, wins, week_games, week_wins, rating_best_week, rating_best);
        ++count;
    }
    while (result->NextRow());
    TC_LOG_INFO("server", ">> Loaded %u brackets data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

Bracket* BracketMgr::TryGetOrCreateBracket(uint64 guid, BracketType bType)
{
    BracketContainer::iterator itr = m_conteiner.find(guid);
    if (itr == m_conteiner.end())
    {
        Bracket *b = NULL;
        b = new Bracket(guid, bType);
        m_conteiner[guid][bType] = b;
        return b;
    }

    BracketList::iterator itr2 = m_conteiner[guid].find(bType);
    if (itr2 == m_conteiner[guid].end())
    {
        Bracket *b = NULL;
        b = new Bracket(guid, bType);
        m_conteiner[guid][bType] = b;
        return b;
    }
    return m_conteiner[guid][bType];
}

void BracketMgr::DeleteBracketInfo(uint64 guid)
{
    BracketContainer::iterator itr = m_conteiner.find(guid);
    if (itr == m_conteiner.end())
        return;

    for (BracketList::iterator itr = m_conteiner[guid].begin(); itr != m_conteiner[guid].end(); ++itr)
        itr->second->SetState(BRACKET_REMOVED);
}