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
#include "DBCEnums.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "BattlePetMgr.h"
#include "Player.h"

// CMSG_BATTLE_PET_SUMMON
void WorldSession::HandleBattlePetSummon(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 0, 1, 5, 3, 4, 7, 2>(guid);
    recvData.ReadGuidBytes<2, 5, 3, 7, 1, 0, 6, 4>(guid);

    // find pet
    PetJournalInfo* petInfo = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!petInfo || petInfo->GetState() == STATE_DELETED)
        return;

    uint32 spellId = petInfo->GetSummonSpell();

    if (spellId && !_player->HasActiveSpell(spellId))
        return;

    if (!spellId)
        spellId = BATTLE_PET_SUMMON_SPELL;

    if (_player->m_SummonSlot[SUMMON_SLOT_MINIPET])
    {
        Creature* oldSummon = _player->GetMap()->GetCreature(_player->m_SummonSlot[SUMMON_SLOT_MINIPET]);

        if (oldSummon && oldSummon->isSummon() && oldSummon->GetUInt64Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) == guid)
            oldSummon->ToTempSummon()->UnSummon();
        else
        {
            _player->SetUInt64Value(PLAYER_FIELD_SUMMONED_BATTLE_PET_GUID, guid);
            _player->CastSpell(_player, spellId, true);
        }
    }
    else
    {
        _player->SetUInt64Value(PLAYER_FIELD_SUMMONED_BATTLE_PET_GUID, guid);
        _player->CastSpell(_player, spellId, true);
    }
}

// CMSG_BATTLE_PET_NAME_QUERY
void WorldSession::HandleBattlePetNameQuery(WorldPacket& recvData)
{
    ObjectGuid creatureGuid, battlepetGuid;
    recvData.ReadGuidMask<7, 6>(battlepetGuid);
    recvData.ReadGuidMask<6>(creatureGuid);
    recvData.ReadGuidMask<1>(battlepetGuid);
    recvData.ReadGuidMask<4, 5>(creatureGuid);
    recvData.ReadGuidMask<2, 5, 0>(battlepetGuid);
    recvData.ReadGuidMask<0, 3, 1, 2>(creatureGuid);
    recvData.ReadGuidMask<4, 3>(battlepetGuid);
    recvData.ReadGuidMask<7>(creatureGuid);

    recvData.ReadGuidBytes<5>(battlepetGuid);
    recvData.ReadGuidBytes<7>(creatureGuid);
    recvData.ReadGuidBytes<2>(battlepetGuid);
    recvData.ReadGuidBytes<3>(creatureGuid);
    recvData.ReadGuidBytes<1, 4>(battlepetGuid);
    recvData.ReadGuidBytes<0>(creatureGuid);
    recvData.ReadGuidBytes<0>(battlepetGuid);
    recvData.ReadGuidBytes<1, 6, 4, 2, 5>(creatureGuid);
    recvData.ReadGuidBytes<7, 3, 6>(battlepetGuid);

    if (Creature* summon = _player->GetMap()->GetCreature(creatureGuid))
    {
        // check battlepet guid
        if (summon->GetUInt64Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) != battlepetGuid)
            return;

        if (Player * owner = summon->GetCharmerOrOwnerPlayerOrPlayerItself())
        {
            if (PetJournalInfo* petInfo = owner->GetBattlePetMgr()->GetPetInfoByPetGUID(battlepetGuid))
            {
                if (petInfo->GetState() == STATE_DELETED)
                    return;

                bool hasCustomName = petInfo->GetCustomName() == "" ? false : true;
                WorldPacket data(SMSG_BATTLE_PET_NAME_QUERY_RESPONSE);
                // creature entry
                data << uint32(petInfo->GetCreatureEntry());
                // timestamp for custom name cache
                data << uint32(hasCustomName ? summon->GetUInt32Value(UNIT_FIELD_BATTLE_PET_COMPANION_NAME_TIMESTAMP) : 0);
                // battlepet guid
                data << uint64(battlepetGuid);
                // need store declined names at rename
                bool hasDeclinedNames = false;
                data.WriteBit(hasCustomName);
                if (hasCustomName)
                {
                    data.WriteBits(petInfo->GetCustomName().length(), 8);
                    data.WriteBit(hasDeclinedNames);

                    for (int i = 0; i < 5; i++)
                        data.WriteBits(0, 7);

                    data.WriteString(petInfo->GetCustomName());

                    /*for (int i = 0; i < 5; i++)
                    data.WriteString(pet->declinedNames[i]);*/
                }

                SendPacket(&data);
            }
        }
    }
}

