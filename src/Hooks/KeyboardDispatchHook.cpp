#include "KeyboardDispatchHook.hpp"
#include "../UI/SpoonMenuLayer.hpp"
#include "../AssistedGameplay/Manager.hpp"
#include "../SpoonSettings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

namespace {
static int stringToKey(const std::string& s) {
    using namespace cocos2d;
    if (s == "F6") return static_cast<int>(enumKeyCodes::KEY_F6);
    if (s == "F7") return static_cast<int>(enumKeyCodes::KEY_F7);
    if (s == "F8") return static_cast<int>(enumKeyCodes::KEY_F8);
    if (s == "F9") return static_cast<int>(enumKeyCodes::KEY_F9);
    if (s == "F10") return static_cast<int>(enumKeyCodes::KEY_F10);
    if (s == "F11") return static_cast<int>(enumKeyCodes::KEY_F11);
    if (s == "F12") return static_cast<int>(enumKeyCodes::KEY_F12);
    if (s == "Space") return static_cast<int>(enumKeyCodes::KEY_Space);
    if (s == "P") return static_cast<int>(enumKeyCodes::KEY_P);
    return static_cast<int>(enumKeyCodes::KEY_Unknown);
}
}

// Geode's 2.2081 binding exposes keyboard input through CCKeyboardDispatcher.
// The hook signature must match dispatchKeyboardMSG exactly.
class $modify(SpoonKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown,
                             bool isKeyRepeat, double timestamp) {
        if (isKeyDown && !isKeyRepeat) {
            const int pressed = static_cast<int>(key);
            const auto& kb = spoon::Settings::get().keybinds();

            if (pressed == stringToKey(kb.menu)) {
                spoon::SpoonMenuLayer::toggle();
                return true;
            }
            if (pressed == stringToKey(kb.emergencyStop)) {
                spoon::ag::Manager::get().emergencyStop();
                return true;
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(
            key, isKeyDown, isKeyRepeat, timestamp);
    }
};
