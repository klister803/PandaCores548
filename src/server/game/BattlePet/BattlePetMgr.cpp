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
#include "GuildMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiersImpl.h"
#include "Guild.h"
#include "Language.h"
#include "Player.h"
#include "SpellMgr.h"
#include "DisableMgr.h"
#include "ScriptMgr.h"
#include "MapManager.h"
#include "Battleground.h"
#include "BattlegroundAB.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Group.h"
#include "Item.h"
#include "BattlePetMgr.h"

BattlePetMgr::BattlePetMgr(Player* owner) : m_player(owner), m_petBattleWild(NULL)
{
    m_PetJournal.clear();
    memset(m_battleSlots, 0, sizeof(PetBattleSlot*)*MAX_ACTIVE_PETS);
}

void BattlePetMgr::AddPetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName, int16 breedID, uint8 state)
{
    m_PetJournal[guid] = new PetInfo(speciesID, creatureEntry, level, display, power, speed, health, maxHealth, quality, xp, flags, spellID, customName, breedID, state);
}

void BattlePetMgr::AddPetBattleSlot(uint64 guid, uint8 slotID)
{
    m_battleSlots[slotID] = new PetBattleSlot(guid);
}

void BattlePetMgr::BuildPetJournal(WorldPacket *data)
{
    ObjectGuid placeholderPet;
    uint32 count = 0;
    data->Initialize(SMSG_BATTLE_PET_JOURNAL, 400);
    uint32 bit_pos = data->bitwpos();
    data->WriteBits(count, 19);

    for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
    {
        // prevent loading deleted pet
        if (pet->second->internalState == STATE_DELETED)
            continue;

        ObjectGuid guid = pet->first;
        uint8 len = pet->second->customName == "" ? 0 : pet->second->customName.length();

        data->WriteBit(!pet->second->breedID);    // hasBreed, inverse
        data->WriteGuidMask<1, 5, 3>(guid);
        data->WriteBit(0);                        // hasOwnerGuidInfo
        data->WriteBit(!pet->second->quality);    // hasQuality, inverse
        data->WriteGuidMask<6, 7>(guid);
        data->WriteBit(!pet->second->flags);      // hasFlags, inverse
        data->WriteGuidMask<0, 4>(guid);
        data->WriteBits(len, 7);                  // custom name length
        data->WriteGuidMask<2>(guid);
        data->WriteBit(1);                        // hasUnk (bool noRename?), inverse
    }

    data->WriteBits(MAX_ACTIVE_PETS, 25);

    for (uint32 i = 0; i < MAX_ACTIVE_PETS; ++i)
    {
        PetBattleSlot * slot = GetPetBattleSlot(i);

        // check slot locked conditions
        bool locked = true;

        switch (i)
        {
            case 0: locked = !m_player->HasSpell(119467); break;
            case 1: locked = !m_player->GetAchievementMgr().HasAchieved(7433); break;
            case 2: locked = !m_player->GetAchievementMgr().HasAchieved(6566); break;
            default: break;
        }

        data->WriteBit(!i);                                          // hasSlotIndex
        data->WriteBit(1);                                           // hasCollarID, inverse
        data->WriteBit(slot ? !slot->IsEmpty() : 1);                 // empty slot, inverse
        data->WriteGuidMask<7, 1, 3, 2, 5, 0, 4, 6>(slot ? slot->petGUID : 0);  // pet guid in slot
        data->WriteBit(locked);                    // locked slot
    }

    data->WriteBit(1);                      // !hasJournalLock

    for (uint32 i = 0; i < MAX_ACTIVE_PETS; ++i)
    {
        PetBattleSlot * slot = GetPetBattleSlot(i);

        //if (!bit)
            //*data << uint32(0);
        data->WriteGuidBytes<3, 7, 5, 1, 4, 0, 6, 2>(slot ? slot->petGUID : 0);
        if (i)
            *data << uint8(i);             // SlotIndex
    }

    for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
    {
        // prevent loading deleted pet
        if (pet->second->internalState == STATE_DELETED)
            continue;

        ObjectGuid guid = pet->first;
        uint8 len = pet->second->customName == "" ? 0 : pet->second->customName.length();

        if (pet->second->breedID)
            *data << uint16(pet->second->breedID);            // breedID
        *data << uint32(pet->second->speciesID);              // speciesID
        *data << uint32(pet->second->speed);                  // speed
        data->WriteGuidBytes<1, 6, 4>(guid);
        *data << uint32(pet->second->displayID);
        *data << uint32(pet->second->maxHealth);              // max health
        *data << uint32(pet->second->power);                  // power
        data->WriteGuidBytes<2>(guid);
        *data << uint32(pet->second->creatureEntry);          // Creature ID
        *data << uint16(pet->second->level);                  // level
        if (len > 0)
            data->WriteString(pet->second->customName);       // custom name
        *data << uint32(pet->second->health);                 // health
        *data << uint16(pet->second->xp);                     // xp
        if (pet->second->quality)
            *data << uint8(pet->second->quality);             // quality
        data->WriteGuidBytes<3, 7, 0>(guid);
        if (pet->second->flags)
            *data << uint16(pet->second->flags);              // flags
        data->WriteGuidBytes<5>(guid);

        count++;
    }

    *data << uint16(0);         // trapLevel
    data->PutBits<uint8>(bit_pos, count, 19);
}

