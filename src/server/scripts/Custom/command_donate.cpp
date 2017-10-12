#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "ScriptPCH.h"
#include "Player.h"


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
            { "add",            SEC_CONFIRMED_GAMEMASTER,  false, &HandleAddMorphCommand,              "", NULL },
            { "del",            SEC_CONFIRMED_GAMEMASTER,  false, &HandleDelMorphCommand,              "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        
        static ChatCommand mountFlyCommandTable[] =
        {
            { "list",           SEC_PLAYER,         false, &HandleListMountFlyCommand,             "", NULL },
            { "use",            SEC_PLAYER,         false, &HandleUseMountFlyCommand,              "", NULL },
            { "add",            SEC_CONFIRMED_GAMEMASTER,  false, &HandleAddMountFlyCommand,              "", NULL },
            { "del",            SEC_GAMEMASTER,  false, &HandleDelMountFlyCommand,              "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        
        static ChatCommand mountGroundCommandTable[] =
        {
            { "list",           SEC_PLAYER,         false, &HandleListMountGroundCommand,             "", NULL },
            { "use",            SEC_PLAYER,         false, &HandleUseMountGroundCommand,              "", NULL },
            { "add",            SEC_CONFIRMED_GAMEMASTER,  false, &HandleAddMountGroundCommand,              "", NULL },
            { "del",            SEC_CONFIRMED_GAMEMASTER,  false, &HandleDelMountGroundCommand,              "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        
        static ChatCommand mountCommandTable[] =
        {
            { "fly",           SEC_PLAYER,         false, NULL,             "", mountFlyCommandTable },
            { "ground",        SEC_PLAYER,         false, NULL,             "", mountGroundCommandTable },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };

        static ChatCommand donateCommandTable[] =
        {
            { "morph",          SEC_PLAYER,         false, NULL,                                "", morphCommandTable },
            { "mount",          SEC_PLAYER,         false, NULL,                                "", mountCommandTable },
            { "add",            SEC_CONFIRMED_GAMEMASTER,  false, &HandleAddDonatCommand,              "", NULL },
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
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = '%u' AND `type` = '1' AND `state` = '0';", accountId);
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
        Player* player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        char* morphId = strtok((char*)args, " ");
        if (!morphId || !player)
            return false;

        int32 morph = atoi(morphId);
        if (!morph)
            return false;

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = '%u' AND `type` = '1' AND itemEntry = '%u' AND `state` = '0';", accountId, morph);
        if (player->isGameMaster() || result)
        {
            if (CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(morph))
            {
                uint32 modelid = ci->GetRandomValidModelId();
                player->SetCustomDisplayId(modelid);
                player->SetDisplayId(modelid, true);
            }
            else
                handler->PSendSysMessage("Cant find morph to use.");
        }
        else
            handler->PSendSysMessage("Cant find morph to use.");

        return true;
    }

    static bool HandleRemoveMorphCommand(ChatHandler* handler, const char* args)
    {
        Player *player = handler->GetSession()->GetPlayer();
        player->DeMorph();
        player->ResetCustomDisplayId();
        player->SetObjectScale(1.0f);
        return true;
    }

    static bool HandleAddMorphCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* morphStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &morphStr);

        char* efirCountStr = strtok(NULL, "\r");
       
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

        if (!morphStr)
            return false;
        uint32 morphId = atoi(morphStr);
        
        if (!efirCountStr)
            return false;
        uint32 efirCount = atoi(efirCountStr);

        if (target->GetItemCount(38186, false) < efirCount)
        {
            handler->PSendSysMessage("Target don`t have efir count %u in bags", efirCount);
            return true;
        }
        else
        {
           target->DestroyItemCount(38186, efirCount, true);
        }
        
       CharacterDatabase.PExecute("replace into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '%u', '1', '%u', '%u', '1', '%u');", target_guid, morphId, morphId, efirCount, accountId);
       handler->PSendSysMessage("Morph add %u for %u efirs", morphId, efirCount);

        return true;
    }    
    static bool HandleDelMorphCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* morphStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &morphStr);

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

        if (!morphStr)
            return false;
        uint32 morphId = atoi(morphStr);
        
        QueryResult result = CharacterDatabase.PQuery("SELECT efirCount FROM character_donate WHERE account = '%u' AND `type` = '1' AND itemEntry = '%u' AND `state` = '0';", accountId, morphId);
        if (result)
        {  
            do
            {
                Field * fetch = result->Fetch();
                uint32 efirCount = fetch[0].GetUInt32();

                target->AddItem(38186, efirCount);
                CharacterDatabase.PExecute("delete from `character_donate` WHERE account = '%u' AND `type` = '1' AND itemEntry = '%u' AND `state` = '0' and efirCount = '%u';", accountId, morphId, efirCount);
                handler->PSendSysMessage("Morph %u delete and %u efirs give player", morphId, efirCount);
            } while ( result->NextRow() );
        }
        else
           handler->PSendSysMessage("Morph %u dont find", morphId);

        return true;
    }
    
    /////////////// mounts 
        static bool HandleListMountFlyCommand(ChatHandler* handler, const char* args)
    {
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = '%u' AND `type` = '2' AND `state` = '0';", accountId);
        if(result)
        {
            do
            {
                Field * fetch = result->Fetch();
                uint32 entry = fetch[0].GetUInt32();

                handler->PSendSysMessage(".donate mount fly use %u", entry);
            } while ( result->NextRow() );
        }
        else
            handler->PSendSysMessage("Cant find mount to use.");
        return true;
    }

    static bool HandleUseMountFlyCommand(ChatHandler* handler, const char* args)
    {
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        char* mountId = strtok((char*)args, " ");
        if (!mountId)
            return false;

        int32 mount = atoi(mountId);
        if (!mount)
            return false;
         
        if (player->isGameMaster())
        {
           CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(mount);
           if (!(ci))
              return false;
           
           uint32 displayID = sObjectMgr->ChooseDisplayId(0, ci);
           sObjectMgr->GetCreatureModelRandomGender(&displayID);
           player->CastSpell(player, 31700);
           player->Mount(displayID);
           handler->PSendSysMessage("It's demo version for GM with GM spell");
           return true;
        }
         
        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 2 AND itemEntry = '%u' AND `state` = '0';", accountId, mount);
        if (result)
        {
           player->CastSpell(player, 121805);
           //player->Mount(displayID, ci->VehicleId, mount);
           new mount_set_display_fly(player, mount);
        }
        else
            handler->PSendSysMessage("Cant find mount to use.");

        return true;
    }
    
   static bool HandleAddMountFlyCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* mountStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &mountStr);

        char* efirCountStr = strtok(NULL, "\r");
       
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

        if (!mountStr)
            return false;
        uint32 mountId = atoi(mountStr);
        
        if (!efirCountStr)
            return false;
        uint32 efirCount = atoi(efirCountStr);

        if (target->GetItemCount(38186, false) < efirCount)
        {
            handler->PSendSysMessage("Target don`t have efir count %u in bags", efirCount);
            return true;
        }
        else
        {
           target->DestroyItemCount(38186, efirCount, true);
        }
        
       CharacterDatabase.PExecute("replace into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '%u', '2', '%u', '%u', '1', '%u');", target_guid, mountId, mountId, efirCount, accountId);
       handler->PSendSysMessage("Fly mount add %u for %u efirs", mountId, efirCount);

        return true;
    }
    
    static bool HandleDelMountFlyCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* morphStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &morphStr);

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

        if (!morphStr)
            return false;
        uint32 morphId = atoi(morphStr);
        
        QueryResult result = CharacterDatabase.PQuery("SELECT efirCount FROM character_donate WHERE account = %u AND `type` = 2 AND itemEntry = '%u' AND `state` = '0';", accountId, morphId);
        if (result)
        {  
            do
            {
                Field * fetch = result->Fetch();
                uint32 efirCount = fetch[0].GetUInt32();

                target->AddItem(38186, efirCount);
                CharacterDatabase.PExecute("delete from `character_donate` WHERE account = %u AND `type` = 2 AND itemEntry = '%u' AND `state` = '0' and efirCount = '%u';", accountId, morphId, efirCount);
                handler->PSendSysMessage("Fly mount %u delete and %u efirs give player", morphId, efirCount);
            } while ( result->NextRow() );
        }
        else
           handler->PSendSysMessage("Fly mount %u dont find", morphId);

        return true;
    }
    
