// Standalone test of the pure decision logic: clang++ -std=c++20 -Wall -Wextra tests/predictor_test.cpp
#include "../src/predictor.hpp"
#include <cstdio>
#include <cstdlib>
using namespace ibd;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static Kin cube(float x) {
    Kin k; k.x = x; k.y = 105; k.dx = 2; k.dy = 0; k.ay = 0; k.grounded = true;
    k.groundY = 105; k.mode = Mode::Cube; k.valid = true; return k;
}
static JumpProfile profile() {
    JumpProfile jp; jp.valid = true; float dy = 8;
    for (int i = 0; i < 32; i++) { jp.dys.push_back(dy); dy -= 0.5f; }
    return jp;
}
static Decision decide(Kin const& k, JumpProfile const& jp, std::vector<Obstacle> const& o) {
    auto st = predict(k, o, 150);
    return decideJump(k, st, jp, o, 40);
}

int main() {
    auto jp = profile();
    std::vector<Obstacle> spike{{{300, 90, 330, 120}, true}};

    // 1) Closed loop: run toward the spike; must Wait, then JumpNow exactly once, and the jump must clear.
    Kin k = cube(100);
    int firedAt = -1; int waits = 0;
    for (int t = 0; t < 200; t++, k.x += k.dx) {
        auto d = decide(k, jp, spike);
        if (d.need == Need::Wait) waits++;
        if (d.need == Need::JumpNow) { firedAt = t; break; }
        CHECK(d.need != Need::Missed);
    }
    CHECK(firedAt > 0);
    CHECK(waits > 5);
    auto o = simulateJump(k, jp, spike, 0);
    CHECK(o.clears);
    CHECK(!simulateJump(k, jp, spike, 2).clears || simulateJump(k, jp, spike, 2).endX < 330);
    std::printf("1) assistant fires at tick %d (x=%.0f), waited %d ticks, jump clears=%d\n", firedAt, k.x, waits, o.clears);

    // 2) Too late: spike is right in front, no jump can clear.
    auto late = decide(cube(296), jp, spike);
    CHECK(late.need == Need::Missed);
    std::printf("2) %s\n", late.reason.c_str());

    // 3) No hazard ahead.
    CHECK(decide(cube(100), jp, {}).need == Need::NoHazard);

    // 4) Unavailable cases are reported, never guessed.
    Kin air = cube(100); air.grounded = false;
    CHECK(decide(air, jp, spike).need == Need::Unavailable);
    Kin ship = cube(100); ship.mode = Mode::Ship;
    CHECK(decide(ship, jp, spike).need == Need::Unavailable);
    CHECK(decide(cube(100), JumpProfile{}, spike).need == Need::Unavailable);
    Kin bad = cube(100); bad.valid = false;
    CHECK(decide(bad, jp, spike).need == Need::Unavailable);

    // 5) A solid wall ahead is reported as unsupported rather than "jumped".
    std::vector<Obstacle> wall{{{300, 90, 330, 150}, false}};
    auto w = decide(cube(100), jp, wall);
    CHECK(w.need == Need::Unavailable);
    std::printf("5) %s\n", w.reason.c_str());

    // 6) Standing on a block's top surface must not count as a collision.
    std::vector<Obstacle> floorBlock{{{0, 60, 1000, 90}, false}};
    CHECK(predict(cube(100), floorBlock, 50).hazardTick < 0);

    std::printf(fails ? "%d FAILURES\n" : "all checks passed\n", fails);
    return fails ? 1 : 0;
}
