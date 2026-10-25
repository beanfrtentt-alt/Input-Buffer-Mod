/**
 * SpoonClient / UI/BufferPage.hpp
 *
 * The page in the F6 menu that lets the user pick a gamemode and tune its
 * buffer settings. The page is a CCLayer with a small scroll view holding
 * one row per gamemode, plus a "Reset to defaults" button at the bottom.
 *
 * Each row shows:
 *   [Gamemode]   [Enabled ⊗]   [Frames: 8]   [-  +]   [Reset]
 *
 * The frames slider is implemented as a discrete +/- pair rather than
 * GD's Slider class because Slider floats are imprecise for 0..30 frame
 * integer values and the user needs exact control.
 */
#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <functional>
#include "Utils.hpp"

namespace spoon {

class BufferPage : public cocos2d::CCLayer {
public:
    static BufferPage* create();
    bool init() override;

    // Called by the menu when this page becomes visible.
    void refresh();

private:
    struct Row {
        Gamemode gamemode;
        CCMenuItemToggler* enabledToggle = nullptr;
        cocos2d::CCLabelBMFont*     framesLabel   = nullptr;
    };

    void buildRows();
    void onEnabledToggle(cocos2d::CCObject* sender);
    void onMinus(cocos2d::CCObject* sender);
    void onPlus(cocos2d::CCObject* sender);
    void onResetAll(cocos2d::CCObject* sender);
    void onResetOne(cocos2d::CCObject* sender);
    void updateRowLabel(Row& r);

    std::vector<Row> m_rows;
    cocos2d::CCMenu* m_contentMenu = nullptr;
};

} // namespace spoon
