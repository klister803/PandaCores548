/*
uwow.biz
*/

#ifndef _BRACKETMGR_H
#define _BRACKETMGR_H

#include "Bracket.h"
#include "Player.h"

class BracketMgr
{
    friend class ACE_Singleton<BracketMgr, ACE_Null_Mutex>;
    BracketMgr() {};
    ~BracketMgr();

    public:
        typedef UNORDERED_MAP<uint64, BracketList> BracketContainer;

        void LoadCharacterBrackets();

        Bracket* TryGetOrCreateBracket(uint64 guid, BracketType bType);
        void DeleteBracketInfo(uint64 guid);

        void LoadBrackets();

    private:
        BracketContainer m_conteiner;
};

#define sBracketMgr ACE_Singleton<BracketMgr, ACE_Null_Mutex>::instance()
#endif