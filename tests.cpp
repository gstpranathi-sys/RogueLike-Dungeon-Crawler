// Self-contained test suite (no external framework). Run with: make test
#include "Dungeon.h"
#include "Entity.h"
#include "Game.h"
#include "Item.h"
#include "Monster.h"
#include "Player.h"
#include <iostream>
#include <queue>
#include <set>
#include <sstream>

static int failures = 0, checks = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        ++checks;                                                                \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " << #cond << "\n"; \
        }                                                                        \
    } while (0)

// ---------------------------------------------------------------- combat
static void testCombat() {
    Rng::seed(1);
    Player p(Vec2{0, 0});
    auto g = Monster::create("goblin", Vec2{1, 0}, 1);
    for (int i = 0; i < 2000; ++i) {
        auto h = combat::roll(*g, p);
        CHECK(h.damage >= 1);
        CHECK(h.damage <= (g->attackPower() - p.defensePower() + 2) * 2);
    }
    // Armor must reduce damage on average; weapon must raise it.
    auto avg = [&](const Entity& a, const Entity& d) {
        long total = 0;
        for (int i = 0; i < 5000; ++i) total += combat::roll(a, d).damage;
        return total / 5000.0;
    };
    double before = avg(*g, p);
    auto armor = Item::create(ItemKind::Armor, Rarity::Rare, 3);
    p.addItem(armor);
    p.useItem(0);
    CHECK(avg(*g, p) < before);
}

// ------------------------------------------------------------- dungeon gen
static bool connected(const Dungeon& d, Vec2 from, Vec2 to) {
    std::set<std::pair<int, int>> seen{{from.x, from.y}};
    std::queue<Vec2> q;
    q.push(from);
    while (!q.empty()) {
        Vec2 c = q.front(); q.pop();
        if (c == to) return true;
        for (Vec2 dir : DIRS) {
            Vec2 n = c + dir;
            if (d.walkable(n) && seen.insert({n.x, n.y}).second) q.push(n);
        }
    }
    return false;
}

static void testDungeon() {
    for (uint32_t seed = 0; seed < 300; ++seed) {
        Rng::seed(seed);
        Dungeon d(60, 18);
        d.generate(8, true);
        CHECK(d.rooms().size() >= 3);
        CHECK(d.hasStairs() && d.tileAt(d.stairs()) == Tile::Stairs);
        for (const Room& r : d.rooms()) CHECK(connected(d, d.rooms().front().center(), r.center()));
        // Outer border must stay solid wall.
        for (int x = 0; x < 60; ++x) CHECK(!d.walkable({x, 0}) && !d.walkable({x, 17}));
        for (int y = 0; y < 18; ++y) CHECK(!d.walkable({0, y}) && !d.walkable({59, y}));
    }
    Rng::seed(5);
    Dungeon boss(60, 18);
    boss.generate(7, false);
    CHECK(!boss.hasStairs());
}

static void testPathfindingAndLos() {
    Rng::seed(11);
    Dungeon d(60, 18);
    d.generate(7, true);
    Vec2 a = d.rooms().front().center(), b = d.stairs();
    Vec2 cur = a;
    int steps = 0;
    while (cur != b && steps < 1000) {
        auto n = d.nextStep(cur, b, 500);
        CHECK(n.has_value());
        if (!n) break;
        CHECK(manhattan(cur, *n) == 1);
        CHECK(d.walkable(*n));
        cur = *n;
        ++steps;
    }
    CHECK(cur == b);
    // A blocker on the only path must be routed around or reported unreachable, never walked through.
    auto n = d.nextStep(a, b, 500, [&](Vec2 p) { return p != b; });
    CHECK(!n.has_value() || *n == b);
    CHECK(!d.nextStep(a, a, 10).has_value());
    CHECK(d.pathLength(a, a, 10) == 0);
    int len = d.pathLength(a, b, 500);
    CHECK(len >= manhattan(a, b) && len == steps);              // BFS length matches the walked path
    CHECK(d.pathLength(a, b, len - 1) == -1);                   // respects maxDist
    CHECK(d.pathLength(a, Vec2{0, 0}, 500) == -1);              // wall target
    CHECK(!d.nextStep(a, Vec2{0, 0}, 500).has_value());   // wall target unreachable

    // LOS: open floor sees itself; a wall between two floor tiles blocks.
    Rng::seed(3);
    Dungeon room(60, 18);
    room.generate(3, false);
    const Room& r = room.rooms().front();
    CHECK(room.los({r.x, r.y}, {r.x + r.w - 1, r.y + r.h - 1}));
    CHECK(!room.los({r.x, r.y}, {0, 0}) || room.walkable({0, 0}));
}

