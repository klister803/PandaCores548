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

#ifndef _WARDEN_BASE_H
#define _WARDEN_BASE_H

#include <map>
#include "Cryptography/ARC4.h"
#include "Cryptography/BigNumber.h"
#include "ByteBuffer.h"
#include "WardenMgr.h"

#define BASE_CHECKS_HEADER "686561646572"
#define EXTENDED_CHECKS_HEADER "53595354454D"

class WorldSession;

enum WardenOpcodes
{
    // Client->Server
    WARDEN_CMSG_MODULE_MISSING                  = 0,
    WARDEN_CMSG_MODULE_OK                       = 1,
    WARDEN_CMSG_CHEAT_CHECKS_RESULT             = 2,
    WARDEN_CMSG_MEM_CHECKS_RESULT               = 3,        // only sent if MEM_CHECK bytes doesn't match
    WARDEN_CMSG_HASH_RESULT                     = 4,
    WARDEN_CMSG_MODULE_FAILED                   = 5,        // this is sent when client failed to load uploaded module due to cache fail

    // Server->Client
    WARDEN_SMSG_MODULE_USE                      = 0,
    WARDEN_SMSG_MODULE_CACHE                    = 1,
    WARDEN_SMSG_CHEAT_CHECKS_REQUEST            = 2,
    WARDEN_SMSG_MODULE_INITIALIZE               = 3,
    WARDEN_SMSG_MEM_CHECKS_REQUEST              = 4,        // byte len; while(!EOF) { byte unk(1); byte index(++); string module(can be 0); int offset; byte len; byte[] bytes_to_compare[len]; }
    WARDEN_SMSG_HASH_REQUEST                    = 5
};

enum WardenState
{
    WARDEN_NOT_INITIALIZED                = 0,
    WARDEN_MODULE_NOT_LOADED              = 1,
    WARDEN_MODULE_LOADED                  = 2,
    WARDEN_MODULE_SPECIAL_DBC_CHECKS      = 3,
    WARDEN_MODULE_WAIT_INITIALIZE         = 4,
    WARDEN_MODULE_READY                   = 5,
    WARDEN_MODULE_WAIT_RESPONSE           = 6,
    WARDEN_MODULE_SET_PLAYER_LOCK         = 7,
    WARDEN_MODULE_SET_PLAYER_PENDING_LOCK = 8  // TODO: now only for checks on login screen, in future rewrited more universal way
};

#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct WardenModuleUse
{
    uint8 Command;
    uint8 ModuleId[32];
    uint8 ModuleKey[16];
    uint32 Size;
};

struct WardenModuleTransfer
{
    uint8 Command;
    uint16 ChunkSize;
    uint8 Data[500];
};

struct WardenInitModuleMPQFunc
{
    uint8 Type;
    uint8 Flag;
    uint8 MpqFuncType;
    uint8 StringBlock;
    uint32 OpenFile;
    uint32 GetFileSize;
    uint32 ReadFile;
    uint32 CloseFile;
};

struct WardenInitModuleLUAFunc
{
    uint8 Type;
    uint8 Flag;
    uint8 StringBlock;
    uint32 GetText;
    uint32 GetLocalizedText;
    uint8 LuaFuncType;
};

struct WardenInitModuleTimeFunc
{
    uint8 Type;
    uint8 Flag;
    uint8 StringBlock;
    uint32 PerfCounter;
    uint8 TimeFuncType;
};

struct WardenHashRequest
{
    uint8 Command;
    uint8 Seed[16];
};

#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

class Warden
{
    friend class WardenWin;
    friend class WardenMac;

    public:
        Warden(WorldSession* session);
        virtual ~Warden();

        bool Init(BigNumber *k);
        void SendModuleToClient();
        void RequestModule();
        void RequestHash();
        void Update();

        virtual void InitializeModule() = 0;
        virtual void HandleHashResult(ByteBuffer &buff) = 0;
        virtual void HandleModuleFailed() = 0;
        virtual void RequestBaseData() = 0;

        virtual void HandleData(ByteBuffer &buff) = 0;

        virtual void CommonChecksHandler(ByteBuffer &buff) = 0;

        virtual void SendDbcChecks() {};
        virtual void InitializeMPQCheckFuncFake() {};

        void DecryptData(uint8* buffer, uint32 length);
        void EncryptData(uint8* buffer, uint32 length);

        //uint8* GetInputKey() { return _clientKeySeed; }
        //uint8* GetOutputKey() { return _serverKeySeed; }

        WardenState GetState() { return _state; }
        void SetState(WardenState state) { _state = state;  }

        static bool IsValidCheckSum(uint32 checksum, const uint8 *data, const uint16 length);
        static uint32 BuildChecksum(const uint8 *data, uint32 length);

        bool IsValidStateInWorld(std::string os);

        // If no check is passed, the default action from config is executed
        std::string Penalty(uint16 checkId);
        void SetPlayerLocked(uint16 checkId, WardenCheck* wd);
        void SetPlayerLocked(std::string message);

        // TODO : rewrite timer system
        void ClientResponseTimerUpdate(uint32 diff);
        void PendingKickTimerUpdate(uint32 diff);
        void ReInitTimerUpdate(uint32 diff);

        void StaticCheatChecksTimerUpdate(uint32 diff);
        void DynamicCheatChecksTimerUpdate(uint32 diff);

    private:
        WorldSession* _session;
        WardenModule* _currentModule;

        RC4_Context _clientRC4State;
        RC4_Context _serverRC4State;
        WardenState _state;

        uint32 _checkTimer;                          // Timer for sending check requests
        uint32 _checkTimer2;
        uint32 _clientResponseTimer;                 // Timer for client response delay
        uint32 _pendingKickTimer;
        uint32 _reInitTimer;
        uint32 _lastUpdateTime;
};

#endif
