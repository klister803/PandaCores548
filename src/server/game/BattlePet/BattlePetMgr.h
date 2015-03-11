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
#define MAX_ACTIVE_BATTLE_PET_ABILITIES 3
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

enum BattlePetDBFlags
{
    BATTLE_PET_FLAG_FAVORITE = 0x01,
    BATTLE_PET_FLAG_REVOKED = 0x04,
    BATTLE_PET_FLAG_LOCKED_FOR_CONVERT = 0x08,
    BATTLE_PET_FLAG_CUSTOM_ABILITY_1 = 0x10,
    BATTLE_PET_FLAG_CUSTOM_ABILITY_2 = 0x20,
    BATTLE_PET_FLAG_CUSTOM_ABILITY_3 = 0x40,
};

enum BattlePetDBSpeciesFlags
{
    SPECIES_FLAG_UNK1 = 0x02,
    SPECIES_FLAG_UNK2 = 0x04,
    SPECIES_FLAG_CAPTURABLE = 0x08,
    SPECIES_FLAG_CANT_TRADE = 0x10, // ~(unsigned __int8)(v3->speciesFlags >> 4) & 1 (cannot put in cage)
    SPECIES_FLAG_UNOBTAINABLE = 0x20,
    SPECIES_FLAG_UNIQUE = 0x40, // (v2->speciesFlags >> 6) & 1)
    SPECIES_FLAG_CANT_BATTLE = 0x80,
    SPECIES_FLAG_UNK3 = 0x200,
    SPECIES_FLAG_ELITE = 0x400,
};

enum BattlePetSpeciesSource
{
    SOURCE_LOOT = 0,
    SOURCE_QUEST = 1,
    SOURCE_VENDOR = 2,
    SOURCE_PROFESSION = 3,
    SOURCE_PET_BATTLE = 4,
    SOURCE_ACHIEVEMENT = 5,
    SOURCE_GAME_EVENT = 6,
    SOURCE_PROMO = 7,
    SOURCE_TCG = 8,
    SOURCE_NOT_AVALIABLE = 0xFFFFFFFF,
};

enum BattlePetEffectProperties
{
    BASEPOINTS = 0,
    ACCURACY   = 1
};

enum PetBattleEffectFlags
{
    PETBATTLE_EFFECT_FLAG_INVALID_TARGET = 0x1,
    PETBATTLE_EFFECT_FLAG_MISS = 0x2,
    PETBATTLE_EFFECT_FLAG_CRIT = 0x4,
    PETBATTLE_EFFECT_FLAG_BLOCKED = 0x8,
    PETBATTLE_EFFECT_FLAG_DODGE = 0x10,
    PETBATTLE_EFFECT_FLAG_HEAL = 0x20,
    PETBATTLE_EFFECT_FLAG_UNKILLABLE = 0x40,
    PETBATTLE_EFFECT_FLAG_REFLECT = 0x80,
    PETBATTLE_EFFECT_FLAG_ABSORB = 0x100,
    PETBATTLE_EFFECT_FLAG_IMMUNE = 0x200,
    PETBATTLE_EFFECT_FLAG_STRONG = 0x400,
    PETBATTLE_EFFECT_FLAG_WEAK = 0x800,
    PETBATTLE_EFFECT_FLAG_BASE = 0x1000,
};

enum TrapStatus
{
    PET_BATTLE_TRAP_ACTIVE = 1, // active trap button
    PET_BATTLE_TRAP_ERR_2 = 2,  // "Traps become available when one of your pets reaches level 3."
    PET_BATTLE_TRAP_ERR_3 = 3,  // "You cannot trap a dead pet."
    PET_BATTLE_TRAP_ERR_4 = 4,  // "The enemy pet's health is not low enough."
    PET_BATTLE_TRAP_ERR_5 = 5,  // "You already have 3 of this pet or your journal is full."
    PET_BATTLE_TRAP_ERR_6 = 6,  // "Your enemy pet is not caputurable."
    PET_BATTLE_TRAP_ERR_7 = 7,  // "Can't trap an NPC's pets"
    PET_BATTLE_TRAP_ERR_8 = 8,  // "Can't trap twice in same battle"
};

