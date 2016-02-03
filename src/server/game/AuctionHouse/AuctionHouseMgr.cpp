/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include <vector>

enum eAuctionHouse
{
    AH_MINIMUM_DEPOSIT = 100,
};

AuctionHouseMgr::AuctionHouseMgr()
{
}

AuctionHouseMgr::~AuctionHouseMgr()
{
    for (ItemMap::iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
        delete itr->second;
}

AuctionHouseObject* AuctionHouseMgr::GetAuctionsMap(uint32 factionTemplateId)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return &mNeutralAuctions;

    // team have linked auction houses
    FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
    if (!u_entry)
        return &mNeutralAuctions;
    else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
        return &mAllianceAuctions;
    else if (u_entry->ourMask & FACTION_MASK_HORDE)
        return &mHordeAuctions;
    else
        return &mNeutralAuctions;
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, Item* pItem, uint32 count)
{
    uint32 MSV = pItem->GetTemplate()->SellPrice;

    if (MSV <= 0)
        return AH_MINIMUM_DEPOSIT;

    float multiplier = CalculatePct(float(entry->depositPercent), 3);
    uint32 timeHr = (((time / 60) / 60) / 12);
    uint32 deposit = uint32(((multiplier * MSV * count / 3) * timeHr * 3) * sWorld->getRate(RATE_AUCTION_DEPOSIT));

    sLog->outDebug(LOG_FILTER_AUCTIONHOUSE, "MSV:        %u", MSV);
    sLog->outDebug(LOG_FILTER_AUCTIONHOUSE, "Items:      %u", count);
    sLog->outDebug(LOG_FILTER_AUCTIONHOUSE, "Multiplier: %f", multiplier);
    sLog->outDebug(LOG_FILTER_AUCTIONHOUSE, "Deposit:    %u", deposit);

    if (deposit < AH_MINIMUM_DEPOSIT)
        return AH_MINIMUM_DEPOSIT;
    else
        return deposit;
}

//does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(AuctionEntry* auction)
{
    Item* pItem = GetAItem(auction->itemGUIDLow);
    if (!pItem)
        return;

    uint32 bidder_accId = 0;
    uint64 bidder_guid = MAKE_NEW_GUID(auction->bidder, 0, HIGHGUID_PLAYER);
    Player* bidder = ObjectAccessor::FindPlayer(bidder_guid);
    // data for gm.log
    if (sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        uint32 bidder_security = 0;
        std::string bidder_name;
        if (bidder)
        {
            bidder_accId = bidder->GetSession()->GetAccountId();
            bidder_security = bidder->GetSession()->GetSecurity();
            bidder_name = bidder->GetName();
        }
        else
        {
            bidder_accId = ObjectMgr::GetPlayerAccountIdByGUID(bidder_guid);
            bidder_security = AccountMgr::GetSecurity(bidder_accId, realmID);

            if (!AccountMgr::IsPlayerAccount(bidder_security)) // not do redundant DB requests
            {
                if (!ObjectMgr::GetPlayerNameByGUID(bidder_guid, bidder_name))
                    bidder_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
            }
        }
        if (!AccountMgr::IsPlayerAccount(bidder_security))
        {
            std::string owner_name;
            if (!ObjectMgr::GetPlayerNameByGUID(auction->owner, owner_name))
                owner_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);

            uint32 owner_accid = ObjectMgr::GetPlayerAccountIdByGUID(auction->owner);

            sLog->outCommand(bidder_accId, "GM %s (Account: %u) won item in auction: %s (Entry: %u Count: %u) and pay money: %u. Original owner %s (Account: %u)",
                bidder_name.c_str(), bidder_accId, pItem->GetTemplate()->Name1.c_str(), pItem->GetEntry(), pItem->GetCount(), auction->bid, owner_name.c_str(), owner_accid);
        }
    }

    // receiver exist
    if (bidder || bidder_accId)
    {
        if (bidder)
        {
            bidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, bidder_guid, 0, 0, auction->itemEntry);
            // FIXME: for offline player need also
            bidder->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS, 1);
        }

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_WON), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, 0, 0))
            .AddItem(pItem)
            .SendMailTo(MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

void AuctionHouseMgr::SendAuctionSalePendingMail(AuctionEntry* auction)
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player* owner = ObjectAccessor::FindPlayer(owner_guid);
    uint32 owner_accId = ObjectMgr::GetPlayerAccountIdByGUID(owner_guid);
    // owner exist (online or offline)
    if (owner || owner_accId)
        MailDraft(auction->BuildAuctionMailSubject(AUCTION_SALE_PENDING), AuctionEntry::BuildAuctionMailBody(auction->bidder, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .SendMailTo(MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED);
}

//call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(AuctionEntry* auction)
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player* owner = ObjectAccessor::FindPlayer(owner_guid);
    uint32 owner_accId = ObjectMgr::GetPlayerAccountIdByGUID(owner_guid);
    // owner exist
    if (owner || owner_accId)
    {
        uint64 profit = auction->bid + auction->deposit - auction->GetAuctionCut();

        //FIXME: what do if owner offline
        if (owner)
        {
            owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS, profit);
            owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD, auction->bid);
            //send auction owner notification, bidder must be current!
            owner->GetSession()->SendAuctionOwnerNotification(auction);
        }

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_SUCCESSFUL), AuctionEntry::BuildAuctionMailBody(auction->bidder, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(profit)
            .SendMailTo(MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY));
    }
}

