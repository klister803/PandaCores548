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
    uint16 specId;

    bool operator < (const ChallengeMember& i) const { return guid > i.guid; }
    bool operator == (const ChallengeMember& i) const { return guid == i.guid; }
};

typedef std::set<ChallengeMember> ChallengeMemberList;

struct Challenge
{
    uint32 Id;              // challenge id
    uint32 guildId;            // is it guild group
    uint16 mapID;
    uint32 recordTime;      // time taken for complite challenge
    uint32 date;            // time when recorde done
    uint8 medal;

    ChallengeMemberList member;
};

typedef std::unordered_map<uint16/*map*/, Challenge *> ChallengeByMap;
typedef std::unordered_map<uint32/*id*/, Challenge *> ChallengeMap;
typedef std::unordered_map<uint64/*MemberGUID*/, ChallengeByMap> ChallengesOfMember;
typedef std::unordered_map<uint32/*guild*/, ChallengeByMap> GuildBestRecord;
typedef std::unordered_map<uint16/*map*/, uint32/*QuestCredit*/> QuestCreditForMap;
typedef std::unordered_map<uint8/*medal*/, uint32/*curency count*/> CurencyRewardMap;
class ChallengeMgr
{
        friend class ACE_Singleton<ChallengeMgr, ACE_Null_Mutex>;

        ChallengeMgr() : challengeGUID(0){};
        ~ChallengeMgr();

    public:
        void LoadFromDB();
        void SaveChallengeToDB(Challenge *c);

        uint32 GenerateChallengeID() { return ++challengeGUID; }
        void CheckBestMapId(Challenge *c);
        void CheckBestGuildMapId(Challenge *c);
        void CheckBestMemberMapId(uint64 guid, Challenge *c);

        void GroupReward(Map *instance, uint32 recordTime, ChallengeMode medal);

        Challenge * BestServerChallenge(uint16 map);
        Challenge * BestGuildChallenge(uint32 guildId, uint16 map);
        ChallengeByMap * BestForMember(uint64 guid)
        {
            ChallengesOfMember::iterator itr = m_ChallengesOfMember.find(guid);
            if (itr == m_ChallengesOfMember.end())
                return NULL;
            return &itr->second;
        }

        uint32 GetValorPointsReward(uint8 medal) { return m_valorPoints[medal]; }

    protected:
        uint32 challengeGUID;
        ChallengeMap m_ChallengeMap;
        ChallengesOfMember m_ChallengesOfMember;
        ChallengeByMap m_BestForMap;
        GuildBestRecord m_GuildBest;
        QuestCreditForMap m_reward;
        CurencyRewardMap m_valorPoints;
};

#define sChallengeMgr ACE_Singleton<ChallengeMgr, ACE_Null_Mutex>::instance()

#endif
