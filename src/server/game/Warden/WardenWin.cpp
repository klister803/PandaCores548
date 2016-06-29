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

#include "AnticheatMgr.h"
#include "Cryptography/HMACSHA1.h"
#include "Cryptography/WardenKeyGeneration.h"
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "ByteBuffer.h"
#include <openssl/md5.h>
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Util.h"
#include "WardenWin.h"
#include "WardenModuleWin.h"
#include "WardenCheckMgr.h"
#include "AccountMgr.h"
#include "SHA2.h"

WardenWin::WardenWin() : Warden()
{
}

WardenWin::~WardenWin()
{
}

void WardenWin::Init(WorldSession* session, BigNumber *k)
{
    _session = session;
    // Generate Warden Key
    // TEST - session key from 5.4.7.18019
    //k->SetHexStr("05EC9A527BD351928B3F3CB8367AA30B9172FB5C74210354ACD8D8E4AA22478D1F0A40E54854AEA0");
    // END TEST
    SHA1Randx WK(k->AsByteArray(), k->GetNumBytes());
    WK.Generate(_inputKey, 16);
    WK.Generate(_outputKey, 16);

    memcpy(_seed, Module.Seed, 16);

    _inputCrypto.Init(_inputKey);
    _outputCrypto.Init(_outputKey);
    //sLog->outDebug(LOG_FILTER_WARDEN, "Server side warden for client %u initializing...", session->GetAccountId());
    //sLog->outDebug(LOG_FILTER_WARDEN, "C->S Key: %s", ByteArrayToHexStr(_inputKey, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "S->C Key: %s", ByteArrayToHexStr(_outputKey, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "  Seed: %s", ByteArrayToHexStr(_seed, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "Loading Module...");

    _module = GetModuleForClient();

    //sLog->outDebug(LOG_FILTER_WARDEN, "Module Key: %s", ByteArrayToHexStr(_module->Key, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "Module ID: %s", ByteArrayToHexStr(_module->Id, 32).c_str());

    RequestModule();
}

ClientWardenModule* WardenWin::GetModuleForClient()
{
    ClientWardenModule *mod = new ClientWardenModule;

    uint32 length = sizeof(Module.Module);

    // data assign
    mod->CompressedSize = length;
    mod->CompressedData = new uint8[length];
    memcpy(mod->CompressedData, Module.Module, length);
    memcpy(mod->Key, Module.ModuleKey, 16);

    // SHA-2 hash
    SHA256Hash sha;
    sha.Initialize();
    sha.UpdateData(mod->CompressedData, length);
    sha.Finalize();

    for (uint8 i = 0; i < 32; i++)
        mod->Id[i] = sha.GetDigest()[i];

    return mod;
}

void WardenWin::InitializeModule(bool recall)
{
    sLog->outDebug(LOG_FILTER_WARDEN, "Initialize module");

    // Create packet structure
    WardenInitModuleRequest Request;
    Request.Command1 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size1 = 20;
    Request.Unk1 = 1;
    Request.Unk2 = 0;
    Request.Type = 1;
    Request.String_library1 = 0;
    Request.Function1[0] = 0x0001466C;                      // SFileOpenFile
    Request.Function1[1] = 0x00012048;                      // SFileGetFileSize
    Request.Function1[2] = 0x00012EF4;                      // SFileReadFile
    Request.Function1[3] = 0x000137A8;                      // SFileCloseFile
    Request.CheckSumm1 = BuildChecksum(&Request.Unk1, 20);

    Request.Command2 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size2 = 8;
    Request.Unk3 = 4;
    Request.Unk4 = 0;
    Request.String_library2 = 0;
    Request.Function2 = 0x00050AC1;                         // FrameScript::GetText
    Request.Function2_set = 1;
    Request.CheckSumm2 = BuildChecksum(&Request.Unk2, 8);

    Request.Command3 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size3 = 8;
    Request.Unk5 = 1;
    Request.Unk6 = 1;
    Request.String_library3 = 0;
    Request.Function3 = 0x0010D627;                         // PerformanceCounter
    Request.Function3_set = 1;
    Request.CheckSumm3 = BuildChecksum(&Request.Unk5, 8);

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenInitModuleRequest));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenInitModuleRequest) + sizeof(uint32));
    pkt << uint32(sizeof(WardenInitModuleRequest));
    pkt.append((uint8*)&Request, sizeof(WardenInitModuleRequest));
    _session->SendPacket(&pkt);
}

void WardenWin::RequestHash()
{
    //sLog->outDebug(LOG_FILTER_WARDEN, "Request hash");

    // Create packet structure
    WardenHashRequest Request;
    Request.Command = WARDEN_SMSG_HASH_REQUEST;
    memcpy(Request.Seed, _seed, 16);

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenHashRequest));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenHashRequest) + sizeof(uint32));
    pkt << uint32(sizeof(WardenHashRequest));
    pkt.append((uint8*)&Request, sizeof(WardenHashRequest));
    _session->SendPacket(&pkt);
}

void WardenWin::HandleHashResult(ByteBuffer &buff)
{
    buff.rpos(buff.wpos());

    // Verify key
    if (memcmp(buff.contents() + 5, Module.ClientKeySeedHash, 20) != 0)
    {
        sLog->outWarn(LOG_FILTER_WARDEN, "%s failed hash reply. Action: %s", _session->GetPlayerName(false).c_str(), Penalty().c_str());
        _session->KickPlayer();
        return;
    }

    //sLog->outDebug(LOG_FILTER_WARDEN, "Request hash reply: succeed");

    // Change keys here
    memcpy(_inputKey, Module.ClientKeySeed, 16);
    memcpy(_outputKey, Module.ServerKeySeed, 16);

    _inputCrypto.Init(_inputKey);
    _outputCrypto.Init(_outputKey);

    _initialized = true;

    _previousTimestamp = getMSTime();
}

