#ifndef _REGULAR_GRID_H
#define _REGULAR_GRID_H


#include <G3D/Ray.h>
#include <G3D/Table.h>
#include <G3D/BoundsTrait.h>
#include <G3D/PositionTrait.h>

#include "Log.h"
#include "Errors.h"
#include "GridDefines.h"

template<class Node>
struct NodeCreator{
    static Node * makeNode(int /*x*/, int /*y*/) { return new Node();}
};

template<class T,
class Node,
class NodeCreatorFunc = NodeCreator<Node>,
    /*class BoundsFunc = BoundsTrait<T>,*/
class PositionFunc = PositionTrait<T>
>
class RegularGrid2D
{
public:

    enum{
        CELL_NUMBER = 64,
        CELLS_MAX_NUMBER = 8,
    };

    #define HGRID_MAP_SIZE  (533.33333f * 64.f)     // shouldn't be changed
    #define CELL_SIZE       float(HGRID_MAP_SIZE/(float)CELL_NUMBER)

    typedef G3D::Table<const T*, Node*> MemberTable;

    MemberTable memberTable;
    Node* nodes[CELL_NUMBER][CELL_NUMBER];

    RegularGrid2D(){
        memset(nodes, 0, sizeof(nodes));
    }

    ~RegularGrid2D(){
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                delete nodes[x][y];
    }

    void insert(const T& value)
    {
        G3D::Vector3 pos;
        PositionFunc::getPosition(value, pos);
        Node& node = getGridFor(pos.x, pos.y);
        node.insert(value);
        memberTable.set(&value, &node);
    }

    void remove(const T& value)
    {
        memberTable[&value]->remove(value);
        // Remove the member
        memberTable.remove(&value);
    }

    void balance()
    {
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                if (Node* n = nodes[x][y])
                    n->balance();
    }

    bool contains(const T& value) const { return memberTable.containsKey(&value); }
    bool empty() const { return memberTable.size() == 0; }
    int size() const { return uint32(memberTable.size()); }

    struct CellArea final
    {
        CellArea() { }
        CellArea(CellCoord low, CellCoord high) : low_bound(low), high_bound(high) {}

        bool operator!() const { return low_bound == high_bound; }

        CellCoord low_bound;
        CellCoord high_bound;
    };

    typedef CoordPair<CELL_NUMBER*CELLS_MAX_NUMBER> CellCoord;

    struct Cell
    {
        int x, y;
        bool operator == (const Cell& c2) const { return x == c2.x && y == c2.y; }
        bool operator = (const Cell& c2) { return x = c2.x; y = c2.y; }

        static Cell ComputeCell(float fx, float fy)
        {
            Cell c = { int(fx * (1.f/CELL_SIZE) + (CELL_NUMBER/2)), int(fy * (1.f/CELL_SIZE) + (CELL_NUMBER/2)) };
            return c;
        }

        static Cell CalculateCell(uint32 x, uint32 y)
        {
            Cell c = { int(x / MAX_NUMBER_OF_CELLS), int(y / CELLS_MAX_NUMBER) };
            return c;
        }

        static CellArea CalculateCellArea(float x, float y, float radius)
        {
            if (radius <= 0.0f)
            {
                CellCoord center = Trinity::ComputeCellCoord(x, y).normalize();
                return CellArea(center, center);
            }

            CellCoord centerX = Trinity::ComputeCellCoord(x - radius, y - radius).normalize();
            CellCoord centerY = Trinity::ComputeCellCoord(x + radius, y + radius).normalize();

            return CellArea(centerX, centerY);
        }

        bool isValid() const { return x >= 0 && x < CELL_NUMBER && y >= 0 && y < CELL_NUMBER;}
    };

    Node& getGridFor(float fx, float fy)
    {
        Cell c = Cell::ComputeCell(fx, fy);
        return getGrid(c.x, c.y);
    }

    Node& getGrid(int x, int y)
    {
        ASSERT(x < CELL_NUMBER && y < CELL_NUMBER);
        if (!nodes[x][y])
            nodes[x][y] = NodeCreatorFunc::makeNode(x, y);
        return *nodes[x][y];
    }

    template<typename RayCallback>
    void intersectRay(const G3D::Ray& ray, RayCallback& intersectCallback, float max_dist)
    {
        intersectRay(ray, intersectCallback, max_dist, ray.origin() + ray.direction() * max_dist);
    }

    template<typename RayCallback>
    void intersectRay(const G3D::Ray& ray, RayCallback& intersectCallback, float& max_dist, const G3D::Vector3& end)
    {
        float maxDist = max_dist;
        Cell cell = Cell::ComputeCell(ray.origin().x, ray.origin().y);
        if (!cell.isValid())
            return;

        if (Node* node = nodes[cell.x][cell.y])
            node->intersectRay(ray, intersectCallback, max_dist);

        if (maxDist > max_dist)
            return;

        CellArea area = Cell::CalculateCellArea(ray.origin().x, ray.origin().y, 250.0f);
        if (!area)
            return;

        for (uint32 x = area.low_bound.x_coord; x <= area.high_bound.x_coord; ++x)
        {
            for (uint32 y = area.low_bound.y_coord; y <= area.high_bound.y_coord; ++y)
            {
                Cell cellAroun = Cell::CalculateCell(x, y);
                if (!cellAroun.isValid() || cellAroun == cell)
                    continue;

                cell = cellAroun;
                if (Node* node = nodes[cellAroun.x][cellAroun.y])
                    node->intersectRay(ray, intersectCallback, max_dist);

                if (maxDist > max_dist)
                    return;
            }
        }
    }

    template<typename IsectCallback>
    void intersectPoint(const G3D::Vector3& point, IsectCallback& intersectCallback)
    {
        Cell cell = Cell::ComputeCell(point.x, point.y);
        if (!cell.isValid())
            return;
        if (Node* node = nodes[cell.x][cell.y])
            node->intersectPoint(point, intersectCallback);
    }

    // Optimized verson of intersectRay function for rays with vertical directions
    template<typename RayCallback>
    void intersectZAllignedRay(const G3D::Ray& ray, RayCallback& intersectCallback, float& max_dist)
    {
        float maxDist = max_dist;
        Cell cell = Cell::ComputeCell(ray.origin().x, ray.origin().y);
        if (!cell.isValid())
            return;

        if (Node* node = nodes[cell.x][cell.y])
            node->intersectRay(ray, intersectCallback, max_dist);

        if (maxDist > max_dist)
            return;

        CellArea area = Cell::CalculateCellArea(ray.origin().x, ray.origin().y, 250.0f);
        if (!area)
            return;

        for (uint32 x = area.low_bound.x_coord; x <= area.high_bound.x_coord; ++x)
        {
            for (uint32 y = area.low_bound.y_coord; y <= area.high_bound.y_coord; ++y)
            {
                Cell cellAroun = Cell::CalculateCell(x, y);
                if (!cellAroun.isValid() || cellAroun == cell)
                    continue;

                cell = cellAroun;
                if (Node* node = nodes[cellAroun.x][cellAroun.y])
                    node->intersectRay(ray, intersectCallback, max_dist);

                if (maxDist > max_dist)
                    return;
            }
        }
    }
};

#undef CELL_SIZE
#undef HGRID_MAP_SIZE

#endif