void WorldSession::HandleBattlePetModifyName(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<1, 6>(guid);
    uint8 len = recvData.ReadBits(7);
    recvData.ReadGuidMask<3, 5>(guid);
    bool hasDeclined = recvData.ReadBit();
    recvData.ReadGuidMask<7, 4, 0, 2>(guid);
    uint8 len1[5];
    if (hasDeclined)
    {
        for (uint8 i = 0; i < 5; ++i)
            len1[i] = recvData.ReadBits(7);
    }
    recvData.ReadGuidBytes<7, 4, 3, 5, 1>(guid);
    std::string customName = recvData.ReadString(len);
    recvData.ReadGuidBytes<0, 2, 6>(guid);
    std::string declinedNames[5];
    if (hasDeclined)
    {
        for (uint8 i = 0; i < 5; ++i)
            declinedNames[i] = recvData.ReadString(len1[i]);
    }

    // find pet
    PetJournalInfo* petInfo = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!petInfo || petInfo->GetState() == STATE_DELETED)
        return;

    // set custom name
    petInfo->SetCustomName(customName);
}

void WorldSession::HandleBattlePetSetFlags(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 flags;
    recvData >> flags;                                       // Flags
    recvData.ReadGuidMask<2, 6, 1, 7, 0>(guid);
    uint32 active = recvData.ReadBits(2);                    // ControlTypes?
    recvData.ReadGuidMask<3, 4, 5>(guid);
    recvData.ReadGuidBytes<3, 5, 2, 1, 0, 6, 7, 4>(guid);

    // find pet
    PetJournalInfo* petInfo = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!petInfo || petInfo->GetState() == STATE_DELETED)
        return;

    if (!active)
        petInfo->RemoveFlag(flags);
    else
        petInfo->SetFlag(flags);
}

void WorldSession::HandleCageBattlePet(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 5, 0, 3, 4, 7, 2, 1>(guid);
    recvData.ReadGuidBytes<4, 1, 5, 3, 0, 6, 7, 2>(guid);

    // unsummon pet if active
    if (_player->m_SummonSlot[SUMMON_SLOT_MINIPET])
    {
        Creature* oldSummon = _player->GetMap()->GetCreature(_player->m_SummonSlot[SUMMON_SLOT_MINIPET]);
        if (oldSummon && oldSummon->isSummon() && oldSummon->GetUInt64Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) == guid)
            oldSummon->ToTempSummon()->UnSummon();
    }

    if (PetJournalInfo * petInfo = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid))
    {
        // some validate
        BattlePetSpeciesEntry const* bp = _player->GetBattlePetMgr()->GetBattlePetSpeciesEntry(petInfo->GetCreatureEntry());

        if (!bp || bp->flags & SPECIES_FLAG_CANT_TRADE)
            return;

        if (petInfo->GetState() == STATE_DELETED)
            return;

        // at first - all operations with check free space
        uint32 itemId = ITEM_BATTLE_PET_CAGE_ID; // Pet Cage
        uint32 count = 1;
        uint32 _noSpaceForCount = 0;
        ItemPosCountVec dest;
        InventoryResult msg = _player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &_noSpaceForCount);
        if (msg != EQUIP_ERR_OK)
            count -= _noSpaceForCount;

        if (count == 0 || dest.empty())
            return;

        // at second - pack item dynamic data
        uint32 dynData = 0;
        dynData |= petInfo->GetBreedID();
        dynData |= uint32(petInfo->GetQuality() << 24);

        // at third - send item
        Item* item = _player->StoreNewItem(dest, itemId, true, 0);

        if (!item)                                               // prevent crash
            return;

        item->SetBattlePet(petInfo->GetSpeciesID(), dynData, petInfo->GetLevel());
        _player->SendNewItem(item, 1, false, true, petInfo);

        // at fourth - unlearn spell - TODO: fix it because more than one spell/battle pet of same type
        _player->removeSpell(petInfo->GetSummonSpell());

        // delete from journal
        _player->GetBattlePetMgr()->DeletePetByPetGUID(guid);

        // send visual to client
        WorldPacket data(SMSG_BATTLE_PET_DELETED);
        data.WriteGuidMask<5, 6, 4, 0, 1, 2, 7, 4>(guid);
        data.WriteGuidBytes<1, 0, 6, 5, 2, 4, 3, 7>(guid);

        // send packet twice? (sniff data)
        SendPacket(&data);
        SendPacket(&data);

        _player->SavePlayerBattlePets();
    }
}

