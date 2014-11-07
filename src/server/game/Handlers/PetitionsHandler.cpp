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
#include "Language.h"
#include "WordFilterMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "GossipDef.h"
#include "SocialMgr.h"

#define CHARTER_DISPLAY_ID 16161

// Charters ID in item_template
enum CharterItemIDs
{
    GUILD_CHARTER                                 = 5863,
};

enum CharterCosts
{
    GUILD_CHARTER_COST                            = 1000,
};

void WorldSession::HandlePetitionBuyOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_BUY");

    ObjectGuid guidNPC;
    std::string name;

    recvData.ReadGuidMask<3, 5, 4, 0, 6, 2, 1, 7>(guidNPC);
    uint32 strLen = recvData.ReadBits(7);
    recvData.ReadGuidBytes<6, 2, 1, 4, 0, 5>(guidNPC);
    name = recvData.ReadString(strLen);
    recvData.ReadGuidBytes<3, 7>(guidNPC);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petitioner with GUID %u tried sell petition: name %s", GUID_LOPART(guidNPC), name.c_str());

    // prevent cheating
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guidNPC, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandlePetitionBuyOpcode - Unit (GUID: %u) not found or you can't interact with him.", GUID_LOPART(guidNPC));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    uint32 charterid = 0;
    uint32 cost = 0;
    uint32 type = 0;
    if (creature->isTabardDesigner())
    {
        // if tabard designer, then trying to buy a guild charter.
        // do not let if already in guild.
        if (_player->GetGuildId())
            return;

        charterid = GUILD_CHARTER;
        cost = GUILD_CHARTER_COST;
        type = GUILD_CHARTER_TYPE;
    }

    if (sGuildMgr->GetGuildByName(name))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_EXISTS_S, name);
        return;
    }
    
    if (sObjectMgr->IsReservedName(name) || !ObjectMgr::IsValidCharterName(name)  ||
        (sWorld->getBoolConfig(CONFIG_WORD_FILTER_ENABLE) && !sWordFilterMgr->FindBadWord(name).empty()))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_INVALID, name);
        return;
    }

    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(charterid);
    if (!pProto)
    {
        _player->SendBuyError(BUY_ERR_CANT_FIND_ITEM, NULL, charterid, 0);
        return;
    }

    if (!_player->HasEnoughMoney(uint64(cost)))
    {                                                       //player hasn't got enough money
        _player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, creature, charterid, 0);
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, charterid, pProto->BuyCount);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, NULL, NULL, charterid);
        return;
    }

    _player->ModifyMoney(-(int32)cost);
    Item* charter = _player->StoreNewItem(dest, charterid, true);
    if (!charter)
        return;

    // ITEM_FIELD_ENCHANTMENT_1_1 is guild id 
    // ITEM_FIELD_ENCHANTMENT_1_1+1 is current signatures count (showed on item)
    charter->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1, charter->GetGUIDLow());
    charter->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1+1, 0);
    charter->SetState(ITEM_CHANGED, _player);
    _player->SendNewItem(charter, NULL, 1, true, false);

    // a petition is invalid, if both the owner and the type matches
    // we checked above, if this player is in an arenateam, so this must be
    // datacorruption
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_BY_OWNER);
    stmt->setUInt32(0, _player->GetGUIDLow());
    stmt->setUInt8(1, type);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    std::ostringstream ssInvalidPetitionGUIDs;

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            ssInvalidPetitionGUIDs << '\'' << fields[0].GetUInt32() << "', ";
        } while (result->NextRow());
    }

    // delete petitions with the same guid as this one
    ssInvalidPetitionGUIDs << '\'' << charter->GetGUIDLow() << '\'';

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Invalid petition GUIDs: %s", ssInvalidPetitionGUIDs.str().c_str());
    CharacterDatabase.EscapeString(name);
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM petition WHERE petitionguid IN (%s)",  ssInvalidPetitionGUIDs.str().c_str());
    trans->PAppend("DELETE FROM petition_sign WHERE petitionguid IN (%s)", ssInvalidPetitionGUIDs.str().c_str());

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION);
    stmt->setUInt32(0, _player->GetGUIDLow());
    stmt->setUInt32(1, charter->GetGUIDLow());
    stmt->setString(2, name);
    stmt->setUInt8(3, uint8(type));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandlePetitionShowSignOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_SHOW_SIGNATURES");

    uint8 signs = 0;
    ObjectGuid petitionguid;
    recvData.ReadGuidMask<0, 5, 2, 1, 4, 6, 3, 7>(petitionguid);
    recvData.ReadGuidBytes<2, 7, 3, 0, 4, 6, 1, 5>(petitionguid);

    // solve (possible) some strange compile problems with explicit use GUID_LOPART(petitionguid) at some GCC versions (wrong code optimization in compiler?)
    uint32 petitionGuidLow = GUID_LOPART(petitionguid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, petitionGuidLow);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outDebug(LOG_FILTER_PLAYER_ITEMS, "Petition %u is not found for player %u %s", GUID_LOPART(petitionguid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName());
        return;
    }
    Field* fields = result->Fetch();
    uint32 type = fields[0].GetUInt8();

    // if has guild => error, return;
    if (_player->GetGuildId())
        return;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);
    stmt->setUInt32(0, petitionGuidLow);

    result = CharacterDatabase.Query(stmt);

    // result == NULL also correct in case no sign yet
    if (result)
        signs = uint8(result->GetRowCount());

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_SHOW_SIGNATURES petition entry: '%u'", petitionGuidLow);

    ObjectGuid playerGUID = _player->GetGUID();

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES, (8+8+4+1+signs*12));
    data.WriteGuidMask<3>(playerGUID);
    data.WriteGuidMask<2>(petitionguid);
    data.WriteGuidMask<0>(playerGUID);
    data.WriteGuidMask<0>(petitionguid);
    data.WriteGuidMask<1>(playerGUID);
    data.WriteGuidMask<6, 7, 1>(petitionguid);
    data.WriteGuidMask<5, 6>(playerGUID);
    data.WriteGuidMask<5>(petitionguid);
    data.WriteGuidMask<4>(playerGUID);
    data.WriteGuidMask<3>(petitionguid);
    data.WriteGuidMask<2>(playerGUID);

    data.WriteBits(signs, 21);                              // sign's count

    for (uint8 i = 0; i < signs; i++)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();
        ObjectGuid plSignGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

        data.WriteGuidMask<2, 0, 3, 6, 4, 5, 7, 1>(plSignGuid);
    }

    data.WriteGuidMask<4>(petitionguid);
    data.WriteGuidMask<7>(playerGUID);

    for (uint8 i = 0; i < signs; i++)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();
        ObjectGuid plSignGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

        data.WriteGuidBytes<6, 3, 0, 4, 7, 5, 1, 2>(plSignGuid);
        data << uint32(0);
    }

    data.WriteGuidBytes<2>(playerGUID);
    data.WriteGuidBytes<4, 2, 3, 0>(petitionguid);
    data.WriteGuidBytes<0>(playerGUID);
    data.WriteGuidBytes<6>(petitionguid);

    data << uint32(petitionGuidLow);               // CGPetitionInfo__m_petitionID

    data.WriteGuidBytes<5, 1>(playerGUID);
    data.WriteGuidBytes<7>(petitionguid);
    data.WriteGuidBytes<6>(playerGUID);
    data.WriteGuidBytes<5, 1>(petitionguid);
    data.WriteGuidBytes<4, 7, 3>(playerGUID);

    SendPacket(&data);
}

