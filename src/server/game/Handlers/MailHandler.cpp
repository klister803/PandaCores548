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

#include "DatabaseEnv.h"
#include "Mail.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Language.h"
#include "WordFilterMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Item.h"
#include "AccountMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

void WorldSession::HandleSendMail(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint64 money, COD;
    std::string receiver, subject, body;
    uint32 bodyLength, subjectLength, receiverLength;
    uint32 stationery, package;
    
    recvData >> stationery;                                 // CGMailInfo__m_selectedStationery
    recvData >> package;                                    // CGMailInfo__m_selectedPackage
    recvData >> money;                                      // CGMailInfo__m_sendMoney
    recvData >> COD;                                        // CGMailInfo__m_sendCOD
    
    recvData.ReadGuidMask<0, 4, 6>(mailbox);
    uint8 items_count = recvData.ReadBits(5);               // attached items count

    if (items_count > MAX_MAIL_ITEMS)                       // client limit
    {
        GetPlayer()->SendMailResult(0, MAIL_SEND, MAIL_ERR_TOO_MANY_ATTACHMENTS);
        recvData.rfinish();                   // set to end to avoid warnings spam
        return;
    }

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Player %u includes %u items, " UI64FMTD " copper and " UI64FMTD " COD copper with stationery = %u, package = %u",
    _player->GetGUIDLow(), items_count, money, COD, stationery, package);

    ObjectGuid itemGUIDs[MAX_MAIL_ITEMS];

    for (uint8 i = 0; i < items_count; ++i)
        recvData.ReadGuidMask<1, 0, 4, 5, 7, 3, 6, 2>(itemGUIDs[i]);

    recvData.ReadGuidMask<1>(mailbox);
    bodyLength = recvData.ReadBits(11);
    receiverLength = recvData.ReadBits(9);
    recvData.ReadGuidMask<7, 5>(mailbox);
    subjectLength = recvData.ReadBits(9);
    recvData.ReadGuidMask<2, 3>(mailbox);

    for (uint8 i = 0; i < items_count; ++i)
    {
        recvData.ReadGuidBytes<1, 7, 2, 3, 5, 4>(itemGUIDs[i]);
        recvData.read_skip<uint8>();           // item slot in mail, not used? researching...
        recvData.ReadGuidBytes<0, 6>(itemGUIDs[i]);
    }

    receiver = recvData.ReadString(receiverLength);
    recvData.ReadGuidBytes<6, 2>(mailbox);
    subject = recvData.ReadString(subjectLength);
    recvData.ReadGuidBytes<7, 4, 1, 5, 3>(mailbox);
    body = recvData.ReadString(bodyLength);
    recvData.ReadGuidBytes<0>(mailbox);

    // packet read complete, now do check

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    if (receiver.empty())
        return;

    Player* player = _player;

    if (player->getLevel() < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_MAIL_SENDER_REQ), sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    // check msg to bad word
    if (sWorld->getBoolConfig(CONFIG_WORD_FILTER_ENABLE))
    {
        std::string badWord = sWordFilterMgr->FindBadWord(subject, true);

        if (badWord.empty())
            badWord = sWordFilterMgr->FindBadWord(body, true);

        if (!badWord.empty())
        {
            SendNotification(LANG_WORD_FILTER_FOUND_BAD_WORD_IN_MAIL, badWord.c_str());
            // send packet
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }
    }

    uint64 rc = 0;
    if (normalizePlayerName(receiver))
        rc = ObjectMgr::GetPlayerGUIDByName(receiver);

    if (!rc)
    {
        sLog->outInfo(LOG_FILTER_NETWORKIO, "Player %u is sending mail to %s (GUID: not existed!) with subject %s and body %s includes %u items, " UI64FMTD " copper and " UI64FMTD " COD copper with stationery = %u, package = %u",
            player->GetGUIDLow(), receiver.c_str(), subject.c_str(), body.c_str(), items_count, money, COD, stationery, package);
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_NOT_FOUND);
        return;
    }

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Player %u is sending mail to %s (GUID: %u) with subject %s and body %s includes %u items, " UI64FMTD " copper and " UI64FMTD " COD copper with stationery = %u, package = %u", player->GetGUIDLow(), receiver.c_str(), GUID_LOPART(rc), subject.c_str(), body.c_str(), items_count, money, COD, stationery, package);

    if (player->GetGUID() == rc)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANNOT_SEND_TO_SELF);
        return;
    }

    uint64 cost = items_count ? 30 * items_count : 30;  // price hardcoded in client

    uint64 reqmoney = cost + money;

    if (!player->HasEnoughMoney(reqmoney) && !player->isGameMaster())
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Player* receive = ObjectAccessor::FindPlayer(rc);

    uint32 rc_team = 0;
    uint8 mails_count = 0;                                  //do not allow to send to one player more than 100 mails
    uint8 receiveLevel = 0;

    if (receive)
    {
        rc_team = receive->GetTeam();
        mails_count = receive->GetMailSize();
        receiveLevel = receive->getLevel();
    }
    else
    {
        rc_team = sObjectMgr->GetPlayerTeamByGUID(rc);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_LEVEL);
        stmt->setUInt32(0, GUID_LOPART(rc));
        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (result)
        {
            Field* fields = result->Fetch();
            receiveLevel = fields[0].GetUInt8();
        }
    }
    //do not allow to have more than 100 mails in mailbox.. mails count is in opcode uint8!!! - so max can be 255..
    if (mails_count > 100)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_CAP_REACHED);
        return;
    }
    // test the receiver's Faction... or all items are account bound
    bool accountBound = items_count ? true : false;
    for (uint8 i = 0; i < items_count; ++i)
    {
        Item* item = player->GetItemByGuid(itemGUIDs[i]);
        if (item)
        {
            ItemTemplate const* itemProto = item->GetTemplate();
            if (!itemProto || !(itemProto->Flags & ITEM_PROTO_FLAG_BIND_TO_ACCOUNT))
            {
                accountBound = false;
                break;
            }
        }
    }

    if (!accountBound && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_MAIL) && player->GetTeam() != rc_team && AccountMgr::IsPlayerAccount(GetSecurity()))
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_YOUR_TEAM);
        return;
    }

    if (receiveLevel < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_MAIL_RECEIVER_REQ), sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    uint32 rc_account = receive
        ? receive->GetSession()->GetAccountId()
        : ObjectMgr::GetPlayerAccountIdByGUID(rc);

    Item* items[MAX_MAIL_ITEMS];

    for (uint8 i = 0; i < items_count; ++i)
    {
        if (!itemGUIDs[i])
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        Item* item = player->GetItemByGuid(itemGUIDs[i]);

        // prevent sending bag with items (cheat: can be placed in bag after adding equipped empty bag to mail)
        if (!item)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        if (!item->CanBeTraded(true))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (item->IsBoundAccountWide() && item->IsSoulBound() && player->GetSession()->GetAccountId() != rc_account)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_NOT_SAME_ACCOUNT);
            return;
        }

        if (item->GetTemplate()->Flags & ITEM_PROTO_FLAG_CONJURED || item->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (COD && item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANT_SEND_WRAPPED_COD);
            return;
        }

        if (item->IsNotEmptyBag())
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_DESTROY_NONEMPTY_BAG);
            return;
        }

        items[i] = item;
    }

    player->SendMailResult(0, MAIL_SEND, MAIL_OK);

    player->ModifyMoney(-int64(reqmoney));
    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL, cost);

    bool needItemDelay = false;

    MailDraft draft(subject, body);

    if (items_count > 0 || money > 0)
    {
        if (items_count > 0)
        {
            for (uint8 i = 0; i < items_count; ++i)
            {
                Item* item = items[i];
                if (!AccountMgr::IsPlayerAccount(GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
                {
                    sLog->outCommand(GetAccountId(), "GM %s (Account: %u) mail item: %s (Entry: %u Count: %u) to player: %s (Account: %u)",
                        GetPlayerName().c_str(), GetAccountId(), item->GetTemplate()->Name1.c_str(), item->GetEntry(), item->GetCount(), receiver.c_str(), rc_account);
                }

                item->SetNotRefundable(GetPlayer()); // makes the item no longer refundable
                player->MoveItemFromInventory(item, true);
                item->SetOwnerGUID(rc);

                draft.AddItem(item);
            }

            // if item send to character at another account, then apply item delivery delay
            needItemDelay = player->GetSession()->GetAccountId() != rc_account;
        }

        if (money > 0 && !AccountMgr::IsPlayerAccount(GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
        {
            //TODO: charcter guid
            sLog->outCommand(GetAccountId(), "GM %s (Account: %u) mail money: " UI64FMTD " to player: %s (Account: %u)",
                GetPlayerName().c_str(), GetAccountId(), money, receiver.c_str(), rc_account);
        }
    }

    // If theres is an item, there is a one hour delivery delay if sent to another account's character.
    uint32 deliver_delay = needItemDelay ? sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY) : 0;

    // will delete item or place to receiver mail list
    draft
        .AddMoney(money)
        .AddCOD(COD)
        .SendMailTo(MailReceiver(receive, GUID_LOPART(rc)), MailSender(player), body.empty() ? MAIL_CHECK_MASK_COPIED : MAIL_CHECK_MASK_HAS_BODY, deliver_delay);
}

// Called when mail is read
void WorldSession::HandleMailMarkAsRead(WorldPacket & recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;

    recvData >> mailId;
    recvData.ReadGuidMask<7, 3, 4, 1, 5, 2, 6, 0>(mailbox);
    recvData.ReadGuidBytes<1, 4, 0, 2, 7, 5, 6, 3>(mailbox);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;
    Mail* m = player->GetMail(mailId);
    if (m)
    {
        if (player->unReadMails)
            --player->unReadMails;
        m->checked = m->checked | MAIL_CHECK_MASK_READ;
        player->m_mailsUpdated = true;
        m->state = MAIL_STATE_CHANGED;
    }
}

// Called when client deletes mail
void WorldSession::HandleMailDelete(WorldPacket & recvData)
{
    uint32 unk, mailId;
    recvData >> unk;                          // this+20
    recvData >> mailId;                       // mailTemplateId

    //if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
    //    return;

    Mail* m = _player->GetMail(mailId);
    Player* player = _player;
    player->m_mailsUpdated = true;
    if (m)
    {
        // delete shouldn't show up for COD mails
        if (m->COD)
        {
            player->SendMailResult(mailId, MAIL_DELETED, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        m->state = MAIL_STATE_DELETED;

        player->RemoveMail(mailId);
        player->RemoveMailItemsFromRedis(mailId);
    }

    player->SendMailResult(mailId, MAIL_DELETED, MAIL_OK);
}

void WorldSession::HandleMailReturnToSender(WorldPacket & recvData)
{
    ObjectGuid owner;
    uint32 mailId;

    recvData >> mailId;
    recvData.ReadGuidMask<5, 7, 4, 0, 1, 2, 6, 3>(owner);
    recvData.ReadGuidBytes<1, 0, 7, 5, 2, 4, 3, 6>(owner);

    //if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
    //    return;

    Player* player = _player;
    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_ERR_INTERNAL_ERROR);
        return;
    }
    //we can return mail now
    //so firstly delete the old one
    player->RemoveMail(mailId);

    // only return mail if the player exists (and delete if not existing)
    if (m->messageType == MAIL_NORMAL && m->sender)
    {
        MailDraft draft(m->subject, m->body);
        if (m->mailTemplateId)
            draft = MailDraft(m->mailTemplateId, false);     // items already included

        if (m->HasItems())
        {
            for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
            {
                if (Item* item = player->GetMItem(itr2->item_guid))
                    draft.AddItem(item);

                player->RemoveMItem(itr2->item_guid);
            }
        }
        draft.AddMoney(m->money).SendReturnToSender(GetAccountId(), m->receiver, m->sender);
    }

    delete m;                                               //we can deallocate old mail
    player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_OK);
}

//called when player takes item attached in mail
void WorldSession::HandleMailTakeItem(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    uint32 itemId;

    recvData >> mailId;
    recvData >> itemId;                                    // item guid low
    recvData.ReadGuidMask<1, 0, 6, 3, 5, 2, 7, 4>(mailbox);
    recvData.ReadGuidBytes<0, 6, 4, 3, 7, 1, 5, 2>(mailbox);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    // prevent cheating with skip client money check
    if (!player->HasEnoughMoney(uint64(m->COD)))
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Item* item = player->GetMItem(itemId);
    if(!item)
        return;

    if(item->GetEntry() == 38186)
        sLog->outDebug(LOG_FILTER_EFIR, "HandleMailTakeItem - item %u; count = %u playerGUID %u, itemGUID %u", item->GetEntry(), item->GetCount(), player->GetGUID(), item->GetGUID());

    item->SetOwnerGUID(player->GetGUID());
    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (msg == EQUIP_ERR_OK)
    {
        m->RemoveItem(itemId);
        m->removedItems.push_back(itemId);

        if (m->COD > 0)                                     //if there is COD, take COD money from player and send them to sender by mail
        {
            uint64 sender_guid = MAKE_NEW_GUID(m->sender, 0, HIGHGUID_PLAYER);
            Player* receive = ObjectAccessor::FindPlayer(sender_guid);

            uint32 sender_accId = 0;

            if (!AccountMgr::IsPlayerAccount(GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
            {
                std::string sender_name;
                if (receive)
                {
                    sender_accId = receive->GetSession()->GetAccountId();
                    sender_name = receive->GetName();
                }
                else
                {
                    // can be calculated early
                    sender_accId = ObjectMgr::GetPlayerAccountIdByGUID(sender_guid);

                    if (!ObjectMgr::GetPlayerNameByGUID(sender_guid, sender_name))
                        sender_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
                }
                sLog->outCommand(GetAccountId(), "GM %s (Account: %u) receive mail item: %s (Entry: %u Count: %u) and send COD money: " UI64FMTD " to player: %s (Account: %u)",
                    GetPlayerName().c_str(), GetAccountId(), item->GetTemplate()->Name1.c_str(), item->GetEntry(), item->GetCount(), m->COD, sender_name.c_str(), sender_accId);
            }
            else if (!receive)
                sender_accId = ObjectMgr::GetPlayerAccountIdByGUID(sender_guid);

            // check player existence
            if (receive || sender_accId)
            {
                MailDraft(m->subject, "")
                    .AddMoney(m->COD)
                    .SendMailTo(MailReceiver(receive, m->sender), MailSender(MAIL_NORMAL, m->receiver), MAIL_CHECK_MASK_COD_PAYMENT);
            }

            player->ModifyMoney(-int32(m->COD));
        }
        m->COD = 0;
        m->state = MAIL_STATE_CHANGED;
        player->m_mailsUpdated = true;
        player->RemoveMItem(item->GetGUIDLow());

        uint32 count = item->GetCount();                      // save counts before store and possible merge with deleting
        item->SetState(ITEM_UNCHANGED);                       // need to set this state, otherwise item cannot be removed later, if neccessary
        player->MoveItemToInventory(dest, item, true);
        player->SerializePlayerMails();

        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_OK, 0, itemId, count);
    }
    else
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_EQUIP_ERROR, msg);
}

void WorldSession::HandleMailTakeMoney(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint64 money;
    uint32 mailId;

    recvData >> mailId;
    recvData >> money;
    recvData.ReadGuidMask<4, 0, 1, 7, 5, 2, 3, 6>(mailbox);
    recvData.ReadGuidBytes<6, 1, 2, 0, 4, 5, 7, 3>(mailbox);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if ((!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL)) ||
        (money > 0 && m->money != money))
    {
        player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_OK);

    player->ModifyMoney(money);
    m->money = 0;
    m->state = MAIL_STATE_CHANGED;
    player->m_mailsUpdated = true;

    // save money and mail to prevent cheating
    player->SerializePlayerMails();
}

