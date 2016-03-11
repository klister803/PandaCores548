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
#include "GroupMgr.h"
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
#include "AccountMgr.h"
#include "TicketMgr.h"
#include "RedisLoadPlayer.h"
#include "AchievementMgr.h"
#include "RedisBuilderMgr.h"
#include "GuildMgr.h"

void Player::LoadFromRedis(uint64 guid, uint8 step)
{
    //sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadFromRedis player guid %u%u step %i get_id %i", guid, step, boost::this_thread::get_id());

    switch (step)
    {
        case LOAD_PLAYER_HOMEBIND: //Load player Home Bind
        {
            InitCharKeys(GUID_LOPART(guid));

            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayer itemKey %s userKey %s accountKey %s criteriaPlKey %s criteriaAcKey %s",
            GetItemKey(), GetUserKey(), GetAccountKey(), GetCriteriaPlKey(), GetCriteriaAcKey());

            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "homebind", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["homebind"]))
                        loadingPlayer->LoadPlayerHomeBind();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_DATA); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_DATA: //Load player data
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "userdata", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (!sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["data"]))
                    {
                        sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayer data is empty");
                        GetSession()->HandlePlayerLogin(GetSession()->GetAccountId(), guid, 2);
                    }
                    else
                    {
                        loadingPlayer->LoadPlayer(guid);
                        loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_GOLD); //Next step load
                    }
                }
            });
            break;
        }
        case LOAD_PLAYER_GOLD: //Load player money
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "money", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["gold"]))
                        loadingPlayer->LoadPlayerGold();

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_ACHIEVEMENT); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_ACHIEVEMENT: //Load account Achievement
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "achievement", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["achievement"]))
                        loadingPlayer->LoadAccountAchievements();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_ACHIEVEMENT); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_ACHIEVEMENT: //Load player Achievement
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "achievement", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["achievement"]))
                        loadingPlayer->LoadPlayerAchievements();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_CRITERIA); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_CRITERIA: //Load player Criteria Progress
        {
            RedisDatabase.AsyncExecute("HGETALL", GetCriteriaPlKey(), guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    std::vector<RedisValue> progressVector;
                    if (sRedisBuilderMgr->LoadFromRedisArray(&v, progressVector))
                        loadingPlayer->LoadPlayerCriteriaProgress(&progressVector);

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_CRITERIA); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_CRITERIA: //Load account Achievement
        {
            RedisDatabase.AsyncExecute("HGETALL", GetCriteriaAcKey(), guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    std::vector<RedisValue> progressVector;
                    if (sRedisBuilderMgr->LoadFromRedisArray(&v, progressVector))
                        loadingPlayer->LoadAccountCriteriaProgress(&progressVector);

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_NEXT); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_NEXT: //Load player data next
        {
            LoadPlayerNext(guid);
            LoadFromRedis(guid, LOAD_PLAYER_GROUP); //Next step load
            break;
        }
        case LOAD_PLAYER_GROUP: //Load player group
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "group", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["group"]))
                        loadingPlayer->LoadPlayerGroup();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_LOOTCOOLDOWN); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_LOOTCOOLDOWN: //Load player loot cooldown
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "lootCooldown", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["lootcooldown"]))
                        loadingPlayer->LoadPlayerLootCooldown();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_CURRENCY); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_CURRENCY: //Load player currency
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "currency", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["currency"]))
                        loadingPlayer->LoadPlayerCurrency();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_BOUNDINSTANCES); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_BOUNDINSTANCES: //Load player Bound Instances
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "boundinstances", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["boundinstances"]))
                        loadingPlayer->LoadPlayerBoundInstances();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_BGDATA); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_BGDATA: //Load player BG Data
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "BGdata", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["bg"]))
                        loadingPlayer->LoadPlayerBG();

                    loadingPlayer->InitSecondPartDataPlayer();
                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_BATTLEPETS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_BATTLEPETS: //Load player Battle Pets
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "battlepets", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["battlepets"]))
                        loadingPlayer->LoadPlayerBattlePets();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_BATTLEPETSLOTS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_BATTLEPETSLOTS: //Load player Battle Pets Slot
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "battlepetslots", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (!sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["battlepetslots"]))
                    {
                        for (int i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
                            loadingPlayer->GetBattlePetMgr()->InitBattleSlot(0, i);
                    }
                    else
                        loadingPlayer->LoadPlayerBattlePetSlots();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_SKILLS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_SKILLS: //Load player Skills
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "skills", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["skills"]))
                        loadingPlayer->LoadPlayerSkills();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_ARCHAEOLOGY); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_ARCHAEOLOGY: //Load player archaeology
        {
            for (uint8 i = 0; i < MAX_RESEARCH_SITES; ++i)
                _digSites[i].count = 0;

            if (!sWorld->getBoolConfig(CONFIG_ARCHAEOLOGY_ENABLED))
            {
                LoadFromRedis(guid, LOAD_PLAYER_TALENTS); //Next step load
                return;
            }

            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "archaeology", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (!sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["archaeology"]))
                    {
                        loadingPlayer->GenerateResearchSites();
                        loadingPlayer->GenerateResearchProjects();
                    }
                    else
                        loadingPlayer->LoadPlayerArchaeology();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_TALENTS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_TALENTS: //Load player Spec and Talent
        {
            uint8 specCount = PlayerData["data"]["speccount"].asUInt();
            SetSpecsCount(specCount == 0 ? 1 : specCount);
            SetActiveSpec(specCount > 1 ? PlayerData["data"]["activespec"].asUInt() : 0);

            SetSpecializationId(0, PlayerData["data"]["specialization1"].asUInt());
            SetSpecializationId(1, PlayerData["data"]["specialization2"].asUInt());

            SetFreeTalentPoints(CalculateTalentsPoints());

            // sanity check
            if (GetSpecsCount() > MAX_TALENT_SPECS || GetActiveSpec() > MAX_TALENT_SPEC || GetSpecsCount() < MIN_TALENT_SPECS)
            {
                SetActiveSpec(0);
                sLog->outError(LOG_FILTER_PLAYER, "Player %s(GUID: %u) has SpecCount = %u and ActiveSpec = %u.", GetName(), GetGUIDLow(), GetSpecsCount(), GetActiveSpec());
            }

            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "talents", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["talents"]))
                        loadingPlayer->LoadPlayerTalents();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_SPELLS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_SPELLS: //Load player spells
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "spells", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["spells"]))
                        loadingPlayer->LoadPlayerSpells();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_MOUNTS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_MOUNTS: //Load player mounts
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "mounts", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["mounts"]))
                        loadingPlayer->LoadPlayerMounts();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_GLYPHS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_GLYPHS: //Load player glyphs
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "glyphs", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["glyphs"]))
                        loadingPlayer->LoadPlayerGlyphs();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_AURAS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_AURAS: //Load player auras
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "auras", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["auras"]))
                        loadingPlayer->LoadPlayerAuras();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_QUESTSTATUS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_QUESTSTATUS: //Load player Quest Status
        {
            _LoadGlyphAuras();
            // add ghost flag (must be after aura load: PLAYER_FLAGS_GHOST set in aura)
            if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
                m_deathState = DEAD;

            // after spell load, learn rewarded spell if need also - test on achievements
            _LoadSpellRewards();

            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "queststatus", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["queststatus"]))
                        loadingPlayer->LoadPlayerQuestStatus();

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_QUESTSTATUS); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_QUESTSTATUS: //Load account Quest Status
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "queststatus", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["queststatus"]))
                        loadingPlayer->LoadAccountQuestStatus();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_QUESTREWARDED); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_QUESTREWARDED: //Load player Quest rewarded
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "questrewarded", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["questrewarded"]))
                        loadingPlayer->LoadPlayerQuestRewarded();

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_QUESTREWARDED); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_QUESTREWARDED: //Load account Quest rewarded
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "questrewarded", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["questrewarded"]))
                        loadingPlayer->LoadAccountQuestRewarded();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_QUESTDAILY); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_QUESTDAILY: //Load player Quest Daily
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "questdaily", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["questdaily"]))
                        loadingPlayer->LoadPlayerQuestDaily();

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_QUESTDAILY); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_QUESTDAILY: //Load account Quest Daily
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "questdaily", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["questdaily"]))
                        loadingPlayer->LoadAccountQuestDaily();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_QUESTWEEKLY); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_QUESTWEEKLY: //Load player Quest Weekly
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "questweekly", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["questweekly"]))
                        loadingPlayer->LoadPlayerQuestWeekly();

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_QUESTWEEKLY); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_QUESTWEEKLY: //Load account Quest Weekly
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "questweekly", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["questweekly"]))
                        loadingPlayer->LoadAccountQuestWeekly();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_QUESTSEASONAL); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_QUESTSEASONAL: //Load player Quest Seasonal
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "questseasonal", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["questseasonal"]))
                        loadingPlayer->LoadPlayerQuestSeasonal();

                    loadingPlayer->LoadFromRedis(guid, LOAD_ACCOUNT_QUESTSEASONAL); //Next step load
                }
            });
            break;
        }
        case LOAD_ACCOUNT_QUESTSEASONAL: //Load account Quest Seasonal
        {
            RedisDatabase.AsyncExecuteH("HGET", GetAccountKey(), "questseasonal", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->AccountDatas["questseasonal"]))
                        loadingPlayer->LoadAccountQuestSeasonal();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_REPUTATION); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_REPUTATION: //Load player Reputation
        {
            //_LoadRandomBGStatus(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADRANDOMBG));

            // after spell and quest load
            InitTalentForLevel();
            InitSpellForLevel();
            learnDefaultSpells();

            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "reputation", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["reputation"]))
                        loadingPlayer->m_reputationMgr.LoadFromDB();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_ITEMS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_ITEMS: //Load player items
        {
            RedisDatabase.AsyncExecute("HGETALL", GetItemKey(), guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    std::vector<RedisValue> itemVector;
                    if (sRedisBuilderMgr->LoadFromRedisArray(&v, itemVector))
                        loadingPlayer->LoadPlayerLoadItems(&itemVector);

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_VOIDSTORAGE); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_VOIDSTORAGE: //Load player Void Storage
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "voidstorage", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["voidstorage"]))
                        loadingPlayer->LoadPlayerVoidStorage();

                    LoadFromRedis(guid, LOAD_PLAYER_ACTIONS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_ACTIONS: //Load player Actions
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "actions", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["actions"]))
                        loadingPlayer->LoadPlayerActions();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_SOCIAL); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_SOCIAL: //Load player social
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "social", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["social"]);
                    sSocialMgr->LoadFromDB(loadingPlayer);
                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_SPELLCOOLDOWNS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_SPELLCOOLDOWNS: //Load player Spell Cooldowns
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "spellcooldowns", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["spellcooldowns"]))
                        loadingPlayer->LoadPlayerSpellCooldowns();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_KILLS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_KILLS: //Load player Kills
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "kills", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["spellcooldowns"]))
                        loadingPlayer->LoadPlayerKills();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_INIT_THIRD); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_INIT_THIRD: //Load player init third part data
        {
            InitThirdPartDataPlayer();
            LoadFromRedis(guid, LOAD_PLAYER_DECLINEDNAME); //Next step load
            break;
        }
        case LOAD_PLAYER_DECLINEDNAME: //Load player Declined Name
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "declinedname", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["declinedname"]))
                        loadingPlayer->LoadPlayerDeclinedName();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_EQUIPMENTSETS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_EQUIPMENTSETS: //Load player Equipment Sets
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "equipmentsets", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["equipmentsets"]))
                        loadingPlayer->LoadPlayerEquipmentSets();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_CUFPROFILES); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_CUFPROFILES: //Load player CUF Profiles
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "cufprofiles", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["cufprofiles"]))
                        loadingPlayer->LoadPlayerCUFProfiles();

                   loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_VISUALS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_VISUALS: //Load player visuals
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "visuals", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["visuals"]))
                        loadingPlayer->LoadPlayerVisuals();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_PLAYERACCOUNTDATA); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_PLAYERACCOUNTDATA: //Load player Account Data
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "playeraccountdata", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["accountdata"]))
                        loadingPlayer->LoadPlayerAccountData();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_PETS); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_LOGIN: //Load player Login
        {
            GetSession()->HandlePlayerLogin(GetSession()->GetAccountId(), guid, 1);
            LoadFromRedis(guid, LOAD_PLAYER_MAILS);
            break;
        }
        case LOAD_PLAYER_MAILS: //Load player mails
        {
            RedisDatabase.AsyncExecute("HGETALL", mailKey, guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* player = HashMapHolder<Player>::Find(guid))
                {
                    std::vector<RedisValue> mailVector;
                    if (sRedisBuilderMgr->LoadFromRedisArray(&v, mailVector))
                        player->LoadPlayerMails(&mailVector);

                    player->LoadFromRedis(guid, LOAD_PLAYER_MAIL_ITEMS);
                }
            });
            break;
        }
        case LOAD_PLAYER_MAIL_ITEMS: //Load player mail items
        {
            RedisDatabase.AsyncExecute("HGETALL", GetMailItemKey(), guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* player = HashMapHolder<Player>::Find(guid))
                {
                    std::vector<RedisValue> itemVector;
                    if (sRedisBuilderMgr->LoadFromRedisArray(&v, itemVector))
                        player->LoadPlayerMailItems(&itemVector);
                }
            });
            break;
        }
        case LOAD_PLAYER_PETS: //Load player pets
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "pets", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    sRedisBuilderMgr->LoadFromRedis(&v, PlayerData["pets"]);
                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_GUILD);
                }
            });
            break;
        }
        case LOAD_PLAYER_GUILD: //Load player guild
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "guild", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["guild"]))
                        loadingPlayer->LoadPlayerGuild();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_CORPSE); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_CORPSE: //Load player corpse
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "corpse", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["corpse"]))
                        loadingPlayer->LoadPlayerCorpse();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_PETITION); //Next step load
                }
            });
            break;
        }
        case LOAD_PLAYER_PETITION: //Load player petitions
        {
            RedisDatabase.AsyncExecuteH("HGET", GetUserKey(), "petitions", guid, [&](const RedisValue &v, uint64 guid) {
                if (Player* loadingPlayer = sObjectMgr->GetPlayerLoad(guid))
                {
                    if (sRedisBuilderMgr->LoadFromRedis(&v, loadingPlayer->PlayerData["petitions"]))
                        loadingPlayer->LoadPlayerPetition();

                    loadingPlayer->LoadFromRedis(guid, LOAD_PLAYER_LOGIN); //Next step load
                }
            });
            break;
        }
        default:
            break;
    }

    //sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadFromRedis end step %i get_id %i", step, boost::this_thread::get_id());
}

