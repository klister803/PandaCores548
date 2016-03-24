/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "RedisBuilderMgr.h"
#include <time.h>

RedisBuilderMgr::RedisBuilderMgr()
{
}

RedisBuilderMgr::~RedisBuilderMgr()
{
}

std::string RedisBuilderMgr::BuildString(Json::Value& data)
{
    return Builder.write(data);
}

bool RedisBuilderMgr::LoadFromRedis(const RedisValue* v, Json::Value& data)
{
    if (!v->isOk() || v->isNull() || !v->isString())
    {
        //sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::LoadFromRedis data is empty");
        return false;
    }

    bool isReader = Reader.parse(v->toString().c_str(), data);
    if (!isReader)
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::LoadFromRedis parce json false");
        return false;
    }

    return true;
}

bool RedisBuilderMgr::LoadFromString(std::string string_data, Json::Value& data)
{
    if (string_data.empty())
    {
        //sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::LoadFromString data is empty");
        return false;
    }

    bool isReader = Reader.parse(string_data.c_str(), data);
    if (!isReader)
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::LoadFromString parce json false");
        return false;
    }

    return true;
}

bool RedisBuilderMgr::LoadFromRedisArray(const RedisValue* v, std::vector<RedisValue>& data)
{
    if (!v->isOk() || v->isNull())
    {
        //sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::LoadFromRedisArray data is empty");
        return false;
    }

    if (!v->isArray())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::LoadFromRedisArray is not array");
        return false;
    }

    data = v->toArray();

    return true;
}

bool RedisBuilderMgr::CheckKey(const char* _key)
{
    RedisValue v = RedisDatabase.Execute("EXISTS", _key);
    if (!v.isOk() || v.isNull())
    {
        //sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::CheckKey data not found %s", _key);
        return false;
    }

    if (!v.toInt())
    {
        sLog->outInfo(LOG_FILTER_REDIS, "RedisBuilderMgr::CheckKey key not exist %s", _key);
        return false;
    }

    return true;
}

void RedisBuilderMgr::InitRedisKey()
{
    queryGuidKey = "r{" + std::to_string(realmID) + "}HIGHESTGUIDS";
    petKey = "r{" + std::to_string(realmID) + "}pets";
    bracketKey = "r{" + std::to_string(realmID) + "}bracket";
    aucItemKey = "r{" + std::to_string(realmID) + "}auc{0}items";
    aucKey = "r{" + std::to_string(realmID) + "}auc";
    guildKey = "r{" + std::to_string(realmID) + "}guild";
    guildFKey = "r{" + std::to_string(realmID) + "}finder";
    guildFMKey = "r{" + std::to_string(realmID) + "}findermember";
    groupKey = "r{" + std::to_string(realmID) + "}group";
    groupMemberKey = "r{" + std::to_string(realmID) + "}groupmember";
    groupInstanceKey = "r{" + std::to_string(realmID) + "}instance";
    challengeKey = "r{" + std::to_string(realmID) + "}challenge";
    ticketKey = "r{" + std::to_string(realmID) + "}ticket";
    mailsKey = "r{" + std::to_string(realmID) + "}mails";
}

void PlayerSave::SaveToDB()
{
    uint32 oldMSTime = getMSTime();

    SaveItem();

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    std::string guid = std::to_string(m_player->GetGUIDLow());
    uint8 index = 0;
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHARACTER_JSON);
    stmt->setUInt32(index++, m_player->GetGUIDLow());
    stmt->setUInt32(index++, m_player->GetSession()->GetAccountId());
    stmt->setString(index++, m_player->GetName());
    stmt->setUInt8(index++, m_player->GetSession()->AccountDatas["enumdata"][guid.c_str()]["slot"].asInt());
    stmt->setUInt8(index++, m_player->getRace());
    stmt->setUInt8(index++, m_player->getClass());
    stmt->setUInt8(index++, m_player->getGender());
    stmt->setUInt8(index++, m_player->getLevel());
    stmt->setUInt32(index++, m_player->GetUInt32Value(PLAYER_XP));
    stmt->setUInt64(index++, m_player->GetMoney());
    stmt->setUInt32(index++, m_player->GetUInt32Value(PLAYER_BYTES));
    stmt->setUInt32(index++, m_player->GetUInt32Value(PLAYER_BYTES_2));
    stmt->setUInt32(index++, m_player->GetUInt32Value(PLAYER_FLAGS));
    stmt->setUInt16(index++, (uint16)m_player->GetMapId());
    stmt->setFloat(index++, finiteAlways(m_player->GetPositionX()));
    stmt->setFloat(index++, finiteAlways(m_player->GetPositionY()));
    stmt->setFloat(index++, finiteAlways(m_player->GetPositionZ()));
    stmt->setUInt32(index++, m_player->m_Played_time[PLAYED_TIME_TOTAL]);
    stmt->setUInt32(index++, m_player->m_Played_time[PLAYED_TIME_LEVEL]);
    stmt->setUInt32(index++, uint32(time(NULL)));
    stmt->setUInt16(index++, (uint16)m_player->GetAtLoginFlag());
    stmt->setUInt16(index++, m_player->getCurrentUpdateZoneID());
    stmt->setUInt32(index++, m_player->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS));
    std::ostringstream ss;
    // cache equipment...
    for (uint32 i = 0; i < EQUIPMENT_SLOT_END * 2; ++i)
        ss << m_player->GetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + i) << ' ';

    // ...and bags for enum opcode
    for (uint32 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Item* item = m_player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            ss << item->GetEntry();
        else
            ss << '0';
        ss << " 0 ";
    }
    stmt->setString(index++, ss.str());
    stmt->setString(index++, sRedisBuilderMgr->BuildString(m_player->PlayerData));
    stmt->setString(index++, sRedisBuilderMgr->BuildString(m_player->PlayerMailData));
    trans->Append(stmt);

    index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_ACCOUNT_JSON);
    stmt->setUInt32(index++, m_player->GetSession()->GetAccountId());
    stmt->setString(index++, sRedisBuilderMgr->BuildString(m_player->GetSession()->AccountDatas));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    sLog->outInfo(LOG_FILTER_REDIS, "PlayerSave::SaveToDB %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void PlayerSave::SaveItem()
{
    for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        if (Item* pItem = m_player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            std::string guid = std::to_string(pItem->GetGUIDLow());
            m_player->PlayerData["items"][guid.c_str()] = pItem->ItemData;
        }
    }

    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = m_player->GetBagByPos(i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item* pItem = m_player->GetItemByPos(i, j))
                {
                    std::string guid = std::to_string(pItem->GetGUIDLow());
                    m_player->PlayerData["items"][guid.c_str()] = pItem->ItemData;
                }
            }
        }
    }

    for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item* pItem = m_player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            std::string guid = std::to_string(pItem->GetGUIDLow());
            m_player->PlayerData["items"][guid.c_str()] = pItem->ItemData;
        }

    // checking every item from 39 to 74 (including bank bags)
    for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_BAG_END; ++i)
        if (Item* pItem = m_player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            std::string guid = std::to_string(pItem->GetGUIDLow());
            m_player->PlayerData["items"][guid.c_str()] = pItem->ItemData;
        }

    for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        if (Bag* pBag = m_player->GetBagByPos(i))
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                if (Item* pItem = pBag->GetItemByPos(j))
                {
                    std::string guid = std::to_string(pItem->GetGUIDLow());
                    m_player->PlayerData["items"][guid.c_str()] = pItem->ItemData;
                }
}
