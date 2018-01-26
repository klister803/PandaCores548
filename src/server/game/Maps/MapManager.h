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

#ifndef TRINITY_MAPMANAGER_H
#define TRINITY_MAPMANAGER_H

#include "Define.h"
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include "Common.h"
#include "Map.h"
#include "Object.h"

class Transport;
struct TransportCreatureProto;
class WorldLocation;

class MapManager
{
    typedef std::mutex LockType;
    typedef std::lock_guard<LockType> GuardType;

    typedef std::unordered_map<uint32, Map*> MapMapType;
    typedef std::vector<bool> InstanceIds;

    MapManager();
    ~MapManager();

    MapManager(const MapManager &);
    MapManager& operator=(const MapManager &);

    public:
        static MapManager* instance()
        {
            static MapManager instance;
            return &instance;
        }

        Map* CreateBaseMap(uint32 mapId);
        Map* FindBaseNonInstanceMap(uint32 mapId) const;
        Map* CreateMap(uint32 mapId, Player* player);
        Map* FindMap(uint32 mapId, uint32 instanceId) const;

        uint32 GetAreaId(uint32 mapid, float x, float y, float z) const
        {
            Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
            return m->GetAreaId(x, y, z);
        }
        uint32 GetAreaId(uint32 mapid, Position const& pos) const { return GetAreaId(mapid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
        uint32 GetAreaId(WorldLocation const& loc) const { return GetAreaId(loc.GetMapId(), loc); }
        uint32 GetZoneId(uint32 mapid, float x, float y, float z) const
        {
            Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
            return m->GetZoneId(x, y, z);
        }
        uint32 GetZoneId(uint32 mapid, Position const& pos) const { return GetZoneId(mapid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
        uint32 GetZoneId(WorldLocation const& loc) const { return GetZoneId(loc.GetMapId(), loc); }
        void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, uint32 mapid, float x, float y, float z) const
        {
            Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
            m->GetZoneAndAreaId(zoneid, areaid, x, y, z);
        }
        void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, uint32 mapid, Position const& pos) const { GetZoneAndAreaId(zoneid, areaid, mapid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
        void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, WorldLocation const& loc) const { GetZoneAndAreaId(zoneid, areaid, loc.GetMapId(), loc); }

        void Initialize(void);
        void Update(uint32);

        void SetGridCleanUpDelay(uint32 t)
        {
            if (t < MIN_GRID_DELAY)
                i_gridCleanUpDelay = MIN_GRID_DELAY;
            else
                i_gridCleanUpDelay = t;
        }

        void SetMapUpdateInterval(uint32 t)
        {
            if (t < MIN_MAP_UPDATE_DELAY)
                t = MIN_MAP_UPDATE_DELAY;

            i_timer.SetInterval(t);
            i_timer.Reset();
        }

        //void LoadGrid(int mapid, int instId, float x, float y, const WorldObject* obj, bool no_unload = false);
        void UnloadAll();

        static bool ExistMapAndVMap(uint32 mapid, float x, float y);
        static bool IsValidMAP(uint32 mapid, bool startUp);

        static bool IsValidMapCoord(uint32 mapid, float x, float y)
        {
            return IsValidMAP(mapid, false) && Trinity::IsValidMapCoord(x, y);
        }

        static bool IsValidMapCoord(uint32 mapid, float x, float y, float z)
        {
            return IsValidMAP(mapid, false) && Trinity::IsValidMapCoord(x, y, z);
        }

        static bool IsValidMapCoord(uint32 mapid, float x, float y, float z, float o)
        {
            return IsValidMAP(mapid, false) && Trinity::IsValidMapCoord(x, y, z, o);
        }

        static bool IsValidMapCoord(WorldLocation const& loc)
        {
            return IsValidMapCoord(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation());
        }

        void DoDelayedMovesAndRemoves();

        void LoadTransports();
        void LoadTransportNPCs();

        typedef std::set<Transport*> TransportSet;
        TransportSet m_Transports;

        typedef std::map<uint32, TransportSet> TransportMap;
        TransportMap m_TransportsByMap;
        TransportMap m_TransportsByInstanceIdMap;

        bool CanPlayerEnter(uint32 mapid, Player* player, bool loginCheck = false);
        void InitializeVisibilityDistanceInfo();

        /* statistics */
        uint32 GetNumInstances();
        uint32 GetNumPlayersInInstances();

        // Instance ID management
        void InitInstanceIds();
        uint32 GenerateInstanceId();
        void RegisterInstanceId(uint32 instanceId);
        void FreeInstanceId(uint32 instanceId);

        uint32 GetNextInstanceId() const { return _nextInstanceId; };
        void SetNextInstanceId(uint32 nextInstanceId) { _nextInstanceId = nextInstanceId; };

        uint32 GetLoadedGrids();

    private:

        Map* FindBaseMap(uint32 mapId) const
        {
            MapMapType::const_iterator iter = i_maps.find(mapId);
            return (iter == i_maps.end() ? NULL : iter->second);
        }

        LockType i_lock;
        uint32 i_gridCleanUpDelay;
        MapMapType i_maps;
        IntervalTimer i_timer;

        InstanceIds _instanceIds;
        uint32 _nextInstanceId;
};
#define sMapMgr MapManager::instance()
#endif