void WorldSession::HandleBattlePetSetSlot(WorldPacket& recvData)
{
    uint8 slotID;
    recvData >> slotID;

    ObjectGuid guid;
    recvData.ReadGuidMask<3, 6, 2, 1, 0, 5, 7, 4>(guid);
    recvData.ReadGuidBytes<1, 5, 3, 0, 4, 7, 2, 6>(guid);

    if (slotID >= MAX_ACTIVE_BATTLE_PETS)
        return;

    // find dest slot
    PetBattleSlot * slot = _player->GetBattlePetMgr()->GetPetBattleSlot(slotID);
    if (!slot)
        return;

    // find current pet
    PetJournalInfo* petInfo = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!petInfo || petInfo->GetState() == STATE_DELETED)
        return;

    // special handle switch slots
    if (!slot->IsEmpty())
    {
        for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
            if (PetBattleSlot* sourceSlot = _player->GetBattlePetMgr()->GetPetBattleSlot(i))
                if (sourceSlot->GetPet() == guid)
                    sourceSlot->SetPet(slot->GetPet());
    }

    slot->SetPet(guid);

    _player->SavePlayerBattlePetSlots();
}

void WorldSession::HandlePetBattleRequestWild(WorldPacket& recvData)
{
    float playerX, playerY, playerZ, playerOrient;
    float petAllyX, petEnemyX;
    float petAllyY, petEnemyY;
    float petAllyZ, petEnemyZ;
    ObjectGuid guid;
    uint32 locationResult;

    recvData >> playerX;
    recvData >> playerZ;
    recvData >> playerY;

    for (int i = 0; i < 2; ++i)
    {
        if (i == 0)
        {
            recvData >> petAllyY;
            recvData >> petAllyZ;
            recvData >> petAllyX;
        }
        else
        {
            recvData >> petEnemyY;
            recvData >> petEnemyZ;
            recvData >> petEnemyX;
        }
    }

    recvData.ReadGuidMask<6>(guid);
    bool hasFacing = recvData.ReadBit();
    recvData.ReadGuidMask<3>(guid);
    bool hasLocationRes = recvData.ReadBit();
    recvData.ReadGuidMask<7, 1, 0, 5, 4, 2>(guid);

    recvData.ReadGuidBytes<1, 5, 0, 2, 6, 4, 3, 7>(guid);

    if (!hasLocationRes)
        recvData >> locationResult;

    if (!hasFacing)
        recvData >> playerOrient;

    // some check before init battle
    uint8 reason = 0;
    if (_player->GetBattlePetMgr()->AllSlotsEmpty())
        reason = 16;
    else if (_player->GetBattlePetMgr()->AllSlotsDead())
        reason = 15;

    if (reason)
    {
        _player->GetBattlePetMgr()->SendPetBattleRequestFailed(reason);
        return;
    }

    WorldPacket data(SMSG_PET_BATTLE_FINALIZE_LOCATION);

    data << playerY;

    for (int i = 0; i < 2; ++i)
    {
        if (i == 0)
        {
            data << petAllyY;
            data << petAllyX;
            data << petAllyZ;
        }
        else
        {
            data << petEnemyY;
            data << petEnemyX;
            data << petEnemyZ;
        }
    }

    data << playerZ;
    data << playerX;

    data.WriteBit(0);
    data.WriteBit(0);

    locationResult = sWorld->getBoolConfig(CONFIG_PET_BATTLES_ENABLED) ? 21 : 0;
    data << _player->GetOrientation();
    data << uint32(locationResult);
    SendPacket(&data);

    // send full update and start pet battle
    if (sWorld->getBoolConfig(CONFIG_PET_BATTLES_ENABLED))
        _player->GetBattlePetMgr()->CreateWildBattle(_player, guid);
}

