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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include <list>
#include <vector>
#include <utility>

//! 5.4.1
void WorldSession::SendVoidStorageTransferResult(VoidTransferError result)
{
    WorldPacket data(SMSG_VOID_TRANSFER_RESULT, 4);
    data << uint32(result);
    SendPacket(&data);
}

//! 5.4.1
void WorldSession::SendVoidStorageFailed(bool unk/*=false*/)
{
    WorldPacket data(SMSG_VOID_STORAGE_FAILED, 4);
    data << uint8(unk << 7);   //unk bit
    SendPacket(&data);
}

//! 5.4.1
void WorldSession::HandleVoidStorageUnlock(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_VOID_STORAGE_UNLOCK");
    Player* player = GetPlayer();

    ObjectGuid npcGuid;
    recvData.ReadGuidMask<3, 2, 0, 1, 5, 6, 7, 4>(npcGuid);
    recvData.ReadGuidBytes<7, 1, 4, 6, 2, 3, 5, 0>(npcGuid);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageUnlock - Unit (GUID: %u) not found or player can't interact with it.", GUID_LOPART(npcGuid));
        return;
    }

    if (player->IsVoidStorageUnlocked())
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageUnlock - Player (GUID: %u, name: %s) tried to unlock void storage a 2nd time.", player->GetGUIDLow(), player->GetName());
        return;
    }

    player->ModifyMoney(-int64(VOID_STORAGE_UNLOCK));
    player->UnlockVoidStorage();
}

//! 5.4.1
void WorldSession::HandleVoidStorageQuery(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_VOID_STORAGE_QUERY");
    Player* player = GetPlayer();

    ObjectGuid npcGuid;
    recvData.ReadGuidMask<1, 2, 4, 7, 3, 5, 6, 0 >(npcGuid);
    recvData.ReadGuidBytes<5, 2, 7, 3, 4, 0, 1, 6>(npcGuid);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        SendVoidStorageFailed(true);
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageQuery - Unit (GUID: %u) not found or player can't interact with it.", GUID_LOPART(npcGuid));
        return;
    }

    if (!player->IsVoidStorageUnlocked())
    {
        SendVoidStorageFailed(true);
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageQuery - Player (GUID: %u, name: %s) queried void storage without unlocking it.", player->GetGUIDLow(), player->GetName());
        return;
    }

    uint8 count = 0;
    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
        if (player->GetVoidStorageItem(i))
            ++count;

    WorldPacket data(SMSG_VOID_STORAGE_CONTENTS, 2 * count + (14 + 4 + 4 + 4 + 4) * count);

    data.WriteBits(count, 7);

    ByteBuffer itemData((14 + 4 + 4 + 4 + 4) * count);

    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
    {
        VoidStorageItem* item = player->GetVoidStorageItem(i);
        if (!item)
            continue;

        ObjectGuid itemId = item->ItemId;
        ObjectGuid creatorGuid = item->CreatorGuid;

        data.WriteGuidMask<3>(creatorGuid);
        data.WriteGuidMask<3, 5>(itemId);
        data.WriteGuidMask<1>(creatorGuid);
        data.WriteGuidMask<0, 7, 2>(itemId);
        data.WriteGuidMask<0>(creatorGuid);
        data.WriteGuidMask<6>(itemId);
        data.WriteGuidMask<4, 5, 2, 7, 6>(creatorGuid);
        data.WriteGuidMask<4, 1>(itemId);


        itemData.WriteGuidBytes<3>(creatorGuid);
        itemData.WriteGuidBytes<6>(itemId);
        itemData.WriteGuidBytes<6>(creatorGuid);
        itemData << uint32(0);                      //unk
        itemData.WriteGuidBytes<7, 4>(itemId);
        itemData.WriteGuidBytes<7>(creatorGuid);
        itemData << uint32(item->ItemEntry);
        itemData << uint32(item->ItemSuffixFactor);
        itemData.WriteGuidBytes<4>(creatorGuid);
        itemData.WriteGuidBytes<0, 1>(itemId);
        itemData.WriteGuidBytes<1>(creatorGuid);
        itemData << uint32(i);
        itemData.WriteGuidBytes<5, 2>(creatorGuid);
        itemData.WriteGuidBytes<5, 3>(itemId);
        itemData.WriteGuidBytes<0>(creatorGuid);
        itemData << uint32(item->ItemRandomPropertyId);
        itemData.WriteGuidBytes<2>(itemId);
    }

    data.FlushBits();
    if (!itemData.empty())
        data.append(itemData);

    SendPacket(&data);
}

