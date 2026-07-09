#include "HexPathFinder.hpp"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PathNode {
    HexCoords coords;
    int gCost;  // Cost from start
    int fCost;  // gCost + heuristic
    PathNode* parent;

    PathNode(HexCoords c, int g, int f, PathNode* p)
        : coords(c), gCost(g), fCost(f), parent(p) {}
};

// HexPathFinder implementation

HexPathFinder::HexPathFinder(HexWorld* world) : world_(world) {}

HexPathFinder::~HexPathFinder() {}

bool HexPathFinder::isWalkable(int q, int r) const {
    // Check bounds
    if (q < 0 || q >= world_->getWidth() || r < 0 || r >= world_->getHeight()) {
        return false;
    }

    // Get the tile at this position
    HexTile* tile = world_->getTileAt(q, r);
    if (!tile || !tile->getTileType()) {
        return false;
    }

    // Check biome - ocean is not walkable
    std::string biome = tile->getTileType()->getBiome();
    return biome != "ocean";
}

std::vector<HexCoords> HexPathFinder::getNeighbors(int q, int r) const {
    std::vector<HexCoords> neighbors;
    // odd-r offset (odd rows shoved right, matching toPixel's +0.5*(r&1))
    static const int evenR[6][2] = {{+1,0},{0,-1},{-1,-1},{-1,0},{-1,+1},{0,+1}};
    static const int oddR[6][2]  = {{+1,0},{+1,-1},{0,-1},{-1,0},{0,+1},{+1,+1}};
    const int (*d)[2] = (r & 1) ? oddR : evenR;
    for (int i = 0; i < 6; ++i) {
        int nq = q + d[i][0];
        int nr = r + d[i][1];
        if (nq >= 0 && nq < world_->getWidth() &&
            nr >= 0 && nr < world_->getHeight()) {
            neighbors.emplace_back(nq, nr);
        }
    }
    return neighbors;
}

static inline void oddrToCube(int q, int r, int& x, int& y, int& z) {
    x = q - (r - (r & 1)) / 2;
    z = r;
    y = -x - z;
}

int HexPathFinder::heuristic(int q1, int r1, int q2, int r2) const {
    int ax, ay, az, bx, by, bz;
    oddrToCube(q1, r1, ax, ay, az);
    oddrToCube(q2, r2, bx, by, bz);
    return (std::abs(ax - bx) + std::abs(ay - by) + std::abs(az - bz)) / 2;
}

std::vector<HexCoords> HexPathFinder::findPath(int startQ, int startR, int targetQ, int targetR) {
    std::vector<HexCoords> path;
    if (startQ == targetQ && startR == targetR) return path;
    if (!isWalkable(targetQ, targetR)) return path;

    auto nodeHash = [](int q, int r) { return q * 10000 + r; };

    std::unordered_set<int> closedSet;
    std::unordered_map<int, PathNode*> best;   // encoded coord -> best-known node
    std::vector<PathNode*> allocated;          // sole owner of every node

    auto cmp = [](PathNode* a, PathNode* b) { return a->fCost > b->fCost; };
    std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(cmp)> openSet(cmp);

    PathNode* startNode = new PathNode(HexCoords(startQ, startR), 0,
                                       heuristic(startQ, startR, targetQ, targetR), nullptr);
    allocated.push_back(startNode);
    openSet.push(startNode);
    best[nodeHash(startQ, startR)] = startNode;

    while (!openSet.empty()) {
        PathNode* current = openSet.top();
        openSet.pop();

        int q = current->coords.q, r = current->coords.r;
        int h = nodeHash(q, r);

        if (closedSet.count(h)) continue;                       // stale duplicate
        auto bestIt = best.find(h);
        if (bestIt != best.end() && bestIt->second != current) continue;  // superseded

        if (q == targetQ && r == targetR) {
            for (PathNode* t = current; t->parent != nullptr; t = t->parent)
                path.insert(path.begin(), t->coords);
            break;
        }

        closedSet.insert(h);

        for (const auto& nb : getNeighbors(q, r)) {
            int nq = nb.q, nr = nb.r, nh = nodeHash(nq, nr);
            if (closedSet.count(nh)) continue;
            if (!isWalkable(nq, nr)) continue;

            int gCost = current->gCost + 1;
            auto it = best.find(nh);
            if (it == best.end() || gCost < it->second->gCost) {
                PathNode* node = new PathNode(HexCoords(nq, nr), gCost,
                                              gCost + heuristic(nq, nr, targetQ, targetR),
                                              current);
                allocated.push_back(node);   // never freed mid-loop
                openSet.push(node);
                best[nh] = node;             // supersede; old stays but is now stale
            }
        }
    }

    for (PathNode* n : allocated) delete n;  // freed exactly once; path holds value copies
    return path;
}
