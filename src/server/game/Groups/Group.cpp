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
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GroupMgr.h"
#include "Group.h"
#include "Formulas.h"
#include "ObjectAccessor.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "MapManager.h"
#include "InstanceSaveMgr.h"
#include "MapInstanced.h"
#include "Util.h"
#include "LFGMgr.h"
#include "UpdateFieldFlags.h"
#include "GuildMgr.h"
#include "Bracket.h"

Roll::Roll(uint64 _guid, LootItem const& li) : itemGUID(_guid), itemid(li.itemid),
    itemRandomPropId(li.randomPropertyId), itemRandomSuffix(li.randomSuffix), itemCount(li.count),
    totalPlayersRolling(0), totalNeed(0), totalGreed(0), totalPass(0), itemSlot(0),
    rollVoteMask(ROLL_ALL_TYPE_NO_DISENCHANT), aoeSlot(0)
{
}

Roll::~Roll()
{
}

void Roll::setLoot(Loot* pLoot)
{
    link(pLoot, this);
}

Loot* Roll::getLoot()
{
    return getTarget();
}

Group::Group() : m_leaderGuid(0), m_leaderName(""), m_groupType(GROUPTYPE_NORMAL),
    m_dungeonDifficulty(REGULAR_DIFFICULTY), m_raidDifficulty(MAN10_DIFFICULTY),
    m_bgGroup(NULL), m_bfGroup(NULL), m_lootMethod(FREE_FOR_ALL), m_lootThreshold(ITEM_QUALITY_UNCOMMON), m_looterGuid(0),
    m_subGroupsCounts(NULL), m_guid(0), m_counter(0), m_maxEnchantingLevel(0), m_dbStoreId(0), m_readyCheckCount(0), m_readyCheck(false),
    m_aoe_slots(0)
{
    for (uint8 i = 0; i < TARGETICONCOUNT; ++i)
        m_targetIcons[i] = 0;
}

Group::~Group()
{
    if (m_bgGroup)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Group::~Group: battleground group being deleted.");
        if (m_bgGroup->GetBgRaid(ALLIANCE) == this) m_bgGroup->SetBgRaid(ALLIANCE, NULL);
        else if (m_bgGroup->GetBgRaid(HORDE) == this) m_bgGroup->SetBgRaid(HORDE, NULL);
        else sLog->outError(LOG_FILTER_GENERAL, "Group::~Group: battleground group is not linked to the correct battleground.");
    }
    Rolls::iterator itr;
    while (!RollId.empty())
    {
        itr = RollId.begin();
        Roll *r = *itr;
        RollId.erase(itr);
        delete(r);
    }

    // it is undefined whether objectmgr (which stores the groups) or instancesavemgr
    // will be unloaded first so we must be prepared for both cases
    // this may unload some instance saves
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        for (BoundInstancesMap::iterator itr2 = m_boundInstances[i].begin(); itr2 != m_boundInstances[i].end(); ++itr2)
            itr2->second.save->RemoveGroup(this);

    // Sub group counters clean up
    delete[] m_subGroupsCounts;
}

bool Group::Create(Player* leader)
{
    uint64 leaderGuid = leader->GetGUID();
    uint32 lowguid = sGroupMgr->GenerateGroupId();

    m_guid = MAKE_NEW_GUID(lowguid, 0, HIGHGUID_GROUP);
    m_leaderGuid = leaderGuid;
    m_leaderName = leader->GetName();

    m_groupType  = (isBGGroup() || isBFGroup()) ? GROUPTYPE_BGRAID : GROUPTYPE_NORMAL;

    if (m_groupType & GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    m_lootMethod = GROUP_LOOT;
    m_lootThreshold = ITEM_QUALITY_UNCOMMON;
    m_looterGuid = leaderGuid;

    m_dungeonDifficulty = REGULAR_DIFFICULTY;
    m_raidDifficulty = MAN10_DIFFICULTY;

    if (!isBGGroup() && !isBFGroup())
    {
        m_dungeonDifficulty = leader->GetDungeonDifficulty();
        m_raidDifficulty = leader->GetRaidDifficulty();

        m_dbStoreId = sGroupMgr->GenerateNewGroupDbStoreId();

        sGroupMgr->RegisterGroupDbStoreId(m_dbStoreId, this);

        // Store group in database
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GROUP);

        uint8 index = 0;

        stmt->setUInt32(index++, m_dbStoreId);
        stmt->setUInt32(index++, GUID_LOPART(m_leaderGuid));
        stmt->setUInt8(index++, uint8(m_lootMethod));
        stmt->setUInt32(index++, GUID_LOPART(m_looterGuid));
        stmt->setUInt8(index++, uint8(m_lootThreshold));
        stmt->setUInt32(index++, uint32(m_targetIcons[0]));
        stmt->setUInt32(index++, uint32(m_targetIcons[1]));
        stmt->setUInt32(index++, uint32(m_targetIcons[2]));
        stmt->setUInt32(index++, uint32(m_targetIcons[3]));
        stmt->setUInt32(index++, uint32(m_targetIcons[4]));
        stmt->setUInt32(index++, uint32(m_targetIcons[5]));
        stmt->setUInt32(index++, uint32(m_targetIcons[6]));
        stmt->setUInt32(index++, uint32(m_targetIcons[7]));
        stmt->setUInt8(index++, uint8(m_groupType));
        stmt->setUInt32(index++, uint8(m_dungeonDifficulty));
        stmt->setUInt32(index++, uint8(m_raidDifficulty));

        CharacterDatabase.Execute(stmt);


        ASSERT(AddMember(leader)); // If the leader can't be added to a new group because it appears full, something is clearly wrong.

        if (!isLFGGroup())
            Player::ConvertInstancesToGroup(leader, this, false);
    }
    else if (!AddMember(leader))
        return false;

    return true;
}

void Group::LoadGroupFromDB(Field* fields)
{
    m_dbStoreId = fields[15].GetUInt32();
    m_guid = MAKE_NEW_GUID(sGroupMgr->GenerateGroupId(), 0, HIGHGUID_GROUP);
    m_leaderGuid = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);

    // group leader not exist
    if (!ObjectMgr::GetPlayerNameByGUID(fields[0].GetUInt32(), m_leaderName))
        return;

    m_lootMethod = LootMethod(fields[1].GetUInt8());
    m_looterGuid = MAKE_NEW_GUID(fields[2].GetUInt32(), 0, HIGHGUID_PLAYER);
    m_lootThreshold = ItemQualities(fields[3].GetUInt8());

    for (uint8 i = 0; i < TARGETICONCOUNT; ++i)
        m_targetIcons[i] = fields[4+i].GetUInt32();

    m_groupType  = GroupType(fields[12].GetUInt8());
    if (m_groupType & GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    uint32 diff = fields[13].GetUInt8();
    if (!IsValidDifficulty(diff, false))
        m_dungeonDifficulty = REGULAR_DIFFICULTY;
    else
        m_dungeonDifficulty = Difficulty(diff);

    uint32 r_diff = fields[14].GetUInt8();
    if (!IsValidDifficulty(r_diff, true))
        m_raidDifficulty = MAN10_DIFFICULTY;
    else
        m_raidDifficulty = Difficulty(r_diff);

    if (m_groupType & GROUPTYPE_LFG)
        sLFGMgr->_LoadFromDB(fields, GetGUID());
}

void Group::LoadMemberFromDB(uint32 guidLow, uint8 memberFlags, uint8 subgroup, uint8 roles)
{
    MemberSlot member;
    member.guid = MAKE_NEW_GUID(guidLow, 0, HIGHGUID_PLAYER);

    // skip non-existed member
    if (!ObjectMgr::GetPlayerNameByGUID(member.guid, member.name))
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_MEMBER);
        stmt->setUInt32(0, guidLow);
        CharacterDatabase.Execute(stmt);
        return;
    }

    member.group = subgroup;
    member.flags = memberFlags;
    member.roles = roles;

    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(subgroup);

    sLFGMgr->SetupGroupMember(member.guid, GetGUID());
}

void Group::ChangeFlagEveryoneAssistant(bool apply)
{
    if (apply)
        m_groupType = GroupType(m_groupType | GROUPTYPE_EVERYONE_IS_ASSISTANT);
    else
        m_groupType = GroupType(m_groupType &~ GROUPTYPE_EVERYONE_IS_ASSISTANT);

    this->SendUpdate();
}

void Group::ConvertToLFG(lfg::LFGDungeonData const* dungeon)
{
    if (dungeon->dbc->subType == LFG_SUBTYPE_FLEX || dungeon->dbc->subType == LFG_SUBTYPE_RAID)
        ConvertToRaid(false);

    m_groupType = GroupType(m_groupType | GROUPTYPE_LFG | GROUPTYPE_UNK1);
    m_lootMethod = NEED_BEFORE_GREED;
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_TYPE);

        stmt->setUInt8(0, uint8(m_groupType));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    SendUpdate();
}

void Group::ConvertToRaid(bool update)
{
    m_groupType = GroupType(m_groupType | GROUPTYPE_RAID);

    _initRaidSubGroupsCounter();

    if (update && !isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_TYPE);

        stmt->setUInt8(0, uint8(m_groupType));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    if (update)
        SendUpdate();

    // update quest related GO states (quest activity dependent from raid membership)
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
        if (Player* player = ObjectAccessor::FindPlayer(citr->guid))
            player->UpdateForQuestWorldObjects();
}

void Group::ConvertToGroup()
{
    if (m_memberSlots.size() > 5)
        return; // What message error should we send?

    m_groupType = GroupType(GROUPTYPE_NORMAL);

    if (m_subGroupsCounts)
    {
        delete[] m_subGroupsCounts;
        m_subGroupsCounts = NULL;
    }

    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_TYPE);

        stmt->setUInt8(0, uint8(m_groupType));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    SendUpdate();

    // update quest related GO states (quest activity dependent from raid membership)
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
        if (Player* player = ObjectAccessor::FindPlayer(citr->guid))
            player->UpdateForQuestWorldObjects();
}

bool Group::AddInvite(Player* player)
{
    if (!player || player->GetGroupInvite())
        return false;
    Group* group = player->GetGroup();
    if (group && (group->isBGGroup() || group->isBFGroup()))
        group = player->GetOriginalGroup();
    if (group)
        return false;

    RemoveInvite(player);

    m_invitees.insert(player);

    player->SetGroupInvite(this);

    sScriptMgr->OnGroupInviteMember(this, player->GetGUID());

    return true;
}

bool Group::AddLeaderInvite(Player* player)
{
    if (!AddInvite(player))
        return false;

    m_leaderGuid = player->GetGUID();
    m_leaderName = player->GetName();
    return true;
}

void Group::RemoveInvite(Player* player)
{
    if (player)
    {
        if(!m_invitees.empty())
            m_invitees.erase(player);
        player->SetGroupInvite(NULL);
    }
}

void Group::RemoveAllInvites()
{
    for (InvitesList::iterator itr=m_invitees.begin(); itr != m_invitees.end(); ++itr)
        if (*itr)
            (*itr)->SetGroupInvite(NULL);

    m_invitees.clear();
}

Player* Group::GetInvited(uint64 guid) const
{
    for (InvitesList::const_iterator itr = m_invitees.begin(); itr != m_invitees.end(); ++itr)
    {
        if ((*itr) && (*itr)->GetGUID() == guid)
            return (*itr);
    }
    return NULL;
}

Player* Group::GetInvited(const std::string& name) const
{
    for (InvitesList::const_iterator itr = m_invitees.begin(); itr != m_invitees.end(); ++itr)
    {
        if ((*itr) && (*itr)->GetName() == name)
            return (*itr);
    }
    return NULL;
}

bool Group::AddCreatureMember(Creature* creature)
{
    uint8 subGroup = 0;
    if (m_subGroupsCounts)
    {
        bool groupFound = false;
        for (; subGroup < MAX_RAID_SUBGROUPS; ++subGroup)
        {
            if (m_subGroupsCounts[subGroup] < MAXGROUPSIZE)
            {
                groupFound = true;
                break;
            }
        }
        if (!groupFound)
            return false;
    }

    MemberSlot member;
    member.guid      = creature->GetGUID();
    member.name      = creature->GetName();
    member.group     = subGroup;
    member.flags     = 0;
    member.roles     = 0;
    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(subGroup);
    SendUpdate();
    return true;
}

