#pragma once
// ANSI colour codes (work on Linux/macOS terminals and Windows 10+ consoles).
namespace ansi {
inline constexpr const char* RESET    = "\033[0m";
inline constexpr const char* RED      = "\033[31m";
inline constexpr const char* GREEN    = "\033[32m";
inline constexpr const char* YELLOW   = "\033[33m";
inline constexpr const char* MAGENTA  = "\033[35m";
inline constexpr const char* CYAN     = "\033[36m";
inline constexpr const char* WHITE    = "\033[37m";
inline constexpr const char* GRAY     = "\033[90m";
inline constexpr const char* BRED     = "\033[91m";
inline constexpr const char* BGREEN   = "\033[92m";
inline constexpr const char* BYELLOW  = "\033[93m";
inline constexpr const char* BMAGENTA = "\033[95m";
inline constexpr const char* BCYAN    = "\033[96m";
}
