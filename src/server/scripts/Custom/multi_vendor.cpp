/* hack script for dk base camp teleport */
#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"
#include "Spell.h"
#include "ObjectMgr.h"
#include "PlayerDump.h"


// Options
enum eEnums
{
    CHANGE_ITEMBACK             = 0,
    CHANGE_FACTION              = 1,
    CHANGE_RACE                 = 2,
    CHANGE_GENDER               = 3,

    LEVEL_UP                    = 6,
    CHANGE_NAME                 = 7,
    // ART_LEVEL_UP                = 8,
    BUY_GOLD                    = 9,
    // ADD_BONUS                   = 10,

    CHANGE_FACTION_COUNT        = 200,
    CHANGE_RACE_COUNT           = 150,
    CHANGE_GENDER_COUNT         = 100,
};

enum eService // like types on DB
{
    SERVICE_CUSTOM_FLY_MOUNT    = -11,
    SERVICE_CUSTOM_GR_MOUNT     = -10,
    SERVICE_CUSTOM_MORPH        = -9,
    
    // SERVICE_SELL_ADD_BONUS      = -8,
    // SERVICE_SELL_ART_XP         = -7,
    SERVICE_SELL_GOLD           = -6,
    SERVICE_CHANGE_NAME         = -5,
    SERVICE_CHANGE_GENDER       = -4,
    SERVICE_CHANGE_FACTION      = -3,
    SERVICE_CHANGE_RACE         = -2,
    SERVICE_LEVEL_UP            = -1,
};

class multi_vendor : public CreatureScript
{
public:
    multi_vendor() : CreatureScript("multi_vendor"){}

    bool OnGossipHello(Player* player, Creature* creature)  override
    {
        QueryResult first_check = LoginDatabase.PQuery("SELECT `sc`.`id` FROM `store_categories` AS sc LEFT JOIN `store_category_locales` AS scl ON `scl`.`category` = `sc`.`id` LEFT JOIN `store_category_realms` AS scr ON `scr`.`category` = `sc`.`id` WHERE `sc`.`enable` = '1' AND `scr`.`realm` = '%u' and `sc`.`id` = '-1'", realmID);
        if (!first_check)
            return false;
        
        LocaleConstant localeConstant = player->GetSession()->GetSessionDbLocaleIndex();
        
        QueryResult result_discount = LoginDatabase.PQuery("SELECT rate from store_discounts where enable = 1 AND UNIX_TIMESTAMP(start) <= UNIX_TIMESTAMP() AND UNIX_TIMESTAMP(end) >= UNIX_TIMESTAMP()");
        float rate = 1;
        if (!result_discount)
            rate = 1;
        else
        {
            Field* fielddi = result_discount->Fetch();
            rate = fielddi[0].GetFloat();
            char discounts[500];
            float disc = (1-rate)*100;
            sprintf(discounts, sObjectMgr->GetTrinityString(20037, localeConstant), disc);
            player->ADD_GOSSIP_ITEM(5, discounts, GOSSIP_SENDER_MAIN, 3001);
        }
        
        if (sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS)) // if test, then free donate
            rate = 0;
        
        char printinfo[500]; // Max length
        
        std::stringstream str1;
        uint32 balans;
        
        QueryResult result = LoginDatabase.PQuery("SELECT balans FROM battlenet_accounts where id = %u", player->GetSession()->GetBattlenetAccountId());
        if (result)
        {
            Field* fields = result->Fetch();
            balans = fields[0].GetUInt32();
        }
        else
            balans = 0;
        
