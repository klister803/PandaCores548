#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "ArenaTeamMgr.h"

class command_arena : public CommandScript
{
public:
    command_arena() : CommandScript("command_arena") { }

    ChatCommand* GetCommands() const
    {

        static ChatCommand arenaDelCommandTable[] =
        {
            { "2x2",            SEC_GAMEMASTER,     false, &HandleArenaDel2x2Command,           "", NULL },
            { "3x3",            SEC_GAMEMASTER,     false, &HandleArenaDel3x3Command,           "", NULL },
            { "5x5",            SEC_GAMEMASTER,     false, &HandleArenaDel5x5Command,           "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand arenaCommandTable[] =
        {
            { "del",            SEC_GAMEMASTER,     false, NULL,                                "", arenaDelCommandTable },
            { "delid",          SEC_GAMEMASTER,     false, &HandleArenaDelIdCommand,            "", NULL },
            { "closeseason",    SEC_ADMINISTRATOR,  false, &HandleCloseSeasonCommand,           "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand customCommandTable[] =
        {
            { "arenasliver",    SEC_GAMEMASTER,     false, &HandleArenaSliverCommand,           "", NULL },
            { "unarenasliver",  SEC_GAMEMASTER,     false, &HandleUnArenaSliverCommand,         "", NULL },
            { "lootcleanid",    SEC_GAMEMASTER,     false, &HandleLootCleanIdCommand,           "", NULL },
            { "unlootcleanid",  SEC_GAMEMASTER,     false, &HandleUnLootCleanIdCommand,         "", NULL },
            { "recoveryitem",   SEC_ADMINISTRATOR,  false, &HandleRecoveryItemCommand,          "", NULL },
            { "unrecoveryitem", SEC_ADMINISTRATOR,  false, &HandleUnRecoveryItemCommand,        "", NULL },
            { "listname",       SEC_GAMEMASTER,     false, &HandleListChangeName,               "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand commandTable[] =
        {
            { "custom",         SEC_GAMEMASTER,     false, NULL,                                "", customCommandTable },
            { "arena",          SEC_GAMEMASTER,     false, NULL,                                "", arenaCommandTable },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        return commandTable;
    }

    static bool HandleArenaDel2x2Command(ChatHandler* handler, const char* args)
    {
        return DelTeam(handler, args, 2);
    }

    static bool HandleArenaDel3x3Command(ChatHandler* handler, const char* args)
    {
        return DelTeam(handler, args, 3);
    }

    static bool HandleArenaDel5x5Command(ChatHandler* handler, const char* args)
    {
        return DelTeam(handler, args, 5);
    }

    static bool HandleArenaDelIdCommand(ChatHandler* handler, const char* args)
    {
        char* TeamId = strtok((char*)args, " ");
        uint32 IdTeam = atoi(TeamId);
        if(!IdTeam)
            return false;
        ArenaTeam *aTeam = sArenaTeamMgr->GetArenaTeamById(IdTeam);
        if (aTeam)
        {
            aTeam->Disband(NULL);
            handler->PSendSysMessage("Team: \"%s\" was deleted.", aTeam->GetName().c_str());
            return true;
        }
        return true;
    }

    static bool HandleArenaSliverCommand(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        CharacterDatabase.PQuery("insert into `character_reward` (`owner_guid`,`type`)value ('%u','8');", target_guid);
        std::string nameLink = handler->playerLink(target_name);
        handler->PSendSysMessage(20023, nameLink.c_str());
        return true;
    }

    static bool HandleUnArenaSliverCommand(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        CharacterDatabase.PQuery("DELETE FROM `character_reward` WHERE `type` = 8 AND `owner_guid` = '%u';", target_guid);
        std::string nameLink = handler->playerLink(target_name);
        handler->PSendSysMessage(20025, nameLink.c_str());
        return true;
    }

    static bool HandleUnLootCleanIdCommand(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        char* tail = strtok(NULL, "");

        // get from tail next item str
        while (char* itemStr = strtok(tail, " "))
        {
            // and get new tail
            tail = strtok(NULL, "");

            // parse item str
            char* itemIdStr = strtok(itemStr, ":");
            char* itemCountStr = strtok(NULL, " ");

            uint32 item_id = atoi(itemIdStr);
            if (!item_id)
                return false;
            ItemTemplate const* item_proto = sObjectMgr->GetItemTemplate(item_id);
            if (!item_proto)
                return false;

            uint32 item_count = itemCountStr ? atoi(itemCountStr) : 1;

            handler->PSendSysMessage(20026, item_id, item_count);
            CharacterDatabase.PQuery("DELETE FROM `character_reward` WHERE `type` = 9 AND `owner_guid` = '%u' AND `id` = %u AND `count` = %u;", target_guid, item_id, item_count);
        }
        return true;
    }

    static bool HandleLootCleanIdCommand(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        char* tail = strtok(NULL, "");

        // get from tail next item str
        while (char* itemStr = strtok(tail, " "))
        {
            // and get new tail
            tail = strtok(NULL, "");

            // parse item str
            char* itemIdStr = strtok(itemStr, ":");
            char* itemCountStr = strtok(NULL, " ");

            uint32 item_id = atoi(itemIdStr);
            if (!item_id)
                return false;
            ItemTemplate const* item_proto = sObjectMgr->GetItemTemplate(item_id);
            if (!item_proto)
                return false;

            uint32 item_count = itemCountStr ? atoi(itemCountStr) : 1;

            //sLog->outError("HandleLootCleanIdCommand item %u, target_guid %u, item_count %u", item_id, target_guid, item_count);

            handler->PSendSysMessage(20024, item_id, item_count);
            CharacterDatabase.PQuery("insert into `character_reward` (`owner_guid`,`type`,`id`,`count`)value ('%u','9','%u','%u');", target_guid, item_id, item_count);
        }

        std::string nameLink = handler->playerLink(target_name);
        handler->PSendSysMessage(20022, nameLink.c_str());
        return true;
    }

    static bool HandleUnRecoveryItemCommand(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        char* tail = strtok(NULL, "");

        // get from tail next item str
        while (char* itemStr = strtok(tail, " "))
        {
            // and get new tail
            tail = strtok(NULL, "");

            // parse item str
            char* itemIdStr = strtok(itemStr, ":");
            char* itemCountStr = strtok(NULL, " ");

            uint32 item_id = atoi(itemIdStr);
            if (!item_id)
                return false;
            ItemTemplate const* item_proto = sObjectMgr->GetItemTemplate(item_id);
            if (!item_proto)
                return false;

            uint32 item_count = itemCountStr ? atoi(itemCountStr) : 1;

            handler->PSendSysMessage(20026, item_id, item_count);
            CharacterDatabase.PQuery("DELETE FROM `character_reward` WHERE `type` = 10 AND `owner_guid` = '%u' AND `id` = %u AND `count` = %u;", target_guid, item_id, item_count);
        }
        return true;
    }

    static bool HandleListChangeName(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        QueryResult result;
        if(target)
            result = CharacterDatabase.PQuery("SELECT * FROM character_rename WHERE guid = %u", target->GetGUIDLow());
        else if(target_guid)
            result = CharacterDatabase.PQuery("SELECT * FROM character_rename WHERE guid = %u", target_guid);
        else
            result = CharacterDatabase.PQuery("SELECT * FROM character_rename WHERE newname = '%s' or oldname = '%s'", target_name.c_str(), target_name.c_str());

        if(result)
        {
            do
            {
                Field * fetch = result->Fetch();
                uint32 guid = fetch[0].GetUInt32();
                uint32 account = fetch[1].GetUInt32();
                std::string newname = fetch[2].GetString();
                std::string oldname = fetch[3].GetString();
                std::string date = fetch[4].GetString();

                handler->PSendSysMessage(20029, account, guid, newname.c_str(), oldname.c_str(), date.c_str());
            } while ( result->NextRow() );
        }
        else
            handler->PSendSysMessage("Cant find change name.");
        return true;
    }

    static bool HandleRecoveryItemCommand(ChatHandler* handler, const char* args)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        char* tail = strtok(NULL, "");

        // get from tail next item str
        while (char* itemStr = strtok(tail, " "))
        {
            // and get new tail
            tail = strtok(NULL, "");

            // parse item str
            char* itemIdStr = strtok(itemStr, ":");
            char* itemCountStr = strtok(NULL, " ");

            uint32 item_id = atoi(itemIdStr);
            if (!item_id)
                return false;
            ItemTemplate const* item_proto = sObjectMgr->GetItemTemplate(item_id);
            if (!item_proto)
                return false;

            uint32 item_count = itemCountStr ? atoi(itemCountStr) : 1;

            //sLog->outError("HandleLootCleanIdCommand item %u, target_guid %u, item_count %u", item_id, target_guid, item_count);

            handler->PSendSysMessage(20024, item_id, item_count);
            CharacterDatabase.PQuery("insert into `character_reward` (`owner_guid`,`type`,`id`,`count`)value ('%u','10','%u','%u');", target_guid, item_id, item_count);
        }

        std::string nameLink = handler->playerLink(target_name);
        handler->PSendSysMessage(20022, nameLink.c_str());
        return true;
    }

    static bool DelTeam(ChatHandler* handler, const char* args, uint8 type)
    {
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        //sLog->outString("giud: %u", target_guid);
        QueryResult memberResult = CharacterDatabase.PQuery("SELECT arenaTeamId FROM arena_team_member WHERE guid = %u", target_guid);

        if (memberResult)
        {
            do {
                Field* memberFields = memberResult->Fetch();
                uint32 teamId = memberFields[0].GetUInt32();
                QueryResult teamResult = CharacterDatabase.PQuery("SELECT type FROM arena_team WHERE arenaTeamId = %u", teamId);
                if (teamResult)
                {
                    Field* arenaFields = teamResult->Fetch();
                    uint8 tmpType = arenaFields[0].GetUInt8();

                    if (tmpType == type)
                    {
                        ArenaTeam *aTeam = sArenaTeamMgr->GetArenaTeamById(teamId);
                        if (aTeam)
                        {
                            aTeam->Disband(NULL);
                            handler->PSendSysMessage("Team: \"%s\" was deleted.", aTeam->GetName().c_str());
                            return true;
                        }
                    }
                }
            }
            while (memberResult->NextRow());
        }

        handler->PSendSysMessage("Cant find player or team.");
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool IsTeamMemberActive(ArenaTeamMember member, ArenaTeam* team)
    {
        if ( ((float)member.SeasonGames / team->GetStats().SeasonGames * 100) >= 50 && int32(team->GetStats().Rating) - int32(member.PersonalRating) < 200)
            return true;

        return false;
    }

    static bool IsTeamActive(ArenaTeam* team)
    {
        if (team->GetStats().SeasonGames < 25)
            return false;

        for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
        {
            ArenaTeamMember member = *pItr;
            // reward players which has 70% games
            if (IsTeamMemberActive(member, team))
                return true;
        }

        return false;
    }

    static bool HandleCloseSeasonCommand(ChatHandler* handler, const char* args)
    {
        sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_START);
        sLog->outArena("ARENA_CLOSESEASON_START");
        // by default:
        // 1 type - top 3 team
        // 2c2
        {
            uint32 firstWinTypeCount = 3;

            QueryResult teamResult = CharacterDatabase.PQuery("SELECT arenaTeamId FROM arena_team WHERE type = 2 ORDER BY rating DESC LIMIT %u", firstWinTypeCount);
            if (teamResult && teamResult->GetRowCount())
            {
                do
                {
                    Field* arenaFields = teamResult->Fetch();
                    uint64 teamId = arenaFields[0].GetUInt64();

                    ArenaTeam *team = sArenaTeamMgr->GetArenaTeamById(teamId);
                    if (!team)
                        continue;

                    //sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_TEAM_WIN, team->GetName().c_str());
                    for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
                    {
                        ArenaTeamMember member = *pItr;
                        // reward players which has 80% games
                        if ( ((float)member.SeasonGames / team->GetStats().SeasonGames * 100) >= 70 )
                        {
                            //sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_PLAYER_WIN, member.Name.c_str());
                            // Send Item
                            //std::string commandArg = member.Name;
                            //commandArg += " \"Season End 2v2\" \"Congratulations, you get the 150 Ethereal Credit!\"";
                            //handler->SendItemsCommand(commandArg.c_str());
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','150')", member.Guid, 38186);

                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','150');", member.Guid, 38186);
                        }
                    }
                }
                while (teamResult->NextRow());
            }
        }

        // 4 winners type
        // by default:
        // 1 type - top 0.5%
        // 2 type - top 3%
        // 3 type - top 10%
        // 3 type - top 35%
        // 3c3
        {
            uint64 teamsCount = 0;
            QueryResult _qteamcount = CharacterDatabase.Query("SELECT count(*) FROM arena_team WHERE type = 3 and rating > 2000");
            if (_qteamcount)
                teamsCount = (*_qteamcount)[0].GetUInt32();
            uint32 firstWinTypeCount  = 1;
            uint32 secondWinTypeCount = uint32(teamsCount * 0.1f);
            if(secondWinTypeCount <= 0)
                secondWinTypeCount = 1;
            uint32 thirdWinTypeCount  = uint32(teamsCount * 0.3f);
            if(thirdWinTypeCount <= 0)
                thirdWinTypeCount = 1;
            uint32 fothWinTypeCount  = uint32(teamsCount * 0.5f);
            if(fothWinTypeCount <= 0)
                fothWinTypeCount = 1;

            uint32 fifthWinTypeCount = teamsCount;

            if(fothWinTypeCount < firstWinTypeCount)
            {
                secondWinTypeCount = firstWinTypeCount;
                thirdWinTypeCount = firstWinTypeCount;
                fothWinTypeCount = firstWinTypeCount;
                fifthWinTypeCount = firstWinTypeCount;
            }

            std::list<ArenaTeam*> firstTypeWinners;
            std::list<ArenaTeam*> secondTypeWinners;
            std::list<ArenaTeam*> thirdTypeWinners;
            std::list<ArenaTeam*> fothTypeWinners;
            std::list<ArenaTeam*> fifthTypeWinners;

            QueryResult teamResult = CharacterDatabase.PQuery("SELECT arenaTeamId FROM arena_team WHERE type = 3 and rating > 2000 ORDER BY rating DESC LIMIT %u", fothWinTypeCount);
            if (teamResult && teamResult->GetRowCount())
            {
                uint32 i = 0;
                do
                {
                    Field* arenaFields = teamResult->Fetch();
                    uint64 teamId = arenaFields[0].GetUInt64();

                    ArenaTeam *team = sArenaTeamMgr->GetArenaTeamById(teamId);
                    if (!team || !IsTeamActive(team))
                        continue;

                    if (i < firstWinTypeCount)
                    {
                        firstTypeWinners.push_back(team);
                    }
                    if (i < secondWinTypeCount)
                    {
                        secondTypeWinners.push_back(team);
                    }
                    if (i < thirdWinTypeCount)
                    {
                        thirdTypeWinners.push_back(team);
                    }
                    if (i < fothWinTypeCount)
                    {
                        fothTypeWinners.push_back(team);
                    }
                    if (i < fifthWinTypeCount)
                    {
                        fifthTypeWinners.push_back(team);
                    }

                    i++;
                }
                while (teamResult->NextRow());
            }

            if (firstTypeWinners.size())
            {
                int playersCount = 0;
                for (std::list<ArenaTeam*>::iterator teamItr = firstTypeWinners.begin(); teamItr != firstTypeWinners.end(); teamItr++)
                {
                    ArenaTeam *team = *teamItr;
                    if (!team)
                        continue;
                    //sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_TEAM_WIN, team->GetName().c_str());
                    for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
                    {
                        ArenaTeamMember member = *pItr;
                        if ( IsTeamMemberActive(member, team) )
                        {
                            //sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_PLAYER_WIN, member.Name.c_str());
                            //std::string commandArg = member.Name;
                            //commandArg += " \"Realm Strongest Team\" \"Congratulations, arena season is over and your team is Top 1 in realm!\"";
                            //handler->SendItemsCommand(commandArg.c_str());
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", member.Guid, 71954);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','200')", member.Guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 6124);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 280);

                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", member.Guid, 71954);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','200')", member.Guid, 38186);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 6124);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 280);

                            playersCount++;
                        }
                    }
                }
            }

            if (secondTypeWinners.size())
            {
                int playersCount = 0;
                for (std::list<ArenaTeam*>::iterator teamItr = secondTypeWinners.begin(); teamItr != secondTypeWinners.end(); teamItr++)
                {
                    ArenaTeam *team = *teamItr;
                    if (!team)
                        continue;
                    for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
                    {
                        ArenaTeamMember member = *pItr;
                        if ( IsTeamMemberActive(member, team) )
                        {
                            // Send Mail
                            //std::string commandArg = member.Name;
                            //commandArg += " \"Season End 3v3\" \"Congratulations, you receive the title of Gladiator!\"";
                            //handler->SendItemsCommand(commandArg.c_str());
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", member.Guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2091);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 42);

                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", member.Guid, 38186);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2091);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 42);
                            playersCount++;
                        }
                    }
                }
            }

            if (thirdTypeWinners.size())
            {
                int playersCount = 0;
                for (std::list<ArenaTeam*>::iterator teamItr = thirdTypeWinners.begin(); teamItr != thirdTypeWinners.end(); teamItr++)
                {
                    ArenaTeam *team = *teamItr;
                    if (!team)
                        continue;
                    for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
                    {
                        ArenaTeamMember member = *pItr;
                        if ( IsTeamMemberActive(member, team) )
                        {
                            // Send Mail
                            //std::string commandArg = member.Name;
                            //commandArg += " \"Season End 3v3\" \"Congratulations, you receive the title of Duelist!\"";
                            //handler->SendItemsCommand(commandArg.c_str());
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2092);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 43);

                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2092);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 43);
                            playersCount++;
                        }
                    }
                }
            }

            if (fothTypeWinners.size())
            {
                int playersCount = 0;
                for (std::list<ArenaTeam*>::iterator teamItr = fothTypeWinners.begin(); teamItr != fothTypeWinners.end(); teamItr++)
                {
                    ArenaTeam *team = *teamItr;
                    if (!team)
                        continue;
                    for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
                    {
                        ArenaTeamMember member = *pItr;
                        if ( IsTeamMemberActive(member, team) )
                        {
                            // Send Mail
                            //std::string commandArg = member.Name;
                            //commandArg += " \"Season End 3v3\" \"Congratulations, you receive the title of Rival!\"";
                            //handler->SendItemsCommand(commandArg.c_str());
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2093);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 44);

                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2093);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 44);
                            playersCount++;
                        }
                    }
                }
            }

            if (fifthTypeWinners.size())
            {
                int playersCount = 0;
                for (std::list<ArenaTeam*>::iterator teamItr = fifthTypeWinners.begin(); teamItr != fifthTypeWinners.end(); teamItr++)
                {
                    ArenaTeam *team = *teamItr;
                    if (!team)
                        continue;
                    for (ArenaTeam::MemberList::iterator pItr = team->m_membersBegin(); pItr != team->m_membersEnd(); ++pItr)
                    {
                        ArenaTeamMember member = *pItr;
                        if ( IsTeamMemberActive(member, team) )
                        {
                            // Send Mail
                            //std::string commandArg = member.Name;
                            //commandArg += " \"Season End 3v3\" \"Congratulations, you receive the title of Challenger!\"";
                            //handler->SendItemsCommand(commandArg.c_str());
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2090);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 45);

                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", member.Guid, 2090);
                            sLog->outArena("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", member.Guid, 45);
                            playersCount++;
                        }
                    }
                }
            }
        }

        sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_END);
        return true;
    }
};

void AddSC_command_arena()
{
    new command_arena();
}