bool Group::AddMember(Player* player)
{
    // Get first not-full group
    uint8 subGroup = 0;
    if (m_subGroupsCounts)
    {
        bool groupFound = false;
        for (; subGroup < MAX_RAID_SUBGROUPS; ++subGroup)
        {
            if (m_subGroupsCounts[subGroup] < MAXGROUPSIZE)
            {
                groupFound = true;
                break;
            }
        }
        // We are raid group and no one slot is free
        if (!groupFound)
            return false;
    }

    MemberSlot member;
    member.guid      = player->GetGUID();
    member.name      = player->GetName();
    member.group     = subGroup;
    member.flags     = 0;
    member.roles     = 0;
    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(subGroup);

    if (player)
    {
        player->SetGroupInvite(NULL);
        if (player->GetGroup())
        {
            if (isBGGroup() || isBFGroup()) // if player is in group and he is being added to BG raid group, then call SetBattlegroundRaid()
                player->SetBattlegroundOrBattlefieldRaid(this, subGroup);
            else //if player is in bg raid and we are adding him to normal group, then call SetOriginalGroup()
                player->SetOriginalGroup(this, subGroup);
        }
        else //if player is not in group, then call set group
            player->SetGroup(this, subGroup);

        // if the same group invites the player back, cancel the homebind timer
        InstanceGroupBind* bind = GetBoundInstance(player);
        if (bind && bind->save->GetInstanceId() == player->GetInstanceId())
            player->m_InstanceValid = true;
    }

    if (!isRaidGroup())                                      // reset targetIcons for non-raid-groups
    {
        for (uint8 i = 0; i < TARGETICONCOUNT; ++i)
            m_targetIcons[i] = 0;
    }

    // insert into the table if we're not a battleground group
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GROUP_MEMBER);

        stmt->setUInt32(0, m_dbStoreId);
        stmt->setUInt32(1, GUID_LOPART(member.guid));
        stmt->setUInt8(2, member.flags);
        stmt->setUInt8(3, member.group);
        stmt->setUInt8(4, member.roles);

        CharacterDatabase.Execute(stmt);

    }

    SendUpdate();

    if (player)
    {
        sScriptMgr->OnGroupAddMember(this, player->GetGUID());

        if (!IsLeader(player->GetGUID()) && !isBGGroup() && !isBFGroup())
        {
            // reset the new member's instances, unless he is currently in one of them
            // including raid/heroic instances that they are not permanently bound to!
            player->ResetInstances(INSTANCE_RESET_GROUP_JOIN, false);
            player->ResetInstances(INSTANCE_RESET_GROUP_JOIN, true);

            if (player->getLevel() >= LEVELREQUIREMENT_HEROIC)
            {
                if (player->GetDungeonDifficulty() != GetDungeonDifficulty())
                {
                    player->SetDungeonDifficulty(GetDungeonDifficulty());
                    player->SendDungeonDifficulty();
                }
                if (player->GetRaidDifficulty() != GetRaidDifficulty())
                {
                    player->SetRaidDifficulty(GetRaidDifficulty());
                    player->SendRaidDifficulty();
                }
            }
        }
        player->SetGroupUpdateFlag(GROUP_UPDATE_FULL);
        UpdatePlayerOutOfRange(player);

        // quest related GO state dependent from raid membership
        if (isRaidGroup())
            player->UpdateForQuestWorldObjects();
        player->UpdateForRaidMarkers(this);

        {
            // Broadcast new player group member fields to rest of the group
            player->SetFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);

            UpdateData groupData(player->GetMapId());
            WorldPacket groupDataPacket;

            // Broadcast group members' fields to player
            for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (itr->getSource() == player)
                    continue;

                if (Player* member = itr->getSource())
                {
                    if (player->HaveAtClient(member))   // must be on the same map, or shit will break
                    {
                        member->SetFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);
                        member->BuildValuesUpdateBlockForPlayer(&groupData, player);
                        member->RemoveFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);
                    }

                    if (member->HaveAtClient(player))
                    {
                        UpdateData newData(player->GetMapId());
                        WorldPacket newDataPacket;
                        player->BuildValuesUpdateBlockForPlayer(&newData, member);
                        if (newData.HasData())
                        {
                            newData.BuildPacket(&newDataPacket);
                            member->SendDirectMessage(&newDataPacket);
                        }
                    }
                }
            }

            if (groupData.HasData())
            {
                groupData.BuildPacket(&groupDataPacket);
                player->SendDirectMessage(&groupDataPacket);
            }

            player->RemoveFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);
        }

        if (m_maxEnchantingLevel < player->GetSkillValue(SKILL_ENCHANTING))
            m_maxEnchantingLevel = player->GetSkillValue(SKILL_ENCHANTING);
    }

    return true;
}

bool Group::RemoveCreatureMember(uint64 guid)
{
    if (!guid)
        return false;

    BroadcastGroupUpdate();

    member_witerator slot = _getMemberWSlot(guid);
    if (slot != m_memberSlots.end())
    {
        SubGroupCounterDecrease(slot->group);
        m_memberSlots.erase(slot);
    }

    SendUpdate();

    return true;
}

bool Group::RemoveMember(uint64 guid, const RemoveMethod &method /*= GROUP_REMOVEMETHOD_DEFAULT*/, uint64 kicker /*= 0*/, const char* reason /*= NULL*/)
{
    BroadcastGroupUpdate();

    sScriptMgr->OnGroupRemoveMember(this, guid, method, kicker, reason);

    // LFG group vote kick handled in scripts
    if (isLFGGroup() && method == GROUP_REMOVEMETHOD_KICK)
        return m_memberSlots.size();

    // remove member and change leader (if need) only if strong more 2 members _before_ member remove (BG/BF allow 1 member group)
    if (GetMembersCount() > ((isBGGroup() || isLFGGroup() || isBFGroup()) ? 1u : 2u))
    {
        Player* player = ObjectAccessor::GetObjectInOrOutOfWorld(guid, (Player*)NULL);
        if (player)
        {
            // Battleground group handling
            if (isBGGroup() || isBFGroup())
                player->RemoveFromBattlegroundOrBattlefieldRaid();
            else
                // Regular group
            {
                if (player->GetOriginalGroup() == this)
                    player->SetOriginalGroup(NULL);
                else
                    player->SetGroup(NULL);

                // quest related GO state dependent from raid membership
                player->UpdateForQuestWorldObjects();
            }

            player->UpdateForRaidMarkers(this);

            if (method == GROUP_REMOVEMETHOD_KICK || method == GROUP_REMOVEMETHOD_KICK_LFG)
            {
                WorldPacket data(SMSG_GROUP_UNINVITE, 0);
                player->GetSession()->SendPacket(&data);
            }

            // Do we really need to send this opcode?
            SendEmptyParty(player);

            _homebindIfInstance(player);
        }

        // Remove player from group in DB
        if (!isBGGroup() && !isBFGroup())
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_MEMBER);
            stmt->setUInt32(0, GUID_LOPART(guid));
            CharacterDatabase.Execute(stmt);
            DelinkMember(guid);
        }

        // Reevaluate group enchanter if the leaving player had enchanting skill or the player is offline
        if ((player && player->GetSkillValue(SKILL_ENCHANTING)) || !player)
            ResetMaxEnchantingLevel();

        // Remove player from loot rolls
        for (Rolls::iterator it = RollId.begin(); it != RollId.end(); ++it)
        {
            Roll* roll = *it;
            Roll::PlayerVote::iterator itr2 = roll->playerVote.find(guid);
            if (itr2 == roll->playerVote.end())
                continue;

            if (itr2->second == GREED || itr2->second == DISENCHANT)
                --roll->totalGreed;
            else if (itr2->second == NEED)
                --roll->totalNeed;
            else if (itr2->second == PASS)
                --roll->totalPass;

            if (itr2->second != NOT_VALID)
                --roll->totalPlayersRolling;

            roll->playerVote.erase(itr2);

            CountRollVote(guid, roll->aoeSlot, MAX_ROLL_TYPE);
        }

        // Update subgroups
        member_witerator slot = _getMemberWSlot(guid);
        if (slot != m_memberSlots.end())
        {
            SubGroupCounterDecrease(slot->group);
            m_memberSlots.erase(slot);
        }

        // Pick new leader if necessary
        if (m_leaderGuid == guid)
        {
            for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
            {
                if (ObjectAccessor::FindPlayer(itr->guid))
                {
                    ChangeLeader(itr->guid);
                    break;
                }
            }
        }

        SendUpdate();

        if (isLFGGroup() && GetMembersCount() == 1)
        {
            Player* leader = ObjectAccessor::FindPlayer(GetLeaderGUID());
            uint32 mapId = sLFGMgr->GetDungeonMapId(GetGUID());
            if (!mapId || !leader || (leader->isAlive() && leader->GetMapId() != mapId))
            {
                Disband();
                return false;
            }
        }

        if (m_memberMgr.getSize() < ((isLFGGroup() || isBGGroup()) ? 1u : 2u))
            Disband();

        return true;
    }
    // If group size before player removal <= 2 then disband it
    else
    {
        Disband();
        return false;
    }
}

void Group::ChangeLeader(uint64 guid)
{
    member_witerator slot = _getMemberWSlot(guid);

    if (slot == m_memberSlots.end())
        return;

    Player* player = ObjectAccessor::FindPlayer(slot->guid);

    // Don't allow switching leader to offline players
    if (!player)
        return;

    sScriptMgr->OnGroupChangeLeader(this, m_leaderGuid, guid);

    if (!isBGGroup() && !isBFGroup())
    {
        // Remove the groups permanent instance bindings
        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end();)
            {
                if (itr->second.perm)
                {
                    itr->second.save->RemoveGroup(this);
                    m_boundInstances[i].erase(itr++);
                }
                else
                    ++itr;
            }
        }

        // Same in the database

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_PERM_BINDING);

        stmt->setUInt32(0, m_dbStoreId);
        stmt->setUInt32(1, player->GetGUIDLow());

        CharacterDatabase.Execute(stmt);

        // Copy the permanent binds from the new leader to the group
        if (!isLFGGroup())
            Player::ConvertInstancesToGroup(player, this, true);

        // Update the group leader
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_LEADER);

        stmt->setUInt32(0, player->GetGUIDLow());
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    m_leaderGuid = player->GetGUID();
    m_leaderName = player->GetName();
    ToggleGroupMemberFlag(slot, MEMBER_FLAG_ASSISTANT, false);

    //! 5.4.1
    WorldPacket data(SMSG_GROUP_SET_LEADER);
    data.WriteBits(slot->name.size(), 6);
    data.FlushBits();
    data << uint8(0);
    data << slot->name;
    BroadcastPacket(&data, true);
}

void Group::Disband(bool hideDestroy /* = false */)
{
    if(!this)
        return;

    sScriptMgr->OnGroupDisband(this);

    Player* player;
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        player = ObjectAccessor::FindPlayer(citr->guid);
        if (!player)
            continue;

        // remove all raid markers
        SetRaidMarker(RAID_MARKER_COUNT, player, 0, false);

        //we cannot call _removeMember because it would invalidate member iterator
        //if we are removing player from battleground raid
        if (isBGGroup() || isBFGroup())
            player->RemoveFromBattlegroundOrBattlefieldRaid();
        else
        {
            //we can remove player who is in battleground from his original group
            if (player->GetOriginalGroup() == this)
                player->SetOriginalGroup(NULL);
            else
                player->SetGroup(NULL);
        }

        // quest related GO state dependent from raid membership
        if (isRaidGroup())
            player->UpdateForQuestWorldObjects();

        if (!player->GetSession())
            continue;

        WorldPacket data;
        if (!hideDestroy)
        {
            data.Initialize(SMSG_GROUP_DESTROYED, 0);
            player->GetSession()->SendPacket(&data);
        }

        //we already removed player from group and in player->GetGroup() is his original group, send update
        if (Group* group = player->GetGroup())
        {
            group->SendUpdate();
        }
        else
        {
            SendEmptyParty(player);
        }

        if (!isLFGGroup())
            _homebindIfInstance(player);
    }
    RollId.clear();
    m_memberSlots.clear();

    RemoveAllInvites();

    if (!isBGGroup() && !isBFGroup())
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP);
        stmt->setUInt32(0, m_dbStoreId);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_MEMBER_ALL);
        stmt->setUInt32(0, m_dbStoreId);
        trans->Append(stmt);

        CharacterDatabase.CommitTransaction(trans);

        ResetInstances(INSTANCE_RESET_GROUP_DISBAND, false, NULL);
        ResetInstances(INSTANCE_RESET_GROUP_DISBAND, true, NULL);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_LFG_DATA);
        stmt->setUInt32(0, m_dbStoreId);
        CharacterDatabase.Execute(stmt);

        sGroupMgr->FreeGroupDbStoreId(this);
    }

    sGroupMgr->RemoveGroup(this);
    delete this;
}

/*********************************************************/
/***                   LOOT SYSTEM                     ***/
/*********************************************************/