        if (sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS)) // if test, then free donate
            balans = 10000;
    
        if (localeConstant == 8) // rus
        {
            str1 << "Ваш баланс: " << balans;
            
            if (balans % 100 > 10 && balans % 100 < 20)
                str1 << " токенов";
            else if (balans % 10 == 1)
                str1 << " токен";
            else if (balans % 10 >= 2 && balans % 10 <= 4 )
                str1 << " токена";
            else
                str1 << " токенов";
        }
        else
            str1 << "Your balance: " << balans << " token";
        
        player->ADD_GOSSIP_ITEM(5, str1.str(), GOSSIP_SENDER_MAIN, 3001);
        
        QueryResult check_service = LoginDatabase.PQuery("SELECT `sp`.`item`, `spr`.`token` FROM `store_products` AS sp LEFT JOIN `store_product_realms` AS spr ON `spr`.`product` = `sp`.`id` WHERE `sp`.`enable` = '1' AND `spr`.`realm` = '%u' AND `spr`.`enable` = 1 and `sp`.`category` = '-1'", realmID); // check service
    
        if (check_service)
        {
            do
            {
                Field* fields = check_service->Fetch();
                int32 id = fields[0].GetInt32();
                int32 cost = fields[1].GetInt32() * rate;
                if (id == SERVICE_SELL_GOLD)
                {
                    sprintf(printinfo, sObjectMgr->GetTrinityString(20045, localeConstant), cost);
                    player->ADD_GOSSIP_ITEM_EXTENDED(0, printinfo, GOSSIP_SENDER_MAIN, 3005, sObjectMgr->GetTrinityString(20046, localeConstant), 0, true); // gold
                }
                if (id == SERVICE_CHANGE_NAME)
                {
                    sprintf(printinfo, sObjectMgr->GetTrinityString(20035, localeConstant), cost);
                    player->ADD_GOSSIP_ITEM_EXTENDED(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_NAME, sObjectMgr->GetTrinityString(20039, localeConstant), 0, false);
                }
                if (id == SERVICE_CHANGE_GENDER)
                {
                    sprintf(printinfo, sObjectMgr->GetTrinityString(20005, localeConstant), cost);
                    player->ADD_GOSSIP_ITEM_EXTENDED(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_GENDER, sObjectMgr->GetTrinityString(20039, localeConstant), 0, false);
                }
                if (id == SERVICE_CHANGE_FACTION)
                {
                    sprintf(printinfo, sObjectMgr->GetTrinityString(20003, localeConstant), cost);
                    player->ADD_GOSSIP_ITEM_EXTENDED(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_FACTION, sObjectMgr->GetTrinityString(20039, localeConstant), 0, false);
                }
                if (id == SERVICE_CHANGE_RACE)
                {
                    sprintf(printinfo, sObjectMgr->GetTrinityString(20004, localeConstant), cost);
                    player->ADD_GOSSIP_ITEM_EXTENDED(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_RACE, sObjectMgr->GetTrinityString(20039, localeConstant), 0, false);
                }
                if (id == SERVICE_LEVEL_UP)
                    player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20032, localeConstant), GOSSIP_SENDER_MAIN, LEVEL_UP);
                
            }while (check_service->NextRow());
        }
        player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20010, localeConstant), GOSSIP_SENDER_MAIN, 3001);
        player->SEND_GOSSIP_MENU(100006,creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)  override
    {
        if(!player || !creature || !player->getAttackers().empty())
            return true;

        if (player->InBattlegroundQueue())
        {
            ChatHandler(player).PSendSysMessage("You should leave from battleground queue.");
            return true;
        }

        QueryResult result_discount = LoginDatabase.PQuery("SELECT rate from store_discounts where enable = 1 AND UNIX_TIMESTAMP(start) <= UNIX_TIMESTAMP() AND UNIX_TIMESTAMP(end) >= UNIX_TIMESTAMP()");
        float rate = 1;
        if (!result_discount)
            rate = 1;
        else
        {
            Field* fielddi = result_discount->Fetch();
            rate = fielddi[0].GetFloat();
        }
        
        if (sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS)) // if test, then free donate
            rate = 0;
        uint32 priceFaction = uint32(400 * rate);
       // if(sWorld->getBoolConfig(CONFIG_FUN_OPTION_ENABLED))
        //    priceFaction = 0;
        uint32 priceName = uint32(200 * rate);
        uint32 priceRace = uint32(300 * rate);
        uint32 priceGender = uint32(200 * rate);
        uint32 priceRecobery = uint32(450 * rate);
        
        QueryResult check_service = LoginDatabase.PQuery("SELECT `sp`.`item`, `spr`.`token` FROM `store_products` AS sp LEFT JOIN `store_product_realms` AS spr ON `spr`.`product` = `sp`.`id` WHERE `sp`.`enable` = '1' AND `spr`.`realm` = '%u' and `sp`.`category` = '-1'", realmID); // check service
        if (check_service)
        {
            do
            {
                Field* fields = check_service->Fetch();
                int32 id = fields[0].GetInt32();
                int32 cost = fields[1].GetInt32();
                
                if (id == SERVICE_CHANGE_FACTION)
                    priceFaction = cost * rate;
                else if (id == SERVICE_CHANGE_RACE)
                    priceRace = cost * rate;
                else if (id == SERVICE_CHANGE_GENDER)
                    priceGender = cost * rate;
                else if (id == SERVICE_CHANGE_NAME)
                    priceName = cost * rate;
            }while (check_service->NextRow());
        }
        //uint32 countefirs = player->GetItemCount(EFIRALS, false);
        uint32 accountId = player->GetSession()->GetAccountId();
        LocaleConstant localeConstant = player->GetSession()->GetSessionDbLocaleIndex();
        if(accountId < 1)
            return true;
        
        uint32 countefirs;
        QueryResult result = LoginDatabase.PQuery("SELECT balans FROM battlenet_accounts where id = %u", player->GetSession()->GetBattlenetAccountId());
        if (result)
        {
            Field* fields = result->Fetch();
            countefirs = fields[0].GetUInt32();

        }
        else
            countefirs = 0;

        bool activate = false;
        ChatHandler chH = ChatHandler(player);

        switch(sender)
        {
            case GOSSIP_SENDER_MAIN:
            {
                switch(action)
                {
                    case CHANGE_FACTION: // 64
                    {
                        if(countefirs < priceFaction)
                        {
                            chH.PSendSysMessage(20000, priceFaction);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }
                        QueryResult result = CharacterDatabase.PQuery("SELECT at_login FROM characters where guid = %u", player->GetGUIDLow());
                        if (result)
                        {
                            Field* fields = result->Fetch();
                            uint32 flags = fields[0].GetUInt32();
                            if (flags & 64) // if  faction
                            {
                                chH.PSendSysMessage(20034);
                                player->CLOSE_GOSSIP_MENU();
                                break;
                            }
                        }
                        else
                        {
                            chH.PSendSysMessage(20034);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '64' WHERE guid = %u", player->GetGUIDLow());
                        if (!sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                        {
                            player->DestroyDonateTokenCount(priceFaction);
                            player->UpdateDonateStatistics(-3);
                            uint8 index = 0;
                            SQLTransaction trans = LoginDatabase.BeginTransaction();
                            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_STORE_ADD_ITEM_LOG_SERVICE);
                            stmt->setUInt32(  index, realmID);
                            stmt->setUInt32(  ++index, player->GetSession()->GetAccountId()); 
                            stmt->setUInt32(  ++index, player->GetSession()->GetBattlenetAccountId()); // select battlenet_account from account where id = %u
                            stmt->setUInt64(  ++index, player->GetGUIDLow());
                            stmt->setUInt64(  ++index, 0); // item_guid
                            stmt->setInt32(  ++index, SERVICE_CHANGE_FACTION); // item entry
                            stmt->setUInt32(  ++index, 1); // item count
                            stmt->setInt64(  ++index, priceFaction); // cost
                            stmt->setUInt32(  ++index, player->getLevel()); // level
                            stmt->setInt32(  ++index, SERVICE_CHANGE_FACTION); // item entry
                            
                            trans->Append(stmt);
                            LoginDatabase.CommitTransaction(trans);
                        }
                      //  LoginDatabase.PExecute("INSERT INTO `store_history` (`realm`, `account`, `bnet_account`, `char_guid`, `char_level`, `product`, `count`, `token`) VALUES (%u, %u, %u, %u, %u, %u, %u, %u);", realmID, player->GetSession()->GetAccountId(), player->GetSession()->GetBattlenetAccountId(), player->GetGUIDLow(), player->getLevel(), -3, 1, priceFaction);
                        activate = true;
                        player->SaveToDB();
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case CHANGE_RACE: // 128
                    {
                        if(countefirs < priceRace)
                        {
                            chH.PSendSysMessage(20000, priceRace);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }
                        QueryResult result = CharacterDatabase.PQuery("SELECT at_login FROM characters where guid = %u", player->GetGUIDLow());
                        if (result)
                        {
                            Field* fields = result->Fetch();
                            uint32 flags = fields[0].GetUInt32();
                            if (flags & 128 || flags & 64) // if race or faction
                            {
                                chH.PSendSysMessage(20034);
                                player->CLOSE_GOSSIP_MENU();
                                break;
                            }
                        }
                        else
                        {
                            chH.PSendSysMessage(20034);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '128' WHERE guid = %u", player->GetGUIDLow());
                        if (!sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                        {
                            player->DestroyDonateTokenCount(priceRace);
                            player->UpdateDonateStatistics(-2);
                            uint8 index = 0;
                            SQLTransaction trans = LoginDatabase.BeginTransaction();
                            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_STORE_ADD_ITEM_LOG_SERVICE);
                            stmt->setUInt32(  index, realmID);
                            stmt->setUInt32(  ++index, player->GetSession()->GetAccountId()); 
                            stmt->setUInt32(  ++index, player->GetSession()->GetBattlenetAccountId()); // select battlenet_account from account where id = %u
                            stmt->setUInt64(  ++index, player->GetGUIDLow());
                            stmt->setUInt64(  ++index, 0); // item_guid
                            stmt->setInt32(  ++index, SERVICE_CHANGE_RACE); // item entry
                            stmt->setUInt32(  ++index, 1); // item count
                            stmt->setInt64(  ++index, priceRace); // cost
                            stmt->setUInt32(  ++index, player->getLevel()); // level
                            stmt->setInt32(  ++index, SERVICE_CHANGE_RACE); // item entry
                            
                            trans->Append(stmt);
                            LoginDatabase.CommitTransaction(trans);
                        }
                        
                     //   LoginDatabase.PExecute("INSERT INTO `store_history` (`realm`, `account`, `bnet_account`, `char_guid`, `char_level`, `product`, `count`, `token`) VALUES (%u, %u, %u, %u, %u, %u, %u, %u);", realmID, player->GetSession()->GetAccountId(), player->GetSession()->GetBattlenetAccountId(), player->GetGUIDLow(), player->getLevel(), -2, 1, priceRace);
                        activate = true;
                        player->SaveToDB();
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case CHANGE_GENDER: // 8
                    {
                        if(countefirs < priceGender)
                        {
                            chH.PSendSysMessage(20000, priceGender);
                            break;
                        }
                        QueryResult result = CharacterDatabase.PQuery("SELECT at_login FROM characters where guid = %u", player->GetGUIDLow());
                        if (result)
                        {
                            Field* fields = result->Fetch();
                            uint32 flags = fields[0].GetUInt32();
                            if (flags & 8 || flags & 64 || flags & 128) // if gender of faction or race
                            {
                                chH.PSendSysMessage(20034);
                                player->CLOSE_GOSSIP_MENU();
                                break;
                            }
                        }
                        else
                        {
                            chH.PSendSysMessage(20034);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '8' WHERE guid = '%u'", player->GetGUIDLow());
                        if (!sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                        {
                            player->DestroyDonateTokenCount(priceGender);
                            player->UpdateDonateStatistics(-4);
                            SQLTransaction trans = LoginDatabase.BeginTransaction();
                            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_STORE_ADD_ITEM_LOG_SERVICE);
                            uint8 index = 0;
                            stmt->setUInt32(  index, realmID);
                            stmt->setUInt32(  ++index, player->GetSession()->GetAccountId()); 
                            stmt->setUInt32(  ++index, player->GetSession()->GetBattlenetAccountId()); // select battlenet_account from account where id = %u
                            stmt->setUInt64(  ++index, player->GetGUIDLow());
                            stmt->setUInt64(  ++index, 0); // item_guid
                            stmt->setInt32(  ++index, SERVICE_CHANGE_GENDER); // item entry
                            stmt->setUInt32(  ++index, 1); // item count
                            stmt->setInt64(  ++index, priceFaction); // cost
                            stmt->setUInt32(  ++index, player->getLevel()); // level
                            stmt->setInt32(  ++index, SERVICE_CHANGE_GENDER); // item entry
                            
                            trans->Append(stmt);
                            LoginDatabase.CommitTransaction(trans);
                        }
                        
                       // LoginDatabase.PExecute("INSERT INTO `store_history` (`realm`, `account`, `bnet_account`, `char_guid`, `char_level`, `product`, `count`, `token`) VALUES (%u, %u, %u, %u, %u, %u, %u, %u);", realmID, player->GetSession()->GetAccountId(), player->GetSession()->GetBattlenetAccountId(), player->GetGUIDLow(), player->getLevel(), -4, 1, priceGender);
                        activate = true;
                        player->SaveToDB();
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case CHANGE_NAME: // 1
                    {
                        if(countefirs < priceName)
                        {
                            chH.PSendSysMessage(20000, priceName);
                            break;
                        }
                        QueryResult result = CharacterDatabase.PQuery("SELECT at_login FROM characters where guid = %u", player->GetGUIDLow());
                        if (result)
                        {
                            Field* fields = result->Fetch();
                            uint32 flags = fields[0].GetUInt32();
                            if (flags & 1 || flags & 64 || flags & 128 || flags & 8) // if name or race or faction or gender
                            {
                                chH.PSendSysMessage(20034);
                                player->CLOSE_GOSSIP_MENU();
                                break;
                            }
                        }
                        else
                        {
                            chH.PSendSysMessage(20034);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_RENAME);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '1' WHERE guid = '%u'", player->GetGUIDLow());
                        if (!sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                        {
                            player->DestroyDonateTokenCount(priceName);
                            player->UpdateDonateStatistics(-5);
                            SQLTransaction trans = LoginDatabase.BeginTransaction();
                            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_STORE_ADD_ITEM_LOG_SERVICE);
                            uint8 index = 0;
                            stmt->setUInt32(  index, realmID);
                            stmt->setUInt32(  ++index, player->GetSession()->GetAccountId()); 
                            stmt->setUInt32(  ++index, player->GetSession()->GetBattlenetAccountId()); // select battlenet_account from account where id = %u
                            stmt->setUInt64(  ++index, player->GetGUIDLow());
                            stmt->setUInt64(  ++index, 0); // item_guid
                            stmt->setInt32(  ++index, SERVICE_CHANGE_NAME); // item entry
                            stmt->setUInt32(  ++index, 1); // item count
                            stmt->setInt64(  ++index, priceFaction); // cost
                            stmt->setUInt32(  ++index, player->getLevel()); // level
                            stmt->setInt32(  ++index, SERVICE_CHANGE_NAME); // item entry
                            
                            trans->Append(stmt);
                            LoginDatabase.CommitTransaction(trans);
                        }
                        
                       // LoginDatabase.PExecute("INSERT INTO `store_history` (`realm`, `account`, `bnet_account`, `char_guid`, `char_level`, `product`, `count`, `token`) VALUES (%u, %u, %u, %u, %u, %u, %u, %u);", realmID, player->GetSession()->GetAccountId(), player->GetSession()->GetBattlenetAccountId(), player->GetGUIDLow(), player->getLevel(), -4, 1, priceGender);
                        activate = true;
                        player->SaveToDB();
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case LEVEL_UP:
                    {
                        player->PlayerTalkClass->ClearMenus();
                        if (player->getLevel() < 100)
                            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, sObjectMgr->GetTrinityString(30100, localeConstant), GOSSIP_SENDER_MAIN, 3003, sObjectMgr->GetTrinityString(20040, localeConstant), 0, true); // level
                        
                        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, sObjectMgr->GetTrinityString(30104, localeConstant), GOSSIP_SENDER_MAIN, 3004, sObjectMgr->GetTrinityString(20040, localeConstant), 0, true); // calculate
                        player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(30102, localeConstant), GOSSIP_SENDER_MAIN, 3001); // exit
                        player->SEND_GOSSIP_MENU(100005, creature->GetGUID());
                        return true;
                    }
                    break;                   
                    case 3001:
                        player->CLOSE_GOSSIP_MENU();
                        return true;
                        break;
                }
                break;
            }
        }

        // if(activate)
            // chH.PSendSysMessage(LANG_CUSTOMIZE_PLAYER, chH.GetNameLink(player).c_str());

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
    
    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        player->PlayerTalkClass->ClearMenus();
        ChatHandler chH = ChatHandler(player);
        
        if (action == 3003 || action == 3004) // buy level
        {           
            ChatHandler chH = ChatHandler(player);
            
            if (!code)
            {
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
            
            float efir;
            efir = 0;
            uint32 ucode = atoi(code);
            if (!ucode)
            {
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
            uint32 level = player->getLevel();
            
            if (ucode > 100|| ucode <= level)
            {
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
            
            for (uint32 i = level+1; i <= ucode; i++)
            {
                if(float const* price = sObjectMgr->GetPriceForLevelUp(i))
                {
                    if (price)
                    {
                        efir += *price; // how many tokens need for get level i
                    }
                    else
                    {
                        chH.PSendSysMessage(20038);
                        player->CLOSE_GOSSIP_MENU();
                        return false;
                    }
                }
            }
            
            float efirend = ceil(efir);
            uint32 efirendu = uint32(efirend);
            if ((!efirendu || efirendu <= 0) && !sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
            {
                chH.PSendSysMessage(20038);
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
            
            if (sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                efirendu = 0;
            
            // calculate gold with arithmetic progression =)
            uint32 start_cost = 1;
            uint32 d = 2;
            uint32 a1 = start_cost + d*(level);
            uint32 an = start_cost + d*(ucode - 1);
            uint64 sum = (a1 + an)*(ucode - level) / 2.; // i hope, that i am smart
            uint64 gold = sum*10000;
            if (action == 3004)
                chH.PSendSysMessage(30105, ucode, efirendu, sum);
            else if (action == 3003)
            {
                if (player->HasDonateToken(efirendu))
                {
                    if (!sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                    {
                        player->DestroyDonateTokenCount(efirendu);
                        player->UpdateDonateStatistics(-1);
                        uint8 index = 0;
                        SQLTransaction trans = LoginDatabase.BeginTransaction();
                        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_STORE_ADD_ITEM_LOG_SERVICE);
                        stmt->setUInt32(  index, realmID);
                        stmt->setUInt32(  ++index, player->GetSession()->GetAccountId()); 
                        stmt->setUInt32(  ++index, player->GetSession()->GetBattlenetAccountId()); // select battlenet_account from account where id = %u
                        stmt->setUInt64(  ++index, player->GetGUIDLow());
                        stmt->setUInt64(  ++index, 0); // item_guid
                        stmt->setInt32(  ++index, SERVICE_LEVEL_UP); // item entry
                        stmt->setUInt32(  ++index, (ucode-level)); // item count
                        stmt->setUInt32(  ++index, efirendu); // cost
                        stmt->setUInt32(  ++index, level); // level
                        stmt->setInt32(  ++index, SERVICE_LEVEL_UP); // item entry
                        
                        trans->Append(stmt);
                        LoginDatabase.CommitTransaction(trans);
                        
                        //  LoginDatabase.PExecute("INSERT INTO `store_history` (`realm`, `account`, `bnet_account`, `char_guid`, `char_level`, `product`, `count`, `token`) VALUES (%u, %u, %u, %u, %u, %u, %u, %u);", realmID, player->GetSession()->GetAccountId(), player->GetSession()->GetBattlenetAccountId(), player->GetGUIDLow(), level, -1, ucode, efirendu);
                    }
                    player->GiveLevel(ucode);
                    
                    uint64 moneyuser = player->GetMoney();
                    int64 newmoney = moneyuser + gold;
                    if (newmoney >= MAX_MONEY_AMOUNT)
                    {
                        newmoney = MAX_MONEY_AMOUNT;
                        player->SetMoney(newmoney);
                    }
                    else
                        player->ModifyMoney(int64(gold));             
                    player->SaveToDB();
                }
                else
                    chH.PSendSysMessage(20000, efirendu);
            }
        }
        if (action == 3005) // buy gold
        {
            ChatHandler chH = ChatHandler(player);
            
            if (!code)
            {
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
            
            uint64 ucode = atoi(code);
            if (!ucode)
            {
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
            uint64 addmoney = (ucode*10000)*1000;
            
            if (!addmoney || addmoney <= 0)
            {
                chH.PSendSysMessage(20038);
                player->CLOSE_GOSSIP_MENU();
                return false;
            }
                
            uint64 moneyuser = player->GetMoney();
            
            int64 newmoney = moneyuser + addmoney;
            if (newmoney > MAX_MONEY_AMOUNT)
            {
                chH.PSendSysMessage(20038);
                player->CLOSE_GOSSIP_MENU();
                return false;
            } 
            
            QueryResult findCost = LoginDatabase.PQuery("SELECT `spr`.`token` FROM `store_products` AS sp LEFT JOIN `store_product_realms` AS spr ON `spr`.`product` = `sp`.`id` WHERE `sp`.`enable` = '1' AND `spr`.`realm` = '%u' and `sp`.`category` = '-1' and `sp`.`item` = '-6'", realmID);
        
            if (!findCost)
            {
                chH.PSendSysMessage(20038);
                player->CLOSE_GOSSIP_MENU();
                return false;
            } 
            
            Field* fields = findCost->Fetch();
            uint32 cost = fields[0].GetUInt32();
            if (!cost)
            {
                chH.PSendSysMessage(20038);
                player->CLOSE_GOSSIP_MENU();
                return false;
            } 
            
            QueryResult result_discount = LoginDatabase.PQuery("SELECT rate from store_discounts where enable = 1 AND UNIX_TIMESTAMP(start) <= UNIX_TIMESTAMP() AND UNIX_TIMESTAMP(end) >= UNIX_TIMESTAMP()");
            float rate = 1;
            if (!result_discount)
                rate = 1;
            else
            {
                Field* fielddi = result_discount->Fetch();
                rate = fielddi[0].GetFloat();
            }
              
            int64 tokens = ucode*cost*rate;
            if ((!tokens || tokens <= 0) && !sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
            {
                chH.PSendSysMessage(20038);
                player->CLOSE_GOSSIP_MENU();
                return false;
            } 
            
            if (player->HasDonateToken(tokens))
            {
                if (!sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS))
                {
                    player->DestroyDonateTokenCount(tokens);
                    player->UpdateDonateStatistics(-6);
                    uint8 index = 0;
                    SQLTransaction trans = LoginDatabase.BeginTransaction();
                    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_STORE_ADD_ITEM_LOG_SERVICE);
                    stmt->setUInt32(  index, realmID);
                    stmt->setUInt32(  ++index, player->GetSession()->GetAccountId()); 
                    stmt->setUInt32(  ++index, player->GetSession()->GetBattlenetAccountId()); // select battlenet_account from account where id = %u
                    stmt->setUInt64(  ++index, player->GetGUIDLow());
                    stmt->setUInt64(  ++index, 0); // item_guid
                    stmt->setInt32(  ++index, SERVICE_SELL_GOLD); // item entry
                    stmt->setUInt32(  ++index, ucode*1000); // item count
                    stmt->setUInt32(  ++index, tokens); // cost
                    stmt->setUInt32(  ++index, player->getLevel()); // level
                    stmt->setInt32(  ++index, SERVICE_SELL_GOLD); // item entry
                    
                    trans->Append(stmt);
                    LoginDatabase.CommitTransaction(trans);
                }
                player->ModifyMoney(int64(addmoney));
                player->SaveToDB();
                chH.PSendSysMessage(20050, ucode*1000, tokens);
            }
            else
                chH.PSendSysMessage(20000, tokens);
            
        }
        
        player->CLOSE_GOSSIP_MENU();
        return true;        
    }
    
    std::string GetClassNameById(uint8 id)
    {
        std::string sClass = "";
        switch (id)
        {
            case CLASS_WARRIOR:         sClass = "Warrior";        break;
            case CLASS_PALADIN:         sClass = "Pala";           break;
            case CLASS_HUNTER:          sClass = "Hunt";           break;
            case CLASS_ROGUE:           sClass = "Rogue";          break;
            case CLASS_PRIEST:          sClass = "Priest";         break;
            case CLASS_DEATH_KNIGHT:    sClass = "DK";             break;
            case CLASS_SHAMAN:          sClass = "Shama";          break;
            case CLASS_MAGE:            sClass = "Mage";           break;
            case CLASS_WARLOCK:         sClass = "Warlock";        break;
            case CLASS_DRUID:           sClass = "Druid";          break;
        }
        return sClass;
    }
};

class item_back : public CreatureScript
{
public:
    item_back() : CreatureScript("item_back"){}

    bool OnGossipHello(Player* player, Creature* creature)  override
    {
        QueryResult result = LoginDatabase.PQuery("SELECT `s_h`.`item_guid`, `s_h`.`item`, `s_h`.`count`, `s_h`.`token` FROM `store_history` AS s_h JOIN `store_product_realms` AS s_p_r ON `s_p_r`.`product` = `s_h`.`product` WHERE `s_h`.`char_guid` = '%u' AND `s_h`.`status` IN ('0', '6') AND `s_h`.`item` > '0' AND `s_h`.`realm` = '%u' AND `s_p_r`.`return` = '1' AND `s_p_r`.`realm` = '%u'", player->GetGUIDLow(), realmID, realmID);
        if (!result)
        {
            LocaleConstant localeConstant = player->GetSession()->GetSessionDbLocaleIndex();
            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20008, localeConstant), GOSSIP_SENDER_MAIN, CHANGE_ITEMBACK);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
        }
        else
        {
            LocaleConstant localeConstant = player->GetSession()->GetSessionDbLocaleIndex();
            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20006, localeConstant), GOSSIP_SENDER_MAIN, CHANGE_ITEMBACK);
            do
            {
                Field* fields = result->Fetch();
                uint32 item_guid = fields[0].GetUInt32();
                uint32 entry = fields[1].GetUInt32();
                uint32 count = fields[2].GetUInt32();
                uint32 token = fields[3].GetUInt32()*0.7;
                ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(entry);
                if (pProto && pProto->Stackable < 2)
                {
                    std::string Name = pProto->Name1;

                    if (localeConstant >= 0)
                    {
                        if (ItemLocale const* il = sObjectMgr->GetItemLocale(pProto->ItemId))
                            ObjectMgr::GetLocaleString(il->Name, localeConstant, Name);
                    }
                    char printinfo[500];
                    sprintf(printinfo, sObjectMgr->GetTrinityString(20043, localeConstant), Name.c_str(), token);
                    player->ADD_GOSSIP_ITEM_EXTENDED(0, printinfo, GOSSIP_SENDER_MAIN, item_guid, sObjectMgr->GetTrinityString(20042, localeConstant), 0, false);
                }
            } while (result->NextRow());
        }

        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "item_back sender %u, action %u", sender, action);

        if(!player || !creature || sender != GOSSIP_SENDER_MAIN || !player->getAttackers().empty())
            return true;

        ChatHandler chH = ChatHandler(player);

        if (player->InBattlegroundQueue())
        {
            chH.PSendSysMessage("You should leave from battleground queue.");
            return true;
        }

        sLog->outDebug(LOG_FILTER_NETWORKIO, "item_back sender %u, action %u", sender, action);

        if(action > 0)
        {
            QueryResult result = LoginDatabase.PQuery("SELECT `s_h`.`char_guid`, `s_h`.`item`, `s_h`.`token`, `s_h`.`count` FROM `store_history` AS s_h JOIN `store_product_realms` AS s_p_r ON `s_p_r`.`product` = `s_h`.`product` WHERE `s_h`.`item_guid` = '%u' AND `s_h`.`status` IN ('0', '6') AND `s_h`.`realm` = '%u' AND`s_h`.`item` > 0 AND `s_p_r`.`return` = '1' AND `s_p_r`.`realm` = '%u'", action, realmID, realmID);
            if(!result)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            Field* fields = result->Fetch();
            uint64 owner_guid = fields[0].GetUInt64();
            uint32 entry = fields[1].GetUInt32();
            uint32 efircount = fields[2].GetUInt32();
            uint32 count = fields[3].GetUInt32();
            uint32 countitem = player->GetItemCount(entry, false);

            if(owner_guid != player->GetGUIDLow())
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            QueryResult result2 = CharacterDatabase.PQuery("SELECT `owner_guid` FROM item_instance WHERE guid = '%u'", action);
            if(!result2 || countitem == 0)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            Field* fields2 = result2->Fetch();
            uint32 owner_guid2 = fields2[0].GetUInt32();
            if(owner_guid != owner_guid2)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            if(player->GetGUIDLow() != owner_guid2)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            //Warning!! action should be uint64
            if (Item *item = GetItemByGuid(action, player))
            {
                sLog->outDebug(LOG_FILTER_EFIR, "ItemBack item %u; count tokens = %u playerGUID %u, itemGUID %u", item->GetEntry(), count, player->GetGUIDLow(), action);

                { // status
                    SQLTransaction transs = LoginDatabase.BeginTransaction();

                    uint8 index = 0;
                    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_HISTORY_RETURN);
                    // stmt->setString(  ++index, TimeToTimestampStr(time(NULL)).c_str());
                    stmt->setUInt32(  index, action); // return
                    stmt->setUInt32(  ++index, realmID);  
                            
                    transs->Append(stmt);
                    LoginDatabase.CommitTransaction(transs); 
                }
                
                player->DestroyItemCount(item, count, true);
                player->AddDonateTokenCount(uint32(efircount * 0.7));
                player->SaveToDB();      
           
                chH.PSendSysMessage(20002, uint32(efircount * 0.7));
            }
        }
        else
            chH.PSendSysMessage(20001);

        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    Item* GetItemByGuid(uint64 guid, Player* player) const
    {
        for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item *pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if (pItem->GetGUIDLow() == guid)
                    return pItem;

        for (int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            if (Item *pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if (pItem->GetGUIDLow() == guid)
                    return pItem;

        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Bag *pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if (pItem->GetGUIDLow() == guid)
                            return pItem;

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            if (Bag *pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if (pItem->GetGUIDLow() == guid)
                            return pItem;
        return NULL;
    }
};


class char_transfer : public CreatureScript
{
public:
    char_transfer() : CreatureScript("char_transfer"){}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        QueryResult result = LoginDatabase.PQuery("SELECT `to_realm`, `name` FROM realm_transfer WHERE from_realm = '%u';", realmID);
        if (!result)
        {
            LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20027, loc_idx), GOSSIP_SENDER_MAIN, 0);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
        }
        else
        {
            LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20031, loc_idx), GOSSIP_SENDER_MAIN, 0);
            do
            {
                Field* fields = result->Fetch();
                uint32 realm = fields[0].GetUInt32();
                std::string name = fields[1].GetString();

                player->ADD_GOSSIP_ITEM(0, name, GOSSIP_SENDER_MAIN, realm);
            }while (result->NextRow());
        }

        player->SetTransferId(0);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        if(!player || !creature || !player->getAttackers().empty())
            return true;

        player->PlayerTalkClass->ClearMenus();
        ChatHandler chH = ChatHandler(player);

        switch (sender)
        {
            case GOSSIP_SENDER_MAIN:
            {
                if(QueryResult check_wpe = LoginDatabase.PQuery("SELECT `name` FROM realm_transfer WHERE from_realm = '%u' AND to_realm = '%u';", realmID, action))
                    player->SetTransferId(action);
                else
                    return true;

                QueryResult result = CharacterDatabase.PQuery("SELECT guid, name FROM characters WHERE account = '%u' AND transfer = 0", player->GetSession()->GetAccountId());
                if (!result)
                {
                    LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
                    player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20027, loc_idx), GOSSIP_SENDER_MAIN, 0);
                    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
                }
                else
                {
                    LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
                    player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20028, loc_idx), GOSSIP_SENDER_MAIN, 0);
                    do
                    {
                        Field* fields = result->Fetch();
                        uint32 guid = fields[0].GetUInt32();
                        std::string name = fields[1].GetString();

                        player->ADD_GOSSIP_ITEM(0, name, GOSSIP_SENDER_INN_INFO, guid);
                    }while (result->NextRow());
                }
                break;
            }
            case GOSSIP_SENDER_INN_INFO:
            {
                if(action > 0)
                {
                    QueryResult result = CharacterDatabase.PQuery("SELECT guid, name FROM characters WHERE guid = '%u' AND account = '%u' AND transfer = 0", action, player->GetSession()->GetAccountId());
                    if(!result)
                    {
                        player->CLOSE_GOSSIP_MENU();
                        return true;
                    }

                    Field* fields = result->Fetch();
                    uint32 guid = fields[0].GetUInt32();
                    std::string name = fields[1].GetString();
                    uint32 account_id = player->GetSession()->GetAccountId();
                    std::string dump;

                    if(PlayerDumpWriter().WriteDump(uint32(guid), dump) != DUMP_SUCCESS)
                    {
                        chH.PSendSysMessage(LANG_COMMAND_EXPORT_FAILED);
                        chH.SetSentErrorMessage(true);
                        return false;
                    }

                    PreparedStatement * stmt = LoginDatabase.GetPreparedStatement(LOGIN_SET_DUMP);
                    if(stmt)
                    {
                        stmt->setUInt32(0, account_id);
                        stmt->setUInt32(1, guid);
                        stmt->setUInt32(2, realmID);
                        stmt->setUInt32(3, player->GetTransferId());
                        stmt->setUInt32(4, 1);
                        stmt->setString(5, dump);
                        LoginDatabase.Execute(stmt);
                    }
                    CharacterDatabase.PQuery("UPDATE characters SET deleteInfos_Name = name, deleteInfos_Account = account, deleteDate = UNIX_TIMESTAMP(), name = '', account = 0, `transfer` = '%u' WHERE guid = %u", player->GetTransferId(), guid);

                    if(player->GetGUIDLow() == guid)
                        player->GetSession()->KickPlayer();
                }
                break;
            }
        }
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }
};

