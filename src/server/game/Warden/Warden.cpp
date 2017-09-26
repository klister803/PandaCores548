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

#include "Cryptography/WardenKeyGeneration.h"
#include "Common.h"
#include "WorldSession.h"
#include "WorldPacket.h"
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

Warden::Warden(WorldSession* session) : _session(session), _checkTimer(0), _clientResponseTimer(0), _pendingKickTimer(0), _reInitTimer(0), _state(WARDEN_NOT_INITIALIZED)
{
    _lastUpdateTime = getMSTime();
}

Warden::~Warden()
{
}

bool Warden::Init(BigNumber *k)
{
    if (!_currentModule)
    {
        sLog->outWarden("Current module is not found. Abort Warden (%s)! Account %u (%s)", _session->GetOS().c_str(), _session->GetAccountId(), _session->GetAccountName().c_str());
        return false;
    }

    uint8 tempClientKeySeed[16];
    uint8 tempServerKeySeed[16];

    SHA1Randx WK(k->AsByteArray(), k->GetNumBytes());
    WK.Generate(tempClientKeySeed, 16);
    WK.Generate(tempServerKeySeed, 16);

    ARC4::rc4_init(&_clientRC4State, tempClientKeySeed, 16);
    ARC4::rc4_init(&_serverRC4State, tempServerKeySeed, 16);

    //sLog->outWarden("Module Key: %s", ByteArrayToHexStr(_module->Key, 16).c_str());
    //sLog->outWarden("Module ID: %s", ByteArrayToHexStr(_module->Id, 16).c_str());
    RequestModule();

    //sLog->outDebug(LOG_FILTER_WARDEN, "Server side warden for client %u initializing...", session->GetAccountId());
    //sLog->outDebug(LOG_FILTER_WARDEN, "C->S Key: %s", ByteArrayToHexStr(_inputKey, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "S->C Key: %s", ByteArrayToHexStr(_outputKey, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "Seed: %s", ByteArrayToHexStr(_seed, 16).c_str());
    //sLog->outDebug(LOG_FILTER_WARDEN, "Loading Module...");
    _state = WARDEN_MODULE_NOT_LOADED;

    return true;
}

void Warden::RequestModule()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request module(0x00)");

    // Create packet structure
    WardenModuleUse request;
    request.Command = WARDEN_SMSG_MODULE_USE;

    memcpy(request.ModuleId, _currentModule->ID, 32);
    memcpy(request.ModuleKey, _currentModule->Key, 16);
    request.Size = _currentModule->CompressedSize;

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&request, sizeof(WardenModuleUse));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenModuleUse) + sizeof(uint32));
    pkt << uint32(sizeof(WardenModuleUse));
    pkt.append((uint8*)&request, sizeof(WardenModuleUse));
    _session->SendPacket(&pkt);
}

void Warden::SendModuleToClient()
{
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Send module to client(0x01)");

    // Create packet structure
    WardenModuleTransfer packet;

    uint32 sizeLeft = _currentModule->CompressedSize;
    uint32 pos = 0;
    uint16 chunkSize = 0;
    while (sizeLeft > 0)
    {
        chunkSize = sizeLeft < 500 ? sizeLeft : 500;
        packet.Command = WARDEN_SMSG_MODULE_CACHE;
        packet.ChunkSize = chunkSize;
        memcpy(packet.Data, &_currentModule->CompressedData[pos], chunkSize);
        sizeLeft -= chunkSize;
        pos += chunkSize;

        uint16 packetSize = sizeof(uint8) + sizeof(uint16) + chunkSize;
        EncryptData((uint8*)&packet, packetSize);

        WorldPacket pkt(SMSG_WARDEN_DATA, packetSize + sizeof(uint32));
        pkt << uint32(packetSize);
        pkt.append((uint8*)&packet, packetSize);
        _session->SendPacket(&pkt);
    }
}

void Warden::RequestHash()
{
    _state = WARDEN_MODULE_LOADED;

    if (_session->GetOS() == "OSX")
        return;

    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: Request hash(0x05)");

    // Create packet structure
    WardenHashRequest Request;
    Request.Command = WARDEN_SMSG_HASH_REQUEST;
    memcpy(Request.Seed, _currentModule->Seed, 16);

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenHashRequest));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenHashRequest) + sizeof(uint32));
    pkt << uint32(sizeof(WardenHashRequest));
    pkt.append((uint8*)&Request, sizeof(WardenHashRequest));
    _session->SendPacket(&pkt);
}