//does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(AuctionEntry* auction)
{
    //return an item in auction to its owner by mail
    Item* pItem = GetAItem(auction->itemGUIDLow);
    if (!pItem)
        return;

    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player* owner = ObjectAccessor::FindPlayer(owner_guid);
    uint32 owner_accId = ObjectMgr::GetPlayerAccountIdByGUID(owner_guid);
    // owner exist
    if (owner || owner_accId)
    {
        if (owner)
            owner->GetSession()->SendAuctionOwnerNotification(auction);

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_EXPIRED), AuctionEntry::BuildAuctionMailBody(0, 0, auction->buyout, auction->deposit, 0))
            .AddItem(pItem)
            .SendMailTo(MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, 0);
    }
}

//this function sends mail to old bidder
void AuctionHouseMgr::SendAuctionOutbiddedMail(AuctionEntry* auction, uint64 newPrice, Player* newBidder)
{
    uint64 oldBidder_guid = MAKE_NEW_GUID(auction->bidder, 0, HIGHGUID_PLAYER);
    Player* oldBidder = ObjectAccessor::FindPlayer(oldBidder_guid);

    uint32 oldBidder_accId = 0;
    if (!oldBidder)
        oldBidder_accId = ObjectMgr::GetPlayerAccountIdByGUID(oldBidder_guid);

    // old bidder exist
    if (oldBidder || oldBidder_accId)
    {
        if (oldBidder && newBidder)
            oldBidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, newBidder->GetGUID(), newPrice, auction->GetAuctionOutBid(), auction->itemEntry);

        MailDraft(auction->BuildAuctionMailSubject(AUCTION_OUTBIDDED), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(auction->bid)
            .SendMailTo(MailReceiver(oldBidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//this function sends mail, when auction is cancelled to old bidder
void AuctionHouseMgr::SendAuctionCancelledToBidderMail(AuctionEntry* auction, Item* item)
{
    uint64 bidder_guid = MAKE_NEW_GUID(auction->bidder, 0, HIGHGUID_PLAYER);
    Player* bidder = ObjectAccessor::FindPlayer(bidder_guid);

    uint32 bidder_accId = 0;
    if (!bidder)
        bidder_accId = ObjectMgr::GetPlayerAccountIdByGUID(bidder_guid);

    if (bidder)
        bidder->GetSession()->SendAuctionRemovedNotification(auction->Id, auction->itemEntry, item->GetItemRandomPropertyId());

    // bidder exist
    if (bidder || bidder_accId)
        MailDraft(auction->BuildAuctionMailSubject(AUCTION_CANCELLED_TO_BIDDER), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, auction->deposit, 0))
            .AddMoney(auction->bid)
            .SendMailTo(MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
}

void AuctionHouseMgr::LoadAuctionItems()
{
    if (RedisDatabase.isConnected())
    {
        char* key = new char[32];
        sprintf(key, "r{%i}auc{%i}items", realmID, 0);
        RedisValue v = RedisDatabase.Execute("EXISTS", key);
        if (v.isOk() && v.toInt())
        {
            RedisDatabase.AsyncExecute("HGETALL", key, 0, [&](const RedisValue &v, uint64 aucId) {
                sAuctionMgr->LoadAuctionItems(&v, aucId);
            });
            return;
        }
    }

    uint32 oldMSTime = getMSTime();

    // data needs to be at first place for Item::LoadFromDB
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTION_ITEMS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 auction items. DB table `auctionhouse` or `item_instance` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 item_guid    = fields[14].GetUInt32();
        uint32 itemEntry    = fields[15].GetUInt32();

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
        if (!proto)
        {
            sLog->outError(LOG_FILTER_GENERAL, "AuctionHouseMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid, itemEntry);
            continue;
        }

        Item* item = NewItemOrBag(proto);
        item->SetItemKey(ITEM_KEY_AUCT, 0);

        if (!item->LoadFromDB(item_guid, 0, fields, itemEntry))
        {
            delete item;
            continue;
        }
        item->SerializeItem();
        AddAItem(item);

        ++count;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u auction items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::LoadAuctionItems(const RedisValue* v, uint64 aucId)
{
    uint32 oldMSTime = getMSTime();

    if (!v->isOk() || v->isNull())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctionItems data is empty");
        return;
    }
    if (!v->isArray())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctionItems not found");
        return;
    }

    std::vector<RedisValue> auctionItemVector = v->toArray();

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

        std::string data = itr->toString();
        ++itr;

        Json::Value AuctionItemJson;
        bool isReader = jsonReader.parse(data.c_str(), AuctionItemJson);
        if (!isReader)
        {
            sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctionItems not parse Id %i", aucId);
            continue;
        }

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

void AuctionHouseMgr::LoadAuctions()
{
    if (RedisDatabase.isConnected())
    {
        char* key = new char[18];
        sprintf(key, "r{%u}auc{%u}", realmID, 0);
        RedisValue v = RedisDatabase.Execute("EXISTS", key);
        if (v.isOk() && v.toInt())
        {
            RedisDatabase.AsyncExecute("HGETALL", key, 0, [&](const RedisValue &v, uint64 aucId) {
                sAuctionMgr->LoadAuctions(&v, aucId);
            });
            return;
        }
    }

    uint32 oldMSTime = getMSTime();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTIONS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    uint32 count = 0;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    do
    {
        Field* fields = result->Fetch();

        AuctionEntry* aItem = new AuctionEntry();
        if (!aItem->LoadFromDB(fields))
        {
            aItem->DeleteFromDB(trans);
            delete aItem;
            continue;
        }

        GetAuctionsMap(aItem->factionTemplateId)->AddAuction(aItem);
        aItem->SerializeAuction();

        //Delete from DB use redis base next time
        aItem->DeleteFromDB(trans);
        count++;
    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u auctions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::LoadAuctions(const RedisValue* v, uint64 aucId)
{
    uint32 oldMSTime = getMSTime();
    if (!v->isOk() || v->isNull())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctions data is empty");
        return;
    }
    if (!v->isArray())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "AuctionHouseMgr::LoadAuctions not found");
        return;
    }

    std::vector<RedisValue> auctionVector = v->toArray();

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

        std::string data = itr->toString();
        ++itr;

        Json::Value AuctionJson;
        bool isReader = jsonReader.parse(data.c_str(), AuctionJson);
        if (!isReader)
        {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::DeSerializePlayerCriteriaProgress not parse achievementID %i", aucId);
            continue;
        }

        AuctionEntry* aItem = new AuctionEntry();
        aItem->Id = Id;
        aItem->auctioneer = AuctionJson["auctioneer"].asUInt();
        aItem->itemGUIDLow = AuctionJson["itemGUIDLow"].asUInt();
        aItem->itemEntry = AuctionJson["itemEntry"].asUInt();
        aItem->itemCount = AuctionJson["itemCount"].asUInt();
        aItem->owner = AuctionJson["owner"].asUInt();
        aItem->startbid = AuctionJson["startbid"].asUInt64();
        aItem->bid = AuctionJson["bid"].asUInt64();
        aItem->buyout = AuctionJson["buyout"].asUInt64();
        aItem->expire_time = AuctionJson["expire_time"].asUInt();
        aItem->bidder = AuctionJson["bidder"].asUInt();
        aItem->deposit = AuctionJson["deposit"].asUInt64();

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
        aItem->UpdateSerializeAuction();
        count++;
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u auctions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::AddAItem(Item* it)
{
    ASSERT(it);
    ASSERT(mAitems.find(it->GetGUIDLow()) == mAitems.end());
    mAitems[it->GetGUIDLow()] = it;
}

bool AuctionHouseMgr::RemoveAItem(uint32 id)
{
    ItemMap::iterator i = mAitems.find(id);
    if (i == mAitems.end())
        return false;

    mAitems.erase(i);
    return true;
}

void AuctionHouseMgr::Update()
{
    mHordeAuctions.Update();
    mAllianceAuctions.Update();
    mNeutralAuctions.Update();
}

AuctionHouseEntry const* AuctionHouseMgr::GetAuctionHouseEntry(uint32 factionTemplateId)
{
    uint32 houseid = 7; // goblin auction house

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        //FIXME: found way for proper auctionhouse selection by another way
        // AuctionHouse.dbc have faction field with _player_ factions associated with auction house races.
        // but no easy way convert creature faction to player race faction for specific city
        switch (factionTemplateId)
        {
            case   12: houseid = 1; break; // human
            case   29: houseid = 6; break; // orc, and generic for horde
            case   55: houseid = 2; break; // dwarf, and generic for alliance
            case   68: houseid = 4; break; // undead
            case   80: houseid = 3; break; // n-elf
            case  104: houseid = 5; break; // trolls
            case  120: houseid = 7; break; // booty bay, neutral
            case  474: houseid = 7; break; // gadgetzan, neutral
            case  855: houseid = 7; break; // everlook, neutral
            case 1604: houseid = 6; break; // b-elfs,
            default:                       // for unknown case
            {
                FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                if (!u_entry)
                    houseid = 7; // goblin auction house
                else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
                    houseid = 1; // human auction house
                else if (u_entry->ourMask & FACTION_MASK_HORDE)
                    houseid = 6; // orc auction house
                else
                    houseid = 7; // goblin auction house
                break;
            }
        }
    }

    return sAuctionHouseStore.LookupEntry(houseid);
}

void AuctionHouseObject::AddAuction(AuctionEntry* auction)
{
    ASSERT(auction);

    AuctionsMap[auction->Id] = auction;
    sScriptMgr->OnAuctionAdd(this, auction);
}

bool AuctionHouseObject::RemoveAuction(AuctionEntry* auction, uint32 /*itemEntry*/)
{
    bool wasInMap = AuctionsMap.erase(auction->Id) ? true : false;

    sScriptMgr->OnAuctionRemove(this, auction);

    auction->DeleteFromRedis();

    // we need to delete the entry, it is not referenced any more
    delete auction;
    return wasInMap;
}

void AuctionHouseObject::Update()
{
    time_t curTime = sWorld->GetGameTime();
    ///- Handle expired auctions

    // If storage is empty, no need to update. next == NULL in this case.
    if (AuctionsMap.empty())
        return;

    std::list<AuctionEntry*> auctionList;
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        if (Aentry && Aentry->expire_time <= curTime)
            auctionList.push_back(Aentry);
    }

    for (std::list<AuctionEntry*>::iterator itr = auctionList.begin(); itr != auctionList.end(); ++itr)
    {
        // from auctionhousehandler.cpp, creates auction pointer & player pointer
        AuctionEntry* auction = *itr;
        if (!auction)
            continue;

        ///- Either cancel the auction if there was no bidder
        if (auction->bidder == 0)
        {
            sAuctionMgr->SendAuctionExpiredMail(auction);
            sScriptMgr->OnAuctionExpire(this, auction);
        }
        ///- Or perform the transaction
        else
        {
            //we should send an "item sold" message if the seller is online
            //we send the item to the winner
            //we send the money to the seller
            sAuctionMgr->SendAuctionSuccessfulMail(auction);
            sAuctionMgr->SendAuctionWonMail(auction);
            sScriptMgr->OnAuctionSuccessful(this, auction);
        }

        uint32 itemEntry = auction->itemEntry;

        RemoveAuction(auction, itemEntry);
        sAuctionMgr->RemoveAItem(auction->itemGUIDLow);
    }
}

void AuctionHouseObject::BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        if (Aentry && Aentry->bidder == player->GetGUIDLow())
        {
            if (itr->second->BuildAuctionInfo(data))
                ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        if (Aentry && Aentry->owner == player->GetGUIDLow())
        {
            if (Aentry->BuildAuctionInfo(data))
                ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListAuctionItems(WorldPacket& data, Player* player,
    std::wstring const& wsearchedname, uint32 page, uint8 levelmin, uint8 levelmax, uint8 canUse,
    uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
    uint32& count, uint32& totalcount)
{
    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    int locdbc_idx = player->GetSession()->GetSessionDbcLocale();

    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry* Aentry = itr->second;
        Item* item = sAuctionMgr->GetAItem(Aentry->itemGUIDLow);
        if (!item)
            continue;

        ItemTemplate const* proto = item->GetTemplate();

        // 0xFFFFFFFF = -1
        if (itemClass != 0xFFFFFFFF && proto->Class != itemClass)
            continue;

        if (itemSubClass != 0xFFFFFFFF && proto->SubClass != itemSubClass)
            continue;

        if (inventoryType != 0xFFFFFFFF && proto->InventoryType != inventoryType)
            continue;

        if (quality != 0xFFFFFFFF && proto->Quality != quality)
            continue;

        if (levelmin != 0 && (proto->RequiredLevel < levelmin || (levelmax != 0 && proto->RequiredLevel > levelmax)))
            continue;

        if (canUse != 0 && player->CanUseItem(item) != EQUIP_ERR_OK)
            continue;

        // Allow search by suffix (ie: of the Monkey) or partial name (ie: Monkey)
        // No need to do any of this if no search term was entered
        if (!wsearchedname.empty())
        {
            std::string name = proto->Name1;
            if (name.empty())
                continue;

            // local name
            if (loc_idx >= 0)
                if (ItemLocale const* il = sObjectMgr->GetItemLocale(proto->ItemId))
                    ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

            // DO NOT use GetItemEnchantMod(proto->RandomProperty) as it may return a result
            //  that matches the search but it may not equal item->GetItemRandomPropertyId()
            //  used in BuildAuctionInfo() which then causes wrong items to be listed
            int32 propRefID = item->GetItemRandomPropertyId();

            if (propRefID)
            {
                // Append the suffix to the name (ie: of the Monkey) if one exists
                // These are found in ItemRandomProperties.dbc, not ItemRandomSuffix.dbc
                //  even though the DBC names seem misleading
                const ItemRandomPropertiesEntry* itemRandProp = sItemRandomPropertiesStore.LookupEntry(propRefID);

                if (itemRandProp)
                {
                    char* temp = itemRandProp->nameSuffix;
                    //char* temp = itemRandProp->nameSuffix;

                    // dbc local name
                    if (temp)
                    {
                        // Append the suffix (ie: of the Monkey) to the name using localization
                        // or default enUS if localization is invalid
                        name += ' ';
                        name += temp[locdbc_idx >= 0 ? locdbc_idx : LOCALE_enUS];
                    }
                }
            }

            // Perform the search (with or without suffix)
            if (!Utf8FitTo(name, wsearchedname))
                continue;
        }

        // Add the item if no search term or if entered search term was found
        if (count < 50 && totalcount >= page)
        {
            ++count;
            Aentry->BuildAuctionInfo(data);
        }

        ++totalcount;
    }
}

//this function inserts to WorldPacket auction's data
bool AuctionEntry::BuildAuctionInfo(WorldPacket& data) const
{
    Item* item = sAuctionMgr->GetAItem(itemGUIDLow);
    if (!item)
    {
        sLog->outError(LOG_FILTER_GENERAL, "AuctionEntry::BuildAuctionInfo: Auction %u has a non-existent item: %u", Id, itemGUIDLow);
        return false;
    }
    data << uint32(Id);
    data << uint32(item->GetEntry());

    for (uint8 i = 0; i < PROP_ENCHANTMENT_SLOT_0; ++i) // PROP_ENCHANTMENT_SLOT_0 = 8
    {
        data << uint32(item->GetEnchantmentId(EnchantmentSlot(i)));
        data << uint32(item->GetEnchantmentDuration(EnchantmentSlot(i)));
        data << uint32(item->GetEnchantmentCharges(EnchantmentSlot(i)));
    }
    
    // while not handle item dynamic info (needed only for battlepets auction lots)
    data << uint32(4);
    data << uint32(0);

    data << int32(item->GetItemRandomPropertyId());                 // Random item property id
    data << uint32(item->GetItemSuffixFactor());                    // SuffixFactor
    data << uint32(item->GetCount());                               // item->count
    data << uint32(item->GetSpellCharges());                        // item->charge FFFFFFF
    data << uint32(0);                                              // Unknown
    data << uint64(owner);                                          // Auction->owner
    data << uint64(startbid);                                       // Auction->startbid (not sure if useful)
    data << uint64(bid ? GetAuctionOutBid() : 0);
    // Minimal outbid
    data << uint64(buyout);                                         // Auction->buyout
    data << uint32((expire_time - time(NULL)) * IN_MILLISECONDS);   // time left
    data << uint64(bidder);                                         // auction->bidder current
    data << uint64(bid);                                            // current bid
    return true;
}

uint64 AuctionEntry::GetAuctionCut() const
{
    int64 cut = int64(CalculatePct(bid, auctionHouseEntry->cutPercent) * sWorld->getRate(RATE_AUCTION_CUT));
    return cut > 0 ? cut : 0;
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint64 AuctionEntry::GetAuctionOutBid() const
{
    uint64 outbid = CalculatePct(bid, 5);
    return outbid ? outbid : 1;
}

void AuctionEntry::DeleteFromDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_AUCTION);
    stmt->setUInt32(0, Id);
    trans->Append(stmt);
}

bool AuctionEntry::LoadFromDB(Field* fields)
{
    uint8 index = 0;

    Id          = fields[index++].GetUInt32();
    auctioneer  = fields[index++].GetUInt32();
    itemGUIDLow = fields[index++].GetUInt32();
    itemEntry   = fields[index++].GetUInt32();
    itemCount   = fields[index++].GetUInt32();
    owner       = fields[index++].GetUInt32();
    buyout      = fields[index++].GetUInt64();
    expire_time = fields[index++].GetUInt32();
    bidder      = fields[index++].GetUInt32();
    bid         = fields[index++].GetUInt64();
    startbid    = fields[index++].GetUInt64();
    deposit     = fields[index++].GetUInt64();

    CreatureData const* auctioneerData = sObjectMgr->GetCreatureData(auctioneer);
    if (!auctioneerData)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Auction %u has not a existing auctioneer (GUID : %u)", Id, auctioneer);
        return false;
    }

    CreatureTemplate const* auctioneerInfo = sObjectMgr->GetCreatureTemplate(auctioneerData->id);
    if (!auctioneerInfo)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Auction %u has not a existing auctioneer (GUID : %u Entry: %u)", Id, auctioneer, auctioneerData->id);
        return false;
    }

    factionTemplateId = auctioneerInfo->faction;
    auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(factionTemplateId);
    if (!auctionHouseEntry)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Auction %u has auctioneer (GUID : %u Entry: %u) with wrong faction %u", Id, auctioneer, auctioneerData->id, factionTemplateId);
        return false;
    }

    // check if sold item exists for guid
    // and itemEntry in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
    if (!sAuctionMgr->GetAItem(itemGUIDLow))
    {
        sLog->outError(LOG_FILTER_GENERAL, "Auction %u has not a existing item : %u", Id, itemGUIDLow);
        return false;
    }
    return true;
}

