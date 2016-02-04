/*
 * Copyright (C) 2005-2016 Uwow <http://uwow.biz/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Containers.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "Group.h"
#include "Spell.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "BattlePetMgr.h"
#include "ReputationMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "AuctionHouseMgr.h"
#include "GameEventMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "RedisBuilderMgr.h"

void Item::SaveItem()
{
    ItemData["state"] = "normal";
    ItemData["itemGuid"] = GetGUIDLow();
    ItemData["slot"] = m_slot;
    ItemData["bagGuid"] = m_container ? m_container->GetGUIDLow() : 0;
    ItemData["owner_guid"] = GetOwnerGUID();
    ItemData["itemEntry"] = GetEntry();
    ItemData["creatorGuid"] = GetUInt64Value(ITEM_FIELD_CREATOR);
    ItemData["giftCreatorGuid"] = GetUInt64Value(ITEM_FIELD_GIFTCREATOR);
    ItemData["giftEntry"] = giftEntry;
    ItemData["count"] = GetCount();
    ItemData["duration"] = GetUInt32Value(ITEM_FIELD_DURATION);
    std::ostringstream ssSpells;
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        ssSpells << GetSpellCharges(i) << ' ';
    ItemData["charges"] = ssSpells.str().c_str();
    ItemData["flags"] = GetUInt32Value(ITEM_FIELD_FLAGS);
    std::ostringstream ssEnchants;
    for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
    {
        ssEnchants << GetEnchantmentId(EnchantmentSlot(i)) << ' ';
        ssEnchants << GetEnchantmentDuration(EnchantmentSlot(i)) << ' ';
        ssEnchants << GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
    }
    ItemData["enchantments"] = ssEnchants.str().c_str();
    ItemData["randomPropertyId"] = GetItemRandomPropertyId();
    ItemData["reforgeId"] = GetReforge();
    ItemData["transmogrifyId"] = GetTransmogrification();
    ItemData["upgradeId"] = GetUpgradeId();
    ItemData["durability"] = GetUInt32Value(ITEM_FIELD_DURABILITY);
    ItemData["playedTime"] = GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME);
    ItemData["text"] = GetText().c_str();
    ItemData["uState"] = GetState();
    ItemData["paidMoney"] = m_paidMoney;
    ItemData["paidExtendedCost"] = m_paidExtendedCost;
    ItemData["paidGuid"] = m_refundRecipient;
    std::ostringstream ss;
    AllowedLooterSet::const_iterator itr = allowedGUIDs.begin();
    ss << *itr;
    for (++itr; itr != allowedGUIDs.end(); ++itr)
        ss << ' ' << *itr;
    ItemData["allowedGUIDs"] = ss.str().c_str();

    std::string index = std::to_string(GetGUIDLow());

    sLog->outInfo(LOG_FILTER_REDIS, "Item::SaveItem slot %u Entry %u Count %u guid %s itemKey %s",
    m_slot, GetEntry(), GetCount(), index.c_str(), itemKey);

    RedisDatabase.AsyncExecuteHSet("HSET", itemKey, index.c_str(), sRedisBuilder->BuildString(ItemData), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Item::SaveItem guid %u%u", guid);
    });
}

bool Item::LoadFromDB(uint32 guid, uint64 owner_guid, Json::Value& itemValue, uint32 entry)
{
    // create item before any checks for store correct guid
    // and allow use "FSetState(ITEM_REMOVED); SaveToDB();" for deleting item from DB
    Object::_Create(guid, 0, HIGHGUID_ITEM);

    sprintf(itemKey, "r{%u}u{%u}items", realmID, owner_guid);

    // Set entry, MUST be before proto check
    SetEntry(entry);
    SetObjectScale(1.0f);

    ItemTemplate const* proto = GetTemplate();
    if (!proto)
    {
        sLog->outInfo(LOG_FILTER_REDIS, "Item::LoadFromDB ItemTemplate not foundentry %u guid %u", entry, guid);
        return false;
    }

    // set owner (not if item is only loaded for gbank/auction/mail
    if (owner_guid != 0)
        SetOwnerGUID(owner_guid);

    bool need_save = false;                                 // need explicit save data at load fixes
    SetUInt64Value(ITEM_FIELD_CREATOR, MAKE_NEW_GUID(itemValue["creatorGuid"].asUInt(), 0, HIGHGUID_PLAYER));
    SetUInt64Value(ITEM_FIELD_GIFTCREATOR, MAKE_NEW_GUID(itemValue["giftCreatorGuid"].asUInt(), 0, HIGHGUID_PLAYER));
    SetCount(itemValue["count"].asInt());
    SetGiftEntry(itemValue["giftEntry"].asInt());

    uint32 duration = itemValue["duration"].asUInt();
    SetUInt32Value(ITEM_FIELD_DURATION, duration);
    // update duration if need, and remove if not need
    if ((proto->Duration == 0) != (duration == 0))
        SetUInt32Value(ITEM_FIELD_DURATION, proto->Duration);

    std::string charges = itemValue["charges"].asString();
    if (charges.size() > MAX_ITEM_PROTO_SPELLS)
    {
        Tokenizer tokens(charges.c_str(), ' ', MAX_ITEM_PROTO_SPELLS);
        if (tokens.size() == MAX_ITEM_PROTO_SPELLS)
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                SetSpellCharges(i, atoi(tokens[i]));
    }

    SetUInt32Value(ITEM_FIELD_FLAGS, itemValue["flags"].asUInt());
    // Remove bind flag for items vs NO_BIND set
    if (IsSoulBound() && proto->Bonding == NO_BIND)
    {
        ApplyModFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_SOULBOUND, false);
        need_save = true;
    }

    std::string enchants = itemValue["enchantments"].asString();
    _LoadIntoDataField(enchants.c_str(), ITEM_FIELD_ENCHANTMENT_1_1, MAX_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET);

    uint32 dynMod1 = itemValue["reforgeId"].asInt();
    uint32 dynMod2 = itemValue["transmogrifyId"].asInt();
    uint32 dynMod3 = itemValue["upgradeId"].asInt();

    if (isBattlePet())
    {
        if (dynMod1 && dynMod2 && dynMod3)
        {
            if (BattlePetSpeciesEntry const* bp = sBattlePetSpeciesStore.LookupEntry(dynMod1))
                SetBattlePet(dynMod1, dynMod2, dynMod3);
        }
    }
    else
    {
        if (dynMod1)
        {
            if (ItemReforgeEntry const* reforge = sItemReforgeStore.LookupEntry(dynMod1))
                SetReforge(dynMod1);
        }

        if (dynMod2)
        {
            if (ItemTemplate const* transProto = sObjectMgr->GetItemTemplate(dynMod2))
            {
                if (proto->Class == transProto->Class)
                    SetTransmogrification(dynMod2);
            }
        }

        ItemLevel = proto->ItemLevel;
        if (ItemUpgradeData const* upgradeData = GetItemUpgradeData(entry))
        {
            for (uint32 i = 0; i < MAX_ITEM_UPDGRADES; ++i)
            {
                ItemUpgradeEntry const* upgradeEntry = upgradeData->upgrade[i];

                if (!upgradeEntry)
                    continue;

                if (upgradeEntry->id == dynMod3 || !dynMod3 && !upgradeEntry->prevUpgradeId)
                {
                    ItemLevel += upgradeEntry->levelBonus;
                    SetUpgradeId(upgradeEntry->id);
                    break;
                }
            }
        }
    }

    SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, itemValue["randomPropertyId"].asInt());
    // recalculate suffix factor
    if (GetItemRandomPropertyId() < 0)
        UpdateItemSuffixFactor();

    uint32 durability = itemValue["durability"].asUInt();
    SetUInt32Value(ITEM_FIELD_DURABILITY, durability);
    // update max durability (and durability) if need
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, proto->MaxDurability);
    if (durability > proto->MaxDurability)
        SetUInt32Value(ITEM_FIELD_DURABILITY, proto->MaxDurability);

    SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, itemValue["playedTime"].asUInt());
    SetText(itemValue["text"].asCString());

    return true;
}

bool Bag::LoadFromDB(uint32 guid, uint64 owner_guid, Json::Value& itemValue, uint32 entry)
{
    if (!Item::LoadFromDB(guid, owner_guid, itemValue, entry))
        return false;

    ItemTemplate const* itemProto = GetTemplate(); // checked in Item::LoadFromDB
    SetUInt32Value(CONTAINER_FIELD_NUM_SLOTS, itemProto->ContainerSlots);
    // cleanup bag content related item value (its will be filled correctly from `character_inventory`)
    for (uint8 i = 0; i < MAX_BAG_SIZE; ++i)
    {
        SetUInt64Value(CONTAINER_FIELD_SLOT_1 + (i*2), 0);
        delete m_bagslot[i];
        m_bagslot[i] = NULL;
    }

    return true;
}
