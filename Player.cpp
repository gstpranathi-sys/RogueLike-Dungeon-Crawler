#include "Player.h"
#include "Ansi.h"
#include <iostream>

Player::Player(Vec2 pos) : Entity("Hero", pos, 40, 6, 1, '@', ansi::BGREEN) {}

int Player::attackPower() const  { return attack_ + (weapon_ ? weapon_->value() : 0); }
int Player::defensePower() const { return defense_ + (armor_ ? armor_->value() : 0); }

int Player::gainXp(int amount) {
    xp_ += amount;
    int gained = 0;
    while (xp_ >= xpToNext()) {
        xp_ -= xpToNext();
        ++level_;
        ++gained;
        maxHp_ += 8;
        hp_ = std::min(maxHp_, hp_ + maxHp_ / 2);   // level-up heals half
        attack_ += 1;
        if (level_ % 2 == 0) defense_ += 1;
    }
    return gained;
}

bool Player::addItem(std::unique_ptr<Item>& item) {
    if (!item || inv_.size() >= MAX_ITEMS) return false;
    inv_.push_back(std::move(item));
    return true;
}

UseResult Player::useItem(size_t index) {
    UseResult r;
    if (index >= inv_.size()) return r;
    r.valid = true;
    Item& item = *inv_[index];

    if (item.kind() == ItemKind::Potion) {
        int healed = heal(item.value());
        if (healed == 0) {
            r.message = "You're already at full health.";
            return r;
        }
        r.message = std::string("You drink the ") + item.name() + " and recover " +
                    std::to_string(healed) + " HP.";
        inv_.erase(inv_.begin() + static_cast<long>(index));
        r.tookTurn = true;
        return r;
    }

    // Equipment: swap with whatever is currently worn so nothing is lost
    // (and the pack never overflows).
    std::unique_ptr<Item>& slot = item.kind() == ItemKind::Weapon ? weapon_ : armor_;
    std::string verb = item.kind() == ItemKind::Weapon ? "wield" : "put on";
    std::string name = item.name();
    std::swap(slot, inv_[index]);
    if (!inv_[index]) inv_.erase(inv_.begin() + static_cast<long>(index));
    r.message = "You " + verb + " the " + name + ".";
    r.tookTurn = true;
    return r;
}

std::unique_ptr<Item> Player::dropItem(size_t index) {
    if (index >= inv_.size()) return nullptr;
    std::unique_ptr<Item> item = std::move(inv_[index]);
    inv_.erase(inv_.begin() + static_cast<long>(index));
    return item;
}

void Player::save(std::ostream& os) const {
    saveBase(os);
    os << ' ' << level_ << ' ' << xp_ << '\n';
    for (const auto* slot : {weapon_.get(), armor_.get()}) {
        os << (slot ? 1 : 0) << '\n';
        if (slot) slot->save(os);
    }
    os << inv_.size() << '\n';
    for (const auto& it : inv_) it->save(os);
}

bool Player::load(std::istream& is) {
    if (!loadBase(is)) return false;
    if (!(is >> level_ >> xp_) || level_ < 1 || xp_ < 0) return false;
    inv_.clear();
    weapon_.reset();
    armor_.reset();
    for (auto* slot : {&weapon_, &armor_}) {
        int has;
        if (!(is >> has)) return false;
        if (has) {
            *slot = Item::load(is);
            if (!*slot) return false;
        }
    }
    size_t n;
    if (!(is >> n) || n > MAX_ITEMS) return false;
    for (size_t i = 0; i < n; ++i) {
        auto it = Item::load(is);
        if (!it) return false;
        inv_.push_back(std::move(it));
    }
    return true;
}
