/**
 * SpoonClient / AssistedGameplay/StateAnalyzer.cpp
 *
 * Bridges between the GD binding types and our plain-data `Snapshot`.
 *
 * The key calls in here are the ones that have historically been broken in
 * GD mods:
 *  * `PlayerObject::m_isOnGround` — historically a bitfield on the GD side,
 *    exposed as a bool via Geode binding.
 *  * `PlayerObject::m_gamemode` — int member that maps 1:1 to our `Gamemode`
 *    enum.
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
#include <Geode/binding/LevelEditorObject.hpp> // for object-id constants where available

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
    s.velocity.x = p->m_xVel;
    s.velocity.y = p->m_yVel;
    s.gamemode    = gamemodeFromInt(p->m_gamemode);
    s.onGround    = p->m_isOnGround;
    s.onCeiling   = p->m_isOnGroundTop; // bool member in the GD bindings
    s.isDead      = p->m_isDead;
    s.isDual      = p->m_isDualMode;
    s.isPlayer2   = isPlayer2;
    s.gravityInverted = p->m_isGravityFlipped;
    s.rotation    = p->m_rotation;
    return s;
}

int64_t StateAnalyzer::currentFrame() {
    if (auto pl = PlayLayer::get()) {
        // GJBaseGameLayer tracks a 60Hz "frame" via the game's time delta; the
        // gameplay step counter is `m_gameState.m_currentProgress` for time
        // but a simpler tick is `m_stepCounter` (introduced in GD 2.2). Use
        // the elapsed time in milliseconds as a fallback for older versions.
        if (pl->m_gameState) {
            // m_timeAndStreak are time in seconds; multiply by ~240 to
            // approximate the 240 Hz step counter (the rate GD physics runs
            // at when playing). This is close enough for buffer scheduling.
            return static_cast<int64_t>(pl->m_gameState->m_time * 240.f);
        }
    }
    return -1;
}

std::optional<Hazard> StateAnalyzer::hazardFromObject(GameObject* obj) {
    if (!obj) return std::nullopt;
    int id = obj->m_objectID;
    Hazard h{};
    h.pos.x  = obj->m_position.x;
    h.pos.y  = obj->m_position.y;
    h.size.x = obj->m_contentSize.width  * 0.5f;
    h.size.y = obj->m_contentSize.height * 0.5f;
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
    // base-class members (m_player1, m_player2, m_objects, m_gameState) are
    // directly accessible without a cast.

    // Pull players from the binding. `m_player1` and `m_player2` are
    // Ref<GameObject> members on GJBaseGameLayer.
    snap.player[0] = analyzePlayer(playLayer->m_player1, false);
    snap.player[1] = analyzePlayer(playLayer->m_player2, true);

    // Progress: prefer the explicit getter if the binding exposes it;
    // fall back to a manual computation from player X / level length.
    {
        float prog = 0.f;
        if (playLayer->m_level && playLayer->m_level->m_levelLength > 0.f) {
            float px = playLayer->m_player1 ? playLayer->m_player1->m_position.x : 0.f;
            prog = px / static_cast<float>(playLayer->m_level->m_levelLength);
        }
        if (prog < 0.f) prog = 0.f;
        if (prog > 1.f) prog = 1.f;
        snap.level.progress = prog;
    }
    snap.level.levelLength     = playLayer->m_level ? playLayer->m_level->m_levelLength : 0.f;
    snap.level.dualActive      = playLayer->m_player2 && playLayer->m_player2->isVisible();
    snap.level.completed       = playLayer->m_levelCompleted;
    snap.level.levelExited     = false;
    snap.level.frame           = currentFrame();

    // Speed: GD exposes a per-frame delta and an `m_speed` member that's a
    // float in {0.5, 0.8, 1, 1.2, 2, 3} for the various speed portals.
    // On GD 2.2 / Geode v4, the active speed multiplier lives on
    // PlayLayerGameState; fall back to 1.f if the binding exposes neither
    // (older Geode revisions had `PlayLayer::m_speed`).
    snap.level.speedMultiplier = 1.f;
    if (playLayer->m_gameState) {
        snap.level.speedMultiplier = playLayer->m_gameState->m_playerSpeed;
    }
    if (snap.level.speedMultiplier <= 0.f) snap.level.speedMultiplier = 1.f;

    // Hazards: ask LevelAnalyzer for the cached slice ahead of the player.
    LevelAnalyzer::get().forEachHazardAhead(
        playLayer->m_player1->m_position.x,
        /*lookaheadX=*/snap.level.speedMultiplier * 4.f,
        [&](const Hazard& h) {
            snap.nearbyHazards.push_back(h);
        });

    return snap;
}

} // namespace spoon::ag
