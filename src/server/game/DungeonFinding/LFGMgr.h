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

#ifndef _LFGMGR_H
#define _LFGMGR_H

#include "Common.h"
#include <ace/Singleton.h>
#include "LFG.h"
#include "LFGQueue.h"
#include "LFGGroupData.h"
#include "LFGPlayerData.h"

class Group;
class Player;
class Quest;

namespace lfg
{

enum LfgOptions
{
    LFG_OPTION_ENABLE_DUNGEON_FINDER             = 0x01,
    LFG_OPTION_ENABLE_RAID_BROWSER               = 0x02,
};

enum LFGMgrEnum
{
    LFG_TIME_ROLECHECK                           = 45 * IN_MILLISECONDS,
    LFG_TIME_BOOT                                = 30,
    LFG_TIME_PROPOSAL                            = 45,
    LFG_QUEUEUPDATE_INTERVAL                     = 15 * IN_MILLISECONDS,
    LFG_SPELL_DUNGEON_COOLDOWN                   = 71328,
    LFG_SPELL_DUNGEON_DESERTER                   = 71041,
    LFG_SPELL_LUCK_OF_THE_DRAW                   = 72221,

    LFG_DUNGEON_KICK_VOTES_NEEDED                = 3,
    LFG_RAID_KICK_VOTES_NEEDED                   = 15,
    LFG_SCENARIO_KICK_VOTES_NEEDED               = 2,
};

/// Proposal states
enum LfgProposalState
{
    LFG_PROPOSAL_INITIATING                      = 0,
    LFG_PROPOSAL_FAILED                          = 1,
    LFG_PROPOSAL_SUCCESS                         = 2
};

/// Teleport errors
enum LfgTeleportError
{
    LFG_TELEPORTERROR_OK                         = 1,      // Internal use
    LFG_TELEPORTERROR_NOT_WHILE_FALLING          = 4,
    LFG_TELEPORTERROR_DONT_REPORT_3              = 5,
    LFG_TELEPORTERROR_DONT_REPORT                = 8,
    LFG_TELEPORTERROR_NOT_WHILE_FATIGUED         = 9,
    LFG_TELEPORTERROR_PLAYER_DEAD                = 10,
    LFG_TELEPORTERROR_DONT_REPORT_2              = 11,
    LFG_TELEPORTERROR_INVALID_TELEPORT_LOCATION  = 13,
};

/// Queue join results
enum LfgJoinResult
{
    LFG_JOIN_OK                                  = 0,      // Joined (no client msg)
    LFG_JOIN_FAILED                              = 28,     // Role Check failed because your group is not viable.
    LFG_JOIN_GROUPFULL                           = 29,     // Your group is already full.
    LFG_JOIN_INTERNAL_ERROR                      = 31,     // Internal LFG Error.
    LFG_JOIN_NOT_MEET_REQS                       = 32,     // You do not meet the requirements for the chosen dungeons.
    //LFG_JOIN_PARTY_NOT_MEET_REQS                 = 33,     // One or more party members do not meet the requirements for the chosen dungeons
    LFG_JOIN_MIXED_RAID_DUNGEON                  = 33,     // You cannot mix dungeons, raids, and random when picking dungeons.
    LFG_JOIN_MULTI_REALM                         = 34,     // The dungeon you chose does not support players from multiple realms.
    LFG_JOIN_DISCONNECTED                        = 35,     // One or more group members are pending invites or disconnected.
    LFG_JOIN_PARTY_INFO_FAILED                   = 36,     // Could not retrieve information about some party members.
    LFG_JOIN_DUNGEON_INVALID                     = 37,     // One or more dungeons was not valid.
    LFG_JOIN_DESERTER                            = 38,     // You can not queue for dungeons until your deserter debuff wears off.
    LFG_JOIN_PARTY_DESERTER                      = 39,     // One or more party members has a deserter debuff.
    LFG_JOIN_RANDOM_COOLDOWN                     = 40,     // You can not queue for random dungeons while on random dungeon cooldown.
    LFG_JOIN_PARTY_RANDOM_COOLDOWN               = 41,     // One or more party members are on random dungeon cooldown.
    LFG_JOIN_TOO_MUCH_MEMBERS                    = 42,     // You have too many group members to queue for that.
    LFG_JOIN_USING_BG_SYSTEM                     = 43,     // You cannot queue for a dungeon or raid while using battlegrounds or arenas.
    LFG_JOIN_ROLE_CHECK_FAILED_2                 = 44,     // The Role Check has failed.
    LFG_JOIN_TOO_FEW_MEMBERS                     = 50,     // You do not have enough group members to queue for that.
    LFG_JOIN_FAILED_REASON_TOO_MANY_LFG          = 51,     // You are queued for too many instances.
};

/// Role check states
enum LfgRoleCheckState
{
    LFG_ROLECHECK_DEFAULT                        = 0,      // Internal use = Not initialized.
    LFG_ROLECHECK_FINISHED                       = 1,      // Role check finished
    LFG_ROLECHECK_INITIALITING                   = 2,      // + Role check begins
    LFG_ROLE_CHECK_FAILED_TIMEOUT                = 3,      // + Role Check failed because a group member did not respond.
    LFG_ROLECHECK_WRONG_ROLES                    = 4,      // + Role Check failed because your group is not viable.
    LFG_ROLECHECK_ABORTED                        = 5,      // ? Your group leader has cancelled the Role Check.
    LFG_ROLECHECK_NO_ROLE                        = 6       // Silent
};

// Forward declaration (just to have all typedef together)
struct LFGDungeonData;
struct LfgReward;
struct LfgRoleCheck;
struct LfgProposal;
struct LfgProposalPlayer;
struct LfgPlayerBoot;

typedef std::map<uint8, LFGQueue> LfgQueueContainer;
typedef std::multimap<uint32, LfgReward const*> LfgRewardContainer;
typedef std::pair<LfgRewardContainer::const_iterator, LfgRewardContainer::const_iterator> LfgRewardContainerBounds;
typedef std::map<uint16, LfgDungeonSet> LfgCachedDungeonContainer;
typedef std::map<uint64, LfgAnswer> LfgAnswerContainer;
typedef std::map<uint64, LfgRoleCheck> LfgRoleCheckContainer;
typedef std::map<uint32, LfgProposal> LfgProposalContainer;
typedef std::map<uint64, LfgProposalPlayer> LfgProposalPlayerContainer;
typedef std::map<uint64, LfgPlayerBoot> LfgPlayerBootContainer;
typedef std::map<uint64, LfgGroupData> LfgGroupDataContainer;
typedef std::map<uint64, LfgPlayerData> LfgPlayerDataContainer;
typedef UNORDERED_MAP<uint32, LFGDungeonData> LFGDungeonContainer;

// Data needed by SMSG_LFG_JOIN_RESULT
struct LfgJoinResultData
{
    LfgJoinResultData(LfgJoinResult _result = LFG_JOIN_OK, LfgRoleCheckState _state = LFG_ROLECHECK_DEFAULT):
        result(_result), state(_state) { }
    LfgJoinResult result;
    LfgRoleCheckState state;
    LfgLockPartyMap lockmap;
};

// Data needed by SMSG_LFG_UPDATE_PARTY and SMSG_LFG_UPDATE_PLAYER
struct LfgUpdateData
{
    LfgUpdateData(LfgUpdateType _type = LFG_UPDATETYPE_DEFAULT): updateType(_type), state(LFG_STATE_NONE), comment("") { }
    LfgUpdateData(LfgUpdateType _type, LfgDungeonSet const& _dungeons, std::string const& _comment = "") :
        updateType(_type), state(LFG_STATE_NONE), dungeons(_dungeons), comment(_comment) { }
    LfgUpdateData(LfgUpdateType _type, LfgState _state, LfgDungeonSet const& _dungeons, std::string const& _comment = ""):
        updateType(_type), state(_state), dungeons(_dungeons), comment(_comment) { }

