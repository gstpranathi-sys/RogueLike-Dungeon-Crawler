#pragma once
#include <iosfwd>
#include <memory>
#include <string>

enum class Rarity { Common = 0, Rare = 1, Epic = 2 };
enum class ItemKind { Weapon = 0, Armor = 1, Potion = 2 };

// Abstract item. Subclasses supply name/symbol/effect; the rest (colour by
// rarity, serialisation, random generation) is shared.
class Item {
protected:
    Rarity rarity_;
    int value_;   // attack bonus, defense bonus, or heal amount depending on kind
public:
    Item(Rarity r, int v) : rarity_(r), value_(v) {}
    virtual ~Item() = default;

    Rarity rarity() const { return rarity_; }
    int value() const { return value_; }
    virtual ItemKind kind() const = 0;
    virtual std::string name() const = 0;
    virtual char symbol() const = 0;
    virtual std::string effect() const = 0;
    virtual bool equippable() const { return false; }

    std::string describe() const { return name() + " (" + effect() + ")"; }
    const char* color() const;

    void save(std::ostream& os) const;
    static std::unique_ptr<Item> load(std::istream& is);          // nullptr on bad data
    static std::unique_ptr<Item> create(ItemKind k, Rarity r, int value);
    static std::unique_ptr<Item> random(int floor);                // better loot deeper down
};

class Weapon : public Item {
public:
    using Item::Item;
    ItemKind kind() const override { return ItemKind::Weapon; }
    std::string name() const override;
    char symbol() const override { return '/'; }
    std::string effect() const override { return "+" + std::to_string(value_) + " attack"; }
    bool equippable() const override { return true; }
};

class Armor : public Item {
public:
    using Item::Item;
    ItemKind kind() const override { return ItemKind::Armor; }
    std::string name() const override;
    char symbol() const override { return '['; }
    std::string effect() const override { return "+" + std::to_string(value_) + " defense"; }
    bool equippable() const override { return true; }
};

class Potion : public Item {
public:
    using Item::Item;
    ItemKind kind() const override { return ItemKind::Potion; }
    std::string name() const override;
    char symbol() const override { return '!'; }
    std::string effect() const override { return "heals " + std::to_string(value_) + " HP"; }
};