void WardenWin::RequestStaticData()
{
    //sLog->outDebug(LOG_FILTER_WARDEN, "Request data");

    // If all checks were done, fill the todo list again
    if (_memChecksTodo.empty())
        _memChecksTodo.assign(sWardenCheckMgr->MemChecksIdPool.begin(), sWardenCheckMgr->MemChecksIdPool.end());

    if (_otherChecksTodo.empty())
        _otherChecksTodo.assign(sWardenCheckMgr->OtherChecksIdPool.begin(), sWardenCheckMgr->OtherChecksIdPool.end());

    //if (_impOtherChecksTodo.empty())
        //_impOtherChecksTodo.assign(sWardenCheckMgr->ImportantOtherChecksIdPool.begin(), sWardenCheckMgr->ImportantOtherChecksIdPool.end());

    //if (_impMemChecksTodo.empty())
        //_impMemChecksTodo.assign(sWardenCheckMgr->ImportantMemChecksIdPool.begin(), sWardenCheckMgr->ImportantMemChecksIdPool.end());

    _serverTicks = getMSTime();

    uint16 id;
    uint8 type;
    WardenCheck* wd;

    _currentChecks.clear();

    // Build check request - general
    for (uint32 i = 0; i < sWorld->getIntConfig(CONFIG_WARDEN_NUM_MEM_CHECKS); ++i)
    {
        // If todo list is done break loop (will be filled on next Update() run)
        if (_memChecksTodo.empty())
            break;

        // Get check id from the end and remove it from todo
        id = _memChecksTodo.back();
        _memChecksTodo.pop_back();

        // Add the id to the list sent in this cycle
        _currentChecks.push_back(id);
    }

    // Build check request - important
    /*for (uint32 i = 0; i < 2; ++i)
    {
        // If todo list is done break loop (will be filled on next Update() run)
        if (_impMemChecksTodo.empty())
            break;

        // Get check id from the end and remove it from todo
        id = _impMemChecksTodo.back();
        _impMemChecksTodo.pop_back();

        // Add the id to the list sent in this cycle
        _currentChecks.push_back(id);
    }*/

    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);

    ACE_READ_GUARD(ACE_RW_Mutex, g, sWardenCheckMgr->_checkStoreLock);

    // other checks - general
    for (uint32 i = 0; i < sWorld->getIntConfig(CONFIG_WARDEN_NUM_OTHER_CHECKS); ++i)
    {
        // If todo list is done break loop (will be filled on next Update() run)
        if (_otherChecksTodo.empty())
            break;

        // Get check id from the end and remove it from todo
        id = _otherChecksTodo.back();
        _otherChecksTodo.pop_back();

        // Add the id to the list sent in this cycle
        _currentChecks.push_back(id);

        wd = sWardenCheckMgr->GetWardenDataById(id);

        switch (wd->Type)
        {
        case MPQ_CHECK:
        case LUA_STR_CHECK:
        case DRIVER_CHECK:
            buff << uint8(wd->Str.size());
            buff.append(wd->Str.c_str(), wd->Str.size());
            break;
        default:
            break;
        }
    }

    // other checks - important
    /*for (uint32 i = 0; i < 2; ++i)
    {
        // If todo list is done break loop (will be filled on next Update() run)
        if (_impOtherChecksTodo.empty())
            break;

        // Get check id from the end and remove it from todo
        id = _impOtherChecksTodo.back();
        _impOtherChecksTodo.pop_back();

        // Add the id to the list sent in this cycle
        _currentChecks.push_back(id);

        wd = sWardenCheckMgr->GetWardenDataById(id);

        switch (wd->Type)
        {
        case MPQ_CHECK:
        case LUA_STR_CHECK:
        case DRIVER_CHECK:
            buff << uint8(wd->Str.size());
            buff.append(wd->Str.c_str(), wd->Str.size());
            break;
        default:
            break;
        }
    }*/

    uint8 xorByte = _inputKey[0];

    // Add separator
    buff << uint8(0x00);
    // set string index
    uint8 index = 1;

    buff << uint8(MEM_CHECK ^ xorByte);
    buff << uint8(0x00);
    buff << uint8(0xF);
    buff << uint32(0xD87AA0);
    buff << uint8(0x6);

    for (std::list<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        wd = sWardenCheckMgr->GetWardenDataById(*itr);

        type = wd->Type;
        buff << uint8(type ^ xorByte);
        switch (type)
        {
        case MEM_CHECK:
        {
            buff << uint8(0x00);
            buff << uint8(0xF);
            buff << uint32(wd->Address);
            buff << uint8(wd->Length);
            break;
        }
        case PAGE_CHECK_A:
        case PAGE_CHECK_B:
        {
            buff.append(wd->Data.AsByteArray(0, false), wd->Data.GetNumBytes());
            buff << uint32(wd->Address);
            buff << uint8(wd->Length);
            break;
        }
        case MPQ_CHECK:
        case LUA_STR_CHECK:
        {
            buff << uint8(index++);
            break;
        }
        case DRIVER_CHECK:
        {
            buff.append(wd->Data.AsByteArray(0, false), wd->Data.GetNumBytes());
            buff << uint8(index++);
            break;
        }
        case MODULE_CHECK:
        {
            uint32 seed = static_cast<uint32>(rand32());
            buff << uint32(seed);
            HmacHash hmac(4, (uint8*)&seed);
            hmac.UpdateData(wd->Str);
            hmac.Finalize();
            buff.append(hmac.GetDigest(), hmac.GetLength());
            break;
        }
        case PROC_CHECK:
        {
            buff.append(wd->Data.AsByteArray(0, false), wd->Data.GetNumBytes());
            buff << uint8(index++);
            buff << uint8(index++);
            buff << uint32(wd->Address);
            buff << uint8(wd->Length);
            break;
        }
        default:
            break;                                      // Should never happen
        }
    }

    buff << uint8(xorByte);

    // Encrypt with warden RC4 key
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + 4);
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);

    _dataSent = true;

    std::stringstream stream;
    stream << "Sent check id's: ";
    for (std::list<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
        stream << *itr << " ";

    //sLog->outDebug(LOG_FILTER_WARDEN, "%s", stream.str().c_str());

    WorldPacket data1(SMSG_SERVERTIME, 8);
    data1 << uint32(12755321);
    data1 << uint32(13904220);
    _session->SendPacket(&data1);
}


