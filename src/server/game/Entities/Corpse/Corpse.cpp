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

#include "Common.h"
#include "Corpse.h"
#include "Player.h"
#include "UpdateMask.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"
#include "Opcodes.h"
#include "GossipDef.h"
#include "World.h"
#include "RedisBuilderMgr.h"

Corpse::Corpse(CorpseType type) : WorldObject(type != CORPSE_BONES), m_type(type)
{
    m_objectType |= TYPEMASK_CORPSE;
    m_objectTypeId = TYPEID_CORPSE;

    m_updateFlag = UPDATEFLAG_STATIONARY_POSITION;

    m_valuesCount = CORPSE_END;

    m_time = time(NULL);

    lootForBody = false;
    lootRecipient = NULL;
    m_owner = NULL;
}

Corpse::~Corpse()
{
}

void Corpse::AddToWorld()
{
    ///- Register the corpse for guid lookup
    if (!IsInWorld())
        sObjectAccessor->AddObject(this);

    Object::AddToWorld();
}

void Corpse::RemoveFromWorld()
{
    ///- Remove the corpse from the accessor
    if (IsInWorld())
        sObjectAccessor->RemoveObject(this);

    WorldObject::RemoveFromWorld();
}

bool Corpse::Create(uint32 guidlow, Map* map)
{
    SetMap(map);
    Object::_Create(guidlow, 0, HIGHGUID_CORPSE);
    return true;
}

bool Corpse::Create(uint32 guidlow, Player* owner)
{
    ASSERT(owner);

    m_owner = owner;

    Relocate(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(), owner->GetOrientation());

    if (!IsPositionValid())
    {
        sLog->outError(LOG_FILTER_PLAYER, "Corpse (guidlow %d, owner %s) not created. Suggested coordinates isn't valid (X: %f Y: %f)",
            guidlow, owner->GetName(), owner->GetPositionX(), owner->GetPositionY());
        return false;
    }

    //we need to assign owner's map for corpse
    //in other way we will get a crash in Corpse::SaveToDB()
    SetMap(owner->GetMap());

    WorldObject::_Create(guidlow, HIGHGUID_CORPSE, owner->GetPhaseMask());

    SetObjectScale(1);
    SetUInt64Value(CORPSE_FIELD_OWNER, owner->GetGUID());

    _gridCoord = Trinity::ComputeGridCoord(GetPositionX(), GetPositionY());

    return true;
}

void Corpse::SaveToDB()
{
    if (!m_owner)
        return;

    m_owner->PlayerCorpse["corpseGuid"] = GetGUIDLow();
    m_owner->PlayerCorpse["posX"] = GetPositionX();
    m_owner->PlayerCorpse["posY"] = GetPositionY();
    m_owner->PlayerCorpse["posZ"] = GetPositionZ();
    m_owner->PlayerCorpse["orientation"] = GetOrientation();
    m_owner->PlayerCorpse["mapId"] = GetMapId();
    m_owner->PlayerCorpse["displayId"] = GetUInt32Value(CORPSE_FIELD_DISPLAY_ID);
    m_owner->PlayerCorpse["itemCache"] = _ConcatFields(CORPSE_FIELD_ITEM, EQUIPMENT_SLOT_END);
    m_owner->PlayerCorpse["bytes1"] = GetUInt32Value(CORPSE_FIELD_BYTES_1);
    m_owner->PlayerCorpse["bytes2"] = GetUInt32Value(CORPSE_FIELD_BYTES_2);
    m_owner->PlayerCorpse["flags"] = GetUInt32Value(CORPSE_FIELD_FLAGS);
    m_owner->PlayerCorpse["dynFlags"] = GetUInt32Value(CORPSE_FIELD_DYNAMIC_FLAGS);
    m_owner->PlayerCorpse["time"] = uint32(m_time);
    m_owner->PlayerCorpse["corpseType"] = GetType();
    m_owner->PlayerCorpse["instanceId"] = GetInstanceId();
    m_owner->PlayerCorpse["phaseMask"] = GetPhaseMask();

    RedisDatabase.AsyncExecuteHSet("HSET", m_owner->GetUserKey(), "corpse", sRedisBuilderMgr->BuildString(m_owner->PlayerCorpse).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Corpse::SaveToDB guid %u", guid);
    });
}

