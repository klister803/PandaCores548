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

/* ScriptData
Name: guild_commandscript
%Complete: 100
Comment: All guild related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "../SharedPtrs/ClassFactory.h"

class guild_commandscript : public CommandScript
{
public:
    guild_commandscript() : CommandScript("guild_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand guildCommandTable[] =
        {
            { "create",         SEC_GAMEMASTER,     true,  &HandleGuildCreateCommand,           "", nullptr },
            { "delete",         SEC_GAMEMASTER,     true,  &HandleGuildDeleteCommand,           "", nullptr },
            { "invite",         SEC_GAMEMASTER,     true,  &HandleGuildInviteCommand,           "", nullptr },
            { "uninvite",       SEC_GAMEMASTER,     true,  &HandleGuildUninviteCommand,         "", nullptr },
            { "rank",           SEC_GAMEMASTER,     true,  &HandleGuildRankCommand,             "", nullptr },
            { "givexp",         SEC_GAMEMASTER,     true,  &HandleGuildXpCommand,               "", nullptr },
            { "levelup",        SEC_GAMEMASTER,     true,  &HandleGuildLevelUpCommand,          "", nullptr },
            { nullptr,          0,                  false, nullptr,                             "", nullptr }
        };
        static ChatCommand commandTable[] =
        {
            { "guild",          SEC_ADMINISTRATOR,  true, nullptr,                                 "", guildCommandTable },
            { nullptr,          0,                  false, nullptr,                                "", nullptr }
        };
        return commandTable;
    }

    /** \brief GM command level 3 - Create a guild.
     *
     * This command allows a GM (level 3) to create a guild.
     *
     * The "args" parameter contains the name of the guild leader
     * and then the name of the guild.
     *
     */
    static bool HandleGuildCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // if not guild name only (in "") then player name
        PlayerPtr target;
        if (!handler->extractPlayerTarget(*args != '"' ? (char*)args : nullptr, &target))
            return false;

        char* tailStr = *args != '"' ? strtok(nullptr, "") : (char*)args;
        if (!tailStr)
            return false;

        char* guildStr = handler->extractQuotedArg(tailStr);
        if (!guildStr)
            return false;

        std::string guildName = guildStr;

        if (target->GetGuildId())
        {
            handler->SendSysMessage(LANG_PLAYER_IN_GUILD);
            return true;
        }

        GuildPtr guild = ClassFactory::ConstructGuild();
        if (!guild->Create(target, guildName))
        {
            handler->SendSysMessage(LANG_GUILD_NOT_CREATED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        sGuildMgr->AddGuild(guild);

        return true;
    }

    static bool HandleGuildDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* guildStr = handler->extractQuotedArg((char*)args);
        if (!guildStr)
            return false;

        std::string guildName = guildStr;

        GuildPtr targetGuild = sGuildMgr->GetGuildByName(guildName);
        if (!targetGuild)
            return false;

        targetGuild->Disband();

        return true;
    }

    static bool HandleGuildInviteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // if not guild name only (in "") then player name
        uint64 targetGuid;
        if (!handler->extractPlayerTarget(*args != '"' ? (char*)args : nullptr, nullptr, &targetGuid))
            return false;

        char* tailStr = *args != '"' ? strtok(nullptr, "") : (char*)args;
        if (!tailStr)
            return false;

        char* guildStr = handler->extractQuotedArg(tailStr);
        if (!guildStr)
            return false;

        std::string guildName = guildStr;
        GuildPtr targetGuild = sGuildMgr->GetGuildByName(guildName);
        if (!targetGuild)
            return false;

        // player's guild membership checked in AddMember before add
        return targetGuild->AddMember(targetGuid);
    }

    static bool HandleGuildUninviteCommand(ChatHandler* handler, char const* args)
    {
        PlayerPtr target;
        uint64 targetGuid;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid))
            return false;

        uint32 guildId = target ? target->GetGuildId() : Player::GetGuildIdFromDB(targetGuid);
        if (!guildId)
            return false;

        GuildPtr targetGuild = sGuildMgr->GetGuildById(guildId);
        if (!targetGuild)
            return false;

        targetGuild->DeleteMember(targetGuid, false, true);
        return true;
    }

    static bool HandleGuildRankCommand(ChatHandler* handler, char const* args)
    {
        char* nameStr;
        char* rankStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &rankStr);
        if (!rankStr)
            return false;

        PlayerPtr target;
        uint64 targetGuid;
        std::string target_name;
        if (!handler->extractPlayerTarget(nameStr, &target, &targetGuid, &target_name))
            return false;

        uint32 guildId = target ? target->GetGuildId() : Player::GetGuildIdFromDB(targetGuid);
        if (!guildId)
            return false;

        GuildPtr targetGuild = sGuildMgr->GetGuildById(guildId);
        if (!targetGuild)
            return false;

        uint8 newRank = uint8(atoi(rankStr));
        return targetGuild->ChangeMemberRank(targetGuid, newRank);
    }

    static bool HandleGuildXpCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        GuildPtr targetGuild;
        uint32 amount = 0;

        if (*args == '"')
        {
            char* guildStr = handler->extractQuotedArg((char*)args);
            if (!guildStr)
                return false;

            targetGuild = sGuildMgr->GetGuildByName(guildStr);
        }
        else
        {
            if (!handler->getSelectedPlayer())
                return false;

            PlayerPtr target = handler->getSelectedPlayer();

            if (!target->GetGuildId())
                return false;

            targetGuild = sGuildMgr->GetGuildById(target->GetGuildId());
        }

        amount = (uint32)atoi(args);

        if (!targetGuild)
            return false;

        targetGuild->GiveXP(amount, handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL);
        return true;
    }

    static bool HandleGuildLevelUpCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        GuildPtr targetGuild;
        uint32 amount = 0;

        if (*args == '"')
        {
            char* guildStr = handler->extractQuotedArg((char*)args);
            if (!guildStr)
                return false;

            targetGuild = sGuildMgr->GetGuildByName(guildStr);
        }
        else
        {
            if (!handler->getSelectedPlayer())
                return false;

            PlayerPtr target = handler->getSelectedPlayer();

            if (!target->GetGuildId())
                return false;

            targetGuild = sGuildMgr->GetGuildById(target->GetGuildId());
        }

        amount = (uint32)atoi(args);

        if (!targetGuild)
            return false;

        uint32 experience = 0;

        for (uint8 i = 0; i < amount; ++i)
        {
            if (targetGuild->GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_MAX_LEVEL))
                break;

            experience = sGuildMgr->GetXPForGuildLevel(targetGuild->GetLevel()) - targetGuild->GetExperience();
            targetGuild->GiveXP(experience, handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL);
        }
        return true;
    }
};

void AddSC_guild_commandscript()
{
    new guild_commandscript();
}
