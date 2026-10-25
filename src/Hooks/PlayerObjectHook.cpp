/**
 * SpoonClient / Hooks/PlayerObjectHook.cpp
 *
 * `$modify` block for PlayerObject. Catches:
 *  * update        — tracks m_gamemode transitions (portals) so the
 *                    BufferManager reloads the right per-gamemode profile.
 *  * pushButton    — routes the human player's "press" event through the
 *                    BufferManager so AG and the human share a single
 *                    queue (see docs/BUFFER_AG_INTERACTION.md).
 *  * releaseButton — routes the human player's "release" event through the
 *                    BufferManager so holds are correctly terminated.
 *  * playerDeath   — flushes the dead player's buffer slot so a queued
 *                    input doesn't auto-fire on respawn.
 */
#include "PlayerObjectHook.hpp"
#include "../BufferManager.hpp"
#include "../AssistedGameplay/StateAnalyzer.hpp"
#include "../AssistedGameplay/Manager.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace {
thread_local spoon::Gamemode s_lastGm[2] = { spoon::Gamemode::Unknown,
                                              spoon::Gamemode::Unknown };
}

class $modify(SpoonPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        bool isP2 = m_isPlayer2;
        spoon::Gamemode now = spoon::gamemodeFromInt(m_gamemode);
        if (now != spoon::Gamemode::Unknown && s_lastGm[isP2 ? 1 : 0] != now) {
            s_lastGm[isP2 ? 1 : 0] = now;
            spoon::BufferManager::get().onGamemodeChange(isP2, now);
        }
    }

    void pushButton(int button) {
        // Pre-flight: route through buffer FIRST so the buffer is updated
        // before GD's own pipeline sees the press (we still call the
        // original afterwards).
        bool isP2 = m_isPlayer2;
        int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
        bool isHold = !spoon::gamemodeSupportsBufferHold(
            spoon::gamemodeFromInt(m_gamemode));
        spoon::BufferManager::get().queueInput(isP2, isHold, frame);

        PlayerObject::pushButton(button);
    }

    void releaseButton(int button) {
        bool isP2 = m_isPlayer2;
        int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
        spoon::BufferManager::get().releaseInput(isP2, frame);

        PlayerObject::releaseButton(button);
    }

    void playerDeath() {
        bool isP2 = m_isPlayer2;
        spoon::BufferManager::get().onDeath(isP2);
        PlayerObject::playerDeath();
    }
};