// ground
        static bool HandleListMountGroundCommand(ChatHandler* handler, const char* args)
    {
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 3 AND `state` = '0';", accountId);
        if(result)
        {
            do
            {
                Field * fetch = result->Fetch();
                uint32 entry = fetch[0].GetUInt32();

                handler->PSendSysMessage(".donate mount ground use %u", entry);
            } while ( result->NextRow() );
        }
        else
            handler->PSendSysMessage("Cant find mount to use.");
        return true;
    }

    static bool HandleUseMountGroundCommand(ChatHandler* handler, const char* args)
    {
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        char* mountId = strtok((char*)args, " ");
        if (!mountId)
            return false;

        int32 mount = atoi(mountId);
        if (!mount)
            return false;

        if (player->isGameMaster())
        {
           CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(mount);
           if (!(ci))
              return false;
           
           uint32 displayID = sObjectMgr->ChooseDisplayId(0, ci);
           sObjectMgr->GetCreatureModelRandomGender(&displayID);
           player->CastSpell(player, 31700);
           player->Mount(displayID);
           handler->PSendSysMessage("It's demo version for GM with GM spell");
           return true;
        }
         
        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 3 AND itemEntry = '%u' AND `state` = '0';", accountId, mount);
        if (result)
        {
           player->CastSpell(player, 58819);

           //player->Mount(displayID, ci->VehicleId, mount);
           new mount_set_display_ground(player, mount);
        }
        else
            handler->PSendSysMessage("Cant find mount to use.");

        return true;
    }
    
    static bool HandleAddMountGroundCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* mountStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &mountStr);

        char* efirCountStr = strtok(NULL, "\r");
       
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

        if (!mountStr)
            return false;
        uint32 mountId = atoi(mountStr);
        
        if (!efirCountStr)
            return false;
        uint32 efirCount = atoi(efirCountStr);

        if (target->GetItemCount(38186, false) < efirCount)
        {
            handler->PSendSysMessage("Target don`t have efir count %u in bags", efirCount);
            return true;
        }
        else
        {
           target->DestroyItemCount(38186, efirCount, true);
        }
        
       CharacterDatabase.PExecute("replace into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '%u', '3', '%u', '%u', '1', '%u');", target_guid, mountId, mountId, efirCount, accountId);
       handler->PSendSysMessage("Ground mount add %u for %u efirs", mountId, efirCount);

        return true;
    }    
        
    static bool HandleDelMountGroundCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* morphStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &morphStr);

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

        if (!morphStr)
            return false;
        uint32 morphId = atoi(morphStr);
        
        QueryResult result = CharacterDatabase.PQuery("SELECT efirCount FROM character_donate WHERE account = %u AND `type` = 3 AND itemEntry = '%u' AND `state` = '0';", accountId, morphId);
        if (result)
        {  
            do
            {
                Field * fetch = result->Fetch();
                uint32 efirCount = fetch[0].GetUInt32();

                target->AddItem(38186, efirCount);
                CharacterDatabase.PExecute("delete from `character_donate` WHERE account = %u AND `type` = 3 AND itemEntry = '%u' AND `state` = '0' and efirCount = '%u';", accountId, morphId, efirCount);
                handler->PSendSysMessage("Ground mount %u delete and %u efirs give player", morphId, efirCount);
            } while ( result->NextRow() );
        }
        else
           handler->PSendSysMessage("Ground mount %u don`t find", morphId);

        return true;
    }

    static bool HandleAddDonatCommand(ChatHandler* handler, const char* args)
    {
        char* nameStr;
        char* itemStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &itemStr);

        char* efirCountStr = strtok(NULL, "\r");
       
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

        if (!itemStr)
            return false;
        uint32 itemId = atoi(itemStr);
        
        if (!efirCountStr)
            return false;
        uint32 efirCount = atoi(efirCountStr);

        if (target->GetItemCount(38186, false) < efirCount)
        {
            handler->PSendSysMessage("Target don`t have efir count %u in bags", efirCount);
            return false;
        }
        else
        {
           target->DestroyItemCount(38186, efirCount, true);
           target->AddItem(itemId, 1);
        }
        return true;
    }
    
