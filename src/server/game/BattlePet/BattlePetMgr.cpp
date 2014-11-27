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

BattlePetMgr::BattlePetMgr(Player* owner) : m_player(owner)
{
    m_PetJournal.clear();
    memset(m_battleSlots, 0, sizeof(PetBattleSlot*)*MAX_PET_BATTLE_SLOT);
}

void BattlePetMgr::AddPetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName, int16 breedID, bool update)
{
    m_PetJournal[guid] = new PetInfo(speciesID, creatureEntry, level, display, power, speed, health, maxHealth, quality, xp, flags, spellID, customName, breedID, update);
}

void BattlePetMgr::AddPetBattleSlot(uint64 guid, uint8 slotID, bool locked)
{
    m_battleSlots[slotID] = new PetBattleSlot(guid, locked);
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
        if (pet->second->deleteMeLater)
            continue;

        ObjectGuid guid = pet->first;
        uint8 len = pet->second->customName == "" ? 0 : pet->second->customName.length();

        data->WriteBit(!pet->second->breedID);    // hasBreed, inverse
        data->WriteGuidMask<1, 5, 3>(guid);
        data->WriteBit(0);                        // has guid
        data->WriteBit(!pet->second->quality);    // hasQuality, inverse
        data->WriteGuidMask<6, 7>(guid);
        data->WriteBit(!pet->second->flags);      // has flags, inverse
        data->WriteGuidMask<0, 4>(guid);
        data->WriteBits(len, 7);                  // custom name length
        data->WriteGuidMask<2>(guid);
        data->WriteBit(1);                        // hasUnk, inverse
    }

    data->WriteBits(MAX_PET_BATTLE_SLOT, 25);     // total battle pet slot count

    for (uint32 i = 0; i < MAX_PET_BATTLE_SLOT; ++i)
    {
        data->WriteBit(!i);                                          // unk bit, related to Int8 (slot ID?)
        data->WriteBit(1);                                           // unk bit, related to Int32 (hasCustomAbility?)
        data->WriteBit(1);                                           // empty slot, inverse
        data->WriteGuidMask<7, 1, 3, 2, 5, 0, 4, 6>(placeholderPet); // pet guid in slot
        data->WriteBit(1);                                           // locked slot
    }

    data->WriteBit(1);                      // unk

    for (uint32 i = 0; i < 3; ++i)
    {
        //if (!bit)
            //*data << uint32(0);
        data->WriteGuidBytes<3, 7, 5, 1, 4, 0, 6, 2>(placeholderPet);
        if (i)
            *data << uint8(i);
    }

    for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
    {
        // prevent loading deleted pet
        if (pet->second->deleteMeLater)
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

    *data << uint16(0);         // unk
    data->PutBits<uint8>(bit_pos, count, 19);
}

void WorldSession::HandleSummonBattlePet(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 0, 1, 5, 3, 4, 7, 2>(guid);
    recvData.ReadGuidBytes<2, 5, 3, 7, 1, 0, 6, 4>(guid);

    // find pet
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->deleteMeLater)
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
                if (pet->deleteMeLater)
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

void WorldSession::HandleBattlePetPutInCage(WorldPacket& recvData)
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

        if (pet->deleteMeLater)
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