//! 5.4.1
void Group::SendLootStartRoll(uint32 countDown, uint32 mapid, const Roll& r)
{
    //! ToDo: itemRandomPropId & itemRandomSuffix
    WorldPacket data(SMSG_LOOT_START_ROLL, (8+4+4+4+4+4+4+1));
    ObjectGuid guid = r.lootedGUID;

    data.WriteGuidMask<1, 0>(guid);
    data.WriteBits(r.TotalEmited(), 3);
    data.WriteGuidMask<3>(guid);
    data.WriteBit(1);
    data.WriteGuidMask<2, 4, 5>(guid);
    data.WriteBits(0, 2);                                           //unk
    data.WriteBit(1);
    data.WriteGuidMask<6>(guid);
    data.WriteBit(r.aoeSlot == 0);
    data.WriteGuidMask<7>(guid);

    data.FlushBits();

    data << uint8(r.rollVoteMask);                                  // roll type mask
    if(r.aoeSlot)
        data << uint8(r.aoeSlot);
    data << uint32(mapid);                                          // 3.3.3 mapid
    data << uint32(countDown);                                      // the countdown time to choose "need" or "greed"
    data.WriteGuidBytes<1>(guid);
    data << uint32(r.itemRandomSuffix);
    data.WriteGuidBytes<3>(guid);
                                                                    // Display ID
    data << uint32(sObjectMgr->GetItemTemplate(r.itemid)->DisplayInfoID);    
    data << uint32(r.itemid);                                       // the itemEntryId for the item that shall be rolled for
    data << uint32(0);
    data.WriteGuidBytes<2>(guid);
    data << uint32(r.itemRandomPropId);
    data << uint8(r.totalPlayersRolling);                           // maybe the number of players rolling for it???
    data.WriteGuidBytes<4, 0, 5>(guid);
    data << uint32(r.itemCount);                                    // items in stack
    data.WriteGuidBytes<7, 6>(guid);

    for (Roll::PlayerVote::const_iterator itr=r.playerVote.begin(); itr != r.playerVote.end(); ++itr)
    {
        Player* p = ObjectAccessor::FindPlayer(itr->first);
        if (!p || !p->GetSession())
            continue;

        if (itr->second == NOT_EMITED_YET)
            p->GetSession()->SendPacket(&data);
    }
}

void Group::SendLootStartRollToPlayer(uint32 countDown, uint32 mapId, Player* p, bool canNeed, Roll const& r)
{
    if (!p || !p->GetSession())
        return;

    //! 5.4.1
    WorldPacket data(SMSG_LOOT_START_ROLL, (8 + 4 + 4 + 4 + 4 + 4 + 4 + 1));
    ObjectGuid guid = r.lootedGUID;

    data.WriteGuidMask<1, 0>(guid);
    data.WriteBits(r.TotalEmited(), 3);
    data.WriteGuidMask<3>(guid);
    data.WriteBit(canNeed);
    data.WriteGuidMask<2, 4, 5>(guid);
    data.WriteBits(0, 2);                                           //unk
    data.WriteBit(1);
    data.WriteGuidMask<6>(guid);
    data.WriteBit(r.aoeSlot == 0);
    data.WriteGuidMask<7>(guid);

    data.FlushBits();

    data << uint8(r.rollVoteMask);                                  // roll type mask
    if(r.aoeSlot)
        data << uint8(r.aoeSlot);
    data << uint32(mapId);                                          // 3.3.3 mapid
    data << uint32(countDown);                                      // the countdown time to choose "need" or "greed"
    data.WriteGuidBytes<1>(guid);
    data << uint32(r.itemRandomSuffix);
    data.WriteGuidBytes<3>(guid);
                                                                    // Display ID
    data << uint32(sObjectMgr->GetItemTemplate(r.itemid)->DisplayInfoID);    
    data << uint32(r.itemid);                                       // the itemEntryId for the item that shall be rolled for
    data << uint32(0);
    data.WriteGuidBytes<2>(guid);
    data << uint32(r.itemRandomPropId);
    data << uint8(r.totalPlayersRolling);                           // maybe the number of players rolling for it???
    data.WriteGuidBytes<4, 0, 5>(guid);
    data << uint32(r.itemCount);                                    // items in stack
    data.WriteGuidBytes<7, 6>(guid);

    p->GetSession()->SendPacket(&data);
}

//! 5.4.1
void Group::SendLootRoll(uint64 sourceGuid, uint64 targetGuid, uint8 rollNumber, uint8 rollType, Roll const& roll)
{
    //! ToDo: itemRandomSuffix & itemRandomPropId
    WorldPacket data(SMSG_LOOT_ROLL, (8+4+8+4+4+4+1+1+1));
    ObjectGuid target = targetGuid;
    ObjectGuid guid = roll.lootedGUID;

    data.WriteBit(1);
    data.WriteGuidMask<6>(guid);
    data.WriteGuidMask<5>(target);
    data.WriteGuidMask<7>(guid);
    data.WriteGuidMask<1>(target);
    data.WriteGuidMask<2>(guid);
    data.WriteBits(0, 2);
    data.WriteBit(0);                                       // 1: "You automatically passed on: %s because you cannot loot that item." - Possibly used in need befor greed
    data.WriteGuidMask<5>(guid);
    data.WriteGuidMask<7, 0>(target);
    data.WriteGuidMask<1, 4, 3>(guid);
    data.WriteBit(0);
    data.WriteGuidMask<6>(target);
    data.WriteBit(roll.aoeSlot == 0);
    data.WriteGuidMask<2, 4>(target);
    data.WriteBits(roll.TotalEmited(), 3);
    data.WriteGuidMask<3>(target);
    data.WriteGuidMask<0>(guid);

    data.FlushBits();

    data << uint32(1);                                      // always 1
    data.WriteGuidBytes<1, 0>(target);
    if (roll.aoeSlot)
        data << uint8(roll.aoeSlot);
    data.WriteGuidBytes<4, 5>(target);
    data.WriteGuidBytes<3, 4, 7, 2, 1>(guid);
    data << uint32(0);
    data.WriteGuidBytes<5, 6>(guid);
    data << uint32(rollNumber);                             // 0: "Need for: [item name]" > 127: "you passed on: [item name]"      Roll number
    data.WriteGuidBytes<6>(target);
    data.WriteGuidBytes<0>(guid);
    data.WriteGuidBytes<7>(target);
    data << uint32(roll.itemid);                            // the itemEntryId for the item that shall be rolled for
    data << uint32(roll.itemRandomSuffix);
    data.WriteGuidBytes<2>(target);
    data << uint32(roll.itemRandomPropId);                  // randomSuffix
    data.WriteGuidBytes<3>(target);
    data << uint8(rollType);                                // 0: "Need for: [item name]" 0: "You have selected need for [item name] 1: need roll 2: greed roll
                                                            // Display ID
    data << uint32(sObjectMgr->GetItemTemplate(roll.itemid)->DisplayInfoID);      

    for (Roll::PlayerVote::const_iterator itr = roll.playerVote.begin(); itr != roll.playerVote.end(); ++itr)
    {
        Player* p = ObjectAccessor::FindPlayer(itr->first);
        if (!p || !p->GetSession())
            continue;

        if (itr->second != NOT_VALID)
            p->GetSession()->SendPacket(&data);
    }
}

//! 5.4.1
void Group::SendLootRollWon(uint64 sourceGuid, uint64 targetGuid, uint8 rollNumber, uint8 rollType, Roll const& roll)
{
    //! ToDo: itemRandomSuffix & itemRandomPropId
    WorldPacket data(SMSG_LOOT_ROLL_WON, 60);
    ObjectGuid target = targetGuid;
    ObjectGuid guid = roll.lootedGUID;
    
    data.WriteBit(roll.aoeSlot == 0);
    data.WriteGuidMask<0>(guid);
    data.WriteGuidMask<4>(target);
    data.WriteGuidMask<1>(guid);
    data.WriteGuidMask<6>(target);
    data.WriteBits(roll.TotalEmited(), 3);
    data.WriteGuidMask<0>(target);
    data.WriteGuidMask<3>(guid);
    data.WriteGuidMask<7>(target);
    data.WriteGuidMask<6>(guid);
    data.WriteGuidMask<1>(target);
    data.WriteBits(0, 2);
    data.WriteGuidMask<4>(guid);
    data.WriteGuidMask<2>(target);
    data.WriteGuidMask<2, 5>(guid);
    data.WriteGuidMask<3>(target);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteGuidMask<5>(target);
    data.WriteGuidMask<7>(guid);

    data.FlushBits();

    data.WriteGuidBytes<3>(target);
    data << uint32(roll.itemRandomSuffix);                  // Item random property ID
    data << uint32(0);
    data << uint32(1);                                      // Always 1
    data.WriteGuidBytes<6>(target);
    data.WriteGuidBytes<0, 4>(guid);
                                                           // Display ID
    data << uint32(sObjectMgr->GetItemTemplate(roll.itemid)->DisplayInfoID);   
    data.WriteGuidBytes<1, 5, 2>(guid);
    data << uint32(roll.itemid);                            // the itemEntryId for the item that shall be rolled for
    if (roll.aoeSlot)
        data << roll.aoeSlot;
    data.WriteGuidBytes<6>(guid);
    data << uint8(rollType);                                // rollType related to SMSG_LOOT_ROLL
    data.WriteGuidBytes<7, 3>(guid);
    data.WriteGuidBytes<4, 2, 1, 0>(target);
    data << uint32(roll.itemRandomPropId);                  // Item random property
    data.WriteGuidBytes<5>(target);
    data << uint32(rollNumber);                             // rollnumber realted to SMSG_LOOT_ROLL
    data.WriteGuidBytes<7>(target);

    //! 5.4.1
    WorldPacket data2(SMSG_LOOT_ROLLS_COMPLETE, 7);
    data2.WriteGuidMask<4, 7, 1, 6, 0, 5, 2, 3>(target);
    data2.WriteGuidBytes<1, 5, 0, 7>(target);
    data2 << uint8(roll.itemSlot);
    data2.WriteGuidBytes<6, 2, 4, 3>(target);

    for (Roll::PlayerVote::const_iterator itr = roll.playerVote.begin(); itr != roll.playerVote.end(); ++itr)
    {
        Player* p = ObjectAccessor::FindPlayer(itr->first);
        if (!p || !p->GetSession())
            continue;

        if (itr->second != NOT_VALID)
        {
            p->GetSession()->SendPacket(&data);
            p->GetSession()->SendPacket(&data2);
        }
    }
}

//! 5.4.1
void Group::SendLootAllPassed(Roll const& roll)
{
    WorldPacket data(SMSG_LOOT_ALL_PASSED, (8+4+4+4+4));
    ObjectGuid guid = roll.lootedGUID;

    data.WriteGuidMask<3>(guid);
    data.WriteBit(0);
    data.WriteGuidMask<6>(guid);
    data.WriteBits(0, 2);
    data.WriteGuidMask<0, 4, 5>(guid);
    data.WriteBit(1);
    data.WriteBit(roll.aoeSlot == 0);
    data.WriteBits(roll.TotalEmited(), 3);
    data.WriteGuidMask<2, 7, 1>(guid);

    data.FlushBits();

    data.WriteGuidBytes<4, 3, 2, 0>(guid);
    data << uint32(roll.itemid);                // real itemID
    data.WriteGuidBytes<6, 5>(guid);
    if (roll.aoeSlot)
        data << uint8(roll.aoeSlot);
    data.WriteGuidBytes<7>(guid);
                                                // DisplayID
    data << uint32(sObjectMgr->GetItemTemplate(roll.itemid)->DisplayInfoID);   
    data << uint32(roll.itemid);
    data << uint32(0);                          // Dynamic Info
    data << uint32(roll.itemid);
    data << uint32(roll.itemid);
    data.WriteGuidBytes<1>(guid);

    for (Roll::PlayerVote::const_iterator itr = roll.playerVote.begin(); itr != roll.playerVote.end(); ++itr)
    {
        Player* player = ObjectAccessor::FindPlayer(itr->first);
        if (!player || !player->GetSession())
            continue;

        if (itr->second != NOT_VALID)
            player->GetSession()->SendPacket(&data);
    }
}

// notify group members which player is the allowed looter for the given creature
void Group::SendLooter(Creature* creature, Player* groupLooter)
{
    ASSERT(creature);

    ObjectGuid guid = creature->GetGUID();
    ObjectGuid gGroupLooter = groupLooter ? groupLooter->GetGUID() : 0;
    ObjectGuid guid28 = 0;

    //! 5.4.1
    WorldPacket data(SMSG_LOOT_LIST, (25));

    data.WriteBit(guid28);
    data.WriteGuidMask<4>(guid);
    data.WriteBit(gGroupLooter);

    if (gGroupLooter)
        data.WriteGuidMask<4, 5, 3, 6, 1, 0, 2, 7>(gGroupLooter);

    data.WriteGuidMask<0>(guid);

    if (guid28)
        data.WriteGuidMask<6, 3, 5, 2, 7, 0, 1, 4>(guid28);

    data.WriteGuidMask<1, 2, 6, 3, 5, 7>(guid);

    data.FlushBits();

    if (guid28)
        data.WriteGuidBytes<7, 6, 5, 2, 4, 1, 3, 0>(guid28);

    if (gGroupLooter)
        data.WriteGuidBytes<5, 0, 1, 7, 3, 6, 2, 4>(gGroupLooter);

    data.WriteGuidBytes<5, 6, 7, 1, 4, 2, 0, 3>(guid);

    BroadcastPacket(&data, false);
}

