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

#ifndef __TRINITY_BATTLEPETMGR_H
#define __TRINITY_BATTLEPETMGR_H

#include <map>
#include <string>

#include "Common.h"
#include <ace/Singleton.h>
#include "DatabaseEnv.h"
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DB2Stores.h"

#define MAX_ACTIVE_PETS 3

enum PetInternalStates
{
    STATE_NORMAL  = 0,
    STATE_UPDATED = 1,
    STATE_DELETED = 2
};

struct PetInfo
{
    PetInfo(uint32 _speciesID, uint32 _creatureEntry, uint8 _level, uint32 _display, uint16 _power, uint16 _speed, int32 _health, uint32 _maxHealth, uint8 _quality, uint16 _xp, uint16 _flags, uint32 _spellID, std::string _customName, int16 _breedID, uint8 state) :
        displayID(_display), power(_power), speed(_speed), maxHealth(_maxHealth),
        health(_health), quality(_quality), xp(_xp), level(_level), flags(_flags), speciesID(_speciesID), creatureEntry(_creatureEntry), summonSpellID(_spellID), customName(_customName), breedID(_breedID), internalState(state) {}

    // game vars
    uint32 speciesID;
    uint32 creatureEntry;
    uint32 displayID;
    uint16 power;
    uint16 speed;
    int32 health;
    uint32 maxHealth;
    uint8 quality;
    uint16 xp;
    uint8 level;
    uint16 flags;
    int16 breedID;
    uint32 summonSpellID;
    std::string customName;
    // service vars
    uint8 internalState;

    // helpers
    void SetCustomName(std::string name) { customName = name; }
    bool HasFlag(uint16 _flag) { return (flags & _flag) != 0; }
    void SetFlag(uint16 _flag) { if (!HasFlag(_flag)) flags |= _flag; }
    void RemoveFlag(uint16 _flag) { flags &= ~_flag; }
    void SetInternalState(uint8 state) { internalState = state; }
    uint16 GetXP() { return xp; }
    int32 GetHealth() { return health; }
    void SetHealth(int32 _health) { health = _health; }
    uint32 GetMaxHealth() { return maxHealth; }
    bool IsDead() { return health <= 0; }
    bool IsHurt() { return !IsDead() && health < maxHealth; }
    uint8 GetType()
    {
        BattlePetSpeciesBySpellIdMap::const_iterator it = sBattlePetSpeciesBySpellId.find(creatureEntry);
        if (it != sBattlePetSpeciesBySpellId.end())
            return it->second->petType;

        return 0;
    }
};

struct PetBattleSlot
{
    PetBattleSlot(uint64 _guid): petGUID(_guid) {}

    uint64 petGUID;

    // helpers
    bool IsEmpty() { return petGUID == 0; }
    void SetPet(uint64 guid) { petGUID = guid; }
    uint64 GetPet() { return petGUID; }
};

struct BattlePetStatAccumulator
{
    BattlePetStatAccumulator(uint32 _healthMod, uint32 _powerMod, uint32 _speedMod): healthMod(_healthMod), powerMod(_powerMod), speedMod(_speedMod), qualityMultiplier(0.0f) {}

    //uint64 guid;
    int32 healthMod;
    int32 powerMod;
    int32 speedMod;
    float qualityMultiplier;

    int32 CalculateHealth()
    {
        return int32((5.0f * healthMod * qualityMultiplier) + 100.0f + /*(unkDword16 * qualityMultiplier)*/ 0.5f);
    }
    int32 CalculateSpeed()
    {
        return int32((/*(speed * unkDword32 * 0.01f) +*/ speedMod * qualityMultiplier) + 0.5f);
    }
    int32 CalculatePower()
    {
        return int32((powerMod * qualityMultiplier) + 0.5f);
    }
    void GetQualityMultiplier(uint8 quality, uint8 level)    
    {
        for (uint32 i = 0; i < sBattlePetBreedQualityStore.GetNumRows(); ++i)
        {
            BattlePetBreedQualityEntry const* qEntry = sBattlePetBreedQualityStore.LookupEntry(i);

            if (!qEntry)
                continue;

            if (qEntry->quality == quality)
                qualityMultiplier = level * 0.01f * qEntry->qualityModifier;
        }
    }
    void Accumulate(uint32 stateID, int32 value)
    {
        BattlePetStateEntry const* state = sBattlePetStateStore.LookupEntry(stateID);

        if (!state)
            return;

        if (state->flags & 0x4)
            speedMod += value;
        /*else if (state->flags & 0x200)
            unkDword32 += value;*/
        /*else if (state->flags & 0x10)
            unkDword16 += value;*/
        else if (state->flags & 0x20)
            healthMod += value;
        else if (state->flags & 0x100)
            powerMod += value;
    }
};

typedef std::map<uint64, PetInfo*> PetJournal;

enum BattlePetFlags
{
    BATTLE_PET_FLAG_FAVORITE            = 0x01,
    BATTLE_PET_FLAG_REVOKED             = 0x04,
    BATTLE_PET_FLAG_LOCKED_FOR_CONVERT  = 0x08,
    BATTLE_PET_FLAG_CUSTOM_ABILITY_1    = 0x10,
    BATTLE_PET_FLAG_CUSTOM_ABILITY_2    = 0x20,
    BATTLE_PET_FLAG_CUSTOM_ABILITY_3    = 0x40,
};

