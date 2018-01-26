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
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "QuestDef.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Battleground.h"
#include "BattlegroundAV.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"

void WorldSession::HandleQuestgiverStatusQueryOpcode(WorldPacket & recvData)
{
    ObjectGuid guid;

    recvData.ReadGuidMask<1, 0, 5, 2, 4, 6, 7, 3>(guid);
    recvData.ReadGuidBytes<0, 3, 4, 5, 6, 1, 7, 2>(guid);

    uint32 questStatus = DIALOG_STATUS_NONE;
    uint32 defstatus = DIALOG_STATUS_NONE;

    Object* questgiver = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT|TYPEMASK_GAMEOBJECT);
    if (!questgiver)
    {
        TC_LOG_INFO("network", "Error in CMSG_QUESTGIVER_STATUS_QUERY, called for not found questgiver (Typeid: %u GUID: %u)", GuidHigh2TypeId(GUID_HIPART(guid)), GUID_LOPART(guid));
        return;
    }

    switch (questgiver->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for npc, guid = %u", uint32(GUID_LOPART(guid)));
            Creature* cr_questgiver=questgiver->ToCreature();
            if (!cr_questgiver->IsHostileTo(_player))       // do not show quest status to enemies
            {
                questStatus = sScriptMgr->GetDialogStatus(_player, cr_questgiver);
                if (questStatus > 6)
                    questStatus = getDialogStatus(_player, cr_questgiver, defstatus);
            }
            break;
        }
        case TYPEID_GAMEOBJECT:
        {
            TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for GameObject guid = %u", uint32(GUID_LOPART(guid)));
            GameObject* go_questgiver=(GameObject*)questgiver;
            questStatus = sScriptMgr->GetDialogStatus(_player, go_questgiver);
            if (questStatus > 6)
                questStatus = getDialogStatus(_player, go_questgiver, defstatus);
            break;
        }
        default:
            TC_LOG_ERROR("network", "QuestGiver called for unexpected type %u", questgiver->GetTypeId());
            break;
    }

    //inform client about status of quest
    _player->PlayerTalkClass->SendQuestGiverStatus(questStatus, guid);
}

void WorldSession::HandleQuestgiverHelloOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 4, 5, 1, 7, 3, 0, 2>(guid);
    recvData.ReadGuidBytes<1, 3, 5, 4, 6, 0, 7, 2>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_HELLO npc = %u", GUID_LOPART(guid));

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!creature)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleQuestgiverHelloOpcode - Unit (GUID: %u) not found or you can't interact with him.",
            GUID_LOPART(guid));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);
    // Stop the npc if moving
    creature->StopMoving();

    if (sScriptMgr->OnGossipHello(_player, creature))
        return;

    _player->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);
    _player->SendPreparedGossip(creature);

    creature->AI()->sGossipHello(_player);
}