void Group::GroupLoot(Loot* loot, WorldObject* pLootedObject)
{
    std::vector<LootItem>::iterator i;
    ItemTemplate const* item;
    uint8 itemSlot = 0;

    for (i = loot->items.begin(); i != loot->items.end(); ++i, ++itemSlot)
    {
        if (i->freeforall || i->currency)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);
        if (!item)
        {
            //sLog->outDebug(LOG_FILTER_GENERAL, "Group::GroupLoot: missing item prototype for item with id: %d", i->itemid);
            continue;
        }

        //roll for over-threshold item if it's one-player loot
        if (item->Quality >= uint32(m_lootThreshold))
        {
            uint64 newitemGUID = MAKE_NEW_GUID(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), 0, HIGHGUID_ITEM);
            Roll* r = new Roll(newitemGUID, *i);
            r->lootedGUID = loot->GetGUID();

            //a vector is filled with only near party members
            for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* member = itr->getSource();
                if (!member || !member->GetSession())
                    continue;
                if (i->AllowedForPlayer(member))
                {
                    if (member->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                    {
                        r->totalPlayersRolling++;

                        if (member->GetPassOnGroupLoot())
                        {
                            r->playerVote[member->GetGUID()] = PASS;
                            r->totalPass++;
                            // can't broadcast the pass now. need to wait until all rolling players are known.
                        }
                        else
                            r->playerVote[member->GetGUID()] = NOT_EMITED_YET;
                    }
                }
            }

            if (r->totalPlayersRolling > 0)
            {
                r->setLoot(loot);
                r->itemSlot = itemSlot;
                r->aoeSlot = ++m_aoe_slots;
                if (item->DisenchantID && m_maxEnchantingLevel >= item->RequiredDisenchantSkill)
                    r->rollVoteMask |= ROLL_FLAG_TYPE_DISENCHANT;

                loot->items[itemSlot].is_blocked = true;

                // If there is any "auto pass", broadcast the pass now.
                if (r->totalPass)
                {
                    for (Roll::PlayerVote::const_iterator itr=r->playerVote.begin(); itr != r->playerVote.end(); ++itr)
                    {
                        Player* p = ObjectAccessor::FindPlayer(itr->first);
                        if (!p || !p->GetSession())
                            continue;

                        if (itr->second == PASS)
                            SendLootRoll(newitemGUID, p->GetGUID(), 128, ROLL_PASS, *r);
                    }
                }

                SendLootStartRoll(60000, pLootedObject->GetMapId(), *r);

                RollId.push_back(r);

                if (Creature* creature = pLootedObject->ToCreature())
                {
                    creature->m_groupLootTimer = 60000;
                    creature->lootingGroupLowGUID = GetLowGUID();
                }
                else if (GameObject* go = pLootedObject->ToGameObject())
                {
                    go->m_groupLootTimer = 60000;
                    go->lootingGroupLowGUID = GetLowGUID();
                }
            }
            else
                delete r;
        }
        else
            i->is_underthreshold = true;
    }

    for (i = loot->quest_items.begin(); i != loot->quest_items.end(); ++i, ++itemSlot)
    {
        if (!i->follow_loot_rules)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);
        if (!item)
        {
            //sLog->outDebug(LOG_FILTER_GENERAL, "Group::GroupLoot: missing item prototype for item with id: %d", i->itemid);
            continue;
        }

        uint64 newitemGUID = MAKE_NEW_GUID(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), 0, HIGHGUID_ITEM);
        Roll* r = new Roll(newitemGUID, *i);
        r->lootedGUID = loot->GetGUID();

        //a vector is filled with only near party members
        for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || !member->GetSession())
                continue;

            if (i->AllowedForPlayer(member))
            {
                if (member->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                {
                    r->totalPlayersRolling++;
                    r->playerVote[member->GetGUID()] = NOT_EMITED_YET;
                }
            }
        }

        if (r->totalPlayersRolling > 0)
        {
            r->setLoot(loot);
            r->itemSlot = itemSlot;
            r->aoeSlot = ++m_aoe_slots;

            loot->quest_items[itemSlot - loot->items.size()].is_blocked = true;

            SendLootStartRoll(60000, pLootedObject->GetMapId(), *r);

            RollId.push_back(r);

            if (Creature* creature = pLootedObject->ToCreature())
            {
                creature->m_groupLootTimer = 60000;
                creature->lootingGroupLowGUID = GetLowGUID();
            }
            else if (GameObject* go = pLootedObject->ToGameObject())
            {
                go->m_groupLootTimer = 60000;
                go->lootingGroupLowGUID = GetLowGUID();
            }
        }
        else
            delete r;
    }
}

void Group::NeedBeforeGreed(Loot* loot, WorldObject* lootedObject)
{
    ItemTemplate const* item;
    uint8 itemSlot = 0;
    for (std::vector<LootItem>::iterator i = loot->items.begin(); i != loot->items.end(); ++i, ++itemSlot)
    {
        if (i->freeforall || i->currency)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);

        //roll for over-threshold item if it's one-player loot
        if (item->Quality >= uint32(m_lootThreshold))
        {
            uint64 newitemGUID = MAKE_NEW_GUID(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), 0, HIGHGUID_ITEM);
            Roll* r = new Roll(newitemGUID, *i);
            r->lootedGUID = loot->GetGUID();

            for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* playerToRoll = itr->getSource();
                if (!playerToRoll || !playerToRoll->GetSession())
                    continue;

                bool allowedForPlayer = i->AllowedForPlayer(playerToRoll);
                if (allowedForPlayer && playerToRoll->IsWithinDistInMap(lootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                {
                    r->totalPlayersRolling++;
                    if (playerToRoll->GetPassOnGroupLoot())
                    {
                        r->playerVote[playerToRoll->GetGUID()] = PASS;
                        r->totalPass++;
                        // can't broadcast the pass now. need to wait until all rolling players are known.
                    }
                    else
                        r->playerVote[playerToRoll->GetGUID()] = NOT_EMITED_YET;
                }
            }

            if (r->totalPlayersRolling > 0)
            {
                r->setLoot(loot);
                r->itemSlot = itemSlot;
                r->aoeSlot = ++m_aoe_slots;

                if (item->DisenchantID && m_maxEnchantingLevel >= item->RequiredDisenchantSkill)
                    r->rollVoteMask |= ROLL_FLAG_TYPE_DISENCHANT;

                if (item->Flags2 & ITEM_FLAGS_EXTRA_NEED_ROLL_DISABLED)
                    r->rollVoteMask &= ~ROLL_FLAG_TYPE_NEED;

                loot->items[itemSlot].is_blocked = true;

                //Broadcast Pass and Send Rollstart
                for (Roll::PlayerVote::const_iterator itr = r->playerVote.begin(); itr != r->playerVote.end(); ++itr)
                {
                    Player* p = ObjectAccessor::FindPlayer(itr->first);
                    if (!p || !p->GetSession())
                        continue;

                    if (itr->second == PASS)
                        SendLootRoll(newitemGUID, p->GetGUID(), 128, ROLL_PASS, *r);
                    else
                        SendLootStartRollToPlayer(60000, lootedObject->GetMapId(), p, p->CanRollForItemInLFG(item, lootedObject) == EQUIP_ERR_OK, *r);
                }

                RollId.push_back(r);

                if (Creature* creature = lootedObject->ToCreature())
                {
                    creature->m_groupLootTimer = 60000;
                    creature->lootingGroupLowGUID = GetLowGUID();
                }
                else if (GameObject* go = lootedObject->ToGameObject())
                {
                    go->m_groupLootTimer = 60000;
                    go->lootingGroupLowGUID = GetLowGUID();
                }
            }
            else
                delete r;
        }
        else
            i->is_underthreshold = true;
    }

    for (std::vector<LootItem>::iterator i = loot->quest_items.begin(); i != loot->quest_items.end(); ++i, ++itemSlot)
    {
        if (!i->follow_loot_rules)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);
        uint64 newitemGUID = MAKE_NEW_GUID(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), 0, HIGHGUID_ITEM);
        Roll* r = new Roll(newitemGUID, *i);
        r->lootedGUID = loot->GetGUID();

        for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* playerToRoll = itr->getSource();
            if (!playerToRoll || !playerToRoll->GetSession())
                continue;

            bool allowedForPlayer = i->AllowedForPlayer(playerToRoll);
            if (allowedForPlayer && playerToRoll->IsWithinDistInMap(lootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
            {
                r->totalPlayersRolling++;
                r->playerVote[playerToRoll->GetGUID()] = NOT_EMITED_YET;
            }
        }

        if (r->totalPlayersRolling > 0)
        {
            r->setLoot(loot);
            r->itemSlot = itemSlot;
            r->aoeSlot = ++m_aoe_slots;

            loot->quest_items[itemSlot - loot->items.size()].is_blocked = true;

            //Broadcast Pass and Send Rollstart
            for (Roll::PlayerVote::const_iterator itr = r->playerVote.begin(); itr != r->playerVote.end(); ++itr)
            {
                Player* p = ObjectAccessor::FindPlayer(itr->first);
                if (!p || !p->GetSession())
                    continue;

                if (itr->second == PASS)
                    SendLootRoll(newitemGUID, p->GetGUID(), 128, ROLL_PASS, *r);
                else
                    SendLootStartRollToPlayer(60000, lootedObject->GetMapId(), p, p->CanRollForItemInLFG(item, lootedObject) == EQUIP_ERR_OK, *r);
            }

            RollId.push_back(r);

            if (Creature* creature = lootedObject->ToCreature())
            {
                creature->m_groupLootTimer = 60000;
                creature->lootingGroupLowGUID = GetLowGUID();
            }
            else if (GameObject* go = lootedObject->ToGameObject())
            {
                go->m_groupLootTimer = 60000;
                go->lootingGroupLowGUID = GetLowGUID();
            }
        }
        else
            delete r;
    }
}

//! 5.4.1
void Group::MasterLoot(Loot* /*loot*/, WorldObject* pLootedObject)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Group::MasterLoot (SMSG_LOOT_MASTER_LIST)");
    uint32 real_count = 0;

    ByteBuffer dataBuffer(GetMembersCount()*8);
    ObjectGuid guid_looted = pLootedObject->GetGUID();
    WorldPacket data(SMSG_LOOT_MASTER_LIST, 12 + GetMembersCount()*8);
    data.WriteGuidMask<5, 4, 6, 1, 0>(guid_looted);
    uint32 pos = data.bitwpos();
    data.WriteBits(0, 24);

    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* looter = itr->getSource();
        if (!looter->IsInWorld())
            continue;

        if (looter->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
        {
            //HardHack! Plr should have off-like hiGuid
            ObjectGuid guid = MAKE_NEW_GUID(looter->GetGUIDLow(), 0, HIGHGUID_PLAYER_MOP);

            data.WriteGuidMask<0, 6, 3, 1, 5, 7, 4, 2>(guid);
            dataBuffer.WriteGuidBytes<6, 7, 2, 0, 5, 3, 1, 4>(guid);
            ++real_count;
            if (real_count >= 10)
                break;
        }
    }

    data.WriteGuidMask<2, 3, 7>(guid_looted);
    data.PutBits<uint32>(pos, real_count, 24);

    data.FlushBits();

    data.append(dataBuffer);
    data.WriteGuidBytes<7, 0, 3, 2, 1, 4, 5, 6>(guid_looted);

    if (Player* player = ObjectAccessor::FindPlayer(GetLooterGuid()))
        player->GetSession()->SendPacket(&data);
}

void Group::DoRollForAllMembers(ObjectGuid guid, uint8 slot, uint32 mapid, Loot* loot, LootItem& item, Player* player)
{
    // Already rolled?
    for (Rolls::iterator iter=RollId.begin(); iter != RollId.end(); ++iter)
        if ((*iter)->itemSlot == slot && loot == (*iter)->getLoot() && (*iter)->isValid())
            return;

    uint64 newitemGUID = MAKE_NEW_GUID(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), 0, HIGHGUID_ITEM);
    Roll* r = new Roll(newitemGUID, item);
    r->lootedGUID = loot->GetGUID();
    WorldObject* pLootedObject = NULL;

    if (IS_CRE_OR_VEH_GUID(guid))
        pLootedObject = player->GetMap()->GetCreature(guid);
    else if (IS_GAMEOBJECT_GUID(guid))
        pLootedObject = player->GetMap()->GetGameObject(guid);

    if (!pLootedObject)
        return;

    //a vector is filled with only near party members
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* member = itr->getSource();
        if (!member || !member->GetSession())
            continue;

        if (item.AllowedForPlayer(member))
        {
            if (auto creature = pLootedObject->ToCreature())
                if (creature->isBoss() && member->GetInstanceId() && !creature->GetThreatTargetLoot(member->GetGUID()))
                    continue;

            if (member->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
            {
                r->totalPlayersRolling++;
                r->playerVote[member->GetGUID()] = NOT_EMITED_YET;
            }
        }
    }

    if (r->totalPlayersRolling > 0)
    {
        r->setLoot(loot);
        r->itemSlot = slot;
        r->aoeSlot = ++m_aoe_slots;     //restart at next loot. it's normall

        RollId.push_back(r);
    }

    SendLootStartRoll(180000, mapid, *r);
}

