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

#include "Cryptography/HMACSHA1.h"
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "ByteBuffer.h"
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Util.h"
#include "WardenWin.h"
#include "WardenMgr.h"
#include "AccountMgr.h"
#include "SpellAuraEffects.h"

WardenWin::WardenWin(WorldSession* session) : Warden(session)
{
    //_currentModule = _wardenMgr->GetModuleByName("A4724A9A523363F17BDFCF39D5F9738EE5E3447E0E91D12D29EB9927EE7F4C07", "Win");
    _currentModule = _wardenMgr->GetModuleById("083B5E0EA6D1BFE9CF604D5B794FD9594C90AA8ABC0339C1DA2F6E1BC41ED6DE", "Win");
}

WardenWin::~WardenWin()
{
}

void WardenWin::InitializeModule()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Initialize module(0x03)");

    ByteBuffer buff;
    InitializeMPQCheckFunc(buff);
    InitializeLuaCheckFunc(buff);
    //InitializeTimeCheckFunc(buff);

    // Encrypt with warden RC4 key.
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + sizeof(uint32));
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);
}

void WardenWin::InitializeMPQCheckFunc(ByteBuffer& buff)
{
    buff << uint8(WARDEN_SMSG_MODULE_INITIALIZE);
    buff << uint16(20);

    WardenInitModuleMPQFunc request;
    request.Type = 1;
    request.Flag = 0;
    request.MpqFuncType = 2;
    request.StringBlock = 0;
    request.OpenFile = 0x0004F9EC; //fake
    request.GetFileSize = 0x00012048;
    request.ReadFile = 0x000131D6;
    request.CloseFile = 0x000137A8;

    buff << uint32(BuildChecksum((uint8*)&request, 20));
    buff.append(request);
}

void WardenWin::InitializeMPQCheckFuncFake()
{
    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_MODULE_INITIALIZE);
    buff << uint16(20);

    WardenInitModuleMPQFunc request;
    request.Type = 1;
    request.Flag = 0;
    request.MpqFuncType = 2;
    request.StringBlock = 0;
    request.OpenFile = 0x00419210;   // FrameScript::Execute
    request.GetFileSize = 0x000218C0;
    request.ReadFile = 0x00022530;
    request.CloseFile = 0x00022910;

    buff << uint32(BuildChecksum((uint8*)&request, 20));
    buff.append(request);

    // Encrypt with warden RC4 key.
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + sizeof(uint32));
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);

    _state = WARDEN_MODULE_READY;
}

void WardenWin::InitializeLuaCheckFunc(ByteBuffer& buff)
{
    buff << uint8(WARDEN_SMSG_MODULE_INITIALIZE);
    buff << uint16(12);

    WardenInitModuleLUAFunc request;
    request.Type = 4;
    request.Flag = 0;
    request.StringBlock = 0;
    request.GetText = 0x00050AC1;
    request.UnkFunction = 0x0004F9EC;
    request.LuaFuncType = 1;

    buff << uint32(BuildChecksum((uint8*)&request, 12));
    buff.append(request);
}

void WardenWin::InitializeTimeCheckFunc(ByteBuffer& buff)
{
    buff << uint8(WARDEN_SMSG_MODULE_INITIALIZE);
    buff << uint16(8);

    WardenInitModuleTimeFunc request;
    request.Type = 1;
    request.Flag = 1;
    request.StringBlock = 0;
    request.PerfCounter = 0x0010D627;
    request.TimeFuncType = 1;

    buff << uint32(BuildChecksum((uint8*)&request, 8));
    buff.append(request);
}

void WardenWin::SendDbcChecks()
{
    _state = WARDEN_MODULE_SPECIAL_DBC_CHECKS;

    uint8 xorByte = _currentModule->ClientKeySeed[0];

    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);
    BuildMPQCHecksList(buff);
    buff << uint8(xorByte);
    // Encrypt with warden RC4 key
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + sizeof(uint32));
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);
}

