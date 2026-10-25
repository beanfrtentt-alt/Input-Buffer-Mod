/**
 * SpoonClient / Hooks/PlayLayerHook.cpp
 *
 * The `$modify` block lives here so the static-init hook registrar is
 * emitted exactly once across the mod. Including this file from
 * `HookManager.cpp` is enough to activate the hook.
 */
#include "PlayLayerHook.hpp"
#include "../AssistedGameplay/Manager.hpp"
#include "../AssistedGameplay/StateAnalyzer.hpp"
#include "../BufferManager.hpp"
#include "../UI/SpoonMenuLayer.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(SpoonPlayLayer, PlayLayer) {
    bool init() {
        if (!PlayLayer::init()) return false;
        spoon::BufferManager::get().onLevelStart();
        spoon::ag::Manager::get().onLevelStart(this);
        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);

        // Drain the buffer for this frame. The drain result is consumed by
        // the next PlayerObject::pushButton / releaseButton tick (driven by
        // GD's own input dispatcher) — see docs/BUFFER_AG_INTERACTION.md
        // for the full pipeline.
        int64_t frame = spoon::ag::StateAnalyzer::get().currentFrame();
        (void)spoon::BufferManager::get().drain(frame);

        spoon::ag::Manager::get().onUpdate(this);

        // If the F6 menu is open, refresh its footer (cheap if nothing changed).
        if (spoon::SpoonMenuLayer::isOpen()) {
            auto* scene = cocos2d::CCDirector::get()->getRunningScene();
            if (auto* menu = scene ? scene->getChildByTag(0xC90C) : nullptr) {
                static_cast<spoon::SpoonMenuLayer*>(menu)->refreshFooter();
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        spoon::BufferManager::get().onLevelRestart();
        spoon::ag::Manager::get().onLevelStart(this);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        spoon::BufferManager::get().onLevelComplete();
        // Manager picks up the `completed` flag from the next Snapshot.
    }

    void onQuit() {
        // Run our cleanup BEFORE the original so the PlayLayer's player
        // pointers are still valid when we ask BufferManager to clear.
        spoon::BufferManager::get().clearAll();
        spoon::ag::Manager::get().onLevelExit();
        PlayLayer::onQuit();
    }
};
