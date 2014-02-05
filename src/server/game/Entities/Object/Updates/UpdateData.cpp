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
#include "WorldPacket.h"
#include "UpdateData.h"
#include "Log.h"
#include "Opcodes.h"
#include "World.h"

UpdateData::UpdateData(uint16 map) : m_map(map), m_build(false)
{
}

void UpdateData::AddOutOfRangeGUID(std::set<uint64>& guids)
{
    ASSERT(!m_build);
    m_outOfRangeGUIDs.insert(guids.begin(), guids.end());
}

void UpdateData::AddOutOfRangeGUID(uint64 guid)
{
    ASSERT(!m_build);
    m_outOfRangeGUIDs.insert(guid);
}

void UpdateData::AddUpdateBlock(const ByteBuffer &block)
{
    ASSERT(!m_build);
    m_blocks.push_back(block);
}

bool UpdateData::BuildPacket()
{
    if (!HasData())
        return false;

    static uint32 const maxBlockCount = 50;
    bool outOfRangeAdded = false;
    uint32 blockCount = m_blocks.size();
    do
    {
        uint32 count = std::min(blockCount, maxBlockCount);
        WorldPacket packet(SMSG_UPDATE_OBJECT, (m_outOfRangeGUIDs.empty() ? 0 : (1 + 4 + 9 * (outOfRangeAdded ? m_outOfRangeGUIDs.size() : 0)) + count));

        packet << uint16(m_map);
        packet << uint32(count + (!m_outOfRangeGUIDs.empty() && !outOfRangeAdded ? 1 : 0));

        if (!m_outOfRangeGUIDs.empty() && !outOfRangeAdded)
        {
            outOfRangeAdded = true;
            packet << uint8(UPDATETYPE_OUT_OF_RANGE_OBJECTS);
            packet << uint32(m_outOfRangeGUIDs.size());

            for (std::set<uint64>::const_iterator i = m_outOfRangeGUIDs.begin(); i != m_outOfRangeGUIDs.end(); ++i)
                packet.appendPackGUID(*i);
        }

        while (count > 0)
        {
            packet.append(m_blocks.front());
            m_blocks.pop_front();
            --count;
            --blockCount;
        }

        packets.push_back(packet);
    }
    while (blockCount > 0);

    m_build = true;

    return true;
}

void UpdateData::SendTo(Player* player)
{
    if (!m_build)
        BuildPacket();

    if (!m_build)
        return;

    for (std::list<WorldPacket>::iterator itr = packets.begin(); itr != packets.end(); ++itr)
        player->SendDirectMessage(&*itr);
}

void UpdateData::Clear()
{
    m_outOfRangeGUIDs.clear();
    m_blocks.clear();
    m_map = 0;
    m_build = false;
    packets.clear();
}