// demo
enum DeathPetResult
{
    HAVE_ALIVE_PETS_MORE_ONE_ALLY  = 0,
    LAST_ALIVE_PET_ALLY            = 1,
    HAVE_ALIVE_PETS_MORE_ONE_ENEMY = 2,
    NO_ALIVE_PETS_ALLY             = 3,
    NO_ALIVE_PETS_ENEMY            = 4
};

class PetJournalInfo
{

public:
    PetJournalInfo(uint32 _speciesID, uint32 _creatureEntry, uint8 _level, uint32 _display, uint16 _power, uint16 _speed, int32 _health, uint32 _maxHealth, uint8 _quality, uint16 _xp, uint16 _flags, uint32 _spellID, std::string _customName, int16 _breedID) :
        displayID(_display), power(_power), speed(_speed), maxHealth(_maxHealth),
        health(_health), quality(_quality), xp(_xp), level(_level), flags(_flags), speciesID(_speciesID), creatureEntry(_creatureEntry), summonSpellID(_spellID), customName(_customName), breedID(_breedID), internalState(STATE_NORMAL) {}

    // helpers
    void SetCustomName(std::string name) { customName = name; }
    std::string GetCustomName() { return customName; }
    bool HasFlag(uint16 _flag) { return (flags & _flag) != 0; }
    void SetFlag(uint16 _flag) { if (!HasFlag(_flag)) flags |= _flag; }
    uint16 GetFlags() { return flags; }
    void RemoveFlag(uint16 _flag) { flags &= ~_flag; }
    void SetInternalState(uint8 state) { internalState = state; }
    BattlePetInternalStates GetInternalState() { return BattlePetInternalStates(internalState); }
    void SetXP(uint16 _xp) { xp = _xp; }
    uint16 GetXP() { return xp; }
    void SetLevel(uint8 _level) { level = _level; }
    uint8 GetLevel() { return level; }
    int32 GetHealth() { return health; }
    float GetHealthPct() { return maxHealth ? 100.f * health / maxHealth : 0.0f; }
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
    uint32 GetSummonSpell() { return summonSpellID; }
    uint32 GetCreatureEntry() { return creatureEntry; }
    bool IsDead() { return health <= 0; }
    bool IsHurt() { return !IsDead() && health < maxHealth; }
    uint8 GetType()
    {
        BattlePetSpeciesBySpellIdMap::const_iterator it = sBattlePetSpeciesBySpellId.find(creatureEntry);
        if (it != sBattlePetSpeciesBySpellId.end())
            return it->second->petType;

        return 0;
    }
    uint32 GetAbilityID(uint8 rank);

private:
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
};

class PetBattleSlot
{
public:
    PetBattleSlot(uint64 _guid): petGUID(_guid) {}

    // helpers
    bool IsEmpty() { return petGUID == 0; }
    void SetPet(uint64 guid) { petGUID = guid; }
    uint64 GetPet() { return petGUID; }

private:
    uint64 petGUID;
};

class BattlePetStatAccumulator
{
public:
    BattlePetStatAccumulator(uint32 _speciesID, uint16 _breedID);

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

private:
    int32 healthMod;
    int32 powerMod;
    int32 speedMod;
    float qualityMultiplier;
};

class PetBattleInfo;
class PetBattleAbilityInfo
{
public:
    PetBattleAbilityInfo(uint32 _ID, uint32 _speciesID);

    uint32 GetID() { return ID; }
    uint32 GetType() { return Type; }
    int32 CalculateDamage(PetBattleInfo* attacker, PetBattleInfo* victim, bool crit);
    int16 CalculateHitResult(PetBattleInfo* attacker, PetBattleInfo* victim);
    int32 GetBaseDamage(PetBattleInfo* attacker, uint32 effectIdx = 0, uint32 turnIndex = 1);
    uint32 GetEffectProperties(uint8 properties, uint32 effectIdx = 0, uint32 turnIndex = 1);
    int8 GetRequiredLevel() { return requiredLevel; }
    float GetAttackModifier(uint8 attackType, uint8 defenseType);

private:
    uint32 ID;
    uint32 Type;
    uint32 turnCooldown;
    uint32 auraAbilityID;
    uint32 auraDuration;
    uint8 requiredLevel;
    uint8 rank;
};

class PetBattleAura
{
    uint32 ID;
    uint32 auraInstanceID; // slot
};

class PetBattleState
{

};

class PetBattleEnviroment
{

};