//! 5.4.1
void WorldSession::HandleVoidStorageTransfer(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_VOID_STORAGE_TRANSFER");
    Player* player = GetPlayer();

    // Read everything

    ObjectGuid npcGuid;
    recvData.ReadGuidMask<7>(npcGuid);
    uint32 countDeposit = recvData.ReadBits(24);
    std::vector<ObjectGuid> itemGuids(countDeposit);

    if (countDeposit > 9)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Player (GUID: %u, name: %s) wants to deposit more than 9 items (%u).", player->GetGUIDLow(), player->GetName(), countDeposit);
        return;
    }

    for (uint32 i = 0; i < countDeposit; ++i)
        recvData.ReadGuidMask<4, 3, 1, 2, 0, 7, 5, 6>(itemGuids[i]);

    uint32 countWithdraw = recvData.ReadBits(24);
    std::vector<ObjectGuid> itemIds(countWithdraw);
    
    recvData.ReadGuidMask<5, 0>(npcGuid);

    if (countWithdraw > 9)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Player (GUID: %u, name: %s) wants to withdraw more than 9 items (%u).", player->GetGUIDLow(), player->GetName(), countWithdraw);
        return;
    }

    for (uint32 i = 0; i < countWithdraw; ++i)
        recvData.ReadGuidMask<3, 7, 6, 1, 4, 0, 2, 5>(itemIds[i]);
    
    recvData.ReadGuidMask<2, 4, 6, 1, 3>(npcGuid);

    for (uint32 i = 0; i < countWithdraw; ++i)
        recvData.ReadGuidBytes<5, 3, 1, 7, 2, 4, 6, 0>(itemIds[i]);

    recvData.ReadGuidBytes<7>(npcGuid);

    for (uint32 i = 0; i < countDeposit; ++i)
        recvData.ReadGuidBytes<3, 7, 5, 1, 6, 0, 4, 2>(itemGuids[i]);

    recvData.ReadGuidBytes<2, 0, 6, 4, 1, 3, 5>(npcGuid);
    
    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Unit (GUID: %u) not found or player can't interact with it.", GUID_LOPART(npcGuid));
        return;
    }

    if (!player->IsVoidStorageUnlocked())
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Player (GUID: %u, name: %s) queried void storage without unlocking it.", player->GetGUIDLow(), player->GetName());
        return;
    }

    if (itemGuids.size() > player->GetNumOfVoidStorageFreeSlots())
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_FULL);
        return;
    }

    uint32 freeBagSlots = 0;
    if (itemIds.size() != 0)
    {
        // make this a Player function
        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
            if (Bag* bag = player->GetBagByPos(i))
                freeBagSlots += bag->GetFreeSlots();
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
            if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ++freeBagSlots;
    }

    if (itemIds.size() > freeBagSlots)
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INVENTORY_FULL);
        return;
    }

    float auraCostMod = player->GetTotalAuraMultiplier(SPELL_AURA_MOD_VOID_STORAGE_AND_TRANSMOGRIFY_COST);
    int64 cost = uint64(itemGuids.size() * VOID_STORAGE_STORE_ITEM * auraCostMod);
    if (!player->HasEnoughMoney(cost))
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_NOT_ENOUGH_MONEY);
        return;
    }

    Item* item = NULL;

    std::pair<VoidStorageItem, uint8> depositItems[VOID_STORAGE_MAX_DEPOSIT];
    uint8 depositCount = 0;
    for (std::vector<ObjectGuid>::iterator itr = itemGuids.begin(); itr != itemGuids.end(); ++itr)
    {
        item = player->GetItemByGuid(*itr);
        if (!item)
        {
            sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Player (GUID: %u, name: %s) wants to deposit an invalid item (item guid: " UI64FMTD ").", player->GetGUIDLow(), player->GetName(), uint64(*itr));
            continue;
        }

        VoidStorageItem itemVS(sObjectMgr->GenerateVoidStorageItemId(), item->GetEntry(), item->GetUInt64Value(ITEM_FIELD_CREATOR), item->GetItemRandomPropertyId(), item->GetItemSuffixFactor());

        uint8 slot = player->AddVoidStorageItem(itemVS);

        depositItems[depositCount++] = std::make_pair(itemVS, slot);

        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
    }

    cost = int64(depositCount * VOID_STORAGE_STORE_ITEM * auraCostMod);

    player->ModifyMoney(-cost);

    VoidStorageItem withdrawItems[VOID_STORAGE_MAX_WITHDRAW];

    VoidStorageItem* itemVS = NULL;
    uint8 withdrawCount = 0;
    uint8 slot = 0;
    ItemPosCountVec dest;
    InventoryResult msg;

    for (std::vector<ObjectGuid>::iterator itr = itemIds.begin(); itr != itemIds.end(); ++itr)
    {
        itemVS = player->GetVoidStorageItem(*itr, slot);
        if (!itemVS)
        {
            sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Player (GUID: %u, name: %s) tried to withdraw an invalid item (id: " UI64FMTD ")", player->GetGUIDLow(), player->GetName(), uint64(*itr));
            continue;
        }
     
        dest.clear();

        msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemVS->ItemEntry, 1);
        if (msg != EQUIP_ERR_OK)
        {
            SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INVENTORY_FULL);
            sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidStorageTransfer - Player (GUID: %u, name: %s) couldn't withdraw item id " UI64FMTD " because inventory was full.", player->GetGUIDLow(), player->GetName(), uint64(*itr));
            return;
        }

        item = player->StoreNewItem(dest, itemVS->ItemEntry, true, itemVS->ItemRandomPropertyId);
        item->SetUInt64Value(ITEM_FIELD_CREATOR, uint64(itemVS->CreatorGuid));
        item->SetBinding(true);
        player->SendNewItem(item, NULL, 1, false, false, false);

        withdrawItems[withdrawCount++] = *itemVS;

        player->DeleteVoidStorageItem(slot);
    }

    WorldPacket data(SMSG_VOID_STORAGE_TRANSFER_CHANGES, ((5 + 5 + (7 + 7) * depositCount +
        7 * withdrawCount) / 8) + 7 * withdrawCount + (7 + 7 + 4 * 4) * depositCount);

    data.WriteBits(depositCount, 4);

    ObjectGuid itemId = 0;
    ObjectGuid creatorGuid = 0;

    for (uint8 i = 0; i < depositCount; ++i)
    {
        itemId = depositItems[i].first.ItemId;
        creatorGuid = depositItems[i].first.CreatorGuid;

        data.WriteGuidMask<3, 0>(creatorGuid);
        data.WriteGuidMask<2>(itemId);
        data.WriteGuidMask<6, 2>(creatorGuid);
        data.WriteGuidMask<6>(itemId);
        data.WriteGuidMask<7>(creatorGuid);
        data.WriteGuidMask<7, 1, 4>(itemId);
        data.WriteGuidMask<1>(creatorGuid);
        data.WriteGuidMask<0, 5, 3>(itemId);
        data.WriteGuidMask<5, 4>(creatorGuid);
    }

    data.WriteBits(withdrawCount, 4);

    for (uint8 i = 0; i < withdrawCount; ++i)
    {
        itemId = withdrawItems[i].ItemId;
        data.WriteGuidMask<3, 7, 1, 6, 4, 0, 5, 2>(itemId);
    }

    data.FlushBits();

    for (uint8 i = 0; i < withdrawCount; ++i)
    {
        ObjectGuid itemId = withdrawItems[i].ItemId;
        data.WriteGuidBytes<3, 7, 6, 0, 4, 1, 5, 2>(itemId);
    }

    for (uint8 i = 0; i < depositCount; ++i)
    {
        itemId = depositItems[i].first.ItemId;
        creatorGuid = depositItems[i].first.CreatorGuid;

        data.WriteGuidBytes<3>(itemId);
        data.WriteGuidBytes<6, 3, 2>(creatorGuid);
        data.WriteGuidBytes<2>(itemId);
        data << uint32(depositItems[i].first.ItemEntry);
        data.WriteGuidBytes<0>(creatorGuid);
        data.WriteGuidBytes<5>(itemId);
        data << uint32(depositItems[i].first.ItemRandomPropertyId);
        data.WriteGuidBytes<0>(itemId);
        data.WriteGuidBytes<7, 1>(creatorGuid);
        data << uint32(0);                      //unk new on 5.4.x
        data.WriteGuidBytes<4>(creatorGuid);
        data.WriteGuidBytes<4>(itemId);
        data << uint32(depositItems[i].first.ItemSuffixFactor);
        data.WriteGuidBytes<5>(creatorGuid);
        data.WriteGuidBytes<6, 1>(itemId);
        data << uint32(depositItems[i].second); // slot
        data.WriteGuidBytes<7>(itemId);
    }

    SendPacket(&data);

    SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_NO_ERROR);
}

