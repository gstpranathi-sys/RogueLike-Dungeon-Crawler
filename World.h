#pragma once
#include "Dungeon.h"
#include "Entity.h"
#include <string>

class Player;

// What a monster is allowed to know about / do to the game. Monsters depend on
// this interface, not on Game itself (dependency inversion): easy to test and
// no circular includes.
class World {
public:
    virtual ~World() = default;
    virtual const Dungeon& dungeon() const = 0;
    virtual Player& player() = 0;
    virtual bool occupied(Vec2 p, const Entity* ignore) const = 0;
    virtual void log(const std::string& message) = 0;
    virtual void attack(Entity& attacker, Entity& defender, const std::string& verb) = 0;
};