void Group::CountRollVote(uint64 playerGUID, uint8 AoeSlot, uint8 Choice)
{
    Rolls::iterator rollI = GetRoll(AoeSlot);
    if (rollI == RollId.end())
        return;
    Roll* roll = *rollI;

    Roll::PlayerVote::iterator itr = roll->playerVote.find(playerGUID);
    // this condition means that player joins to the party after roll begins
    if (itr == roll->playerVote.end())
        return;

    if (roll->getLoot())
        if (roll->getLoot()->items.empty())
            return;

    switch (Choice)
    {
    case ROLL_PASS:                                     // Player choose pass
        ++roll->totalPass;
        SendLootRoll(0, playerGUID, 0, ROLL_PASS, *roll);
        itr->second = PASS;
        break;
    case ROLL_NEED:                                     // player choose Need
        ++roll->totalNeed;
        SendLootRoll(0, playerGUID, 0, ROLL_NEED, *roll);
        itr->second = NEED;
        break;
    case ROLL_GREED:                                    // player choose Greed
        ++roll->totalGreed;
        SendLootRoll(0, playerGUID, 0, ROLL_GREED, *roll);
        itr->second = GREED;
        break;
    case ROLL_DISENCHANT:                               // player choose Disenchant
        ++roll->totalGreed;
        SendLootRoll(0, playerGUID, 0, ROLL_DISENCHANT, *roll);
        itr->second = DISENCHANT;
        break;
    }

    if (roll->TotalEmited() >= roll->totalPlayersRolling)
        CountTheRoll(rollI);
}

//called when roll timer expires
void Group::EndRoll(Loot* pLoot)
{
    for (Rolls::iterator itr = RollId.begin(); itr != RollId.end();)
    {
        if ((*itr)->getLoot() == pLoot) {
            CountTheRoll(itr);           //i don't have to edit player votes, who didn't vote ... he will pass
            itr = RollId.begin();
        }
        else
            ++itr;
    }
}

void Group::CountTheRoll(Rolls::iterator rollI)
{
    Roll* roll = *rollI;
    if (!roll->isValid())                                   // is loot already deleted ?
    {
        RollId.erase(rollI);
        delete roll;
        return;
    }

    //end of the roll
    if (roll->totalNeed > 0)
    {
        if (!roll->playerVote.empty())
        {
            uint8 maxresul = 0;
            uint64 maxguid  = (*roll->playerVote.begin()).first;
            Player* player;

            for (Roll::PlayerVote::const_iterator itr=roll->playerVote.begin(); itr != roll->playerVote.end(); ++itr)
            {
                if (itr->second != NEED)
                    continue;

                uint8 randomN = urand(1, 100);
                SendLootRoll(0, itr->first, randomN, ROLL_NEED, *roll);
                if (maxresul < randomN)
                {
                    maxguid  = itr->first;
                    maxresul = randomN;
                }
            }
            SendLootRollWon(0, maxguid, maxresul, ROLL_NEED, *roll);
            player = ObjectAccessor::FindPlayer(maxguid);

            if (player && player->GetSession())
            {
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT, roll->itemid, maxresul);

                ItemPosCountVec dest;
                LootItem* item = &(roll->itemSlot >= roll->getLoot()->items.size() ? roll->getLoot()->quest_items[roll->itemSlot - roll->getLoot()->items.size()] : roll->getLoot()->items[roll->itemSlot]);
                InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count);
                if (msg == EQUIP_ERR_OK)
                {
                    item->is_looted = true;
                    roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                    roll->getLoot()->unlootedCount--;
                    AllowedLooterSet looters = item->GetAllowedLooters();
                    player->StoreNewItem(dest, roll->itemid, true, item->randomPropertyId, looters);
                }
                else
                {
                    item->is_blocked = false;
                    player->SendEquipError(msg, NULL, NULL, roll->itemid);
                }
            }
        }
    }
    else if (roll->totalGreed > 0)
    {
        if (!roll->playerVote.empty())
        {
            uint8 maxresul = 0;
            uint64 maxguid = (*roll->playerVote.begin()).first;
            Player* player;
            RollVote rollvote = NOT_VALID;

            Roll::PlayerVote::iterator itr;
            for (itr = roll->playerVote.begin(); itr != roll->playerVote.end(); ++itr)
            {
                if (itr->second != GREED && itr->second != DISENCHANT)
                    continue;

                uint8 randomN = urand(1, 100);
                SendLootRoll(0, itr->first, randomN, itr->second, *roll);
                if (maxresul < randomN)
                {
                    maxguid  = itr->first;
                    maxresul = randomN;
                    rollvote = itr->second;
                }
            }
            SendLootRollWon(0, maxguid, maxresul, rollvote, *roll);
            player = ObjectAccessor::FindPlayer(maxguid);

            if (player && player->GetSession())
            {
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT, roll->itemid, maxresul);

                LootItem* item = &(roll->itemSlot >= roll->getLoot()->items.size() ? roll->getLoot()->quest_items[roll->itemSlot - roll->getLoot()->items.size()] : roll->getLoot()->items[roll->itemSlot]);

                if (rollvote == GREED)
                {
                    ItemPosCountVec dest;
                    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count);
                    if (msg == EQUIP_ERR_OK)
                    {
                        item->is_looted = true;
                        roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                        roll->getLoot()->unlootedCount--;
                        AllowedLooterSet looters = item->GetAllowedLooters();
                        player->StoreNewItem(dest, roll->itemid, true, item->randomPropertyId, looters);
                    }
                    else
                    {
                        item->is_blocked = false;
                        player->SendEquipError(msg, NULL, NULL, roll->itemid);
                    }
                }
                else if (rollvote == DISENCHANT)
                {
                    item->is_looted = true;
                    roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                    roll->getLoot()->unlootedCount--;
                    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(roll->itemid);
                    player->AutoStoreLoot(pProto->DisenchantID, LootTemplates_Disenchant, 0, true);
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, 13262); // Disenchant
                }
            }
        }
    }
    else
    {
        SendLootAllPassed(*roll);

        // remove is_blocked so that the item is lootable by all players
        LootItem* item = &(roll->itemSlot >= roll->getLoot()->items.size() ? roll->getLoot()->quest_items[roll->itemSlot - roll->getLoot()->items.size()] : roll->getLoot()->items[roll->itemSlot]);
        if (item)
            item->is_blocked = false;
    }

    RollId.erase(rollI);
    delete roll;
}

//! 5.4.1
void Group::SetTargetIcon(uint8 id, ObjectGuid whoGuid, ObjectGuid targetGuid)
{
    if (id >= TARGETICONCOUNT)
        return;

    // clean other icons
    if (targetGuid != 0)
        for (int i=0; i<TARGETICONCOUNT; ++i)
            if (m_targetIcons[i] == targetGuid)
                SetTargetIcon(i, 0, 0);

    m_targetIcons[id] = targetGuid;

    WorldPacket data(SMSG_RAID_TARGET_UPDATE_SINGLE, (1+8+1+8));

    data.WriteGuidMask<3, 2, 0, 4>(targetGuid);
    data.WriteGuidMask<0>(whoGuid);
    data.WriteGuidMask<1>(targetGuid);
    data.WriteGuidMask<4, 1>(whoGuid);
    data.WriteGuidMask<5>(targetGuid);
    data.WriteGuidMask<5, 3, 2, 7>(whoGuid);
    data.WriteGuidMask<6>(targetGuid);
    data.WriteGuidMask<6>(whoGuid);
    data.WriteGuidMask<7>(targetGuid);

    data << uint8(id);
    data.WriteGuidBytes<6, 4>(whoGuid);
    data.WriteGuidBytes<7>(targetGuid);
    data.WriteGuidBytes<3>(whoGuid);
    data.WriteGuidBytes<6, 5>(targetGuid);
    data.WriteGuidBytes<7, 5, 0>(whoGuid);
    data.WriteGuidBytes<1, 2>(targetGuid);
    data << uint8(IsHomeGroup() ? 0 : 1);
    data.WriteGuidBytes<2, 1>(whoGuid);
    data.WriteGuidBytes<3, 0, 4>(targetGuid);

    BroadcastPacket(&data, true);
}

//! 5.4.1
void Group::SendTargetIconList(WorldSession* session)
{
    if (!session)
        return;

    WorldPacket data(SMSG_RAID_TARGET_UPDATE_ALL, (1+TARGETICONCOUNT*9));
    data << uint8(IsHomeGroup() ? 0 : 1);
    size_t count = 0;
    size_t pos = data.wpos();
    data.WriteBits(TARGETICONCOUNT, 23);
    ByteBuffer dataBuffer;

    for (uint8 i = 0; i < TARGETICONCOUNT; ++i)
    {
        if (m_targetIcons[i] == 0)
            continue;
        ObjectGuid guid = m_targetIcons[i];
    
        data.WriteGuidMask<7, 6, 0, 1, 5, 2, 3, 4>(guid);

        dataBuffer.WriteGuidBytes<2, 0, 6, 5, 4>(guid);
        dataBuffer << uint8(i);
        dataBuffer.WriteGuidBytes<7, 3, 1>(guid);
        ++count;
    }
    data.FlushBits();
    data.PutBits<uint32>(pos, count, 23);
    data.append(dataBuffer);
    session->SendPacket(&data);

    //SMSG_RAID_MARKERS
}

void Group::SendUpdate()
{
    for (member_witerator witr = m_memberSlots.begin(); witr != m_memberSlots.end(); ++witr)
        SendUpdateToPlayer(witr->guid, &(*witr));
}

void Group::SendUpdateToPlayer(uint64 playerGUID, MemberSlot* slot)
{
    Player* player = ObjectAccessor::FindPlayer(playerGUID);

    if (!player || !player->GetSession() || player->GetGroup() != this)
        return;

    // if MemberSlot wasn't provided
    if (!slot)
    {
        member_witerator witr = _getMemberWSlot(playerGUID);

        if (witr == m_memberSlots.end()) // if there is no MemberSlot for such a player
            return;

        slot = &(*witr);
    }

    Player* member = NULL;

    uint8 const gStatusOfline = (isBGGroup() || isBFGroup()) ? MEMBER_STATUS_PVP : 0;
    uint8 const gStatusOnline = gStatusOfline | MEMBER_STATUS_ONLINE;

    ObjectGuid leaderGuid = m_leaderGuid;
    ObjectGuid guid = m_guid;
    ObjectGuid looterGuid = m_looterGuid;
    ObjectGuid plguid = playerGUID;

    bool sendDifficultyInfo = true;

    ByteBuffer dataBuffer(40 * GetMembersCount());

    //! 5.4.1
    WorldPacket data(SMSG_PARTY_UPDATE, 60 * GetMembersCount() + 41);

    data << uint32(++m_counter);
    data << uint8(0);                                                   // unk
    data << uint8(IsHomeGroup() ? 0 : 1);                               // 0 - home group, 1 - instance group
    data << uint8(m_groupType);                                         // group type (flags in 3.3)
    data << uint32(0);                                                  // unk, sometime 32 in sniff (flags ?)

    data.WriteGuidMask<3, 6>(leaderGuid);
    data.WriteGuidMask<6, 7, 2, 5, 3>(guid);
    data.WriteGuidMask<0, 5>(leaderGuid);
    data.WriteBit(true);                                                // HasLooterGuid
    data.WriteGuidMask<4>(leaderGuid);
    data.WriteBits(GetMembersCount() , 21);

    if (true)                                                           // HasLooterGuid
        data.WriteGuidMask<4, 6, 5, 7, 0, 1, 2, 3>(looterGuid);

    // Send self first
    data.WriteBits(slot->name.size(), 6);
    data.WriteGuidMask<4, 3, 7, 0, 1, 2, 6, 5>(plguid);

    dataBuffer << uint8(gStatusOnline);                                 // online-state
    dataBuffer << uint8(slot->flags);
    dataBuffer.WriteGuidBytes<2, 7, 4, 0>(plguid);
    dataBuffer << uint8(slot->group);
    dataBuffer.WriteGuidBytes<6, 1, 5, 3>(plguid);
    dataBuffer << uint8(slot->roles);
    dataBuffer.WriteString(slot->name);

    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        if (citr->guid == slot->guid)
            continue;

        member = ObjectAccessor::FindPlayer(citr->guid);

        data.WriteBits(citr->name.size(), 6);
        data.WriteGuidMask<4, 3, 7, 0, 1, 2, 6, 5>(citr->guid);

        dataBuffer << uint8(member ? gStatusOnline : gStatusOfline);  // online-state
        dataBuffer << uint8(citr->flags);
        dataBuffer.WriteGuidBytes<2, 7, 4, 0>(citr->guid);
        dataBuffer << uint8(citr->group);
        dataBuffer.WriteGuidBytes<6, 1, 5, 3>(citr->guid);
        dataBuffer << uint8(citr->roles);
        dataBuffer.WriteString(citr->name);
    }

    data.WriteBit(m_groupType & GROUPTYPE_LFG);
    data.WriteGuidMask<1>(guid);
    data.WriteBit(sendDifficultyInfo);
    data.WriteGuidMask<4>(leaderGuid);
    data.WriteGuidMask<0>(guid);
    data.WriteGuidMask<2>(leaderGuid);

    ByteBuffer lfgBuff;
    if (m_groupType & GROUPTYPE_LFG)
    {
        lfg::LFGDungeonData const* dungeon = sLFGMgr->GetLFGDungeon(sLFGMgr->GetDungeon(m_guid, true), player->GetTeam());
        lfg::LFGDungeonData const* rDungeon = NULL;
        if (dungeon)
        {
            lfg::LfgDungeonSet const& dungeons = sLFGMgr->GetSelectedDungeons(guid);
            if (!dungeons.empty())
            {
                 rDungeon = sLFGMgr->GetLFGDungeon(*dungeons.begin(), player->GetTeam());
                 if (rDungeon && rDungeon->type != LFG_TYPE_RANDOM)
                     rDungeon = NULL;
            }
        }

        lfg::LfgState lfgState = sLFGMgr->GetState(m_guid);

        data.WriteBit(lfgState != lfg::LFG_STATE_FINISHED_DUNGEON); // can be rewarded
        data.WriteBit(dungeon ? 0 : 1);

        lfgBuff << uint32(rDungeon ? rDungeon->Entry() : 0);
        lfgBuff << uint8(0);                                        // 0 always
        lfgBuff << uint8(GetMembersCount() - 1);
        lfgBuff << uint8(0);
        lfgBuff << uint8(0);
        uint8 flags = 0;
        if (lfgState == lfg::LFG_STATE_FINISHED_DUNGEON || dungeon && dungeon->dbc->flags & LFG_FLAG_NON_BACKFILLABLE)
            flags |= 2;
        lfgBuff << uint8(flags);
        lfgBuff << float(dungeon ? 1.0f : 0.0f);
        lfgBuff << uint32(dungeon ? dungeon->Entry() : 0);
    }

    data.WriteGuidMask<7, 1>(leaderGuid);

    data.FlushBits();

    if (m_groupType & GROUPTYPE_LFG)
        data.append(lfgBuff);

    // Insert member data colected before
    data.append(dataBuffer);

    if (true)                                                           // HasLooterGuid
    {
        data << uint8(m_lootThreshold);                                 // loot threshold
        data.WriteGuidBytes<5, 4>(looterGuid);
        data << uint8(m_lootMethod);                                    // loot method
        data.WriteGuidBytes<3, 1, 0, 6, 2, 7>(looterGuid);
    }

    data.WriteGuidBytes<2>(guid);

    if (sendDifficultyInfo)
    {
        data << uint32(m_raidDifficulty);
        data << uint32(m_dungeonDifficulty);
    }

    data.WriteGuidBytes<5, 3, 1, 0>(guid);
    data.WriteGuidBytes<7, 2, 0, 1>(leaderGuid);
    data.WriteGuidBytes<7>(guid);
    data.WriteGuidBytes<6, 4, 5>(leaderGuid);
    data.WriteGuidBytes<6, 4>(guid);
    data.WriteGuidBytes<3>(guid);

    player->GetSession()->SendPacket(&data);
}

