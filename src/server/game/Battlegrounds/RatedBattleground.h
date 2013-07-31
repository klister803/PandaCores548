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

#ifndef __RATEDBATTLEGROUND_H
#define __RATEDBATTLEGROUND_H

#include "Common.h"

enum RatedBattlegroundTypes
{
    RATED_BATTLEGROUND_TYPE_10X10,
    RATED_BATTLEGROUND_TYPE_15X15,
    RATED_BATTLEGROUND_TYPE_MAX
};

#define WORLD_STATE_ENABLE_RATED_BG 5508

class RatedBattleground
{
public:
    RatedBattleground(uint64 pGuid);
    ~RatedBattleground();

    uint16 getRating() { return m_rating; }
    uint16 getMMV()    { return m_mmv;    }

    uint32 getGames();
    uint32 getWins();
    uint32 getWeekGames();
    uint32 getWeekWins();

    void LoadStats();
    void SaveStats();

    uint16 FinishGame(bool win, uint16 opponents_mmv);

    static uint16 ConquestPointReward;

private:

    int16 WonAgainst(uint16 opponents_mmv);
    int16 LostAgainst(uint16 opponents_mmv);

    struct GamesStats
    {
        uint32 week_games;
        uint32 week_wins;
        uint32 games;
        uint32 wins;
    };

    uint16 m_rating;
    uint16 m_mmv;

    GamesStats m_gamesStats;

    uint64 m_owner;
};

#endif
