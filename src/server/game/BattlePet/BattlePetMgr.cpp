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

#include "WorldPacket.h"
#include "Player.h"
#include "BattlePetMgr.h"

BattlePetMgr::BattlePetMgr(Player* owner) : m_player(owner), m_petBattleWild(NULL)
{
    // clear journal
    m_PetJournal.clear();
    // clear slots
    m_battleSlots.clear();
}

void BattlePetMgr::AddPetToList(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName, int16 breedID, uint8 state)
{
    m_PetJournal[guid] = new PetJournalInfo(speciesID, creatureEntry, level, display, power, speed, health, maxHealth, quality, xp, flags, spellID, customName, breedID, state);
}

void BattlePetMgr::InitBattleSlot(uint64 guid, uint8 index)
{
    m_battleSlots[index] = new PetBattleSlot(guid);
}

bool BattlePetMgr::SlotIsLocked(uint8 index)
{
    bool locked = true;

    switch (index)
    {
        case 0: locked = !m_player->HasSpell(119467); break;
        case 1: locked = !m_player->HasAchieved(7433), false; break;
        case 2: locked = !m_player->HasAchieved(6566), false; break;
        default: break;
    }

    return locked;
}

bool BattlePetMgr::BuildPetJournal(WorldPacket *data)
{
    uint32 count = 0;

    ByteBuffer bitData;
    ByteBuffer byteData;

    uint32 countPos = bitData.bitwpos();
    bitData.WriteBits(0, 19);

    for (auto itr : m_PetJournal)
    {
        PetJournalInfo* petInfo = itr.second;

        // prevent crash
        if (!petInfo)
            return false;

        // prevent loading deleted pet
        if (petInfo->GetState() == STATE_DELETED)
            continue;

        ObjectGuid guid = itr.first;
        uint8 len = petInfo->GetCustomName() == "" ? 0 : petInfo->GetCustomName().length();

        bitData.WriteBit(!petInfo->GetBreedID());     // hasBreed, inverse
        bitData.WriteGuidMask<1, 5, 3>(guid);
        bitData.WriteBit(0);                          // hasOwnerGuidInfo
        bitData.WriteBit(!petInfo->GetQuality());     // hasQuality, inverse
        bitData.WriteGuidMask<6, 7>(guid);
        bitData.WriteBit(!petInfo->GetFlags());       // hasFlags, inverse
        bitData.WriteGuidMask<0, 4>(guid);
        bitData.WriteBits(len, 7);                    // custom name length
        bitData.WriteGuidMask<2>(guid);
        bitData.WriteBit(1);                          // hasUnk (bool noRename?), inverse

        if (petInfo->GetBreedID())
            byteData << uint16(petInfo->GetBreedID());            // breedID
        byteData << uint32(petInfo->GetSpeciesID());              // speciesID
        byteData << uint32(petInfo->GetSpeed());                  // speed
        byteData.WriteGuidBytes<1, 6, 4>(guid);
        byteData << uint32(petInfo->GetDisplayID());
        byteData << uint32(petInfo->GetMaxHealth());              // max health
        byteData << uint32(petInfo->GetPower());                  // power
        byteData.WriteGuidBytes<2>(guid);
        byteData << uint32(petInfo->GetCreatureEntry());          // Creature ID
        byteData << uint16(petInfo->GetLevel());                  // level
        if (len > 0)
            byteData.WriteString(petInfo->GetCustomName());       // custom name
        byteData << uint32(petInfo->GetHealth());                 // health
        byteData << uint16(petInfo->GetXP());                     // xp
        if (petInfo->GetQuality())
            byteData << uint8(petInfo->GetQuality());             // quality
        byteData.WriteGuidBytes<3, 7, 0>(guid);
        if (petInfo->GetFlags())
            byteData << uint16(petInfo->GetFlags());              // flags
        byteData.WriteGuidBytes<5>(guid);

        ++count;
    }

    bitData.WriteBits(MAX_ACTIVE_BATTLE_PETS, 25);

    // fill battle slots data
    ByteBuffer byteSlotData;

    // pointer to last element
    auto itr2 = m_battleSlots.end();
    if (!m_battleSlots.empty())
        --itr2;

    for (auto itr : m_battleSlots)
    {
        uint8 slotIndex = itr.first;
        PetBattleSlot* slot = itr.second;

        if (!slot)
            return false;

        bitData.WriteBit(!slotIndex);                                      // hasSlotIndex
        bitData.WriteBit(1);                                               // hasCollarID, inverse
        bitData.WriteBit(!slot->IsEmpty());                                // empty slot, inverse
        bitData.WriteGuidMask<7, 1, 3, 2, 5, 0, 4, 6>(slot->GetPet());     // pet guid in slot
        bitData.WriteBit(SlotIsLocked(slotIndex));                         // locked slot

        if (itr.first == itr2->first)
            bitData.WriteBit(1);                      // !hasJournalLock

        byteSlotData.WriteGuidBytes<3, 7, 5, 1, 4, 0, 6, 2>(slot->GetPet());
        if (slotIndex)
            byteSlotData << uint8(slotIndex);        // SlotIndex
    }

    bitData.FlushBits();
    bitData.PutBits<uint32>(countPos, count, 19);

    data->Initialize(SMSG_BATTLE_PET_JOURNAL);
    data->append(bitData);
    data->append(byteSlotData);
    data->append(byteData);

    *data << uint16(0);                                        // trapLevel
    return true;
}

void BattlePetMgr::SendEmptyPetJournal()
{
    WorldPacket data(SMSG_BATTLE_PET_JOURNAL);
    data.WriteBits(0, 19);
    data.WriteBits(0, 25);
    data.WriteBit(0);
    data << uint16(0);

    m_player->GetSession()->SendPacket(&data);
}

void BattlePetMgr::CloseWildPetBattle()
{
    // remove battle object
    delete m_petBattleWild;
    m_petBattleWild = NULL;
}