bool Player::LoadPlayer(uint64 playerGuid)
{
    uint32 guid = GUID_LOPART(playerGuid);

    if (!PlayerData.isMember("data") || PlayerData["data"].empty())
    {
        sLog->outError(LOG_FILTER_PLAYER, "Player (GUID: %u) loading from wrong account (is: %u)", guid, GetSession()->GetAccountId());
        GetSession()->HandlePlayerLogin(GetSession()->GetAccountId(), playerGuid, 2);
        return false;
    }

    uint8 m_realmID = PlayerData["data"]["realm"].asInt();

    uint32 dbAccountId = PlayerData["data"]["account"].asUInt();

    // check if the character's account in the db and the logged in account match.
    // player should be able to load/delete character only with correct account!
    if (dbAccountId != GetSession()->GetAccountId())
    {
        sLog->outError(LOG_FILTER_PLAYER, "Player (GUID: %u) loading from wrong account (is: %u, should be: %u)", guid, GetSession()->GetAccountId(), dbAccountId);
        GetSession()->HandlePlayerLogin(GetSession()->GetAccountId(), playerGuid, 2);
        return false;
    }

    // if (holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADBANNED))
    // {
        // sLog->outError(LOG_FILTER_PLAYER, "Player (GUID: %u) is banned, can't load.", guid);
        // return false;
    // }

    Object::_Create(guid, 0, HIGHGUID_PLAYER);

    m_name = PlayerData["data"]["name"].asString();

    // check name limitations
    // if (ObjectMgr::CheckPlayerName(m_name) != CHAR_NAME_SUCCESS ||
        // (AccountMgr::IsPlayerAccount(GetSession()->GetSecurity()) && sObjectMgr->IsReservedName(m_name)))
    // {
        // PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        // stmt->setUInt16(0, uint16(AT_LOGIN_RENAME));
        // stmt->setUInt32(1, guid);
        // CharacterDatabase.Execute(stmt);

        // return false;
    // }

    // overwrite possible wrong/corrupted guid
    SetUInt64Value(OBJECT_FIELD_GUID, MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER));

    uint8 Gender = PlayerData["data"]["gender"].asInt();
    if (!IsValidGender(Gender))
    {
        sLog->outError(LOG_FILTER_PLAYER, "Player (GUID: %u) has wrong gender (%hu), can't be loaded.", guid, Gender);
        GetSession()->HandlePlayerLogin(GetSession()->GetAccountId(), playerGuid, 2);
        return false;
    }

    // overwrite some data fields
    SetRace(PlayerData["data"]["race"].asInt());
    SetClass(PlayerData["data"]["class"].asInt());
    SetGender(Gender);


    SetUInt32Value(UNIT_FIELD_LEVEL, PlayerData["data"]["level"].asInt());
    SetUInt32Value(PLAYER_XP, PlayerData["data"]["xp"].asInt());

    _LoadIntoDataField(PlayerData["data"]["exploredZones"].asCString(), PLAYER_EXPLORED_ZONES_1, PLAYER_EXPLORED_ZONES_SIZE);
    _LoadIntoDataField(PlayerData["data"]["knownTitles"].asCString(), PLAYER_FIELD_KNOWN_TITLES, KNOWN_TITLES_SIZE * 2);

    SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, DEFAULT_WORLD_OBJECT_SIZE);
    SetFloatValue(UNIT_FIELD_COMBATREACH, 1.5f);
    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);

    return true;
}

bool Player::LoadPlayerNext(uint64 playerGuid)
{
    uint32 guid = GUID_LOPART(playerGuid);

    if (!PlayerData.isMember("data") || PlayerData["data"].empty())
    {
        sLog->outError(LOG_FILTER_PLAYER, "Player (GUID: %u) loading from wrong account (is: %u)", guid, GetSession()->GetAccountId());
        GetSession()->HandlePlayerLogin(GetSession()->GetAccountId(), playerGuid, 2);
        return false;
    }

    uint8 Gender = PlayerData["data"]["gender"].asInt();

    uint64 money = PlayerData["gold"].asUInt64();
    if (money > MAX_MONEY_AMOUNT)
        money = MAX_MONEY_AMOUNT;
    SetMoney(money);

    SetUInt32Value(PLAYER_BYTES, PlayerData["data"]["playerBytes"].asUInt());
    SetUInt32Value(PLAYER_BYTES_2, PlayerData["data"]["playerBytes2"].asUInt());
    SetByteValue(PLAYER_BYTES_3, 0, Gender);
    SetByteValue(PLAYER_BYTES_3, 1, PlayerData["data"]["drunk"].asUInt());
    SetUInt32Value(PLAYER_FLAGS, PlayerData["data"]["playerFlags"].asUInt());
    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, PlayerData["data"]["watchedFaction"].asUInt());

    // set which actionbars the client has active - DO NOT REMOVE EVER AGAIN (can be changed though, if it does change fieldwise)
    SetByteValue(PLAYER_FIELD_BYTES, 2, PlayerData["data"]["actionBars"].asUInt());

    m_currentPetNumber = PlayerData["data"]["currentpetnumber"].asUInt();
    LoadPetSlot(PlayerData["data"]["petslot"].asString());

    InitDisplayIds();

    // cleanup inventory related item value fields (its will be filled correctly in _LoadInventory)
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        SetUInt64Value(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2), 0);
        SetVisibleItemSlot(slot, NULL);

        delete m_items[slot];
        m_items[slot] = NULL;
    }

    sLog->outDebug(LOG_FILTER_REDIS, "Load Basic value of player %s is: ", m_name.c_str());
    outDebugValues();

    //Need to call it to initialize m_team (m_team can be calculated from race)
    //Other way is to saves m_team into characters table.
    setFactionForRace(getRace());

    // load home bind and check in same time class/race pair, it used later for restore broken positions
    // if (!_LoadHomeBind(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADHOMEBIND)))
        // return false;

    InitPrimaryProfessions();                               // to max set before any spell loaded

    // init saved position, and fix it later if problematic
    Relocate(PlayerData["data"]["position_x"].asFloat(), PlayerData["data"]["position_y"].asFloat(), PlayerData["data"]["position_z"].asFloat(), PlayerData["data"]["orientation"].asFloat());

    uint32 mapId = PlayerData["data"]["map"].asUInt();
    uint32 instanceId = PlayerData["data"]["instance_id"].asUInt();

    uint32 dungeonDiff = PlayerData["data"]["instance_mode_mask"].asUInt() & 0xFFFF;
    if (!IsValidDifficulty(dungeonDiff, false) || !dungeonDiff)
        dungeonDiff = REGULAR_DIFFICULTY;
    uint32 raidDiff = (PlayerData["data"]["instance_mode_mask"].asUInt() >> 16) & 0xFFFF;
    if (!IsValidDifficulty(raidDiff, true) || !raidDiff)
        raidDiff = MAN10_DIFFICULTY;
    SetDungeonDifficulty(Difficulty(dungeonDiff));          // may be changed in _LoadGroup
    SetRaidDifficulty(Difficulty(raidDiff));                // may be changed in _LoadGroup

    InitBrackets();

    SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, PlayerData["data"]["totalKills"].asUInt());
    SetUInt16Value(PLAYER_FIELD_KILLS, 0, PlayerData["data"]["todayKills"].asUInt());
    SetUInt16Value(PLAYER_FIELD_KILLS, 1, PlayerData["data"]["yesterdayKills"].asUInt());

    return true;
}

void Player::LoadPlayerGroup()
{
    if (!PlayerData.isMember("group") || PlayerData["group"].empty())
        return;

    if (Group* group = sGroupMgr->GetGroupByDbStoreId(PlayerData["group"].asUInt()))
    {
        uint8 subgroup = group->GetMemberGroup(GetGUID());
        SetGroup(group, subgroup);
        if (getLevel() >= LEVELREQUIREMENT_HEROIC)
        {
            // the group leader may change the instance difficulty while the player is offline
            SetDungeonDifficulty(group->GetDungeonDifficulty());
            SetRaidDifficulty(group->GetRaidDifficulty());
        }
    }
}

void Player::LoadPlayerLootCooldown()
{
    if (!PlayerData.isMember("lootcooldown") || PlayerData["lootcooldown"].empty())
        return;

    for (auto iter = PlayerData["lootcooldown"].begin(); iter != PlayerData["lootcooldown"].end(); ++iter)
    {
        auto LootCooldownJson = *iter;
        for (auto itr = LootCooldownJson.begin(); itr != LootCooldownJson.end(); ++itr)
        {
            auto value = (*itr);
            playerLootCooldown lootCooldown;
            lootCooldown.entry = atoi(itr.memberName());
            lootCooldown.type = value["type"].asUInt();
            lootCooldown.difficultyMask = value["difficultyMask"].asUInt();
            lootCooldown.respawnTime = value["respawnTime"].asUInt();
            lootCooldown.state = false;
            m_playerLootCooldown[lootCooldown.type][lootCooldown.entry] = lootCooldown;
        }
    }
}

void Player::LoadPlayerCurrency()
{
    if (!PlayerData.isMember("currency") || PlayerData["currency"].empty())
        return;

    for (auto itr = PlayerData["currency"].begin(); itr != PlayerData["currency"].end(); ++itr)
    {
        uint16 currencyID = atoi(itr.memberName());

        CurrencyTypesEntry const* currency = sCurrencyTypesStore.LookupEntry(currencyID);
        if (!currency)
            continue;

        auto value = (*itr);
        PlayerCurrency cur;
        cur.state       = PLAYERCURRENCY_UNCHANGED;
        cur.weekCount   = value["weekCount"].asInt();
        cur.totalCount = value["totalCount"].asInt();
        cur.seasonTotal = value["seasonTotal"].asInt();
        uint8 flags = value["flags"].asInt();
        cur.curentCap = value["curentCap"].asInt();

        cur.flags = flags & PLAYERCURRENCY_MASK_USED_BY_CLIENT;
        cur.currencyEntry = currency;

        _currencyStorage.insert(PlayerCurrenciesMap::value_type(currencyID, cur));
    }
}

void Player::LoadPlayerBoundInstances()
{
    if (!PlayerData.isMember("boundinstances") || PlayerData["boundinstances"].empty())
        return;

    Group* group = GetGroup();

    for (auto itr = PlayerData["boundinstances"].begin(); itr != PlayerData["boundinstances"].end(); ++itr)
    {
        uint32 instanceId = atoi(itr.memberName());

        auto value = (*itr);
        bool perm   = value["perm"].asBool();
        uint32 mapId = value["map"].asInt();
        uint8 difficulty = value["difficulty"].asInt();
        uint32 challenge = value["challenge"].asInt();
        std::string data = value["data"].asString();
        uint32 completedEncounter = value["completedEncounters"].asInt();
        time_t saveTime = time_t(value["saveTime"].asUInt());

        bool deleteInstance = false;

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsDungeon())
        {
            sLog->outError(LOG_FILTER_PLAYER, "_LoadBoundInstances: player %s(%d) has bind to not existed or not dungeon map %d", GetName(), GetGUIDLow(), mapId);
            deleteInstance = true;
        }
        else if (difficulty >= MAX_DIFFICULTY)
        {
            sLog->outError(LOG_FILTER_PLAYER, "_LoadBoundInstances: player %s(%d) has bind to not existed difficulty %d instance for map %u", GetName(), GetGUIDLow(), difficulty, mapId);
            deleteInstance = true;
        }
        else
        {
            MapDifficulty const* mapDiff = GetMapDifficultyData(mapId, Difficulty(difficulty));
            if (!mapDiff)
            {
                sLog->outError(LOG_FILTER_PLAYER, "_LoadBoundInstances: player %s(%d) has bind to not existed difficulty %d instance for map %u", GetName(), GetGUIDLow(), difficulty, mapId);
                deleteInstance = true;
            }
            else if (!perm && group)
            {
                sLog->outError(LOG_FILTER_PLAYER, "_LoadBoundInstances: player %s(%d) is in group %d but has a non-permanent character bind to map %d, %d, %d", GetName(), GetGUIDLow(), GUID_LOPART(group->GetGUID()), mapId, instanceId, difficulty);
                deleteInstance = true;
            }
            else if (sWorld->getOldInstanceResetTime(mapDiff->resetTime) > saveTime)
                deleteInstance = true;
        }

        if (deleteInstance)
            continue;

        // since non permanent binds are always solo bind, they can always be reset
        if (InstanceSave* save = sInstanceSaveMgr->AddInstanceSave(mapId, instanceId, Difficulty(difficulty), completedEncounter, challenge, data, !perm, true))
        {
            BindToInstance(save, perm, true);
            save->SetSaveTime(time(NULL));
            save->SetPerm(perm);
        }
    }
}

void Player::LoadPlayerBG()
{
    if (!PlayerData.isMember("bg") || PlayerData["bg"].empty())
        return;

    m_bgData.bgInstanceID = PlayerData["bg"]["instanceId"].asInt();
    m_bgData.bgTeam = PlayerData["bg"]["team"].asInt();
    m_bgData.joinPos = WorldLocation(PlayerData["bg"]["joinMapId"].asInt(),    // Map
                                          PlayerData["bg"]["joinX"].asFloat(),     // X
                                          PlayerData["bg"]["joinY"].asFloat(),     // Y
                                          PlayerData["bg"]["joinZ"].asFloat(),     // Z
                                          PlayerData["bg"]["joinO"].asFloat());    // Orientation
    m_bgData.taxiPath[0] = PlayerData["bg"]["taxiStart"].asInt();
    m_bgData.taxiPath[1] = PlayerData["bg"]["taxiEnd"].asInt();
    m_bgData.mountSpell = PlayerData["bg"]["mountSpell"].asInt();
}

