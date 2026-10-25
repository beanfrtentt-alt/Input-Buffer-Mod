/**
 * SpoonClient / BufferManager.cpp
 *
 * The runtime per-gamemode buffer. The state machine below is the heart of
 * the mod — every input the player presses (or that Assisted Gameplay
 * schedules) flows through here, and the buffer decides when to release
 * it into the actual GD input pipeline.
 *
 * The "buffer frame" timing
 * -------------------------
 * GD's update loop runs at ~240 Hz on most systems; that's the unit the
 * buffer counts in. A `frames` of 8 means the input will linger up to 8
 * gameplay ticks waiting for a clean place to apply. If 8 frames elapse
 * with no jump opportunity, the input is dropped silently so the next
 * press is not delayed indefinitely.
 *
 * Hold vs Tap
 * -----------
 * Tap gamemodes (Cube, Ball, Robot, Spider, Swing) consume the input the
 * instant a press is possible. Hold gamemodes (Ship, UFO, Wave) are special:
 * a buffered input means "begin holding for the next X frames" rather than
 * "tap once as soon as possible". For those we drain the input as a
 * continuous-hold request whose duration is governed by the gamemode's
 * natural physics, NOT by the buffer size.
 *
 * Death handling
 * --------------
 * On death, every pending input is discarded. This prevents a stale input
 * from auto-triggering on respawn, which is the single most common source of
 * "stuck input" bugs across GD bot mods. The respawn invulnerability window
 * (about 60 frames) gives the buffer time to re-acquire stable state without
 * consuming inputs from before the death.
 */
#include "BufferManager.hpp"
#include "SpoonSettings.hpp"
#include <algorithm>

namespace spoon {

BufferManager& BufferManager::get() {
    static BufferManager instance;
    return instance;
}

BufferManager::BufferManager() {
    m_activeGamemode = { Gamemode::Cube, Gamemode::Cube };
    for (int i = 0; i < 2; ++i)
        m_profiles[i] = Settings::get().profile(Gamemode::Cube);
}

void BufferManager::onLevelStart() {
    m_pendingInputs[0].reset();
    m_pendingInputs[1].reset();
    reloadProfile(false);
    reloadProfile(true);
}

void BufferManager::onLevelRestart() { onLevelStart(); }
void BufferManager::onLevelComplete() {
    m_pendingInputs[0].reset();
    m_pendingInputs[1].reset();
}

void BufferManager::onDeath(bool player2) {
    // Discard the buffer for the player that died. In dual mode, if only one
    // player died, the other player's buffer is left untouched — they may
    // have a legitimate queued input that needs to fire while the dead player
    // is respawning.
    m_pendingInputs[player2 ? 1 : 0].reset();
}

void BufferManager::onGamemodeChange(bool player2, Gamemode newMode) {
    if (newMode == Gamemode::Unknown) return;
    int slot = player2 ? 1 : 0;
    if (m_activeGamemode[slot] == newMode) return;

    // A gamemode transition flushes the buffer for the previous gamemode.
    // Holding a tap input across a portal is a recipe for unwanted double
    // jumps; safer to drop the buffered input at the portal frame.
    m_pendingInputs[slot].reset();

    m_activeGamemode[slot] = newMode;
    reloadProfile(player2);
}

void BufferManager::reloadProfile(bool player2) {
    int slot = player2 ? 1 : 0;
    m_profiles[slot] = Settings::get().profile(m_activeGamemode[slot]);
}

void BufferManager::queueInput(bool isPlayer2, bool isHold, int64_t currentFrame) {
    int slot = isPlayer2 ? 1 : 0;
    const auto& p = m_profiles[slot];
    if (!p.enabled || p.frames <= 0) {
        // No buffering — the hook will still see a fresh drain next tick.
        m_pendingInputs[slot] = BufferedInput{
            .queuedAtFrame   = currentFrame,
            .isHold          = isHold,
            .isPlayer2       = isPlayer2,
            .remainingFrames = 0
        };
        return;
    }
    // Replace any previously-buffered input. Two queued inputs in the same
    // window would otherwise compound and lead to dropped jumps.
    m_pendingInputs[slot] = BufferedInput{
        .queuedAtFrame   = currentFrame,
        .isHold          = isHold,
        .isPlayer2       = isPlayer2,
        .remainingFrames = p.frames
    };
}

BufferManager::DrainResult BufferManager::drain(int64_t currentFrame) {
    DrainResult r;
    for (int slot = 0; slot < 2; ++slot) {
        auto& opt = m_pendingInputs[slot];
        if (!opt) continue;
        auto& in = *opt;
        // Drop the input if the buffer has fully expired. We use 0 (not -1)
        // as the floor so that an input set with remainingFrames==0 still
        // drains exactly once on the next drain() call.
        if (in.remainingFrames < 0) {
            opt.reset();
            continue;
        }
        // Apply the input. Taps consume the input immediately; holds mark
        // the slot as "currently held" until releaseInput is called.
        if (in.isHold) {
            if (slot == 0) r.p1Hold = true; else r.p2Hold = true;
        } else {
            if (slot == 0) r.p1Tap = true; else r.p2Tap = true;
        }
        // Decrement the buffer counter. The input remains in the queue for
        // hold inputs (so subsequent drains keep the hold active) but is
        // consumed for tap inputs after one drain.
        if (!in.isHold) {
            opt.reset();
        } else {
            --in.remainingFrames;
            // Auto-release if the buffer has fully expired on a hold input
            // that was never released. This bounds the duration of a hold
            // that was scheduled by Assisted Gameplay.
            if (in.remainingFrames < 0) {
                if (slot == 0) r.p1Release = true; else r.p2Release = true;
                opt.reset();
            }
        }
    }
    return r;
}

void BufferManager::releaseInput(bool isPlayer2, int64_t /*currentFrame*/) {
    int slot = isPlayer2 ? 1 : 0;
    if (m_pendingInputs[slot] && m_pendingInputs[slot]->isHold) {
        // Mark for release on the next drain call.
        m_pendingInputs[slot]->remainingFrames = -1;
    } else {
        m_pendingInputs[slot].reset();
    }
}

void BufferManager::clearAll() {
    m_pendingInputs[0].reset();
    m_pendingInputs[1].reset();
}

} // namespace spoon