void WardenWin::HandleHashResult(ByteBuffer &buff)
{
    uint8 clientSeedHash[20];
    buff.read(clientSeedHash, 20);

    std::string hashStr = _wardenMgr->ByteArrayToString(clientSeedHash, 20);
    std::string realHashStr = _wardenMgr->ByteArrayToString(_currentModule->ClientKeySeedHash, 20);

    if (strcmp(hashStr.c_str(), realHashStr.c_str()))
    {
        sLog->outWarden("Player %s (guid: %u, account: %u) failed hash reply. Action: Kick",
            _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request hash reply - succeeded");

    ARC4::rc4_init(&_clientRC4State, _currentModule->ClientKeySeed, 16);
    ARC4::rc4_init(&_serverRC4State, _currentModule->ServerKeySeed, 16);

    _checkTimer = 20 * IN_MILLISECONDS;
    _clientResponseTimer = 0;
    _state = WARDEN_MODULE_READY;
}

void WardenWin::HandleModuleFailed()
{
    sLog->outWarden("Player %s (guid: %u, account: %u) has received CMSG_WARDEN_MODULE_FAILED. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
    _session->KickPlayer();
}

void WardenWin::BuildBaseChecksList(ByteBuffer &buff)
{
    _currentChecks.clear();

    if (_baseChecksList.empty())
        _baseChecksList.assign(_wardenMgr->BaseChecksIdPool.begin(), _wardenMgr->BaseChecksIdPool.end());

    uint16 totalPacketSize = urand(100, 320);
    uint16 currentPacketSize = 0;

    ByteBuffer stringBuff;
    ByteBuffer dataBuff;

    uint8 xorByte = _currentModule->ClientKeySeed[0];

    // TIME_CHECK
    dataBuff << uint8(0x00);

    // Header
    dataBuff << uint8(_currentModule->CheckTypes[MEM_CHECK] ^ xorByte);
    dataBuff << uint8(0x00);
    dataBuff << uint8(0xF);
    dataBuff << uint32(0xD87AA0);
    dataBuff << uint8(0x6);

    uint8 index = 1;

    ACE_READ_GUARD(ACE_RW_Mutex, g, _wardenMgr->_checkStoreLock);

    do
    {
        if (_baseChecksList.empty())
            break;

        std::vector<uint16>::iterator itr = _wardenMgr->GetRandomCheckFromList(_baseChecksList.begin(), _baseChecksList.end());
        uint16 id = (*itr);
        _baseChecksList.erase(itr);

        WardenCheck* wd = _wardenMgr->GetCheckDataById(id);

        if (!wd)
            continue;

        if (wd->Type == INTERNAL_CHECK)
            continue;

        if (!wd->Enabled)
            continue;

        if (wd->Type == MPQ_CHECK && !wd->Address)
            continue;

        if (!_currentModule->CheckTypes[wd->Type])
            continue;

        currentPacketSize += BuildCheckData(wd, dataBuff, stringBuff, index);
        _currentChecks.push_back(id);
    } 
    while (currentPacketSize < totalPacketSize);

    if (!stringBuff.empty())
        buff.append(stringBuff);
    buff.append(dataBuff);
}

void WardenWin::BuildMPQCHecksList(ByteBuffer &buff)
{
    _currentChecks.clear();

    if (_baseChecksList.empty())
        _baseChecksList.assign(_wardenMgr->BaseChecksIdPool.begin(), _wardenMgr->BaseChecksIdPool.end());

    ByteBuffer stringBuff;
    ByteBuffer dataBuff;

    uint8 xorByte = _currentModule->ClientKeySeed[0];
    uint8 index = 1;

    // separator
    dataBuff << uint8(0x00);
    // get client locale
    dataBuff << uint8(_currentModule->CheckTypes[MEM_CHECK] ^ xorByte);
    dataBuff << uint8(0x00);
    dataBuff << uint8(0xF);
    dataBuff << uint32(0x0106ED50);
    dataBuff << uint8(0x4);

    ACE_READ_GUARD(ACE_RW_Mutex, g, _wardenMgr->_checkStoreLock);

    for (auto &checkId : _baseChecksList)
    {
        WardenCheck* wd = _wardenMgr->GetCheckDataById(checkId);

        if (!wd)
            continue;

        if (wd->Type != MPQ_CHECK)
            continue;

        // TODO: temp hack for divide MPQ_CHECKS, with 0 - real MPQ checks, 1 - fake MPQ checks
        if (wd->Type == MPQ_CHECK && wd->Address)
            continue;

        BuildCheckData(wd, dataBuff, stringBuff, index);
        _currentChecks.push_back(checkId);
    }

    buff.append(stringBuff);
    buff.append(dataBuff);
}

uint16 WardenWin::BuildCheckData(WardenCheck* wd, ByteBuffer &buff, ByteBuffer &stringBuff, uint8 &index)
{
    uint16 prevDataSize = buff.size() + stringBuff.size();
    uint8 xorByte = _currentModule->ClientKeySeed[0];

    // TODO: support PROC_CHECK double string
    switch (wd->Type)
    {
        case MPQ_CHECK:
        case LUA_STR_CHECK:
        case DRIVER_CHECK:
        case LUA_EXEC_CHECK:
            stringBuff << uint8(wd->Str.size());
            stringBuff.append(wd->Str.c_str(), wd->Str.size());
            // only for default module!
            if (wd->Type == PROC_CHECK)
            {
                std::string procName = "VirtualQuery";
                stringBuff << uint8(procName.size());
                stringBuff.append(procName.c_str(), procName.size());
            }
            if (wd->Type == LUA_EXEC_CHECK)
            {
                std::string luaStr = "LU";
                stringBuff << uint8(luaStr.size());
                stringBuff.append(luaStr.c_str(), luaStr.size());
            }
            break;
        default:
            break;
    }

    buff << uint8(_currentModule->CheckTypes[wd->Type] ^ xorByte);
    switch (wd->Type)
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
            BigNumber d;
            d.SetHexStr(wd->Data.c_str());
            buff.append(d.AsByteArray(24, false), 24);
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
        case LUA_EXEC_CHECK:
        {
            buff << uint8(index++);
            buff << uint8(index++);
            break;
        }
        case DRIVER_CHECK:
        {
            BigNumber d;
            d.SetHexStr(wd->Data.c_str());
            buff.append(d.AsByteArray(24, false), 24);
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
            uint32 seed = static_cast<uint32>(rand32());
            buff << uint32(seed);
            HmacHash hmac(4, (uint8*)&seed);
            hmac.UpdateData(wd->Str);
            hmac.Finalize();
            buff.append(hmac.GetDigest(), hmac.GetLength());
            break;
        }
        default:
            break;                                      // Should never happen
    }

    uint16 currentDataSize = buff.size() + stringBuff.size();
    return (currentDataSize - prevDataSize);
}

void WardenWin::RequestBaseData()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request static data");

    _serverTicks = getMSTime();
    uint8 xorByte = _currentModule->ClientKeySeed[0];

    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);
    BuildBaseChecksList(buff);
    buff << uint8(xorByte);

    // Encrypt with warden RC4 key
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + sizeof(uint32));
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);

    _state = WARDEN_MODULE_WAIT_RESPONSE;
    _clientResponseTimer = 90 * IN_MILLISECONDS;

    std::stringstream stream;
    stream << "WARDEN: Sent check id's: ";
    for (std::vector<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
        stream << *itr << " ";

    sLog->outDebug(LOG_FILTER_WARDEN, "%s", stream.str().c_str());
    sLog->outWarden("%s", stream.str().c_str());
    sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str(), _session->GetPlayer());

    //WorldPacket data1(SMSG_SERVERTIME, 8);
    //data1 << uint32(12755321);
    //data1 << uint32(13904220);
    //_session->SendPacket(&data1);
}

