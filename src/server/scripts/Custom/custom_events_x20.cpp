#include "ScriptPCH.h"
#include "Player.h"
#include "AchievementMgr.h"

// 37711 token

//1 - первый вход
//2 - остатки валюты
//3 - лвл ап

class check_on_points : public PlayerScript
{
    public:
        check_on_points() : PlayerScript("check_on_points") {}

        void OnLogin(Player *player)  // проверка при входе
        {
           uint32 id = 37711;
           ChatHandler chH = ChatHandler(player);
           if (sWorld->getBoolConfig(CONFIG_CUSTOM_X20))
           {
              // первый вход - даем валюту
              QueryResult result_first = CharacterDatabase.PQuery("SELECT * FROM custom_account_checker WHERE account = '%u' and type = '1';", player->GetSession()->GetAccountId()); 
              if (!result_first)
              {
                uint32 noSpaceForCount = 0;
                uint32 count = player->GetAchievementPoints();

                if (count)  // new char maybe
                {
                   ItemPosCountVec dest;
                   InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count, &noSpaceForCount);
                   if (msg != EQUIP_ERR_OK || dest.empty())                                
                   {
                       chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                       return;
                   }

                   Item* item = player->StoreNewItem(dest, id, true, Item::GenerateItemRandomPropertyId(id));

                   if (count > 0 && item)
                       player->SendNewItem(item, count, false, true);
                }
                    
                 CharacterDatabase.PExecute("INSERT INTO `custom_account_checker` (`type`, `account`) VALUES ('1', '%u');", player->GetSession()->GetAccountId()); 
               }
              QueryResult result_two = CharacterDatabase.PQuery("SELECT * FROM custom_account_checker WHERE account = '%u' and type = '2';", player->GetSession()->GetAccountId()); 
              if (result_two)
              {
                  do
                  {
                      Field* fields = result_two->Fetch();
                      uint32 count1 = fields[1].GetUInt32();
                      uint32 count_save = count1;
                      
                       //Adding items
                      uint32 noSpaceForCount = 0;

                      // check space and find places
                      ItemPosCountVec dest;
                      InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count1, &noSpaceForCount);
                      if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
                          count1 -= noSpaceForCount;

                      if (count1 == 0 || dest.empty())                         // can't add any
                      {
                          chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                          return;
                      }

                      Item* item = player->StoreNewItem(dest, id, true, Item::GenerateItemRandomPropertyId(id));

                      if (count1 > 0 && item)
                          player->SendNewItem(item, count1, false, true);

                      if (noSpaceForCount > 0)
                      {
                          chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                          CharacterDatabase.PExecute("UPDATE custom_account_checker SET count = count - %u WHERE account = '%u' and type = '2' and count = '%u'", count1, player->GetSession()->GetAccountId(), count_save);
                      } 
                      else
                         CharacterDatabase.PExecute("delete from custom_account_checker WHERE account = '%u' and type = '2' and count = '%u'", player->GetSession()->GetAccountId(), count_save);              
                  }while (result_two->NextRow());                      
              }
           
           
           }                   
        }
};


