/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"

void WorldSession::HandlePurchaseGetProductList(WorldPacket & /*recvData*/)
{
    // zero opcode
    WorldPacket data(SMSG_PURCHASE_STORE_PURCHASE_LIST_UPDATED, 3);
    data.WriteBits(0, 19);      // purchase count
    SendPacket(&data);
}

void WorldSession::HandlePurchaseGetPurchaseList(WorldPacket & /*recvData*/)
{
    // zero opcode
    WorldPacket data(SMSG_PURCHASE_STORE_PRODUCTS_UPDATED, 4 + 4 + 3 + 3);
    data << uint32(5);          // rubles
    data << uint32(3);          // region is locked
    data.WriteBits(0, 19);      // products count
    data.WriteBits(0, 21);      // group count
    SendPacket(&data);
}