void WorldSession::HandlePetitionQueryOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_QUERY");   // ok

    uint32 guildguid;
    ObjectGuid petitionguid;
    recvData >> guildguid;                                 // in Trinity always same as GUID_LOPART(petitionguid)
    recvData.ReadGuidMask<4, 7, 0, 2, 3, 6, 1, 5>(petitionguid);
    recvData.ReadGuidBytes<4, 6, 7, 5, 3, 0, 1, 2>(petitionguid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_QUERY Petition GUID %u Guild GUID %u", GUID_LOPART(petitionguid), guildguid);

    SendPetitionQueryOpcode(petitionguid);
}

void WorldSession::SendPetitionQueryOpcode(uint64 petitionguid)
{
    ObjectGuid ownerguid;
    uint32 type;
    std::string name = "NO_NAME_FOR_GUID";

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);

    stmt->setUInt32(0, GUID_LOPART(petitionguid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        ownerguid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
        name      = fields[1].GetString();
        type      = fields[2].GetUInt8();
    }
    else
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_QUERY failed for petition (GUID: %u)", GUID_LOPART(petitionguid));
        return;
    }

    WorldPacket data(SMSG_PETITION_QUERY_RESPONSE);
    bool unk = data.WriteBit(1);
    if (unk)
    {
        data.WriteGuidMask<5>(ownerguid);
        data.WriteBits(name.size(), 7);              // guild name->length
        data.WriteGuidMask<3, 6, 1>(ownerguid);

        for (uint32 i = 0; i < 10; i++)
            data.WriteBits(0, 6);                    // unk strings[]->length

        data.WriteGuidMask<0>(ownerguid);
        data.WriteBits(0, 12);                       // unk string 1->length
        data.WriteGuidMask<2, 7, 4>(ownerguid);

        data.WriteGuidBytes<0>(ownerguid);
        data << uint32(GUID_LOPART(petitionguid));   // petition low guid
        data << uint32(0);
        data << uint32(GUILD_CHARTER_TYPE);          // unk number 4 in sniffs, type?
        data.WriteString("");                        // unk string 1
        data << uint32(0);
        data.WriteGuidBytes<5>(ownerguid);
        data << uint32(0);
        data.WriteGuidBytes<6, 4>(ownerguid);
        data.WriteString(name);                      // guild name
        data.WriteGuidBytes<3>(ownerguid);
        data << uint32(GUILD_CHARTER_TYPE);          // unk number 4 in sniffs, type?
        data.WriteGuidBytes<2>(ownerguid);
        data << uint16(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data.WriteGuidBytes<7, 1>(ownerguid);

        for (uint32 i = 0; i < 10; i++)
            data.WriteString("");                     // unk strings[]

        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
    }

    data << uint32(GUID_LOPART(petitionguid));        // petition low guid

    SendPacket(&data);
}

