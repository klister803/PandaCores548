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

#ifndef _WARDENCHECKMGR_H
#define _WARDENCHECKMGR_H

#include <map>
#include "Cryptography/BigNumber.h"

enum WardenCheckType
{
    INTERNAL_CHECK  = 0,
    TIME_CHECK      = 0,
    MEM_CHECK       = 1,
    PAGE_CHECK_A    = 2,
    PAGE_CHECK_B    = 3,
    MPQ_CHECK       = 4,
    DRIVER_CHECK    = 5,
    MODULE_CHECK    = 6,
    LUA_STR_CHECK   = 7,
    PROC_CHECK      = 8,
    UNK_CHECK       = 9,
    UNK_CHECK_1     = 10,
    MAX_CHECK_TYPE
};

struct WardenModule
{
    WardenModule()
    {
        memset(ID, 0, 32);
        memset(Key, 0, 16);
        memset(Seed, 0, 16);
        memset(ServerKeySeed, 0, 16);
        memset(ClientKeySeed, 0, 16);
        memset(ClientKeySeedHash, 0, 20);
        memset(CheckTypes, 0, MAX_CHECK_TYPE);
        os = "";
    }

    uint8 ID[32];
    uint8 Key[16];
    uint8 Seed[16];
    uint8 ServerKeySeed[16];
    uint8 ClientKeySeed[16];
    uint8 ClientKeySeedHash[20];
    uint8 CheckTypes[MAX_CHECK_TYPE];
    std::string os;
    //RC4_Context ClientRC4State;
    //RC4_Context ServerRC4State;
    uint8* CompressedData;
    uint32 CompressedSize;
};

enum WardenActions
{
    WARDEN_ACTION_LOG,
    WARDEN_ACTION_KICK,
    WARDEN_ACTION_BAN
};

struct WardenCheck
{
    WardenCheck(uint8 checkType, std::string data, std::string str, uint32 address, uint8 length, std::string result, std::string comment, WardenActions action, uint32 bantime) : Type(checkType),
        Data(data), Str(str), Address(address), Length(length), Result(result), Comment(comment), Action(action), BanTime(bantime) {}

    uint8 Type;
    std::string Data;
    std::string Str;
    uint32 Address;
    uint8 Length;
    std::string Result;
    std::string Comment;
    enum WardenActions Action;
    uint32 BanTime;
};

class WardenMgr
{
    friend class ACE_Singleton<WardenMgr, ACE_Null_Mutex>;
    WardenMgr();
    ~WardenMgr();

    public:
        // We have a linear key without any gaps, so we use vector for fast access
        typedef std::vector<WardenCheck*> CheckContainer;
        typedef std::unordered_map<uint32, WardenModule*> ModuleContainer;

        // module helpers
        void LoadWardenModules(std::string os);
        bool LoadModule(const char * FileName, std::string os);
        WardenModule* GetModuleByName(std::string name, std::string os);
        WardenModule* GetModuleById(uint8 Id);

        // check helpers
        WardenCheck* GetCheckDataById(uint16 Id);

        std::vector<uint16> BaseChecksIdPool;

        void LoadWardenChecks();
        void LoadWardenOverrides();

        // utlities
        std::string ByteArrayToString(const uint8 * packet_data, uint16 length);
        std::vector<uint16>::iterator GetRandomCheckFromList(std::vector<uint16>::iterator begin, std::vector<uint16>::iterator end);

        ACE_RW_Mutex _checkStoreLock;
        ACE_RW_Mutex _moduleStoreLock;

    private:
        CheckContainer checkStore;
        ModuleContainer moduleStore;
};

#define _wardenMgr ACE_Singleton<WardenMgr, ACE_Null_Mutex>::instance()

#endif