// packet updates pet stats after finish battle and other actions (renaming?)
void BattlePetMgr::SendUpdatePets(std::list<uint64> &updates, bool added)
{
    ByteBuffer bitData;
    ByteBuffer byteData;
    int32 realCount = updates.size();

    bitData.WriteBit(added);
    uint32 countPos = bitData.bitwpos();
    bitData.WriteBits(updates.size(), 19);

    for (auto i : updates)
    {
        PetJournalInfo* petInfo = GetPetInfoByPetGUID(i);

        if (!petInfo || petInfo->GetState() == STATE_DELETED)
        {
            --realCount;

            // impossible, but....not sended packet in this case
            if (realCount < 0)
                return;

            continue;
        }

        ObjectGuid guid = i;
        uint8 len = petInfo->GetCustomName() == "" ? 0 : petInfo->GetCustomName().length();

        bitData.WriteBits(len, 7);                         // custom name length
        bitData.WriteBit(!petInfo->GetFlags());            // !hasFlags
        bitData.WriteBit(1);                               // !hasUnk
        bitData.WriteGuidMask<2>(guid);
        bitData.WriteBit(!petInfo->GetBreedID());          // !hasBreed
        bitData.WriteBit(!petInfo->GetQuality());          // !hasQuality
        bitData.WriteGuidMask<1, 6, 3, 7, 0, 4, 5>(guid);
        bitData.WriteBit(0);                               // hasGuid

        /*if (hasGuid)
        {
        bitData.WriteGuidMask<7, 0, 6, 2, 1, 3, 5, 4>(petGuid2);
        }*/

        /*if (hasGuid)
        {
        byteData << uint32(0); // unk
        byteData.WriteGuidBytes<0, 1, 2, 3, 4, 5, 7, 6>(petGuid2);
        byteData << uint32(0); // unk1
        }*/

        byteData << uint16(petInfo->GetLevel());                  // level
        byteData << uint32(petInfo->GetMaxHealth());              // total health
        if (petInfo->GetFlags())
            byteData << uint16(petInfo->GetFlags());              // flags
        if (petInfo->GetQuality())
            byteData << uint8(petInfo->GetQuality());             // quality
        byteData.WriteGuidBytes<3>(guid);
        byteData << uint32(petInfo->GetCreatureEntry());          // creature ID
        byteData << uint32(petInfo->GetSpeed());                  // speed
        if (petInfo->GetBreedID())
            byteData << uint16(petInfo->GetBreedID());            // breed ID
        byteData << uint32(petInfo->GetHealth());                 // remaining health
        byteData << uint32(petInfo->GetDisplayID());              // display ID
        if (len > 0)
            byteData.WriteString(petInfo->GetCustomName());       // custom name
        byteData.WriteGuidBytes<5, 4, 7>(guid);
        byteData << uint32(petInfo->GetSpeciesID());              // species ID
        byteData.WriteGuidBytes<2, 6>(guid);
        byteData << uint16(petInfo->GetXP());                     // experience
        byteData.WriteGuidBytes<0, 1>(guid);
        byteData << uint32(petInfo->GetPower());                  // power
    }

    bitData.FlushBits();
    // prevent damage update packet
    if (updates.size() != realCount)
        bitData.PutBits<uint32>(countPos, realCount, 19);

    WorldPacket data(SMSG_BATTLE_PET_UPDATES);
    data.append(bitData);
    data.append(byteData);

    m_player->GetSession()->SendPacket(&data);
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

uint8 BattlePetMgr::GetRandomQuailty()
{
    // 42% - grey, 33% - white, 19% - green, 6% - rare
    uint32 r = urand(0, 1000);

    uint8 quality = 0;
    if (r >= 420 && r < 750)
        quality = 1;
    else if (r >= 750 && r < 940)
        quality = 2;
    else if (r >= 940 && r <= 1000)
        quality = 3;

    return quality;
}

uint16 BattlePetMgr::GetRandomBreedID(uint32 speciesID)
{
    if (std::vector<uint32> const* breeds = sObjectMgr->GetPossibleBreedsForSpecies(speciesID))
    {
        uint32 sum = 0;
        for (auto itr : *breeds)
            sum += GetWeightForBreed(itr);

        uint32 r = urand(0, sum);
        uint32 current_sum = 0;

        for (auto itr : *breeds)
        {
            uint16 breedID = itr;
            if (current_sum <= r && r < current_sum + GetWeightForBreed(breedID))
                return breedID;

            current_sum += GetWeightForBreed(breedID);
        }
    }

    return 0;
}

void BattlePetMgr::CreateWildBattle(Player* initiator, ObjectGuid wildCreatureGuid)
{
    delete m_petBattleWild;
    m_petBattleWild = new PetBattleWild(initiator);
    m_petBattleWild->Init(wildCreatureGuid);
}

void BattlePetMgr::SendPetBattleRequestFailed(uint8 reason)
{
    WorldPacket data(SMSG_PET_BATTLE_REQUEST_FAILED);
    data.WriteBit(0);
    data << uint8(reason);
    m_player->GetSession()->SendPacket(&data);
}

bool BattlePetMgr::AllSlotsEmpty()
{
    for (auto s : m_battleSlots)
    {
        PetBattleSlot * slot = s.second;

        if (!slot)
            return true;

        if (!slot->IsEmpty())
            return false;
    }

    return true;
}

bool BattlePetMgr::AllSlotsDead()
{
    for (auto s : m_battleSlots)
    {
        PetBattleSlot * slot = s.second;

        if (!slot)
            return true;

        if (!slot->IsEmpty())
        {
            if (PetJournalInfo * pet = GetPetInfoByPetGUID(slot->GetPet()))
            {
                if (!pet->IsDead())
                    return false;
            }
        }
        else
            return false;
    }

    return true;
}

bool BattlePetMgr::PetIsSlotted(uint64 guid)
{
    for (auto s : m_battleSlots)
    {
        if (PetBattleSlot * slot = s.second)
        {
            if (slot->GetPet() == guid)
                return true;
        }
    }

    return false;
}

// BattlePetStatAccumulator
BattlePetStatAccumulator::BattlePetStatAccumulator(uint32 _speciesID, uint16 _breedID) : healthMod(0), powerMod(0), speedMod(0), qualityMultiplier(0.0f)
{
    BattlePetSpeciesStateBySpecMapBounds bounds = sBattlePetSpeciesStateBySpecId.equal_range(_speciesID);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        Accumulate(std::get<0>(itr->second), std::get<1>(itr->second));

    BattlePetBreedStateByBreedMapBounds bounds1 = sBattlePetBreedStateByBreedId.equal_range(_breedID);
    for (auto itr1 = bounds1.first; itr1 != bounds1.second; ++itr1)
        Accumulate(std::get<0>(itr1->second), std::get<1>(itr1->second));
}

// PetBattleWild
PetBattleWild::PetBattleWild(Player* owner) : m_player(owner)
{
    battleInfo.clear();

    nextRoundFinal = false;
    abandoned = false;
}

bool PetBattleWild::PrepareBattleInfo(ObjectGuid creatureGuid)
{
    int8 petID = 0;

    // PLAYER (copying data from journal and other sources)
    for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
    {
        if (PetBattleSlot* _slot = m_player->GetBattlePetMgr()->GetPetBattleSlot(i))
        {
            if (!_slot->IsEmpty())
            {
                if (PetJournalInfo* petInfo = m_player->GetBattlePetMgr()->GetPetInfoByPetGUID(_slot->GetPet()))
                {
                    // dead or deleted pets are not valid
                    if (petInfo->IsDead() || petInfo->GetState() == STATE_DELETED)
                        continue;

                    PetBattleInfo* pbInfo = new PetBattleInfo();

                    pbInfo->SetPetID(petID);
                    pbInfo->SetTeam(TEAM_ALLY);
                    pbInfo->SetGUID(_slot->GetPet());

                    pbInfo->CopyPetInfo(petInfo);

                    for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++j)
                    {
                        uint32 ID = petInfo->GetAbilityID(j); 
                        pbInfo->SetAbilityID(ID, j);
                    }

                    ++petID;

                    // added to array
                    battleInfo.push_back(pbInfo);
                }
            }
        }
    }

    // NPC (creating new data)
    Creature * wildPet = m_player->GetMap()->GetCreature(creatureGuid);

    if (!wildPet)
        return false;

    uint8 wildPetLevel = wildPet->GetUInt32Value(UNIT_FIELD_WILD_BATTLE_PET_LEVEL);

    BattlePetSpeciesEntry const* s = m_player->GetBattlePetMgr()->GetBattlePetSpeciesEntry(wildPet->GetEntry());

    if (!s)
        return false;

    CreatureTemplate const* t = sObjectMgr->GetCreatureTemplate(wildPet->GetEntry());

    if (!t)
        return false;

    // roll creature count
    uint32 creatureCount = 1;
    if (wildPetLevel > 5)
        creatureCount = 2;
    else if (wildPetLevel > 10)
        creatureCount = 3;

    // set enemy PetID
    petID = 3;

    for (uint8 i = 0; i < creatureCount; ++i)
    {
        // roll random quality and breed
        uint8 quality = m_player->GetBattlePetMgr()->GetRandomQuailty();
        uint16 breedID = m_player->GetBattlePetMgr()->GetRandomBreedID(s->ID);

        BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(s->ID, breedID);
        accumulator->CalcQualityMultiplier(quality, wildPetLevel);
        uint32 health = accumulator->CalculateHealth();
        uint32 power = accumulator->CalculatePower();
        uint32 speed = accumulator->CalculateSpeed();
        delete accumulator;

        PetBattleSlot* slot = new PetBattleSlot(0);
        PetJournalInfo* petInfo = new PetJournalInfo(s->ID, wildPet->GetEntry(), wildPetLevel, t->Modelid1, power, speed, health, health, quality, 0, 0, s->spellId, "", breedID, STATE_NORMAL);
        PetBattleInfo* pbInfo = new PetBattleInfo();

        pbInfo->SetPetID(petID);
        pbInfo->SetTeam(TEAM_ENEMY);
        pbInfo->SetGUID(slot->GetPet());

        pbInfo->CopyPetInfo(petInfo);

        for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++j)
        {
            uint32 ID = petInfo->GetAbilityID(j);
            pbInfo->SetAbilityID(ID, j);
        }

        delete slot;
        delete petInfo;

        ++petID;

        // added to array
        battleInfo.push_back(pbInfo);
    }

    SetBattleState(1);
    SetCurrentRoundID(0);

    return true;
}

