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

struct BattlePetJournalData
{
    BattlePetJournalData(uint32 _speciesID, uint32 _creatureEntry, uint8 _level, uint32 _display, uint16 _power, uint16 _speed, uint32 _health, uint32 _maxHealth, uint8 _quality, uint16 _xp, uint16 _flags, uint32 _spellID, std::string _customName) :
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

    void SetCustomName(std::string name) { customName = name; }
    void SetFlags(uint16 _flags) { flags = _flags; }
};

typedef std::map<uint64, BattlePetJournalData*> BattlePetJournal;

enum BattlePetFlags
{
    BATTLE_PET_FLAG_FAVORITE            = 0x01,
    BATTLE_PET_FLAG_REVOKED             = 0x04,
    BATTLE_PET_FLAG_LOCKED_FOR_CONVERT  = 0x08,
};

enum BattlePetData
{
    BREED_ID    = 0,
    FLAGS       = 1,
    CUSTOM_NAME = 2,
    LEVEL       = 3,
    QUALITY     = 4,
    DISPLAY_ID  = 5,
    SPEED       = 6,
    POWER       = 7,
    HEALTH      = 8,
    MAX_HEALTH  = 9,
    EXP         = 10,
};

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);

    void FillBattlePetJournal();
    void BuildBattlePetJournal(WorldPacket *data);

    void AddBattlePetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint8 level, uint32 display, uint16 power, uint16 speed, uint32 health, uint32 maxHealth, uint8 quality, uint16 xp, uint16 flags, uint32 spellID, std::string customName = "", int16 breedID = -1);

    void SendClosePetBattle();
    void SendUpdatePets(uint8 petCount);

    void GiveXP();

    Player* GetPlayer() const { return m_player; }
    BattlePetJournalData* GetBattlePetData(uint64 guid)
    {
        BattlePetJournal::const_iterator pet = m_battlePetJournal.find(guid);
        if (pet != m_battlePetJournal.end())
            return pet->second;

        return NULL;
    }

    //template<typename T> 
    //void SetBattlePetData(uint64 guid, BattlePetData bpd, T value);

    bool HasBattlePet(uint64 guid)
    {
        BattlePetJournal::const_iterator pet = m_battlePetJournal.find(guid);
        if (pet != m_battlePetJournal.end())
            return true;

        return false;
    }
    const BattlePetJournal &GetBattlePetJournal() { return m_battlePetJournal; }

private:
    Player* m_player;
    BattlePetJournal m_battlePetJournal;
};

#endif