void WorldSession::HandleQuestgiverAcceptQuestOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;
    recvData >> questId;
    recvData.ReadGuidMask<6, 7>(guid);
    bool unk1 = recvData.ReadBit();
    recvData.ReadGuidMask<1, 5, 2, 4, 3, 0>(guid);
    recvData.ReadGuidBytes<7, 6, 0, 1, 4, 3, 2, 5>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_ACCEPT_QUEST npc = %u, quest = %u, unk1 = %u", uint32(GUID_LOPART(guid)), questId, unk1);

    Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT|TYPEMASK_GAMEOBJECT|TYPEMASK_ITEM|TYPEMASK_PLAYER);

    if (!object || object == _player)
        return;

    // no or incorrect quest giver
    if (!object || (object->GetTypeId() != TYPEID_PLAYER && !object->hasQuest(questId)) ||
        (object->GetTypeId() == TYPEID_PLAYER && object != _player && !object->ToPlayer()->CanShareQuest(questId)))
    {
        _player->PlayerTalkClass->SendCloseGossip();
        _player->SetDivider(0);
        return;
    }

    if (object && object->GetTypeId() == TYPEID_PLAYER && !object->hasQuest(questId))
        return;

    // some kind of WPE protection
    if (!_player->CanInteractWithQuestGiver(object))
        return;

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        // prevent cheating
        if (!GetPlayer()->CanTakeQuest(quest, true))
        {
            _player->PlayerTalkClass->SendCloseGossip();
            _player->SetDivider(0);
            return;
        }

        if (object && object->GetTypeId() == TYPEID_PLAYER && !quest->HasFlag(QUEST_FLAGS_SHARABLE))
            return;

        if (_player->GetDivider() != 0)
        {
            Player* player = ObjectAccessor::FindPlayer(_player->GetDivider());
            if (!player)
            {
                _player->SetDivider(0);
                return;
            }
            if (player)
            {
                if (!player->CanShareQuest(questId))
                {
                    player->SendPushToPartyResponse(_player, QUEST_PARTY_MSG_CANT_TAKE_QUEST);
                    _player->SetDivider(0);
                    return;
                }
                player->SendPushToPartyResponse(_player, QUEST_PARTY_MSG_ACCEPT_QUEST);
                _player->SetDivider(0);
            }
        }

        if (_player->CanAddQuest(quest, true))
        {
            _player->AddQuest(quest, object);

            if (quest->HasFlag(QUEST_FLAGS_PARTY_ACCEPT))
            {
                if (Group* group = _player->GetGroup())
                {
                    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                    {
                        Player* player = itr->getSource();

                        if (!player || player == _player)     // not self
                            continue;

                        if (player->CanTakeQuest(quest, true))
                        {
                            player->SetDivider(_player->GetGUID());

                            //need confirmation that any gossip window will close
                            player->PlayerTalkClass->SendCloseGossip();

                            _player->SendQuestConfirmAccept(quest, player);
                        }
                    }
                }
            }

            if (_player->CanCompleteQuest(questId))
                _player->CompleteQuest(questId);

            switch (object->GetTypeId())
            {
                case TYPEID_UNIT:
                    sScriptMgr->OnQuestAccept(_player, (object->ToCreature()), quest);
                    (object->ToCreature())->AI()->sQuestAccept(_player, quest);
                    break;
                case TYPEID_ITEM:
                case TYPEID_CONTAINER:
                {
                    sScriptMgr->OnQuestAccept(_player, ((Item*)object), quest);

                    // destroy not required for quest finish quest starting item
                    bool destroyItem = true;
                    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
                    {
                        if ((quest->RequiredItemId[i] == ((Item*)object)->GetEntry()) && (((Item*)object)->GetTemplate()->MaxCount > 0))
                        {
                            destroyItem = false;
                            break;
                        }
                    }

                    if (destroyItem)
                        _player->DestroyItem(((Item*)object)->GetBagSlot(), ((Item*)object)->GetSlot(), true);

                    break;
                }
                case TYPEID_GAMEOBJECT:
                    sScriptMgr->OnQuestAccept(_player, ((GameObject*)object), quest);
                    (object->ToGameObject())->AI()->QuestAccept(_player, quest);
                    break;
                default:
                    break;
            }
            _player->PlayerTalkClass->SendCloseGossip();

            if (quest->GetSrcSpell() > 0)
                _player->CastSpell(_player, quest->GetSrcSpell(), true);

            return;
        }
    }

    _player->PlayerTalkClass->SendCloseGossip();
}

