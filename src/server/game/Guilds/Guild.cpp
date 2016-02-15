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

#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GuildFinderMgr.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Config.h"
#include "SocialMgr.h"
#include "Log.h"
#include "AccountMgr.h"

#define MAX_GUILD_BANK_TAB_TEXT_LEN 500
#define EMBLEM_PRICE 10 * GOLD

inline uint32 _GetGuildBankTabPrice(uint8 tabId)
{
    switch (tabId)
    {
        case 0: return 100;
        case 1: return 250;
        case 2: return 500;
        case 3: return 1000;
        case 4: return 2500;
        case 5: return 5000;
        default: return 0;
    }
}

void Guild::SendCommandResult(WorldSession* session, GuildCommandType type, GuildCommandError errCode, const std::string& param)
{
    WorldPacket data(SMSG_GUILD_COMMAND_RESULT, 8 + 1);
    data.WriteBits(param.size(), 8);
    data << uint32(errCode);
    data << uint32(type);
    data.WriteString(param);
    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_COMMAND_RESULT)");
}

void Guild::SendSaveEmblemResult(WorldSession* session, GuildEmblemError errCode)
{
    WorldPacket data(SMSG_SAVE_GUILD_EMBLEM, 4);
    data << uint32(errCode);
    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_SAVE_GUILD_EMBLEM)");
}

// LogHolder
Guild::LogHolder::~LogHolder()
{
    // Cleanup
    for (GuildLog::iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
        delete (*itr);
}

// Adds event loaded from database to collection
inline void Guild::LogHolder::LoadEvent(LogEntry* entry)
{
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
        m_nextGUID = entry->GetGUID();
    m_log.push_front(entry);
}

// Adds new event happened in game.
// If maximum number of events is reached, oldest event is removed from collection.
inline void Guild::LogHolder::AddEvent(Guild* guild, LogEntry* entry)
{
    // Check max records limit
    if (m_log.size() >= m_maxRecords)
    {
        LogEntry* oldEntry = m_log.front();
        delete oldEntry;
        m_log.pop_front();
    }
    guild->UpdateGuildEventLog(entry);
    // Add event to list
    m_log.push_back(entry);
}

inline void Guild::LogHolder::AddEventBank(Guild* guild, LogEntry* entry)
{
    // Check max records limit
    if (m_log.size() >= m_maxRecords)
    {
        LogEntry* oldEntry = m_log.front();
        delete oldEntry;
        m_log.pop_front();
    }
    guild->UpdateGuildBankEventLog(entry);
    // Add event to list
    m_log.push_back(entry);
}

// Writes information about all events into packet.
inline void Guild::LogHolder::WritePacket(WorldPacket& data) const
{
    ByteBuffer buffer;
    data.WriteBits(m_log.size(), 21);
    for (GuildLog::const_iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
        (*itr)->WritePacket(data, buffer);

    data.FlushBits();
    data.append(buffer);
}

inline uint32 Guild::LogHolder::GetNextGUID()
{
    // Next guid was not initialized. It means there are no records for this holder in DB yet.
    // Start from the beginning.
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
        m_nextGUID = 0;
    else
        m_nextGUID = (m_nextGUID + 1) % m_maxRecords;
    return m_nextGUID;
}

///////////////////////////////////////////////////////////////////////////////
// EventLogEntry
void Guild::EventLogEntry::WritePacket(WorldPacket& data, ByteBuffer& content) const
{
    ObjectGuid guid1 = MAKE_NEW_GUID(m_playerGuid1, 0, HIGHGUID_PLAYER);
    ObjectGuid guid2 = MAKE_NEW_GUID(m_playerGuid2, 0, HIGHGUID_PLAYER);

    data.WriteGuidMask<2>(guid1);
    data.WriteGuidMask<4>(guid2);
    data.WriteGuidMask<7, 1>(guid1);
    data.WriteGuidMask<0, 6, 3>(guid2);
    data.WriteGuidMask<3, 5, 0, 4>(guid1);
    data.WriteGuidMask<7>(guid2);
    data.WriteGuidMask<6>(guid1);
    data.WriteGuidMask<2, 1, 5>(guid2);

    content.WriteGuidBytes<3>(guid2);
    content.WriteGuidBytes<3>(guid1);
    content.WriteGuidBytes<2, 1, 3>(guid2);
    content.WriteGuidBytes<1>(guid1);
    // Event type
    content << uint8(m_eventType);
    content.WriteGuidBytes<0, 7>(guid2);
    content.WriteGuidBytes<7, 4>(guid1);
    content.WriteGuidBytes<5>(guid2);
    content.WriteGuidBytes<5, 0>(guid1);
    // New Rank
    content << uint8(m_newRank);
    // Event timestamp
    content << uint32(::time(NULL) - m_timestamp);
    content.WriteGuidBytes<2, 6>(guid1);
}

///////////////////////////////////////////////////////////////////////////////
// BankEventLogEntry
void Guild::BankEventLogEntry::WritePacket(WorldPacket& data, ByteBuffer& content) const
{
    ObjectGuid logGuid = MAKE_NEW_GUID(m_playerGuid, 0, HIGHGUID_PLAYER);

    bool hasItem = m_eventType == GUILD_BANK_LOG_DEPOSIT_ITEM || m_eventType == GUILD_BANK_LOG_WITHDRAW_ITEM ||
                   m_eventType == GUILD_BANK_LOG_MOVE_ITEM || m_eventType == GUILD_BANK_LOG_MOVE_ITEM2;
    bool itemMoved = (m_eventType == GUILD_BANK_LOG_MOVE_ITEM || m_eventType == GUILD_BANK_LOG_MOVE_ITEM2);
    bool hasStack = (hasItem && m_itemStackCount > 1);

    data.WriteBit(hasItem);
    data.WriteGuidMask<3, 7, 0>(logGuid);
    data.WriteBit(IsMoneyEvent());
    data.WriteBit(itemMoved);
    data.WriteGuidMask<2>(logGuid);
    data.WriteBit(hasStack);
    data.WriteGuidMask<6, 1, 5, 4>(logGuid);

    content.WriteGuidBytes<6, 3, 4>(logGuid);
    if (hasStack)
        content << uint32(m_itemStackCount);
    content.WriteGuidBytes<0, 2>(logGuid);
    content << uint8(m_eventType);
    if (hasItem)
        content << uint32(m_itemOrMoney);
    content << uint32(time(NULL) - m_timestamp);
    if (itemMoved)
        content << uint8(m_destTabId);
    content.WriteGuidBytes<1, 5>(logGuid);
    if (IsMoneyEvent())
        content << uint64(m_itemOrMoney);
    content.WriteGuidBytes<7>(logGuid);
}

///////////////////////////////////////////////////////////////////////////////
// RankInfo
void Guild::RankInfo::LoadFromDB(Field* fields)
{
    m_rankId            = fields[1].GetUInt8();
    m_name              = fields[2].GetString();
    m_rights            = fields[3].GetUInt32();
    m_bankMoneyPerDay   = fields[4].GetUInt32();
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        m_rights |= GR_RIGHT_ALL;
}

void Guild::RankInfo::CreateMissingTabsIfNeeded(uint8 tabs, bool logOnCreate /* = false */)
{
    for (uint8 i = 0; i < tabs; ++i)
    {
        GuildBankRightsAndSlots& rightsAndSlots = m_bankTabRightsAndSlots[i];
        if (rightsAndSlots.GetTabId() == i)
            continue;

        rightsAndSlots.SetTabId(i);
        if (m_rankId == GR_GUILDMASTER)
            rightsAndSlots.SetGuildMasterValues();

        if (logOnCreate)
            sLog->outError(LOG_FILTER_GUILD, "Guild %u has broken Tab %u for rank %u. Created default tab.", m_guildId, i, m_rankId);
    }
}

void Guild::RankInfo::SetName(const std::string& name)
{
    if (m_name == name)
        return;

    m_name = name;
}

void Guild::RankInfo::SetRights(uint32 rights)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        rights = GR_RIGHT_ALL;

    if (m_rights == rights)
        return;

    m_rights = rights;
}

void Guild::RankInfo::SetBankMoneyPerDay(uint32 money)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        money = uint32(GUILD_WITHDRAW_MONEY_UNLIMITED);

    if (m_bankMoneyPerDay == money)
        return;

    m_bankMoneyPerDay = money;
}

void Guild::RankInfo::SetBankTabSlotsAndRights(GuildBankRightsAndSlots rightsAndSlots)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        rightsAndSlots.SetGuildMasterValues();

    GuildBankRightsAndSlots& guildBR = m_bankTabRightsAndSlots[rightsAndSlots.GetTabId()];
    guildBR = rightsAndSlots;
}

///////////////////////////////////////////////////////////////////////////////
// BankTab
bool Guild::BankTab::LoadFromDB(Field* fields)
{
    m_name = fields[2].GetString();
    m_icon = fields[3].GetString();
    m_text = fields[4].GetString();
    return true;
}

bool Guild::BankTab::LoadItemFromDB(Field* fields)
{
    uint8 slotId = fields[16].GetUInt8();
    uint32 itemGuid = fields[17].GetUInt32();
    uint32 itemEntry = fields[18].GetUInt32();
    if (slotId >= GUILD_BANK_MAX_SLOTS)
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid slot for item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
    if (!proto)
    {
        sLog->outError(LOG_FILTER_GUILD, "Unknown item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    Item* pItem = NewItemOrBag(proto);
    pItem->SetItemKey(ITEM_KEY_GUILD, m_guildId);

    if (!pItem->LoadFromDB(itemGuid, 0, fields, itemEntry))
    {
        sLog->outError(LOG_FILTER_GUILD, "Item (GUID %u, id: %u) not found in item_instance, deleting from guild bank!", itemGuid, itemEntry);

        PreparedStatement *stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_NONEXISTENT_GUILD_BANK_ITEM);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_tabId);
        stmt->setUInt8 (2, slotId);
        CharacterDatabase.Execute(stmt);

        delete pItem;
        return false;
    }

    pItem->SetTabId(m_tabId);
    pItem->SetSlot(slotId);
    pItem->SaveItem();

    pItem->AddToWorld();
    m_items[slotId] = pItem;
    return true;
}

// Deletes contents of the tab from the world (and from DB if necessary)
void Guild::BankTab::Delete(bool removeItemsFromDB)
{
    for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
        if (Item* pItem = m_items[slotId])
        {
            pItem->RemoveFromWorld();
            if (removeItemsFromDB)
                pItem->DeleteFromRedis();
            delete pItem;
            pItem = NULL;
        }
}

void Guild::BankTab::SetInfo(const std::string& name, const std::string& icon)
{
    if (m_name == name && m_icon == icon)
        return;

    m_name = name;
    m_icon = icon;
}

void Guild::BankTab::SetText(const std::string& text)
{
    if (m_text == text)
        return;

    m_text = text;
    utf8truncate(m_text, MAX_GUILD_BANK_TAB_TEXT_LEN);          // DB and client size limitation
}

// Sets/removes contents of specified slot.
// If pItem == NULL contents are removed.
bool Guild::BankTab::SetItem(uint8 slotId, Item* item)
{
    if (slotId >= GUILD_BANK_MAX_SLOTS)
        return false;

    m_items[slotId] = item;

    if (item)
    {
        item->SetUInt64Value(ITEM_FIELD_CONTAINED, 0);
        item->SetUInt64Value(ITEM_FIELD_OWNER, 0);
        item->SetTabId(m_tabId);
        item->SetSlot(slotId);
        item->SaveItem();
    }
    return true;
}

void Guild::BankTab::SendText(Guild const* guild, WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_BANK_QUERY_TEXT_RESULT, 4 + m_text.size() + 2);
    data.WriteBits(m_text.size(), 14);
    data.WriteString(m_text);
    data << uint32(m_tabId);

    if (session)
        session->SendPacket(&data);
    else
        guild->BroadcastPacket(&data);
}

///////////////////////////////////////////////////////////////////////////////
// Member
void Guild::Member::SetStats(Player* player)
{
    m_name      = player->GetName();
    m_level     = player->getLevel();
    m_class     = player->getClass();
    m_zoneId    = player->GetZoneId();
    m_gender    = player->getGender();
    m_accountId = player->GetSession()->GetAccountId();
    m_achievementPoints = player->GetAchievementPoints();

    for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
    {
        uint32 id = player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + i);
        m_professionInfo[i] = ProfessionInfo(id, player->GetSkillValue(id), player->GetSkillStep(id));
    }

    std::set<uint32> profSpells[MAX_GUILD_PROFESSIONS];
    PlayerSpellMap const& spellMap = player->GetSpellMap();
    for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
    {
        if (itr->second->state == PLAYERSPELL_REMOVED)
            continue;

        if (!itr->second->active || itr->second->disabled)
            continue;

        for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
        {
            if (IsPartOfSkillLine(m_professionInfo[i].skillId, itr->first))
                profSpells[i].insert(itr->first);
        }
    }

    for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
        m_professionInfo[i].GenerateRecipesMask(profSpells[i]);
}

void Guild::UpdateProf(Player* player)
{
    if (Member* member = GetMember(player->GetGUID()))
        member->UpdateProf(player);
}

void Guild::Member::UpdateProf(Player* player)
{
    for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
    {
        uint32 id = player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + i);
        m_professionInfo[i] = ProfessionInfo(id, player->GetSkillValue(id), player->GetSkillStep(id));
    }

    std::set<uint32> profSpells[MAX_GUILD_PROFESSIONS];
    PlayerSpellMap const& spellMap = player->GetSpellMap();
    for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
    {
        if (itr->second->state == PLAYERSPELL_REMOVED)
            continue;

        if (!itr->second->active || itr->second->disabled)
            continue;

        for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
        {
            if (IsPartOfSkillLine(m_professionInfo[i].skillId, itr->first))
                profSpells[i].insert(itr->first);
        }
    }

    for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
    {
        m_professionInfo[i].GenerateRecipesMask(profSpells[i]);
        if (Guild* guild = player->GetGuild())
            guild->AddGuildRecipes(&m_professionInfo[i]);
    }
}

void Guild::SetAchievementPoints(Player* player)
{
    if (Member* member = GetMember(player->GetGUID()))
    {
        member->SetAchievementPoints(player->GetAchievementPoints());
        member->SaveGuildMember();
    }
}

uint8 Guild::GetRankId(Player* player)
{
    if (Member* member = GetMember(player->GetGUID()))
        return member->GetRankId();

    return 0;
}

void Guild::Member::SetStats(const std::string& name, uint8 level, uint8 _class, uint32 zoneId, uint32 accountId, uint32 reputation, uint8 gender, uint32 achPoints,
                             uint32 profId1, uint32 profValue1, uint8 profRank1, std::string const& recipesMask1, uint32 profId2, uint32 profValue2, uint8 profRank2, std::string const& recipesMask2)
{
    m_name      = name;
    m_level     = level;
    m_class     = _class;
    m_zoneId    = zoneId;
    m_gender    = gender;
    m_accountId = accountId;
    m_totalReputation = reputation;
    m_achievementPoints = achPoints;

    m_professionInfo[0] = ProfessionInfo(profId1, profValue1, profRank1);
    m_professionInfo[0].knownRecipes.LoadFromString(recipesMask1);
    m_professionInfo[1] = ProfessionInfo(profId2, profValue2, profRank2);
    m_professionInfo[1].knownRecipes.LoadFromString(recipesMask2);
}

void Guild::Member::SetPublicNote(const std::string& publicNote)
{
    if (m_publicNote == publicNote)
        return;

    m_publicNote = publicNote;

    SaveGuildMember();
}

void Guild::Member::SetOfficerNote(const std::string& officerNote)
{
    if (m_officerNote == officerNote)
        return;

    m_officerNote = officerNote;

    SaveGuildMember();
}

void Guild::Member::ChangeRank(uint8 newRank)
{
    m_rankId = newRank;

    // Update rank information in player's field, if he is online.
    if (Player* player = FindPlayer())
        player->SetRank(newRank);

    SaveGuildMember();
}