void AuctionHouseMgr::DeleteExpiredAuctionsAtStartup()
{
    // Deletes expired auctions. Should be called at server start before loading auctions.

    // DO NOT USE after auctions are already loaded since this deletes from the DB
    //  and assumes the auctions HAVE NOT been loaded into a list or AuctionEntryMap yet

    uint32 oldMSTime = getMSTime();
    uint32 expirecount = 0;
    time_t curTime = sWorld->GetGameTime();

    // Query the DB to see if there are any expired auctions
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_AUCTIONS);
    stmt->setUInt32(0, (uint32)curTime+60);
    PreparedQueryResult expAuctions = CharacterDatabase.Query(stmt);

    if (!expAuctions)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> No expired auctions to delete");
        return;
    }

    do
    {
        Field* fields = expAuctions->Fetch();

        AuctionEntry* auction = new AuctionEntry();

        // Can't use LoadFromDB() because it assumes the auction map is loaded
        if (!auction->LoadFromFieldList(fields))
        {
            // For some reason the record in the DB is broken (possibly corrupt
            //  faction info). Delete the object and move on.
            delete auction;
            continue;
        }

        if (auction->bidder==0)
        {
            // Cancel the auction, there was no bidder
            sAuctionMgr->SendAuctionExpiredMail(auction);
        }
        else
        {
            // Send the item to the winner and money to seller
            sAuctionMgr->SendAuctionSuccessfulMail(auction);
            sAuctionMgr->SendAuctionWonMail(auction);
        }

        // Call the appropriate AuctionHouseObject script
        //  ** Do we need to do this while core is still loading? **
        sScriptMgr->OnAuctionExpire(GetAuctionsMap(auction->factionTemplateId), auction);

        // Delete the auction from the DB
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        auction->DeleteFromDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        // Release memory
        delete auction;
        ++expirecount;

    } while (expAuctions->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Deleted %u expired auctions in %u ms", expirecount, GetMSTimeDiffToNow(oldMSTime));


}