void WorldSession::HandleBattlePetSummon(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 0, 1, 5, 3, 4, 7, 2>(guid);
    recvData.ReadGuidBytes<2, 5, 3, 7, 1, 0, 6, 4>(guid);

    // find pet
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->internalState == STATE_DELETED)
        return;

    uint32 spellId = pet->summonSpellID;

    if (!_player->HasActiveSpell(spellId))
        return;

    if (_player->m_SummonSlot[SUMMON_SLOT_MINIPET])
    {
        Creature* oldSummon = _player->GetMap()->GetCreature(_player->m_SummonSlot[SUMMON_SLOT_MINIPET]);
        if (oldSummon && oldSummon->isSummon() && oldSummon->GetUInt64Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) == guid)
            oldSummon->ToTempSummon()->UnSummon();
        else
            _player->CastSpell(_player, spellId, true);
    }
    else
        _player->CastSpell(_player, spellId, true);
}

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
            if (PetInfo* pet = owner->GetBattlePetMgr()->GetPetInfoByPetGUID(battlepetGuid))
            {
                if (pet->internalState == STATE_DELETED)
                    return;

                bool hasCustomName = pet->customName == "" ? false : true;
                WorldPacket data(SMSG_BATTLE_PET_NAME_QUERY_RESPONSE);
                // creature entry
                data << uint32(pet->creatureEntry);
                // timestamp for custom name cache
                data << uint32(hasCustomName ? summon->GetUInt32Value(UNIT_FIELD_BATTLE_PET_COMPANION_NAME_TIMESTAMP) : 0);
                // battlepet guid
                data << uint64(battlepetGuid);
                // need store declined names at rename
                bool hasDeclinedNames = false;
                data.WriteBit(hasCustomName);
                if (hasCustomName)
                {
                    data.WriteBits(pet->customName.length(), 8);
                    data.WriteBit(hasDeclinedNames);

                    for (int i = 0; i < 5; i++)
                        data.WriteBits(0, 7);

                    data.WriteString(pet->customName);

                    /*for (int i = 0; i < 5; i++)
                        data.WriteString(pet->declinedNames[i]);*/
                }

                SendPacket(&data);
            }
        }
    }
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

    if (PetInfo * pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid))
    {
        // some validate
        BattlePetSpeciesEntry const* bp = _player->GetBattlePetMgr()->GetBattlePetSpeciesEntry(pet->creatureEntry);

        if (!bp || bp->flags & SPECIES_FLAG_CANT_TRADE)
            return;

        if (pet->internalState == STATE_DELETED)
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

        // at second - create item dynamic data
        uint32 dynData = 0;
        dynData |= pet->breedID;
        dynData |= uint32(pet->quality << 24);

        // at third - send item
        Item* item = _player->StoreNewItem(dest, itemId, true, 0);

        if (!item)                                               // prevent crash
            return;

        item->SetBattlePet(pet->speciesID, dynData, pet->level);
        _player->SendNewItem(item, pet, 1, false, true);

        // at fourth - unlearn spell - TODO: fix it because more than one spell/battle pet of same type
        _player->removeSpell(pet->summonSpellID);

        // delete from journal
        _player->GetBattlePetMgr()->DeletePetByPetGUID(guid);

        // send visual to client
        WorldPacket data(SMSG_BATTLE_PET_DELETED);
        data.WriteGuidMask<5, 6, 4, 0, 1, 2, 7, 4>(guid);
        data.WriteGuidBytes<1, 0, 6, 5, 2, 4, 3, 7>(guid);

        // send packet twice? (sniff data)
        SendPacket(&data);
        SendPacket(&data);
    }
}

void WorldSession::HandleBattlePetSetSlot(WorldPacket& recvData)
{
    uint8 slotID;
    recvData >> slotID;

    ObjectGuid guid;
    recvData.ReadGuidMask<3, 6, 2, 1, 0, 5, 7, 4>(guid);
    recvData.ReadGuidBytes<1, 5, 3, 0, 4, 7, 2, 6>(guid);

    if (slotID >= MAX_ACTIVE_PETS)
        return;

    // find slot
    PetBattleSlot * slot = _player->GetBattlePetMgr()->GetPetBattleSlot(slotID);

    if (!slot)
        return;

    // find pet
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->internalState == STATE_DELETED)
        return;

    slot->SetPet(guid);
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

    locationResult = 21;
    data << _player->GetOrientation();
    data << uint32(locationResult);
    SendPacket(&data);

    // send full update and start pet battle
    _player->GetBattlePetMgr()->InitWildBattle(_player, guid);
}