    LfgUpdateType updateType;
    LfgState state;
    LfgDungeonSet dungeons;
    std::string comment;
};

// Data needed by SMSG_LFG_QUEUE_STATUS
struct LfgQueueStatusData
{
    LfgQueueStatusData(uint32 _dungeonId = 0, int32 _waitTime = -1, int32 _waitTimeAvg = -1, int32 _waitTimeTank = -1, int32 _waitTimeHealer = -1,
        int32 _waitTimeDps = -1, uint32 _queuedTime = 0, LfgQueueData* _queueInfo = NULL) :
        dungeonId(_dungeonId), waitTime(_waitTime), waitTimeAvg(_waitTimeAvg), waitTimeTank(_waitTimeTank), waitTimeHealer(_waitTimeHealer),
        waitTimeDps(_waitTimeDps), queuedTime(_queuedTime), queueInfo(_queueInfo) { }

    uint32 dungeonId;
    int32 waitTime;
    int32 waitTimeAvg;
    int32 waitTimeTank;
    int32 waitTimeHealer;
    int32 waitTimeDps;
    uint32 queuedTime;
    LfgQueueData* queueInfo;
};

struct LfgPlayerRewardData
{
    LfgPlayerRewardData(uint32 random, uint32 current, bool _done, bool _bonus, LfgReward const* _reward):
        rdungeonEntry(random), sdungeonEntry(current), done(_done), bonus(_bonus), reward(_reward) { }
    uint32 rdungeonEntry;
    uint32 sdungeonEntry;
    bool done;
    bool bonus;
    LfgReward const* reward;
};

/// Reward info
struct LfgReward
{
    LfgReward(uint32 _maxLevel = 0, uint32 _firstQuest = 0, uint32 _otherQuest = 0, uint32 _bonusQuestId = 0):
        maxLevel(_maxLevel), firstQuest(_firstQuest), otherQuest(_otherQuest), bonusQuestId(_bonusQuestId) { }

