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
        GetSession()->AccountDatas["criteria"].removeMember(achievID.c_str());
        RedisDatabase.AsyncExecuteH("HDEL", GetCriteriaAcKey(), achievID.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Guild::DeleteCriteriaProgress guid %u", guid);
        });
    }
    else
    {
        PlayerData["criteria"].removeMember(achievID.c_str());
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
        GetSession()->AccountDatas["criteria"][achievID.c_str()] = CriteriaAc;
        RedisDatabase.AsyncExecuteHSet("HSET", GetCriteriaAcKey(), achievID.c_str(), CriteriaAc, GetGUID(), [&](const RedisValue &v, uint64 guid) {
            //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerCriteria account guid %u", guid);
        });
    }
    else
    {
        PlayerData["criteria"][achievID.c_str()] = CriteriaPl;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaPlKey, achievID.c_str(), CriteriaPl, GetGUID(), [&](const RedisValue &v, uint64 guid) {
            //sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerCriteria player guid %u", guid);
        });
    }
}

void Player::RemoveMailFromRedis(Mail* m)
{
    std::string index = std::to_string(m->messageID);
    RedisDatabase.AsyncExecuteH("HDEL", mailKey, index.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailFromRedis id %u", guid);
    });

    PlayerMailData["mails"].removeMember(index.c_str());

    RedisDatabase.AsyncExecuteH("HDEL", sRedisBuilderMgr->GetMailsKey(), index.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {});

    //Del items
    for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
    {
        std::string guid = std::to_string(itr2->item_guid);
        RedisDatabase.AsyncExecuteH("HDEL", GetMailItemKey(), guid.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailFromRedis items id %u", guid);
        });
    }
}

void Player::RemoveMailItems(Mail* m)
{
    for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
        RemoveMailItem(itr2->item_guid, true);
}

void Player::RemoveMailItem(uint32 item_guid, bool isDelete)
{
    std::string guid = std::to_string(item_guid);
    if (isDelete)
    {
        RedisDatabase.AsyncExecuteH("HDEL", GetMailItemKey(), guid.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailFromRedis items id %u", guid);
        });
    }
    PlayerMailData["mitems"].removeMember(guid.c_str());
}

void Player::UpdatePlayerAccountData(AccountDataType type, time_t tm, std::string data)
{
    if ((1 << type) & GLOBAL_CACHE_MASK)
    {
        std::string index = std::to_string(type);
        GetSession()->AccountDatas["data"][index.c_str()]["Time"] = tm;
        GetSession()->AccountDatas["data"][index.c_str()]["Data"] = data.c_str();

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "data", GetSession()->AccountDatas["data"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerAccountData player guid %u", guid);
        });
    }
    else
    {
        std::string index = std::to_string(type);
        PlayerData["accountdata"][index.c_str()]["Time"] = tm;
        PlayerData["accountdata"][index.c_str()]["Data"] = data.c_str();

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "accountdata", PlayerData["accountdata"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerAccountData player guid %u", guid);
        });
    }
}

void Player::UpdateTutorials(uint8 index, uint32 value)
{
    std::string indexStr = std::to_string(index);
    GetSession()->AccountDatas["tutorials"][indexStr.c_str()] = value;

    RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "tutorials", GetSession()->AccountDatas["tutorials"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdateTutorials player guid %u", guid);
    });
}

