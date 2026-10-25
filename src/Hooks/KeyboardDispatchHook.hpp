/**
 * SpoonClient / Hooks/KeyboardDispatchHook.hpp
 *
 * Hooks the keyboard dispatcher to:
 *  1. Open/close the F6 menu when the user presses the menu key.
 *  2. Trigger the Assisted Gameplay emergency-stop key (F8 by default).
 *  3. Consume those keys so GD's default menu doesn't also see them.
 *
 * Implemented via Geode v5's `$modify` on `CCKeyboardDispatcher`.
 */
#pragma once
#include <Geode/Geode.hpp>

namespace spoon::hooks {

class KeyboardDispatchHook {
public:
    static void register_() {}
};

} // namespace spoon::hooks