class many_in_one_donate : public CreatureScript
{
    public:
        many_in_one_donate() : CreatureScript("many_in_one_donate") { }

        bool OnGossipHello(Player* player, Creature* creature)  override
        {
            LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
            
            QueryResult result_discount = LoginDatabase.PQuery("SELECT rate from store_discounts where enable = 1 AND UNIX_TIMESTAMP(start) <= UNIX_TIMESTAMP() AND UNIX_TIMESTAMP(end) >= UNIX_TIMESTAMP()");
            float rate = 1;
            if (!result_discount)
                rate = 1;
            else
            {
                Field* fielddi = result_discount->Fetch();
                rate = fielddi[0].GetFloat();
                char discounts[500];
                float disc = (1-rate)*100;
                sprintf(discounts, sObjectMgr->GetTrinityString(20036, loc_idx), disc);
                player->ADD_GOSSIP_ITEM(5, discounts, GOSSIP_SENDER_MAIN, 3001);
            }
            
            std::stringstream str1;
            uint32 balans;
        
            QueryResult result = LoginDatabase.PQuery("SELECT balans FROM battlenet_accounts where id = %u", player->GetSession()->GetBattlenetAccountId());
            if (result)
            {
                Field* fields = result->Fetch();
                balans = fields[0].GetUInt32();

            }
            else
                balans = 0;
            
            if (sWorld->getBoolConfig(CONFIG_DONATE_ON_TESTS)) // if test, then free donate
                balans = 10000;

            if (loc_idx == 8) // rus
            {
                str1 << "Ваш баланс: " << balans;
                
                if (balans % 100 > 10 && balans % 100 < 20)
                    str1 << " токенов";
                else if (balans % 10 == 1)
                    str1 << " токен";
                else if (balans % 10 >= 2 && balans % 10 <= 4 )
                    str1 << " токена";
                else
                    str1 << " токенов";
            }
            else
                str1 << "Your balance: " << balans << " token";
            
            player->ADD_GOSSIP_ITEM(5, str1.str(), GOSSIP_SENDER_MAIN, 230099);
            
            if(std::vector<DonVenCat> const* donvencat = sObjectMgr->GetDonateVendorCat(0))
            {
                if (donvencat)
                {
                    for (std::vector<DonVenCat>::const_iterator itr = donvencat->begin(); itr != donvencat->end(); ++itr)
                    {
                        if ((*itr).action == -1)
                            continue;
                        
                        switch (loc_idx)
                        {
                            case 0:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEn, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 1:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameKo, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 2:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameFr, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 3:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameDe, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 4:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameCn, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 5:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameTw, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 6:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEs, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 7:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEm, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 8:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameRu, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 9:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NamePt, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 10:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameIt, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            case 11:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameUa, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;
                            default:
                                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEn, GOSSIP_SENDER_MAIN, (*itr).action);
                                break;                          
                        }
                    }
                }
                else
                {
                    player->CLOSE_GOSSIP_MENU();
                    return false;
                }
            }           
            player->SEND_GOSSIP_MENU(60000, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
        {
            LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
            player->PlayerTalkClass->ClearMenus();
            switch (sender)
            {
                case GOSSIP_SENDER_MAIN:
                {
                    if (action == 230099)
                    {
                        player->CLOSE_GOSSIP_MENU();
                        return true;
                    }
                    
                    if (action >= 230100)
                    {
                        player->GetSession()->SendListInventory(creature->GetGUID(), action);
                        player->CustomMultiDonate = action;
                    }
                    else
                    {
                        QueryResult result_discount = LoginDatabase.PQuery("SELECT rate from store_discounts where enable = 1 AND UNIX_TIMESTAMP(start) <= UNIX_TIMESTAMP() AND UNIX_TIMESTAMP(end) >= UNIX_TIMESTAMP()");
                        float rate = 1;
                        if (!result_discount)
                            rate = 1;
                        else
                        {
                            Field* fielddi = result_discount->Fetch();
                            rate = fielddi[0].GetFloat();
                            char discounts[500];
                            float disc = (1-rate)*100;
                            sprintf(discounts, sObjectMgr->GetTrinityString(20036, loc_idx), disc);
                            player->ADD_GOSSIP_ITEM(5, discounts, GOSSIP_SENDER_MAIN, 3001);
                        }
                        
                        std::stringstream str1;
                        uint32 balans;
                        
                        QueryResult result = LoginDatabase.PQuery("SELECT balans FROM battlenet_accounts where id = %u", player->GetSession()->GetBattlenetAccountId());
                        if (result)
                        {
                            Field* fields = result->Fetch();
                            balans = fields[0].GetUInt32();

                        }
                        else
                            balans = 0;

                        if (loc_idx == 8) // rus
                        {
                            str1 << "Ваш баланс: " << balans;
                            
                            if (balans % 100 > 10 && balans % 100 < 20)
                                str1 << " токенов";
                            else if (balans % 10 == 1)
                                str1 << " токен";
                            else if (balans % 10 >= 2 && balans % 10 <= 4 )
                                str1 << " токена";
                            else
                                str1 << " токенов";
                        }
                        else
                            str1 << "Your balance: " << balans << " token";
                                    
                        player->ADD_GOSSIP_ITEM(5, str1.str(), GOSSIP_SENDER_MAIN, 230099);   
                        
                        // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Дебаг 0", GOSSIP_SENDER_MAIN, 1);
                        if(std::vector<DonVenCat> const* donvencat = sObjectMgr->GetDonateVendorCat(action))
                        {
                         //   player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Дебаг 1", GOSSIP_SENDER_MAIN, 1);
                            if (donvencat)
                            {
                                // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Дебаг 2", GOSSIP_SENDER_MAIN, 1);
                                for (std::vector<DonVenCat>::const_iterator itr = donvencat->begin(); itr != donvencat->end(); ++itr)
                                {
                                    if ((*itr).action == -1)
                                        continue;
                                    
                                    switch (loc_idx)
                                    {
                                        case 0:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEn, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 1:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameKo, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 2:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameFr, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 3:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameDe, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 4:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameCn, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 5:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameTw, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 6:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEs, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 7:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEm, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 8:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameRu, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 9:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NamePt, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 10:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameIt, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        case 11:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameUa, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;
                                        default:
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, (*itr).NameEn, GOSSIP_SENDER_MAIN, (*itr).action);
                                            break;                        
                                    }
                                }
                                if (action != 0)
                                {
                                    QueryResult result = LoginDatabase.PQuery("SELECT pid FROM store_categories where id = %u", action);
                                    if (result)
                                    {
                                        Field* fields = result->Fetch();
                                        uint32 back = fields[0].GetUInt32();
                                        
                                        if (loc_idx == 8)
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Назад", GOSSIP_SENDER_MAIN, back);
                                        else
                                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Back", GOSSIP_SENDER_MAIN, back);
                                    }
                                }
                            }   
                            else
                            {
                                player->CLOSE_GOSSIP_MENU();
                                // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Дебаг 3", GOSSIP_SENDER_MAIN, 1);
                                return true;
                            }
                        }
                        else
                        {
                            player->CLOSE_GOSSIP_MENU();
                            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Дебаг 4", GOSSIP_SENDER_MAIN, 1);
                            return true;
                        }             

                        player->SEND_GOSSIP_MENU(60000, creature->GetGUID());
                    }
                }
                break;
            }
            return true;
        }
};


  // old