void Player::UpdatePlayerPet(Pet* pet)
{
    if (!pet || !pet->GetCharmInfo())
        return;

    std::string id = std::to_string(pet->GetCharmInfo()->GetPetNumber());
    PlayerData["pets"][id.c_str()]["entry"] = pet->GetEntry();
    PlayerData["pets"][id.c_str()]["modelid"] = pet->GetNativeDisplayId();
    PlayerData["pets"][id.c_str()]["level"] = pet->getLevel();
    PlayerData["pets"][id.c_str()]["exp"] = pet->GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    PlayerData["pets"][id.c_str()]["Reactstate"] = pet->GetReactState();
    PlayerData["pets"][id.c_str()]["name"] = pet->GetName();
    PlayerData["pets"][id.c_str()]["renamed"] = pet->HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ? 0 : 1;
    PlayerData["pets"][id.c_str()]["curhealth"] = pet->GetHealth();
    PlayerData["pets"][id.c_str()]["curmana"] = pet->GetPower(POWER_MANA);

    std::ostringstream ss;
    ss.str("");
    for (uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
    {
        ss << uint32(pet->GetCharmInfo()->GetActionBarEntry(i)->GetType()) << ' '
            << uint32(pet->GetCharmInfo()->GetActionBarEntry(i)->GetAction()) << ' ';
    };
    PlayerData["pets"][id.c_str()]["abdata"] = ss.str();
    PlayerData["pets"][id.c_str()]["savetime"] = time(NULL);
    PlayerData["pets"][id.c_str()]["CreatedBySpell"] = pet->GetUInt32Value(UNIT_CREATED_BY_SPELL);
    PlayerData["pets"][id.c_str()]["PetType"] = pet->getPetType();
    PlayerData["pets"][id.c_str()]["specialization"] = pet->GetSpecializationId();

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "pets", PlayerData["pets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerPet player guid %u", guid);
    });
    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetPetKey(), id.c_str(), pet->PetDataSpell, GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerPet player guid %u", guid);
    });
}

void Player::RemovePlayerPet(Pet* pet)
{
    if (!pet || !pet->GetCharmInfo())
        return;

    std::string petId = std::to_string(pet->GetCharmInfo()->GetPetNumber());
    PlayerData["pets"].removeMember(petId.c_str());

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "pets", PlayerData["pets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemovePlayerPet player guid %u", guid);
    });
    RedisDatabase.AsyncExecuteH("HDEL", sRedisBuilderMgr->GetPetKey(), petId.c_str(), pet->GetCharmInfo()->GetPetNumber(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemovePlayerPet id %u", guid);
    });
}

void Player::DeleteFromRedis(uint64 playerguid, uint32 /*accountId*/)
{
    uint32 guid = GUID_LOPART(playerguid);
    char* itemKey = new char[23];
    char* userKey = new char[18];
    char* criteriaPlKey = new char[23];
    char* criteriaAcKey = new char[23];
    char* mailKey = new char[23];
    char* mailItemKey = new char[23];

    sprintf(itemKey, "r{%u}u{%u}items", realmID, guid);
    sprintf(userKey, "r{%u}u{%u}", realmID, guid);
    sprintf(criteriaPlKey, "r{%u}u{%u}crit", realmID, guid);
    sprintf(mailKey, "r{%u}u{%u}mails", realmID, guid);
    sprintf(mailItemKey, "r{%i}u{%i}mitems", realmID, guid);

    RedisDatabase.AsyncExecute("DEL", itemKey, playerguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::DeleteFromRedis item id %u", guid);
    });
    RedisDatabase.AsyncExecute("DEL", userKey, playerguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::DeleteFromRedis user id %u", guid);
    });
    RedisDatabase.AsyncExecute("DEL", criteriaPlKey, playerguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::DeleteFromRedis criteria id %u", guid);
    });
    RedisDatabase.AsyncExecute("DEL", mailKey, playerguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::DeleteFromRedis mail id %u", guid);
    });
    RedisDatabase.AsyncExecute("DEL", mailItemKey, playerguid, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::DeleteFromRedis mail item id %u", guid);
    });
}