void WorldSession::HandleQuestgiverQueryQuestOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;
    bool unk1;

    recvData >> questId;
    recvData.ReadGuidMask<6, 0, 1, 3, 7, 2>(guid);
    unk1 = recvData.ReadBit();
    recvData.ReadGuidMask<4, 5>(guid);
    recvData.ReadGuidBytes<5, 0, 4, 7, 3, 1, 6, 2>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_QUERY_QUEST npc = %u, quest = %u, unk1 = %u", uint32(GUID_LOPART(guid)), questId, unk1);

    // Verify that the guid is valid and is a questgiver or involved in the requested quest
    Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM);
    if (!object || (!object->hasQuest(questId) && !object->hasInvolvedQuest(questId)))
    {
        _player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        // not sure here what should happen to quests with QUEST_FLAGS_AUTOCOMPLETE
        // if this breaks them, add && object->GetTypeId() == TYPEID_ITEM to this check
        // item-started quests never have that flag
        if (!_player->CanTakeQuest(quest, true))
            return;

        if ( _player->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
            _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, object->GetGUID(), true);
        else if (_player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
            _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, object->GetGUID(), _player->CanCompleteQuest(quest->GetQuestId()), true);
        else
        {
            if (quest->IsAutoAccept() && _player->CanAddQuest(quest, true))
            {
                if (Creature* pQuestGiver = ObjectAccessor::GetCreature(*_player, guid))
                    if (pQuestGiver->IsAIEnabled)
                        sScriptMgr->OnQuestAccept(_player, pQuestGiver, quest);

                _player->AddQuest(quest, object);
                if (_player->CanCompleteQuest(questId))
                    _player->CompleteQuest(questId);
            }
            _player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, object->GetGUID(), true);
        }
    }
}

void WorldSession::HandleQuestQueryOpcode(WorldPacket & recvData)
{
    if (!_player)
        return;

    uint32 questId;

    ObjectGuid guid;
    recvData >> questId;
    recvData.ReadGuidMask<3, 5, 0, 1, 2, 6, 4, 7>(guid);
    recvData.ReadGuidBytes<2, 0, 4, 6, 7, 1, 3, 5>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUEST_QUERY quest = %u", questId);

    _player->PlayerTalkClass->SendQuestQueryResponse(questId);
}

