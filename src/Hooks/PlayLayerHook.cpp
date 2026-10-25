/**
 * SpoonClient / Hooks/PlayLayerHook.cpp
 *
 * Lifecycle hooks for PlayLayer. Per-frame work lives on GJBaseGameLayer
 * because PlayLayer does not redeclare update() in the 2.2081 bindings.
 */
#include "PlayLayerHook.hpp"
#include "../AssistedGameplay/Manager.hpp"
#include "../BufferManager.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>

using namespace geode::prelude;

class $modify(SpoonPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        spoon::BufferManager::get().onLevelStart();
        spoon::ag::Manager::get().onLevelStart(this);
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        spoon::BufferManager::get().onLevelRestart();
        spoon::ag::Manager::get().onLevelStart(this);
    }

    void resetLevelFromStart() {
        PlayLayer::resetLevelFromStart();
        spoon::BufferManager::get().onLevelRestart();
        spoon::ag::Manager::get().onLevelStart(this);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        spoon::BufferManager::get().onLevelComplete();
    }

    void onQuit() {
        spoon::BufferManager::get().clearAll();
        spoon::ag::Manager::get().onLevelExit();
        PlayLayer::onQuit();
    }
};
