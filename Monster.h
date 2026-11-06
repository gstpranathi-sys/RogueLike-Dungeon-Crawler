#pragma once
#include "Entity.h"
#include "World.h"
#include <iosfwd>
#include <memory>
#include <string>

// Base monster. act() is the polymorphic AI hook: each subclass behaves differently.
class Monster : public Entity {
protected:
    int xpReward_;
    int actEvery_;       // 1 = acts every turn, 2 = every other turn (slow)
    int tick_ = 0;
    bool awake_ = true;

    virtual void act(World& w) = 0;
    virtual const char* verb() const { return "hits"; }

    bool adjacentToPlayer(World& w) const;
    bool stepToward(World& w, Vec2 target);
    void chaseAndAttack(World& w);

public:
    Monster(std::string name, Vec2 pos, int hp, int atk, int def, char symbol,
            const char* color, int xp, int actEvery = 1);

    int xp() const { return xpReward_; }
    void takeTurn(World& w);
    virtual std::string typeId() const = 0;

    void save(std::ostream& os) const;
    static std::unique_ptr<Monster> load(std::istream& is);
    static std::unique_ptr<Monster> create(const std::string& typeId, Vec2 pos, int floor);
    static std::unique_ptr<Monster> randomForFloor(Vec2 pos, int floor);
};

class Goblin : public Monster {         // runs straight at you
public:
    explicit Goblin(Vec2 p);
    std::string typeId() const override { return "goblin"; }
protected:
    void act(World& w) override;
    const char* verb() const override { return "slashes"; }
};

class Archer : public Monster {         // slow; shoots from range, stabs when cornered
public:
    explicit Archer(Vec2 p);
    std::string typeId() const override { return "archer"; }
protected:
    void act(World& w) override;
    const char* verb() const override { return "shoots"; }
};

class Troll : public Monster {          // sleeps until you get close; slow but brutal
public:
    explicit Troll(Vec2 p);
    std::string typeId() const override { return "troll"; }
protected:
    void act(World& w) override;
    const char* verb() const override { return "smashes"; }
};

class Spider : public Monster {         // fast chaser whose bite poisons
public:
    explicit Spider(Vec2 p);
    std::string typeId() const override { return "spider"; }
protected:
    void act(World& w) override;
    const char* verb() const override { return "bites"; }
};

class Dragon : public Monster {         // final boss: melee plus fire breath
public:
    explicit Dragon(Vec2 p);
    std::string typeId() const override { return "dragon"; }
protected:
    void act(World& w) override;
    const char* verb() const override { return "claws"; }
};