    bool RewardPlayer(Player* player, LFGDungeonData const* randomDungeon, LFGDungeonData const* normalDungeon, bool bonusObjective) const;

    uint32 maxLevel;
    uint32 firstQuest;
    uint32 otherQuest;
    uint32 bonusQuestId;
};

/// Stores player data related to proposal to join
struct LfgProposalPlayer
{
    LfgProposalPlayer(): role(0), accept(LFG_ANSWER_PENDING), group(0) { }
    uint8 role;                                            ///< Proposed role
    LfgAnswer accept;                                      ///< Accept status (-1 not answer | 0 Not agree | 1 agree)
    uint64 group;                                          ///< Original group guid. 0 if no original group
};

/// Stores group data related to proposal to join
struct LfgProposal
{
    LfgProposal(uint32 dungeon = 0): id(0), dungeonId(dungeon), state(LFG_PROPOSAL_INITIATING),
        group(0), leader(0), cancelTime(0), encounters(0), isNew(true)
        { }

    uint32 id;                                             ///< Proposal Id
    uint32 dungeonId;                                      ///< Dungeon to join
    LfgProposalState state;                                ///< State of the proposal
    uint64 group;                                          ///< Proposal group (0 if new)
    uint64 leader;                                         ///< Leader guid.
    time_t cancelTime;                                     ///< Time when we will cancel this proposal
    uint32 encounters;                                     ///< Dungeon Encounters
    bool isNew;                                            ///< Determines if it's new group or not
    LfgGuidList queues;                                    ///< Queue Ids to remove/readd
    LfgGuidList showorder;                                 ///< Show order in update window
    LfgProposalPlayerContainer players;                    ///< Players data
};

/// Stores all rolecheck info of a group that wants to join
struct LfgRoleCheck
{
    time_t cancelTime;                                     ///< Time when the rolecheck will fail
    LfgRolesMap roles;                                     ///< Player selected roles
    LfgRoleCheckState state;                               ///< State of the rolecheck
    LfgDungeonSet dungeons;                                ///< Dungeons group is applying for (expanded random dungeons)
    uint32 rDungeonId;                                     ///< Random Dungeon Id.
    uint64 leader;                                         ///< Leader of the group
};

/// Stores information of a current vote to kick someone from a group
struct LfgPlayerBoot
{
    time_t cancelTime;                                     ///< Time left to vote
    bool inProgress;                                       ///< Vote in progress
    LfgAnswerContainer votes;                              ///< Player votes (-1 not answer | 0 Not agree | 1 agree)
    uint8 votesNeeded;
    uint64 victim;                                         ///< Player guid to be kicked (can't vote)
    std::string reason;                                    ///< kick reason
};

struct LfgRoleData
{
    LfgRoleData(LFGDungeonData const* data);
    LfgRoleData(uint32 dungeonId);

