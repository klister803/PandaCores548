/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#ifndef _MMAP_MANAGER_H
#define _MMAP_MANAGER_H

#include "Define.h"
#include "DetourAlloc.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "MapDefines.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//  move map related classes
namespace MMAP
{
    typedef std::unordered_map<uint32, dtTileRef> MMapTileSet;
    typedef std::unordered_map<uint32, dtNavMeshQuery*> NavMeshQuerySet;

    class MMapData
    {
    public:
        MMapData(dtNavMesh* mesh, uint32 mapId);
        ~MMapData();

        dtNavMesh* GetNavMesh();

        // we have to use single dtNavMeshQuery for every instance, since those are not thread safe
        NavMeshQuerySet navMeshQueries;     // instanceId to query

        dtNavMesh* navMesh;
        MMapTileSet loadedTileRefs;

    private:
        uint32 _mapId;
    };


    typedef std::unordered_map<uint32, MMapData*> MMapDataSet;
    typedef std::unordered_set<uint32> GoDataSet;

    // singleton class
    // holds all all access to mmap loading unloading and meshes
    class MMapManager
    {
        public:
            MMapManager() : loadedTiles(0), thread_safe_environment(true) {}
            ~MMapManager();

            void InitializeThreadUnsafe(std::vector<uint32> const& mapIds);
            bool loadMap(const std::string& basePath, uint32 mapId, int32 x, int32 y);
            bool unloadMap(uint32 mapId, int32 x, int32 y);
            bool unloadMap(uint32 mapId);
            bool unloadMapInstance(uint32 mapId, uint32 instanceId);
            bool loadGameObject(uint32 displayId);

            // the returned [dtNavMeshQuery const*] is NOT threadsafe
            dtNavMeshQuery const* GetNavMeshQuery(uint32 mapId, uint32 instanceId);
            dtNavMeshQuery const* GetModelNavMeshQuery(uint32 displayId, uint32 instanceId);
            dtNavMesh const* GetNavMesh(uint32 mapId);

            uint32 getLoadedTilesCount() const { return loadedTiles; }
            uint32 getLoadedMapsCount() const { return uint32(loadedMMaps.size()); }

        private:
            bool loadMapData(uint32 mapId);
            uint32 packTileID(int32 x, int32 y);

            MMapDataSet::const_iterator GetMMapData(uint32 mapId) const;
            MMapDataSet loadedMMaps;
            uint32 loadedTiles;
            bool thread_safe_environment;
            MMapDataSet loadedModels;
            GoDataSet errorModels;
    };
}

#endif