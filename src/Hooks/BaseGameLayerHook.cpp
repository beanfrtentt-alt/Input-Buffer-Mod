/**
 * SpoonClient / Hooks/BaseGameLayerHook.cpp
 *
 * GJBaseGameLayer is where GD actually consumes player clicks
 * (`handleButton`) and where the per-frame `update` virtual lives.
 *
 * We intercept jump (PlayerButton::Jump) so both the human player and
 * Assisted Gameplay share BufferManager. Synthetic clicks from drain()
 * set a re-entrancy guard so they pass through to the original.
 */
#include "BaseGameLayerHook.hpp"
#include "../AssistedGameplay/Manager.hpp"
#include "../AssistedGameplay/StateAnalyzer.hpp"
#include "../BufferManager.hpp"
#include "../SpoonSettings.hpp"
#include "../UI/SpoonMenuLayer.hpp"
#include "../Utils.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>

using namespace geode::prelude;

namespace {
thread_local int s_injectDepth = 0;

struct InjectGuard {
    InjectGuard() { ++s_injectDepth; }
    ~InjectGuard() { --s_injectDepth; }
};

spoon::Gamemode gamemodeOf(PlayerObject* p) {
    if (!p) return spoon::Gamemode::Cube;
    if (p->m_isShip) return spoon::Gamemode::Ship;
    if (p->m_isBall) return spoon::Gamemode::Ball;
    if (p->m_isBird) return spoon::Gamemode::UFO;
    if (p->m_isDart) return spoon::Gamemode::Wave;
    if (p->m_isRobot) return spoon::Gamemode::Robot;
    if (p->m_isSpider) return spoon::Gamemode::Spider;
    if (p->m_isSwing) return spoon::Gamemode::Swing;
    return spoon::Gamemode::Cube;
}

bool jumpReady(PlayerObject* p) {
    if (!p) return false;
    auto gm = gamemodeOf(p);
    if (!spoon::gamemodeSupportsBufferHold(gm)) return true;
    return p->m_isOnGround;
}

void applyDrain(GJBaseGameLayer* layer, const spoon::BufferManager::DrainResult& r) {
    InjectGuard guard;
    auto const jump = static_cast<int>(PlayerButton::Jump);
    if (r.p1Tap || r.p1Hold) layer->handleButton(true, jump, true);
    if (r.p1Release)         layer->handleButton(false, jump, true);
    if (r.p2Tap || r.p2Hold) layer->handleButton(true, jump, false);
    if (r.p2Release)         layer->handleButton(false, jump, false);
}
}

class $modify(SpoonBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (s_injectDepth > 0 || button != static_cast<int>(PlayerButton::Jump)) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto* pl = typeinfo_cast<PlayLayer*>(this);
        if (!pl) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        bool isP2 = !isPlayer1;
        auto* player = isP2 ? m_player2 : m_player1;
        auto gm = gamemodeOf(player);
        auto const& profile = spoon::Settings::get().profile(gm);
        if (!profile.enabled || profile.frames <= 0) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
        bool isHold = !spoon::gamemodeSupportsBufferHold(gm);
        auto& buf = spoon::BufferManager::get();
        if (down) {
            buf.queueInput(isP2, isHold, frame);
        } else {
            bool alreadyApplied = buf.wasApplied(isP2);
            buf.releaseInput(isP2, frame);
            if (alreadyApplied) {
                InjectGuard guard;
                GJBaseGameLayer::handleButton(false, button, isPlayer1);
            }
        }
    }

    void update(float dt) {
        auto* pl = typeinfo_cast<PlayLayer*>(this);
        if (pl && !pl->m_isPaused) {
            spoon::ag::Manager::get().onUpdate(pl);

            bool p1Ready = jumpReady(m_player1);
            bool p2Ready = jumpReady(m_player2);
            int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
            auto drained = spoon::BufferManager::get().drain(frame, p1Ready, p2Ready);
            applyDrain(this, drained);

            if (spoon::SpoonMenuLayer::isOpen()) {
                auto* scene = cocos2d::CCDirector::get()->getRunningScene();
                if (auto* menu = scene ? scene->getChildByTag(0xC90C) : nullptr) {
                    static_cast<spoon::SpoonMenuLayer*>(menu)->refreshFooter();
                }
            }
        }

        GJBaseGameLayer::update(dt);
    }
};
