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

#include "ObjectAccessor.h"
#include "ObjectMgr.h"

#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "DynamicObject.h"
#include "Vehicle.h"
#include "WorldPacket.h"
#include "Item.h"
#include "Corpse.h"
#include "GridNotifiers.h"
#include "MapManager.h"
#include "Map.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "ObjectDefines.h"
#include "MapInstanced.h"
#include "World.h"

#include <cmath>

ObjectAccessor::ObjectAccessor()
{
}

ObjectAccessor::~ObjectAccessor()
{
}

WorldObjectPtr ObjectAccessor::GetWorldObject(constWorldObjectPtr p, uint64 guid)
{
    switch (GUID_HIPART(guid))
    {
        case HIGHGUID_PLAYER:        return GetPlayer(p, guid);
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_GAMEOBJECT:    return GetGameObject(p, guid);
        case HIGHGUID_VEHICLE:
        case HIGHGUID_UNIT:          return GetCreature(p, guid);
        case HIGHGUID_PET:           return GetPet(p, guid);
        case HIGHGUID_DYNAMICOBJECT: return GetDynamicObject(p, guid);
        case HIGHGUID_CORPSE:        return GetCorpse(p, guid);
        default:                     return nullptr;
    }
}

ObjectPtr ObjectAccessor::GetObjectByTypeMask(constWorldObjectPtr p, uint64 guid, uint32 typemask)
{
    switch (GUID_HIPART(guid))
    {
        case HIGHGUID_ITEM:
            if (typemask & TYPEMASK_ITEM && p->GetTypeId() == TYPEID_PLAYER)
                return (TO_CONST_PLAYER(p))->GetItemByGuid(guid);
            break;
        case HIGHGUID_PLAYER:
            if (typemask & TYPEMASK_PLAYER)
                return GetPlayer(p, guid);
            break;
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_GAMEOBJECT:
            if (typemask & TYPEMASK_GAMEOBJECT)
                return GetGameObject(p, guid);
            break;
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
            if (typemask & TYPEMASK_UNIT)
                return GetCreature(p, guid);
            break;
        case HIGHGUID_PET:
            if (typemask & TYPEMASK_UNIT)
                return GetPet(p, guid);
            break;
        case HIGHGUID_DYNAMICOBJECT:
            if (typemask & TYPEMASK_DYNAMICOBJECT)
                return GetDynamicObject(p, guid);
            break;
        case HIGHGUID_CORPSE:
            break;
    }

    return nullptr;
}

CorpsePtr ObjectAccessor::GetCorpse(constWorldObjectPtr u, uint64 guid)
{
    return GetObjectInMap(guid, u->GetMap(), (Corpse*)nullptr);
}

GameObjectPtr ObjectAccessor::GetGameObject(constWorldObjectPtr u, uint64 guid)
{
    return TO_GAMEOBJECT(GetObjectInMap(guid, u->GetMap(), (GameObject*)nullptr));
}

DynamicObjectPtr ObjectAccessor::GetDynamicObject(constWorldObjectPtr u, uint64 guid)
{
    return TO_DYNAMICOBJECT(GetObjectInMap(guid, u->GetMap(), (DynamicObject*)nullptr));
}

UnitPtr ObjectAccessor::GetUnit(constWorldObjectPtr u, uint64 guid)
{
    return GetObjectInMap(guid, u->GetMap(), (Unit*)nullptr);
}

CreaturePtr ObjectAccessor::GetCreature(constWorldObjectPtr u, uint64 guid)
{
    return GetObjectInMap(guid, u->GetMap(), (Creature*)nullptr);
}

PetPtr ObjectAccessor::GetPet(constWorldObjectPtr u, uint64 guid)
{
    return GetObjectInMap(guid, u->GetMap(), (Pet*)nullptr);
}

PlayerPtr ObjectAccessor::GetPlayer(constWorldObjectPtr u, uint64 guid)
{
    return GetObjectInMap(guid, u->GetMap(), (Player*)nullptr);
}

TransportPtr ObjectAccessor::GetTransport(constWorldObjectPtr u, uint64 guid)
{
    return GetObjectInMap(guid, u->GetMap(), (Transport*)nullptr);
}

CreaturePtr ObjectAccessor::GetCreatureOrPetOrVehicle(constWorldObjectPtr u, uint64 guid)
{
    if (IS_PET_GUID(guid))
        return GetPet(u, guid);

    if (IS_CRE_OR_VEH_GUID(guid))
        return GetCreature(u, guid);

    return nullptr;
}

PetPtr ObjectAccessor::FindPet(uint64 guid)
{
    return GetObjectInWorld(guid, (Pet*)nullptr);
}

PlayerPtr ObjectAccessor::FindPlayer(uint64 guid)
{
    return GetObjectInWorld(guid, (Player*)nullptr);
}

UnitPtr ObjectAccessor::FindUnit(uint64 guid)
{
    return GetObjectInWorld(guid, (Unit*)nullptr);
}