uint8 PetBattleWild::GetTotalPetCountInTeam(uint8 team, bool onlyActive)
{
    uint8 count = 0;

    for (auto itr : battleInfo)
    {
        PetBattleInfo * pb = itr;

        if (!pb || pb->GetTeam() != team)
            continue;

        if (onlyActive)
        {
            if (pb->IsDead() || pb->Captured() || pb->Caged())
                continue;
        }

        ++count;
    }

    return count;
}

PetBattleInfo* PetBattleWild::GetPet(uint8 petNumber)
{
    for (auto itr : battleInfo)
    {
        PetBattleInfo * pb = itr;

        if (!pb || pb->GetPetID() != petNumber)
            continue;

        return pb;
    }

    return NULL;
}

PetBattleInfo* PetBattleWild::GetFrontPet(uint8 team)
{
    for (auto itr : battleInfo)
    {
        PetBattleInfo * pb = itr;

        if (!pb || pb->GetTeam() != team || !pb->IsFrontPet())
            continue;

        return pb;
    }

    return NULL;
}

void PetBattleWild::SetFrontPet(uint8 team, uint8 petNumber)
{
    PetBattleInfo * pb = GetFrontPet(team);

    if (!pb)
        return;

    pb->SetFrontPet(false);

    PetBattleInfo * pb1 = GetPet(petNumber);

    if (!pb1)
        return;

    pb1->SetFrontPet(true);
}

void PetBattleWild::SendFullUpdate(ObjectGuid creatureGuid)
{
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
        data.WriteBits(GetTotalPetCountInTeam(i), 2);
        data.WriteGuidMask<1>(teamGuids[i]);

        for (auto itr : battleInfo)
        {
            PetBattleInfo * pb = itr;

            // check vaild
            if (!pb || pb->GetTeam() != i)
                continue;

            uint64 guid = pb->GetGUID();

            data.WriteGuidMask<3, 4, 0>(guid);
            data.WriteBit(0);
            data.WriteGuidMask<6>(guid);
            data.WriteBits(4, 21);                   // state count
            data.WriteBits(1, 20);                   // abilities count
            data.WriteGuidMask<5>(guid);

            data.WriteBit(0);

            data.WriteGuidMask<2, 1, 7>(guid);
            data.WriteBit(1);

            uint8 len = pb->GetCustomName() == "" ? 0 : pb->GetCustomName().length();
            data.WriteBits(len, 7);
            data.WriteBits(0, 21);                   // auras count
        }

        data.WriteGuidMask<6>(teamGuids[i]);
        data.WriteBit(0);
        data.WriteGuidMask<3>(teamGuids[i]);
    }

    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(1);

    data.WriteGuidMask<6, 0, 1, 4, 2, 5, 3, 7>(creatureGuid);

    data.WriteBit(0);
    data.WriteBit(1);
    data.WriteBit(0);

    for (uint8 i = 0; i < 2; ++i)
    {
        for (auto itr : battleInfo)
        {
            PetBattleInfo * pb = itr;

            // check vaild
            if (!pb || pb->GetTeam() != i)
                continue;

            uint64 guid = pb->GetGUID();

            // abilities
            for (uint8 k = 0; k < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++k)
            {
                if (k != 0)
                    break;

                data << uint8(0);
                data << uint16(0);
                data << uint8(0);
                data << uint16(0);
                data << uint32(pb->GetAbilityID(k));
                /*if (i == 0)
                    data << uint32(204);
                else
                    data << uint32(battleInfo[i][j]->GetAbilityInfoByIndex(k)->GetID());*/
            }

            data << uint16(pb->GetXP());
            data << uint32(pb->GetMaxHealth());

            data.WriteGuidBytes<1>(guid);

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
            data << uint32(pb->GetSpeed()); // speed

            // auras
            //

            data.WriteGuidBytes<4>(guid);
            uint8 len = pb->GetCustomName() == "" ? 0 : pb->GetCustomName().length();
            if (len > 0)
                data.WriteString(pb->GetCustomName());
            data << uint16(pb->GetQuality());
            data.WriteGuidBytes<0>(guid);
            data << uint32(pb->GetHealth());
            data.WriteGuidBytes<3>(guid);
            data << uint32(pb->GetSpeciesID());
            data << uint32(0);
            data.WriteGuidBytes<2, 6>(guid);
            data << uint32(pb->GetDisplayID());
            data << uint16(pb->GetLevel());
            data.WriteGuidBytes<7, 5>(guid);
            data << uint32(pb->GetPower());
            uint8 slot = GetPetTeamIndex(pb->GetPetID());
            data << uint8(slot);                       // Slot
        }

        data << uint32(427);                           // TrapSpellID
        data.WriteGuidBytes<2, 5>(teamGuids[i]);
        data << uint32(2);                             // TrapStatus

        data.WriteGuidBytes<4, 0, 7, 6>(teamGuids[i]);
        data << uint8(6);                              // InputFlags
        data << uint8(0);                              // FrontPet
        data.WriteGuidBytes<1, 3>(teamGuids[i]);
    }

    data.WriteGuidBytes<6, 2, 1, 3, 0, 4, 7, 5>(creatureGuid);

    data << uint16(30);                                // WaitingForFrontPetsMaxSecs | PvpMaxRoundTime
    data << uint16(30);                                // WaitingForFrontPetsMaxSecs | PvpMaxRoundTime
    data << uint8(GetBattleState());                   // CurPetBattleState
    data << uint32(0);                                 // CurRound
    data << uint8(10);                                 // ForfeitPenalty

    m_player->GetSession()->SendPacket(&data);
}

