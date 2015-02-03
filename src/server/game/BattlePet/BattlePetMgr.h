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

#define MAX_ACTIVE_BATTLE_PETS 3
#define MAX_BATTLE_PET_LEVEL 25

enum BattlePetInternalStates
{
    STATE_NORMAL  = 0,
    STATE_UPDATED = 1,
    STATE_DELETED = 2
};

enum BattlePetTeam
{
    TEAM_ALLY = 0,
    TEAM_ENEMY = 1
};

enum BattlePetBreeds
{
    BATTLE_PET_BREED_BB = 3,  // 25% health, 25% power, 25% speed
    BATTLE_PET_BREED_PP = 4,  // 100% power
    BATTLE_PET_BREED_SS = 5,  // 100% speed
    BATTLE_PET_BREED_HH = 6,  // 100% health
    BATTLE_PET_BREED_HP = 7,  // 45% health, 45% power
    BATTLE_PET_BREED_PS = 8,  // 45% power, 45% speed
    BATTLE_PET_BREED_HS = 9,  // 45% health, 45% speed
    BATTLE_PET_BREED_PB = 10, // 45% power, 20% health, 20% speed
    BATTLE_PET_BREED_SB = 11, // 45% speed, 20% power, 20% health
    BATTLE_PET_BREED_HB = 12, // 45% health, 20% power, 20% speed
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
    std::string GetCustomName() { return customName; }
    bool HasFlag(uint16 _flag) { return (flags & _flag) != 0; }
    void SetFlag(uint16 _flag) { if (!HasFlag(_flag)) flags |= _flag; }
    void RemoveFlag(uint16 _flag) { flags &= ~_flag; }
    void SetInternalState(uint8 state) { internalState = state; }
    BattlePetInternalStates GetInternalState() { return BattlePetInternalStates(internalState); }
    void SetXP(uint16 _xp) { xp = _xp; }
    uint16 GetXP() { return xp; }
    void SetLevel(uint8 _level) { level = _level; }
    uint8 GetLevel() { return level; }
    int32 GetHealth() { return health; }
    void SetHealth(int32 _health) { health = _health; }
    uint32 GetMaxHealth() { return maxHealth; }
    void SetMaxHealth(uint32 _maxHealth) { maxHealth = _maxHealth; }
    void SetPower(uint16 _power) { power = _power; }
    void SetSpeed(uint16 _speed) { speed = _speed; }
    uint16 GetSpeed() { return speed; }
    uint16 GetPower() { return power; }
    uint16 GetBreedID() { return breedID; }
    void SetBreedID(uint16 _breedID) { breedID = _breedID; }
    uint8 GetQuality() { return quality; }
    void SetQuality(uint8 _quality) { quality = _quality; }
    uint32 GetSpeciesID() { return speciesID; }
    uint32 GetDisplayID() { return displayID; }
    bool IsDead() { return health <= 0; }
    bool IsHurt() { return !IsDead() && health < maxHealth; }
    uint8 GetType()
    {
        BattlePetSpeciesBySpellIdMap::const_iterator it = sBattlePetSpeciesBySpellId.find(creatureEntry);
        if (it != sBattlePetSpeciesBySpellId.end())
            return it->second->petType;

        return 0;
    }
    uint32 GetActiveAbilityID(uint8 rank);
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
    void CalcQualityMultiplier(uint8 quality, uint8 level)    
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

struct PetBattleAbility
{
    PetBattleAbility(uint32 _ID) : ID(_ID) {}

    uint32 ID;
    uint8 type;
    float mods[10];
    uint16 cooldownRemaining;
    uint16 lockdownRemaining;

    void CalculateAbilityData();
    uint32 GetBasePoints(uint32 turnIndex, uint32 effectIdx);
    uint32 GetRequiredLevel();
    uint32 GetEffectIDByAbilityID();
};

struct PetBattleData
{
    PetBattleData(uint8 _petX, PetBattleSlot* _slot, PetInfo* _tempPet, bool _active) : petX(_petX), slot(_slot), tempPet(_tempPet), active(_active) {}

    uint8 petX;
    PetBattleSlot* slot;
    PetInfo* tempPet;
    PetBattleAbility* activeAbilities[3];
    bool active;

    PetInfo* GetPetInfo() { return tempPet; }
    uint8 GetPetNumber() { return petX; }
    PetBattleSlot* GetSlot() { return slot; }
};

struct PetBattleEffectTarget
{
    PetBattleEffectTarget(int8 _petX, uint8 _type) : petX(_petX), type(_type) {}

