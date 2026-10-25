/**
 * SpoonClient / Hooks/KeyboardDispatchHook.cpp
 *
 * `$modify` block for KeyboardEventDispatcher. The two methods we
 * override are:
 *
 *   * `dispatchKeyboardPressed(enumKeyCodes, bool, bool)` — GD's main
 *     keyboard-press dispatcher. Returning true from this consumes the
 *     event so no other handler (including GD's own menu) sees it.
 *   * `dispatchKeyboardReleased(enumKeyCodes, bool)` — the symmetric
 *     release handler.
 *
 * Note on signatures: the exact parameter set on these methods varies a
 * little between Geode SDK revisions. Geode v5 (against GD 2.2081)
 * exposes them as:
 *
 *     bool KeyboardEventDispatcher::dispatchKeyboardPressed(enumKeyCodes key,
 *                                                            bool isCallbackKeys,
 *                                                            bool isRegularKey);
 *     bool KeyboardEventDispatcher::dispatchKeyboardReleased(enumKeyCodes key,
 *                                                             bool isCallbackKeys);
 *
 * If the binding header in the active SDK version has a different
 * signature, the build will fail at this file. The fix is to update the
 * signatures below to match — see docs/TROUBLESHOOTING.md for steps.
 */
#include "KeyboardDispatchHook.hpp"
#include "../UI/SpoonMenuLayer.hpp"
#include "../AssistedGameplay/Manager.hpp"
#include "../SpoonSettings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/KeyboardEventDispatcher.hpp>

using namespace geode::prelude;

namespace {

// Convert a key-string from Settings ("F6", "F8", etc.) into a
// cocos2d enumKeyCodes value. Returns 0 (the sentinel "no mapping")
// for unrecognised strings.
static int stringToKey(const std::string& s) {
    using namespace cocos2d;
    if (s == "F6")  return static_cast<int>(enumKeyCodes::KEY_F6);
    if (s == "F7")  return static_cast<int>(enumKeyCodes::KEY_F7);
    if (s == "F8")  return static_cast<int>(enumKeyCodes::KEY_F8);
    if (s == "F9")  return static_cast<int>(enumKeyCodes::KEY_F9);
    if (s == "F10") return static_cast<int>(enumKeyCodes::KEY_F10);
    if (s == "F11") return static_cast<int>(enumKeyCodes::KEY_F11);
    if (s == "F12") return static_cast<int>(enumKeyCodes::KEY_F12);
    if (s == "Space") return static_cast<int>(enumKeyCodes::KEY_Space);
    if (s == "P")   return static_cast<int>(enumKeyCodes::KEY_P);
    return 0;
}

} // anonymous namespace

class $modify(SpoonKeyboardEventDispatcher, KeyboardEventDispatcher) {
    bool dispatchKeyboardPressed(enumKeyCodes key, bool isCallbackKeys,
                                  bool isRegularKey) {
        // Check F6 (menu) first; if it matches, toggle the menu and
        // return true (consumed) so GD doesn't also act on the key.
        int k = static_cast<int>(key);
        auto& kb = spoon::Settings::get().keybinds();

        if (k == stringToKey(kb.menu)) {
            spoon::SpoonMenuLayer::toggle();
            return true; // consume
        }
        if (k == stringToKey(kb.emergencyStop)) {
            spoon::ag::Manager::get().emergencyStop();
            return true; // consume
        }

        // Otherwise pass through to GD's dispatcher.
        return KeyboardEventDispatcher::dispatchKeyboardPressed(
            key, isCallbackKeys, isRegularKey);
    }

    bool dispatchKeyboardReleased(enumKeyCodes key, bool isCallbackKeys) {
        // Consume releases for our hotkeys too so GD doesn't see them.
        int k = static_cast<int>(key);
        auto& kb = spoon::Settings::get().keybinds();
        if (k == stringToKey(kb.menu) || k == stringToKey(kb.emergencyStop)) {
            return true;
        }
        return KeyboardEventDispatcher::dispatchKeyboardReleased(key, isCallbackKeys);
    }
};