void WorldSession::HandleQuestgiverChooseRewardOpcode(WorldPacket& recvData)
{
    uint32 questId, reward = 0;
    uint32 slot;
    ObjectGuid guid;
    recvData >> questId >> slot;
    recvData.ReadGuidMask<0, 4, 7, 6, 2, 3, 5, 1>(guid);
    recvData.ReadGuidBytes<4, 3, 6, 0, 1, 2, 5, 7>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_CHOOSE_REWARD npc = %u, quest = %u, slot = %u", uint32(GUID_LOPART(guid)), questId, slot);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    Object* object = _player;

    if (!quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
    {
        //TODO: Doing something less dirty
        for (int i = 0; i < QUEST_REWARD_CHOICES_COUNT; i++)
            if (quest->RewardChoiceItemId[i] == slot)
                reward = i;
        if (reward >= QUEST_REWARD_CHOICES_COUNT)
        {
            TC_LOG_ERROR("network", "Error in CMSG_QUESTGIVER_CHOOSE_REWARD: player %s (guid %d) tried to get invalid reward (%u) (probably packet hacking)", _player->GetName(), _player->GetGUIDLow(), reward);
            return;
        }
        object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT|TYPEMASK_GAMEOBJECT);
        if (!object || !object->hasInvolvedQuest(questId))
            return;

        // some kind of WPE protection
        if (!_player->CanInteractWithQuestGiver(object))
            return;

    }
    if ((!_player->CanSeeStartQuest(quest) &&  _player->GetQuestStatus(questId) == QUEST_STATUS_NONE) ||
        (_player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE && !quest->IsAutoComplete()))
    {
        TC_LOG_ERROR("network", "HACK ALERT: Player %s (guid: %u) is trying to complete quest (id: %u) but he has no right to do it!",
            _player->GetName(), _player->GetGUIDLow(), questId);
        return;
    }

    if (_player->CanRewardQuest(quest, reward, true, slot))
    {
        _player->RewardQuest(quest, reward, object, true, slot);

        switch (object->GetTypeId())
        { 
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            {
                //For AutoSubmition was added plr case there as it almost same exclude AI script cases.
                Creature *creatureQGiver = object->ToCreature();
                if (!creatureQGiver || !(sScriptMgr->OnQuestReward(_player, creatureQGiver, quest, reward)))
                {
                    if (creatureQGiver)
                        creatureQGiver->AI()->sQuestReward(_player, quest, reward);
                    // Send next quest
                    if (Quest const* nextQuest = _player->GetNextQuest(guid, quest))
                    {
                        if (nextQuest->IsAutoAccept() && _player->CanAddQuest(nextQuest, true) && _player->CanTakeQuest(nextQuest, true))
                        {
                            if (creatureQGiver)
                            {
                                sScriptMgr->OnQuestAccept(_player, creatureQGiver, nextQuest);
                                creatureQGiver->AI()->sQuestAccept(_player, nextQuest);
                            }

                            _player->AddQuest(nextQuest, object);
                            if (_player->CanCompleteQuest(nextQuest->GetQuestId()))
                                _player->CompleteQuest(nextQuest->GetQuestId());
                            _player->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, true);
                        }else
                            _player->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, false);
                    }
                    else
                    {
                        if (quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
                        {
                            uint32 nextQuestCh = quest->GetNextQuestInChain();
                            if (nextQuestCh)
                            {
                                _player->AddQuest(sObjectMgr->GetQuestTemplate(nextQuestCh), object);
                                if (_player->CanCompleteQuest(sObjectMgr->GetQuestTemplate(nextQuestCh)->GetQuestId()))
                                    _player->CompleteQuest(sObjectMgr->GetQuestTemplate(nextQuestCh)->GetQuestId());
                                _player->PlayerTalkClass->SendQuestGiverQuestDetails(sObjectMgr->GetQuestTemplate(nextQuestCh), guid, false);
                            }
                        }
                    }
                }
                break;
            }
        case TYPEID_GAMEOBJECT:
            if (!sScriptMgr->OnQuestReward(_player, ((GameObject*)object), quest, reward))
            {
                // Send next quest
                if (Quest const* nextQuest = _player->GetNextQuest(guid, quest))
                {
                    if (nextQuest->IsAutoAccept() && _player->CanAddQuest(nextQuest, true) && _player->CanTakeQuest(nextQuest, true))
                    {
                        _player->AddQuest(nextQuest, object);
                        if (_player->CanCompleteQuest(nextQuest->GetQuestId()))
                            _player->CompleteQuest(nextQuest->GetQuestId());
                    }

                    _player->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, true);
                }

                object->ToGameObject()->AI()->QuestReward(_player, quest, reward);
            }
            break;
        default:
            break;
        }
        // As quest complete send available quests. Need when questgiver from next quest chain staying near questtaker
        HandleQuestgiverStatusMultipleQuery(recvData);
        // AutoTake system
        _player->PrepareAreaQuest( _player->GetAreaId());
    }
    else 
        _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
}

void WorldSession::HandleQuestgiverRequestRewardOpcode(WorldPacket & recvData)
{
    uint32 questId;
    ObjectGuid guid;

    recvData >> questId;
    recvData.ReadGuidMask<5, 6, 1, 7, 0, 3, 2, 4>(guid);
    recvData.ReadGuidBytes<5, 1, 0, 4, 7, 3, 2, 6>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_REQUEST_REWARD npc = %u, quest = %u", uint32(GUID_LOPART(guid)), questId);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    if (!quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
    {
        Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT|TYPEMASK_GAMEOBJECT);
        if (!object || !object->hasInvolvedQuest(questId))
            return;

        // some kind of WPE protection
        if (!_player->CanInteractWithQuestGiver(object))
            return;
    }

    if (_player->CanCompleteQuest(questId))
        _player->CompleteQuest(questId);

    if (_player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        return;

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
        _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
}

void WorldSession::HandleQuestgiverCancel(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_CANCEL");

    _player->PlayerTalkClass->SendCloseGossip();
}

void WorldSession::HandleQuestLogSwapQuest(WorldPacket& recvData)
{
    uint8 slot1, slot2;
    recvData >> slot1 >> slot2;

    if (slot1 == slot2 || slot1 >= MAX_QUEST_LOG_SIZE || slot2 >= MAX_QUEST_LOG_SIZE)
        return;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTLOG_SWAP_QUEST slot 1 = %u, slot 2 = %u", slot1, slot2);

    GetPlayer()->SwapQuestSlot(slot1, slot2);
}

