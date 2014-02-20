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


#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Bracket.h"

class rbg_commandscript : public CommandScript
{
public:
    rbg_commandscript() : CommandScript("rbg_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand commandTable[] =
        {
            { "rbg",            SEC_PLAYER,         false, &HandleRBGCommand,                  "", NULL },
            { NULL,             0,                  false, NULL,                               "", NULL }
        };
        return commandTable;
    }

    static bool HandleRBGCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        RatedBattleground* rbg = player->getBracket(BRACKET_TYPE_RATED_BG);
        handler->PSendSysMessage("Rating: %u;", rbg->getRating());
        handler->PSendSysMessage("Wins: %u; Total: %u;", rbg->getWeekWins(), rbg->getWeekGames());
        return true;
    }
};

void AddSC_rbg_commandscript()
{
    new rbg_commandscript();
}