void Player::InitSecondPartDataPlayer()
{
    uint32 mapId = PlayerData["data"]["map"].asUInt();
    uint32 instanceId = PlayerData["data"]["instance_id"].asUInt();
    std::string taxi_nodes = PlayerData["data"]["taxi_path"].asString();

#define RelocateToHomebind(){ mapId = m_homebindMapId; instanceId = 0; Relocate(m_homebindX, m_homebindY, m_homebindZ); }

    GetSession()->SetPlayer(this);
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);

    m_atLoginFlags = PlayerData["data"]["at_login"].asUInt();

    if(m_atLoginFlags & AT_LOGIN_UNLOCK)
    {
        bool BGdesert = false;
        bool DungeonDesert = false;
        bool MalDeRez = false;

        RemoveAtLoginFlag(AT_LOGIN_UNLOCK, true);
        if (HasAura(26013)) // deserteur
            BGdesert = true;
        if (HasAura(71041)) // deserteur de donjon
            DungeonDesert = true;
        if (HasAura(15007))
            MalDeRez = true;


        RemoveAllAuras();
        RemoveFromGroup();

        if (BGdesert)
            AddAura(26013, this);
        if (DungeonDesert)
            AddAura(71041, this);
        if (MalDeRez)
            AddAura(15007, this);

        m_mustResurrectFromUnlock = true;
        RelocateToHomebind();
    }
    else if (!mapEntry || !IsPositionValid())
    {
        RemoveAtLoginFlag(AT_LOGIN_UNLOCK, true);
        sLog->outError(LOG_FILTER_PLAYER, "Player (guidlow %d) have invalid coordinates (MapId: %u X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.", GetGUID(), mapId, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        RelocateToHomebind();
    }
    // Player was saved in Arena or Bg
    else if (mapEntry && mapEntry->IsBattlegroundOrArena())
    {
        Battleground* currentBg = NULL;
        if (m_bgData.bgInstanceID)                                                //saved in Battleground
            currentBg = sBattlegroundMgr->GetBattleground(m_bgData.bgInstanceID, BATTLEGROUND_TYPE_NONE);

        bool player_at_bg = currentBg && currentBg->IsPlayerInBattleground(GetGUID());

        if (player_at_bg && currentBg->GetStatus() != STATUS_WAIT_LEAVE)
        {
            BattlegroundQueueTypeId bgQueueTypeId = sBattlegroundMgr->BGQueueTypeId(currentBg->GetTypeID(), currentBg->GetJoinType());
            AddBattlegroundQueueId(bgQueueTypeId);

            m_bgData.bgTypeID = currentBg->GetTypeID();

            //join player to battleground group
            currentBg->EventPlayerLoggedIn(this);
            currentBg->AddOrSetPlayerToCorrectBgGroup(this, m_bgData.bgTeam);

            SetInviteForBattlegroundQueueType(bgQueueTypeId, currentBg->GetInstanceID());
        }
        // Bg was not found - go to Entry Point
        else
        {
            // leave bg
            if (player_at_bg)
                currentBg->RemovePlayerAtLeave(GetGUID(), false, true);

            // Do not look for instance if bg not found
            const WorldLocation& _loc = GetBattlegroundEntryPoint();
            mapId = _loc.GetMapId(); instanceId = 0;

            // Db field type is type int16, so it can never be MAPID_INVALID
            //if (mapId == MAPID_INVALID) -- code kept for reference
            if (int16(mapId) == int16(-1)) // Battleground Entry Point not found (???)
            {
                sLog->outError(LOG_FILTER_PLAYER, "Player (guidlow %d) was in BG in database, but BG was not found, and entry point was invalid! Teleport to default race/class locations.", GetGUID());
                RelocateToHomebind();
            }
            else
                Relocate(&_loc);

            // We are not in BG anymore
            m_bgData.bgInstanceID = 0;
        }
    }
    // currently we do not support taxi in instance
    else if (!taxi_nodes.empty())
    {
        instanceId = 0;

        // Not finish taxi flight path
        if (m_bgData.HasTaxiPath())
        {
            for (int i = 0; i < 2; ++i)
                m_taxi.AddTaxiDestination(m_bgData.taxiPath[i]);
        }
        else if (!m_taxi.LoadTaxiDestinationsFromString(taxi_nodes, GetTeam()))
        {
            // problems with taxi path loading
            TaxiNodesEntry const* nodeEntry = NULL;
            if (uint32 node_id = m_taxi.GetTaxiSource())
                nodeEntry = sTaxiNodesStore.LookupEntry(node_id);

            if (!nodeEntry)                                      // don't know taxi start node, to homebind
            {
                sLog->outError(LOG_FILTER_PLAYER, "Character %u have wrong data in taxi destination list, teleport to homebind.", GetGUIDLow());
                RelocateToHomebind();
            }
            else                                                // have start node, to it
            {
                sLog->outError(LOG_FILTER_PLAYER, "Character %u have too short taxi destination list, teleport to original node.", GetGUIDLow());
                mapId = nodeEntry->map_id;
                Relocate(nodeEntry->x, nodeEntry->y, nodeEntry->z, 0.0f);
            }
            m_taxi.ClearTaxiDestinations();
        }

        if (uint32 node_id = m_taxi.GetTaxiSource())
        {
            // save source node as recall coord to prevent recall and fall from sky
            TaxiNodesEntry const* nodeEntry = sTaxiNodesStore.LookupEntry(node_id);
            if (nodeEntry && nodeEntry->map_id == GetMapId())
            {
                ASSERT(nodeEntry);                                  // checked in m_taxi.LoadTaxiDestinationsFromString
                mapId = nodeEntry->map_id;
                Relocate(nodeEntry->x, nodeEntry->y, nodeEntry->z, 0.0f);
            }

            // flight will started later
        }
    }

    // Map could be changed before
    mapEntry = sMapStore.LookupEntry(mapId);
    // client without expansion support
    if (mapEntry)
    {
        if (GetSession()->Expansion() < mapEntry->Expansion())
        {
            sLog->outDebug(LOG_FILTER_REDIS, "Player %s using client without required expansion tried login at non accessible map %u", GetName(), mapId);
            RelocateToHomebind();
        }

        // fix crash (because of if (Map* map = _FindMap(instanceId)) in MapInstanced::CreateInstance)
        if (instanceId)
            if (InstanceSave* save = GetInstanceSave(mapId, mapEntry->IsRaid()))
                if (save->GetInstanceId() != instanceId)
                    instanceId = 0;
    }

    // NOW player must have valid map
    // load the player's map here if it's not already loaded
    Map* map = sMapMgr->CreateMap(mapId, this);

    if (!map)
    {
        instanceId = 0;
        AreaTriggerStruct const* at = sObjectMgr->GetGoBackTrigger(mapId);
        if (at)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player (guidlow %d) is teleported to gobacktrigger (Map: %u X: %f Y: %f Z: %f O: %f).", GetGUID(), mapId, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
            Relocate(at->target_X, at->target_Y, at->target_Z, GetOrientation());
            mapId = at->target_mapId;
        }
        else
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player (guidlow %d) is teleported to home (Map: %u X: %f Y: %f Z: %f O: %f).", GetGUID(), mapId, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
            RelocateToHomebind();
        }

        map = sMapMgr->CreateMap(mapId, this);
        if (!map)
        {
            PlayerInfo const* info = sObjectMgr->GetPlayerInfo(getRace(), getClass());
            mapId = info->mapId;
            Relocate(info->positionX, info->positionY, info->positionZ, 0.0f);
            sLog->outError(LOG_FILTER_PLAYER, "Player (guidlow %d) have invalid coordinates (X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.", GetGUID(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
            map = sMapMgr->CreateMap(mapId, this);
            if (!map)
            {
                sLog->outError(LOG_FILTER_PLAYER, "Player (guidlow %d) has invalid default map coordinates (X: %f Y: %f Z: %f O: %f). or instance couldn't be created", GetGUID(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
                return;
            }
        }
    }

    // if the player is in an instance and it has been reset in the meantime teleport him to the entrance
    if (instanceId && !sInstanceSaveMgr->GetInstanceSave(instanceId) && !map->IsBattlegroundOrArena())
    {
        AreaTriggerStruct const* at = sObjectMgr->GetMapEntranceTrigger(mapId);
        if (at)
            Relocate(at->target_X, at->target_Y, at->target_Z, at->target_Orientation);
        else
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player %s(GUID: %u) logged in to a reset instance (map: %u) and there is no area-trigger leading to this map. Thus he can't be ported back to the entrance. This _might_ be an exploit attempt.", GetName(), GetGUIDLow(), mapId);
            RelocateToHomebind();
        }
    }

    SetMap(map);
    StoreRaidMapDifficulty();

    // randomize first save time in range [CONFIG_INTERVAL_SAVE] around [CONFIG_INTERVAL_SAVE]
    // this must help in case next save after mass player load after server startup
    m_nextSave = sWorld->getIntConfig(CONFIG_INTERVAL_SAVE) / 10;

    SaveRecallPosition();

    time_t now = time(NULL);
    time_t logoutTime = time_t(PlayerData["data"]["logout_time"].asUInt());

    // since last logout (in seconds)
    uint32 time_diff = uint32(now - logoutTime); //uint64 is excessive for a time_diff in seconds.. uint32 allows for 136~ year difference.

    // set value, including drunk invisibility detection
    // calculate sobering. after 15 minutes logged out, the player will be sober again
    uint8 newDrunkValue = 0;
    if (time_diff < uint32(GetDrunkValue()) * 9)
        newDrunkValue = GetDrunkValue() - time_diff / 9;

    SetDrunkValue(newDrunkValue);

    m_cinematic = PlayerData["data"]["cinematic"].asUInt();
    m_Played_time[PLAYED_TIME_TOTAL]= PlayerData["data"]["totaltime"].asUInt();
    m_Played_time[PLAYED_TIME_LEVEL]= PlayerData["data"]["leveltime"].asUInt();

    SetTalentResetCost(PlayerData["data"]["resettalents_cost"].asUInt());
    SetTalentResetTime(PlayerData["data"]["resettalents_time"].asUInt());

    SetSpecializationResetCost(PlayerData["data"]["resetspecialization_cost"].asUInt());
    SetSpecializationResetTime(time_t(PlayerData["data"]["resetspecialization_time"].asUInt()));

    m_taxi.LoadTaxiMask(PlayerData["data"]["taximask"].asString());            // must be before InitTaxiNodesForLevel

    uint32 extraflags = PlayerData["data"]["extra_flags"].asUInt();

    m_stableSlots = PlayerData["data"]["stable_slots"].asUInt();
    if (m_stableSlots > MAX_PET_STABLES)
    {
        sLog->outError(LOG_FILTER_PLAYER, "Player can have not more %u stable slots, but have in DB %u", MAX_PET_STABLES, uint32(m_stableSlots));
        m_stableSlots = MAX_PET_STABLES;
    }

    // Honor system
    // Update Honor kills data
    m_lastHonorUpdateTime = logoutTime;
    UpdateHonorFields();

    m_deathExpireTime = time_t(PlayerData["data"]["death_expire_time"].asUInt());
    if (m_deathExpireTime > now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP)
        m_deathExpireTime = now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP-1;

    // clear channel spell data (if saved at channel spell casting)
    SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, 0);
    SetUInt32Value(UNIT_CHANNEL_SPELL, 0);

    // clear charm/summon related fields
    SetOwnerGUID(0);
    SetUInt64Value(UNIT_FIELD_CHARMEDBY, 0);
    SetUInt64Value(UNIT_FIELD_CHARM, 0);
    SetUInt64Value(UNIT_FIELD_SUMMON, 0);
    SetUInt64Value(PLAYER_FARSIGHT, 0);
    SetCreatorGUID(0);

    RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVEMENT);

    // reset some aura modifiers before aura apply
    SetUInt32Value(PLAYER_TRACK_CREATURES, 0);
    SetUInt32Value(PLAYER_TRACK_RESOURCES, 0);

    // make sure the unit is considered out of combat for proper loading
    ClearInCombat();

    // make sure the unit is considered not in duel for proper loading
    SetUInt64Value(PLAYER_DUEL_ARBITER, 0);
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    // reset stats before loading any modifiers
    InitStatsForLevel();
    InitGlyphsForLevel();
    InitTaxiNodesForLevel();
    InitRunes();

    // rest bonus can only be calculated after InitStatsForLevel()
    m_rest_bonus = PlayerData["data"]["rest_bonus"].asUInt();

    if (time_diff > 0)
    {
        //speed collect rest bonus in offline, in logout, far from tavern, city (section/in hour)
        float bubble0 = 0.031f;
        //speed collect rest bonus in offline, in logout, in tavern, city (section/in hour)
        float bubble1 = 0.125f;
        float bubble = PlayerData["data"]["is_logout_resting"].asUInt() > 0
            ? bubble1*sWorld->getRate(RATE_REST_OFFLINE_IN_TAVERN_OR_CITY)
            : bubble0*sWorld->getRate(RATE_REST_OFFLINE_IN_WILDERNESS);
        bubble *= GetTotalAuraMultiplier(SPELL_AURA_MOD_REST_GAINED);

        SetRestBonus(GetRestBonus()+ time_diff*((float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP)/72000)*bubble);
    }
}

void Player::LoadPlayerBattlePets()
{
    if (!AccountDatas.isMember("battlepets") || AccountDatas["battlepets"].empty())
        return;

    for (auto itr = AccountDatas["battlepets"].begin(); itr != AccountDatas["battlepets"].end(); ++itr)
    {
        uint64 petGuid = atoi(itr.memberName());
        auto value = (*itr);
        std::string customName = value["customName"].asString();
        uint32 creatureEntry = value["creatureEntry"].asInt();
        uint32 speciesID = value["speciesID"].asInt();
        uint32 spell = value["spell"].asInt();
        uint8 level = value["level"].asInt();
        uint32 displayID = value["displayID"].asInt();
        uint16 power = value["power"].asInt();
        uint16 speed = value["speed"].asInt();
        uint32 health = value["health"].asInt();
        uint32 maxHealth = value["maxHealth"].asInt();
        uint8 quality = value["quality"].asInt();
        uint16 xp = value["xp"].asInt();
        uint16 flags = value["flags"].asInt();
        int16 breedID = value["breedID"].asInt();

        // recalculate stats after change breed
        if (!breedID)
        {
            breedID = GetBattlePetMgr()->GetRandomBreedID(speciesID);

            if (!breedID)
                continue;

            float pct = float(health * 100.0f) / maxHealth;

            BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(speciesID, breedID);
            accumulator->CalcQualityMultiplier(quality, level);
            maxHealth = accumulator->CalculateHealth();
            power = accumulator->CalculatePower();
            speed = accumulator->CalculateSpeed();
            delete accumulator;

            if (pct != 100.0f)
                health = int32(maxHealth * float(pct / 100.0f));
            else
                health = maxHealth;
        }

        // prevent some undefined behavoir
        if (health > maxHealth)
            health = maxHealth;

        GetBattlePetMgr()->AddPetToList(petGuid, speciesID, creatureEntry, level, displayID, power, speed, health, maxHealth, quality, xp, flags, spell, customName, breedID, STATE_NORMAL);
        UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL, creatureEntry);
    }
}

void Player::LoadPlayerBattlePetSlots()
{
    if (!AccountDatas.isMember("battlepetslots") || AccountDatas["battlepetslots"].empty())
    {
        for (int i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
            GetBattlePetMgr()->InitBattleSlot(0, i);
        return;
    }

    for (auto itr = AccountDatas["battlepetslots"].begin(); itr != AccountDatas["battlepetslots"].end(); ++itr)
    {
        uint8 index = atoi(itr.memberName());
        GetBattlePetMgr()->InitBattleSlot(itr->asInt(), index);
    }
}

void Player::LoadPlayerSkills()
{
    if (!PlayerData.isMember("skills") || PlayerData["skills"].empty())
        return;

    uint32 count = 0;
    uint8 professionCount = 0;
    for (auto itr = PlayerData["skills"].begin(); itr != PlayerData["skills"].end(); ++itr)
    {
        uint16 skill = atoi(itr.memberName());
        auto valueSkill = (*itr);
        uint16 value = valueSkill["value"].asInt();
        uint16 max = valueSkill["max"].asInt();

        SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skill);
        if (!pSkill)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Character %u has skill %u that does not exist.", GetGUIDLow(), skill);
            continue;
        }

        // set fixed skill ranges
        switch (GetSkillRangeType(pSkill, false))
        {
            case SKILL_RANGE_LANGUAGE:                      // 300..300
                value = max = 300;
                break;
            case SKILL_RANGE_MONO:                          // 1..1, grey monolite bar
                value = max = 1;
                break;
            default:
                break;
        }
        if (value == 0)
            continue;

        uint16 field = count / 2;
        uint8 offset = count & 1;

        SetUInt16Value(PLAYER_SKILL_LINEID_0 + field, offset, skill);
        uint16 step = 0;

        if (pSkill->categoryId == SKILL_CATEGORY_SECONDARY)
            step = max / 75;

        if (pSkill->categoryId == SKILL_CATEGORY_PROFESSION)
        {
            step = max / 75;

            if (professionCount < 2)
                SetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + professionCount++, skill);
        }

        SetUInt16Value(PLAYER_SKILL_STEP_0 + field, offset, step);
        SetUInt16Value(PLAYER_SKILL_RANK_0 + field, offset, value);
        SetUInt16Value(PLAYER_SKILL_MAX_RANK_0 + field, offset, max);
        SetUInt16Value(PLAYER_SKILL_MODIFIER_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_TALENT_0 + field, offset, 0);

        mSkillStatus.insert(SkillStatusMap::value_type(skill, SkillStatusData(count, SKILL_UNCHANGED)));

        learnSkillRewardedSpells(skill, value);

        ++count;

        if (count >= PLAYER_MAX_SKILLS)                      // client limit
        {
            sLog->outError(LOG_FILTER_PLAYER, "Character %u has more than %u skills.", GetGUIDLow(), PLAYER_MAX_SKILLS);
            break;
        }
    }

    for (; count < PLAYER_MAX_SKILLS; ++count)
    {
        uint16 field = count / 2;
        uint8 offset = count & 1;

        SetUInt16Value(PLAYER_SKILL_LINEID_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_STEP_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_RANK_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_MAX_RANK_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_MODIFIER_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_TALENT_0 + field, offset, 0);
    }

    // special settings
    if (getClass() == CLASS_DEATH_KNIGHT)
    {
        uint8 base_level = std::min(getLevel(), uint8(sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL)));
        if (base_level < 1)
            base_level = 1;
        uint16 base_skill = (base_level-1)*5;               // 270 at starting level 55
        if (base_skill < 1)
            base_skill = 1;                                 // skill mast be known and then > 0 in any case

        if (GetPureSkillValue(SKILL_FIRST_AID) < base_skill)
            SetSkill(SKILL_FIRST_AID, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_AXES) < base_skill)
            SetSkill(SKILL_AXES, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_DEFENSE) < base_skill)
            SetSkill(SKILL_DEFENSE, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_POLEARMS) < base_skill)
            SetSkill(SKILL_POLEARMS, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_SWORDS) < base_skill)
            SetSkill(SKILL_SWORDS, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_2H_AXES) < base_skill)
            SetSkill(SKILL_2H_AXES, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_2H_SWORDS) < base_skill)
            SetSkill(SKILL_2H_SWORDS, 0, base_skill, base_skill);
        if (GetPureSkillValue(SKILL_UNARMED) < base_skill)
            SetSkill(SKILL_UNARMED, 0, base_skill, base_skill);
    }
}

