#pragma once
#include <cstdlib>

// Integer 2D coordinate used everywhere on the grid.
struct Vec2 {
    int x = 0, y = 0;
    constexpr Vec2() = default;
    constexpr Vec2(int px, int py) : x(px), y(py) {}
    constexpr bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2& o) const { return !(*this == o); }
    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
};

inline int manhattan(Vec2 a, Vec2 b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

inline constexpr Vec2 DIRS[4] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
