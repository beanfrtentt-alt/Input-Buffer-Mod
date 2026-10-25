/**
 * SpoonClient / UI/AssistedGameplayPage.cpp
 *
 * Lays out the AG controls in a single vertical column inside the F6 panel.
 *
 * The layout uses CCMenu for hit-testing; everything is local coordinates
 * with the origin at the bottom-left of the panel content area.
 */
#include "AssistedGameplayPage.hpp"
#include "../SpoonSettings.hpp"
#include "../AssistedGameplay/Manager.hpp"
#include "../AssistedGameplay/OverlayUI.hpp"
#include "../BufferManager.hpp"
#include <Geode/Geode.hpp>
#include <cstdio>

using namespace geode;
using namespace geode::prelude;
using namespace cocos2d;

namespace spoon {

static const ccColor3B kLabelColor  { 220, 220, 220 };
static const ccColor3B kAccentColor { 255, 220, 90  };

AssistedGameplayPage* AssistedGameplayPage::create() {
    auto* p = new AssistedGameplayPage();
    if (p && p->init()) { p->autorelease(); return p; }
    CC_SAFE_DELETE(p);
    return nullptr;
}

bool AssistedGameplayPage::init() {
    if (!CCLayer::init()) return false;

    auto& ag = Settings::get().ag();
    auto* menu = CCMenu::create();
    menu->setPosition({0, 0});
    menu->setTouchEnabled(true);
    addChild(menu, 1);

    // === Row 1: Big enable toggle ============================================
    constexpr float kTopY = 230.f;
    auto* tOn  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    auto* tOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto* enBtn = CCMenuItemToggler::create(tOff, tOn, this,
        menu_selector(AssistedGameplayPage::onToggleEnabled));
    enBtn->setPosition({ -200.f, kTopY });
    enBtn->toggle(ag.enabled);
    menu->addChild(enBtn);
    m_enableToggle = enBtn;

    auto* enLbl = CCLabelBMFont::create("Assisted Gameplay", "bigFont.fnt");
    enLbl->setScale(0.7f);
    enLbl->setColor(kAccentColor);
    enLbl->setAnchorPoint({0.f, 0.5f});
    auto* enItem = CCMenuItemSpriteExtra::create(enLbl, enLbl, nullptr, nullptr);
    enItem->setEnabled(false);
    enItem->setPosition({ -170.f, kTopY });
    menu->addChild(enItem);

    // === Row 2: Status (live) ================================================
    m_statusLabel = CCLabelBMFont::create("Status: Idle", "bigFont.fnt");
    m_statusLabel->setScale(0.55f);
    m_statusLabel->setColor(kLabelColor);
    m_statusLabel->setAnchorPoint({0.f, 0.5f});
    auto* sItem = CCMenuItemSpriteExtra::create(m_statusLabel, m_statusLabel,
        nullptr, nullptr);
    sItem->setEnabled(false);
    sItem->setPosition({ -330.f, kTopY - 30.f });
    menu->addChild(sItem);

    // === Row 3: Last reason (live) ==========================================
    m_reasonLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_reasonLabel->setScale(0.5f);
    m_reasonLabel->setColor({255, 220, 90});
    m_reasonLabel->setAnchorPoint({0.f, 0.5f});
    auto* rItem = CCMenuItemSpriteExtra::create(m_reasonLabel, m_reasonLabel,
        nullptr, nullptr);
    rItem->setEnabled(false);
    rItem->setPosition({ -330.f, kTopY - 56.f });
    menu->addChild(rItem);

    // === Row 4: Strategy picker =============================================
    auto* stratLbl = CCLabelBMFont::create("Strategy:", "bigFont.fnt");
    stratLbl->setScale(0.55f);
    stratLbl->setColor(kLabelColor);
    stratLbl->setAnchorPoint({0.f, 0.5f});
    auto* slItem = CCMenuItemSpriteExtra::create(stratLbl, stratLbl, nullptr, nullptr);
    slItem->setEnabled(false);
    slItem->setPosition({ -330.f, kTopY - 92.f });
    menu->addChild(slItem);

    m_strategyLabel = CCLabelBMFont::create("Receding Horizon", "bigFont.fnt");
    m_strategyLabel->setScale(0.55f);
    m_strategyLabel->setColor(kAccentColor);
    m_strategyLabel->setAnchorPoint({0.5f, 0.5f});
    auto* sVal = CCMenuItemSpriteExtra::create(m_strategyLabel, m_strategyLabel,
        nullptr, nullptr);
    sVal->setEnabled(false);
    sVal->setPosition({ -120.f, kTopY - 92.f });
    menu->addChild(sVal);

    auto* leftBtn = CCMenuItemSpriteExtra::create(
        CCLabelBMFont::create("<", "bigFont.fnt"),
        CCLabelBMFont::create("<", "bigFont.fnt"),
        this, menu_selector(AssistedGameplayPage::onStrategyLeft));
    leftBtn->setPosition({ -180.f, kTopY - 92.f });
    menu->addChild(leftBtn);

    auto* rightBtn = CCMenuItemSpriteExtra::create(
        CCLabelBMFont::create(">", "bigFont.fnt"),
        CCLabelBMFont::create(">", "bigFont.fnt"),
        this, menu_selector(AssistedGameplayPage::onStrategyRight));
    rightBtn->setPosition({ -60.f, kTopY - 92.f });
    menu->addChild(rightBtn);

    // === Row 5: Lookahead slider (manual -/+ for precise integer frames) ==
    auto* laLbl = CCLabelBMFont::create("Lookahead:", "bigFont.fnt");
    laLbl->setScale(0.55f);
    laLbl->setColor(kLabelColor);
    laLbl->setAnchorPoint({0.f, 0.5f});
    auto* laItem = CCMenuItemSpriteExtra::create(laLbl, laLbl, nullptr, nullptr);
    laItem->setEnabled(false);
    laItem->setPosition({ -330.f, kTopY - 122.f });
    menu->addChild(laItem);

    m_lookaheadLabel = CCLabelBMFont::create("18f", "bigFont.fnt");
    m_lookaheadLabel->setScale(0.55f);
    m_lookaheadLabel->setColor(kAccentColor);
    m_lookaheadLabel->setAnchorPoint({0.5f, 0.5f});
    auto* laVal = CCMenuItemSpriteExtra::create(m_lookaheadLabel, m_lookaheadLabel,
        nullptr, nullptr);
    laVal->setEnabled(false);
    laVal->setPosition({ -120.f, kTopY - 122.f });
    menu->addChild(laVal);

    auto* laMinus = CCMenuItemSpriteExtra::create(
        CCLabelBMFont::create("-", "bigFont.fnt"),
        CCLabelBMFont::create("-", "bigFont.fnt"),
        this, menu_selector(AssistedGameplayPage::onLookaheadMinus));
    laMinus->setPosition({ -180.f, kTopY - 122.f });
    menu->addChild(laMinus);

    auto* laPlus = CCMenuItemSpriteExtra::create(
        CCLabelBMFont::create("+", "bigFont.fnt"),
        CCLabelBMFont::create("+", "bigFont.fnt"),
        this, menu_selector(AssistedGameplayPage::onLookaheadPlus));
    laPlus->setPosition({ -60.f, kTopY - 122.f });
    menu->addChild(laPlus);

    // === Row 6: Show overlay toggle =========================================
    auto* ovOn  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    auto* ovOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto* ovBtn = CCMenuItemToggler::create(ovOff, ovOn, this,
        menu_selector(AssistedGameplayPage::onToggleOverlay));
    ovBtn->setPosition({ -200.f, kTopY - 154.f });
    ovBtn->toggle(ag.showOverlay);
    menu->addChild(ovBtn);
    m_overlayToggle = ovBtn;

    auto* ovLbl = CCLabelBMFont::create("Show overlay", "bigFont.fnt");
    ovLbl->setScale(0.55f);
    ovLbl->setColor(kLabelColor);
    ovLbl->setAnchorPoint({0.f, 0.5f});
    auto* ovItem = CCMenuItemSpriteExtra::create(ovLbl, ovLbl, nullptr, nullptr);
    ovItem->setEnabled(false);
    ovItem->setPosition({ -170.f, kTopY - 154.f });
    menu->addChild(ovItem);

    // === Row 7: Debug logs toggle ==========================================
    auto* dbgOn  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    auto* dbgOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto* dbgBtn = CCMenuItemToggler::create(dbgOff, dbgOn, this,
        menu_selector(AssistedGameplayPage::onToggleDebug));
    dbgBtn->setPosition({ -200.f, kTopY - 184.f });
    dbgBtn->toggle(ag.showDebug);
    menu->addChild(dbgBtn);
    m_debugToggle = dbgBtn;

    auto* dbgLbl = CCLabelBMFont::create("Debug logs", "bigFont.fnt");
    dbgLbl->setScale(0.55f);
    dbgLbl->setColor(kLabelColor);
    dbgLbl->setAnchorPoint({0.f, 0.5f});
    auto* dbgItem = CCMenuItemSpriteExtra::create(dbgLbl, dbgLbl, nullptr, nullptr);
    dbgItem->setEnabled(false);
    dbgItem->setPosition({ -170.f, kTopY - 184.f });
    menu->addChild(dbgItem);

    // === Row 8: Emergency stop ==============================================
    auto* esBtn = CCMenuItemSpriteExtra::create(
        CCLabelBMFont::create("STOP NOW (F8)", "bigFont.fnt"),
        CCLabelBMFont::create("STOP NOW (F8)", "bigFont.fnt"),
        this, menu_selector(AssistedGameplayPage::onEmergencyStop));
    esBtn->setPosition({ 0.f, kTopY - 220.f });
    menu->addChild(esBtn);

    refreshLive();
    return true;
}

void AssistedGameplayPage::refreshLive() {
    using namespace spoon::ag;
    auto& s = Settings::get();
    auto st = Manager::get().status();
    if (m_statusLabel) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Status: %s", statusLabel(st).data());
        m_statusLabel->setString(buf);
    }
    if (m_strategyLabel) {
        m_strategyLabel->setString(
            s.ag().strategy == "static-analysis" ? "Static Analysis"
                                                 : "Receding Horizon");
    }
    if (m_lookaheadLabel) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%df", s.ag().lookaheadFrames);
        m_lookaheadLabel->setString(buf);
    }
}

