#include "Entity.h"
#include <iostream>

void Entity::saveBase(std::ostream& os) const {
    os << pos_.x << ' ' << pos_.y << ' ' << hp_ << ' ' << maxHp_ << ' '
       << attack_ << ' ' << defense_ << ' ' << poison_;
}

bool Entity::loadBase(std::istream& is) {
    int x, y, hp, maxHp, atk, def, poison;
    if (!(is >> x >> y >> hp >> maxHp >> atk >> def >> poison)) return false;
    if (maxHp <= 0 || hp < 0 || hp > maxHp || poison < 0) return false;
    pos_ = {x, y};
    hp_ = hp; maxHp_ = maxHp; attack_ = atk; defense_ = def; poison_ = poison;
    return true;
}

namespace combat {
Hit roll(const Entity& a, const Entity& d) {
    int dmg = a.attackPower() - d.defensePower() + Rng::range(-1, 2);
    bool crit = Rng::chance(0.10);
    if (crit) dmg *= 2;
    return {std::max(1, dmg), crit};
}
}  // namespace combat
