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
    _currentModule = _wardenMgr->GetModuleByName("083B5E0EA6D1BFE9CF604D5B794FD9594C90AA8ABC0339C1DA2F6E1BC41ED6DE", "Win");
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
    InitializeTimeCheckFunc(buff);

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
    request.OpenFile = 0x0001466C;
    request.GetFileSize = 0x00012048;
    request.ReadFile = 0x00012EF4;
    request.CloseFile = 0x000137A8;

    buff << uint32(BuildChecksum((uint8*)&request, 20));
    buff.append(request);
}

void WardenWin::InitializeLuaCheckFunc(ByteBuffer& buff)
{
    buff << uint8(WARDEN_SMSG_MODULE_INITIALIZE);
    buff << uint16(8);

    WardenInitModuleLUAFunc request;
    request.Type = 4;
    request.Flag = 0;
    request.StringBlock = 0;
    request.GetText = 0x00050AC1;
    request.LuaFuncType = 1;

    buff << uint32(BuildChecksum((uint8*)&request, 8));
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

void WardenWin::HandleHashResult(ByteBuffer &buff, bool newCrypto)
{
    buff.rfinish();

    if (!newCrypto)
    {
        // Verify key
        if (memcmp(buff.contents() + 5, _currentModule->ClientKeySeedHash, 20) != 0)
        {
            sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request hash reply - failed");
            sLog->outWarden("Player %s (guid: %u, account: %u) failed hash reply. Action: Kick",
                _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
            _session->KickPlayer();
            return;
        }

        sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request hash reply - succeed");
    }

    ARC4::rc4_init(&_clientRC4State, _currentModule->ClientKeySeed, 16);
    ARC4::rc4_init(&_serverRC4State, _currentModule->ServerKeySeed, 16);

    _state = WARDEN_MODULE_READY;
    _checkTimer = sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF);
}

bool WardenWin::ValidatePacket(ByteBuffer &buff)
{
    uint16 length;
    buff >> length;
    uint32 checksum;
    buff >> checksum;

    if (!length)
    {
        buff.rfinish();
        sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: packet length failed");
        sLog->outWarden("Player %s (guid: %u, account: %u) failed packet length. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return false;
    }

    if (!IsValidCheckSum(checksum, buff.contents() + buff.rpos(), length))
    {
        buff.rfinish();
        sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: packet checksum failed");
        sLog->outWarden("Player %s (guid: %u, account: %u) failed checksum. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
        return false;
    }

    return true;
}

bool WardenWin::ValidatePacketHeader(ByteBuffer &buff)
{
    uint8 headerRes;
    buff >> headerRes;

    if (headerRes)
        return false;

    uint8 bytes[6];
    buff.read(bytes, 6);
    std::string headerStr = _wardenMgr->ByteArrayToString(bytes, 6);

    if (headerStr == BASE_CHECKS_HEADER)
    {
        HandleBaseData(buff);
        return true;
    }
    else if (headerStr == EXTENDED_CHECKS_HEADER)
    {
        //HandleExtendedData(buff);
        return true;
    }
    else
        return false;
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

        if (!_currentModule->CheckTypes[wd->Type])
            continue;

        currentPacketSize += BuildCheckData(wd, dataBuff, stringBuff, index);
        _currentChecks.push_back(id);
    } 
    while (currentPacketSize < totalPacketSize);

    buff.append(stringBuff);
    buff.append(dataBuff);
    return;
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
            stringBuff << uint8(wd->Str.size());
            stringBuff.append(wd->Str.c_str(), wd->Str.size());
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
        /*case PROC_CHECK:
        {
            buff.append(wd->i.AsByteArray(0, false), wd->i.GetNumBytes());
            buff << uint8(index++);
            buff << uint8(index++);
            buff << uint32(wd->Address);
            buff << uint8(wd->Length);
            break;
        }*/
        default:
            break;                                      // Should never happen
    }

    uint16 currentDataSize = buff.size() + stringBuff.size();
    return (currentDataSize - prevDataSize);
}

void WardenWin::RequestBaseData()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request static data");

    if (GetState() != WARDEN_MODULE_READY)
        return;

    _serverTicks = getMSTime();
    uint8 xorByte = _currentModule->ClientKeySeed[0];

    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);
    BuildBaseChecksList(buff);
    buff << uint8(xorByte);

    //test
    sLog->outWarden("DATA: %s", _wardenMgr->ByteArrayToString(const_cast<uint8*>(buff.contents()), buff.size()));

    // Encrypt with warden RC4 key
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size() + sizeof(uint32));
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);

    _state = WARDEN_MODULE_WAIT_RESPONSE;
    _clientResponseTimer = sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_RESPONSE_DELAY);

    //std::stringstream stream;
    //stream << "WARDEN: Sent check id's: ";
    //for (std::vector<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
        //stream << *itr << " ";

    //sLog->outDebug(LOG_FILTER_WARDEN, "%s", stream.str().c_str());
    //sLog->outWarden("%s", stream.str().c_str());
    //sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str(), _session->GetPlayer());

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

    if (GetState() != WARDEN_MODULE_WAIT_RESPONSE)
    {
        buff.rfinish();
        return;
    }

    if (!ValidatePacket(buff))
        return;

    /*uint8 timeCheckResult;
    buff >> timeCheckResult;

    if (!timeCheckResult)
    {
        buff.rfinish();
        sLog->outWarden("Player %s (guid: %u, account: %u) failed TIME_CHECK. Action: Kick", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
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

    if (!ValidatePacketHeader(buff))
    {
        buff.rfinish();
        sLog->outWarden("WARDEN: Player %s (guid: %u, account: %u) failed validate Warden packet header. Player kicked", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId());
        _session->KickPlayer();
    }
}

void WardenWin::HandleBaseData(ByteBuffer &buff)
{
    WardenCheck *rd = nullptr;
    uint8 type = 0;

    // TODO:
    _state = WARDEN_MODULE_READY;
    _checkTimer = sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF);

    ACE_READ_GUARD(ACE_RW_Mutex, g, _wardenMgr->_checkStoreLock);

    for (std::vector<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        rd = _wardenMgr->GetCheckDataById(*itr);

        if (!rd)
            continue;

        type = rd->Type;

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
                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    SetPlayerLocked(*itr, rd);
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

                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    SetPlayerLocked(*itr, rd);
                    return;
                }

                buff.rpos(buff.rpos() + 1);
                break;
            }
            case MPQ_CHECK:
            {
                uint8 mpqResult;
                buff >> mpqResult;

                if (mpqResult)
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
                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    SetPlayerLocked(*itr, rd);
                    return;
                }

                buff.rpos(buff.rpos() + 20);
                break;
            }
            case LUA_STR_CHECK:
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
                    sLog->outWarden("Player %s (guid: %u, account: %u) failed Warden check %u. Action: %s", _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), *itr, Penalty(*itr).c_str());
                    SetPlayerLocked(*itr, rd);
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

std::string WardenWin::GetMPQHashForLocales(uint16 checkId)
{
    // TODO: rewrite it
    std::string res = "";

    if (checkId != 38)
        return res;

    uint8 locale = uint8(_session->GetSessionDbLocaleIndex());

    if (locale == 8)
        return res;

    switch (locale)
    {
        case 0: res = "C641C7FAE6884C47A658F0917F42775ABC1FA029"; break;
        case 1: res = "33BACB99BE0281A19C3769FAA500EB2D2747E74C"; break;
        case 2: res = "63908B6B7B6B0C92E2B3B619E77B2291E43C50A5"; break;
        case 3: res = "0DA690C5DF94A6518645BE2A90F86CACDD24D2E8"; break;
        case 6: res = "FF6A52D40F4A1234261F10FCF1F00E9489ECE05D"; break;
        case 7: res = "0618F583338A7E8E94580F2E56B65830E6CEBF69"; break;
    }

    return res;
}