//! 5.4.1
void Group::SendEmptyParty(Player *player)
{
    ObjectGuid leaderGuid = 0;
    ObjectGuid guid = 0;

    WorldPacket data(SMSG_PARTY_UPDATE, 60);

    data << uint32(++m_counter);
    data << uint8(0);                                                   // unk
    data << uint8(IsHomeGroup() ? 0 : 1);
    data << uint8(0);                                                   // group type (flags in 3.3)
    data << uint32(0);                                                  // unk, sometime 32 in sniff (flags ?)
    data.WriteGuidMask<3, 6>(leaderGuid);
    data.WriteGuidMask<6, 7, 2, 5, 3>(guid);
    data.WriteGuidMask<0, 5>(leaderGuid);
    data.WriteBit(false);                                                // HasLooterGuid
    data.WriteGuidMask<4>(leaderGuid);
    data.WriteBits(0 , 21);
    data.WriteBit(false);
    data.WriteGuidMask<1>(guid);
    data.WriteBit(false);
    data.WriteGuidMask<4>(leaderGuid);
    data.WriteGuidMask<0>(guid);
    data.WriteGuidMask<2>(leaderGuid);
    data.WriteGuidMask<7, 1>(leaderGuid);
    data.FlushBits();
    data.WriteGuidBytes<2>(guid);
    data.WriteGuidBytes<5, 3, 1, 0>(guid);
    data.WriteGuidBytes<7, 2, 0, 1>(leaderGuid);
    data.WriteGuidBytes<7>(guid);
    data.WriteGuidBytes<6, 4, 5>(leaderGuid);
    data.WriteGuidBytes<6, 4>(guid);
    data.WriteGuidBytes<3>(guid);

    player->GetSession()->SendPacket(&data);
}

void Group::UpdatePlayerOutOfRange(Player* player)
{
    if (!player || !player->IsInWorld())
        return;

    WorldPacket data;
    player->GetSession()->BuildPartyMemberStatsChangedPacket(player, &data);

    Player* member;
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        member = itr->getSource();
        if (member && !member->IsWithinDist(player, member->GetSightRange(), false))
            member->GetSession()->SendPacket(&data);
    }
}

void Group::BroadcastAddonMessagePacket(WorldPacket* packet, const std::string& prefix, bool ignorePlayersInBGRaid, int group, uint64 ignore)
{
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (!player || (ignore != 0 && player->GetGUID() == ignore) || (ignorePlayersInBGRaid && player->GetGroup() != this))
            continue;

        if (WorldSession* session = player->GetSession())
            if (session && (group == -1 || itr->getSubGroup() == group))
                if (session->IsAddonRegistered(prefix))
                    session->SendPacket(packet);
    }
}

void Group::BroadcastPacket(WorldPacket* packet, bool ignorePlayersInBGRaid, int group, uint64 ignore)
{
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (!player || (ignore != 0 && player->GetGUID() == ignore) || (ignorePlayersInBGRaid && player->GetGroup() != this))
            continue;

        if (player->GetSession() && (group == -1 || itr->getSubGroup() == group))
            player->GetSession()->SendPacket(packet);
    }
}

void Group::BroadcastReadyCheck(WorldPacket* packet)
{
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (player && player->GetSession())
            if (IsLeader(player->GetGUID()) || IsAssistant(player->GetGUID()) || m_groupType & GROUPTYPE_EVERYONE_IS_ASSISTANT)
                player->GetSession()->SendPacket(packet);
    }
}

//! 5.4.1
void Group::OfflineReadyCheck()
{
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        Player* player = ObjectAccessor::FindPlayer(citr->guid);
        if (!player || !player->GetSession())
        {
            bool ready = false;
            ObjectGuid plGUID = citr->guid;
            ObjectGuid grpGUID = GetGUID();

            //! 5.4.1
            WorldPacket data(SMSG_RAID_READY_CHECK_RESPONSE);
            data.WriteGuidMask<6>(plGUID);
            data.WriteBit(ready);
            data.WriteGuidMask<5>(plGUID);
            data.WriteGuidMask<3, 2>(grpGUID);
            data.WriteGuidMask<1, 0>(plGUID);
            data.WriteGuidMask<1>(grpGUID);
            data.WriteGuidMask<2>(plGUID);
            data.WriteGuidMask<4, 6>(grpGUID);
            data.WriteGuidMask<3, 4>(plGUID);
            data.WriteGuidMask<5, 7>(grpGUID);
            data.WriteGuidMask<7>(plGUID);
            data.WriteGuidMask<0>(grpGUID);

            data.FlushBits();

            data.WriteGuidBytes<0, 6>(plGUID);
            data.WriteGuidBytes<0>(grpGUID);
            data.WriteGuidBytes<3>(plGUID);
            data.WriteGuidBytes<5, 6>(grpGUID);
            data.WriteGuidBytes<2>(plGUID);
            data.WriteGuidBytes<2>(grpGUID);
            data.WriteGuidBytes<7>(plGUID);
            data.WriteGuidBytes<4, 3>(grpGUID);
            data.WriteGuidBytes<4>(plGUID);
            data.WriteGuidBytes<1, 7>(grpGUID);
            data.WriteGuidBytes<5, 1>(plGUID);

            BroadcastReadyCheck(&data);

            m_readyCheckCount++;
        }
    }
}

bool Group::_setMembersGroup(uint64 guid, uint8 group)
{
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return false;

    slot->group = group;

    SubGroupCounterIncrease(group);

    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_SUBGROUP);

        stmt->setUInt8(0, group);
        stmt->setUInt32(1, GUID_LOPART(guid));

        CharacterDatabase.Execute(stmt);
    }

    return true;
}

bool Group::SameSubGroup(Player const* member1, Player const* member2) const
{
    if (!member1 || !member2)
        return false;

    if (member1->GetGroup() != this || member2->GetGroup() != this)
        return false;
    else
        return member1->GetSubGroup() == member2->GetSubGroup();
}

// Allows setting sub groups both for online or offline members
void Group::ChangeMembersGroup(uint64 guid, uint8 group)
{
    // Only raid groups have sub groups
    if (!isRaidGroup())
        return;

    // Check if player is really in the raid
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    // Abort if the player is already in the target sub group
    uint8 prevSubGroup = GetMemberGroup(guid);
    if (prevSubGroup == group)
        return;

    // Update the player slot with the new sub group setting
    slot->group = group;

    // Increase the counter of the new sub group..
    SubGroupCounterIncrease(group);

    // ..and decrease the counter of the previous one
    SubGroupCounterDecrease(prevSubGroup);

    // Preserve new sub group in database for non-raid groups
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_SUBGROUP);

        stmt->setUInt8(0, group);
        stmt->setUInt32(1, GUID_LOPART(guid));

        CharacterDatabase.Execute(stmt);
    }

    // In case the moved player is online, update the player object with the new sub group references
    if (Player* player = ObjectAccessor::FindPlayer(guid))
    {
        if (player->GetGroup() == this)
            player->GetGroupRef().setSubGroup(group);
        else
        {
            // If player is in BG raid, it is possible that he is also in normal raid - and that normal raid is stored in m_originalGroup reference
            prevSubGroup = player->GetOriginalSubGroup();
            player->GetOriginalGroupRef().setSubGroup(group);
        }
    }

    // Broadcast the changes to the group
    SendUpdate();
}

