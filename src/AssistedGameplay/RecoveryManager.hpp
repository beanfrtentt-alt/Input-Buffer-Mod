/**
 * SpoonClient / AssistedGameplay/RecoveryManager.hpp
 *
 * Watches the live snapshot stream from Manager and detects:
 *
 *   * Death            — `player.isDead` becomes true.
 *   * Respawn          — player.isDead flips back to false after a death.
 *   * Level Restart    — `level.frame` decreases (the gameplay tick went
 *                        backward, which only happens on a restart).
 *   * Level Exit       — PlayLayer becomes null or `level.levelExited` is set.
 *   * Gamemode Change  — BufferManager reloads the active profile and we
 *                        also reset the planner's "currently holding" state.
 *   * Dual-mode Entry  — P2 slot becomes active.
 *
 * On each event the planner's in-progress plan is invalidated and the
 * InputController is asked to flush. This prevents stale inputs from
 * being applied to a freshly-spawned player.
 */
#pragma once
#include "State.hpp"
#include <string_view>

namespace spoon::ag {

class RecoveryManager {
public:
    static RecoveryManager& get();

    // Feed the latest snapshot. Returns one of these enum values describing
    // what (if anything) was detected.
    enum class Event : uint8_t {
        None            = 0,
        PlayerDied      = 1,
        PlayerRespawned = 2,
        LevelRestart    = 3,
        LevelExited     = 4,
        GamemodeChanged = 5,
    };

    Event observe(const Snapshot& snap);

    // Reset internal bookkeeping — used when AG is freshly enabled so that
    // we don't misinterpret the first frame as a "gamemode change" or a
    // "respawn".
    void reset();

    // For UI display: last detected event, in human-readable form.
    std::string_view lastEventLabel() const;

private:
    RecoveryManager();
    Gamemode m_lastGamemode[2] = { Gamemode::Unknown, Gamemode::Unknown };
    int64_t  m_lastFrame       = -1;
    bool     m_wasDead[2]      = { false, false };
    bool     m_dualWasActive   = false;
    Event    m_lastEvent      = Event::None;
};

} // namespace spoon::ag
