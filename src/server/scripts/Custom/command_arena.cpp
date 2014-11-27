#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"

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
            { "rbg",            SEC_GAMEMASTER,     false, &HandleArenaDelRbgCommand,           "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand arenaCommandTable[] =
        {
            { "del",            SEC_GAMEMASTER,     false, NULL,                                "", arenaDelCommandTable },
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
        return DelTeam(handler, args, 0);
    }

    static bool HandleArenaDel3x3Command(ChatHandler* handler, const char* args)
    {
        return DelTeam(handler, args, 1);
    }

    static bool HandleArenaDel5x5Command(ChatHandler* handler, const char* args)
    {
        return DelTeam(handler, args, 2);
    }

    static bool HandleArenaDelRbgCommand(ChatHandler* handler, const char* args)
    {
        return DelTeam(handler, args, 3);
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
        QueryResult bracketsResult = CharacterDatabase.PQuery("SELECT * FROM character_brackets_info WHERE guid = '%u' AND `bracket` = '%u'", target_guid, type);
        if (bracketsResult)
        {
            CharacterDatabase.PQuery("DELETE FROM character_brackets_info WHERE guid = '%u' AND `bracket` = '%u'", target_guid, type);
            handler->PSendSysMessage("Player: \"%s\" was deleted bracket %u stats.", target_name.c_str(), type);
        }

        handler->PSendSysMessage("Cant find player bracket %u stats.", type);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleCloseSeasonCommand(ChatHandler* handler, const char* args)
    {
        bool debugOnly = false;
        if(!debugOnly)
            sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_START);
        sLog->outArena("Окончание сезона Ноябрь 2014");
        // by default:
        // 0 type - top 6 char
        // 2c2
        {
            uint32 firstWinTypeCount = 6;
            QueryResult teamResult = CharacterDatabase.PQuery("SELECT ch.guid, ch.name, wins, games, rating FROM `character_brackets_info` cbi LEFT JOIN characters ch ON ch.guid = cbi.guid WHERE `bracket` = 0 ORDER BY rating DESC LIMIT %u", firstWinTypeCount);
            if (teamResult && teamResult->GetRowCount())
            {
                uint32 count = 1;
                do
                {
                    Field* Fields = teamResult->Fetch();
                    uint64 guid = Fields[0].GetUInt64();
                    std::string name = Fields[1].GetString();
                    uint32 wins = Fields[2].GetUInt32();
                    uint32 games = Fields[3].GetUInt32();
                    uint32 rating = Fields[4].GetUInt32();

                    if(!debugOnly)
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','150')", guid, 38186);

                    sLog->outArena("За %u место в топе 2на2 получил %s(%u) 150 эфира, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    count++;
                }
                while (teamResult->NextRow());
            }
        }

        // by default:
        // 1 - top offlokie 0.1%
        // 2 - top offlokie 0.5%
        // 3 - top offlokie 3%
        // 4 - top offlokie 10%
        // 5 - top offlokie 35%
        // 3c3
        {
            if(QueryResult arenaWinner = CharacterDatabase.PQuery("SELECT ch.guid, ch.name, wins, games, rating FROM `character_brackets_info` cbi LEFT JOIN characters ch ON ch.guid = cbi.guid WHERE `bracket` = 1 and rating > 1000 AND wins > 49 ORDER BY rating DESC"))
            {
                uint32 count = 1;
                uint32 playerCount = arenaWinner->GetRowCount();
                uint32 firstWinTypeCount = uint32(playerCount * 0.001f);
                if(firstWinTypeCount < 3)
                    firstWinTypeCount = 3;
                uint32 secondWinTypeCount = uint32(playerCount * 0.005f);
                if(secondWinTypeCount < 3)
                    secondWinTypeCount = 3;
                uint32 thirdWinTypeCount  = uint32(playerCount * 0.03f);
                if(thirdWinTypeCount < 3)
                    thirdWinTypeCount = 3;
                uint32 fothWinTypeCount  = uint32(playerCount * 0.1f);
                if(fothWinTypeCount < 3)
                    fothWinTypeCount = 3;
                uint32 fifthWinTypeCount  = uint32(playerCount * 0.35f);
                if(fifthWinTypeCount < 3)
                    fifthWinTypeCount = 3;

                sLog->outArena("Количество чаров %u первая %u, вторая %u, третья %u, четвертая %u, петая %u", playerCount, firstWinTypeCount, secondWinTypeCount, thirdWinTypeCount, fothWinTypeCount, fifthWinTypeCount);

                do
                {
                    Field* Fields = arenaWinner->Fetch();
                    uint64 guid = Fields[0].GetUInt64();
                    std::string name = Fields[1].GetString();
                    uint32 wins = Fields[2].GetUInt32();
                    uint32 games = Fields[3].GetUInt32();
                    uint32 rating = Fields[4].GetUInt32();

                    //Season achivement Гладиатор / Gladiator
                    if(count <= firstWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 95041);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','200')", guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 8666);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 343);
                        }

                        sLog->outArena("За %u место в топе 3на3(0.1) получил %s(%u) efir=200, item=95041, achiv=8666, title=343, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Гладиатор / Gladiator
                    if(count <= secondWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2091);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 42);
                        }

                        sLog->outArena("За %u место в топе 3на3(0.5) получил %s(%u) efir=250, achiv=2091, title=42, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Дуэлянт / Duelist
                    if(count <= thirdWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2092);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 43);
                        }

                        sLog->outArena("За %u место в топе 3на3(3.0) получил %s(%u) achiv=2092, title=43, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Фаворит / Rival
                    if(count <= fothWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        }

                        sLog->outArena("За %u место в топе 3на3(10.0) получил %s(%u) achiv=2093, title=44, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Претендент / Challenger
                    if(count <= fifthWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        }

                        sLog->outArena("За %u место в топе 3на3(35.0) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }
                    count++;
                }
                while (arenaWinner->NextRow());
            }
        }

        // by default:
        // 1 - top offlokie 0.1%
        // 2 - top offlokie 0.5%
        // 3 - top offlokie 3%
        // 4 - top offlokie 10%
        // 5 - top offlokie 35%
        // 5c5
        {
            if(QueryResult arenaWinner = CharacterDatabase.PQuery("SELECT ch.guid, ch.name, wins, games, rating FROM `character_brackets_info` cbi LEFT JOIN characters ch ON ch.guid = cbi.guid WHERE `bracket` = 2 and rating > 1000 AND wins > 49 ORDER BY rating DESC"))
            {
                uint32 count = 1;
                uint32 playerCount = arenaWinner->GetRowCount();
                uint32 firstWinTypeCount = uint32(playerCount * 0.001f);
                if(firstWinTypeCount < 5)
                    firstWinTypeCount = 5;
                uint32 secondWinTypeCount = uint32(playerCount * 0.005f);
                if(secondWinTypeCount < 5)
                    secondWinTypeCount = 5;
                uint32 thirdWinTypeCount  = uint32(playerCount * 0.03f);
                if(thirdWinTypeCount < 5)
                    thirdWinTypeCount = 5;
                uint32 fothWinTypeCount  = uint32(playerCount * 0.1f);
                if(fothWinTypeCount < 5)
                    fothWinTypeCount = 5;
                uint32 fifthWinTypeCount  = uint32(playerCount * 0.35f);
                if(fifthWinTypeCount < 5)
                    fifthWinTypeCount = 5;

                do
                {
                    Field* Fields = arenaWinner->Fetch();
                    uint64 guid = Fields[0].GetUInt64();
                    std::string name = Fields[1].GetString();
                    uint32 wins = Fields[2].GetUInt32();
                    uint32 games = Fields[3].GetUInt32();
                    uint32 rating = Fields[4].GetUInt32();

                    //Season achivement Гладиатор / Gladiator
                    if(count <= firstWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 95041);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','200')", guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 8666);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 343);
                        }

                        sLog->outArena("За %u место в топе 5на5(0.1) получил %s(%u) efir=200, item=95041, achiv=8666, title=343, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Гладиатор / Gladiator
                    if(count <= secondWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2091);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 42);
                        }

                        sLog->outArena("За %u место в топе 5на5(0.5) получил %s(%u) efir=250, achiv=2091, title=42, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Дуэлянт / Duelist
                    if(count <= thirdWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2092);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 43);
                        }

                        sLog->outArena("За %u место в топе 5на5(3.0) получил %s(%u) achiv=2092, title=43, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Фаворит / Rival
                    if(count <= fothWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        }

                        sLog->outArena("За %u место в топе 5на5(10.0) получил %s(%u) achiv=2093, title=44, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Претендент / Challenger
                    if(count <= fifthWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        }

                        sLog->outArena("За %u место в топе 5на5(35.0) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }
                    count++;
                }
                while (arenaWinner->NextRow());
            }
        }

        // by default:
        // 1 - top offlokie 0.5%
        // RBG
        {
            if(QueryResult arenaWinner = CharacterDatabase.PQuery("SELECT ch.guid, ch.name, wins, games, rating, ch.race FROM `character_brackets_info` cbi LEFT JOIN characters ch ON ch.guid = cbi.guid WHERE `bracket` = 3 and rating > 1000 AND wins > 49 ORDER BY rating DESC"))
            {
                uint32 count = 1;
                uint32 playerCount = arenaWinner->GetRowCount();
                uint32 firstWinTypeCount = uint32(playerCount * 0.005f);
                if(firstWinTypeCount < 10)
                    firstWinTypeCount = 10;

                do
                {
                    Field* Fields = arenaWinner->Fetch();
                    uint64 guid = Fields[0].GetUInt64();
                    std::string name = Fields[1].GetString();
                    uint32 wins = Fields[2].GetUInt32();
                    uint32 games = Fields[3].GetUInt32();
                    uint32 rating = Fields[4].GetUInt32();
                    uint32 team = Player::TeamForRace(Fields[5].GetUInt8());
                    uint32 achivement = team == HORDE ? 6941 : 6942;

                    //Season achivement Герой Орды / Hero of the Horde / Hero of the Alliance / Герой Альянса
                    if(count <= firstWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", guid, 38186);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, achivement);
                        }

                        sLog->outArena("За %u место в топе РБГ(0.5) получил %s(%u) efir=250, achiv=%u побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, achivement, wins, games, rating);
                    }
                    count++;
                }
                while (arenaWinner->NextRow());
            }
        }

        if(!debugOnly)
            sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_END);
        return true;
    }
};

void AddSC_command_arena()
{
    new command_arena();
}