// Retrieve the next Round-Roubin player for the group
//
// No update done if loot method is Master or FFA.
//
// If the RR player is not yet set for the group, the first group member becomes the round-robin player.
// If the RR player is set, the next player in group becomes the round-robin player.
//
// If ifneed is true,
//      the current RR player is checked to be near the looted object.
//      if yes, no update done.
//      if not, he loses his turn.
void Group::UpdateLooterGuid(WorldObject* pLootedObject, bool ifneed)
{
    uint64 oldLooterGUID = GetLooterGuid();
    switch (GetLootMethod())
    {
        case FREE_FOR_ALL:
            return;
        case MASTER_LOOT:
        {
            if (Player* player = ObjectAccessor::FindPlayer(oldLooterGUID))
                if (player->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                    return;

            if (oldLooterGUID != m_leaderGuid)
            {
                if (Player* player = ObjectAccessor::FindPlayer(m_leaderGuid))
                {
                    if (player->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                    {
                        SetLooterGuid(m_leaderGuid);
                        SendUpdate();
                        return;
                    }
                }
            }
            break;
        }
        default:
            // round robin style looting applies for all low
            // quality items in each loot method except free for all and master loot
            break;
    }

    member_citerator guid_itr = _getMemberCSlot(oldLooterGUID);
    if (guid_itr != m_memberSlots.end())
    {
        if (ifneed)
        {
            // not update if only update if need and ok
            Player* looter = ObjectAccessor::FindPlayer(guid_itr->guid);
            if (looter && looter->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                return;
        }
        ++guid_itr;
    }

    // search next after current
    Player* pNewLooter = NULL;
    for (member_citerator itr = guid_itr; itr != m_memberSlots.end(); ++itr)
    {
        if (Player* player = ObjectAccessor::FindPlayer(itr->guid))
            if (player->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
            {
                pNewLooter = player;
                break;
            }
    }

    if (!pNewLooter)
    {
        // search from start
        for (member_citerator itr = m_memberSlots.begin(); itr != guid_itr; ++itr)
        {
            if (Player* player = ObjectAccessor::FindPlayer(itr->guid))
                if (player->IsWithinDistInMap(pLootedObject, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false))
                {
                    pNewLooter = player;
                    break;
                }
        }
    }

    if (pNewLooter)
    {
        if (oldLooterGUID != pNewLooter->GetGUID())
        {
            SetLooterGuid(pNewLooter->GetGUID());
            SendUpdate();
        }
    }
    else
    {
        SetLooterGuid(0);
        SendUpdate();
    }
}

//! ToDo: Arena check battlegroup
GroupJoinBattlegroundResult Group::CanJoinBattlegroundQueue(Battleground const* bgOrTemplate, BattlegroundQueueTypeId bgQueueTypeId, uint32 MinPlayerCount, uint32 /*MaxPlayerCount*/, bool isRated, uint32 arenaSlot)
{
    // check if this group is LFG group
    if (isLFGGroup())
        return ERR_LFG_CANT_USE_BATTLEGROUND;

    BattlemasterListEntry const* bgEntry = sBattlemasterListStore.LookupEntry(bgOrTemplate->GetTypeID());
    if (!bgEntry)
        return ERR_BATTLEGROUND_JOIN_FAILED;            // shouldn't happen

    // check for min / max count
    uint32 memberscount = GetMembersCount();

    if (memberscount > bgEntry->maxGroupSize)                // no MinPlayerCount for battlegrounds
        return ERR_BATTLEGROUND_NONE;                        // ERR_GROUP_JOIN_BATTLEGROUND_TOO_MANY handled on client side

    // get a player as reference, to compare other players' stats to (arena team id, queue id based on level, etc.)
    Player* reference = GetFirstMember()->getSource();
    // no reference found, can't join this way
    if (!reference)
        return ERR_BATTLEGROUND_JOIN_FAILED;

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgOrTemplate->GetMapId(), reference->getLevel());
    if (!bracketEntry)
        return ERR_BATTLEGROUND_JOIN_FAILED;

    Bracket* bracket = reference->getBracket(BracketType(arenaSlot));
    ASSERT(bracket);

    uint32 mmr = bracket->getMMV();
    uint32 team = reference->GetTeam();

    BattlegroundQueueTypeId bgQueueTypeIdRandom = BattlegroundMgr::BGQueueTypeId(BATTLEGROUND_RB, 0);

    // check every member of the group to be able to join
    memberscount = 0;
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next(), ++memberscount)
    {
        Player* member = itr->getSource();
        // offline member? don't let join
        if (!member)
            return ERR_BATTLEGROUND_JOIN_FAILED;

        if (member->HasAura(125761) && isRated)
            return ERR_BATTLEGROUND_JOIN_FAILED;
        // don't allow cross-faction join as group
        if (member->GetTeam() != team)
            return ERR_BATTLEGROUND_JOIN_TIMED_OUT;
        // not in the same battleground level braket, don't let join
        PvPDifficultyEntry const* memberBracketEntry = GetBattlegroundBracketByLevel(bracketEntry->mapId, member->getLevel());
        if (memberBracketEntry != bracketEntry)
            return ERR_BATTLEGROUND_JOIN_RANGE_INDEX;
        // don't let join rated matches if the arena team id doesn't match
        //if (isRated && member->GetArenaTeamId(arenaSlot) != arenaTeamId)
        //    return ERR_BATTLEGROUND_JOIN_FAILED;
        // don't let join if someone from the group is already in that bg queue
        if (member->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeId))
            return ERR_BATTLEGROUND_JOIN_FAILED;            // not blizz-like
        // don't let join if someone from the group is in bg queue random
        if (member->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeIdRandom))
            return ERR_IN_RANDOM_BG;
        // don't let join to bg queue random if someone from the group is already in bg queue
        if (bgOrTemplate->GetTypeID() == BATTLEGROUND_RB && member->InBattlegroundQueue())
            return ERR_IN_NON_RANDOM_BG;
        // check for deserter debuff in case not arena queue
        if (bgOrTemplate->GetTypeID() != BATTLEGROUND_AA && !member->CanJoinToBattleground())
            return ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS;
        // check if member can join any more battleground queues
        if (!member->HasFreeBattlegroundQueueId())
            return ERR_BATTLEGROUND_TOO_MANY_QUEUES;        // not blizz-like
        // check if someone in party is using dungeon system
        if (member->isUsingLfg())
            return ERR_LFG_CANT_USE_BATTLEGROUND;
    }

    // only check for MinPlayerCount since MinPlayerCount == MaxPlayerCount for arenas...
    if ((bgOrTemplate->isArena() || isRated) && memberscount != MinPlayerCount)
        return ERR_ARENA_TEAM_PARTY_SIZE;

    return ERR_BATTLEGROUND_NONE;
}

//===================================================
//============== Roll ===============================
//===================================================

void Roll::targetObjectBuildLink()
{
    // called from link()
    getTarget()->addLootValidatorRef(this);
}

void Group::SetDungeonDifficulty(Difficulty difficulty)
{
    m_dungeonDifficulty = difficulty;
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_DIFFICULTY);

        stmt->setUInt8(0, uint8(m_dungeonDifficulty));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (!player->GetSession())
            continue;

        player->SetDungeonDifficulty(difficulty);
        player->SendDungeonDifficulty();
    }
}

void Group::SetRaidDifficulty(Difficulty difficulty)
{
    m_raidDifficulty = difficulty;
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_RAID_DIFFICULTY);

        stmt->setUInt8(0, uint8(m_raidDifficulty));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (!player->GetSession())
            continue;

        player->SetRaidDifficulty(difficulty);
        player->SendRaidDifficulty();
    }
}

bool Group::InCombatToInstance(uint32 instanceId)
{
    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (player && !player->getAttackers().empty() && player->GetInstanceId() == instanceId && (player->GetMap()->IsRaidOrHeroicDungeon()))
            for (std::set<Unit*>::const_iterator i = player->getAttackers().begin(); i != player->getAttackers().end(); ++i)
                if ((*i) && (*i)->GetTypeId() == TYPEID_UNIT && (*i)->ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
                    return true;
    }
    return false;
}

void Group::ResetInstances(uint8 method, bool isRaid, Player* SendMsgTo)
{
    if (isBGGroup() || isBFGroup())
        return;

    // method can be INSTANCE_RESET_ALL, INSTANCE_RESET_CHANGE_DIFFICULTY, INSTANCE_RESET_GROUP_DISBAND

    // we assume that when the difficulty changes, all instances that can be reset will be
    Difficulty diff = GetDifficulty(isRaid);

    for (BoundInstancesMap::iterator itr = m_boundInstances[diff].begin(); itr != m_boundInstances[diff].end();)
    {
        InstanceSave* instanceSave = itr->second.save;
        const MapEntry* entry = sMapStore.LookupEntry(itr->first);
        if (!entry || entry->IsRaid() != isRaid || (!instanceSave->CanReset() && method != INSTANCE_RESET_GROUP_DISBAND))
        {
            ++itr;
            continue;
        }

        if (method == INSTANCE_RESET_ALL)
        {
            // the "reset all instances" method can only reset normal maps
            if (entry->map_type == MAP_RAID || diff == HEROIC_DIFFICULTY)
            {
                ++itr;
                continue;
            }
        }

        bool isEmpty = true;
        // if the map is loaded, reset it
        Map* map = sMapMgr->FindMap(instanceSave->GetMapId(), instanceSave->GetInstanceId());
        if (map && map->IsDungeon() && !(method == INSTANCE_RESET_GROUP_DISBAND && !instanceSave->CanReset()))
        {
            if (instanceSave->CanReset())
                isEmpty = ((InstanceMap*)map)->Reset(method);
            else
                isEmpty = !map->HavePlayers();
        }

        if (SendMsgTo)
        {
            if (isEmpty)
                SendMsgTo->SendResetInstanceSuccess(instanceSave->GetMapId());
            else
                SendMsgTo->SendResetInstanceFailed(0, instanceSave->GetMapId());
        }

        if (isEmpty || method == INSTANCE_RESET_GROUP_DISBAND || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
            // do not reset the instance, just unbind if others are permanently bound to it
            if (instanceSave->CanReset())
                instanceSave->DeleteFromDB();
            else
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_BY_INSTANCE);

                stmt->setUInt32(0, instanceSave->GetInstanceId());

                CharacterDatabase.Execute(stmt);
            }


            // i don't know for sure if hash_map iterators
            m_boundInstances[diff].erase(itr);
            itr = m_boundInstances[diff].begin();
            // this unloads the instance save unless online players are bound to it
            // (eg. permanent binds or GM solo binds)
            instanceSave->RemoveGroup(this);
        }
        else
            ++itr;
    }
}

InstanceGroupBind* Group::GetBoundInstance(Player* player)
{
    uint32 mapid = player->GetMapId();
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
    return GetBoundInstance(mapEntry);
}

InstanceGroupBind* Group::GetBoundInstance(Map* aMap)
{
    // Currently spawn numbering not different from map difficulty
    Difficulty difficulty = GetDifficulty(aMap->IsRaid());

    // some instances only have one difficulty
    GetDownscaledMapDifficultyData(aMap->GetId(), difficulty);

    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(aMap->GetId());
    if (itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return NULL;
}

InstanceGroupBind* Group::GetBoundInstance(MapEntry const* mapEntry)
{
    if (!mapEntry)
        return NULL;

    Difficulty difficulty = GetDifficulty(mapEntry->IsRaid());

    // some instances only have one difficulty
    GetDownscaledMapDifficultyData(mapEntry->MapID, difficulty);

    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapEntry->MapID);
    if (itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return NULL;
}

InstanceGroupBind* Group::BindToInstance(InstanceSave* save, bool permanent, bool load)
{
    if (!save || isBGGroup() || isBFGroup())
        return NULL;

    InstanceGroupBind& bind = m_boundInstances[save->GetDifficulty()][save->GetMapId()];
    if (!load && (!bind.save || permanent != bind.perm || save != bind.save))
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GROUP_INSTANCE);

        stmt->setUInt32(0, m_dbStoreId);
        stmt->setUInt32(1, save->GetInstanceId());
        stmt->setBool(2, permanent);

        CharacterDatabase.Execute(stmt);
    }

    if (bind.save != save)
    {
        if (bind.save)
            bind.save->RemoveGroup(this);
        save->AddGroup(this);
    }

    bind.save = save;
    bind.perm = permanent;
    if (!load)
        sLog->outDebug(LOG_FILTER_MAPS, "Group::BindToInstance: Group (guid: %u, storage id: %u) is now bound to map %d, instance %d, difficulty %d",
        GUID_LOPART(GetGUID()), m_dbStoreId, save->GetMapId(), save->GetInstanceId(), save->GetDifficulty());

    return &bind;
}

void Group::UnbindInstance(uint32 mapid, uint8 difficulty, bool unload)
{
    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    if (itr != m_boundInstances[difficulty].end())
    {
        if (!unload)
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_BY_GUID);

            stmt->setUInt32(0, m_dbStoreId);
            stmt->setUInt32(1, itr->second.save->GetInstanceId());

            CharacterDatabase.Execute(stmt);
        }

        itr->second.save->RemoveGroup(this);                // save can become invalid
        m_boundInstances[difficulty].erase(itr);
    }
}

void Group::_homebindIfInstance(Player* player)
{
    if (player && !player->isGameMaster() && sMapStore.LookupEntry(player->GetMapId())->IsDungeon())
        player->m_InstanceValid = false;
}

void Group::BroadcastGroupUpdate(void)
{
    // FG: HACK: force flags update on group leave - for values update hack
    // -- not very efficient but safe
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        Player* pp = ObjectAccessor::FindPlayer(citr->guid);
        if (pp && pp->IsInWorld())
        {
            pp->ForceValuesUpdateAtIndex(UNIT_FIELD_BYTES_2);
            pp->ForceValuesUpdateAtIndex(UNIT_FIELD_FACTIONTEMPLATE);
            sLog->outDebug(LOG_FILTER_GENERAL, "-- Forced group value update for '%s'", pp->GetName());
        }
    }
}

void Group::ResetMaxEnchantingLevel()
{
    m_maxEnchantingLevel = 0;
    Player* pMember = NULL;
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        pMember = ObjectAccessor::FindPlayer(citr->guid);
        if (pMember && m_maxEnchantingLevel < pMember->GetSkillValue(SKILL_ENCHANTING))
            m_maxEnchantingLevel = pMember->GetSkillValue(SKILL_ENCHANTING);
    }
}

void Group::SetLootMethod(LootMethod method)
{
    m_lootMethod = method;
}

void Group::SetLooterGuid(uint64 guid)
{
    m_looterGuid = guid;
}

void Group::SetLootThreshold(ItemQualities threshold)
{
    m_lootThreshold = threshold;
}

void Group::SetLfgRoles(uint64 guid, const uint8 roles)
{
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    slot->roles = roles;
}

bool Group::IsFull() const
{
    return isRaidGroup() ? (m_memberSlots.size() >= MAXRAIDSIZE) : (m_memberSlots.size() >= MAXGROUPSIZE);
}

bool Group::isLFGGroup() const
{
    return m_groupType & GROUPTYPE_LFG;
}

bool Group::isRaidGroup() const
{
    return m_groupType & GROUPTYPE_RAID;
}

bool Group::isBGGroup() const
{
    return m_bgGroup != NULL;
}

bool Group::isArenaGroup() const
{
    return m_bgGroup && m_bgGroup->isArena();
}

bool Group::isBFGroup() const
{
    return m_bfGroup != NULL;
}

bool Group::IsCreated() const
{
    return GetMembersCount() > 0;
}

uint64 Group::GetLeaderGUID() const
{
    return m_leaderGuid;
}

uint64 Group::GetGUID() const
{
    return m_guid;
}

uint32 Group::GetLowGUID() const
{
    return GUID_LOPART(m_guid);
}

const char * Group::GetLeaderName() const
{
    return m_leaderName.c_str();
}

LootMethod Group::GetLootMethod() const
{
    return m_lootMethod;
}

uint64 Group::GetLooterGuid() const
{
    return m_looterGuid;
}

ItemQualities Group::GetLootThreshold() const
{
    return m_lootThreshold;
}

bool Group::IsMember(uint64 guid) const
{
    return _getMemberCSlot(guid) != m_memberSlots.end();
}

bool Group::IsLeader(uint64 guid) const
{
    return (GetLeaderGUID() == guid);
}

