#pragma once
#include <cstdint>
#include <random>

// One seedable random source for the whole game, so any run can be
// reproduced from its seed (great for debugging and for tests).
class Rng {
public:
    static void seed(uint32_t s) { seed_ = s; engine() = std::mt19937(s); }
    static uint32_t seed() { return seed_; }
    static int range(int lo, int hi) {            // inclusive both ends
        std::uniform_int_distribution<int> d(lo, hi);
        return d(engine());
    }
    static bool chance(double p) {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        return d(engine()) < p;
    }
private:
    static std::mt19937& engine() { static std::mt19937 e(std::random_device{}()); return e; }
    static inline uint32_t seed_ = 0;
};
