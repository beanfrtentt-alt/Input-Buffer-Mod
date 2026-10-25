/**
 * SpoonClient / AssistedGameplay/OverlayUI.hpp
 *
 * Small, non-interactive CCLayer shown on top of the play layer while AG
 * is enabled. The overlay:
 *   * Shows the current AG Status (Idle / Active / Recovering / ...).
 *   * Shows the active gamemode label and the active buffer frame count.
 *   * Shows the last planner reason ("Floor hazard ahead — jump").
 *
 * Design constraints:
 *   * The overlay MUST NOT eat input events. It has `setTouchEnabled(false)`
 *     and `setKeyboardEnabled(false)` so cocos2d skips it entirely when
 *     dispatching touches / keys.
 *   * The overlay is rebuilt only when its visible state changes (status,
 *     reason string, gamemode). So even if it's drawn every frame by
 *     cocos2d, no text is re-rendered unless something changed.
 *   * The overlay is parented to the PlayLayer's parent CCDirector scene,
 *     so it survives the player respawn (PlayLayer is reused across
 *     respawns).
 */
#pragma once
#include <Geode/Geode.hpp>
#include "State.hpp"

class PlayLayer;

namespace spoon::ag {

class OverlayUI {
public:
    static OverlayUI& get();

    // Show / hide the overlay. Called from Manager::setEnabled.
    void show();
    void hide();

    // Update visible state. Cheap; only mutates CCNodes if state differs.
    void update(Status s, std::string_view reason, Gamemode gm, int bufFrames);

private:
    OverlayUI();
    void build();
    void rebuild();

    cocos2d::CCLayer*      m_layer        = nullptr;
    cocos2d::CCLabelBMFont* m_titleLbl   = nullptr;
    cocos2d::CCLabelBMFont* m_statusLbl   = nullptr;
    cocos2d::CCLabelBMFont* m_gamemodeLbl = nullptr;
    cocos2d::CCLabelBMFont* m_bufferLbl   = nullptr;
    cocos2d::CCLabelBMFont* m_reasonLbl   = nullptr;

    Status     m_lastStatus = Status::Idle;
    Gamemode   m_lastGm     = Gamemode::Unknown;
    int        m_lastBuf    = -1;
    std::string m_lastReason;
    bool        m_visible   = false;
};

} // namespace spoon::ag
