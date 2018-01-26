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
            { "checkitem",       SEC_GAMEMASTER,     false, &HandleCharacterItemsCheckCommand,               "", NULL },
            { "checkitembag",       SEC_GAMEMASTER,     false, &HandleCharacterItemsCheckBagCommand,               "", NULL },
            { "dmselect",       SEC_GAMEMASTER,     false, &HandleCharacterDMSelectCommand,               "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand logCommandTable[] =
        {
            { "addchar",        SEC_GAMEMASTER,     false, &HandleAddCharCommand,               "", NULL },
            { "removechar",     SEC_GAMEMASTER,     false, &HandleRemoveCharCommand,            "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand commandTable[] =
        {
            { "custom",         SEC_GAMEMASTER,     false, NULL,                                "", customCommandTable },
            { "arena",          SEC_GAMEMASTER,     false, NULL,                                "", arenaCommandTable },
            { "log",            SEC_GAMEMASTER,   false, NULL,                                  "", logCommandTable },
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

            //TC_LOG_ERROR("HandleLootCleanIdCommand item %u, target_guid %u, item_count %u", item_id, target_guid, item_count);

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

            //TC_LOG_ERROR("HandleLootCleanIdCommand item %u, target_guid %u, item_count %u", item_id, target_guid, item_count);

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
        TC_LOG_DEBUG("arena","Окончание сезона Ноябрь 2017");
        // by default:
        // 0 type - 1 - 3
        // 2 type - 4 - 12
        // 3 type - 13 - 25
        // 4 type - 26 - 50
        // 2c2
        {
            QueryResult teamResult = CharacterDatabase.Query("SELECT ch.guid, ch.name, wins, games, rating FROM `character_brackets_info` cbi LEFT JOIN characters ch ON ch.guid = cbi.guid WHERE `bracket` = 0 ORDER BY rating DESC");
            if (teamResult && teamResult->GetRowCount())
            {
                uint32 playerCount = teamResult->GetRowCount();
                uint32 firstWinTypeCount = 1;
                uint32 secondWinTypeCount = uint32(playerCount * 0.1f);
                uint32 thirdWinTypeCount  = uint32(playerCount * 0.3f);
                uint32 fothWinTypeCount  = uint32(playerCount * 0.5f);
                uint32 fifthWinTypeCount  = uint32(playerCount * 1.0f);

                uint32 teamNumber = 1;
                do
                {
                    Field* Fields = teamResult->Fetch();
                    uint64 guid = Fields[0].GetUInt64();
                    std::string name = Fields[1].GetString();
                    uint32 wins = Fields[2].GetUInt32();
                    uint32 games = Fields[3].GetUInt32();
                    uint32 rating = Fields[4].GetUInt32();

                    uint32 titleId = 0;
                    uint32 efirCount = 0;
                    if(!debugOnly)
                    {
                        if (teamNumber >= firstWinTypeCount && teamNumber <= firstWinTypeCount)
                        {
                            titleId = 279;
                            efirCount = 500;
                        }
                        if (teamNumber > firstWinTypeCount && teamNumber <= secondWinTypeCount)
                        {
                            titleId = 42;
                            efirCount = 250;
                        }
                        if (teamNumber > secondWinTypeCount && teamNumber <= thirdWinTypeCount)
                        {
                            titleId = 43;
                            efirCount = 100;
                        }
                        if (teamNumber > thirdWinTypeCount && teamNumber <= fothWinTypeCount)
                        {
                            titleId = 44;
                            efirCount = 75;
                        }
                        if (teamNumber > fothWinTypeCount && teamNumber <= fifthWinTypeCount)
                        {
                            titleId = 45;
                            efirCount = 50;
                        }

                        if (efirCount)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','%u')", guid, 38186, efirCount);
                            TC_LOG_DEBUG("arena","INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','%u');", guid, 38186, efirCount);
                        }
                        if (titleId)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, titleId);
                            TC_LOG_DEBUG("arena","INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, titleId);
                        }
                        // CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 500054); //Tabard winter 2017
                        // CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','150')", guid, 38186);
                    }
                    TC_LOG_DEBUG("arena","За %u место в топе 2на2 получил %s(%u) %u эфира, побед %u, игр %u, рейтинг %u", teamNumber, name.c_str(), guid, efirCount, wins, games, rating);

                    teamNumber++;
                }
                while (teamResult->NextRow());
            }
        }

        if (realmID != 43) // x100 only
            return true;

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
                uint32 playerCount = arenaWinner->GetRowCount();
                uint32 firstWinTypeCount = 3;
                uint32 secondWinTypeCount  = uint32(playerCount * 0.005f);
                if(secondWinTypeCount < 6)
                    secondWinTypeCount = 6;
                uint32 thirdWinTypeCount  = uint32(playerCount * 0.03f);
                if(thirdWinTypeCount < 6)
                    thirdWinTypeCount = 6;
                uint32 fothWinTypeCount  = uint32(playerCount * 0.1f);
                if(fothWinTypeCount < 6)
                    fothWinTypeCount = 6;
                uint32 fifthWinTypeCount  = uint32(playerCount * 0.35f);
                if(fifthWinTypeCount < 6)
                    fifthWinTypeCount = 6;

                TC_LOG_DEBUG("arena","Количество чаров %u первая %u, вторая %u, третья %u, четвертая %u, петая %u", playerCount, firstWinTypeCount, secondWinTypeCount, thirdWinTypeCount, fothWinTypeCount, fifthWinTypeCount);

                uint32 teamNumber = 1;
                do
                {
                    Field* Fields = arenaWinner->Fetch();
                    uint64 guid = Fields[0].GetUInt64();
                    std::string name = Fields[1].GetString();
                    uint32 wins = Fields[2].GetUInt32();
                    uint32 games = Fields[3].GetUInt32();
                    uint32 rating = Fields[4].GetUInt32();

                    //Season achivement Гладиатор / Gladiator
                    if (teamNumber >= firstWinTypeCount && teamNumber <= firstWinTypeCount)
                    {
                        // CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 71954); //item
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','900')", guid, 38186); //item
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 8214); // achivement
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 343); //Title

                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2091);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 42);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2092);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 43);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        TC_LOG_DEBUG("arena","За %u место в топе 3на3(0.001) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", teamNumber, name.c_str(), guid, wins, games, rating);
                    }
                    if (teamNumber > firstWinTypeCount && teamNumber <= secondWinTypeCount)
                    {
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','600')", guid, 38186); //item
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2091);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 42);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2092);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 43);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        TC_LOG_DEBUG("arena","За %u место в топе 3на3(0.005) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", teamNumber, name.c_str(), guid, wins, games, rating);
                    }
                    if (teamNumber > secondWinTypeCount && teamNumber <= thirdWinTypeCount)
                    {
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", guid, 38186); //item
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2092);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 43);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        TC_LOG_DEBUG("arena","За %u место в топе 3на3(0.03) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", teamNumber, name.c_str(), guid, wins, games, rating);
                    }
                    if (teamNumber > thirdWinTypeCount && teamNumber <= fothWinTypeCount)
                    {
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','100')", guid, 38186); //item
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        TC_LOG_DEBUG("arena","За %u место в топе 3на3(0.1) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", teamNumber, name.c_str(), guid, wins, games, rating);
                    }
                    if (teamNumber > fothWinTypeCount && teamNumber <= fifthWinTypeCount)
                    {
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','75')", guid, 38186); //item
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        TC_LOG_DEBUG("arena","За %u место в топе 3на3(0.1) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", teamNumber, name.c_str(), guid, wins, games, rating);
                    }
                    teamNumber++;
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
        /*{
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
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 85785); //item
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','200')", guid, 38186); //item
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 8643); // achivement
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 378); //Title
                        }

                        TC_LOG_DEBUG("arena","За %u место в топе 5на5(0.1) получил %s(%u) efir=200, item=85785, achiv=6938, title=281, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
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

                        TC_LOG_DEBUG("arena","За %u место в топе 5на5(0.5) получил %s(%u) efir=250, achiv=2091, title=42, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Дуэлянт / Duelist
                    if(count <= thirdWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2092);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 43);
                        }

                        TC_LOG_DEBUG("arena","За %u место в топе 5на5(3.0) получил %s(%u) achiv=2092, title=43, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Фаворит / Rival
                    if(count <= fothWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2093);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 44);
                        }

                        TC_LOG_DEBUG("arena","За %u место в топе 5на5(10.0) получил %s(%u) achiv=2093, title=44, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }

                    //Претендент / Challenger
                    if(count <= fifthWinTypeCount)
                    {
                        if(!debugOnly)
                        {
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 38313); //Tabard summer 2015
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, 2090);
                            CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','2','%u','0')", guid, 45);
                        }

                        TC_LOG_DEBUG("arena","За %u место в топе 5на5(35.0) получил %s(%u) achiv=2090, title=45, побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, wins, games, rating);
                    }
                    count++;
                }
                while (arenaWinner->NextRow());
            }
        }*/

        // by default:
        // 1 - top offlokie 0.5%
        // RBG
        // {
            // if(QueryResult arenaWinner = CharacterDatabase.PQuery("SELECT ch.guid, ch.name, wins, games, rating, ch.race FROM `character_brackets_info` cbi LEFT JOIN characters ch ON ch.guid = cbi.guid WHERE `bracket` = 3 and rating > 1000 AND wins > 49 ORDER BY rating DESC"))
            // {
                // uint32 count = 1;
                // uint32 playerCount = arenaWinner->GetRowCount();
                // uint32 firstWinTypeCount = uint32(playerCount * 0.005f);
                // if(firstWinTypeCount < 10)
                    // firstWinTypeCount = 10;

                // do
                // {
                    // Field* Fields = arenaWinner->Fetch();
                    // uint64 guid = Fields[0].GetUInt64();
                    // std::string name = Fields[1].GetString();
                    // uint32 wins = Fields[2].GetUInt32();
                    // uint32 games = Fields[3].GetUInt32();
                    // uint32 rating = Fields[4].GetUInt32();
                    // uint32 team = Player::TeamForRace(Fields[5].GetUInt8());
                    // uint32 achivement = team == HORDE ? 6941 : 6942;

                    // //Season achivement Герой Орды / Hero of the Horde / Hero of the Alliance / Герой Альянса
                    // if(count <= firstWinTypeCount)
                    // {
                        // if(!debugOnly)
                        // {
                            // // CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", guid, 500022); //Tabard winter 2016
                            // CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','250')", guid, 38186);
                            // CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','1','%u','0')", guid, achivement);
                        // }

                        // TC_LOG_DEBUG("arena","За %u место в топе РБГ(0.5) получил %s(%u) efir=250, achiv=%u побед %u, игр %u, рейтинг %u", count, name.c_str(), guid, achivement, wins, games, rating);
                    // }
                    // count++;
                // }
                // while (arenaWinner->NextRow());
            // }
        // }

        if(!debugOnly)
            sWorld->SendWorldText(LANG_ARENA_CLOSESEASON_END);
        return true;
    }

    static bool HandleAddCharCommand(ChatHandler* handler, const char* args)
    {
        std::string name = args;
        if (uint64 guid = sObjectMgr->GetPlayerGUIDByName(name))
        {
            sObjectMgr->AddCharToDupeLog(guid);
            handler->PSendSysMessage("Added.");
        }

        return true;
    }

    static bool HandleRemoveCharCommand(ChatHandler* handler, const char* args)
    {
        std::string name = args;
        if (uint64 guid = sObjectMgr->GetPlayerGUIDByName(name))
        {
            sObjectMgr->RemoveCharFromDupeList(guid);
            handler->PSendSysMessage("Removed.");
        }

        return true;
    }
    
    static bool HandleCharacterItemsCheckCommand(ChatHandler* handler, const char* args) //проверяет, есть нужные итемы у игрока или нет
    {         
        Player* player = handler->GetSession()->GetPlayer();
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
            return false;

        char* ItemsStr = strtok(NULL, "\r");
        
        if (!ItemsStr)
            return false;
        uint32 itemId = atoi(ItemsStr);
        
        QueryResult result = CharacterDatabase.PQuery("SELECT SUM(`count`) FROM `item_instance` WHERE `itemEntry` = '%u' AND `owner_guid` = '%u';", itemId, target_guid);
        if (result)
        {  
            do
            {
               Field * fetch = result->Fetch();
               uint32 count = fetch[0].GetUInt32();

               std::string nameLink = handler->playerLink(target_name);
               handler->PSendSysMessage("Игрок %s имеет %u итемов с идом %u", nameLink.c_str(), count, itemId);
            } while ( result->NextRow() );
        }

        return true;
    }
    
    static bool HandleCharacterDMSelectCommand(ChatHandler* handler, const char* args) //селект на ДМ
    {         
        Player* player = handler->GetSession()->GetPlayer();
      
        
        QueryResult result = CharacterDatabase.PQuery("SELECT * FROM `custom_character_deathmatch` ORDER BY kills DESC LIMIT 10;");
        if (result)
        {  
            uint32 index = 0;
            do
            {
               Field * fetch = result->Fetch();
               uint64 guid = fetch[0].GetUInt64();
               std::string namePl;
               if (!ObjectMgr::GetPlayerNameByGUID(guid, namePl))
                   continue;
               else
                   index++;
               uint32 kills = fetch[1].GetUInt32();
               uint32 deads = fetch[2].GetUInt32();
               uint32 count = fetch[3].GetUInt32();
               
               handler->PSendSysMessage("Игрок %s имеет %u киллов, %u смертей и сыграл %u игр. Занимает место %u", namePl.c_str(), kills, deads, count, index);
            } while ( result->NextRow() && index < 10 );
        }

        return true;
    }
   
    static bool HandleCharacterItemsCheckBagCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* itemIdStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &itemIdStr);
       
        Player* player = handler->GetSession()->GetPlayer();
        Player* target;
        uint64 target_guid;
        std::string target_name;
        if (!handler->extractPlayerTarget(nameStr, &target, &target_guid, &target_name))
            return false;
        if (!target && target_guid)
             target = sObjectMgr->GetPlayerByLowGUID(target_guid);
        if (!target)
            return false;
        uint32 accountId = target->GetSession()->GetAccountId();
        
        if (!itemIdStr)
            return false;
        uint32 itemId = atoi(itemIdStr);
        std::string nameLink = handler->playerLink(target_name);
        handler->PSendSysMessage("%s имеет %u предметов с идом %u в сумке", nameLink.c_str(), target->GetItemCount(itemId, false), itemId);
           
        return true;

    }
};

void AddSC_command_arena()
{
    new command_arena();
}
