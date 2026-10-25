/**
 * SpoonClient / Hooks/PlayLayerHook.hpp
 *
 * Hooks PlayLayer's key lifecycle methods using Geode v5's `$modify` macro.
 *
 * The actual `$modify` block lives in `PlayLayerHook.cpp` — putting it in
 * a .cpp file (rather than the header) ensures the hook registrar is
 * instantiated exactly once across the whole mod, otherwise the hook
 * would fire N times for N TUs that include the header.
 *
 *   * `init`           — fires once per level load.
 *   * per-frame work is on GJBaseGameLayer::update (PlayLayer does not
 *     redeclare update in the 2.2081 bindings).
 *   * `resetLevel`     — fires on retry / level restart.
 *   * `levelComplete`  — fires on the win condition.
 *   * `onQuit`         — fires when the user exits to menu.
 *
 * Each override calls the original `PlayLayer::<method>(...)` first to
 * let GD do its own setup, then runs our hook logic.
 */
#pragma once
#include <Geode/Geode.hpp>

namespace spoon::hooks {

class PlayLayerHook {
public:
    // No-op registration retained for HookManager symmetry. With `$modify`,
    // the hook is wired in at static-init time by the macro itself; this
    // function exists purely so HookManager::registerAll() reads cleanly.
    static void register_() {}
};

} // namespace spoon::hooks
