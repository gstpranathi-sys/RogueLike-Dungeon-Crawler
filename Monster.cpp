#include "Monster.h"
#include "Ansi.h"
#include "Player.h"
#include <iostream>
#include <vector>

// ---------------------------------------------------------------- base class
Monster::Monster(std::string name, Vec2 pos, int hp, int atk, int def, char symbol,
                 const char* color, int xp, int actEvery)
    : Entity(std::move(name), pos, hp, atk, def, symbol, color), xpReward_(xp), actEvery_(actEvery) {}

void Monster::takeTurn(World& w) {
    if (!alive()) return;
    ++tick_;
    if (actEvery_ > 1 && tick_ % actEvery_ != 0) return;   // slow monsters skip turns
    act(w);
}

bool Monster::adjacentToPlayer(World& w) const { return manhattan(pos_, w.player().pos()) == 1; }

bool Monster::stepToward(World& w, Vec2 target) {
    // BFS around walls; other monsters count as obstacles.
    auto step = w.dungeon().nextStep(pos_, target, 45,
        [&](Vec2 p) { return p != target && w.occupied(p, this); });
    if (!step) return false;
    pos_ = *step;
    return true;
}

void Monster::chaseAndAttack(World& w) {
    Player& p = w.player();
    if (adjacentToPlayer(w)) w.attack(*this, p, verb());
    else if (manhattan(pos_, p.pos()) <= 20) stepToward(w, p.pos());
}

void Monster::save(std::ostream& os) const {
    os << typeId() << ' ';
    saveBase(os);
    os << ' ' << tick_ << ' ' << (awake_ ? 1 : 0) << '\n';
}

std::unique_ptr<Monster> Monster::load(std::istream& is) {
    std::string id;
    if (!(is >> id)) return nullptr;
    auto m = create(id, {0, 0}, 1);
    if (!m || !m->loadBase(is)) return nullptr;
    int awake;
    if (!(is >> m->tick_ >> awake)) return nullptr;
    m->awake_ = awake != 0;
    return m;
}

std::unique_ptr<Monster> Monster::create(const std::string& id, Vec2 pos, int floor) {
    std::unique_ptr<Monster> m;
    if (id == "goblin") m = std::make_unique<Goblin>(pos);
    else if (id == "archer") m = std::make_unique<Archer>(pos);
    else if (id == "troll") m = std::make_unique<Troll>(pos);
    else if (id == "spider") m = std::make_unique<Spider>(pos);
    else if (id == "dragon") m = std::make_unique<Dragon>(pos);
    else return nullptr;
    if (id != "dragon") {                      // monsters get tougher with depth
        int f = std::max(1, floor) - 1;
        m->boost(3 * f, f / 2, f / 3);
    }
    return m;
}

std::unique_ptr<Monster> Monster::randomForFloor(Vec2 pos, int floor) {
    std::vector<const char*> pool = {"goblin", "goblin", "goblin", "archer", "archer"};
    if (floor >= 2) pool.push_back("troll");
    if (floor >= 3) { pool.push_back("spider"); pool.push_back("spider"); }
    if (floor >= 4) pool.push_back("troll");
    return create(pool[static_cast<size_t>(Rng::range(0, static_cast<int>(pool.size()) - 1))], pos, floor);
}

// ------------------------------------------------------------------ Goblin
Goblin::Goblin(Vec2 p) : Monster("Goblin", p, 12, 5, 0, 'g', ansi::GREEN, 8) {}
void Goblin::act(World& w) { chaseAndAttack(w); }

// ------------------------------------------------------------------ Archer
Archer::Archer(Vec2 p) : Monster("Skeleton Archer", p, 9, 5, 0, 'a', ansi::YELLOW, 12, 2) {}
void Archer::act(World& w) {
    // Slow (acts every other turn), so the player can always close the distance.
    Player& p = w.player();
    int d = manhattan(pos_, p.pos());
    if (d > 14) return;
    if (d <= 1) { w.attack(*this, p, "stabs"); return; }                      // cornered: melee
    if (d <= 7 && w.dungeon().los(pos_, p.pos())) { w.attack(*this, p, verb()); return; }   // fire from range
    stepToward(w, p.pos());
}

// ------------------------------------------------------------------- Troll
Troll::Troll(Vec2 p) : Monster("Cave Troll", p, 28, 8, 2, 'T', ansi::RED, 25, 2) { awake_ = false; }
void Troll::act(World& w) {
    if (!awake_) {
        if (manhattan(pos_, w.player().pos()) > 5) return;
        awake_ = true;
        w.log(std::string(ansi::BYELLOW) + "The Cave Troll wakes up and roars!" + ansi::RESET);
    }
    chaseAndAttack(w);
}

// ------------------------------------------------------------------ Spider
Spider::Spider(Vec2 p) : Monster("Giant Spider", p, 14, 4, 1, 'S', ansi::MAGENTA, 14) {}
void Spider::act(World& w) {
    Player& p = w.player();
    if (!adjacentToPlayer(w)) { chaseAndAttack(w); return; }
    w.attack(*this, p, verb());
    if (p.alive() && Rng::chance(0.5)) {
        p.addPoison(4);
        w.log(std::string(ansi::BGREEN) + "You have been poisoned!" + ansi::RESET);
    }
}

// ------------------------------------------------------------------ Dragon
Dragon::Dragon(Vec2 p) : Monster("Dragon", p, 100, 14, 3, 'D', ansi::BRED, 100) { awake_ = false; }
void Dragon::act(World& w) {
    Player& p = w.player();
    int d = manhattan(pos_, p.pos());
    bool clear = w.dungeon().los(pos_, p.pos());
    if (!awake_) {
        if (d > 9 && !(clear && d <= 12)) return;
        awake_ = true;
        w.log(std::string(ansi::BRED) + "The Dragon rises from its hoard!" + ansi::RESET);
        return;
    }
    if (d == 1) { w.attack(*this, p, verb()); return; }
    if (clear && d <= 6 && Rng::chance(0.35)) { w.attack(*this, p, "breathes fire on"); return; }
    stepToward(w, p.pos());
}