void WardenWin::HandleData(ByteBuffer &buff)
{
    //sLog->outDebug(LOG_FILTER_WARDEN, "Handle data");

    uint16 length;
    buff >> length;

    if (!length)
    {
        buff.rfinish();
        sLog->outWarn(LOG_FILTER_WARDEN, "%s invalid Warden packet. Action: %s", _session->GetPlayerName(false).c_str(), Penalty().c_str());
        _session->KickPlayer();
        return;
    }

    uint32 checksum;
    buff >> checksum;

    if (!IsValidCheckSum(checksum, buff.contents() + buff.rpos(), length))
    {
        buff.rfinish();
        sLog->outWarn(LOG_FILTER_WARDEN, "%s failed checksum. Action: %s", _session->GetPlayerName(false).c_str(), Penalty().c_str());
        _session->KickPlayer();
        return;
    }

    // read header
    uint8 headerRes;
    buff >> headerRes;

    if (headerRes != 0)
    {
        //sLog->outWarden("Failed read header for account Id %u", _session->GetAccountId());
        buff.rpos(buff.wpos());
        //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed read Warden packet header. Player kicked", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    uint8 sign[6];
    buff.read(sign, 6);
    std::string packetSign = ConvertPacketDataToString(sign, 6);

    std::string staticCheckSign = GetSignature(_module->Id, STATIC_CHECK);

    // static checks: verify header not equal kick player
    if (!strcmp(packetSign.c_str(), staticCheckSign.c_str()))
    {
        HandleStaticData(buff);
        return;
    }

    std::string dynamicCheckSign = GetSignature(_module->Id, DYNAMIC_CHECK);

    // dynamic checks: verify header not equal kick player
    if (!strcmp(packetSign.c_str(), dynamicCheckSign.c_str()))
    {
        // handle system data
        /*if (isDebuggerPresentFunc == 0x00)
        {
            uint8 res;
            buff >> res;
            if (res != 0)
            {
                //sLog->outWarden("Failed read system import data for account Id %u", _session->GetAccountId());
                buff.rpos(buff.wpos());
                //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed read Warden packet data. Player kicked", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
                _session->KickPlayer();
                return;
            }

            buff >> isDebuggerPresentFunc;
        }*/

        HandleDynamicData(buff);
        return;
    }

    //sLog->outWarden("Failed check header for account Id %u", _session->GetAccountId());
    buff.rpos(buff.wpos());
    //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed check Warden packet header. Player kicked",
        //_session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
    _session->KickPlayer();
}

void WardenWin::HandleStaticData(ByteBuffer &buff)
{
    _dataSent = false;
    m_clientResponseAlert = 0;
    _clientResponseTimer = sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_RESPONSE_DELAY);
    _checkTimer = sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF);
    //sLog->outError("Packet of static checks answers has received by server");

    WardenCheckResult *rs;
    WardenCheck *rd;
    uint8 type;

    ACE_READ_GUARD(ACE_RW_Mutex, g, sWardenCheckMgr->_checkStoreLock);

    for (std::list<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        rd = sWardenCheckMgr->GetWardenDataById(*itr);
        rs = sWardenCheckMgr->GetWardenResultById(*itr);

        type = rd->Type;
        switch (type)
        {
        case MEM_CHECK:
        {
            uint8 Mem_Result;
            buff >> Mem_Result;

            if (Mem_Result != 0)
            {
                //sLog->outWarden("RESULT MEM_CHECK not 0x00, CheckId %u account Id %u", *itr, _session->GetAccountId());
                buff.rpos(buff.wpos());
                //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(rd).c_str());
                return;
            }

            std::string packet_data = ConvertPacketDataToString(buff.contents() + buff.rpos(), rd->Length);
            if (strcmp(rs->Result.c_str(), packet_data.c_str()))
            {
                //sLog->outWarden("RESULT MEM_CHECK fail CheckId %u, account Id %u, Failed data %s", *itr, _session->GetAccountId(), packet_data.c_str());
                buff.rpos(buff.wpos());
                //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(rd).c_str());
                return;
            }

            buff.rpos(buff.rpos() + rd->Length);
            break;
        }
        case PAGE_CHECK_A:
        case PAGE_CHECK_B:
        case DRIVER_CHECK:
        case MODULE_CHECK:
        {
            std::string packet_data = ConvertPacketDataToString(buff.contents() + buff.rpos(), 1);
            if (strcmp(rs->Result.c_str(), packet_data.c_str()))
            {
                if (type == PAGE_CHECK_A || type == PAGE_CHECK_B)
                    //sLog->outWarden("RESULT PAGE_CHECK fail, CheckId %u, account Id %u", *itr, _session->GetAccountId());
                if (type == MODULE_CHECK)
                    //sLog->outWarden("RESULT MODULE_CHECK fail, CheckId %u, account Id %u", *itr, _session->GetAccountId());
                if (type == DRIVER_CHECK)
                    //sLog->outWarden("RESULT DRIVER_CHECK fail, CheckId %u, account Id %u", *itr, _session->GetAccountId());

                buff.rpos(buff.wpos());
                sLog->outWarn(LOG_FILTER_WARDEN, "%s failed Warden check %u. Action: %s", _session->GetPlayerName(false).c_str(), *itr, Penalty(rd).c_str());
                return;
            }

            buff.rpos(buff.rpos() + 1);
            break;
        }
        case LUA_STR_CHECK:
        {
            uint8 Lua_Result;
            buff >> Lua_Result;

            //if (Lua_Result != 0)
                //sLog->outWarden("unk byte in LUA_STR_CHECK is equal 1, CheckId %u, account Id %u", *itr, _session->GetAccountId());

            uint8 luaStrLen;
            buff >> luaStrLen;

            if (luaStrLen != 0)
            {
                char *str = new char[luaStrLen + 1];
                memset(str, 0, luaStrLen + 1);
                memcpy(str, buff.contents() + buff.rpos(), luaStrLen);
                //sLog->outWarden("RESULT LUA_STR_CHECK fail, CheckId %u, account Id %u", *itr, _session->GetAccountId());
                //sLog->outWarden("Lua string found: %s", str);
                delete[] str;

                buff.rpos(buff.wpos());
                //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(rd).c_str());
                return;
            }

            break;
        }
        case MPQ_CHECK:
        {
            uint8 Mpq_Result;
            buff >> Mpq_Result;

            if (Mpq_Result != 0)
            {
                //sLog->outWarden("RESULT MPQ_CHECK not 0x00 account id %u", _session->GetAccountId());
                buff.rpos(buff.wpos());
                //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(rd).c_str());
                return;
            }

            std::string packet_data = ConvertPacketDataToString(buff.contents() + buff.rpos(), rd->Length);
            if (strcmp(rs->Result.c_str(), packet_data.c_str()))
            {
                //sLog->outWarden("RESULT MPQ_CHECK fail, CheckId %u, account Id %u", *itr, _session->GetAccountId());
                buff.rpos(buff.wpos());
                //sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(rd).c_str());
                return;
            }

            buff.rpos(buff.rpos() + 20);                // 20 bytes SHA1
            break;
        }
        default:                                        // Should never happen
            break;
        }
    }
}

void WardenWin::RequestDynamicData()
{
    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);

    uint8 xorByte = _inputKey[0];

    buff << uint8(0x00);

    // header
    buff << uint8(MEM_CHECK ^ xorByte);
    buff << uint8(0x00);
    buff << uint8(0xF);
    buff << uint32(0x00E76E50);
    buff << uint8(0x6);

    // construct dynamic part of packet
    buff << uint8(MEM_CHECK ^ xorByte);
    buff << uint8(0x00);
    buff << uint8(0xF);

    bool dataCreate = false;

    // first packet in chain
    if (playerDynamicBase == 0x00)
    {
        buff << uint32(0x011E45DC);
        buff << uint8(0x04);
        dataCreate = true;
    }
    else if (playerDynamicBase != 0x00)
    {
        // sended for correct selection of base packet
        buff << uint32(playerDynamicBase);
        buff << uint8(0x04);

        uint32 run_speed = playerDynamicBase + 0x8A8;//!checked
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(run_speed);
        buff << uint8(0x04);

        uint32 flight_speed = playerDynamicBase + 0x8B8;//!checked
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(flight_speed);
        buff << uint8(0x04);

        uint32 swim_speed = playerDynamicBase + 0x8B0;//!checked
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(swim_speed);
        buff << uint8(0x04);

        uint32 mov_flags = playerDynamicBase + 0x858;//!checked
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(mov_flags);
        buff << uint8(0x04);

        uint32 cur_speed = playerDynamicBase + 0x8A0;//!checked
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(cur_speed);
        buff << uint8(0x04);

        /*uint32 vert_delta = playerDynamicBase + 0x858;//not found
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(vert_delta);
        buff << uint8(0x04);

        uint32 faction = playerDynamicBase + 0x1A34;//not found
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(faction);
        buff << uint8(0x04);

        uint32 map_z = 0x00D3945C;
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(map_z);
        buff << uint8(0x04);

        /*uint32 coord_x = playerDynamicBase + 0x798;
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint32(coord_x);
        buff << uint8(0x04);

        uint32 coord_y = playerDynamicBase + 0x79C;
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint32(coord_y);
        buff << uint8(0x04);

        uint32 coord_z = playerDynamicBase + 0x7A0;//not found
        buff << uint8(MEM_CHECK ^ xorByte);
        buff << uint8(0x00);
        buff << uint8(0xF);
        buff << uint32(coord_z);
        buff << uint8(0x04);*/

        dataCreate = true;
    }

    if (dataCreate)
    {
        buff << uint8(xorByte);

        // Encrypt with warden RC4 key.
        EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

        WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + 4);
        pkt << uint32(buff.size());
        pkt.append(buff);
        _session->SendPacket(&pkt);

        _dynDataSent = true;
    }
}