//! 5.4.1
void WorldSession::HandleVoidSwapItem(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_VOID_SWAP_ITEM");

    Player* player = GetPlayer();
    uint32 newSlot;
    ObjectGuid npcGuid;
    ObjectGuid itemId;

    recvData >> newSlot;
    recvData.ReadGuidMask<2, 3, 5, 7>(npcGuid);
    recvData.ReadGuidMask<0, 2>(itemId);
    recvData.ReadGuidMask<6>(npcGuid);
    recvData.ReadGuidMask<4, 1, 6, 5>(itemId);
    recvData.ReadGuidMask<4, 1>(npcGuid);
    recvData.ReadGuidMask<7, 3>(itemId);
    recvData.ReadGuidMask<0>(npcGuid);

    recvData.ReadGuidBytes<0>(itemId);
    recvData.ReadGuidBytes<0, 1, 2, 7, 4>(npcGuid);
    recvData.ReadGuidBytes<7, 1, 5>(itemId);
    recvData.ReadGuidBytes<3, 5>(npcGuid);
    recvData.ReadGuidBytes<6, 4, 3, 2>(itemId);
    recvData.ReadGuidBytes<6>(npcGuid);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidSwapItem - Unit (GUID: %u) not found or player can't interact with it.", GUID_LOPART(npcGuid));
        return;
    }

    if (!player->IsVoidStorageUnlocked())
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidSwapItem - Player (GUID: %u, name: %s) queried void storage without unlocking it.", player->GetGUIDLow(), player->GetName());
        return;
    }

    uint8 oldSlot;
    if (!player->GetVoidStorageItem(itemId, oldSlot))
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleVoidSwapItem - Player (GUID: %u, name: %s) requested swapping an invalid item (slot: %u, itemid: " UI64FMTD ").", player->GetGUIDLow(), player->GetName(), newSlot, uint64(itemId));
        return;
    }

    bool usedSrcSlot = player->GetVoidStorageItem(oldSlot) != NULL; // should be always true
    bool usedDestSlot = player->GetVoidStorageItem(newSlot) != NULL;
    ObjectGuid itemIdDest;
    if (usedDestSlot)
        itemIdDest = player->GetVoidStorageItem(newSlot)->ItemId;

    if (!player->SwapVoidStorageItem(oldSlot, newSlot))
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INTERNAL_ERROR_1);
        return;
    }

    WorldPacket data(SMSG_VOID_ITEM_SWAP_RESPONSE, 1 + (usedSrcSlot + usedDestSlot) * (1 + 7 + 4));
    
    data.WriteBit(!usedSrcSlot);
    data.WriteBit(!itemIdDest);
    data.WriteGuidMask<2, 5, 4, 1, 7, 6, 0, 3>(itemIdDest);
    data.WriteBit(!itemId);
    data.WriteBit(!usedDestSlot);
    data.WriteGuidMask<6, 1, 4, 3, 0, 7, 2, 5>(itemId);

    data.FlushBits();

    data.WriteGuidBytes<7, 2, 6, 1, 0, 4, 3, 5>(itemId);
    data.WriteGuidBytes<5, 6, 7, 0, 2, 4, 1, 3>(itemIdDest);
    
    if (usedSrcSlot)
        data << uint32(newSlot);
    if (usedDestSlot)
        data << uint32(oldSlot);

    SendPacket(&data);
}