void WorldSession::HandlePetBattleInputFirstPet(WorldPacket& recvData)
{
    uint8 firstPetID;
    recvData >> firstPetID;

    if (PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild())
        petBattle->FirstRound();
}

void WorldSession::HandlePetBattleRequestUpdate(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 2, 3, 7, 0, 4>(guid);
    recvData.ReadBit();                                       // unk
    recvData.ReadGuidMask<5, 1>(guid);
    recvData.ReadGuidBytes<3, 5, 6, 7, 1, 0, 2, 4>(guid);

    // send full update? wrong packet?
    //_player->GetBattlePetMgr()->InitWildBattle(_player, guid);
}

void WorldSession::HandlePetBattleInput(WorldPacket& recvData)
{
    bool bit = recvData.ReadBit();
    bool bit1 = recvData.ReadBit();
    bool bit2 = recvData.ReadBit();
    bool bit3 = recvData.ReadBit();
    bool bit4 = recvData.ReadBit();
    bool bit5 = recvData.ReadBit();
    bool bit6 = recvData.ReadBit();

    uint32 abilityID = 0;
    uint32 roundID = 0;
    uint8 endGame = 0;

    if (!bit5)
        recvData.read_skip<uint8>();
    if (!bit6)
        recvData >> roundID;
    if (!bit1)
        recvData >> endGame;
    if (!bit)
        recvData >> abilityID;
    if (!bit2)
        recvData.read_skip<uint8>();
    if (!bit4)
        recvData.read_skip<uint8>();

    // skip other action - trap, forfeit, skip turn, etc....
    if (!abilityID && endGame != 4)
        return;

    PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild();

    if (!petBattle)
        return;

    int8 state = -1;
    if (endGame == 4)
    {
        petBattle->FinishPetBattle();
        _player->GetBattlePetMgr()->ClosePetBattle();
        return;
    }
    else
        petBattle->CalculateRoundData(state, roundID);

    if (state == -1)
        return;

    petBattle->RoundResults();

    if (state)
        petBattle->FinalRound();
}

void BattlePetMgr::ClosePetBattle()
{
    PetInfo* allyPet = GetPetBattleWild()->GetAllyPet();

    if (allyPet)
    {
        uint16 level = allyPet->level;
        uint32 health = allyPet->health;
        uint16 xp = allyPet->xp;

        if (PetBattleSlot * slot = GetPetBattleSlot(0))
        {
            if (PetInfo * pet = GetPetInfoByPetGUID(slot->GetPet()))
            {
                pet->level = level;
                pet->health = health;
                pet->xp = xp;

                pet->SetInternalState(STATE_UPDATED);
                SendUpdatePets();
            }
        }
    }

    delete m_petBattleWild;
    m_petBattleWild = NULL;
}

// packet updates pet stats after finish battle and other actions (renaming?)
void BattlePetMgr::SendUpdatePets(bool added)
{
    uint32 count = 0;
    WorldPacket data(SMSG_BATTLE_PET_UPDATES);
    data.WriteBit(added);
    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 19);                              // placeholder, count of update pets
    for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
    {
        if (pet->second->internalState != STATE_UPDATED || pet->second->internalState == STATE_DELETED)
            continue;

        ObjectGuid guid = pet->first;
        uint8 len = pet->second->customName == "" ? 0 : pet->second->customName.length();

        data.WriteBits(len, 7);                         // custom name length
        data.WriteBit(!pet->second->flags);             // !hasFlags
        data.WriteBit(1);                               // !hasUnk
        data.WriteGuidMask<2>(guid);
        data.WriteBit(!pet->second->breedID);           // !hasBreed
        data.WriteBit(!pet->second->quality);           // !hasQuality
        data.WriteGuidMask<1, 6, 3, 7, 0, 4, 5>(guid);
        data.WriteBit(0);                               // hasGuid

        /*if (hasGuid)
        {
            data.WriteGuidMask<7, 0, 6, 2, 1, 3, 5, 4>(petGuid2);
        }*/
    }

    for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
    {
        if (pet->second->internalState != STATE_UPDATED || pet->second->internalState == STATE_DELETED)
            continue;

        ObjectGuid guid = pet->first;
        uint8 len = pet->second->customName == "" ? 0 : pet->second->customName.length();

        /*if (hasGuid)
        {
            data << uint32(0); // unk
            data.WriteGuidBytes<0, 1, 2, 3, 4, 5, 7, 6>(petGuid2);
            data << uint32(0); // unk1
        }*/

        data << uint16(pet->second->level);                  // level
        data << uint32(pet->second->maxHealth);              // total health
        if (pet->second->flags)
            data << uint16(pet->second->flags);              // flags
        if (pet->second->quality)
            data << uint8(pet->second->quality);             // quality
        data.WriteGuidBytes<3>(guid);
        data << uint32(pet->second->creatureEntry);          // creature ID
        data << uint32(pet->second->speed);                  // speed
        if (pet->second->breedID)
            data << uint16(pet->second->breedID);            // breed ID

        data << uint32(pet->second->health);                 // remaining health
        data << uint32(pet->second->displayID);              // display ID
        if (len > 0)
            data.WriteString(pet->second->customName);       // custom name
        data.WriteGuidBytes<5, 4, 7>(guid);
        data << uint32(pet->second->speciesID);              // species ID
        data.WriteGuidBytes<2, 6>(guid);
        data << uint16(pet->second->xp);                     // experience
        data.WriteGuidBytes<0, 1>(guid);
        data << uint32(pet->second->power);                  // power

        count++;
        pet->second->SetInternalState(STATE_NORMAL);
    }

    data.PutBits<uint8>(bit_pos, count, 19);
    m_player->GetSession()->SendPacket(&data);
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
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->internalState == STATE_DELETED)
        return;

    // set custom name
    pet->SetCustomName(customName);
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
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->internalState == STATE_DELETED)
        return;

    if (!active)
        pet->RemoveFlag(flags);
    else
        pet->SetFlag(flags);
}