void WardenWin::HandleDynamicData(ByteBuffer &buff)
{
    Player * plr = _session->GetPlayer();

    if (!plr || !plr->IsInWorld() || plr->IsBeingTeleported())
    {
        buff.rpos(buff.wpos());
        _dynamicCheckTimer = _session->GetLatency() ? _session->GetLatency() : 1;
        _dynDataSent = false;
        return;
    }

    //sLog->outError("Packet of dyn checks anwsers has received by server");
    //_dynDataSent = false;

    uint8 Mem_Result;
    buff >> Mem_Result;

    if (Mem_Result != 0x00)
    {
        buff.rpos(buff.wpos());
        // for debug
        //sLog->outWarden("RESULT MEM_CHECK not 0x00(get base data), Special check failed on account Id %u", _session->GetAccountId());
        // nulled
        ClearAddresses();
        // restart timer
        _dynamicCheckTimer = _session->GetLatency() ? _session->GetLatency() : 1;
        return;
    }

    // first packet in chain
    if (playerDynamicBase == 0x00)
    {
        buff >> playerDynamicBase;
        _dynamicCheckTimer = 2000;
        _dynDataSent = false;
        return;
    }

    // data from client
    if (playerDynamicBase != 0x00)
    {
        uint32 check_data;
        buff >> check_data;

        // check correct data
        // player on vehicle
        if (plr->GetVehicle())
        {
            if (check_data != 0xA34D90)
            {
                buff.rpos(buff.wpos());
                // for debug
                //sLog->outWarden("Check Data - %X on account %u", check_data, _session->GetAccountId());
                ClearAddresses();
                // restart timer
                _dynamicCheckTimer = _session->GetLatency() ? _session->GetLatency() : 1;
                return;
            }
        }
        // player not vehicle
        else
        {
            if (check_data != 0xD58490)
            {
                buff.rpos(buff.wpos());
                // for debug
                //sLog->outWarden("Check Data - %X on account %u", check_data, _session->GetAccountId());
                ClearAddresses();
                // restart timer
                _dynamicCheckTimer = _session->GetLatency() ? _session->GetLatency() : 1;
                return;
            }
        }

        // initialize temp variables
        bool speedAlertsActivate = false;
        bool speedExtAlertsActivate = false;
        bool moveFlagsAlertsActivate = false;
        bool failedCoordsAlertsActivate = false;

        float baseRunClientSpeed = 0.0f;
        if (!ReadMemChunk(buff, baseRunClientSpeed))
            return;

        float baseFlightClientSpeed = 0.0f;
        if (!ReadMemChunk(buff, baseFlightClientSpeed))
            return;

        float baseSwimClientSpeed = 0.0f;
        if (!ReadMemChunk(buff, baseSwimClientSpeed))
            return;

        uint32 m_flags = 0;
        if (!ReadMemChunk(buff, m_flags))
            return;

        float curClientSpeed = 0.0f;
        if (!ReadMemChunk(buff, curClientSpeed))
            return;

        /*float vDeltaConst = 0.0f;
        if (!ReadMemChunk(buff, vDeltaConst))
            return;

        uint32 faction_id = 0;
        if (!ReadMemChunk(buff, faction_id))
            return;

        float map_z = 0.0f;
        if (!ReadMemChunk(buff, map_z))
            return;

        float client_z = 0.0f;
        if (!ReadMemChunk(buff, client_z))
            return;*/

        MoveType mtype = SelectSpeedType(m_flags);
        bool cheat_check = true;

        //if (_session->GetSecurity() >= SEC_GAMEMASTER)
            //cheat_check = false;

        if (cheat_check)
        {
            // get move type of speed
            UnitMoveType move_type = UnitMoveType(mtype);

            // calculate server speed for player/vehicles
            float serverSpeed = 0.0f;
            if (plr->GetVehicle())
                serverSpeed = GetServerSpeed(plr->GetVehicleBase(), move_type);
            else
                serverSpeed = GetServerSpeed(plr, move_type);

            float baseClientSpeed = 0.0f;
            if (move_type == MOVE_FLIGHT)
                baseClientSpeed = baseFlightClientSpeed;
            else if (move_type == MOVE_RUN)
                baseClientSpeed = baseRunClientSpeed;
            else if (move_type == MOVE_SWIM)
                baseClientSpeed = baseSwimClientSpeed;

            //sLog->outError("Current speed - %f, base speed - %f, server speed (%u) - %f, moving - %s, falling - %s, launched - %s,  launched speed - %f, on vehicle - %s, speed vehicle - %f, speed alert - %u, speed ext alert - %u, moveflag alerts - %u", curClientSpeed, baseClientSpeed, int8(move_type), serverSpeed, plr->isMoving() ? "true" : "false", plr->IsFalling() ? "true" : "false", plr->isLaunched() ? "true" : "false", plr->GetSpeedXY(), plr->GetVehicle() ? "true" : "false", plr->GetVehicleBase() ? GetServerSpeed(plr->GetVehicleBase(), move_type) : 0.0f,
            //m_speedAlert, m_speedExtAlert, m_moveFlagsAlert);
            AreaTableEntry const* srcZoneEntry = GetAreaEntryByAreaID(plr->GetZoneId());
            AreaTableEntry const* srcAreaEntry = GetAreaEntryByAreaID(plr->GetAreaId());

            if (!HasAnticheatImmune())
            {
                // SPEED - CHECK BLOCK
                // 1000% speed - this value impossible get legal methods, only controlled by server (spline movement)
                if (plr && !plr->GetAnticheatMgr()->HasState(PLAYER_STATE_LAUNCHED) && curClientSpeed >= 77.0f && curClientSpeed > plr->GetAnticheatMgr()->GetSpeedXY())
                {
                    if (m_speedExtAlert == int8(sWorld->getIntConfig(CONFIG_WARDEN_NUM_SPEED_EXT_ALERTS)))
                    {
                        // logs
                        sLog->outWarden("CLIENT WARDEN: Player - %s banned - over-speed speedhack, data : base_speed - %f, server_speed - %f, cur_speed - %f, on_vehicle - %s", _session->GetPlayerName(), baseClientSpeed, serverSpeed, curClientSpeed, plr->GetVehicle() ? "true" : "false");
                        ClearAlerts();

                        // test system - enable ban after
                        //_session->KickPlayer();
                        return;

                        // ban
                        /*std::stringstream duration;
                        duration << sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_BAN_DURATION) << "s";
                        std::string accountName;
                        AccountMgr::GetName(_session->GetAccountId(), accountName);
                        std::stringstream banReason;
                        banReason << "Over-speed speedhack";
                        sWorld->BanAccount(BAN_ACCOUNT, accountName, duration.str(), banReason.str(), "Warden Anticheat");
                        return;*/
                    }

                    m_speedExtAlert++;
                    speedExtAlertsActivate = true;
                }
                // heuristic Hitchhiker detect - banned
                else if (plr && !plr->GetVehicle() && baseSwimClientSpeed > GetServerSpeed(plr, MOVE_SWIM) && baseSwimClientSpeed == baseRunClientSpeed && baseSwimClientSpeed > playerBaseMoveSpeed[MOVE_RUN])
                {
                    if (m_speedExtAlert == int8(sWorld->getIntConfig(CONFIG_WARDEN_NUM_SPEED_EXT_ALERTS)))
                    {
                        bool correct = CheckCorrectBoundValues(baseSwimClientSpeed);
                        sLog->outWarden("CLIENT WARDEN: Player %s %s - detect Hitchhiker's Hack, base swim speed - %f, server swim speed - %f, on_vehicle - %s", _session->GetPlayerName(), correct ? "banned" : "kicked", baseSwimClientSpeed, GetServerSpeed(plr, MOVE_SWIM), plr->GetVehicle() ? "true" : "false");
                        ClearAlerts();

                        if (correct)
                        {
                            // generate cryptostream
                            /*std::stringstream c_data1;
                            int veh = plr->GetVehicle() ? 1 : 0;
                            c_data1 << "HS" << baseSwimClientSpeed << "|" << GetServerSpeed(plr, MOVE_SWIM) << "|" << veh;
                            std::string cipherText1 = EncryptCustomData(c_data1.str());*/

                            // test system - enable ban after
                            //_session->KickPlayer();
                            return;

                            // ban
                            /*std::stringstream duration;
                            duration << sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_BAN_DURATION) << "s";
                            std::string accountName;
                            AccountMgr::GetName(_session->GetAccountId(), accountName);
                            std::stringstream banReason;
                            banReason << "Hitchhiker's Hack detected";
                            sWorld->BanAccount(BAN_ACCOUNT, accountName, duration.str(), banReason.str(), "Warden Anticheat");
                            return;*/
                        }
                        else
                        {
                            //_session->KickPlayer();
                            return;
                        }
                    }

                    m_speedExtAlert++;
                    speedExtAlertsActivate = true;
                }

                if (plr && !plr->GetAnticheatMgr()->HasState(PLAYER_STATE_LAUNCHED) && baseClientSpeed >= 7.0f && serverSpeed >= 7.0f && baseClientSpeed > serverSpeed && baseClientSpeed - serverSpeed > 5.0f)
                {
                    if (m_speedAlert == int8(sWorld->getIntConfig(CONFIG_WARDEN_NUM_SPEED_ALERTS)))
                    {
                        //buff.rpos(buff.wpos());
                        sLog->outWarden("CLIENT WARDEN: Player - %s must be banned - force change base movespeed (Hithchiker's Hack, etc.), data : base_speed - %f, server_speed - %f, cur_speed - %f, on_vehicle - %s, on_transport - %s, on_taxi - %s, falling - %s, map name - %s, zone_name - %s, subzone_name - %s",
                            _session->GetPlayerName(), baseClientSpeed, serverSpeed, curClientSpeed, plr->GetVehicle() ? "true" : "false", plr->GetTransport() ? "true" : "false", plr->m_taxi.GetCurrentTaxiPath() ? "true" : "false", (plr->IsFalling() || plr->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR)) ? "true" : "false",
                            plr->GetMap() ? plr->GetMap()->GetMapName() : "<unknown>", srcZoneEntry ? srcZoneEntry->area_name : "<unknown>", srcAreaEntry ? srcAreaEntry->area_name : "<unknown>");
                        ClearAlerts();
                        //_session->KickPlayer();
                        return;
                    }

                    m_speedAlert++;
                    speedAlertsActivate = true;
                }
                else if (curClientSpeed > 7.0f && curClientSpeed < 77.0f && curClientSpeed > serverSpeed && curClientSpeed > baseClientSpeed &&
                    curClientSpeed - serverSpeed > 5.0f && curClientSpeed - baseClientSpeed > 5.0f)
                {
                    bool correct = true;

                    // check launched state, if speed is bigger launched XY speed - alert
                    if (plr && plr->GetAnticheatMgr()->HasState(PLAYER_STATE_LAUNCHED))
                    {
                        if (plr->GetAnticheatMgr()->GetSpeedXY() > 0.0f && curClientSpeed > plr->GetAnticheatMgr()->GetSpeedXY())
                            correct = false;
                    }
                    else
                    {
                        // check falling, if not falling flag - alert
                        if (plr && !(plr->m_movementInfo.GetMovementFlags() & (MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR)) && !plr->IsFalling())
                            correct = false;
                    }

                    if (!correct)
                    {
                        m_speedAlert++;
                        speedAlertsActivate = true;
                    }

                    if (m_speedAlert == int8(sWorld->getIntConfig(CONFIG_WARDEN_NUM_SPEED_ALERTS)))
                    {
                        sLog->outWarden("CLIENT WARDEN: Player - %s must be banned - force change current speed(WoWEmuHacker, etc.), data : base_speed - %f, server_speed - %f, cur_speed - %f, on_vehicle - %s, on_transport - %s, on_taxi - %s, falling - %s, map name - %s, zone_name - %s, subzone_name - %s",
                            _session->GetPlayerName(), baseClientSpeed, serverSpeed, curClientSpeed, plr->GetVehicle() ? "true" : "false", plr->GetTransport() ? "true" : "false", plr->m_taxi.GetCurrentTaxiPath() ? "true" : "false", (plr->IsFalling() || plr->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR)) ? "true" : "false",
                            plr->GetMap() ? plr->GetMap()->GetMapName() : "<unknown>", srcZoneEntry ? srcZoneEntry->area_name : "<unknown>", srcAreaEntry ? srcAreaEntry->area_name : "<unknown>");
                        ClearAlerts();
                        //_session->KickPlayer();
                        return;
                    }
                }

                // REAL MOVEMENT FLAGS - CHECK BLOCK
                std::string mflags_reason = "";
                uint16 banMask = 0;
                uint32 weirdMoveFlags = 0;

                if (!CheckMovementFlags(m_flags, mflags_reason, banMask))
                {
                    if (m_moveFlagsAlert == int8(sWorld->getIntConfig(CONFIG_WARDEN_NUM_MOVEFLAGS_ALERTS)))
                    {
                        sLog->outWarden("CLIENT WARDEN: Player - %s has incorrect moveflags, reason - %s, moveflags - %X (%s), on_vehicle - %s, on_transport - %s, on_taxi - %s, falling - %s, map name - %s, zone_name - %s, subzone_name - %s", _session->GetPlayerName(), mflags_reason.c_str(), m_flags, GetMovementFlagInfo(m_flags).c_str(),
                            plr->GetVehicle() ? "true" : "false", plr->GetTransport() ? "true" : "false", plr->m_taxi.GetCurrentTaxiPath() ? "true" : "false", (plr->IsFalling() || plr->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR)) ? "true" : "false",
                            plr->GetMap() ? plr->GetMap()->GetMapName() : "<unknown>", srcZoneEntry ? srcZoneEntry->area_name : "<unknown>", srcAreaEntry ? srcAreaEntry->area_name : "<unknown>");
                        ClearAlerts();
                        //_session->KickPlayer();
                        return;
                    }

                    m_moveFlagsAlert++;
                    moveFlagsAlertsActivate = true;
                    //sLog->outError("CLIENT WARDEN: player - %s, incorrect moveflags - %X (%s)", _session->GetPlayerName(), m_flags, GetMovementFlagInfo(m_flags).c_str());
                }

                // SYNCRONIZE SERVER/CLIENT COORDINATES - CHECK BLOCK (DISABLED)
                // CALCULATE MAP Z COORDINAT - CHECK BLOCK (ENABLED)
                // extreme shutdown in config
                /*if (sWorld->getIntConfig(CONFIG_WARDEN_NUM_FALIED_COORDS_ALERTS))
                {
                    // ENABLED - reseacrhing....
                    // exculde transport/launched/taxi/teleported/falling states
                    if (!plr->GetTransport() && !plr->IsFalling() && !plr->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR) &&
                        !plr->isLaunched())
                    {
                        if (!plr->IsFlying() && !plr->IsAllowFlying() && !plr->IsInWater() && !plr->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY) && map_z != 0.0f && client_z > map_z + 2.0f)
                        {
                            if (m_failedCoordsAlert == int8(sWorld->getIntConfig(CONFIG_WARDEN_NUM_FALIED_COORDS_ALERTS)))
                            {
                                buff.rpos(buff.wpos());
                                sLog->outWarden("CLIENT WARDEN: player - %s has invalid Z height, server_x - %f, server_y - %f, server_z - %f, client_z - %f, server speed - %f, client speed - %f, on_vehicle - %s, falling - %s, map name - %s, zone_name - %s, subzone_name - %s, client_map_height - %f", _session->GetPlayerName(),
                                    plr->GetPositionX(), plr->GetPositionY(), plr->GetPositionZ(), client_z, serverSpeed, curClientSpeed, plr->GetVehicle() ? "true" : "false", (plr->IsFalling() || plr->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR)) ? "true" : "false",
                                    plr->GetMap() ? plr->GetMap()->GetMapName() : "<unknown>", srcZoneEntry ? srcZoneEntry->area_name[sWorld->GetDefaultDbcLocale()] : "<unknown>", srcAreaEntry ? srcAreaEntry->area_name[sWorld->GetDefaultDbcLocale()] : "<unknown>", map_z);

                                ClearAlerts();
                                SendControlMovementPacket(SMSG_MOVE_STOP_SWIM, true, MOVEMENTFLAG_FALLING);
                                _dynDataSent = false;
                                return;
                            }

                            m_failedCoordsAlert++;
                            failedCoordsAlertsActivate = true;
                        }
                    }
                }*/
            }

            // DYNAMIC VERTICAL DELTA - CHECK BLOCK
            /*if (vDeltaConst != 1.0f)
            {
                // Wallhack!
                if (vDeltaConst == 255.0f)
                {
                    sLog->outWarden("CLIENT WARDEN: WallClimb on account %u, player %s", _session->GetAccountId(), _session->GetPlayerName());
                    _session->KickPlayer();
                    _dynDataSent = false;
                    return;
                }
            }

            // FACTION ID - CHECK BLOCK
            if (plr->isSpectator() && !IsNotTeamFaction(plr->GetTeam(), faction_id) && faction_id != 35 && faction_id < 1629)
            {
                sLog->outWarden("CLIENT WARDEN: Faction ID is not allowed for spectator (client - %u, server - %u) on account %u, player %s", faction_id, plr->getFaction(), _session->GetAccountId(), _session->GetPlayerName());
                _session->KickPlayer();
                _dynDataSent = false;
                return;
            }*/

            DecreaseAlertCount(CONFIG_WARDEN_NUM_SPEED_ALERTS, m_speedAlert, speedAlertsActivate);
            DecreaseAlertCount(CONFIG_WARDEN_NUM_SPEED_EXT_ALERTS, m_speedExtAlert, speedExtAlertsActivate);
            DecreaseAlertCount(CONFIG_WARDEN_NUM_MOVEFLAGS_ALERTS, m_moveFlagsAlert, moveFlagsAlertsActivate);
            DecreaseAlertCount(CONFIG_WARDEN_NUM_FALIED_COORDS_ALERTS, m_failedCoordsAlert, failedCoordsAlertsActivate);
        }

        _dynDataSent = false;
        _dynamicCheckTimer = 2000;
    }
}

