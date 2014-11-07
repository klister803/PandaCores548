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

#include "ScriptPCH.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"

class CustomRewardScript : public PlayerScript
{
public:
    CustomRewardScript() : PlayerScript("CustomRewardScript") { }

    void OnLogin(Player* player)
    {
        if(!player)
            return;
        uint32 owner_guid = player->GetGUIDLow();
        ChatHandler chH = ChatHandler(player);

        //type:
        // 1 achievement
        // 2 title
        // 3 item
        // 4 quest
        // 5 give 1 level
        // 6 level up to
        // 7 spell
        // 8 Remove pvp items from sliver char
        // 9 Remove items from char from loot
        bool rewarded = false;
        QueryResult result = CharacterDatabase.PQuery("SELECT guid, type, id, count FROM `character_reward` WHERE owner_guid = '%u'", owner_guid);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 guid = fields[0].GetUInt32();
                uint32 type = fields[1].GetUInt32();
                uint32 id = fields[2].GetUInt32();
                uint32 count = fields[3].GetUInt32();
                rewarded = false;
                switch (type)
                {
                    case 1: // Add achievement to char
                    {
                        if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(id))
                            player->CompletedAchievement(achievementEntry);
                        rewarded = true;
                    }
                    break;
                    case 2: // Add title to char
                    {
                        if(CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(id))
                        {
                            char const* targetName = player->GetName();
                            char titleNameStr[80];
                            snprintf(titleNameStr, 80, titleInfo->name, targetName);
                            player->SetTitle(titleInfo);
                            chH.PSendSysMessage(LANG_TITLE_ADD_RES, id, titleNameStr, targetName);
                            rewarded = true;
                        }
                    }
                    break;
                    case 3: // Add items to char
                    {
                        if(ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(id))
                        {
                            //Adding items
                            uint32 noSpaceForCount = 0;

                            // check space and find places
                            ItemPosCountVec dest;
                            InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count, &noSpaceForCount);
                            if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
                                count -= noSpaceForCount;

                            if (count == 0 || dest.empty())                         // can't add any
                            {
                                chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                                break;
                            }

                            Item* item = player->StoreNewItem(dest, id, true, Item::GenerateItemRandomPropertyId(id));

                            if (count > 0 && item)
                                player->SendNewItem(item, NULL, count, false, true);

                            if (noSpaceForCount > 0)
                            {
                                chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                                CharacterDatabase.PExecute("UPDATE character_reward SET count = count - %u WHERE guid = %u", count, guid);
                            }
                            else
                                rewarded = true;
                        }
                    }
                    break;
                    case 4: // Add quest to char
                    {
                        //not implemented
                    }
                    break;
                    case 5: // Add 1 level to char
                    {
                        //not implemented
                    }
                    break;
                    case 6: // Level up to
                    {
                        //not implemented
                    }
                    break;
                    case 7: // Learn spell
                    {
                        //not implemented
                    }
                    break;
                    case 8:
                    {
                        CleanItems(player);
                        //player->SaveToDB();
                        rewarded = true;
                    }
                    break;
                    case 9:
                    {
                        FindItem(player, id, count);
                        //player->SaveToDB();
                        rewarded = true;
                    }
                    break;
                    default:
                        break;
                }
                if(rewarded)
                    CharacterDatabase.PExecute("DELETE FROM character_reward WHERE guid = %u", guid);
            }while (result->NextRow());
        }
        else
        {
            if(QueryResult share_result = CharacterDatabase.PQuery("SELECT * FROM `character_share` WHERE guid = '%u'", owner_guid))
            {
                uint32 totaltime = player->GetTotalPlayedTime();
                bool update = false;
                Field* fields = share_result->Fetch();
                uint32 bonus1 = fields[1].GetBool();
                uint32 bonus2 = fields[2].GetBool();
                uint32 bonus3 = fields[3].GetBool();
                uint32 bonus4 = fields[4].GetBool();
                uint32 bonus5 = fields[5].GetBool();
                uint32 bonus6 = fields[6].GetBool();
                uint32 bonus7 = fields[7].GetBool();
                uint32 bonus8 = fields[8].GetBool();
                uint32 bonus9 = fields[9].GetBool();
                uint32 bonus10 = fields[10].GetBool();
                uint32 bonus11 = fields[11].GetBool();

                if (sWorld->getBoolConfig(CONFIG_SHARE_ENABLE))
                {
                    if(!bonus1 && totaltime >= (1 * HOUR))
                    {
                        player->ModifyMoney(500000);
                        update = true;
                        bonus1 = true;
                    }
                    if(!bonus2 && totaltime >= (10 * HOUR))
                    {
                        player->ModifyMoney(2500000);
                        update = true;
                        bonus2 = true;
                    }
                    if(!bonus3 && totaltime >= (20 * HOUR))
                    {
                        player->ModifyMoney(5000000);
                        update = true;
                        bonus3 = true;
                    }
                    if(!bonus4 && totaltime >= (50 * HOUR))
                    {
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 72068);
                        update = true;
                        bonus4 = true;
                    }
                }

                if(!bonus5 && totaltime >= (100 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 49663);
                    update = true;
                    bonus5 = true;
                }
                if(!bonus6 && totaltime >= (500 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 83086);
                    update = true;
                    bonus6 = true;
                }
                if(!bonus7 && totaltime >= (1000 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 106246);
                    update = true;
                    bonus7 = true;
                }
                if(!bonus8 && totaltime >= (2500 * HOUR))
                {
                    if(player->GetTeam() == HORDE)
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 76902);
                    else
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 76889);
                    update = true;
                    bonus8 = true;
                }
                if(!bonus9 && totaltime >= (5000 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 69846);
                    update = true;
                    bonus9 = true;
                }
                if(!bonus10 && totaltime >= (9000 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 21176);
                    update = true;
                    bonus10 = true;
                }
                if(!bonus11 && totaltime >= (250 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 68385);
                    update = true;
                    bonus11 = true;
                }
                if(update)
                    CharacterDatabase.PQuery("UPDATE `character_share` SET `bonus1` = '%u', `bonus2` = '%u', `bonus3` = '%u', `bonus4` = '%u', `bonus5` = '%u', `bonus6` = '%u', `bonus7` = '%u', `bonus8` = '%u', `bonus9` = '%u', `bonus10` = '%u', `bonus11` = '%u' WHERE guid = '%u'", bonus1, bonus2, bonus3, bonus4, bonus5, bonus6, bonus7, bonus8, bonus9, bonus10, bonus11, owner_guid);
            }
            else
                CharacterDatabase.PQuery("INSERT INTO `character_share` (`guid`, `bonus1`, `bonus2`, `bonus3`, `bonus4`, `bonus5`, `bonus6`, `bonus7`, `bonus8`, `bonus9`, `bonus10`, `bonus11`) values('%u','0','0','0','0','0','0','0','0','0','0','0')", owner_guid);
        }
    }

    void FindItem(Player* player, uint32 entry, uint32 count)
    {
        // in inventory
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if(pItem->GetEntry() == entry)
                {
                    count = ItemDel(pItem, player, count);
                    if(!count)
                        return;
                }

        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if(pItem->GetEntry() == entry)
                        {
                            count = ItemDel(pItem, player, count);
                            if(!count)
                                return;
                        }

        for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                count = ItemDel(pItem, player, count);
                if(!count)
                    return;
            }

        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if(pItem->GetEntry() == entry)
                {
                    count = ItemDel(pItem, player, count);
                    if(!count)
                        return;
                }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if(pItem->GetEntry() == entry)
                        {
                            count = ItemDel(pItem, player, count);
                            if(!count)
                                return;
                        }
    }

    uint32 ItemDel(Item* _item, Player* player, uint32 count)
    {
        uint32 tempcount = count;
        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE itemguid = '%u'", _item->GetGUIDLow());
        if(!result)
        {
            ChatHandler chH = ChatHandler(player);
            if (_item->GetCount() >= count)
                count = 0;
            else
                count -=_item->GetCount();

            player->DestroyItemCount(_item, tempcount, true);
            //sLog->outError("ItemDel item delete %u, count %u, tempcount %u", _item->GetEntry(), count, tempcount);
            chH.PSendSysMessage(20021, sObjectMgr->GetItemTemplate(_item->GetEntry())->Name1.c_str());
        }
        return count;
    }

    void CleanItems(Player* player)
    {
        // in inventory
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ItemIfExistDel(pItem, player);

        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        ItemIfExistDel(pItem, player);

        for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ItemIfExistDel(pItem, player);

        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ItemIfExistDel(pItem, player);

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        ItemIfExistDel(pItem, player);
    }

    void ItemIfExistDel(Item* _item, Player* player)
    {
        switch (_item->GetEntry())
        {
            case 84967:
            case 84961:
            case 84895:
            case 85133:
            case 85127:
            case 85118:
            case 84963:
            case 84962:
            case 84894:
            case 84893:
            case 85129:
            case 85128:
            case 85117:
            case 85116:
            case 84966:
            case 84965:
            case 85132:
            case 85131:
            case 84971:
            case 84970:
            case 84964:
            case 85137:
            case 85136:
            case 85130:
            case 84969:
            case 84968:
            case 85135:
            case 85134:
            case 84786:
            case 85108:
            case 84789:
            case 84788:
            case 84787:
            case 85111:
            case 85110:
            case 85109:
            case 84791:
            case 85113:
            case 84785:
            case 85107:
            case 84790:
            case 85112:
            case 84896:
            case 85119:
            case 84897:
            case 85120:
            case 84900:
            case 85123:
            case 84899:
            case 84898:
            case 85122:
            case 85121:
            case 84855:
            case 84859:
            case 84863:
            case 84864:
            case 85031:
            case 85035:
            case 85039:
            case 85040:
            case 85069:
            case 85068:
            case 84909:
            case 84908:
            case 84905:
            case 84904:
            case 85072:
            case 85073:
            case 84977:
            case 84978:
            case 84979:
            case 84837:
            case 84838:
            case 84842:
            case 84846:
            case 85013:
            case 85016:
            case 85017:
            case 85021:
            case 85052:
            case 85047:
            case 84884:
            case 84883:
            case 84878:
            case 84875:
            case 85053:
            case 85062:
            case 85078:
            case 85074:
            case 84929:
            case 84928:
            case 84923:
            case 84917:
            case 85083:
            case 85085:
            case 84954:
            case 84955:
            case 84956:
            case 85092:
            case 85093:
            case 85094:
            case 84814:
            case 84815:
            case 84816:
            case 85004:
            case 85005:
            case 85006:
            case 85037:
            case 85033:
            case 85030:
            case 84861:
            case 84857:
            case 84854:
            case 84852:
            case 84850:
            case 84848:
            case 85041:
            case 85043:
            case 85045:
            case 85067:
            case 85066:
            case 85065:
            case 84995:
            case 84907:
            case 84906:
            case 84903:
            case 84902:
            case 84901:
            case 84792:
            case 85070:
            case 85071:
            case 84972:
            case 84973:
            case 84976:
            case 84982:
            case 85025:
            case 85023:
            case 85022:
            case 85018:
            case 85015:
            case 84843:
            case 84839:
            case 84836:
            case 84833:
            case 84832:
            case 84830:
            case 85026:
            case 85058:
            case 85055:
            case 85051:
            case 85049:
            case 84882:
            case 84880:
            case 84877:
            case 84873:
            case 84871:
            case 84868:
            case 85060:
            case 85064:
            case 85084:
            case 85082:
            case 85080:
            case 84927:
            case 84925:
            case 84920:
            case 84919:
            case 84916:
            case 84913:
            case 85087:
            case 85088:
            case 85089:
            case 84947:
            case 84948:
            case 84953:
            case 84960:
            case 85098:
            case 85100:
            case 85101:
            case 85106:
            case 84808:
            case 84809:
            case 84813:
            case 84819:
            case 84996:
            case 85003:
            case 85009:
            case 85010:
            case 85034:
            case 85029:
            case 84865:
            case 84862:
            case 84860:
            case 84858:
            case 85036:
            case 85038:
            case 84990:
            case 84989:
            case 84988:
            case 84800:
            case 84799:
            case 84798:
            case 84796:
            case 84994:
            case 84980:
            case 84981:
            case 84983:
            case 84984:
            case 85012:
            case 85011:
            case 84847:
            case 84845:
            case 84844:
            case 84841:
            case 85014:
            case 85020:
            case 84874:
            case 84879:
            case 84881:
            case 84885:
            case 85048:
            case 85050:
            case 85054:
            case 85061:
            case 85076:
            case 85075:
            case 84930:
            case 84926:
            case 84924:
            case 84921:
            case 85079:
            case 85081:
            case 84946:
            case 84957:
            case 84958:
            case 84959:
            case 85095:
            case 85096:
            case 85097:
            case 85099:
            case 84817:
            case 84818:
            case 84820:
            case 84821:
            case 84997:
            case 84998:
            case 85007:
            case 85008:
            case 85044:
            case 85042:
            case 85032:
            case 84856:
            case 84853:
            case 84851:
            case 84849:
            case 85046:
            case 84993:
            case 84992:
            case 84991:
            case 84987:
            case 84797:
            case 84795:
            case 84794:
            case 84793:
            case 84974:
            case 84975:
            case 84985:
            case 84986:
            case 85028:
            case 85027:
            case 85024:
            case 85019:
            case 84840:
            case 84835:
            case 84834:
            case 84831:
            case 85059:
            case 85057:
            case 85056:
            case 84876:
            case 84872:
            case 84870:
            case 84869:
            case 85063:
            case 85090:
            case 85086:
            case 85077:
            case 84922:
            case 84918:
            case 84915:
            case 84914:
            case 85091:
            case 84949:
            case 84950:
            case 84951:
            case 84952:
            case 85102:
            case 85103:
            case 85104:
            case 85105:
            case 84810:
            case 84811:
            case 84812:
            case 84822:
            case 84999:
            case 85000:
            case 85001:
            case 85002:
            case 84886:
            case 84887:
            case 84888:
            case 84889:
            case 84890:
            case 84891:
            case 84892:
            case 84829:
            case 84828:
            case 84827:
            case 84826:
            case 84825:
            case 84824:
            case 84823:
            case 84942:
            case 84941:
            case 84940:
            case 84939:
            case 84938:
            case 84937:
            case 84936:
            case 84935:
            case 84934:
            case 84933:
            case 84932:
            case 84931:
            case 84943:
            case 84944:
            case 84801:
            case 84802:
            case 84803:
            case 84804:
            case 84805:
            case 84806:
            case 84807:
            case 84867:
            case 84866:
            case 85115:
            case 85114:
            case 84912:
            case 84911:
            case 84910:
            case 85126:
            case 85125:
            case 85124:
            {
                QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE itemguid = '%u'", _item->GetGUIDLow());
                if(!result)
                {
                    ChatHandler chH = ChatHandler(player);
                    player->DestroyItem(_item->GetBagSlot(), _item->GetSlot(), true);
                    //sLog->outError("ItemIfExistDel item delete %u", _item->GetEntry());
                    chH.PSendSysMessage(20020, sObjectMgr->GetItemTemplate(_item->GetEntry())->Name1.c_str());
                }
            }
        }
    }
};

void AddSC_custom_reward()
{
    new CustomRewardScript();
}
