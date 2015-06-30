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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "ByteBuffer.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "World.h"
#include "Player.h"
#include "Util.h"
#include "Warden.h"
#include "AccountMgr.h"

Warden::Warden() : _inputCrypto(16), _outputCrypto(16), _checkTimer(10000/*10 sec*/), _clientResponseTimer(0), _dataSent(false), _initialized(false)
{
}

Warden::~Warden()
{
    delete[] _module->CompressedData;
    delete _module;
    _module = NULL;
    _initialized = false;
}

void Warden::SendModuleToClient()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "Send module to client");

    // Create packet structure
    WardenModuleTransfer packet;

    uint32 sizeLeft = _module->CompressedSize;
    uint32 pos = 0;
    uint16 burstSize;
    while (sizeLeft > 0)
    {
        burstSize = sizeLeft < 500 ? sizeLeft : 500;
        packet.Command = WARDEN_SMSG_MODULE_CACHE;
        packet.DataSize = burstSize;
        memcpy(packet.Data, &_module->CompressedData[pos], burstSize);
        sizeLeft -= burstSize;
        pos += burstSize;

        EncryptData((uint8*)&packet, burstSize + 3);
        WorldPacket pkt1(SMSG_WARDEN_DATA, burstSize + 3 + 4);
        pkt1 << uint32(burstSize + 3);
        pkt1.append((uint8*)&packet, burstSize + 3);
        _session->SendPacket(&pkt1);
    }
}

void Warden::RequestModule()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "Request module");

    // Create packet structure
    WardenModuleUse request;
    request.Command = WARDEN_SMSG_MODULE_USE;

    memcpy(request.ModuleId, _module->Id, 32);
    memcpy(request.ModuleKey, _module->Key, 16);
    request.Size = _module->CompressedSize;

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&request, sizeof(WardenModuleUse));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenModuleUse) + 4);
    pkt << uint32(sizeof(WardenModuleUse));
    pkt.append((uint8*)&request, sizeof(WardenModuleUse));
    _session->SendPacket(&pkt);
}

void Warden::Update()
{
    if (_initialized)
    {
        uint32 currentTimestamp = getMSTime();
        uint32 diff = currentTimestamp - _previousTimestamp;
        _previousTimestamp = currentTimestamp;

        if (_dataSent)
        {
            uint32 maxClientResponseDelay = sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_RESPONSE_DELAY);

            /*if (maxClientResponseDelay > 0)
            {
                // Kick player if client response delays more than set in config
                if (_clientResponseTimer > maxClientResponseDelay * IN_MILLISECONDS)
                {
                    sLog->outWarn(LOG_FILTER_WARDEN, "%s (latency: %u, IP: %s) exceeded Warden module response delay for more than %s - disconnecting client",
                                   _session->GetPlayerName(false).c_str(), _session->GetLatency(), _session->GetRemoteAddress().c_str(), secsToTimeString(maxClientResponseDelay, true).c_str());
                    _session->KickPlayer();
                }
                else
                    _clientResponseTimer += diff;
            }*/
        }
        else
        {
            if (diff >= _checkTimer)
                RequestData();
            else
                _checkTimer -= diff;
        }
    }
}

void Warden::DecryptData(uint8* buffer, uint32 length)
{
    _inputCrypto.UpdateData(length, buffer);
}

void Warden::EncryptData(uint8* buffer, uint32 length)
{
    _outputCrypto.UpdateData(length, buffer);
}

bool Warden::IsValidCheckSum(uint32 checksum, const uint8* data, const uint16 length)
{
    uint32 newChecksum = BuildChecksum(data, length);

    if (checksum != newChecksum)
    {
        sLog->outDebug(LOG_FILTER_WARDEN, "CHECKSUM IS NOT VALID");
        return false;
    }
    else
    {
        sLog->outDebug(LOG_FILTER_WARDEN, "CHECKSUM IS VALID");
        return true;
    }
}

uint32 Warden::BuildChecksum(const uint8* data, uint32 length)
{
    uint8 hash[20];
    SHA1(data, length, hash);
    uint32 checkSum = 0;
    for (uint8 i = 0; i < 5; ++i)
        checkSum = checkSum ^ *(uint32*)(&hash[0] + i * 4);

    return checkSum;
}