void Player::UpdatePlayerEquipmentSet(uint32 id, EquipmentSet eqset, bool isRemove)
{
    std::string index = std::to_string(id);
    if (isRemove)
        PlayerData["equipmentsets"].removeMember(index.c_str());
    else
    {
        PlayerData["equipmentsets"][index.c_str()]["setguid"] = eqset.Guid;
        PlayerData["equipmentsets"][index.c_str()]["name"] = eqset.Name.c_str();
        PlayerData["equipmentsets"][index.c_str()]["iconname"] = eqset.IconName.c_str();
        PlayerData["equipmentsets"][index.c_str()]["ignore_mask"] = eqset.IgnoreMask;
        PlayerData["equipmentsets"][index.c_str()]["item0"] = eqset.Items[0];
        PlayerData["equipmentsets"][index.c_str()]["item1"] = eqset.Items[1];
        PlayerData["equipmentsets"][index.c_str()]["item2"] = eqset.Items[2];
        PlayerData["equipmentsets"][index.c_str()]["item3"] = eqset.Items[3];
        PlayerData["equipmentsets"][index.c_str()]["item4"] = eqset.Items[4];
        PlayerData["equipmentsets"][index.c_str()]["item5"] = eqset.Items[5];
        PlayerData["equipmentsets"][index.c_str()]["item6"] = eqset.Items[6];
        PlayerData["equipmentsets"][index.c_str()]["item7"] = eqset.Items[7];
        PlayerData["equipmentsets"][index.c_str()]["item8"] = eqset.Items[8];
        PlayerData["equipmentsets"][index.c_str()]["item9"] = eqset.Items[9];
        PlayerData["equipmentsets"][index.c_str()]["item10"] = eqset.Items[10];
        PlayerData["equipmentsets"][index.c_str()]["item11"] = eqset.Items[11];
        PlayerData["equipmentsets"][index.c_str()]["item12"] = eqset.Items[12];
        PlayerData["equipmentsets"][index.c_str()]["item13"] = eqset.Items[13];
        PlayerData["equipmentsets"][index.c_str()]["item14"] = eqset.Items[14];
        PlayerData["equipmentsets"][index.c_str()]["item15"] = eqset.Items[15];
        PlayerData["equipmentsets"][index.c_str()]["item16"] = eqset.Items[16];
        PlayerData["equipmentsets"][index.c_str()]["item17"] = eqset.Items[17];
        PlayerData["equipmentsets"][index.c_str()]["item18"] = eqset.Items[18];
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "equipmentsets", PlayerData["equipmentsets"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerEquipmentSets player guid %u", guid);
    });
}

void Player::DeletePetitions()
{
    PlayerData["petitions"].clear();

    RedisDatabase.AsyncExecuteH("HDEL", userKey, "petitions", GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::DeletePetitions guid %u", guid);
    });
}

void Player::UpdatePlayerVoidStorage(uint8 slot)
{
    std::string index = std::to_string(slot);
    if (_voidStorageItems[slot])
    {
        PlayerData["voidstorage"][index.c_str()]["itemId"] = _voidStorageItems[slot]->ItemId;
        PlayerData["voidstorage"][index.c_str()]["itemEntry"] = _voidStorageItems[slot]->ItemEntry;
        PlayerData["voidstorage"][index.c_str()]["creatorGuid"] = _voidStorageItems[slot]->CreatorGuid;
        PlayerData["voidstorage"][index.c_str()]["randomProperty"] = _voidStorageItems[slot]->ItemRandomPropertyId;
        PlayerData["voidstorage"][index.c_str()]["suffixFactor"] = _voidStorageItems[slot]->ItemSuffixFactor;
    }
    else
        PlayerData["voidstorage"].removeMember(index.c_str());
}

void Player::SavePlayerVoidStorage()
{
    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "voidstorage", PlayerData["voidstorage"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::SavePlayerVoidStorage player guid %u", guid);
    });
}