bool WardenWin::CheckMovementFlags(uint32 moveflags, std::string &reason, uint16 &banMask)
{
    // get player
    Player * plr = _session->GetPlayer();

    if (!plr)
        return false;

    bool correct = true;

    if (moveflags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING))
    {
        if (plr->GetVehicle() && plr->GetVehicleBase())
        {
            if (Unit * vb = plr->GetVehicleBase())
            {
                if (!IsAllowVehicleFlying(vb))
                {
                    if (moveflags & 0x80000000)
                        reason += "Vehicle flyhack detected (Hitchhiker's Hack type)";
                    else
                        reason += "Vehicle flyhack detected (WoWEmuHacker type)";

                    banMask |= RESP_FLYHACK;
                    correct = false;
                }
            }
        }
        else
        {
            if (!IsAllowPlayerFlying(plr))
            {
                if (moveflags & 0x80000000)
                    reason += "Flyhack detected (Hitchhiker's Hack type)";
                else
                    reason += "Flyhack detected (WoWEmuHacker type)";

                banMask |= RESP_FLYHACK;
                correct = false;
            }
        }
    }

    if (moveflags & MOVEMENTFLAG_SWIMMING)
    {
        if (!plr->IsInWater() && !plr->GetAnticheatMgr()->HasState(PLAYER_STATE_SWIMMING))
        {
            reason += " + AirSwimHack detected";
            banMask |= RESP_AIRSWIM_HACK;
            correct = false;
        }
    }

    if (moveflags & MOVEMENTFLAG_DISABLE_GRAVITY && !plr->GetVehicle())
    {
        reason += " + Freeze Z detected";
        banMask |= RESP_FREEZE_Z_HACK;
        correct = false;
    }

    if (moveflags & MOVEMENTFLAG_WATERWALKING)
    {
        bool auraWaterwalk = plr->HasAuraType(SPELL_AURA_WATER_WALK) || plr->HasAura(60068) || plr->HasAura(61081);
        if (plr->GetVehicle() && plr->GetVehicleBase())
        {
            if (plr->GetVehicleBase()->HasAuraType(SPELL_AURA_WATER_WALK))
            {
                reason += " + WaterwalkHack detected (vehicle)";
                banMask |= RESP_WATERWALK_HACK;
                correct = false;
            }
        }
        else
        {
            if (!plr->GetAnticheatMgr()->HasState(PLAYER_STATE_WATER_WALK) && !auraWaterwalk)
            {
                reason += " + WaterwalkHack detected";
                banMask |= RESP_WATERWALK_HACK;
                correct = false;
            }
        }
    }

    if (moveflags & MOVEMENTFLAG_HOVER)
    {
        if (plr->GetVehicle() && plr->GetVehicleBase())
        {
            if (plr->GetVehicleBase()->HasAuraType(SPELL_AURA_HOVER))
            {
                reason += " + LevitateHack detected (vehicle)";
                banMask |= RESP_LEVITATE_HACK;
                correct = false;
            }
        }
        else
        {
            bool auraHover = plr->HasAuraType(SPELL_AURA_HOVER);
            if (!plr->GetAnticheatMgr()->HasState(PLAYER_STATE_HOVER) && !auraHover)
            {
                reason += " + LevitateHack detected";
                banMask |= RESP_LEVITATE_HACK;
                correct = false;
            }
        }
    }

    // cut mistake begin
    std::string new_info = "";
    int i = -1;
    for (std::string::iterator itr = reason.begin(); itr != reason.end(); ++itr)
    {
        i++;

        if (i < 3 && (*itr == ' ' || *itr == '+'))
            continue;

        new_info += *itr;
    }

    if (new_info == "")
        new_info = "none";

    reason = new_info;
    return correct;
}