void WorldSession::HandleQuestLogRemoveQuest(WorldPacket& recvData)
{
    uint8 slot;
    recvData >> slot;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTLOG_REMOVE_QUEST slot = %u", slot);

    if (slot < MAX_QUEST_LOG_SIZE)
    {
        if (uint32 questId = _player->GetQuestSlotQuestId(slot))
        {
            if (!_player->TakeQuestSourceItem(questId, true))
                return;                                     // can't un-equip some items, reject quest cancel

            if (const Quest *quest = sObjectMgr->GetQuestTemplate(questId))
            {
                if (quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED))
                    _player->RemoveTimedQuest(questId);
            }

            _player->TakeQuestSourceItem(questId, true); // remove quest src item from player
            _player->RemoveActiveQuest(questId);
            _player->GetAchievementMgr().RemoveTimedAchievement(CRITERIA_TIMED_TYPE_QUEST, questId);

            TC_LOG_INFO("network", "Player %u abandoned quest %u", _player->GetGUIDLow(), questId);
        }

        _player->SetQuestSlot(slot, 0);

        _player->UpdateAchievementCriteria(CRITERIA_TYPE_QUEST_ABANDONED, 1);
    }
}

void WorldSession::HandleQuestConfirmAccept(WorldPacket& recvData)
{
    uint32 questId;
    recvData >> questId;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUEST_CONFIRM_ACCEPT questId = %u", questId);

    if (const Quest* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if (!quest->HasFlag(QUEST_FLAGS_PARTY_ACCEPT))
            return;

        Player* pOriginalPlayer = ObjectAccessor::FindPlayer(_player->GetDivider());

        if (!pOriginalPlayer)
            return;

        if (quest->IsRaidQuest())
        {
            if (!_player->IsInSameRaidWith(pOriginalPlayer))
                return;
        }
        else
        {
            if (!_player->IsInSameGroupWith(pOriginalPlayer))
                return;
        }

        if (_player->CanAddQuest(quest, true))
            _player->AddQuest(quest, NULL);                // NULL, this prevent DB script from duplicate running

        _player->SetDivider(0);
    }
}

void WorldSession::HandleQuestgiverCompleteQuest(WorldPacket& recvData)
{
    uint32 questId;
    ObjectGuid playerGuid;
    bool autoCompleteMode;      // 0 - standard complete quest mode with npc, 1 - auto-complete mode
    recvData >> questId;
    recvData.ReadGuidMask<0, 7, 5, 2, 4, 6, 3>(playerGuid);

    autoCompleteMode = recvData.ReadBit();

    recvData.ReadGuidMask<1>(playerGuid);
    recvData.ReadGuidBytes<2, 3, 1, 0, 5, 7, 6, 4>(playerGuid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_COMPLETE_QUEST npc = %u, questId = %u", uint32(GUID_LOPART(playerGuid)), questId);

    if (autoCompleteMode == 0)
    {
        Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, playerGuid, TYPEMASK_UNIT|TYPEMASK_GAMEOBJECT);
        if (!object || !object->hasInvolvedQuest(questId))
            return;

    // some kind of WPE protection
    if (!_player->CanInteractWithQuestGiver(object))
        return;
    }

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if (autoCompleteMode && !quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
        {
            TC_LOG_ERROR("network", "Possible hacking attempt: Player %s [playerGuid: %u] tried to complete questId [entry: %u] by auto-submit flag for quest witch not suport it.",
                _player->GetName(), _player->GetGUIDLow(), questId);
            return;
        }
        if (!_player->CanSeeStartQuest(quest) && _player->GetQuestStatus(questId) == QUEST_STATUS_NONE)
        {
            TC_LOG_ERROR("network", "Possible hacking attempt: Player %s [playerGuid: %u] tried to complete questId [entry: %u] without being in possession of the questId!",
                          _player->GetName(), _player->GetGUIDLow(), questId);
            return;
        }
        // TODO: need a virtual function
        if (_player->InBattleground())
            if (Battleground* bg = _player->GetBattleground())
                if (bg->GetTypeID() == BATTLEGROUND_AV)
                    ((BattlegroundAV*)bg)->HandleQuestComplete(questId, _player);

        if (_player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        {
            if (quest->IsRepeatable())
                _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, playerGuid, _player->CanCompleteRepeatableQuest(quest), false);
            else
                _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, playerGuid, _player->CanRewardQuest(quest, false), false);
        }
        else
        {
            if (quest->GetReqItemsCount())                  // some items required
                _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, playerGuid, _player->CanRewardQuest(quest, false), false);
            else                                            // no items required
                _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, playerGuid, !autoCompleteMode);
        }
    }
}

