/**
 * SpoonClient / AssistedGameplay/LevelAnalyzer.hpp
 *
 * Caches a flat, sorted-by-X list of hazards derived from a level's
 * `GJBaseGameLayer::m_objects` ArrayList. This is the only component of
 * Assisted Gameplay that touches the level's object list directly; every
 * other AG subsystem goes through this cache.
 *
 * Cache invalidation
 * ------------------
 * The level object list is built once at level load and never mutated
 * during normal gameplay (no live editing, no spawn-on-the-fly for
 * hazards — only cosmetic FX objects are added). Therefore, we rebuild
 * the cache only on `onLevelStart()`. We do invalidate on
 * `onLevelRestart()` because some mods (practice-mode compatibility) add
 * pseudo-objects that we'd otherwise keep stale.
 *
 * Spatial query
 * --------------
 * `forEachHazardAhead(x, dx, fn)` runs an iterator over the cached hazards
 * whose X coordinate falls within `[x - dx_back, x + dx_forward]`. The
 * caller-supplied callback receives `const Hazard&` by value.
 */
#pragma once
#include <Geode/Geode.hpp>
#include "State.hpp"
#include <functional>
#include <vector>
#include <algorithm>

class PlayLayer;

namespace spoon::ag {

class LevelAnalyzer {
public:
    static LevelAnalyzer& get();

    // Rebuild the cache. Called from Manager::onLevelStart.
    void rebuild(PlayLayer* playLayer);

    // Clear the cache. Called on level exit / PlayLayer destruction.
    void clear();

    // True iff the cache currently holds at least one hazard for the level.
    bool isValid() const { return m_valid; }

    // Iterate over all hazards with X within `[centerX - dxBack,
    // centerX + dxForward]`. Cheap — uses binary-search on a sorted vector.
    void forEachHazardAhead(float centerX, float dxForward,
                            std::function<void(const Hazard&)> fn) const {
        forEachHazardAround(centerX, dxForward, 20.f, std::move(fn));
    }

    // Lower-level: lookup in window [centerX - dxBack, centerX + dxForward].
    void forEachHazardAround(float centerX, float dxForward, float dxBack,
                             std::function<void(const Hazard&)> fn) const;

    // Returns the count of cached hazards (debug + UI display).
    std::size_t hazardCount() const { return m_hazards.size(); }

private:
    LevelAnalyzer();
    static bool sortByX(const Hazard& a, const Hazard& b) {
        return a.pos.x < b.pos.x;
    }

    std::vector<Hazard> m_hazards;
    bool                 m_valid = false;
};

} // namespace spoon::ag