// Loads member's data from database.
// If member has broken fields (level, class) returns false.
// In this case member has to be removed from guild.
bool Guild::Member::LoadFromDB(Field* fields)
{
    m_publicNote    = fields[3].GetString();
    m_officerNote   = fields[4].GetString();
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime  = fields[5].GetUInt32();
    m_bankRemaining[GUILD_BANK_MAX_TABS].value      = fields[6].GetUInt32();
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; ++i)
    {
        m_bankRemaining[i].resetTime                = fields[7 + i * 2].GetUInt32();
        m_bankRemaining[i].value                    = fields[8 + i * 2].GetUInt32();
    }

    SetStats(fields[23].GetString(),
             fields[24].GetUInt8(),                         // characters.level
             fields[25].GetUInt8(),                         // characters.class
             fields[26].GetUInt16(),                        // characters.zone
             fields[27].GetUInt32(),                        // characters.account
             fields[29].GetUInt32(),                        // character_reputation.standing
             fields[34].GetUInt8(),                         // characters.gender
             fields[33].GetUInt32(),                        // achievement points
             fields[35].GetUInt32(),                        // prof id 1
             fields[36].GetUInt32(),                        // prof value 1
             fields[37].GetUInt8(),                         // prof rank 1
             fields[38].GetString(),                        // prof recipes mask 1
             fields[39].GetUInt32(),                        // prof id 2
             fields[40].GetUInt32(),                        // prof value 2
             fields[41].GetUInt8(),                         // prof rank 2
             fields[32].GetString()                         // prof recipes mask 2
             );
    m_logoutTime    = fields[28].GetUInt32();               // characters.logout_time
    m_totalActivity = fields[30].GetUInt64();
    m_weekActivity = fields[31].GetUInt64();
    m_weekReputation = fields[32].GetUInt32();

    if (!CheckStats())
        return false;

    if (!m_zoneId)
        m_zoneId = 5736; //Hack

    return true;
}

