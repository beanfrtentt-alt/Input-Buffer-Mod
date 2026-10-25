/**
 * SpoonClient / AssistedGameplay/ActionPlanner.cpp
 *
 * Receding-horizon decision logic. The rules below are intentionally simple
 * and well-commented — they are the "reliable working baseline" the spec
 * asks for. A future trajectory-simulation upgrade can replace
 * `planCube`/`planShip`/... without touching the surrounding manager code,
 * because the public API is a single `decide(...)` call returning a `Plan`.
 *
 * Tuning knobs (constants at the top of the file) were chosen against the
 * official GD 2.2074 demo levels; they'll likely need a per-profile
 * calibration UI in a follow-up release.
 */
#include "ActionPlanner.hpp"
#include <cstring>
#include <cstdio>
#include <cmath>

namespace spoon::ag {

namespace {

// Cube speed is ~311.58 u/frame on normal speed; we approximate as 5.0 per
// gameplay frame at speedMult=1 and scale by the multiplier the game reports.
constexpr float kBaseSpeedU = 5.0f;
// Default jump height for cube. Used to convert "is there an obstacle to
// clear" into "is the obstacle short enough to clear".
constexpr float kCubeJumpHeight = 70.f;
// Forward distance at which a cube should start its jump (in world units).
// Tuned to ~14 frames at base speed: just enough for the jump to peak above
// the spike's hitbox.
constexpr float kCubeJumpDistance = 70.f;

// Ship / UFO: hold the click to climb when an obstacle's bounding box would
// intersect the player at the predicted position 8 frames from now.
constexpr float kShipClimbDistance = 90.f;

// Wave: the look-ahead is short — wave is most sensitive.
constexpr float kWaveClimbDistance = 40.f;

} // namespace

float ActionPlanner::speedWorldPerFrame(float speedMult) {
    return kBaseSpeedU * speedMult;
}

static void setReason(Plan& p, const char* s) {
    std::snprintf(p.reason, sizeof(p.reason), "%s", s);
}

static const Hazard* nearestHazardAhead(const Snapshot& snap, const PlayerState& p,
                                         float maxDist) {
    const Hazard* best = nullptr;
    float bestDx = maxDist;
    for (const auto& h : snap.nearbyHazards) {
        float dx = h.pos.x - p.pos.x;
        if (dx < 0.f) continue;
        if (dx >= bestDx) continue;
        bestDx = dx;
        best = &h;
    }
    return best;
}

Plan ActionPlanner::decide(const Snapshot& snap, int agLookaheadFrames) {
    Plan r{};
    if (snap.level.completed || snap.level.levelExited) {
        setReason(r, "Level complete");
        return r;
    }
    // Choose the appropriate player slot. In dual mode we alternate the
    // decision per tick — for simplicity we focus on P1; P2 gets a mirrored
    // decision in a real dual scenario. (TODO: per-slot decision for dual.)
    const PlayerState& p = snap.player[0];
    if (p.isDead) {
        setReason(r, "Player dead");
        return r;
    }
    Plan local{};
    switch (p.gamemode) {
        case Gamemode::Cube:   local = planCube  (p, snap, agLookaheadFrames); break;
        case Gamemode::Ship:   local = planShip  (p, snap, agLookaheadFrames); break;
        case Gamemode::Ball:   local = planBall  (p, snap, agLookaheadFrames); break;
        case Gamemode::UFO:    local = planUfo   (p, snap, agLookaheadFrames); break;
        case Gamemode::Wave:   local = planWave  (p, snap, agLookaheadFrames); break;
        case Gamemode::Robot:  local = planRobot (p, snap, agLookaheadFrames); break;
        case Gamemode::Spider: local = planSpider(p, snap, agLookaheadFrames); break;
        case Gamemode::Swing:  local = planSwing (p, snap, agLookaheadFrames); break;
        case Gamemode::Unknown:
            setReason(local, "Unknown gamemode");
            return local;
    }
    local.targetPlayer2 = false;
    return local;
}

Plan ActionPlanner::planCube(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    Plan r{};
    auto* h = nearestHazardAhead(snap, p, kCubeJumpDistance);
    if (!h) {
        setReason(r, "Clear path — no input");
        return r;
    }
    // On ground + obstacle ahead = jump.
    if (p.onGround) {
        r.wantPress = true;
        r.wantHold  = true;
        setReason(r, "Floor hazard ahead — jump");
        return r;
    }
    // In air: hold for longer jump if there's a tall obstacle and we're still
    // rising; release to land if we're descending so we can land cleanly and
    // jump again at the next obstacle.
    if (p.velocity.y > 0.f && h->size.y > kCubeJumpHeight * 0.5f) {
        r.wantHold = true;
        setReason(r, "Holding jump for tall obstacle");
    } else {
        r.wantRelease = true;
        setReason(r, "Releasing to land");
    }
    return r;
}

Plan ActionPlanner::planShip(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    Plan r{};
    auto* h = nearestHazardAhead(snap, p, kShipClimbDistance);
    if (!h) {
        // No obstacle ahead — release so we don't climb into the ceiling.
        r.wantRelease = true;
        setReason(r, "Clear — descend");
        return r;
    }
    // If the obstacle is above us, climb; if it's at our altitude and we're
    // close to the floor, climb; otherwise release to dive under it.
    bool obstacleAbove = (h->pos.y - h->size.y) > p.pos.y;
    if (obstacleAbove || p.pos.y < 120.f) {
        r.wantHold = true;
        setReason(r, "Climb over obstacle");
    } else {
        r.wantRelease = true;
        setReason(r, "Dive under obstacle");
    }
    return r;
}

Plan ActionPlanner::planBall(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    Plan r{};
    // The ball rides the floor or the ceiling. We switch sides when an
    // upcoming floor hazard would hit us.
    auto* h = nearestHazardAhead(snap, p, kCubeJumpDistance);
    if (!h) {
        setReason(r, "Clear path");
        return r;
    }
    // If we're on the floor and a floor hazard is ahead, flip to ceiling.
    // If we're on the ceiling and a ceiling hazard is ahead, flip to floor.
    bool hazardOnOurSide = (p.onGround && h->pos.y < 130.f) ||
                           (p.onCeiling && h->pos.y > 200.f);
    if (hazardOnOurSide && p.onGround != p.onCeiling) {
        r.wantPress = true;
        setReason(r, "Flip to opposite surface");
    } else {
        setReason(r, "Stay on surface");
    }
    return r;
}

Plan ActionPlanner::planUfo(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    // UFO is similar to ship but discrete taps for ascent rather than holds.
    Plan r{};
    auto* h = nearestHazardAhead(snap, p, kShipClimbDistance);
    if (!h) {
        setReason(r, "Clear path — descend");
        return r;
    }
    if (p.pos.y < h->pos.y + h->size.y) {
        r.wantPress = true;
        setReason(r, "Tap to gain altitude");
    }
    return r;
}

Plan ActionPlanner::planWave(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    Plan r{};
    auto* h = nearestHazardAhead(snap, p, kWaveClimbDistance);
    if (!h) {
        r.wantRelease = true;
        setReason(r, "Clear — descend");
        return r;
    }
    // Wave climbs when held, dives when released. Mirror the rule used for
    // ship, but with a much shorter look-ahead and one-frame granularity.
    if (h->pos.y > p.pos.y) {
        r.wantHold = true;
        setReason(r, "Climb to clear");
    } else {
        r.wantRelease = true;
        setReason(r, "Dive under");
    }
    return r;
}

Plan ActionPlanner::planRobot(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    // Robot's jump is similar to cube but variable height (hold for higher).
    return planCube(p, snap, 0);
}

Plan ActionPlanner::planSpider(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    // Spider teleports between floor and ceiling on tap.
    return planBall(p, snap, 0);
}

Plan ActionPlanner::planSwing(const PlayerState& p, const Snapshot& snap, int /*la*/) {
    // Swing fires on key release — so we want press + hold + release at
    // precisely the right moment. Simplest reliable behavior: hold the
    // input, and release when an obstacle is within jump distance.
    Plan r{};
    auto* h = nearestHazardAhead(snap, p, kCubeJumpDistance);
    if (!h) {
        r.wantHold = true;
        setReason(r, "Hold for next swing");
        return r;
    }
    if (p.onGround) {
        r.wantRelease = true;
        setReason(r, "Release to swing-fire");
    }
    return r;
}

} // namespace spoon::ag
