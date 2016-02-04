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

void AuctionEntry::SaveAuction()
{
    AuctionData["Id"] = Id;
    AuctionData["auctioneer"] = auctioneer;
    AuctionData["itemGUIDLow"] = itemGUIDLow;
    AuctionData["itemEntry"] = itemEntry;
    AuctionData["itemCount"] = itemCount;
    AuctionData["owner"] = owner;
    AuctionData["startbid"] = startbid;
    AuctionData["bid"] = bid;
    AuctionData["buyout"] = buyout;
    AuctionData["expire_time"] = expire_time;
    AuctionData["bidder"] = bidder;
    AuctionData["deposit"] = deposit;
    AuctionData["factionTemplateId"] = factionTemplateId;

    std::string index = std::to_string(Id);

    RedisDatabase.AsyncExecuteHSet("HSET", auctionKey, index.c_str(), sRedisBuilder->BuildString(AuctionData), Id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionEntry::SaveAuction guid %u", guid);
    });
}

void AuctionEntry::UpdateSaveAuction()
{
    AuctionData["Id"] = Id;
    AuctionData["auctioneer"] = auctioneer;
    AuctionData["itemGUIDLow"] = itemGUIDLow;
    AuctionData["itemEntry"] = itemEntry;
    AuctionData["itemCount"] = itemCount;
    AuctionData["owner"] = owner;
    AuctionData["startbid"] = startbid;
    AuctionData["bid"] = bid;
    AuctionData["buyout"] = buyout;
    AuctionData["expire_time"] = expire_time;
    AuctionData["bidder"] = bidder;
    AuctionData["deposit"] = deposit;
    AuctionData["factionTemplateId"] = factionTemplateId;
}

void AuctionEntry::DeleteFromRedis()
{
    std::string index = std::to_string(Id);

    RedisDatabase.AsyncExecuteH("HDEL", auctionKey, index.c_str(), Id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Item::DeleteFromRedis guid %u", guid);
    });
}

void AuctionHouseMgr::LoadAuctions(const RedisValue* v, uint64 aucId)
{
    uint32 oldMSTime = getMSTime();
    std::vector<RedisValue> auctionVector;
    if (!sRedisBuilder->LoadFromRedisArray(v, auctionVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctions data is empty");
        return;
    }

    uint32 count = 0;
    for (auto itr = auctionVector.begin(); itr != auctionVector.end();)
    {
        uint32 Id = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        Json::Value valueData;
        if (!sRedisBuilder->LoadFromRedis(&(*itr), valueData))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerCriteriaProgress not parse achievementID %i", aucId);
            continue;
        }
        else
            ++itr;

        AuctionEntry* aItem = new AuctionEntry();
        aItem->Id = Id;
        aItem->auctioneer = valueData["auctioneer"].asUInt();
        aItem->itemGUIDLow = valueData["itemGUIDLow"].asUInt();
        aItem->itemEntry = valueData["itemEntry"].asUInt();
        aItem->itemCount = valueData["itemCount"].asUInt();
        aItem->owner = valueData["owner"].asUInt();
        aItem->startbid = valueData["startbid"].asUInt64();
        aItem->bid = valueData["bid"].asUInt64();
        aItem->buyout = valueData["buyout"].asUInt64();
        aItem->expire_time = valueData["expire_time"].asUInt();
        aItem->bidder = valueData["bidder"].asUInt();
        aItem->deposit = valueData["deposit"].asUInt64();

        CreatureData const* auctioneerData = sObjectMgr->GetCreatureData(aItem->auctioneer);
        if (!auctioneerData)
            continue;

        CreatureTemplate const* auctioneerInfo = sObjectMgr->GetCreatureTemplate(auctioneerData->id);
        if (!auctioneerInfo)
            continue;

        aItem->factionTemplateId = auctioneerInfo->faction;
        aItem->auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(aItem->factionTemplateId);

        if (!aItem->auctionHouseEntry)
            continue;

        // check if sold item exists for guid
        // and itemEntry in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
        if (!sAuctionMgr->GetAItem(aItem->itemGUIDLow))
            continue;

        GetAuctionsMap(aItem->factionTemplateId)->AddAuction(aItem);
        aItem->UpdateSaveAuction();
        count++;
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u auctions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::LoadAuctionItems(const RedisValue* v, uint64 aucId)
{
    uint32 oldMSTime = getMSTime();

    std::vector<RedisValue> auctionItemVector;
    if (!sRedisBuilder->LoadFromRedisArray(v, auctionItemVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctionItems data is empty");
        return;
    }

    uint32 count = 0;
    for (auto itr = auctionItemVector.begin(); itr != auctionItemVector.end();)
    {
        uint32 item_guid = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        Json::Value AuctionItemJson;
        if (!sRedisBuilder->LoadFromRedis(&(*itr), AuctionItemJson))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctionItems not parse Id %i", aucId);
            continue;
        }
        else
            ++itr;

        uint32 itemEntry    = AuctionItemJson["itemEntry"].asInt();

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
        if (!proto)
        {
            sLog->outError(LOG_FILTER_GENERAL, "AuctionHouseMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid, itemEntry);
            continue;
        }

        Item* item = NewItemOrBag(proto);
        item->SetItemKey(ITEM_KEY_AUCT, 0);

        if (!item->LoadFromDB(item_guid, 0, AuctionItemJson, itemEntry))
        {
            delete item;
            continue;
        }
        AddAItem(item);

        count++;
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u auction items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}
