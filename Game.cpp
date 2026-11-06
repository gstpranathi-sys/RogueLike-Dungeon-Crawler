#include "Game.h"
#include "Ansi.h"
#include "Terminal.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {
const char* const SAVE_FILE = "savegame.dat";
const int SAVE_VERSION = 2;

std::string coloredItem(const Item& it) {
    return std::string(it.color()) + it.describe() + ansi::RESET;
}

std::string hpBar(int cur, int mx, int width) {
    int filled = mx > 0 ? (cur * width + mx - 1) / mx : 0;
    if (cur > 0 && filled < 1) filled = 1;
    filled = std::max(0, std::min(width, filled));
    const char* col = cur * 2 > mx ? ansi::BGREEN : (cur * 4 > mx ? ansi::BYELLOW : ansi::BRED);
    return std::string(col) + "[" + std::string(static_cast<size_t>(filled), '#') +
           std::string(static_cast<size_t>(width - filled), '-') + "]" + ansi::RESET;
}

bool keyToAction(int k, Action& a) {
    switch (k) {
        case 'w': case 'W': case term::KEY_UP:    a = Action::Up; return true;
        case 's': case 'S': case term::KEY_DOWN:  a = Action::Down; return true;
        case 'a': case 'A': case term::KEY_LEFT:  a = Action::Left; return true;
        case 'd': case 'D': case term::KEY_RIGHT: a = Action::Right; return true;
        case '.': case ' ':                       a = Action::Wait; return true;
        case 'g': case 'G': case ',':             a = Action::Pickup; return true;
    }
    return false;
}
}  // namespace

// ============================================================ setup / floors
Game::Game(uint32_t seed) : seed_(seed), dungeon_(MAP_W, MAP_H), player_(Vec2{0, 0}) { newGame(); }

void Game::newGame() {
    Rng::seed(seed_);
    player_ = Player(Vec2{0, 0});
    floor_ = 1; turn_ = 0; kills_ = 0;
    state_ = State::Playing;
    log_.clear();
    skipMonsters_ = false;
    generateFloor();
    log(std::string(ansi::BCYAN) + "You descend into the dungeon. Find the stairs (>) and slay the Dragon on floor " +
        std::to_string(FINAL_FLOOR) + "." + ansi::RESET);
}

Vec2 Game::randomSpawn(size_t firstRoom, size_t lastRoom) const {
    const auto& rooms = dungeon_.rooms();
    for (int t = 0; t < 300; ++t) {
        const Room& r = rooms[static_cast<size_t>(
            Rng::range(static_cast<int>(firstRoom), static_cast<int>(lastRoom)))];
        Vec2 p{Rng::range(r.x, r.x + r.w - 1), Rng::range(r.y, r.y + r.h - 1)};
        if (dungeon_.tileAt(p) != Tile::Floor || occupied(p, nullptr)) continue;
        if (std::any_of(ground_.begin(), ground_.end(), [&](const GroundItem& g) { return g.pos == p; })) continue;
        return p;
    }
    return rooms[firstRoom].center();
}

void Game::generateFloor() {
    const bool boss = floor_ == FINAL_FLOOR;
    dungeon_.generate(floor_ > 2 ? 8 : 7, !boss);
    monsters_.clear();
    ground_.clear();

    const auto& rooms = dungeon_.rooms();
    player_.setPos(rooms.front().center());
    // Rooms 1..last are spawn rooms; on the boss floor the last room is reserved for the Dragon.
    size_t lastSpawn = boss ? rooms.size() - 2 : rooms.size() - 1;

    int monsterCount = 2 + floor_;
    for (int i = 0; i < monsterCount; ++i)
        monsters_.push_back(Monster::randomForFloor(randomSpawn(1, lastSpawn), floor_));
    if (boss) monsters_.push_back(Monster::create("dragon", rooms.back().center(), floor_));

    int itemCount = 3 + floor_ / 2;
    for (int i = 0; i < itemCount; ++i)
        ground_.push_back({randomSpawn(1, lastSpawn), Item::random(floor_)});
    ground_.push_back({randomSpawn(1, lastSpawn),                    // always at least one heal per floor
                       Item::create(ItemKind::Potion, Rarity::Common, 12 + Rng::range(0, 4))});

    dungeon_.computeFov(player_.pos(), FOV_RADIUS);
}