// class many_in_one_donate : public CreatureScript
// {
    // public:
        // many_in_one_donate() : CreatureScript("many_in_one_donate") { }

        // bool OnGossipHello(Player* player, Creature* creature)
        // {
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Табарды", GOSSIP_SENDER_MAIN, 220026);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Оружие", GOSSIP_SENDER_MAIN, 220013);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Осада Оргриммара (566 ilvl)", GOSSIP_SENDER_MAIN, 220012);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Осада Оргриммара (572 ilvl) #1", GOSSIP_SENDER_MAIN, 220021);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Осада Оргриммара (572 ilvl) #2", GOSSIP_SENDER_MAIN, 220023);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Тир 15-16", GOSSIP_SENDER_MAIN, 220015);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "PvP 15 сезон 550 ilvl (офф)", GOSSIP_SENDER_MAIN, 220017);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "PvP 15 сезон 550 ilvl ", GOSSIP_SENDER_MAIN, 220018);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Ювелирные изделия", GOSSIP_SENDER_MAIN, 220014);           

            // player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            // return true;
        // }

        // bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        // {
            // player->PlayerTalkClass->ClearMenus();
            // player->GetSession()->SendListInventory(creature->GetGUID(), action);
            // player->CustomMultiDonate = action;
            // return true;
        // }