// Validate player fields. Returns false if corrupted fields are found.
bool Guild::Member::CheckStats() const
{
    if (m_level < 1)
    {
        sLog->outError(LOG_FILTER_GUILD, "Player (GUID: %u) has a broken data in field `characters`.`level`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    if (m_class < CLASS_WARRIOR || m_class >= MAX_CLASSES)
    {
        sLog->outError(LOG_FILTER_GUILD, "Player (GUID: %u) has a broken data in field `characters`.`class`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    if (m_gender != GENDER_FEMALE && m_gender != GENDER_MALE)
    {
        sLog->outError(LOG_FILTER_GUILD, "Player (GUID: %u) has a broken data in field `characters`.`gender`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    return true;
}

void Guild::Member::ResetValues()
{
    m_weekActivity = 0;
    m_weekReputation = 0;
}

// Decreases amount of money/slots left for today.
// If (tabId == GUILD_BANK_MAX_TABS) decrease money amount.
// Otherwise decrease remaining items amount for specified tab.
void Guild::Member::DecreaseBankRemainingValue(uint8 tabId, uint32 amount)
{
    m_bankRemaining[tabId].value -= amount;
    SaveGuildMember();
}

// Get amount of money/slots left for today.
// If (tabId == GUILD_BANK_MAX_TABS) return money amount.
// Otherwise return remaining items amount for specified tab.
// If reset time was more than 24 hours ago, renew reset time and reset amount to maximum value.
uint32 Guild::Member::GetBankRemainingValue(uint8 tabId, const Guild* guild) const
{
    // Guild master has unlimited amount.
    if (IsRank(GR_GUILDMASTER))
        return tabId == GUILD_BANK_MAX_TABS ? GUILD_WITHDRAW_MONEY_UNLIMITED : GUILD_WITHDRAW_SLOT_UNLIMITED;

    // Check rights for non-money tab.
    if (tabId != GUILD_BANK_MAX_TABS)
        if ((guild->_GetRankBankTabRights(m_rankId, tabId) & GUILD_BANK_RIGHT_VIEW_TAB) != GUILD_BANK_RIGHT_VIEW_TAB)
            return 0;

    uint32 curTime = uint32(::time(NULL) / MINUTE); // minutes
    if (curTime > m_bankRemaining[tabId].resetTime + 24 * HOUR / MINUTE)
    {
        RemainingValue& rv = const_cast <RemainingValue&> (m_bankRemaining[tabId]);
        rv.resetTime = curTime;
        rv.value = tabId == GUILD_BANK_MAX_TABS ?
            guild->_GetRankBankMoneyPerDay(m_rankId) :
            guild->_GetRankBankTabSlotsPerDay(m_rankId, tabId);

        const_cast<Member*>(this)->SaveGuildMember();
    }
    return m_bankRemaining[tabId].value;
}

inline void Guild::Member::ResetTabTimes()
{
    bool needSave = false;
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        if (m_bankRemaining[tabId].resetTime != 0)
            needSave = true;
        m_bankRemaining[tabId].resetTime = 0;
    }

    if (needSave)
        SaveGuildMember();
}

inline void Guild::Member::ResetMoneyTime()
{
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime = 0;
}

///////////////////////////////////////////////////////////////////////////////
// EmblemInfo
void EmblemInfo::LoadFromDB(Field* fields)
{
    m_style             = fields[3].GetUInt8();
    m_color             = fields[4].GetUInt8();
    m_borderStyle       = fields[5].GetUInt8();
    m_borderColor       = fields[6].GetUInt8();
    m_backgroundColor   = fields[7].GetUInt8();
}

void EmblemInfo::WritePacket(WorldPacket& data) const
{
    data << uint32(m_style);
    data << uint32(m_color);
    data << uint32(m_borderStyle);
    data << uint32(m_borderColor);
    data << uint32(m_backgroundColor);
}

///////////////////////////////////////////////////////////////////////////////
// MoveItemData
bool Guild::MoveItemData::CheckItem(uint32& splitedAmount)
{
    ASSERT(m_pItem);
    if (splitedAmount > m_pItem->GetCount())
        return false;
    if (splitedAmount == m_pItem->GetCount())
        splitedAmount = 0;
    return true;
}

bool Guild::MoveItemData::CanStore(Item* pItem, bool swap, bool sendError)
{
    m_vec.clear();
    InventoryResult msg = CanStore(pItem, swap);
    if (sendError && msg != EQUIP_ERR_OK)
        m_pPlayer->SendEquipError(msg, pItem);
    return (msg == EQUIP_ERR_OK);
}

bool Guild::MoveItemData::CloneItem(uint32 count)
{
    ASSERT(m_pItem);
    m_pClonedItem = m_pItem->CloneItem(count);
    if (!m_pClonedItem)
    {
        m_pPlayer->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, m_pItem);
        return false;
    }
    m_pClonedItem->SetItemKey(ITEM_KEY_GUILD, m_pGuild->GetId());
    if(m_pItem->GetEntry() == 38186)
        sLog->outDebug(LOG_FILTER_EFIR, "BankMoveItemData - CloneItem of item %u; count = %u playerGUID %u, guild %u", m_pItem->GetEntry(), count, m_pGuild->GetId(), m_pItem->GetGUID());
    return true;
}

void Guild::MoveItemData::LogAction(MoveItemData* pFrom) const
{
    ASSERT(pFrom->GetItem());

    sScriptMgr->OnGuildItemMove(m_pGuild, m_pPlayer, pFrom->GetItem(),
        pFrom->IsBank(), pFrom->GetContainer(), pFrom->GetSlotId(),
        IsBank(), GetContainer(), GetSlotId());
}

inline void Guild::MoveItemData::CopySlots(SlotIds& ids) const
{
    for (ItemPosCountVec::const_iterator itr = m_vec.begin(); itr != m_vec.end(); ++itr)
        ids.insert(uint8(itr->pos));
}

///////////////////////////////////////////////////////////////////////////////
// PlayerMoveItemData
bool Guild::PlayerMoveItemData::InitItem()
{
    m_pItem = m_pPlayer->GetItemByPos(m_container, m_slotId);
    if (m_pItem)
    {
        // Anti-WPE protection. Do not move non-empty bags to bank.
        if (m_pItem->IsNotEmptyBag())
        {
            m_pPlayer->SendEquipError(EQUIP_ERR_DESTROY_NONEMPTY_BAG, m_pItem);
            m_pItem = NULL;
        }
        // Bound items cannot be put into bank.
        else if (!m_pItem->CanBeTraded())
        {
            m_pPlayer->SendEquipError(EQUIP_ERR_CANT_SWAP, m_pItem);
            m_pItem = NULL;
        }
    }
    return (m_pItem != NULL);
}

void Guild::PlayerMoveItemData::RemoveItem(MoveItemData* /*pOther*/, uint32 splitedAmount)
{
    if (splitedAmount)
    {
        m_pItem->SetCount(m_pItem->GetCount() - splitedAmount);
        m_pItem->SaveItem();
    }
    else
    {
        m_pPlayer->MoveItemFromInventory(m_container, m_slotId, true);
        m_pItem->DeleteFromRedis();
        m_pItem = NULL;
    }
}

Item* Guild::PlayerMoveItemData::StoreItem(Item* pItem)
{
    ASSERT(pItem);

    if(pItem->GetEntry() == 38186)
        sLog->outDebug(LOG_FILTER_EFIR, "PlayerMoveItemData::StoreItem - item %u; count = %u playerGUID %u, itemGUID %u", pItem->GetEntry(), pItem->GetCount(), m_pPlayer->GetGUID(), pItem->GetGUID());

    m_pPlayer->MoveItemToInventory(m_vec, pItem, true);
    return pItem;
}

void Guild::PlayerMoveItemData::LogBankEvent(MoveItemData* pFrom, uint32 count) const
{
    ASSERT(pFrom);
    // Bank -> Char
    m_pGuild->_LogBankEvent(GUILD_BANK_LOG_WITHDRAW_ITEM, pFrom->GetContainer(), m_pPlayer->GetGUIDLow(),
        pFrom->GetItem()->GetEntry(), count);
}

inline InventoryResult Guild::PlayerMoveItemData::CanStore(Item* pItem, bool swap)
{
    return m_pPlayer->CanStoreItem(m_container, m_slotId, m_vec, pItem, swap);
}

///////////////////////////////////////////////////////////////////////////////
// BankMoveItemData
bool Guild::BankMoveItemData::InitItem()
{
    m_pItem = m_pGuild->_GetItem(m_container, m_slotId);
    return (m_pItem != NULL);
}

bool Guild::BankMoveItemData::HasStoreRights(MoveItemData* pOther) const
{
    ASSERT(pOther);
    // Do not check rights if item is being swapped within the same bank tab
    if (pOther->IsBank() && pOther->GetContainer() == m_container)
        return true;
    return m_pGuild->_MemberHasTabRights(m_pPlayer->GetGUID(), m_container, GUILD_BANK_RIGHT_DEPOSIT_ITEM);
}

bool Guild::BankMoveItemData::HasWithdrawRights(MoveItemData* pOther) const
{
    ASSERT(pOther);
    // Do not check rights if item is being swapped within the same bank tab
    if (pOther->IsBank() && pOther->GetContainer() == m_container)
        return true;
    return (m_pGuild->_GetMemberRemainingSlots(m_pPlayer->GetGUID(), m_container) != 0);
}

void Guild::BankMoveItemData::RemoveItem(MoveItemData* pOther, uint32 splitedAmount)
{
    ASSERT(m_pItem);
    if (splitedAmount)
    {
        m_pItem->SetCount(m_pItem->GetCount() - splitedAmount);
        m_pItem->SaveItem();
    }
    else
    {
        m_pItem->DeleteFromRedis();
        m_pGuild->_RemoveItem(m_container, m_slotId);
        m_pItem = NULL;
    }
    // Decrease amount of player's remaining items (if item is moved to different tab or to player)
    if (!pOther->IsBank() || pOther->GetContainer() != m_container)
        m_pGuild->_DecreaseMemberRemainingSlots(m_pPlayer->GetGUID(), m_container);
}

Item* Guild::BankMoveItemData::StoreItem(Item* pItem)
{
    if (!pItem)
        return NULL;

    BankTab* pTab = m_pGuild->GetBankTab(m_container);
    if (!pTab)
        return NULL;

    Item* pLastItem = pItem;
    for (ItemPosCountVec::const_iterator itr = m_vec.begin(); itr != m_vec.end(); )
    {
        ItemPosCount pos(*itr);
        ++itr;

        sLog->outDebug(LOG_FILTER_GUILD, "GUILD STORAGE: StoreItem tab = %u, slot = %u, item = %u, count = %u",
            m_container, m_slotId, pItem->GetEntry(), pItem->GetCount());
        pLastItem = _StoreItem(pTab, pItem, pos, itr != m_vec.end());

        pLastItem->UpdateItemKey(ITEM_KEY_GUILD, m_pGuild->GetId());
    }
    return pLastItem;
}

void Guild::BankMoveItemData::LogBankEvent(MoveItemData* pFrom, uint32 count) const
{
    ASSERT(pFrom->GetItem());
    if (pFrom->IsBank())
        // Bank -> Bank
        m_pGuild->_LogBankEvent(GUILD_BANK_LOG_MOVE_ITEM, pFrom->GetContainer(), m_pPlayer->GetGUIDLow(),
            pFrom->GetItem()->GetEntry(), count, m_container);
    else
        // Char -> Bank
        m_pGuild->_LogBankEvent(GUILD_BANK_LOG_DEPOSIT_ITEM, m_container, m_pPlayer->GetGUIDLow(),
            pFrom->GetItem()->GetEntry(), count);
}

void Guild::BankMoveItemData::LogAction(MoveItemData* pFrom) const
{
    MoveItemData::LogAction(pFrom);
    if (!pFrom->IsBank() && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE) && !AccountMgr::IsPlayerAccount(m_pPlayer->GetSession()->GetSecurity()))       // TODO: move to scripts
        sLog->outCommand(m_pPlayer->GetSession()->GetAccountId(),
            "GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u)",
            m_pPlayer->GetName(), m_pPlayer->GetSession()->GetAccountId(),
            pFrom->GetItem()->GetTemplate()->Name1.c_str(), pFrom->GetItem()->GetEntry(), pFrom->GetItem()->GetCount(),
            m_pGuild->GetId());
}

Item* Guild::BankMoveItemData::_StoreItem(BankTab* pTab, Item* pItem, ItemPosCount& pos, bool clone) const
{
    uint8 slotId = uint8(pos.pos);
    uint32 count = pos.count;
    if (Item* pItemDest = pTab->GetItem(slotId))
    {
        pItemDest->SetCount(pItemDest->GetCount() + count);
        pItemDest->SaveItem();
        if (!clone)
        {
            pItem->RemoveFromWorld();
            pItem->DeleteFromRedis();
            delete pItem;
        }
        return pItemDest;
    }

    if (clone)
        pItem = pItem->CloneItem(count);
    else
        pItem->SetCount(count);

    pItem->SetItemKey(ITEM_KEY_GUILD, m_pGuild->GetId());

    if(pItem->GetEntry() == 38186)
        sLog->outDebug(LOG_FILTER_EFIR, "BankMoveItemData - CloneItem %i of item %u; count = %u playerGUID %u, guild %u", clone, pItem->GetEntry(), count, m_pGuild->GetId(), pItem->GetGUID());

    if (pTab->SetItem(slotId, pItem))
        return pItem;

    return NULL;
}

// Tries to reserve space for source item.
// If item in destination slot exists it must be the item of the same entry
// and stack must have enough space to take at least one item.
// Returns false if destination item specified and it cannot be used to reserve space.
bool Guild::BankMoveItemData::_ReserveSpace(uint8 slotId, Item* pItem, Item* pItemDest, uint32& count)
{
    uint32 requiredSpace = pItem->GetMaxStackCount();
    if (pItemDest)
    {
        // Make sure source and destination items match and destination item has space for more stacks.
        if (pItemDest->GetEntry() != pItem->GetEntry() || pItemDest->GetCount() >= pItem->GetMaxStackCount())
            return false;
        requiredSpace -= pItemDest->GetCount();
    }
    // Let's not be greedy, reserve only required space
    requiredSpace = std::min(requiredSpace, count);

    // Reserve space
    ItemPosCount pos(slotId, requiredSpace);
    if (!pos.isContainedIn(m_vec))
    {
        m_vec.push_back(pos);
        count -= requiredSpace;
    }
    return true;
}

void Guild::BankMoveItemData::CanStoreItemInTab(Item* pItem, uint8 skipSlotId, bool merge, uint32& count)
{
    for (uint8 slotId = 0; (slotId < GUILD_BANK_MAX_SLOTS) && (count > 0); ++slotId)
    {
        // Skip slot already processed in CanStore (when destination slot was specified)
        if (slotId == skipSlotId)
            continue;

        Item* pItemDest = m_pGuild->_GetItem(m_container, slotId);
        if (pItemDest == pItem)
            pItemDest = NULL;

        // If merge skip empty, if not merge skip non-empty
        if ((pItemDest != NULL) != merge)
            continue;

        _ReserveSpace(slotId, pItem, pItemDest, count);
    }
}

InventoryResult Guild::BankMoveItemData::CanStore(Item* pItem, bool swap)
{
    sLog->outDebug(LOG_FILTER_GUILD, "GUILD STORAGE: CanStore() tab = %u, slot = %u, item = %u, count = %u",
        m_container, m_slotId, pItem->GetEntry(), pItem->GetCount());

    uint32 count = pItem->GetCount();
    // Soulbound items cannot be moved
    if (pItem->IsSoulBound())
        return EQUIP_ERR_DROP_BOUND_ITEM;

    // Make sure destination bank tab exists
    if (m_container >= m_pGuild->GetPurchasedTabsSize())
        return EQUIP_ERR_WRONG_BAG_TYPE;

    // Slot explicitely specified. Check it.
    if (m_slotId != NULL_SLOT)
    {
        Item* pItemDest = m_pGuild->_GetItem(m_container, m_slotId);
        // Ignore swapped item (this slot will be empty after move)
        if ((pItemDest == pItem) || swap)
            pItemDest = NULL;

        if (!_ReserveSpace(m_slotId, pItem, pItemDest, count))
            return EQUIP_ERR_CANT_STACK;

        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // Slot was not specified or it has not enough space for all the items in stack
    // Search for stacks to merge with
    if (pItem->GetMaxStackCount() > 1)
    {
        CanStoreItemInTab(pItem, m_slotId, true, count);
        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // Search free slot for item
    CanStoreItemInTab(pItem, m_slotId, false, count);
    if (count == 0)
        return EQUIP_ERR_OK;

    return EQUIP_ERR_BANK_FULL;
}

///////////////////////////////////////////////////////////////////////////////
// Guild
Guild::Guild() : m_id(0), m_leaderGuid(0), m_createdDate(0), m_bankMoney(0), m_eventLog(NULL),
    m_achievementMgr(this), _level(1), _experience(0), _todayExperience(0), _newsLog(this)
{
    memset(&m_bankEventLog, 0, (GUILD_BANK_MAX_TABS + 1) * sizeof(LogHolder*));
}

Guild::~Guild()
{
    _DeleteBankItems();

    // Cleanup
    if (m_eventLog)
        delete m_eventLog;
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
        if (m_bankEventLog[tabId])
            delete m_bankEventLog[tabId];
    for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        delete itr->second;
}

// Creates new guild with default data and saves it to database.
bool Guild::Create(Player* pLeader, const std::string& name)
{
    // Check if guild with such name already exists
    if (sGuildMgr->GetGuildByName(name))
        return false;

    WorldSession* pLeaderSession = pLeader->GetSession();
    if (!pLeaderSession)
        return false;

    m_id = sObjectMgr->GenerateGuildId();
    m_leaderGuid = pLeader->GetGUID();
    m_name = name;
    m_info = "";
    m_motd = "No message set.";
    m_bankMoney = 0;
    m_createdDate = ::time(NULL);
    _level = 1;
    _CreateLogHolders();
    CreateKey();

    sLog->outDebug(LOG_FILTER_GUILD, "GUILD: creating guild [%s] for leader %s (%u)",
        name.c_str(), pLeader->GetName(), GUID_LOPART(m_leaderGuid));

    DeleteMembers();

    // Create default ranks
    _CreateDefaultGuildRanks(pLeaderSession->GetSessionDbLocaleIndex());
    // Add guildmaster
    bool ret = AddMember(m_leaderGuid, GR_GUILDMASTER);
    if (ret)
        // Call scripts on successful create
        sScriptMgr->OnGuildCreate(this, pLeader, name);

    SendGuildEventRankUpdate(); //SendGuildEventRankUpdate(0);

    SaveGuild();

    return ret;
}

// Disbands guild and deletes all related data from database
void Guild::Disband()
{
    // Call scripts before guild data removed from database
    sScriptMgr->OnGuildDisband(this);

    SendGuildEventDisbanded();

    // Remove all members
    while (!m_members.empty())
    {
        Members::const_iterator itr = m_members.begin();
        DeleteMember(itr->second->GetGUID(), true);
    }

    DeleteMembers();

    // Free bank tab used memory and delete items stored in them
    _DeleteBankItems(true);

    sObjectMgr->DeleteAllItems(ITEM_KEY_GUILD, m_id);

    RedisDatabase.AsyncExecute("DEL", GetGuildKey(), m_id, [&](const RedisValue &v, uint64 guid) {});

    sGuildFinderMgr->DeleteGuild(m_id);

    sGuildMgr->RemoveGuild(m_id);
}

///////////////////////////////////////////////////////////////////////////////
// HANDLE CLIENT COMMANDS
void Guild::HandleRoster(WorldSession* session /*= NULL*/)
{
    ByteBuffer memberData(100);
    // Guess size
    WorldPacket data(SMSG_GUILD_ROSTER);

    data << uint32(0);
    data << uint32(sWorld->getIntConfig(CONFIG_GUILD_WEEKLY_REP_CAP));
    data << uint32(secsToTimeBitFields(m_createdDate));
    data << uint32(m_accountsNumber.size());

    data.WriteBits(m_members.size(), 17);

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Member* member = itr->second;
        Player* player = member->FindPlayer();
        ObjectGuid guid = member->GetGUID();

        data.WriteGuidMask<0, 2, 1>(guid);
        data.WriteBit(0);               // Has Authenticator
        data.WriteBits(member->GetPublicNote().length(), 8);
        data.WriteGuidMask<4>(guid);
        data.WriteBits(member->GetName().length(), 6);
        data.WriteBits(member->GetOfficerNote().length(), 8);
        data.WriteGuidMask<5, 6, 3, 7>(guid);
        data.WriteBit(0);               // Can Scroll of Ressurect
    }

    data.WriteBits(m_info.length(), 11);
    data.WriteBits(m_motd.length(), 10);

    data.FlushBits();

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Member* member = itr->second;
        Player* player = member->FindPlayer();
        size_t pubNoteLength = member->GetPublicNote().length();
        size_t offNoteLength = member->GetOfficerNote().length();

        ObjectGuid guid = member->GetGUID();
        uint8 flags = GUILDMEMBER_STATUS_NONE;
        if (player)
        {
            flags |= GUILDMEMBER_STATUS_ONLINE;
            if (player->isAFK())
                flags |= GUILDMEMBER_STATUS_AFK;
            if (player->isDND())
                flags |= GUILDMEMBER_STATUS_DND;
        }

        data << uint8(flags);
        data.WriteGuidBytes<3>(guid);
        for (uint8 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
            data << uint32(member->GetProfessionInfo(i).skillRank) << uint32(member->GetProfessionInfo(i).skillValue) << uint32(member->GetProfessionInfo(i).skillId);
        data.WriteGuidBytes<1>(guid);
        data << uint32(player ? player->GetAchievementPoints() : member->GetAchievementPoints());// player->GetAchievementMgr().GetCompletedAchievementsAmount()
        data.WriteGuidBytes<6>(guid);
        data << uint8(member->GetClass());
        data << uint8(player ? player->getLevel() : member->GetLevel());
        data << uint32(realmID);
        data << int32(member->GetTotalReputation());                                              // Guild rep
        data << float(player ? 0.0f : float(::time(NULL) - member->GetLogoutTime()) / DAY);
        data.WriteGuidBytes<2>(guid);
        data << uint8(player ? player->getGender() : member->GetGender());
        data.WriteGuidBytes<5>(guid);
        data << uint32(std::max(int32(sWorld->getIntConfig(CONFIG_GUILD_WEEKLY_REP_CAP) - member->GetWeekReputation()), 0));   // Remaining guild week Rep
        data << uint64(member->GetTotalActivity());
        data.WriteGuidBytes<4>(guid);
        data.WriteString(member->GetName());
        data << uint64(member->GetWeekActivity());
        data << uint32(player ? player->GetZoneId() : member->GetZoneId());
        data.WriteString(member->GetPublicNote());
        data.WriteString(member->GetOfficerNote());
        data.WriteGuidBytes<7, 0>(guid);
        data << uint32(member->GetRankId());
    }

    data.WriteString(m_motd);
    data.WriteString(m_info);

    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_ROSTER)");
}

void Guild::HandleQuery(WorldSession* session)
{
    WorldPacket data(SMSG_GUILD_QUERY_RESPONSE, 8 * 32 + 200);      // Guess size

    ObjectGuid guid = GetGUID();
    data.WriteGuidMask<5, 0, 3, 4, 7, 1>(guid);
    data.WriteBit(1);                                               // has data

    data.WriteGuidMask<7, 5, 0, 3, 4>(guid);
    data.WriteBits(m_name.size(), 7);
    data.WriteGuidMask<2, 6, 1>(guid);
    data.WriteBits(_GetRanksSize(), 21);
    for (uint32 i = 0; i < _GetRanksSize(); ++i)
        data.WriteBits(m_ranks[i].GetName().size(), 7);

    data.WriteGuidMask<2, 6>(guid);

    data << uint32(m_emblemInfo.GetStyle());
    data.WriteGuidBytes<2, 6, 4, 3>(guid);
    data << uint32(realmID);
    for (uint32 i = 0; i < _GetRanksSize(); ++i)
    {
        data.WriteString(m_ranks[i].GetName());
        data << uint32(i);                                          // Rank order of creation
        data << uint32(m_ranks[i].GetId());                         // Rank order of "importance" (sorting by rights)
    }
    data.WriteGuidBytes<5>(guid);
    data.WriteString(m_name);
    data << uint32(m_emblemInfo.GetColor());
    data << uint32(m_emblemInfo.GetBorderColor());
    data << uint32(m_emblemInfo.GetBorderStyle());
    data.WriteGuidBytes<1>(guid);
    data << uint32(m_emblemInfo.GetBackgroundColor());
    data.WriteGuidBytes<0, 7>(guid);

    data.WriteGuidBytes<3, 0, 5, 2, 7, 1, 6, 4>(guid);

    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_QUERY_RESPONSE)");
}

void Guild::HandleGuildRanks(WorldSession* session) const
{
    // perhaps move to guild.cpp.....
    ByteBuffer rankData(100);
    WorldPacket data(SMSG_GUILD_RANK, 100);

    data.WriteBits(_GetRanksSize(), 17);

    for (uint8 i = 0; i < _GetRanksSize(); ++i)
    {
        RankInfo const* rankInfo = GetRankInfo(i);

        data.WriteBits(rankInfo->GetName().length(), 7);

        rankData << uint32(rankInfo->GetId());
        rankData << uint32(rankInfo->GetRights());
        for (uint8 j = 0; j < GUILD_BANK_MAX_TABS; ++j)
        {
            rankData << uint32(rankInfo->GetBankTabSlotsPerDay(j));
            rankData << uint32(rankInfo->GetBankTabRights(j));
        }
        rankData.WriteString(rankInfo->GetName());
        rankData << uint32(i);
        rankData << uint32(rankInfo->GetBankMoneyPerDay() / 10000);
    }

    if (!rankData.empty())
    {
        data.FlushBits();
        data.append(rankData);
    }

    session->SendPacket(&data);
}

void Guild::HandleSetMOTD(WorldSession* session, const std::string& motd)
{
    if (m_motd == motd)
        return;

    // Player must have rights to set MOTD
    if (!_HasRankRight(session->GetPlayer(), GR_RIGHT_SETMOTD))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    else
    {
        m_motd = motd;

        sScriptMgr->OnGuildMOTDChanged(this, motd);

        SaveGuild();
        SendMotd();
    }
}

void Guild::HandleSetInfo(WorldSession* session, const std::string& info)
{
    if (m_info == info)
        return;

    // Player must have rights to set guild's info
    if (!_HasRankRight(session->GetPlayer(), GR_RIGHT_MODIFY_GUILD_INFO))
        SendCommandResult(session, GUILD_CREATE_S, ERR_GUILD_PERMISSIONS);
    else
    {
        m_info = info;

        sScriptMgr->OnGuildInfoChanged(this, info);

        SaveGuild();
    }
}

void Guild::HandleSetEmblem(WorldSession* session, const EmblemInfo& emblemInfo)
{
    Player* player = session->GetPlayer();
    if (!_IsLeader(player))
        // "Only guild leaders can create emblems."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_NOTGUILDMASTER);
    else if (!player->HasEnoughMoney(uint64(EMBLEM_PRICE)))
        // "You can't afford to do that."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_NOTENOUGHMONEY);
    else
    {
        player->ModifyMoney(-int64(EMBLEM_PRICE));

        m_emblemInfo = emblemInfo;
        UpdateGuildEmblem();

        // "Guild Emblem saved."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_SUCCESS);

        HandleQuery(session);

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_EMBLEM, 1, 0, 0, 0, player);
    }
}

void Guild::HandleSetLeader(WorldSession* session, const std::string& name)
{
    Player* player = session->GetPlayer();
    // Only leader can assign new leader
    if (!_IsLeader(player))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    // Old leader must be a member of guild
    else if (Member* pOldLeader = GetMember(player->GetGUID()))
    {
        // New leader must be a member of guild
        if (Member* pNewLeader = GetMember(session, name))
        {
            _SetLeaderGUID(pNewLeader);
            pOldLeader->ChangeRank(GR_OFFICER);
            SendGuildEventLeader(pNewLeader->GetGUID(), name, player->GetGUID(), player->GetName());
        }
    }
    else
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
}

void Guild::HandleSetBankTabInfo(WorldSession* session, uint8 tabId, const std::string& name, const std::string& icon)
{
    if (BankTab* pTab = GetBankTab(tabId))
    {
        pTab->SetInfo(name, icon);
        UpdateGuildBankTab(pTab);

        WorldPacket data(SMSG_GUILD_EVENT_GUILDBANK_TAB_UPDATE, 4 + 2 + name.size() + icon.size());
        data << uint32(tabId);
        data.WriteBits(name.size(), 7);
        data.WriteBits(icon.size(), 9);
        data.WriteString(icon);
        data.WriteString(name);
        BroadcastPacket(&data);
    }
}

void Guild::HandleSetMemberNote(WorldSession* session, std::string const& note, uint64 guid, bool isPublic)
{
    // Player must have rights to set public/officer note
    if (!_HasRankRight(session->GetPlayer(), isPublic ? GR_RIGHT_EPNOTE : GR_RIGHT_EOFFNOTE))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    else if (Member* member = GetMember(guid))
    {
        if (isPublic)
            member->SetPublicNote(note);
        else
            member->SetOfficerNote(note);
        HandleRoster(session);
    }
}

void Guild::HandleSetRankInfo(WorldSession* session, uint32 rankId, const std::string& name, uint32 rights, uint32 moneyPerDay, GuildBankRightsAndSlotsVec rightsAndSlots)
{
    // Only leader can modify ranks
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    else if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Changed RankName to '%s', rights to 0x%08X", name.c_str(), rights);

        rankInfo->SetName(name);
        rankInfo->SetRights(rights);
        _SetRankBankMoneyPerDay(rankId, moneyPerDay);

        for (GuildBankRightsAndSlotsVec::const_iterator itr = rightsAndSlots.begin(); itr != rightsAndSlots.end(); ++itr)
            _SetRankBankTabRightsAndSlots(rankId, *itr);

        UpdateGuildRank(rankInfo);

        SendGuildEventRankUpdate(rankId);
    }
}

void Guild::HandleBuyBankTab(WorldSession* session, uint8 tabId)
{
    Player* player = session->GetPlayer();
    if (!player)
        return;

    Member const* member = GetMember(player->GetGUID());
    if (!member)
        return;

    if (tabId != GetPurchasedTabsSize())
        return;

    uint32 tabCost = _GetGuildBankTabPrice(tabId) * GOLD;
    if (!tabCost)
        return;

    if (!player->HasEnoughMoney(uint64(tabCost)))                   // Should not happen, this is checked by client
        return;

    if (!_CreateNewBankTab())
        return;

    player->ModifyMoney(-int64(tabCost));
    WorldPacket data(SMSG_GUILD_EVENT_TAB_PURCHASED, 0);
    BroadcastPacket(&data);
}

