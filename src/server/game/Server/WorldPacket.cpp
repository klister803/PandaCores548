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

#include <zlib.h>
#include "WorldPacket.h"
#include "World.h"

//! Compresses packet in place
void WorldPacket::Compress(z_stream* compressionStream)
{
    Opcodes uncompressedOpcode = GetOpcode();
    if (uncompressedOpcode == SMSG_COMPRESSED_OPCODE)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Packet with opcode 0x%04X is already compressed!", uncompressedOpcode);
        return;
    }

    Opcodes opcode = SMSG_COMPRESSED_OPCODE;
    uint32 size = wpos();
    uint32 destsize = compressBound(size);

    std::vector<uint8> storage(destsize);

    _compressionStream = compressionStream;
    if (!Compress(static_cast<void*>(&storage[0]), &destsize, static_cast<const void*>(contents()), size))
        return;

    clear();
    reserve(destsize + sizeof(uint32)*3);
    *this << uint32(size);
    *this << uint32(destsize);
    *this << uint32(uncompressedOpcode);
    append(&storage[0], destsize);
    SetOpcode(opcode);

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Successfully compressed opcode %u (len %u) to %u (len %u)", uncompressedOpcode, size, opcode, destsize);
}

//! Compresses another packet and stores it in self (source left intact)
bool WorldPacket::Compress(z_stream* compressionStream, WorldPacket const* source)
{
    ASSERT(source != this);

    Opcodes uncompressedOpcode = source->GetOpcode();
    if (uncompressedOpcode == SMSG_COMPRESSED_OPCODE)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Packet with opcode 0x%04X is already compressed!", uncompressedOpcode);
        return false;
    }

    Opcodes opcode = SMSG_COMPRESSED_OPCODE;
    uint32 size = source->wpos();
    uint32 destsize = compressBound(size);

    size_t sizePos = 0;
    resize(destsize + 12);

    uint32 adler_origina = adler32( 0x9827D8F1u, static_cast<const Bytef*>(source->contents()), size);

    _compressionStream = compressionStream;
    if (!Compress(&_storage[0] + 12, &destsize, static_cast<const void*>(source->contents()), size);
        return false;

    uint32 adler_compred = adler32( 0x9827D8F1u, static_cast<Bytef*>(&_storage[0] + 12), destsize); 

    put<uint32>(0, size);
    put<uint32>(4, adler_compred);
    put<uint32>(8, adler_origina);
    resize(destsize + 12);

    SetOpcode(opcode);

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Successfully compressed opcode %u (len %u) to %u (len %u) adler(before %u| after %u)", uncompressedOpcode, size, opcode, destsize, adler_origina, adler_compred);
    return true;
}

bool WorldPacket::Compress(void* dst, uint32 *dst_size, const void* src, int src_size, int flush)
{
    _compressionStream->next_out = (Bytef*)dst;
    _compressionStream->avail_out = *dst_size;
    _compressionStream->next_in = (Bytef*)src;
    _compressionStream->avail_in = (uInt)src_size;

    int32 z_res = deflate(_compressionStream, Z_SYNC_FLUSH);
    if (z_res != Z_OK)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Can't compress packet (zlib: deflate) Error code: %i (%s, msg: %s)", z_res, zError(z_res), _compressionStream->msg);
        *dst_size = 0;
        return false;
    }

    if (_compressionStream->avail_in != 0)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Can't compress packet (zlib: deflate not greedy)");
        *dst_size = 0;
        return false;
    }

    *dst_size -= _compressionStream->avail_out;
    return true;
}