void WorldSession::HandlePetitionRenameOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode MSG_PETITION_RENAME");

    ObjectGuid petitionGuid;
    uint32 type;
    std::string newName;

    recvData.ReadGuidMask<4, 7, 5, 1, 2, 6>(petitionGuid);
    uint32 strLen = recvData.ReadBits(7);
    recvData.ReadGuidMask<3, 0>(petitionGuid);
    recvData.ReadGuidBytes<0, 1, 5, 2, 7>(petitionGuid);
    newName = recvData.ReadString(strLen);
    recvData.ReadGuidBytes<4, 6, 3>(petitionGuid);

    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, GUID_LOPART(petitionGuid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        type = fields[0].GetUInt8();
    }
    else
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_PETITION_QUERY failed for petition (GUID: %u)", GUID_LOPART(petitionGuid));
        return;
    }

    if (sGuildMgr->GetGuildByName(newName))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_EXISTS_S, newName);
        return;
    }
    if (sObjectMgr->IsReservedName(newName) || !ObjectMgr::IsValidCharterName(newName) ||
        (sWorld->getBoolConfig(CONFIG_WORD_FILTER_ENABLE) && !sWordFilterMgr->FindBadWord(newName).empty()))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_INVALID, newName);
        return;
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PETITION_NAME);

    stmt->setString(0, newName);
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    CharacterDatabase.Execute(stmt);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petition (GUID: %u) renamed to '%s'", GUID_LOPART(petitionGuid), newName.c_str());

    WorldPacket data(SMSG_PETITION_RENAME, (8+newName.size()+1));
    data.WriteGuidMask<1, 0>(petitionGuid);
    data.WriteBits(newName.size(), 7);
    data.WriteGuidMask<3, 5, 2, 6, 4, 7>(petitionGuid);
    data.WriteGuidBytes<6, 7>(petitionGuid);
    data.WriteString(newName);
    data.WriteGuidBytes<2, 4, 1, 0, 3, 5>(petitionGuid);
    SendPacket(&data);
}

