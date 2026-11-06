#pragma once
#include "Dungeon.h"
#include "Item.h"
#include "Monster.h"
#include "Player.h"
#include "World.h"
#include <cstdint>
#include <deque>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

enum class Action { Up, Down, Left, Right, Wait, Pickup };

struct GroundItem {
    Vec2 pos;
    std::unique_ptr<Item> item;
};

// Owns all game state and rules. Rules (act/useItem/...) are separate from
// presentation (renderFrame/run), so the whole game can be tested headlessly.
class Game : public World {
public:
    enum class State { Playing, Won, Dead, Quit };
    static constexpr int MAP_W = 60, MAP_H = 18, FINAL_FLOOR = 5, FOV_RADIUS = 8;

    explicit Game(uint32_t seed);

    int run();                              // interactive terminal loop

    // --- game rules (no I/O) ---
    bool act(Action a);                     // true if a turn was spent
    bool useItem(size_t index);
    bool dropItem(size_t index);

    State state() const { return state_; }
    int floor() const { return floor_; }
    int turn() const { return turn_; }
    int kills() const { return kills_; }
    uint32_t seed() const { return seed_; }
    const Player& player() const { return player_; }
    const std::vector<std::unique_ptr<Monster>>& monsters() const { return monsters_; }
    const std::vector<GroundItem>& groundItems() const { return ground_; }
    const std::deque<std::string>& messages() const { return log_; }

    void save(std::ostream& os) const;
    bool load(std::istream& is);            // all-or-nothing: false leaves state untouched
    bool saveToFile() const;
    bool loadFromFile();

    std::string renderFrame() const;

    // --- World interface (used by monsters) ---
    const Dungeon& dungeon() const override { return dungeon_; }
    Player& player() override { return player_; }
    bool occupied(Vec2 p, const Entity* ignore) const override;
    void log(const std::string& message) override;
    void attack(Entity& attacker, Entity& defender, const std::string& verb) override;

private:
    uint32_t seed_;
    Dungeon dungeon_;
    Player player_;
    std::vector<std::unique_ptr<Monster>> monsters_;
    std::vector<GroundItem> ground_;
    std::deque<std::string> log_;
    int floor_ = 1, turn_ = 0, kills_ = 0;
    State state_ = State::Playing;
    bool skipMonsters_ = false;

    void newGame();
    void generateFloor();
    Vec2 randomSpawn(size_t firstRoom, size_t lastRoom) const;
    bool tryMove(Vec2 dir);
    void pickupItems();
    bool pickupHere();
    void descend();
    void endTurn();
    void cleanupDead();
    void onDeath(Entity& e);
    void cellAt(Vec2 p, char& glyph, const char*& color) const;

    void inventoryMenu();
    void helpScreen();
    char titleScreen();
};
