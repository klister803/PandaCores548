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
#include "WorldSession.h"
#include "WorldPacket.h"

void WorldSession::SendAuthResponse(uint8 code, bool queued, uint32 queuePos)
{
    WorldPacket packet(SMSG_AUTH_RESPONSE);
    
    uint8 PacketRaw[] = {0x80, 0x00, 0x01, 0x60, 0x00, 0x00, 0x80, 0x00, 0x03, 0xC0, 0xC4, 0x00, 0x00, 0x2C, 0x10, 0x00, 0x04, 0x0A, 0x04, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09, 0x00, 0x0B, 0x00, 0x06, 0x02, 0x02, 0x00, 0x00, 0x00, 0x4C, 0x65, 0x76, 0x65, 0x6C, 0x20, 0x38, 0x35, 0x06, 0x03, 0x0B, 0x03, 0x03, 0x03, 0x09, 0x03, 0x08, 0x03, 0x0A, 0x03, 0x02, 0x03, 0x05, 0x03, 0x04, 0x03, 0x07, 0x03, 0x01, 0x03, 0x06, 0x05, 0x0B, 0x05, 0x03, 0x05, 0x09, 0x05, 0x08, 0x05, 0x0A, 0x05, 0x02, 0x05, 0x05, 0x05, 0x04, 0x05, 0x07, 0x05, 0x01, 0x05, 0x50, 0x72, 0x65, 0x6D, 0x61, 0x64, 0x65, 0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74, 0x65, 0x72, 0x73, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x6C, 0x65, 0x76, 0x65, 0x6C, 0x20, 0x38, 0x35, 0x2B, 0x20, 0x63, 0x6F, 0x6E, 0x74, 0x65, 0x6E, 0x74, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB2, 0x96, 0xC5, 0x00, 0x0B, 0x01, 0x16, 0x01, 0x0A, 0x01, 0x04, 0x00, 0x03, 0x00, 0x19, 0x01, 0x1A, 0x01, 0x06, 0x00, 0x05, 0x00, 0x02, 0x00, 0x07, 0x00, 0x09, 0x01, 0x01, 0x00, 0x08, 0x00, 0x18, 0x01, 0x04, 0x0C };
    for(int i = 0; i < 188; i++)
        packet << uint8(PacketRaw[i]);
    SendPacket(&packet);
    return;

    uint32 realmRaceCount = 15;
    uint32 realmClassCount = 11;

    packet.WriteBit(1);                                    // has account info
    packet.WriteBit(0);                                    // Unk
    packet.WriteBits(realmClassCount, 25);                  // Read realmRaceResult.count // 11 (class ?)
    packet.WriteBits(0, 22);                               // Unk
    packet.WriteBits(realmRaceCount, 25);                 // Read realmClassResult.count // 15 (race ?)

    packet.WriteBit(queued);
    if(queued)
    {
        packet.WriteBit(0);
        packet << uint32(queuePos);
    }
    packet.FlushBits();

    packet << uint8(0);
    packet << uint8(Expansion());

    for(uint32 i = 0; i < realmClassCount; i++)
    {
        switch(i)
        {
            case 0:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_WARRIOR);
                break;
            case 1:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_PALADIN);
                break;
            case 2:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_HUNTER);
                break;
            case 3:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_ROGUE);
                break;
            case 4:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_PRIEST);
                break;
            case 5:
                packet << uint8(2); // Wotlk
                packet << uint8(CLASS_DEATH_KNIGHT);
                break;
            case 6:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_SHAMAN);
                break;
            case 7:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_MAGE);
                break;
            case 8:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_WARLOCK);
                break;
            case 9:
                packet << uint8(0); // Prebc
                packet << uint8(CLASS_DRUID);
                break;
            case 10:
                packet << uint8(4); // MoP
                packet << uint8(CLASS_MONK);
                break;
        }
    }

    packet << uint32(0);
    packet << uint32(0);
    packet << uint32(0);

    packet << uint8(0);
    packet << uint8(RACE_HUMAN);
    packet << uint8(0);
    packet << uint8(RACE_ORC);
    packet << uint8(0);
    packet << uint8(RACE_DWARF);
    packet << uint8(0);
    packet << uint8(RACE_NIGHTELF);
    packet << uint8(0);
    packet << uint8(RACE_UNDEAD_PLAYER);
    packet << uint8(0);
    packet << uint8(RACE_TAUREN);
    packet << uint8(0);
    packet << uint8(RACE_GNOME);
    packet << uint8(0);
    packet << uint8(RACE_TROLL);
    packet << uint8(3);
    packet << uint8(RACE_GOBLIN);
    packet << uint8(1);
    packet << uint8(RACE_BLOODELF);
    packet << uint8(1);
    packet << uint8(RACE_DRAENEI);
    packet << uint8(3);
    packet << uint8(RACE_WORGEN);
    packet << uint8(4);
    packet << uint8(RACE_PANDAREN_NEUTRAL);
    packet << uint8(4);
    packet << uint8(RACE_PANDAREN_ALLI);
    packet << uint8(4);
    packet << uint8(RACE_PANDAREN_HORDE);
    /*for(uint32 i = 0; i < realmRaceCount; i++)
    {
        packet << uint8(0);                                // expansion
        packet << uint8(0);                                // class
    }*/

    packet << uint8(0);                                    // BillingPlanFlags
    packet << uint8(code);   

    SendPacket(&packet);
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPacket data(SMSG_CLIENTCACHE_VERSION, 4);
    data << uint32(version);
    SendPacket(&data);
}
