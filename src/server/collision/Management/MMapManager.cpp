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

#include "MMapManager.h"
#include "Errors.h"
#include "Log.h"
#include "Config.h"
#include "MMapFactory.h"
#include "StringFormat.h"
#include "World.h"

namespace MMAP
{
    static char const* const MAP_FILE_NAME_FORMAT = "%s/mmaps/%04i.mmap";
    static char const* const TILE_FILE_NAME_FORMAT = "%s/mmaps/%04i%02i%02i.mmtile";

    // ######################## MMapManager ########################
    MMapManager::~MMapManager()
    {
        for (MMapDataSet::iterator i = loadedMMaps.begin(); i != loadedMMaps.end(); ++i)
            delete i->second;

        // by now we should not have maps loaded
        // if we had, tiles in MMapData->mmapLoadedTiles, their actual data is lost!
    }

    void MMapManager::InitializeThreadUnsafe(std::vector<uint32> const& mapIds)
    {
        // the caller must pass the list of all mapIds that will be used in the MMapManager lifetime
        for (uint32 const& mapId : mapIds)
            loadedMMaps.insert(MMapDataSet::value_type(mapId, nullptr));

        thread_safe_environment = false;
    }

    MMapDataSet::const_iterator MMapManager::GetMMapData(uint32 mapId) const
    {
        // return the iterator if found or end() if not found/NULL
        MMapDataSet::const_iterator itr = loadedMMaps.find(mapId);
        if (itr != loadedMMaps.cend() && !itr->second)
            itr = loadedMMaps.cend();

        return itr;
    }

    bool MMapManager::loadMapData(uint32 mapId)
    {
        // we already have this map loaded?
        MMapDataSet::iterator itr = loadedMMaps.find(mapId);
        if (itr != loadedMMaps.end())
        {
            if (itr->second)
                return true;
        }
        else
        {
            if (thread_safe_environment)
                itr = loadedMMaps.insert(MMapDataSet::value_type(mapId, nullptr)).first;
            else
                ASSERT(false, "Invalid mapId %u passed to MMapManager after startup in thread unsafe environment", mapId);
        }

        // load and init dtNavMesh - read parameters from file
        std::string fileName = Trinity::StringFormat(MAP_FILE_NAME_FORMAT, ConfigMgr::GetStringDefault("DataDir", ".").c_str(), mapId);
        FILE* file = fopen(fileName.c_str(), "rb");
        if (!file)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMapData: Error: Could not open mmap file '%s'", fileName.c_str());
            return false;
        }