void WorldSession::HandlePetitionSignOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_PETITION_SIGN");    // ok

    Field* fields;
    ObjectGuid petitionGuid;
    uint8 unk;
    recvData >> unk;
    recvData.ReadGuidMask<1, 7, 4, 5, 3, 6, 0, 2>(petitionGuid);
    recvData.ReadGuidBytes<4, 6, 7, 2, 5, 3, 1, 0>(petitionGuid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURES);

    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Petition %u is not found for player %u %s", GUID_LOPART(petitionGuid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName());
        return;
    }

    fields = result->Fetch();
    uint64 ownerGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
    uint64 signs = fields[1].GetUInt64();
    uint8 type = fields[2].GetUInt8();

    if (type != GUILD_CHARTER_TYPE)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Petition %u of player %u %s has not supported type %u", GUID_LOPART(petitionGuid), GetPlayer()->GetGUIDLow(), GetPlayer()->GetName(), type);
        return;
    }

    uint32 playerGuid = _player->GetGUIDLow();
    if (GUID_LOPART(ownerGuid) == playerGuid)
        return;

    // not let enemies sign guild charter
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeam() != sObjectMgr->GetPlayerTeamByGUID(ownerGuid))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (_player->GetGuildId())
    {
        Guild::SendCommandResult(this, GUILD_INVITE_S, ERR_ALREADY_IN_GUILD_S, _player->GetName());
        return;
    }

    if (_player->GetGuildIdInvited())
    {
        Guild::SendCommandResult(this, GUILD_INVITE_S, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
        return;
    }

    if (++signs > type)                                        // client signs maximum
        return;

    // Client doesn't allow to sign petition two times by one character, but not check sign by another character from same account
    // not allow sign another player from already sign player account
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIG_BY_ACCOUNT);

    stmt->setUInt32(0, GetAccountId());
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));

    result = CharacterDatabase.Query(stmt);

    if (result)
    {
        // close at signer side
        SendPetitionSignResult(_player->GetGUID(), petitionGuid, PETITION_SIGN_ALREADY_SIGNED);
        return;
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION_SIGNATURE);

    stmt->setUInt32(0, GUID_LOPART(ownerGuid));
    stmt->setUInt32(1, GUID_LOPART(petitionGuid));
    stmt->setUInt32(2, playerGuid);
    stmt->setUInt32(3, GetAccountId());

    CharacterDatabase.Execute(stmt);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "PETITION SIGN: GUID %u by player: %s (GUID: %u Account: %u)", GUID_LOPART(petitionGuid), _player->GetName(), playerGuid, GetAccountId());

    // close at signer side
    SendPetitionSignResult(_player->GetGUID(), petitionGuid, PETITION_SIGN_OK);

    if (Player* owner = ObjectAccessor::FindPlayer(ownerGuid))
    {
        // update signs count on charter
        if (Item* item = owner->GetItemByGuid(petitionGuid))
            item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1+1, signs);

        // update sign result for owner
        owner->GetSession()->SendPetitionSignResult(_player->GetGUID(), petitionGuid, PETITION_SIGN_OK);
    }
}

