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

#ifndef TRINITY_OBJECTACCESSOR_H
#define TRINITY_OBJECTACCESSOR_H

#include "Define.h"
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include "UnorderedMap.h"

#include "UpdateData.h"

#include "GridDefines.h"
#ifndef INCLUDES_FOR_SHARED_PTR
#include "Player.h"
#include "Transport.h"
#else
#include "../Entities/Player/Player.h"
#include "../Transport/Transport.h"
#endif
#include "Object.h"

#include <set>

class Creature;
class Corpse;
class Unit;
class GameObject;
class DynamicObject;
class WorldObject;
class Vehicle;
class Map;
class WorldRunnable;
class Transport;

template <class T>
class HashMapHolder
{
    public:

        typedef UNORDERED_MAP<uint64, std::shared_ptr<T>> MapType;
        typedef ACE_RW_Thread_Mutex LockType;

        static void Insert(std::shared_ptr<T> o)
        {
            TRINITY_WRITE_GUARD(LockType, i_lock);
            m_objectMap[o->GetGUID()] = o;
        }

        static void Remove(std::shared_ptr<T> o)
        {
            TRINITY_WRITE_GUARD(LockType, i_lock);
            m_objectMap.erase(o->GetGUID());
        }

        static std::shared_ptr<T> Find(uint64 guid)
        {
            TRINITY_READ_GUARD(LockType, i_lock);
            typename MapType::iterator itr = m_objectMap.find(guid);
            return (itr != m_objectMap.end()) ? itr->second : std::shared_ptr<T>();
        }

        static MapType& GetContainer() { return m_objectMap; }

        static LockType* GetLock() { return &i_lock; }

    private:

        //Non instanceable only static
        HashMapHolder() {}

        static LockType i_lock;
        static MapType  m_objectMap;
};

class ObjectAccessor
{
    friend class ACE_Singleton<ObjectAccessor, ACE_Null_Mutex>;
    private:
        ObjectAccessor();
        ~ObjectAccessor();
        ObjectAccessor(const ObjectAccessor&);
        ObjectAccessor& operator=(const ObjectAccessor&);

    public:
        // TODO: override these template functions for each holder type and add assertions

        template<class T> static std::shared_ptr<T> GetObjectInOrOutOfWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        static UnitPtr GetObjectInOrOutOfWorld(uint64 guid, Unit* /*typeSpecifier*/)
        {
            if (IS_PLAYER_GUID(guid))
                return TO_UNIT(GetObjectInOrOutOfWorld(guid, (Player*)nullptr));

            if (IS_PET_GUID(guid))
                return TO_UNIT(GetObjectInOrOutOfWorld(guid, (Pet*)nullptr));

            return TO_UNIT(GetObjectInOrOutOfWorld(guid, (Creature*)nullptr));
        }

        // returns object if is in world
        template<class T> static std::shared_ptr<T> GetObjectInWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        // Player may be not in world while in ObjectAccessor
        static PlayerPtr GetObjectInWorld(uint64 guid, Player* /*typeSpecifier*/)
        {
            PlayerPtr player = HashMapHolder<Player>::Find(guid);
            if (player != nullptr && player->IsInWorld())
                return player;
            return nullptr;
        }

        static UnitPtr GetObjectInWorld(uint64 guid, Unit* /*typeSpecifier*/)
        {
            if (IS_PLAYER_GUID(guid))
                return TO_UNIT(GetObjectInWorld(guid, (Player*)nullptr));

            if (IS_PET_GUID(guid))
                return TO_UNIT(GetObjectInWorld(guid, (Pet*)nullptr));

            return TO_UNIT(GetObjectInWorld(guid, (Creature*)nullptr));
        }

        // returns object if is in map
        template<class T> static std::shared_ptr<T> GetObjectInMap(uint64 guid, MapPtr map, T* /*typeSpecifier*/)
        {
            ASSERT(map);
            if (std::shared_ptr<T> obj = GetObjectInWorld(guid, (T*)nullptr))
                if (obj->GetMap() == map)
                    return obj;
            return nullptr;
        }