//called when player lists his received mails
void WorldSession::HandleGetMailList(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    recvData.ReadGuidMask<0, 2, 6, 7, 5, 4, 3, 1>(mailbox);
    recvData.ReadGuidBytes<1, 2, 4, 6, 0, 3, 5, 7>(mailbox);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    // client can't work with packets > max int16 value
    const uint32 maxPacketSize = 32767;

    GameObject* _mailbox = NULL;
    Trinity::MailBoxMasterCheck check(player);
    Trinity::GameObjectSearcher<Trinity::MailBoxMasterCheck> searcher(player, _mailbox, check);
    player->VisitNearbyObject(5.0f, searcher);

    WorldPacket data(SMSG_MAIL_LIST_RESULT, 200);         // guess size

    // create bit stream
    uint32 shownCount = 0;                                // real send to client mails amount
    uint32 totalCount  = 0;                               // real mails amount
    time_t cur_time = time(NULL);

    data << uint32(0);                                    // placeholder, real mail's count
    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 18);                                // placeholder, client mail show count

    // create bit stream
    for (PlayerMails::iterator itr = player->GetMailBegin(); itr != player->GetMailEnd(); ++itr)
    {
        // Only first 50 mails are displayed
        if (shownCount >= 50)
        {
            totalCount += 1;
            continue;
        }

        // skip deleted or not delivered (deliver delay not expired) mails
        if ((*itr)->state == MAIL_STATE_DELETED || cur_time < (*itr)->deliver_time)
            continue;

        uint8 item_count = (*itr)->items.size();                 // max count is MAX_MAIL_ITEMS (12)
        bool normalMail = (*itr)->messageType == MAIL_NORMAL;
        bool otherMail = (*itr)->messageType != MAIL_NORMAL;

        data.WriteBits((*itr)->subject.size(), 8);               // mail subject size
        data.WriteBit(otherMail);                                // non-player sender
        data.WriteBits(item_count, 17);                          // attached items count

        for (int i = 0; i < item_count; i++)
            data.WriteBit(0);                                    // unk bit, maybe related to accBound items

        data.WriteBit(normalMail);                               // player sender

        if (normalMail)
        {
            ObjectGuid plSender = MAKE_NEW_GUID((*itr)->sender, 0, HIGHGUID_PLAYER);
            data.WriteGuidMask<2, 0, 3, 7, 1, 4, 5, 6>(plSender);
        }

        data.WriteBits((*itr)->body.size(), 13);                 // mail body size

        ++totalCount;
        ++shownCount;
    }

    // required more reliable check bit stream size -> byte data size
    shownCount = 0;
    totalCount = 0;

    // create raw byte data
    for (PlayerMails::iterator itr = player->GetMailBegin(); itr != player->GetMailEnd(); ++itr)
    {
        Mail* mail = (*itr);
        // Only first 50 mails are displayed
        if (!mail || shownCount >= 50)
        {
            totalCount += 1;
            continue;
        }

        // skip deleted or not delivered (deliver delay not expired) mails
        if ((*itr)->state == MAIL_STATE_DELETED || cur_time < (*itr)->deliver_time)
            continue;

        uint8 item_count = mail->items.size();                 // max count is MAX_MAIL_ITEMS (12)
        bool normalMail = mail->messageType == MAIL_NORMAL;
        bool otherMail = mail->messageType != MAIL_NORMAL;

        for (uint8 i = 0; i < item_count; i++)
        {
            MailItemInfo* MailItem = &mail->items[i];
            Item* item = MailItem ? player->GetMItem(MailItem->item_guid) : NULL;

            data << uint32((item ? item->GetCount() : 0));   // stack count
            data << uint8(i);                                // item index, order in attachments
            // item dynamic info (needed for battlepets again!)
            data << uint32(4);
            data << uint32(0);
            //
            data << uint32((item ? item->GetGUIDLow() : 0));
            data << uint32((item ? item->GetItemSuffixFactor() : 0));
            data << uint32((item ? item->GetEntry() : 0));

            for (uint8 j = 0; j < MAX_INSPECTED_ENCHANTMENT_SLOT; j++)
            {
                data << uint32((item ? item->GetEnchantmentId((EnchantmentSlot)j) : 0));
                data << uint32((item ? item->GetEnchantmentDuration((EnchantmentSlot)j) : 0));
                data << uint32((item ? item->GetEnchantmentCharges((EnchantmentSlot)j) : 0));
            }

            data << uint32((item ? item->GetUInt32Value(ITEM_FIELD_DURABILITY) : 0));
            data << uint32((item ? item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY) : 0));
            data << int32((item ? item->GetItemRandomPropertyId() : 0));
            data << uint32((item ? item->GetSpellCharges() : 0));
        }

        data << uint32(mail->messageID);                       // message ID?
        data << uint8(mail->messageType);                      // message type

        if (normalMail)
        {
            ObjectGuid plSender = MAKE_NEW_GUID(mail->sender, 0, HIGHGUID_PLAYER);
            data.WriteGuidBytes<2, 6, 0, 5, 4, 7, 3, 1>(plSender);
        }

        data.WriteString(mail->body);                          // mail body
        data << uint64(mail->money);                           // money
        data << uint32(mail->mailTemplateId);                  // mail template ID (MailTemplate.dbc)
        data.WriteString(mail->subject);                       // mail subject
        if (otherMail)
            data << uint32(mail->sender);                      // non-player sender : creatureID or AuctionHouseID
        data << uint32(0);                                       // package, 2 - some interesting icon (Package.dbc)
        data << uint64(mail->COD);                             // COD
        data << uint32(mail->checked);                         // checked
        data << uint32(mail->stationery);                      // stationery (Stationery.dbc)
        data << float((mail->expire_time-time(NULL))/DAY);     // time to expire

        ++totalCount;
        ++shownCount;
    }

    data.put<uint32>(0, totalCount);                         // this will display warning about undelivered mail to player if realCount > mailsCount
    data.FlushBits();
    data.PutBits<uint8>(bit_pos, shownCount, 18);
    SendPacket(&data);

    // recalculate m_nextMailDelivereTime and unReadMails
    _player->UpdateNextMailTimeAndUnreads();
}