/*void Warden::TestSendMemCheck()
{
    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);
    //buff << uint8(0x04);
    //buff << uint32(0x65786540);
    buff << uint8(0x00);
    // Test Unk Check
    //buff << uint8(0x69);
    // Test Mem Check
    //buff << uint8(0x82);
    //buff << uint8(0x00);
    //buff << uint32(0x0629933C);
    //buff << uint8(0xA);
    //buff << uint8(0x1D);
    // test WPE check
    WardenCheck* wd = sWardenCheckMgr->GetWardenDataById(1);
    buff << uint8(0x06);
    /*buff << uint32(0xA444519C);
    //uint8 p[20] = { 0xC4, 0x19, 0x52, 0x1B, 0x6D, 0x39, 0x99, 0x0C, 0x1D, 0x95, 0x32, 0x9C, 0x8D, 0x94, 0xB5, 0x92, 0x26, 0xCB, 0xAA, 0x98 };
    uint8 p1[20] = { 0x98, 0xAA, 0xCB, 0x26, 0x92, 0xB5, 0x94, 0x8D, 0x9C, 0x32, 0x95, 0x1D, 0x0C, 0x99, 0x39, 0x6D, 0x1B, 0x52, 0x19, 0xC4 };
    buff.append(p1, 20);
    buff << uint32(16507);
    buff << uint8(32);
    buff.append(wd->Data.AsByteArray(0, false), wd->Data.GetNumBytes());
    buff << uint32(wd->Address);
    buff << uint8(wd->Length);
    buff << uint8(0x1D);

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Uncrypted 0x02 packet - %s", ByteArrayToHexStr(const_cast<uint8*>(buff.contents()), buff.size()).c_str());

    // Encrypt with warden RC4 key
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());

    sLog->outInfo(LOG_FILTER_NETWORKIO, "Crypted 0x02 packet - %s", ByteArrayToHexStr(const_cast<uint8*>(buff.contents()), buff.size()).c_str());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size());
    pkt << uint32(buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);
}*/

std::string Warden::Penalty(WardenCheck* check /*= NULL*/)
{
    WardenActions action;

    if (check)
        action = check->Action;
    else
        action = WardenActions(sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_FAIL_ACTION));

    switch (action)
    {
    case WARDEN_ACTION_LOG:
        return "None";
        break;
    case WARDEN_ACTION_KICK:
        _session->KickPlayer();
        return "Kick";
        break;
    case WARDEN_ACTION_BAN:
        {
            std::stringstream duration;
            duration << sWorld->getIntConfig(CONFIG_WARDEN_CLIENT_BAN_DURATION) << "s";
            std::string accountName;
            AccountMgr::GetName(_session->GetAccountId(), accountName);
            std::stringstream banReason;
            // Check can be NULL, for example if the client sent a wrong signature in the warden packet (CHECKSUM FAIL)
            if (check)
                banReason << check->Comment;

            sWorld->BanAccount(BAN_ACCOUNT, accountName, duration.str(), banReason.str(), "Warden Anticheat");

            return "Ban";
        }
    default:
        break;
    }
    return "Undefined";
}

void WorldSession::HandleWardenDataOpcode(WorldPacket& recvData)
{
    uint32 cryptedSize;
    recvData >> cryptedSize;
    _warden->DecryptData(const_cast<uint8*>(recvData.contents() + sizeof(uint32)), cryptedSize);
    uint8 opcode;
    recvData >> opcode;
    sLog->outDebug(LOG_FILTER_WARDEN, "Got packet, opcode %02X, size %u", opcode, uint32(recvData.size()));
    recvData.hexlike();

    switch (opcode)
    {
        case WARDEN_CMSG_MODULE_MISSING:
            _warden->SendModuleToClient();
            break;
        case WARDEN_CMSG_MODULE_OK:
            _warden->RequestHash();
            break;
        case WARDEN_CMSG_CHEAT_CHECKS_RESULT:
            _warden->HandleData(recvData);
            break;
        case WARDEN_CMSG_MEM_CHECKS_RESULT:
            sLog->outDebug(LOG_FILTER_WARDEN, "NYI WARDEN_CMSG_MEM_CHECKS_RESULT received!");
            break;
        case WARDEN_CMSG_HASH_RESULT:
            _warden->HandleHashResult(recvData);
            //_warden->InitializeModule();
            //_warden->TestSendMemCheck();
            break;
        case WARDEN_CMSG_MODULE_FAILED:
            sLog->outWarn(LOG_FILTER_WARDEN, "WARDEN_CMSG_MODULE_FAILED received, kick player %s!", GetPlayerName(false).c_str());
            KickPlayer();
            break;
        default:
            sLog->outDebug(LOG_FILTER_WARDEN, "Got unknown warden opcode %02X of size %u.", opcode, uint32(recvData.size() - 1));
            break;
    }
}