BattlePetStatAccumulator* BattlePetMgr::InitStateValuesFromDB(uint32 speciesID, uint16 breedID)
{
    BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(0, 0, 0);

    for (uint32 i = 0; i < sBattlePetSpeciesStateStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesStateEntry const* entry = sBattlePetSpeciesStateStore.LookupEntry(i);

        if (!entry)
            continue;

        if (entry->speciesID == speciesID)
            accumulator->Accumulate(entry->stateID, entry->stateModifier);
    }

    for (uint32 i = 0; i < sBattlePetBreedStateStore.GetNumRows(); ++i)
    {
        BattlePetBreedStateEntry const* entry1 = sBattlePetBreedStateStore.LookupEntry(i);

        if (!entry1)
            continue;

        if (entry1->breedID == breedID)
            accumulator->Accumulate(entry1->stateID, entry1->stateModifier);
    }

    return accumulator;
}

void BattlePetMgr::InitWildBattle(Player* initiator, ObjectGuid wildCreatureGuid)
{
    m_petBattleWild = new PetBattleWild(initiator);
    m_petBattleWild->Prepare(wildCreatureGuid);
}

// test functions
uint32 BattlePetMgr::GetAbilityID(uint32 speciesID, uint8 abilityIndex)
{
    for (uint32 i = 0; i < sBattlePetSpeciesXAbilityStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesXAbilityEntry const* xEntry = sBattlePetSpeciesXAbilityStore.LookupEntry(i);

        if (!xEntry)
            continue;

        if (xEntry->speciesID == speciesID && xEntry->rank == abilityIndex)
        {
            if (xEntry->requiredLevel > 2)
                continue;
            else
                return xEntry->abilityID;
        }
    }        

    return 0;
}

uint32 BattlePetMgr::GetEffectIDByAbilityID(uint32 abilityID)
{
    // get turn data entry
    for (uint32 i = 0; i < sBattlePetAbilityTurnStore.GetNumRows(); ++i)
    {
        BattlePetAbilityTurnEntry const* tEntry = sBattlePetAbilityTurnStore.LookupEntry(i);

        if (!tEntry)
            continue;

        if (tEntry->AbilityID == abilityID && tEntry->turnIndex == 1)
        {
            // get effect data
            for (uint32 j = 0; j < sBattlePetAbilityEffectStore.GetNumRows(); ++j)
            {
                BattlePetAbilityEffectEntry const* eEntry = sBattlePetAbilityEffectStore.LookupEntry(j);

                if (!eEntry)
                    continue;

                if (eEntry->TurnEntryID == tEntry->ID)
                    return eEntry->ID;
            }
        }
    }  

    return 0;
}

