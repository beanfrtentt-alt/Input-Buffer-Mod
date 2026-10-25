/**
 * SpoonClient / UI/SpoonMenuLayer.cpp
 *
 * Builds the panel and three tabs. The actual page content lives in
 * BufferPage / AssistedGameplayPage / ModulesPage — this file only wires
 * them into tabs and handles the close / F6 / escape keys.
 */
#include "SpoonMenuLayer.hpp"
#include "BufferPage.hpp"
#include "AssistedGameplayPage.hpp"
#include "ModulesPage.hpp"
#include "../SpoonSettings.hpp"
#include "../BufferManager.hpp"
#include <Geode/Geode.hpp>
#include <cstdio>

using namespace geode;
using namespace geode::prelude;
using namespace cocos2d;

namespace spoon {

bool SpoonMenuLayer::s_isOpen = false;

static const ccColor4B kPanelColor { 24, 26, 32, 240 };

SpoonMenuLayer* SpoonMenuLayer::create() {
    auto* p = new SpoonMenuLayer();
    if (p && p->init()) {
        p->autorelease();
        return p;
    }
    CC_SAFE_DELETE(p);
    return nullptr;
}

bool SpoonMenuLayer::init() {
    if (!CCLayer::init()) return false;

    setTouchEnabled(true);
    setKeyboardEnabled(true);
    setKeypadEnabled(true);

    CCSize win = CCDirector::get()->getWinSize();

    // Dim background so the menu reads against any level backdrop.
    auto* dim = CCLayerColor::create({ 0, 0, 0, 120 });
    dim->setTouchEnabled(false);
    dim->setKeyboardEnabled(false);
    addChild(dim, 0);

    // Panel: 720 x 540 centered. CCScale9Sprite gives us rounded corners.
    m_panel = CCScale9Sprite::create("GJ_square01.png");
    m_panel->setColor({ 24, 26, 32 });
    m_panel->setOpacity(240);
    m_panel->setContentSize({ 720.f, 540.f });
    m_panel->setPosition({ win.width / 2.f, win.height / 2.f });
    addChild(m_panel, 1);

    // Title label
    m_titleLbl = CCLabelBMFont::create("Spoon Client", "bigFont.fnt");
    m_titleLbl->setScale(0.9f);
    m_titleLbl->setColor({ 255, 220, 90 });
    m_titleLbl->setPosition({ win.width / 2.f, win.height / 2.f + 235.f });
    addChild(m_titleLbl, 2);

    // Close button (top-right of panel)
    auto* closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    auto* closeBtn = CCMenuItemSpriteExtra::create(
        closeSpr, closeSpr,
        this, menu_selector(SpoonMenuLayer::onClose));
    closeBtn->setPosition({ 720.f * 0.5f - 22.f, 540.f * 0.5f - 22.f });

    auto* closeMenu = CCMenu::create(closeBtn, nullptr);
    closeMenu->setPosition({0,0});
    m_panel->addChild(closeMenu, 99);

    // Tabs: Modules | Buffer | Assisted Gameplay
    constexpr float kTabSpacing = 130.f;
    constexpr float kTabY = 540.f * 0.5f - 32.f;

    // Helper: build a tab button pointing at a member-function selector.
    // The tab index is stashed on the button's tag.
    auto makeTabBtn = [this](const char* text, int idx, SEL_MenuHandler sel) {
        auto* b = CCMenuItemSpriteExtra::create(
            CCLabelBMFont::create(text, "bigFont.fnt"),
            CCLabelBMFont::create(text, "bigFont.fnt"),
            this, sel);
        b->setTag(idx);
        return b;
    };

    // Tab buttons are positioned relative to panel center.
    auto* modsBtn  = makeTabBtn("Modules", 0,
        menu_selector(SpoonMenuLayer::onTabSelect));
    auto* bufBtn   = makeTabBtn("Buffer",  1,
        menu_selector(SpoonMenuLayer::onTabSelect));
    auto* agBtn    = makeTabBtn("Assisted", 2,
        menu_selector(SpoonMenuLayer::onTabSelect));
    modsBtn ->setPosition({ -kTabSpacing, kTabY });
    bufBtn  ->setPosition({         0,     kTabY });
    agBtn   ->setPosition({  kTabSpacing, kTabY });

    m_tabMenu = CCMenu::create(modsBtn, bufBtn, agBtn, nullptr);
    m_tabMenu->setPosition({ win.width / 2.f, win.height / 2.f });
    addChild(m_tabMenu, 2);

    // Page containers
    m_modulesPage = ModulesPage::create();
    m_bufferPage  = BufferPage::create();
    m_agPage      = AssistedGameplayPage::create();
    m_modulesPage->setPosition({ win.width / 2.f - 360.f, win.height / 2.f - 250.f });
    m_bufferPage ->setPosition({ win.width / 2.f - 360.f, win.height / 2.f - 250.f });
    m_agPage     ->setPosition({ win.width / 2.f - 360.f, win.height / 2.f - 250.f });
    addChild(m_modulesPage, 3);
    addChild(m_bufferPage,  3);
    addChild(m_agPage,      3);

    // Footer: active gamemode + active buffer
    m_footerLbl = CCLabelBMFont::create("", "bigFont.fnt");
    m_footerLbl->setScale(0.55f);
    m_footerLbl->setColor({ 200, 200, 200 });
    m_footerLbl->setPosition({ win.width / 2.f, win.height / 2.f - 270.f });
    addChild(m_footerLbl, 2);

    showPage(0);
    refreshFooter();
    s_isOpen = true;
    return true;
}

void SpoonMenuLayer::showPage(int idx) {
    m_currentPage = idx;
    m_modulesPage->setVisible(idx == 0);
    m_bufferPage ->setVisible(idx == 1);
    m_agPage     ->setVisible(idx == 2);
}

void SpoonMenuLayer::onClose(CCObject*) {
    s_isOpen = false;
    removeFromParent();
}

void SpoonMenuLayer::onTabSelect(CCObject* sender) {
    if (!sender) return;
    int idx = sender->getTag();
    showPage(idx);
}

void SpoonMenuLayer::keyDown(enumKeyCodes key, double timestamp) {
    if (key == enumKeyCodes::KEY_Escape || key == enumKeyCodes::KEY_F6) {
        onClose(nullptr);
        return;
    }
    switch (m_currentPage) {
        case 0: m_modulesPage->keyDown(key, timestamp); break;
        case 1: m_bufferPage->keyDown(key, timestamp);  break;
        case 2: m_agPage->keyDown(key, timestamp);      break;
    }
}

void SpoonMenuLayer::keyUp(enumKeyCodes, double) {
}

void SpoonMenuLayer::refreshFooter() {
    if (!m_footerLbl) return;
    auto gm = BufferManager::get().activeGamemode(false);
    int frames = BufferManager::get().bufferFrames(false);
    auto& s = Settings::get();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "Active gamemode: %s   |   Buffer: %d frames   |   AG: %s",
                  gamemodeName(gm).data(), frames,
                  s.ag().enabled ? "ON" : "OFF");
    m_footerLbl->setString(buf);
}

void SpoonMenuLayer::toggle() {
    auto* scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    if (s_isOpen) {
        // Find existing instance and close it.
        auto* existing = static_cast<SpoonMenuLayer*>(
            scene->getChildByTag(/*tag=*/0xC90C));
        if (existing) existing->onClose(nullptr);
        return;
    }
    auto* menu = SpoonMenuLayer::create();
    if (!menu) return;
    menu->setTag(0xC90C);
    scene->addChild(menu, /*z=*/10000);
}

} // namespace spoon