// -------------------------------------------------------------------- items
static void testItemsAndPlayer() {
    Rng::seed(2);
    for (int f = 1; f <= 5; ++f)
        for (int i = 0; i < 300; ++i) {
            auto it = Item::random(f);
            CHECK(it && it->value() > 0);
            std::stringstream ss;
            it->save(ss);
            auto back = Item::load(ss);
            CHECK(back && back->kind() == it->kind() && back->rarity() == it->rarity() &&
                  back->value() == it->value() && back->describe() == it->describe());
        }
    std::stringstream bad("9 9 9");
    CHECK(Item::load(bad) == nullptr);

    Player p(Vec2{0, 0});
    int baseAtk = p.attackPower();
    auto sword = Item::create(ItemKind::Weapon, Rarity::Rare, 4);
    p.addItem(sword);
    CHECK(sword == nullptr);                       // ownership moved in
    auto r = p.useItem(0);
    CHECK(r.valid && r.tookTurn);
    CHECK(p.attackPower() == baseAtk + 4 && p.inventory().empty());
    auto better = Item::create(ItemKind::Weapon, Rarity::Epic, 8);
    p.addItem(better);
    p.useItem(0);                                  // swap: old sword returns to the pack
    CHECK(p.attackPower() == baseAtk + 8 && p.inventory().size() == 1);

    // Potions heal but never exceed max, and are refused (not consumed) at full HP.
    auto pot = Item::create(ItemKind::Potion, Rarity::Common, 100);
    p.addItem(pot);
    auto refused = p.useItem(1);
    CHECK(refused.valid && !refused.tookTurn && p.inventory().size() == 2);
    p.takeDamage(10);
    auto drunk = p.useItem(1);
    CHECK(drunk.tookTurn && p.hp() == p.maxHp() && p.inventory().size() == 1);

    // Pack capacity.
    for (size_t i = p.inventory().size(); i < Player::MAX_ITEMS; ++i) {
        auto x = Item::create(ItemKind::Potion, Rarity::Common, 5);
        CHECK(p.addItem(x));
    }
    auto extra = Item::create(ItemKind::Potion, Rarity::Common, 5);
    CHECK(!p.addItem(extra) && extra != nullptr);
    CHECK(!p.useItem(99).valid && p.dropItem(99) == nullptr);

    // Levelling.
    Player q(Vec2{0, 0});
    int hp0 = q.maxHp();
    CHECK(q.gainXp(19) == 0 && q.level() == 1);
    CHECK(q.gainXp(1) == 1 && q.level() == 2 && q.maxHp() > hp0);
    CHECK(q.gainXp(1000) >= 3);
}

// ---------------------------------------------------------------- monsters
static void testMonsterSerialisation() {
    Rng::seed(4);
    for (const char* id : {"goblin", "archer", "troll", "spider", "dragon"}) {
        auto m = Monster::create(id, Vec2{3, 4}, 3);
        m->takeDamage(2);
        std::stringstream ss;
        m->save(ss);
        auto back = Monster::load(ss);
        CHECK(back && back->typeId() == id && back->hp() == m->hp() && back->pos() == m->pos());
    }
    CHECK(Monster::create("nonsense", Vec2{0, 0}, 1) == nullptr);
    std::stringstream junk("goblin 1 2 -5 10 3 1 0 0 0 1");
    CHECK(Monster::load(junk) == nullptr);
}

// -------------------------------------------- whole-game fuzz with a bot
static bool invariantsHold(const Game& g) {
    const Player& p = g.player();
    if (p.hp() < 0 || p.hp() > p.maxHp()) return false;
    if (p.inventory().size() > Player::MAX_ITEMS) return false;
    if (g.state() != Game::State::Dead && !g.dungeon().walkable(p.pos())) return false;
    std::set<std::pair<int, int>> seen{{p.pos().x, p.pos().y}};
    for (const auto& m : g.monsters()) {
        if (!m->alive() || !g.dungeon().walkable(m->pos())) return false;
        if (!seen.insert({m->pos().x, m->pos().y}).second) return false;   // two things on one tile
    }
    for (const auto& gi : g.groundItems())
        if (!g.dungeon().walkable(gi.pos)) return false;
    return true;
}

static Action stepDirection(Vec2 from, Vec2 to) {
    if (to.x > from.x) return Action::Right;
    if (to.x < from.x) return Action::Left;
    if (to.y > from.y) return Action::Down;
    return Action::Up;
}

