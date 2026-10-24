#pragma once
// Pure trajectory / timing math. No cocos, no hooks, no drawing, no input.
// Everything here works on plain data copied out of the game, runs in a bounded
// number of steps (horizon * obstacle count, both capped) and never touches game state.
//
// IMPORTANT: this is an *extrapolation* of observed per-tick motion, not the game's
// real physics step. It is approximate and will be wrong around triggers, portals,
// pads/orbs, slopes, moving objects and mode-specific mechanics.

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace ibd {

enum class Mode { Cube, Ship, Ball, Ufo, Wave, Robot, Spider, Swing, Unknown };

inline const char* modeName(Mode m) {
    switch (m) {
        case Mode::Cube: return "cube";   case Mode::Ship: return "ship";
        case Mode::Ball: return "ball";   case Mode::Ufo: return "ufo";
        case Mode::Wave: return "wave";   case Mode::Robot: return "robot";
        case Mode::Spider: return "spider"; case Mode::Swing: return "swing";
        default: return "unknown";
    }
}

struct Box { float x0 = 0, y0 = 0, x1 = 0, y1 = 0; };
struct Obstacle { Box box; bool hazard = false; };
struct Pt { float x = 0, y = 0; };

inline bool overlaps(Box const& a, Box const& b) {
    return a.x0 < b.x1 && a.x1 > b.x0 && a.y0 < b.y1 && a.y1 > b.y0;
}

// Player state sampled from the game (units: world units / game ticks).
struct Kin {
    float x = 0, y = 0;
    float dx = 0, dy = 0;      // observed change per tick
    float ay = 0;              // observed change of dy per tick
    bool grounded = false;
    bool upsideDown = false;
    float groundY = 0;         // last observed y while grounded
    Mode mode = Mode::Unknown;
    bool valid = false;
};

// Heuristics (documented limits, not game constants):
constexpr float kPlayerHalf = 15.f;     // a cube is 30x30 world units
constexpr float kHazardShrink = 0.6f;   // hazard sprite rects are bigger than their hitboxes

inline Box playerBox(float x, float y) {
    return {x - kPlayerHalf, y - kPlayerHalf, x + kPlayerHalf, y + kPlayerHalf};
}
inline Box hazardBox(Box b) {
    float cx = (b.x0 + b.x1) * .5f, cy = (b.y0 + b.y1) * .5f;
    float hw = (b.x1 - b.x0) * .5f * kHazardShrink, hh = (b.y1 - b.y0) * .5f * kHazardShrink;
    return {cx - hw, cy - hh, cx + hw, cy + hh};
}

struct Prediction {
    std::vector<Pt> pts;          // pts[i] = predicted position i+1 ticks ahead
    int hazardTick = -1;          // -1 = none inside the horizon
    Pt hazardAt;
    int landTick = -1;            // -1 = none inside the horizon (or already grounded)
    Pt landAt;
    int nextActionTick = 0;       // ticks until a jump/click is expected to be possible
    bool dangerNarrow = false;    // hazard is close in time: little room to react
    std::string confidence = "none";
    std::string note;
    bool valid = false;
};

inline std::string confidenceFor(Mode m, bool grounded) {
    switch (m) {
        case Mode::Cube: return grounded ? "medium" : "medium";
        case Mode::Ball: return "low";
        case Mode::Wave: return "low";
        default: return "very low";
    }
}

inline Prediction predict(Kin const& k, std::vector<Obstacle> const& obs, int horizon) {
    Prediction r;
    if (!k.valid) { r.note = "no data yet"; return r; }
    horizon = std::clamp(horizon, 1, 400);
    r.valid = true;
    r.confidence = confidenceFor(k.mode, k.grounded);
    r.note = "extrapolated from observed motion; triggers/pads/orbs/portals not modelled";
    if (k.mode != Mode::Cube && k.mode != Mode::Wave && k.mode != Mode::Ball)
        r.note += "; mode not modelled (straight/parabolic guess)";

    float x = k.x, y = k.y, dy = k.dy;
    float ay = k.grounded ? 0.f : k.ay;
    float gdir = k.upsideDown ? 1.f : -1.f;  // direction gravity pulls (sign of ay while falling)
    r.pts.reserve((size_t)horizon);

    for (int i = 1; i <= horizon; i++) {
        x += k.dx;
        if (!k.grounded || std::fabs(dy) > 0.01f) { dy += ay; y += dy; }

        // Landing: crossing the last known floor level while moving in gravity's direction.
        bool landing = !k.grounded && r.landTick < 0 &&
            ((gdir < 0 && y <= k.groundY && dy <= 0) || (gdir > 0 && y >= k.groundY && dy >= 0));
        if (landing) {
            y = k.groundY; dy = 0; ay = 0;
            r.landTick = i; r.landAt = {x, y};
        }
        r.pts.push_back({x, y});

        if (r.hazardTick < 0) {
            Box pb = playerBox(x, y);
            for (auto const& o : obs) {
                Box ob = o.hazard ? hazardBox(o.box) : o.box;
                if (!overlaps(pb, ob)) continue;
                bool landingOnSolid = !o.hazard && ((gdir < 0 && dy <= 0) || (gdir > 0 && dy >= 0));
                if (landingOnSolid && r.landTick < 0) {
                    r.landTick = i; r.landAt = {x, y};
                    dy = 0; ay = 0;
                } else if (o.hazard || !landingOnSolid) {
                    r.hazardTick = i; r.hazardAt = {x, y};
                }
                break;
            }
        }
    }

    r.nextActionTick = (k.grounded || k.mode != Mode::Cube) ? 0 : (r.landTick >= 0 ? r.landTick : -1);
    r.dangerNarrow = r.hazardTick >= 0 && r.hazardTick <= 12;
    return r;
}

// ---- Jump profile learned from the player's own previous jumps (cube only) ----
struct JumpProfile {
    std::vector<float> dys;   // per-tick dy after take-off, normalised so "up" is positive
    float speedKey = 0;       // player speed the profile was recorded at
    bool valid = false;
};

// Would a jump started `delay` ticks from now (flat run until then) avoid every hazard?
inline bool jumpClears(Kin const& k, JumpProfile const& p, std::vector<Obstacle> const& obs, int delay) {
    if (!k.valid || !p.valid || !k.grounded) return false;
    float up = k.upsideDown ? -1.f : 1.f;
    float x = k.x, y = k.y;
    int total = delay + (int)p.dys.size();
    for (int i = 1; i <= total; i++) {
        x += k.dx;
        if (i > delay) y += p.dys[(size_t)(i - delay - 1)] * up;
        Box pb = playerBox(x, y);
        for (auto const& o : obs) {
            if (o.hazard && overlaps(pb, hazardBox(o.box))) return false;
        }
    }
    return true;
}

struct Correction {
    bool found = false;
    int delay = 0;
    bool immediateClears = false;
    std::string reason;
};

// Looks for the smallest delay (0..maxDelay) at which a jump is predicted to clear all hazards.
inline Correction findCorrection(Kin const& k, JumpProfile const& p,
                                 std::vector<Obstacle> const& obs, int maxDelay) {
    Correction c;
    if (!k.valid)         { c.reason = "no player data"; return c; }
    if (k.mode != Mode::Cube) { c.reason = "mode not supported (cube only)"; return c; }
    if (!k.grounded)      { c.reason = "player not grounded"; return c; }
    if (!p.valid)         { c.reason = "no jump profile yet (complete one normal jump first)"; return c; }
    if (jumpClears(k, p, obs, 0)) {
        c.immediateClears = true; c.reason = "immediate jump predicted safe; no correction needed";
        return c;
    }
    for (int d = 1; d <= maxDelay; d++) {
        if (jumpClears(k, p, obs, d)) {
            c.found = true; c.delay = d;
            c.reason = "immediate jump predicted to hit a hazard; delaying " + std::to_string(d) + " ticks predicted to clear";
            return c;
        }
    }
    c.reason = "no safe delay found within window";
    return c;
}

} // namespace ibd
