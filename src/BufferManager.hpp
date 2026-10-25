/**
 * SpoonClient / BufferManager.hpp
 *
 * Per-gamemode input buffer. The buffer is the *only* place in the mod that
 * decides when a queued input becomes a real GD input — both the human
 * (via our Keyboard hook) and Assisted Gameplay (via InputController) push
 * inputs into the buffer; the buffer, configured per-gamemode, decides when
 * and how to release them.
 *
 * Design goals
 * -------------
 *  * Each gamemode has its own `BufferProfile` (enabled + frame count).
 *  * The buffer state lives independently of any player object so it can
 *    survive gamemode transitions, deaths, and dual-mode swaps cleanly.
 *  * The runtime buffer has TWO slots: one for player 1, one for player 2
 *    (dual mode). These are independent — a Cube-P1 + Ship-P2 dual keeps
 *    separate buffers.
 *  * Inputs are timestamped against the gameplay frame counter rather than
 *    wall-clock time, so behavior is deterministic across framerates.
 *
 * Lifecycle
 * ----------
 *  * `onLevelStart()`     — clears all buffer state, loads the active profile
 *  * `onGamemodeChange()` — flushes the active gamemode's pending buffers,
 *                           then loads the new profile's buffer for that
 *                           player slot
 *  * `onDeath()`          — discards every pending input
 *  * `onLevelRestart()`   — same as onLevelStart()
 *  * `onLevelComplete()`  — flushes everything
 *
 * Interaction with the actual GD input pipeline
 * ----------------------------------------------
 * The buffer does NOT call `PlayerObject::pushButton` directly. Instead, when
 * the buffer drains, it sets a pending "input request" that our
 * `PlayerObjectHook::pushButtonHook` consumes. This guarantees that:
 *   1) Game state is always consistent — the input is processed during GD's
 *      normal update tick, not at arbitrary buffer-drain moments.
 *   2) Assisted Gameplay and the human player share the exact same input
 *      path — no inputs get "stuck" because they were pushed via the wrong
 *      API.
 *   3) Buffer behavior is testable without a live PlayLayer.
 */
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "Utils.hpp"
#include "SpoonSettings.hpp"

namespace spoon {

struct BufferedInput {
    // Frame at which the input was queued (gameplay frame counter from
    // GJBaseGameLayer::m_gameState.m_currentProgress / step counter).
    int64_t  queuedAtFrame   = 0;
    // True if the input is a "hold" (continuous, used by ship/ufo/wave) vs
    // a "tap" (discrete click used by cube/ball/robot/spider/swing).
    bool     isHold          = false;
    // Player slot: false = P1, true = P2 (dual mode).
    bool     isPlayer2       = false;
    // How many frames the input has already been buffered for (decremented
    // per frame; drained when <= 0 OR when a release happens after a release
    // window elapses for hold-style inputs).
    int      remainingFrames = 0;
};

class BufferManager {
public:
    static BufferManager& get();

    // Called by SpoonClientHooks at key lifecycle events. Safe to call even
    // when not currently in a level.
    void onLevelStart();
    void onLevelRestart();
    void onLevelComplete();
    void onDeath(bool player2 = false);

    // Player gamemode transition (called from PlayerObjectHook::gamemodeChanged
    // via PlayerObject mode flags).
    void onGamemodeChange(bool player2, Gamemode newMode);

    // ----- Input queue API ----------------------------------------------------
    // Push an input from either the keyboard hook (human) or InputController
    // (Assisted Gameplay). `isHold` selects tap-vs-hold semantics; the actual
    // gamemode-specific behavior (e.g., ship continuous-hold) is determined
    // by the active BufferProfile.
    void queueInput(bool isPlayer2, bool isHold, int64_t currentFrame);

    // Drain one tick of the buffer. Called every gameplay frame from the
    // PlayLayerHook. Returns the input(s) that the hook should forward to
    // PlayerObject this tick. Multiple inputs in a tick is rare but legal
    // (e.g., dual mode holding P1 + pressing P2 simultaneously).
    struct DrainResult {
        bool p1Tap   = false;
        bool p1Hold  = false;
        bool p2Tap   = false;
        bool p2Hold  = false;
        bool p1Release = false;
        bool p2Release = false;
    };
    // `p1Ready` / `p2Ready` are jump opportunities for tap gamemodes
    // (typically PlayerObject::m_isOnGround). Hold gamemodes ignore this
    // and apply as soon as the input is queued.
    DrainResult drain(int64_t currentFrame, bool p1Ready = true, bool p2Ready = true);

    bool wasApplied(bool isPlayer2) const {
        return m_applied[isPlayer2 ? 1 : 0];
    }

    // Explicit release (called when the user releases the key or AG stops).
    void releaseInput(bool isPlayer2, int64_t currentFrame);

    // Clear all pending inputs immediately. Used by AG emergency stop and
    // by RecoveryManager when the level state goes inconsistent.
    void clearAll();

    // Snapshot for the F6 menu.
    Gamemode activeGamemode(bool player2) const {
        return m_activeGamemode[player2 ? 1 : 0];
    }
    bool isEnabled(bool player2) const {
        return m_profiles[player2 ? 1 : 0].enabled;
    }
    int  bufferFrames(bool player2) const {
        return m_profiles[player2 ? 1 : 0].frames;
    }

    // The number of currently-pending buffered inputs. For UI display.
    int  pendingCount() const { return m_pendingInputs[0].has_value() +
                                       m_pendingInputs[1].has_value(); }

private:
    BufferManager();

    // Reload the active BufferProfile for the given player slot. Called when
    // the gamemode changes or when the F6 menu updates a profile.
    void reloadProfile(bool player2);

    std::array<BufferProfile, 2> m_profiles;
    std::array<Gamemode, 2>       m_activeGamemode;
    std::array<std::optional<BufferedInput>, 2> m_pendingInputs;
    std::array<bool, 2>           m_applied { false, false };
};

} // namespace spoon
