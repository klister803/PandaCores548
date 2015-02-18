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
    m_PetJournal[guid] = new PetJournalInfo(speciesID, creatureEntry, level, display, power, speed, health, maxHealth, quality, xp, flags, spellID, customName, breedID, state);
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

    for (PetJournal::const_iterator journal = m_PetJournal.begin(); journal != m_PetJournal.end(); ++journal)
    {
        PetJournalInfo* petInfo = journal->second;

        // prevent crash
        if (!petInfo)
            continue;

        // prevent loading deleted pet
        if (petInfo->GetInternalState() == STATE_DELETED)
            continue;

        ObjectGuid guid = journal->first;
        uint8 len = petInfo->GetCustomName() == "" ? 0 : petInfo->GetCustomName().length();

        data->WriteBit(!petInfo->GetBreedID());     // hasBreed, inverse
        data->WriteGuidMask<1, 5, 3>(guid);
        data->WriteBit(0);                          // hasOwnerGuidInfo
        data->WriteBit(!petInfo->GetQuality());     // hasQuality, inverse
        data->WriteGuidMask<6, 7>(guid);
        data->WriteBit(!petInfo->GetFlags());       // hasFlags, inverse
        data->WriteGuidMask<0, 4>(guid);
        data->WriteBits(len, 7);                    // custom name length
        data->WriteGuidMask<2>(guid);
        data->WriteBit(1);                          // hasUnk (bool noRename?), inverse
    }

    data->WriteBits(MAX_ACTIVE_BATTLE_PETS, 25);

    for (uint32 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
    {
        PetBattleSlot * slot = GetPetBattleSlot(i);

        if (!slot)
        {
            data->WriteBit(!i);                                          // hasSlotIndex
            data->WriteBit(1);                                           // hasCollarID, inverse
            data->WriteBit(1);                                           // empty slot, inverse
            data->WriteGuidMask<7, 1, 3, 2, 5, 0, 4, 6>(placeholderPet); // pet guid in slot
            data->WriteBit(1);                                           // locked slot
            continue;
        }

        // check slot locked conditions
        bool locked = true;

        switch (i)
        {
            case 0: locked = !m_player->HasSpell(119467); break;
            case 1: locked = /*!m_player->GetAchievementMgr().HasAchieved(7433)*/false; break;
            case 2: locked = /*!m_player->GetAchievementMgr().HasAchieved(6566)*/false; break;
            default: break;
        }

        data->WriteBit(!i);                                              // hasSlotIndex
        data->WriteBit(1);                                               // hasCollarID, inverse
        data->WriteBit(!slot->IsEmpty());                                // empty slot, inverse
        data->WriteGuidMask<7, 1, 3, 2, 5, 0, 4, 6>(slot->GetPet());     // pet guid in slot
        data->WriteBit(locked);                                          // locked slot
    }

    data->WriteBit(1);                      // !hasJournalLock

    for (uint32 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
    {
        PetBattleSlot * slot = GetPetBattleSlot(i);

        if (!slot)
        {
            data->WriteGuidBytes<3, 7, 5, 1, 4, 0, 6, 2>(placeholderPet);
            if (i)
                *data << uint8(i);             // SlotIndex

            continue;
        }

        //if (!bit)
            //*data << uint32(0);
        data->WriteGuidBytes<3, 7, 5, 1, 4, 0, 6, 2>(slot->GetPet());
        if (i)
            *data << uint8(i);             // SlotIndex
    }

    for (PetJournal::const_iterator journal = m_PetJournal.begin(); journal != m_PetJournal.end(); ++journal)
    {
        PetJournalInfo* petInfo = journal->second;

        // prevent crash
        if (!petInfo)
            continue;

        // prevent loading deleted pet
        if (petInfo->GetInternalState() == STATE_DELETED)
            continue;

        ObjectGuid guid = journal->first;
        uint8 len = petInfo->GetCustomName() == "" ? 0 : petInfo->GetCustomName().length();

        if (petInfo->GetBreedID())
            *data << uint16(petInfo->GetBreedID());            // breedID
        *data << uint32(petInfo->GetSpeciesID());              // speciesID
        *data << uint32(petInfo->GetSpeed());                  // speed
        data->WriteGuidBytes<1, 6, 4>(guid);
        *data << uint32(petInfo->GetDisplayID());
        *data << uint32(petInfo->GetMaxHealth());              // max health
        *data << uint32(petInfo->GetPower());                  // power
        data->WriteGuidBytes<2>(guid);
        *data << uint32(petInfo->GetCreatureEntry());          // Creature ID
        *data << uint16(petInfo->GetLevel());                  // level
        if (len > 0)
            data->WriteString(petInfo->GetCustomName());       // custom name
        *data << uint32(petInfo->GetHealth());                 // health
        *data << uint16(petInfo->GetXP());                     // xp
        if (petInfo->GetQuality())
            *data << uint8(petInfo->GetQuality());             // quality
        data->WriteGuidBytes<3, 7, 0>(guid);
        if (petInfo->GetFlags())
            *data << uint16(petInfo->GetFlags());              // flags
        data->WriteGuidBytes<5>(guid);

        count++;
    }

    *data << uint16(0);                                        // trapLevel
    data->PutBits<uint8>(bit_pos, count, 19);
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
    uint32 count = 0;
    WorldPacket data(SMSG_BATTLE_PET_UPDATES);
    data.WriteBit(added);
    data.WriteBits(updates.size(), 19);                              // placeholder, count of update pets
    for (std::list<uint64>::iterator i = updates.begin(); i != updates.end(); ++i)
    {
        PetJournalInfo* petInfo = GetPetInfoByPetGUID((*i));

        if (!petInfo || petInfo->GetInternalState() == STATE_DELETED)
            continue;

        ObjectGuid guid = (*i);
        uint8 len = petInfo->GetCustomName() == "" ? 0 : petInfo->GetCustomName().length();

        data.WriteBits(len, 7);                         // custom name length
        data.WriteBit(!petInfo->GetFlags());            // !hasFlags
        data.WriteBit(1);                               // !hasUnk
        data.WriteGuidMask<2>(guid);
        data.WriteBit(!petInfo->GetBreedID());          // !hasBreed
        data.WriteBit(!petInfo->GetQuality());          // !hasQuality
        data.WriteGuidMask<1, 6, 3, 7, 0, 4, 5>(guid);
        data.WriteBit(0);                               // hasGuid

        /*if (hasGuid)
        {
            data.WriteGuidMask<7, 0, 6, 2, 1, 3, 5, 4>(petGuid2);
        }*/
    }

    for (std::list<uint64>::iterator i = updates.begin(); i != updates.end(); ++i)
    {
        PetJournalInfo* petInfo = GetPetInfoByPetGUID((*i));

        if (!petInfo || petInfo->GetInternalState() == STATE_DELETED)
            continue;

        ObjectGuid guid = (*i);
        uint8 len = petInfo->GetCustomName() == "" ? 0 : petInfo->GetCustomName().length();

        /*if (hasGuid)
        {
            data << uint32(0); // unk
            data.WriteGuidBytes<0, 1, 2, 3, 4, 5, 7, 6>(petGuid2);
            data << uint32(0); // unk1
        }*/

        data << uint16(petInfo->GetLevel());                  // level
        data << uint32(petInfo->GetMaxHealth());              // total health
        if (petInfo->GetFlags())
            data << uint16(petInfo->GetFlags());              // flags
        if (petInfo->GetQuality())
            data << uint8(petInfo->GetQuality());             // quality
        data.WriteGuidBytes<3>(guid);
        data << uint32(petInfo->GetCreatureEntry());          // creature ID
        data << uint32(petInfo->GetSpeed());                  // speed
        if (petInfo->GetBreedID())
            data << uint16(petInfo->GetBreedID());            // breed ID
        data << uint32(petInfo->GetHealth());                 // remaining health
        data << uint32(petInfo->GetDisplayID());              // display ID
        if (len > 0)
            data.WriteString(petInfo->GetCustomName());       // custom name
        data.WriteGuidBytes<5, 4, 7>(guid);
        data << uint32(petInfo->GetSpeciesID());              // species ID
        data.WriteGuidBytes<2, 6>(guid);
        data << uint16(petInfo->GetXP());                     // experience
        data.WriteGuidBytes<0, 1>(guid);
        data << uint32(petInfo->GetPower());                  // power
    }

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

void BattlePetMgr::CreateWildBattle(Player* initiator, ObjectGuid wildCreatureGuid)
{
    m_petBattleWild = new PetBattleWild(initiator);
    m_petBattleWild->Init(wildCreatureGuid);
}

// BattlePetStatAccumulator
BattlePetStatAccumulator::BattlePetStatAccumulator(uint32 _speciesID, uint16 _breedID) : healthMod(0), powerMod(0), speedMod(0), qualityMultiplier(0.0f)
{
    for (uint32 i = 0; i < sBattlePetSpeciesStateStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesStateEntry const* entry = sBattlePetSpeciesStateStore.LookupEntry(i);

        if (!entry)
            continue;

        if (entry->speciesID == _speciesID)
            Accumulate(entry->stateID, entry->stateModifier);
    }

    for (uint32 i = 0; i < sBattlePetBreedStateStore.GetNumRows(); ++i)
    {
        BattlePetBreedStateEntry const* entry1 = sBattlePetBreedStateStore.LookupEntry(i);

        if (!entry1)
            continue;

        if (entry1->breedID == _breedID)
            Accumulate(entry1->stateID, entry1->stateModifier);
    }
}

// PetBattleWild
PetBattleWild::PetBattleWild(Player* owner) : m_player(owner)
{
    for (uint8 i = 0; i < 2; ++i)
    {
        for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PETS; ++j)
        {
            battleInfo[i][j] = new PetBattleInfo();

            for (uint8 k = 0; k < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++k)
                battleInfo[i][j]->SetAbilityInfo(NULL, k);
        }

        petsCount[i] = 0;
    }

    nextRoundFinal = false;
    abandoned = false;
}

bool PetBattleWild::PrepareBattleInfo(ObjectGuid creatureGuid)
{
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
                    if (petInfo->IsDead() || petInfo->GetInternalState() == STATE_DELETED)
                        continue;

                    battleInfo[TEAM_ALLY][i]->SetPetNumber(i);
                    battleInfo[TEAM_ALLY][i]->SetGUID(_slot->GetPet());
                    battleInfo[TEAM_ALLY][i]->SetPetInfo(petInfo);

                    petsCount[TEAM_ALLY]++;

                    uint32 speciesID = petInfo->GetSpeciesID();
                    for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++j)
                    {
                        uint32 ID = petInfo->GetAbilityID(j); 

                        PetBattleAbilityInfo* a = new PetBattleAbilityInfo(ID, speciesID);
                        battleInfo[TEAM_ALLY][i]->SetAbilityInfo(a, j);
                    }
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
    uint32 creatureCount = 2;

    for (uint8 i = 0; i < creatureCount; ++i)
    {
        // roll random quality
        uint8 quality = 0;
        if (roll_chance_i(10))
            quality = 3;
        else if (roll_chance_i(25))
            quality = 2;
        else if (roll_chance_i(60))
            quality = 1;

        BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(s->ID, 12);
        accumulator->CalcQualityMultiplier(quality, wildPetLevel);
        uint32 health = accumulator->CalculateHealth();
        uint32 power = accumulator->CalculatePower();
        uint32 speed = accumulator->CalculateSpeed();
        delete accumulator;

        PetBattleSlot* slot = new PetBattleSlot(0);
        PetJournalInfo* petInfo = new PetJournalInfo(s->ID, wildPet->GetEntry(), wildPetLevel, t->Modelid1, power, speed, health, health, quality, 0, 0, s->spellId, "", 12, 0);

        battleInfo[TEAM_ENEMY][i]->SetPetNumber(i + MAX_ACTIVE_BATTLE_PETS);
        battleInfo[TEAM_ENEMY][i]->SetGUID(slot->GetPet());
        battleInfo[TEAM_ENEMY][i]->SetPetInfo(petInfo);

        petsCount[TEAM_ENEMY]++;

        uint32 speciesID = petInfo->GetSpeciesID();
        for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++j)
        {
            uint32 ID = petInfo->GetAbilityID(j);

            PetBattleAbilityInfo* a = new PetBattleAbilityInfo(ID, speciesID);
            battleInfo[TEAM_ENEMY][i]->SetAbilityInfo(a, j);
        }

        delete slot;
        delete petInfo;
    }

    petBattleState = 1;

    return true;
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
        data.WriteBits(petsCount[i], 2);
        data.WriteGuidMask<1>(teamGuids[i]);

        for (uint8 j = 0; j < petsCount[i]; ++j)
        {
            // check vaild
            if (!battleInfo[i][j] || !battleInfo[i][j]->Vaild())
                continue;

            uint64 guid = battleInfo[i][j]->GetGUID();

            data.WriteGuidMask<3, 4, 0>(guid);
            data.WriteBit(0);
            data.WriteGuidMask<6>(guid);
            data.WriteBits(4, 21);                   // state count
            data.WriteBits(1, 20);                   // abilities count
            data.WriteGuidMask<5>(guid);

            data.WriteBit(0);

            data.WriteGuidMask<2, 1, 7>(guid);
            data.WriteBit(1);

            uint8 len = battleInfo[i][j]->GetCustomName() == "" ? 0 : battleInfo[i][j]->GetCustomName().length();
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

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < petsCount[i]; ++j)
        {
            // check vaild
            if (!battleInfo[i][j] || !battleInfo[i][j]->Vaild())
                continue;

            uint64 guid = battleInfo[i][j]->GetGUID();

            // abilities
            for (uint8 k = 0; k < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++k)
            {
                if (k != 0)
                    break;

                data << uint8(0);
                data << uint16(0);
                data << uint8(0);
                data << uint16(0);
                data << uint32(battleInfo[i][j]->GetAbilityInfoByIndex(k)->GetID());
            }

            data << uint16(battleInfo[i][j]->GetXP());
            data << uint32(battleInfo[i][j]->GetMaxHealth());

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
            data << uint32(battleInfo[i][j]->GetSpeed()); // speed

            // auras
            //

            data.WriteGuidBytes<4>(guid);
            uint8 len = battleInfo[i][j]->GetCustomName() == "" ? 0 : battleInfo[i][j]->GetCustomName().length();
            if (len > 0)
                data.WriteString(battleInfo[i][j]->GetCustomName());
            data << uint16(battleInfo[i][j]->GetQuality());
            data.WriteGuidBytes<0>(guid);
            data << uint32(battleInfo[i][j]->GetHealth());
            data.WriteGuidBytes<3>(guid);
            data << uint32(battleInfo[i][j]->GetSpeciesID());
            data << uint32(0);
            data.WriteGuidBytes<2, 6>(guid);
            data << uint32(battleInfo[i][j]->GetDisplayID());
            data << uint16(battleInfo[i][j]->GetLevel());
            data.WriteGuidBytes<7, 5>(guid);
            data << uint32(battleInfo[i][j]->GetPower());
            data << uint8(j);                          // Slot
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
    data << uint8(petBattleState);                     // CurPetBattleState
    data << uint32(0);                                 // CurRound
    data << uint8(10);                                 // ForfeitPenalty

    m_player->GetSession()->SendPacket(&data);
}