// };

// 220031
// class many_in_one_donate_trans : public CreatureScript
// {
    // public:
        // many_in_one_donate_trans() : CreatureScript("many_in_one_donate_trans") { }

        // bool OnGossipHello(Player* player, Creature* creature)
        // {
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Верховые животные #1", GOSSIP_SENDER_MAIN, 220016);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Верховые животные #2", GOSSIP_SENDER_MAIN, 220025);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Спутники", GOSSIP_SENDER_MAIN, 200204);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Рисовки и раритетные вещи", GOSSIP_SENDER_MAIN, 200203);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Накидки и рубашки с аурами", GOSSIP_SENDER_MAIN, 200205);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Вещи на трансмогрификацию", GOSSIP_SENDER_MAIN, 1);
            // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Легендарные камни", GOSSIP_SENDER_MAIN, 250116);
            

            // player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            // return true;
        // }

        // bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        // {
            // player->PlayerTalkClass->ClearMenus();
            // switch(action)
            // {
                // case 1:
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Т1", GOSSIP_SENDER_MAIN, 220182);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Т2", GOSSIP_SENDER_MAIN, 220183);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Т3", GOSSIP_SENDER_MAIN, 220184);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Т4", GOSSIP_SENDER_MAIN, 220185);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Т5", GOSSIP_SENDER_MAIN, 220186);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Т6", GOSSIP_SENDER_MAIN, 220187);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "а1", GOSSIP_SENDER_MAIN, 220188);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "а2", GOSSIP_SENDER_MAIN, 220189);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "а3", GOSSIP_SENDER_MAIN, 220190);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "а4", GOSSIP_SENDER_MAIN, 220191);
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Ткань", GOSSIP_SENDER_MAIN, 250112);            
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Кожа", GOSSIP_SENDER_MAIN, 250111);            
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Кольчуга", GOSSIP_SENDER_MAIN, 250113);            
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Латы", GOSSIP_SENDER_MAIN, 250110);            
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Оружие", GOSSIP_SENDER_MAIN, 250114);            
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Плащи", GOSSIP_SENDER_MAIN, 250115); 
                    
                    // player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, "Трансмогрификация (испытания в подземельях. золото)", GOSSIP_SENDER_MAIN, 220024);            
                    // player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
                    // break;    
                // default:
                    // player->GetSession()->SendListInventory(creature->GetGUID(), action);
                    // player->CustomMultiDonate = action;
                    // break;
            // }
            // return true;
        // }
// };

void AddSC_multi_vendor()
{
    new multi_vendor();
    new item_back();
    new char_transfer();
    
    new many_in_one_donate();
    // new many_in_one_donate_trans();
}