void WorldSession::HandleQuestgiverQuestAutoLaunch(WorldPacket& /*recvPacket*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_QUEST_AUTOLAUNCH");
}

void WorldSession::HandlePushQuestToParty(WorldPacket& recvPacket)
{
    uint32 questId;
    recvPacket >> questId;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_PUSHQUESTTOPARTY questId = %u", questId);

    if (_player->GetQuestStatus(questId) == QUEST_STATUS_NONE || _player->GetQuestStatus(questId) == QUEST_STATUS_REWARDED)
        return;

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if (Group* group = _player->GetGroup())
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* player = itr->getSource();

                if (!player || player == _player)         // skip self
                    continue;

                _player->SendPushToPartyResponse(player, QUEST_PARTY_MSG_SHARING_QUEST);

                if (!player->SatisfyQuestStatus(quest, false))
                {
                    _player->SendPushToPartyResponse(player, QUEST_PARTY_MSG_HAVE_QUEST);
                    continue;
                }

                if (player->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
                {
                    _player->SendPushToPartyResponse(player, QUEST_PARTY_MSG_FINISH_QUEST);
                    continue;
                }

                if (!player->CanTakeQuest(quest, false))
                {
                    _player->SendPushToPartyResponse(player, QUEST_PARTY_MSG_CANT_TAKE_QUEST);
                    continue;
                }

                if (!player->SatisfyQuestLog(false))
                {
                    _player->SendPushToPartyResponse(player, QUEST_PARTY_MSG_LOG_FULL);
                    continue;
                }

                if (player->GetDivider() != 0)
                {
                    _player->SendPushToPartyResponse(player, QUEST_PARTY_MSG_BUSY);
                    continue;
                }

                player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, _player->GetGUID(), true);
                player->SetDivider(_player->GetGUID());
            }
        }
    }
}

void WorldSession::HandleQuestPushResult(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    uint8 msg;

    recvPacket >> msg >> Unused<uint32>();
    recvPacket.ReadGuidMask<5, 0, 1, 2, 6, 4, 3, 7>(guid);
    recvPacket.ReadGuidBytes<5, 3, 0, 6, 7, 1, 2, 4>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUEST_PUSH_RESULT");

    if (_player->GetDivider() == 0)
        return;

    Player* player = ObjectAccessor::FindPlayer(_player->GetDivider());
    if (!player)
        return;

    WorldPacket data(SMSG_QUEST_PUSH_RESULT, (8+1));
    data.WriteGuidMask<7, 0, 4, 6, 1, 2, 3, 5>(guid);
    data.WriteGuidBytes<2, 4, 6>(guid);
    data << uint8(msg);
    data.WriteGuidBytes<0, 3, 7, 1, 5>(guid);
    player->GetSession()->SendPacket(&data);
    _player->SetDivider(0);
}

uint32 WorldSession::getDialogStatus(Player* player, Object* questgiver, uint32 defstatus)
{
    uint32 result = defstatus;

    QuestRelationBounds qr;
    QuestRelationBounds qir;

    switch (questgiver->GetTypeId())
    {
        case TYPEID_GAMEOBJECT:
        {
            qr  = sObjectMgr->GetGOQuestRelationBounds(questgiver->GetEntry());
            qir = sObjectMgr->GetGOQuestInvolvedRelationBounds(questgiver->GetEntry());
            break;
        }
        case TYPEID_UNIT:
        {
            qr  = sObjectMgr->GetCreatureQuestRelationBounds(questgiver->GetEntry());
            qir = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(questgiver->GetEntry());
            break;
        }
        default:
            //its imposible, but check ^)
            TC_LOG_ERROR("network", "Warning: GetDialogStatus called for unexpected type %u", questgiver->GetTypeId());
            return DIALOG_STATUS_NONE;
    }

    for (QuestRelations::const_iterator i = qir.first; i != qir.second; ++i)
    {
        uint32 result2 = 0;
        uint32 quest_id = i->second;
        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (!quest)
            continue;

        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_SHOW_MARK, quest->GetQuestId());
        if (!sConditionMgr->IsObjectMeetToConditions(player, conditions))
            continue;

        conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_ACCEPT, quest->GetQuestId());
        if (!sConditionMgr->IsObjectMeetToConditions(player, conditions))
            continue;

        QuestStatus status = player->GetQuestStatus(quest_id);
        if ((status == QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(quest_id)) ||
            (quest->IsAutoComplete() && player->CanTakeQuest(quest, false)))
        {
            if (quest->IsAutoComplete() && quest->IsRepeatable())
                result2 = DIALOG_STATUS_REWARD_REP;
            else
                result2 = DIALOG_STATUS_REWARD;
        }
        else if (status == QUEST_STATUS_INCOMPLETE)
            result2 = DIALOG_STATUS_INCOMPLETE;

        if (result2 > result)
            result = result2;
    }

    for (QuestRelations::const_iterator i = qr.first; i != qr.second; ++i)
    {
        uint32 result2 = 0;
        uint32 quest_id = i->second;
        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (!quest)
            continue;

        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_SHOW_MARK, quest->GetQuestId());
        if (!sConditionMgr->IsObjectMeetToConditions(player, conditions))
            continue;

        conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_ACCEPT, quest->GetQuestId());
        if (!sConditionMgr->IsObjectMeetToConditions(player, conditions))
            continue;

        QuestStatus status = player->GetQuestStatus(quest_id);
        if (status == QUEST_STATUS_NONE)
        {
            if (player->CanSeeStartQuest(quest))
            {
                if (player->SatisfyQuestLevel(quest, false))
                {
                    if (quest->IsAutoComplete() || (quest->IsRepeatable() && player->IsQuestRewarded(quest_id)))
                        result2 = DIALOG_STATUS_REWARD_REP;
                    else if (player->getLevel() <= ((player->GetQuestLevel(quest) == -1) ? player->getLevel() : player->GetQuestLevel(quest) + sWorld->getIntConfig(CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF)))
                    {
                        if (quest->HasFlag(QUEST_FLAGS_DAILY) || quest->HasFlag(QUEST_FLAGS_WEEKLY))
                            result2 = DIALOG_STATUS_AVAILABLE_REP;
                        else
                            result2 = DIALOG_STATUS_AVAILABLE;
                    }
                    else
                        result2 = DIALOG_STATUS_LOW_LEVEL_AVAILABLE;
                }
                else
                    result2 = DIALOG_STATUS_UNAVAILABLE;
            }
        }

        if (result2 > result)
            result = result2;
    }

    return result;
}

void WorldSession::HandleQuestgiverStatusMultipleQuery(WorldPacket& /*recvPacket*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_STATUS_MULTIPLE_QUERY");

    uint32 count = 0;

    WorldPacket data(SMSG_QUESTGIVER_STATUS_MULTIPLE);

    uint32 pos = data.bitwpos();
    data.WriteBits(0, 21);

    ByteBuffer buff;
    for (GuidUnorderedSet::const_iterator itr = _player->m_clientGUIDs.begin(); itr != _player->m_clientGUIDs.end(); ++itr)
    {
        uint32 questStatus = DIALOG_STATUS_NONE;
        uint32 defstatus = DIALOG_STATUS_NONE;

        if (IS_CRE_OR_VEH_OR_PET_GUID(*itr))
        {
            // need also pet quests case support
            Creature* questgiver = ObjectAccessor::GetCreatureOrPetOrVehicle(*GetPlayer(), *itr);
            if (!questgiver || questgiver->IsHostileTo(_player))
                continue;
            if (!questgiver->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
                continue;
            questStatus = sScriptMgr->GetDialogStatus(_player, questgiver);
            if (questStatus > 6)
                questStatus = getDialogStatus(_player, questgiver, defstatus);

            data.WriteGuidMask<6, 1, 2, 5, 0, 7, 3, 4>(questgiver->GetGUID());

            buff.WriteGuidBytes<4, 1, 7, 0, 2, 3>(questgiver->GetGUID());
            buff << uint32(questStatus);
            buff.WriteGuidBytes<5, 6>(questgiver->GetGUID());
            ++count;
        }
        else if (IS_GAMEOBJECT_GUID(*itr))
        {
            GameObject* questgiver = GetPlayer()->GetMap()->GetGameObject(*itr);
            if (!questgiver)
                continue;
            if (questgiver->GetGoType() != GAMEOBJECT_TYPE_QUESTGIVER)
                continue;
            questStatus = sScriptMgr->GetDialogStatus(_player, questgiver);
            if (questStatus > 6)
                questStatus = getDialogStatus(_player, questgiver, defstatus);

            data.WriteGuidMask<6, 1, 2, 5, 0, 7, 3, 4>(questgiver->GetGUID());

            buff.WriteGuidBytes<4, 1, 7, 0, 2, 3>(questgiver->GetGUID());
            buff << uint32(questStatus);
            buff.WriteGuidBytes<5, 6>(questgiver->GetGUID());
            ++count;
        }
    }

    data.FlushBits();
    if (!buff.empty())
        data.append(buff);

    data.PutBits(pos, count, 21);                       // write real count
    SendPacket(&data);
}

void WorldSession::HandleQueryQuestsCompleted(WorldPacket& /*recvData*/)
{
    size_t rew_count = _player->GetRewardedQuestCount();

    WorldPacket data(SMSG_QUERY_QUESTS_COMPLETED_RESPONSE, 4 + 4 * rew_count);
    data << uint32(rew_count);

    const RewardedQuestSet &rewQuests = _player->getRewardedQuests();
    for (RewardedQuestSet::const_iterator itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
        data << uint32(*itr);

    SendPacket(&data);
}

void WorldSession::HandleQuestNpcQuery(WorldPacket& recvData)
{
    uint32 count = recvData.ReadBits(22);
    QuestStarter* starterMap = sObjectMgr->GetCreatureQuestStarterMap();

    std::vector<uint32> quests;
    for (uint32 i = 0; i < count; ++i)
    {
        uint32 questId;
        recvData >> questId;

        Quest const* _quest = sObjectMgr->GetQuestTemplate(questId);
        if (!_quest)
        {
            TC_LOG_ERROR("server", "HandleQuestNpcQuery: Invalide questId %u for player %u", questId, GetPlayer()->GetGUIDLow());
            continue;
        }

        if (starterMap->find(questId) == starterMap->end())
            continue;

        quests.push_back(questId);
    }

    WorldPacket data(SMSG_QUEST_NPC_QUERY_RESPONSE, 3 + quests.size() * (4 + 3));
    data.WriteBits(quests.size(), 21);

    ByteBuffer buffer;
    for (uint32 i = 0; i < quests.size(); ++i)
    {
        QuestStarter::const_iterator itr = starterMap->find(quests[i]);
        data.WriteBits(itr->second.size(), 22);

        buffer << uint32(quests[i]);
        for (QuestObject::const_iterator obj = itr->second.begin(); obj != itr->second.end(); ++obj)
            buffer << uint32(*obj);
    }

    data.FlushBits();
    if (!buffer.empty())
        data.append(buffer);

    SendPacket(&data);
}