std::string WardenWin::GetMovementFlagInfo(uint32 moveFlags)
{
    std::string info = "";

    if (moveFlags & MOVEMENTFLAG_FORWARD)
        info += "moveflag_forward";

    if (moveFlags & MOVEMENTFLAG_BACKWARD)
        info += " | moveflag_backward";

    if (moveFlags & MOVEMENTFLAG_STRAFE_LEFT)
        info += " | moveflag_strafe_left";

    if (moveFlags & MOVEMENTFLAG_STRAFE_RIGHT)
        info += " | moveflag_strafe_right";

    if (moveFlags & MOVEMENTFLAG_LEFT)
        info += " | moveflag_turn_left";

    if (moveFlags & MOVEMENTFLAG_RIGHT)
        info += " | moveflag_turn_right";

    if (moveFlags & MOVEMENTFLAG_PITCH_UP)
        info += " | moveflag_pitch_up";

    if (moveFlags & MOVEMENTFLAG_PITCH_DOWN)
        info += " | moveflag_pitch_down";

    if (moveFlags & MOVEMENTFLAG_WALKING)
        info += " | moveflag_walking";

    if (moveFlags & MOVEMENTFLAG_DISABLE_GRAVITY)
        info += " | moveflag_levitating";

    if (moveFlags & MOVEMENTFLAG_ROOT)
        info += " | moveflag_root";

    if (moveFlags & MOVEMENTFLAG_FALLING)
        info += " | moveflag_falling";

    if (moveFlags & MOVEMENTFLAG_SWIMMING)
        info += " | moveflag_swim";

    if (moveFlags & MOVEMENTFLAG_ASCENDING)
        info += " | moveflag_ascending";

    if (moveFlags & MOVEMENTFLAG_DESCENDING)
        info += " | moveflag_descending";

    if (moveFlags & MOVEMENTFLAG_CAN_FLY)
        info += " | moveflag_can_fly";

    if (moveFlags & MOVEMENTFLAG_FLYING)
        info += " | moveflag_flying";

    //if (moveFlags & MOVEMENTFLAG_ONTRANSPORT)
        //info += " | moveflag_on_transport";

    if (moveFlags & MOVEMENTFLAG_SPLINE_ELEVATION)
        info += " | moveflag_spilne_elevation";

    //if (moveFlags & MOVEMENTFLAG_SPLINE_ENABLED)
        //info += " | moveflag_spilne_enabled";

    if (moveFlags & MOVEMENTFLAG_WATERWALKING)
        info += " | moveflag_waterwalking";

    if (moveFlags & MOVEMENTFLAG_FALLING_SLOW)
        info += " | moveflag_falling_slow";

    if (moveFlags & MOVEMENTFLAG_HOVER)
        info += " | moveflag_hover";

    //if (moveFlags & MOVEMENTFLAG_INTERNAL)
        //info += " | moveflag_internal";

    // cut mistake begin
    std::string new_info = "";
    int i = -1;
    for (std::string::iterator itr = info.begin(); itr != info.end(); ++itr)
    {
        i++;

        if (i < 3 && (*itr == ' ' || *itr == '|'))
            continue;

        new_info += *itr;
    }

    if (new_info == "")
        new_info = "none";

    return new_info;
}