class PetBattleInfo
{

public:
    PetBattleInfo() {}
    void SetGUID(uint64 _guid) { guid = _guid; }
    uint64 GetGUID() { return guid; }
    void SetPetID(uint8 petNumber) { petX = petNumber; }
    uint8 GetPetID() { return petX; }
    void CopyPetInfo(PetJournalInfo* petInfo);
    void SetTeam(uint8 _team) { team = _team; }
    uint8 GetTeam() { return team; }
    void SetFrontPet(bool apply) { frontPet = apply; }
    bool IsFrontPet() { return frontPet; }
    void SetAbilityID(uint32 abilityID, uint8 index) { abilities[index] = abilityID; }
    uint32 GetAbilityID(uint8 index) { return abilities[index]; }
    void SetCaptured(bool apply)
    {
        captured = apply;
        caged = apply;
    }

    bool Captured() { return captured; }
    bool Caged() { return caged; }

    void SetCustomName(std::string name) { customName = name; }
    std::string GetCustomName() { return customName; }
    bool HasFlag(uint16 _flag) { return (flags & _flag) != 0; }
    void SetFlag(uint16 _flag) { if (!HasFlag(_flag)) flags |= _flag; }
    uint16 GetFlags() { return flags; }
    void RemoveFlag(uint16 _flag) { flags &= ~_flag; }
    void SetXP(uint16 _xp) { xp = _xp; }
    uint16 GetXP() { return xp; }
    void SetTotalXP(uint16 _xp) { totalXP = _xp; }
    uint16 GetTotalXP() { return totalXP; }
    void SetLevel(uint8 _level) { level = _level; }
    uint8 GetLevel() { return level; }
    void SetNewLevel(uint8 _level) { newLevel = _level; }
    uint8 GetNewLevel() { return newLevel; }
    int32 GetHealth() { return health; }
    float GetHealthPct() { return maxHealth ? 100.f * health / maxHealth : 0.0f; }
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
    uint32 GetSummonSpell() { return summonSpellID; }
    uint32 GetCreatureEntry() { return creatureEntry; }
    bool IsDead() { return health <= 0; }
    bool IsHurt() { return !IsDead() && health < maxHealth; }
    uint8 GetType()
    {
        BattlePetSpeciesBySpellIdMap::const_iterator it = sBattlePetSpeciesBySpellId.find(creatureEntry);
        if (it != sBattlePetSpeciesBySpellId.end())
            return it->second->petType;

        return 0;
    }
    bool HasAbility(uint32 abilityID)
    {
        for (uint8 i = 0; i < MAX_ACTIVE_BATTLE_PET_ABILITIES; ++i)
            if (abilities[i] == abilityID)
                return true;

        return false;
    }

private:
    uint64 guid;
    uint8 petX;
    uint8 team;
    uint32 speciesID;
    uint32 creatureEntry;
    uint32 displayID;
    uint16 power;
    uint16 speed;
    int32 health;
    uint32 maxHealth;
    uint8 quality;
    uint16 xp;
    uint16 totalXP;
    uint8 level;
    uint8 newLevel;
    uint16 flags;
    int16 breedID;
    uint32 summonSpellID;
    std::string customName;
    uint32 abilities[MAX_ACTIVE_BATTLE_PET_ABILITIES];
    std::map<uint32, uint8> abilityCooldowns;
    std::map<uint32, PetBattleAura*> auras;
    std::map<uint32, PetBattleState*> states;
    uint8 status;
    bool frontPet;
    bool captured;
    bool caged;
};

struct PetBattleEffectTarget
{
    PetBattleEffectTarget(int8 _petX, uint8 _type) : petX(_petX), type(_type), remainingHealth(0), status(0), stateID(0), stateValue(0),
        auraAbilityID(0), auraInstanceID(0), roundsRemaining(0) {}

    uint8 type;
    int8 petX;
    // type 6
    int32 remainingHealth;
    // type 1
    uint8 status;
    // type 4
    uint8 stateID;
    uint32 stateValue;
    // type 0
    uint32 auraAbilityID;
    uint32 auraInstanceID;
    uint32 roundsRemaining;
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

    void AddTarget(PetBattleEffectTarget * target) { targets.push_back(target); }
};

struct PetBattleRoundResults
{
    PetBattleRoundResults(uint32 round) : roundID(round) {}
    ~PetBattleRoundResults();

