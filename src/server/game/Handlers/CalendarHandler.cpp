/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

/*
----- Opcodes Not Used yet -----

SMSG_CALENDAR_CLEAR_PENDING_ACTION SendCalendarClearPendingAction()
SMSG_CALENDAR_RAID_LOCKOUT_UPDATED SendCalendarRaidLockoutUpdated(InstanceSave const* save)

----- Opcodes without Sniffs -----
SMSG_CALENDAR_FILTER_GUILD              [ for (... uint32(count) { packguid(???), uint8(???) } ]
SMSG_CALENDAR_ARENA_TEAM                [ for (... uint32(count) { packguid(???), uint8(???) } ]
CMSG_CALENDAR_EVENT_INVITE_NOTES        [ packguid(Invitee), uint64(inviteId), string(Text), Boolean(Unk) ]
SMSG_CALENDAR_EVENT_INVITE_NOTES        [ uint32(unk1), uint32(unk2), uint32(unk3), uint32(unk4), uint32(unk5) ]
SMSG_CALENDAR_EVENT_INVITE_NOTES_ALERT  [ uint64(inviteId), string(Text) ]
SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT [ Structure unkown ]

*/

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "InstanceSaveMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "CalendarMgr.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"

void WorldSession::HandleCalendarGetCalendar(WorldPacket& /*recvData*/)
{
    uint64 guid = _player->GetGUID();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_GET_CALENDAR [" UI64FMTD "]", guid);

    time_t cur_time = time_t(time(NULL));

    CalendarEventIdList const& events = sCalendarMgr->GetPlayerEvents(guid);
    CalendarInviteIdList const& invites = sCalendarMgr->GetPlayerInvites(guid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_SEND_CALENDAR [" UI64FMTD "]", guid);
    WorldPacket data(SMSG_CALENDAR_SEND_CALENDAR, 1000);   // Impossible to get the correct size without doing a double iteration of some elements
    data << uint32(cur_time);                              // server time
    data << uint32(secsToTimeBitFields(cur_time));         // server time
    data << uint32(1135753200);                            // unk (28.12.2005 07:00)

    uint32 bpos1 = data.bitwpos();
    data.WriteBits(0, 20);                                 //Raid Reset Count

    uint32 bpos2 = data.bitwpos();
    data.WriteBits(invites.size(), 19);                    //Invite Count

    uint32 bpos3 = data.bitwpos();
    data.WriteBits(events.size(), 19);                     //Event Count

    uint32 bpos4 = data.bitwpos();
    data.WriteBits(0, 20);                                 //Instance Reset Count

    ObjectGuid tmpGUID = 0;
    ObjectGuid tmpGUID2 = 0;

    ByteBuffer eventBuffer;
    uint32 p1 = 0;
    for (CalendarEventIdList::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(*it);

        if (!calendarEvent)
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_SEND_CALENDAR: No Event found with id [" UI64FMTD "]", *it);
            continue;
        }

        tmpGUID = calendarEvent->GetCreatorGUID();

        data.WriteGuidMask<5, 4, 6>(tmpGUID2); //guild
        data.WriteGuidMask<0, 3>(tmpGUID); //creator
        data.WriteGuidMask<2, 3>(tmpGUID2);
        data.WriteGuidMask<4, 1, 7>(tmpGUID);
        data.WriteGuidMask<7>(tmpGUID2);
        data.WriteBits(calendarEvent->GetTitle().size(), 8);
        data.WriteGuidMask<5, 2>(tmpGUID);
        data.WriteGuidMask<1, 0>(tmpGUID2);
        data.WriteGuidMask<6>(tmpGUID);

        eventBuffer.WriteGuidBytes<6>(tmpGUID2);
        eventBuffer.WriteGuidBytes<2>(tmpGUID);
        eventBuffer.WriteGuidBytes<7>(tmpGUID2);
        eventBuffer << uint64(*it);
        eventBuffer << uint32(calendarEvent->GetType());
        eventBuffer.WriteString(calendarEvent->GetTitle().c_str());
        eventBuffer.WriteGuidBytes<7, 1>(tmpGUID);
        eventBuffer.WriteGuidBytes<0>(tmpGUID2);
        eventBuffer.WriteGuidBytes<6>(tmpGUID);
        eventBuffer << uint32(calendarEvent->GetDungeonId());
        eventBuffer.WriteGuidBytes<3>(tmpGUID);
        eventBuffer << uint32(calendarEvent->GetTime());
        eventBuffer.WriteGuidBytes<5>(tmpGUID);
        eventBuffer << uint32(calendarEvent->GetFlags());
        eventBuffer.WriteGuidBytes<0>(tmpGUID);
        eventBuffer.WriteGuidBytes<5, 1, 2, 3, 4>(tmpGUID2);
        eventBuffer.WriteGuidBytes<4>(tmpGUID);
        ++p1;
    }

    ByteBuffer instanceBuffer;
    uint32 p2 = 0;
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        for (Player::BoundInstancesMap::const_iterator itr = _player->m_boundInstances[i].begin(); itr != _player->m_boundInstances[i].end(); ++itr)
            if (itr->second.perm)
            {
                InstanceSave* save = itr->second.save;
                tmpGUID = save->GetInstanceId();    // instance save id as unique instance copy id
                data.WriteGuidMask<5, 4, 1, 6, 2, 0, 7, 3>(tmpGUID);

                instanceBuffer.WriteGuidBytes<5, 2, 1>(tmpGUID);
                instanceBuffer << uint32(save->GetDifficulty());
                instanceBuffer.WriteGuidBytes<7>(tmpGUID);
                instanceBuffer << uint32(save->GetResetTime() - cur_time);
                instanceBuffer.WriteGuidBytes<4, 3, 6, 0>(tmpGUID);
                instanceBuffer << uint32(save->GetMapId());
                ++p2;
            }

    ByteBuffer inviteBuffer;
    uint32 p3 = 0;
    for (CalendarInviteIdList::const_iterator it = invites.begin(); it != invites.end(); ++it)
    {
        CalendarInvite* invite = sCalendarMgr->GetInvite(*it);
        CalendarEvent* calendarEvent = invite ? sCalendarMgr->GetEvent(invite->GetEventId()) : NULL;

        if (!calendarEvent)
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_SEND_CALENDAR: No Invite found with id [" UI64FMTD "]", *it);
            continue;
        }

        tmpGUID = calendarEvent->GetCreatorGUID();
        data.WriteGuidMask<7, 0, 5, 4, 1, 6, 3, 2>(tmpGUID);

        inviteBuffer.WriteGuidBytes<3, 4>(tmpGUID);
        inviteBuffer << uint8(0);
        inviteBuffer.WriteGuidBytes<5>(tmpGUID);
        inviteBuffer << uint64(invite->GetEventId());
        inviteBuffer << uint8(0);
        inviteBuffer << uint8(0);
        inviteBuffer.WriteGuidBytes<2, 0>(tmpGUID);
        inviteBuffer << uint64(invite->GetInviteId());
        inviteBuffer.WriteGuidBytes<7, 1, 6>(tmpGUID);
        
        //eventBuffer << uint8(invite->GetStatus());
        //eventBuffer << uint8(invite->GetRank());
        //eventBuffer << uint8(calendarEvent->GetGuildId() != 0);
        ++p3;
    }

    // TODO: Fix this, how we do know how many and what holidays to send?
    uint32 holidayCount = 0;
    uint32 bpos5 = data.bitwpos();
    data.WriteBits(holidayCount, 16);
    for (uint32 i = 0; i < holidayCount; ++i)
    {
        HolidaysEntry const* holiday = sHolidaysStore.LookupEntry(666);

        data << uint32(holiday->Id);                        // m_ID
        data << uint32(holiday->Region);                    // m_region, might be looping
        data << uint32(holiday->Looping);                   // m_looping, might be region
        data << uint32(holiday->Priority);                  // m_priority
        data << uint32(holiday->CalendarFilterType);        // m_calendarFilterType

        for (uint8 j = 0; j < MAX_HOLIDAY_DATES; ++j)
            data << uint32(holiday->Date[j]);               // 26 * m_date

        for (uint8 j = 0; j < MAX_HOLIDAY_DURATIONS; ++j)
            data << uint32(holiday->Duration[j]);           // 10 * m_duration

        for (uint8 j = 0; j < MAX_HOLIDAY_FLAGS; ++j)
            data << uint32(holiday->CalendarFlags[j]);      // 10 * m_calendarFlags

        data << holiday->TextureFilename;                   // m_textureFilename (holiday name)
    }

    data.FlushBits();
    data.PutBits<uint32>(bpos3, p1, 19);
    data.PutBits<uint32>(bpos4, p2, 20);
    data.PutBits<uint32>(bpos2, p3, 19);
    data.PutBits<uint32>(bpos5, holidayCount, 16);

    data.append(eventBuffer);
    data.append(inviteBuffer);
    data.append(instanceBuffer);

    uint32 counter = 0;
    std::set<uint32> sentMaps;
    for (MapDifficultyMap::const_iterator itr = sMapDifficultyMap.begin(); itr != sMapDifficultyMap.end(); ++itr)
    {
        uint32 map_diff_pair = itr->first;
        uint32 mapId = PAIR32_LOPART(map_diff_pair);
        Difficulty difficulty = Difficulty(PAIR32_HIPART(map_diff_pair));

        if (sentMaps.find(mapId) != sentMaps.end())
            continue;

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsRaid())
            continue;

        time_t timeReset = 0;
        MapDifficulty const* mapDiff = GetMapDifficultyData(mapId, difficulty);
        if (mapDiff && mapDiff->resetTime)
            timeReset = sWorld->getInstanceResetTime(mapDiff->resetTime);
        else
            continue;

        sentMaps.insert(mapId);

        data << uint32(mapId);
        data << uint32(timeReset - cur_time);
        data << uint32(mapEntry->unk_time);
        ++counter;
    }
    data.PutBits<uint32>(bpos1, counter, 20);

    SendPacket(&data);
}