class mount_set_display_fly : public BasicEvent
{
    public:
    mount_set_display_fly(Player* player, uint32 mount) : player(player), mount(mount) 
    {
        player->m_Events.AddEvent(this, player->m_Events.CalculateTime(1550));
    }

    bool Execute(uint64 /*time*/, uint32 /*diff*/)
    {
        if (player->HasAura(121805))
        {          
            Player * pPlayer = player->GetSession()->GetPlayer();

            CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(mount);
            if (!(ci))
                return false;

            uint32 displayID = sObjectMgr->ChooseDisplayId(0, ci);
            sObjectMgr->GetCreatureModelRandomGender(&displayID);

            player->Mount(displayID, ci->VehicleId, mount);
        }

        return true;
    }
    Player* player;
    uint32 mount;
};
   
class mount_set_display_ground : public BasicEvent
{
   public:
      mount_set_display_ground(Player* player, uint32 mount) : player(player), mount(mount) 
    {
        player->m_Events.AddEvent(this, player->m_Events.CalculateTime(1550));
    }

    bool Execute(uint64 /*time*/, uint32 /*diff*/)
    {
        if (player->HasAura(58819))
        {          
            Player * pPlayer = player->GetSession()->GetPlayer();

            CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(mount);
            if (!(ci))
                return false;

            uint32 displayID = sObjectMgr->ChooseDisplayId(0, ci);
            sObjectMgr->GetCreatureModelRandomGender(&displayID);

            player->Mount(displayID, ci->VehicleId, mount);
        }

        return true;
    }
    
    Player* player;
    uint32 mount;
    
   };
};



void AddSC_command_donate()
{
    new command_donate();
}