//used when player copies mail body to his inventory
void WorldSession::HandleMailCreateTextItem(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;

    recvData >> mailId;
    recvData.ReadGuidMask<4, 6, 2, 3, 7, 1, 5, 0>(mailbox);
    recvData.ReadGuidBytes<0, 7, 1, 5, 3, 2, 4, 6>(mailbox);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || (m->body.empty() && !m->mailTemplateId) || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    Item* bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if (!bodyItem->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), MAIL_BODY_ITEM_TEMPLATE, player))
    {
        delete bodyItem;
        return;
    }

    // in mail template case we need create new item text
    if (m->mailTemplateId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if (!mailTemplateEntry)
        {
            player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        bodyItem->SetText(mailTemplateEntry->content);
    }
    else
        bodyItem->SetText(m->body);

    bodyItem->SetUInt32Value(ITEM_FIELD_CREATOR, m->sender);
    bodyItem->SetFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_MAIL_TEXT_MASK);

    sLog->outInfo(LOG_FILTER_NETWORKIO, "HandleMailCreateTextItem mailid=%u", mailId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, bodyItem, false);
    if (msg == EQUIP_ERR_OK)
    {
        m->checked = m->checked | MAIL_CHECK_MASK_COPIED;
        m->state = MAIL_STATE_CHANGED;
        player->m_mailsUpdated = true;

        player->StoreItem(dest, bodyItem, true);
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_OK);
    }
    else
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_EQUIP_ERROR, msg);
        delete bodyItem;
    }
}