void Guild::HandleSpellEffectBuyBankTab(WorldSession* session, uint8 tabId)
{
    if (tabId != GetPurchasedTabsSize())
        return;

    Player* player = session->GetPlayer();
    if (!_CreateNewBankTab())
        return;

    WorldPacket data(SMSG_GUILD_EVENT_TAB_PURCHASED, 0);
    BroadcastPacket(&data);

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS, tabId + 1, 0, 0, 0, player);
}

void Guild::HandleInviteMember(WorldSession* session, const std::string& name)
{
    Player* pInvitee = sObjectAccessor->FindPlayerByName(name.c_str());
    if (!pInvitee)
    {
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PLAYER_NOT_FOUND_S, name);
        return;
    }

    Player* player = session->GetPlayer();
    // Do not show invitations from ignored players
    if (pInvitee->GetSocial()->HasIgnore(player->GetGUIDLow()))
        return;
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && pInvitee->GetTeam() != player->GetTeam())
    {
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_NOT_ALLIED, name);
        return;
    }

    // Invited player cannot be in another guild
    if (pInvitee->GetGuildId())
    {
        SendCommandResult(session, GUILD_INVITE_S, ERR_ALREADY_IN_GUILD_S, name);
        return;
    }

    // Invited player cannot be invited
    if (pInvitee->GetGuildIdInvited())
    {
        SendCommandResult(session, GUILD_INVITE_S, ERR_ALREADY_INVITED_TO_GUILD_S, name);
        return;
    }

    // Inviting player must have rights to invite
    if (!_HasRankRight(player, GR_RIGHT_INVITE))
    {
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
        return;
    }

    sLog->outDebug(LOG_FILTER_GUILD, "Player %s invited %s to join his Guild", player->GetName(), name.c_str());

    pInvitee->SetGuildIdInvited(m_id, player->GetGUID());
    _LogEvent(GUILD_EVENT_LOG_INVITE_PLAYER, player->GetGUIDLow(), pInvitee->GetGUIDLow());

    ObjectGuid oldGuildGuid = MAKE_NEW_GUID(pInvitee->GetGuildId(), 0, pInvitee->GetGuildId() ? uint32(HIGHGUID_GUILD) : 0);
    ObjectGuid newGuildGuid = GetGUID();

    WorldPacket data(SMSG_GUILD_INVITE, 100);
    data << uint32(0);                      // unk
    data << uint32(m_emblemInfo.GetBackgroundColor());
    data << uint32(m_emblemInfo.GetStyle());
    data << uint32(GetLevel());
    data << uint32(realmID);
    data << uint32(m_emblemInfo.GetBorderStyle());
    data << uint32(m_emblemInfo.GetColor());
    data << uint32(m_emblemInfo.GetBorderColor());
    data << uint32(realmID);

    data.WriteBits(strlen(player->GetName()), 6);
    data.WriteGuidMask<7, 2, 6, 3, 5>(newGuildGuid);
    data.WriteBits(pInvitee->GetGuildName().length(), 7);
    data.WriteGuidMask<4>(oldGuildGuid);
    data.WriteGuidMask<1>(newGuildGuid);
    data.WriteGuidMask<1>(oldGuildGuid);
    data.WriteBits(m_name.length(), 7);
    data.WriteGuidMask<5, 0>(oldGuildGuid);
    data.WriteGuidMask<4>(newGuildGuid);
    data.WriteGuidMask<3, 7>(oldGuildGuid);
    data.WriteGuidMask<0>(newGuildGuid);
    data.WriteGuidMask<6, 2>(oldGuildGuid);

    data.WriteGuidBytes<3>(newGuildGuid);
    data.WriteString(player->GetName());
    data.WriteGuidBytes<1>(newGuildGuid);
    data.WriteGuidBytes<3, 1, 0>(oldGuildGuid);
    data.WriteString(m_name);
    data.WriteGuidBytes<7>(newGuildGuid);
    data.WriteGuidBytes<6, 4>(oldGuildGuid);
    data.WriteGuidBytes<4, 5, 6>(newGuildGuid);
    data.WriteString(pInvitee->GetGuildName());
    data.WriteGuidBytes<7, 2>(oldGuildGuid);
    data.WriteGuidBytes<2, 0>(newGuildGuid);
    data.WriteGuidBytes<5>(oldGuildGuid);

    pInvitee->GetSession()->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_INVITE)");
}

void Guild::HandleAcceptMember(WorldSession* session)
{
    Player* player = session->GetPlayer();
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) &&
        player->GetTeam() != sObjectMgr->GetPlayerTeamByGUID(GetLeaderGUID()))
        return;

    if (AddMember(player->GetGUID()))
    {
        _LogEvent(GUILD_EVENT_LOG_JOIN_GUILD, player->GetGUIDLow());
        SendGuildEventJoinMember(player->GetGUID(), player->GetName());
        sGuildFinderMgr->RemoveMembershipRequest(player->GetGUIDLow(), GUID_LOPART(this->GetGUID()));
        UpdateGuildRecipes();
    }
}

void Guild::HandleLeaveMember(WorldSession* session)
{
    Player* player = session->GetPlayer();
    // If leader is leaving
    if (_IsLeader(player))
    {
        if (m_members.size() > 1)
            // Leader cannot leave if he is not the last member
            SendCommandResult(session, GUILD_QUIT_S, ERR_GUILD_LEADER_LEAVE);
        else if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_UNDELETABLE_LEVEL))
            SendCommandResult(session, GUILD_QUIT_S, ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
        else
            // Guild is disbanded if leader leaves.
            Disband();
    }
    else
    {
        SendGuildEventRemoveMember(player->GetGUID(), player->GetName());
        SendCommandResult(session, GUILD_QUIT_S, ERR_PLAYER_NO_MORE_IN_GUILD, m_name);

        DeleteMember(player->GetGUID(), false, false);

        _LogEvent(GUILD_EVENT_LOG_LEAVE_GUILD, player->GetGUIDLow());
        UpdateGuildRecipes();
    }
}

void Guild::HandleRemoveMember(WorldSession* session, uint64 guid)
{
    Player* player = session->GetPlayer();
    Player* removedPlayer = ObjectAccessor::FindPlayer(guid);
    Member* member = GetMember(guid);

    // Player must have rights to remove members
    if (!_HasRankRight(player, GR_RIGHT_REMOVE))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    // Removed player must be a member of the guild
    else if (member)
    {
        // Guild masters cannot be removed
        if (member->IsRank(GR_GUILDMASTER))
            SendCommandResult(session, GUILD_QUIT_S, ERR_GUILD_LEADER_LEAVE);
        // Do not allow to remove player with the same rank or higher
        else if (member->IsRankNotLower(player->GetRank()))
            SendCommandResult(session, GUILD_QUIT_S, ERR_GUILD_RANK_TOO_HIGH_S, member->GetName());
        else
        {
            // After call to DeleteMember pointer to member becomes invalid
            _LogEvent(GUILD_EVENT_LOG_UNINVITE_PLAYER, player->GetGUIDLow(), GUID_LOPART(guid));
            SendGuildEventRemoveMember(member->GetGUID(), member->GetName(), player->GetGUID(), player->GetName());
            DeleteMember(guid, false, true);
        }
    }
    else if (removedPlayer)
        SendCommandResult(session, GUILD_QUIT_S, ERR_PLAYER_NO_MORE_IN_GUILD, removedPlayer->GetName());
}

void Guild::HandleUpdateMemberRank(WorldSession* session, uint64 targetGuid, bool demote)
{
    Player* player = session->GetPlayer();

    // Promoted player must be a member of guild
    if (Member* member = GetMember(targetGuid))
    {
        uint32 oldrank = member->GetRankId();
        if (!_HasRankRight(player, demote ? GR_RIGHT_DEMOTE : GR_RIGHT_PROMOTE))
        {
            SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
            return;
        }

        // Player cannot promote himself
        if (member->IsSamePlayer(player->GetGUID()))
        {
            SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_NAME_INVALID);
            return;
        }

        if (demote)
        {
            // Player can demote only lower rank members
            if (member->IsRankNotLower(player->GetRank()))
            {
                SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_RANK_TOO_HIGH_S, member->GetName());
                return;
            }
            // Lowest rank cannot be demoted
            if (member->GetRankId() >= _GetLowestRankId())
            {
                SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_RANK_TOO_LOW_S, member->GetName());
                return;
            }
        }
        else
        {
            // Allow to promote only to lower rank than member's rank
            // member->GetRank() + 1 is the highest rank that current player can promote to
            if (member->IsRankNotLower(player->GetRank() + 1))
            {
                SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_RANK_TOO_HIGH_S, member->GetName());
                return;
            }
        }

        uint32 newRankId = member->GetRankId() + (demote ? 1 : -1);
        member->ChangeRank(newRankId);
        _LogEvent(demote ? GUILD_EVENT_LOG_DEMOTE_PLAYER : GUILD_EVENT_LOG_PROMOTE_PLAYER, player->GetGUIDLow(), GUID_LOPART(member->GetGUID()), newRankId);
        SendGuildRanksUpdate(player->GetGUID(), member->GetGUID(), newRankId);
    }
}

void Guild::HandleSetMemberRank(WorldSession* session, uint64 targetGuid, uint64 setterGuid, uint32 rank)
{
    Player* player = session->GetPlayer();

    GuildRankRights rights = GR_RIGHT_PROMOTE;
    GuildCommandType type = GUILD_PROMOTE_PLAYER;

    // Promoted player must be a member of guild
    if (Member* member = GetMember(targetGuid))
    {
        if (rank > member->GetRankId())
        {
            rights = GR_RIGHT_DEMOTE;
            type = GUILD_DEMOTE_PLAYER;
        }

        bool demote = (type == GUILD_DEMOTE_PLAYER) ? true : false;

        if (!_HasRankRight(player, rights))
        {
            SendCommandResult(session, type, ERR_GUILD_PERMISSIONS);
            return;
        }

        // Player cannot promote himself
        if (member->IsSamePlayer(player->GetGUID()))
        {
            SendCommandResult(session, type, ERR_GUILD_NAME_INVALID);
            return;
        }

        if (demote)
        {
            // Player can demote only lower rank members
            if (member->IsRankNotLower(player->GetRank()))
            {
                SendCommandResult(session, type, ERR_GUILD_RANK_TOO_HIGH_S, member->GetName());
                return;
            }
            // Lowest rank cannot be demoted
            if (member->GetRankId() >= _GetLowestRankId())
            {
                SendCommandResult(session, type, ERR_GUILD_RANK_TOO_LOW_S, member->GetName());
                return;
            }
        }
        else
        {
            // Allow to promote only to lower rank than member's rank
            // member->GetRank() + 1 is the highest rank that current player can promote to
            if (member->IsRankNotLower(player->GetRank() + 1))
            {
                SendCommandResult(session, type, ERR_GUILD_RANK_TOO_HIGH_S, member->GetName());
                return;
            }
        }

        SendGuildRanksUpdate(setterGuid, targetGuid, rank);
    }
}

void Guild::HandleSwapRanks(WorldSession* session, uint32 id, bool up)
{
    RankInfo* rankinfo = NULL;
    RankInfo* rankinfo2 = NULL;
    uint32 id2 = id - (-1 + 2*uint8(up));
    for (uint32 i = 0; i < m_ranks.size(); ++i)
    {
        if (m_ranks[i].GetId() == id)
            rankinfo = &m_ranks[i];
        if (m_ranks[i].GetId() == id2)
            rankinfo2 = &m_ranks[i];
    }
    if (!rankinfo || !rankinfo2)
        return;

    RankInfo tmp = NULL;
    tmp = *rankinfo2;
    rankinfo2->SetName(rankinfo->GetName());
    rankinfo2->SetRights(rankinfo->GetRights());
    UpdateGuildRank(rankinfo2);

    rankinfo->SetName(tmp.GetName());
    rankinfo->SetRights(tmp.GetRights());
    UpdateGuildRank(rankinfo);

    SendGuildEventRankUpdate();
}

void Guild::HandleAddNewRank(WorldSession* session, std::string const& name) //, uint32 rankId)
{
    if (_GetRanksSize() >= GUILD_RANKS_MAX_COUNT)
        return;

    // Only leader can add new rank
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    else
    {
        _CreateRank(name, GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
        SendGuildEventRankUpdate(_GetRanksSize() - 1);
    }
}

void Guild::HandleRemoveRank(WorldSession* session, uint32 rankId)
{
    // Cannot remove rank if total count is minimum allowed by the client
    if (_GetRanksSize() <= GUILD_RANKS_MIN_COUNT)
        return;

    // Only leader can delete ranks
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    else
    {
        m_ranks.erase(m_ranks.begin() + rankId);

        // Restruct m_ranks
        for (uint8 i = 0; i < m_ranks.size(); ++i)
            if (m_ranks[i].GetId() != i)
                m_ranks[i].SetId(i);

        RefreshGuildRank();
        SendGuildEventRankUpdate();
    }
}

void Guild::HandleMemberDepositMoney(WorldSession* session, uint64 amount, bool cashFlow /*=false*/)
{
    Player* player = session->GetPlayer();

    // Call script after validation and before money transfer.
    sScriptMgr->OnGuildMemberDepositMoney(this, player, amount);

    // Add money to bank
    _ModifyBankMoney(amount, true);
    // Remove money from player
    player->ModifyMoney(-int64(amount));

    // Log GM action (TODO: move to scripts)
    if (!AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(player->GetSession()->GetAccountId(),
            "GM %s (Account: %u) deposit money (Amount: " UI64FMTD ") to guild bank (Guild ID %u)",
            player->GetName(), player->GetSession()->GetAccountId(), amount, m_id);
    }
    // Log guild bank event
    _LogBankEvent(cashFlow ? GUILD_BANK_LOG_CASH_FLOW_DEPOSIT : GUILD_BANK_LOG_DEPOSIT_MONEY, uint8(0), player->GetGUIDLow(), amount);

    if (!cashFlow)
        SendBankList(session, 0, false, false, false);
}

bool Guild::HandleMemberWithdrawMoney(WorldSession* session, uint64 amount, bool repair)
{
    if (m_bankMoney < amount)                               // Not enough money in bank
        return false;

    Player* player = session->GetPlayer();
    if (!_HasRankRight(player, repair ? GR_RIGHT_WITHDRAW_REPAIR : GR_RIGHT_WITHDRAW_GOLD))
        return false;

    int64 remainingMoney = _GetMemberRemainingMoney(player->GetGUID());
    if (!remainingMoney)
        return false;

    if (remainingMoney > 0 && remainingMoney < (int64)amount)
        return false;

    // Call script after validation and before money transfer.
    sScriptMgr->OnGuildMemberWitdrawMoney(this, player, amount, repair);

    // Update remaining money amount
    if (remainingMoney > 0 && remainingMoney < uint64(GUILD_WITHDRAW_MONEY_UNLIMITED))
        if (Member* member = GetMember(player->GetGUID()))
            member->DecreaseBankRemainingValue(GUILD_BANK_MAX_TABS, amount);
    // Remove money from bank
    _ModifyBankMoney(amount, false);
    // Add money to player (if required)
    if (!repair)
        player->ModifyMoney(amount);

    // Log guild bank event
    _LogBankEvent(repair ? GUILD_BANK_LOG_REPAIR_MONEY : GUILD_BANK_LOG_WITHDRAW_MONEY, uint8(0), player->GetGUIDLow(), amount);

    SendMoneyInfo(session);

    if (!repair)
        SendBankList(session, 0, false, false, false);

    return true;
}

void Guild::HandleMemberLogout(WorldSession* session)
{
    Player* player = session->GetPlayer();
    if (Member* member = GetMember(player->GetGUID()))
    {
        member->UpdateLogoutTime();
        member->SaveGuildMember();
    }

    SendGuildEventOnline(player->GetGUID(), player->GetName(), false);
}

void Guild::HandleDisband(WorldSession* session)
{
    // Only leader can disband guild
    if (!_IsLeader(session->GetPlayer()))
        Guild::SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_PERMISSIONS);
    else if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_UNDELETABLE_LEVEL))
        Guild::SendCommandResult(session, GUILD_INVITE_S, ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
    else
    {
        Disband();
        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Guild Successfully Disbanded");
    }
}