MoveType WardenWin::SelectSpeedType(uint32 moveFlags)
{
    // get player
    Player * plr = _session->GetPlayer();

    if (!plr)
        return MOVETYPE_NONE;

    if (moveFlags & MOVEMENTFLAG_SWIMMING)
    {
        if (moveFlags & MOVEMENTFLAG_BACKWARD)
            return MOVETYPE_SWIM_BACK;
        else
            return MOVETYPE_SWIM;
    }
    else if (moveFlags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING))
    {
        if (moveFlags & MOVEMENTFLAG_BACKWARD)
            return MOVETYPE_FLIGHT_BACK;
        else
            return MOVETYPE_FLIGHT;
    }
    else if (moveFlags & MOVEMENTFLAG_WALKING)
        return MOVETYPE_WALK;
    else if (moveFlags & MOVEMENTFLAG_BACKWARD)
        return MOVETYPE_RUN_BACK;

    return MOVETYPE_RUN;
}

void WardenWin::SendControlMovementPacket(uint32 opcode, bool added, uint32 moveFlags)
{
    Player * plr = _session->GetPlayer();

    if (!plr)
        return;

    if (moveFlags)
    {
        MovementInfo mInfo = plr->m_movementInfo;
        if (added)
            mInfo.AddMovementFlag(MovementFlags(moveFlags));
        else
            mInfo.RemoveMovementFlag(MovementFlags(moveFlags));

        WorldPacket data((Opcodes)opcode, 200);
        mInfo.moverGUID = plr->GetGUID();
        _session->WriteMovementInfo(data, &mInfo);
        plr->SendMessageToSet(&data, true);
    }
}

bool WardenWin::IsPlayerFaction(uint32 faction)
{
    static uint32 factions[] = { 1, 2, 3, 4, 5, 6, 1610, 115, 116, 1629 };

    for (int i = 0; i < sizeof(factions); i++)
        if (faction == factions[i])
            return true;

    return false;
}

bool WardenWin::IsNotAllowedFaction(uint32 faction)
{
    static uint32 factions_n[] = { 0, 14 };

    for (int i = 0; i < sizeof(factions_n); i++)
        if (faction == factions_n[i])
            return true;

    return false;
}

bool WardenWin::IsNotTeamFaction(uint32 team, uint32 faction)
{
    static uint32 factions_h[] = { 2, 5, 6, 116, 1610 };
    static uint32 factions_a[] = { 1, 3, 4, 117, 1629 };

    if (team == ALLIANCE)
    {
        for (int i = 0; i < sizeof(factions_h); i++)
            if (faction == factions_h[i])
                return true;
    }

    if (team == HORDE)
    {
        for (int i = 0; i < sizeof(factions_a); i++)
            if (faction == factions_a[i])
                return true;
    }

    return false;
}

std::string WardenWin::ConvertPacketDataToString(const uint8 * packet_data, uint16 length)
{
    std::ostringstream ss;

    // convert packet data to string
    for (uint32 i = 0; i < length; i++)
    {
        if (int(packet_data[i]) < 16)
            ss << std::uppercase << std::hex << "0" << int(packet_data[i]) << "";
        else
            ss << std::uppercase << std::hex << int(packet_data[i]) << "";
    }

    std::string data_str = ss.str();
    return data_str;
}

bool WardenWin::HasAnticheatImmune()
{
    // get player
    Player * plr = _session->GetPlayer();

    if (!plr)
        return true;

    if (!plr->IsInWorld())
        return true;

    if (plr->IsBeingTeleported())
        return true;

    if (plr->m_taxi.GetCurrentTaxiPath())
        return true;

    // temp immune anticheat checks for gunship battle
    if (plr->GetMapId() == 631 && plr->GetTransport())
        return true;

    if (plr->GetAnticheatMgr()->HasSpline())
        return true;

    return false;
}

