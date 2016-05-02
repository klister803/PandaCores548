/*
 * Copyright (C) 2008-2016 TrinityCore <http://uwow.biz/>
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
 *
 * This is for LK x1 
 */

#include "ScriptPCH.h"
#include "Player.h"

#define DEFAULT_MESSAGE 100000

struct VisualData
{
    uint32 Menu;
    uint32 Submenu;
    uint32 Icon;
    uint32 Id;
    std::string Name;
    uint32 PriceInGold;
    std::string Questions;
};
 
VisualData vData[] =
{
    { 1, 0, GOSSIP_ICON_BATTLE, 3789, "Берсерк (Berserk)" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета? (Are you really want use it`s enchantment for 1 Gold Coint?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3854, "Сила заклинаний" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3273, "Смертельный лёд" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3225, "Палач" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3870, "Высасывание крови" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 1899, "Нечестивое оружие" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2674, "Всплеск чар" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2675, "Военачальник" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2671, "Тайная и огненная сила заклинаний" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2672, "Темная и ледяная сила заклинаний" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3365, "Руна сломанных мечей" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2673, "Мангуст" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2343, "Сила заклинаний" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 1, 2, GOSSIP_ICON_TALK, 0, "Следующая страница ->" , 0, "" },
 
    { 2, 0, GOSSIP_ICON_BATTLE, 425, "Синее свечение" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3855, "Сила заклинаний 3" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1894, "Ледяное оружие" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1103, "Ловкость" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1898, "Похищение жизни" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3345, "Сила Земли" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1743, "Фиолетовое свечение" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3093, "Белое свечение" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1900, "Рыцарь" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3846, "Сила заклинания 2" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1606, "Сила Атаки" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 283, "Неистовство ветра" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1, "Камнедробитель" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 2, 3, GOSSIP_ICON_TALK, 0, "Следующая страница ->" , 0, "" },
    { 2, 1, GOSSIP_ICON_TALK, 0, "<- Предыдущая страница" , 0, "" },
 
    { 3, 0, GOSSIP_ICON_BATTLE, 3265, "Блеклое синее свечение" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 2, "Ледяное клеймо" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 3, "Язык пламени" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 3266, "Праведное покрытие оружия" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 1903, "Дух" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 13, "Заострение" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 26, "Мороз" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 7, "Смертельный яд" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 803, "Огненное оружие" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 1896, "Урон от оружия" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 2666, "Интеллект" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 25, "Ледяная тьма" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 0, GOSSIP_ICON_BATTLE, 3369, "Руна оплавленного ледника" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 3, 4, GOSSIP_ICON_TALK, 0, "Следующая страница ->" , 0, "" },
    { 3, 2, GOSSIP_ICON_TALK, 0, "<- Предыдущая страница" , 0, "" },
    
    { 4, 0, GOSSIP_ICON_BATTLE, 3368, "Руна павшего рыцаря" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 3869, "Отведение удара" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4098, "Ветроступ" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4099, "Обвал" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4097, "Силовой поток" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4067, "Лавина" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 5035, "Легендарный деспотизм" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 5125, "Окровавленная танцующая сталь" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4066, "Лечение" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4074, "Истребитель элементалей" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4083, "Ураган" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4084, "Песня сердца" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 0, GOSSIP_ICON_BATTLE, 4443, "Сила стихий" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 4, 5, GOSSIP_ICON_TALK, 0, "Следующая страница ->" , 0, "" },    
    { 4, 3, GOSSIP_ICON_TALK, 0, "<- Предыдущая страница" , 0, "" },
    
    { 5, 0, GOSSIP_ICON_BATTLE, 4445, "Колосс" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 5, 0, GOSSIP_ICON_BATTLE, 4444, "Танцующая сталь" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 5, 0, GOSSIP_ICON_BATTLE, 4446, "Песнь реки" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 5, 0, GOSSIP_ICON_BATTLE, 4442, "Нефритовый дух" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 5, 0, GOSSIP_ICON_BATTLE, 4441, "Песнь ветра" , 0, "Вы действительно хотите наложить это зачарование за |TInterface/ICONS/Inv_qiraj_jewelblessed:30|t Золотая Монета?" },
    { 5, 4, GOSSIP_ICON_TALK, 0, "<- Предыдущая страница" , 0, "" },
};
 
class npc_visualweapon : public CreatureScript
{
public:
    npc_visualweapon() : CreatureScript("npc_visualweapon") { }

    bool MainHand;

    void SetVisual(Player* player, uint32 visual, Item* item)
    {
        const ItemTemplate* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("No item equipped in selected slot.");
            return;
        }

        if (itemTemplate->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_SPEAR ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_Obsolete ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_EXOTIC ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_EXOTIC2 ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_MISCELLANEOUS ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE ||
			   itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_Obsolete)
            return;

        player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, visual);
    }

    void GetMenu(Player* player, Creature* creature, uint32 menuId)
    {
            uint32 PriceInGold = 1000000; // 100 golds

        for (uint8 i = 0; i < (sizeof(vData) / sizeof(*vData)); i++)
        {
            if (vData[i].Menu == menuId)
                player->ADD_GOSSIP_ITEM_EXTENDED(vData[i].Icon, vData[i].Name, GOSSIP_SENDER_MAIN, i, vData[i].Questions, vData[i].PriceInGold, false);
        }

        player->SEND_GOSSIP_MENU(DEFAULT_MESSAGE, creature->GetGUID());
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Main hand", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Left hand", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

        player->SEND_GOSSIP_MENU(DEFAULT_MESSAGE, creature->GetGUID());

        return true;
    }
 
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                MainHand = true;
                GetMenu(player, creature, 1);
                return false;
                break;

            case GOSSIP_ACTION_INFO_DEF + 2:
                MainHand = false;
                GetMenu(player, creature, 1);
                return false;
                break;
        }
        
        uint32 menuData = vData[action].Submenu;
        GetMenu(player, creature, menuData);
        uint8 slot = MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
        if (menuData == 0)
        {
            uint32 enchantId = vData[action].Id;
            menuData = vData[action].Menu;
               if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
               {
                  if (player->GetItemCount(185100) >= 1) // 1 монета
                  {                  
                      SetVisual(player, enchantId, item);

                      PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_VISUAL_ENCHANT);
                      stmt->setUInt32(0, player->GetGUIDLow());
                      stmt->setUInt32(1, item->GetGUIDLow());
                      stmt->setUInt32(2, enchantId);
                      stmt->setUInt32(3, slot);
                      CharacterDatabase.Execute(stmt);
                      player->DestroyItemCount(185100, 1, true);
                      player->m_customVisualEnchant[item->GetGUIDLow()] = enchantId;

                      player->PlayerTalkClass->SendCloseGossip();
                  }
                  else
                     ChatHandler(player->GetSession()).PSendSysMessage("You don`t have 1 token to buy");
               }
               else
                   ChatHandler(player->GetSession()).PSendSysMessage("No item equipped in selected slot."); 
                
                player->PlayerTalkClass->SendCloseGossip();
        }
        return true;
    }
};

void AddSC_npc_visualweapon()
{
    new npc_visualweapon;
}