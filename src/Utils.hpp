/**
 * SpoonClient / Utils.hpp
 *
 * Tiny shared utilities: gamemode enum, gamemode-to-string, clamp helpers,
 * and a debug-logging macro controlled by the `agShowDebug` setting.
 */
#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>

namespace spoon {

// Internal enum used by Spoon Client. Derive the active mode from the
// PlayerObject mode flags because the 2.2081 bindings do not expose m_gamemode.
enum class Gamemode : int {
    Cube   = 0,
    Ship   = 1,
    Ball   = 2,
    UFO    = 3,
    Wave   = 4,
    Robot  = 5,
    Spider = 6,
    Swing  = 7,
    Unknown = -1
};

inline std::string_view gamemodeName(Gamemode g) {
    switch (g) {
        case Gamemode::Cube:   return "Cube";
        case Gamemode::Ship:   return "Ship";
        case Gamemode::Ball:   return "Ball";
        case Gamemode::UFO:    return "UFO";
        case Gamemode::Wave:   return "Wave";
        case Gamemode::Robot:  return "Robot";
        case Gamemode::Spider: return "Spider";
        case Gamemode::Swing:  return "Swing";
        case Gamemode::Unknown: return "Unknown";
    }
    return "Unknown";
}

inline Gamemode gamemodeFromInt(int v) {
    if (v < 0 || v > 7) return Gamemode::Unknown;
    return static_cast<Gamemode>(v);
}

inline bool gamemodeSupportsBufferHold(Gamemode g) {
    // Hold-and-release buffer is meaningful for click-style gamemodes:
    // Cube, Ball, Robot, Spider, Swing use discrete clicks; Ship/UFO/Wave
    // use continuous hold input where buffering is naturally shorter.
    switch (g) {
        case Gamemode::Cube:
        case Gamemode::Ball:
        case Gamemode::Robot:
        case Gamemode::Spider:
        case Gamemode::Swing:
            return true;
        case Gamemode::Ship:
        case Gamemode::UFO:
        case Gamemode::Wave:
            return false;
        case Gamemode::Unknown:
            return false;
    }
    return false;
}

// Clamp a value into [lo, hi]. Used everywhere for input validation.
template <typename T>
constexpr T clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Integer power-of-two check (used by some chunking logic in LevelAnalyzer).
inline bool isPow2(uint32_t v) { return v && !(v & (v - 1)); }

// Convert a key string like "F6" / "F8" to its cocos2d KEY_* enum value.
// Keys live in <cocos2d/keyboard_types/KeyboardEvent.h> in modern Geode.
// Implementation in Utils.cpp.
} // namespace spoon

#ifdef SPOON_DEBUG
#  include <Geode/Loader.hpp>
#  define SPOON_LOG(...) ::geode::log::info("[SpoonClient] " __VA_ARGS__)
#  define SPOON_WARN(...) ::geode::log::warn("[SpoonClient] " __VA_ARGS__)
#  define SPOON_ERR(...)  ::geode::log::error("[SpoonClient] " __VA_ARGS__)
#else
#  define SPOON_LOG(...) ((void)0)
#  define SPOON_WARN(...) ::geode::log::warn("[SpoonClient] " __VA_ARGS__)
#  define SPOON_ERR(...)  ::geode::log::error("[SpoonClient] " __VA_ARGS__)
#endif