//TODO Fix me! ... this void has probably bad condition, but good data are sent
void WorldSession::HandleQueryNextMailTime(WorldPacket & /*recvData*/)
{
    WorldPacket data(MSG_QUERY_NEXT_MAIL_TIME, 8 + _player->unReadMails*24);

    if (_player->unReadMails > 0)
    {
        data << float(0);                                  // float
        data << uint32(0);                                 // count

        uint32 count = 0;
        time_t now = time(NULL);
        std::set<uint32> sentSenders;
        for (PlayerMails::iterator itr = _player->GetMailBegin(); itr != _player->GetMailEnd(); ++itr)
        {
            Mail* m = (*itr);
            // must be not checked yet
            if (m->checked & MAIL_CHECK_MASK_READ)
                continue;

            // and already delivered
            if (now < m->deliver_time)
                continue;

            // only send each mail sender once
            if (sentSenders.count(m->sender))
                continue;

            data << uint64(m->messageType == MAIL_NORMAL ? m->sender : 0);  // player guid
            data << uint32(m->messageType != MAIL_NORMAL ? m->sender : 0);  // non-player entries
            data << uint32(m->messageType);
            data << uint32(m->stationery);
            data << float(m->deliver_time - now);

            sentSenders.insert(m->sender);
            ++count;
            if (count == 2)                                  // do not display more than 2 mails
                break;
        }

        data.put<uint32>(4, count);
    }
    else
    {
        data << float(-DAY);
        data << uint32(0);
    }

    SendPacket(&data);
}