void Player::LoadPlayerArchaeology()
{
    if (!PlayerData.isMember("archaeology") || PlayerData["archaeology"].empty())
    {
        GenerateResearchSites();
        GenerateResearchProjects();
        return;
    }

    // Loading current zones
    Tokenizer tokens(PlayerData["archaeology"]["sites"].asString(), ' ');
    if (tokens.size() == MAX_RESEARCH_SITES)
    {
        _researchSites.clear();

        for (uint8 i = 0; i < tokens.size(); ++i)
            _researchSites.insert(uint32(atoi(tokens[i])));
    }
    else
        GenerateResearchSites();

    // Loading current zone info
    Tokenizer tokens2(PlayerData["archaeology"]["counts"].asString(), ' ');
    if (tokens2.size() == MAX_RESEARCH_SITES)
    {
        for (uint8 i = 0; i < MAX_RESEARCH_SITES; ++i)
            _digSites[i].count = uint32(atoi(tokens2[i]));
    }

    // Loading current projects
    Tokenizer tokens3(PlayerData["archaeology"]["projects"].asString(), ' ');
    if (tokens3.size() == MAX_RESEARCH_PROJECTS)
    {
        for (uint8 i = 0; i < MAX_RESEARCH_PROJECTS; ++i)
            if (ResearchProjectEntry const* entry = sResearchProjectStore.LookupEntry(atoi(tokens3[i])))
                if (entry->IsVaid())
                    ReplaceResearchProject(0, entry->ID);
    }
    else
        GenerateResearchProjects();

    for (auto itr = PlayerData["archaeology"]["finds"].begin(); itr != PlayerData["archaeology"]["finds"].end(); ++itr)
    {
        uint32 id = atoi(itr.memberName());
        auto value = (*itr);
        uint32 count = value["count"].asInt();
        uint32 date = value["date"].asInt();

        ResearchProjectEntry const* rs = sResearchProjectStore.LookupEntry(id);
        if (!rs)
            continue;

        CompletedProject cp;
        cp.entry = rs;
        cp.count = count;
        cp.date = date;

        _completedProjects.push_back(cp);
    }
}

void Player::LoadPlayerTalents()
{
    if (!PlayerData.isMember("talents") || PlayerData["talents"].empty())
        return;

    for (auto iter = PlayerData["talents"].begin(); iter != PlayerData["talents"].end(); ++iter)
    {
        auto TalentsJson = *iter;
        uint8 spec = atoi(iter.memberName());
        for (auto itr = TalentsJson.begin(); itr != TalentsJson.end(); ++itr)
        {
            uint32 spellId = atoi(itr.memberName());
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
            if (!spell)
                continue;

            TalentEntry const* talent = sTalentStore.LookupEntry(spell->talentId);
            if (!talent)
                continue;

            AddTalent(talent, spec, false);
            //sLog->outDebug(LOG_FILTER_REDIS, "Player::LoadPlayerTalents spec %u spellId %u talentId %u", spec, spellId, spell->talentId);
        }
    }
}

void Player::LoadPlayerSpells()
{
    if (!PlayerData.isMember("spells") || PlayerData["spells"].empty())
        return;

    for (auto itr = PlayerData["spells"].begin(); itr != PlayerData["spells"].end(); ++itr)
    {
        uint32 spellId = atoi(itr.memberName());
        auto value = (*itr);
        bool active = value["active"].asInt();
        bool disabled = value["disabled"].asInt();

        addSpell(spellId, active, false, false, disabled, true, true);
    }
}

void Player::LoadPlayerMounts()
{
    if (!AccountDatas.isMember("mounts") || AccountDatas["mounts"].empty())
        return;

    for (auto itr = AccountDatas["mounts"].begin(); itr != AccountDatas["mounts"].end(); ++itr)
    {
        uint32 spellId = atoi(itr.memberName());
        bool active = (*itr)["active"].asInt();

        addSpell(spellId, active, false, false, false, true);
    }
}

void Player::LoadPlayerGlyphs()
{
    if (!PlayerData.isMember("glyphs") || PlayerData["glyphs"].empty())
        return;

    for (auto iter = PlayerData["glyphs"].begin(); iter != PlayerData["glyphs"].end(); ++iter)
    {
        uint8 spec = atoi(iter.memberName());
        auto glyphs = *iter;
        for (auto itr = glyphs.begin(); itr != glyphs.end(); ++itr)
        {
            uint8 slot = atoi(itr.memberName());
            auto glyph = *itr;
            _talentMgr->SpecInfo[spec].Glyphs[slot] = glyph.asInt();
        }
    }
}

void Player::LoadPlayerAuras()
{
    if (!PlayerData.isMember("auras") || PlayerData["auras"].empty())
        return;

    time_t now = time(NULL);
    time_t logoutTime = time_t(PlayerData["data"]["logout_time"].asUInt());

    // since last logout (in seconds)
    uint32 time_diff = uint32(now - logoutTime);

    std::list<auraEffectData> auraEffectList;
    for (auto itr = PlayerData["auras"]["effect"].begin(); itr != PlayerData["auras"]["effect"].end(); ++itr)
    {
        auto auraEffect = (*itr);
        uint8 slot = atoi(itr.memberName());
        uint8 effect = auraEffect["effect"].asInt();
        uint32 baseamount = auraEffect["baseamount"].asInt();
        uint32 amount = auraEffect["amount"].asInt();

        auraEffectList.push_back(auraEffectData(slot, effect, amount, baseamount));
    }

    for (auto itr = PlayerData["auras"]["aura"].begin(); itr != PlayerData["auras"]["aura"].end(); ++itr)
    {
        auto auraValue = (*itr);
        uint8 slot = atoi(itr.memberName());

        int32 damage[32];
        int32 baseDamage[32];
        uint64 caster_guid = auraValue["caster_guid"].asUInt64();
        uint32 spellid = auraValue["spell"].asInt();
        uint32 effmask = auraValue["effect_mask"].asInt();
        uint32 recalculatemask = auraValue["recalculate_mask"].asInt();
        uint8 stackcount = auraValue["stackcount"].asInt();
        int32 maxduration = auraValue["maxduration"].asInt();
        int32 remaintime = auraValue["remaintime"].asInt();
        uint8 remaincharges = auraValue["remaincharges"].asInt();

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
        if (!spellInfo)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Unknown aura (spellid %u), ignore.", spellid);
            continue;
        }

        // negative effects should continue counting down after logout
        if (remaintime != -1 && !spellInfo->IsPositive())
        {
            if (remaintime / IN_MILLISECONDS <= int32(time_diff))
                continue;

            remaintime -= time_diff*IN_MILLISECONDS;
        }

        // prevent wrong values of remaincharges
        if (spellInfo->ProcCharges)
        {
            // we have no control over the order of applying auras and modifiers allow auras
            // to have more charges than value in SpellInfo
            if (remaincharges <= 0/* || remaincharges > spellproto->procCharges*/)
                remaincharges = spellInfo->ProcCharges;
        }
        else
            remaincharges = 0;

        for(std::list<auraEffectData>::iterator itr = auraEffectList.begin(); itr != auraEffectList.end(); ++itr)
        {
            if(itr->_slot == slot)
            {
                damage[itr->_effect] = itr->_amount;
                baseDamage[itr->_effect] = itr->_baseamount;
            }
        }

        Aura* aura = Aura::TryCreate(spellInfo, effmask, this, NULL, &baseDamage[0], NULL, caster_guid);
        if (aura != NULL)
        {
            if (!aura->CanBeSaved())
            {
                aura->Remove();
                continue;
            }

            if (InArena())
                if (aura->GetId() == 125761)
                {
                    aura->Remove();
                    continue;
                }

            aura->SetLoadedState(maxduration, remaintime, remaincharges, stackcount, recalculatemask, &damage[0]);
            aura->ApplyForTargets();
            sLog->outInfo(LOG_FILTER_PLAYER, "Added aura spellid %u, effectmask %u", spellInfo->Id, effmask);
        }
    }
}

void Player::LoadPlayerQuestStatus()
{
    if (!PlayerData.isMember("queststatus") || PlayerData["queststatus"].empty())
        return;

    uint16 slot = FindQuestSlot(0);
    for (auto itr = PlayerData["queststatus"].begin(); itr != PlayerData["queststatus"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());
        auto value = (*itr);

        if (m_QuestStatus.find(quest_id) != m_QuestStatus.end())
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
        {
            // find or create
            QuestStatusData& questStatusData = m_QuestStatus[quest_id];

            uint8 qstatus = value["status"].asInt();
            if (qstatus < MAX_QUEST_STATUS)
                questStatusData.Status = QuestStatus(qstatus);
            else
            {
                questStatusData.Status = QUEST_STATUS_INCOMPLETE;
                sLog->outError(LOG_FILTER_PLAYER, "Player %s (GUID: %u) has invalid quest %d status (%u), replaced by QUEST_STATUS_INCOMPLETE(3).",
                    GetName(), GetGUIDLow(), quest_id, qstatus);
            }

            questStatusData.Explored = (value["explored"].asInt() > 0);

            time_t quest_time = time_t(value["Timer"].asUInt());

            if (quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED) && !GetQuestRewardStatus(quest_id))
            {
                AddTimedQuest(quest_id);

                if (quest_time <= sWorld->GetGameTime())
                    questStatusData.Timer = 1;
                else
                    questStatusData.Timer = uint32((quest_time - sWorld->GetGameTime()) * IN_MILLISECONDS);
            }
            else
                quest_time = 0;

            for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            {
                std::string index = std::to_string(i);
                questStatusData.CreatureOrGOCount[i] = value["mobcount"][index.c_str()].asInt();
                questStatusData.ItemCount[i] = value["itemcount"][index.c_str()].asInt();
            }

            questStatusData.PlayerCount = value["playercount"].asInt();

            // add to quest log
            if (slot < MAX_QUEST_LOG_SIZE && questStatusData.Status != QUEST_STATUS_NONE)
            {
                SetQuestSlot(slot, quest_id, uint32(quest_time)); // cast can't be helped

                if (questStatusData.Status == QUEST_STATUS_COMPLETE)
                    SetQuestSlotState(slot, QUEST_STATE_COMPLETE);
                else if (questStatusData.Status == QUEST_STATUS_FAILED)
                    SetQuestSlotState(slot, QUEST_STATE_FAIL);

                for (uint8 idx = 0; idx < QUEST_OBJECTIVES_COUNT; ++idx)
                    if (questStatusData.CreatureOrGOCount[idx])
                        SetQuestSlotCounter(slot, idx, questStatusData.CreatureOrGOCount[idx]);

                if (questStatusData.PlayerCount)
                    SetQuestSlotCounter(slot, QUEST_PVP_KILL_SLOT, questStatusData.PlayerCount);

                ++slot;
            }

            sLog->outDebug(LOG_FILTER_REDIS, "Quest status is {%u} for quest {%u} for player (GUID: %u) slot %u", questStatusData.Status, quest_id, GetGUIDLow(), slot);
        }
    }

    // clear quest log tail
    for (uint16 i = slot; i < MAX_QUEST_LOG_SIZE; ++i)
        SetQuestSlot(i, 0);
}

void Player::LoadAccountQuestStatus()
{
    if (!AccountDatas.isMember("queststatus") || AccountDatas["queststatus"].empty())
        return;

    uint16 slot = FindQuestSlot(0);
    for (auto itr = AccountDatas["queststatus"].begin(); itr != AccountDatas["queststatus"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());
        auto value = (*itr);

        if (m_QuestStatus.find(quest_id) != m_QuestStatus.end())
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
        {
            // find or create
            QuestStatusData& questStatusData = m_QuestStatus[quest_id];

            uint8 qstatus = value["status"].asInt();
            if (qstatus < MAX_QUEST_STATUS)
                questStatusData.Status = QuestStatus(qstatus);
            else
            {
                questStatusData.Status = QUEST_STATUS_INCOMPLETE;
                sLog->outError(LOG_FILTER_PLAYER, "Player %s (GUID: %u) has invalid quest %d status (%u), replaced by QUEST_STATUS_INCOMPLETE(3).",
                    GetName(), GetGUIDLow(), quest_id, qstatus);
            }

            questStatusData.Explored = (value["explored"].asInt() > 0);

            time_t quest_time = time_t(value["Timer"].asInt());

            if (quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED) && !GetQuestRewardStatus(quest_id))
            {
                AddTimedQuest(quest_id);

                if (quest_time <= sWorld->GetGameTime())
                    questStatusData.Timer = 1;
                else
                    questStatusData.Timer = uint32((quest_time - sWorld->GetGameTime()) * IN_MILLISECONDS);
            }
            else
                quest_time = 0;

            for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            {
                std::string index = std::to_string(i);
                questStatusData.CreatureOrGOCount[i] = value["mobcount"][index.c_str()].asInt();
                questStatusData.ItemCount[i] = value["itemcount"][index.c_str()].asInt();
            }

            questStatusData.PlayerCount = value["playercount"].asInt();

            // add to quest log
            if (slot < MAX_QUEST_LOG_SIZE && questStatusData.Status != QUEST_STATUS_NONE)
            {
                SetQuestSlot(slot, quest_id, uint32(quest_time)); // cast can't be helped

                if (questStatusData.Status == QUEST_STATUS_COMPLETE)
                    SetQuestSlotState(slot, QUEST_STATE_COMPLETE);
                else if (questStatusData.Status == QUEST_STATUS_FAILED)
                    SetQuestSlotState(slot, QUEST_STATE_FAIL);

                for (uint8 idx = 0; idx < QUEST_OBJECTIVES_COUNT; ++idx)
                    if (questStatusData.CreatureOrGOCount[idx])
                        SetQuestSlotCounter(slot, idx, questStatusData.CreatureOrGOCount[idx]);

                if (questStatusData.PlayerCount)
                    SetQuestSlotCounter(slot, QUEST_PVP_KILL_SLOT, questStatusData.PlayerCount);

                ++slot;
            }

            sLog->outDebug(LOG_FILTER_REDIS, "Quest status is {%u} for quest {%u} for player (GUID: %u)", questStatusData.Status, quest_id, GetGUIDLow());
        }
    }

    // clear quest log tail
    for (uint16 i = slot; i < MAX_QUEST_LOG_SIZE; ++i)
        SetQuestSlot(i, 0);
}

void Player::LoadPlayerQuestRewarded()
{
    if (!PlayerData.isMember("questrewarded") || PlayerData["questrewarded"].empty())
        return;

    for (auto itr = PlayerData["questrewarded"].begin(); itr != PlayerData["questrewarded"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());

        if (m_RewardedQuests.find(quest_id) != m_RewardedQuests.end())
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
        {
            // learn rewarded spell if unknown
            learnQuestRewardedSpells(quest);

            // set rewarded title if any
            if (quest->GetCharTitleId())
            {
                if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(quest->GetCharTitleId()))
                    SetTitle(titleEntry);
            }

            if (uint32 talents = quest->GetBonusTalents())
                AddQuestRewardedTalentCount(talents);
        }

        m_RewardedQuests.insert(quest_id);
    }
}