void Corpse::DeleteBonesFromWorld()
{
    ASSERT(GetType() == CORPSE_BONES);
    Corpse* corpse = ObjectAccessor::GetCorpse(*this, GetGUID());

    if (!corpse)
    {
        sLog->outError(LOG_FILTER_PLAYER, "Bones %u not found in world.", GetGUIDLow());
        return;
    }

    AddObjectToRemoveList();
}

void Corpse::DeleteFromDB()
{
    if (!m_owner)
        return;

    m_owner->PlayerCorpse.clear();

    RedisDatabase.AsyncExecuteH("HDEL", m_owner->GetUserKey(), "corpse", GetGUIDLow(), [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Corpse::DeleteFromDB id %u", guid);
    });
}

bool Corpse::LoadCorpseFromDB(Json::Value corpseData, Player* owner)
{
    //        0     1     2     3            4      5          6          7       8       9      10        11    12          13          14          15         16
    // SELECT posX, posY, posZ, orientation, mapId, displayId, itemCache, bytes1, bytes2, flags, dynFlags, time, corpseType, instanceId, phaseMask, corpseGuid, guid FROM corpse WHERE corpseType <> 0

    m_owner = owner;

    float posX   = corpseData["posX"].asFloat();
    float posY   = corpseData["posY"].asFloat();
    float posZ   = corpseData["posZ"].asFloat();
    float o      = corpseData["orientation"].asFloat();
    uint32 mapId = corpseData["mapId"].asUInt();

    Object::_Create(corpseData["corpseGuid"].asUInt(), 0, HIGHGUID_CORPSE);

    SetUInt32Value(CORPSE_FIELD_DISPLAY_ID, corpseData["displayId"].asUInt());
    _LoadIntoDataField(corpseData["itemCache"].asCString(), CORPSE_FIELD_ITEM, EQUIPMENT_SLOT_END);
    SetUInt32Value(CORPSE_FIELD_BYTES_1, corpseData["bytes1"].asUInt());
    SetUInt32Value(CORPSE_FIELD_BYTES_2, corpseData["bytes2"].asUInt());
    SetUInt32Value(CORPSE_FIELD_FLAGS, corpseData["flags"].asUInt());
    SetUInt32Value(CORPSE_FIELD_DYNAMIC_FLAGS, corpseData["dynFlags"].asUInt());
    SetUInt64Value(CORPSE_FIELD_OWNER, m_owner->GetGUID());

    m_time = time_t(corpseData["time"].asUInt());

    uint32 instanceId  = corpseData["instanceId"].asUInt();
    uint32 phaseMask   = corpseData["phaseMask"].asUInt();

    // place
    SetLocationInstanceId(instanceId);
    SetLocationMapId(mapId);
    SetPhaseMask(phaseMask, false);
    Relocate(posX, posY, posZ, o);

    if (!IsPositionValid())
    {
        sLog->outError(LOG_FILTER_PLAYER, "Corpse (guid: %u, owner: %u) is not created, given coordinates are not valid (X: %f, Y: %f, Z: %f)",
            GetGUIDLow(), GUID_LOPART(GetOwnerGUID()), posX, posY, posZ);
        return false;
    }

    _gridCoord = Trinity::ComputeGridCoord(GetPositionX(), GetPositionY());
    return true;
}

bool Corpse::IsExpired(time_t t) const
{
    if (m_type == CORPSE_BONES)
        return m_time < t - 5 * MINUTE;
    else
        return m_time < t - 3 * DAY;
}