PlayerPtr ObjectAccessor::FindPlayerByName(const char* name)
{
    TRINITY_READ_GUARD(HashMapHolder<Player>::LockType, *HashMapHolder<Player>::GetLock());
    std::string nameStr = name;
    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    HashMapHolder<Player>::MapType const& m = GetPlayers();
    for (HashMapHolder<Player>::MapType::const_iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        if (!iter->second->IsInWorld())
            continue;
        std::string currentName = iter->second->GetName();
        std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::tolower);
        if (nameStr.compare(currentName) == 0)
            return iter->second;
    }

    return nullptr;
}

void ObjectAccessor::SaveAllPlayers()
{
    TRINITY_READ_GUARD(HashMapHolder<Player>::LockType, *HashMapHolder<Player>::GetLock());
    HashMapHolder<Player>::MapType const& m = GetPlayers();
    for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        itr->second->SaveToDB();
}

CorpsePtr ObjectAccessor::GetCorpseForPlayerGUID(uint64 guid)
{
    TRINITY_READ_GUARD(ACE_RW_Thread_Mutex, i_corpseLock);

    Player2CorpsesMapType::iterator iter = i_player2corpse.find(guid);
    if (iter == i_player2corpse.end())
        return nullptr;

    ASSERT(iter->second->GetType() != CORPSE_BONES);

    return iter->second;
}

void ObjectAccessor::RemoveCorpse(CorpsePtr corpse)
{
    ASSERT(corpse && corpse->GetType() != CORPSE_BONES);

    //TODO: more works need to be done for corpse and other world object
    if (MapPtr map = corpse->FindMap())
    {
        corpse->DestroyForNearbyPlayers();
        if (corpse->IsInGrid())
            map->RemoveFromMap(corpse, false);
        else
        {
            corpse->RemoveFromWorld();
            corpse->ResetMap();
        }
    }
    else

        corpse->RemoveFromWorld();

    // Critical section
    {
        TRINITY_WRITE_GUARD(ACE_RW_Thread_Mutex, i_corpseLock);

        Player2CorpsesMapType::iterator iter = i_player2corpse.find(corpse->GetOwnerGUID());
        if (iter == i_player2corpse.end()) // TODO: Fix this
            return;

        // build mapid*cellid -> guid_set map
        CellCoord cellCoord = Trinity::ComputeCellCoord(corpse->GetPositionX(), corpse->GetPositionY());
        sObjectMgr->DeleteCorpseCellData(corpse->GetMapId(), cellCoord.GetId(), GUID_LOPART(corpse->GetOwnerGUID()));

        i_player2corpse.erase(iter);
    }
}

void ObjectAccessor::AddCorpse(CorpsePtr corpse)
{
    ASSERT(corpse && corpse->GetType() != CORPSE_BONES);

    // Critical section
    {
        TRINITY_WRITE_GUARD(ACE_RW_Thread_Mutex, i_corpseLock);

        ASSERT(i_player2corpse.find(corpse->GetOwnerGUID()) == i_player2corpse.end());
        i_player2corpse[corpse->GetOwnerGUID()] = corpse;

        // build mapid*cellid -> guid_set map
        CellCoord cellCoord = Trinity::ComputeCellCoord(corpse->GetPositionX(), corpse->GetPositionY());
        sObjectMgr->AddCorpseCellData(corpse->GetMapId(), cellCoord.GetId(), GUID_LOPART(corpse->GetOwnerGUID()), corpse->GetInstanceId());
    }
}

void ObjectAccessor::AddCorpsesToGrid(GridCoord const& gridpair, GridType& grid, MapPtr map)
{
    TRINITY_READ_GUARD(ACE_RW_Thread_Mutex, i_corpseLock);

    for (Player2CorpsesMapType::iterator iter = i_player2corpse.begin(); iter != i_player2corpse.end(); ++iter)
    {
        // We need this check otherwise a corpose may be added to a grid twice
        if (iter->second->IsInGrid())
            continue;

        if (iter->second->GetGridCoord() == gridpair)
        {
            // verify, if the corpse in our instance (add only corpses which are)
            if (map->Instanceable())
            {
                if (iter->second->GetInstanceId() == map->GetInstanceId())
                    grid.AddWorldObject(iter->second);
            }
            else
                grid.AddWorldObject(iter->second);
        }
    }
}