void Player::UpdatePlayerSpell(PlayerSpell* pSpell, uint32 spellId, bool isDelete)
{
    std::string spell = std::to_string(spellId);
    if (isDelete)
    {
        if(pSpell->mount)
            GetSession()->AccountDatas["spells"].removeMember(spell.c_str());
        else
            PlayerData["spells"].removeMember(spell.c_str());
    }
    else
    {
        if(pSpell->mount)
            GetSession()->AccountDatas["mounts"][spell.c_str()]["active"] = pSpell->active;
        else
        {
            PlayerData["spells"][spell.c_str()]["active"] = pSpell->active;
            PlayerData["spells"][spell.c_str()]["disabled"] = pSpell->disabled;
        }
    }
}

void Player::UpdatePlayerLootCooldown(playerLootCooldown* lootCooldown, uint32 entry, uint8 type, bool isDelete)
{
    std::string typeS = std::to_string(type);
    std::string entryS = std::to_string(entry);
    if (isDelete)
        PlayerData["lootcooldown"][typeS.c_str()].removeMember(entryS.c_str());
    else
    {
        PlayerData["lootcooldown"][typeS.c_str()][entryS.c_str()]["type"] = lootCooldown->type;
        PlayerData["lootcooldown"][typeS.c_str()][entryS.c_str()]["difficultyMask"] = lootCooldown->difficultyMask;
        PlayerData["lootcooldown"][typeS.c_str()][entryS.c_str()]["respawnTime"] = lootCooldown->respawnTime;
    }
}

void Player::UpdatePlayerCurrency(PlayerCurrency* currency, uint32 id)
{
    std::string currencyID = std::to_string(id);
    PlayerData["currency"][currencyID.c_str()]["weekCount"] = currency->weekCount;
    PlayerData["currency"][currencyID.c_str()]["totalCount"] = currency->totalCount;
    PlayerData["currency"][currencyID.c_str()]["seasonTotal"] = currency->seasonTotal;
    PlayerData["currency"][currencyID.c_str()]["flags"] = currency->flags;
    PlayerData["currency"][currencyID.c_str()]["curentCap"] = currency->curentCap;

    SavePlayerCurrency();
}

void Player::RemoveInstance(uint32 instance)
{
    std::string instanceID = std::to_string(instance);
    PlayerData["boundinstances"].removeMember(instanceID.c_str());
}

void Player::UpdatePlayerSkill(uint32 skillId, uint16 value, uint16 max, uint16 pos, bool isDelete)
{
    std::string skill = std::to_string(skillId);
    if (isDelete)
        PlayerData["skills"].removeMember(skill.c_str());
    else
    {
        PlayerData["skills"][skill.c_str()]["value"] = value;
        PlayerData["skills"][skill.c_str()]["max"] = max;
        PlayerData["skills"][skill.c_str()]["pos"] = pos;
    }
}

void Player::UpdatePlayerTalent(uint32 spellId, uint8 specId, bool deleteTal, bool deleteSpec)
{
    std::string spell = std::to_string(spellId);
    std::string spec = std::to_string(specId);
    if (deleteTal)
        PlayerData["talents"][spec.c_str()].removeMember(spell.c_str());
    else if (deleteSpec)
        PlayerData["talents"].removeMember(spec.c_str());
    else
        PlayerData["talents"][spec.c_str()][spell.c_str()] = specId;
}

void Player::UpdatePlayerGlyph(uint8 slot, uint32 glyph)
{
    std::string index = std::to_string(slot);
    std::string spec = std::to_string(GetActiveSpec());
    PlayerData["glyphs"][spec.c_str()][index.c_str()] = glyph;
}