void WorldSession::SendPetitionSignResult(uint64 playerGuid, uint64 petitionGuid, uint8 result)
{
    WorldPacket data(SMSG_PETITION_SIGN_RESULTS, 8 + 8 + 1 + 1 + 1);

    data.WriteGuidMask<6>(playerGuid);
    data.WriteGuidMask<3, 0, 6, 7>(petitionGuid);
    data.WriteGuidMask<2>(playerGuid);
    data.WriteGuidMask<5>(petitionGuid);
    data.WriteGuidMask<4>(playerGuid);
    data.WriteGuidMask<2>(petitionGuid);
    data.WriteGuidMask<1, 5, 7, 0>(playerGuid);
    data.WriteBits(result, 4);
    data.WriteGuidMask<1>(petitionGuid);
    data.WriteGuidMask<3>(playerGuid);
    data.WriteGuidMask<4>(petitionGuid);

    data.WriteGuidBytes<5>(petitionGuid);
    data.WriteGuidBytes<0, 4>(playerGuid);
    data.WriteGuidBytes<1>(petitionGuid);
    data.WriteGuidBytes<2, 3>(playerGuid);
    data.WriteGuidBytes<4, 3>(petitionGuid);
    data.WriteGuidBytes<7>(playerGuid);
    data.WriteGuidBytes<2>(petitionGuid);
    data.WriteGuidBytes<5, 1>(playerGuid);
    data.WriteGuidBytes<6>(petitionGuid);
    data.WriteGuidBytes<6>(playerGuid);
    data.WriteGuidBytes<0, 7>(petitionGuid);

    SendPacket(&data);
}

void WorldSession::HandlePetitionDeclineOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode MSG_PETITION_DECLINE");  // ok

    ObjectGuid petitionguid;
    uint64 ownerguid;
    recvData.ReadGuidMask<7, 3, 5, 1, 0, 6, 2, 4>(petitionguid);
    recvData.ReadGuidBytes<6, 0, 3, 5, 2, 1, 7, 4>(petitionguid);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petition %u declined by %u", GUID_LOPART(petitionguid), _player->GetGUIDLow());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_OWNER_BY_GUID);

    stmt->setUInt32(0, GUID_LOPART(petitionguid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return;

    Field* fields = result->Fetch();
    ownerguid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);

    /*Player* owner = ObjectAccessor::FindPlayer(ownerguid);
    if (owner)                                               // petition owner online
    {
        WorldPacket data(MSG_PETITION_DECLINE, 8);
        data << uint64(_player->GetGUID());
        owner->GetSession()->SendPacket(&data);
    }*/
}

