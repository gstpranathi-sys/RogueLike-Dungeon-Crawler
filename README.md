# Roguelike Dungeon Crawler (C++17, no external libraries)

A turn-based, console roguelike: five procedurally generated floors, fog of war,
five monster types with distinct AI, loot with rarities, equipment, poison, a final
boss, and save/load. Pure standard C++ (one small platform layer for raw key input).

## Build / run / test
    make            # build ./roguelike
    make run        # build and play
    make test       # unit tests + whole-game fuzz test (bot plays 60 games, checks invariants every turn)
    make balance    # simulate 300 games with a "sensible" bot and print win rate / where it dies
    ./roguelike --seed 42     # reproduce an exact dungeon (the seed is shown on the title/end screens)

Needs g++/clang with C++17. Windows: build with MinGW-w64 (`g++ -std=c++17 -Iinclude src/*.cpp -o roguelike.exe`)
in Windows Terminal / Windows 10+ console (ANSI colours are enabled automatically). The
Windows input path (`_getch`) is written but has NOT been tested by me; Linux/macOS are the tested paths.
Terminal should be at least 80x26.

## Controls
    WASD / arrow keys   move; walk into a monster to attack it
    .  or  space        wait a turn
    g                   pick up item under you (items are also picked up automatically when stepping on them)
    i                   inventory: 1-9 use/equip, d+number drop, Esc close
    ?                   help        q  save and quit

## Gameplay
- Reach floor 5 and kill the Dragon to win. Death is permanent (the save is deleted).
- Fog of war: you only see monsters/items in line of sight; explored terrain stays dimly remembered.
- Gear: weapons (+attack), armor (+defense), potions. Rarity by colour: white common, cyan rare, magenta epic. Deeper floors drop better loot.
- Equipping swaps with what you wear (the old item goes back to your pack). Potions are not wasted at full HP.
- Monsters: `g` Goblin (chases) - `a` Skeleton Archer (slow, shoots from range) - `T` Cave Troll (sleeps, slow, hits hard) -
  `S` Giant Spider (bite poisons) - `D` Dragon (boss: claws + fire breath).
- Monsters path-find around walls (BFS) and block each other.

## Architecture (what to talk about in your viva)
    Types.h / Rng.h / Ansi.h   small utilities; Rng is seedable so every run is reproducible
    Entity  (abstract-ish base) stats, damage, poison; virtual attackPower()/defensePower()
      Player                    inventory, equipment slots, XP/levels; overrides stats to add gear bonuses
      Monster (abstract)        virtual act() = the AI hook; template-method takeTurn() handles speed
        Goblin / Archer / Troll / Spider / Dragon     each overrides act(): polymorphism
    Item (abstract)             Weapon / Armor / Potion; factory methods create()/random()/load()
    Dungeon                     room+corridor generation, Bresenham line of sight, BFS pathfinding
    World (interface)           what monsters may see/do; Game implements it (dependency inversion)
    Game                        rules + state; rendering and the terminal loop are separate from the rules
    Terminal                    the only platform-specific code (raw input, ANSI)

OOP/C++ concepts demonstrated: inheritance, polymorphism (virtual functions), abstraction (pure virtual
base classes, `World` interface), encapsulation, composition (Game owns Dungeon/Player/monsters), RAII and
`std::unique_ptr` ownership (no raw `new`/`delete`), move semantics (items move between pack/equipment/floor),
factory + template-method patterns, `const` correctness, `std::optional`, `std::function`, lambdas, STL algorithms.

## Testing & quality
- `tests/tests.cpp`: 33,000+ assertions: combat math, 300 generated dungeons are connected with solid borders,
  pathfinding/line-of-sight, item and monster serialisation round-trips, equip/drink/capacity rules, levelling,
  pickup/drop, and an exact save->load->save byte-for-byte round trip. Corrupt/truncated saves are rejected without
  touching the running game.
- A fuzz bot plays 60 whole games and checks invariants after *every* turn (nobody inside a wall, no two things
  on one tile, HP in range, pack size limit...).
- Built and run clean under AddressSanitizer + UBSan (`-fsanitize=address,undefined`).
- `make balance`: with the shipped numbers a competent bot (loots, equips upgrades, rests, drinks potions) wins
  about 70% of runs and nearly all deaths happen at the Dragon. Humans will do worse than the bot, so expect a
  real challenge. Tune stats in `Monster.cpp` / `Item.cpp` / `Player.cpp` and re-run `make balance`.

## Known limits (honest list)
- Balance was tuned against a bot, not human playtesters.
- One save slot (`savegame.dat` in the working directory). Save format is versioned (`ROGUESAVE 2`).
- RNG state is not saved; after loading, the RNG is re-seeded from (seed, turn) so play is deterministic but
  not identical to an uninterrupted run.
- No unequip command (swap gear by equipping something else), no stairs up.

## Ideas to extend (good for your team's split)
Dungeon: vaults/secret rooms, traps, themed floors. Monsters: more types, group AI, status effects on the player.
Items: unequip, scrolls, identify mechanic. UI: message colours by category, minimap, a high-score file.
