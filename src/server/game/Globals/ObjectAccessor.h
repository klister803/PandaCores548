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
#include "Object.h"
#include "Player.h"
#include "Transport.h"
#include "SpinLock.hpp"

#include <ting/shared_mutex.hpp>
#include <mutex>
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

static uint32 const INCREMENT_COUNTER = 1000000;

template <class T>
class HashMapHolder
{
    public:

        typedef std::unordered_map<uint64, T*> MapType;
        typedef std::unordered_map<std::string, T*> MapTypeStr;
        typedef std::vector<T*> MapTypeVector;

        typedef ting::shared_mutex LockType;
        typedef std::lock_guard<LockType> WriteGuardType;
        typedef ting::shared_lock<LockType> ReadGuardType;

        static void Insert(T* o)
        {
            volatile uint32 _guidlow = o->GetGUIDLow(); // For debug
            volatile uint32 _sizeV = _size; // For debug
            if (_guidlow >= _size) // If guid buged don`t check it
                return;
            _objectVector[_guidlow] = o;
            {
                WriteGuardType guard(i_lock);
                _objectMap[o->GetGUID()] = o;
                if (o->GetTypeId() == TYPEID_PLAYER)
                {
                    std::string _name = o->GetName();
                    std::transform(_name.begin(), _name.end(), _name.begin(), ::tolower);
                    _objectMapStr[_name] = o;
                }
            }
        }

        static void Remove(T* o)
        {
            volatile uint32 _guidlow = o->GetGUIDLow(); // For debug
            volatile uint32 _sizeV = _size; // For debug
            if (_guidlow >= _size) // If guid buged don`t check it
                return;
            _objectVector[_guidlow] = NULL;
            {
                WriteGuardType guard(i_lock);
                _objectMap.erase(o->GetGUID());
                if (o->GetTypeId() == TYPEID_PLAYER)
                {
                    std::string _name = o->GetName();
                    std::transform(_name.begin(), _name.end(), _name.begin(), ::tolower);
                    _objectMapStr.erase(_name);
                }
            }
        }

        static T* Find(uint64 guid)
        {
            volatile uint32 _guidlow = PAIR64_LOPART(guid); // For debug
            volatile uint32 _sizeV = _size; // For debug
            if (_guidlow >= _size) // If guid buged don`t check it
                return NULL;
            return _objectVector[_guidlow];
        }

        static T* FindStr(std::string name)
        {
            ReadGuardType guard(i_lock);
            typename MapTypeStr::iterator itr = _objectMapStr.find(name);
            return (itr != _objectMapStr.end()) ? itr->second : NULL;
        }

        static void SetSize(uint32 size)
        {
            if (_objectVector.size() < (size + INCREMENT_COUNTER))
            {
                // WriteGuardType guard(i_lockVector);
                _objectVector.resize(size + INCREMENT_COUNTER * 2);
                _size = _objectVector.size();
            }
        }

        static MapType& GetContainer() { return _objectMap; }

        static LockType& GetLock() { return i_lock; }

    private:

        //Non instanceable only static
        HashMapHolder() {}

        static LockType i_lock;
        static LockType i_lockVector;
        static MapType _objectMap;
        static MapTypeStr _objectMapStr;
        static MapTypeVector _objectVector;
        static uint32 _size;
};

class ObjectAccessor
{
    typedef Trinity::SpinLock ObjectLock;
    typedef std::lock_guard<ObjectLock> ObjectGuard;

    typedef ting::shared_mutex CorpseLock;
    typedef std::lock_guard<CorpseLock> CorpseReadGuard;
    typedef ting::shared_lock<CorpseLock> CorpseWriteGuard;

    private:
        ObjectAccessor();
        ~ObjectAccessor();
        ObjectAccessor(const ObjectAccessor&);
        ObjectAccessor& operator=(const ObjectAccessor&);

    public:
        // TODO: override these template functions for each holder type and add assertions

        static ObjectAccessor* instance()
        {
            static ObjectAccessor instance;
            return &instance;
        }

        template<class T> static T* GetObjectInOrOutOfWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        static Unit* GetObjectInOrOutOfWorld(uint64 guid, Unit* /*typeSpecifier*/)
        {
            if (IS_PLAYER_GUID(guid))
                return (Unit*)GetObjectInOrOutOfWorld(guid, (Player*)NULL);

            if (IS_PET_GUID(guid))
                return (Unit*)GetObjectInOrOutOfWorld(guid, (Pet*)NULL);

            return (Unit*)GetObjectInOrOutOfWorld(guid, (Creature*)NULL);
        }

        // returns object if is in world
        template<class T> static T* GetObjectInWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        // Player may be not in world while in ObjectAccessor
        static Player* GetObjectInWorld(uint64 guid, Player* /*typeSpecifier*/)
        {
            Player* player = HashMapHolder<Player>::Find(guid);
            if (player && player->IsInWorld())
                return player;
            return NULL;
        }