void Guild::HandleGuildPartyRequest(WorldSession* session)
{
    Player* player = session->GetPlayer();
    Group* group = player->GetGroup();

    // Make sure player is a member of the guild and that he is in a group.
    if (!IsMember(player->GetGUID()) || !group)
        return;

    WorldPacket data(SMSG_GUILD_PARTY_STATE_RESPONSE, 13);
    data.WriteBit(player->GetMap()->GetOwnerGuildId(player->GetTeam()) == GetId());     // Is guild group
    data << uint32(0);                                                                  // Current guild members
    data << uint32(0);                                                                  // Needed guild members
    data << float(0.f);                                                                 // Guild XP multiplier

    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent (SMSG_GUILD_PARTY_STATE_RESPONSE)");
}

///////////////////////////////////////////////////////////////////////////////
// Send data to client
void Guild::SendEventLog(WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_EVENT_LOG_QUERY_RESULT, 1 + m_eventLog->GetSize() * (1 + 8 + 4));
    m_eventLog->WritePacket(data);
    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_EVENT_LOG_QUERY_RESULT)");
}

void Guild::SendBankLog(WorldSession* session, uint8 tabId) const
{
    // GUILD_BANK_MAX_TABS send by client for money log
    if (tabId < GetPurchasedTabsSize() || tabId == GUILD_BANK_MAX_TABS)
    {
        LogHolder::GuildLog const* log = m_bankEventLog[tabId]->GetLog();
        WorldPacket data(SMSG_GUILD_BANK_LOG_QUERY_RESULTS, log->size() * (4 * 4 + 1) + 1 + 1);

        ByteBuffer buffer;
        data.WriteBits(log->size(), 21);
        for (LogHolder::GuildLog::const_iterator itr = log->begin(); itr != log->end(); ++itr)
            (*itr)->WritePacket(data, buffer);

        data.WriteBit(GetLevel() >= 5 && tabId == GUILD_BANK_MAX_TABS);     // has Cash Flow perk

        data.FlushBits();
        data.append(buffer);

        if (GetLevel() >= 5 && tabId == GUILD_BANK_MAX_TABS)//tabId == GUILD_BANK_MAX_TABS && hasCashFlow)
            data << uint64(0);//cashFlowContribution);
        data << uint32(tabId);

        session->SendPacket(&data);
        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_BANK_LOG_QUERY_RESULT) for tab %u", tabId);
    }
}

void Guild::SendBankList(WorldSession* session, uint8 tabId, bool withContent, bool withTabInfo, bool fullUpdate) const
{
    ByteBuffer tabData;
    WorldPacket data(SMSG_GUILD_BANK_QUERY_RESULTS);

    data << int32(_GetMemberRemainingSlots(session->GetPlayer()->GetGUID(), 0));
    data << uint64(m_bankMoney);
    data << uint32(tabId);

    data.WriteBits(withTabInfo ? GetPurchasedTabsSize() : 0, 21);

    if (withTabInfo)
    {
        for (uint8 i = 0; i < GetPurchasedTabsSize(); ++i)
        {
            data.WriteBits(m_bankTabs[i]->GetName().length(), 7);
            data.WriteBits(m_bankTabs[i]->GetIcon().length(), 9);

            tabData << uint32(i);
            tabData.WriteString(m_bankTabs[i]->GetName());
            tabData.WriteString(m_bankTabs[i]->GetIcon());
        }
    }

    uint32 itemCount = 0;
    uint32 bitpos = data.bitwpos();
    data.WriteBits(itemCount, 18);

    if (withContent && _MemberHasTabRights(session->GetPlayer()->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
    {
        if (BankTab const* tab = GetBankTab(tabId))
        {
            for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
            {
                if (Item* tabItem = tab->GetItem(slotId))
                {
                    ++itemCount;

                    tabData << uint32(tabItem->GetItemSuffixFactor());      // SuffixFactor
                    tabData << uint32(0);
                    tabData << uint32(tabItem->GetItemRandomPropertyId());
                    tabData << uint32(0);
                    tabData << uint32(slotId);
                    tabItem->AppendDynamicInfo(tabData);

                    uint32 socketEnchants = 0;
                    uint32 socketIndex = 0;
                    // only sockets
                    for (uint32 enchSlot = SOCK_ENCHANTMENT_SLOT; enchSlot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++enchSlot)
                    {
                        if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(enchSlot)))
                        {
                            tabData << uint32(enchantId);
                            tabData << uint32(socketIndex);
                            ++socketEnchants;
                        }

                        ++socketIndex;
                    }

                    data.WriteBits(socketEnchants, 21);
                    data.WriteBit(tabItem->IsLocked());                     // Locked

                    tabData << uint32(0);
                    tabData << uint32(abs(tabItem->GetSpellCharges()));     // Spell charges
                    tabData << uint32(tabItem->GetEntry());
                    tabData << uint32(tabItem->GetCount());                 // ITEM_FIELD_STACK_COUNT
                }
            }
        }
    }

    //data.WriteBit(fullUpdate);             // fullUpdate
    data.WriteBit(0);

    data.FlushBits();
    if (!tabData.empty())
        data.append(tabData);

    data.PutBits(bitpos, itemCount, 18);

    session->SendPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::SendBankTabText(WorldSession* session, uint8 tabId) const
{
     if (BankTab const* tab = GetBankTab(tabId))
        tab->SendText(this, session);
}

void Guild::SendPermissions(WorldSession* session) const
{
    uint64 guid = session->GetPlayer()->GetGUID();
    uint32 rankId = session->GetPlayer()->GetRank();

    WorldPacket data(SMSG_GUILD_PERMISSIONS_QUERY_RESULTS, 4 * 15 + 1);
    data << uint32(rankId);
    data << int32(_GetMemberRemainingMoney(guid));
    data << uint32(_GetRankRights(rankId));
    data << uint32(GetPurchasedTabsSize());

    data.WriteBits(GUILD_BANK_MAX_TABS, 21);
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        data << uint32(_GetRankBankTabRights(rankId, tabId));
        data << int32(_GetMemberRemainingSlots(guid, tabId));
    }

    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_PERMISSIONS_QUERY_RESULTS)");
}

void Guild::SendMoneyInfo(WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_BANK_MONEY_WITHDRAWN, 4);
    // -1 in 64bit is 0xFFFFFFFFFFFFFFFF not 0xFFFFFFFF, 
    // so transform uint32 0xFFFFFFFF to -1 and then send -1 as uint64
    // Now _GetMemberRemainingMoney return int value no need double convertation
    data << int64(_GetMemberRemainingMoney(session->GetPlayer()->GetGUID()));
    session->SendPacket(&data);
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent SMSG_GUILD_BANK_MONEY_WITHDRAWN");
}

void Guild::SendLoginInfo(WorldSession* session)
{
    /*
        Login sequence:
          SMSG_GUILD_EVENT - GE_MOTD
          SMSG_GUILD_RANK
          SMSG_GUILD_EVENT - GE_SIGNED_ON
          -- learn perks
          SMSG_GUILD_REPUTATION_WEEKLY_CAP
          SMSG_GUILD_ACHIEVEMENT_DATA
          SMSG_GUILD_MEMBER_DAILY_RESET // bank withdrawal reset
    */

    SendMotd(session);

    HandleGuildRanks(session);

    SendGuildEventOnline(session->GetPlayer()->GetGUID(), session->GetPlayer()->GetName(), true);
    // Send to self separately, player is not in world yet and is not found by _BroadcastEvent
    SendGuildEventOnline(session->GetPlayer()->GetGUID(), session->GetPlayer()->GetName(), true, session);

    WorldPacket data(SMSG_GUILD_MEMBER_DAILY_RESET, 0); // tells the client to request bank withdrawal limit
    session->SendPacket(&data);

    if (!sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        return;

    for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
        if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
            if (entry->Level <= GetLevel())
                session->GetPlayer()->learnSpell(entry->SpellId, true);

    SendGuildReputationWeeklyCap(session);

    GetAchievementMgr().SendAllAchievementData(session->GetPlayer());
}

void Guild::SendGuildReputationWeeklyCap(WorldSession* session) const
{
    if (Member const* member = GetMember(session->GetPlayer()->GetGUID()))
    {
        WorldPacket data(SMSG_GUILD_REPUTATION_WEEKLY_CAP, 4);
        data << uint32(member->GetWeekReputation());
        session->SendPacket(&data);
    }
}

void Guild::SendGuildChallengesInfo(WorldSession* session) const
{
    // Types: 1 - dungeon, 2 - raid, 3 - rated bg, 4 - scenario, 5 - challenge dungeon
    if (Member const* member = GetMember(session->GetPlayer()->GetGUID()))
    {
        uint32 gold[5] = { 250, 1000, 500, 100, 400 };
        uint32 goldMaxLevel[5] = { 0, 0, 0, 0, 0 };
        uint32 count[5] = { 0, 0, 0, 0, 0 };
        uint32 maxCount[5] = { 7, 1, 3, 15, 3 };
        uint32 XP[5] = { 250000, 1000000, 400000, 150000, 500000 };

        WorldPacket data(SMSG_GUILD_CHALLENGE_UPDATED);
        for (uint8 i = 0; i < 5; ++i)
            data << uint32(gold[i]);
        for (uint8 i = 0; i < 5; ++i)
            data << uint32(goldMaxLevel[i]);
        for (uint8 i = 0; i < 5; ++i)
            data << uint32(count[i]);
        for (uint8 i = 0; i < 5; ++i)
            data << uint32(XP[i]);
        for (uint8 i = 0; i < 5; ++i)
            data << uint32(maxCount[i]);

        session->SendPacket(&data);
    }
}

void Guild::SendGuildChallengeComplete(WorldSession* session) const
{
    if (Member const* member = GetMember(session->GetPlayer()->GetGUID()))
    {
        WorldPacket data(SMSG_GUILD_CHALLENGE_COMPLETED);

        // example for dungeon
        data << uint32(/*0+*/1);              // Count
        data << uint32(1);                    // Type
        data << uint32(250000);               // XP, if exists else 0
        data << uint32(7);                    // MaxCount
        data << uint32(250);                  // Gold, if exists else 0

        session->SendPacket(&data);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Loading methods
bool Guild::LoadFromDB(Field* fields)
{
    m_id            = fields[0].GetUInt32();
    m_name          = fields[1].GetString();
    m_leaderGuid    = MAKE_NEW_GUID(fields[2].GetUInt32(), 0, HIGHGUID_PLAYER);
    m_emblemInfo.LoadFromDB(fields);
    m_info          = fields[8].GetString();
    m_motd          = fields[9].GetString();
    m_createdDate   = time_t(fields[10].GetUInt32());
    m_bankMoney     = fields[11].GetUInt64();
    _level          = fields[12].GetUInt32();
    _experience     = fields[13].GetUInt64();
    _todayExperience = fields[14].GetUInt64();

    uint8 purchasedTabs = uint8(fields[15].GetUInt64());
    if (purchasedTabs > GUILD_BANK_MAX_TABS)
        purchasedTabs = GUILD_BANK_MAX_TABS;

    m_bankTabs.resize(purchasedTabs);
    for (uint8 i = 0; i < purchasedTabs; ++i)
        m_bankTabs[i] = new BankTab(m_id, i);

    _CreateLogHolders();
    CreateKey();

    return true;
}

void Guild::LoadRankFromDB(Field* fields)
{
    RankInfo rankInfo(m_id);

    rankInfo.LoadFromDB(fields);

    m_ranks.push_back(rankInfo);
}

bool Guild::LoadMemberFromDB(Field* fields)
{
    uint32 lowguid = fields[1].GetUInt32();
    Member *member = new Member(this, MAKE_NEW_GUID(lowguid, 0, HIGHGUID_PLAYER), fields[2].GetUInt8());
    if (!member->LoadFromDB(fields))
    {
        DeleteMemberGuild(lowguid);
        delete member;
        return false;
    }
    member->SaveGuildMember();
    m_members[lowguid] = member;
    return true;
}

void Guild::LoadBankRightFromDB(Field* fields)
{
                                           // tabId              rights                slots
    GuildBankRightsAndSlots rightsAndSlots(fields[1].GetUInt8(), fields[3].GetUInt8(), fields[4].GetUInt32());
                                  // rankId             tabId
    _SetRankBankTabRightsAndSlots(fields[2].GetUInt8(), rightsAndSlots);
}

bool Guild::LoadEventLogFromDB(Field* fields)
{
    if (m_eventLog->CanInsert())
    {
        m_eventLog->LoadEvent(new EventLogEntry(
            m_id,                                       // guild id
            fields[1].GetUInt32(),                      // guid
            time_t(fields[6].GetUInt32()),              // timestamp
            GuildEventLogTypes(fields[2].GetUInt8()),   // event type
            fields[3].GetUInt32(),                      // player guid 1
            fields[4].GetUInt32(),                      // player guid 2
            fields[5].GetUInt8()));                     // rank
        return true;
    }
    return false;
}

bool Guild::LoadBankEventLogFromDB(Field* fields)
{
    uint8 dbTabId = fields[1].GetUInt8();
    bool isMoneyTab = (dbTabId == GUILD_BANK_MONEY_LOGS_TAB);
    if (dbTabId < GetPurchasedTabsSize() || isMoneyTab)
    {
        uint8 tabId = isMoneyTab ? uint8(GUILD_BANK_MAX_TABS) : dbTabId;
        LogHolder* pLog = m_bankEventLog[tabId];
        if (pLog->CanInsert())
        {
            uint32 guid = fields[2].GetUInt32();
            GuildBankEventLogTypes eventType = GuildBankEventLogTypes(fields[3].GetUInt8());
            if (BankEventLogEntry::IsMoneyEvent(eventType))
            {
                if (!isMoneyTab)
                {
                    sLog->outError(LOG_FILTER_GUILD, "GuildBankEventLog ERROR: MoneyEvent(LogGuid: %u, Guild: %u) does not belong to money tab (%u), ignoring...", guid, m_id, dbTabId);
                    return false;
                }
            }
            else if (isMoneyTab)
            {
                sLog->outError(LOG_FILTER_GUILD, "GuildBankEventLog ERROR: non-money event (LogGuid: %u, Guild: %u) belongs to money tab, ignoring...", guid, m_id);
                return false;
            }
            pLog->LoadEvent(new BankEventLogEntry(
                m_id,                                   // guild id
                guid,                                   // guid
                time_t(fields[8].GetUInt32()),          // timestamp
                dbTabId,                                // tab id
                eventType,                              // event type
                fields[4].GetUInt32(),                  // player guid
                fields[5].GetUInt32(),                  // item or money
                fields[6].GetUInt16(),                  // itam stack count
                fields[7].GetUInt8()));                 // dest tab id
        }
    }
    return true;
}

bool Guild::LoadBankTabFromDB(Field* fields)
{
    uint8 tabId = fields[1].GetUInt8();
    if (tabId >= GetPurchasedTabsSize())
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid tab (tabId: %u) in guild bank, skipped.", tabId);
        return false;
    }
    return m_bankTabs[tabId]->LoadFromDB(fields);
}

bool Guild::LoadBankItemFromDB(Field* fields)
{
    uint8 tabId = fields[15].GetUInt8();
    if (tabId >= GetPurchasedTabsSize())
    {
        sLog->outError(LOG_FILTER_GUILD, "Invalid tab for item (GUID: %u, id: #%u) in guild bank, skipped.",
            fields[17].GetUInt32(), fields[18].GetUInt32());
        return false;
    }
    return m_bankTabs[tabId]->LoadItemFromDB(fields);
}

// Validates guild data loaded from database. Returns false if guild should be deleted.
bool Guild::Validate()
{
    // Validate ranks data
    // GUILD RANKS represent a sequence starting from 0 = GUILD_MASTER (ALL PRIVILEGES) to max 9 (lowest privileges).
    // The lower rank id is considered higher rank - so promotion does rank-- and demotion does rank++
    // Between ranks in sequence cannot be gaps - so 0, 1, 2, 4 is impossible
    // Min ranks count is 5 and max is 10.
    bool broken_ranks = false;
    if (_GetRanksSize() < GUILD_RANKS_MIN_COUNT || _GetRanksSize() > GUILD_RANKS_MAX_COUNT)
    {
        sLog->outError(LOG_FILTER_GUILD, "Guild %u has invalid number of ranks, creating new...", m_id);
        broken_ranks = true;
    }
    else
    {
        for (uint8 rankId = 0; rankId < _GetRanksSize(); ++rankId)
        {
            RankInfo* rankInfo = GetRankInfo(rankId);
            if (rankInfo->GetId() != rankId)
            {
                sLog->outError(LOG_FILTER_GUILD, "Guild %u has broken rank id %u, creating default set of ranks...", m_id, rankId);
                broken_ranks = true;
            }
            else
                rankInfo->CreateMissingTabsIfNeeded(GetPurchasedTabsSize(), true);
        }
    }

    if (broken_ranks)
    {
        m_ranks.clear();
        _CreateDefaultGuildRanks(DEFAULT_LOCALE);
    }

    // Validate members' data
    for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->second->GetRankId() > _GetRanksSize())
            itr->second->ChangeRank(_GetLowestRankId());

    // Repair the structure of the guild.
    // If the guildmaster doesn't exist or isn't member of the guild
    // attempt to promote another member.
    Member* pLeader = GetMember(m_leaderGuid);
    if (!pLeader)
    {
        DeleteMember(m_leaderGuid);
        // If no more members left, disband guild
        if (m_members.empty())
        {
            Disband();
            return false;
        }
    }
    else if (!pLeader->IsRank(GR_GUILDMASTER))
        _SetLeaderGUID(pLeader);

    // Check config if multiple guildmasters are allowed
    if (!ConfigMgr::GetBoolDefault("Guild.AllowMultipleGuildMaster", 0))
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->GetRankId() == GR_GUILDMASTER && !itr->second->IsSamePlayer(m_leaderGuid))
                itr->second->ChangeRank(GR_OFFICER);

    _UpdateAccountsNumber();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Broadcasts
void Guild::BroadcastToGuild(WorldSession* session, bool officerOnly, const std::string& msg, uint32 language) const
{
    if (session && session->GetPlayer() && _HasRankRight(session->GetPlayer(), officerOnly ? GR_RIGHT_OFFCHATSPEAK : GR_RIGHT_GCHATSPEAK))
    {
        WorldPacket data;
        ChatHandler::FillMessageData(&data, session, officerOnly ? CHAT_MSG_OFFICER : CHAT_MSG_GUILD, language, NULL, 0, msg.c_str(), NULL);
        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (Player* player = itr->second->FindPlayer())
                if (player->GetSession() && _HasRankRight(player, officerOnly ? GR_RIGHT_OFFCHATLISTEN : GR_RIGHT_GCHATLISTEN) &&
                    !player->GetSocial()->HasIgnore(session->GetPlayer()->GetGUIDLow()))
                    player->GetSession()->SendPacket(&data);
    }
}

void Guild::BroadcastAddonToGuild(WorldSession* session, bool officerOnly, const std::string& msg, const std::string& prefix) const
{
    if (session && session->GetPlayer() && _HasRankRight(session->GetPlayer(), officerOnly ? GR_RIGHT_OFFCHATSPEAK : GR_RIGHT_GCHATSPEAK))
    {
        WorldPacket data;
        ChatHandler::FillMessageData(&data, session, officerOnly ? CHAT_MSG_OFFICER : CHAT_MSG_GUILD, LANG_ADDON, NULL, 0, msg.c_str(), NULL, prefix.c_str());
        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (Player* player = itr->second->FindPlayer())
                if (player->GetSession() && _HasRankRight(player, officerOnly ? GR_RIGHT_OFFCHATLISTEN : GR_RIGHT_GCHATLISTEN) &&
                    !player->GetSocial()->HasIgnore(session->GetPlayer()->GetGUIDLow()) &&
                    player->GetSession()->IsAddonRegistered(prefix))
                        player->GetSession()->SendPacket(&data);
    }
}

void Guild::BroadcastPacketToRank(WorldPacket* packet, uint8 rankId) const
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->second->IsRank(rankId))
            if (Player* player = itr->second->FindPlayer())
                player->GetSession()->SendPacket(packet);
}