void WorldSession::HandleBattlePetOpcode166F(WorldPacket& recvData)
{
    float playerX, playerY, playerZ, playerOrient;
    float petAllyX, petEnemyX;
    float petAllyY, petEnemyY;
    float petAllyZ, petEnemyZ;
    uint32 unkCounter;

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

    bool facing = recvData.ReadBit();
    bool uunk = recvData.ReadBit();

    if (!uunk)
        recvData >> unkCounter;

    if (!facing)
        recvData >> playerOrient;

    WorldPacket data(SMSG_BATTLE_PET_FINALIZE_LOCATION);

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

    data << _player->GetOrientation();
    data << uint32(21);
    SendPacket(&data);

    // send full update
    WorldPacket data1(SMSG_BATTLE_PET_FULL_UPDATE);
    for (uint8 i = 0; i < 3; ++i)
    {
        data1.WriteBits(0, 21);
        data1.WriteBits(0, 21);
    }

    data1.WriteBit(0);
    data1.WriteBit(0);
    data1.WriteBit(0);

    ObjectGuid guid2 = _player->GetSelectedUnit()->GetObjectGuid();
    ObjectGuid guid3 = 0;
    ObjectGuid ownerGuid = _player->GetObjectGuid();
    // important strange GUID...
    // [0] Guid: Full: 0x1D3B6D0500000007 Type: 259 Low: 7
    // = player GUID with inverse byte order! WTF???
    // TEST
    ObjectGuid guid = ((ownerGuid & 0x00000000000000FF) << 56) | ((ownerGuid & 0x000000000000FF00) << 40) | ((ownerGuid & 0x0000000000FF0000) << 24) | ((ownerGuid & 0x00000000FF000000) <<  8) |
        ((ownerGuid & 0x000000FF00000000) >>  8) | ((ownerGuid & 0x0000FF0000000000) >> 24) | ((ownerGuid & 0x00FF000000000000) >> 40) | ((ownerGuid & 0xFF00000000000000) >> 56);
    ObjectGuid guid4 = 0;

    for (uint8 i = 0; i < 2; ++i)
    {
        if (i == 0)
            data1.WriteGuidMask<2>(guid);
        else
            data1.WriteGuidMask<2>(guid4);

        data1.WriteBit(1);

        if (i == 0)
            data1.WriteGuidMask<5, 7, 4>(guid);
        else
            data1.WriteGuidMask<5, 7, 4>(guid4);

        data1.WriteBit(0);

        if (i == 0)
            data1.WriteGuidMask<0>(guid);
        else
            data1.WriteGuidMask<0>(guid4);

        data1.WriteBits(1, 2);                              // pet count

        if (i == 0)
            data1.WriteGuidMask<1>(guid);
        else
            data1.WriteGuidMask<1>(guid4);

        //for (uint8 j = 0; j < 1; ++j)
        //{
            // TEST, i=0 - player, i=1 - wild pet
            if (i == 0)
            {
                data1.WriteGuidMask<3, 4, 0>(ownerGuid);
                data1.WriteBit(0);
                data1.WriteGuidMask<6>(ownerGuid);
                data1.WriteBits(6, 21);  // state count
                data1.WriteBits(1, 20);
                data1.WriteGuidMask<5>(ownerGuid);

                data1.WriteBit(0);
                data1.WriteGuidMask<2, 1, 7>(ownerGuid);
                data1.WriteBit(1);
                data1.WriteBits(0, 7);
                data1.WriteBits(0, 21);

                //data1.WriteBit(1);
                //data1.WriteBit(0);
                //data1.WriteBit(0);
            }
            else
            {
                data1.WriteGuidMask<3, 4, 0>(guid3);
                data1.WriteBit(0);
                data1.WriteGuidMask<6>(guid3);
                data1.WriteBits(5, 21);   // state count
                data1.WriteBits(1, 20);
                data1.WriteGuidMask<5>(guid3);

                data1.WriteBit(0);
                data1.WriteGuidMask<2, 1, 7>(guid3);
                data1.WriteBit(1);
                data1.WriteBits(0, 7);
                data1.WriteBits(0, 21);
            }
        //}

        if (i == 0)
            data1.WriteGuidMask<6>(guid);
        else
            data1.WriteGuidMask<6>(guid4);

        data1.WriteBit(0);

        if (i == 0)
            data1.WriteGuidMask<3>(guid);
        else
            data1.WriteGuidMask<3>(guid4);
    }

    data1.WriteBit(1);
    data1.WriteBit(0);
    data1.WriteBit(1);

    data1.WriteGuidMask<6, 0, 1, 4, 2, 5, 3, 7>(guid2);

    data1.WriteBit(0);
    data1.WriteBit(1);
    data1.WriteBit(0);

    for (uint8 i = 0; i < 2; ++i)
    {
        for (uint8 j = 0; j < 1; ++j)
        {
            // TEST, i=0 - player, i=1 - wild pet
            if (i == 0)
            {
                // 1 ability (for 1)
                uint32 abilityID = 114;
                data1 << uint8(0);
                data1 << uint16(0);
                data1 << uint8(0);  // slot index
                data1 << uint16(0);
                data1 << uint32(abilityID); // ability ID
            }
            else
            {
                // 1 ability (for 1)
                uint32 abilityID1 = 484;
                data1 << uint8(0);
                data1 << uint16(0);
                data1 << uint8(0);  // slot index
                data1 << uint16(0);
                data1 << uint32(abilityID1); // ability ID
            }

            data1 << uint16(25);  // experience
            data1 << uint32(250); // total HP

            if (i == 0)
                data1.WriteGuidBytes<1>(ownerGuid);
            else
                data1.WriteGuidBytes<1>(guid3);

            // States
            if (i == 0)
            {
                data1 << uint32(1600); // some fucking strange value!
                data1 << uint32(20);   // stateID from BattlePetState.db2 -> Stat_Speed
                data1 << uint32(1800); // some fucking strange value1!
                data1 << uint32(18);   // stateID from BattlePetState.db2 -> Stat_Power
                data1 << uint32(1700); // some fucking strange value2!
                data1 << uint32(19);   // stateID from BattlePetState.db2 -> Stat_Stamina
                data1 << uint32(5);    // crit chance % value
                data1 << uint32(40);   // stateID from BattlePetState.db2 -> Stat_CritChance
                data1 << uint32(1);    // enable
                data1 << uint32(45);   // stateID from BattlePetState.db2 -> Passive_Flying
                data1 << uint32(50);   // % value
                data1 << uint32(25);   // stateID from BattlePetState.db2 -> Mod_SpeedPercent
            }
            else
            {
                data1 << uint32(1900);
                data1 << uint32(19);   // stateID from BattlePetState.db2 -> Stat_Stamina
                data1 << uint32(1700);
                data1 << uint32(18);   // stateID from BattlePetState.db2 -> Stat_Power
                data1 << uint32(1600);
                data1 << uint32(20);   // stateID from BattlePetState.db2 -> Stat_Speed
                data1 << uint32(5);
                data1 << uint32(40);   // stateID from BattlePetState.db2 -> Stat_CritChance
                data1 << uint32(1);
                data1 << uint32(42);   // stateID from BattlePetState.db2 -> Passive_Critter
            }

            data1 << uint32(0);
            data1 << uint32(25); // speed

            // aura handle test
            // 239 - flying creature passive
            /*if (i == 0)
            {
                data1 << uint32(-1); // duration?
                data1 << uint32(1);
                data1 << uint8(0);
                data1 << uint32(239); // auraID = spellID
            }*/

            if (i == 0)
                data1.WriteGuidBytes<4>(ownerGuid);
            else
                data1.WriteGuidBytes<4>(guid3);

            // Custom Name
            //data1.WriteString("");
            // Quality
            data1 << uint16(3);

            if (i == 0)
                data1.WriteGuidBytes<0>(ownerGuid);
            else
                data1.WriteGuidBytes<0>(guid3);

            // current HP
            if (i == 0)
                data1 << uint32(250);
            else
                data1 << uint32(250);

            if (i == 0)
                data1.WriteGuidBytes<3>(ownerGuid);
            else
                data1.WriteGuidBytes<3>(guid3);

            // Species ID
            if (i == 0)
                data1 << uint32(292);
            else
                data1 << uint32(293);

            data1 << uint32(0);

            if (i == 0)
                data1.WriteGuidBytes<2, 6>(ownerGuid);
            else
                data1.WriteGuidBytes<2, 6>(guid3);

            // display ID?
            if (i == 0)
                data1 << uint32(36944);
            else
                data1 << uint32(1924);

            // Level
            if (i == 0)
                data1 << uint16(15);
            else
                data1 << uint16(15);

            if (i == 0)
                data1.WriteGuidBytes<7, 5>(ownerGuid);
            else
                data1.WriteGuidBytes<7, 5>(guid3);

            // Attack Power
            if (i == 0)
                data1 << uint32(25);
            else
                data1 << uint32(25);

            data1 << uint8(0); // pet slot index?
        }

        data1 << uint32(0);   // trap spell ID, default 427

        if (i == 0)
            data1.WriteGuidBytes<2, 5>(guid);
        else
            data1.WriteGuidBytes<2, 5>(guid4);
        
        data1 << uint32(2);

        if (i == 0)
            data1.WriteGuidBytes<4, 0, 7, 6>(guid);
        else
            data1.WriteGuidBytes<4, 0, 7, 6>(guid4);

        data1 << uint8(6);
        data1 << uint8(0);

        if (i == 0)
            data1.WriteGuidBytes<1, 3>(guid);
        else
            data1.WriteGuidBytes<1, 3>(guid4);
    }

    data1.WriteGuidBytes<6, 2, 1, 3, 0, 4, 7, 5>(guid2);

    data1 << uint16(30);
    data1 << uint16(30);
    data1 << uint8(1);
    data1 << uint32(0);
    data1 << uint8(10);

    SendPacket(&data1);
}