void Warden::Update()
{
    uint32 currentUpdateTime = getMSTime();
    uint32 diff = currentUpdateTime - _lastUpdateTime;
    _lastUpdateTime = currentUpdateTime;

    // disable update for mac
    if (_session->GetOS() == "OSX")
        return;

    Player* player = _session->GetPlayer();

    // removing player with incorrect state from world
    if (player && player->IsInWorld() && !player->IsBeingTeleported())
    {
        if (!IsValidStateInWorld(_session->GetOS()))
        {
            sLog->outWarden("Warden (%s) not correctly initialized (state: %u) on account %u, player %s", _session->GetOS().c_str(), uint8(_state), _session->GetAccountId(), _session->GetPlayerName());
            _state = WARDEN_MODULE_SET_PLAYER_LOCK;
            _pendingKickTimer = 10 * IN_MILLISECONDS;
        }
        else
        {
            // custom state for pending lock after MPQ checks on auth
            if (_state == WARDEN_MODULE_SET_PLAYER_PENDING_LOCK)
            {
                sLog->outWarden("Warden (%s) set pending lock on account %u, player %s", _session->GetOS().c_str(), _session->GetAccountId(), _session->GetPlayerName());
                SetPlayerLocked("uWoW Anticheat: Banned MPQ patches detected.");
            }
        }
    }

    switch (_state)
    {
        // need check it
        case WARDEN_NOT_INITIALIZED:
        case WARDEN_MODULE_NOT_LOADED:
        case WARDEN_MODULE_LOADED:
        case WARDEN_MODULE_SPECIAL_DBC_CHECKS:
        case WARDEN_MODULE_SET_PLAYER_PENDING_LOCK:
            break;
        case WARDEN_MODULE_WAIT_INITIALIZE:
        {
            ReInitTimerUpdate(diff);
            break;
        }
        case WARDEN_MODULE_READY:
        case WARDEN_MODULE_WAIT_RESPONSE:
        {
            if (player && player->IsInWorld() && !player->IsBeingTeleported())
            {
                if (_state == WARDEN_MODULE_READY)
                    StaticCheatChecksTimerUpdate(diff);
                else
                    ClientResponseTimerUpdate(diff);
            }
            break;
        }
        case WARDEN_MODULE_SET_PLAYER_LOCK:
        {
            PendingKickTimerUpdate(diff);
            break;
        }
        default:
            break;
    }
}

void Warden::ClientResponseTimerUpdate(uint32 diff)
{
    if (!_clientResponseTimer)
        return;

    if (_clientResponseTimer <= diff)
    {
        _clientResponseTimer = 0;
        sLog->outWarden("Player %s (guid: %u, account: %u, latency: %u, IP: %s) with module state %u exceeded Warden module (%s) response delay for more than 90s - disconnecting client",
            _session->GetPlayerName(), _session->GetGuidLow(), _session->GetAccountId(), _session->GetLatency(), _session->GetRemoteAddress().c_str(), uint8(_state), _session->GetOS().c_str());
        _session->KickPlayer();
        return;
    }
    else
        _clientResponseTimer -= diff;
}

void Warden::StaticCheatChecksTimerUpdate(uint32 diff)
{
    if (!_checkTimer)
        return;

    if (_checkTimer <= diff)
    {
        _checkTimer = 0;
        RequestBaseData();
    }
    else
        _checkTimer -= diff;
}

void Warden::PendingKickTimerUpdate(uint32 diff)
{
    if (!_pendingKickTimer)
        return;

    if (_pendingKickTimer <= diff)
    {
        _pendingKickTimer = 0;
        _session->KickPlayer();
    }
    else
        _pendingKickTimer -= diff;
}

void Warden::ReInitTimerUpdate(uint32 diff)
{
    if (!_reInitTimer)
        return;

    if (_reInitTimer <= diff)
    {
        _reInitTimer = 0;
        InitializeMPQCheckFuncFake();
    }
    else
        _reInitTimer -= diff;
}

void Warden::DecryptData(uint8* buffer, uint32 length)
{
    ARC4::rc4_process(&_clientRC4State, buffer, length);
}

void Warden::EncryptData(uint8* buffer, uint32 length)
{
    ARC4::rc4_process(&_serverRC4State, buffer, length);
}