    void Init(LFGDungeonData const* data);

    uint8 tanksNeeded;
    uint8 healerNeeded;
    uint8 dpsNeeded;
    uint8 minTanksNeeded;
    uint8 minHealerNeeded;
    uint8 minDpsNeeded;
};

struct LFGDungeonData
{
    LFGDungeonData(): id(0), name(""), map(0), type(0), expansion(0), random_id(0), minlevel(0),
        maxlevel(0), difficulty(REGULAR_DIFFICULTY), seasonal(false), x(0.0f), y(0.0f), z(0.0f), o(0.0f), internalType(LFG_TYPE_DUNGEON), dbc(NULL)
       { }
    LFGDungeonData(LFGDungeonEntry const* _dbc): id(_dbc->ID), name(_dbc->name), map(_dbc->map),
        type(_dbc->type), expansion(_dbc->expansion), random_id(_dbc->random_id),
        minlevel(_dbc->minlevel), maxlevel(_dbc->maxlevel), difficulty(Difficulty(_dbc->difficulty)),
        seasonal(_dbc->flags & LFG_FLAG_SEASONAL), x(0.0f), y(0.0f), z(0.0f), o(0.0f),
        internalType(_dbc->GetInternalType()), dbc(_dbc)
        { }

    uint32 id;
    std::string name;
    uint16 map;
    uint8 type;
    uint8 expansion;
    uint32 random_id;
    uint8 minlevel;
    uint8 maxlevel;
    Difficulty difficulty;
    bool seasonal;
    uint8 internalType;
    float x, y, z, o;
    LFGDungeonEntry const* dbc;

    // Helpers
    uint32 Entry() const { return id + (type << 24); }
};

class LFGMgr
{
    friend class ACE_Singleton<LFGMgr, ACE_Null_Mutex>;

    private:
        LFGMgr();
        ~LFGMgr();

    public:
        // Functions used outside lfg namespace
        void Update(uint32 diff);

        // World.cpp
        /// Finish the dungeon for the given group. All check are performed using internal lfg data
        void FinishDungeon(uint64 gguid, uint32 dungeonId);
        /// Loads rewards for random dungeons
        void LoadRewards();
        /// Loads dungeons from dbc and adds teleport coords
        void LoadLFGDungeons(bool reload = false);