void PetBattleInfo::CopyPetInfo(PetJournalInfo* petInfo)
{
    // copying from journal
    speciesID = petInfo->GetSpeciesID();
    summonSpellID = petInfo->GetSummonSpell();
    creatureEntry = petInfo->GetCreatureEntry();
    displayID = petInfo->GetDisplayID();
    customName = petInfo->GetCustomName();
    breedID = petInfo->GetBreedID();
    health = petInfo->GetHealth();
    maxHealth = petInfo->GetMaxHealth();
    speed = petInfo->GetSpeed();
    power = petInfo->GetPower();
    flags = petInfo->GetFlags();
    quality = petInfo->GetQuality();
    level = petInfo->GetLevel();
    xp = petInfo->GetXP();
    // for final stage
    totalXP = petInfo->GetXP();
    newLevel = petInfo->GetLevel();

    frontPet = false;
    captured = false;
    caged = false;
}

void PetBattleWild::Init(ObjectGuid creatureGuid)
{
    if (!PrepareBattleInfo(creatureGuid))
        return;

    SendFullUpdate(creatureGuid);
}

bool PetBattleWild::FirstRoundHandler(uint8 allyFrontPetID, uint8 enemyFrontPetID)
{
    PetBattleRoundResults* firstRound = new PetBattleRoundResults(0);

    // set active pets for team
    SetFrontPet(TEAM_ALLY, allyFrontPetID);
    SetFrontPet(TEAM_ENEMY, enemyFrontPetID);

    PetBattleEffect* effect = new PetBattleEffect(0, 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(allyFrontPetID, 3);

    effect->targets.push_back(target);
    firstRound->effects.push_back(effect);

    PetBattleEffect* effect1 = new PetBattleEffect(3, 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target1 = new PetBattleEffectTarget(enemyFrontPetID, 3);

    effect1->targets.push_back(target1);
    firstRound->effects.push_back(effect1);

    SendFirstRound(firstRound);
    delete firstRound;

    return true;
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

    for (auto itr : firstRound->effects)
    {
        PetBattleEffect* effect = itr;

        if (!effect)
            return;

        data.WriteBit(!effect->petBattleEffectType);
        data.WriteBit(!effect->sourceAuraInstanceID);
        data.WriteBit(!effect->abilityEffectID);
        // effect target count
        data.WriteBits(effect->targets.size(), 25);
        data.WriteBit(1);

        for (auto i : effect->targets)
        {
            PetBattleEffectTarget* target = i;

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

    for (auto itr : firstRound->effects)
    {
        PetBattleEffect* effect = itr;

        if (!effect)
            return;

        for (auto i : effect->targets)
        {
            PetBattleEffectTarget* target = i;

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

void PetBattleWild::ForceReplacePetHandler(uint32 roundID, uint8 newFrontPet, uint8 team)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(roundID);

    // check active pets
    PetBattleInfo* frontPet = GetFrontPet(team);

    if (!frontPet)
        return;

    // paranoia check
    if (GetTeamByPetID(newFrontPet) != team)
        return;

    round->ProcessPetSwap(frontPet->GetPetID(), newFrontPet);
    SendForceReplacePet(round);

    SetFrontPet(team, newFrontPet);

    delete round;
}

void PetBattleWild::SendForceReplacePet(PetBattleRoundResults* round)
{
    // send packet (testing!)
    WorldPacket data(SMSG_BATTLE_PET_REPLACEMENTS_MADE);

    for (uint8 i = 0; i < 2; ++i)
    {
        data << uint16(0); // RoundTimeSecs
        data << uint8(0);  // NextInputFlags
        data << uint8(2);  // NextTrapStatus
    }

    data << uint32(round->roundID);

    // cooldowns count
    data.WriteBits(0, 20);
    // effect count
    data.WriteBits(round->effects.size(), 22);

    for (auto itr : round->effects)
    {
        PetBattleEffect* effect = itr;

        if (!effect)
            return;

        data.WriteBit(!effect->stackDepth);
        data.WriteBit(!effect->abilityEffectID);
        data.WriteBit(effect->casterPBOID == -1 ? 1 : 0);
        data.WriteBit(!effect->flags);

        // effect target count
        data.WriteBits(effect->targets.size(), 25);
        data.WriteBit(!effect->petBattleEffectType);

        for (auto i : effect->targets)
        {
            PetBattleEffectTarget* target = i;

            if (!target)
                return;

            data.WriteBit(target->petX == -1 ? 1 : 0);
            data.WriteBits(target->type, 3); // TargetType
        }

        data.WriteBit(!effect->turnInstanceID);
        data.WriteBit(!effect->sourceAuraInstanceID);
    }

    data.WriteBit(0); // hasNextPetBattleState

    // cooldowns
    //

    data.WriteBits(round->petXDiedNumbers.size(), 3);

    for (auto itr : round->effects)
    {
        PetBattleEffect* effect = itr;

        if (!effect)
            return;

        for (auto i : effect->targets)
        {
            PetBattleEffectTarget* target = i;

            if (!target)
                return;

            data << uint8(target->petX); // target Petx - pet number
        }

        data << uint8(effect->casterPBOID);         // casterPBOID
        data << uint8(effect->petBattleEffectType); // PetBattleEffectType
    }

    data << uint8(2); // NextPetBattleState

    m_player->GetSession()->SendPacket(&data);
}

bool PetBattleWild::UseAbilityHandler(uint32 abilityID, uint32 roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(roundID);
    // default state - need system for control it
    SetBattleState(2);

    // check front pets
    PetBattleInfo* allyPet = GetFrontPet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetFrontPet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    // check on cheats
    if (!allyPet->HasAbility(abilityID))
        return false;

    // check speed and set attacker/victim of round
    // TODO: check some speed states and auras
    PetBattleInfo* attacker = allyPet->GetSpeed() > enemyPet->GetSpeed() ? allyPet : enemyPet;
    PetBattleInfo* victim = allyPet->GetSpeed() > enemyPet->GetSpeed() ? enemyPet : allyPet;

    bool damage = true;
    // TODO: script battle for wild pets
    if (damage)
    {
        // first turn: attack - TODO: script battle for wild pets (death check in some process)
        uint32 castAbilityID = (attacker->GetTeam() == TEAM_ALLY) ? abilityID : attacker->GetAbilityID(0);

        if (!castAbilityID)
            return false;

        uint32 effectID = GetVisualEffectIDByAbilityID(castAbilityID);
        round->ProcessAbilityDamage(attacker, victim, castAbilityID, effectID, 1);

        // second turn: response - TODO: script battle for wild pets (death check in some process)
        castAbilityID = (victim->GetTeam() == TEAM_ALLY) ? abilityID : victim->GetAbilityID(0);

        if (!castAbilityID)
            return false;

        effectID = GetVisualEffectIDByAbilityID(castAbilityID);
        round->ProcessAbilityDamage(victim, attacker, castAbilityID, effectID, 2);

        // find right place for some handler
        if (allyPet->IsDead() && !GetTotalPetCountInTeam(allyPet->GetTeam(), true))
        {
            SetWinner(enemyPet->GetTeam());
            nextRoundFinal = true;
        }
        else if (enemyPet->IsDead() && !GetTotalPetCountInTeam(enemyPet->GetTeam(), true))
        {
            SetWinner(allyPet->GetTeam());
            nextRoundFinal = true;
        }
        else if (allyPet->IsDead() && GetTotalPetCountInTeam(allyPet->GetTeam(), true) > 1)
            SetBattleState(3);
    }
    else
        return false;

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // increase round number
    round->roundID++;
    SetCurrentRoundID(round->roundID);

    CheckTrapStatuses(round);
    CheckInputFlags(round);

    SendRoundResults(round);

    // find right place for some handler
    if (enemyPet->IsDead() && GetTotalPetCountInTeam(TEAM_ENEMY, true))
        ForceReplacePetHandler(round->roundID, enemyPet->GetPetID() + 1, TEAM_ENEMY);
    else if (allyPet->IsDead() && GetTotalPetCountInTeam(TEAM_ALLY, true) == 1)
        ForceReplacePetHandler(round->roundID, GetLastAlivePetID(TEAM_ALLY), TEAM_ALLY);

    // return to default state - need system for control it
    petBattleState = 2;

    delete round;
    return true;
}

int8 PetBattleWild::GetLastAlivePetID(uint8 team)
{
    for (auto itr : battleInfo)
    {
        PetBattleInfo * pb = itr;

        if (!pb || pb->GetTeam() != team)
            continue;

        if (!pb->IsDead() && !pb->Captured() && !pb->Caged())
            return pb->GetPetID();
    }

    return -1;
}

uint32 PetBattleWild::GetCurrentRoundID()
{
    return currentRoundID;
}

bool PetBattleWild::SkipTurnHandler(uint32 _roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);
    // default state - need system for control it
    petBattleState = 2;

    // check active pets
    PetBattleInfo* allyPet = GetFrontPet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetFrontPet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    round->ProcessSkipTurn(allyPet->GetPetID());

    // response enemy after player pet skip turn
    uint32 castAbilityID = enemyPet->GetAbilityID(0);

    if (!castAbilityID)
        return false;

    uint32 effectID = GetVisualEffectIDByAbilityID(castAbilityID);
    round->ProcessAbilityDamage(enemyPet, allyPet, castAbilityID, effectID, 1);

    // find right place for some handler
    if (allyPet->IsDead() && !GetTotalPetCountInTeam(allyPet->GetTeam(), true))
    {
        SetWinner(enemyPet->GetTeam());
        nextRoundFinal = true;
    }
    else if (allyPet->IsDead() && GetTotalPetCountInTeam(allyPet->GetTeam(), true) > 1)
        SetBattleState(3);

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // increase round number
    round->roundID++;
    SetCurrentRoundID(round->roundID);

    CheckTrapStatuses(round);
    CheckInputFlags(round);

    SendRoundResults(round);

    if (allyPet->IsDead() && GetTotalPetCountInTeam(TEAM_ALLY, true) == 1)
        ForceReplacePetHandler(round->roundID, GetLastAlivePetID(TEAM_ALLY), TEAM_ALLY);

    delete round;
    return true;
}

bool PetBattleWild::UseTrapHandler(uint32 _roundID)
{
    // check active pets
    PetBattleInfo* allyPet = GetFrontPet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetFrontPet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    // cheater checks, TODO:
    if (allyPet->IsDead())
        return false;

    if (enemyPet->IsDead() || enemyPet->GetHealthPct() > 20)
        return false;

    // check chance - base implemented
    uint8 trapped = 1;
    //if (roll_chance_i(70))
        //trapped = 1;

    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);
    // default state - need system for control it
    petBattleState = 2;

    // demo
    PetBattleEffect* effect = new PetBattleEffect(allyPet->GetPetID(), 698, 5, 0, 0, 1, 1);
    PetBattleEffectTarget * target = new PetBattleEffectTarget(enemyPet->GetPetID(), 1);
    target->status = trapped;

    effect->AddTarget(target);
    round->AddEffect(effect);

    // demo
    if (!trapped)
    {
        // in response cast ability
    }
    else
    {
        enemyPet->SetCaptured(true);
        round->petXDiedNumbers.push_back(enemyPet->GetPetID());

        if (!GetTotalPetCountInTeam(TEAM_ENEMY, true))
        {
            SetWinner(TEAM_ALLY);
            nextRoundFinal = true;
        }
    }

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // set trap status
    round->SetTrapStatus(TEAM_ALLY, PET_BATTLE_TRAP_ERR_8);

    // increase round
    round->roundID++;
    SetCurrentRoundID(round->roundID);

    SendRoundResults(round);

    if (GetTotalPetCountInTeam(TEAM_ENEMY, true))
        ForceReplacePetHandler(round->roundID, enemyPet->GetPetID() + 1, TEAM_ENEMY);

    delete round;
    return true;
}

bool PetBattleWild::SwapPetHandler(uint8 newFrontPet, uint32 _roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);
    // default state - need system for control it
    petBattleState = 2;

    // check active pets
    PetBattleInfo* allyPet = GetFrontPet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetFrontPet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    if (newFrontPet < 0 || newFrontPet > 2)
        return false;

    if (allyPet->GetPetID() == newFrontPet)
    {
        SkipTurnHandler(_roundID);
        return true;
    }

    round->ProcessPetSwap(allyPet->GetPetID(), newFrontPet);

    SetFrontPet(TEAM_ALLY, newFrontPet);
    // check front pet
    allyPet = GetFrontPet(TEAM_ALLY);

    if (!allyPet)
        return false;

    // response enemy after player pet swap
    uint32 castAbilityID = enemyPet->GetAbilityID(0);

    if (!castAbilityID)
        return false;

    uint32 effectID = GetVisualEffectIDByAbilityID(castAbilityID);
    round->ProcessAbilityDamage(enemyPet, allyPet, castAbilityID, effectID, 1);

    // find right place for some handler
    if (allyPet->IsDead() && !GetTotalPetCountInTeam(allyPet->GetTeam(), true))
    {
        SetWinner(enemyPet->GetTeam());
        nextRoundFinal = true;
    }
    else if (allyPet->IsDead() && GetTotalPetCountInTeam(allyPet->GetTeam(), true) > 1)
        SetBattleState(3);

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // increase round number
    round->roundID++;
    SetCurrentRoundID(round->roundID);

    CheckTrapStatuses(round);
    SendRoundResults(round);

    if (allyPet->IsDead() && GetTotalPetCountInTeam(TEAM_ALLY, true) == 1)
        ForceReplacePetHandler(round->roundID, GetLastAlivePetID(TEAM_ALLY), TEAM_ALLY);

    delete round;
    return true;
}

void PetBattleWild::CheckTrapStatuses(PetBattleRoundResults* round)
{
    // check active pets
    PetBattleInfo* allyPet = GetFrontPet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetFrontPet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return;

    // default for wild pets and NPC
    round->SetTrapStatus(TEAM_ENEMY, PET_BATTLE_TRAP_ERR_2);

    if (enemyPet->Caged() || enemyPet->Captured())
        return;

    uint8 allyTrapStatus = PET_BATTLE_TRAP_ERR_4;

    if (enemyPet->GetHealthPct() < 20)
        allyTrapStatus = PET_BATTLE_TRAP_ACTIVE;

    // some checks
    uint32 creatureEntry = enemyPet->GetCreatureEntry();
    if (m_player->GetBattlePetMgr()->GetPetCount(creatureEntry) >= 3)
        allyTrapStatus = PET_BATTLE_TRAP_ERR_5;

    if (allyPet->IsDead() || enemyPet->IsDead())
        allyTrapStatus = PET_BATTLE_TRAP_ERR_3;

    round->SetTrapStatus(TEAM_ALLY, allyTrapStatus);
}

void PetBattleWild::CheckInputFlags(PetBattleRoundResults* round)
{
    for (uint8 i = 0; i < 2; ++i)
        round->SetInputFlags(i, 0);

    // check special state
    if (GetBattleState() == 3)
    {
        round->SetInputFlags(TEAM_ALLY, 8);
        round->SetInputFlags(TEAM_ENEMY, 6);
    }
}

void PetBattleWild::SendRoundResults(PetBattleRoundResults* round)
{
    WorldPacket data(SMSG_PET_BATTLE_ROUND_RESULT);
    data.WriteBit(0); // !hasNextBattleState
    // cooldowns count
    data.WriteBits(0, 20);
    // effects count
    data.WriteBits(round->effects.size(), 22);
    for (auto itr : round->effects)
    {
        PetBattleEffect* effect = itr;

        if (!effect)
            return;

        data.WriteBit(effect->casterPBOID == -1 ? 1 : 0); // !hasCasterPBOID
        data.WriteBit(!effect->petBattleEffectType); // !hasPetBattleEffectType
        data.WriteBit(!effect->flags); // !hasEffectFlags
        data.WriteBit(!effect->turnInstanceID); // !hasTurnInstanceID
        data.WriteBit(!effect->stackDepth); // !hasStackDepth
        data.WriteBit(!effect->abilityEffectID); // !hasAbilityEffectID

        data.WriteBits(effect->targets.size(), 25);
        for (auto i : effect->targets)
        {
            PetBattleEffectTarget* target = i;

            if (!target)
                return;

            data.WriteBits(target->type, 3);

            if (target->type == 6)
                data.WriteBit(!target->remainingHealth); // !hasRemainingHealth

            if (target->type == 4)
            {
                data.WriteBit(!target->stateID);
                data.WriteBit(!target->stateValue);
            }

            if (target->type == 1)
                data.WriteBit(!target->status);

            data.WriteBit(target->petX == -1 ? 1 : 0); // !hasPetX
        }

        data.WriteBit(!effect->sourceAuraInstanceID); // !hasSourceAuraInstanceID
    }

    data.WriteBits(round->petXDiedNumbers.size(), 3);

    // cooldowns
    //

    for (auto itr : round->effects)
    {
        PetBattleEffect* effect = itr;

        if (!effect)
            return;

        if (effect->turnInstanceID)
            data << uint16(effect->turnInstanceID);

        for (std::list<PetBattleEffectTarget*>::iterator i = effect->targets.begin(); i != effect->targets.end(); ++i)
        {
            PetBattleEffectTarget* target = (*i);

            if (!target)
                return;

            if (target->type == 4)
            {
                if (target->stateID)
                    data << uint32(target->stateID);
                if (target->stateValue)
                    data << uint32(target->stateValue);
            }

            if (target->petX != -1)
                data << uint8(target->petX); // targetPetX

            if (target->type == 6)
            {
                if (target->remainingHealth)
                    data << int32(target->remainingHealth);
            }

            if (target->type == 1)
            {
                if (target->status)
                    data << uint32(target->status);
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
        data << uint8(round->inputFlags[i]);
        data << uint16(0); // RoundTimeSecs
        data << uint8(round->trapStatus[i]);  // NextTrapStatus
    }

    data << uint32(round->roundID);
    data << uint8(petBattleState); // NextPetBattleState

    if (round->petXDiedNumbers.size() > 0)
    {
        for (uint8 i = 0; i < round->petXDiedNumbers.size(); ++i)
            data << uint8(round->petXDiedNumbers[i]);
    }

    m_player->GetSession()->SendPacket(&data);
}

bool PetBattleWild::FinalRoundHandler(bool abandoned)
{
    PetBattleInfo* allyPet = GetFrontPet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetFrontPet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    // rewardXP only for winner
    if (GetWinner() == TEAM_ALLY)
    {
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
        if (allyPet->GetLevel() == MAX_BATTLE_PET_LEVEL)
            rewardXp = 0;
        newXp = oldXp + rewardXp;
        uint32 totalXp = m_player->GetBattlePetMgr()->GetXPForNextLevel(allyPet->GetLevel());

        // check level-up
        if (newXp >= totalXp)
        {
            allyPet->SetNewLevel(allyPet->GetLevel() + 1);
            uint16 remXp = newXp - totalXp;
            allyPet->SetTotalXP(remXp);

            // recalculate stats
            BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(allyPet->GetSpeciesID(), allyPet->GetBreedID());
            accumulator->CalcQualityMultiplier(allyPet->GetQuality(), allyPet->GetNewLevel());
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
            allyPet->SetTotalXP(newXp);
    }
    else
    {
        if (abandoned)
        {
            // all alive pets penalty
            for (BattleInfo::const_iterator itr = battleInfo.begin(); itr != battleInfo.end(); ++itr)
            {
                PetBattleInfo * pb = (*itr);

                if (!pb || pb->GetTeam() != TEAM_ALLY)
                    continue;

                if (!pb->IsDead())
                {
                    int32 percent10 = pb->GetHealth() / 10;
                    int32 newHealth = pb->GetHealth() - percent10;
                    pb->SetHealth(newHealth);
                }
            }
        }
    }

    SendFinalRound();

    return true;
}

void PetBattleWild::SendFinalRound()
{
    WorldPacket data(SMSG_PET_BATTLE_FINAL_ROUND);
    // petsCount
    data.WriteBits(battleInfo.size(), 20);

    for (uint8 i = 0; i < 2; i++)
    {
        for (auto itr : battleInfo)
        {
            PetBattleInfo * pb = itr;

            if (!pb || pb->GetTeam() != i)
                continue;

            data.WriteBit(pb->Captured() || pb->Caged()); // hasCaptured | hasCaged
            data.WriteBit(1); // hasSeenAction
            data.WriteBit(!pb->GetTotalXP()); // !hasXp
            data.WriteBit(pb->Captured() || pb->Caged()); // hasCaptured | hasCaged
            data.WriteBit(0); // !hasInitialLevel
            data.WriteBit(0); // !hasLevel
            data.WriteBit(pb->GetTotalXP() || (pb->GetLevel() < pb->GetNewLevel())); // awardedXP
        }
    }

    data.WriteBit(0); // pvpBattle

    // winners bits
    for (uint8 i = 0; i < 2; ++i)
        data.WriteBit(winners[i]);

    data.WriteBit(abandoned); // abandoned

    for (uint8 i = 0; i < 2; i++)
    {
        for (auto itr : battleInfo)
        {
            PetBattleInfo * pb = itr;

            if (!pb || pb->GetTeam() != i)
                continue;

            if (pb->GetTotalXP())
                data << uint16(pb->GetTotalXP());

            data << uint8(pb->GetPetID()); // Pboid
            data << uint32(pb->GetHealth()); // RemainingHealth
            data << uint16(pb->GetNewLevel());  // NewLevel
            data << uint32(pb->GetMaxHealth()); // maxHealth
            data << uint16(pb->GetLevel()); // InitialLevel
        }
    }

    for (uint8 i = 0; i < 2; ++i)
        data << uint32(0); // NpcCreatureID

    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::SendFinishPetBattle()
{
    WorldPacket data(SMSG_PET_BATTLE_FINISHED);
    m_player->GetSession()->SendPacket(&data);
}

void PetBattleWild::UpdatePetsAfterBattle()
{
    std::list<uint64> updates;
    updates.clear();

    // find trapped pets
    for (uint8 i = 0; i < 2; ++i)
    {
        for (auto itr : battleInfo)
        {
            PetBattleInfo * pb = itr;

            if (!pb || pb->GetTeam() != i)
                continue;

            if (i == TEAM_ENEMY && !pb->Captured())
                continue;

            // update loadout
            if (i == TEAM_ALLY)
            {
                PetJournalInfo* loadoutInfo = m_player->GetBattlePetMgr()->GetPetInfoByPetGUID(pb->GetGUID());

                if (!loadoutInfo)
                    continue;

                // recalculate stats
                BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(pb->GetSpeciesID(), pb->GetBreedID());
                accumulator->CalcQualityMultiplier(pb->GetQuality(), pb->GetNewLevel());
                uint32 health = accumulator->CalculateHealth();
                uint32 power = accumulator->CalculatePower();
                uint32 speed = accumulator->CalculateSpeed();
                delete accumulator;

                // update
                loadoutInfo->SetLevel(pb->GetNewLevel());
                if (pb->IsDead())
                    loadoutInfo->SetHealth(0);
                else
                    loadoutInfo->SetHealth(pb->GetHealth());
                loadoutInfo->SetQuality(pb->GetQuality());
                loadoutInfo->SetXP(pb->GetTotalXP());
                loadoutInfo->SetPower(power);
                loadoutInfo->SetSpeed(speed);
                loadoutInfo->SetMaxHealth(health);
                loadoutInfo->SetState(STATE_UPDATED);

                updates.push_back(pb->GetGUID());
            }
            // update trapped pets and added in journal
            else
            {
                uint64 petguid = sObjectMgr->GenerateBattlePetGuid();

                BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(pb->GetSpeciesID(), pb->GetBreedID());
                accumulator->CalcQualityMultiplier(pb->GetQuality(), pb->GetLevel());
                uint32 health = accumulator->CalculateHealth();
                uint32 power = accumulator->CalculatePower();
                uint32 speed = accumulator->CalculateSpeed();
                delete accumulator;

                m_player->GetBattlePetMgr()->AddPetToList(petguid, pb->GetSpeciesID(), pb->GetCreatureEntry(), pb->GetLevel(), pb->GetDisplayID(), power, speed, pb->GetHealth(), health, pb->GetQuality(), 0, 0, pb->GetSummonSpell(), "", pb->GetBreedID(), STATE_UPDATED);
                // hack, fix it!
                if (pb->GetSummonSpell())
                    m_player->learnSpell(pb->GetSummonSpell(), false);

                std::list<uint64> newPets;
                newPets.clear();
                newPets.push_back(petguid);

                m_player->GetBattlePetMgr()->SendUpdatePets(newPets, true);
                m_player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL, pb->GetCreatureEntry());
                m_player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COLLECT_BATTLEPET);
                m_player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE, pb->GetSpeciesID(), pb->GetQuality(), pb->GetType());
                m_player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAPTURE_BATTLE_PET_CREDIT);
            }

            if (GetWinner() == TEAM_ALLY)
            {
                m_player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN, 0, 0, pb->GetType());

                if (pb->GetNewLevel() > pb->GetLevel())
                    m_player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_LEVEL_UP, pb->GetNewLevel());
            }
            else
                m_player->GetAchievementMgr().ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BATTLEPET_WIN, ACHIEVEMENT_CRITERIA_CONDITION_NO_LOSE_PET_BATTLE);
        }
    }

    if (!updates.empty())
        m_player->GetBattlePetMgr()->SendUpdatePets(updates, false);
}

