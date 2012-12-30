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

#ifndef TRINITY_OBJECTGRIDLOADER_H
#define TRINITY_OBJECTGRIDLOADER_H

#include "TypeList.h"
#include "Define.h"
#include "GridLoader.h"
#include "GridDefines.h"
#include "Cell.h"

class ObjectWorldLoader;

class ObjectGridLoader
{
    friend class ObjectWorldLoader;

    public:
        ObjectGridLoader(NGridTypePtr &grid, MapPtr map, const Cell &cell)
            : i_cell(cell), i_grid(grid), i_map(map), i_gameObjects(0), i_creatures(0), i_corpses (0)
            {}

        void Visit(std::shared_ptr<GameObjectMapType> &m);
        void Visit(std::shared_ptr<CreatureMapType> &m);
        void Visit(std::shared_ptr<CorpseMapType> &) const {}
        void Visit(std::shared_ptr<DynamicObjectMapType>&) const {}

        void LoadN(void);

        template<class T> static void SetObjectCell(std::shared_ptr<T> obj, CellCoord const& cellCoord);

    private:
        Cell i_cell;
        NGridTypePtr &i_grid;
        MapPtr i_map;
        uint32 i_gameObjects;
        uint32 i_creatures;
        uint32 i_corpses;
};

//Stop the creatures before unloading the NGrid
class ObjectGridStoper
{
    public:
        void Visit(std::shared_ptr<CreatureMapType> &m);
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &) {}
};

//Move the foreign creatures back to respawn positions before unloading the NGrid
class ObjectGridEvacuator
{
    public:
        void Visit(std::shared_ptr<CreatureMapType> &m);
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &) {}
};

//Clean up and remove from world
class ObjectGridCleaner
{
    public:
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &);
};

//Delete objects before deleting NGrid
class ObjectGridUnloader
{
    public:
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &m);
};
#endif
