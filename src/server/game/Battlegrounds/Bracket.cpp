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

#include "Bracket.h"
#include "DatabaseEnv.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"

Bracket::Bracket(uint64 guid, BracketType type) :
    m_owner(guid), m_Type(type), m_rating(0), m_state(BRACKET_NEW), m_ratingLastChange(0), m_mmr_lastChage(0)
{
    m_mmv    = sWorld->getIntConfig(CONFIG_ARENA_START_MATCHMAKER_RATING);
    memset(values, 0, sizeof(uint32) * BRACKET_END);
}

void Bracket::InitStats(uint16 rating, uint16 mmr, uint32 games, uint32 wins, uint32 week_games, uint32 week_wins, uint16 best_week, uint16 best)
{
    m_rating = rating;
    m_mmv = mmr;

    values[BRACKET_SEASON_GAMES] = games;
    values[BRACKET_SEASON_WIN] = wins;
    values[BRACKET_WEEK_GAMES] = week_games;
    values[BRACKET_WEEK_WIN] = week_wins;
    values[BRACKET_WEEK_BEST] = best_week;
    values[BRACKET_BEST] = best;

    m_state = BRACKET_UNCHANGED;
}

float GetChanceAgainst(int ownRating, int opponentRating)
{
    // Returns the chance to win against a team with the given rating, used in the rating adjustment calculation
    // ELO system
    return 1.0f / (1.0f + exp(log(10.0f) * (float)((float)opponentRating - (float)ownRating) / 300.0f));
}

int GetRatingMod(int ownRating, int opponentRating, bool won /*, float confidence_factor*/)
{
    // 'Chance' calculation - to beat the opponent
    // This is a simulation. Not much info on how it really works
    float chance = GetChanceAgainst(ownRating, opponentRating);
    float won_mod = (won) ? 1.0f : 0.0f;

    // Calculate the rating modification
    float mod;

    // TODO: Replace this hack with using the confidence factor (limiting the factor to 2.0f)
    if (won && ownRating < 1300)
    {
        if (ownRating < 1000)
            mod = 96.0f * (won_mod - chance);
        else
            mod = (48.0f + (48.0f * (1300.0f - float(ownRating)) / 300.0f)) * (won_mod - chance);
    }
    else
    {
        float winbonus = 0.0f;

        if (won && chance < 0.4f)
        {
            winbonus = (1.0f - (chance / 0.4f)) * 24.0f;
        }

        mod = (24.0f + winbonus) * (won_mod - chance);
    }

	// in any way should be decrase
	if (!won && mod == 0.0f && ownRating > 0)
		return -1.0f;

    return (int)RoundingFloatValue(mod);

}


int GetMatchmakerRatingMod(int ownRating, int opponentRating, bool won )
{
    // 'Chance' calculation - to beat the opponent
    // This is a simulation. Not much info on how it really works
    float chance = GetChanceAgainst(ownRating, opponentRating);
    float won_mod = (won) ? 1.0f : 0.0f;
    float mod = won_mod - chance;

    // Work in progress:
    /*
    // This is a simulation, as there is not much info on how it really works
    float confidence_mod = min(1.0f - fabs(mod), 0.5f);

    // Apply confidence factor to the mod:
    mod *= confidence_factor

    // And only after that update the new confidence factor
    confidence_factor -= ((confidence_factor - 1.0f) * confidence_mod) / confidence_factor;
    */

    // Real rating modification
    mod *= 48.0f;

    return (int)RoundingFloatValue(mod);
}

void Bracket::SaveStats(SQLTransaction* trans)
{
    int32 index = 0;
    PreparedStatement* stmt = NULL;

    switch(m_state)
    {
        case BRACKET_NEW:
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHARACTER_BRACKETS_STATS);
            stmt->setUInt32(index++, GUID_LOPART(m_owner));
            stmt->setUInt8(index++, m_Type);
            stmt->setUInt16(index++, m_rating);
            stmt->setUInt16(index++, values[BRACKET_BEST]);
            stmt->setUInt16(index++, values[BRACKET_WEEK_BEST]);
            stmt->setUInt16(index++, m_mmv);
            stmt->setUInt32(index++, values[BRACKET_SEASON_GAMES]);
            stmt->setUInt32(index++, values[BRACKET_SEASON_WIN]);
            stmt->setUInt32(index++, values[BRACKET_WEEK_GAMES]);
            stmt->setUInt32(index++, values[BRACKET_WEEK_WIN]);
            break;
        case BRACKET_CHANGED:
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BRACKETS_STATS);
            stmt->setUInt16(index++, m_rating);
            stmt->setUInt16(index++, values[BRACKET_BEST]);
            stmt->setUInt16(index++, values[BRACKET_WEEK_BEST]);
            stmt->setUInt16(index++, m_mmv);
            stmt->setUInt32(index++, values[BRACKET_SEASON_GAMES]);
            stmt->setUInt32(index++, values[BRACKET_SEASON_WIN]);
            stmt->setUInt32(index++, values[BRACKET_WEEK_GAMES]);
            stmt->setUInt32(index++, values[BRACKET_WEEK_WIN]);
            stmt->setUInt32(index++, GUID_LOPART(m_owner));
            stmt->setUInt8(index++, m_Type);
            break;
        default:
            //Do nothing.
            return;
    }

    if (trans)
        (*trans)->Append(stmt);
    else
        CharacterDatabase.Execute(stmt);

    m_state = BRACKET_UNCHANGED;
}

uint16 Bracket::FinishGame(bool win, uint16 opponents_mmv)
{
    values[BRACKET_SEASON_GAMES]++;
    values[BRACKET_WEEK_GAMES]++;

    if (win)
    {
        values[BRACKET_SEASON_WIN]++;
        values[BRACKET_WEEK_WIN]++;
    }

    m_ratingLastChange = (win) ? WonAgainst(opponents_mmv) : LostAgainst(opponents_mmv);
    m_rating += m_ratingLastChange;
    m_mmr_lastChage = GetMatchmakerRatingMod(m_mmv, opponents_mmv, win);
    m_mmv += m_mmr_lastChage;

    if (m_rating > values[BRACKET_WEEK_BEST])
        values[BRACKET_WEEK_BEST] = m_rating;

    if (m_rating > values[BRACKET_BEST])
        values[BRACKET_BEST] = m_rating;
    
    if (Player* player = ObjectAccessor::FindPlayer(m_owner))
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING, m_rating, GetSlotByType());

    // this should be done at week reset
    //if (Player* player = ObjectAccessor::FindPlayer(m_owner))
    //{
    //    if (m_rating > player->GetMaxPersonalArenaRatingRequirement(BRACKET_TYPE_ARENA_2))
    //        player->UpdateConquestCurrencyCap(CURRENCY_TYPE_CONQUEST_META_ARENA);        
    //}

    if (m_state == BRACKET_UNCHANGED)
        m_state = BRACKET_CHANGED;

    SaveStats();

    return m_ratingLastChange;
}

uint16 Bracket::GetSlotByType()
{
    switch (m_Type)
    {
        case BRACKET_TYPE_ARENA_2: return 2;
        case BRACKET_TYPE_ARENA_3: return 3;
        case BRACKET_TYPE_ARENA_5: return 5;
        default:
            break;
    }
    return 0xFF;
}

int16 Bracket::WonAgainst(uint16 opponents_mmv)
{
    return GetRatingMod(m_rating, opponents_mmv, true);
}

int16 Bracket::LostAgainst(uint16 opponents_mmv)
{
    return GetRatingMod(m_rating, opponents_mmv, false);
}