void WorldSession::HandlePetBattleInputFirstPet(WorldPacket& recvData)
{
    uint8 firstPetID;
    recvData >> firstPetID;

    PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild();

    if (!petBattle)
        return;

    if (!firstPetID)
    {
        if (!petBattle->FirstRoundHandler(firstPetID, 3))
        {
            petBattle->FinishPetBattle(true);
            // error response
        }
    }
    // replace player pet if previous are DEAD!
    else
        petBattle->ForceReplacePetHandler(petBattle->GetCurrentRoundID(), firstPetID, TEAM_ALLY);
}

void WorldSession::HandlePetBattleRequestUpdate(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 2, 3, 7, 0, 4>(guid);
    bool cancelled = recvData.ReadBit();                                       // Cancelled
    recvData.ReadGuidMask<5, 1>(guid);
    recvData.ReadGuidBytes<3, 5, 6, 7, 1, 0, 2, 4>(guid);
}

void WorldSession::HandlePetBattleInput(WorldPacket& recvData)
{
    bool bit = recvData.ReadBit();
    bool bit1 = recvData.ReadBit();
    bool bit2 = recvData.ReadBit();
    bool ignoreAbandonPenalty = recvData.ReadBit();
    bool bit4 = recvData.ReadBit();
    bool bit5 = recvData.ReadBit();
    bool bit6 = recvData.ReadBit();

    uint32 abilityID = 0;
    uint32 roundID = 0;
    uint8 moveType = 0;
    int8 newFrontPet = -1;
    uint8 battleInterrupted = 0;

    if (!bit5)
        recvData >> battleInterrupted;
    if (!bit6)
        recvData >> roundID;
    if (!bit1)
        recvData >> moveType;
    if (!bit)
        recvData >> abilityID;
    if (!bit2)
        recvData.read_skip<uint8>(); // debugFlags
    if (!bit4)
        recvData >> newFrontPet;

    PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild();

    if (!petBattle)
        return;

    // UseAbility
    if (abilityID && moveType == 1)
    {
        if (!petBattle->UseAbilityHandler(abilityID, roundID))
        {
            petBattle->FinishPetBattle(true);
            // error response
            return;
        }
    }
    // SkipTurn / SwapPet
    else if (moveType == 2 && newFrontPet != -1)
    {
        if (!petBattle->SwapPetHandler(newFrontPet, roundID))
        {
            petBattle->FinishPetBattle(true);
            // error response
            return;
        }
    }
    // TrapPet
    else if (moveType == 3)
    {
        if (!petBattle->UseTrapHandler(roundID))
        {
            petBattle->FinishPetBattle(true);
            // error response
            return;
        }
    }
    // Forfeit - handle in QuitNotify

    // FinalRound
    if (petBattle->NextRoundFinal() && moveType != 4)
    {
        if (!petBattle->FinalRoundHandler(false))
        {
            petBattle->FinishPetBattle(true);
            // error response
            return;
        }
    }
}

void WorldSession::HandlePetBattleFinalNotify(WorldPacket& recvData)
{
    if (PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild())
        petBattle->FinishPetBattle();
}

void WorldSession::HandlePetBattleQuitNotify(WorldPacket& recvData)
{
    if (PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild())
    {
        petBattle->SetAbandoned(true);
        petBattle->SetWinner(TEAM_ENEMY);

        if (!petBattle->FinalRoundHandler(true))
        {
            petBattle->FinishPetBattle();
            // error response
        }
    }
}

void WorldSession::HandleBattlePetDelete(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<4, 3, 5, 7, 2, 1, 6, 0>(guid);
    recvData.ReadGuidBytes<6, 3, 5, 0, 4, 7, 1, 2>(guid);

    // find current pet
    PetJournalInfo* petInfo = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!petInfo || petInfo->GetState() == STATE_DELETED)
        return;

    if (petInfo->GetFlags() & 0xC)
        return;

    if (_player->GetBattlePetMgr()->PetIsSlotted(guid))
        return;

    _player->GetBattlePetMgr()->DeletePetByPetGUID(guid);
    _player->SavePlayerBattlePets();
}