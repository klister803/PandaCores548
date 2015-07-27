/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "Opcodes.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include "WorldPacket.h"

struct ExpansionInfoStrunct
{
    uint8 raceOrClass;
    uint8 expansion;
};

ExpansionInfoStrunct classExpansionInfo[MAX_CLASSES - 1] =
{
    { CLASS_WARRIOR, EXP_VANILLA },
    { CLASS_PALADIN, EXP_VANILLA },
    { CLASS_HUNTER, EXP_VANILLA },
    { CLASS_ROGUE, EXP_VANILLA },
    { CLASS_PRIEST, EXP_VANILLA },
    { CLASS_DEATH_KNIGHT, EXP_WOTLK },
    { CLASS_SHAMAN, EXP_VANILLA },
    { CLASS_MAGE, EXP_VANILLA },
    { CLASS_WARLOCK, EXP_VANILLA },
    { CLASS_MONK, EXP_PANDARIA },
    { CLASS_DRUID, EXP_VANILLA }
};

ExpansionInfoStrunct raceExpansionInfo[MAX_PLAYABLE_RACES] =
{
    { RACE_HUMAN, EXP_VANILLA },
    { RACE_ORC, EXP_VANILLA },
    { RACE_DWARF, EXP_VANILLA },
    { RACE_NIGHTELF, EXP_VANILLA },
    { RACE_UNDEAD_PLAYER, EXP_VANILLA },
    { RACE_TAUREN, EXP_VANILLA },
    { RACE_GNOME, EXP_VANILLA },
    { RACE_TROLL, EXP_VANILLA },
    { RACE_GOBLIN, EXP_CATACLYSM },
    { RACE_BLOODELF, EXP_BC },
    { RACE_DRAENEI, EXP_BC },
    { RACE_WORGEN, EXP_CATACLYSM },
    { RACE_PANDAREN_NEUTRAL, EXP_PANDARIA },
    { RACE_PANDAREN_ALLI, EXP_PANDARIA },
    { RACE_PANDAREN_HORDE, EXP_PANDARIA }
};

void WorldSession::SendAuthResponse(uint8 code, bool hasAccountData, bool queued, uint32 queuePos)
{
    WorldPacket packet(SMSG_AUTH_RESPONSE);

    packet << uint8(code);
    packet.WriteBit(queued);
    if (queued)
        packet.WriteBit(1);

    packet.WriteBit(hasAccountData);
    std::string realmName = sWorld->GetRealmName();
    std::string trimmedName = sWorld->GetTrimmedRealmName();
    if (hasAccountData)
    {
        uint8 count5 = 1;
        packet.WriteBit(0);
        packet.WriteBits(count5, 21);
        packet.WriteBits(0, 21);
        packet.WriteBits(MAX_PLAYABLE_RACES, 23);
        packet.WriteBit(0);
        packet.WriteBit(0);

        for (uint8 i = 0; i < count5; ++i)
        {
            packet.WriteBits(realmName.size(), 8);
            packet.WriteBit(1);
            packet.WriteBits(trimmedName.size(), 8);
        }

        packet.WriteBit(0);
        packet.WriteBits(MAX_CLASSES - 1, 23);

        packet << uint32(0);
        packet << uint32(0);
        packet << uint8(Expansion());

        for (uint8 i = 0; i < MAX_PLAYABLE_RACES; ++i)
        {
            packet << uint8(raceExpansionInfo[i].expansion);
            packet << uint8(raceExpansionInfo[i].raceOrClass);
        }

        packet << uint8(Expansion());
        packet << uint32(0);

        for (uint8 i = 0; i < MAX_CLASSES - 1; ++i)
        {
            packet << uint8(classExpansionInfo[i].raceOrClass);
            packet << uint8(classExpansionInfo[i].expansion);
        }

        for (uint8 i = 0; i < count5; ++i)
        {
            packet.WriteString(realmName);
            packet.WriteString(trimmedName);
            packet << uint32(realmID);
        }

        packet << uint32(0);    //2957796664... 3495927968
        packet << uint32(0);    //69076... 43190
    }

    if (queued)
        packet << uint32(queuePos);

    SendPacket(&packet);
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPacket data(SMSG_CLIENTCACHE_VERSION, 4);
    data << uint32(version);
    SendPacket(&data);
}

void WorldSession::SendBattlePay()
{
    WorldPacket data(SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE, 7);
    data << uint32(0);
    data.WriteBits(0, 19);
    data.FlushBits();
    SendPacket(&data);
}

void WorldSession::SendDisplayPromo(int32 promo)
{
    WorldPacket data(SMSG_DISPLAY_PROMOTION, 7);
    data << promo;
    SendPacket(&data);
}