void Game::descend() {
    ++floor_;
    generateFloor();
    skipMonsters_ = true;     // monsters on the new floor don't get a free move
    log(std::string(ansi::BCYAN) + "You descend to floor " + std::to_string(floor_) +
        (floor_ == FINAL_FLOOR ? ". You feel a terrible heat ahead..." : ".") + ansi::RESET);
}

// ================================================================== rules
bool Game::occupied(Vec2 p, const Entity* ignore) const {
    if (ignore != &player_ && player_.pos() == p) return true;
    for (const auto& m : monsters_)
        if (m.get() != ignore && m->alive() && m->pos() == p) return true;
    return false;
}

void Game::log(const std::string& message) {
    log_.push_back(message);
    if (log_.size() > 50) log_.pop_front();
}

void Game::attack(Entity& a, Entity& d, const std::string& verb) {
    combat::Hit hit = combat::roll(a, d);
    d.takeDamage(hit.damage);
    std::ostringstream m;
    if (&a == &player_)
        m << ansi::BGREEN << "You " << verb << " the " << d.name() << " for " << hit.damage << " damage."
          << (hit.crit ? " Critical hit!" : "") << ansi::RESET;
    else
        m << ansi::BRED << "The " << a.name() << " " << verb << " you for " << hit.damage << " damage."
          << (hit.crit ? " Critical!" : "") << ansi::RESET;
    log(m.str());
    if (!d.alive()) onDeath(d);
}

void Game::onDeath(Entity& e) {
    if (&e == &player_) {
        state_ = State::Dead;
        log(std::string(ansi::BRED) + "You have died." + ansi::RESET);
        return;
    }
    auto* m = dynamic_cast<Monster*>(&e);
    if (!m) return;
    ++kills_;
    log(std::string(ansi::YELLOW) + "The " + m->name() + " dies. (+" + std::to_string(m->xp()) + " xp)" + ansi::RESET);
    int levels = player_.gainXp(m->xp());
    if (levels > 0)
        log(std::string(ansi::BYELLOW) + "Level up! You are now level " + std::to_string(player_.level()) + "." + ansi::RESET);
    if (m->typeId() == "dragon") {
        state_ = State::Won;
        log(std::string(ansi::BGREEN) + "The Dragon falls. The dungeon is yours!" + ansi::RESET);
    }
}

void Game::cleanupDead() {
    monsters_.erase(std::remove_if(monsters_.begin(), monsters_.end(),
                                   [](const std::unique_ptr<Monster>& m) { return !m->alive(); }),
                    monsters_.end());
}

void Game::pickupItems() {
    for (auto it = ground_.begin(); it != ground_.end();) {
        if (it->pos != player_.pos()) { ++it; continue; }
        std::string desc = coloredItem(*it->item);
        if (player_.addItem(it->item)) {
            log("You pick up " + desc + ".");
            it = ground_.erase(it);
        } else {
            log("Your pack is full; you leave " + desc + " here.");
            ++it;
        }
    }
}

bool Game::pickupHere() {
    bool anyHere = std::any_of(ground_.begin(), ground_.end(),
                               [&](const GroundItem& g) { return g.pos == player_.pos(); });
    if (!anyHere) { log("There is nothing here to pick up."); return false; }
    size_t before = player_.inventory().size();
    pickupItems();
    return player_.inventory().size() > before;     // a turn passes only if something was taken
}

bool Game::tryMove(Vec2 dir) {
    Vec2 target = player_.pos() + dir;
    for (auto& m : monsters_)
        if (m->alive() && m->pos() == target) { attack(player_, *m, "hit"); return true; }
    if (!dungeon_.walkable(target)) return false;       // bumping a wall costs nothing
    player_.setPos(target);
    if (dungeon_.tileAt(target) == Tile::Stairs) { descend(); return true; }
    pickupItems();
    return true;
}