void WorldSession::HandleOfferPetitionOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_OFFER_PETITION");

    uint8 signs = 0;
    ObjectGuid petitionguid, plguid;
    uint32 type, unk;
    Player* player;
    recvData >> unk;
    recvData.ReadGuidMask<4>(petitionguid);
    recvData.ReadGuidMask<0>(plguid);
    recvData.ReadGuidMask<2>(petitionguid);
    recvData.ReadGuidMask<6>(plguid);
    recvData.ReadGuidMask<5>(petitionguid);
    recvData.ReadGuidMask<2>(plguid);
    recvData.ReadGuidMask<1, 7, 6>(petitionguid);
    recvData.ReadGuidMask<4, 3>(plguid);
    recvData.ReadGuidMask<3, 0>(petitionguid);
    recvData.ReadGuidMask<1, 7, 5>(plguid);

    recvData.ReadGuidBytes<7, 6>(plguid);
    recvData.ReadGuidBytes<3>(petitionguid);
    recvData.ReadGuidBytes<0, 2, 5>(plguid);
    recvData.ReadGuidBytes<4, 2, 0, 6, 5, 1>(petitionguid);
    recvData.ReadGuidBytes<3, 4, 1>(plguid);
    recvData.ReadGuidBytes<7>(petitionguid);

    player = ObjectAccessor::FindPlayer(plguid);
    if (!player)
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_TYPE);

    stmt->setUInt32(0, GUID_LOPART(petitionguid));

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return;

    Field* fields = result->Fetch();
    type = fields[0].GetUInt8();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "OFFER PETITION: type %u, GUID1 %u, to player id: %u", type, GUID_LOPART(petitionguid), GUID_LOPART(plguid));

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeam() != player->GetTeam())
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (player->GetGuildId())
    {
        Guild::SendCommandResult(this, GUILD_INVITE_S, ERR_ALREADY_IN_GUILD_S, _player->GetName());
        return;
    }

    if (player->GetGuildIdInvited())
    {
        Guild::SendCommandResult(this, GUILD_INVITE_S, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
        return;
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);
    stmt->setUInt32(0, GUID_LOPART(petitionguid));

    result = CharacterDatabase.Query(stmt);

    // result == NULL also correct charter without signs
    if (result)
    {
        signs = uint8(result->GetRowCount());

        // check already signed petition
        for (uint8 i = 0; i < signs; i++)
        {
            Field* fields1 = result->Fetch();
            uint32 lowGuid = fields1[0].GetUInt32();
            ObjectGuid plSignGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

            if (player->GetObjectGuid() == plSignGuid)
            {
                // TODO : find and send correctly data structure, this response are not worked...research in future 
                //SendPetitionSignResult(player->GetGUID(), petitionguid, PETITION_SIGN_ALREADY_SIGNED);
                return;
            }
        }
    }

    ObjectGuid playerGUID = _player->GetGUID();

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES, (8+8+4+1+signs*12));
    data.WriteGuidMask<3>(playerGUID);
    data.WriteGuidMask<2>(petitionguid);
    data.WriteGuidMask<0>(playerGUID);
    data.WriteGuidMask<0>(petitionguid);
    data.WriteGuidMask<1>(playerGUID);
    data.WriteGuidMask<6, 7, 1>(petitionguid);
    data.WriteGuidMask<5, 6>(playerGUID);
    data.WriteGuidMask<5>(petitionguid);
    data.WriteGuidMask<4>(playerGUID);
    data.WriteGuidMask<3>(petitionguid);
    data.WriteGuidMask<2>(playerGUID);

    data.WriteBits(signs, 21);                              // sign's count

    for (uint8 i = 0; i < signs; i++)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();
        ObjectGuid plSignGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

        data.WriteGuidMask<2, 0, 3, 6, 4, 5, 7, 1>(plSignGuid);
    }

    data.WriteGuidMask<4>(petitionguid);
    data.WriteGuidMask<7>(playerGUID);

    for (uint8 i = 0; i < signs; i++)
    {
        Field* fields2 = result->Fetch();
        uint32 lowGuid = fields2[0].GetUInt32();
        ObjectGuid plSignGuid = MAKE_NEW_GUID(lowGuid, 0, HIGHGUID_PLAYER);

        data.WriteGuidBytes<6, 3, 0, 4, 7, 5, 1, 2>(plSignGuid);
        data << uint32(0);
    }

    data.WriteGuidBytes<2>(playerGUID);
    data.WriteGuidBytes<4, 2, 3, 0>(petitionguid);
    data.WriteGuidBytes<0>(playerGUID);
    data.WriteGuidBytes<6>(petitionguid);

    data << uint32(GUID_LOPART(petitionguid));        // CGPetitionInfo__m_petitionID

    data.WriteGuidBytes<5, 1>(playerGUID);
    data.WriteGuidBytes<7>(petitionguid);
    data.WriteGuidBytes<6>(playerGUID);
    data.WriteGuidBytes<5, 1>(petitionguid);
    data.WriteGuidBytes<4, 7, 3>(playerGUID);

    player->GetSession()->SendPacket(&data);
}