void WorldSession::HandleCalendarGetEvent(WorldPacket& recvData)
{
    uint64 eventId;
    recvData >> eventId;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_GET_EVENT. Event: ["
        UI64FMTD "] Event [" UI64FMTD "]", _player->GetGUID(), eventId);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
        SendCalendarEvent(*calendarEvent, CALENDAR_SENDTYPE_GET);
}

void WorldSession::HandleCalendarGuildFilter(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_GUILD_FILTER [" UI64FMTD "]", _player->GetGUID());

    int32 unk1, unk2, unk3;
    recvData >> unk1;
    recvData >> unk2;
    recvData >> unk3;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Calendar: CMSG_CALENDAR_GUILD_FILTER - unk1: %d unk2: %d unk3: %d", unk1, unk2, unk3);
}

void WorldSession::HandleCalendarArenaTeam(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_ARENA_TEAM [" UI64FMTD "]", _player->GetGUID());

    int32 unk1;
    recvData >> unk1;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Calendar: CMSG_CALENDAR_ARENA_TEAM - unk1: %d", unk1);
}

//! 5.4.1
void WorldSession::HandleCalendarAddEvent(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    std::string title;
    std::string description;
    uint8 type;
    uint32 maxInvites;
    int32 dungeonId;
    uint32 eventTime;
    uint32 flags;
    uint64 inviteId = 0;
    uint64 invitee = 0;
    uint8 status = CALENDAR_STATUS_NO_OWNER;
    uint8 rank = CALENDAR_RANK_PLAYER;

    recvData >> flags >> eventTime >> maxInvites >> dungeonId >> type;
    uint32 inviteCount = recvData.ReadBits(22);

    std::map<uint32, ObjectGuid> inviteCountGuid;
    for(uint32 i = 0; i < inviteCount; ++i)
        recvData.ReadGuidMask<1, 7, 2, 3, 4, 0, 6, 5>(inviteCountGuid[i]);

    uint16 titleLen = recvData.ReadBits(5);
    uint16 descrLen = recvData.ReadBits(11);

    recvData.ResetBitReader();

    title = recvData.ReadString(titleLen);

    CalendarAction action;
    uint64 eventID = sCalendarMgr->GetFreeEventId();

    for(uint32 i = 0; i < inviteCount; ++i)
    {
        recvData.ReadGuidBytes<2>(inviteCountGuid[i]);
        recvData >> status;
        recvData.ReadGuidBytes<5, 3>(inviteCountGuid[i]);
        recvData >> rank;
        recvData.ReadGuidBytes<1, 7, 0, 4, 6>(inviteCountGuid[i]);

        inviteId = sCalendarMgr->GetFreeInviteId();

        CalendarInvite Invite(inviteId);
        Invite.SetEventId(action.Event.GetEventId());
        Invite.SetInviteId(inviteId);
        Invite.SetInvitee(inviteCountGuid[i]);
        Invite.SetStatus((CalendarInviteStatus) status);
        Invite.SetRank((CalendarModerationRank) rank);
        Invite.SetSenderGUID(guid);
        action.SetInvite(Invite);
    }

    description = recvData.ReadString(descrLen);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_ADD_EVENT: [" UI64FMTD "] "
        "Title %s, Description %s, type %u, MaxInvites %u, "
        "Dungeon ID %d, Time %u Flags %u, Invitee [" UI64FMTD "] "
        "Status %d, Rank %d", guid, title.c_str(), description.c_str(),
        type, maxInvites, dungeonId, eventTime,
        flags, invitee, status, rank);

    action.SetAction(CALENDAR_ACTION_ADD_EVENT);
    action.SetPlayer(_player);
    action.Event.SetEventId(eventID);
    action.Event.SetCreatorGUID(guid);
    action.Event.SetType((CalendarEventType) type);
    action.Event.SetFlags(flags);
    action.Event.SetTime(eventTime);
    action.Event.SetTimeZoneTime(eventTime);
    action.Event.SetRepeatable(false);
    action.Event.SetMaxInvites(maxInvites);
    action.Event.SetDungeonId(dungeonId);
    action.Event.SetGuildId((flags & CALENDAR_FLAG_GUILD_ONLY) ? GetPlayer()->GetGuildId() : 0);
    action.Event.SetTitle(title);
    action.Event.AddInvite(inviteId);
    action.Event.SetDescription(description);

    sCalendarMgr->AddAction(action, eventID);
}

