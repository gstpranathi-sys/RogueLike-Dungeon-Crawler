// Balance simulator: a bot that plays "sensibly" (loots, equips upgrades, fights, drinks
// potions, rests) and reports win rate. Used to tune difficulty. Run: make balance
#include "Game.h"
#include <algorithm>
#include <iostream>
#include <map>

namespace {
Action dirTo(Vec2 from, Vec2 to) {
    if (to.x > from.x) return Action::Right;
    if (to.x < from.x) return Action::Left;
    if (to.y > from.y) return Action::Down;
    return Action::Up;
}

struct Bot {
    const Monster* lockedMonster = nullptr;   // sticks with a target so it never flip-flops
    bool hasItemLock = false;
    Vec2 itemLock;
    int lockFloor = 0;

    static bool equipUpgrades(Game& g) {
        const Player& p = g.player();
        for (size_t i = 0; i < p.inventory().size(); ++i) {
            const Item& it = *p.inventory()[i];
            if (!it.equippable()) continue;
            const Item* cur = it.kind() == ItemKind::Weapon ? p.weapon() : p.armor();
            if (!cur || it.value() > cur->value()) { g.useItem(i); return true; }
        }
        return false;
    }

    void turn(Game& g) {
        const Player& p = g.player();
        const Dungeon& d = g.dungeon();
        if (lockFloor != g.floor()) { lockFloor = g.floor(); lockedMonster = nullptr; hasItemLock = false; }

        int nearestDist = 999;
        const Monster* nearest = nullptr;
        for (const auto& m : g.monsters()) {
            int dist = d.pathLength(p.pos(), m->pos(), 200);   // real walking distance, not through walls
            if (dist >= 0 && dist < nearestDist) { nearestDist = dist; nearest = m.get(); }
        }

        if (p.hp() * 100 < p.maxHp() * 45 && nearestDist <= 3) {
            for (size_t i = 0; i < p.inventory().size(); ++i)
                if (p.inventory()[i]->kind() == ItemKind::Potion) { g.useItem(i); return; }
        }
        if (equipUpgrades(g)) return;
        if (p.hp() * 100 < p.maxHp() * 70 && nearestDist > 8) { g.act(Action::Wait); return; }

        // Drop the lock if its monster died / item was taken.
        bool monsterAlive = lockedMonster && std::any_of(g.monsters().begin(), g.monsters().end(),
            [&](const std::unique_ptr<Monster>& m) { return m.get() == lockedMonster; });
        if (!monsterAlive) lockedMonster = nullptr;
        if (hasItemLock && !std::any_of(g.groundItems().begin(), g.groundItems().end(),
                [&](const GroundItem& gi) { return gi.pos == itemLock; })) hasItemLock = false;

        Vec2 target = p.pos();
        bool have = false;
        if (nearestDist <= 2) { target = nearest->pos(); have = true; }    // fight what's on us
        if (!have && hasItemLock && p.inventory().size() < Player::MAX_ITEMS) { target = itemLock; have = true; }
        if (!have && p.inventory().size() < Player::MAX_ITEMS) {
            int best = 999;
            for (const auto& gi : g.groundItems()) {
                int dist = d.pathLength(p.pos(), gi.pos, 200);
                if (dist >= 0 && dist < best) { best = dist; itemLock = gi.pos; hasItemLock = true; target = gi.pos; have = true; }
            }
        }
        if (!have && lockedMonster) { target = lockedMonster->pos(); have = true; }
        if (!have && nearest) { lockedMonster = nearest; target = nearest->pos(); have = true; }
        if (!have && d.hasStairs()) { target = d.stairs(); have = true; }
        if (!have) { g.act(Action::Wait); return; }

        if (target == p.pos()) { g.act(Action::Pickup); return; }       // standing on the item
        auto n = d.nextStep(p.pos(), target, 500);
        g.act(n ? dirTo(p.pos(), *n) : Action::Wait);
    }
};
}  // namespace

int main(int argc, char** argv) {
    int games = argc > 1 ? std::atoi(argv[1]) : 100;
    int wins = 0, deaths = 0, unfinished = 0;
    std::map<int, int> deathFloor;
    long turnsOnWin = 0, levelOnWin = 0;
    for (int s = 1; s <= games; ++s) {
        Game g(static_cast<uint32_t>(s));
        Bot bot;
        for (int t = 0; t < 30000 && g.state() == Game::State::Playing; ++t) bot.turn(g);
        if (g.state() == Game::State::Won) { ++wins; turnsOnWin += g.turn(); levelOnWin += g.player().level(); }
        else if (g.state() == Game::State::Dead) { ++deaths; ++deathFloor[g.floor()]; }
        else {
            ++unfinished;
            std::cout << "  unfinished: seed " << s << " floor " << g.floor() << " hp " << g.player().hp() << "/"
                      << g.player().maxHp() << " pos " << g.player().pos().x << "," << g.player().pos().y
                      << " pack " << g.player().inventory().size() << " monsters:";
            for (const auto& m : g.monsters())
                std::cout << " " << m->typeId() << "@" << m->pos().x << "," << m->pos().y << "(hp" << m->hp() << ")";
            std::cout << "\n";
        }
    }
    std::cout << games << " games: " << wins << " wins (" << 100 * wins / games << "%), " << deaths
              << " deaths, " << unfinished << " unfinished\n";
    for (auto& [f, c] : deathFloor) std::cout << "  deaths on floor " << f << ": " << c << "\n";
    if (wins) std::cout << "  avg turns on win " << turnsOnWin / wins << ", avg level " << levelOnWin / wins << "\n";
}
