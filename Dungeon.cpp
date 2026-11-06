#include "Dungeon.h"
#include "Rng.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <string>

Dungeon::Dungeon(int w, int h)
    : w_(w), h_(h), tiles_(static_cast<size_t>(w * h), Tile::Wall),
      explored_(static_cast<size_t>(w * h), 0), visible_(static_cast<size_t>(w * h), 0) {}

void Dungeon::carveRoom(const Room& r) {
    for (int y = r.y; y < r.y + r.h; ++y)
        for (int x = r.x; x < r.x + r.w; ++x) tiles_[idx({x, y})] = Tile::Floor;
}

void Dungeon::carveLine(Vec2 a, Vec2 b) {       // a and b share an x or a y
    int dx = (b.x > a.x) - (b.x < a.x), dy = (b.y > a.y) - (b.y < a.y);
    Vec2 p = a;
    tiles_[idx(p)] = Tile::Floor;
    while (p != b) {
        p = {p.x + dx, p.y + dy};
        tiles_[idx(p)] = Tile::Floor;
    }
}

void Dungeon::carveCorridor(Vec2 a, Vec2 b) {
    Vec2 corner = Rng::chance(0.5) ? Vec2{b.x, a.y} : Vec2{a.x, b.y};
    carveLine(a, corner);
    carveLine(corner, b);
}

void Dungeon::generate(int targetRooms, bool withStairs) {
    for (int attempt = 0; attempt < 500; ++attempt) {
        tiles_.assign(static_cast<size_t>(w_ * h_), Tile::Wall);
        explored_.assign(static_cast<size_t>(w_ * h_), 0);
        visible_.assign(static_cast<size_t>(w_ * h_), 0);
        rooms_.clear();

        for (int t = 0; t < 300 && static_cast<int>(rooms_.size()) < targetRooms; ++t) {
            int rw = std::min(Rng::range(5, 11), w_ - 3), rh = std::min(Rng::range(4, 6), h_ - 3);
            Room r{Rng::range(1, w_ - rw - 1), Rng::range(1, h_ - rh - 1), rw, rh};
            bool overlaps = std::any_of(rooms_.begin(), rooms_.end(),
                                        [&](const Room& o) { return r.intersects(o, 1); });
            if (overlaps) continue;
            carveRoom(r);
            if (!rooms_.empty()) carveCorridor(rooms_.back().center(), r.center());
            rooms_.push_back(r);
        }
        if (rooms_.size() >= 3) break;
    }

    stairs_ = {-1, -1};
    if (withStairs && !rooms_.empty()) {
        stairs_ = rooms_.back().center();
        tiles_[idx(stairs_)] = Tile::Stairs;
    }
}

bool Dungeon::los(Vec2 a, Vec2 b) const {
    int x0 = a.x, y0 = a.y;
    const int x1 = b.x, y1 = b.y;
    const int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        bool endpoint = (x0 == a.x && y0 == a.y) || (x0 == x1 && y0 == y1);
        if (!endpoint && tileAt({x0, y0}) == Tile::Wall) return false;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return true;
}

void Dungeon::computeFov(Vec2 o, int radius) {
    std::fill(visible_.begin(), visible_.end(), 0);
    for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy > radius * radius) continue;
            Vec2 p{o.x + dx, o.y + dy};
            if (!inBounds(p) || !los(o, p)) continue;
            visible_[idx(p)] = 1;
            explored_[idx(p)] = 1;
        }
}

std::optional<Vec2> Dungeon::nextStep(Vec2 from, Vec2 to, int maxDist,
                                      const std::function<bool(Vec2)>& blocked) const {
    if (from == to || !inBounds(from) || !inBounds(to)) return std::nullopt;
    const int n = w_ * h_, s = idx(from), g = idx(to);
    std::vector<int> prev(static_cast<size_t>(n), -2), dist(static_cast<size_t>(n), 0);
    std::queue<int> q;
    prev[static_cast<size_t>(s)] = -1;
    q.push(s);
    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        if (cur == g) break;
        if (dist[static_cast<size_t>(cur)] >= maxDist) continue;
        Vec2 cp{cur % w_, cur / w_};
        for (Vec2 d : DIRS) {
            Vec2 np = cp + d;
            if (!inBounds(np) || !walkable(np)) continue;
            int ni = idx(np);
            if (prev[static_cast<size_t>(ni)] != -2) continue;
            if (np != to && blocked && blocked(np)) continue;
            prev[static_cast<size_t>(ni)] = cur;
            dist[static_cast<size_t>(ni)] = dist[static_cast<size_t>(cur)] + 1;
            q.push(ni);
        }
    }
    if (prev[static_cast<size_t>(g)] == -2) return std::nullopt;
    int cur = g;
    while (prev[static_cast<size_t>(cur)] != s) cur = prev[static_cast<size_t>(cur)];
    return Vec2{cur % w_, cur / w_};
}

int Dungeon::pathLength(Vec2 from, Vec2 to, int maxDist) const {
    if (!inBounds(from) || !inBounds(to) || !walkable(from) || !walkable(to)) return -1;
    if (from == to) return 0;
    std::vector<int> dist(static_cast<size_t>(w_ * h_), -1);
    std::queue<Vec2> q;
    dist[static_cast<size_t>(idx(from))] = 0;
    q.push(from);
    while (!q.empty()) {
        Vec2 c = q.front();
        q.pop();
        int dc = dist[static_cast<size_t>(idx(c))];
        if (dc >= maxDist) continue;
        for (Vec2 d : DIRS) {
            Vec2 n = c + d;
            if (!walkable(n) || dist[static_cast<size_t>(idx(n))] != -1) continue;
            dist[static_cast<size_t>(idx(n))] = dc + 1;
            if (n == to) return dc + 1;
            q.push(n);
        }
    }
    return -1;
}

void Dungeon::save(std::ostream& os) const {
    os << w_ << ' ' << h_ << ' ' << stairs_.x << ' ' << stairs_.y << '\n';
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            Tile t = tiles_[idx({x, y})];
            os << (t == Tile::Wall ? '#' : t == Tile::Stairs ? '>' : '.');
        }
        os << '\n';
    }
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) os << (explored_[idx({x, y})] ? '1' : '0');
        os << '\n';
    }
}

bool Dungeon::load(std::istream& is) {
    int w, h, sx, sy;
    if (!(is >> w >> h >> sx >> sy)) return false;
    if (w <= 0 || h <= 0 || w > 500 || h > 500) return false;
    std::vector<Tile> tiles(static_cast<size_t>(w * h), Tile::Wall);
    std::vector<char> explored(static_cast<size_t>(w * h), 0);
    std::string line;
    for (int y = 0; y < h; ++y) {
        if (!(is >> line) || static_cast<int>(line.size()) != w) return false;
        for (int x = 0; x < w; ++x) {
            char c = line[static_cast<size_t>(x)];
            if (c != '#' && c != '.' && c != '>') return false;
            tiles[static_cast<size_t>(y * w + x)] =
                c == '#' ? Tile::Wall : c == '>' ? Tile::Stairs : Tile::Floor;
        }
    }
    for (int y = 0; y < h; ++y) {
        if (!(is >> line) || static_cast<int>(line.size()) != w) return false;
        for (int x = 0; x < w; ++x) explored[static_cast<size_t>(y * w + x)] = line[static_cast<size_t>(x)] == '1';
    }
    w_ = w; h_ = h;
    tiles_ = std::move(tiles);
    explored_ = std::move(explored);
    visible_.assign(static_cast<size_t>(w * h), 0);
    rooms_.clear();
    stairs_ = {sx, sy};
    return true;
}
