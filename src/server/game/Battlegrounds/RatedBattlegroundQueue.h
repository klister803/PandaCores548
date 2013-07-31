/*
 * Copyright (C)   2013    uWoW <http://uwow.biz>
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef __RATEDBATTLEGROUNDQUEUE_H
#define __RATEDBATTLEGROUNDQUEUE_H

struct GroupQueueInfo;

class RatedBattlegroundQueue
{
    friend class ACE_Singleton<RatedBattlegroundQueue, ACE_Null_Mutex>;

private:
    RatedBattlegroundQueue();
    ~RatedBattlegroundQueue();

public:
    void Update();
    GroupQueueInfo* AddGroup(Player* leader);
    void RemovePlayer(uint64 player);

    bool GetQueueInfoByPlayer(uint64 playerGuid, GroupQueueInfo *result);
    GroupQueueInfo* GetQueueInfoByPlayer(uint64 playerGuid);

private:
    bool InviteGroup(GroupQueueInfo* ginfo, Battleground* bg, uint32 side);

    typedef std::set<GroupQueueInfo*> QueueStore;
    typedef UNORDERED_MAP<uint64, GroupQueueInfo*> PlayersQueueStore;

    QueueStore m_queueStore;
    PlayersQueueStore m_playersQueueStore;
};

#define sRBGQueue ACE_Singleton<RatedBattlegroundQueue, ACE_Null_Mutex>::instance()
#endif