    std::list<PetBattleEffect*> effects;
    uint32 roundID;
    std::vector<uint8> petXDiedNumbers;
    uint8 trapStatus[2];
    uint8 inputFlags[2];

    void AddEffect(PetBattleEffect* effect) { effects.push_back(effect); }

    void ProcessAbilityDamage(PetBattleInfo* caster, PetBattleInfo* target, uint32 abilityID, uint32 effectID, uint8 turnInstanceID);
    void ProcessPetSwap(uint8 oldPetNumber, uint8 newPetNumber);
    void ProcessSkipTurn(uint8 petNumber);
    void ProcessSetState(PetBattleInfo* caster, PetBattleInfo* target, uint32 effectID, uint8 stateID, uint32 stateValue, uint8 turnInstanceID);
    void AuraProcessingBegin();
    void AuraProcessingEnd();

    void AddDeadPet(uint8 petNumber);

    void SetTrapStatus(uint8 team, uint8 status) { trapStatus[team] = status; }
    uint8 GetTrapStatus(uint8 team) { return trapStatus[team]; }
    void SetInputFlags(uint8 team, uint8 flags) { inputFlags[team] = flags; }
    uint8 GetInputFlags(uint8 team) { return inputFlags[team]; }
};

typedef std::map<uint64, PetJournalInfo*> PetJournal;
typedef std::map<uint8, PetBattleSlot*> PetBattleSlots;
typedef std::list<PetBattleInfo*> BattleInfo;

class PetBattleWild
{
public:
    PetBattleWild(Player* owner);

    bool CreateBattleInfo();
    bool PrepareBattleInfo(ObjectGuid creatureGuid);
    void Init(ObjectGuid creatureGuid);

    void SendFullUpdate(ObjectGuid creatureGuid);

    void ForceReplacePetHandler(uint32 roundID, uint8 newFrontPet, uint8 team);

    bool FirstRoundHandler(uint8 allyFrontPetID, uint8 enemyFrontPetID);
    bool UseAbilityHandler(uint32 abilityID, uint32 roundID);
    bool SkipTurnHandler(uint32 _roundID);
    bool UseTrapHandler(uint32 _roundID);
    bool SwapPetHandler(uint8 newFrontPet, uint32 _roundID);
    void SendFirstRound(PetBattleRoundResults* firstRound);
    void SendRoundResults(PetBattleRoundResults* round);
    void SendForceReplacePet(PetBattleRoundResults* round);
    bool FinalRoundHandler(bool abandoned);
    void SendFinalRound();
    //void SetCurrentRound(PetBattleRoundResults* round) { curRound = round; }
    //void SetFinalRound(PetBattleFinalRound* _finalRound) { finalRound = _finalRound; }
    void CheckTrapStatuses(PetBattleRoundResults* round);
    void CheckInputFlags(PetBattleRoundResults* round);
    void UpdatePetsAfterBattle();

    void FinishPetBattle(bool error = false);
    void SendFinishPetBattle();

    PetBattleInfo* GetFrontPet(uint8 team);
    void SetFrontPet(uint8 team, uint8 petNumber);

    bool NextRoundFinal() { return nextRoundFinal; }
    void SetAbandoned(bool apply) { abandoned = apply; }

    uint8 GetRandomQuailty();
    uint16 GetRandomBreedID();

    void SetWinner(uint8 team) 
    {
        for (uint8 i = 0; i < 2; ++i)
        {
            if (i == team)
                winners[i] = 1;
            else
                winners[i] = 0;
        }
    }
    uint8 GetWinner()
    {
        for (uint8 i = 0; i < 2; ++i)
        {
            if (winners[i] == 1)
                return i;
        }

        return 0;
    }

    uint8 GetTotalPetCountInTeam(uint8 team, bool onlyActive = false);

    // only demo
    int8 GetLastAlivePetID(uint8 team);

    uint8 GetTeamIndex(uint8 petNumber)
    {
        switch (petNumber)
        {
            case 0: return 0;
            case 1: return 1;
            case 2: return 2;
            case 3: return 0;
            case 4: return 1;
            case 5: return 2;
            default: return 0;
        }
    }