bool Warden::IsValidCheckSum(uint32 checksum, const uint8* data, const uint16 length)
{
    uint32 newChecksum = BuildChecksum(data, length);

    if (checksum != newChecksum)
        return false;
    else
        return true;
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

void Warden::SetPlayerLocked(std::string message)
{
    if (Player* player = _session->GetPlayer())
    {
        WorldPacket data(SMSG_MESSAGECHAT, 200);
        player->BuildMonsterChat(&data, CHAT_MSG_RAID_BOSS_EMOTE, message.c_str(), LANG_UNIVERSAL, player->GetName(), player->GetGUID());
        player->SendDirectMessage(&data);
    }

    _state = WARDEN_MODULE_SET_PLAYER_LOCK;
    _pendingKickTimer = 5 * IN_MILLISECONDS;
}

void Warden::SetPlayerLocked(uint16 checkId, WardenCheck* wd)
{
    std::string message = "uWoW Anticheat: Unknown internal error";

    if (wd)
    {
        switch (wd->Type)
        {
            case MPQ_CHECK: message = "uWoW Anticheat: Banned MPQ patches detected."; break;
            case LUA_STR_CHECK: message = "uWoW Anticheat: Banned addons detected."; break;
            case PAGE_CHECK_A:
            case PAGE_CHECK_B:
            {
                message = "uWoW Anticheat: Injected cheat code detected.";
                break;
            }
            case MEM_CHECK: message = "uWoW Anticheat: Unknown cheat detected."; break;
            case MODULE_CHECK: message = "uWoW Anticheat: Banned DLLs detected."; break;
            case PROC_CHECK: message = "uWoW Anticheat: API hooks detected."; break;
            default: message = "uWoW Anticheat: Unknown suspicious program detected."; break;
        }

        if (wd->BanReason != "")
            message = "uWoW Anticheat: " + wd->BanReason + " detected.";
    }

    if (Player* player = _session->GetPlayer())
    {
        WorldPacket data(SMSG_MESSAGECHAT, 200);
        player->BuildMonsterChat(&data, CHAT_MSG_RAID_BOSS_EMOTE, message.c_str(), LANG_UNIVERSAL, player->GetName(), player->GetGUID());
        player->SendDirectMessage(&data);
    }

    _state = WARDEN_MODULE_SET_PLAYER_LOCK;
    _pendingKickTimer = 5 * IN_MILLISECONDS;
}

std::string Warden::Penalty(uint16 checkId)
{
    WardenCheck* check = _wardenMgr->GetCheckDataById(checkId);

    if (check)
    {
        switch (check->Action)
        {
            case WARDEN_ACTION_LOG:
                return "None";
            case WARDEN_ACTION_INSTANT_KICK:
            {
                _session->KickPlayer();
                return "Kick (instant)";
            }
            case WARDEN_ACTION_BAN:
            {
                std::stringstream duration;

                // permaban
                if (check->BanTime == 0)
                    duration << "-1";
                else
                    duration << check->BanTime << "s";

                std::string accountName;
                AccountMgr::GetName(_session->GetAccountId(), accountName);
                std::stringstream banReason;
                banReason << "Cheat detected";

                if (check->BanReason == "")
                    banReason << ": " << "Unknown Hack" << " (CheckId: " << checkId << ")";
                else
                    banReason << ": " << check->BanReason << " (CheckId: " << checkId << ")";

                sWorld->BanAccount(BAN_ACCOUNT, accountName, duration.str(), banReason.str(), "uWoW Anticheat");
                return "Ban";
            }
            case WARDEN_ACTION_PENDING_KICK:
            {
                SetPlayerLocked(checkId, check);
                return "Kick (with message)";
            }
            default:
                break;
        }
    }

    // impossible, but..
    _session->KickPlayer();
    return "Unknown Error";
}

bool Warden::IsValidStateInWorld(std::string os)
{
    // not check internal state, because this state in update is IMPOSSIBLE
    if (_state == WARDEN_NOT_INITIALIZED)
        return true;

    if (os == "Win")
    {
        if (_state >= WARDEN_MODULE_READY)
            return true;

        // waiting reInit timer triggered
        if (_reInitTimer && _state == WARDEN_MODULE_WAIT_INITIALIZE)
            return true;
    }
    else
    {
        if (_state == WARDEN_MODULE_LOADED)
            return true;
    }

    return false;
}

void WorldSession::HandleWardenDataOpcode(WorldPacket& recvData)
{
    uint32 cryptedSize;
    recvData >> cryptedSize;

    _warden->DecryptData(const_cast<uint8*>(recvData.contents() + sizeof(uint32)), cryptedSize);

    uint8 opcode;
    recvData >> opcode;
    sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: CMSG opcode %02X, size %u", opcode, uint32(recvData.size()));
    //sLog->outWarden("Raw Packet Decrypted Data - %s", ByteArrayToHexStr(const_cast<uint8*>(recvData.contents()), recvData.size()).c_str());

    switch (opcode)
    {
        case WARDEN_CMSG_MODULE_MISSING:
        {
            _warden->SendModuleToClient();
            break;
        }
        case WARDEN_CMSG_MODULE_OK:
        {
            _warden->RequestHash();
            break;
        }
        case WARDEN_CMSG_CHEAT_CHECKS_RESULT:
        {
            _warden->HandleData(recvData);
            break;
        }
        case WARDEN_CMSG_MEM_CHECKS_RESULT:
        {
            sLog->outDebug(LOG_FILTER_WARDEN, "WARDEN: CMSG_MEM_CHECKS_RESULT received!");
            break;
        }
        case WARDEN_CMSG_HASH_RESULT:
        {
            _warden->HandleHashResult(recvData);
            _warden->InitializeModule();
            _warden->SendDbcChecks();
            break;
        }
        case WARDEN_CMSG_MODULE_FAILED:
        {
            _warden->HandleModuleFailed();
            break;
        }
        default:
        {
            sLog->outWarden("Unknown CMSG opcode %02X, size %u, account %s, module state - %u", opcode, uint32(recvData.size() - 5), GetAccountName().c_str(), uint8(_warden->GetState()));
            break;
        }
    }
}