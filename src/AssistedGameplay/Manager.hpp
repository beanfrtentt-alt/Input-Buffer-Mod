/**
 * SpoonClient / AssistedGameplay/Manager.hpp
 *
 * Coordinator for the entire Assisted Gameplay subsystem. It owns the
 * per-tick scheduling and is the only entry point the rest of the mod
 * talks to (Hooks -> Manager; F6 menu -> Manager).
 *
 * Per-tick pipeline
 * -----------------
 *   1. Hooks call `onUpdate(playLayer)` every gameplay frame.
 *   2. Manager asks StateAnalyzer for a fresh Snapshot.
 *   3. RecoveryManager consumes the Snapshot and returns an Event. If the
 *      event is anything but None, Manager clears the planner state and
 *      (depending on the event) flushes the InputController.
 *   4. ActionPlanner::decide() returns a Plan.
 *   5. InputController::apply() pushes the Plan through the BufferManager.
 *   6. OverlayUI::update() refreshes the on-screen overlay.
 *
 * The pipeline is throttled: only every Nth gameplay frame actually runs
 * the planner (the Snapshot is always collected because the Recovery
 * manager needs every frame; the planner / InputController are skipped
 * on throttled frames to save CPU). N defaults to 3.
 */
#pragma once
#include "State.hpp"

class PlayLayer;

namespace spoon::ag {

class Manager {
public:
    static Manager& get();

    // Toggle AG on/off. Persisted via Settings.
    void setEnabled(bool enabled);
    bool enabled() const { return m_enabled; }

    // Called by Hooks when a level starts (PlayLayer::init).
    void onLevelStart(PlayLayer* pl);
    // Called by Hooks every gameplay update tick.
    void onUpdate(PlayLayer* pl);
    // Called by Hooks when the level exits / PlayLayer is destroyed.
    void onLevelExit();
    // Called by Hooks when an emergency-stop key is pressed.
    void emergencyStop();

    Status status() const { return m_status; }

private:
    Manager();
    void runPipeline(PlayLayer* pl);
    void setStatus(Status s);

    bool   m_enabled = false;
    Status m_status  = Status::Idle;
    int    m_throttleCounter = 0;
    static constexpr int kThrottleN = 3;
};

} // namespace spoon::ag
