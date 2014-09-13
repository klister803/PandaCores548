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

struct PetBattleData
{
    PetBattleData(BattlePetSpeciesEntry const* speciesEntry, uint32 level, uint32 display, uint32 power, uint32 speed, uint32 health, uint32 maxHealth, uint32 quality, uint32 experience, uint32 spellId) :
        m_displayID(display), m_power(power), m_speed(speed), m_maxHealth(maxHealth),
        m_health(health), m_quality(quality), m_experience(experience), m_speciesEntry(speciesEntry), m_level(level), m_spellId(spellId) {}

    uint32 m_displayID;
    uint32 m_power;
    uint32 m_speed;
    uint32 m_health;
    uint32 m_maxHealth;
    uint32 m_quality;
    uint32 m_experience;
    uint32 m_level;
    uint32 m_spellId;

    BattlePetSpeciesEntry const* m_speciesEntry;
};

typedef std::list<PetBattleData> PetBattleDataList;

class BattlePetMgr
{
public:
    explicit BattlePetMgr(Player* owner);

    void BuildBattlePetJournal(WorldPacket *data);
    void GetBattlePetList(PetBattleDataList &petBattleList) const;
    void SendClosePetBattle(Player * plr);
    Player* GetPlayer() const { return m_player; }

private:
    Player* m_player;
};

enum BattlePetFlags
{
    BATTLE_PET_FLAG_FAVORITE            = 1,
    BATTLE_PET_FLAG_REVOKED             = 4,
    BATTLE_PET_FLAG_LOCKED_FOR_CONVERT  = 8,
};

#endif