void Guild::BroadcastPacket(WorldPacket* packet) const
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player* player = itr->second->FindPlayer())
            player->GetSession()->SendPacket(packet);
}

///////////////////////////////////////////////////////////////////////////////
// Members handling
bool Guild::AddMember(uint64 guid, uint8 rankId)
{
    uint32 lowguid = GUID_LOPART(guid);

    const CharacterNameData* nameData = sWorld->GetCharacterNameData(lowguid);
    if (!nameData)
        return false;

    Player* player = ObjectAccessor::FindPlayer(guid);
    // Player cannot be in guild
    if (player)
    {
        if (player->GetGuildId() != 0)
            return false;
    }
    else if (nameData->m_guildId != 0)
            return false;

    // Remove all player signs from another petitions
    // This will be prevent attempt to join many guilds and corrupt guild data integrity
    Player::RemovePetitionsAndSigns(guid, GUILD_CHARTER_TYPE);

    // If rank was not passed, assign lowest possible rank
    if (rankId == GUILD_RANK_NONE)
        rankId = _GetLowestRankId();

    Member* member = new Member(this, guid, rankId);
    if (player)
    {
        member->SetStats(player);
        player->SetInGuild(m_id);
        player->SetGuild(this);
        player->SetRank(member->GetRankId());
        player->SetGuildLevel(GetLevel());
    }
    else
    {
        bool ok = false;
        // Player must exist
        member->SetStats(
            nameData->m_name,
            nameData->m_level,
            nameData->m_class,
            nameData->m_zoneId,
            nameData->m_accountId,
            0,
            nameData->m_gender,
            0, 0, 0, 0, "", 0, 0, 0, "");     // ach points and professions set on first login

        ok = member->CheckStats();
        if (!ok)
        {
            delete member;
            return false;
        }
    }
    m_members[lowguid] = member;

    member->SaveGuildMember();
    // If player not in game data in will be loaded from guild tables, so no need to update it!
    if (player)
    {
        player->SetInGuild(m_id);
        player->SetRank(rankId);
        player->SetGuildLevel(GetLevel());
        player->SetGuildIdInvited(0);
        if (sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        {
            for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
                if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
                    if (entry->Level <= GetLevel())
                        player->learnSpell(entry->SpellId, true);
        }

        if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(REP_GUILD))
            player->GetReputationMgr().SetReputation(factionEntry, 0);

        player->SavePlayerGuild();
    }

    m_accountsNumber.insert(nameData->m_accountId);

    // Call scripts if member was succesfully added (and stored to database)
    sScriptMgr->OnGuildAddMember(this, player, rankId);

    return true;
}

void Guild::DeleteMember(uint64 guid, bool isDisbanding, bool isKicked)
{
    uint32 lowguid = GUID_LOPART(guid);
    Player* player = ObjectAccessor::FindPlayer(guid);

    // Guild master can be deleted when loading guild and guid doesn't exist in characters table
    // or when he is removed from guild by gm command
    if (m_leaderGuid == guid && !isDisbanding)
    {
        Member* oldLeader = NULL;
        Member* newLeader = NULL;
        for (Guild::Members::iterator i = m_members.begin(); i != m_members.end(); ++i)
        {
            if (i->first == lowguid)
                oldLeader = i->second;
            else if (!newLeader || newLeader->GetRankId() > i->second->GetRankId())
                newLeader = i->second;
        }
        if (!newLeader)
        {
            Disband();
            return;
        }

        _SetLeaderGUID(newLeader);

        // If player not online data in data field will be loaded from guild tabs no need to update it !!
        if (Player* newLeaderPlayer = newLeader->FindPlayer())
            newLeaderPlayer->SetRank(GR_GUILDMASTER);

        // If leader does not exist (at guild loading with deleted leader) do not send broadcasts
        if (oldLeader)
        {
            SendGuildEventLeader(newLeader->GetGUID(), newLeader->GetName(), oldLeader->GetGUID(), oldLeader->GetName());
            SendGuildEventRemoveMember(guid, oldLeader->GetName().c_str());
        }
    }
    // Call script on remove before member is acutally removed from guild (and database)
    sScriptMgr->OnGuildRemoveMember(this, player, isDisbanding, isKicked);

    if (Member* member = GetMember(guid))
        delete member;
    m_members.erase(lowguid);

    // If player not online data in data field will be loaded from guild tabs no need to update it !!
    if (player)
    {
        player->SetInGuild(0);
        player->SetRank(0);
        player->SetGuildLevel(0);
        for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
            if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
                player->removeSpell(entry->SpellId, false, false);

        if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(REP_GUILD))
            player->GetReputationMgr().SetReputation(factionEntry, 0);

        player->SavePlayerSpells();
        player->SavePlayerGuild();
        player->SetGuild(NULL);
    }

    DeleteMemberGuild(lowguid);
    if (!isDisbanding)
        _UpdateAccountsNumber();
}

bool Guild::ChangeMemberRank(uint64 guid, uint8 newRank)
{
    if (newRank <= _GetLowestRankId())                    // Validate rank (allow only existing ranks)
        if (Member* member = GetMember(guid))
        {
            member->ChangeRank(newRank);
            return true;
        }
    return false;
}

bool Guild::IsMember(uint64 guid)
{
    Members::const_iterator itr = m_members.find(GUID_LOPART(guid));
    return itr != m_members.end();
}

///////////////////////////////////////////////////////////////////////////////
// Bank (items move)
void Guild::SwapItems(Player* player, uint8 tabId, uint8 slotId, uint8 destTabId, uint8 destSlotId, uint32 splitedAmount)
{
    if (tabId >= GetPurchasedTabsSize() || slotId >= GUILD_BANK_MAX_SLOTS ||
        destTabId >= GetPurchasedTabsSize() || destSlotId >= GUILD_BANK_MAX_SLOTS)
        return;

    if (tabId == destTabId && slotId == destSlotId)
        return;

    BankMoveItemData from(this, player, tabId, slotId);
    BankMoveItemData to(this, player, destTabId, destSlotId);
    _MoveItems(&from, &to, splitedAmount);
}

void Guild::SwapItemsWithInventory(Player* player, bool toChar, uint8 tabId, uint8 slotId, uint8 playerBag, uint8 playerSlotId, uint32 splitedAmount)
{
    if ((slotId >= GUILD_BANK_MAX_SLOTS && slotId != NULL_SLOT) || tabId >= GetPurchasedTabsSize())
        return;

    BankMoveItemData bankData(this, player, tabId, slotId);
    PlayerMoveItemData charData(this, player, playerBag, playerSlotId);
    if (toChar)
        _MoveItems(&bankData, &charData, splitedAmount);
    else
        _MoveItems(&charData, &bankData, splitedAmount);
}

///////////////////////////////////////////////////////////////////////////////
// Bank tabs
void Guild::SetBankTabText(uint8 tabId, const std::string& text)
{
    if (BankTab* pTab = GetBankTab(tabId))
    {
        pTab->SetText(text);
        UpdateGuildBankTab(pTab);
        SendGuildEventTabTextChanged(tabId);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
void Guild::_CreateLogHolders()
{
    m_eventLog = new LogHolder(m_id, sWorld->getIntConfig(CONFIG_GUILD_EVENT_LOG_COUNT));
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
        m_bankEventLog[tabId] = new LogHolder(m_id, sWorld->getIntConfig(CONFIG_GUILD_BANK_EVENT_LOG_COUNT));
}

bool Guild::_CreateNewBankTab()
{
    if (GetPurchasedTabsSize() >= GUILD_BANK_MAX_TABS)
        return false;

    uint8 tabId = GetPurchasedTabsSize();                      // Next free id
    m_bankTabs.push_back(new BankTab(m_id, tabId));

    ++tabId;
    for (Ranks::iterator itr = m_ranks.begin(); itr != m_ranks.end(); ++itr)
    {
        (*itr).CreateMissingTabsIfNeeded(tabId, false);
        UpdateGuildRank(&(*itr));
    }

    SaveGuild();
    return true;
}

void Guild::_CreateDefaultGuildRanks(LocaleConstant loc)
{
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_MASTER,   loc), GR_RIGHT_ALL);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_OFFICER,  loc), GR_RIGHT_ALL);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_VETERAN,  loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_MEMBER,   loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_INITIATE, loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
}

void Guild::_CreateRank(const std::string& name, uint32 rights)
{
    if (_GetRanksSize() >= GUILD_RANKS_MAX_COUNT)
        return;

    // Ranks represent sequence 0, 1, 2, ... where 0 means guildmaster
    uint32 newRankId = _GetRanksSize();

    RankInfo info(m_id, newRankId, name, rights, 0);
    m_ranks.push_back(info);

    info.CreateMissingTabsIfNeeded(GetPurchasedTabsSize());
    UpdateGuildRank(&info);
}

// Updates the number of accounts that are in the guild
// Player may have many characters in the guild, but with the same account
void Guild::_UpdateAccountsNumber()
{
    // We use a set to be sure each element will be unique
    m_accountsNumber.clear();
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        m_accountsNumber.insert(itr->second->GetAccountId());
}

// Detects if player is the guild master.
// Check both leader guid and player's rank (otherwise multiple feature with
// multiple guild masters won't work)
bool Guild::_IsLeader(Player* player) const
{
    if (player->GetGUID() == m_leaderGuid)
        return true;
    if (const Member* member = GetMember(player->GetGUID()))
        return member->IsRank(GR_GUILDMASTER);
    return false;
}

void Guild::_DeleteBankItems(bool removeItemsFromDB)
{
    for (uint8 tabId = 0; tabId < GetPurchasedTabsSize(); ++tabId)
    {
        m_bankTabs[tabId]->Delete(removeItemsFromDB);
        delete m_bankTabs[tabId];
        m_bankTabs[tabId] = NULL;
    }
    m_bankTabs.clear();
}

bool Guild::_ModifyBankMoney(uint64 amount, bool add)
{
    if (add)
        m_bankMoney += amount;
    else
    {
        // Check if there is enough money in bank.
        if (m_bankMoney < amount)
            return false;
        m_bankMoney -= amount;
    }

    SaveGuildMoney();

    return true;
}

void Guild::_SetLeaderGUID(Member* pLeader)
{
    if (!pLeader)
        return;

    m_leaderGuid = pLeader->GetGUID();
    pLeader->ChangeRank(GR_GUILDMASTER);

    SaveGuild();
}

void Guild::_SetRankBankMoneyPerDay(uint32 rankId, uint32 moneyPerDay)
{
    if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->IsRank(rankId))
                itr->second->ResetMoneyTime();

        rankInfo->SetBankMoneyPerDay(moneyPerDay);
    }
}

void Guild::_SetRankBankTabRightsAndSlots(uint8 rankId, GuildBankRightsAndSlots rightsAndSlots)
{
    if (rightsAndSlots.GetTabId() >= GetPurchasedTabsSize())
        return;

    if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->IsRank(rankId))
                itr->second->ResetTabTimes();

        rankInfo->SetBankTabSlotsAndRights(rightsAndSlots);
    }
}

inline std::string Guild::_GetRankName(uint32 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetName();
    return "<unknown>";
}

inline uint32 Guild::_GetRankRights(uint32 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetRights();
    return 0;
}

inline uint32 Guild::_GetRankBankMoneyPerDay(uint32 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetBankMoneyPerDay();
    return 0;
}

inline uint32 Guild::_GetRankBankTabSlotsPerDay(uint32 rankId, uint8 tabId) const
{
    if (tabId < GetPurchasedTabsSize())
        if (const RankInfo* rankInfo = GetRankInfo(rankId))
            return rankInfo->GetBankTabSlotsPerDay(tabId);
    return 0;
}

inline uint32 Guild::_GetRankBankTabRights(uint32 rankId, uint8 tabId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetBankTabRights(tabId);
    return 0;
}

inline int32 Guild::_GetMemberRemainingSlots(uint64 guid, uint8 tabId) const
{
    if (const Member* member = GetMember(guid))
        return member->GetBankRemainingValue(tabId, this);
    return 0;
}

inline int32 Guild::_GetMemberRemainingMoney(uint64 guid) const
{
    if (const Member* member = GetMember(guid))
        return member->GetBankRemainingValue(GUILD_BANK_MAX_TABS, this);
    return 0;
}

inline void Guild::_DecreaseMemberRemainingSlots(uint64 guid, uint8 tabId)
{
    // Remaining slots must be more then 0
    if (uint32 remainingSlots = _GetMemberRemainingSlots(guid, tabId))
        // Ignore guild master
        if (remainingSlots < uint32(GUILD_WITHDRAW_SLOT_UNLIMITED))
            if (Member* member = GetMember(guid))
                member->DecreaseBankRemainingValue(tabId, 1);
}