void PetBattleWild::FinishPetBattle(bool error)
{
    SendFinishPetBattle();

    if (!error)
        UpdatePetsAfterBattle();

    m_player->GetBattlePetMgr()->CloseWildPetBattle();
}

uint32 PetBattleWild::GetVisualEffectIDByAbilityID(uint32 abilityID, uint8 turnIndex)
{
    BattlePetTurnByAbilityIdMapBounds bounds = sBattlePetTurnByAbilityId.equal_range(abilityID);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
    {
        uint8 _turnIndex = std::get<1>(itr->second);

        if (_turnIndex == turnIndex)
        {
            uint32 turnID = std::get<0>(itr->second);

            auto itr = sBattlePetEffectEntryByTurnId.find(turnID);
            if (itr != sBattlePetEffectEntryByTurnId.end())
                return itr->second->ID;
        }
    }

    return 0;
}

// PetBattleAbility
PetBattleAbilityInfo::PetBattleAbilityInfo(uint32 _ID, uint32 _speciesID)
{
    BattlePetAbilityEntry const *abilityEntry = sBattlePetAbilityStore.LookupEntry(_ID);

    if (abilityEntry)
    {
        // get data from abilityEntry
        ID = abilityEntry->ID;
        Type = abilityEntry->Type;
        turnCooldown = abilityEntry->turnCooldown;
        auraAbilityID = abilityEntry->auraAbilityID;
        auraDuration = abilityEntry->auraDuration;
        // get data from xAbilityEntry
        // fucking blizzard...
        for (uint32 i = 0; i < sBattlePetSpeciesXAbilityStore.GetNumRows(); ++i)
        {
            BattlePetSpeciesXAbilityEntry const* xEntry = sBattlePetSpeciesXAbilityStore.LookupEntry(i);

            if (!xEntry)
                continue;

            if (xEntry->abilityID == abilityEntry->ID && xEntry->speciesID == _speciesID)
            {
                requiredLevel = xEntry->requiredLevel;
                rank = xEntry->rank;
                break;
            }
        }
    }
}

