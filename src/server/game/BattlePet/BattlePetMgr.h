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

struct PetInfo
{
    PetInfo(uint32 _speciesID, uint32 _creatureEntry, uint8 _level, uint32 _display, uint16 _power, uint16 _speed, uint32 _health, uint32 _maxHealth, uint8 _quality, uint16 _xp, uint16 _flags, uint32 _spellID, std::string _customName) :
        displayID(_display), power(_power), speed(_speed), maxHealth(_maxHealth),
        health(_health), quality(_quality), xp(_xp), level(_level), flags(_flags), speciesID(_speciesID), creatureEntry(_creatureEntry), summonSpellID(_spellID), customName(_customName), breedID(-1) {}

    uint32 speciesID;
    uint32 creatureEntry;
    uint32 displayID;
    uint16 power;
    uint16 speed;
    uint32 health;
    uint32 maxHealth;
    uint8 quality;
    uint16 xp;
    uint8 level;
    uint16 flags;
    int16 breedID;
    uint32 summonSpellID;
    std::string customName;

    // helpers
    void SetCustomName(std::string name) { customName = name; }
    void SetFlags(uint16 _flags) { flags = _flags; }
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

enum BattlePetSpeciesSource
{
    SOURCE_RANDOM_LOOT = 0,
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

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);

    void FillPetJournal();
    void BuildPetJournal(WorldPacket *data);

    void AddPetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName = "", int16 breedID = -1);

    void SendClosePetBattle();
    void SendUpdatePets(uint8 petCount);

    void GiveXP();

    Player* GetPlayer() const { return m_player; }
    PetInfo* GetPetInfoByPetGUID(uint64 guid)
    {
        PetJournal::const_iterator pet = m_PetJournal.find(guid);
        if (pet != m_PetJournal.end())
            return pet->second;

        return NULL;
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

private:
    Player* m_player;
    PetJournal m_PetJournal;
};

#endif