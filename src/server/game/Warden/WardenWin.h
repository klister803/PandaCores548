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

#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

#define ZONE_DALARAN 4395

enum MoveType
{
    MOVETYPE_WALK = 0,
    MOVETYPE_RUN = 1,
    MOVETYPE_RUN_BACK = 2,
    MOVETYPE_SWIM = 3,
    MOVETYPE_SWIM_BACK = 4,
    MOVETYPE_TURN_RATE = 5,
    MOVETYPE_FLIGHT = 6,
    MOVETYPE_FLIGHT_BACK = 7,
    MOVETYPE_PITCH_RATE = 8,
    MOVETYPE_NONE = 255
};

enum ResponseMask
{
    RESP_NONE = 0x00,
    RESP_FLYHACK = 0x01,
    RESP_AIRSWIM_HACK = 0x02,
    RESP_FREEZE_Z_HACK = 0x04,
    RESP_WATERWALK_HACK = 0x08,
    RESP_LEVITATE_HACK = 0x10
};

enum WardenTypeCheck
{
    STATIC_CHECK = 0,
    DYNAMIC_CHECK = 1
};

struct WardenInitModuleRequest
{
    uint8 Command1;
    uint16 Size1;
    uint32 CheckSumm1;
    uint8 Unk1;
    uint8 Unk2;
    uint8 Type;
    uint8 String_library1;
    uint32 Function1[4];

    uint8 Command2;
    uint16 Size2;
    uint32 CheckSumm2;
    uint8 Unk3;
    uint8 Unk4;
    uint8 String_library2;
    uint32 Function2;
    uint8 Function2_set;

    uint8 Command3;
    uint16 Size3;
    uint32 CheckSumm3;
    uint8 Unk5;
    uint8 Unk6;
    uint8 String_library3;
    uint32 Function3;
    uint8 Function3_set;
};

#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

class WorldSession;
class Warden;

class WardenWin : public Warden
{
    public:
        WardenWin();
        ~WardenWin();

        void Init(WorldSession* session, BigNumber* K);
        ClientWardenModule* GetModuleForClient();
        void InitializeModule(bool recall);
        void RequestHash();
        void HandleHashResult(ByteBuffer &buff);

        void RequestStaticData();
        void RequestDynamicData();

        void HandleData(ByteBuffer &buff);
        void HandleStaticData(ByteBuffer &buff);
        void HandleDynamicData(ByteBuffer &buff);

        std::string ConvertPacketDataToString(const uint8 * packet_data, uint16 length);
        bool HasAnticheatImmune();
        void DecreaseAlertCount(WorldIntConfigs index, int8 &count, bool activate);

        // for multi-module system
        std::string GetSignature(uint8 * moduleId, WardenTypeCheck wtc);

        // speed checks helper functions
        MoveType SelectSpeedType(uint32 moveFlags);
        float GetServerSpeed(Unit * obj, UnitMoveType mtype);

        // movement flags checks helper functions
        bool CheckMovementFlags(uint32 moveflags, std::string &reason, uint16 &banMask);
        std::string GetMovementFlagInfo(uint32 moveFlags);
        bool IsAllowVehicleFlying(Unit * vb);
        bool IsAllowPlayerFlying(Player * plr);

        // controlling player movement helper functions
        void SendControlMovementPacket(uint32 opcode, bool added, uint32 moveFlags);

        // faction checks helper functions
        bool IsPlayerFaction(uint32 faction);
        bool IsNotAllowedFaction(uint32 faction);
        bool IsNotTeamFaction(uint32 team, uint32 faction);

        // some helper func in memory reading
        template<class T>
        bool ReadMemChunk(ByteBuffer &buff, T &data);
        template<class T>
        bool CheckCorrectBoundValues(T val);
        bool CheckCorrectBoundValues(float val);

    private:
        uint32 _serverTicks;
        std::list<uint16> _otherChecksTodo;
        std::list<uint16> _memChecksTodo;
        std::list<uint16> _impMemChecksTodo;
        std::list<uint16> _impOtherChecksTodo;
        std::list<uint16> _currentChecks;
};

#endif