void WorldSession::HandleBattlePetReadyForBattle(WorldPacket& recvData)
{
    uint8 state;
    recvData >> state;

    if (!state)
    {
        WorldPacket data(SMSG_BATTLE_PET_FIRST_ROUND);
        for (uint8 i = 0; i < 2; ++i)
        {
            data << uint16(0);
            data << uint8(2);
            data << uint8(0);
        }

        data << uint32(0);
        data.WriteBits(0, 3);
        data.WriteBit(0);
        data.WriteBits(2, 22);

        for (uint8 i = 0; i < 2; ++i)
        {
            data.WriteBit(0);
            data.WriteBit(1);
            data.WriteBit(1);
            data.WriteBits(1, 25);

            data.WriteBit(1);

            // for bits25
            data.WriteBit(0);
            data.WriteBits(3, 3);
            //

            data.WriteBit(1);
            data.WriteBit(1);
            data.WriteBit(0);
        }

        data.WriteBits(0, 20);

        // for
        data << uint8(0); //0
        data << uint8(0); //0 0
        data << uint8(4); //0

        data << uint8(3); //1
        data << uint8(3); //1 0
        data << uint8(4); //1
        //
        data << uint8(2);
        /*for (uint8 i = 0; i < 2; ++i)
        {
            data << uint16(0);
            data << uint8(2);
            data << uint8(0);
        }
        data << uint32(0);
        data.WriteBits(0, 3);
        data.WriteBit(0);
        data.WriteBits(4, 22);            // effect count
        uint32 bit[4] = {1, 1, 0, 0};
        uint32 bit1[4] = {1, 1, 1, 1};
        uint32 bit2[4] = {0, 0, 1, 1};
        uint32 bits25[4] = {1, 1, 1, 1};
        uint32 bit3[4] = {0, 0, 1, 1};
        //
        uint32 bit4[4] = {0, 0, 1, 1};
        uint32 bit5[4] = {0, 0, 1, 1};
        uint32 bit6[4] = {0, 0, 1, 1};
        for (uint8 i = 0; i < 4; ++i)
        {
            data.WriteBit(bit[i]);
            data.WriteBit(bit1[i]);
            data.WriteBit(bit2[i]);
            data.WriteBits(bits25[i], 25);
            data.WriteBit(bit3[i]);

            for (uint8 j = 0; j < 1; ++j)
            {
                if (i == 0 || i == 1)
                    data.WriteBit(0);
                else
                    data.WriteBit(1);

                if (i == 0 || i == 1)
                    data.WriteBits(6, 3);
                else
                    data.WriteBits(3, 3);

                if (i == 0 || i == 1)
                    data.WriteBit(0);
            }

            data.WriteBit(bit4[i]);
            data.WriteBit(bit5[i]);
            data.WriteBit(bit6[i]);
        }

        for (uint8 i = 0; i < 4; ++i)
        {
            if (!bit1[i])
                data << uint16(0);

            for (uint8 j = 0; j < 1; ++j)
            {
                if (i == 0)
                    data << uint32(58);
                else if (i == 1)
                    data << uint32(30);

                if (i == 0)
                    data << uint8(0);
                else if (i == 1)
                    data << uint8(3);
            }

            if (!bit3[i])
            {
                if (i == 0)
                    data << uint16(1);
                else if (i == 1)
                    data << uint16(2);
            }

            if (!bit2[i])
                data << uint32(379);

            if (!bit6[i])
            {
                if (i == 0)
                    data << uint8(3);
                else if (i == 1)
                    data << uint8(0);
            }

            if (!bit5[i])
                data << uint8(1);

            if (!bit4[i])
                data << uint32(4096);

            if (!bit[i])
            {
                if (i == 2)
                    data << uint8(13);
                else if (i == 3)
                    data << uint8(14);
            }
        }
        data << uint8(2);*/
        SendPacket(&data);
    }
}

