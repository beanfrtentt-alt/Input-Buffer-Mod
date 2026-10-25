/**
 * SpoonClient / UI/SpoonMenuLayer.hpp
 *
 * The F6 menu. This is a modal CCLayer shown over the running scene. It
 * captures keyboard + touch input so it can be navigated safely while the
 * game continues to run.
 *
 * Layout
 * ------
 *   ┌─────────────────────────────────────────┐
 *   │ Spoon Client                  [X] close │
 *   │ ─────────────────────────────────────── │
 *   │  Tabs:                                  │
 *   │   [ Modules ] [ Buffer ] [ AG ]         │
 *   │  ─────────────────────────────────────  │
 *   │                                         │
 *   │       (active page content, scrollable)  │
 *   │                                         │
 *   │  ─────────────────────────────────────  │
 *   │  Active gamemode: Cube | Buffer: 8f     │
 *   └─────────────────────────────────────────┘
 *
 * Implementation
 * ---------------
 * The menu is a CCLayer with touch+keyboard enabled that intercepts all
 * touch events when open (so the gameplay doesn't get them). Tab switching
 * is implemented by showing/hiding three child CCLayers (one per page)
 * rather than rebuilding — switching is O(1) and the scroll position of
 * each page is preserved.
 *
 * The menu is a singleton per-process: if the user presses F6 while the
 * menu is already open, we just close it.
 */
#pragma once
#include <Geode/Geode.hpp>

class PlayLayer;

namespace spoon {

class BufferPage;
class AssistedGameplayPage;
class ModulesPage;

class SpoonMenuLayer : public cocos2d::CCLayer {
public:
    static SpoonMenuLayer* create();
    bool init() override;
    void keyDown(cocos2d::enumKeyCodes key) override;
    void keyUp(cocos2d::enumKeyCodes key) override;
    bool ccTouchBegan(cocos2d::CCTouch* t, cocos2d::CCEvent* e) override { return true; }
    void ccTouchMoved(cocos2d::CCTouch*, cocos2d::CCEvent*) override {}
    void ccTouchEnded(cocos2d::CCTouch*, cocos2d::CCEvent*) override {}
    void ccTouchCancelled(cocos2d::CCTouch*, cocos2d::CCEvent*) override {}

    // Open / close from anywhere — used by the keyboard hook.
    static void toggle();
    static bool isOpen() { return s_isOpen; }

    // Refresh the footer's "Active gamemode" line. Called every gameplay
    // tick by the play-layer hook when the menu is open.
    void refreshFooter();

private:
    void showPage(int idx);
    void onClose(cocos2d::CCObject*);
    void onTabSelect(cocos2d::CCObject*);   // reads sender->getTag() to pick the page

    cocos2d::CCMenu*           m_tabMenu   = nullptr;
    cocos2d::CCLabelBMFont*    m_titleLbl  = nullptr;
    cocos2d::CCLabelBMFont*    m_footerLbl = nullptr;
    cocos2d::extension::CCScale9Sprite*   m_panel     = nullptr;

    BufferPage*             m_bufferPage = nullptr;
    AssistedGameplayPage*   m_agPage     = nullptr;
    ModulesPage*            m_modulesPage= nullptr;

    int                       m_currentPage = 0;
    static bool               s_isOpen;
};

} // namespace spoon
