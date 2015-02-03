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
    memset(m_battleSlots, 0, sizeof(PetBattleSlot*)*MAX_ACTIVE_BATTLE_PETS);
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

    data->WriteBits(MAX_ACTIVE_BATTLE_PETS, 25);

    for (uint32 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
    {
        PetBattleSlot * slot = GetPetBattleSlot(i);

        // check slot locked conditions
        bool locked = true;

        switch (i)
        {
            case 0: locked = !m_player->HasSpell(119467); break;
            case 1: locked = /*!m_player->GetAchievementMgr().HasAchieved(7433)*/false; break;
            case 2: locked = /*!m_player->GetAchievementMgr().HasAchieved(6566)*/false; break;
            default: break;
        }

        data->WriteBit(!i);                                          // hasSlotIndex
        data->WriteBit(1);                                           // hasCollarID, inverse
        data->WriteBit(slot ? !slot->IsEmpty() : 1);                 // empty slot, inverse
        data->WriteGuidMask<7, 1, 3, 2, 5, 0, 4, 6>(slot ? slot->petGUID : 0);  // pet guid in slot
        data->WriteBit(locked);                    // locked slot
    }

    data->WriteBit(1);                      // !hasJournalLock

    for (uint32 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
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

    if (slotID >= MAX_ACTIVE_BATTLE_PETS)
        return;

    // find dest slot
    PetBattleSlot * slot = _player->GetBattlePetMgr()->GetPetBattleSlot(slotID);

    if (!slot)
        return;

    // find current pet
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->internalState == STATE_DELETED)
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

    locationResult = sWorld->getBoolConfig(CONFIG_PET_BATTLES_ENABLED) ? 21 : 0;
    data << _player->GetOrientation();
    data << uint32(locationResult);
    SendPacket(&data);

    // send full update and start pet battle
    if (sWorld->getBoolConfig(CONFIG_PET_BATTLES_ENABLED))
        _player->GetBattlePetMgr()->InitWildBattle(_player, guid);
}

void WorldSession::HandlePetBattleInputFirstPet(WorldPacket& recvData)
{
    uint8 firstPetID;
    recvData >> firstPetID;

    if (PetBattleWild* petBattle = _player->GetBattlePetMgr()->GetPetBattleWild())
    {
        PetBattleRoundResults* firstRound = petBattle->PrepareFirstRound(firstPetID);

        if (firstRound)
        {
            petBattle->SendFirstRound(firstRound);

            delete firstRound;
            firstRound = NULL;
        }
        else
            petBattle->FinishPetBattle();
    }
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
    bool ignoreAbandonPenalty = recvData.ReadBit();
    bool bit4 = recvData.ReadBit();
    bool bit5 = recvData.ReadBit();
    bool bit6 = recvData.ReadBit();

    uint32 abilityID = 0;
    uint32 roundID = 0;
    uint8 moveType = 0;
    uint8 newFrontPet = 0;
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
        PetBattleRoundResults* round = petBattle->UseAbility(abilityID, roundID);

        if (round)
        {
            petBattle->SendRoundResults(round);

            delete round;
            round = NULL;
        }
        else
            petBattle->FinishPetBattle();
    }
    // SkipTurn
    else if (moveType == 2)
    {
        PetBattleRoundResults* round = petBattle->SkipTurn(roundID);

        if (round)
        {
            petBattle->SendRoundResults(round);

            delete round;
            round = NULL;
        }
        else
            petBattle->FinishPetBattle();
    }
    // TrapPet
    /*else if ()
    {

    }
    // Forfeit - handle in QuitNotify
    else if ()
    {

    }*/

    if (petBattle->NextRoundFinal() && moveType != 4)
    {
        PetBattleFinalRound* finalRound = petBattle->PrepareFinalRound();

        if (finalRound)
        {
            petBattle->SendFinalRound(finalRound);

            delete finalRound;
            finalRound = NULL;
        }
        else
            petBattle->FinishPetBattle();
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

        PetBattleFinalRound* finalRound = petBattle->PrepareFinalRound(true);

        if (finalRound)
        {
            petBattle->SendFinalRound(finalRound);

            delete finalRound;
            finalRound = NULL;
        }
        else
            petBattle->FinishPetBattle();
    }
}

