/**
 * SpoonClient / AssistedGameplay/ActionPlanner.hpp
 *
 * Decides what inputs AG should press this tick, based on the Snapshot.
 *
 * Strategy
 * --------
 * The planner is *receding-horizon*: every call re-decides from the
 * current player state. It does not produce a long sequence — it produces
 * the next single input decision, then re-evaluates on the next tick.
 *
 * Gamemode-specific rules
 * -----------------------
 *  * **Cube / Robot**: hold to jump when an obstacle on the floor is within
 *    a forward distance threshold determined by `lookaheadFrames` and the
 *    current speed multiplier. Release immediately after the jump apex so
 *    the player doesn't make a single huge jump.
 *  * **Ball / Spider**: tap to switch direction when an upcoming floor
 *    spike's X is within the look-ahead window AND the player's current
 *    gravity side puts them on the wrong side. Tap pattern: switch —> ride
 *    the ceiling for the gap —> switch back.
 *  * **Ship / UFO / Wave**: hold to ascend when an obstacle is approaching
 *    above floor level; release when there's a clear corridor. Wave is the
 *    hardest gamemode to automate; we use a tight bound (≈6 frame look-ahead)
 *    because wave physics is highly non-linear.
 *  * **Swing**: like cube, but the jump is on click release, so we plan a
 *    hold-then-release sequence.
 *
 * Outputs
 * -------
 *  * `wantPress` — true if AG should issue a fresh press this tick.
 *  * `wantHold`  — true if AG should keep holding input through this tick.
 *  * `targetPlayer2` — true if the action is for the dual-mode P2 slot.
 *
 * Limitations
 * -----------
 *  * No simulation: we do not actually step the player's physics forward
 *    to verify a plan. This means extremely tight timing windows can fail.
 *    A real trajectory simulator is a future-work item.
 *  * Moving saws and rotating disks are treated as static hazards at their
 *    current position. Their actual swept volume may overlap the player
 *    even if our snapshot says they're "elsewhere".
 */
#pragma once
#include "State.hpp"

namespace spoon::ag {

struct Plan {
    bool wantPress    = false;
    bool wantHold     = false;
    bool wantRelease  = false;
    bool targetPlayer2 = false;
    // Human-readable explanation for OverlayUI debugging.
    char  reason[64]  = {};
};

class ActionPlanner {
public:
    // Decide the next input. `snap` is the latest Snapshot from
    // StateAnalyzer. `agLookahead` is the configured number of frames
    // to look ahead — it determines the window in world units.
    Plan decide(const Snapshot& snap, int agLookaheadFrames);

private:
    // Gamemode-specific planners. Each one only inspects the relevant
    // subset of `snap` and returns a Plan for the active player slot.
    Plan planCube   (const PlayerState& p, const Snapshot& snap, int la);
    Plan planShip   (const PlayerState& p, const Snapshot& snap, int la);
    Plan planBall   (const PlayerState& p, const Snapshot& snap, int la);
    Plan planUfo    (const PlayerState& p, const Snapshot& snap, int la);
    Plan planWave   (const PlayerState& p, const Snapshot& snap, int la);
    Plan planRobot  (const PlayerState& p, const Snapshot& snap, int la);
    Plan planSpider (const PlayerState& p, const Snapshot& snap, int la);
    Plan planSwing  (const PlayerState& p, const Snapshot& snap, int la);

    // Threshold helpers (world units per frame): used to convert frame
    // look-ahead into world distance.
    static float speedWorldPerFrame(float speedMult);
};

} // namespace spoon::ag
