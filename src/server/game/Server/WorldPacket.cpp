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
#include "ByteConverter.h"

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
    if (!Compress(static_cast<void*>(&storage[0]), &destsize, static_cast<const void*>(contents()), size, Z_SYNC_FLUSH))
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
    uint32 size = source->wpos()+4;
    uint32 destsize = compressBound(size);
    uint32 size2 = destsize;
    size_t sizePos = 0;

    ByteBuffer b(size);
    b << uint32(uncompressedOpcode);
    b.append(source->contents(), size-4);

    uint32 adler_origina = adler32( 0x9827D8F1u, b.contents(), size);

    //source->appenToBegin<uint32>(uncompressedOpcode);
    //source->contents()->insert(source->contents()->begin(), 4, uncompressedOpcode)
    std::vector<uint8> storage(destsize);
    
    //! ToDo: client alway send inflate result -3 with msg: invalid stored block lengths
    if (!Compress(&storage[0], &destsize, b.contents(), size, Z_SYNC_FLUSH))
        return false;

    uint32 adler_compred = adler32( 0x9827D8F1u, &storage[0], destsize); 

    clear();
    reserve(destsize + 12);

    *this << uint32(size);
    *this << uint32(adler_origina);
    *this << uint32(adler_compred);
    append(&storage[0], destsize);

    SetOpcode(opcode);

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Successfully compressed opcode %u (len %u) to %u (len %u) adler(before %u| after %u) size %u wp %u)", uncompressedOpcode, size, opcode, destsize, adler_origina, adler_compred, this->size(), wpos());

    // TEST
    /*uLongf destsize2 = compressBound(size);
    ByteBuffer dest(size);
    z_stream stream;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;  
    stream.next_out = (Bytef*)(dest.contents());
    stream.avail_out = size;
    stream.next_in = (Bytef*)&storage[0];
    stream.avail_in = (uInt)destsize;
    int z_res = inflateInit(&stream);
    if (z_res != Z_OK) return false;
    z_res = inflate(&stream, Z_SYNC_FLUSH);
    if (z_res != Z_STREAM_END)
        inflateEnd(&stream);
    else
        z_res = inflateEnd(&stream);
    sLog->outError(LOG_FILTER_NETWORKIO, "zlib:Inflate Error code: %i (%s) size %i s2 %i", z_res, zError(z_res), size, size2);*/
    // END TEST
    return true;
}

//! Z_FINISH | Z_SYNC_FLUSH | Z_NO_FLUSH
bool WorldPacket::Compress(void* dst, uint32 *dst_size, const void* src, int src_size, int flush)
{
    z_stream c_stream;

    c_stream.zalloc = (alloc_func)0;
    c_stream.zfree = (free_func)0;
    c_stream.opaque = (voidpf)0;

    // default Z_BEST_SPEED (1)
    int z_res = deflateInit(&c_stream, sWorld->getIntConfig(CONFIG_COMPRESSION));
    if (z_res != Z_OK)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Can't compress update packet (zlib: deflateInit) Error code: %i (%s)", z_res, zError(z_res));
        *dst_size = 0;
        return false;
    }

    c_stream.next_out = (Bytef*)dst;
    c_stream.avail_out = *dst_size;
    c_stream.next_in = (Bytef*)src;
    c_stream.avail_in = (uLongf)src_size;

    z_res = deflate(&c_stream, Z_NO_FLUSH);
    if (z_res != Z_OK)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Can't compress update packet (zlib: deflate) Error code: %i (%s)", z_res, zError(z_res));
        *dst_size = 0;
        return false;
    }

    if (c_stream.avail_in != 0)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Can't compress update packet (zlib: deflate not greedy)");
        *dst_size = 0;
        return false;
    }

    z_res = deflate(&c_stream, Z_FINISH);
    if (z_res != Z_STREAM_END)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Can't compress update packet (zlib: deflate should report Z_STREAM_END instead %i (%s)", z_res, zError(z_res));
        *dst_size = 0;
        return false;
    }

    *dst_size = c_stream.total_out;

    z_res = deflateEnd(&c_stream);
    if (z_res != Z_OK)
    {
        sLog->outError(LOG_FILTER_GENERAL, "Can't compress update packet (zlib: deflateEnd) Error code: %i (%s)", z_res, zError(z_res));
        *dst_size = 0;
        return false;
    }

    return true;
}