bool AuctionEntry::LoadFromFieldList(Field* fields)
{
    // Loads an AuctionEntry item from a field list. Unlike "LoadFromDB()", this one
    //  does not require the AuctionEntryMap to have been loaded with items. It simply
    //  acts as a wrapper to fill out an AuctionEntry struct from a field list

    uint8 index = 0;

    Id          = fields[index++].GetUInt32();
    auctioneer  = fields[index++].GetUInt32();
    itemGUIDLow = fields[index++].GetUInt32();
    itemEntry   = fields[index++].GetUInt32();
    itemCount   = fields[index++].GetUInt32();
    owner       = fields[index++].GetUInt32();
    buyout      = fields[index++].GetUInt64();
    expire_time = fields[index++].GetUInt32();
    bidder      = fields[index++].GetUInt32();
    bid         = fields[index++].GetUInt64();
    startbid    = fields[index++].GetUInt64();
    deposit     = fields[index++].GetUInt64();

    CreatureData const* auctioneerData = sObjectMgr->GetCreatureData(auctioneer);
    if (!auctioneerData)
    {
        sLog->outError(LOG_FILTER_GENERAL, "AuctionEntry::LoadFromFieldList() - Auction %u has not a existing auctioneer (GUID : %u)", Id, auctioneer);
        return false;
    }

    CreatureTemplate const* auctioneerInfo = sObjectMgr->GetCreatureTemplate(auctioneerData->id);
    if (!auctioneerInfo)
    {
        sLog->outError(LOG_FILTER_GENERAL, "AuctionEntry::LoadFromFieldList() - Auction %u has not a existing auctioneer (GUID : %u Entry: %u)", Id, auctioneer, auctioneerData->id);
        return false;
    }

    factionTemplateId = auctioneerInfo->faction;
    auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(factionTemplateId);

    if (!auctionHouseEntry)
    {
        sLog->outError(LOG_FILTER_GENERAL, "AuctionEntry::LoadFromFieldList() - Auction %u has auctioneer (GUID : %u Entry: %u) with wrong faction %u", Id, auctioneer, auctioneerData->id, factionTemplateId);
        return false;
    }

    return true;
}

std::string AuctionEntry::BuildAuctionMailSubject(MailAuctionAnswers response) const
{
    std::ostringstream strm;
    strm << itemEntry << ":0:" << response << ':' << Id << ':' << itemCount;
    return strm.str();
}

std::string AuctionEntry::BuildAuctionMailBody(uint32 lowGuid, uint64 bid, uint64 buyout, uint64 deposit, uint64 cut)
{
    std::ostringstream strm;
    strm.width(16);
    strm << std::right << std::hex << MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);   // HIGHGUID_PLAYER always present, even for empty guids
    strm << std::dec << ':' << bid << ':' << buyout;
    strm << ':' << deposit << ':' << cut;
    return strm.str();
}