void WorldSession::HandleCalendarUpdateEvent(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    std::string title;
    std::string description;
    uint8 type;
    bool repeatable;
    uint32 maxInvites;
    int32 dungeonId;
    uint32 eventPackedTime;
    uint32 timeZoneTime;
    uint32 flags;

    recvData >> eventId >> inviteId >> title >> description >> type;
    recvData >> repeatable >> maxInvites >> dungeonId;
    recvData  >> eventPackedTime >> timeZoneTime >> flags;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_UPDATE_EVENT [" UI64FMTD "] EventId [" UI64FMTD
        "], InviteId [" UI64FMTD "] Title %s, Description %s, type %u "
        "Repeatable %u, MaxInvites %u, Dungeon ID %d, Time %u "
        "Time2 %u, Flags %u", guid, eventId, inviteId, title.c_str(),
        description.c_str(), type, repeatable, maxInvites, dungeonId,
        eventPackedTime, timeZoneTime, flags);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_MODIFY_EVENT);
    action.SetPlayer(_player);
    action.SetActionInviteId(inviteId);
    action.Event.SetEventId(eventId);
    action.Event.SetType((CalendarEventType) type);
    action.Event.SetFlags((CalendarFlags) flags);
    action.Event.SetTime(eventPackedTime);
    action.Event.SetTimeZoneTime(timeZoneTime);
    action.Event.SetRepeatable(repeatable);
    action.Event.SetDungeonId(dungeonId);
    action.Event.SetTitle(title);
    action.Event.SetDescription(description);
    action.Event.SetMaxInvites(maxInvites);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarRemoveEvent(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    uint32 flags;

    recvData >> eventId >> inviteId >> flags;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_REMOVE_EVENT [" UI64FMTD "], EventId [" UI64FMTD
        "] inviteId [" UI64FMTD "] Flags?: %u", guid, eventId, inviteId, flags);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_REMOVE_EVENT);
    action.SetPlayer(_player);
    action.SetActionInviteId(inviteId);
    action.Event.SetEventId(eventId);
    action.Event.SetFlags((CalendarFlags) flags);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarCopyEvent(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    uint32 time;

    recvData >> eventId >> inviteId >> time;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_COPY_EVENT [" UI64FMTD "], EventId [" UI64FMTD
        "] inviteId [" UI64FMTD "] Time: %u", guid, eventId, inviteId, time);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_COPY_EVENT);
    action.SetPlayer(_player);
    action.SetActionInviteId(inviteId);
    action.Event.SetEventId(eventId);
    action.Event.SetTime(time);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarEventInvite(WorldPacket& recvData)
{
    time_t now = time(NULL);
    if (now - timeAddIgnoreOpcode < 3)
    {
        recvData.rfinish();
        return;
    }
    else
       timeAddIgnoreOpcode = now;

    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    std::string name;
    uint8 status;
    uint8 rank;
    uint64 invitee = 0;
    uint32 team = 0;

    recvData >> eventId >> inviteId >> name >> status >> rank;
    if (Player* player = sObjectAccessor->FindPlayerByName(name.c_str()))
    {
        invitee = player->GetGUID();
        team = player->GetTeam();
    }
    else
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_RACE_ACC_BY_NAME);
        stmt->setString(0, name);
        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            Field* fields = result->Fetch();
            invitee = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
            team = Player::TeamForRace(fields[1].GetUInt8());
        }
    }

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_EVENT_INVITE [" UI64FMTD "], EventId ["
        UI64FMTD "] InviteId [" UI64FMTD "] Name %s ([" UI64FMTD "]), status %u, "
        "Rank %u", guid, eventId, inviteId, name.c_str(), invitee, status, rank);

    if (!invitee)
    {
        SendCalendarCommandResult(CALENDAR_ERROR_PLAYER_NOT_FOUND);
        return;
    }

    if (_player->GetTeam() != team && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CALENDAR))
    {
        SendCalendarCommandResult(CALENDAR_ERROR_NOT_ALLIED);
        return;
    }

    // TODO: Check ignore, even if offline (db query)

    uint64 inviteID = sCalendarMgr->GetFreeInviteId();
    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_ADD_EVENT_INVITE);
    action.SetPlayer(_player);
    action.SetActionInviteId(inviteId);

    CalendarInvite Invite(inviteID);
    Invite.SetEventId(eventId);
    Invite.SetInviteId(inviteID);
    Invite.SetSenderGUID(_player->GetGUID());
    Invite.SetInvitee(invitee);
    Invite.SetRank((CalendarModerationRank) rank);
    Invite.SetStatus((CalendarInviteStatus) status);
    action.SetInvite(Invite);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarEventSignup(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint8 status;

    recvData >> eventId >> status;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_EVENT_SIGNUP [" UI64FMTD "] EventId ["
        UI64FMTD "] Status %u", guid, eventId, status);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_SIGNUP_TO_EVENT);
    action.SetPlayer(_player);
    action.SetExtraData(GetPlayer()->GetGuildId());
    action.SetActionInviteId((CalendarInviteStatus) status);
    action.Event.SetEventId(eventId);
    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarEventRsvp(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    uint8 status;

    recvData >> eventId >> inviteId >> status;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_EVENT_RSVP [" UI64FMTD"] EventId ["
        UI64FMTD "], InviteId [" UI64FMTD "], status %u", guid, eventId,
        inviteId, status);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_MODIFY_EVENT_INVITE);
    action.SetPlayer(_player);
    action.SetActionInviteId(inviteId);

    CalendarInvite Invite(inviteId);
    Invite.SetInviteId(inviteId);
    Invite.SetEventId(eventId);
    Invite.SetStatus((CalendarInviteStatus) status);
    action.SetInvite(Invite);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarEventRemoveInvite(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 invitee;
    uint64 eventId;
    uint64 owninviteId;
    uint64 inviteId;

    recvData.readPackGUID(invitee);
    recvData >> inviteId >> owninviteId >> eventId;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_EVENT_REMOVE_INVITE ["
        UI64FMTD "] EventId [" UI64FMTD "], OwnInviteId ["
        UI64FMTD "], Invitee ([" UI64FMTD "] id: [" UI64FMTD "])",
        guid, eventId, owninviteId, invitee, inviteId);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_REMOVE_EVENT_INVITE);
    action.SetPlayer(_player);
    action.SetActionInviteId(owninviteId);

    CalendarInvite Invite(inviteId);
    Invite.SetInviteId(inviteId);
    Invite.SetEventId(eventId);
    Invite.SetInvitee(invitee);
    action.SetInvite(Invite);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarEventStatus(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 owninviteId;
    uint8 status;

    recvData.readPackGUID(invitee);
    recvData >> eventId >>  inviteId >> owninviteId >> status;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_EVENT_STATUS [" UI64FMTD"] EventId ["
        UI64FMTD "] OwnInviteId [" UI64FMTD "], Invitee ([" UI64FMTD "] id: ["
        UI64FMTD "], status %u", guid, eventId, owninviteId, invitee, inviteId, status);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_MODIFY_EVENT_INVITE);
    action.SetPlayer(_player);
    action.SetActionInviteId(owninviteId);

    CalendarInvite Invite(inviteId);
    Invite.SetInviteId(inviteId);
    Invite.SetEventId(eventId);
    Invite.SetInvitee(invitee);
    Invite.SetStatus((CalendarInviteStatus) status);
    action.SetInvite(Invite);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarEventModeratorStatus(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 owninviteId;
    uint8 status;

    recvData.readPackGUID(invitee);
    recvData >> eventId >>  inviteId >> owninviteId >> status;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_EVENT_MODERATOR_STATUS [" UI64FMTD "] EventId ["
        UI64FMTD "] OwnInviteId [" UI64FMTD "], Invitee ([" UI64FMTD "] id: ["
        UI64FMTD "], status %u", guid, eventId, owninviteId, invitee, inviteId, status);

    CalendarAction action;
    action.SetAction(CALENDAR_ACTION_MODIFY_MODERATOR_EVENT_INVITE);
    action.SetPlayer(_player);
    action.SetActionInviteId(owninviteId);

    CalendarInvite Invite(inviteId);
    Invite.SetInviteId(inviteId);
    Invite.SetEventId(eventId);
    Invite.SetInvitee(invitee);
    Invite.SetStatus((CalendarInviteStatus) status);
    action.SetInvite(Invite);

    sCalendarMgr->AddAction(action, eventId);
}

