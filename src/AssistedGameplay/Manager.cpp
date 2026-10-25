/**
 * SpoonClient / AssistedGameplay/Manager.cpp
 *
 * Glue code. The actual decision-making is delegated to:
 *   * StateAnalyzer        (collect)
 *   * LevelAnalyzer        (cache)
 *   * RecoveryManager      (observe)
 *   * ActionPlanner        (decide)
 *   * InputController      (apply)
 *   * OverlayUI            (display)
 *
 * The Manager's job is sequencing them and handling lifecycle transitions
 * without leaking state.
 */
#include "Manager.hpp"
#include "StateAnalyzer.hpp"
#include "LevelAnalyzer.hpp"
#include "RecoveryManager.hpp"
#include "ActionPlanner.hpp"
#include "InputController.hpp"
#include "OverlayUI.hpp"
#include "../SpoonSettings.hpp"
#include "../BufferManager.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>

using namespace geode;
using namespace geode::prelude;

namespace spoon::ag {

Manager& Manager::get() {
    static Manager instance;
    return instance;
}

Manager::Manager() = default;

void Manager::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;

    auto& ag = Settings::get().agMut();
    ag.enabled = enabled;
    Settings::get().saveAgSettings();

    if (!enabled) {
        InputController::get().reset();
        OverlayUI::get().hide();
        BufferManager::get().clearAll();
        setStatus(Status::Idle);
    } else {
        RecoveryManager::get().reset();
        if (auto* pl = PlayLayer::get()) {
            LevelAnalyzer::get().rebuild(pl);
            BufferManager::get().onLevelStart();
        }
        setStatus(Status::Ready);
        if (Settings::get().ag().showOverlay)
            OverlayUI::get().show();
        else
            OverlayUI::get().hide();
    }
}

void Manager::setStatus(Status s) {
    m_status = s;
    auto& ag = Settings::get().agMut();
    if (ag.showOverlay) {
        OverlayUI::get().update(
            s, "",
            BufferManager::get().activeGamemode(false),
            BufferManager::get().bufferFrames(false));
    }
}

void Manager::onLevelStart(PlayLayer* pl) {
    if (!m_enabled) return;
    LevelAnalyzer::get().rebuild(pl);
    RecoveryManager::get().reset();
    BufferManager::get().onLevelStart();
    setStatus(Status::Ready);
    if (Settings::get().ag().showOverlay)
        OverlayUI::get().show();
}

void Manager::onLevelExit() {
    if (!m_enabled) return;
    LevelAnalyzer::get().clear();
    InputController::get().reset();
    BufferManager::get().clearAll();
    OverlayUI::get().hide();
    setStatus(Status::Idle);
}

void Manager::emergencyStop() {
    if (!m_enabled) return;
    InputController::get().reset();
    BufferManager::get().clearAll();
    setStatus(Status::Paused);
}

void Manager::onUpdate(PlayLayer* pl) {
    if (!m_enabled || !pl) return;
    runPipeline(pl);
}

void Manager::runPipeline(PlayLayer* pl) {
    Snapshot snap = StateAnalyzer::get().analyze(pl);

    auto ev = RecoveryManager::get().observe(snap);
    switch (ev) {
        case RecoveryManager::Event::PlayerDied:
            InputController::get().reset();
            BufferManager::get().clearAll();
            setStatus(Status::Recovering);
            break;
        case RecoveryManager::Event::PlayerRespawned:
            setStatus(Status::Active);
            break;
        case RecoveryManager::Event::LevelRestart:
            LevelAnalyzer::get().rebuild(pl);
            InputController::get().reset();
            BufferManager::get().onLevelRestart();
            setStatus(Status::Ready);
            break;
        case RecoveryManager::Event::LevelExited:
            onLevelExit();
            return;
        case RecoveryManager::Event::GamemodeChanged:
            // BufferManager already reloaded its profile from the
            // PlayerObjectHook; we just flush the planner state.
            InputController::get().reset();
            setStatus(Status::Active);
            break;
        case RecoveryManager::Event::None:
        default:
            // No transition — fall through to the planner.
            if (m_status == Status::Ready) setStatus(Status::Active);
            break;
    }

    if (snap.level.completed) {
        setStatus(Status::Completed);
        InputController::get().reset();
        return;
    }

    // Throttle the planner: every Nth frame we ask the planner for a fresh
    // plan. The buffer in between is held by InputController so a "hold"
    // decision continues to apply between ticks.
    if (++m_throttleCounter < kThrottleN) return;
    m_throttleCounter = 0;

    int la = Settings::get().ag().lookaheadFrames;
    static ActionPlanner s_planner; // stateless; safe to keep static
    Plan plan = s_planner.decide(snap, la);
    InputController::get().apply(plan, snap.level.frame);

    if (Settings::get().ag().showOverlay) {
        OverlayUI::get().update(
            m_status, plan.reason,
            BufferManager::get().activeGamemode(false),
            BufferManager::get().bufferFrames(false));
    }
}

} // namespace spoon::ag