uint32 BattlePetMgr::GetBasePoints(uint32 abilityID, uint32 turnIndex, uint32 effectIdx)
{
    // get turn data entry
    for (uint32 i = 0; i < sBattlePetAbilityTurnStore.GetNumRows(); ++i)
    {
        BattlePetAbilityTurnEntry const* tEntry = sBattlePetAbilityTurnStore.LookupEntry(i);

        if (!tEntry)
            continue;

        if (tEntry->AbilityID == abilityID && tEntry->turnIndex == turnIndex)
        {
            // get effect data
            for (uint32 j = 0; j < sBattlePetAbilityEffectStore.GetNumRows(); ++j)
            {
                BattlePetAbilityEffectEntry const* eEntry = sBattlePetAbilityEffectStore.LookupEntry(j);

                if (!eEntry)
                    continue;

                if (eEntry->TurnEntryID == tEntry->ID)
                {
                    // get effect properties data
                    for (uint32 k = 0; k < sBattlePetEffectPropertiesStore.GetNumRows(); ++k)
                    {
                        BattlePetEffectPropertiesEntry const* pEntry = sBattlePetEffectPropertiesStore.LookupEntry(k);

                        if (!pEntry)
                            continue;

                        if (eEntry->propertiesID == pEntry->ID)
                        {
                            char* desc = "Points";

                            for (uint8 l = 0; l < MAX_EFFECT_PROPERTIES; ++l)
                            {
                                if (!strcmp(pEntry->propertyDescs[l], desc))
                                    return eEntry->propertyValues[l];
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

uint8 BattlePetMgr::GetAbilityType(uint32 abilityID)
{
    for (uint32 i = 0; i < sBattlePetAbilityStore.GetNumRows(); ++i)
    {
        BattlePetAbilityEntry const* aEntry = sBattlePetAbilityStore.LookupEntry(i);

        if (!aEntry)
            continue;

        if (aEntry->ID == abilityID)
            return aEntry->Type;
    }

    return 0;
}

float BattlePetMgr::GetAttackModifier(uint8 attackType, uint8 defenseType)
{
    // maybe number of attack types?
    uint32 formulaValue = 0xA;
    uint32 modId = defenseType * formulaValue + attackType;

    for (uint32 i = 0; i < sGtBattlePetTypeDamageModStore.GetNumRows(); ++i)
    {
        GtBattlePetTypeDamageModEntry const* gt = sGtBattlePetTypeDamageModStore.LookupEntry(i);

        if (!gt)
            continue;

        if (gt->Id == modId)
            return gt->value;
    }

    return 1.0f;
}

// wild pet battle class handler
PetBattleWild::PetBattleWild(Player* owner) : m_player(owner), roundID(0)
{
}

void PetBattleWild::Prepare(ObjectGuid creatureGuid)
{
    // get pet data for player 1
    battleslots[0] = m_player->GetBattlePetMgr()->GetPetBattleSlot(0);

    if (!battleslots[0])
        return;

    uint64 allyPetGuid = battleslots[0]->GetPet();
    pets[0] = m_player->GetBattlePetMgr()->GetPetInfoByPetGUID(allyPetGuid);

    if (!pets[0])
        return;

    // create pet data for player 2 (wild)
    Creature * wildPet = m_player->GetMap()->GetCreature(creatureGuid);

    if (!wildPet)
        return;

    BattlePetSpeciesEntry const* s = m_player->GetBattlePetMgr()->GetBattlePetSpeciesEntry(wildPet->GetEntry());

    if (!s)
        return;

    CreatureTemplate const* t = sObjectMgr->GetCreatureTemplate(wildPet->GetEntry());

    if (!t)
        return;

    // roll random quality
    uint8 quality = 0;
    if (roll_chance_i(1))
        quality = 4;
    else if (roll_chance_i(10))
        quality = 3;
    else if (roll_chance_i(25))
        quality = 2;
    else if (roll_chance_i(60))
        quality = 1;

    BattlePetStatAccumulator* accumulator = m_player->GetBattlePetMgr()->InitStateValuesFromDB(s->ID, 12);
    accumulator->GetQualityMultiplier(quality, wildPet->getLevel());
    uint32 health = accumulator->CalculateHealth();
    uint32 power = accumulator->CalculatePower();
    uint32 speed = accumulator->CalculateSpeed();
    delete accumulator;

    pets[1] = new PetInfo(s->ID, wildPet->GetEntry(), wildPet->getLevel(), t->Modelid1, power, speed, health, health, quality, 0, 0, s->spellId, "", 12, 0);
    battleslots[1] = new PetBattleSlot(0);

    if (!pets[1])
        return;

    if (!battleslots[1])
        return;

    // some abilities and effects
    abilities[0] = m_player->GetBattlePetMgr()->GetAbilityID(pets[0]->speciesID, 0);
    abilities[1] = m_player->GetBattlePetMgr()->GetAbilityID(pets[1]->speciesID, 0);

    effects[0] = m_player->GetBattlePetMgr()->GetEffectIDByAbilityID(abilities[0]);
    effects[1] = m_player->GetBattlePetMgr()->GetEffectIDByAbilityID(abilities[1]);

    if (!effects[0] || !effects[1])
        return;

    if (pets[0]->IsDead())
        return;

    WorldPacket data(SMSG_PET_BATTLE_FULL_UPDATE);
    // BITPACK
    // enviro states and auras count and bitpack handle (weather?) (some enviroment variables?)
    for (uint8 i = 0; i < 3; ++i)
    {
        data.WriteBits(0, 21);
        data.WriteBits(0, 21);
    }

    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);

    guids[0] = m_player->GetBattlePetMgr()->InverseGuid(m_player->GetObjectGuid());
    guids[1] = 0;

    for (uint8 i = 0; i < 2; ++i)
    {
        data.WriteGuidMask<2>(guids[i]);
        data.WriteBit(1);
        data.WriteGuidMask<5, 7, 4>(guids[i]);
        data.WriteBit(0);
        data.WriteGuidMask<0>(guids[i]);
        data.WriteBits(1, 2);                      // pet count : temporary - 1
        data.WriteGuidMask<1>(guids[i]);

        for (uint8 j = 0; j < MAX_ACTIVE_PETS; ++j)
        {
            if (j != 0)
                break;

            data.WriteGuidMask<3, 4, 0>(battleslots[i]->GetPet());
            data.WriteBit(0);
            data.WriteGuidMask<6>(battleslots[i]->GetPet());
            data.WriteBits(4, 21);  // state count
            data.WriteBits(1, 20);  // abilities count
            data.WriteGuidMask<5>(battleslots[i]->GetPet());

            data.WriteBit(0);

            data.WriteGuidMask<2, 1, 7>(battleslots[i]->GetPet());
            data.WriteBit(1);

            uint8 len = pets[i]->customName == "" ? 0 : pets[i]->customName.length();
            data.WriteBits(len, 7);
            data.WriteBits(0, 21);  // auras count
        }

        data.WriteGuidMask<6>(guids[i]);
        data.WriteBit(0);
        data.WriteGuidMask<3>(guids[i]);
    }

    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(1);

    data.WriteGuidMask<6, 0, 1, 4, 2, 5, 3, 7>(wildPet->GetObjectGuid());

    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);

    // BYTEPACK
    uint32 abilityID[2] = {114, 484};

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < MAX_ACTIVE_PETS; ++j)
        {
            if (j != 0)
                break;

            data << uint8(0);
            data << uint16(0);
            data << uint8(0);
            data << uint16(0);
            data << uint32(abilities[i] ? abilities[i] : abilityID[i]);

            data << uint16(pets[i]->xp);
            data << uint32(pets[i]->maxHealth);

            data.WriteGuidBytes<1>(battleslots[i]->GetPet());

            // states - same for testing
            data << uint32(1600); // some fucking strange value!
            data << uint32(20);   // stateID from BattlePetState.db2 -> Stat_Speed
            data << uint32(1800); // some fucking strange value1!
            data << uint32(18);   // stateID from BattlePetState.db2 -> Stat_Power
            data << uint32(1700); // some fucking strange value2!
            data << uint32(19);   // stateID from BattlePetState.db2 -> Stat_Stamina
            data << uint32(5);    // crit chance % value
            data << uint32(40);   // stateID from BattlePetState.db2 -> Stat_CritChance

            data << uint32(0);
            data << uint32(pets[i]->speed); // speed

            // auras
            //

            data.WriteGuidBytes<4>(battleslots[i]->GetPet());
            uint8 len = pets[i]->customName == "" ? 0 : pets[i]->customName.length();
            if (len > 0)
                data.WriteString(pets[i]->customName);
            data << uint16(pets[i]->quality);
            data.WriteGuidBytes<0>(battleslots[i]->GetPet());
            data << uint32(pets[i]->health);
            data.WriteGuidBytes<3>(battleslots[i]->GetPet());
            data << uint32(pets[i]->speciesID);
            data << uint32(0);
            data.WriteGuidBytes<2, 6>(battleslots[i]->GetPet());
            data << uint32(pets[i]->displayID);
            data << uint16(pets[i]->level);
            data.WriteGuidBytes<7, 5>(battleslots[i]->GetPet());
            data << uint32(pets[i]->power);
            data << uint8(0); // pet slot index?
        }

        data << uint32(0);   // trap spell ID
        data.WriteGuidBytes<2, 5>(guids[i]);
        data << uint32(2);

        data.WriteGuidBytes<4, 0, 7, 6>(guids[i]);
        data << uint8(6);
        data << uint8(0);
        data.WriteGuidBytes<1, 3>(guids[i]);
    }

    data.WriteGuidBytes<6, 2, 1, 3, 0, 4, 7, 5>(wildPet->GetObjectGuid());

    data << uint16(30);
    data << uint16(30);
    data << uint8(1);
    data << uint32(0);
    data << uint8(10);

    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::FirstRound()
{
    WorldPacket data(SMSG_PET_BATTLE_FIRST_ROUND);
    for (uint8 i = 0; i < 2; ++i)
    {
        data << uint16(0); // RoundTimeSecs
        data << uint8(2);  // NextInputFlags
        data << uint8(0);  // NextTrapStatus
    }

    data << uint32(roundID);

    // unk count
    data.WriteBits(0, 3);
    //
    data.WriteBit(0);
    // effects count
    data.WriteBits(2, 22);

    for (uint8 i = 0; i < 2; ++i) // 22bits
    {
        data.WriteBit(0);
        data.WriteBit(1);
        data.WriteBit(1);
        // effect target count
        data.WriteBits(1, 25);
        data.WriteBit(1);

        for (uint8 j = 0; j < 1; ++j) // 25bits
        {
            data.WriteBit(0);
            // action number?
            data.WriteBits(3, 3);
        }

        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(0);
    }

    // cooldowns count
    data.WriteBits(0, 20);

    for (uint8 i = 0; i < 2; ++i) // 22bits
    {
        for (uint8 j = 0; j < 1; ++j) // 25bits
            data << uint8(!i ? 0 : 3); // Petx - pet number

        data << uint8(!i ? 0 : 3); // CasterPBOID?
        data << uint8(4);          // StackDepth?
    }

    data << uint8(2); // NextPetBattleState?

    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::CalculateRoundData(int8 &state, uint32 _roundID)
{
    PetInfo* allyPet = pets[0];
    PetInfo* enemyPet = pets[1];

    if (!allyPet || !enemyPet)
    {
        state = -1;
        return;
    }

    // base efects flags
    effectFlags[0] = 0x1000;
    effectFlags[1] = 0x1000;

    // miss

    // Player 1 - calc ability damage (test calc!!) / player 1 always started
    uint32 damage = m_player->GetBattlePetMgr()->GetBasePoints(abilities[0]) * (1 + allyPet->power * 0.05f);
    // some random element
    damage = urand(damage - 5, damage + 5);

    // modifiers
    uint8 type = m_player->GetBattlePetMgr()->GetAbilityType(abilities[0]);
    float mod = m_player->GetBattlePetMgr()->GetAttackModifier(type, enemyPet->GetType());

    // flags
    if (mod > 1.0f)
        effectFlags[0] |= 0x400;
    else if (mod < 1.0f)
        effectFlags[0] |= 0x800;

    damage *= mod;

    // crit
    bool crit = roll_chance_i(5);

    if (crit)
    {
        effectFlags[0] |= 0x4;
        damage *= 2;
    }

    enemyPet->health -= damage;

    if (enemyPet->health > 0)
    {
        // miss

        // Player 2 - calc ability damage (test calc!!)
        uint32 damage1 = m_player->GetBattlePetMgr()->GetBasePoints(abilities[1]) * (1 + enemyPet->power * 0.05f);
        // some random element
        damage1 = urand(damage1 - 5, damage1 + 5);

        // modifiers
        uint8 type1 = m_player->GetBattlePetMgr()->GetAbilityType(abilities[1]);
        float mod1 = m_player->GetBattlePetMgr()->GetAttackModifier(type1, allyPet->GetType());

        // flags
        if (mod1 > 1.0f)
            effectFlags[1] |= 0x400;
        else if (mod1 < 1.0f)
            effectFlags[1] |= 0x800;

        damage1 *= mod;

        // crit
        bool crit1 = roll_chance_i(5);

        if (crit1)
        {
            effectFlags[1] |= 0x4;
            damage1 *= 2;
        }

        allyPet->health -= damage1;
    }

    if (allyPet->health <= 0 || enemyPet->health <= 0)
        state = 1;
    else
        state = 0;

    roundID = _roundID;
}

void PetBattleWild::RoundResults()
{
    // increase round
    roundID++;

    // test hack
    uint32 effectCount = 4;
    if (pets[1] && pets[1]->IsDead())
        effectCount = 2;

    WorldPacket data(SMSG_PET_BATTLE_ROUND_RESULT);
    data.WriteBit(0);
    // cooldowns count
    data.WriteBits(0, 20);
    // effects count
    data.WriteBits(effectCount, 22);
    // 2 effects - SetHealth and 2 - unk, maybe for scene playback
    uint8 bit[4] = {0, 1, 0, 1};
    uint8 bit1[4] = {1, 0, 1, 0};
    uint8 bit2[4] = {0, 1, 0, 1};
    uint8 bit3[4] = {0, 1, 0, 1};
    uint8 bit4[4] = {0, 1, 0, 1};
    uint8 bit5[4] = {0, 1, 0, 1};
    uint8 bit6[4] = {0, 1, 0, 1};
    uint8 bit7[4] = {1, 1, 1, 1};
    uint8 actNumbers[4] = {6, 3, 6, 3};
    for (uint8 i = 0; i < effectCount; ++i)
    {
        data.WriteBit(bit[i]);
        data.WriteBit(bit1[i]);
        data.WriteBit(bit2[i]);
        data.WriteBit(bit3[i]);
        data.WriteBit(bit4[i]);
        data.WriteBit(bit5[i]);

        // effect target count
        data.WriteBits(1, 25);

        for (uint8 j = 0; j < 1; ++j)
        {
            data.WriteBits(actNumbers[i], 3);

            if (actNumbers[i] == 6)
                data.WriteBit(0);              // !hasSetHealth

            data.WriteBit(bit6[i]);
        }

        data.WriteBit(bit7[i]);
    }

    // unk count
    data.WriteBits(0, 3);

    // cooldowns
    //

    for (uint8 i = 0; i < effectCount; ++i)
    {
        if (!bit3[i])
            data << uint16(!i ? 1 : 2); // TurnInstanceID

        for (uint8 j = 0; j < 1; ++j)
        {
            if (!bit6[i])
                data << uint8(!i ? 3 : 0); // target?

            if (actNumbers[i] == 6)
                data << uint32(pets[!i ? 1 : 0]->health);  // remainingHealth
        }

        if (!bit2[i])
            data << uint16(!i ? effectFlags[0] : effectFlags[1]); // Flags

        if (!bit7[i])
            data << uint16(0);    // SourceAuraInstanceID

        if (!bit[i])
            data << uint8(!i ? 0 : 3);     // CasterPBOID

        if (!bit4[i])
            data << uint8(1);     // StackDepth

        if (!bit5[i])
            data << uint32(!i ? effects[0] : effects[1]); // AbilityEffectID

        if (!bit1[i])
            data << uint8((i == 1) ? 13 : 14); // PetBattleEffectType
    }

    for (uint8 i = 0; i < 2; ++i)
    {
        data << uint8(0);  // NextTrapStatus
        data << uint16(0); // RoundTimeSecs
        data << uint8(2);  // NextInputFlags
    }

    data << uint32(roundID);
    data << uint8(2); // NextPetBattleState?

    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::FinalRound()
{
    int8 winner[2] = {-1, -1};
    PetInfo* allyPet = pets[0];
    PetInfo* enemyPet = pets[1];

    if (!allyPet || !enemyPet)
        return;

    // some calc
    if (allyPet->health <= 0)
    {
        // test hack
        allyPet->health = 0;
        winner[0] = 0;
        winner[1] = 1;
    }
    else if (enemyPet->health <= 0)
    {
        enemyPet->health = 0;
        winner[0] = 1;
        winner[1] = 0;
    }

    if (winner[0] == -1 || winner[1] == -1)
        return;

    bool levelUp = false;

    WorldPacket data(SMSG_PET_BATTLE_FINAL_ROUND);
    // pet depends count
    data.WriteBits(2, 20);

    for (uint8 i = 0; i < 2; i++)
    {
        data.WriteBit(0); // unused
        data.WriteBit(1); // unused
        data.WriteBit(!i ? 0 : 1); // !hasXp
        data.WriteBit(0); // unused
        data.WriteBit(0); // !hasInitialLevel
        data.WriteBit(0); // !hasLevel
        data.WriteBit(0); // unused
    }

    data.WriteBit(0); // !abandoned

    // winners bits
    for (uint8 i = 0; i < 2; ++i)
        data.WriteBit(winner[i]);

    data.WriteBit(0); // !hasXpReward???

    for (uint8 i = 0; i < 2; ++i)
    {
        if (!i)
        {
            uint16 xp = RewardXP(winner[0], levelUp);
            data << uint16(xp);
        }

        data << uint8(!i ? 0 : 3); // Pboid
        data << uint32(pets[i]->health); // RemainingHealth
        data << uint16(levelUp ? pets[i]->level+1 : pets[i]->level);  // Level
        data << uint32(pets[i]->maxHealth); // maxHealth
        data << uint16(pets[i]->level); // InitialLevel
    }

    for (uint8 i = 0; i < 2; ++i)
        data << uint32(0); // NpcCreatureID

    m_player->GetSession()->SendPacket(&data);
}

uint16 PetBattleWild::RewardXP(bool winner, bool& levelUp)
{
    PetInfo* allyPet = pets[0];
    PetInfo* enemyPet = pets[1];

    if (!allyPet || !enemyPet)
        return 0;

    if (allyPet->level == 2)
        return 0;

    // simple calc xp
    uint16 xp = allyPet->xp;

    // scaling xp -> quality (win)
    uint16 winXP = 10;

    if (enemyPet->quality == 4)
        winXP += 15;
    else if (enemyPet->quality == 3)
        winXP += 10;
    else if (enemyPet->quality == 2)
        winXP += 5;
    else if (enemyPet->quality == 0)
        winXP -= 2;

    // scaling xp -> quality (lose)
    uint16 loseXP = 5;

    if (enemyPet->quality == 4)
        loseXP += 7;
    else if (enemyPet->quality == 3)
        loseXP += 5;
    else if (enemyPet->quality == 2)
        loseXP += 2;
    else if (enemyPet->quality == 0)
        loseXP -= 3;

    if (winner)
        xp += winXP;
    else
        xp += loseXP;

    if (xp > 50)
    {
        xp = 0;
        levelUp = true;
        allyPet->level = 2;
    }

    allyPet->xp = xp;
    return xp;
}

void PetBattleWild::FinishPetBattle()
{
    WorldPacket data(SMSG_PET_BATTLE_FINISHED);
    m_player->GetSession()->SendPacket(&data);
}

uint32 PetBattleWild::GetEffectID(uint32 abilityID, uint8 effectIndex)
{
    return 0;
}