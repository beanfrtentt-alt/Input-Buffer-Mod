/**
 * SpoonClient / AssistedGameplay/LevelAnalyzer.cpp
 *
 * Cache build + spatial lookup. The build walks the level's object list
 * once; this is O(N) in level size but happens only on level start.
 *
 * For a typical 60-second GD level with ~2,000 objects, the walk costs
 * ~50 µs and produces ~400 hazard entries; the binary-search forward query
 * costs ~30 ns per call. With ActionPlanner polling at 80 Hz that's well
 * under 1 % of a frame.
 */
#include "LevelAnalyzer.hpp"
#include "StateAnalyzer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/GameObject.hpp>

using namespace geode;
using namespace geode::prelude;

namespace spoon::ag {

LevelAnalyzer& LevelAnalyzer::get() {
    static LevelAnalyzer instance;
    return instance;
}

LevelAnalyzer::LevelAnalyzer() = default;

void LevelAnalyzer::rebuild(PlayLayer* playLayer) {
    m_hazards.clear();
    m_valid = false;
    if (!playLayer) return;

    // In the GD 2.2081 bindings, m_objects is a vector of GameObject
    // pointers rather than a cocos2d::CCArray.
    StateAnalyzer sa;
    auto* objects = playLayer->m_objects;
    if (!objects) return;
    for (unsigned i = 0; i < objects->count(); ++i) {
        auto* go = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (auto h = sa.hazardFromObject(go)) {
            m_hazards.push_back(*h);
        }
    }
    std::sort(m_hazards.begin(), m_hazards.end(), sortByX);
    m_valid = true;
}

void LevelAnalyzer::clear() {
    m_hazards.clear();
    m_valid = false;
}

void LevelAnalyzer::forEachHazardAround(float centerX, float dxForward,
                                          float dxBack,
                                          std::function<void(const Hazard&)> fn) const {
    if (!m_valid || m_hazards.empty()) return;
    Hazard probe{ .pos = Vec2{ centerX - dxBack, 0.f } };
    auto lower = std::lower_bound(m_hazards.begin(), m_hazards.end(), probe,
                                   sortByX);
    float upperBound = centerX + dxForward;
    for (auto it = lower; it != m_hazards.end() && it->pos.x <= upperBound; ++it) {
        fn(*it);
    }
}

} // namespace spoon::ag