        template<class T> static std::shared_ptr<T> GetObjectInWorld(uint32 mapid, float x, float y, uint64 guid, T* /*fake*/)
        {
            std::shared_ptr<T> obj = HashMapHolder<T>::Find(guid);
            if (!obj || obj->GetMapId() != mapid)
                return nullptr;

            CellCoord p = Trinity::ComputeCellCoord(x, y);
            if (!p.IsCoordValid())
            {
                sLog->outError(LOG_FILTER_GENERAL, "ObjectAccessor::GetObjectInWorld: invalid coordinates supplied X:%f Y:%f grid cell [%u:%u]", x, y, p.x_coord, p.y_coord);
                return nullptr;
            }

            CellCoord q = Trinity::ComputeCellCoord(obj->GetPositionX(), obj->GetPositionY());
            if (!q.IsCoordValid())
            {
                sLog->outError(LOG_FILTER_GENERAL, "ObjectAccessor::GetObjecInWorld: object (GUID: %u TypeId: %u) has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), q.x_coord, q.y_coord);
                return nullptr;
            }

            int32 dx = int32(p.x_coord) - int32(q.x_coord);
            int32 dy = int32(p.y_coord) - int32(q.y_coord);

            if (dx > -2 && dx < 2 && dy > -2 && dy < 2)
                return obj;
            else
                return nullptr;
        }

        // these functions return objects only if in map of specified object
        static WorldObjectPtr GetWorldObject(constWorldObjectPtr&, uint64);
        static ObjectPtr GetObjectByTypeMask(constWorldObjectPtr&, uint64, uint32 typemask);
        static CorpsePtr GetCorpse(constWorldObjectPtr& u, uint64 guid);
        static GameObjectPtr GetGameObject(constWorldObjectPtr& u, uint64 guid);
        static DynamicObjectPtr GetDynamicObject(constWorldObjectPtr& u, uint64 guid);
        static UnitPtr GetUnit(constWorldObjectPtr&, uint64 guid);
        static CreaturePtr GetCreature(constWorldObjectPtr& u, uint64 guid);
        static PetPtr GetPet(constWorldObjectPtr&, uint64 guid);
        static PlayerPtr GetPlayer(constWorldObjectPtr&, uint64 guid);
        static CreaturePtr GetCreatureOrPetOrVehicle(constWorldObjectPtr&, uint64);
        static TransportPtr GetTransport(constWorldObjectPtr& u, uint64 guid);

        // these functions return objects if found in whole world
        // ACCESS LIKE THAT IS NOT THREAD SAFE
        static PetPtr FindPet(uint64);
        static PlayerPtr FindPlayer(uint64);
        static CreaturePtr FindCreature(uint64);
        static UnitPtr FindUnit(uint64);
        static PlayerPtr FindPlayerByName(const char* name);

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<Player>::MapType const& GetPlayers()
        {
            return HashMapHolder<Player>::GetContainer();
        }

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<Creature>::MapType const& GetCreatures()
        {
            return HashMapHolder<Creature>::GetContainer();
        }

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<GameObject>::MapType const& GetGameObjects()
        {
            return HashMapHolder<GameObject>::GetContainer();
        }

        template<class T> static void AddObject(std::shared_ptr<T> object)
        {
            HashMapHolder<T>::Insert(object);
        }

        template<class T> static void RemoveObject(std::shared_ptr<T> object)
        {
            HashMapHolder<T>::Remove(object);
        }

        static void SaveAllPlayers();

        //non-static functions
        void AddUpdateObject(ObjectPtr obj)
        {
            TRINITY_GUARD(ACE_Thread_Mutex, i_objectLock);
            i_objects.insert(obj);
        }

        void RemoveUpdateObject(ObjectPtr obj)
        {
            TRINITY_GUARD(ACE_Thread_Mutex, i_objectLock);
            i_objects.erase(obj);
        }

        //Thread safe
        CorpsePtr GetCorpseForPlayerGUID(uint64 guid);
        void RemoveCorpse(CorpsePtr corpse);
        void AddCorpse(CorpsePtr corpse);
        void AddCorpsesToGrid(GridCoord const& gridpair, GridType& grid, MapPtr map);
        CorpsePtr ConvertCorpseForPlayer(uint64 player_guid, bool insignia = false);

        //Thread unsafe
        void Update(uint32 diff);
        void RemoveOldCorpses();
        void UnloadAll();

    private:
        static void _buildChangeObjectForPlayer(WorldObjectPtr, UpdateDataMapType&);
        static void _buildPacket(PlayerPtr, ObjectPtr, UpdateDataMapType&);
        void _update();

        typedef UNORDERED_MAP<uint64, CorpsePtr> Player2CorpsesMapType;
        typedef UNORDERED_MAP<PlayerPtr, UpdateData>::value_type UpdateDataValueType;

        std::set<ObjectPtr> i_objects;
        Player2CorpsesMapType i_player2corpse;

        ACE_Thread_Mutex i_objectLock;
        ACE_RW_Thread_Mutex i_corpseLock;
};

#define sObjectAccessor ACE_Singleton<ObjectAccessor, ACE_Null_Mutex>::instance()
#endif