float WardenWin::GetServerSpeed(Unit * obj, UnitMoveType mtype)
{
    int32 main_speed_mod = 0;
    float stack_bonus = 1.0f;
    float non_stack_bonus = 1.0f;

    switch (mtype)
    {
        // Only apply debuffs
    case MOVE_FLIGHT_BACK:
    case MOVE_RUN_BACK:
    case MOVE_SWIM_BACK:
        break;
    case MOVE_WALK:
        return 0.0f;
    case MOVE_RUN:
    {
        if (obj->IsMounted()) // Use on mount auras
        {
            main_speed_mod = obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
            stack_bonus = obj->GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS);
            non_stack_bonus += obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK) / 100.0f;
        }
        else
        {
            main_speed_mod = obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED);
            stack_bonus = obj->GetTotalAuraMultiplier(SPELL_AURA_MOD_SPEED_ALWAYS);
            non_stack_bonus += obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPEED_NOT_STACK) / 100.0f;
        }
        break;
    }
    case MOVE_SWIM:
    {
        main_speed_mod = obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SWIM_SPEED);
        break;
    }
    case MOVE_FLIGHT:
    {
        if (obj->GetTypeId() == TYPEID_UNIT && obj->IsControlledByPlayer()) // not sure if good for pet
        {
            main_speed_mod = obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);
            stack_bonus = obj->GetTotalAuraMultiplier(SPELL_AURA_MOD_VEHICLE_SPEED_ALWAYS);

            // for some spells this mod is applied on vehicle owner
            int32 owner_speed_mod = 0;

            if (Unit* owner = obj->GetCharmer())
                owner_speed_mod = owner->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

            main_speed_mod = std::max(main_speed_mod, owner_speed_mod);
        }
        else if (obj->IsMounted())
        {
            main_speed_mod = obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
            stack_bonus = obj->GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS);
        }
        else             // Use not mount (shapeshift for example) auras (should stack)
            main_speed_mod = obj->GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED) + obj->GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

        non_stack_bonus += obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK) / 100.0f;

        // Pursuit of Justice (increased flight speed on mounts)
        if (AuraEffect * pj = obj->GetAuraEffect(SPELL_AURA_MOD_INCREASE_SPEED, SPELLFAMILY_GENERIC, 1797, 0))
        {
            float non_stack_bonus2 = 1.0f;
            non_stack_bonus2 += pj->GetAmount() / 100.0f;

            if (non_stack_bonus2 > non_stack_bonus)
                non_stack_bonus = non_stack_bonus2;
        }

        break;
    }
    default:
        //sLog->outError("Unit::UpdateSpeed: Unsupported move type (%d)", mtype);
        return 0.0f;
    }

    // now we ready for speed calculation
    float speed = std::max(non_stack_bonus, stack_bonus);
    if (main_speed_mod)
        AddPct(speed, main_speed_mod);

    switch (mtype)
    {
    case MOVE_RUN:
    case MOVE_SWIM:
    case MOVE_FLIGHT:
    {
        // Set creature speed rate
        if (obj->GetTypeId() == TYPEID_UNIT)
        {
            if (obj->isPet())
            {
                speed *= obj->ToCreature()->GetCreatureTemplate()->speed_run;
            }
            else if (Unit* owner = obj->GetAnyOwner()) // Must check for owner
            {
                if (owner->ToPlayer())
                {
                    if (obj->HasUnitState(UNIT_STATE_FOLLOW) && !obj->isInCombat())
                    {
                        // Sync speed with owner when near or slower
                        float owner_speed = owner->GetSpeedRate(mtype);
                        if (speed < owner_speed)
                            speed = owner_speed;

                        // Decrease speed when near to help prevent stop-and-go movement
                        // and increase speed when away to help prevent falling behind
                        speed *= std::max(0.6f + (obj->GetDistance(owner) / 10.0f), 1.1f);
                    }
                }
                else
                    speed *= obj->ToCreature()->GetCreatureTemplate()->speed_run;
            }
            else
                speed *= obj->ToCreature()->GetCreatureTemplate()->speed_run;    // at this point, MOVE_WALK is never reached
        }
        // Normalize speed by 191 aura SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED if need
        // TODO: possible affect only on MOVE_RUN
        if (int32 normalization = obj->GetMaxPositiveAuraModifier(SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED))
        {
            // Use speed from aura
            float max_speed = normalization / (obj->IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
            if (speed > max_speed)
                speed = max_speed;
        }
        break;
    }
    default:
        break;
    }

    // for creature case, we check explicit if mob searched for assistance
    if (obj->GetTypeId() == TYPEID_UNIT)
    {
        if (obj->ToCreature()->HasSearchedAssistance())
            speed *= 0.66f;                                 // best guessed value, so this will be 33% reduction. Based off initial speed, mob can then "run", "walk fast" or "walk".
    }

    // Apply strongest slow aura mod to speed
    if (float minSpeedMod = (float)obj->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED))
    {
        float min_speed = minSpeedMod / 100.0f;
        if (speed < min_speed && mtype != MOVE_SWIM)
            speed = min_speed;
    }

    if (mtype == MOVE_SWIM)
    {
        if (float minSwimSpeedMod = (float)obj->GetMaxPositiveAuraModifier(SPELL_AURA_INCREASE_MIN_SWIM_SPEED))
        {
            float min_speed = minSwimSpeedMod / 100.0f;
            if (speed < min_speed)
                speed = min_speed;
        }
    }

    if (speed <= 0) //crash client
        speed = 0.01f;

    return obj->GetSpeedRate(mtype)*(obj->IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
}

bool WardenWin::IsAllowVehicleFlying(Unit * vb)
{
    if (vb->HasAuraType(SPELL_AURA_FLY) || vb->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED) || vb->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED)
        || vb->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))
        return true;

    if (vb->GetTypeId() == TYPEID_UNIT)
    {
        if (vb->ToCreature() && vb->ToCreature()->CanFly())
            return true;
    }

    return false;
}

bool WardenWin::IsAllowPlayerFlying(Player * plr)
{
    if (plr->HasAuraType(SPELL_AURA_FLY) || plr->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED) || plr->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED)
        || plr->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))
        return true;

    if (plr->GetAnticheatMgr()->HasState(PLAYER_STATE_CAN_FLY))
        return true;

    return false;
}

void WardenWin::DecreaseAlertCount(WorldIntConfigs index, int8 &count, bool activate)
{
    // decrease alert if not activated
    if (!activate)
    {
        count--;

        if (count < 0 || count > int8(sWorld->getIntConfig(index)))
            count = 0;
    }
}

template<class T>
bool WardenWin::ReadMemChunk(ByteBuffer &buff, T &data)
{
    uint8 mem_result;
    buff >> mem_result;

    if (mem_result != 0x00)
    {
        buff.rpos(buff.wpos());
        // nulled
        ClearAddresses();
        // restart timer
        _dynamicCheckTimer = 2000;
        _dynDataSent = false;
        return false;
    }

    buff >> data;

    return true;
}

template<class T>
bool WardenWin::CheckCorrectBoundValues(T val)
{
    // checked strange values of speed, player kicked
    if (val >= 10000)
        return false;
    else
        return true;
}

bool WardenWin::CheckCorrectBoundValues(float val)
{
    // checked strange values of speed, player kicked
    if (val >= 10000.0f)
        return false;
    else
    {
        uint8 count = 0;
        float f = val;
        if (ceil(f) != f) do count++; while (((f *= 10) - (int)f) != 0);

        if (count <= 3)
            return true;
        else
            return false;
    }
}

std::string WardenWin::GetSignature(uint8 * moduleId, WardenTypeCheck wtc)
{
    //std::string Id = ConvertPacketDataToString(moduleId, 16);

    //if (!strcmp("79C0768D657977D697E10BAD956CCED1", Id.c_str()))
    //{
        if (wtc == STATIC_CHECK)
            return "686561646572";
        else
            return "53595354454D";
    //}

    return "";
}