uint64 Group::GetMemberGUID(const std::string& name)
{
    for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->name == name)
            return itr->guid;
    return 0;
}

bool Group::IsAssistant(uint64 guid) const
{
    member_citerator mslot = _getMemberCSlot(guid);
    if (mslot == m_memberSlots.end())
        return false;
    return mslot->flags & MEMBER_FLAG_ASSISTANT;
}

bool Group::SameSubGroup(uint64 guid1, uint64 guid2) const
{
    member_citerator mslot2 = _getMemberCSlot(guid2);
    if (mslot2 == m_memberSlots.end())
        return false;
    return SameSubGroup(guid1, &*mslot2);
}

bool Group::SameSubGroup(uint64 guid1, MemberSlot const* slot2) const
{
    member_citerator mslot1 = _getMemberCSlot(guid1);
    if (mslot1 == m_memberSlots.end() || !slot2)
        return false;
    return (mslot1->group == slot2->group);
}

bool Group::HasFreeSlotSubGroup(uint8 subgroup) const
{
    return (m_subGroupsCounts && m_subGroupsCounts[subgroup] < MAXGROUPSIZE);
}


uint8 Group::GetMemberGroup(uint64 guid) const
{
    member_citerator mslot = _getMemberCSlot(guid);
    if (mslot == m_memberSlots.end())
        return (MAX_RAID_SUBGROUPS+1);
    return mslot->group;
}

void Group::SetBattlegroundGroup(Battleground* bg)
{
    m_bgGroup = bg;
}

void Group::SetBattlefieldGroup(Battlefield *bg)
{
    m_bfGroup = bg;
}

void Group::setGroupMemberRole(uint64 guid, uint32 role)
{
    for (MemberSlotList::iterator member = m_memberSlots.begin(); member != m_memberSlots.end(); ++member)
    {
        if (member->guid == guid)
        {
            member->roles = role;
            break;
        }
    }
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_ROLE);
    if (stmt != NULL)
    {
        stmt->setUInt8(0, role);
        stmt->setUInt32(1, GUID_LOPART(guid));
        CharacterDatabase.Execute(stmt);
    }
}

void Group::SetGroupMemberFlag(uint64 guid, bool apply, GroupMemberFlags flag)
{
    // Assistants, main assistants and main tanks are only available in raid groups
    if (!isRaidGroup())
        return;

    // Check if player is really in the raid
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    // Do flag specific actions, e.g ensure uniqueness
    switch (flag) {
    case MEMBER_FLAG_MAINASSIST:
        RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINASSIST);         // Remove main assist flag from current if any.
        break;
    case MEMBER_FLAG_MAINTANK:
        RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINTANK);           // Remove main tank flag from current if any.
        break;
    case MEMBER_FLAG_ASSISTANT:
        break;
    default:
        return;                                                      // This should never happen
    }

    // Switch the actual flag
    ToggleGroupMemberFlag(slot, flag, apply);

    // Preserve the new setting in the db
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_FLAG);

    stmt->setUInt8(0, slot->flags);
    stmt->setUInt32(1, GUID_LOPART(guid));

    CharacterDatabase.Execute(stmt);

    // Broadcast the changes to the group
    SendUpdate();
}

Difficulty Group::GetDifficulty(bool isRaid) const
{
    return isRaid ? m_raidDifficulty : m_dungeonDifficulty;
}

Difficulty Group::GetDungeonDifficulty() const
{
    return m_dungeonDifficulty;
}

Difficulty Group::GetRaidDifficulty() const
{
    return m_raidDifficulty;
}

bool Group::isRollLootActive() const
{
    return !RollId.empty();
}

void Group::ErraseRollbyRealSlot(uint8 slot, Loot* loot)
{
    for (Rolls::iterator iter=RollId.begin(); iter != RollId.end();++iter)
    {
        if ((*iter)->itemSlot == slot && loot == (*iter)->getLoot() && (*iter)->isValid() )
        {
            delete *iter;
            RollId.erase(iter);
            break;
        }
    }
}

Group::Rolls::iterator Group::GetRoll(uint8 _aoeSlot)
{
    Rolls::iterator iter;
    for (iter=RollId.begin(); iter != RollId.end(); ++iter)
        if ((*iter)->aoeSlot == _aoeSlot && (*iter)->isValid())
            return iter;
    return RollId.end();
}

void Group::LinkMember(GroupReference* pRef)
{
    m_memberMgr.insertFirst(pRef);
}

void Group::DelinkMember(uint64 guid)
{
    GroupReference* ref = m_memberMgr.getFirst();
    while (ref)
    {
        GroupReference* nextRef = ref->next();
        if (ref->getSource()->GetGUID() == guid)
        {
            ref->unlink();
            break;
        }
        ref = nextRef;
    }
}

Group::BoundInstancesMap& Group::GetBoundInstances(Difficulty difficulty)
{
    return m_boundInstances[difficulty];
}

void Group::_initRaidSubGroupsCounter()
{
    // Sub group counters initialization
    if (!m_subGroupsCounts)
        m_subGroupsCounts = new uint8[MAX_RAID_SUBGROUPS];

    memset((void*)m_subGroupsCounts, 0, (MAX_RAID_SUBGROUPS)*sizeof(uint8));

    for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        ++m_subGroupsCounts[itr->group];
}

Group::member_citerator Group::_getMemberCSlot(uint64 Guid) const
{
    for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->guid == Guid)
            return itr;
    return m_memberSlots.end();
}

Group::member_witerator Group::_getMemberWSlot(uint64 Guid)
{
    for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->guid == Guid)
            return itr;
    return m_memberSlots.end();
}

void Group::SubGroupCounterIncrease(uint8 subgroup)
{
    if (m_subGroupsCounts)
        ++m_subGroupsCounts[subgroup];
}

void Group::SubGroupCounterDecrease(uint8 subgroup)
{
    if (m_subGroupsCounts)
        --m_subGroupsCounts[subgroup];
}

void Group::RemoveUniqueGroupMemberFlag(GroupMemberFlags flag)
{
    for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->flags & flag)
            itr->flags &= ~flag;
}

void Group::ToggleGroupMemberFlag(member_witerator slot, uint8 flag, bool apply)
{
    if (apply)
        slot->flags |= flag;
    else
        slot->flags &= ~flag;
}

bool Group::IsGuildGroup(uint32 guildId, bool AllInSameMap, bool AllInSameInstanceId)
{
    uint32 mapId = 0;
    uint32 InstanceId = 0;
    uint32 count = 0;
    std::vector<Player*> members;
    // First we populate the array
    for (GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next()) // Loop trought all members
        if (Player *player = itr->getSource())
            if (player->GetGuildId() == guildId) // Check if it has a guild
                members.push_back(player);

    bool ret = false;
    count = members.size();
    for (std::vector<Player*>::iterator itr = members.begin(); itr != members.end(); ++itr) // Iterate through players
    {
        if (Player* player = (*itr))
        {
            if (mapId == 0)
                mapId = player->GetMapId();

            if (InstanceId == 0)
                InstanceId = player->GetInstanceId();

            if (player->GetMap()->IsNonRaidDungeon() && !ret)
                if (count >= 3)
                    ret = true;

            if (player->GetMap()->IsRaid() && !ret)
            {
                switch (player->GetMap()->GetDifficulty())
                {
                    case MAN10_DIFFICULTY:
                    case MAN10_HEROIC_DIFFICULTY:
                        if (count >= 8)
                            ret = true;
                        break;

                    case MAN25_DIFFICULTY:
                    case MAN25_HEROIC_DIFFICULTY:
                        if (count >= 20)
                            ret = true;
                        break;
                }
            }

            if (player->GetMap()->IsBattleArena() && !ret)
                if (count == GetMembersCount())
                    ret = true;

            if (player->GetMap()->IsBattleground() && !ret)
                if (Battleground* bg = player->GetBattleground())
                    if (count >= uint32(bg->GetMaxPlayers() * 0.8f))
                        ret = true;

            // ToDo: Check 40-player raids: 10/40

            if (AllInSameMap && (mapId != player->GetMapId()))
                return false;

            if (AllInSameInstanceId && (InstanceId != player->GetInstanceId()))
                return false;
        }
    }

    return ret;
}

void Group::UpdateGuildAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1, uint32 miscValue2, uint32 miscValue3, Unit* pUnit, WorldObject* pRewardSource)
{
    // We will update criteria for each guild in grouplist but only once
    std::list<uint32> guildList;
    for (GroupReference *itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player *pPlayer = itr->getSource())
        {
            // Check for reward
            if (pRewardSource)
            {
                if (!pPlayer->IsAtGroupRewardDistance(pRewardSource))
                    continue;

                if (!pPlayer->isAlive())
                    continue;
            }

            uint32 guildId = pPlayer->GetGuildId();

            if (guildId == 0)
                continue;

            if (!guildList.empty())
            {
                // if we already have any guild in list
                // then check new guild
                bool bUnique = true;
                for (std::list<uint32>::const_iterator itr2 = guildList.begin(); itr2 != guildList.end(); ++itr2)
                {
                    if ((*itr2) == guildId)
                    {
                        bUnique = false;
                        break;
                    }
                }
                // If we have already rewarded current guild then continue
                // else update criteria 
                if (bUnique && guildId)
                {
                    guildList.push_back(guildId);
                    //if (Guild* pGuild = sGuildMgr->GetGuildById(guildId))
                        //pGuild->GetAchievementMgr().UpdateAchievementCriteria(type, miscValue1, miscValue2, miscValue3, pUnit, pPlayer);
                }
            }
            else
            {
                // If that's first guild in list
                // then add to the list and update criteria
                guildList.push_back(guildId);
                //if (Guild* pGuild = sGuildMgr->GetGuildById(guildId))
                    //pGuild->GetAchievementMgr().UpdateAchievementCriteria(type, miscValue1, miscValue2, miscValue3, pUnit, pPlayer);
            }
        }
    }
}

bool Group::leaderInstanceCheckFail()
{
    if (Player *leader = ObjectAccessor::FindPlayer(GetLeaderGUID()))
    {
        const uint32 leaderInstanceID = leader->GetInstanceId();
        if (leaderInstanceID == 0)
        {
            Player *pMember = NULL;
            for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
            {
                pMember = ObjectAccessor::FindPlayer(citr->guid);
                if ( pMember && (pMember->GetInstanceId() != leaderInstanceID) )
                    return true;
            }
        }       
    }

    return false;
}

uint32 Group::GetAverageMMR(BracketType bracket) const
{
    uint32 matchMakerRating = 0;
    uint32 playerDivider = 0;

    Player *pMember = NULL;
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        pMember = ObjectAccessor::FindPlayer(citr->guid);
        if (pMember)
        {
            matchMakerRating += pMember->getBracket(bracket)->getMMV();
            ++playerDivider;
        }
    }

    // x/0 = crash
    if (playerDivider == 0)
        playerDivider = 1;

    matchMakerRating /= playerDivider;

    return matchMakerRating;
}

void Group::SetRaidMarker(uint8 id, Player* who, uint64 targetGuid, bool update /*=true*/)
{
    if (!who)
        return;

    if (id >= RAID_MARKER_COUNT)
    {
        // remove all markers
        for (uint8 i = 0; i < RAID_MARKER_COUNT; ++i)
            m_raidMarkers[i] = 0;
    }
    else
        m_raidMarkers[id] = targetGuid;

    if (update)
        SendRaidMarkerUpdate();
}

void Group::SendRaidMarkerUpdate()
{
    WorldPacket data(SMSG_RAID_MARKERS_CHANGED, 4 + 1 + 1 + 5 * (4 + 4 + 4 + 4));
    data << uint8(IsHomeGroup() ? 0 : 1);
    uint32 mask = 0;
    uint8 count = 0;
    for (uint8 i = 0; i < RAID_MARKER_COUNT; ++i)
        if (m_raidMarkers[i])
        {
            mask |= 1 << i;
            ++count;
        }
    data << uint32(mask);

    data.WriteBits(count, 3);
    for (uint8 i = 0; i < count; ++i)
        data.WriteBits(0, 8);   // guid

    std::vector<uint32> pos(count);
    for (uint8 i = 0; i < count; ++i)
    {
        pos[i] = data.wpos();
        data << uint32(0);      // map id
        data << float(0.0f);    // x
        data << float(0.0f);    // y
        data << float(0.0f);    // z
    }

    for (GroupReference* itr = GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* player = itr->getSource();
        if (!player)
            continue;

        uint32 mapId = player->GetMapId();
        for (uint8 i = 0; i < count; ++i)
            data.put<uint32>(pos[i], mapId);
        player->SendDirectMessage(&data);
    }
}

void Group::ClearRaidMarker(uint64 guid)
{
    for (uint8 i = 0; i < RAID_MARKER_COUNT; ++i)
    {
        if (m_raidMarkers[i] == guid)
        {
            m_raidMarkers[i] = 0;
            SendRaidMarkerUpdate();
            break;
        }
    }
}

bool Group::HasRaidMarker(uint64 guid) const
{
    for (uint8 i = 0; i < RAID_MARKER_COUNT; ++i)
        if (m_raidMarkers[i] == guid)
            return true;

    return false;
}