class npc_custom_multi_x20 : public CreatureScript
{
public:
    npc_custom_multi_x20() : CreatureScript("npc_custom_multi_x20") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
              QueryResult result_three = CharacterDatabase.PQuery("SELECT * FROM custom_account_checker WHERE account = '%u' and type = '3';", player->GetSession()->GetAccountId()); 
              if (!result_three)
              {
            //   player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Один бесплатный левел ап до 90-го уровня", GOSSIP_SENDER_MAIN, 1);
              }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Эксклюзивные предметы за Эпическую Валюту", GOSSIP_SENDER_MAIN, 2); 
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Рисованные маунты и морфы за Эпическую Валюту", GOSSIP_SENDER_MAIN, 3); 
            player->SEND_GOSSIP_MENU(100001, creature->GetGUID());
            return true;
        }
        
        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            ChatHandler chH = ChatHandler(player);
            switch (action)
            {
                case 1:
                {
                  if (player->getLevel() == 90)
                  {
                     chH.PSendSysMessage("Вы итак уже 90-го уровня!");
                     player->CLOSE_GOSSIP_MENU();
                      break;
                  }
                  QueryResult result_three = CharacterDatabase.PQuery("SELECT * FROM custom_account_checker WHERE account = '%u' and type = '3';", player->GetSession()->GetAccountId()); 
                  if (!result_three)
                  {
                     CharacterDatabase.PExecute("INSERT INTO `custom_account_checker` (`type`, `account`) VALUES ('3', '%u');", player->GetSession()->GetAccountId()); 
                     player->GiveLevel(90);
                  }
                  player->CLOSE_GOSSIP_MENU();
                }
                  break;
                case 2:   
                  player->GetSession()->SendListInventory(creature->GetGUID());
                  break;
                case 3:
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[маунт] Синий Киражский боевой танк [бегает везде] - 4.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 100); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[маунт] Красный Киражский боевой танк [бегает везде] - 4.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 101); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[маунт] Зеленый Киражский боевой танк [бегает везде] - 4.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 102); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[маунт] Желтый Киражский боевой танк [бегает везде] - 4.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 103); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[морф] Демоническая эльфийка в латах с крыльями [id 25484] - 10.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 104); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[морф] Демоническая эльфийка в ткане с крыльями [id 25373] - 10.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 105); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[морф] Изера - 10.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 106); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[морф] Алекстраза - 10.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 107); 
                  player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "[морф] Ноздорму - 10.000 Эпической Валюты Достижений", GOSSIP_SENDER_MAIN, 108); 
                  player->SEND_GOSSIP_MENU(100001, creature->GetGUID());
                  break;
                case 100:
                  GiveGroundMount(player, creature, 15666, 4000);
                  player->CLOSE_GOSSIP_MENU();
                  break;                
                case 101:
                  GiveGroundMount(player, creature, 15716, 4000);
                  player->CLOSE_GOSSIP_MENU();
                  break;
                case 102:
                  GiveGroundMount(player, creature, 15715, 4000);
                  player->CLOSE_GOSSIP_MENU();
                  break;
                case 103:
                  GiveGroundMount(player, creature, 15714, 4000);
                  player->CLOSE_GOSSIP_MENU();
                  break;               
                case 104:
                  GiveMorph(player, creature, 25484, 10000);
                  player->CLOSE_GOSSIP_MENU();
                  break;                
                case 105:
                  GiveMorph(player, creature, 25373, 10000);
                  player->CLOSE_GOSSIP_MENU();
                  break;                
                case 106:
                  GiveMorph(player, creature, 40289, 10000);
                  player->CLOSE_GOSSIP_MENU();
                  break;                
                case 107:
                  GiveMorph(player, creature, 26917, 10000);
                  player->CLOSE_GOSSIP_MENU();
                  break;
                case 108:
                  GiveMorph(player, creature, 54476, 10000);
                  player->CLOSE_GOSSIP_MENU();
                  break;
            }
            return true;
        }
        
        void GiveFlyMount(Player* player, Creature* creature, uint32 entry, uint32 cost)
        {
           ChatHandler chH = ChatHandler(player);
           uint32 accountId = player->GetSession()->GetAccountId();

           QueryResult check = CharacterDatabase.PQuery("SELECT * FROM character_donate WHERE account = '%u' AND `type` = '2' AND itemEntry = '%u';", accountId, entry);
            if (check)
            {
               chH.PSendSysMessage("Вы уже имеете этого маунта.", cost);
               return;
            }
           
           if (player->GetItemCount(37711, false) < cost)
           {
               chH.PSendSysMessage("Вы не имеете %u Эпической Валюты Достижений для покупки", cost);
               return;
           }
           else
           {
              player->DestroyItemCount(37711, cost, true);
           }
           
          CharacterDatabase.PExecute("insert into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '0', '2', '%u', '%u', '1', '%u');", player->GetGUID(), entry, 0, accountId); 
          chH.PSendSysMessage("Вы купили маунта %u за %u Эпической Валюты Достижений. Для использования маунта пропишите .donate mount fly use %u", entry, cost, entry);          
        } 
        
        void GiveGroundMount(Player* player, Creature* creature, uint32 entry, uint32 cost)
        {
           ChatHandler chH = ChatHandler(player);
           uint32 accountId = player->GetSession()->GetAccountId();

           QueryResult check = CharacterDatabase.PQuery("SELECT * FROM character_donate WHERE account = '%u' AND `type` = '3' AND itemEntry = '%u';", accountId, entry);
            if (check)
            {
               chH.PSendSysMessage("Вы уже имеете этого маунта.", cost);
               return;
            }
           
           if (player->GetItemCount(37711, false) < cost)
           {
               chH.PSendSysMessage("Вы не имеете %u Эпической Валюты Достижений для покупки этого маунта", cost);
               return;
           }
           else
           {
              player->DestroyItemCount(37711, cost, true);
           }
           
          CharacterDatabase.PExecute("insert into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '0', '3', '%u', '%u', '1', '%u');", player->GetGUID(), entry, 0, accountId); 
          chH.PSendSysMessage("Вы купили маунта %u за %u Эпической Валюты Достижений. Для использования маунта пропишите .donate mount ground use %u", entry, cost, entry);          
        }        
        
        void GiveMorph(Player* player, Creature* creature, uint32 entry, uint32 cost)
        {
           ChatHandler chH = ChatHandler(player);
           uint32 accountId = player->GetSession()->GetAccountId();

           QueryResult check = CharacterDatabase.PQuery("SELECT * FROM character_donate WHERE account = '%u' AND `type` = '1' AND itemEntry = '%u';", accountId, entry);
            if (check)
            {
               chH.PSendSysMessage("Вы уже имеете этот морф.", cost);
               return;
            }
           
           if (player->GetItemCount(37711, false) < cost)
           {
               chH.PSendSysMessage("Вы не имеете %u Эпической Валюты Достижений для покупки этого морфа", cost);
               return;
           }
           else
           {
              player->DestroyItemCount(37711, cost, true);
           }
           
          CharacterDatabase.PExecute("insert into `character_donate` (`owner_guid`, `itemguid`, `type`, `itemEntry`, `efircount`, `count`, `account`) value ('%u', '0', '1', '%u', '%u', '1', '%u');", player->GetGUID(), entry, 0, accountId); 
          chH.PSendSysMessage("Вы купили морф %u за %u Эпической Валюты Достижений. Для использования морфа пропишите .donate morph use %u", entry, cost, entry);          
        }
};        