void Player::UpdatePlayerQuestStatus(uint32 questId, QuestStatusData* q_status, bool isDelete)
{
    std::string quest_id = std::to_string(questId);
    if(isDelete)
    {
        if (q_status->account)
        {
            GetSession()->AccountDatas["queststatus"].removeMember(quest_id.c_str());

            RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "queststatus", GetSession()->AccountDatas["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestStatus account guid %u", guid);
            });
        }
        else
        {
            PlayerData["queststatus"].removeMember(quest_id.c_str());

            RedisDatabase.AsyncExecuteHSet("HSET", userKey, "queststatus", PlayerData["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestStatus player guid %u", guid);
            });
        }
        return;
    }

    if (q_status->account)
    {
        GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["status"] = int32(q_status->Status);
        GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["explored"] = q_status->Explored;
        GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["timer"] = q_status->Timer;
        GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["playercount"] = q_status->PlayerCount;
        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
        {
            std::string index = std::to_string(i);
            GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["mobcount"][index.c_str()] = q_status->CreatureOrGOCount[i];
            GetSession()->AccountDatas["queststatus"][quest_id.c_str()]["itemcount"][index.c_str()] = q_status->ItemCount[i];
        }

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "queststatus", GetSession()->AccountDatas["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestStatus account guid %u", guid);
        });
    }
    else
    {
        PlayerData["queststatus"][quest_id.c_str()]["status"] = int32(q_status->Status);
        PlayerData["queststatus"][quest_id.c_str()]["explored"] = q_status->Explored;
        PlayerData["queststatus"][quest_id.c_str()]["timer"] = q_status->Timer;
        PlayerData["queststatus"][quest_id.c_str()]["playercount"] = q_status->PlayerCount;
        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
        {
            std::string index = std::to_string(i);
            PlayerData["queststatus"][quest_id.c_str()]["mobcount"][index.c_str()] = q_status->CreatureOrGOCount[i];
            PlayerData["queststatus"][quest_id.c_str()]["itemcount"][index.c_str()] = q_status->ItemCount[i];
        }

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "queststatus", PlayerData["queststatus"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestStatus player guid %u", guid);
        });
    }
}

void Player::UpdatePlayerQuestRewarded(uint32 questId, bool isDelete)
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;
    std::string quest_id = std::to_string(questId);
    if (quest->GetType() == QUEST_TYPE_ACCOUNT)
    {
        if (isDelete)
            GetSession()->AccountDatas["questrewarded"].removeMember(quest_id.c_str());
        else
            GetSession()->AccountDatas["questrewarded"][quest_id.c_str()] = quest_id;

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questrewarded", GetSession()->AccountDatas["questrewarded"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestRewarded account guid %u", guid);
        });
    }
    else
    {
        if (isDelete)
            PlayerData["questrewarded"].removeMember(quest_id.c_str());
        else
            PlayerData["questrewarded"][quest_id.c_str()] = quest_id;

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questrewarded", PlayerData["questrewarded"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestRewarded player guid %u", guid);
        });
    }
}

void Player::UpdatePlayerQuestDaily(uint32 questId, bool isClear)
{
    if (isClear)
    {
        GetSession()->AccountDatas["questdaily"].clear();
        PlayerData["questdaily"].clear();

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questdaily", GetSession()->AccountDatas["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestDaily account guid %u", guid);
        });

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questdaily", PlayerData["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestDaily player guid %u", guid);
        });
        return;
    }

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;
    std::string quest_id = std::to_string(questId);
    if (quest->GetType() == QUEST_TYPE_ACCOUNT)
    {
        GetSession()->AccountDatas["questdaily"][quest_id.c_str()] = uint64(m_lastDailyQuestTime);

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questdaily", GetSession()->AccountDatas["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestDaily account guid %u", guid);
        });
    }
    else
    {
        PlayerData["questdaily"][quest_id.c_str()] = uint64(m_lastDailyQuestTime);

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questdaily", PlayerData["questdaily"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestDaily player guid %u", guid);
        });
    }
}

void Player::UpdatePlayerQuestWeekly(uint32 questId, bool isClear)
{
    if (isClear)
    {
        GetSession()->AccountDatas["questweekly"].clear();
        PlayerData["questweekly"].clear();

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questweekly", GetSession()->AccountDatas["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestWeekly account guid %u", guid);
        });

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questweekly", PlayerData["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestWeekly player guid %u", guid);
        });
        return;
    }

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;
    std::string quest_id = std::to_string(questId);
    if (quest->GetType() == QUEST_TYPE_ACCOUNT)
    {
        GetSession()->AccountDatas["questweekly"][quest_id.c_str()] = questId;

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questweekly", GetSession()->AccountDatas["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestWeekly account guid %u", guid);
        });
    }
    else
    {
        PlayerData["questweekly"][quest_id.c_str()] = questId;

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questweekly", PlayerData["questweekly"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestWeekly player guid %u", guid);
        });
    }
}