void WorldSession::HandleCalendarComplain(WorldPacket& recvData)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId;
    uint64 complainGUID;

    recvData >> eventId >> complainGUID;
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_COMPLAIN [" UI64FMTD "] EventId ["
        UI64FMTD "] guid [" UI64FMTD "]", guid, eventId, complainGUID);
}

void WorldSession::HandleCalendarGetNumPending(WorldPacket& /*recvData*/)
{
    uint64 guid = _player->GetGUID();
    uint32 pending = sCalendarMgr->GetPlayerNumPending(guid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_CALENDAR_GET_NUM_PENDING: [" UI64FMTD
        "] Pending: %u", guid, pending);

    WorldPacket data(SMSG_CALENDAR_SEND_NUM_PENDING, 4);
    data << uint32(pending);
    SendPacket(&data);
}

// ----------------------------------- SEND ------------------------------------

void WorldSession::SendCalendarEvent(CalendarEvent const& calendarEvent, CalendarSendEventType sendEventType)
{
    uint64 eventId = calendarEvent.GetEventId();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_SEND_EVENT [" UI64FMTD "] EventId ["
        UI64FMTD "] SendType %u", _player->GetGUID(), eventId, sendEventType);

    WorldPacket data(SMSG_CALENDAR_SEND_EVENT);
    data << uint8(sendEventType);
    data.appendPackGUID(calendarEvent.GetCreatorGUID());
    data << uint64(eventId);
    data << calendarEvent.GetTitle().c_str();
    data << calendarEvent.GetDescription().c_str();
    data << uint8(calendarEvent.GetType());
    data << uint8(calendarEvent.GetRepeatable());
    data << uint32(calendarEvent.GetMaxInvites());
    data << int32(calendarEvent.GetDungeonId());
    data << uint32(calendarEvent.GetFlags());
    data << uint32(calendarEvent.GetTime());
    data << uint32(calendarEvent.GetTimeZoneTime());
    //data << uint32(calendarEvent.GetGuildId());
    data << uint64(0);
    CalendarInviteIdList const& invites = calendarEvent.GetInviteIdList();
    data << uint32(invites.size());
    for (CalendarInviteIdList::const_iterator it = invites.begin(); it != invites.end(); ++it)
    {
        if (CalendarInvite* invite = sCalendarMgr->GetInvite(*it))
        {
            uint64 guid = invite->GetInvitee();
            Player* player = ObjectAccessor::FindPlayer(guid);
            uint8 level = player ? player->getLevel() : Player::GetLevelFromDB(guid);

            data.appendPackGUID(guid);
            data << uint8(level);
            data << uint8(invite->GetStatus());
            data << uint8(invite->GetRank());
            data << uint8(calendarEvent.GetGuildId() != 0);
            data << uint64(invite->GetInviteId());
            data << uint32(invite->GetStatusTime());
            data << invite->GetText().c_str();
        }
        else
        {
            data.appendPackGUID(_player->GetGUID());
            data << uint8(0) << uint8(0) << uint8(0) << uint8(0)
                << uint64(0) << uint32(0) << uint8(0);

            sLog->outError(LOG_FILTER_NETWORKIO, "SendCalendarEvent: No Invite found with id [" UI64FMTD "]", *it);
        }
    }
    SendPacket(&data);
}

void WorldSession::SendCalendarEventInvite(CalendarInvite const& invite, bool pending)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = invite.GetEventId();
    uint64 inviteId = invite.GetInviteId();
    uint64 invitee = invite.GetInvitee();
    uint8 status = invite.GetStatus();
    uint32 statusTime = invite.GetStatusTime();
    Player* player = ObjectAccessor::FindPlayer(invitee);
    uint8 level = player ? player->getLevel() : Player::GetLevelFromDB(invitee);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_INVITE [" UI64FMTD "] EventId ["
        UI64FMTD "] InviteId [" UI64FMTD "] Invitee [" UI64FMTD "] "
        " Level %u, Status %u, StatusTime %u" , guid, eventId, inviteId,
        invitee, level, status, statusTime);

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE, 8 + 8 + 8 + 1 + 1 + 1 + (statusTime ? 4 : 0) + 1);
    data.appendPackGUID(invitee);
    data << uint64(eventId);
    data << uint64(inviteId);
    data << uint8(level);
    data << uint8(status);
    if (statusTime)
        data << uint8(1) << uint32(statusTime);
    else
        data << uint8(0);
    data << uint8(pending);

    SendPacket(&data);
}

