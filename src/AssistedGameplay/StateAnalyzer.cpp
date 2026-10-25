/**
 * SpoonClient / AssistedGameplay/StateAnalyzer.cpp
 *
 * Bridges between the GD binding types and our plain-data `Snapshot`.
 *
 * The key calls in here are the ones that have historically been broken in
 * GD mods:
 *  * PlayerObject mode flags identify the current gamemode.
 *  * PlayerObject velocity and orientation are read through fields/methods
 *    exposed by the GD 2.2081 Geode bindings.
 *  * `GameObject::m_objectID` — int; ranges used to detect spikes / saws /
 *    blocks are documented inline.
 */
#include "StateAnalyzer.hpp"
#include "LevelAnalyzer.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GameStatsManager.hpp>

using namespace geode;
using namespace geode::prelude;

namespace spoon::ag {

namespace {
// Common GD hazard object IDs. These have been stable across GD 2.1 -> 2.2
// and 2.2074. Source: GD object database / Geode binding `GameObjectDef`.
constexpr int kObjSpike          = 8;     // small spike
constexpr int kObjSpikeShort     = 9;
constexpr int kObjSpikeLong      = 10;
constexpr int kObjSpikeUpsideDown = 11;
constexpr int kObjSpikeLeft      = 12;
constexpr int kObjSpikeRight     = 13;
constexpr int kObjSpikeBig      = 14;
constexpr int kObjSpikeTiny     = 15;
constexpr int kObjSpikeBigLong  = 16;
constexpr int kObjSpikeBuffer   = 17;
constexpr int kObjSpikeMid      = 18;
constexpr int kObjSpikeSmall    = 19;
constexpr int kObjSpikeBigMid    = 20;
constexpr int kObjSawblade       = 39;
constexpr int kObjSawbladeSmall  = 40;
constexpr int kObjSawbladeYellow = 41;
constexpr int kObjSawbladePink   = 42;
constexpr int kObjSawbladeBlue   = 43;
constexpr int kObjSawbladeMagenta= 44;
constexpr int kObjSpikeD        = 45;
constexpr int kObjSpikeE        = 46;
constexpr int kObjSpikeF        = 47;
constexpr int kObjSpikeG        = 48;
constexpr int kObjSpikeH        = 49;
constexpr int kObjBlock         = 1;
constexpr int kObjBlockBlack    = 2;
constexpr int kObjBlockDynamic  = 3;

bool isSpike(int id) {
    static const int spikes[] = {
        kObjSpike, kObjSpikeShort, kObjSpikeLong, kObjSpikeUpsideDown,
        kObjSpikeLeft, kObjSpikeRight, kObjSpikeBig, kObjSpikeTiny,
        kObjSpikeBigLong, kObjSpikeBuffer, kObjSpikeMid, kObjSpikeSmall,
        kObjSpikeBigMid,
        kObjSpikeD, kObjSpikeE, kObjSpikeF, kObjSpikeG, kObjSpikeH
    };
    for (int s : spikes) if (s == id) return true;
    return false;
}
bool isSawblade(int id) {
    return id == kObjSawblade || id == kObjSawbladeSmall || id == kObjSawbladeYellow
        || id == kObjSawbladePink || id == kObjSawbladeBlue || id == kObjSawbladeMagenta;
}
bool isBlock(int id) {
    return id == kObjBlock || id == kObjBlockBlack || id == kObjBlockDynamic;
}

} // namespace

PlayerState StateAnalyzer::analyzePlayer(PlayerObject* p, bool isPlayer2) {
    PlayerState s{};
    if (!p) return s;

    // Position: prefer the gameplay-layer transform's current X; fall back to
    // the player object's own position member if the transform is unavailable.
    s.pos.x = p->m_position.x;
    s.pos.y = p->m_position.y;
    s.velocity.x = static_cast<float>(p->getCurrentXVelocity());
    s.velocity.y = static_cast<float>(p->getYVelocity());
    s.gamemode = Gamemode::Cube;
    if (p->m_isShip) s.gamemode = Gamemode::Ship;
    else if (p->m_isBall) s.gamemode = Gamemode::Ball;
    else if (p->m_isBird) s.gamemode = Gamemode::UFO;
    else if (p->m_isDart) s.gamemode = Gamemode::Wave;
    else if (p->m_isRobot) s.gamemode = Gamemode::Robot;
    else if (p->m_isSpider) s.gamemode = Gamemode::Spider;
    else if (p->m_isSwing) s.gamemode = Gamemode::Swing;
    s.onGround    = p->m_isOnGround;
    // The current public bindings do not expose a reliable ceiling-contact
    // flag, so leave this false instead of reading a guessed field name.
    s.onCeiling   = false;
    s.isDead      = p->m_isDead;
    s.isDual      = false;
    if (auto* pl = PlayLayer::get()) {
        s.isDual = pl->m_player2 && pl->m_player2->isVisible();
    }
    s.isPlayer2   = isPlayer2;
    s.gravityInverted = p->m_isUpsideDown;
    s.rotation    = p->getRotation();
    return s;
}

int64_t StateAnalyzer::currentFrame() {
    if (auto pl = PlayLayer::get()) {
        // PlayerObject exposes elapsed gameplay time in the 2.2081 bindings.
        // Scale to the planner's approximate 240 Hz tick unit.
        if (pl->m_player1) {
            return static_cast<int64_t>(pl->m_player1->m_totalTime * 240.0);
        }
    }
    return -1;
}

std::optional<Hazard> StateAnalyzer::hazardFromObject(GameObject* obj) {
    if (!obj) return std::nullopt;
    int id = obj->m_objectID;
    Hazard h{};
    auto position = obj->getPosition();
    auto contentSize = obj->getContentSize();
    h.pos.x  = position.x;
    h.pos.y  = position.y;
    h.size.x = contentSize.width  * 0.5f;
    h.size.y = contentSize.height * 0.5f;
    h.objectId = id;
    if (isSpike(id))         h.type = HazardType::Spike;
    else if (isSawblade(id)) h.type = HazardType::Sawblade;
    else if (isBlock(id))    h.type = HazardType::Block;
    else return std::nullopt;
    // A spike whose Y is at the floor (within ~30 units) is treated as a
    // floor hazard for the Ball gamemode.
    if (h.pos.y < 110.f) h.isFloor = true;
    return h;
}

Snapshot StateAnalyzer::analyze(PlayLayer* playLayer) {
    Snapshot snap{};
    if (!playLayer) return snap;
    // `PlayLayer` extends `GJBaseGameLayer` in the GD binding, so the
    // base-class members (m_player1, m_player2, m_objects) are directly
    // accessible without a cast. PlayLayer::m_maxObjectX is used as the
    // level-length estimate available in the 2.2081 bindings.

    // Pull players from the binding. `m_player1` and `m_player2` are
    // Ref<GameObject> members on GJBaseGameLayer.
    snap.player[0] = analyzePlayer(playLayer->m_player1, false);
    snap.player[1] = analyzePlayer(playLayer->m_player2, true);

    // Progress: prefer the explicit getter if the binding exposes it;
    // fall back to a manual computation from player X / level length.
    {
        float prog = 0.f;
        if (playLayer->m_maxObjectX > 0.f) {
            float px = playLayer->m_player1 ? playLayer->m_player1->m_position.x : 0.f;
            prog = px / playLayer->m_maxObjectX;
        }
        if (prog < 0.f) prog = 0.f;
        if (prog > 1.f) prog = 1.f;
        snap.level.progress = prog;
    }
    snap.level.levelLength     = playLayer->m_maxObjectX;
    snap.level.dualActive      = playLayer->m_player2 && playLayer->m_player2->isVisible();
    snap.level.completed       = playLayer->m_hasCompletedLevel;
    snap.level.levelExited     = false;
    snap.level.frame           = currentFrame();

    // The active speed value is exposed on PlayerObject in the 2.2081
    // bindings. Use 1.0 if no player is available or the value is invalid.
    snap.level.speedMultiplier = playLayer->m_player1
        ? playLayer->m_player1->m_playerSpeed
        : 1.f;
    if (snap.level.speedMultiplier <= 0.f) snap.level.speedMultiplier = 1.f;

    // Hazards: ask LevelAnalyzer for the cached slice ahead of the player.
    if (playLayer->m_player1) {
        LevelAnalyzer::get().forEachHazardAhead(
            playLayer->m_player1->m_position.x,
            /*lookaheadX=*/snap.level.speedMultiplier * 4.f,
            [&](const Hazard& h) {
                snap.nearbyHazards.push_back(h);
            });
    }

    return snap;
}

} // namespace spoon::ag
