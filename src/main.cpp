/**
 * SpoonClient / main.cpp
 *
 * Entry point for the Geode v5 mod. In v5 the `GEODE_API(Mod)` macro is
 * optional — startup work is typically done by hooking a class whose
 * `init` is called once at game boot. We follow the example-mod pattern
 * and hook `MenuLayer::init` (Geometry Dash's main-menu scene) to:
 *
 *   1. Run settings load + version-aware migration.
 *   2. Log a startup banner so users can confirm in the Geode log viewer
 *      that the mod loaded.
 *
 * The actual gameplay hooks (`PlayLayer`, `PlayerObject`,
 * `KeyboardEventDispatcher`) are activated by `$modify` blocks in their
 * respective hook .cpp files. Geode v5 auto-registers every `$modify`
 * block at static-init time, so there's no central `register_()` call
 * needed.
 */
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "SpoonSettings.hpp"
#include "Hooks/HookManager.hpp"

using namespace geode::prelude;

// `$modify` block — fires when MenuLayer::init is first called (i.e.,
// when the main menu first loads after GD startup). Returning false
// skips our setup but lets GD's normal flow continue.
class $modify(SpoonMenuLayerBoot, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Run one-time startup work. The static-init guard prevents us
        // from re-running on every MenuLayer re-creation (GD rebuilds
        // the menu scene every time the player exits a level back to
        // the main menu, so this init fires repeatedly).
        static bool s_started = false;
        if (!s_started) {
            s_started = true;
            spoon::Settings::get().load();
            spoon::hooks::HookManager::get().registerAll();
            log::info(
                "[SpoonClient] v2.0.0 initialized. "
                "F6 menu key = '{}', AG emergency stop = '{}'.",
                spoon::Settings::get().keybinds().menu,
                spoon::Settings::get().keybinds().emergencyStop
            );
        }
        return true;
    }
};