void Player::UpdatePlayerQuestSeasonal(uint32 questId, uint32 eventId, bool isRemove)
{
    std::string event = std::to_string(eventId);

    if (isRemove)
    {
        PlayerData["questseasonal"].removeMember(event.c_str());
        GetSession()->AccountDatas["questseasonal"].removeMember(event.c_str());

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questseasonal", GetSession()->AccountDatas["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestSeasonal account guid %u", guid);
        });

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questseasonal", PlayerData["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestSeasonal player guid %u", guid);
        });
        return;
    }

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;
    std::string quest_id = std::to_string(eventId);
    if (quest->GetType() == QUEST_TYPE_ACCOUNT)
    {
        GetSession()->AccountDatas["questseasonal"][event.c_str()][quest_id.c_str()] = questId;

        RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "questseasonal", GetSession()->AccountDatas["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestSeasonal account guid %u", guid);
        });
    }
    else
    {
        PlayerData["questseasonal"][event.c_str()][quest_id.c_str()] = questId;

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "questseasonal", PlayerData["questseasonal"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerQuestSeasonal player guid %u", guid);
        });
    }
}

void Player::UpdatePlayerBattlePets(PetJournalInfo* pjInfo, uint64 guid, bool isDelete)
{
    if (!pjInfo)
        return;
    std::string petGuid = std::to_string(guid);
    if (isDelete)
    {
        GetSession()->AccountDatas["battlepets"].removeMember(petGuid.c_str());
        return;
    }

    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["customName"] = pjInfo->GetCustomName();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["creatureEntry"] = pjInfo->GetCreatureEntry();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["speciesID"] = pjInfo->GetSpeciesID();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["spell"] = pjInfo->GetSummonSpell();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["level"] = pjInfo->GetLevel();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["displayID"] = pjInfo->GetDisplayID();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["power"] = pjInfo->GetPower();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["speed"] = pjInfo->GetSpeed();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["health"] = pjInfo->GetHealth();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["maxHealth"] = pjInfo->GetMaxHealth();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["quality"] = pjInfo->GetQuality();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["xp"] = pjInfo->GetXP();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["flags"] = pjInfo->GetFlags();
    GetSession()->AccountDatas["battlepets"][petGuid.c_str()]["breedID"] = pjInfo->GetBreedID();
}

void Player::UpdatePlayerReputation(FactionState* faction)
{
    std::string factionId = std::to_string(faction->ID);
    PlayerData["reputation"][factionId.c_str()]["standing"] = faction->Standing;
    PlayerData["reputation"][factionId.c_str()]["flags"] = faction->Flags;

    SavePlayerReputation();
}

void Player::UpdatePlayerAction(uint8 buttonId, uint32 action, uint8 type, uint8 specId, bool isRemove)
{
    std::string button = std::to_string(buttonId);
    std::string spec = std::to_string(specId);
    if(isRemove)
        PlayerData["actions"][spec.c_str()].removeMember(button.c_str());
    else
    {
        PlayerData["actions"][spec.c_str()][button.c_str()]["action"] = action;
        PlayerData["actions"][spec.c_str()][button.c_str()]["type"] = type;
    }

    SavePlayerActions();
}

