/**
 * SpoonClient / Hooks/PlayerObjectHook.cpp
 *
 * `$modify` block for PlayerObject. Catches:
 *  * update        — tracks mode-flag transitions (portals) so the
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

        bool isP2 = m_isSecondPlayer;
        spoon::Gamemode now = spoon::Gamemode::Cube;
        if (m_isShip) now = spoon::Gamemode::Ship;
        else if (m_isBall) now = spoon::Gamemode::Ball;
        else if (m_isBird) now = spoon::Gamemode::UFO;
        else if (m_isDart) now = spoon::Gamemode::Wave;
        else if (m_isRobot) now = spoon::Gamemode::Robot;
        else if (m_isSpider) now = spoon::Gamemode::Spider;
        else if (m_isSwing) now = spoon::Gamemode::Swing;
        if (now != spoon::Gamemode::Unknown && s_lastGm[isP2 ? 1 : 0] != now) {
            s_lastGm[isP2 ? 1 : 0] = now;
            spoon::BufferManager::get().onGamemodeChange(isP2, now);
        }
    }

    bool pushButton(PlayerButton button) {
        bool isP2 = m_isSecondPlayer;
        int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
        spoon::Gamemode mode = spoon::Gamemode::Cube;
        if (m_isShip) mode = spoon::Gamemode::Ship;
        else if (m_isBall) mode = spoon::Gamemode::Ball;
        else if (m_isBird) mode = spoon::Gamemode::UFO;
        else if (m_isDart) mode = spoon::Gamemode::Wave;
        else if (m_isRobot) mode = spoon::Gamemode::Robot;
        else if (m_isSpider) mode = spoon::Gamemode::Spider;
        else if (m_isSwing) mode = spoon::Gamemode::Swing;
        bool isHold = !spoon::gamemodeSupportsBufferHold(mode);
        spoon::BufferManager::get().queueInput(isP2, isHold, frame);
        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        bool isP2 = m_isSecondPlayer;
        int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
        spoon::BufferManager::get().releaseInput(isP2, frame);
        return PlayerObject::releaseButton(button);
    }

    void playerDestroyed(bool noEffects) {
        bool isP2 = m_isSecondPlayer;
        spoon::BufferManager::get().onDeath(isP2);
        PlayerObject::playerDestroyed(noEffects);
    }
};