CorpsePtr ObjectAccessor::ConvertCorpseForPlayer(uint64 player_guid, bool insignia /*=false*/)
{
    CorpsePtr corpse = GetCorpseForPlayerGUID(player_guid);
    if (!corpse)
    {
        //in fact this function is called from several places
        //even when player doesn't have a corpse, not an error
        return nullptr;
    }

    sLog->outDebug(LOG_FILTER_GENERAL, "Deleting Corpse and spawned bones.");

    // Map can be nullptr
    MapPtr map = corpse->FindMap();

    // remove corpse from player_guid -> corpse map and from current map
    RemoveCorpse(corpse);

    // remove corpse from DB
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    corpse->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    CorpsePtr bones = nullptr;
    // create the bones only if the map and the grid is loaded at the corpse's location
    // ignore bones creating option in case insignia

    if (map && (insignia ||
        (map->IsBattlegroundOrArena() ? sWorld->getBoolConfig(CONFIG_DEATH_BONES_BG_OR_ARENA) : sWorld->getBoolConfig(CONFIG_DEATH_BONES_WORLD))) &&
        !map->IsRemovalGrid(corpse->GetPositionX(), corpse->GetPositionY()))
    {
        // Create bones, don't change Corpse
        bones = CorpsePtr(new Corpse);
        bones->Create(corpse->GetGUIDLow(), map);

        for (uint8 i = OBJECT_FIELD_TYPE + 1; i < CORPSE_END; ++i)                    // don't overwrite guid and object type
            bones->SetUInt32Value(i, corpse->GetUInt32Value(i));

        bones->SetGridCoord(corpse->GetGridCoord());
        // bones->m_time = m_time;                              // don't overwrite time
        // bones->m_type = m_type;                              // don't overwrite type
        bones->Relocate(corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetOrientation());
        bones->SetPhaseMask(corpse->GetPhaseMask(), false);

        bones->SetUInt32Value(CORPSE_FIELD_FLAGS, CORPSE_FLAG_UNK2 | CORPSE_FLAG_BONES);
        bones->SetUInt64Value(CORPSE_FIELD_OWNER, 0);

        for (uint8 i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (corpse->GetUInt32Value(CORPSE_FIELD_ITEM + i))
                bones->SetUInt32Value(CORPSE_FIELD_ITEM + i, 0);
        }

        // add bones in grid store if grid loaded where corpse placed
        map->AddToMap(bones);
    }

    // all references to the corpse should be removed at this point

    return bones;
}

void ObjectAccessor::RemoveOldCorpses()
{
    time_t now = time(nullptr);
    Player2CorpsesMapType::iterator next;
    for (Player2CorpsesMapType::iterator itr = i_player2corpse.begin(); itr != i_player2corpse.end(); itr = next)
    {
        next = itr;
        ++next;

        if (!itr->second->IsExpired(now))
            continue;

        ConvertCorpseForPlayer(itr->first);
    }
}

void ObjectAccessor::Update(uint32 /*diff*/)
{
    UpdateDataMapType update_players;

    while (!i_objects.empty())
    {
        ObjectPtr obj = *i_objects.begin();
        ASSERT(obj && obj->IsInWorld());
        i_objects.erase(i_objects.begin());
        obj->BuildUpdate(update_players);
    }

    WorldPacket packet;                                     // here we allocate a std::vector with a size of 0x10000
    for (UpdateDataMapType::iterator iter = update_players.begin(); iter != update_players.end(); ++iter)
    {
        iter->second.BuildPacket(&packet);
        FindPlayer(iter->first)->GetSession()->SendPacket(&packet);
        packet.clear();                                     // clean the string
    }
}

void ObjectAccessor::UnloadAll()
{
    for (Player2CorpsesMapType::const_iterator itr = i_player2corpse.begin(); itr != i_player2corpse.end(); ++itr)
    {
        itr->second->RemoveFromWorld();
    }
}

/// Define the static members of HashMapHolder

template <class T> UNORDERED_MAP< uint64, std::shared_ptr<T> > HashMapHolder<T>::m_objectMap;
template <class T> typename HashMapHolder<T>::LockType HashMapHolder<T>::i_lock;

/// Global definitions for the hashmap storage

template class HashMapHolder<Player>;
template class HashMapHolder<Pet>;
template class HashMapHolder<GameObject>;
template class HashMapHolder<DynamicObject>;
template class HashMapHolder<Creature>;
template class HashMapHolder<Corpse>;
template class HashMapHolder<Transport>;

template PlayerPtr ObjectAccessor::GetObjectInWorld<Player>(uint32 mapid, float x, float y, uint64 guid, Player* /*fake*/);
template PetPtr ObjectAccessor::GetObjectInWorld<Pet>(uint32 mapid, float x, float y, uint64 guid, Pet* /*fake*/);
template CreaturePtr ObjectAccessor::GetObjectInWorld<Creature>(uint32 mapid, float x, float y, uint64 guid, Creature* /*fake*/);
template CorpsePtr ObjectAccessor::GetObjectInWorld<Corpse>(uint32 mapid, float x, float y, uint64 guid, Corpse* /*fake*/);
template GameObjectPtr ObjectAccessor::GetObjectInWorld<GameObject>(uint32 mapid, float x, float y, uint64 guid, GameObject* /*fake*/);
template DynamicObjectPtr ObjectAccessor::GetObjectInWorld<DynamicObject>(uint32 mapid, float x, float y, uint64 guid, DynamicObject* /*fake*/);
template TransportPtr ObjectAccessor::GetObjectInWorld<Transport>(uint32 mapid, float x, float y, uint64 guid, Transport* /*fake*/);
