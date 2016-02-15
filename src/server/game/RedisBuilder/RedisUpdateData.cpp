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
#include "RedisLoadPlayer.h"
#include "AchievementMgr.h"
#include "RedisBuilderMgr.h"

void Player::UpdateSavePlayer()
{
    SavePlayer();
    SavePlayerAuras();
    sWorld->UpdateCharacterNameDataZoneGuild(GetGUIDLow(), m_zoneUpdateId, GetGuildId(), GetRank());
}

void Player::DeleteCriteriaProgress(AchievementEntry const* achievement)
{
    std::string achievID = std::to_string(achievement->ID);
    if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
    {
        AccountCriteriaJson.removeMember(achievID.c_str());
        RedisDatabase.AsyncExecuteH("HDEL", criteriaAcKey, achievID.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteCriteriaProgress guid %u", guid);
        });
    }
    else
    {
        PlayerCriteriaJson.removeMember(achievID.c_str());
        RedisDatabase.AsyncExecuteH("HDEL", criteriaPlKey, achievID.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteCriteriaProgress guid %u", guid);
        });
    }
}

void Player::UpdateCriteriaProgress(AchievementEntry const* achievement, CriteriaProgressMap* progressMap)
{
    std::string achievID = std::to_string(achievement->ID);
    Json::Value CriteriaPl;
    Json::Value CriteriaAc;

    for (auto iter = progressMap->begin(); iter != progressMap->end(); ++iter)
    {
        if (iter->second.deactiveted)
            continue;

        // store data only for real progress
        if (iter->second.counter != 0)
        {
            std::string criteriaId = std::to_string(iter->first);
            if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
            {
                CriteriaAc[criteriaId.c_str()]["counter"] = iter->second.counter;
                CriteriaAc[criteriaId.c_str()]["date"] = iter->second.date;
                CriteriaAc[criteriaId.c_str()]["completed"] = iter->second.completed;
            }
            else
            {
                CriteriaPl[criteriaId.c_str()]["counter"] = iter->second.counter;
                CriteriaPl[criteriaId.c_str()]["date"] = iter->second.date;
                CriteriaPl[criteriaId.c_str()]["completed"] = iter->second.completed;
            }
        }
    }

    if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
    {
        AccountCriteriaJson[achievID.c_str()] = CriteriaAc;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaAcKey, achievID.c_str(), sRedisBuilder->BuildString(CriteriaAc).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerCriteria account guid %u", guid);
        });
    }
    else
    {
        PlayerCriteriaJson[achievID.c_str()] = CriteriaPl;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaPlKey, achievID.c_str(), sRedisBuilder->BuildString(CriteriaPl).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerCriteria player guid %u", guid);
        });
    }
}

void Player::RemoveMailFromRedis(uint32 id)
{
    std::string index = std::to_string(id);
    RedisDatabase.AsyncExecuteH("HDEL", mailKey, index.c_str(), id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailFromRedis id %u", guid);
    });
}

void Player::RemoveMailItemsFromRedis(uint32 id)
{
    char* _key = new char[32];
    sprintf(_key, "r{%u}m{%u}items", realmID, id);

    RedisDatabase.AsyncExecute("DEL", _key, id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailItemsFromRedis id %u", guid);
    });
}

void Player::UpdatePlayerAccountData(AccountDataType type, time_t tm, std::string data)
{
    if ((1 << type) & GLOBAL_CACHE_MASK)
    {
        std::string index = std::to_string(type);
        AccountDataJson[index.c_str()]["Time"] = tm;
        AccountDataJson[index.c_str()]["Data"] = data.c_str();

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "accountdata", sRedisBuilder->BuildString(AccountDataJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerAccountData player guid %u", guid);
        });
    }
    else
    {
        std::string index = std::to_string(type);
        PlayerAccountDataJson[index.c_str()]["Time"] = tm;
        PlayerAccountDataJson[index.c_str()]["Data"] = data.c_str();

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "playeraccountdata", sRedisBuilder->BuildString(PlayerAccountDataJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerAccountData player guid %u", guid);
        });
    }
}

void Player::UpdateTutorials(uint8 index, uint32 value)
{
    std::string indexStr = std::to_string(index);
    AccountTutorialsJson[indexStr.c_str()] = value;

    RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "tutorials", sRedisBuilder->BuildString(AccountTutorialsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdateTutorials player guid %u", guid);
    });
}

void Player::UpdatePlayerPet(Pet* pet)
{
    if (!pet || !pet->GetCharmInfo())
        return;

    std::string id = std::to_string(pet->GetCharmInfo()->GetPetNumber());
    PlayerPetsJson[id.c_str()]["entry"] = pet->GetEntry();
    PlayerPetsJson[id.c_str()]["modelid"] = pet->GetNativeDisplayId();
    PlayerPetsJson[id.c_str()]["level"] = pet->getLevel();
    PlayerPetsJson[id.c_str()]["exp"] = pet->GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    PlayerPetsJson[id.c_str()]["Reactstate"] = pet->GetReactState();
    PlayerPetsJson[id.c_str()]["name"] = pet->GetName();
    PlayerPetsJson[id.c_str()]["renamed"] = pet->HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ? 0 : 1;
    PlayerPetsJson[id.c_str()]["curhealth"] = pet->GetHealth();
    PlayerPetsJson[id.c_str()]["curmana"] = pet->GetPower(POWER_MANA);

    std::ostringstream ss;
    ss.str("");
    for (uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
    {
        ss << uint32(pet->GetCharmInfo()->GetActionBarEntry(i)->GetType()) << ' '
            << uint32(pet->GetCharmInfo()->GetActionBarEntry(i)->GetAction()) << ' ';
    };
    PlayerPetsJson[id.c_str()]["abdata"] = ss.str();
    PlayerPetsJson[id.c_str()]["savetime"] = time(NULL);
    PlayerPetsJson[id.c_str()]["CreatedBySpell"] = pet->GetUInt32Value(UNIT_CREATED_BY_SPELL);
    PlayerPetsJson[id.c_str()]["PetType"] = pet->getPetType();
    PlayerPetsJson[id.c_str()]["specialization"] = pet->GetSpecializationId();

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "pets", sRedisBuilder->BuildString(PlayerPetsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerPet player guid %u", guid);
    });
    RedisDatabase.AsyncExecuteHSet("HSET", sObjectMgr->GetPetKey(), id.c_str(), sRedisBuilder->BuildString(pet->PetDataSpell).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerPet player guid %u", guid);
    });
}

void Player::RemovePlayerPet(Pet* pet)
{
    if (!pet || !pet->GetCharmInfo())
        return;

    std::string petId = std::to_string(pet->GetCharmInfo()->GetPetNumber());
    PlayerPetsJson.removeMember(petId.c_str());

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "pets", sRedisBuilder->BuildString(PlayerPetsJson).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemovePlayerPet player guid %u", guid);
    });
    RedisDatabase.AsyncExecuteH("HDEL", sObjectMgr->GetPetKey(), petId.c_str(), pet->GetCharmInfo()->GetPetNumber(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemovePlayerPet id %u", guid);
    });
}