inline bool Guild::_MemberHasTabRights(uint64 guid, uint8 tabId, uint32 rights) const
{
    if (const Member* member = GetMember(guid))
    {
        // Leader always has full rights
        if (member->IsRank(GR_GUILDMASTER) || m_leaderGuid == guid)
            return true;
        return (_GetRankBankTabRights(member->GetRankId(), tabId) & rights) == rights;
    }
    return false;
}

// Add new event log record
inline void Guild::_LogEvent(GuildEventLogTypes eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank)
{
    m_eventLog->AddEvent(this, new EventLogEntry(m_id, m_eventLog->GetNextGUID(), eventType, playerGuid1, playerGuid2, newRank));

    sScriptMgr->OnGuildEvent(this, uint8(eventType), playerGuid1, playerGuid2, newRank);
}

// Add new bank event log record
void Guild::_LogBankEvent(GuildBankEventLogTypes eventType, uint8 tabId, uint32 lowguid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId)
{
    if (tabId > GUILD_BANK_MAX_TABS)
        return;

     // not logging moves within the same tab
    if (eventType == GUILD_BANK_LOG_MOVE_ITEM && tabId == destTabId)
        return;

    uint8 dbTabId = tabId;
    if (BankEventLogEntry::IsMoneyEvent(eventType))
    {
        tabId = GUILD_BANK_MAX_TABS;
        dbTabId = GUILD_BANK_MONEY_LOGS_TAB;
    }
    LogHolder* pLog = m_bankEventLog[tabId];
    pLog->AddEventBank(this, new BankEventLogEntry(m_id, pLog->GetNextGUID(), eventType, dbTabId, lowguid, itemOrMoney, itemStackCount, destTabId));

    sScriptMgr->OnGuildBankEvent(this, uint8(eventType), tabId, lowguid, itemOrMoney, itemStackCount, destTabId);
}

inline Item* Guild::_GetItem(uint8 tabId, uint8 slotId) const
{
    if (const BankTab* tab = GetBankTab(tabId))
        return tab->GetItem(slotId);
    return NULL;
}

inline void Guild::_RemoveItem(uint8 tabId, uint8 slotId)
{
    if (BankTab* pTab = GetBankTab(tabId))
        pTab->SetItem(slotId, NULL);
}

void Guild::_MoveItems(MoveItemData* pSrc, MoveItemData* pDest, uint32 splitedAmount)
{
    // 1. Initialize source item
    if (!pSrc->InitItem())
        return; // No source item

    // 2. Check source item
    if (!pSrc->CheckItem(splitedAmount))
        return; // Source item or splited amount is invalid
    /*
    if (pItemSrc->GetCount() == 0)
    {
        sLog->outFatal(LOG_FILTER_GENERAL, "Guild::SwapItems: Player %s(GUIDLow: %u) tried to move item %u from tab %u slot %u to tab %u slot %u, but item %u has a stack of zero!",
            player->GetName(), player->GetGUIDLow(), pItemSrc->GetEntry(), tabId, slotId, destTabId, destSlotId, pItemSrc->GetEntry());
        //return; // Commented out for now, uncomment when it's verified that this causes a crash!!
    }
    // */

    // 3. Check destination rights
    if (!pDest->HasStoreRights(pSrc))
        return; // Player has no rights to store item in destination

    // 4. Check source withdraw rights
    if (!pSrc->HasWithdrawRights(pDest))
        return; // Player has no rights to withdraw items from source

    // 5. Check split
    if (splitedAmount)
    {
        // 5.1. Clone source item
        if (!pSrc->CloneItem(splitedAmount))
            return; // Item could not be cloned

        if(pSrc->GetItem()->GetEntry() == 38186)
            sLog->outDebug(LOG_FILTER_EFIR, "_MoveItems - CloneItem of item %u; splitedAmount = %u playerGUID %u, guild %u", pSrc->GetItem()->GetEntry(), splitedAmount, GetId(), pSrc->GetItem()->GetGUID());

        // 5.2. Move splited item to destination
        _DoItemsMove(pSrc, pDest, true, splitedAmount);
    }
    else // 6. No split
    {
        // 6.1. Try to merge items in destination (pDest->GetItem() == NULL)
        if (!_DoItemsMove(pSrc, pDest, false)) // Item could not be merged
        {
            // 6.2. Try to swap items
            // 6.2.1. Initialize destination item
            if (!pDest->InitItem())
                return;

            // 6.2.2. Check rights to store item in source (opposite direction)
            if (!pSrc->HasStoreRights(pDest))
                return; // Player has no rights to store item in source (opposite direction)

            if (!pDest->HasWithdrawRights(pSrc))
                return; // Player has no rights to withdraw item from destination (opposite direction)

            // 6.2.3. Swap items (pDest->GetItem() != NULL)
            _DoItemsMove(pSrc, pDest, true);
        }
    }

    // 7. Send changes
    SendGuildEventBankSlotChanged();    //call client to send list
    _SendBankContentUpdate(pSrc, pDest);   //after SendGuildEventBankSlotChanged client ask for double update, but we will send full
}

bool Guild::_DoItemsMove(MoveItemData* pSrc, MoveItemData* pDest, bool sendError, uint32 splitedAmount)
{
    Item* pDestItem = pDest->GetItem();
    bool swap = (pDestItem != NULL);

    Item* pSrcItem = pSrc->GetItem(splitedAmount);
    // 1. Can store source item in destination
    if (!pDest->CanStore(pSrcItem, swap, sendError))
        return false;

    // 2. Can store destination item in source
    if (swap)
        if (!pSrc->CanStore(pDestItem, true, true))
            return false;

    // GM LOG (TODO: move to scripts)
    pDest->LogAction(pSrc);
    if (swap)
        pSrc->LogAction(pDest);

    // 3. Log bank events
    pDest->LogBankEvent(pSrc, pSrcItem->GetCount());
    if (swap)
        pSrc->LogBankEvent(pDest, pDestItem->GetCount());

    // 4. Remove item from source
    pSrc->RemoveItem(pDest, splitedAmount);

    // 5. Remove item from destination
    if (swap)
        pDest->RemoveItem(pSrc);

    // 6. Store item in destination
    pDest->StoreItem(pSrcItem);

    // 7. Store item in source
    if (swap)
        pSrc->StoreItem(pDestItem);

    return true;
}

void Guild::_SendBankContentUpdate(MoveItemData* pSrc, MoveItemData* pDest) const
{
    ASSERT(pSrc->IsBank() || pDest->IsBank());

    uint8 tabId = 0;
    SlotIds slots;
    if (pSrc->IsBank()) // B ->
    {
        tabId = pSrc->GetContainer();
        slots.insert(pSrc->GetSlotId());
        if (pDest->IsBank()) // B -> B
        {
            // Same tab - add destination slots to collection
            if (pDest->GetContainer() == pSrc->GetContainer())
                pDest->CopySlots(slots);
            else // Different tabs - send second message
            {
                SlotIds destSlots;
                pDest->CopySlots(destSlots);
                _SendBankContentUpdate(pDest->GetContainer(), destSlots);
            }
        }
    }
    else if (pDest->IsBank()) // C -> B
    {
        tabId = pDest->GetContainer();
        pDest->CopySlots(slots);
    }

    _SendBankContentUpdate(tabId, slots);
}

void Guild::_SendBankContentUpdate(uint8 tabId, SlotIds slots) const
{
    if (BankTab const* tab = GetBankTab(tabId))
    {
        ByteBuffer tabData;
        WorldPacket data(SMSG_GUILD_BANK_QUERY_RESULTS);
        size_t rempos = data.wpos();
        data << uint32(-1);                                      // Item withdraw amount, will be filled later
        data << uint64(m_bankMoney);
        data << uint32(tabId);

        data.WriteBits(0, 21);                                                      // Tab count
        data.WriteBits(slots.size(), 18);                                           // Item count

        for (SlotIds::const_iterator itr = slots.begin(); itr != slots.end(); ++itr)
        {
            Item const* tabItem = tab->GetItem(*itr);

            tabData << uint32(tabItem ? tabItem->GetItemSuffixFactor() : 0);        // SuffixFactor
            tabData << uint32(0);
            tabData << uint32(tabItem ? tabItem->GetItemRandomPropertyId() : 0);
            tabData << uint32(0);
            tabData << uint32(*itr);

            if (tabItem)
                tabItem->AppendDynamicInfo(tabData);
            else
            {
                tabData << uint32(4);
                tabData << uint32(0);
            }

            uint32 enchantCount = 0;

            if (tabItem)
            {
                uint32 socketIndex = 0;
                // only sockets
                for (uint32 enchSlot = SOCK_ENCHANTMENT_SLOT; enchSlot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++enchSlot)
                {
                    if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(enchSlot)))
                    {
                        tabData << uint32(enchantId);
                        tabData << uint32(socketIndex);
                        ++enchantCount;
                    }

                    ++socketIndex;
                }
            }

            data.WriteBits(enchantCount, 21);                                       // enchantment count
            data.WriteBit(tabItem ? tabItem->IsLocked() : 0);                       // Locked

            tabData << uint32(0);
            tabData << uint32(tabItem ? abs(tabItem->GetSpellCharges()) : 0);       // Spell charges
            tabData << uint32(tabItem ? tabItem->GetEntry() : 0);
            tabData << uint32(tabItem ? tabItem->GetCount() : 0);                   // ITEM_FIELD_STACK_COUNT
        }

        data.WriteBit(0);               // fullUpdate

        data.FlushBits();
        if (!tabData.empty())
            data.append(tabData);

        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (_MemberHasTabRights(itr->second->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
                if (Player* player = itr->second->FindPlayer())
                {
                    data.put<int32>(rempos, _GetMemberRemainingSlots(player->GetGUID(), tabId));
                    player->GetSession()->SendPacket(&data);
                }

        sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent (SMSG_GUILD_BANK_LIST)");
    }
}

void Guild::_BroadcastEvent(GuildEvents guildEvent, uint64 guid, const char* param1, const char* param2, const char* param3) const
{
    uint8 count = !param3 ? (!param2 ? (!param1 ? 0 : 1) : 2) : 3;

    WorldPacket data(SMSG_GUILD_EVENT, 1 + 1 + count + (guid ? 8 : 0));
    data << uint8(guildEvent);
    data << uint8(count);

    if (param3)
        data << param1 << param2 << param3;
    else if (param2)
        data << param1 << param2;
    else if (param1)
        data << param1;

    if (guid)
        data << uint64(guid);

    BroadcastPacket(&data);

    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent SMSG_GUILD_EVENT");
}

void Guild::SendGuildRanksUpdate(uint64 setterGuid, uint64 targetGuid, uint32 rank)
{
    ObjectGuid tarGuid = targetGuid;
    ObjectGuid setGuid = setterGuid;

    Member* member = GetMember(targetGuid);
    ASSERT(member);

    WorldPacket data(SMSG_GUILD_RANKS_UPDATE, 4 + 8 + 8 + 1 + 1 + 1);
    data.WriteGuidMask<0, 6>(setterGuid);
    data.WriteGuidMask<7>(targetGuid);
    data.WriteGuidMask<3>(setterGuid);
    data.WriteGuidMask<4, 3>(targetGuid);
    data.WriteGuidMask<5>(setterGuid);
    data.WriteGuidMask<2>(targetGuid);
    data.WriteGuidMask<4>(setterGuid);
    data.WriteGuidMask<0>(targetGuid);
    data.WriteGuidMask<1, 2>(setterGuid);
    data.WriteGuidMask<6, 1>(targetGuid);
    data.WriteBit(rank < member->GetRankId()); // 1 == promoted, 0 == demoted
    data.WriteGuidMask<7>(setterGuid);
    data.WriteGuidMask<5>(targetGuid);

    data.WriteGuidBytes<1>(setterGuid);
    data << uint32(rank);
    data.WriteGuidBytes<4, 1>(targetGuid);
    data.WriteGuidBytes<2>(setterGuid);
    data.WriteGuidBytes<5>(targetGuid);
    data.WriteGuidBytes<7, 6, 5, 0>(setterGuid);
    data.WriteGuidBytes<0, 3>(targetGuid);
    data.WriteGuidBytes<4>(setterGuid);
    data.WriteGuidBytes<7, 2, 6>(targetGuid);
    data.WriteGuidBytes<3>(setterGuid);

    BroadcastPacket(&data);

    member->ChangeRank(rank);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Sent SMSG_GUILD_RANKS_UPDATE");
}

void Guild::GiveXP(uint32 xp, Player* source)
{
    if (!sWorld->getBoolConfig(CONFIG_GUILD_LEVELING_ENABLED))
        return;

    /// @TODO: Award reputation and count activity for player

    if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_MAX_LEVEL))
        xp = 0; // SMSG_GUILD_XP_GAIN is always sent, even for no gains

    //if (GetLevel() >= GUILD_EXPERIENCE_UNCAPPED_LEVEL)
        //xp = std::min(xp, sWorld->getIntConfig(CONFIG_GUILD_DAILY_XP_CAP) - uint32(_todayExperience));

    WorldPacket data(SMSG_GUILD_XP_GAIN, 8);
    data << uint64(xp);    // XP missing for next level
    source->GetSession()->SendPacket(&data);

    _experience += xp;
    _todayExperience += xp;

    if (!xp)
        return;

    uint32 oldLevel = GetLevel();

    // Ding, mon!
    while (GetExperience() >= sGuildMgr->GetXPForGuildLevel(GetLevel()) && GetLevel() < sWorld->getIntConfig(CONFIG_GUILD_MAX_LEVEL))
    {
        _experience -= sGuildMgr->GetXPForGuildLevel(GetLevel());
        ++_level;

        // Find all guild perks to learn
        std::vector<uint32> perksToLearn;
        for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
            if (GuildPerkSpellsEntry const* entry = sGuildPerkSpellsStore.LookupEntry(i))
                if (entry->Level > oldLevel && entry->Level <= GetLevel())
                    perksToLearn.push_back(entry->SpellId);

        // Notify all online players that guild level changed and learn perks
        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)

        {
            if (Player* player = itr->second->FindPlayer())
            {
                player->SetGuildLevel(GetLevel());
                for (size_t i = 0; i < perksToLearn.size(); ++i)
                    player->learnSpell(perksToLearn[i], true);
            }
        }
    
        GetNewsLog().AddNewEvent(GUILD_NEWS_LEVEL_UP, time(NULL), 0, 0, _level);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL, GetLevel(), 0, 0, NULL, source);

        ++oldLevel;
    }
}

uint32 Guild::RepGainedBy(Player* player, uint32 amount)
{
    if (!amount || !player)
        return 0;

    Member* member = GetMember(player->GetGUID());
    if(!member)
        return 0;

    amount = std::min(amount, uint32(GUILD_WEEKLY_REP_CAP - member->GetWeekReputation()));
    
    // Guild Champion Tabard        
    Unit::AuraEffectList const& auras = player->GetAuraEffectsByType(SPELL_AURA_MOD_REPUTATION_GAIN_PCT);
    for (Unit::AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
        AddPct(amount, (*i)->GetAmount());

    if (amount)
    {
        member->RepEarned(player, amount);
        member->SaveGuildMember();
    }

    return amount;
}

void Guild::Member::RepEarned(Player* player, uint32 value)
{
    FactionEntry const* entry = sFactionStore.LookupEntry(REP_GUILD);
    if(!entry || !player)
        return;

    AddReputation(value);
    player->GetReputationMgr().ModifyReputation(entry, int32(value));

    SendGuildReputationWeeklyCap(player->GetSession(), GetWeekReputation());
}

void Guild::Member::SendGuildReputationWeeklyCap(WorldSession* session, uint32 reputation) const
{
    uint32 cap = sWorld->getIntConfig(CONFIG_GUILD_WEEKLY_REP_CAP) - reputation;
    WorldPacket data(SMSG_GUILD_REPUTATION_WEEKLY_CAP, 4);
    data << uint32(cap);
    session->SendPacket(&data);
}

void Guild::SendGuildXP(WorldSession* session) const
{
    Member const* member = GetMember(session->GetGuidLow());

    WorldPacket data(SMSG_GUILD_XP, 40);
    uint64 xpMissing = sGuildMgr->GetXPForGuildLevel(GetLevel()) >= GetExperience() ?
        sGuildMgr->GetXPForGuildLevel(GetLevel()) : GetExperience();    // XP missing for next level
    data << uint64(xpMissing);
    data << uint64(GetTodayExperience());
    data << uint64(GetTodayExperience());
    data << uint64(GetExperience());
    session->SendPacket(&data);
}

void Guild::ResetDailyExperience()
{
    _todayExperience = 0;

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player* player = itr->second->FindPlayer())
            SendGuildXP(player->GetSession());
}