    uint8 type;
    int8 petX;
    // only SetHealth effect
    int32 remainingHealth;
};

struct PetBattleEffect
{
    PetBattleEffect(int8 _casterPBOID, uint32 _abilityEffectID, uint8 _petBattleEffectType, uint16 _flags, uint16 _sourceAuraInstanceID, uint16 _turnInstanceID, uint8 _stackDepth) :
        casterPBOID(_casterPBOID), abilityEffectID(_abilityEffectID), petBattleEffectType(_petBattleEffectType), flags(_flags), sourceAuraInstanceID(_sourceAuraInstanceID), turnInstanceID(_turnInstanceID), stackDepth(_stackDepth) {}

    std::list<PetBattleEffectTarget*> targets;
    uint32 abilityEffectID;
    uint16 flags;
    uint16 sourceAuraInstanceID;
    uint16 turnInstanceID;
    uint8 petBattleEffectType;
    int8 casterPBOID;
    uint8 stackDepth;
};

struct PetBattleRoundResults
{
    PetBattleRoundResults(uint32 round) : roundID(round) {}

    std::list<PetBattleEffect*> effects;
    uint32 roundID;
    std::vector<uint8> petXDiedNumbers;
};

struct PetBattleFinalRound
{
    std::vector<uint8> levelupPetNumbers;
    std::map<uint8, uint16> rewardedXP;

    uint16 GetRewardedXP(uint8 petNumber) 
    {
        std::map<uint8, uint16>::iterator itr = rewardedXP.find(petNumber);
        if (itr == rewardedXP.end())
            return 0;

        return itr->second;
    }

    bool isLevelUp(uint8 petNumber)
    {
        for (uint8 i = 0; i < levelupPetNumbers.size(); ++i)
        {
            if (levelupPetNumbers[i] == petNumber)
                return true;
        }

        return false;
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

    bool InitBattleData();
    void Prepare(ObjectGuid creatureGuid);

    PetBattleRoundResults* PrepareFirstRound(uint8 frontPet);
    void SendFirstRound(PetBattleRoundResults* firstRound);
    PetBattleRoundResults* UseAbility(uint32 abilityID, uint32 _roundID);
    void SendRoundResults(PetBattleRoundResults* round);
    PetBattleFinalRound* PrepareFinalRound();
    void SendFinalRound(PetBattleFinalRound* finalRound);
    //void SetCurrentRound(PetBattleRoundResults* round) { curRound = round; }
    //void SetFinalRound(PetBattleFinalRound* _finalRound) { finalRound = _finalRound; }

    void FinishPetBattle();
    void SendFinishPetBattle();

    PetBattleData* GetPetBattleData(uint8 team, uint8 index) { return battleData[team][index]; }
    PetBattleData* GetPetBattleData(uint8 petNumber) 
    {
        if (petNumber > 5)
            return NULL;

        uint8 index = petNumber;
        uint8 team = TEAM_ALLY;
        if (petNumber > 2)
        {
            index = petNumber - 3;
            team = TEAM_ENEMY;
        }

        return battleData[team][index];
    }

    PetBattleData* GetActivePet(uint8 team)
    {
        for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
        {
            if (!battleData[team][i])
                continue;

            if (battleData[team][i]->active)
                return battleData[team][i];
        }

        return NULL;
    }
    void SetActivePet(uint8 team, uint8 index)
    {
        // clear all
        for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
        {
            if (!battleData[team][i])
                return;

            battleData[team][i]->active = false;
        }
        // set needed
        battleData[team][index]->active = true;
    }
    bool NextRoundFinal() { return nextRoundFinal; }

private:
    Player* m_player;

protected:
    PetBattleData* battleData[2][MAX_ACTIVE_BATTLE_PETS];
    //PetBattleRoundResults* curRound;
    //PetBattleFinalRound* finalRound;
    uint32 petsCount[2];
    uint64 teamGuids[2];
    uint8 winners[2];
    bool nextRoundFinal;

};

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);
    ~BattlePetMgr()
    {
        for (PetJournal::const_iterator itr = m_PetJournal.begin(); itr != m_PetJournal.end(); ++itr)
            delete itr->second;

        for (int i = 0; i < MAX_ACTIVE_BATTLE_PETS; ++i)
            delete m_battleSlots[i];
    }

    void BuildPetJournal(WorldPacket *data);

    void AddPetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName = "", int16 breedID = 0, uint8 state = STATE_NORMAL);
    void AddPetBattleSlot(uint64 guid, uint8 slotID);

    void CloseWildPetBattle();
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
    uint64 GetPetGUIDBySlot(uint8 slotID)
    {
        if (m_battleSlots[slotID])
            return m_battleSlots[slotID]->GetPet();
    }

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

    uint32 GetXPForNextLevel(uint8 level);

private:
    Player* m_player;
    PetJournal m_PetJournal;
    PetBattleSlot* m_battleSlots[MAX_ACTIVE_BATTLE_PETS];
    PetBattleWild* m_petBattleWild;
};

#endif