        dtNavMeshParams params;
        uint32 count = uint32(fread(&params, sizeof(dtNavMeshParams), 1, file));
        fclose(file);
        if (count != 1)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMapData: Error: Could not read params from file '%s'", fileName.c_str());
            return false;
        }

        dtNavMesh* mesh = dtAllocNavMesh();
        ASSERT(mesh);
        if (dtStatusFailed(mesh->init(&params)))
        {
            dtFreeNavMesh(mesh);
            TC_LOG_DEBUG("maps", "MMAP:loadMapData: Failed to initialize dtNavMesh for mmap %04u from file %s", mapId, fileName.c_str());
            return false;
        }

        TC_LOG_DEBUG("maps", "MMAP:loadMapData: Loaded %04i.mmap", mapId);

        // store inside our map list
        MMapData* mmap_data = new MMapData(mesh, mapId);

        itr->second = mmap_data;
        return true;
    }

    uint32 MMapManager::packTileID(int32 x, int32 y)
    {
        return uint32(x << 16 | y);
    }

    bool MMapManager::loadMap(const std::string& /*basePath*/, uint32 mapId, int32 x, int32 y)
    {
        // make sure the mmap is loaded and ready to load tiles
        if (!loadMapData(mapId))
            return false;

        // get this mmap data
        MMapData* mmap = loadedMMaps[mapId];
        ASSERT(mmap->navMesh);

        // check if we already have this tile loaded
        uint32 packedGridPos = packTileID(x, y);
        if (mmap->loadedTileRefs.find(packedGridPos) != mmap->loadedTileRefs.end())
            return false;

        // load this tile :: mmaps/MMMMXXYY.mmtile
        std::string fileName = Trinity::StringFormat(TILE_FILE_NAME_FORMAT, ConfigMgr::GetStringDefault("DataDir", ".").c_str(), mapId, x, y);
        FILE* file = fopen(fileName.c_str(), "rb");
        if (!file)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMap: Could not open mmtile file '%s'", fileName.c_str());
            return false;
        }

        // read header
        MmapTileHeader fileHeader;
        if (fread(&fileHeader, sizeof(MmapTileHeader), 1, file) != 1 || fileHeader.mmapMagic != MMAP_MAGIC)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMap: Bad header in mmap %04u%02i%02i.mmtile", mapId, x, y);
            fclose(file);
            return false;
        }

        if (fileHeader.mmapVersion != MMAP_VERSION)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMap: %04u%02i%02i.mmtile was built with generator v%i, expected v%i",
                mapId, x, y, fileHeader.mmapVersion, MMAP_VERSION);
            fclose(file);
            return false;
        }

        long pos = ftell(file);
        fseek(file, 0, SEEK_END);
        if (int64(fileHeader.size) > ftell(file) - pos)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMap: %04u%02i%02i.mmtile has corrupted data size", mapId, x, y);
            fclose(file);
            return false;
        }

        fseek(file, pos, SEEK_SET);

        unsigned char* data = (unsigned char*)dtAlloc(fileHeader.size, DT_ALLOC_PERM);
        ASSERT(data);

        size_t result = fread(data, fileHeader.size, 1, file);
        if (!result)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadMap: Bad header or data in mmap %04u%02i%02i.mmtile", mapId, x, y);
            fclose(file);
            return false;
        }

        fclose(file);

        dtMeshHeader* header = (dtMeshHeader*)data;
        dtTileRef tileRef = 0;

        // memory allocated for data is now managed by detour, and will be deallocated when the tile is removed
        if (dtStatusSucceed(mmap->navMesh->addTile(data, fileHeader.size, 0, 0, &tileRef)))
        {
            mmap->loadedTileRefs.insert(std::pair<uint32, dtTileRef>(packedGridPos, tileRef));
            ++loadedTiles;
            TC_LOG_DEBUG("maps", "MMAP:loadMap: Loaded mmtile %04i[%02i, %02i] into %04i[%02i, %02i]", mapId, x, y, mapId, header->x, header->y);
            return true;
        }

        TC_LOG_DEBUG("maps", "MMAP:loadMap: Could not load %04u%02i%02i.mmtile into navmesh", mapId, x, y);
        dtFree(data);
        return false;
    }

    bool MMapManager::unloadMap(uint32 mapId, int32 x, int32 y)
    {
        // check if we have this map loaded
        MMapDataSet::const_iterator itr = GetMMapData(mapId);
        if (itr == loadedMMaps.end())
        {
            // file may not exist, therefore not loaded
            TC_LOG_DEBUG("maps", "MMAP:unloadMap: Asked to unload not loaded navmesh map. %04u%02i%02i.mmtile", mapId, x, y);
            return false;
        }

        MMapData* mmap = itr->second;

        // check if we have this tile loaded
        uint32 packedGridPos = packTileID(x, y);
        if (mmap->loadedTileRefs.find(packedGridPos) == mmap->loadedTileRefs.end())
        {
            // file may not exist, therefore not loaded
            TC_LOG_DEBUG("maps", "MMAP:unloadMap: Asked to unload not loaded navmesh tile. %04u%02i%02i.mmtile", mapId, x, y);
            return false;
        }

        dtTileRef tileRef = mmap->loadedTileRefs[packedGridPos];

        // unload, and mark as non loaded
        unsigned char* data = NULL;
        if (dtStatusFailed(mmap->navMesh->removeTile(tileRef, &data, NULL)))
        {
            // this is technically a memory leak
            // if the grid is later reloaded, dtNavMesh::addTile will return error but no extra memory is used
            // we cannot recover from this error - assert out
            TC_LOG_DEBUG("maps", "MMAP:unloadMap: Could not unload %04u%02i%02i.mmtile from navmesh", mapId, x, y);
            std::abort();
        }
        else
        {
            mmap->loadedTileRefs.erase(packedGridPos);
            --loadedTiles;
            TC_LOG_DEBUG("maps", "MMAP:unloadMap: Unloaded mmtile %04i[%02i, %02i] from %04i", mapId, x, y, mapId);

            dtFree(data);
            return true;
        }

        return false;
    }

    bool MMapManager::unloadMap(uint32 mapId)
    {
        MMapDataSet::iterator itr = loadedMMaps.find(mapId);
        if (itr == loadedMMaps.end() || !itr->second)
        {
            // file may not exist, therefore not loaded
            TC_LOG_DEBUG("maps", "MMAP:unloadMap: Asked to unload not loaded navmesh map %04u", mapId);
            return false;
        }

        // unload all tiles from given map
        MMapData* mmap = itr->second;
        for (MMapTileSet::iterator i = mmap->loadedTileRefs.begin(); i != mmap->loadedTileRefs.end(); ++i)
        {
            uint32 x = (i->first >> 16);
            uint32 y = (i->first & 0x0000FFFF);
            unsigned char* data = NULL;
            if (dtStatusFailed(mmap->navMesh->removeTile(i->second, &data, NULL)))
                TC_LOG_DEBUG("maps", "MMAP:unloadMap: Could not unload %04u%02i%02i.mmtile from navmesh", mapId, x, y);
            else
            {
                dtFree(data);
                --loadedTiles;
                TC_LOG_DEBUG("maps", "MMAP:unloadMap: Unloaded mmtile %04i[%02i, %02i] from %04i", mapId, x, y, mapId);
            }
        }

        delete mmap;
        itr->second = nullptr;
        TC_LOG_DEBUG("maps", "MMAP:unloadMap: Unloaded %04i.mmap", mapId);

        return true;
    }

    bool MMapManager::unloadMapInstance(uint32 mapId, uint32 instanceId)
    {
        // check if we have this map loaded
        MMapDataSet::const_iterator itr = GetMMapData(mapId);
        if (itr == loadedMMaps.end())
        {
            // file may not exist, therefore not loaded
            TC_LOG_DEBUG("maps", "MMAP:unloadMapInstance: Asked to unload not loaded navmesh map %04u", mapId);
            return false;
        }

        MMapData* mmap = itr->second;
        if (mmap->navMeshQueries.find(instanceId) == mmap->navMeshQueries.end())
        {
            TC_LOG_DEBUG("maps", "MMAP:unloadMapInstance: Asked to unload not loaded dtNavMeshQuery mapId %04u instanceId %u", mapId, instanceId);
            return false;
        }

        dtNavMeshQuery* query = mmap->navMeshQueries[instanceId];

        dtFreeNavMeshQuery(query);
        mmap->navMeshQueries.erase(instanceId);
        TC_LOG_DEBUG("maps", "MMAP:unloadMapInstance: Unloaded mapId %04u instanceId %u", mapId, instanceId);

        return true;
    }

    dtNavMesh const* MMapManager::GetNavMesh(uint32 mapId)
    {
        MMapDataSet::const_iterator itr = GetMMapData(mapId);
        if (itr == loadedMMaps.end())
            return NULL;

        return itr->second->GetNavMesh();
    }

    dtNavMeshQuery const* MMapManager::GetNavMeshQuery(uint32 mapId, uint32 instanceId)
    {
        MMapDataSet::const_iterator itr = GetMMapData(mapId);
        if (itr == loadedMMaps.end())
            return NULL;

        MMapData* mmap = itr->second;
        if (mmap->navMeshQueries.find(instanceId) == mmap->navMeshQueries.end())
        {
            // allocate mesh query
            dtNavMeshQuery* query = dtAllocNavMeshQuery();
            ASSERT(query);
            if (dtStatusFailed(query->init(mmap->GetNavMesh(), 1024)))
            {
                dtFreeNavMeshQuery(query);
                TC_LOG_DEBUG("maps", "MMAP:GetNavMeshQuery: Failed to initialize dtNavMeshQuery for mapId %04u instanceId %u", mapId, instanceId);
                return NULL;
            }

            TC_LOG_DEBUG("maps", "MMAP:GetNavMeshQuery: created dtNavMeshQuery for mapId %04u instanceId %u", mapId, instanceId);
            mmap->navMeshQueries.insert(std::pair<uint32, dtNavMeshQuery*>(instanceId, query));
        }

        return mmap->navMeshQueries[instanceId];
    }

    bool MMapManager::loadGameObject(uint32 displayId)
    {
        // we already have this map loaded?
        if (loadedModels.find(displayId) != loadedModels.end())
            return true;

        if (errorModels.find(displayId) != errorModels.end())
            return false;

        // load and init dtNavMesh - read parameters from file
        uint32 pathLen = sWorld->GetDataPath().length() + strlen("mmaps/go00000.mmap") + 1;
        char *fileName = new char[pathLen];
        snprintf(fileName, pathLen, (sWorld->GetDataPath() + "mmaps/go%05i.mmap").c_str(), displayId);

        FILE* file = fopen(fileName, "rb");
        if (!file)
        {
            errorModels.insert(displayId);
            // TC_LOG_DEBUG("maps", "MMAP:loadGameObject: Error: Could not open mmap file %s", fileName);
            delete [] fileName;
            return false;
        }

        MmapTileHeader fileHeader;
        fread(&fileHeader, sizeof(MmapTileHeader), 1, file);

        if (fileHeader.mmapMagic != MMAP_MAGIC)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadGameObject: Bad header in mmap %s", fileName);
            fclose(file);
            delete [] fileName;
            return false;
        }

        if (fileHeader.mmapVersion != MMAP_VERSION)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadGameObject: %s was built with generator v%i, expected v%i", fileName, fileHeader.mmapVersion, MMAP_VERSION);
            fclose(file);
            delete [] fileName;
            return false;
        }
        unsigned char* data = (unsigned char*)dtAlloc(fileHeader.size, DT_ALLOC_PERM);
        ASSERT(data);

        size_t result = fread(data, fileHeader.size, 1, file);
        if (!result)
        {
            TC_LOG_DEBUG("maps", "MMAP:loadGameObject: Bad header or data in mmap %s", fileName);
            fclose(file);
            delete [] fileName;
            return false;
        }

        fclose(file);

        dtNavMesh* mesh = dtAllocNavMesh();
        ASSERT(mesh);
        dtStatus r = mesh->init(data, fileHeader.size, DT_TILE_FREE_DATA);
        if (dtStatusFailed(r))
        {
            dtFreeNavMesh(mesh);
            TC_LOG_DEBUG("maps", "MMAP:loadGameObject: Failed to initialize dtNavMesh from file %s. Result 0x%x.", fileName, r);
            delete [] fileName;
            return false;
        }
        TC_LOG_DEBUG("maps", "MMAP:loadGameObject: Loaded file %s [size=%u]", fileName, fileHeader.size);
        delete [] fileName;

        MMapData* mmap_data = new MMapData(mesh, displayId);
        loadedModels.insert(std::pair<uint32, MMapData*>(displayId, mmap_data));
        return true;
    }

    dtNavMeshQuery const* MMapManager::GetModelNavMeshQuery(uint32 displayId, uint32 instanceId)
    {
        if (loadedModels.find(displayId) == loadedModels.end())
            return NULL;

        MMapData* mmap = loadedModels[displayId];
        if (mmap->navMeshQueries.find(instanceId) == mmap->navMeshQueries.end())
        {
            // allocate mesh query
            dtNavMeshQuery* query = dtAllocNavMeshQuery();
            ASSERT(query);
            if (dtStatusFailed(query->init(mmap->GetNavMesh(), 2048)))
            {
                dtFreeNavMeshQuery(query);
                TC_LOG_DEBUG("maps", "MMAP:GetNavMeshQuery: Failed to initialize dtNavMeshQuery for displayid %04u instanceId %u", displayId, instanceId);
                return NULL;
            }

            TC_LOG_DEBUG("maps", "MMAP:GetNavMeshQuery: created dtNavMeshQuery for displayid %04u instanceId %u", displayId, instanceId);
            mmap->navMeshQueries.insert(std::pair<uint32, dtNavMeshQuery*>(instanceId, query));
        }

        return mmap->navMeshQueries[instanceId];
    }

    MMapData::MMapData(dtNavMesh* mesh, uint32 mapId)
    {
        navMesh = mesh;
        _mapId = mapId;
    }

    MMapData::~MMapData()
    {
        for (NavMeshQuerySet::iterator i = navMeshQueries.begin(); i != navMeshQueries.end(); ++i)
            dtFreeNavMeshQuery(i->second);

        dtFreeNavMesh(navMesh);
    }

    dtNavMesh* MMapData::GetNavMesh()
    {
        return navMesh;
    }
}
