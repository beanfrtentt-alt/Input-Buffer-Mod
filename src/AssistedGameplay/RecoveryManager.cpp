/**
 * SpoonClient / AssistedGameplay/RecoveryManager.cpp
 *
 * Implementation note: the "level.frame decreases" detection is fragile
 * against GD 2.2074's experimental replay system, where frame can briefly
 * rewind. We tolerate rewinds of up to 4 frames as "noise" and only
 * treat a rewind of >60 frames (>= 0.25 s) as a real restart.
 */
#include "RecoveryManager.hpp"
#include <string_view>

namespace spoon::ag {

RecoveryManager& RecoveryManager::get() {
    static RecoveryManager instance;
    return instance;
}

RecoveryManager::RecoveryManager() = default;

void RecoveryManager::reset() {
    m_lastGamemode[0] = m_lastGamemode[1] = Gamemode::Unknown;
    m_lastFrame      = -1;
    m_wasDead[0]     = m_wasDead[1] = false;
    m_dualWasActive  = false;
    m_lastEvent     = Event::None;
}

RecoveryManager::Event RecoveryManager::observe(const Snapshot& snap) {
    Event ev = Event::None;

    // --- Level-exit detection (highest priority) ---
    if (snap.level.levelExited) {
        ev = Event::LevelExited;
        m_lastEvent = ev;
        return ev;
    }

    // --- Death / respawn ---
    bool p1DeadNow = snap.player[0].isDead;
    if (p1DeadNow && !m_wasDead[0]) {
        ev = Event::PlayerDied;
    } else if (!p1DeadNow && m_wasDead[0]) {
        ev = Event::PlayerRespawned;
    }
    m_wasDead[0] = p1DeadNow;
    m_wasDead[1] = snap.player[1].isDead;

    // --- Level restart detection (frame went backwards by a meaningful amount) ---
    if (m_lastFrame > 0 && snap.level.frame > 0 &&
        m_lastFrame - snap.level.frame > 60) {
        ev = Event::LevelRestart;
    }
    m_lastFrame = snap.level.frame;

    // --- Gamemode change detection ---
    for (int slot = 0; slot < 2; ++slot) {
        Gamemode now = snap.player[slot].gamemode;
        if (m_lastGamemode[slot] != Gamemode::Unknown &&
            m_lastGamemode[slot] != now) {
            ev = Event::GamemodeChanged;
        }
        m_lastGamemode[slot] = now;
    }

    // --- Dual mode entry detection ---
    bool dualNow = snap.level.dualActive;
    if (dualNow && !m_dualWasActive) {
        // Entering dual mode is treated as a "gamemode change" event from
        // AG's perspective — we want to flush and re-plan because the
        // planner now needs to coordinate two players.
        ev = Event::GamemodeChanged;
    }
    m_dualWasActive = dualNow;

    m_lastEvent = ev;
    return ev;
}

std::string_view RecoveryManager::lastEventLabel() const {
    switch (m_lastEvent) {
        case Event::None:            return "None";
        case Event::PlayerDied:      return "Player died";
        case Event::PlayerRespawned: return "Player respawned";
        case Event::LevelRestart:    return "Level restarted";
        case Event::LevelExited:     return "Level exited";
        case Event::GamemodeChanged: return "Gamemode changed";
    }
    return "Unknown";
}

} // namespace spoon::ag