void Player::LoadAccountQuestRewarded()
{
    if (!AccountDatas.isMember("questrewarded") || AccountDatas["questrewarded"].empty())
        return;

    for (auto itr = AccountDatas["questrewarded"].begin(); itr != AccountDatas["questrewarded"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());

        if (m_RewardedQuests.find(quest_id) != m_RewardedQuests.end())
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
        {
            // learn rewarded spell if unknown
            learnQuestRewardedSpells(quest);

            // set rewarded title if any
            if (quest->GetCharTitleId())
            {
                if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(quest->GetCharTitleId()))
                    SetTitle(titleEntry);
            }

            if (uint32 talents = quest->GetBonusTalents())
                AddQuestRewardedTalentCount(talents);
        }

        m_RewardedQuests.insert(quest_id);
    }
}

void Player::LoadPlayerQuestDaily()
{
    if (!PlayerData.isMember("questdaily") || PlayerData["questdaily"].empty())
        return;

    m_DFQuests.clear();
    for (auto itr = PlayerData["questdaily"].begin(); itr != PlayerData["questdaily"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());
        m_lastDailyQuestTime = time_t(itr->asUInt());

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (!quest)
            continue;

        if (quest->IsDFQuest())
        {
            m_DFQuests.insert(quest->GetQuestId());
            continue;
        }

        if (m_dailyquests.find(quest_id) == m_dailyquests.end())
            m_dailyquests.insert(quest_id);
    }
}

void Player::LoadAccountQuestDaily()
{
    if (!AccountDatas.isMember("questdaily") || AccountDatas["questdaily"].empty())
        return;

    for (auto itr = AccountDatas["questdaily"].begin(); itr != AccountDatas["questdaily"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());
        m_lastDailyQuestTime = time_t(itr->asUInt());

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (!quest)
            continue;

        if (quest->IsDFQuest())
        {
            m_DFQuests.insert(quest->GetQuestId());
            continue;
        }

        if (m_dailyquests.find(quest_id) == m_dailyquests.end())
            m_dailyquests.insert(quest_id);
    }
}

void Player::LoadPlayerQuestWeekly()
{
    if (!PlayerData.isMember("questweekly") || PlayerData["questweekly"].empty())
        return;

    for (auto itr = PlayerData["questweekly"].begin(); itr != PlayerData["questweekly"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
            continue;

        if (m_weeklyquests.find(quest_id) != m_weeklyquests.end())
            continue;

        m_weeklyquests.insert(quest_id);
    }
}

void Player::LoadAccountQuestWeekly()
{
    if (!AccountDatas.isMember("questweekly") || AccountDatas["questweekly"].empty())
        return;

    m_weeklyquests.clear();

    for (auto itr = AccountDatas["questweekly"].begin(); itr != AccountDatas["questweekly"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
            continue;

        if (m_weeklyquests.find(quest_id) != m_weeklyquests.end())
            continue;

        m_weeklyquests.insert(quest_id);
    }
}

void Player::LoadPlayerQuestSeasonal()
{
    if (!PlayerData.isMember("questseasonal") || PlayerData["questseasonal"].empty())
        return;

    m_seasonalquests.clear();

    for (auto itr = PlayerData["questseasonal"].begin(); itr != PlayerData["questseasonal"].end(); ++itr)
    {
        uint32 event_id = atoi(itr.memberName());
        auto data = (*itr);
        for (auto iter = data.begin(); iter != data.end(); ++iter)
        {
            uint32 quest_id = atoi(iter.memberName());

            Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
            if (quest)
                continue;

            if (m_seasonalquests.find(quest_id) != m_seasonalquests.end())
                continue;

            m_seasonalquests[event_id].insert(quest_id);
        }
    }
}

void Player::LoadAccountQuestSeasonal()
{
    if (!AccountDatas.isMember("questseasonal") || AccountDatas["questseasonal"].empty())
        return;

    m_seasonalquests.clear();

    for (auto itr = AccountDatas["questseasonal"].begin(); itr != AccountDatas["questseasonal"].end(); ++itr)
    {
        uint32 quest_id = atoi(itr.memberName());
        uint32 event_id = time_t(itr->asUInt());

        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (quest)
            continue;

        if (m_seasonalquests.find(quest_id) != m_seasonalquests.end())
            continue;

        m_seasonalquests[event_id].insert(quest_id);
    }
}

void ReputationMgr::LoadFromDB()
{
    // Set initial reputations (so everything is nifty before DB data load)
    Initialize();

    if (!_player->PlayerData.isMember("reputation") || _player->PlayerData["reputation"].empty())
        return;

    for (auto itr = _player->PlayerData["reputation"].begin(); itr != _player->PlayerData["reputation"].end(); ++itr)
    {
        uint32 factiont_id = atoi(itr.memberName());
        auto value = (*itr);

        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factiont_id);
        if (factionEntry && (factionEntry->reputationListID >= 0))
        {
            FactionState* faction = &_factions[factionEntry->reputationListID];

            // update standing to current
            faction->Standing = value["standing"].asInt();

            // update counters
            int32 BaseRep = GetBaseReputation(factionEntry);
            ReputationRank old_rank = ReputationToRank(BaseRep);
            ReputationRank new_rank = ReputationToRank(BaseRep + faction->Standing);
            UpdateRankCounters(old_rank, new_rank);

            uint32 dbFactionFlags = value["flags"].asInt();

            if (dbFactionFlags & FACTION_FLAG_VISIBLE)
                SetVisible(faction);                    // have internal checks for forced invisibility

            if (dbFactionFlags & FACTION_FLAG_INACTIVE)
                SetInactive(faction, true);              // have internal checks for visibility requirement

            if (dbFactionFlags & FACTION_FLAG_AT_WAR)  // DB at war
                SetAtWar(faction, true);                 // have internal checks for FACTION_FLAG_PEACE_FORCED
            else                                        // DB not at war
            {
                // allow remove if visible (and then not FACTION_FLAG_INVISIBLE_FORCED or FACTION_FLAG_HIDDEN)
                if (faction->Flags & FACTION_FLAG_VISIBLE)
                    SetAtWar(faction, false);            // have internal checks for FACTION_FLAG_PEACE_FORCED
            }

            // set atWar for hostile
            if (GetRank(factionEntry) <= REP_HOSTILE)
                SetAtWar(faction, true);

            // enable war on faction Oracles/Frenzyheart Tribe
            if (GetRank(factionEntry) <= REP_HOSTILE && (factionEntry->ID == 1104 || factionEntry->ID == 1105))
                faction->Flags |= FACTION_FLAG_AT_WAR;

            // reset changed flag if values similar to saved in DB
            if (faction->Flags == dbFactionFlags)
                faction->needSend = false;
        }
    }
}

void Player::LoadPlayerLoadItems(std::vector<RedisValue>* itemVector)
{
    time_t now = time(NULL);
    time_t logoutTime = time_t(PlayerData["data"]["logout_time"].asUInt());

    // since last logout (in seconds)
    uint32 time_diff = uint32(now - logoutTime);

    uint32 zoneId = GetZoneId();

    std::map<uint32, Bag*> bagMap;                                  // fast guid lookup for bags
    std::map<uint32, Item*> invalidBagMap;                          // fast guid lookup for bags
    std::list<Item*> problematicItems;
    std::map<uint32, Json::Value> itemInBag;                        // Save item in bag, for next step loading

    for (std::vector<RedisValue>::iterator itr = itemVector->begin(); itr != itemVector->end();)
    {
        uint32 itemId = itr->toInt();
        ++itr;

        Json::Value loadItemJson;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), loadItemJson))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerLoadItems not parse itemId %i", itemId);
            continue;
        }
        else
            ++itr;

        uint32 itemGuid = loadItemJson["itemGuid"].asUInt();

        uint32 bagGuid = loadItemJson["bagGuid"].asUInt();
        if(bagGuid)
        {
            itemInBag[itemGuid] = loadItemJson;
            continue;
        }

        if (Item* item = _LoadItem(zoneId, time_diff, loadItemJson))
        {
            uint8 slot = loadItemJson["slot"].asInt();

            uint8 err = EQUIP_ERR_OK;
            // Item is not in bag
            item->SetContainer(NULL);
            item->SetSlot(slot);

            if (slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END)
            {
                AddItemToBuyBackSlot(item);
                continue;
            }
            // check for already equiped item
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                err = EQUIP_ERR_ITEM_MAX_COUNT;

            if (err == EQUIP_ERR_OK)
            {
                if (IsInventoryPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    ItemPosCountVec dest;
                    err = CanStoreItem(INVENTORY_SLOT_BAG_0, slot, dest, item, false);
                    if (err == EQUIP_ERR_OK)
                        item = StoreItem(dest, item, true);
                }
                else if (IsEquipmentPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    uint16 dest;
                    err = CanEquipItem(slot, dest, item, false, false);
                    if (err == EQUIP_ERR_OK)
                        QuickEquipItem(dest, item);
                }
                else if (IsBankPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    ItemPosCountVec dest;
                    err = CanBankItem(INVENTORY_SLOT_BAG_0, slot, dest, item, false, false);
                    if (err == EQUIP_ERR_OK)
                        item = BankItem(dest, item, true);
                }
            }

            // Remember bags that may contain items in them
            if (err == EQUIP_ERR_OK)
            {
                if (IsBagPos(item->GetPos()))
                    if (Bag* pBag = item->ToBag())
                        bagMap[item->GetGUIDLow()] = pBag;
            }
            else
                if (IsBagPos(item->GetPos()))
                    if (item->IsBag())
                        invalidBagMap[item->GetGUIDLow()] = item;

            // Item's state may have changed after storing
            if (err == EQUIP_ERR_OK)
                item->SetState(ITEM_UNCHANGED, this);
            else
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) which can't be loaded into inventory (Bag GUID: %u, slot: %u) by reason %u. Item will be sent by mail.",
                    GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), bagGuid, slot, err);
                //item->DeleteFromInventoryDB(trans);
                RemoveItemDurations(item);
                RemoveTradeableItem(item);
                problematicItems.push_back(item);
            }
        }

        // Send problematic items by mail
        while (!problematicItems.empty())
        {
            std::string subject = GetSession()->GetTrinityString(LANG_NOT_EQUIPPED_ITEM);

            MailDraft draft(subject, "There were problems with equipping item(s).");
            for (uint8 i = 0; !problematicItems.empty() && i < MAX_MAIL_ITEMS; ++i)
            {
                draft.AddItem(problematicItems.front());
                problematicItems.pop_front();
            }
            draft.SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
        }
    }

    for (auto itr = itemInBag.begin(); itr != itemInBag.end(); ++itr)
    {
        uint32 itemId = itr->first;
        Json::Value loadItemJson = itr->second;

        uint32 itemGuid = loadItemJson["itemGuid"].asUInt();
        uint32 bagGuid = loadItemJson["bagGuid"].asUInt();

        if (Item* item = _LoadItem(zoneId, time_diff, loadItemJson))
        {
            uint8  slot = loadItemJson["slot"].asInt();

            uint8 err = EQUIP_ERR_OK;

            item->SetSlot(NULL_SLOT);
            // Item is in the bag, find the bag
            std::map<uint32, Bag*>::iterator itr = bagMap.find(bagGuid);
            if (itr != bagMap.end())
            {
                ItemPosCountVec dest;
                err = CanStoreItem(itr->second->GetSlot(), slot, dest, item);
                if (err == EQUIP_ERR_OK)
                    item = StoreItem(dest, item, true);
            }
            else if (invalidBagMap.find(bagGuid) != invalidBagMap.end())
            {
                std::map<uint32, Item*>::iterator itr = invalidBagMap.find(bagGuid);
                if (std::find(problematicItems.begin(), problematicItems.end(), itr->second) != problematicItems.end())
                    err = EQUIP_ERR_INTERNAL_BAG_ERROR;
            }
            else
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) which doesnt have a valid bag (Bag GUID: %u, slot: %u). Possible cheat?",
                    GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), bagGuid, slot);
                RemoveItemDurations(item);
                RemoveTradeableItem(item);
                item->DeleteFromRedis();
                delete item;
                continue;
            }

            // Item's state may have changed after storing
            if (err == EQUIP_ERR_OK)
                item->SetState(ITEM_UNCHANGED, this);
            else
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) which can't be loaded into inventory (Bag GUID: %u, slot: %u) by reason %u. Item will be sent by mail.",
                    GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), bagGuid, slot, err);
                //item->DeleteFromInventoryDB(trans);
                RemoveItemDurations(item);
                RemoveTradeableItem(item);
                problematicItems.push_back(item);
            }
        }

        // Send problematic items by mail
        while (!problematicItems.empty())
        {
            std::string subject = GetSession()->GetTrinityString(LANG_NOT_EQUIPPED_ITEM);

            MailDraft draft(subject, "There were problems with equipping item(s).");
            for (uint8 i = 0; !problematicItems.empty() && i < MAX_MAIL_ITEMS; ++i)
            {
                draft.AddItem(problematicItems.front());
                problematicItems.pop_front();
            }
            draft.SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
        }
    }

    //if (isAlive())
    _ApplyAllItemMods();
}

Item* Player::_LoadItem(uint32 zoneId, uint32 timeDiff, Json::Value& itemValue)
{
    Item* item = NULL;
    uint32 itemGuid = itemValue["itemGuid"].asUInt();
    uint32 itemEntry = itemValue["itemEntry"].asInt();
    if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry))
    {
        bool remove = false;
        item = NewItemOrBag(proto);
        item->SetItemKey(ITEM_KEY_USER, GetGUIDLow());

        if (item->LoadFromDB(itemGuid, GetGUID(), itemValue, itemEntry))
        {
            // Do not allow to have item limited to another map/zone in alive state
            if (isAlive() && item->IsLimitedToAnotherMapOrZone(GetMapId(), zoneId))
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s', map: %u) has item (GUID: %u, entry: %u) limited to another map (%u). Deleting item.",
                    GetGUIDLow(), GetName(), GetMapId(), item->GetGUIDLow(), item->GetEntry(), zoneId);
                remove = true;
            }
            // "Conjured items disappear if you are logged out for more than 15 minutes"
            else if (timeDiff > 15 * MINUTE && proto->Flags & ITEM_PROTO_FLAG_CONJURED)
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s', diff: %u) has conjured item (GUID: %u, entry: %u) with expired lifetime (15 minutes). Deleting item.",
                    GetGUIDLow(), GetName(), timeDiff, item->GetGUIDLow(), item->GetEntry());
                remove = true;
            }
            else if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_REFUNDABLE))
            {
                if (item->GetPlayedTime() > (2 * HOUR))
                {
                    sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) with expired refund time (%u). Deleting refund data and removing refundable flag.",
                        GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), item->GetPlayedTime());

                    item->RemoveFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_REFUNDABLE);
                }
                else
                {
                    item->SetRefundRecipient(itemValue["paidGuid"].asInt());
                    item->SetPaidMoney(itemValue["paidMoney"].asUInt());
                    item->SetPaidExtendedCost(itemValue["paidExtendedCost"].asInt());
                    AddRefundReference(item->GetGUIDLow());
                }
            }
            else if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE))
            {
                std::string strGUID = itemValue["allowedGUIDs"].asString();
                if (strGUID.empty())
                    item->RemoveFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE);
                else
                {
                    Tokenizer GUIDlist(strGUID, ' ');
                    AllowedLooterSet looters;
                    for (Tokenizer::const_iterator itr = GUIDlist.begin(); itr != GUIDlist.end(); ++itr)
                        looters.insert(atol(*itr));
                    item->SetSoulboundTradeable(looters);
                    AddTradeableItem(item);
                }
            }
            else if (proto->HolidayId)
            {
                remove = true;
                GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
                GameEventMgr::ActiveEvents const& activeEventsList = sGameEventMgr->GetActiveEventList();
                for (GameEventMgr::ActiveEvents::const_iterator itr = activeEventsList.begin(); itr != activeEventsList.end(); ++itr)
                {
                    if (uint32(events[*itr].holiday_id) == proto->HolidayId)
                    {
                        remove = false;
                        break;
                    }
                }
            }
        }
        else
        {
            sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has broken item (GUID: %u, entry: %u) in inventory. Deleting item.",
                GetGUIDLow(), GetName(), itemGuid, itemEntry);
            remove = true;
        }
        // Remove item from inventory if necessary
        if (remove)
        {
            //Item::DeleteFromInventoryDB(trans, itemGuid);
            item->FSetState(ITEM_REMOVED);
            //item->SaveToDB(trans);                           // it also deletes item object!
            item = NULL;
        }
    }
    else
    {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has unknown item (entry: %u) in inventory. Deleting item.",
            GetGUIDLow(), GetName(), itemEntry);
        /* Delete de cette foutue fonction, jamais tu ne delete des items !
        Item::DeleteFromInventoryDB(trans, itemGuid);
        Item::DeleteFromDB(trans, itemGuid);
        */
    }
    return item;
}

