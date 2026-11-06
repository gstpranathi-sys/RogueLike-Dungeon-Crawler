#pragma once
#include <string>

// Thin cross-platform layer: raw single-key input (no Enter needed) and
// ANSI output. Everything platform-specific lives in Terminal.cpp.
namespace term {
constexpr int KEY_EOF = -1, KEY_ESC = 27;
constexpr int KEY_UP = 1001, KEY_DOWN = 1002, KEY_LEFT = 1003, KEY_RIGHT = 1004;

void init();                       // raw mode, alternate screen, hidden cursor
void shutdown();                   // restores everything (also runs at exit)
int readKey();                     // blocks for one key press
void write(const std::string& s);  // write + flush
}