void WorldSession::HandleTurnInPetitionOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received opcode CMSG_TURN_IN_PETITION");

    // Get petition guid from packet
    WorldPacket data;
    ObjectGuid petitionGuid;

    recvData.ReadGuidMask<6, 5, 7, 4, 3, 0, 1, 2>(petitionGuid);
    recvData.ReadGuidBytes<3, 5, 4, 2, 7, 0, 1, 6>(petitionGuid);

    // Check if player really has the required petition charter
    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
        return;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Petition %u turned in by %u", GUID_LOPART(petitionGuid), _player->GetGUIDLow());

    // Get petition data from db
    uint32 ownerguidlo;
    uint32 type;
    std::string name;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();
        ownerguidlo = fields[0].GetUInt32();
        name = fields[1].GetString();
        type = fields[2].GetUInt8();
    }
    else
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Player %s (guid: %u) tried to turn in petition (guid: %u) that is not present in the database", _player->GetName(), _player->GetGUIDLow(), GUID_LOPART(petitionGuid));
        return;
    }

    // Only the petition owner can turn in the petition
    if (_player->GetGUIDLow() != ownerguidlo)
        return;

    // Check if player is already in a guild
    if (_player->GetGuildId())
    {
        data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
        data.WriteBits(PETITION_TURN_ALREADY_IN_GUILD, 4);
        _player->GetSession()->SendPacket(&data);
        return;
    }

    // Check if guild name is already taken
    if (sGuildMgr->GetGuildByName(name))
    {
        Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_NAME_EXISTS_S, name);
        return;
    }

    // Get petition signatures from db
    uint8 signatures;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PETITION_SIGNATURE);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    result = CharacterDatabase.Query(stmt);

    if (result)
        signatures = uint8(result->GetRowCount());
    else
        signatures = 0;

    uint32 requiredSignatures;
    requiredSignatures = sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS);

    // Notify player if signatures are missing
    if (signatures < requiredSignatures)
    {
        data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
        data.WriteBits(PETITION_TURN_NEED_MORE_SIGNATURES, 4);
        SendPacket(&data);
        return;
    }

    // Proceed with guild creation

    // Delete charter item
    _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    // Create guild
    Guild* guild = new Guild;

    if (!guild->Create(_player, name))
    {
        delete guild;
        return;
    }

    // Register guild and add guild master
    sGuildMgr->AddGuild(guild);

    // Add members from signatures
    for (uint8 i = 0; i < signatures; ++i)
    {
        Field* fields = result->Fetch();
        guild->AddMember(MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));
        result->NextRow();
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(petitionGuid));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    // created
    sLog->outDebug(LOG_FILTER_NETWORKIO, "TURN IN PETITION GUID %u", GUID_LOPART(petitionGuid));

    data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
    data.WriteBits(PETITION_TURN_OK, 4);
    SendPacket(&data);
}

void WorldSession::HandlePetitionShowListOpcode(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Received CMSG_PETITION_SHOWLIST");

    ObjectGuid guid;
    recvData.ReadGuidMask<1, 3, 0, 6, 7, 2, 5, 4>(guid);
    recvData.ReadGuidBytes<1, 7, 4, 2, 3, 5, 6, 0>(guid);

    SendPetitionShowList(guid);
}

void WorldSession::SendPetitionShowList(uint64 guid)
{
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandlePetitionShowListOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    if (!creature->isTabardDesigner())
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandlePetitionShowListOpcode - Unit (GUID: %u) is not a tabard designer.", uint32(GUID_LOPART(guid)));
        return;
    }

    WorldPacket data(SMSG_PETITION_SHOWLIST, 4 + 8 + 1);
    data.WriteGuidMask<4, 0, 1, 6, 3, 7, 5, 2>(guid);
    data.WriteGuidBytes<2, 3, 1, 5>(guid);
    data << uint32(GUILD_CHARTER_COST);                 // charter cost
    data.WriteGuidBytes<4, 0, 6, 7>(guid);

    SendPacket(&data);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Sent SMSG_PETITION_SHOWLIST");
}
