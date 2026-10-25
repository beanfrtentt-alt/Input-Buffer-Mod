/**
 * SpoonClient / BufferManager.cpp
 *
 * Runtime per-gamemode buffer. Human jumps (via GJBaseGameLayer::handleButton)
 * and Assisted Gameplay (via InputController) both queue here. Drain decides
 * when those queued clicks become real GD inputs.
 *
 * Tap gamemodes (Cube, Ball, Robot, Spider, Swing): keep the click for up to
 * `frames` ticks and fire it on the first jump opportunity (on-ground). If
 * the window expires with no opportunity, the click is dropped.
 *
 * Hold gamemodes (Ship, UFO, Wave): start the hold as soon as it is queued
 * and keep it until releaseInput() or the buffer window expires.
 */
#include "BufferManager.hpp"
#include "SpoonSettings.hpp"

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
    m_applied[0] = m_applied[1] = false;
    reloadProfile(false);
    reloadProfile(true);
}

void BufferManager::onLevelRestart() { onLevelStart(); }
void BufferManager::onLevelComplete() {
    m_pendingInputs[0].reset();
    m_pendingInputs[1].reset();
    m_applied[0] = m_applied[1] = false;
}

void BufferManager::onDeath(bool player2) {
    m_pendingInputs[player2 ? 1 : 0].reset();
    m_applied[player2 ? 1 : 0] = false;
}

void BufferManager::onGamemodeChange(bool player2, Gamemode newMode) {
    if (newMode == Gamemode::Unknown) return;
    int slot = player2 ? 1 : 0;
    if (m_activeGamemode[slot] == newMode) return;

    m_pendingInputs[slot].reset();
    m_applied[slot] = false;
    m_activeGamemode[slot] = newMode;
    reloadProfile(player2);
}

void BufferManager::reloadProfile(bool player2) {
    int slot = player2 ? 1 : 0;
    m_profiles[slot] = Settings::get().profile(m_activeGamemode[slot]);
}

void BufferManager::queueInput(bool isPlayer2, bool isHold, int64_t currentFrame) {
    int slot = isPlayer2 ? 1 : 0;
    reloadProfile(isPlayer2);
    const auto& p = m_profiles[slot];
    m_applied[slot] = false;
    if (!p.enabled || p.frames <= 0) {
        m_pendingInputs[slot] = BufferedInput{
            .queuedAtFrame   = currentFrame,
            .isHold          = isHold,
            .isPlayer2       = isPlayer2,
            .remainingFrames = 0
        };
        return;
    }
    m_pendingInputs[slot] = BufferedInput{
        .queuedAtFrame   = currentFrame,
        .isHold          = isHold,
        .isPlayer2       = isPlayer2,
        .remainingFrames = p.frames
    };
}

BufferManager::DrainResult BufferManager::drain(int64_t currentFrame, bool p1Ready, bool p2Ready) {
    (void)currentFrame;
    DrainResult r;
    for (int slot = 0; slot < 2; ++slot) {
        auto& opt = m_pendingInputs[slot];
        if (!opt) continue;
        auto& in = *opt;
        const bool ready = (slot == 0) ? p1Ready : p2Ready;

        if (in.remainingFrames < 0) {
            if (in.isHold && m_applied[slot]) {
                if (slot == 0) r.p1Release = true; else r.p2Release = true;
            }
            opt.reset();
            m_applied[slot] = false;
            continue;
        }

        if (in.isHold) {
            if (!m_applied[slot]) {
                if (slot == 0) r.p1Hold = true; else r.p2Hold = true;
                m_applied[slot] = true;
            }
            --in.remainingFrames;
            if (in.remainingFrames < 0) {
                if (slot == 0) r.p1Release = true; else r.p2Release = true;
                opt.reset();
                m_applied[slot] = false;
            }
            continue;
        }

        // Tap: wait for a jump opportunity, then fire once.
        if (!ready) {
            --in.remainingFrames;
            if (in.remainingFrames < 0) {
                opt.reset();
                m_applied[slot] = false;
            }
            continue;
        }

        if (slot == 0) r.p1Tap = true; else r.p2Tap = true;
        m_applied[slot] = true;
        opt.reset();
    }
    return r;
}

void BufferManager::releaseInput(bool isPlayer2, int64_t /*currentFrame*/) {
    int slot = isPlayer2 ? 1 : 0;
    if (m_pendingInputs[slot] && m_pendingInputs[slot]->isHold) {
        m_pendingInputs[slot]->remainingFrames = -1;
    } else {
        m_pendingInputs[slot].reset();
    }
}

void BufferManager::clearAll() {
    m_pendingInputs[0].reset();
    m_pendingInputs[1].reset();
    m_applied[0] = m_applied[1] = false;
}

} // namespace spoon