void WorldSession::HandleBattlePetOpcode1ACF(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 2, 3, 7, 0, 4>(guid);
    recvData.ReadBit();
    recvData.ReadGuidMask<5, 1>(guid);
    recvData.ReadGuidBytes<3, 5, 6, 7, 1, 0, 2, 4>(guid);

    sLog->outError(LOG_FILTER_GENERAL, "GUID from packet 0x1ACF - %u", uint64(guid));
}

void WorldSession::HandleBattlePetUseAction(WorldPacket& recvData)
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
    if (!abilityID && !bit6)
        return;

    // finish
    if (bit6 && endGame == 4)
    {
        _player->GetBattlePetMgr()->SendClosePetBattle();
        return;
    }

    WorldPacket data(SMSG_BATTLE_PET_ROUND_RESULT);
    data.WriteBit(0);
    data.WriteBits(0, 20);
    data.WriteBits(4, 22);
    // 0
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBits(1, 25);
    data.WriteBits(6, 3);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    // 1
    /*data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBits(1, 25);
    data.WriteBits(0, 3);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(1);*/
    // 2
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBits(1, 25);
    data.WriteBits(6, 3);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    // 3
    /*data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBits(1, 25);
    data.WriteBits(4, 3);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);*/
    // 4
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBits(1, 25);
    data.WriteBits(3, 3);
    data.WriteBit(1);
    data.WriteBit(1);
    // 5
    /*data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBits(1, 25);
    data.WriteBits(0, 3);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);*/
    // 6
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBits(1, 25);
    data.WriteBits(3, 3);
    data.WriteBit(1);
    data.WriteBit(1);
    // 7
    /*data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBits(1, 25);
    data.WriteBits(1, 3);
    data.WriteBit(0);
    data.WriteBit(0);
    data.WriteBit(1);*/

    data.WriteBits(0, 3);

    uint32 remHp1 = 250;
    uint32 remHp2 = 250;

    uint32 winner = 2;

    // test calc damage - 2 round battle ended
    if (roundID == 0)
    {
        remHp1 = 250 - urand(50, 95);
        remHp2 = 250 - urand(50, 95);
    }
    else if (roundID == 1)
    {
        // roll winner
        winner = urand(0, 1);

        if (winner)
        {
            remHp2 = -2;
            remHp1 = 34;
        }
        else
        {
            remHp2 = 35;
            remHp1 = -1;
        }
    }

    // 0
    uint16 val7 = 1;       // opponent index
    uint8 val18 = 3;       // target ID (0,1,2 - 1 opponent | 3,4,5 - 2 opponent)
    uint32 val9 = 2;       // remaining health
    uint16 val10 = 4100;   // attack flags
    uint8 val11 = 0;       // attacker ID (0,1,2 - 1 opponent | 3,4,5 - 2 opponent)
    uint8 val12 = 1;
    uint32 val13 = 281;    // effect ID
    data << uint16(val7);  // opponent index
    data << uint8(val18);  // target ID (0,1,2 - 1 opponent | 3,4,5 - 2 opponent)
    data << int32(remHp1);  // remaining health
    data << uint16(val10); // attack flags
    data << uint8(val11);  // attacker ID (0,1,2 - 1 opponent | 3,4,5 - 2 opponent)
    data << uint8(val12);
    data << uint32(val13); // effect ID
    // 1
    /*uint16 val8 = 1;
    uint8 val = 0;
    uint8 val1 = 3;
    uint8 val2 = 1;
    uint8 val3 = 2;
    uint32 val4 = 239;
    uint32 val5 = 1;
    uint32 val6 = -1;
    uint32 val7 = 379;
    data << uint16(val8);
    data << uint8(val);
    data << uint32(val4);
    data << uint32(val5);
    data << uint32(val6);
    data << uint8(val1);
    data << uint8(val2);
    data << uint32(val7);
    data << uint8(val3);*/
    // 2
    uint16 val = 2;      // opponent index
    uint8 val1 = 0;      // target ID (0,1,2 - 1 opponent | 3,4,5 - 2 opponent)
    uint32 val2 = 30;    // remaining health
    uint16 val3 = 4096;  // attack flags
    uint8 val4 = 3;      // attacker ID (0,1,2 - 1 opponent | 3,4,5 - 2 opponent)
    uint8 val5 = 1;
    uint32 val6 = 769;   // effect ID
    data << uint16(val);
    data << uint8(val1);
    data << int32(remHp2);
    data << uint16(val3);
    data << uint8(val4);
    data << uint8(val5);
    data << uint32(val6);
    // 3
    /*data << uint16(1);
    data << uint32(1);
    data << uint32(1);
    data << uint8(0);
    data << uint8(3);
    data << uint8(1);
    data << uint32(1987);
    data << uint8(6);*/
    // 4
    data << uint8(13);
    // 5
    /*data << uint8(4);
    data << uint32(1);
    data << uint32(239);
    data << uint32(2);
    data << uint32(-1);
    data << uint8(4);
    data << uint8(3);*/
    // 6
    data << uint8(14);
    // 7
    /*data << uint16(2);
    data << uint8(0);
    data << uint32(10); // speed (some value related to ...)
    data << uint8(0);
    data << uint8(1);
    data << uint8(8);*/

    // for 2
    data << uint8(0);
    data << uint16(0);
    data << uint8(2);
    // ---
    data << uint8(0);
    data << uint16(0);
    data << uint8(2);
    //

    roundID++;
    data << uint32(roundID);     // Round ID
    data << uint8(2);

    SendPacket(&data);

    // final
    if (roundID == 2)
    {
        if (winner == 2)
            return;

        WorldPacket data1(SMSG_BATTLE_PET_FINAL_ROUND);
        data1.WriteBits(2, 20);

        for (uint8 i = 0; i < 2; i++)
        {
            data1.WriteBit(0);
            data1.WriteBit(1);

            if (i == 0)
                data1.WriteBit(winner ? 1 : 0);
            else
                data1.WriteBit(1);

            data1.WriteBit(0);
            data1.WriteBit(0);
            data1.WriteBit(0);
            data1.WriteBit(0);
        }

        data1.WriteBit(0);

        // winner bit
        for (uint8 i = 0; i < 2; i++)
        {
            if (i == 0)
                data1.WriteBit(winner ? 0 : 1);
            else
                data1.WriteBit(winner ? 1 : 0);
        }

        // forfeit/abandoned bit
        data1.WriteBit(0);

        for (uint8 i = 0; i < 2; i++)
        {
            // experience
            if (!winner && i == 0)
                data1 << uint16(15);

            // pet ID
            if (i == 0)
                data1 << uint8(0);
            else
                data1 << uint8(3);

            // remaining health
            if (i == 0)
            {
                if (!winner)
                {
                    data1 << int32(35);
                }
                else
                {
                    data1 << int32(-2);
                }
            }
            else
            {
                if (!winner)
                {
                    data1 << int32(-1);
                }
                else
                {
                    data1 << int32(34);
                }
            }

            // level after battle
            if (i == 0)
            {
                if (!winner)
                {
                    data1 << uint16(16);
                }
                else
                {
                    data1 << uint16(15);
                }
            }
            else
            {
                data1 << uint16(15);
            }

            // total health
            data1 << uint32(250);
            // level before battle
            data1 << uint16(15);
        }

        data1 << uint32(0);
        data1 << uint32(0);

        SendPacket(&data1);
    }
}

