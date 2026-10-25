/**
 * SpoonClient / Hooks/BaseGameLayerHook.hpp
 *
 * Hooks the GJBaseGameLayer destructor via Geode v5's `$modify` macro.
 * This is a defensive hook — the PlayLayer::onQuit hook already cleans
 * everything up, but if a level is destroyed via an unusual path (e.g.,
 * a quick-edit jump or another mod's intervention) this destructor hook
 * ensures BufferManager + AG state is still flushed.
 */
#pragma once
#include <Geode/Geode.hpp>

namespace spoon::hooks {

class BaseGameLayerHook {
public:
    static void register_() {}
};

} // namespace spoon::hooks