void WardenWin::HandleData(ByteBuffer &buff)
{
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Handle common data");

    // reset response wait timer
    _clientResponseTimer = 0;

    // read and validate length+checksum - for all packets
    uint16 length;
    buff >> length;
    uint32 checksum;
    buff >> checksum;

    if (!length)
    {
        buff.rfinish();
        sLog->outWarden("Player %s (guid: %u, account: %u) failed packet length. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    if (!IsValidCheckSum(checksum, buff.contents() + buff.rpos(), length))
    {
        buff.rfinish();
        sLog->outWarden("Player %s (guid: %u, account: %u) failed checksum. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    // handlers (header + payload) - divide packets
    if (GetState() == WARDEN_MODULE_WAIT_RESPONSE)
        HandleBaseChecksData(buff);
    else if (GetState() == WARDEN_MODULE_SPECIAL_DBC_CHECKS)
        HandleDbcChecksData(buff);
    else
    {
        buff.rfinish();
        sLog->outWarden("Player %s (guid: %u, account: %u) has received wrong Warden packet. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
    }
}

void WardenWin::HandleBaseChecksData(ByteBuffer &buff)
{
    /*uint8 timeCheckResult;
    buff >> timeCheckResult;

    if (!timeCheckResult)
    {
        buff.rfinish();
        sLog->outWarden("Player %s (guid: %u, account: %u) failed TIME_CHECK. Action: Kick", _session->GetPlayerName().c_str(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    uint32 newClientTicks;
    buff >> newClientTicks;

    uint32 ticksNow = getMSTime();
    uint32 ourTicks = newClientTicks + (ticksNow - _serverTicks);

    sLog->outDebug(LOG_FILTER_WARDEN, "ServerTicks %u", ticksNow);         // Now
    sLog->outDebug(LOG_FILTER_WARDEN, "RequestTicks %u", _serverTicks);    // At request
    sLog->outDebug(LOG_FILTER_WARDEN, "Ticks %u", newClientTicks);         // At response
    sLog->outDebug(LOG_FILTER_WARDEN, "Ticks diff %u", ourTicks - newClientTicks);*/

    uint8 headerRes;
    buff >> headerRes;

    if (headerRes)
    {
        buff.rfinish();
        sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed validate Warden packet header. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    uint8 bytes[6];
    buff.read(bytes, 6);
    std::string headerStr = _wardenMgr->ByteArrayToString(bytes, 6);

    if (headerStr == BASE_CHECKS_HEADER)
        CommonChecksHandler(buff);
    //else if (headerStr == EXTENDED_CHECKS_HEADER)
        //ExtendedChecksHandler(buff);
    else
    {
        buff.rfinish();
        sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed validate Warden packet header. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return;
    }
}

void WardenWin::HandleDbcChecksData(ByteBuffer &buff)
{
    ACE_READ_GUARD(ACE_RW_Mutex, g, _wardenMgr->_checkStoreLock);

    uint8 memResult;
    buff >> memResult;

    if (memResult)
    {
        buff.rfinish();
        sLog->outWarden("Function for MEM_CHECK hasn't been called, CheckId Special account Id %u. Action: Kick", _session->GetAccountId());
        _session->KickPlayer();
        return;
    }

    uint32 clientLocale;
    buff >> clientLocale;

    for (std::vector<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        WardenCheck* rd = _wardenMgr->GetCheckDataById(*itr);

        // remove whole packet if data array is corrupted or check in packet was disabled
        if (!rd || !rd->Enabled)
        {
            buff.rfinish();
            sLog->outWarden("Warden check with Id %u has disabled or checkStorage has corrupted data for player %s (guid: %u, account: %u). Action: None", *itr, _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
            return;
        }

        uint8 mpqResult;
        buff >> mpqResult;

        if (mpqResult)
        {
            buff.rfinish();
            sLog->outWarden("Function for MPQ_CHECK hasn't been called, CheckId %u account Id %u. Action: Kick", *itr, _session->GetAccountId());
            _session->KickPlayer();
            return;
        }

        std::string result = rd->Result;
        std::string packet_data = _wardenMgr->ByteArrayToString(buff.contents() + buff.rpos(), 20);

        // handle custom MPQ data - mpq with locale strings and custom allowed patches
        std::string customHash = GetCustomMPQData(rd->Str, packet_data, clientLocale);

        if (customHash != "")
            result = customHash;

        // debug
        //sLog->outWarden("RESULT MPQ_CHECK, CheckId %u account Id %u, realData - %s, packetData - %s, client locale - %u", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str(), clientLocale);

        if (strcmp(result.c_str(), packet_data.c_str()))
        {
            buff.rfinish();
            sLog->outWarden("RESULT MPQ_CHECK fail, CheckId %u account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), result.c_str(), packet_data.c_str());
            sLog->outWarden("Account %s (Id: %u) failed Warden check %u. Action: %s", _session->GetAccountName().c_str(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
            _state = WARDEN_MODULE_SET_PLAYER_PENDING_LOCK;
            return;
        }

        buff.rpos(buff.rpos() + 20);
    }

    _state = WARDEN_MODULE_WAIT_INITIALIZE;
    _reInitTimer = 4 * IN_MILLISECONDS;
}

void WardenWin::CommonChecksHandler(ByteBuffer &buff)
{
    WardenCheck *rd = nullptr;

    // TODO:
    _state = WARDEN_MODULE_READY;
    _checkTimer = 33 * IN_MILLISECONDS;

    ACE_READ_GUARD(ACE_RW_Mutex, g, _wardenMgr->_checkStoreLock);

    for (std::vector<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        rd = _wardenMgr->GetCheckDataById(*itr);

        if (!rd)
            continue;

        // remove whole packet if data array is corrupted or check in packet was disabled
        if (!rd || !rd->Enabled)
        {
            buff.rfinish();
            sLog->outWarden("Warden check with Id %u has disabled or checkStorage has corrupted data for player %s (guid: %u, account: %u). Action: None", *itr, _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
            return;
        }

        uint8 type = rd->Type;

        switch (type)
        {
            case MEM_CHECK:
            {
                uint8 memResult;
                buff >> memResult;

                if (memResult)
                {
                    buff.rfinish();
                    sLog->outWarden("Function for MEM_CHECK hasn't been called, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    _session->KickPlayer();
                    return;
                }

                std::string packet_data = _wardenMgr->ByteArrayToString(buff.contents() + buff.rpos(), rd->Length);
                if (strcmp(rd->Result.c_str(), packet_data.c_str()))
                {
                    buff.rfinish();
                    sLog->outWarden("RESULT MEM_CHECK fail CheckId %u account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str());
                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName().c_str(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    return;
                }

                buff.rpos(buff.rpos() + rd->Length);
                break;
            }
            case PAGE_CHECK_A:
            case PAGE_CHECK_B:
            case DRIVER_CHECK:
            case MODULE_CHECK:
            case PROC_CHECK:
            {
                std::string packet_data = _wardenMgr->ByteArrayToString(buff.contents() + buff.rpos(), 1);
                if (strcmp(rd->Result.c_str(), packet_data.c_str()))
                {
                    buff.rfinish();

                    if (type == PAGE_CHECK_A || type == PAGE_CHECK_B)
                        sLog->outWarden("RESULT PAGE_CHECK fail, CheckId %u, account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str());
                    if (type == MODULE_CHECK)
                        sLog->outWarden("RESULT MODULE_CHECK fail, CheckId %u, account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str());
                    if (type == DRIVER_CHECK)
                        sLog->outWarden("RESULT DRIVER_CHECK fail, CheckId %u, account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str());
                    if (type == PROC_CHECK)
                        sLog->outWarden("RESULT PROC_CHECK fail, CheckId %u, account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str());

                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName().c_str(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    return;
                }

                buff.rpos(buff.rpos() + 1);
                break;
            }
            case MPQ_CHECK:
            {
                uint8 mpqResult;
                buff >> mpqResult;

                /*if (mpqResult)
                {
                    buff.rfinish();
                    sLog->outWarden("Function for MPQ_CHECK hasn't been called, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    _session->KickPlayer();
                    return;
                }

                std::string packet_data = _wardenMgr->ByteArrayToString(buff.contents() + buff.rpos(), 20);

                // handle locales hash
                std::string extResult = GetMPQHashForLocales(*itr);
                if (extResult != "")
                    rd->Result = extResult;

                if (strcmp(rd->Result.c_str(), packet_data.c_str()))
                {
                    buff.rfinish();
                    sLog->outWarden("RESULT MPQ_CHECK fail, CheckId %u account Id %u, realData - %s, failedData - %s", *itr, _session->GetAccountId(), rd->Result.c_str(), packet_data.c_str());
                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName().c_str(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    SetPlayerLocked(*itr, rd);
                    return;
                }

                buff.rpos(buff.rpos() + 20);*/
                break;
            }
            case LUA_STR_CHECK:
            case LUA_EXEC_CHECK:
            {
                uint8 luaResult;
                buff >> luaResult;

                if (luaResult)
                {
                    buff.rfinish();
                    sLog->outWarden("Function for LUA_STR_CHECK hasn't been called, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    _session->KickPlayer();
                    return;
                }

                uint8 luaStrLen;
                buff >> luaStrLen;

                if (luaStrLen)
                {
                    char *str = new char[luaStrLen + 1];
                    memset(str, 0, luaStrLen + 1);
                    memcpy(str, buff.contents() + buff.rpos(), luaStrLen);
                    sLog->outWarden("RESULT LUA_STR_CHECK fail, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    sLog->outWarden("Lua string found: %s", str);
                    delete[] str;

                    buff.rfinish();
                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName().c_str(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    return;
                }
                break;
            }
            default:
                break;
        }
    }

    // right place, but fucking returns in checks
    //_state = WARDEN_MODULE_READY;
    //_checkTimer = sWorld->getIntConfig(CONFIG_WARDEN_STATIC_CHECK_HOLDOFF);
}

std::string WardenWin::GetCustomMPQData(std::string dbcName, std::string packet, uint32 clientLocale)
{
    std::string result = "";

    // get normalize fileName
    dbcName = dbcName.substr(14, dbcName.size());

    // check locales
    if (clientLocale != LOCALE_ruRU)
        result = _wardenMgr->GetLocalesMPQHash(dbcName, clientLocale);

    // logged koKR, zhCN, zhTW and allowed it
    if (clientLocale == LOCALE_koKR || clientLocale == LOCALE_zhCN || clientLocale == LOCALE_zhTW)
    {
        sLog->outWarden("Detect rare locale %u, data from packet %s for future fixes", clientLocale, packet.c_str());
        result = packet;
    }

    // check custom patches
    std::string comment = "";
    if (_wardenMgr->CheckCustomMPQDataHash(dbcName, result, packet, comment))
        sLog->outWarden("Account %s (id: %u) have custom MPQ patch (%s). Action: None", _session->GetAccountName().c_str(), _session->GetAccountId(), comment.c_str());

    return result;
}