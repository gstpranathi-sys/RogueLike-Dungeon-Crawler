#include "Game.h"
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

int main(int argc, char** argv) {
    uint32_t seed = std::random_device{}();
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else {
            std::cout << "Usage: roguelike [--seed N]\n"
                         "  --seed N   reproduce a specific dungeon (same seed = same floors)\n";
            return arg == "--help" ? 0 : 1;
        }
    }
    Game game(seed);
    return game.run();
}