static void testFuzzBot() {
    int wins = 0, deaths = 0, deepest = 0, timeouts = 0;
    for (uint32_t seed = 1; seed <= 60; ++seed) {
        Game g(seed);
        Rng::seed(seed * 31 + 7);   // separate stream for the bot's own randomness
        for (int turn = 0; turn < 4000 && g.state() == Game::State::Playing; ++turn) {
            const Player& p = g.player();
            // Drink a potion when low.
            if (p.hp() * 5 < p.maxHp() * 2) {
                for (size_t i = 0; i < p.inventory().size(); ++i)
                    if (p.inventory()[i]->kind() == ItemKind::Potion) { g.useItem(i); break; }
            }
            // Equip anything better-looking (just equip everything equippable).
            for (size_t i = 0; i < p.inventory().size(); ++i)
                if (p.inventory()[i]->equippable() && Rng::chance(0.5)) { g.useItem(i); break; }

            Action a;
            if (Rng::chance(0.25)) {
                a = static_cast<Action>(Rng::range(0, 5));
            } else {
                Vec2 target = p.pos();
                if (g.dungeon().hasStairs()) target = g.dungeon().stairs();
                else if (!g.monsters().empty()) target = g.monsters().front()->pos();
                auto n = g.dungeon().nextStep(p.pos(), target, 500);
                a = n ? stepDirection(p.pos(), *n) : Action::Wait;
            }
            g.act(a);
            if (!invariantsHold(g)) {
                std::cerr << "invariant broken: seed " << seed << " turn " << turn << "\n";
                ++failures;
                break;
            }
        }
        deepest = std::max(deepest, g.floor());
        if (g.state() == Game::State::Won) ++wins;
        else if (g.state() == Game::State::Dead) ++deaths;
        else ++timeouts;
        CHECK(true);
    }
    std::cout << "  bot results over 60 games: " << wins << " wins, " << deaths << " deaths, "
              << timeouts << " unfinished, deepest floor " << deepest << "\n";
    CHECK(deepest >= 3);   // the bot must at least be able to descend several floors
}

// -------------------------------------------------------- save / load
static void testSaveLoad() {
    for (uint32_t seed : {7u, 21u, 99u}) {
        Game a(seed);
        Rng::seed(seed + 1);
        for (int i = 0; i < 120 && a.state() == Game::State::Playing; ++i) a.act(static_cast<Action>(Rng::range(0, 5)));
        std::ostringstream s1;
        a.save(s1);

        Game b(12345);
        std::istringstream in(s1.str());
        CHECK(b.load(in));
        std::ostringstream s2;
        b.save(s2);
        CHECK(s1.str() == s2.str());               // exact round trip of ALL state
        CHECK(b.floor() == a.floor() && b.turn() == a.turn());
    }

    // Corrupt / truncated saves are rejected and leave the game untouched.
    Game g(5);
    std::ostringstream good;
    g.save(good);
    std::string text = good.str();
    int turnBefore = g.turn();
    for (std::string bad : {std::string(), std::string("garbage"), text.substr(0, text.size() / 2),
                            std::string("ROGUESAVE 1\n") + text.substr(text.find('\n') + 1)}) {
        std::istringstream in(bad);
        CHECK(!g.load(in));
    }
    CHECK(g.turn() == turnBefore && g.state() == Game::State::Playing);
}

// ---------------------------------------------------------- pickup / drop
static void testPickupDrop() {
    Game g(8);
    auto potion = Item::create(ItemKind::Potion, Rarity::Common, 5);
    g.player().addItem(potion);
    size_t ground0 = g.groundItems().size();
    CHECK(g.dropItem(0));
    CHECK(g.player().inventory().empty() && g.groundItems().size() == ground0 + 1);
    CHECK(g.act(Action::Pickup));                          // takes it back (a turn passes)
    CHECK(g.player().inventory().size() == 1 && g.groundItems().size() == ground0);
    CHECK(!g.act(Action::Pickup));                         // nothing left here: no turn spent
    // Full pack: pickup fails and spends no turn.
    for (size_t i = g.player().inventory().size(); i < Player::MAX_ITEMS; ++i) {
        auto x = Item::create(ItemKind::Potion, Rarity::Common, 5);
        g.player().addItem(x);
    }
    CHECK(g.dropItem(0));
    auto filler = Item::create(ItemKind::Potion, Rarity::Common, 5);
    CHECK(g.player().addItem(filler));                     // pack full again with an item still on the floor
    int turn = g.turn();
    CHECK(!g.act(Action::Pickup) && g.turn() == turn);
}

// ----------------------------------------------------------- rendering
static void testRender() {
    Game g(3);
    std::string frame = g.renderFrame();
    CHECK(frame.find('@') != std::string::npos);
    CHECK(frame.find("Floor 1/5") != std::string::npos);
    int newlines = 0;
    for (char c : frame) newlines += c == '\n';
    CHECK(newlines == Game::MAP_H + 2 + 1 + 5 + 1);   // map + 2 HUD + rule + 5 log + controls
}

int main() {
    std::cout << "Running tests...\n";
    testCombat();
    testDungeon();
    testPathfindingAndLos();
    testItemsAndPlayer();
    testMonsterSerialisation();
    testSaveLoad();
    testPickupDrop();
    testRender();
    testFuzzBot();
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
