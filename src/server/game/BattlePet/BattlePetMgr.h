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
    BattlePetJournalData(uint32 _speciesID, uint32 _creatureEntry, uint32 _level, uint32 _display, uint32 _power, uint32 _speed, uint32 _health, uint32 _maxHealth, uint32 _quality, uint32 _xp, uint32 _flags) :
        displayID(_display), power(_power), speed(_speed), maxHealth(_maxHealth),
        health(_health), quality(_quality), xp(_xp), level(_level), flags(_flags), speciesID(_speciesID), creatureEntry(_creatureEntry) {}

    uint32 speciesID;
    uint32 creatureEntry;
    uint32 displayID;
    uint32 power;
    uint32 speed;
    uint32 health;
    uint32 maxHealth;
    uint32 quality;
    uint32 xp;
    uint32 level;
    uint32 flags;

    //BattlePetSpeciesEntry const* m_speciesEntry;
};

typedef std::map<uint64, BattlePetJournalData*> BattlePetJournal;

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);

    void FillBattlePetJournal();
    void BuildBattlePetJournal(WorldPacket *data);

    void AddBattlePetInJournal(uint64 guid, uint32 speciesID, uint32 creatureEntry, uint32 level, uint32 display, uint32 power, uint32 speed, uint32 health, uint32 maxHealth, uint32 quality, uint32 xp, uint32 flags);

    void SendClosePetBattle();
    void SendUpdatePets(uint8 petCount);

    void GiveXP();

    Player* GetPlayer() const { return m_player; }
    const BattlePetJournal &GetBattlePetJournal() { return m_battlePetJournal; }

private:
    Player* m_player;
    BattlePetJournal m_battlePetJournal;
};

enum BattlePetFlags
{
    BATTLE_PET_FLAG_FAVORITE            = 1,
    BATTLE_PET_FLAG_REVOKED             = 4,
    BATTLE_PET_FLAG_LOCKED_FOR_CONVERT  = 8,
};

#endif