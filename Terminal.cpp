#include "Terminal.h"
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace term {

static bool g_active = false;
#ifndef _WIN32
static termios g_orig;
static bool g_raw = false;
#endif

void write(const std::string& s) {
    std::fwrite(s.data(), 1, s.size(), stdout);
    std::fflush(stdout);
}

void shutdown() {
    if (!g_active) return;
    g_active = false;
    write("\033[0m\033[?25h\033[?1049l");
#ifndef _WIN32
    if (g_raw) tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig);
#endif
}

void init() {
    if (g_active) return;
    g_active = true;
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &g_orig) == 0) {
        termios raw = g_orig;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        g_raw = true;
    }
#endif
    write("\033[?1049h\033[?25l");   // alternate screen + hide cursor
    std::atexit(shutdown);
}

int readKey() {
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        switch (_getch()) {
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 75: return KEY_LEFT;
            case 77: return KEY_RIGHT;
        }
        return 0;
    }
    return c;
#else
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return KEY_EOF;
    if (c != 27) return c;
    // Possible arrow-key escape sequence: ESC [ A/B/C/D
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{0, 30000};
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) return KEY_ESC;
    unsigned char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) <= 0) return KEY_ESC;
    if (seq[0] != '[' && seq[0] != 'O') return KEY_ESC;
    if (read(STDIN_FILENO, &seq[1], 1) <= 0) return KEY_ESC;
    switch (seq[1]) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
    }
    return 0;
#endif
}

}  // namespace term
