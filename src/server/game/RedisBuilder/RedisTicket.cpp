/*
 * Copyright (C) 2005-2016 Uwow <http://uwow.biz/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "TicketMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Chat.h"
#include "World.h"
#include "RedisBuilderMgr.h"

void GmTicket::SaveTicket()
{
    TicketData["_playerGuid"] = GUID_LOPART(_playerGuid);
    TicketData["_playerName"] = _playerName;
    TicketData["_message"] = _message;
    TicketData["_createTime"] = _createTime;
    TicketData["_mapId"] = _mapId;
    TicketData["_posX"] = _posX;
    TicketData["_posY"] = _posY;
    TicketData["_posZ"] = _posZ;
    TicketData["_lastModifiedTime"] = _lastModifiedTime;
    TicketData["_closedBy"] = _closedBy;
    TicketData["_assignedTo"] = GUID_LOPART(_assignedTo);
    TicketData["_comment"] = _comment;
    TicketData["_completed"] = _completed;
    TicketData["_escalatedStatus"] = _escalatedStatus;
    TicketData["_viewed"] = _viewed;

    std::string index = std::to_string(_id);

    RedisDatabase.AsyncExecuteHSet("HSET", sRedisBuilderMgr->GetTicketKey(), index.c_str(), sRedisBuilderMgr->BuildString(TicketData).c_str(), _id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "GmTicket::SaveTicket _id %u", guid);
    });
}

void TicketMgr::LoadFromRedis()
{
    uint32 oldMSTime = getMSTime();

    RedisValue tickets = RedisDatabase.Execute("HGETALL", sRedisBuilderMgr->GetTicketKey());

    std::vector<RedisValue> ticketsVector;
    if (!sRedisBuilderMgr->LoadFromRedisArray(&tickets, ticketsVector))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "TicketMgr::LoadFromRedis tickets not found");
        return;
    }

    uint32 count = 0;
    for (auto itr = ticketsVector.begin(); itr != ticketsVector.end();)
    {
        uint32 ticketId = atoi(itr->toString().c_str());
        ++itr;
        if (itr->isInt())
        {
            ++itr;
            continue;
        }

        Json::Value data;
        if (!sRedisBuilderMgr->LoadFromRedis(&(*itr), data))
        {
            ++itr;
            sLog->outInfo(LOG_FILTER_REDIS, "TicketMgr::LoadFromRedis not parse ticketId %i", ticketId);
            continue;
        }
        else
            ++itr;

        GmTicket* ticket = new GmTicket();
        ticket->LoadFromDB(ticketId, data);

        if (!ticket->IsClosed())
            ++_openTicketCount;

        // Update max ticket id if necessary
        uint32 id = ticket->GetId();
        if (_lastTicketId < id)
            _lastTicketId = id;

        _ticketList[id] = ticket;

        ++count;
    }
    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u GM tickets in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void GmTicket::LoadFromDB(uint32 ticketId, Json::Value data)
{
    TicketData = data;

    _id                 = ticketId;
    _playerGuid         = MAKE_NEW_GUID(data["_playerGuid"].asUInt(), 0, HIGHGUID_PLAYER);
    _playerName         = data["_playerName"].asString();
    _message            = data["_message"].asString();
    _createTime         = data["_createTime"].asUInt();
    _mapId              = data["_mapId"].asUInt();
    _posX               = data["_posX"].asFloat();
    _posY               = data["_posY"].asFloat();
    _posZ               = data["_posZ"].asFloat();
    _lastModifiedTime   = data["_lastModifiedTime"].asUInt();
    _closedBy           = data["_closedBy"].asUInt();
    _assignedTo         = MAKE_NEW_GUID(data["_assignedTo"].asUInt(), 0, HIGHGUID_PLAYER);
    _comment            = data["_comment"].asString();
    _completed          = data["_completed"].asBool();
    _escalatedStatus    = GMTicketEscalationStatus(data["_escalatedStatus"].asUInt());
    _viewed             = data["_viewed"].asBool();
}

void GmTicket::DeleteTicket()
{
    TicketData.clear();
    std::string index = std::to_string(_id);

    RedisDatabase.AsyncExecuteH("HDEL", sRedisBuilderMgr->GetTicketKey(), index.c_str(), _id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "GmTicket::DeleteTicket _id %u", guid);
    });
}
