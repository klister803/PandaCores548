/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#ifndef _WARDEN_WIN_H
#define _WARDEN_WIN_H

#include <map>
#include "Cryptography/ARC4.h"
#include "Cryptography/BigNumber.h"
#include "ByteBuffer.h"
#include "Warden.h"

class WorldSession;
class Warden;

class WardenWin : public Warden
{
    public:
        WardenWin(WorldSession* session);
        ~WardenWin();

        void InitializeModule();
        void InitializeMPQCheckFunc(ByteBuffer& buff);
        void InitializeLuaCheckFunc(ByteBuffer& buff);
        void InitializeTimeCheckFunc(ByteBuffer& buff);

        void HandleHashResult(ByteBuffer &buff, bool newCrypto = false);
        void RequestBaseData();

        void HandleData(ByteBuffer &buff);
        void HandleBaseData(ByteBuffer &buff);

        bool ValidatePacket(ByteBuffer &buff);
        bool ValidatePacketHeader(ByteBuffer &buff);

        void BuildBaseChecksList(ByteBuffer &buff);
        uint16 BuildCheckData(WardenCheck* wd, ByteBuffer &buff, ByteBuffer &stringBuf, uint8 &index);

        std::string GetMPQHashForLocales(uint16 checkId);

    private:
        uint32 _serverTicks;
        std::vector<uint16> _baseChecksList;
        std::vector<uint16> _currentChecks;
};

#endif
