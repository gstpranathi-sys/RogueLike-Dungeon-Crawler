#include "Item.h"
#include "Ansi.h"
#include "Rng.h"
#include <iostream>

namespace {
const char* const WEAPONS[] = {"Rusty Sword", "Steel Sword", "Runed Blade"};
const char* const ARMORS[]  = {"Leather Armor", "Chainmail", "Dragon Plate"};
const char* const POTIONS[] = {"Minor Potion", "Healing Potion", "Elixir of Life"};
}

std::string Weapon::name() const { return WEAPONS[static_cast<int>(rarity_)]; }
std::string Armor::name() const  { return ARMORS[static_cast<int>(rarity_)]; }
std::string Potion::name() const { return POTIONS[static_cast<int>(rarity_)]; }

const char* Item::color() const {
    switch (rarity_) {
        case Rarity::Rare: return ansi::BCYAN;
        case Rarity::Epic: return ansi::BMAGENTA;
        default: return ansi::WHITE;
    }
}

void Item::save(std::ostream& os) const {
    os << static_cast<int>(kind()) << ' ' << static_cast<int>(rarity_) << ' ' << value_ << '\n';
}

std::unique_ptr<Item> Item::create(ItemKind k, Rarity r, int value) {
    switch (k) {
        case ItemKind::Weapon: return std::make_unique<Weapon>(r, value);
        case ItemKind::Armor:  return std::make_unique<Armor>(r, value);
        case ItemKind::Potion: return std::make_unique<Potion>(r, value);
    }
    return nullptr;
}

std::unique_ptr<Item> Item::load(std::istream& is) {
    int k, r, v;
    if (!(is >> k >> r >> v)) return nullptr;
    if (k < 0 || k > 2 || r < 0 || r > 2 || v < 0 || v > 1000) return nullptr;
    return create(static_cast<ItemKind>(k), static_cast<Rarity>(r), v);
}

std::unique_ptr<Item> Item::random(int floor) {
    double roll = Rng::range(0, 999) / 1000.0;
    double epic = 0.03 + 0.02 * floor;
    double rare = 0.22 + 0.03 * floor;
    Rarity r = roll < epic ? Rarity::Epic : (roll < epic + rare ? Rarity::Rare : Rarity::Common);
    int ri = static_cast<int>(r);

    int k = Rng::range(0, 9);
    if (k < 3) {
        static const int base[] = {2, 4, 7};
        return create(ItemKind::Weapon, r, base[ri] + Rng::range(0, 1));
    }
    if (k < 6) {
        static const int base[] = {1, 2, 4};
        return create(ItemKind::Armor, r, base[ri] + Rng::range(0, 1));
    }
    static const int base[] = {12, 22, 40};
    return create(ItemKind::Potion, r, base[ri] + Rng::range(0, 4));
}