void WorldSession::SendCalendarEventInviteAlert(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = calendarEvent.GetEventId();
    uint64 inviteId = invite.GetInviteId();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_INVITE_ALERT [" UI64FMTD "] EventId ["
        UI64FMTD "] InviteId [" UI64FMTD "]", guid, eventId, inviteId);

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_ALERT);
    data << uint64(eventId);
    data << calendarEvent.GetTitle().c_str();
    data << uint32(calendarEvent.GetTime());
    data << uint32(calendarEvent.GetFlags());
    data << uint32(calendarEvent.GetType());
    data << uint32(calendarEvent.GetDungeonId());
    data << uint64(inviteId);
    data << uint8(invite.GetStatus());
    data << uint8(invite.GetRank());
    data.appendPackGUID(calendarEvent.GetCreatorGUID());
    data.appendPackGUID(invite.GetSenderGUID());
    SendPacket(&data);
}

void WorldSession::SendCalendarEventUpdateAlert(CalendarEvent const& calendarEvent, CalendarSendEventType sendEventType)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = calendarEvent.GetEventId();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_UPDATED_ALERT ["
        UI64FMTD "] EventId [" UI64FMTD "]", guid, eventId);


    WorldPacket data(SMSG_CALENDAR_EVENT_UPDATED_ALERT, 1 + 8 + 4 + 4 + 4 + 1 + 4 +
        calendarEvent.GetTitle().size() + calendarEvent.GetDescription().size() + 1 + 4 + 4);
    data << uint8(sendEventType);
    data << uint64(eventId);
    data << uint32(calendarEvent.GetTime());
    data << uint32(calendarEvent.GetFlags());
    data << uint32(calendarEvent.GetTime());
    data << uint8(calendarEvent.GetType());
    data << uint32(calendarEvent.GetDungeonId());
    data << calendarEvent.GetTitle().c_str();
    data << calendarEvent.GetDescription().c_str();
    data << uint8(calendarEvent.GetRepeatable());
    data << uint32(calendarEvent.GetMaxInvites());
    data << uint32(0); // FIXME
    SendPacket(&data);
}