void AssistedGameplayPage::onToggleEnabled(CCObject* sender) {
    auto* t = static_cast<CCMenuItemToggler*>(sender);
    Manager::get().setEnabled(t->isToggled());
}

void AssistedGameplayPage::onToggleOverlay(CCObject* sender) {
    auto* t = static_cast<CCMenuItemToggler*>(sender);
    auto& s = Settings::get();
    s.agMut().showOverlay = t->isToggled();
    s.saveAgSettings();
    if (s.agMut().showOverlay && s.ag().enabled) {
        spoon::ag::OverlayUI::get().show();
    } else {
        spoon::ag::OverlayUI::get().hide();
    }
}

void AssistedGameplayPage::onToggleDebug(CCObject* sender) {
    auto* t = static_cast<CCMenuItemToggler*>(sender);
    auto& s = Settings::get();
    s.agMut().showDebug = t->isToggled();
    s.saveAgSettings();
}

void AssistedGameplayPage::onStrategyLeft(CCObject*) {
    auto& s = Settings::get();
    s.agMut().strategy = "static-analysis";
    s.saveAgSettings();
    refreshLive();
}

void AssistedGameplayPage::onStrategyRight(CCObject*) {
    auto& s = Settings::get();
    s.agMut().strategy = "receding-horizon";
    s.saveAgSettings();
    refreshLive();
}

void AssistedGameplayPage::onLookaheadMinus(CCObject*) {
    auto& s = Settings::get();
    s.agMut().lookaheadFrames = clamp(s.agMut().lookaheadFrames - 1, 4, 90);
    s.saveAgSettings();
    refreshLive();
}

void AssistedGameplayPage::onLookaheadPlus(CCObject*) {
    auto& s = Settings::get();
    s.agMut().lookaheadFrames = clamp(s.agMut().lookaheadFrames + 1, 4, 90);
    s.saveAgSettings();
    refreshLive();
}

void AssistedGameplayPage::onEmergencyStop(CCObject*) {
    spoon::ag::Manager::get().emergencyStop();
}

} // namespace spoon
