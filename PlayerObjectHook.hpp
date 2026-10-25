/**
 * SpoonClient / Hooks/PlayerObjectHook.hpp
 *
 * Hooks PlayerObject's gamemode-change, push/release-button, and death
 * methods via Geode v5's `$modify` macro. The actual block is in the .cpp.
 */
#pragma once
#include <Geode/Geode.hpp>

namespace spoon::hooks {

class PlayerObjectHook {
public:
    static void register_() {}
};

} // namespace spoon::hooks