enum BattlePetSpeciesFlags
{
    SPECIES_FLAG_UNK1            = 0x02,
    SPECIES_FLAG_UNK2            = 0x04,
    SPECIES_FLAG_CAPTURABLE      = 0x08,
    SPECIES_FLAG_CANT_TRADE      = 0x10, // ~(unsigned __int8)(v3->speciesFlags >> 4) & 1 (cannot put in cage)
    SPECIES_FLAG_UNOBTAINABLE    = 0x20,
    SPECIES_FLAG_UNIQUE          = 0x40, // (v2->speciesFlags >> 6) & 1)
    SPECIES_FLAG_CANT_BATTLE     = 0x80,
    SPECIES_FLAG_UNK3            = 0x200,
    SPECIES_FLAG_ELITE           = 0x400,
};

enum BattlePetSpeciesSource
{
    SOURCE_LOOT        = 0,
    SOURCE_QUEST       = 1,
    SOURCE_VENDOR      = 2,
    SOURCE_PROFESSION  = 3,
    SOURCE_PET_BATTLE  = 4,
    SOURCE_ACHIEVEMENT = 5,
    SOURCE_GAME_EVENT  = 6,
    SOURCE_PROMO       = 7,
    SOURCE_TCG         = 8,
    SOURCE_NOT_AVALIABLE   = 0xFFFFFFFF,
};

class PetBattleWild
{
public:
    PetBattleWild(Player* owner);
    void Prepare(ObjectGuid creatureGuid);
    void FirstRound();
    void RoundResults();
    void FinalRound();
    void FinishPetBattle();
    uint16 RewardXP(bool winner, bool& levelUp);

    void CalculateRoundData(int8 &state, uint32 _roundID);
    PetInfo* GetAllyPet() { return pets[0]; }
    uint32 GetEffectID(uint32 abilityID, uint8 effectIndex);

private:
    Player* m_player;

protected:
    PetInfo* pets[2];
    PetBattleSlot* battleslots[2];
    uint64 guids[2];
    uint32 abilities[2];
    uint32 effects[2];
    uint32 effectFlags[2];
    uint32 roundID;

};

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);
    ~BattlePetMgr()
    {
        for (PetJournal::const_iterator itr = m_PetJournal.begin(); itr != m_PetJournal.end(); ++itr)
            delete itr->second;

        for (int i = 0; i < MAX_ACTIVE_PETS; ++i)
            delete m_battleSlots[i];
    }

    void BuildPetJournal(WorldPacket *data);

    void AddPetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName = "", int16 breedID = 0, uint8 state = STATE_NORMAL);
    void AddPetBattleSlot(uint64 guid, uint8 slotID);

    void ClosePetBattle();
    void SendUpdatePets(bool added = false);

    void InitWildBattle(Player* initiator, ObjectGuid wildCreatureGuid);

    // generate stat functions (need rewrite in future)
    BattlePetStatAccumulator* InitStateValuesFromDB(uint32 speciesID, uint16 breedID);
    float GetQualityMultiplier(uint8 quality, uint8 level);

    Player* GetPlayer() const { return m_player; }
    PetInfo* GetPetInfoByPetGUID(uint64 guid)
    {
        PetJournal::const_iterator pet = m_PetJournal.find(guid);
        if (pet != m_PetJournal.end())
            return pet->second;

        return NULL;
    }

    void DeletePetByPetGUID(uint64 guid)
    {
        PetJournal::const_iterator pet = m_PetJournal.find(guid);
        if (pet == m_PetJournal.end())
            return;

        pet->second->internalState = STATE_DELETED;
    }

    uint64 GetPetGUIDBySpell(uint32 spell)
    {
        for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
        {
            PetInfo * pi = pet->second;

            if (!pi || pi->internalState == STATE_DELETED)
                continue;

            if (pi->summonSpellID == spell)
                return pet->first;
        }

        return 0;
    }

    bool HasPet(uint64 guid)
    {
        PetJournal::const_iterator pet = m_PetJournal.find(guid);
        if (pet != m_PetJournal.end())
            return true;

        return false;
    }

    const PetJournal &GetPetJournal() { return m_PetJournal; }

    BattlePetSpeciesEntry const* GetBattlePetSpeciesEntry(uint32 creatureEntry)
    {
        BattlePetSpeciesBySpellIdMap::const_iterator it = sBattlePetSpeciesBySpellId.find(creatureEntry);
        if (it != sBattlePetSpeciesBySpellId.end())
            return it->second;

        return NULL;
    }

    PetBattleSlot* GetPetBattleSlot(uint8 slotID) { return m_battleSlots[slotID]; }

    ObjectGuid InverseGuid(ObjectGuid guid)
    {
        return ((guid & 0x00000000000000FF) << 56) | ((guid & 0x000000000000FF00) << 40) | ((guid & 0x0000000000FF0000) << 24) | ((guid & 0x00000000FF000000) <<  8) |
        ((guid & 0x000000FF00000000) >>  8) | ((guid & 0x0000FF0000000000) >> 24) | ((guid & 0x00FF000000000000) >> 40) | ((guid & 0xFF00000000000000) >> 56);
    }

    PetBattleWild* GetPetBattleWild() { return m_petBattleWild; }

    // test functions
    uint32 GetAbilityID(uint32 speciesID, uint8 abilityIndex);
    uint32 GetEffectIDByAbilityID(uint32 abilityID);
    uint32 GetBasePoints(uint32 abilityID, uint32 turnIndex = 1, uint32 effectIdx = 1);
    uint8 GetAbilityType(uint32 abilityID);
    float GetAttackModifier(uint8 attackType, uint8 defenseType);

private:
    Player* m_player;
    PetJournal m_PetJournal;
    PetBattleSlot* m_battleSlots[MAX_ACTIVE_PETS];
    PetBattleWild* m_petBattleWild;
};

/*struct PetBattlePlayerUpdate
{
    PetInfo pets[MAX_ACTIVE_PETS];
};*/

#endif