class npc_ball_for_football : public CreatureScript
{
public:
    npc_ball_for_football() : CreatureScript("npc_ball_for_football") { }
    
      struct npc_ball_for_footballAI : public ScriptedAI 
      { 
         npc_ball_for_footballAI(Creature* creature) : ScriptedAI(creature) 
         {
            goal = false;
            GoalTimer = 5000;
            SetCombatMovement(false);
         }
         
         bool goal;
         uint32 GoalTimer;

        void UpdateAI(uint32 diff)
        {
           if (me->GetPositionX() <= 844.5f && !goal)
             if (me->GetPositionY() <= 253.0f && me->GetPositionY() >= 234.0f)
             {
                me->AddAura(58169, me);
                goal = true;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
                me->NearTeleportTo(984.65, 245.60, 0, 0);
                GoalTimer = 15000;
             } 
             else
                me->NearTeleportTo(848.5f, me->GetPositionY(), 0, 0);
             
             if (me->GetPositionX() >= 1124.5f && !goal)
                if (me->GetPositionY() <= 256.0f && me->GetPositionY() >= 237.0f)
                {
                   me->AddAura(65964, me);
                   goal = true;
                   me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
                   me->NearTeleportTo(984.65, 245.60, 0, 0);
                   GoalTimer = 15000;
                }
                else
                   me->NearTeleportTo(1120.5f, me->GetPositionY(), 0, 0);
                
             if (me->GetPositionY() >= 294)
                   me->NearTeleportTo(me->GetPositionX(), 291.9f, 0, 0);
             
             if (me->GetPositionY() <= 195.4)
                   me->NearTeleportTo(me->GetPositionX(), 199.2f, 0, 0);
                
             if (goal)
                if (GoalTimer <= diff)
                {
                   goal = false;
                   me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
                   GoalTimer = 15000;
                } else GoalTimer -= diff;           
        }
         
      };           
      
      CreatureAI* GetAI(Creature* creature) const 
      { 
        return new npc_ball_for_footballAI (creature); 
      } 
};  

class vehicle_football_vehicle : public VehicleScript
{
public:
    vehicle_football_vehicle() : VehicleScript("vehicle_football_vehicle") {}
    
        void OnRemovePassenger(Vehicle* /*transport*/, Unit* player)
        {
            if (!player)
                return;
            if (player->GetTypeId() == TYPEID_PLAYER)
                player->ToPlayer()->TeleportTo(870, 1324, 1116, 561, 4.4);
            
        }
};

void AddSC_custom_events_x20()
{
   new check_on_points();
   new npc_custom_multi_x20();
   new npc_ball_for_football();
   new vehicle_football_vehicle();
}