/**
 * SpoonClient / Hooks/BaseGameLayerHook.cpp
 *
 * In earlier revisions this file hooked the GJBaseGameLayer destructor as
 * a defensive cleanup path. With Geode v5's `$modify` macro pattern, dtor
 * overrides add risk without clear benefit, and the PlayLayer::onQuit
 * hook already covers the normal-cleanup case comprehensively.
 *
 * We keep the file (and the no-op HookManager entry) for two reasons:
 *  1. HookManager::registerAll() doesn't need a special-case skip.
 *  2. Future contributors who want to add a defensive dtor hook have
 *     an obvious place to put it.
 *
 * No code is emitted beyond the no-op register_().
 */
#include "BaseGameLayerHook.hpp"

namespace spoon::hooks {
// Intentionally empty — see header doc above.
}
