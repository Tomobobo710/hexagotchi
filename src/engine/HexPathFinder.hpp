#ifndef HEX_PATH_FINDER_HPP
#define HEX_PATH_FINDER_HPP

#include "HexWorld.hpp"
#include <vector>
#include <queue>
#include <unordered_set>

// A* pathfinding for hex grid
class HexPathFinder {
public:
    HexPathFinder(HexWorld* world);
    ~HexPathFinder();

    // Find path from start hex to target hex
    // Returns empty vector if no path found
    std::vector<HexCoords> findPath(int startQ, int startR, int targetQ, int targetR);

    // Check if a hex is walkable (not ocean)
    bool isWalkable(int q, int r) const;

private:
    HexWorld* world_;

    // Get hex neighbors in axial coordinates
    std::vector<HexCoords> getNeighbors(int q, int r) const;

    // Heuristic: Manhattan distance for axial hex coords
    int heuristic(int q1, int r1, int q2, int r2) const;
};

#endif
