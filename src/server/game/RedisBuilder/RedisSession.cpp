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

#include "BracketMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Bracket.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "RedisBuilderMgr.h"

void WorldSession::SaveEnum()
{
    RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "enumdata", sRedisBuilder->BuildString(EnumData).c_str(), GetAccountId(), [&](const RedisValue &v, uint64 _accountId) {
        sLog->outInfo(LOG_FILTER_REDIS, "WorldSession::SaveEnum _accountId %u", _accountId);
    });
}

void WorldSession::LoadEnumData(const RedisValue* v, uint64 accountId)
{
    if (!sRedisBuilder->LoadFromRedis(v, EnumData))
    {
        // remove expired bans
        PreparedStatement* stmt = NULL;

        /// get all the data necessary for loading all characters (along with their pets) on the account

        if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM_DECLINED_NAME);
        else
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM);

        stmt->setUInt32(0, GetAccountId());

        _charEnumCallback = CharacterDatabase.AsyncQuery(stmt);
        sLog->outInfo(LOG_FILTER_REDIS, "WorldSession::LoadEnumData data is empty");
        return;
    }

    WorldPacket data(SMSG_CHAR_ENUM);

    uint32 charCount = 0;
    ByteBuffer bitBuffer;
    ByteBuffer dataBuffer;

    _allowedCharsToLogin.clear();

    charCount = uint32(EnumData.size());

    bitBuffer.WriteBits(charCount, 16);

    bitBuffer.reserve(24 * charCount / 8);
    dataBuffer.reserve(charCount * 381);

    for (auto iter = EnumData.begin(); iter != EnumData.end(); ++iter)
    {
        uint32 guid = atoi(iter.memberName());
        auto dataValue = *iter;

        Player::BuildEnumData(guid, dataValue, &dataBuffer, &bitBuffer);
        _allowedCharsToLogin.insert(guid);
    }

    bitBuffer.WriteBit(1);
    bitBuffer.WriteBits(0, 21); // restricted faction change rules count
    bitBuffer.FlushBits();

    data.append(bitBuffer);
    if (charCount)
        data.append(dataBuffer);

    SendPacket(&data);
}

void WorldSession::SaveEnumData(PreparedQueryResult result)
{
    Field* fields = result->Fetch();
    uint32 guid = fields[0].GetUInt32();
    std::string index = std::to_string(guid);
    EnumData[index.c_str()]["name"] = fields[1].GetString();
    EnumData[index.c_str()]["plrRace"] = fields[2].GetUInt8();
    EnumData[index.c_str()]["plrClass"] = fields[3].GetUInt8();
    EnumData[index.c_str()]["gender"] = fields[4].GetUInt8();
    EnumData[index.c_str()]["skin"] = uint8(fields[5].GetUInt32() & 0xFF);
    EnumData[index.c_str()]["face"] = uint8((fields[5].GetUInt32() >> 8) & 0xFF);
    EnumData[index.c_str()]["hairStyle"] = uint8((fields[5].GetUInt32() >> 16) & 0xFF);
    EnumData[index.c_str()]["hairColor"] = uint8((fields[5].GetUInt32() >> 24) & 0xFF);
    EnumData[index.c_str()]["facialHair"] = uint8(fields[6].GetUInt32() & 0xFF);
    EnumData[index.c_str()]["level"] = fields[7].GetUInt8();
    EnumData[index.c_str()]["zone"] = fields[8].GetUInt16();
    EnumData[index.c_str()]["mapId"] = fields[9].GetUInt16();
    EnumData[index.c_str()]["x"] = fields[10].GetFloat();
    EnumData[index.c_str()]["y"] = fields[11].GetFloat();
    EnumData[index.c_str()]["z"] = fields[12].GetFloat();
    EnumData[index.c_str()]["guildId"] = fields[13].GetUInt32();
    EnumData[index.c_str()]["playerFlags"] = fields[14].GetUInt32();
    EnumData[index.c_str()]["atLoginFlags"] = fields[15].GetUInt32();
    EnumData[index.c_str()]["petEntry"] = fields[16].GetUInt32();
    EnumData[index.c_str()]["petDisplayId"] = fields[17].GetUInt32();
    EnumData[index.c_str()]["petLevel"] = fields[18].GetUInt32();
    EnumData[index.c_str()]["equipment"] = fields[19].GetString();
    EnumData[index.c_str()]["charFlags"] = fields[20].GetUInt32();
    EnumData[index.c_str()]["slot"] = fields[21].GetUInt8();
    SaveEnum();
}

void WorldSession::UpdateEnumData(Player* player)
{
    std::string index = std::to_string(player->GetGUIDLow());
    EnumData[index.c_str()]["name"] = player->GetName();
    EnumData[index.c_str()]["plrRace"] = player->getRace();
    EnumData[index.c_str()]["plrClass"] = player->getClass();
    EnumData[index.c_str()]["gender"] = player->getGender();
    EnumData[index.c_str()]["skin"] = uint8(player->GetUInt32Value(PLAYER_BYTES) & 0xFF);
    EnumData[index.c_str()]["face"] = uint8((player->GetUInt32Value(PLAYER_BYTES) >> 8) & 0xFF);
    EnumData[index.c_str()]["hairStyle"] = uint8((player->GetUInt32Value(PLAYER_BYTES) >> 16) & 0xFF);
    EnumData[index.c_str()]["hairColor"] = uint8((player->GetUInt32Value(PLAYER_BYTES) >> 24) & 0xFF);
    EnumData[index.c_str()]["facialHair"] = uint8(player->GetUInt32Value(PLAYER_BYTES_2) & 0xFF);
    EnumData[index.c_str()]["level"] = player->getLevel();
    EnumData[index.c_str()]["zone"] = player->getCurrentUpdateZoneID();
    EnumData[index.c_str()]["mapId"] = player->GetMapId();
    EnumData[index.c_str()]["x"] = player->GetPositionX();
    EnumData[index.c_str()]["y"] = player->GetPositionY();
    EnumData[index.c_str()]["z"] = player->GetPositionZ();
    EnumData[index.c_str()]["guildId"] = player->GetGuildId();
    EnumData[index.c_str()]["playerFlags"] = player->GetUInt32Value(PLAYER_FLAGS);
    EnumData[index.c_str()]["atLoginFlags"] = player->GetAtLoginFlag();
    if (Pet* _pet = player->GetPet())
    {
        EnumData[index.c_str()]["petEntry"] = _pet->GetEntry();
        EnumData[index.c_str()]["petDisplayId"] = _pet->GetDisplayId();
        EnumData[index.c_str()]["petLevel"] = _pet->getLevel();
    }
    else
    {
        EnumData[index.c_str()]["petEntry"] = 0;
        EnumData[index.c_str()]["petDisplayId"] = 0;
        EnumData[index.c_str()]["petLevel"] = 0;
    }
    std::ostringstream ss;
    // cache equipment...
    for (uint32 i = 0; i < EQUIPMENT_SLOT_END * 2; ++i)
        ss << player->GetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + i) << ' ';
    EnumData[index.c_str()]["equipment"] = ss.str();
    SaveEnum();
}

void WorldSession::DeleteEnumData(uint32 guid)
{
    std::string index = std::to_string(guid);
    EnumData.removeMember(index.c_str());
    SaveEnum();
}
