#pragma once
#include "Entity.h"
#include "Item.h"
#include <memory>
#include <string>
#include <vector>

struct UseResult {
    bool valid = false;      // false = bad inventory index
    bool tookTurn = false;   // false = nothing happened (e.g. potion at full HP)
    std::string message;
};

class Player : public Entity {
public:
    static constexpr size_t MAX_ITEMS = 9;
    explicit Player(Vec2 pos);

    int attackPower() const override;     // base + weapon
    int defensePower() const override;    // base + armor

    int level() const { return level_; }
    int xp() const { return xp_; }
    int xpToNext() const { return xpForLevel(level_); }
    static int xpForLevel(int level) { return 20 + 15 * (level - 1); }
    int gainXp(int amount);               // returns number of levels gained

    bool addItem(std::unique_ptr<Item>& item);        // moves item in on success
    UseResult useItem(size_t index);                  // equip (swapping) or drink
    std::unique_ptr<Item> dropItem(size_t index);     // nullptr if bad index

    const std::vector<std::unique_ptr<Item>>& inventory() const { return inv_; }
    const Item* weapon() const { return weapon_.get(); }
    const Item* armor() const { return armor_.get(); }

    void save(std::ostream& os) const;
    bool load(std::istream& is);

private:
    int level_ = 1;
    int xp_ = 0;
    std::vector<std::unique_ptr<Item>> inv_;
    std::unique_ptr<Item> weapon_, armor_;
};
