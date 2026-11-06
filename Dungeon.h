#pragma once
#include "Types.h"
#include <functional>
#include <iosfwd>
#include <optional>
#include <vector>

enum class Tile : char { Wall, Floor, Stairs };

struct Room {
    int x, y, w, h;
    Vec2 center() const { return {x + w / 2, y + h / 2}; }
    bool intersects(const Room& o, int margin) const {
        return x - margin < o.x + o.w && x + w + margin > o.x &&
               y - margin < o.y + o.h && y + h + margin > o.y;
    }
};

// The map: procedural generation, line of sight / fog of war, and BFS pathfinding.
class Dungeon {
public:
    explicit Dungeon(int w = 60, int h = 18);

    // Non-overlapping rooms joined by L-shaped corridors (always connected).
    void generate(int targetRooms, bool withStairs);

    int width() const { return w_; }
    int height() const { return h_; }
    bool inBounds(Vec2 p) const { return p.x >= 0 && p.y >= 0 && p.x < w_ && p.y < h_; }
    Tile tileAt(Vec2 p) const { return inBounds(p) ? tiles_[idx(p)] : Tile::Wall; }
    bool walkable(Vec2 p) const { return tileAt(p) != Tile::Wall; }
    const std::vector<Room>& rooms() const { return rooms_; }
    bool hasStairs() const { return stairs_.x >= 0; }
    Vec2 stairs() const { return stairs_; }

    // Bresenham line of sight (walls block; the endpoints themselves don't).
    bool los(Vec2 a, Vec2 b) const;
    void computeFov(Vec2 origin, int radius);
    bool visible(Vec2 p) const { return inBounds(p) && visible_[idx(p)]; }
    bool explored(Vec2 p) const { return inBounds(p) && explored_[idx(p)]; }

    // First step of a shortest 4-directional path, or nullopt if unreachable /
    // farther than maxDist. 'blocked' lets callers treat tiles (other monsters) as walls.
    std::optional<Vec2> nextStep(Vec2 from, Vec2 to, int maxDist,
                                 const std::function<bool(Vec2)>& blocked = {}) const;

    // Number of steps on the shortest walkable path, or -1 if unreachable / beyond maxDist.
    int pathLength(Vec2 from, Vec2 to, int maxDist) const;

    void save(std::ostream& os) const;
    bool load(std::istream& is);

private:
    int w_, h_;
    std::vector<Tile> tiles_;
    std::vector<char> explored_, visible_;
    std::vector<Room> rooms_;
    Vec2 stairs_{-1, -1};

    int idx(Vec2 p) const { return p.y * w_ + p.x; }
    void carveRoom(const Room& r);
    void carveLine(Vec2 a, Vec2 b);
    void carveCorridor(Vec2 a, Vec2 b);
};