void Guild::ResetWeek()
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        itr->second->ResetValues();
}

void Guild::GuildNewsLog::AddNewEvent(GuildNews eventType, time_t date, uint64 playerGuid, uint32 flags, uint32 data)
{
    uint32 id = GetNewId();
    GuildNewsEntry& log = _newsLog[id];
    log.EventType = eventType;
    log.PlayerGuid = GUID_LOPART(playerGuid);
    log.Data = data;
    log.Flags = flags;
    log.Date = date;

    _guild->UpdateGuildNewsLog(&log, id);

    WorldPacket packet;
    BuildNewsData(id, log, packet);
    GetGuild()->BroadcastPacket(&packet);
}

void Guild::GuildNewsLog::LoadFromDB(PreparedQueryResult result)
{
    if (!result)
        return;
    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetInt32();
        GuildNewsEntry& log = _newsLog[id];
        log.EventType = GuildNews(fields[1].GetInt32());
        log.PlayerGuid = fields[2].GetInt32();
        log.Data = fields[3].GetInt32();
        log.Flags = fields[4].GetInt32();
        log.Date = time_t(fields[5].GetInt32());
    }
    while (result->NextRow());

    Shrink();
}

void Guild::GuildNewsLog::Shrink()
{
    while (_newsLog.size() > MAX_GUILD_NEWS_ENTRIES)
        _newsLog.erase(_newsLog.begin());
}

uint32 Guild::GuildNewsLog::GetNewId() const
{
    return !_newsLog.empty() ? _newsLog.rbegin()->first : 0;
}

void Guild::GuildNewsLog::BuildNewsData(uint32 id, GuildNewsEntry& guildNew, WorldPacket& data)
{
    data.Initialize(SMSG_GUILD_NEWS_UPDATE, (21 + _newsLog.size() * (26 + 8)) / 8 + (8 + 6 * 4) * _newsLog.size());
    data.WriteBits(1, 19);

    ObjectGuid guid = MAKE_NEW_GUID(guildNew.PlayerGuid, 0, HIGHGUID_PLAYER);

    data.WriteGuidMask<5, 2, 0, 3>(guid);
    data.WriteBits(0, 24); // Not yet implemented used for guild achievements
    data.WriteGuidMask<1, 6, 4, 7>(guid);

    data << uint32(secsToTimeBitFields(guildNew.Date));
    data.WriteGuidBytes<1>(guid);
    data << uint32(guildNew.Flags);   // 1 sticky
    data.WriteGuidBytes<7, 6, 2>(guid);
    data << uint32(0);                  // always 0
    data << uint32(guildNew.EventType);
    data.WriteGuidBytes<3, 0, 5>(guid);
    data << uint32(id);
    data << uint32(guildNew.Data);
    data.WriteGuidBytes<4>(guid);
}

void Guild::GuildNewsLog::BuildNewsData(WorldPacket& data)
{
    data.Initialize(SMSG_GUILD_NEWS_UPDATE, 7 + 32);
    data.WriteBits(_newsLog.size(), 19); // size, we are only sending 1 news here

    for (GuildNewsLogMap::const_iterator it = _newsLog.begin(); it != _newsLog.end(); ++it)
    {
        ObjectGuid guid = MAKE_NEW_GUID(it->second.PlayerGuid, 0, HIGHGUID_PLAYER);

        data.WriteGuidMask<5, 2, 0, 3>(guid);
        data.WriteBits(0, 24); // Not yet implemented used for guild achievements
        data.WriteGuidMask<1, 6, 4, 7>(guid);
    }

    for (GuildNewsLogMap::const_iterator it = _newsLog.begin(); it != _newsLog.end(); ++it)
    {
        ObjectGuid guid = MAKE_NEW_GUID(it->second.PlayerGuid, 0, HIGHGUID_PLAYER);

        data << uint32(secsToTimeBitFields(it->second.Date));
        data.WriteGuidBytes<1>(guid);
        data << uint32(it->second.Flags);   // 1 sticky
        data.WriteGuidBytes<7, 6, 2>(guid);
        data << uint32(0);                  // always 0
        data << uint32(it->second.EventType);
        data.WriteGuidBytes<3, 0, 5>(guid);
        data << uint32(it->first);
        data << uint32(it->second.Data);
        data.WriteGuidBytes<4>(guid);
    }
}

void Guild::SendMotd(WorldSession* session)
{
    sLog->outDebug(LOG_FILTER_GUILD, "WORLD: Sent guild MOTD (SMSG_GUILD_EVENT_MOTD)");

    WorldPacket data(SMSG_GUILD_EVENT_MOTD, m_motd.size() + 2);
    data.WriteBits(m_motd.size(), 10);
    data.WriteString(m_motd);

    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);
}

void Guild::SendGuildEventJoinMember(uint64 guid, std::string name)
{
    WorldPacket data(SMSG_GUILD_EVENT_JOIN_MEMBER, 8 + 1 + 4 + name.size() + 1);
    data.WriteBits(name.size(), 6);
    data.WriteGuidMask<5, 4, 7, 1, 3, 0, 6, 2>(guid);

    data.WriteGuidBytes<2, 6, 0, 1, 3, 4, 7>(guid);
    data << uint32(realmID);
    data.WriteGuidBytes<5>(guid);
    data.WriteString(name);

    BroadcastPacket(&data);
}

void Guild::SendGuildEventRemoveMember(uint64 guid, std::string name, uint64 kickerGuid, std::string kickerName)
{
    bool kicked = kickerGuid != 0;

    WorldPacket data(SMSG_GUILD_EVENT_REMOVE_MEMBER, 8 + 8 + 1 + 1 + name.size() + 1 + kickerName.size() + 1 + 1 + 4 + 4);
    data.WriteGuidMask<4, 1, 0>(guid);
    data.WriteBit(kicked);
    data.WriteGuidMask<3>(guid);
    if (kicked)
    {
        data.WriteBit(0);       // has kicker realm id
        data.WriteBit(0);       // has kicker name
        data.WriteBit(0);       // has kicker guid
        data.WriteBits(kickerName.size(), 6);
        data.WriteGuidMask<6, 5, 2, 7, 3, 0, 1, 4>(kickerGuid);
    }
    data.WriteGuidMask<7>(guid);
    data.WriteBits(name.size(), 6);
    data.WriteGuidMask<6, 2, 5>(guid);
    if (kicked)
    {
        data.WriteGuidBytes<1, 7, 3, 5, 0, 6, 2, 4>(kickerGuid);
        data << uint32(realmID);
        data.WriteString(kickerName);
    }
    data.WriteGuidBytes<0, 7>(guid);
    data.WriteString(name);
    data.WriteGuidBytes<3, 1, 5, 2>(guid);
    data << uint32(realmID);
    data.WriteGuidBytes<6, 4>(guid);

    BroadcastPacket(&data);
}

void Guild::SendGuildEventLeader(uint64 guid, std::string name, uint64 oldGuid, std::string oldName)
{
    WorldPacket data(SMSG_GUILD_EVENT_LEADER, 1 + 1 + 1 + 1 + 8 + 8 + oldName.size() + name.size());
    data.WriteBits(name.size(), 6);
    data.WriteGuidMask<7>(oldGuid);
    data.WriteGuidMask<5, 3>(guid);
    data.WriteBits(oldName.size(), 6);
    data.WriteGuidMask<1, 5>(oldGuid);
    data.WriteBit(0);           // 1 - replaced, 0 - changed
    data.WriteGuidMask<4, 6>(oldGuid);
    data.WriteGuidMask<7>(guid);
    data.WriteGuidMask<3, 0>(oldGuid);
    data.WriteGuidMask<6, 2, 0, 1>(guid);
    data.WriteGuidMask<2>(oldGuid);
    data.WriteGuidMask<4>(guid);

    data.WriteGuidBytes<7>(oldGuid);
    data.WriteString(oldName);
    data.WriteGuidBytes<1>(oldGuid);
    data.WriteGuidBytes<2, 1>(guid);
    data.WriteGuidBytes<2>(oldGuid);
    data.WriteGuidBytes<6>(guid);
    data.WriteGuidBytes<5>(oldGuid);
    data.WriteGuidBytes<7>(guid);
    data.WriteGuidBytes<6>(oldGuid);
    data.WriteGuidBytes<5>(guid);
    data.WriteGuidBytes<3>(oldGuid);
    data.WriteString(name);
    data.WriteGuidBytes<0, 3>(guid);
    data.WriteGuidBytes<4, 0>(oldGuid);
    data << uint32(realmID);        // old leader realm id
    data << uint32(realmID);        // new leader realm id
    data.WriteGuidBytes<4>(guid);

    BroadcastPacket(&data);
}

void Guild::SendGuildEventDisbanded()
{
    WorldPacket data(SMSG_GUILD_EVENT_GUILD_DISBANDED, 0);
    BroadcastPacket(&data);
}

void Guild::SendGuildEventRankUpdate(uint32 rankId)
{
    if (rankId)
    {
        WorldPacket data(SMSG_GUILD_EVENT_RANK_UPDATE, 4);
        data << uint32(rankId);
        BroadcastPacket(&data);
    }
    else
    {
        WorldPacket data(SMSG_GUILD_EVENT_RANKS_UPDATE, 0);
        BroadcastPacket(&data);
    }
}

void Guild::SendGuildEventOnline(uint64 guid, std::string name, bool online, WorldSession* session)
{
    WorldPacket data(SMSG_GUILD_EVENT_ONLINE, 8 + 1 + name.size() + 1 + 1 + 4);
    data.WriteGuidMask<1>(guid);
    data.WriteBit(online);
    data.WriteGuidMask<7, 6>(guid);
    data.WriteBits(name.size(), 6);
    data.WriteBit(0);               // 0 - normal chat, 1 - remote chat
    data.WriteGuidMask<4, 3, 2, 5, 0>(guid);

    data.WriteGuidBytes<5, 0, 6, 4, 3>(guid);
    data << uint32(realmID);
    data.WriteGuidBytes<2, 7>(guid);
    data.WriteString(name);
    data.WriteGuidBytes<1>(guid);

    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);
}

void Guild::SendGuildEventTabTextChanged(uint32 tabId, WorldSession* session)
{
    WorldPacket data(SMSG_GUILD_EVENT_GUILDBANK_TEXT_CHANGED, 4);
    data << uint32(tabId);
    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);
}

void Guild::SendGuildEventBankSlotChanged()
{
    WorldPacket data(SMSG_GUILD_EVENT_GUILDBANKBAGSLOTS_CHANGED, 0);
    BroadcastPacket(&data);
}

void Guild::KnownRecipes::GenerateMask(uint32 skillId, std::set<uint32> const& spells)
{
    Clear();

    uint32 index = 0;
    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const* entry = sSkillLineAbilityStore.LookupEntry(i);
        if (!entry)
            continue;

        if (entry->skillId != skillId)
            continue;

        ++index;
        if (spells.find(entry->spellId) == spells.end())
            continue;

        if (index / 8 > KNOW_RECIPES_MASK_SIZE)
            break;

        recipesMask[index / 8] |= 1 << (index % 8);
    }
}

std::string Guild::KnownRecipes::GetMaskForSave() const
{
    std::stringstream ss;
    for (uint32 i = 0; i < KNOW_RECIPES_MASK_SIZE; ++i)
        ss << uint32(recipesMask[i]) << " ";

    return ss.str();
}

void Guild::KnownRecipes::LoadFromString(std::string const& str)
{
    Clear();

    Tokenizer tok(str, ' ');
    for (uint32 i = 0; i < tok.size(); ++i)
        recipesMask[i] = atoi(tok[i]);
}

bool Guild::KnownRecipes::IsEmpty() const
{
    for (uint32 i = 0; i < KNOW_RECIPES_MASK_SIZE; ++i)
        if (recipesMask[i])
            return false;

    return true;
}

void Guild::UpdateGuildRecipes(uint32 skillId)
{
    if (skillId)
        _guildRecipes[skillId].Clear();
    else
        _guildRecipes.clear();

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
        {
            Guild::Member::ProfessionInfo const& info = itr->second->GetProfessionInfo(i);

            if (info.skillId && (!skillId || info.skillId == skillId))
                for (uint32 j = 0; j < KNOW_RECIPES_MASK_SIZE; ++j)
                    _guildRecipes[info.skillId].recipesMask[j] |= info.knownRecipes.recipesMask[j];
        }
    }
}

void Guild::AddGuildRecipes(Guild::Member::ProfessionInfo const* info)
{
    for (uint32 j = 0; j < KNOW_RECIPES_MASK_SIZE; ++j)
        _guildRecipes[info->skillId].recipesMask[j] |= info->knownRecipes.recipesMask[j];
}

void Guild::SendGuildMembersForRecipeResponse(WorldSession* session, uint32 skillId, uint32 spellId)
{
    uint32 index = 0;
    bool found = false;
    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const* entry = sSkillLineAbilityStore.LookupEntry(i);
        if (!entry)
            continue;

        if (entry->skillId != skillId)
            continue;

        ++index;
        if (entry->spellId == spellId)
        {
            found = true;
            break;
        }
    }

    if (!found || index / 8 > KNOW_RECIPES_MASK_SIZE)
        return;

    std::set<uint64> guids;
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
        {
            Member::ProfessionInfo const& info = itr->second->GetProfessionInfo(i);
            if (info.skillId != skillId)
                continue;

            if (info.knownRecipes.recipesMask[index / 8] & (1 << (index % 8)))
                guids.insert(itr->second->GetGUID());
        }
    }

    WorldPacket* data = new WorldPacket(SMSG_GUILD_MEMBERS_FOR_RECIPE, 4 + 4 + 3 + guids.size() * 9);
    *data << uint32(skillId);
    *data << uint32(spellId);
    data->WriteBits(guids.size(), 24);
    for (std::set<uint64>::const_iterator itr = guids.begin(); itr != guids.end(); ++itr)
        data->WriteGuidMask<2, 0, 1, 6, 7, 5, 3, 4>(*itr);
    for (std::set<uint64>::const_iterator itr = guids.begin(); itr != guids.end(); ++itr)
        data->WriteGuidBytes<1, 2, 7, 4, 6, 3, 5, 0>(*itr);

    session->GetPlayer()->ScheduleMessageSend(data, 500);
}

void Guild::SendGuildMemberRecipesResponse(WorldSession* session, ObjectGuid playerGuid, uint32 skillId)
{
    Member* member = GetMember(playerGuid);
    if (!member)
        return;

    for (uint32 i = 0; i < MAX_GUILD_PROFESSIONS; ++i)
    {
        Member::ProfessionInfo const& info = member->GetProfessionInfo(i);
        if (info.skillId == skillId)
        {
            WorldPacket* data = new WorldPacket(SMSG_GUILD_MEMBER_RECIPES, 8 + 1 + 4 * 3 + 300);
            data->WriteGuidMask<0, 7, 5, 2, 4, 1, 6, 3>(playerGuid);

            *data << uint32(info.skillValue);
            data->WriteGuidBytes<7, 1, 6>(playerGuid);
            data->append(info.knownRecipes.recipesMask, KNOW_RECIPES_MASK_SIZE);
            *data << uint32(info.skillRank);
            data->WriteGuidBytes<4, 0>(playerGuid);
            *data << uint32(info.skillId);
            data->WriteGuidBytes<5, 3, 2>(playerGuid);

            session->GetPlayer()->ScheduleMessageSend(data, 500);
            return;
        }
    }
}

void Guild::CreateKey()
{
    guildKey = new char[32];
    guildMemberKey = new char[32];
    criteriaKey = new char[32];
    itemKey = new char[32];
    sprintf(guildKey, "r{%u}g{%u}", realmID, m_id);
    sprintf(guildMemberKey, "r{%u}g{%u}member", realmID, m_id);
    sprintf(criteriaKey, "r{%u}g{%u}crit", realmID, m_id);
    sprintf(itemKey, "r{%i}g{%i}items", realmID, m_id);
}