void Player::UpdatePlayerSocial(uint32 friendGuid, std::string note, uint8 flag, bool isNote, bool isRemove)
{
    std::string friend_guid = std::to_string(friendGuid);
    if (isRemove)
        PlayerData["social"].removeMember(friend_guid.c_str());
    else
    {
        if (isNote)
            PlayerData["social"][friend_guid.c_str()]["Note"] = note.c_str();
        else
            PlayerData["social"][friend_guid.c_str()]["Flags"] = flag;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "social", PlayerData["social"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerSocial player guid %u", guid);
    });
}

void Player::UpdatePlayerSpellCooldown(uint32 spellId, uint32 itemId, double time, bool isRemove, bool isClear)
{
    std::string spell = std::to_string(spellId);
    if (isClear)
        PlayerData["spellcooldowns"].clear();
    else if (isRemove)
        PlayerData["spellcooldowns"].removeMember(spell.c_str());
    else
    {
        PlayerData["spellcooldowns"][spell.c_str()]["item"] = itemId;
        PlayerData["spellcooldowns"][spell.c_str()]["time"] = uint64(time);
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "spellcooldowns", PlayerData["spellcooldowns"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerSpellCooldown player guid %u", guid);
    });
}

void Player::UpdatePlayerKill(uint32 victimGuid, uint32 count, bool isClear)
{
    if (isClear)
        PlayerData["kills"].clear();
    else
    {
        std::string victim = std::to_string(victimGuid);
        PlayerData["kills"][victim.c_str()] = count;
    }

    RedisDatabase.AsyncExecuteHSet("HSET", userKey, "kills", PlayerData["kills"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::UpdatePlayerKill player guid %u", guid);
    });
}

void Player::UpdateAchievement(AchievementEntry const* achievement, CompletedAchievementData* ca)
{
    std::string achievementId = std::to_string(achievement->ID);
    if (ca->first_guid_real != ca->first_guid || ca->first_guid_real == GetGUIDLow())
    {
        PlayerData["achievement"][achievementId.c_str()]["date"] = ca->date;
        PlayerData["achievement"][achievementId.c_str()]["changed"] = ca->changed;

        RedisDatabase.AsyncExecuteHSet("HSET", userKey, "achievement", PlayerData["achievement"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::InitAchievement player guid %u", guid);
        });
    }

    GetSession()->AccountDatas["achievement"][achievementId.c_str()]["first_guid"] = ca->first_guid_real;
    GetSession()->AccountDatas["achievement"][achievementId.c_str()]["date"] = ca->date;
    GetSession()->AccountDatas["achievement"][achievementId.c_str()]["changed"] = ca->changed;

    RedisDatabase.AsyncExecuteHSet("HSET", GetAccountKey(), "achievement", GetSession()->AccountDatas["achievement"], GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::InitAchievement account guid %u", guid);
    });
}

void Player::UpdatePlayerMail(Mail* m)
{
    std::string message = std::to_string(m->messageID);
    m->MailJson["messageType"] = m->messageType;
    m->MailJson["sender"] = m->sender;
    m->MailJson["receiver"] = m->receiver;
    m->MailJson["subject"] = m->subject;
    m->MailJson["body"] = m->body;
    m->MailJson["has_items"] = m->HasItems();
    m->MailJson["expire_time"] = m->expire_time;
    m->MailJson["deliver_time"] = m->deliver_time;
    m->MailJson["money"] = m->money;
    m->MailJson["COD"] = m->COD;
    m->MailJson["checked"] = m->checked;
    m->MailJson["stationery"] = m->stationery;
    m->MailJson["mailTemplateId"] = m->mailTemplateId;

    PlayerMailData["mails"][message.c_str()] = m->MailJson;
    PlayerMailData["mails"][message.c_str()]["items"] = m->MailItemJson;

    RedisDatabase.AsyncExecuteHSet("HSET", mailKey, message.c_str(), m->MailJson, GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::InitPlayerMails player guid %u", guid);
    });

    Json::Value expireStr = m->expire_time;
    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetMailsKey(), message.c_str(), expireStr, GetGUID(), [&](const RedisValue &v, uint64 guid) {});

}