void WorldSession::SendCalendarEventRemovedAlert(CalendarEvent const& calendarEvent)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = calendarEvent.GetEventId();
    uint32 eventTime = (calendarEvent.GetTime());

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_REMOVED_ALERT [" UI64FMTD "] EventId ["
        UI64FMTD "] Time %u", guid, eventId, eventTime);

    WorldPacket data(SMSG_CALENDAR_EVENT_REMOVED_ALERT, 1 + 8 + 1);
    data << uint8(1); // FIXME: If true does not SignalEvent(EVENT_CALENDAR_ACTION_PENDING)
    data << uint64(eventId);
    data << uint32(eventTime);
    SendPacket(&data);
}

void WorldSession::SendCalendarEventStatus(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = calendarEvent.GetEventId();
    uint64 inviteId = invite.GetInviteId();
    uint64 invitee = invite.GetInvitee();
    uint32 eventTime = (calendarEvent.GetTime());
    uint32 flags = calendarEvent.GetFlags();
    uint8 status = invite.GetStatus();
    uint8 rank = invite.GetRank();
    uint32 statusTime = secsToTimeBitFields(invite.GetStatusTime());


    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_STATUS [" UI64FMTD "] EventId ["
        UI64FMTD "] InviteId [" UI64FMTD "] Invitee [" UI64FMTD "] Time %u "
        "Flags %u, Status %u, Rank %u, StatusTime %u",
        guid, eventId, inviteId, invitee, eventTime, flags, status, rank,
        statusTime);

    WorldPacket data(SMSG_CALENDAR_EVENT_STATUS, 8 + 8 + 4 + 4 + 1 + 1 + 4);
    data.appendPackGUID(invitee);
    data << uint64(eventId);
    data << uint32(eventTime);
    data << uint32(flags);
    data << uint8(status);
    data << uint8(rank);
    data << uint32(statusTime);
    SendPacket(&data);
}