void Player::LoadPlayerVoidStorage()
{
    if (!PlayerData.isMember("voidstorage") || PlayerData["voidstorage"].empty())
        return;

    time_t now = time(NULL);
    time_t logoutTime = time_t(PlayerData["data"]["logout_time"].asUInt());

    // since last logout (in seconds)
    uint32 time_diff = uint32(now - logoutTime);

    for (auto iter = PlayerData["voidstorage"].begin(); iter != PlayerData["voidstorage"].end(); ++iter)
    {
        auto voidValue = *iter;
        uint8 slot = atoi(iter.memberName());

        uint64 itemId = voidValue["itemId"].asUInt64();
        uint32 itemEntry = voidValue["itemEntry"].asUInt();
        uint32 creatorGuid = voidValue["creatorGuid"].asUInt();
        uint32 randomProperty = voidValue["randomProperty"].asUInt();
        uint32 suffixFactor = voidValue["suffixFactor"].asUInt();

        if (!itemId)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player::_LoadVoidStorage - Player (GUID: %u, name: %s) has an item with an invalid id (item id: " UI64FMTD ", entry: %u).", GetGUIDLow(), GetName(), itemId, itemEntry);
            continue;
        }

        if (!sObjectMgr->GetItemTemplate(itemEntry))
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player::_LoadVoidStorage - Player (GUID: %u, name: %s) has an item with an invalid entry (item id: " UI64FMTD ", entry: %u).", GetGUIDLow(), GetName(), itemId, itemEntry);
            continue;
        }

        if (slot >= VOID_STORAGE_MAX_SLOT)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player::_LoadVoidStorage - Player (GUID: %u, name: %s) has an item with an invalid slot (item id: " UI64FMTD ", entry: %u, slot: %u).", GetGUIDLow(), GetName(), itemId, itemEntry, slot);
            continue;
        }

        if (!sObjectMgr->GetPlayerByLowGUID(creatorGuid))
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player::_LoadVoidStorage - Player (GUID: %u, name: %s) has an item with an invalid creator guid, set to 0 (item id: " UI64FMTD ", entry: %u, creatorGuid: %u).", GetGUIDLow(), GetName(), itemId, itemEntry, creatorGuid);
            creatorGuid = 0;
        }

        _voidStorageItems[slot] = new VoidStorageItem(itemId, itemEntry, creatorGuid, randomProperty, suffixFactor);
    }

    // update items with duration and realtime
    UpdateItemDuration(time_diff, true);
}

void Player::LoadPlayerActions()
{
    if (!PlayerData.isMember("actions") || PlayerData["actions"].empty())
        return;

    for (uint8 i = 0; i < MAX_SPEC_COUNT; ++i)
        m_actionButtons[i].clear();

    for (auto iter = PlayerData["actions"].begin(); iter != PlayerData["actions"].end(); ++iter)
    {
        uint8 spec = atoi(iter.memberName());
        auto infoData = *iter;
        for (auto itr = infoData.begin(); itr != infoData.end(); ++itr)
        {
            uint8 button = atoi(itr.memberName());
            auto buttonData = *itr;
            uint32 action = buttonData["action"].asInt();
            uint8 type = buttonData["type"].asInt();

            if (ActionButton* ab = addActionButton(button, action, type, spec))
                ab->uState = ACTIONBUTTON_UNCHANGED;
            else
                // Will deleted in DB at next save (it can create data until save but marked as deleted)
                m_actionButtons[spec][button].uState = ACTIONBUTTON_DELETED;
        }
    }
}

void SocialMgr::LoadFromDB(Player* player)
{
    PlayerSocial *social = &m_socialMap[player->GetGUIDLow()];
    social->SetPlayerGUID(player->GetGUIDLow());

    uint32 friend_guid = 0;
    uint8 flags = 0;
    std::string note = "";

    if (player->PlayerData.isMember("social") && !player->PlayerData["social"].empty())
    {
        for (auto iter = player->PlayerData["social"].begin(); iter != player->PlayerData["social"].end(); ++iter)
        {
            auto socialValue = *iter;
            friend_guid = atoi(iter.memberName());
            flags = socialValue["Flags"].asInt();
            note = socialValue["Note"].asString();

            social->m_playerSocialMap[friend_guid] = FriendInfo(flags, note);

            // client's friends list and ignore list limit
            if (social->m_playerSocialMap.size() >= (SOCIALMGR_FRIEND_LIMIT + SOCIALMGR_IGNORE_LIMIT))
                break;
        }
    }

    player->m_social = social;
    player->m_social->SetPlayer(player);
}

void Player::LoadPlayerSpellCooldowns()
{
    if (!PlayerData.isMember("spellcooldowns") || PlayerData["spellcooldowns"].empty())
        return;

    time_t curTime = time(NULL);

    for (auto iter = PlayerData["spellcooldowns"].begin(); iter != PlayerData["spellcooldowns"].end(); ++iter)
    {
        auto cooldownValue = *iter;
        uint32 spell_id = atoi(iter.memberName());
        uint32 item_id = cooldownValue["item"].asInt();
        time_t db_time = time_t(cooldownValue["time"].asInt64());

        if (!sSpellMgr->GetSpellInfo(spell_id))
        {
            sLog->outError(LOG_FILTER_REDIS, "Player %u has unknown spell %u in `character_spell_cooldown`, skipping.", GetGUIDLow(), spell_id);
            continue;
        }

        // skip outdated cooldown
        if (db_time <= curTime)
            continue;

        AddSpellCooldown(spell_id, item_id, (double)db_time);
    }
}

void Player::LoadPlayerKills()
{
    if (!PlayerData.isMember("kills") || PlayerData["kills"].empty())
        return;

    for (auto iter = PlayerData["kills"].begin(); iter != PlayerData["kills"].end(); ++iter)
    {
        uint32 victim_guid = atoi(iter.memberName());
        uint32 count = iter->asInt();

        KillInfo &info = m_killsPerPlayer[victim_guid];
        info.state = KILL_UNCHANGED;
        info.count = count;
    }
}

void Player::InitThirdPartDataPlayer()
{
    // unread mails and next delivery time, actual mails not loaded
    //_LoadMailInit(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADMAILCOUNT), holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADMAILDATE));

    // check PLAYER_CHOSEN_TITLE compatibility with PLAYER_FIELD_KNOWN_TITLES
    // note: PLAYER_FIELD_KNOWN_TITLES updated at quest status loaded
    uint32 curTitle = PlayerData["data"]["chosenTitle"].asUInt();
    if (curTitle && !HasTitle(curTitle))
        curTitle = 0;

    SetUInt32Value(PLAYER_CHOSEN_TITLE, curTitle);

    SetUInt32Value(PLAYER_HOME_PLAYER_REALM, realmID);

    // has to be called after last Relocate() in Player::LoadFromDB
    SetFallInformation(0, GetPositionZ());

    // Spell code allow apply any auras to dead character in load time in aura/spell/item loading
    // Do now before stats re-calculation cleanup for ghost state unexpected auras
    if (!isAlive())
        RemoveAllAurasOnDeath();
    else
        RemoveAllAurasRequiringDeadTarget();

    //apply all stat bonuses from items and auras
    SetCanModifyStats(true);
    UpdateAllStats();

    // restore remembered power/health values (but not more max values)
    uint32 savedHealth = PlayerData["data"]["health"].asUInt();
    SetHealth(savedHealth > GetMaxHealth() ? GetMaxHealth() : savedHealth);

    if (GetPowerIndexByClass(POWER_MANA, getClass()) != MAX_POWERS)
    {
        uint32 savedPower = PlayerData["data"]["power"].asUInt();
        uint32 maxPower = GetUInt32Value(UNIT_FIELD_MAXPOWER1 + 0);
        SetPower(POWER_MANA, (savedPower > maxPower) ? maxPower : savedPower);
    }

    // must be after loading spells and talents
    Tokenizer talentTrees(PlayerData["data"]["talentTree"].asString(), ' ', MAX_TALENT_SPECS);

    sLog->outDebug(LOG_FILTER_REDIS, "The value of player %s after load item and aura is: ", m_name.c_str());
    outDebugValues();

    uint32 extraflags = PlayerData["data"]["extra_flags"].asUInt();

    // GM state
    if (!AccountMgr::IsPlayerAccount(GetSession()->GetSecurity()))
    {
        switch (sWorld->getIntConfig(CONFIG_GM_LOGIN_STATE))
        {
            default:
            case 0:                      break;             // disable
            case 1: SetGameMaster(true); break;             // enable
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_GM_ON)
                    SetGameMaster(true);
                break;
        }

        switch (sWorld->getIntConfig(CONFIG_GM_VISIBLE_STATE))
        {
            default:
            case 0: SetGMVisible(false); break;             // invisible
            case 1:                      break;             // visible
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_GM_INVISIBLE)
                    SetGMVisible(false);
                break;
        }

        switch (sWorld->getIntConfig(CONFIG_GM_CHAT))
        {
            default:
            case 0:                  break;                 // disable
            case 1: SetGMChat(true); break;                 // enable
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_GM_CHAT)
                    SetGMChat(true);
                break;
        }

        switch (sWorld->getIntConfig(CONFIG_GM_WHISPERING_TO))
        {
            default:
            case 0:                          break;         // disable
            case 1: SetAcceptWhispers(true); break;         // enable
            case 2:                                         // save state
                if (extraflags & PLAYER_EXTRA_ACCEPT_WHISPERS)
                    SetAcceptWhispers(true);
                break;
        }
    }

    // RaF stuff.
    m_grantableLevels = PlayerData["data"]["grantableLevels"].asUInt();
    SetLfgBonusFaction(PlayerData["data"]["lfgBonusFaction"].asUInt());

    if (GetSession()->IsARecruiter() || (GetSession()->GetRecruiterId() != 0))
        SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_REFER_A_FRIEND);

    if (m_grantableLevels > 0)
        SetByteValue(PLAYER_FIELD_BYTES, 1, 0x01);

    m_achievementMgr.CheckAllAchievementCriteria(this);

    if (m_mustResurrectFromUnlock)
        ResurrectPlayer(1, true);

    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetGUID()))
        if (!ticket->IsClosed() && ticket->IsCompleted())
            ticket->SendResponse(GetSession());

    // Clean bug Specialization Spells
    RemoveNotActiveSpecializationSpells();
    _ApplyOrRemoveItemEquipDependentAuras(0, false);

   // Check professions
    //if(sWorld->getBoolConfig(CONFIG_CHECK_PROF_AT_LOGIN) && GetSession()->GetSecurity() < SEC_GAMEMASTER)
    {
        uint32 prof_count = 0;
        std::vector<uint32> prof_skills;
        prof_skills.push_back(164);     // Blacksmithing
        prof_skills.push_back(165);     // Leatherworking
        prof_skills.push_back(171);     // Alchemy
        prof_skills.push_back(182);     // Herbalism
        prof_skills.push_back(186);     // Mining
        prof_skills.push_back(197);     // Tailoring
        prof_skills.push_back(202);     // Engineering
        prof_skills.push_back(333);     // Enchanting
        prof_skills.push_back(393);     // Skinning
        prof_skills.push_back(755);     // Jewelcrafting
        prof_skills.push_back(773);     // Inscription

        for(std::vector<uint32>::iterator itr = prof_skills.begin(); itr != prof_skills.end(); ++itr)
        {
            uint32 skill_id = *itr;
            if(HasSkill(skill_id))
            {
                ++prof_count;
                if(prof_count > 2)
                    SetSkill(skill_id,0 , 0, 0);
            }    
        }
    }

    PlayerSpellMap _spells = m_spells;
    //dual spec check
    if (GetSpecsCount() == 2)
    {
        PlayerSpellMap::iterator itr = _spells.find(63645);
        if (itr == _spells.end())
            learnSpell(63645, true);

        itr = _spells.find(63644);
        if (itr == _spells.end())
            learnSpell(63644, true);
    }else
    {
        PlayerSpellMap::iterator itr = _spells.find(63645);
        if(itr != _spells.end())
        {
            SetSpecsCount(2);
            SetActiveSpec(1);
        }
    }

    //if(PreparedQueryResult PersonnalRateResult = holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_PERSONAL_RATE))
        //m_PersonnalXpRate = (PersonnalRateResult->Fetch())[0].GetFloat();
}

void Player::LoadPlayerDeclinedName()
{
    if (!PlayerData.isMember("declinedname") || PlayerData["declinedname"].empty())
        return;

    delete m_declinedname;
    m_declinedname = new DeclinedName;

    for (auto iter = PlayerData["declinedname"].begin(); iter != PlayerData["declinedname"].end(); ++iter)
    {
        uint32 index = atoi(iter.memberName());
        m_declinedname->name[index] = iter->asString();
    }
}

void Player::LoadPlayerEquipmentSets()
{
    if (!PlayerData.isMember("equipmentsets") || PlayerData["equipmentsets"].empty())
        return;

    uint32 count = 0;
    for (auto iter = PlayerData["equipmentsets"].begin(); iter != PlayerData["equipmentsets"].end(); ++iter)
    {
        uint8 index = atoi(iter.memberName());
        auto equipmentValue = *iter;

        EquipmentSet eqSet;

        eqSet.Guid      = equipmentValue["setguid"].asInt64();
        eqSet.Name      = equipmentValue["name"].asString();
        eqSet.IconName  = equipmentValue["iconname"].asString();
        eqSet.IgnoreMask = equipmentValue["ignore_mask"].asInt();
        eqSet.state     = EQUIPMENT_SET_UNCHANGED;

         eqSet.Items[0] = equipmentValue["item0"].asInt();
         eqSet.Items[1] = equipmentValue["item1"].asInt();
         eqSet.Items[2] = equipmentValue["item2"].asInt();
         eqSet.Items[3] = equipmentValue["item3"].asInt();
         eqSet.Items[4] = equipmentValue["item4"].asInt();
         eqSet.Items[5] = equipmentValue["item5"].asInt();
         eqSet.Items[6] = equipmentValue["item6"].asInt();
         eqSet.Items[7] = equipmentValue["item7"].asInt();
         eqSet.Items[8] = equipmentValue["item8"].asInt();
         eqSet.Items[9] = equipmentValue["item9"].asInt();
         eqSet.Items[10] = equipmentValue["item10"].asInt();
         eqSet.Items[11] = equipmentValue["item11"].asInt();
         eqSet.Items[12] = equipmentValue["item12"].asInt();
         eqSet.Items[13] = equipmentValue["item13"].asInt();
         eqSet.Items[14] = equipmentValue["item14"].asInt();
         eqSet.Items[15] = equipmentValue["item15"].asInt();
         eqSet.Items[16] = equipmentValue["item16"].asInt();
         eqSet.Items[17] = equipmentValue["item17"].asInt();
         eqSet.Items[18] = equipmentValue["item18"].asInt();

        m_EquipmentSets[index] = eqSet;

        ++count;

        if (count >= MAX_EQUIPMENT_SET_INDEX)                // client limit
            break;
    }
}