void BattlePetMgr::CloseWildPetBattle()
{
    uint8 updateCount = 0;

    for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
    {
        if (PetBattleSlot * slot = GetPetBattleSlot(i))
        {
            if (PetInfo * pet = GetPetInfoByPetGUID(slot->GetPet()))
            {
                pet->SetInternalState(STATE_UPDATED);
                updateCount++;
            }
        }
    }

    // update marked pets
    if (updateCount)
        SendUpdatePets();

    // remove battle object
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

uint32 PetBattleAbility::GetEffectIDByAbilityID()
{
    // get turn data entry
    for (uint32 i = 0; i < sBattlePetAbilityTurnStore.GetNumRows(); ++i)
    {
        BattlePetAbilityTurnEntry const* tEntry = sBattlePetAbilityTurnStore.LookupEntry(i);

        if (!tEntry)
            continue;

        if (tEntry->AbilityID == ID && tEntry->turnIndex == 1)
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

// testing....
uint32 BattlePetMgr::GetXPForNextLevel(uint8 level)
{
    switch (level)
    {
        case 1: return 50; break;
        case 2: return 110; break;
        case 3: return 120; break;
        case 4: return 195; break;
        case 5: return 280; break;
        case 6: return 450; break;
        case 7: return 560; break;
        case 8: return 595; break;
        case 9: return 720; break;
        case 10: return 760; break;
        case 11: return 900; break;
        case 12: return 945; break;
        case 13: return 990; break;
        case 14: return 1150; break;
        case 15: return 1200; break;
        case 16: return 1250; break;
        case 17: return 1430; break;
        case 18: return 1485; break;
        case 19: return 1555; break;
        case 20: return 1595; break;
        case 21: return 1800; break;
        case 22: return 1860; break;
        case 23: return 1920; break;
        case 24: return 1980; break;
        default: return 9999; break;
    }
}

// wild pet battle class handler
PetBattleWild::PetBattleWild(Player* owner) : m_player(owner)
{
}

bool PetBattleWild::InitBattleData()
{
    for (uint8 i = 0; i < 2; ++i)
    {
        // zero data
        for (uint8 j = 0; j < 3; ++j)
        {
            battleData[i][j] = new PetBattleData(0, NULL, NULL, false);

            for (uint8 k = 0; k < 3; ++k)
                battleData[i][j]->activeAbilities[k] = NULL;
        }

        // zero counts
        petsCount[i] = 0;
    }

    nextRoundFinal = false;
    abandoned = false;

    return true;
}

void PetBattleWild::Prepare(ObjectGuid creatureGuid)
{
    if (!InitBattleData())
        return;

    // get pet data for player (1)
    for (uint32 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
    {
        if (PetBattleSlot* _slot = m_player->GetBattlePetMgr()->GetPetBattleSlot(i))
        {
            if (!_slot->IsEmpty())
            {
                battleData[TEAM_ALLY][i]->petX = i;
                battleData[TEAM_ALLY][i]->slot = _slot;
                uint64 petGuid = battleData[TEAM_ALLY][i]->GetSlot()->GetPet();
                battleData[TEAM_ALLY][i]->tempPet = m_player->GetBattlePetMgr()->GetPetInfoByPetGUID(petGuid);

                // not find real pet object
                if (!battleData[TEAM_ALLY][i]->tempPet || !battleData[TEAM_ALLY][i]->slot)
                {
                    FinishPetBattle();
                    return;
                }

                petsCount[TEAM_ALLY]++;

                for (uint8 j = 0; j < 3; ++j)
                {
                    uint32 ID = battleData[TEAM_ALLY][i]->tempPet->GetActiveAbilityID(j);
                    battleData[TEAM_ALLY][i]->activeAbilities[j] = new PetBattleAbility(ID);

                    if (!battleData[TEAM_ALLY][i]->activeAbilities[j])
                    {
                        FinishPetBattle();
                        return;
                    }

                    battleData[TEAM_ALLY][i]->activeAbilities[j]->CalculateAbilityData();
                }
            }
        }
    }

    // create pet data for wild pet (2)
    Creature * wildPet = m_player->GetMap()->GetCreature(creatureGuid);

    if (!wildPet)
        return;

    uint8 wildPetLevel = wildPet->GetUInt32Value(UNIT_FIELD_WILD_BATTLE_PET_LEVEL);

    BattlePetSpeciesEntry const* s = m_player->GetBattlePetMgr()->GetBattlePetSpeciesEntry(wildPet->GetEntry());

    if (!s)
        return;

    CreatureTemplate const* t = sObjectMgr->GetCreatureTemplate(wildPet->GetEntry());

    if (!t)
        return;

    // roll creatture count
    uint32 creatureCount = 1;

    for (uint8 i = 0; i < creatureCount; ++i)
    {
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
        accumulator->CalcQualityMultiplier(quality, wildPetLevel);
        uint32 health = accumulator->CalculateHealth();
        uint32 power = accumulator->CalculatePower();
        uint32 speed = accumulator->CalculateSpeed();
        delete accumulator;

        battleData[TEAM_ENEMY][i]->petX = i+MAX_ACTIVE_BATTLE_PETS;
        battleData[TEAM_ENEMY][i]->slot = new PetBattleSlot(0);
        battleData[TEAM_ENEMY][i]->tempPet = new PetInfo(s->ID, wildPet->GetEntry(), wildPetLevel, t->Modelid1, power, speed, health, health, quality, 0, 0, s->spellId, "", 12, 0);

        // not find real pet object
        if (!battleData[TEAM_ENEMY][i]->tempPet || !battleData[TEAM_ENEMY][i]->slot)
        {
            FinishPetBattle();
            return;
        }

        petsCount[TEAM_ENEMY]++;

        for (uint8 j = 0; j < 3; ++j)
        {
            uint32 ID = battleData[TEAM_ENEMY][i]->tempPet->GetActiveAbilityID(j);
            battleData[TEAM_ENEMY][i]->activeAbilities[j] = new PetBattleAbility(ID);

            if (!battleData[TEAM_ALLY][i]->activeAbilities[j])
            {
                FinishPetBattle();
                return;
            }

            battleData[TEAM_ENEMY][i]->activeAbilities[j]->CalculateAbilityData();
        }
    }

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

    teamGuids[0] = m_player->GetBattlePetMgr()->InverseGuid(m_player->GetObjectGuid());
    teamGuids[1] = 0;

    for (uint8 i = 0; i < 2; ++i)
    {
        data.WriteGuidMask<2>(teamGuids[i]);
        data.WriteBit(1);
        data.WriteGuidMask<5, 7, 4>(teamGuids[i]);
        data.WriteBit(0);
        data.WriteGuidMask<0>(teamGuids[i]);
        data.WriteBits(petsCount[i], 2);
        data.WriteGuidMask<1>(teamGuids[i]);

        for (uint8 j = 0; j < petsCount[i]; ++j)
        {
            PetBattleSlot* slot = battleData[i][j]->GetSlot();
            PetInfo* pet = battleData[i][j]->GetPetInfo();

            if (!slot || !pet)
            {
                FinishPetBattle();
                return;
            }

            data.WriteGuidMask<3, 4, 0>(slot->GetPet());
            data.WriteBit(0);
            data.WriteGuidMask<6>(slot->GetPet());
            data.WriteBits(4, 21);  // state count
            data.WriteBits(1, 20);  // abilities count
            data.WriteGuidMask<5>(slot->GetPet());

            data.WriteBit(0);

            data.WriteGuidMask<2, 1, 7>(slot->GetPet());
            data.WriteBit(1);

            uint8 len = pet->GetCustomName() == "" ? 0 : pet->GetCustomName().length();
            data.WriteBits(len, 7);
            data.WriteBits(0, 21);  // auras count
        }

        data.WriteGuidMask<6>(teamGuids[i]);
        data.WriteBit(0);
        data.WriteGuidMask<3>(teamGuids[i]);
    }

    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(1);

    data.WriteGuidMask<6, 0, 1, 4, 2, 5, 3, 7>(wildPet->GetObjectGuid());

    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < petsCount[i]; ++j)
        {

            PetBattleSlot* slot = battleData[i][j]->GetSlot();
            PetInfo* pet = battleData[i][j]->GetPetInfo();

            if (!slot || !pet)
            {
                FinishPetBattle();
                return;
            }

            // abilities
            //for (uint8 i = 0; i < 3; ++i)
            //{
                data << uint8(0);
                data << uint16(0);
                data << uint8(0);
                data << uint16(0);
                data << uint32(battleData[i][j]->activeAbilities[0]->ID);
            //}

            data << uint16(pet->GetXP());
            data << uint32(pet->GetMaxHealth());

            data.WriteGuidBytes<1>(slot->GetPet());

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
            data << uint32(pet->GetSpeed()); // speed

            // auras
            //

            data.WriteGuidBytes<4>(slot->GetPet());
            uint8 len = pet->GetCustomName() == "" ? 0 : pet->GetCustomName().length();
            if (len > 0)
                data.WriteString(pet->GetCustomName());
            data << uint16(pet->GetQuality());
            data.WriteGuidBytes<0>(slot->GetPet());
            data << uint32(pet->GetHealth());
            data.WriteGuidBytes<3>(slot->GetPet());
            data << uint32(pet->GetSpeciesID());
            data << uint32(0);
            data.WriteGuidBytes<2, 6>(slot->GetPet());
            data << uint32(pet->GetDisplayID());
            data << uint16(pet->GetLevel());
            data.WriteGuidBytes<7, 5>(slot->GetPet());
            data << uint32(pet->GetPower());
            data << uint8(j); // Slot
        }

        data << uint32(0);   // TrapSpellID
        data.WriteGuidBytes<2, 5>(teamGuids[i]);
        data << uint32(2);   // TrapStatus

        data.WriteGuidBytes<4, 0, 7, 6>(teamGuids[i]);
        data << uint8(6);  // InputFlags
        data << uint8(0);  // FrontPet
        data.WriteGuidBytes<1, 3>(teamGuids[i]);
    }

    data.WriteGuidBytes<6, 2, 1, 3, 0, 4, 7, 5>(wildPet->GetObjectGuid());

    data << uint16(30); // WaitingForFrontPetsMaxSecs | PvpMaxRoundTime
    data << uint16(30); // WaitingForFrontPetsMaxSecs | PvpMaxRoundTime
    data << uint8(1);   // CurPetBattleState
    data << uint32(0);  // CurRound
    data << uint8(10);  // ForfeitPenalty

    m_player->GetSession()->SendPacket(&data);
}

PetBattleRoundResults* PetBattleWild::PrepareFirstRound(uint8 frontPet)
{
    // set active
    SetActivePet(TEAM_ALLY, frontPet);
    SetActivePet(TEAM_ENEMY, 0);

    PetBattleRoundResults* firstRound = new PetBattleRoundResults(0);
    PetBattleEffect* effect = new PetBattleEffect(0, 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(0, 3);

    effect->targets.push_back(target);
    firstRound->effects.push_back(effect);

    PetBattleEffect* effect1 = new PetBattleEffect(3, 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target1 = new PetBattleEffectTarget(3, 3);

    effect1->targets.push_back(target1);
    firstRound->effects.push_back(effect1);

    return firstRound;
}

void PetBattleWild::SendFirstRound(PetBattleRoundResults* firstRound)
{
    WorldPacket data(SMSG_PET_BATTLE_FIRST_ROUND);
    for (uint8 i = 0; i < 2; ++i)
    {
        data << uint16(0); // RoundTimeSecs
        data << uint8(2);  // NextTrapStatus
        data << uint8(0);  // NextInputFlags
    }

    data << uint32(firstRound->roundID);

    // petX Died
    data.WriteBits(firstRound->petXDiedNumbers.size(), 3);
    //
    data.WriteBit(0);
    // effects count
    data.WriteBits(firstRound->effects.size(), 22);

    for (std::list<PetBattleEffect*>::iterator itr = firstRound->effects.begin(); itr != firstRound->effects.end(); ++itr)
    {
        PetBattleEffect* effect = (*itr);

        if (!effect)
            return;

        data.WriteBit(!effect->petBattleEffectType);
        data.WriteBit(!effect->sourceAuraInstanceID);
        data.WriteBit(!effect->abilityEffectID);
        // effect target count
        data.WriteBits(effect->targets.size(), 25);
        data.WriteBit(1);

        for (std::list<PetBattleEffectTarget*>::iterator i = effect->targets.begin(); i != effect->targets.end(); ++i)
        {
            PetBattleEffectTarget* target = (*i);

            if (!target)
                return;

            data.WriteBit(target->petX == -1 ? 1 : 0);
            data.WriteBits(target->type, 3); // TargetType
        }

        data.WriteBit(!effect->flags);
        data.WriteBit(!effect->stackDepth);
        data.WriteBit(effect->casterPBOID == -1 ? 1 : 0);
    }

    // cooldowns count
    data.WriteBits(0, 20);

    for (std::list<PetBattleEffect*>::iterator itr = firstRound->effects.begin(); itr != firstRound->effects.end(); ++itr)
    {
        PetBattleEffect* effect = (*itr);

        if (!effect)
            return;

        for (std::list<PetBattleEffectTarget*>::iterator i = effect->targets.begin(); i != effect->targets.end(); ++i)
        {
            PetBattleEffectTarget* target = (*i);

            if (!target)
                return;

            data << uint8(target->petX); // target Petx - pet number
        }

        data << uint8(effect->casterPBOID); // CasterPBOID
        data << uint8(effect->petBattleEffectType); // PetBattleEffectType
    }

    data << uint8(2); // NextPetBattleState

    m_player->GetSession()->SendPacket(&data);
}

PetBattleRoundResults* PetBattleWild::UseAbility(uint32 abilityID, uint32 _roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);

    // generate effects and effectTargets data for player 1 (ONLY TESTING!!)
    if (!battleData[TEAM_ALLY][0]->activeAbilities[0] || !battleData[TEAM_ENEMY][0]->activeAbilities[0])
        return NULL;
    if (!battleData[TEAM_ALLY][0]->GetPetInfo() || !battleData[TEAM_ENEMY][0]->GetPetInfo())
        return NULL;

    PetBattleEffect* effect = new PetBattleEffect(battleData[TEAM_ALLY][0]->GetPetNumber(), battleData[TEAM_ALLY][0]->activeAbilities[0]->GetEffectIDByAbilityID(), 0, 0x1000, 0, 1, 1);
    // calculate damage
    uint32 base = battleData[TEAM_ALLY][0]->activeAbilities[0]->GetBasePoints(1, 0);
    uint32 baseDamage = base * (1 + battleData[TEAM_ALLY][0]->GetPetInfo()->GetPower() * 0.05f);
    uint32 cleanDamage = urand(baseDamage - 5, baseDamage + 5);
    // mods
    float mod = battleData[TEAM_ALLY][0]->activeAbilities[0]->mods[battleData[TEAM_ENEMY][0]->GetPetInfo()->GetType()];
    uint32 finalDamage = cleanDamage * mod;

    if (mod > 1.0f)
        effect->flags |= 0x400;
    else if (mod < 1.0f)
        effect->flags |= 0x800;

    // crit
    if (roll_chance_i(5))
    {
        effect->flags |= 0x4;
        finalDamage *= 2;
    }

    // ally -> enemy
    // setHealth (only for base abilities, needed full rewrite damage system for other...)
    int32 oldHealth = battleData[TEAM_ENEMY][0]->GetPetInfo()->GetHealth();
    battleData[TEAM_ENEMY][0]->GetPetInfo()->SetHealth(oldHealth - finalDamage);
    int32 newHealth = battleData[TEAM_ENEMY][0]->GetPetInfo()->GetHealth();

    bool enemyDied = false;

    if (newHealth <= 0)
    {
        round->petXDiedNumbers.push_back(battleData[TEAM_ENEMY][0]->GetPetNumber());
        enemyDied = true;
        winners[TEAM_ALLY] = 1;
        winners[TEAM_ENEMY] = 0;
        nextRoundFinal = true;
    }

    PetBattleEffectTarget * target = new PetBattleEffectTarget(battleData[TEAM_ENEMY][0]->GetPetNumber(), 6);
    target->remainingHealth = newHealth;

    effect->targets.push_back(target);
    round->effects.push_back(effect);

    PetBattleEffect* effect1 = new PetBattleEffect(-1, 0, 13, 0, 0, 0, 0);
    PetBattleEffectTarget * target1 = new PetBattleEffectTarget(-1, 3);

    effect1->targets.push_back(target1);
    round->effects.push_back(effect1);

    if (!enemyDied)
    {
        // generate effects and effectTargets data for player 2 (ONLY TESTING!!)
        PetBattleEffect* effect2 = new PetBattleEffect(battleData[TEAM_ENEMY][0]->GetPetNumber(), battleData[TEAM_ENEMY][0]->activeAbilities[0]->GetEffectIDByAbilityID(), 0, 0x1000, 0, 2, 1);
        // calculate damage
        uint32 base = battleData[TEAM_ENEMY][0]->activeAbilities[0]->GetBasePoints(1, 0);
        uint32 baseDamage = base * (1 + battleData[TEAM_ENEMY][0]->GetPetInfo()->GetPower() * 0.05f);
        uint32 cleanDamage = urand(baseDamage - 5, baseDamage + 5);
        // mods
        float mod = battleData[TEAM_ENEMY][0]->activeAbilities[0]->mods[battleData[TEAM_ALLY][0]->GetPetInfo()->GetType()];
        uint32 finalDamage = cleanDamage * mod;

        if (mod > 1.0f)
            effect2->flags |= 0x400;
        else if (mod < 1.0f)
            effect2->flags |= 0x800;

        // crit
        if (roll_chance_i(5))
        {
            effect2->flags |= 0x4;
            finalDamage *= 2;
        }

        // enemy -> ally
        // setHealth (only for base abilities, needed full rewrite damage system for other...)
        int32 oldHealth = battleData[TEAM_ALLY][0]->GetPetInfo()->GetHealth();
        battleData[TEAM_ALLY][0]->GetPetInfo()->SetHealth(oldHealth - finalDamage);
        int32 newHealth = battleData[TEAM_ALLY][0]->GetPetInfo()->GetHealth();

        if (newHealth <= 0)
        {
            round->petXDiedNumbers.push_back(battleData[TEAM_ALLY][0]->GetPetNumber());
            winners[TEAM_ALLY] = 0;
            winners[TEAM_ENEMY] = 1;
            nextRoundFinal = true;
        }

        PetBattleEffectTarget * target2 = new PetBattleEffectTarget(battleData[TEAM_ALLY][0]->GetPetNumber(), 6);
        target2->remainingHealth = newHealth;

        effect2->targets.push_back(target2);
        round->effects.push_back(effect2);

        PetBattleEffect* effect3 = new PetBattleEffect(-1, 0, 14, 0, 0, 0, 0);
        PetBattleEffectTarget * target3 = new PetBattleEffectTarget(-1, 3);

        effect3->targets.push_back(target3);
        round->effects.push_back(effect3);
    }

    // increase round
    round->roundID++;
    return round;
}

PetBattleRoundResults* PetBattleWild::SkipTurn(uint32 _roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);

    PetBattleEffect* effect = new PetBattleEffect(0, 0, 4, 1, 0, 0, 0);
    PetBattleEffectTarget * target = new PetBattleEffectTarget(0, 6);

    effect->targets.push_back(target);
    round->effects.push_back(effect);

    // generate effects and effectTargets data for player 2 (ONLY TESTING!!)
    PetBattleEffect* effect2 = new PetBattleEffect(battleData[TEAM_ENEMY][0]->GetPetNumber(), battleData[TEAM_ENEMY][0]->activeAbilities[0]->GetEffectIDByAbilityID(), 0, 0x1000, 0, 2, 1);
    // calculate damage
    uint32 base = battleData[TEAM_ENEMY][0]->activeAbilities[0]->GetBasePoints(1, 0);
    uint32 baseDamage = base * (1 + battleData[TEAM_ENEMY][0]->GetPetInfo()->GetPower() * 0.05f);
    uint32 cleanDamage = urand(baseDamage - 5, baseDamage + 5);
    // mods
    float mod = battleData[TEAM_ENEMY][0]->activeAbilities[0]->mods[battleData[TEAM_ALLY][0]->GetPetInfo()->GetType()];
    uint32 finalDamage = cleanDamage * mod;

    if (mod > 1.0f)
        effect2->flags |= 0x400;
    else if (mod < 1.0f)
        effect2->flags |= 0x800;

    // crit
    if (roll_chance_i(5))
    {
        effect2->flags |= 0x4;
        finalDamage *= 2;
    }

    // enemy -> ally
    // setHealth (only for base abilities, needed full rewrite damage system for other...)
    int32 oldHealth = battleData[TEAM_ALLY][0]->GetPetInfo()->GetHealth();
    battleData[TEAM_ALLY][0]->GetPetInfo()->SetHealth(oldHealth - finalDamage);
    int32 newHealth = battleData[TEAM_ALLY][0]->GetPetInfo()->GetHealth();

    if (newHealth <= 0)
    {
        round->petXDiedNumbers.push_back(battleData[TEAM_ALLY][0]->GetPetNumber());
        winners[TEAM_ALLY] = 0;
        winners[TEAM_ENEMY] = 1;
        nextRoundFinal = true;
    }

    PetBattleEffectTarget * target2 = new PetBattleEffectTarget(battleData[TEAM_ALLY][0]->GetPetNumber(), 6);
    target2->remainingHealth = newHealth;

    effect2->targets.push_back(target2);
    round->effects.push_back(effect2);

    PetBattleEffect* effect1 = new PetBattleEffect(-1, 0, 13, 0, 0, 0, 0);
    PetBattleEffectTarget * target1 = new PetBattleEffectTarget(-1, 3);

    effect1->targets.push_back(target1);
    round->effects.push_back(effect1);

    PetBattleEffect* effect3 = new PetBattleEffect(-1, 0, 14, 0, 0, 0, 0);
    PetBattleEffectTarget * target3 = new PetBattleEffectTarget(-1, 3);

    effect3->targets.push_back(target3);
    round->effects.push_back(effect3);
    
    // increase round
    round->roundID++;
    return round;
}

void PetBattleWild::SendRoundResults(PetBattleRoundResults* round)
{
    WorldPacket data(SMSG_PET_BATTLE_ROUND_RESULT);
    data.WriteBit(0); // !hasNextBattleState
    // cooldowns count
    data.WriteBits(0, 20);
    // effects count
    data.WriteBits(round->effects.size(), 22);
    for (std::list<PetBattleEffect*>::iterator itr = round->effects.begin(); itr != round->effects.end(); ++itr)
    {
        PetBattleEffect* effect = (*itr);

        if (!effect)
            return;

        data.WriteBit(effect->casterPBOID == -1 ? 1 : 0); // !hasCasterPBOID
        data.WriteBit(!effect->petBattleEffectType); // !hasPetBattleEffectType
        data.WriteBit(!effect->flags); // !hasEffectFlags
        data.WriteBit(!effect->turnInstanceID); // !hasTurnInstanceID?
        data.WriteBit(!effect->stackDepth); // !hasStackDepth
        data.WriteBit(!effect->abilityEffectID); // !hasAbilityEffectID

        data.WriteBits(effect->targets.size(), 25);
        for (std::list<PetBattleEffectTarget*>::iterator i = effect->targets.begin(); i != effect->targets.end(); ++i)
        {
            PetBattleEffectTarget* target = (*i);

            if (!target)
                return;

            data.WriteBits(target->type, 3);

            if (target->type == 6)
                data.WriteBit(0); // !hasRemainingHealth

            data.WriteBit(target->petX == -1 ? 1 : 0); // !hasPetX
        }

        data.WriteBit(!effect->sourceAuraInstanceID); // !hasSourceAuraInstanceID?
    }

    data.WriteBits(round->petXDiedNumbers.size(), 3);

    // cooldowns
    //

    for (std::list<PetBattleEffect*>::iterator itr = round->effects.begin(); itr != round->effects.end(); ++itr)
    {
        PetBattleEffect* effect = (*itr);

        if (!effect)
            return;

        if (effect->turnInstanceID)
            data << uint16(effect->turnInstanceID);

        for (std::list<PetBattleEffectTarget*>::iterator i = effect->targets.begin(); i != effect->targets.end(); ++i)
        {
            PetBattleEffectTarget* target = (*i);

            if (!target)
                return;

            if (target->type == 6)
            {
                data << uint8(target->petX); // targetPetX
                data << int32(target->remainingHealth);
            }
        }

        if (effect->flags)
            data << uint16(effect->flags);

        if (effect->sourceAuraInstanceID)
            data << uint16(effect->sourceAuraInstanceID);

        if (effect->casterPBOID != -1)
            data << uint8(effect->casterPBOID);

        if (effect->stackDepth)
            data << uint8(effect->stackDepth);

        if (effect->abilityEffectID)
            data << uint32(effect->abilityEffectID);

        if (effect->petBattleEffectType)
            data << uint8(effect->petBattleEffectType);
    }

    for (uint8 i = 0; i < 2; ++i)
    {
        data << uint8(0);  // NextInputFlags
        data << uint16(0); // RoundTimeSecs
        data << uint8(2);  // NextTrapStatus
    }

    data << uint32(round->roundID);
    data << uint8(2); // NextPetBattleState

    if (round->petXDiedNumbers.size() > 0)
    {
        for (uint8 i = 0; i < round->petXDiedNumbers.size(); ++i)
            data << uint8(round->petXDiedNumbers[i]);
    }

    m_player->GetSession()->SendPacket(&data);
}

PetBattleFinalRound* PetBattleWild::PrepareFinalRound(bool abandoned)
{
    PetInfo* allyPet = battleData[TEAM_ALLY][0]->GetPetInfo();
    PetInfo* enemyPet = battleData[TEAM_ENEMY][0]->GetPetInfo();

    if (!allyPet || !enemyPet)
        return NULL;

    PetBattleFinalRound* finalRound = new PetBattleFinalRound();

    // rewardXP only for winner
    if (winners[TEAM_ALLY])
    {
        finalRound->rewardedXP[battleData[TEAM_ENEMY][0]->GetPetNumber()] = 0;

        // calculate xp
        if (allyPet->GetLevel() == MAX_BATTLE_PET_LEVEL)
            finalRound->rewardedXP[battleData[TEAM_ALLY][0]->GetPetNumber()] = 0;

        uint16 oldXp = allyPet->GetXP();
        uint16 newXp = 0;

        int8 levelDiff = allyPet->GetLevel() - enemyPet->GetLevel();
        // some checks
        if (levelDiff > 2)
            levelDiff = 2;
        if (levelDiff < -5)
            levelDiff = -5;

        // formula
        uint16 rewardXp = (enemyPet->GetLevel() + 9) * (levelDiff + 5);
        newXp = oldXp + rewardXp;
        uint32 totalXp = m_player->GetBattlePetMgr()->GetXPForNextLevel(allyPet->GetLevel());

        // check level-up
        if (newXp >= totalXp)
        {
            allyPet->SetLevel(allyPet->GetLevel() + 1);
            uint16 remXp = newXp - totalXp;
            allyPet->SetXP(remXp);
            finalRound->rewardedXP[battleData[TEAM_ALLY][0]->GetPetNumber()] = remXp;
            finalRound->levelupPetNumbers.push_back(battleData[TEAM_ALLY][0]->GetPetNumber());

            // recalculate stats
            BattlePetStatAccumulator* accumulator = m_player->GetBattlePetMgr()->InitStateValuesFromDB(allyPet->GetSpeciesID(), BATTLE_PET_BREED_SS);
            accumulator->CalcQualityMultiplier(allyPet->GetQuality(), allyPet->GetLevel());
            uint32 health = accumulator->CalculateHealth();
            uint32 power = accumulator->CalculatePower();
            uint32 speed = accumulator->CalculateSpeed();
            delete accumulator;

            allyPet->SetHealth(health);
            allyPet->SetMaxHealth(health);
            allyPet->SetPower(power);
            allyPet->SetSpeed(speed);
        }
        else
        {
            allyPet->SetXP(newXp);
            finalRound->rewardedXP[battleData[TEAM_ALLY][0]->GetPetNumber()] = newXp;
        }

    }
    else
    {
        finalRound->rewardedXP[battleData[TEAM_ALLY][0]->GetPetNumber()] = 0;
        finalRound->rewardedXP[battleData[TEAM_ENEMY][0]->GetPetNumber()] = 0;

        if (abandoned)
        {
            uint32 percent10 = allyPet->GetHealth() / 10;
            allyPet->SetHealth(allyPet->GetHealth() - percent10);
        }
    }

    return finalRound;
}

void PetBattleWild::SendFinalRound(PetBattleFinalRound* finalRound)
{
    WorldPacket data(SMSG_PET_BATTLE_FINAL_ROUND);
    // petsCount
    uint8 totalPetsCount = 0;
    uint32 bit_pos = data.bitwpos();
    data.WriteBits(totalPetsCount, 20);

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < 1; ++j)
        {
            PetBattleData* petData = GetPetBattleData(i, j);

            if (!petData)
                return;

            bool levelup = finalRound->isLevelUp(petData->GetPetNumber());

            data.WriteBit(0); // hasCaptured | hasCaged
            data.WriteBit(1); // hasSeenAction
            data.WriteBit(!finalRound->GetRewardedXP(petData->GetPetNumber())); // !hasXp
            data.WriteBit(0); // hasCaptured | hasCaged
            data.WriteBit(0); // !hasInitialLevel
            data.WriteBit(0); // !hasLevel
            data.WriteBit(finalRound->GetRewardedXP(petData->GetPetNumber()) || levelup); // awardedXP
        }
    }

    data.WriteBit(0); // pvpBattle

    // winners bits
    for (uint8 i = 0; i < 2; ++i)
        data.WriteBit(winners[i]);

    data.WriteBit(abandoned); // abandoned

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < 1; ++j)
        {
            PetBattleData* petData = GetPetBattleData(i, j);

            if (!petData)
                return;

            PetInfo* pet = petData->GetPetInfo();

            if (!pet)
                return;

            bool levelup = finalRound->isLevelUp(petData->GetPetNumber());
            uint16 rewardXP = finalRound->GetRewardedXP(petData->GetPetNumber());
            if (rewardXP)
                data << uint16(rewardXP);

            data << uint8(petData->GetPetNumber()); // Pboid
            data << uint32(pet->GetHealth()); // RemainingHealth
            data << uint16(pet->GetLevel());  // NewLevel
            data << uint32(pet->GetMaxHealth()); // maxHealth
            data << uint16(levelup ? pet->GetLevel()-1 : pet->GetLevel()); // InitialLevel

            totalPetsCount++;
        }
    }

    for (uint8 i = 0; i < 2; ++i)
        data << uint32(0); // NpcCreatureID

    data.PutBits<uint8>(bit_pos, totalPetsCount, 20);
    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::SendFinishPetBattle()
{
    WorldPacket data(SMSG_PET_BATTLE_FINISHED);
    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::FinishPetBattle()
{
    SendFinishPetBattle();
    m_player->GetBattlePetMgr()->CloseWildPetBattle();
}

void PetBattleAbility::CalculateAbilityData()
{
    for (uint32 i = 0; i < sBattlePetAbilityStore.GetNumRows(); ++i)
    {
        BattlePetAbilityEntry const* aEntry = sBattlePetAbilityStore.LookupEntry(i);

        if (!aEntry)
            continue;

        if (aEntry->ID == ID)
        {
            type = aEntry->Type;
            break;
        }
    }

    // maybe number of attack types?
    uint32 formulaValue = 0xA;
    for (uint8 i = 0; i < formulaValue; ++i)
    {
        uint32 modId = i * formulaValue + type;

        for (uint32 j = 0; j < sGtBattlePetTypeDamageModStore.GetNumRows(); ++j)
        {
            GtBattlePetTypeDamageModEntry const* gt = sGtBattlePetTypeDamageModStore.LookupEntry(j);

            if (!gt)
                continue;

            if (gt->Id == modId)
            {
                mods[i] = gt->value;
                break;
            }
        }
    }

    cooldownRemaining = 0;
    lockdownRemaining = 0;
}

uint32 PetBattleAbility::GetBasePoints(uint32 turnIndex, uint32 effectIdx)
{
    // get turn data entry
    for (uint32 i = 0; i < sBattlePetAbilityTurnStore.GetNumRows(); ++i)
    {
        BattlePetAbilityTurnEntry const* tEntry = sBattlePetAbilityTurnStore.LookupEntry(i);

        if (!tEntry)
            continue;

        if (tEntry->AbilityID == ID && tEntry->turnIndex == turnIndex)
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

uint32 PetBattleAbility::GetRequiredLevel()
{
    for (uint32 i = 0; i < sBattlePetSpeciesXAbilityStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesXAbilityEntry const* xEntry = sBattlePetSpeciesXAbilityStore.LookupEntry(i);

        if (!xEntry)
            continue;

        if (xEntry->abilityID == ID)
            return xEntry->requiredLevel;
    }

    return 0;
}

uint32 PetInfo::GetActiveAbilityID(uint8 rank)
{
    for (uint32 i = 0; i < sBattlePetSpeciesXAbilityStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesXAbilityEntry const* xEntry = sBattlePetSpeciesXAbilityStore.LookupEntry(i);

        if (!xEntry)
            continue;

        if (xEntry->speciesID == speciesID && xEntry->rank == rank)
        {
            if (rank == 0)
            {
                if (flags & BATTLE_PET_FLAG_CUSTOM_ABILITY_1)
                {
                    if (xEntry->requiredLevel == 10)
                        return xEntry->abilityID;
                    else
                        continue;
                }
                else
                {
                    if (xEntry->requiredLevel < 10)
                        return xEntry->abilityID;
                }
            }

            if (rank == 1)
            {
                if (flags & BATTLE_PET_FLAG_CUSTOM_ABILITY_2)
                {
                    if (xEntry->requiredLevel == 15)
                        return xEntry->abilityID;
                    else
                        continue;
                }
                else
                {
                    if (xEntry->requiredLevel < 15)
                        return xEntry->abilityID;
                }
            }

            if (rank == 2)
            {
                if (flags & BATTLE_PET_FLAG_CUSTOM_ABILITY_3)
                {
                    if (xEntry->requiredLevel == 20)
                        return xEntry->abilityID;
                    else
                        continue;
                }
                else
                {
                    if (xEntry->requiredLevel < 20)
                        return xEntry->abilityID;
                }
            }
        }
    }

    return 0;
}