void Game::endTurn() {
    ++turn_;
    cleanupDead();
    if (state_ != State::Playing) return;

    if (player_.poisoned()) {
        player_.tickPoison();
        log(std::string(ansi::BGREEN) + "Poison burns you for 1 damage." + ansi::RESET);
        if (!player_.alive()) { onDeath(player_); return; }
    } else if (turn_ % 10 == 0) {
        player_.heal(1);                                 // slow natural regeneration
    }

    if (skipMonsters_) {
        skipMonsters_ = false;
    } else {
        for (size_t i = 0; i < monsters_.size() && state_ == State::Playing; ++i)
            monsters_[i]->takeTurn(*this);
        cleanupDead();
    }
    dungeon_.computeFov(player_.pos(), FOV_RADIUS);
}

bool Game::act(Action a) {
    if (state_ != State::Playing) return false;
    bool spent = false;
    switch (a) {
        case Action::Up:    spent = tryMove(DIRS[0]); break;
        case Action::Down:  spent = tryMove(DIRS[1]); break;
        case Action::Left:  spent = tryMove(DIRS[2]); break;
        case Action::Right: spent = tryMove(DIRS[3]); break;
        case Action::Wait:  spent = true; break;
        case Action::Pickup: spent = pickupHere(); break;
    }
    if (spent) endTurn();
    return spent;
}

bool Game::useItem(size_t index) {
    if (state_ != State::Playing) return false;
    UseResult r = player_.useItem(index);
    if (!r.valid) return false;
    log(r.message);
    if (r.tookTurn) endTurn();
    return true;
}

bool Game::dropItem(size_t index) {
    if (state_ != State::Playing) return false;
    auto item = player_.dropItem(index);
    if (!item) return false;
    log("You drop " + coloredItem(*item) + ".");
    ground_.push_back({player_.pos(), std::move(item)});
    endTurn();
    return true;
}

// ============================================================ save / load
void Game::save(std::ostream& os) const {
    os << "ROGUESAVE " << SAVE_VERSION << '\n';
    os << seed_ << ' ' << floor_ << ' ' << turn_ << ' ' << kills_ << '\n';
    player_.save(os);
    dungeon_.save(os);
    os << monsters_.size() << '\n';
    for (const auto& m : monsters_) m->save(os);
    os << ground_.size() << '\n';
    for (const auto& g : ground_) {
        os << g.pos.x << ' ' << g.pos.y << ' ';
        g.item->save(os);
    }
}

bool Game::load(std::istream& is) {
    std::string magic;
    int version;
    if (!(is >> magic >> version) || magic != "ROGUESAVE" || version != SAVE_VERSION) return false;
    uint32_t seed;
    int fl, turn, kills;
    if (!(is >> seed >> fl >> turn >> kills) || fl < 1 || fl > FINAL_FLOOR || turn < 0 || kills < 0) return false;

    Player p(Vec2{0, 0});
    if (!p.load(is)) return false;
    Dungeon d;
    if (!d.load(is) || !d.walkable(p.pos())) return false;

    size_t mc;
    if (!(is >> mc) || mc > 500) return false;
    std::vector<std::unique_ptr<Monster>> ms;
    for (size_t i = 0; i < mc; ++i) {
        auto m = Monster::load(is);
        if (!m || !d.walkable(m->pos())) return false;
        ms.push_back(std::move(m));
    }
    size_t gc;
    if (!(is >> gc) || gc > 500) return false;
    std::vector<GroundItem> gs;
    for (size_t i = 0; i < gc; ++i) {
        int x, y;
        if (!(is >> x >> y)) return false;
        auto it = Item::load(is);
        if (!it || !d.walkable(Vec2{x, y})) return false;
        gs.push_back({Vec2{x, y}, std::move(it)});
    }

    // Everything parsed: commit.
    seed_ = seed; floor_ = fl; turn_ = turn; kills_ = kills;
    Rng::seed(seed + static_cast<uint32_t>(turn) * 7919u);
    player_ = std::move(p);
    dungeon_ = std::move(d);
    monsters_ = std::move(ms);
    ground_ = std::move(gs);
    state_ = State::Playing;
    skipMonsters_ = false;
    log_.clear();
    dungeon_.computeFov(player_.pos(), FOV_RADIUS);
    return true;
}