void WorldSession::SendCalendarEventModeratorStatusAlert(CalendarInvite const& invite)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = invite.GetEventId();
    uint64 invitee = invite.GetInvitee();
    uint8 status = invite.GetStatus();


    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT [" UI64FMTD
        "] Invitee [" UI64FMTD "] EventId [" UI64FMTD "] Status %u ", guid,
        invitee, eventId, status);


    WorldPacket data(SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT, 8 + 8 + 1 + 1);
    data.appendPackGUID(invitee);
    data << uint64(eventId);
    data << uint8(status);
    data << uint8(1); // FIXME
    SendPacket(&data);
}

void WorldSession::SendCalendarEventInviteRemoveAlert(CalendarEvent const& calendarEvent, CalendarInviteStatus status)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = calendarEvent.GetEventId();
    uint32 eventTime = (calendarEvent.GetTime());
    uint32 flags = calendarEvent.GetFlags();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT ["
        UI64FMTD "] EventId [" UI64FMTD "] Time %u, Flags %u, Status %u",
        guid, eventId, eventTime, flags, status);

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT, 8 + 4 + 4 + 1);
    data << uint64(eventId);
    data << uint32(eventTime);
    data << uint32(flags);
    data << uint8(status);
    SendPacket(&data);
}

void WorldSession::SendCalendarEventInviteRemove(CalendarInvite const& invite, uint32 flags)
{
    uint64 guid = _player->GetGUID();
    uint64 eventId = invite.GetEventId();
    uint64 invitee = invite.GetInvitee();

    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_EVENT_INVITE_REMOVED ["
        UI64FMTD "] Invitee [" UI64FMTD "] EventId [" UI64FMTD
        "] Flags %u", guid, invitee, eventId, flags);

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED, 8 + 4 + 4 + 1);
    data.appendPackGUID(invitee);
    data << uint32(eventId);
    data << uint32(flags);
    data << uint8(1); // FIXME
    SendPacket(&data);
}