void Player::LoadPlayerCUFProfiles()
{
    if (!PlayerData.isMember("cufprofiles") || PlayerData["cufprofiles"].empty())
        return;

    uint32 count = 0;
    for (auto iter = PlayerData["cufprofiles"].begin(); iter != PlayerData["cufprofiles"].end(); ++iter)
    {
        uint8 id = atoi(iter.memberName());
        auto cufValue = *iter;

        std::string name   = cufValue["profileName"].asString();
        uint16 frameHeight = cufValue["frameHeight"].asInt();
        uint16 frameWidth  = cufValue["frameWidth"].asInt();
        uint8 sortBy       = cufValue["sortBy"].asInt();
        uint8 healthText   = cufValue["showHealthText"].asInt();
        uint32 options     = cufValue["options"].asInt();
        uint8 unk146       = cufValue["Unk146"].asInt();
        uint8 unk147       = cufValue["Unk147"].asInt();
        uint8 unk148       = cufValue["Unk148"].asInt();
        uint16 unk150      = cufValue["Unk150"].asInt();
        uint16 unk152      = cufValue["Unk152"].asInt();
        uint16 unk154      = cufValue["Unk154"].asInt();

        if (id > MAX_CUF_PROFILES)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player::_LoadCUFProfiles - Player (GUID: %u, name: %s) has an CUF profile with invalid id (id: %u), max is %i.", GetGUIDLow(), GetName(), id, MAX_CUF_PROFILES);
            continue;
        }

        _CUFProfiles[id] = new CUFProfile(name, frameHeight, frameWidth, sortBy, healthText, options, unk146, unk147, unk148, unk150, unk152, unk154);
    }
}

void Player::LoadPlayerVisuals()
{
    if (!PlayerData.isMember("visuals") || PlayerData["visuals"].empty())
        return;

    if (!m_vis)
        m_vis = new Visuals;

    m_vis->m_visHead = PlayerData["visuals"]["head"].asInt();
    m_vis->m_visShoulders = PlayerData["visuals"]["shoulders"].asInt();
    m_vis->m_visChest = PlayerData["visuals"]["chest"].asInt();
    m_vis->m_visWaist = PlayerData["visuals"]["waist"].asInt();
    m_vis->m_visLegs = PlayerData["visuals"]["legs"].asInt();
    m_vis->m_visFeet = PlayerData["visuals"]["feet"].asInt();
    m_vis->m_visWrists = PlayerData["visuals"]["wrists"].asInt();
    m_vis->m_visHands = PlayerData["visuals"]["hands"].asInt();
    m_vis->m_visBack = PlayerData["visuals"]["back"].asInt();
    m_vis->m_visMainhand = PlayerData["visuals"]["main"].asInt();
    m_vis->m_visOffhand = PlayerData["visuals"]["off"].asInt();
    m_vis->m_visRanged = PlayerData["visuals"]["ranged"].asInt();
    m_vis->m_visTabard = PlayerData["visuals"]["tabard"].asInt();
    m_vis->m_visShirt = PlayerData["visuals"]["shirt"].asInt();
}

void Player::LoadPlayerAccountData()
{
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (PER_CHARACTER_CACHE_MASK & (1 << i))
            GetSession()->m_accountData[i] = AccountData();

    if (!PlayerData.isMember("accountdata") || PlayerData["accountdata"].empty())
        return;

    for (auto iter = PlayerData["accountdata"].begin(); iter != PlayerData["accountdata"].end(); ++iter)
    {
        uint32 type = atoi(iter.memberName());
        auto dataValue = *iter;

        GetSession()->m_accountData[type].Time = time_t(dataValue["Time"].asUInt());
        GetSession()->m_accountData[type].Data = dataValue["Time"].asString();
    }
}

void Player::LoadPlayerHomeBind()
{
    if (!PlayerData.isMember("homebind") || PlayerData["homebind"].empty())
        return;

    m_homebindMapId = PlayerData["homebind"]["mapId"].asInt();
    m_homebindAreaId = PlayerData["homebind"]["zoneId"].asInt();
    m_homebindX = PlayerData["homebind"]["posX"].asFloat();
    m_homebindY = PlayerData["homebind"]["posY"].asFloat();
    m_homebindZ = PlayerData["homebind"]["posZ"].asFloat();
}

void Player::LoadAccountAchievements()
{
    if (!AccountDatas.isMember("achievement") || AccountDatas["achievement"].empty())
        return;

    for (auto iter = AccountDatas["achievement"].begin(); iter != AccountDatas["achievement"].end(); ++iter)
    {
        uint32 achievementid = atoi(iter.memberName());
        auto dataValue = *iter;

        uint32 first_guid    = dataValue["first_guid"].asInt64();
        uint32 date    = dataValue["date"].asInt();

        m_achievementMgr.AddAccountAchievements(achievementid, first_guid, date);
    }
}

void Player::LoadPlayerAchievements()
{
    if (!PlayerData.isMember("achievement") || PlayerData["achievement"].empty())
        return;

    for (auto iter = PlayerData["achievement"].begin(); iter != PlayerData["achievement"].end(); ++iter)
    {
        uint32 achievementid = atoi(iter.memberName());
        auto dataValue = *iter;
        uint32 date    = dataValue["date"].asInt();

        m_achievementMgr.AddAchievements(achievementid, date);
    }
}

void Player::LoadPlayerCriteriaProgress(std::vector<RedisValue>* progressVector)
{
    for (auto itr = progressVector->begin(); itr != progressVector->end();)
    {
        uint32 achievementID = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        std::string achievID = std::to_string(achievementID);
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), PlayerData["criteria"][achievID.c_str()]))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerCriteriaProgress not parse achievementID %i", achievementID);
            continue;
        }
        else
            ++itr;
    }

    LoadPlayerCriteriaProgress();
}

void Player::LoadAccountCriteriaProgress(std::vector<RedisValue>* progressVector)
{
    for (auto itr = progressVector->begin(); itr != progressVector->end();)
    {
        uint32 achievementID = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        std::string achievID = std::to_string(achievementID);
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), AccountDatas["criteria"][achievID.c_str()]))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadAccountCriteriaProgress not parse achievementID %i", achievementID);
            continue;
        }
        else
            ++itr;
    }

    LoadAccountCriteriaProgress();
    m_achievementMgr.GenerateProgressMap();
}

void Player::LoadPlayerGold()
{
    if (!PlayerData.isMember("gold") || PlayerData["gold"].empty())
        return;

    SetMoney(PlayerData["gold"].asUInt64());
}

void Player::LoadPlayerMails(std::vector<RedisValue>* mailVector)
{
    for (std::vector<RedisValue>::iterator itr = mailVector->begin(); itr != mailVector->end();)
    {
        uint32 messageID = atoi(itr->toString().c_str());
        std::string message = itr->toString();
        ++itr;

        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), PlayerMailData["mails"][message.c_str()]))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerMails not parse messageID %i", messageID);
            continue;
        }
        else
            ++itr;
    }
    LoadPlayerMails();
}

void Player::LoadPlayerMailItems(std::vector<RedisValue>* itemVector)
{
    for (std::vector<RedisValue>::iterator itr = itemVector->begin(); itr != itemVector->end();)
    {
        uint32 guid = atoi(itr->toString().c_str());
        std::string itemGuid = itr->toString();
        ++itr;

        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), PlayerMailData["mitems"][itemGuid.c_str()]))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerLoadItems not parse itemId %s", itemGuid.c_str());
            continue;
        }
        else
            ++itr;
    }
    LoadPlayerMailItems();
}

void Player::LoadPlayerGuild()
{
    if (!PlayerData.isMember("guild") || PlayerData["guild"].empty())
        return;

    uint32 guildId = PlayerData["guild"].asUInt();
    if (guildId)
    {
        if (Guild* guild = sGuildMgr->GetGuildById(guildId))
        {
            SetInGuild(guildId);
            SetGuild(guild);
            SetRank(guild->GetRankId(this));
            SetGuildLevel(guild->GetLevel());
        }
        else
            SavePlayerGuild();
    }
}

void Player::LoadPlayerCorpse()
{
    if (!PlayerData.isMember("corpse") || PlayerData["corpse"].empty())
        return;

    Corpse* corpse = new Corpse(CorpseType(PlayerData["corpse"]["corpseType"].asUInt()));
    if (!corpse->LoadCorpseFromDB(PlayerData["corpse"], this))
        delete corpse;
    else
        sObjectAccessor->AddCorpse(corpse);
}

void Player::LoadPlayerPetition()
{
    sObjectMgr->AddPlayerPetition(PlayerData["petitions"]["guid"].asUInt(), this);
}