bool Game::saveToFile() const {
    std::ofstream out(SAVE_FILE);
    if (!out) return false;
    save(out);
    return static_cast<bool>(out);
}

bool Game::loadFromFile() {
    std::ifstream in(SAVE_FILE);
    return in && load(in);
}

// ============================================================== rendering
void Game::cellAt(Vec2 p, char& glyph, const char*& color) const {
    if (!dungeon_.explored(p)) { glyph = ' '; color = ansi::RESET; return; }
    const bool vis = dungeon_.visible(p);
    switch (dungeon_.tileAt(p)) {
        case Tile::Wall:   glyph = '#'; color = vis ? ansi::WHITE : ansi::GRAY; break;
        case Tile::Floor:  glyph = '.'; color = vis ? ansi::WHITE : ansi::GRAY; break;
        case Tile::Stairs: glyph = '>'; color = vis ? ansi::BCYAN : ansi::CYAN; break;
    }
    if (!vis) return;                                    // remembered tiles show terrain only
    for (const auto& g : ground_)
        if (g.pos == p) { glyph = g.item->symbol(); color = g.item->color(); }
    for (const auto& m : monsters_)
        if (m->alive() && m->pos() == p) { glyph = m->symbol(); color = m->color(); }
    if (player_.pos() == p) { glyph = player_.symbol(); color = player_.color(); }
}

std::string Game::renderFrame() const {
    std::string out = "\033[H";
    auto line = [&](const std::string& s) { out += s; out += "\033[K\n"; };

    for (int y = 0; y < MAP_H; ++y) {
        std::string row;
        const char* last = nullptr;
        for (int x = 0; x < MAP_W; ++x) {
            char g;
            const char* c;
            cellAt({x, y}, g, c);
            if (c != last) { row += c; last = c; }
            row += g;
        }
        row += ansi::RESET;
        line(row);
    }

    std::ostringstream h1;
    h1 << ansi::BCYAN << "Floor " << floor_ << "/" << FINAL_FLOOR << ansi::RESET << "  HP "
       << hpBar(player_.hp(), player_.maxHp(), 12) << ' ' << player_.hp() << '/' << player_.maxHp()
       << "  Lv " << player_.level() << " (" << player_.xp() << '/' << player_.xpToNext() << " xp)";
    if (player_.poisoned()) h1 << "  " << ansi::BGREEN << "POISONED" << ansi::RESET;
    line(h1.str());

    std::ostringstream h2;
    h2 << "ATK " << player_.attackPower() << "  DEF " << player_.defensePower() << "  Weapon: "
       << (player_.weapon() ? coloredItem(*player_.weapon()) : "none") << "  Armor: "
       << (player_.armor() ? coloredItem(*player_.armor()) : "none");
    line(h2.str());

    line(std::string(ansi::GRAY) + std::string(MAP_W, '-') + ansi::RESET);
    const int shown = 5;
    int start = std::max(0, static_cast<int>(log_.size()) - shown);
    for (int i = 0; i < shown; ++i)
        line(start + i < static_cast<int>(log_.size()) ? log_[static_cast<size_t>(start + i)] : "");

    line(std::string(ansi::GRAY) + "WASD/arrows move   . wait   g pickup   i inventory   ? help   q save & quit" + ansi::RESET);
    out += "\033[J";
    return out;
}

// ================================================================ screens
char Game::titleScreen() {
    bool hasSave = false;
    { std::ifstream f(SAVE_FILE); hasSave = f.good(); }
    term::write(std::string("\033[H\033[2J") + ansi::BCYAN +
        "\n  =============================================\n"
        "        R O G U E L I K E   D U N G E O N\n"
        "  =============================================\n\n" + ansi::RESET +
        "  Explore five procedurally generated floors, fight\n"
        "  monsters, collect gear, and slay the Dragon.\n\n"
        "  Seed: " + std::to_string(seed_) + "\n\n"
        "  [N] New game\n" +
        (hasSave ? "  [C] Continue saved game\n" : "") +
        "  [Q] Quit\n");
    while (true) {
        int k = term::readKey();
        if (k == term::KEY_EOF) return 'q';
        k = std::tolower(k);
        if (k == 'n' || k == 'q') return static_cast<char>(k);
        if (k == 'c' && hasSave) return 'c';
    }
}