void PetBattleInfo::SetPetInfo(PetJournalInfo* petInfo)
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

    activePet = false;
    captured = false;
    caged = false;
}

void PetBattleWild::Init(ObjectGuid creatureGuid)
{
    if (!PrepareBattleInfo(creatureGuid))
        return;

    SendFullUpdate(creatureGuid);
}

PetBattleRoundResults* PetBattleWild::PrepareFirstRound(uint8 frontPet)
{
    // set active
    SetActivePet(TEAM_ALLY, frontPet);
    SetActivePet(TEAM_ENEMY, 0);

    // effectSetFrontPet / effectSwapPet
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

void PetBattleWild::ReplacePetHandler(uint8 petNumberSource, uint8 petNumberDest, uint8 index, uint32 roundID, bool enemy)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(roundID);

    PetBattleEffect* effect = new PetBattleEffect(petNumberSource, 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(petNumberDest, 3);

    effect->AddTarget(target);
    round->AddEffect(effect);

    // send packet (testing!)
    WorldPacket data(SMSG_BATTLE_PET_REPLACEMENTS_MADE);

    for (uint8 i = 0; i < 2; ++i)
    {
        data << uint16(0); // RoundTimeSecs
        data << uint8(0);  // NextInputFlags
        data << uint8(2);  // NextTrapStatus
    }

    data << uint32(roundID);

    // cooldowns count
    data.WriteBits(0, 20);
    // effect count
    data.WriteBits(round->effects.size(), 22);

    for (std::list<PetBattleEffect*>::iterator itr = round->effects.begin(); itr != round->effects.end(); ++itr)
    {
        PetBattleEffect* effect = (*itr);

        if (!effect)
            return;

        data.WriteBit(!effect->stackDepth);
        data.WriteBit(!effect->abilityEffectID);
        data.WriteBit(!effect->casterPBOID);
        data.WriteBit(!effect->flags);

        // effect target count
        data.WriteBits(effect->targets.size(), 25);

        data.WriteBit(!effect->petBattleEffectType);

        for (std::list<PetBattleEffectTarget*>::iterator i = effect->targets.begin(); i != effect->targets.end(); ++i)
        {
            PetBattleEffectTarget* target = (*i);

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

    for (std::list<PetBattleEffect*>::iterator itr = round->effects.begin(); itr != round->effects.end(); ++itr)
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

        data << uint8(effect->casterPBOID);         // casterPBOID
        data << uint8(effect->petBattleEffectType); // PetBattleEffectType
    }

    data << uint8(2); // NextPetBattleState

    m_player->GetSession()->SendPacket(&data);

    if (enemy)
        SetActivePet(TEAM_ENEMY, index);
    else
        SetActivePet(TEAM_ALLY, index);

    delete round;
}

bool PetBattleWild::UseAbilityHandler(uint32 abilityID, uint32 roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(roundID);

    // check active pets
    PetBattleInfo* allyPet = GetActivePet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetActivePet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    // check on cheats
    if (!allyPet->HasAbility(abilityID))
        return false;

    // check speed and set attacker/victim of round
    // TODO: check some speed states and auras
    int8 firstPetX = allyPet->GetSpeed() > enemyPet->GetSpeed() ? allyPet->GetPetNumber() : enemyPet->GetPetNumber();
    PetBattleInfo* first = GetPetBattleInfo(firstPetX);

    if (!first)
        return false;

    // temporary, need moved attribute "team" to PetBattleInfo and rewrited init
    int8 secondPetX = -1;
    if (first->GetPetNumber() == allyPet->GetPetNumber())
        secondPetX = enemyPet->GetPetNumber();
    else
        secondPetX = allyPet->GetPetNumber();

    PetBattleInfo* second = GetPetBattleInfo(secondPetX);

    // some paranoia
    if (!second)
        return false;

    // some testing
    bool replace[2];
    replace[TEAM_ALLY] = false;
    replace[TEAM_ENEMY] = false;

    uint32 damage = 0;
    // TODO: script battle for wild pets
    uint32 castAbilityID = 0;
    if (first->GetPetNumber() == allyPet->GetPetNumber())
        castAbilityID = abilityID;
    else
    {
        if (PetBattleAbilityInfo* ainfo = first->GetAbilityInfoByIndex(0))
            castAbilityID = ainfo->GetID();
    }

    if (!castAbilityID)
        return false;

    if (PetBattleAbilityInfo* ainfo = first->GetAbilityInfoByID(castAbilityID))
    {
        damage = ainfo->CalculateDamage(first, second);
        round->ProcessAbilityDamage(first, second, castAbilityID, damage, 1);

        if (second->IsDead())
        {
            // added petID to died ID
            round->petXDiedNumbers.push_back(second->GetPetNumber());

            // set state DEAD (testing!)
            PetBattleEffect* effect = new PetBattleEffect(first->GetPetNumber(), first->GetVisualEffectIDByAbilityID(castAbilityID), 6, 0, 0, 1, 1);
            PetBattleEffectTarget* target = new PetBattleEffectTarget(second->GetPetNumber(), 4);
            effect->AddTarget(target);
            round->AddEffect(effect);

            // replace pet or end battle
            if (enemyPet->IsDead())
            {
                if (GetAlivePetCountInTeam(TEAM_ENEMY))
                    replace[TEAM_ENEMY] = true;
                else
                {
                    SetWinner(TEAM_ALLY);
                    nextRoundFinal = true;
                }
            }
            else
            {
                if (!GetAlivePetCountInTeam(TEAM_ALLY))
                {
                    SetWinner(TEAM_ENEMY);
                    nextRoundFinal = true;
                }
            }
        }
        else
        {
            uint32 damage = 0;
            // TODO: script battle for wild pets
            uint32 castAbilityID = 0;
            if (second->GetPetNumber() == allyPet->GetPetNumber())
                castAbilityID = abilityID;
            else
            {
                if (PetBattleAbilityInfo* ainfo = second->GetAbilityInfoByIndex(0))
                    castAbilityID = ainfo->GetID();
            }

            if (!castAbilityID)
                return false;

            if (PetBattleAbilityInfo* ainfo = second->GetAbilityInfoByID(castAbilityID))
            {
                damage = ainfo->CalculateDamage(second, first);
                round->ProcessAbilityDamage(second, first, castAbilityID, damage, 2);
            }
            else
                return false;
        }
    }
    else
        return false;

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // increase round number
    round->roundID++;

    CheckTrapStatuses(round);
    SendRoundResults(round);

    if (replace[TEAM_ENEMY])
        ReplacePetHandler(enemyPet->GetPetNumber(), enemyPet->GetPetNumber() + 1, GetPetIndexByPetNumber(enemyPet->GetPetNumber() + 1), round->roundID, true);

    delete round;
    return true;
}

bool PetBattleWild::SkipTurnHandler(uint32 _roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);

    // check active pets
    PetBattleInfo* allyPet = GetActivePet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetActivePet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    round->ProcessSkipTurn(allyPet);

    uint32 damage = 0;
    if (PetBattleAbilityInfo* ainfo = enemyPet->GetAbilityInfoByIndex(0))
    {
        damage = ainfo->CalculateDamage(enemyPet, allyPet);
        round->ProcessAbilityDamage(enemyPet, allyPet, ainfo->GetID(), damage, 1);
    }
    else
        return false;

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // increase round number
    round->roundID++;

    CheckTrapStatuses(round);
    SendRoundResults(round);

    delete round;
    return true;
}

bool PetBattleWild::UseTrapHandler(uint32 _roundID)
{
    // check active pets
    PetBattleInfo* allyPet = GetActivePet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetActivePet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    // cheater checks, TODO:
    if (allyPet->IsDead())
        return false;

    if (enemyPet->IsDead() || enemyPet->GetHealthPct() > 20)
        return false;

    // check chance - base implemented
    uint8 trapped = 0;
    if (roll_chance_i(70))
        trapped = 1;

    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);

    PetBattleEffect* effect = new PetBattleEffect(allyPet->GetPetNumber(), 698, 5, 0, 0, 1, 1);
    PetBattleEffectTarget * target = new PetBattleEffectTarget(enemyPet->GetPetNumber(), 1);
    target->status = trapped;

    effect->AddTarget(target);
    round->AddEffect(effect);

    // some testing
    bool replace[2];
    replace[TEAM_ALLY] = false;
    replace[TEAM_ENEMY] = false;

    if (!trapped)
    {
        // in response cast ability
    }
    else
    {
        enemyPet->SetCaptured(true);
        round->petXDiedNumbers.push_back(enemyPet->GetPetNumber());

        if (GetAlivePetCountInTeam(TEAM_ENEMY))
            replace[TEAM_ENEMY] = true;
        else
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

    SendRoundResults(round);

    if (replace[TEAM_ENEMY])
        ReplacePetHandler(enemyPet->GetPetNumber(), enemyPet->GetPetNumber() + 1, GetPetIndexByPetNumber(enemyPet->GetPetNumber() + 1), round->roundID, true);

    delete round;
    return true;
}

bool PetBattleWild::SwapPetHandler(uint8 newFrontPet, uint32 _roundID)
{
    PetBattleRoundResults* round = new PetBattleRoundResults(_roundID);

    // check active pets
    PetBattleInfo* allyPet = GetActivePet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetActivePet(TEAM_ENEMY);

    if (!allyPet || !enemyPet)
        return false;

    // simple combination of effect 4 and target type 3 - needed function
    PetBattleEffect* effect = new PetBattleEffect(allyPet->GetPetNumber(), 0, 4, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(newFrontPet, 3);

    effect->AddTarget(target);
    round->AddEffect(effect);

    SetActivePet(TEAM_ALLY, GetPetIndexByPetNumber(newFrontPet));
    // check active pets
    allyPet = GetActivePet(TEAM_ALLY);

    if (!allyPet)
        return false;

    uint32 damage = 0;
    if (PetBattleAbilityInfo* ainfo = enemyPet->GetAbilityInfoByIndex(0))
    {
        damage = ainfo->CalculateDamage(enemyPet, allyPet);
        round->ProcessAbilityDamage(enemyPet, allyPet, ainfo->GetID(), damage, 1);
    }
    else
        return false;

    round->AuraProcessingBegin();
    round->AuraProcessingEnd();

    // increase round number
    round->roundID++;

    CheckTrapStatuses(round);
    SendRoundResults(round);

    delete round;
    return true;
}

void PetBattleWild::CheckTrapStatuses(PetBattleRoundResults* round)
{
    // check active pets
    PetBattleInfo* allyPet = GetActivePet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetActivePet(TEAM_ENEMY);

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
    uint32 spell = enemyPet->GetSummonSpell();
    if (m_player->HasActiveSpell(spell))
        allyTrapStatus = PET_BATTLE_TRAP_ERR_5;

    if (allyPet->IsDead() || enemyPet->IsDead())
        allyTrapStatus = PET_BATTLE_TRAP_ERR_3;

    round->SetTrapStatus(TEAM_ALLY, allyTrapStatus);
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
        data.WriteBit(!effect->turnInstanceID); // !hasTurnInstanceID
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
                data.WriteBit(!target->remainingHealth); // !hasRemainingHealth

            if (target->type == 4)
            {
                data.WriteBit(0);
                data.WriteBit(0);
            }

            if (target->type == 1)
                data.WriteBit(0); // !hasUnk

            data.WriteBit(target->petX == -1 ? 1 : 0); // !hasPetX
        }

        data.WriteBit(!effect->sourceAuraInstanceID); // !hasSourceAuraInstanceID
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

            if (target->type == 4)
            {
                data << uint32(1);
                data << uint32(1);
            }

            if (target->petX != -1)
                data << uint8(target->petX); // targetPetX

            if (target->type == 6)
                data << int32(target->remainingHealth);

            if (target->type == 1)
                data << uint32(1);
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
        data << uint8(round->trapStatus[i]);  // NextTrapStatus
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

bool PetBattleWild::FinalRoundHandler(bool abandoned)
{
    PetBattleInfo* allyPet = GetActivePet(TEAM_ALLY);
    PetBattleInfo* enemyPet = GetActivePet(TEAM_ENEMY);

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
            /*BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(allyPet->GetSpeciesID(), BATTLE_PET_BREED_SS);
            accumulator->CalcQualityMultiplier(allyPet->GetQuality(), allyPet->GetNewLevel());
            uint32 health = accumulator->CalculateHealth();
            uint32 power = accumulator->CalculatePower();
            uint32 speed = accumulator->CalculateSpeed();
            delete accumulator;

            allyPet->SetHealth(health);
            allyPet->SetMaxHealth(health);
            allyPet->SetPower(power);
            allyPet->SetSpeed(speed);*/
        }
        else
            allyPet->SetTotalXP(newXp);
    }
    else
    {
        if (abandoned)
        {
            // all alive pets penalty
            for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PETS; ++j)
            {
                if (PetBattleInfo* petInfo = GetPetBattleInfo(TEAM_ALLY, j))
                {
                    if (petInfo->Vaild() && !petInfo->IsDead())
                    {
                        int32 percent10 = petInfo->GetHealth() / 10;
                        int32 newHealth = petInfo->GetHealth() - percent10;
                        petInfo->SetHealth(newHealth);
                    }
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
    uint8 totalPetsCount = 0;
    uint32 bit_pos = data.bitwpos();
    data.WriteBits(totalPetsCount, 20);

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PETS; ++j)
        {
            PetBattleInfo* petData = GetPetBattleInfo(i, j);

            if (!petData || !petData->Vaild())
                continue;

            data.WriteBit(petData->Captured() || petData->Caged()); // hasCaptured | hasCaged
            data.WriteBit(1); // hasSeenAction
            data.WriteBit(!petData->GetTotalXP()); // !hasXp
            data.WriteBit(petData->Captured() || petData->Caged()); // hasCaptured | hasCaged
            data.WriteBit(0); // !hasInitialLevel
            data.WriteBit(0); // !hasLevel
            data.WriteBit(petData->GetTotalXP() || (petData->GetLevel() < petData->GetNewLevel())); // awardedXP
        }
    }

    data.WriteBit(0); // pvpBattle

    // winners bits
    for (uint8 i = 0; i < 2; ++i)
        data.WriteBit(winners[i]);

    data.WriteBit(abandoned); // abandoned

    for (uint8 i = 0; i < 2; i++)
    {
        for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PETS; ++j)
        {
            PetBattleInfo* petData = GetPetBattleInfo(i, j);

            if (!petData || !petData->Vaild())
                continue;

            if (petData->GetTotalXP())
                data << uint16(petData->GetTotalXP());

            data << uint8(petData->GetPetNumber()); // Pboid
            data << uint32(petData->GetHealth()); // RemainingHealth
            data << uint16(petData->GetNewLevel());  // NewLevel
            data << uint32(petData->GetMaxHealth()); // maxHealth
            data << uint16(petData->GetLevel()); // InitialLevel

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

void PetBattleWild::UpdateLoadout()
{
    std::list<uint64> updates;
    updates.clear();

    // find trapped pets
    for (uint8 i = 0; i < 2; ++i)
    {
        for (uint8 j = 0; j < MAX_ACTIVE_BATTLE_PETS; ++j)
        {
            PetBattleInfo* battleInfo = GetPetBattleInfo(i, j);

            if (!battleInfo || !battleInfo->Vaild())
                continue;

            if (i == TEAM_ENEMY && !battleInfo->Captured())
                continue;

            // update loadout
            if (i == TEAM_ALLY)
            {
                PetJournalInfo* loadoutInfo = m_player->GetBattlePetMgr()->GetPetInfoByPetGUID(battleInfo->GetGUID());

                if (!loadoutInfo)
                    continue;

                // recalculate stats
                BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(battleInfo->GetSpeciesID(), BATTLE_PET_BREED_SS);
                accumulator->CalcQualityMultiplier(battleInfo->GetQuality(), battleInfo->GetNewLevel());
                uint32 health = accumulator->CalculateHealth();
                uint32 power = accumulator->CalculatePower();
                uint32 speed = accumulator->CalculateSpeed();
                delete accumulator;

                // update
                loadoutInfo->SetBreedID(BATTLE_PET_BREED_SS);
                loadoutInfo->SetLevel(battleInfo->GetNewLevel());
                if (battleInfo->IsDead())
                    loadoutInfo->SetHealth(0);
                else
                    loadoutInfo->SetHealth(battleInfo->GetHealth());
                loadoutInfo->SetQuality(battleInfo->GetQuality());
                loadoutInfo->SetXP(battleInfo->GetTotalXP());
                loadoutInfo->SetPower(power);
                loadoutInfo->SetSpeed(speed);
                loadoutInfo->SetMaxHealth(health);

                updates.push_back(battleInfo->GetGUID());
            }
            // update trapped pets
            else
            {
                if (!battleInfo->Captured())
                    continue;

                uint64 petguid = sObjectMgr->GenerateBattlePetGuid();

                BattlePetStatAccumulator* accumulator = new BattlePetStatAccumulator(battleInfo->GetSpeciesID(), battleInfo->GetBreedID());
                accumulator->CalcQualityMultiplier(battleInfo->GetQuality(), battleInfo->GetLevel());
                uint32 health = accumulator->CalculateHealth();
                uint32 power = accumulator->CalculatePower();
                uint32 speed = accumulator->CalculateSpeed();
                delete accumulator;

                m_player->GetBattlePetMgr()->AddPetInJournal(petguid, battleInfo->GetSpeciesID(), battleInfo->GetCreatureEntry(), battleInfo->GetLevel(), battleInfo->GetDisplayID(), power, speed, battleInfo->GetHealth(), health, battleInfo->GetQuality(), 0, 0, battleInfo->GetSummonSpell(), "", battleInfo->GetBreedID());
                // hack, fix it!
                m_player->learnSpell(battleInfo->GetSummonSpell(), false);

                std::list<uint64> newPets;
                newPets.clear();

                newPets.push_back(petguid);

                m_player->GetBattlePetMgr()->SendUpdatePets(newPets, true);
            }
        }
    }

    if (updates.size() > 0)
        m_player->GetBattlePetMgr()->SendUpdatePets(updates, false);
}

void PetBattleWild::FinishPetBattle(bool error)
{
    SendFinishPetBattle();
    if (!error)
        UpdateLoadout();
    m_player->GetBattlePetMgr()->CloseWildPetBattle();
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

uint32 PetBattleInfo::GetVisualEffectIDByAbilityID(uint32 abilityID, uint8 turnIndex)
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
                    return eEntry->ID;
            }
        }
    }

    return 0;
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

// PetBattleRoundResults
PetBattleRoundResults::~PetBattleRoundResults()
{
    // delete targets
    for (std::list<PetBattleEffect*>::iterator itr = effects.begin(); itr != effects.end(); ++itr)
        for (std::list<PetBattleEffectTarget*>::iterator itr2 = (*itr)->targets.begin(); itr2 != (*itr)->targets.end(); ++itr2)
            delete (*itr2);

    // delete effects
    for (std::list<PetBattleEffect*>::iterator itr = effects.begin(); itr != effects.end(); ++itr)
        delete (*itr);
}

void PetBattleRoundResults::ProcessAbilityDamage(PetBattleInfo* attacker, PetBattleInfo* victim, uint32 abilityID, uint32 damage, uint8 turnInstanceID)
{
    // simple combination of effect 0 and target type 6
    if (PetBattleAbilityInfo* ainfo = attacker->GetAbilityInfoByID(abilityID))
    {
        PetBattleEffect* effect = new PetBattleEffect(attacker->GetPetNumber(), attacker->GetVisualEffectIDByAbilityID(abilityID), 0, 0x1000, 0, turnInstanceID, 1);
        PetBattleEffectTarget* target = new PetBattleEffectTarget(victim->GetPetNumber(), 6);

        // process in battle
        int32 newHealth = victim->GetHealth() - damage;
        victim->SetHealth(newHealth);

        // process for round result
        target->remainingHealth = newHealth;

        effect->AddTarget(target);
        AddEffect(effect);
    }
}

void PetBattleRoundResults::ProcessSkipTurn(PetBattleInfo* attacker)
{
    // simple combination of effect 4 and target type 3
    // send PETBATTLE_EFFECT_FLAG_INVALID_TARGET for something? fake swap?
    PetBattleEffect* effect = new PetBattleEffect(attacker->GetPetNumber(), 0, 4, 1, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(attacker->GetPetNumber(), 3);

    effect->AddTarget(target);
    AddEffect(effect);
}

void PetBattleRoundResults::AuraProcessingBegin()
{
    PetBattleEffect* effect = new PetBattleEffect(-1, 0, 13, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(-1, 3);

    effect->targets.push_back(target);
    effects.push_back(effect);
}

void PetBattleRoundResults::AuraProcessingEnd()
{
    PetBattleEffect* effect = new PetBattleEffect(-1, 0, 14, 0, 0, 0, 0);
    PetBattleEffectTarget* target = new PetBattleEffectTarget(-1, 3);

    effect->targets.push_back(target);
    effects.push_back(effect);
}

int32 PetBattleAbilityInfo::GetBaseDamage(PetBattleInfo* attacker, uint32 effectIdx, uint32 turnIndex)
{
    uint32 basePoints = GetEffectProperties(BASEPOINTS, effectIdx, turnIndex);
    return basePoints * (1 + attacker->GetPower() * 0.05f);
}

int32 PetBattleAbilityInfo::CalculateDamage(PetBattleInfo* attacker, PetBattleInfo* victim)
{
    int32 baseDamage = GetBaseDamage(attacker);
    int32 cleanDamage = urand(baseDamage - 5, baseDamage + 5);
    // mods
    float mod = GetAttackModifier(GetType(), victim->GetType());

    return cleanDamage * mod;
}