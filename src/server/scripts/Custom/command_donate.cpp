#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"

class command_donate : public CommandScript
{
public:
    command_donate() : CommandScript("command_donate") { }

    ChatCommand* GetCommands() const
    {

        static ChatCommand morphCommandTable[] =
        {
            { "list",           SEC_PLAYER,         false, &HandleListMorphCommand,             "", NULL },
            { "use",            SEC_PLAYER,         false, &HandleUseMorphCommand,              "", NULL },
            { "remove",         SEC_PLAYER,         false, &HandleRemoveMorphCommand,           "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand donateCommandTable[] =
        {
            { "morph",          SEC_PLAYER,         false, NULL,                                "", morphCommandTable },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand commandTable[] =
        {
            { "donate",         SEC_PLAYER,         false, NULL,                                "", donateCommandTable },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        return commandTable;
    }

    static bool HandleListMorphCommand(ChatHandler* handler, const char* args)
    {
        uint32 accountId = handler->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 1;", accountId);
        if(result)
        {
            do
            {
                Field * fetch = result->Fetch();
                uint32 entry = fetch[0].GetUInt32();

                handler->PSendSysMessage(".donate morph use %u", entry);
            } while ( result->NextRow() );
        }
        else
            handler->PSendSysMessage("Cant find morph to use.");
        return true;
    }

    static bool HandleUseMorphCommand(ChatHandler* handler, const char* args)
    {
        Player* plr = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        char* morphId = strtok((char*)args, " ");
        if(!morphId)
            return false;

        int32 morph = atoi(morphId);
        if(!morph)
            return false;

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 1 AND itemEntry = '%u';", accountId, morph);
        if(result)
        {
            CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(morph);
            uint32 modelid = ci->GetRandomValidModelId();
            plr->SetDisplayId(modelid);
        }
        else
            handler->PSendSysMessage("Cant find morph to use.");

        return true;
    }

    static bool HandleRemoveMorphCommand(ChatHandler* handler, const char* args)
    {
        Player* plr = handler->GetSession()->GetPlayer();
        plr->DeMorph();
        return true;
    }
};

void AddSC_command_donate()
{
    new command_donate();
}
