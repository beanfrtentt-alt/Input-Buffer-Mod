/**
 * SpoonClient / main.cpp
 *
 * Geode v5 entry: `$on_mod(Loaded)` loads settings and registers keybind
 * listeners. `MenuLayer::init` logs a one-time startup banner.
 */
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/GameEvent.hpp>
#include "SpoonSettings.hpp"
#include "Hooks/HookManager.hpp"
#include "UI/SpoonMenuLayer.hpp"
#include "AssistedGameplay/Manager.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    spoon::Settings::get().load();
    spoon::hooks::HookManager::get().registerAll();

    listenForAllSettingChanges([](std::string_view, std::shared_ptr<SettingV3>) {
        spoon::Settings::get().load();
    });

    listenForKeybindSettingPresses("menuKey", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) {
            spoon::SpoonMenuLayer::toggle();
        }
    });

    listenForKeybindSettingPresses("agEmergencyStopKey", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) {
            spoon::ag::Manager::get().emergencyStop();
        }
    });
}

class $modify(SpoonMenuLayerBoot, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        static bool s_started = false;
        if (!s_started) {
            s_started = true;
            log::info(
                "[SpoonClient] v2.0.0 initialized. "
                "Menu key = '{}', AG emergency stop = '{}'.",
                spoon::Settings::get().keybinds().menu,
                spoon::Settings::get().keybinds().emergencyStop
            );
        }
        return true;
    }
};