    uint8 GetTeamByPetID(uint8 petNumber)
    {
        switch (petNumber)
        {
            case 0:
            case 1:
            case 2:
                return 0;
            case 3:
            case 4:
            case 5:
                return 1;
            default: return 0;
        }
    }

    uint32 GetCurrentRoundID();
    void SetCurrentRoundID(uint32 roundID) { currentRoundID = roundID; }
    void SetBattleState(uint8 state) { petBattleState = state; }
    uint8 GetBattleState() { return petBattleState; }

    uint32 GetVisualEffectIDByAbilityID(uint32 abilityID, uint8 turnIndex = 1);

private:
    Player* m_player;

protected:
    BattleInfo battleInfo;
    //PetBattleRoundResults* curRound;
    //PetBattleFinalRound* finalRound;
    std::map<uint32, PetBattleEnviroment*> enviro;
    uint32 currentRoundID;
    uint64 teamGuids[2];
    uint8 winners[2];
    uint8 petBattleState;
    bool nextRoundFinal;
    bool abandoned;

};

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);
    ~BattlePetMgr()
    {
        for (PetJournal::const_iterator itr = m_PetJournal.begin(); itr != m_PetJournal.end(); ++itr)
            delete itr->second;

        for (PetBattleSlots::const_iterator itr = m_battleSlots.begin(); itr != m_battleSlots.end(); ++itr)
            delete itr->second;
    }

    bool BuildPetJournal(WorldPacket *data);
    void SendEmptyPetJournal();

    void AddPetToList(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName = "", int16 breedID = 0, uint8 state = STATE_NORMAL);
    void InitBattleSlot(uint64 guid, uint8 slotID);

    void CloseWildPetBattle();
    void SendUpdatePets(std::list<uint64> &updates, bool added = false);

    void CreateWildBattle(Player* initiator, ObjectGuid wildCreatureGuid);

    Player* GetPlayer() const { return m_player; }

    PetJournalInfo* GetPetInfoByPetGUID(uint64 guid)
    {
        PetJournal::const_iterator pet = m_PetJournal.find(guid);
        if (pet != m_PetJournal.end())
            return pet->second;

        return NULL;
    }

    uint32 GetPetCount(uint32 creatureEntry)
    {
        uint32 count = 0;

        for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
        {
            PetJournalInfo * pi = pet->second;

            if (!pi || pi->GetInternalState() == STATE_DELETED)
                continue;

            if (pi->GetCreatureEntry() == creatureEntry)
                ++count;
        }

        return count;
    }

    void DeletePetByPetGUID(uint64 guid)
    {
        PetJournal::const_iterator pet = m_PetJournal.find(guid);
        if (pet == m_PetJournal.end())
            return;

        pet->second->SetInternalState(STATE_DELETED);
    }

    uint64 GetPetGUIDBySpell(uint32 spell)
    {
        for (PetJournal::const_iterator pet = m_PetJournal.begin(); pet != m_PetJournal.end(); ++pet)
        {
            PetJournalInfo * pi = pet->second;

            if (!pi || pi->GetInternalState() == STATE_DELETED)
                continue;

            if (pi->GetSummonSpell() == spell)
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

    PetBattleSlot* GetPetBattleSlot(uint8 index) { return m_battleSlots[index]; }

    uint64 GetPetGUIDBySlot(uint8 index)
    {
        PetBattleSlots::const_iterator itr = m_battleSlots.find(index);
        if (itr != m_battleSlots.end())
            return itr->first;

        return 0;
    }

    bool SlotIsLocked(uint8 index);

    ObjectGuid InverseGuid(ObjectGuid guid)
    {
        return ((guid & 0x00000000000000FF) << 56) | ((guid & 0x000000000000FF00) << 40) | ((guid & 0x0000000000FF0000) << 24) | ((guid & 0x00000000FF000000) <<  8) |
        ((guid & 0x000000FF00000000) >>  8) | ((guid & 0x0000FF0000000000) >> 24) | ((guid & 0x00FF000000000000) >> 40) | ((guid & 0xFF00000000000000) >> 56);
    }

    PetBattleWild* GetPetBattleWild() { return m_petBattleWild; }

    uint32 GetXPForNextLevel(uint8 level);

private:
    Player* m_player;
    PetJournal m_PetJournal;
    PetBattleSlots m_battleSlots;
    PetBattleWild* m_petBattleWild;
};

#endif