void Game::helpScreen() {
    term::write(std::string("\033[H\033[2J") + ansi::BCYAN + "\n  HELP\n\n" + ansi::RESET +
        "  Move / attack : WASD or arrow keys (walk into a monster to hit it)\n"
        "  Wait a turn   : . or space\n"
        "  Pick up item  : g  (items are also picked up automatically when you step on them)\n"
        "  Inventory     : i   (1-9 use/equip, d + number to drop)\n"
        "  Save and quit : q\n\n"
        "  @ you   > stairs   / weapon   [ armor   ! potion\n"
        "  g Goblin (chases)      a Archer (shoots from range)\n"
        "  T Troll (sleeps, slow, hits hard)   S Spider (poison)\n"
        "  D Dragon (final boss, floor 5)\n\n"
        "  Item colours: white common, cyan rare, magenta epic.\n"
        "  Dark-grey tiles are remembered; you only see monsters in line of sight.\n\n"
        "  Press any key...");
    term::readKey();
}

void Game::inventoryMenu() {
    while (true) {
        std::ostringstream o;
        o << "\033[H\033[2J" << ansi::BCYAN << "\n  INVENTORY" << ansi::RESET << "\n\n  Weapon: "
          << (player_.weapon() ? coloredItem(*player_.weapon()) : "none") << "\n  Armor : "
          << (player_.armor() ? coloredItem(*player_.armor()) : "none") << "\n\n";
        const auto& inv = player_.inventory();
        if (inv.empty()) o << "  (pack is empty)\n";
        for (size_t i = 0; i < inv.size(); ++i) o << "  " << (i + 1) << ") " << coloredItem(*inv[i]) << '\n';
        o << "\n  1-9 use/equip   d drop   i or Esc close\n";
        term::write(o.str());

        int k = term::readKey();
        if (k == term::KEY_EOF || k == term::KEY_ESC || k == 'i' || k == 'q') return;
        if (k >= '1' && k <= '9') {
            if (useItem(static_cast<size_t>(k - '1'))) return;
        } else if (k == 'd') {
            term::write("  Drop which item (1-9)? ");
            int n = term::readKey();
            if (n >= '1' && n <= '9' && dropItem(static_cast<size_t>(n - '1'))) return;
        }
    }
}

// ================================================================= main loop
int Game::run() {
    term::init();
    char choice = titleScreen();
    if (choice == 'q') { term::shutdown(); return 0; }
    if (choice == 'c' && !loadFromFile()) log("Save file was unreadable; starting a new game.");

    int lastSavedFloor = floor_;
    while (state_ == State::Playing) {
        term::write(renderFrame());
        int k = term::readKey();
        Action a;
        if (k == term::KEY_EOF || k == 'q' || k == 'Q') { saveToFile(); state_ = State::Quit; }
        else if (keyToAction(k, a)) {
            act(a);
            if (state_ == State::Playing && floor_ != lastSavedFloor) { saveToFile(); lastSavedFloor = floor_; }
        }
        else if (k == 'i') inventoryMenu();
        else if (k == '?') helpScreen();
    }

    if (state_ != State::Quit) std::remove(SAVE_FILE);   // permadeath / victory: no stale save
    term::write(renderFrame());
    std::string msg;
    switch (state_) {
        case State::Won:
            msg = std::string(ansi::BGREEN) + "\n  *** VICTORY! *** You slew the Dragon on turn " + std::to_string(turn_) + ".";
            break;
        case State::Dead:
            msg = std::string(ansi::BRED) + "\n  *** YOU DIED *** on floor " + std::to_string(floor_) + ".";
            break;
        default:
            msg = std::string(ansi::BCYAN) + "\n  Game saved. See you next time!";
    }
    msg += std::string(ansi::RESET) + "  Level " + std::to_string(player_.level()) + ", " +
           std::to_string(kills_) + " kills, seed " + std::to_string(seed_) + ".\n  Press any key...";
    term::write(msg);
    term::readKey();
    term::shutdown();
    return 0;
}