void Player::BuildEnumData(uint32 gid, Json::Value dataValue, ByteBuffer* dataBuffer, ByteBuffer* bitBuffer)
{
    ObjectGuid guid = MAKE_NEW_GUID(gid, 0, HIGHGUID_PLAYER);
    std::string name = dataValue["name"].asString();
    uint8 plrRace = dataValue["plrRace"].asUInt();
    uint8 plrClass = dataValue["plrClass"].asUInt();
    uint8 gender = dataValue["gender"].asUInt();
    uint8 skin = dataValue["skin"].asUInt();
    uint8 face = dataValue["face"].asUInt();
    uint8 hairStyle = dataValue["hairStyle"].asUInt();
    uint8 hairColor = dataValue["hairColor"].asUInt();
    uint8 facialHair = dataValue["facialHair"].asUInt();
    uint8 level = dataValue["level"].asUInt();
    uint32 zone = dataValue["zone"].asUInt();
    uint32 mapId = dataValue["mapId"].asUInt();
    float x = dataValue["x"].asFloat();
    float y = dataValue["y"].asFloat();
    float z = dataValue["z"].asFloat();
    uint32 guildId = dataValue["guildId"].asUInt();
    ObjectGuid guildGuid = MAKE_NEW_GUID(guildId, 0, guildId ? uint32(HIGHGUID_GUILD) : 0);
    uint32 playerFlags = dataValue["playerFlags"].asUInt();
    uint32 atLoginFlags = dataValue["atLoginFlags"].asUInt();
    Tokenizer equipment(dataValue["equipment"].asString(), ' ');
    uint8 slotList = dataValue["slot"].asUInt();

    uint32 charFlags = CHARACTER_FLAG_UNK29 | CHARACTER_FLAG_UNK24 | CHARACTER_FLAG_UNK22;
    if (playerFlags & PLAYER_FLAGS_HIDE_HELM)
        charFlags |= CHARACTER_FLAG_HIDE_HELM;

    if (playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
        charFlags |= CHARACTER_FLAG_HIDE_CLOAK;

    if (playerFlags & PLAYER_FLAGS_GHOST)
        charFlags |= CHARACTER_FLAG_GHOST;

    if (atLoginFlags & AT_LOGIN_RENAME)
        charFlags |= CHARACTER_FLAG_RENAME;

    //if (fields[20].GetUInt32())
        //charFlags |= CHARACTER_FLAG_LOCKED_BY_BILLING;

    /*if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        if (!fields[22].GetString().empty())
            charFlags |= CHARACTER_FLAG_DECLINED;
    }else*/
        charFlags |= CHARACTER_FLAG_DECLINED;

    uint32 customizationFlag = CHAR_CUSTOMIZE_FLAG_2 | CHAR_CUSTOMIZE_FLAG_24;
    if (atLoginFlags & AT_LOGIN_CUSTOMIZE)
        customizationFlag = CHAR_CUSTOMIZE_FLAG_CUSTOMIZE;
    else if (atLoginFlags & AT_LOGIN_CHANGE_FACTION)
        customizationFlag = CHAR_CUSTOMIZE_FLAG_FACTION;
    else if (atLoginFlags & AT_LOGIN_CHANGE_RACE)
        customizationFlag = CHAR_CUSTOMIZE_FLAG_RACE;

    uint32 petDisplayId = 0;
    uint32 petLevel   = 0;
    uint32 petFamily  = 0;
    // show pet at selection character in character list only for non-ghost character
    if (!(playerFlags & PLAYER_FLAGS_GHOST) && (plrClass == CLASS_WARLOCK || plrClass == CLASS_HUNTER || plrClass == CLASS_DEATH_KNIGHT))
    {
        uint32 entry = dataValue["petEntry"].asUInt();
        if (CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(entry))
        {
            petDisplayId = dataValue["petDisplayId"].asUInt();
            petLevel     = dataValue["petLevel"].asUInt();
            petFamily    = creatureInfo->family;
        }
    }

    // Packet content flags
    bitBuffer->WriteGuidMask<3>(guildGuid);
    bitBuffer->WriteBit(atLoginFlags & AT_LOGIN_FIRST);
    bitBuffer->WriteGuidMask<6>(guid);
    bitBuffer->WriteGuidMask<1>(guildGuid);
    bitBuffer->WriteGuidMask<1, 5>(guid);
    bitBuffer->WriteGuidMask<6>(guildGuid);
    bitBuffer->WriteGuidMask<7, 0>(guid);
    bitBuffer->WriteGuidMask<5>(guildGuid);
    bitBuffer->WriteGuidMask<2>(guid);
    bitBuffer->WriteBits(uint32(name.length()), 6);
    bitBuffer->WriteGuidMask<4>(guid);
    bitBuffer->WriteGuidMask<4, 2>(guildGuid);
    bitBuffer->WriteGuidMask<3>(guid);
    bitBuffer->WriteGuidMask<0, 7>(guildGuid);

    *dataBuffer << uint8(skin);                                 // Skin
    dataBuffer->WriteGuidBytes<2, 7>(guid);
    *dataBuffer << uint32(petDisplayId);                        // Pet DisplayID
    dataBuffer->WriteString(name);                              // Name

    // Character data
    for (uint8 slot = 0; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        uint32 visualbase = slot * 2;
        uint32 itemId = GetUInt32ValueFromArray(equipment, visualbase);
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
        {
            *dataBuffer << uint32(0);
            *dataBuffer << uint32(0);
            *dataBuffer << uint8(0);
            continue;
        }

        SpellItemEnchantmentEntry const *enchant = NULL;
        uint32 enchants = GetUInt32ValueFromArray(equipment, visualbase + 1);
        for (uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & (enchants >> enchantSlot*16);
            if (!enchantId)
                continue;

            enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
            if (enchant)
                break;
        }

        *dataBuffer << uint32(proto->DisplayInfoID);
        *dataBuffer << uint32(enchant ? enchant->aura_id : 0);
        *dataBuffer << uint8(proto->InventoryType);
    }

    dataBuffer->WriteGuidBytes<4, 6>(guid);
    *dataBuffer << uint8(level);                                // Level
    *dataBuffer << float(y);                                    // Y
    *dataBuffer << float(x);                                    // X
    *dataBuffer << uint8(face);                                 // Face
    dataBuffer->WriteGuidBytes<0>(guildGuid);

    *dataBuffer << uint8(slotList);                             // List order
    *dataBuffer << uint32(zone);                                // Zone id

    dataBuffer->WriteGuidBytes<7>(guildGuid);

    *dataBuffer << uint32(charFlags);                           // Character flags
    *dataBuffer << uint32(mapId);                               // Map Id
    *dataBuffer << uint8(plrRace);                              // Race
    *dataBuffer << float(z);                                    // Z

    dataBuffer->WriteGuidBytes<1>(guildGuid);

    *dataBuffer << uint8(gender);                               // Gender

    dataBuffer->WriteGuidBytes<3>(guid);

    *dataBuffer << uint8(hairColor);                            // Hair color

    dataBuffer->WriteGuidBytes<5>(guildGuid);

    *dataBuffer << uint8(plrClass);                             // Class

    dataBuffer->WriteGuidBytes<2>(guildGuid);
    dataBuffer->WriteGuidBytes<1>(guid);

    *dataBuffer << uint32(customizationFlag);                   // Character customization flags
    *dataBuffer << uint8(facialHair);                           // Facial hair

    dataBuffer->WriteGuidBytes<6>(guildGuid);
    dataBuffer->WriteGuidBytes<0>(guid);

    *dataBuffer << uint8(hairStyle);                            // Hair style

    dataBuffer->WriteGuidBytes<5>(guid);

    *dataBuffer << uint32(petFamily);                           // Pet family

    dataBuffer->WriteGuidBytes<2>(guildGuid);

    *dataBuffer << uint32(petLevel);                            // Pet level

    dataBuffer->WriteGuidBytes<4>(guildGuid);
}

bool Player::LoadPlayerFromJson(uint64 guid)
{
    InitCharKeys(GUID_LOPART(guid));

    LoadPlayerHomeBind();

    if(!LoadPlayer(guid))
        return false;

    LoadPlayerGold();
    LoadAccountAchievements();
    LoadPlayerAchievements();
    LoadPlayerCriteriaProgress();
    LoadPlayerCriteriaProgress();

    if(!LoadPlayerNext(guid))
        return false;

    LoadPlayerGroup();
    LoadPlayerLootCooldown();
    LoadPlayerCurrency();
    LoadPlayerBoundInstances();
    LoadPlayerBG();
    InitSecondPartDataPlayer();
    LoadPlayerBattlePets();
    LoadPlayerBattlePetSlots();
    LoadPlayerSkills();

    for (uint8 i = 0; i < MAX_RESEARCH_SITES; ++i)
        _digSites[i].count = 0;

    if (sWorld->getBoolConfig(CONFIG_ARCHAEOLOGY_ENABLED))
        LoadPlayerArchaeology();

    uint8 specCount = PlayerData["data"]["speccount"].asUInt();
    SetSpecsCount(specCount == 0 ? 1 : specCount);
    SetActiveSpec(specCount > 1 ? PlayerData["data"]["activespec"].asUInt() : 0);

    SetSpecializationId(0, PlayerData["data"]["specialization1"].asUInt());
    SetSpecializationId(1, PlayerData["data"]["specialization2"].asUInt());

    SetFreeTalentPoints(CalculateTalentsPoints());

    // sanity check
    if (GetSpecsCount() > MAX_TALENT_SPECS || GetActiveSpec() > MAX_TALENT_SPEC || GetSpecsCount() < MIN_TALENT_SPECS)
    {
        SetActiveSpec(0);
        sLog->outError(LOG_FILTER_PLAYER, "Player %s(GUID: %u) has SpecCount = %u and ActiveSpec = %u.", GetName(), GetGUIDLow(), GetSpecsCount(), GetActiveSpec());
    }

    LoadPlayerTalents();
    LoadPlayerSpells();
    LoadPlayerMounts();
    LoadPlayerGlyphs();
    LoadPlayerAuras();

    _LoadGlyphAuras();
    // add ghost flag (must be after aura load: PLAYER_FLAGS_GHOST set in aura)
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        m_deathState = DEAD;

    // after spell load, learn rewarded spell if need also - test on achievements
    _LoadSpellRewards();

    LoadPlayerQuestStatus();
    LoadAccountQuestStatus();
    LoadPlayerQuestRewarded();
    LoadAccountQuestRewarded();
    LoadPlayerQuestDaily();
    LoadAccountQuestDaily();
    LoadPlayerQuestWeekly();
    LoadAccountQuestWeekly();
    LoadPlayerQuestSeasonal();
    LoadAccountQuestSeasonal();

    // after spell and quest load
    InitTalentForLevel();
    InitSpellForLevel();
    learnDefaultSpells();

    m_reputationMgr.LoadFromDB();
    LoadPlayerLoadItems();
    LoadPlayerVoidStorage();
    LoadPlayerActions();
    sSocialMgr->LoadFromDB(this);
    LoadPlayerSpellCooldowns();
    LoadPlayerKills();
    InitThirdPartDataPlayer();
    LoadPlayerDeclinedName();
    LoadPlayerEquipmentSets();
    LoadPlayerCUFProfiles();
    LoadPlayerVisuals();
    LoadPlayerAccountData();
    LoadPlayerGuild();
    LoadPlayerCorpse();
    LoadPlayerPetition();
    LoadPlayerMails();
    LoadPlayerMailItems(true);

    //Save data to redis
    SavePlayerDataAll();

    return true;
}

void Player::LoadPlayerCriteriaProgress()
{
    if (!PlayerData.isMember("criteria") || PlayerData["criteria"].empty())
        return;

    for (auto iter = PlayerData["criteria"].begin(); iter != PlayerData["criteria"].end(); ++iter)
    {
        std::string achievID = iter.memberName();
        uint32 achievementid = atoi(iter.memberName());

        time_t now = time(NULL);
        for (auto iter = PlayerData["criteria"][achievID.c_str()].begin(); iter != PlayerData["criteria"][achievID.c_str()].end(); ++iter)
        {
            uint32 char_criteria_id = atoi(iter.memberName());
            auto dataValue = *iter;

            time_t date    = time_t(dataValue["date"].asUInt());
            uint32 counter = dataValue["counter"].asUInt();
            bool completed = dataValue["completed"].asBool();

            m_achievementMgr.AddCriteriaProgress(achievementid, char_criteria_id, date, counter, completed);
        }
    }
}

void Player::LoadAccountCriteriaProgress()
{
    if (!AccountDatas.isMember("criteria") || AccountDatas["criteria"].empty())
    {
        m_achievementMgr.GenerateProgressMap();
        return;
    }

    for (auto itr = AccountDatas["criteria"].begin(); itr != AccountDatas["criteria"].end(); ++itr)
    {
        uint32 achievementid = atoi(itr.memberName());
        auto data = *itr;

        for (auto iter = data.begin(); iter != data.end(); ++iter)
        {
            uint32 char_criteria_id = atoi(iter.memberName());
            auto dataValue = *iter;

            time_t date    = time_t(dataValue["date"].asUInt());
            uint32 counter = dataValue["counter"].asUInt();
            bool completed = dataValue["completed"].asBool();

            m_achievementMgr.AddAccountCriteriaProgress(achievementid, char_criteria_id, date, counter, completed);
        }
    }

    m_achievementMgr.GenerateProgressMap();
}

void Player::LoadPlayerMails()
{
    if (!PlayerMailData.isMember("mails") || PlayerMailData["mails"].empty())
        return;

    for (auto itr = PlayerMailData["mails"].begin(); itr != PlayerMailData["mails"].end(); ++itr)
    {
        uint32 messageID = atoi(itr.memberName());
        auto mailData = *itr;

        Mail* m = new Mail;

        m->messageID      = messageID;
        m->messageType    = mailData["messageType"].asUInt();
        m->sender         = mailData["sender"].asUInt();
        m->receiver       = mailData["receiver"].asUInt();
        m->subject        = mailData["subject"].asString();
        m->body           = mailData["body"].asString();
        m->expire_time    = time_t(mailData["expire_time"].asUInt());
        m->deliver_time   = time_t(mailData["deliver_time"].asUInt());
        m->money          = mailData["money"].asUInt64();
        m->COD            = mailData["COD"].asUInt64();
        m->checked        = mailData["checked"].asUInt();
        m->stationery     = mailData["stationery"].asUInt();
        m->mailTemplateId = mailData["mailTemplateId"].asUInt();

        if (m->mailTemplateId && !sMailTemplateStore.LookupEntry(m->mailTemplateId))
        {
            sLog->outError(LOG_FILTER_PLAYER, "Player::_LoadMail - Mail (%u) have not existed MailTemplateId (%u), remove at load", m->messageID, m->mailTemplateId);
            m->mailTemplateId = 0;
        }

        m->state = MAIL_STATE_UNCHANGED;

        m_mail.push_back(m);
    }

    m_mailsLoaded = true;
    sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerMails end");
}

void Player::LoadPlayerMailItems(bool isSave)
{
    if (!PlayerMailData.isMember("mitems"))
        return;

    for (auto itr = PlayerMailData["mitems"].begin(); itr != PlayerMailData["mitems"].end(); ++itr)
    {
        std::string guid = itr.memberName();
        uint32 itemGuid = atoi(itr.memberName());
        auto itemData = *itr;

        uint32 itemEntry = itemData["itemEntry"].asUInt();

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
        if (!proto)
        {
            RedisDatabase.AsyncExecuteH("HDEL", GetMailItemKey(), guid.c_str(), itemGuid, [&](const RedisValue &v, uint64 guid) {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerMailItems items id %u", guid);
            });
            return;
        }

        uint32 mailId = itemData["m_mailId"].asUInt();

        Mail* mail = GetMail(mailId);
        if (!mail)
        {
            RedisDatabase.AsyncExecuteH("HDEL", GetMailItemKey(), guid.c_str(), itemGuid, [&](const RedisValue &v, uint64 guid) {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerMailItems items id %u", guid);
            });
            sLog->outInfo(LOG_FILTER_REDIS, "Player::LoadPlayerMailItems mail %u not found ", mailId);
            return;
        }

        Item* item = NewItemOrBag(proto);
        item->SetItemKey(ITEM_KEY_MAIL, GetGUIDLow());

        if (!item->LoadFromDB(itemGuid, GetGUIDLow(), itemData, itemEntry))
        {
            item->SetState(ITEM_REMOVED);
            delete item;
            return;
        }

        mail->AddItem(itemGuid, itemEntry);
        AddMItem(item);

        //Need save when load from other source
        if (isSave)
            item->SaveItem();
    }
}

void Player::LoadPlayerLoadItems()
{
    time_t now = time(NULL);
    time_t logoutTime = time_t(PlayerData["data"]["logout_time"].asUInt());

    // since last logout (in seconds)
    uint32 time_diff = uint32(now - logoutTime);

    uint32 zoneId = GetZoneId();

    std::map<uint32, Bag*> bagMap;                                  // fast guid lookup for bags
    std::map<uint32, Item*> invalidBagMap;                          // fast guid lookup for bags
    std::list<Item*> problematicItems;
    std::map<uint32, Json::Value> itemInBag;                        // Save item in bag, for next step loading

    for (auto itr = PlayerData["items"].begin(); itr != PlayerData["items"].end(); ++itr)
    {
        uint32 itemGuid = atoi(itr.memberName());
        auto loadItemJson = *itr;

        uint32 bagGuid = loadItemJson["bagGuid"].asUInt();
        if(bagGuid)
        {
            itemInBag[itemGuid] = loadItemJson;
            continue;
        }

        if (Item* item = _LoadItem(zoneId, time_diff, loadItemJson))
        {
            uint8 slot = loadItemJson["slot"].asInt();

            uint8 err = EQUIP_ERR_OK;
            // Item is not in bag
            item->SetContainer(NULL);
            item->SetSlot(slot);

            if (slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END)
            {
                AddItemToBuyBackSlot(item);
                continue;
            }
            // check for already equiped item
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                err = EQUIP_ERR_ITEM_MAX_COUNT;

            if (err == EQUIP_ERR_OK)
            {
                if (IsInventoryPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    ItemPosCountVec dest;
                    err = CanStoreItem(INVENTORY_SLOT_BAG_0, slot, dest, item, false);
                    if (err == EQUIP_ERR_OK)
                        item = StoreItem(dest, item, true);
                }
                else if (IsEquipmentPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    uint16 dest;
                    err = CanEquipItem(slot, dest, item, false, false);
                    if (err == EQUIP_ERR_OK)
                        QuickEquipItem(dest, item);
                }
                else if (IsBankPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    ItemPosCountVec dest;
                    err = CanBankItem(INVENTORY_SLOT_BAG_0, slot, dest, item, false, false);
                    if (err == EQUIP_ERR_OK)
                        item = BankItem(dest, item, true);
                }
            }

            // Remember bags that may contain items in them
            if (err == EQUIP_ERR_OK)
            {
                if (IsBagPos(item->GetPos()))
                    if (Bag* pBag = item->ToBag())
                        bagMap[item->GetGUIDLow()] = pBag;
            }
            else
                if (IsBagPos(item->GetPos()))
                    if (item->IsBag())
                        invalidBagMap[item->GetGUIDLow()] = item;

            // Item's state may have changed after storing
            if (err == EQUIP_ERR_OK)
                item->SetState(ITEM_UNCHANGED, this);
            else
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) which can't be loaded into inventory (Bag GUID: %u, slot: %u) by reason %u. Item will be sent by mail.",
                    GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), bagGuid, slot, err);
                //item->DeleteFromInventoryDB(trans);
                RemoveItemDurations(item);
                RemoveTradeableItem(item);
                problematicItems.push_back(item);
            }
        }

        // Send problematic items by mail
        while (!problematicItems.empty())
        {
            std::string subject = GetSession()->GetTrinityString(LANG_NOT_EQUIPPED_ITEM);

            MailDraft draft(subject, "There were problems with equipping item(s).");
            for (uint8 i = 0; !problematicItems.empty() && i < MAX_MAIL_ITEMS; ++i)
            {
                draft.AddItem(problematicItems.front());
                problematicItems.pop_front();
            }
            draft.SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
        }
    }

    for (auto itr = itemInBag.begin(); itr != itemInBag.end(); ++itr)
    {
        uint32 itemId = itr->first;
        Json::Value loadItemJson = itr->second;

        uint32 itemGuid = loadItemJson["itemGuid"].asUInt();
        uint32 bagGuid = loadItemJson["bagGuid"].asUInt();

        if (Item* item = _LoadItem(zoneId, time_diff, loadItemJson))
        {
            uint8  slot = loadItemJson["slot"].asInt();

            uint8 err = EQUIP_ERR_OK;

            item->SetSlot(NULL_SLOT);
            // Item is in the bag, find the bag
            std::map<uint32, Bag*>::iterator itr = bagMap.find(bagGuid);
            if (itr != bagMap.end())
            {
                ItemPosCountVec dest;
                err = CanStoreItem(itr->second->GetSlot(), slot, dest, item);
                if (err == EQUIP_ERR_OK)
                    item = StoreItem(dest, item, true);
            }
            else if (invalidBagMap.find(bagGuid) != invalidBagMap.end())
            {
                std::map<uint32, Item*>::iterator itr = invalidBagMap.find(bagGuid);
                if (std::find(problematicItems.begin(), problematicItems.end(), itr->second) != problematicItems.end())
                    err = EQUIP_ERR_INTERNAL_BAG_ERROR;
            }
            else
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) which doesnt have a valid bag (Bag GUID: %u, slot: %u). Possible cheat?",
                    GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), bagGuid, slot);
                RemoveItemDurations(item);
                RemoveTradeableItem(item);
                item->DeleteFromRedis();
                delete item;
                continue;
            }

            // Item's state may have changed after storing
            if (err == EQUIP_ERR_OK)
                item->SetState(ITEM_UNCHANGED, this);
            else
            {
                sLog->outInfo(LOG_FILTER_REDIS, "Player::_LoadInventory: player (GUID: %u, name: '%s') has item (GUID: %u, entry: %u) which can't be loaded into inventory (Bag GUID: %u, slot: %u) by reason %u. Item will be sent by mail.",
                    GetGUIDLow(), GetName(), item->GetGUIDLow(), item->GetEntry(), bagGuid, slot, err);
                //item->DeleteFromInventoryDB(trans);
                RemoveItemDurations(item);
                RemoveTradeableItem(item);
                problematicItems.push_back(item);
            }
        }

        // Send problematic items by mail
        while (!problematicItems.empty())
        {
            std::string subject = GetSession()->GetTrinityString(LANG_NOT_EQUIPPED_ITEM);

            MailDraft draft(subject, "There were problems with equipping item(s).");
            for (uint8 i = 0; !problematicItems.empty() && i < MAX_MAIL_ITEMS; ++i)
            {
                draft.AddItem(problematicItems.front());
                problematicItems.pop_front();
            }
            draft.SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
        }
    }

    //if (isAlive())
    _ApplyAllItemMods();
}
