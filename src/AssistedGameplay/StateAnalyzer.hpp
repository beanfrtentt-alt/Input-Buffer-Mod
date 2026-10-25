/**
 * SpoonClient / AssistedGameplay/StateAnalyzer.hpp
 *
 * Collects a `Snapshot` from the live Geometry Dash state. Isolated from
 * the rest of AG so it can be unit-tested by passing mock PlayLayer /
 * PlayerObject pointers (the binding headers have everything in scope).
 *
 * The analyzer never caches — it reads fresh values every call. The
 * ActionPlanner calls it once per planning tick, which is intentionally
 * throttled (every 3 gameplay frames, see Manager.cpp) so that the
 * performance cost is bounded.
 */
#pragma once
#include <Geode/Geode.hpp>
#include "State.hpp"

// Forward-declare at global scope — Geode v4 binding classes live in the
// global namespace, not inside any sub-namespace.
class PlayLayer;
class PlayerObject;
class GJBaseGameLayer;
class GameObject;

namespace spoon::ag {

class StateAnalyzer {
public:
    // Build a full snapshot. `playLayer` may be nullptr while in menu; in
    // that case the returned Snapshot has status-relevant fields zeroed.
    Snapshot analyze(PlayLayer* playLayer);

    // Convenience: collect just the player state for a single player slot.
    // Used by the InputController to make micro-decisions during a hold.
    PlayerState analyzePlayer(PlayerObject* p, bool isPlayer2);

    // Build a Hazard record from a generic GameObject. Returns std::nullopt
    // if the object isn't a hazard (decoration, trigger, etc.).
    std::optional<Hazard> hazardFromObject(GameObject* obj);

    // Returns the current gameplay frame counter sourced from
    // GJBaseGameLayer's step state. Returns -1 if no level is active.
    int64_t currentFrame();
};

} // namespace spoon::ag