uint32 PetBattleAbilityInfo::GetEffectProperties(uint8 properties, uint32 effectIdx, uint32 turnIndex)
{
    char* desc = "Points";
    // get string for properties
    switch (properties)
    {
        case 1: desc = "Accuracy"; break;
        default: break;
    }

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

float PetBattleAbilityInfo::GetAttackModifier(uint8 attackType, uint8 defenseType)
{
    // maybe number of attack types?
    uint32 formulaValue = 0xA;
    uint32 modId = defenseType * formulaValue + Type;

    for (uint32 j = 0; j < sGtBattlePetTypeDamageModStore.GetNumRows(); ++j)
    {
        GtBattlePetTypeDamageModEntry const* gt = sGtBattlePetTypeDamageModStore.LookupEntry(j);

        if (!gt)
            continue;

        if (gt->Id == modId)
            return gt->value;
    }

    return 0.0f;
}

// PetJournalInfo
uint32 PetJournalInfo::GetAbilityID(uint8 rank)
{
    bool customAbility = false;

    switch (rank)
    {
        case 0: customAbility = (flags & BATTLE_PET_FLAG_CUSTOM_ABILITY_1) ? true : false; break;
        case 1: customAbility = (flags & BATTLE_PET_FLAG_CUSTOM_ABILITY_2) ? true : false; break;
        case 2: customAbility = (flags & BATTLE_PET_FLAG_CUSTOM_ABILITY_3) ? true : false; break;
    }

    BattlePetXAbilityEntryBySpecIdMapBounds bounds = sBattlePetXAbilityEntryBySpecId.equal_range(speciesID);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
    {
        BattlePetSpeciesXAbilityEntry const* xEntry = itr->second;

        if (!xEntry)
            continue;

        bool _customAbility = xEntry->requiredLevel >= 10;

        if (xEntry->rank == rank && customAbility == _customAbility)
            return xEntry->abilityID;
    }

    return 0;
}

// PetBattleRoundResults
PetBattleRoundResults::~PetBattleRoundResults()
{
    // delete targets
    for (auto itr : effects)
        for (auto itr2 : itr->targets)
            delete itr2;

    // delete effects
    for (auto itr : effects)
        delete itr;
}

void PetBattleRoundResults::ProcessAbilityDamage(PetBattleInfo* caster, PetBattleInfo* target, uint32 abilityID, uint32 effectID, uint8 turnInstanceID)
{
    // check on dead
    if (caster->IsDead() || target->IsDead())
        return;

    // base pre-calculate damage and flags
    PetBattleAbilityInfo* ainfo = new PetBattleAbilityInfo(abilityID, caster->GetSpeciesID());
    uint16 flags = ainfo->CalculateHitResult(caster, target);
    bool crit = false;
    if (flags & PETBATTLE_EFFECT_FLAG_CRIT)
        crit = true;
    uint32 damage = ainfo->CalculateDamage(caster, target, crit);

    // simple combination of effect 0 and target type 6
    PetBattleEffect* effect = new PetBattleEffect(caster->GetPetID(), effectID, 0, flags, 0, turnInstanceID, 1);
    PetBattleEffectTarget* t = new PetBattleEffectTarget(target->GetPetID(), 6);

    // process in battle
    int32 newHealth = target->GetHealth() - damage;
    target->SetHealth(newHealth);

    // process for round result
    t->remainingHealth = newHealth;

    effect->AddTarget(t);
    AddEffect(effect);

    // process if victim dead
    if (target->IsDead())
    {
        // added petID to died ID
        AddDeadPet(target->GetPetID());
        // set state DEAD (testing!)
        ProcessSetState(caster, target, effectID, 1, 1, turnInstanceID);
    }
}

void PetBattleRoundResults::AddDeadPet(uint8 petNumber)
{
    petXDiedNumbers.push_back(petNumber);
}

void PetBattleRoundResults::ProcessSetState(PetBattleInfo* caster, PetBattleInfo* target, uint32 effectID, uint8 stateID, uint32 stateValue, uint8 turnInstanceID)
{
    PetBattleEffect* effect = new PetBattleEffect(caster->GetPetID(), effectID, 6, 0, 0, turnInstanceID, 1);
    PetBattleEffectTarget* t = new PetBattleEffectTarget(target->GetPetID(), 4);

    t->stateID = stateID;
    t->stateValue = stateValue;

    effect->AddTarget(t);
    AddEffect(effect);
}

void PetBattleRoundResults::ProcessPetSwap(uint8 oldPetNumber, uint8 newPetNumber)
{
    // simple combination of effect 4 and target type 3
    PetBattleEffect* effect = new PetBattleEffect(oldPetNumber, 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(newPetNumber, 3);

    effect->AddTarget(target);
    AddEffect(effect);
}

void PetBattleRoundResults::ProcessSkipTurn(uint8 petNumber)
{
    // simple combination of effect 4 and target type 3
    PetBattleEffect* effect = new PetBattleEffect(petNumber, 0, 4, 1, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(petNumber, 3);

    effect->AddTarget(target);
    AddEffect(effect);
}

void PetBattleRoundResults::AuraProcessingBegin()
{
    PetBattleEffect* effect = new PetBattleEffect(-1, 0, 13, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(-1, 3);

    effect->AddTarget(target);
    AddEffect(effect);
}

void PetBattleRoundResults::AuraProcessingEnd()
{
    PetBattleEffect* effect = new PetBattleEffect(-1, 0, 14, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(-1, 3);

    effect->AddTarget(target);
    AddEffect(effect);
}

int32 PetBattleAbilityInfo::GetBaseDamage(PetBattleInfo* attacker, uint32 effectIdx, uint32 turnIndex)
{
    uint32 basePoints = GetEffectProperties(BASEPOINTS, effectIdx, turnIndex);
    return basePoints * (1 + attacker->GetPower() * 0.05f);
}

int32 PetBattleAbilityInfo::CalculateDamage(PetBattleInfo* attacker, PetBattleInfo* victim, bool crit)
{
    int32 baseDamage = GetBaseDamage(attacker);
    int32 cleanDamage = urand(baseDamage - 5, baseDamage + 5);
    // mods
    float mod = GetAttackModifier(GetType(), victim->GetType());
    int32 finalDamage = cleanDamage * mod;
    if (crit)
        finalDamage *= 2;

    return finalDamage;
}

int16 PetBattleAbilityInfo::CalculateHitResult(PetBattleInfo* attacker, PetBattleInfo* victim)
{
    uint16 flags = PETBATTLE_EFFECT_FLAG_BASE;
    // mods
    float mod = GetAttackModifier(GetType(), victim->GetType());

    if (roll_chance_i(5))
        flags |= PETBATTLE_EFFECT_FLAG_CRIT;

    if (mod > 1.0f)
        flags |= PETBATTLE_EFFECT_FLAG_STRONG;
    else if (mod < 1.0f)
        flags |= PETBATTLE_EFFECT_FLAG_WEAK;

    return flags;
}