void BattlePetMgr::SendClosePetBattle()
{
    WorldPacket data(SMSG_BATTLE_PET_BATTLE_FINISHED);
    m_player->GetSession()->SendPacket(&data);
}

// packet updates pet stats after finish battle and other actions (renaming?)
void BattlePetMgr::SendUpdatePets()
{
    uint32 count = 0;
    WorldPacket data(SMSG_BATTLE_PET_UPDATES);
    data.WriteBit(1);
    uint32 bit_pos = data.bitwpos();
    data.WriteBits(0, 19);                              // placeholder, count of update pets
    for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
    {
        if (!pet->second->sendUpdate || pet->second->deleteMeLater)
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
        if (!pet->second->sendUpdate || pet->second->deleteMeLater)
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
        pet->second->sendUpdate = false;
    }

    data.PutBits<uint8>(bit_pos, count, 19);
    m_player->GetSession()->SendPacket(&data);
}

void WorldSession::HandleBattlePetRename(WorldPacket& recvData)
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

    if (!pet || pet->deleteMeLater)
        return;

    // set custom name
    pet->SetCustomName(customName);
}

void WorldSession::HandleBattlePetSetData(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 type;
    recvData >> type;
    recvData.ReadGuidMask<2, 6, 1, 7, 0>(guid);
    uint32 val = recvData.ReadBits(2);
    recvData.ReadGuidMask<3, 4, 5>(guid);
    recvData.ReadGuidBytes<3, 5, 2, 1, 0, 6, 7, 4>(guid);

    // find pet
    PetInfo* pet = _player->GetBattlePetMgr()->GetPetInfoByPetGUID(guid);

    if (!pet || pet->deleteMeLater)
        return;

    // set data (only for testing)
    // type=1 - flags
    if (type == 1)
        pet->SetFlags(val);
}

void BattlePetMgr::GiveXP()
{
}
