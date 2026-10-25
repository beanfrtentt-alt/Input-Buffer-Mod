/**
 * SpoonClient / UI/AssistedGameplayPage.hpp
 *
 * The page in the F6 menu dedicated to Assisted Gameplay. Provides:
 *   - The big "Assisted Gameplay" enable toggle.
 *   - Strategy dropdown (receding-horizon vs static-analysis).
 *   - Lookahead slider (4..90 frames).
 *   - Show-overlay toggle.
 *   - Debug-logging toggle.
 *   - Emergency-stop keybind entry.
 *   - Current status label (live).
 *   - Last planner reason label (live).
 */
#pragma once
#include <Geode/Geode.hpp>
#include "../AssistedGameplay/State.hpp"

namespace spoon {

class AssistedGameplayPage : public cocos2d::CCLayer {
public:
    static AssistedGameplayPage* create();
    bool init() override;

    // Live UI refresh — called every gameplay tick while the menu is open
    // so the status + reason labels stay up to date.
    void refreshLive();

private:
    void onToggleEnabled(cocos2d::CCObject*);
    void onToggleOverlay(cocos2d::CCObject*);
    void onToggleDebug(cocos2d::CCObject*);
    void onStrategyLeft(cocos2d::CCObject*);
    void onStrategyRight(cocos2d::CCObject*);
    void onLookaheadMinus(cocos2d::CCObject*);
    void onLookaheadPlus(cocos2d::CCObject*);
    void onEmergencyStop(cocos2d::CCObject*);

    cocos2d::CCMenuItemToggler* m_enableToggle = nullptr;
    cocos2d::CCLabelBMFont*     m_strategyLabel = nullptr;
    cocos2d::CCLabelBMFont*     m_lookaheadLabel = nullptr;
    cocos2d::CCMenuItemToggler* m_overlayToggle = nullptr;
    cocos2d::CCMenuItemToggler* m_debugToggle    = nullptr;
    cocos2d::CCLabelBMFont*     m_statusLabel   = nullptr;
    cocos2d::CCLabelBMFont*     m_reasonLabel   = nullptr;
};

} // namespace spoon
