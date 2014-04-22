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

#ifndef TRINITY_CHALLENGEMGR_H
#define TRINITY_CHALLENGEMGR_H

#include <ace/Singleton.h>

struct ChallengeMember
{
    uint64 guid;
    uint32 specId;
};

typedef std::set<ChallengeMember> ChallengeMemberList;

struct Challenge
{
    uint32 Id;              // challenge id
    uint32 mapID;
    uint32 recordTime;      // time taken for complite challenge
    uint32 Date;            // time when recorde done
    uint32 Medal;

    ChallengeMemberList member;
};

class CalendarMgr
{
        friend class ACE_Singleton<ChallengeMgr, ACE_Null_Mutex>;

        ChallengeMgr() : challengeID(0){};
        ~ChallengeMgr(){};

    public:
        void LoadFromDB();
        void GenerateChallengeID() { return ++challengeID; }

    protected:
        uint32 challengeID;
};

#define sChallengeMgr ACE_Singleton<ChallengeMgr, ACE_Null_Mutex>::instance()

#endif
