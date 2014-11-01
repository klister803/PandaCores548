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
#include "ByteBuffer.h"
#include "WorldPacket.h"
#include "UpdateData.h"
#include "Log.h"
#include "Opcodes.h"
#include "World.h"
#include "zlib.h"

UpdateData::UpdateData(uint16 map) : m_map(map)/*, m_blockCount(0)*/
{
}

void UpdateData::AddOutOfRangeGUID(std::set<uint64>& guids)
{
    m_outOfRangeGUIDs.insert(guids.begin(), guids.end());
}

void UpdateData::AddOutOfRangeGUID(uint64 guid)
{
    m_outOfRangeGUIDs.insert(guid);
}

void UpdateData::AddUpdateBlock(const ByteBuffer &block)
{
    m_blocks.push_back(block);
}

bool UpdateData::BuildPacket(std::list<WorldPacket*>& packets)
{
    if (!HasData())
        return false;

    static uint32 const maxBlockCount = 50;
    bool outOfRangeAdded = false;
    uint32 blockCount = m_blocks.size();
    do
    {
        uint32 count = std::min(blockCount, maxBlockCount);
        WorldPacket* packet = new WorldPacket(SMSG_UPDATE_OBJECT, (m_outOfRangeGUIDs.empty() ? 0 : (1 + 4 + 9 * (outOfRangeAdded ? m_outOfRangeGUIDs.size() : 0)) + count));

        *packet << uint16(m_map);
        *packet << uint32(count + (!m_outOfRangeGUIDs.empty() && !outOfRangeAdded ? 1 : 0));

        if (!m_outOfRangeGUIDs.empty() && !outOfRangeAdded)
        {
            outOfRangeAdded = true;
            *packet << uint8(UPDATETYPE_OUT_OF_RANGE_OBJECTS);
            *packet << uint32(m_outOfRangeGUIDs.size());

            for (std::set<uint64>::const_iterator i = m_outOfRangeGUIDs.begin(); i != m_outOfRangeGUIDs.end(); ++i)
                packet->appendPackGUID(*i);
        }

        for (BlockList::const_iterator itr = m_blocks.begin(); itr != m_blocks.end() && count > 0; ++itr, --count, --blockCount)
            packet->append(*itr);

        packets.push_back(packet);
    }
    while (blockCount > 0);

    return true;
}

void UpdateData::Clear()
{
    m_outOfRangeGUIDs.clear();
    m_blocks.clear();
    m_map = 0;
}