        static Unit* GetObjectInWorld(uint64 guid, Unit* /*typeSpecifier*/)
        {
            if (IS_PLAYER_GUID(guid))
                return (Unit*)GetObjectInWorld(guid, (Player*)NULL);

            if (IS_PET_GUID(guid))
                return (Unit*)GetObjectInWorld(guid, (Pet*)NULL);

            return (Unit*)GetObjectInWorld(guid, (Creature*)NULL);
        }

        // returns object if is in map
        template<class T> static T* GetObjectInMap(uint64 guid, Map* map, T* /*typeSpecifier*/)
        {
            ASSERT(map);
            if (T * obj = GetObjectInWorld(guid, (T*)NULL))
                if (obj->GetMap() == map)
                    return obj;
            return NULL;
        }

        template<class T> static T* GetObjectInWorld(uint32 mapid, float x, float y, uint64 guid, T* /*fake*/)
        {
            T* obj = HashMapHolder<T>::Find(guid);
            if (!obj || obj->GetMapId() != mapid)
                return NULL;

            CellCoord p = Trinity::ComputeCellCoord(x, y);
            if (!p.IsCoordValid())
            {
                sLog->outError(LOG_FILTER_GENERAL, "ObjectAccessor::GetObjectInWorld: invalid coordinates supplied X:%f Y:%f grid cell [%u:%u]", x, y, p.x_coord, p.y_coord);
                return NULL;
            }

            CellCoord q = Trinity::ComputeCellCoord(obj->GetPositionX(), obj->GetPositionY());
            if (!q.IsCoordValid())
            {
                sLog->outError(LOG_FILTER_GENERAL, "ObjectAccessor::GetObjecInWorld: object (GUID: %u TypeId: %u) has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), q.x_coord, q.y_coord);
                return NULL;
            }

            int32 dx = int32(p.x_coord) - int32(q.x_coord);
            int32 dy = int32(p.y_coord) - int32(q.y_coord);

            if (dx > -2 && dx < 2 && dy > -2 && dy < 2)
                return obj;
            else
                return NULL;
        }

        // these functions return objects only if in map of specified object
        static WorldObject* GetWorldObject(WorldObject const&, uint64);
        static Object* GetObjectByTypeMask(WorldObject const&, uint64, uint32 typemask);
        static Corpse* GetCorpse(WorldObject const& u, uint64 guid);
        static GameObject* GetGameObject(WorldObject const& u, uint64 guid);
        static DynamicObject* GetDynamicObject(WorldObject const& u, uint64 guid);
        static AreaTrigger* GetAreaTrigger(WorldObject const& u, uint64 guid);
        static Unit* GetUnit(WorldObject const&, uint64 guid);
        static Creature* GetCreature(WorldObject const& u, uint64 guid);
        static Pet* GetPet(WorldObject const&, uint64 guid);
        static Player* GetPlayer(WorldObject const&, uint64 guid);
        static Creature* GetCreatureOrPetOrVehicle(WorldObject const&, uint64);
        static Transport* GetTransport(WorldObject const& u, uint64 guid);

        // these functions return objects if found in whole world
        // ACCESS LIKE THAT IS NOT THREAD SAFE
        static Pet* FindPet(uint64);
        static Player* FindPlayer(uint64);
        static Creature* FindCreature(uint64);
        static Unit* FindUnit(uint64);
        static Player* FindPlayerByName(std::string name);

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

        template<class T> static void AddObject(T* object)
        {
            HashMapHolder<T>::Insert(object);
        }

        template<class T> static void RemoveObject(T* object)
        {
            HashMapHolder<T>::Remove(object);
        }

        static void SaveAllPlayers();
        static void SetGuidSize(HighGuid type, uint32 size);

        //non-static functions
        void AddUpdateObject(Object* obj)
        {
            ObjectGuard guard(i_objectLock);
            i_objects.insert(obj);
        }

        void RemoveUpdateObject(Object* obj)
        {
            ObjectGuard guard(i_objectLock);
            i_objects.erase(obj);
        }

        //Thread safe
        Corpse* GetCorpseForPlayerGUID(uint64 guid);
        void RemoveCorpse(Corpse* corpse);
        void AddCorpse(Corpse* corpse);
        void AddCorpsesToGrid(GridCoord const& gridpair, Grid& grid, Map* map);
        Corpse* ConvertCorpseForPlayer(uint64 player_guid, bool insignia = false);

        //Thread unsafe
        void Update(uint32 diff);
        void RemoveOldCorpses();
        void UnloadAll();

    private:
        static void _buildChangeObjectForPlayer(WorldObject*, UpdateDataMapType&);
        static void _buildPacket(Player*, Object*, UpdateDataMapType&);
        void _update();

        typedef std::unordered_map<uint64, Corpse*> Player2CorpsesMapType;
        typedef std::unordered_map<Player*, UpdateData>::value_type UpdateDataValueType;

        std::set<Object*> i_objects;
        ObjectLock i_objectLock;

        Player2CorpsesMapType i_player2corpse;
        CorpseLock i_corpseLock;
};

#define sObjectAccessor ObjectAccessor::instance()
#endif
