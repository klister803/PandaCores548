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
            { "add",            SEC_GAMEMASTER,  false, &HandleAddMorphCommand,              "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        
        static ChatCommand mountFlyCommandTable[] =
        {
            { "list",           SEC_PLAYER,         false, &HandleListMountFlyCommand,             "", NULL },
            { "use",            SEC_PLAYER,         false, &HandleUseMountFlyCommand,              "", NULL },
            { "add",            SEC_GAMEMASTER,  false, &HandleAddMountFlyCommand,              "", NULL },
            { NULL,             0,                  false, NULL,                                "", NULL }
        };
        
        static ChatCommand mountGroundCommandTable[] =
        {
            { "list",           SEC_PLAYER,         false, &HandleListMountGroundCommand,             "", NULL },
            { "use",            SEC_PLAYER,         false, &HandleUseMountGroundCommand,              "", NULL },
            { "add",            SEC_GAMEMASTER,  false, &HandleAddMountGroundCommand,              "", NULL },
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

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 1 AND `state` = '0';", accountId);
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
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        char* morphId = strtok((char*)args, " ");
        if (!morphId)
            return false;

        int32 morph = atoi(morphId);
        if (!morph)
            return false;

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 1 AND itemEntry = '%u' AND `state` = '0';", accountId, morph);
        if (player->isGameMaster() || result)
        {
            CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(morph);
            uint32 modelid = ci->GetRandomValidModelId();
            player->SetDisplayId(modelid, true);
            player->SetCustomDisplayId(modelid);
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
        
       CharacterDatabase.PExecute("replace into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '0', '1', '%u', '%u', '1', '%u');", target_guid, morphId, efirCount, accountId);
       handler->PSendSysMessage("Morph add %u for %u efirs", morphId, efirCount);

        return true;
    }
    
    /////////////// mounts 
        static bool HandleListMountFlyCommand(ChatHandler* handler, const char* args)
    {
        Player *player = handler->GetSession()->GetPlayer();
        uint32 accountId = handler->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 2 AND `state` = '0';", accountId);
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
           
           CharacterDatabase.PQuery("UPDATE character_donate set itemguid = '0' WHERE account = %u AND `type` = 2 AND `state` = '0';", accountId); //clear
           CharacterDatabase.PQuery("UPDATE character_donate set itemguid = '1' WHERE account = %u AND `type` = 2 AND itemEntry = '%u' AND `state` = '0';", accountId, mount); //select active mount
           //player->Mount(displayID, ci->VehicleId, mount);
           player->m_Events.AddEvent(new mount_set_display_fly(player), player->m_Events.CalculateTime(1600));
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
        
       CharacterDatabase.PExecute("replace into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '0', '2', '%u', '%u', '1', '%u');", target_guid, mountId, efirCount, accountId);
       handler->PSendSysMessage("Fly mount add %u for %u efirs", mountId, efirCount);

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
           
           CharacterDatabase.PQuery("UPDATE character_donate set itemguid = '0' WHERE account = %u AND `type` = 3 AND `state` = '0';", accountId); //clear
           CharacterDatabase.PQuery("UPDATE character_donate set itemguid = '1' WHERE account = %u AND `type` = 3 AND itemEntry = '%u' AND `state` = '0';", accountId, mount); //select active mount

           //player->Mount(displayID, ci->VehicleId, mount);
           player->m_Events.AddEvent(new mount_set_display_ground(player), player->m_Events.CalculateTime(1700));
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
        
       CharacterDatabase.PExecute("replace into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '0', '3', '%u', '%u', '1', '%u');", target_guid, mountId, efirCount, accountId);
       handler->PSendSysMessage("Ground mount add %u for %u efirs", mountId, efirCount);

        return true;
    }
    
class mount_set_display_fly : public BasicEvent
{
   public:
      mount_set_display_fly(Player* player) : player(player) {}

      bool Execute(uint64 /*time*/, uint32 /*diff*/)
         {
            if (player->HasAura(121805))
               {          
                          Player * pPlayer = player->GetSession()->GetPlayer();
                          uint32 accountId = pPlayer->GetSession()->GetAccountId();
                          QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 2 AND itemguid = '1' AND `state` = '0';", accountId);
                          if (result)
                          {
                             do
                              {
                                   Field * fetch = result->Fetch();
                                   uint32 mount = fetch[0].GetUInt32();
                                   CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(mount);
                                   if (!(ci))
                                      return false;
                                   
                                   uint32 displayID = sObjectMgr->ChooseDisplayId(0, ci);
                                   sObjectMgr->GetCreatureModelRandomGender(&displayID);
                                   
                                   player->Mount(displayID, ci->VehicleId, mount);
                             } while ( result->NextRow() );
                             
                          }
               }
               
            return true;
         }
      Player* player;
   };
   
   class mount_set_display_ground : public BasicEvent
{
   public:
      mount_set_display_ground(Player* player) : player(player) {}

      bool Execute(uint64 /*time*/, uint32 /*diff*/)
         {
            if (player->HasAura(58819))
               {          
                          Player * pPlayer = player->GetSession()->GetPlayer();
                          uint32 accountId = pPlayer->GetSession()->GetAccountId();
                          QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE account = %u AND `type` = 3 AND itemguid = '1' AND `state` = '0';", accountId);
                          if (result)
                          {
                             do
                              {
                                   Field * fetch = result->Fetch();
                                   uint32 mount = fetch[0].GetUInt32();
                                   CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(mount);
                                   if (!(ci))
                                      return false;
                                   
                                   uint32 displayID = sObjectMgr->ChooseDisplayId(0, ci);
                                   sObjectMgr->GetCreatureModelRandomGender(&displayID);
                                   
                                   player->Mount(displayID, ci->VehicleId, mount);
                             } while ( result->NextRow() );
                             
                          }
               }
               
            return true;
         }
      Player* player;
   };
};



void AddSC_command_donate()
{
    new command_donate();
}
