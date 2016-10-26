/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#ifndef TRINITY_NGRID_H
#define TRINITY_NGRID_H

/** NGrid is nothing more than a wrapper of the Grid with an NxN cells
 */

#include "Grid.h"
#include "GridDefines.h"
#include "Timer.h"
#include "Util.h"

#define DEFAULT_VISIBILITY_NOTIFY_PERIOD      1000

class GridInfo final
{
public:
    GridInfo()
        : i_timer(0), i_unloadActiveLockCount(0), i_unloadReferenceLock(0)
        , i_unloadExplicitLock(0)
    { }

    explicit GridInfo(time_t expiry, bool unload = true)
        : i_timer(expiry), i_unloadActiveLockCount(0), i_unloadReferenceLock(0)
        , i_unloadExplicitLock(unload ? 0 : 1)
    { }

    const TimeTracker& getTimeTracker() const { return i_timer; }

    bool getUnloadLock() const
    {
        return i_unloadActiveLockCount || i_unloadReferenceLock || i_unloadExplicitLock;
    }

    void setUnloadReferenceLock(bool on)
    {
        i_unloadReferenceLock = on ? 1 : 0;
    }

    void incUnloadActiveLock()
    {
        ++i_unloadActiveLockCount;
    }

    void decUnloadActiveLock()
    {
        if (i_unloadActiveLockCount)
            --i_unloadActiveLockCount;
    }

    void setTimer(const TimeTracker& pTimer)
    {
        i_timer = pTimer;
    }

    void ResetTimeTracker(time_t interval)
    {
        i_timer.Reset(interval);
    }

    void UpdateTimeTracker(time_t diff)
    {
        i_timer.Update(diff);
    }

private:
    TimeTracker i_timer;

    uint16 i_unloadActiveLockCount;                     // lock from active object spawn points (prevent clone loading)
    uint8 i_unloadReferenceLock;                        // lock from instance map copy
    uint8 i_unloadExplicitLock;                         // explicit config setting
};

enum GridState
{
    GRID_STATE_INVALID,
    GRID_STATE_ACTIVE,
    GRID_STATE_IDLE,
    GRID_STATE_REMOVAL,
    MAX_GRID_STATE
};

class NGrid final
{
public:
    NGrid(int32 x, int32 y, time_t expiry, bool unload = true)
        : i_GridInfo(expiry, unload), i_x(x), i_y(y)
        , i_cellstate(GRID_STATE_INVALID), i_GridObjectDataLoaded(false)
    { }

    Grid & GetGrid(const uint32 x, const uint32 y)
    {
        return i_cells[x * MAX_NUMBER_OF_CELLS + y];
    }

    Grid const & GetGrid(const uint32 x, const uint32 y) const
    {
        return i_cells[x * MAX_NUMBER_OF_CELLS + y];
    }

    GridState GetGridState(void) const { return i_cellstate; }
    void SetGridState(GridState s) { i_cellstate = s; }

    int32 getX() const { return i_x; }
    int32 getY() const { return i_y; }

    bool isGridObjectDataLoaded() const { return i_GridObjectDataLoaded; }
    void setGridObjectDataLoaded(bool pLoaded) { i_GridObjectDataLoaded = pLoaded; }

    GridInfo & getGridInfo() { return i_GridInfo; }
    const TimeTracker& getTimeTracker() const { return i_GridInfo.getTimeTracker(); }
    bool getUnloadLock() const { return i_GridInfo.getUnloadLock(); }
    void setUnloadReferenceLock(bool on) { i_GridInfo.setUnloadReferenceLock(on); }
    void incUnloadActiveLock() { i_GridInfo.incUnloadActiveLock(); }
    void decUnloadActiveLock() { i_GridInfo.decUnloadActiveLock(); }
    void ResetTimeTracker(time_t interval) { i_GridInfo.ResetTimeTracker(interval); }
    void UpdateTimeTracker(time_t diff) { i_GridInfo.UpdateTimeTracker(diff); }

    // Visit all Grids (cells) in NGrid (grid)
    template <typename Visitor>
    void VisitAllGrids(Visitor &&visitor)
    {
        for (auto &cell : i_cells)
            cell.Visit(visitor);
    }

    // Visit a single Grid (cell) in NGrid (grid)
    template <typename Visitor>
    void VisitGrid(const uint32 x, const uint32 y, Visitor &&visitor)
    {
        GetGrid(x, y).Visit(visitor);
    }

    template <typename T>
    std::size_t GetWorldObjectCountInNGrid() const
    {
        std::size_t count = 0;
        for (auto &cell : i_cells)
            count += cell.template GetWorldObjectCountInGrid<T>();
        return count;
    }

private:
    GridInfo i_GridInfo;
    int32 i_x;
    int32 i_y;
    GridState i_cellstate;
    Grid i_cells[MAX_NUMBER_OF_CELLS * MAX_NUMBER_OF_CELLS];
    bool i_GridObjectDataLoaded;
};

#endif
