#pragma once
#include "Rng.h"
#include "Types.h"
#include <algorithm>
#include <iosfwd>
#include <string>
#include <utility>

// Anything that lives on the map and can fight: Player and Monster derive from this.
class Entity {
protected:
    std::string name_;
    Vec2 pos_;
    int hp_, maxHp_, attack_, defense_;
    char symbol_;
    const char* color_;
    int poison_ = 0;

public:
    Entity(std::string name, Vec2 pos, int hp, int atk, int def, char symbol, const char* color)
        : name_(std::move(name)), pos_(pos), hp_(hp), maxHp_(hp), attack_(atk), defense_(def),
          symbol_(symbol), color_(color) {}
    Entity(const Entity&) = default;
    Entity& operator=(const Entity&) = default;
    Entity(Entity&&) = default;
    Entity& operator=(Entity&&) = default;
    virtual ~Entity() = default;

    const std::string& name() const { return name_; }
    Vec2 pos() const { return pos_; }
    void setPos(Vec2 p) { pos_ = p; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    char symbol() const { return symbol_; }
    const char* color() const { return color_; }
    bool alive() const { return hp_ > 0; }

    // Effective stats are virtual: Player adds equipment bonuses on top.
    virtual int attackPower() const { return attack_; }
    virtual int defensePower() const { return defense_; }

    void takeDamage(int amount) { hp_ = std::max(0, hp_ - std::max(0, amount)); }
    int heal(int amount) {                       // returns HP actually restored
        int before = hp_;
        hp_ = std::min(maxHp_, hp_ + std::max(0, amount));
        return hp_ - before;
    }
    void boost(int hpBonus, int atkBonus, int defBonus) {
        maxHp_ += hpBonus; hp_ += hpBonus; attack_ += atkBonus; defense_ += defBonus;
    }

    void addPoison(int turns) { poison_ = std::max(poison_, turns); }
    bool poisoned() const { return poison_ > 0; }
    int tickPoison() {                           // returns damage dealt this tick
        if (poison_ <= 0) return 0;
        --poison_;
        takeDamage(1);
        return 1;
    }

    void saveBase(std::ostream& os) const;
    bool loadBase(std::istream& is);
};

namespace combat {
struct Hit { int damage; bool crit; };
// attack - defense, small random swing, 10% critical (double damage), minimum 1.
Hit roll(const Entity& attacker, const Entity& defender);
}