void WorldSession::SendCalendarClearPendingAction()
{
    uint64 guid = _player->GetGUID();
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_CLEAR_PENDING_ACTION [" UI64FMTD "]", guid);

    WorldPacket data(SMSG_CALENDAR_CLEAR_PENDING_ACTION, 0);
    SendPacket(&data);
}

void WorldSession::SendCalendarCommandResult(CalendarError err, char const* param /*= NULL*/)
{
    uint64 guid = _player->GetGUID();
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_COMMAND_RESULT [" UI64FMTD "] Value: %u", guid, err);

    WorldPacket data(SMSG_CALENDAR_COMMAND_RESULT, 0);
    data << uint32(0);
    data << uint8(0);
    switch (err)
    {
        case CALENDAR_ERROR_OTHER_INVITES_EXCEEDED:
        case CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S:
        case CALENDAR_ERROR_IGNORING_YOU_S:
            data << param;
            break;
        default:
            data << uint8(0);
            break;
    }

    data << uint32(err);

    SendPacket(&data);
}

void WorldSession::SendCalendarRaidLockout(InstanceSave* save, bool add)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "%s", add ? "SMSG_CALENDAR_RAID_LOCKOUT_ADDED" : "SMSG_CALENDAR_RAID_LOCKOUT_REMOVED");
    time_t currTime = time(NULL);

    WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_REMOVED, (add ? 4 : 0) + 4 + 4 + 4 + 8);
    if (add)
    {
        data.SetOpcode(SMSG_CALENDAR_RAID_LOCKOUT_ADDED);
        data << uint32(secsToTimeBitFields(currTime));
    }

    data << uint32(save->GetMapId());
    data << uint32(save->GetDifficulty());
    data << uint32(save->GetResetTime() - currTime);
    data << uint64(save->GetInstanceId());
    SendPacket(&data);
}

void WorldSession::SendCalendarRaidLockoutUpdated(InstanceSave* save)
{
    if (!save)
        return;

    uint64 guid = _player->GetGUID();
    sLog->outDebug(LOG_FILTER_NETWORKIO, "SMSG_CALENDAR_RAID_LOCKOUT_UPDATED [" UI64FMTD
        "] Map: %u, Difficulty %u", guid, save->GetMapId(), save->GetDifficulty());

    time_t cur_time = time_t(time(NULL));

    WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_UPDATED, 4 + 4 + 4 + 4 + 8);
    data << secsToTimeBitFields(cur_time);
    data << uint32(save->GetMapId());
    data << uint32(save->GetDifficulty());
    data << uint32(0); // Amount of seconds that has changed to the reset time
    data << uint32(save->GetResetTime() - cur_time);
    SendPacket(&data);
}
