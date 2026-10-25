/**
 * SpoonClient / AssistedGameplay/State.hpp
 *
 * Plain-data snapshot of every piece of game state that Assisted Gameplay
 * cares about. Keeping this in a single struct means the StateAnalyzer is
 * trivially testable — you can hand-build a State and feed it to the
 * ActionPlanner without a live PlayLayer.
 *
 * All distances and positions are in *Geometry Dash world units* (the same
 * coordinate system `PlayerObject::m_position` uses), not pixels. The
 * X axis points right (level progression); the Y axis points up.
 *
 * Note on `nearbyHazards`: a hazard is anything the player would collide
 * with destructively (spikes, sawblades, hazard blocks). The StateAnalyzer
 * filters the level's full object list down to those whose bounding boxes
 * intersect a forward window of about `agLookaheadFrames` worth of player
 * movement. Each entry is in world coordinates already (no need to
 * transform again at decision time).
 */
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include "Utils.hpp"

namespace spoon::ag {

struct Vec2 {
    float x = 0.f, y = 0.f;
};

enum class HazardType : uint8_t {
    Spike      = 0,  // small triangular obstacle, lethal on contact
    Sawblade   = 1,  // rotating circle, lethal on contact
    Block      = 2,  // solid block — lethal only if hit from the wrong side
    Pit        = 3,  // gap in floor — lethal to fall into
    Ceiling    = 4   // overhead hazard for ship/ufo/wave
};

struct Hazard {
    HazardType  type   = HazardType::Spike;
    Vec2        pos    {};        // center, world coords
    Vec2        size   {};        // half-extents (radius for round)
    bool        isFloor = false;  // touches the floor (relevant to ball)
    int         objectId = 0;     // GD object id (debugging)
};

struct PlayerState {
    Vec2        pos       {};
    Vec2        velocity  {};     // units per gameplay frame
    Gamemode    gamemode  = Gamemode::Cube;
    bool        onGround  = false;
    bool        onCeiling = false;
    bool        isDead    = false;
    bool        isDual    = false;
    bool        isPlayer2 = false;
    bool        gravityInverted = false;
    float       rotation  = 0.f;  // for ball/spider — flipping orientation
};

struct LevelState {
    // 0..1 fraction of the level the player has already passed.
    float       progress        = 0.f;
    // Total length in world units (PlayLayer::m_levelLength).
    float       levelLength     = 0.f;
    // Active speed multiplier (1x = 311.58 u/frame for cube on normal speed).
    float       speedMultiplier = 1.f;
    // True when the level is currently in a dual portal section.
    bool        dualActive      = false;
    // True when the level has been completed (PlayLayer::m_levelCompleted).
    bool        completed       = false;
    // True when PlayLayer has been destroyed (level exit).
    bool        levelExited     = false;
    // Frame counter as observed by GJBaseGameLayer.
    int64_t     frame           = 0;
};

struct Snapshot {
    PlayerState player[2];   // index 0 = P1, index 1 = P2 (valid iff dual)
    LevelState  level {};
    std::vector<Hazard> nearbyHazards;
};

// Status machine for the AG subsystem. Drives the OverlayUI label.
enum class Status : uint8_t {
    Idle        = 0,  // AG disabled, no level loaded
    Ready       = 1,  // Level loaded, waiting for gameplay start
    Active      = 2,  // Plan being executed
    Recovering  = 3,  // Player died, plan invalidated, waiting for respawn
    Paused      = 4,  // Emergency stop pressed or AG mid-cycle toggled off
    Completed   = 5,  // Level finished successfully
    Error       = 6,  // An internal error occurred (logged, AG stops)
};

inline std::string_view statusLabel(Status s) {
    switch (s) {
        case Status::Idle:       return "Idle";
        case Status::Ready:      return "Ready";
        case Status::Active:     return "Active";
        case Status::Recovering: return "Recovering";
        case Status::Paused:     return "Paused";
        case Status::Completed:  return "Completed";
        case Status::Error:      return "Error";
    }
    return "Unknown";
}

} // namespace spoon::ag