        // Multiple files
        /// Check if given guid applied for random dungeon
        bool selectedRandomLfgDungeon(uint64 guid);
        /// Check if given guid applied for given map and difficulty. Used to know
        bool inLfgDungeonMap(uint64 guid, uint32 map, Difficulty difficulty);
        /// Get selected dungeons
        LfgDungeonSet const& GetSelectedDungeons(uint64 guid);
        /// Get current lfg state
        LfgState GetState(uint64 guid);
        /// Get current dungeon
        uint32 GetDungeon(uint64 guid, bool asId = true);
        /// Get the map id of the current dungeon
        uint32 GetDungeonMapId(uint64 guid);
        /// Get kicks left in current group
        uint8 GetKicksLeft(uint64 gguid);
        /// Load Lfg group info from DB
        void _LoadFromDB(Field* fields, uint64 guid);
        /// Initializes player data after loading group data from DB
        void SetupGroupMember(uint64 guid, uint64 gguid);
        /// Return Lfg dungeon entry for given dungeon id
        uint32 GetLFGDungeonEntry(uint32 id);
        LFGDungeonData const* GetLFGDungeon(uint32 id);
        LFGDungeonData const* GetLFGDungeon(uint32 mapId, Difficulty diff, uint32 team);

        // cs_lfg
        /// Get current player roles
        uint8 GetRoles(uint64 guid);
        /// Get current player comment (used for LFR)
        std::string const& GetComment(uint64 gguid);
        /// Gets current lfg options
        uint32 GetOptions();
        /// Sets new lfg options
        void SetOptions(uint32 options);
        /// Checks if given lfg option is enabled
        bool isOptionEnabled(uint32 option);
        /// Clears queue - Only for internal testing
        void Clean();
        /// Dumps the state of the queue - Only for internal testing
        std::string DumpQueueInfo(bool full = false);

        // LFGScripts
        /// Get leader of the group (using internal data)
        uint64 GetLeader(uint64 guid);
        /// Sets player team
        void SetTeam(uint64 guid, uint8 team);
        /// Sets player group
        void SetGroup(uint64 guid, uint64 group);
        /// Gets player group
        uint64 GetGroup(uint64 guid);
        /// Sets the leader of the group
        void SetLeader(uint64 gguid, uint64 leader);
        /// Removes saved group data
        void RemoveGroupData(uint64 guid);
        /// Removes a player from a group
        uint8 RemovePlayerFromGroup(uint64 gguid, uint64 guid);
        /// Adds player to group
        void AddPlayerToGroup(uint64 gguid, uint64 guid);

        // LFGHandler
        /// Get locked dungeons
        LfgLockMap const GetLockedDungeons(uint64 guid);
        /// Returns current lfg status
        LfgUpdateData GetLfgStatus(uint64 guid);
        /// Checks if Seasonal dungeon is active
        bool IsSeasonActive(uint32 dungeonId);
        /// Gets the dungeon reward corresponding to given dungeon and player level
        LfgReward const* GetDungeonReward(uint32 dungeon, uint8 level);
        /// Returns all rewardable dungeons for given level and expansion
        LfgDungeonSet GetRewardableDungeons(uint8 level, uint8 expansion);
        ///
        uint32 GetBonusValorPoints(uint32 dungeonId) const;
        /// Teleport a player to/from selected dungeon
        void TeleportPlayer(Player* player, bool out, bool fromOpcode = false);
        /// Inits new proposal to boot a player
        void InitBoot(uint64 gguid, uint64 kguid, uint64 vguid, std::string const& reason);
        /// Updates player boot proposal with new player answer
        void UpdateBoot(uint64 guid, bool accept);
        /// Updates proposal to join dungeon with player answer
        void UpdateProposal(uint32 proposalId, uint64 guid, bool accept);
        /// Updates the role check with player answer
        void UpdateRoleCheck(uint64 gguid, uint64 guid = 0, uint8 roles = PLAYER_ROLE_NONE);
        /// Sets player lfg roles
        void SetRoles(uint64 guid, uint8 roles);
        /// Sets player lfr comment
        void SetComment(uint64 guid, std::string const& comment);
        /// Join Lfg with selected roles, dungeons and comment
        void JoinLfg(Player* player, uint8 roles, LfgDungeonSet& dungeons, std::string const& comment);
        /// Leaves lfg
        void LeaveLfg(uint64 guid);

        // LfgQueue
        /// Get last lfg state (NONE, DUNGEON or FINISHED_DUNGEON)
        LfgState GetOldState(uint64 guid);
        LFGQueue& GetQueue(uint64 guid);

