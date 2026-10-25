/**
 * SpoonClient / Hooks/PlayerObjectHook.cpp
 *
 * Tracks gamemode flag changes and flushes the matching buffer slot on
 * death. Jump press/release is intercepted on GJBaseGameLayer::handleButton
 * so we do not hook PlayerObject::pushButton (that would double-queue).
 */
#include "PlayerObjectHook.hpp"
#include "../BufferManager.hpp"
#include "../Utils.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace {
thread_local spoon::Gamemode s_lastGm[2] = { spoon::Gamemode::Unknown,
                                              spoon::Gamemode::Unknown };

spoon::Gamemode currentGamemode(PlayerObject* p) {
    if (!p) return spoon::Gamemode::Unknown;
    if (p->m_isShip) return spoon::Gamemode::Ship;
    if (p->m_isBall) return spoon::Gamemode::Ball;
    if (p->m_isBird) return spoon::Gamemode::UFO;
    if (p->m_isDart) return spoon::Gamemode::Wave;
    if (p->m_isRobot) return spoon::Gamemode::Robot;
    if (p->m_isSpider) return spoon::Gamemode::Spider;
    if (p->m_isSwing) return spoon::Gamemode::Swing;
    return spoon::Gamemode::Cube;
}
}

class $modify(SpoonPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        bool isP2 = m_isSecondPlayer;
        spoon::Gamemode now = currentGamemode(this);
        if (now != spoon::Gamemode::Unknown && s_lastGm[isP2 ? 1 : 0] != now) {
            s_lastGm[isP2 ? 1 : 0] = now;
            spoon::BufferManager::get().onGamemodeChange(isP2, now);
        }
    }

    void playerDestroyed(bool noEffects) {
        spoon::BufferManager::get().onDeath(m_isSecondPlayer);
        PlayerObject::playerDestroyed(noEffects);
    }
};
