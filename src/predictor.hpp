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
    bool hitSolid = false;        // the predicted collision is with a solid block, not a hazard
    Box hitBox;                   // the obstacle that was hit (unshrunk)
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
                // Solids are shrunk slightly so standing exactly on a block's top
                // surface is not reported as a collision.
                Box ob = o.hazard ? hazardBox(o.box)
                                  : Box{o.box.x0 + 2.f, o.box.y0 + 2.f, o.box.x1 - 2.f, o.box.y1 - 2.f};
                if (!overlaps(pb, ob)) continue;
                // Only a genuinely falling player can "land" on a solid.
                bool landingOnSolid = !o.hazard && ((gdir < 0 && dy < -0.01f) || (gdir > 0 && dy > 0.01f));
                if (landingOnSolid && r.landTick < 0) {
                    r.landTick = i; r.landAt = {x, y};
                    dy = 0; ay = 0;
                } else if (o.hazard || !landingOnSolid) {
                    r.hazardTick = i; r.hazardAt = {x, y};
                    r.hitSolid = !o.hazard; r.hitBox = o.box;
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

struct JumpOutcome { bool clears = false; float endX = 0; };

// Simulates a jump started `delay` ticks from now (flat run until then) using the learned
// profile. `clears` = no hazard overlap anywhere along the arc; `endX` = x at the end of it.
inline JumpOutcome simulateJump(Kin const& k, JumpProfile const& p,
                                std::vector<Obstacle> const& hazards, int delay) {
    JumpOutcome out;
    if (!k.valid || !p.valid || !k.grounded) return out;
    float up = k.upsideDown ? -1.f : 1.f;
    float x = k.x, y = k.y;
    int total = delay + (int)p.dys.size();
    out.clears = true;
    for (int i = 1; i <= total; i++) {
        x += k.dx;
        if (i > delay) y += p.dys[(size_t)(i - delay - 1)] * up;
        Box pb = playerBox(x, y);
        for (auto const& o : hazards) {
            if (o.hazard && overlaps(pb, hazardBox(o.box))) { out.clears = false; out.endX = x; return out; }
        }
    }
    out.endX = x;
    return out;
}

inline bool jumpClears(Kin const& k, JumpProfile const& p, std::vector<Obstacle> const& hazards, int delay) {
    return simulateJump(k, p, hazards, delay).clears;
}

// What should the click assistant do on this tick?
enum class Need {
    Unavailable,  // required game-state information is missing; cannot judge
    NoHazard,     // nothing on the current path needs a jump
    Wait,         // a jump will be needed; the player still has time to click (or window not open yet)
    JumpNow,      // last safe tick: a jump now is predicted to clear, one tick later is not
    Missed        // a jump is needed but no jump from here is predicted to clear
};

struct Decision {
    Need need = Need::Unavailable;
    std::string reason;
    int windowOpensIn = -1;    // ticks until a jump becomes safe (Wait, not open yet)
    int windowClosesIn = -1;   // ticks until the last safe moment (Wait, window open)
};

// `straight` must be predict() of the same Kin (player keeps going straight, no input).
// `hazards` = obstacles ahead (hazards only matter). `maxScan` bounds the search in ticks.
inline Decision decideJump(Kin const& k, Prediction const& straight, JumpProfile const& p,
                           std::vector<Obstacle> const& hazards, int maxScan) {
    Decision d;
    if (!k.valid)                 { d.reason = "no player data yet"; return d; }
    if (k.mode != Mode::Cube)     { d.reason = std::string("mode not supported (") + modeName(k.mode) + "); cube only"; return d; }
    if (!k.grounded)              { d.reason = "airborne: no jump available"; return d; }
    if (!p.valid)                 { d.reason = "no jump profile yet: do one normal clicked jump first"; return d; }
    if (!straight.valid)          { d.reason = "no prediction available"; return d; }
    if (straight.hazardTick < 0)  { d.need = Need::NoHazard; d.reason = "no hazard on current path"; return d; }
    if (straight.hitSolid)        { d.reason = "path blocked by a solid block; only hazards (spikes/saws) are supported"; return d; }

    maxScan = std::clamp(maxScan, 2, 100);
    // Only start judging once the hazard is within (scan range + one jump arc); further out
    // the window is simply not in range yet.
    int reach = maxScan + (int)p.dys.size();
    if (straight.hazardTick > reach) {
        d.need = Need::Wait;
        d.reason = "hazard in ~" + std::to_string(straight.hazardTick) + " ticks; jump window not in scan range yet";
        return d;
    }
    // A jump delayed past the tick of contact can never clear, so this search is complete.
    int limit = std::max(straight.hazardTick, 1);
    float passX = hazardBox(straight.hitBox).x1 + kPlayerHalf;   // landing must be past the hazard
    auto ok = [&](int dl) {
        auto o = simulateJump(k, p, hazards, dl);
        return o.clears && o.endX >= passX;
    };

    if (ok(0)) {
        if (!ok(1)) {
            d.need = Need::JumpNow; d.windowClosesIn = 0;
            d.reason = "last safe tick: waiting one more tick is predicted to fail";
        } else {
            int e = 1;
            while (e < limit && ok(e + 1)) e++;
            d.need = Need::Wait; d.windowClosesIn = e;
            d.reason = "jump window open; closes in ~" + std::to_string(e) + " ticks";
        }
        return d;
    }
    for (int dl = 1; dl <= limit; dl++) {
        if (ok(dl)) {
            d.need = Need::Wait; d.windowOpensIn = dl;
            d.reason = "jump window opens in ~" + std::to_string(dl) + " ticks";
            return d;
        }
    }
    d.need = Need::Missed;
    d.reason = "no jump from here is predicted to clear (window passed or hazard too wide)";
    return d;
}

} // namespace ibd