        /// Check if given group guid is lfg
        bool IsLfgGroup(uint64 guid);
        /// Gets the player count of given group
        uint8 GetPlayerCount(uint64 guid);
        /// Add a new Proposal
        uint32 AddProposal(LfgProposal& proposal);
        /// Checks if all players are queued
        bool AllQueued(LfgGuidList const& check);
        /// Checks if given roles match, modifies given roles map with new roles
        static bool CheckGroupRoles(LfgRolesMap& groles, LfgRoleData const& roleData, bool removeLeaderFlag = true);
        /// Checks if given players are ignoring each other
        static bool HasIgnore(uint64 guid1, uint64 guid2);
        /// Sends queue status to player
        static void SendLfgQueueStatus(uint64 guid, LfgQueueStatusData const& data);
        void SendUpdateStatus(Player* player, LfgUpdateData const& updateData, bool party);

        uint8 GetVotesNeededForKick(uint64 gguid);

    private:
        uint8 GetTeam(uint64 guid);
        void RestoreState(uint64 guid, char const* debugMsg);
        void ClearState(uint64 guid, char const* debugMsg);
        void SetDungeon(uint64 guid, uint32 dungeon);
        void SetSelectedDungeons(uint64 guid, LfgDungeonSet const& dungeons);
        void DecreaseKicksLeft(uint64 guid);
        void SetState(uint64 guid, LfgState state);
        void RemovePlayerData(uint64 guid);
        void GetCompatibleDungeons(LfgDungeonSet& dungeons, LfgGuidSet const& players, LfgLockPartyMap& lockMap);
        void _SaveToDB(uint64 guid, uint32 db_guid);

        // Proposals
        void RemoveProposal(LfgProposalContainer::iterator itProposal, LfgUpdateType type);
        void MakeNewGroup(LfgProposal const& proposal);

        // Generic
        LfgDungeonSet const& GetDungeonsByRandom(uint32 randomdungeon);
        LfgType GetDungeonType(uint32 dungeon);

        void SendLfgBootProposalUpdate(uint64 guid, LfgPlayerBoot const& boot);
        void SendLfgJoinResult(uint64 guid, LfgJoinResultData const& data);
        void SendLfgRoleChosen(uint64 guid, uint64 pguid, uint8 roles);
        void SendLfgRoleCheckUpdate(uint64 guid, LfgRoleCheck const& roleCheck);
        void SendLfgUpdateParty(uint64 guid, LfgUpdateData const& data);
        void SendLfgUpdatePlayer(uint64 guid, LfgUpdateData const& data);
        void SendLfgUpdateProposal(uint64 guid, LfgProposal const& proposal);

        LfgGuidSet const& GetPlayers(uint64 guid);

        // General variables
        uint32 m_QueueTimer;                               ///< used to check interval of update
        uint32 m_lfgProposalId;                            ///< used as internal counter for proposals
        uint32 m_options;                                  ///< Stores config options

        LfgQueueContainer QueuesStore;                     ///< Queues
        LfgCachedDungeonContainer CachedDungeonMapStore;   ///< Stores all dungeons by groupType
        // Reward System
        LfgRewardContainer RewardMapStore;                 ///< Stores rewards for random dungeons
        LFGDungeonContainer  LfgDungeonStore;
        // Rolecheck - Proposal - Vote Kicks
        LfgRoleCheckContainer RoleChecksStore;             ///< Current Role checks
        LfgProposalContainer ProposalsStore;               ///< Current Proposals
        LfgPlayerBootContainer BootsStore;                 ///< Current player kicks
        LfgPlayerDataContainer PlayersStore;               ///< Player data
        LfgGroupDataContainer GroupsStore;                 ///< Group data
};

} // namespace lfg

#define sLFGMgr ACE_Singleton<lfg::LFGMgr, ACE_Null_Mutex>::instance()
#endif
