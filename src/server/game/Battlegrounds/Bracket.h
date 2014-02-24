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

#ifndef __BRACKET_H
#define __BRACKET_H

#include "Common.h"
#include "Player.h"

#define WORLD_STATE_ENABLE_RATED_BG 5508

class Bracket
{
public:
    Bracket(Player *plr, BracketType type);
    ~Bracket() {};

    void InitStats(uint16 rating, uint16 mmr, uint32 games, uint32 wins, uint32 week_games, uint32 week_wins, uint16 best_week, uint16 best);

    uint16 getRating() const { return m_rating; }
    uint16 getMMV()    const { return m_mmv;    }
    
    void SaveStats(SQLTransaction* trans = NULL);

    uint16 FinishGame(bool win, uint16 opponents_mmv);
    uint32 GetBracketInfo(BracketInfoType i) const { return values[i]; }
private:

    int16 WonAgainst(uint16 opponents_mmv);
    int16 LostAgainst(uint16 opponents_mmv);

    uint32 values[BRACKET_END];                 //used for store data from Player::PLAYER_FIELD_ARENA_TEAM_INFO_1_1

    uint16 m_rating;
    uint16 m_mmv;
    BracketType m_Type;

    uint64 m_owner;
};

#endif
