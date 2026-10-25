/**
 * SpoonClient / UI/BufferPage.cpp
 *
 * Implementation. The layout is vertical: each gamemode is a row about
 * 38 points tall. The whole page has 8 rows + a "reset all" button —
 * that's ~340 points of content, which fits inside the panel's content
 * area without needing a ScrollView at most GD resolutions.
 *
 * For 4K / windowed-at-720p cases we attach a CCScrollView and let it
 * scroll; the menu's parent (SpoonMenuLayer) handles the page itself
 * (the panel scroll is page-local, not menu-level).
 */
#include "BufferPage.hpp"
#include "../SpoonSettings.hpp"
#include "../BufferManager.hpp"
#include <Geode/Geode.hpp>
#include <cstdio>

using namespace geode;
using namespace geode::prelude;
using namespace cocos2d;

namespace spoon {

static const ccColor3B kLabelColor  { 220, 220, 220 };
static const ccColor3B kAccentColor { 255, 220, 90  };

BufferPage* BufferPage::create() {
    auto* p = new BufferPage();
    if (p && p->init()) {
        p->autorelease();
        return p;
    }
    CC_SAFE_DELETE(p);
    return nullptr;
}

bool BufferPage::init() {
    if (!CCLayer::init()) return false;
    buildRows();
    return true;
}

static CCMenuItemSpriteExtra* makeBtn(const char* text, float scale,
                                     CCObject* target, SEL_MenuHandler sel) {
    auto* l = CCLabelBMFont::create(text, "bigFont.fnt");
    l->setScale(scale);
    auto* bg = CCScale9Sprite::create("GJ_button_01.png");
    bg->setContentSize({ 26.f, 22.f });
    return CCMenuItemSpriteExtra::create(bg, bg, target, sel);
}

void BufferPage::buildRows() {
    constexpr float kRowH = 36.f;
    constexpr float kRowYTop = 0.f;

    auto* menu = CCMenu::create();
    menu->setPosition({0, 0});
    menu->setTouchEnabled(true);
    addChild(menu, 1);
    m_contentMenu = menu;

    auto& settings = Settings::get();

    for (int i = 0; i < 8; ++i) {
        Gamemode gm = static_cast<Gamemode>(i);
        const auto& profile = settings.profile(gm);

        float y = kRowYTop - i * kRowH;

        // Gamemode name label
        auto* nameLbl = CCLabelBMFont::create(gamemodeName(gm).data(), "bigFont.fnt");
        nameLbl->setScale(0.6f);
        nameLbl->setColor(kAccentColor);
        nameLbl->setAnchorPoint({0.f, 0.5f});
        auto* nameItem = CCMenuItemSpriteExtra::create(nameLbl, nameLbl, nullptr, nullptr);
        nameItem->setPosition({ -330.f, y });
        nameItem->setEnabled(false);
        menu->addChild(nameItem);

        // Enabled toggle (using GD's toggle button sprite)
        auto* toggleOn  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        auto* toggleOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        auto* tog = CCMenuItemToggler::create(toggleOff, toggleOn, this,
            menu_selector(BufferPage::onEnabledToggle));
        tog->setPosition({ -180.f, y });
        tog->setTag(i);
        tog->toggle(profile.enabled);
        menu->addChild(tog);

        // Frames label
        auto* fLbl = CCLabelBMFont::create("0", "bigFont.fnt");
        fLbl->setScale(0.55f);
        fLbl->setColor(kLabelColor);
        fLbl->setAnchorPoint({0.5f, 0.5f});
        auto* fItem = CCMenuItemSpriteExtra::create(fLbl, fLbl, nullptr, nullptr);
        fItem->setEnabled(false);
        fItem->setPosition({ -60.f, y });
        menu->addChild(fItem);

        // Minus / Plus / Reset buttons
        auto* mBtn = makeBtn("-", 0.7f, this, menu_selector(BufferPage::onMinus));
        mBtn->setPosition({ -10.f, y });
        mBtn->setTag(i);
        menu->addChild(mBtn);

        auto* pBtn = makeBtn("+", 0.7f, this, menu_selector(BufferPage::onPlus));
        pBtn->setPosition({ 30.f, y });
        pBtn->setTag(i);
        menu->addChild(pBtn);

        auto* rBtn = makeBtn("Reset", 0.45f, this, menu_selector(BufferPage::onResetOne));
        rBtn->setPosition({ 90.f, y });
        rBtn->setTag(i);
        menu->addChild(rBtn);

        Row row{
            .gamemode = gm,
            .enabledToggle = tog,
            .framesLabel   = fLbl
        };
        m_rows.push_back(row);
        updateRowLabel(m_rows.back());
    }

    // Reset-all row
    auto* allBtn = makeBtn("Reset all to defaults", 0.55f, this,
                           menu_selector(BufferPage::onResetAll));
    allBtn->setPosition({ 0.f, kRowYTop - 8 * kRowH - 12.f });
    menu->addChild(allBtn);
}

void BufferPage::refresh() {
    for (auto& r : m_rows) {
        const auto& p = Settings::get().profile(r.gamemode);
        r.enabledToggle->toggle(p.enabled);
        updateRowLabel(r);
    }
}

void BufferPage::updateRowLabel(Row& r) {
    const auto& p = Settings::get().profile(r.gamemode);
    if (r.framesLabel) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%df", p.frames);
        r.framesLabel->setString(buf);
    }
}

void BufferPage::onEnabledToggle(CCObject* sender) {
    auto* t = static_cast<CCMenuItemToggler*>(sender);
    int i = t->getTag();
    auto& s = Settings::get();
    auto p = s.profile(static_cast<Gamemode>(i));
    p.enabled = t->isToggled();
    s.saveBufferProfile(p);
    // BufferManager picks up the new profile on the next gamemode change
    // or level restart; this is fine because the toggle only affects
    // inputs queued after that point.
}

void BufferPage::onMinus(CCObject* sender) {
    int i = sender->getTag();
    auto& s = Settings::get();
    auto p = s.profile(static_cast<Gamemode>(i));
    p.frames = clamp(p.frames - 1, 0, 30);
    s.saveBufferProfile(p);
    updateRowLabel(m_rows[i]);
}

void BufferPage::onPlus(CCObject* sender) {
    int i = sender->getTag();
    auto& s = Settings::get();
    auto p = s.profile(static_cast<Gamemode>(i));
    p.frames = clamp(p.frames + 1, 0, 30);
    s.saveBufferProfile(p);
    updateRowLabel(m_rows[i]);
}

void BufferPage::onResetOne(CCObject* sender) {
    int i = sender->getTag();
    Settings::get().resetProfile(static_cast<Gamemode>(i));
    updateRowLabel(m_rows[i]);
    // Reflect new toggle state
    m_rows[i].enabledToggle->toggle(Settings::get().profile(static_cast<Gamemode>(i)).enabled);
}

void BufferPage::onResetAll(CCObject*) {
    Settings::get().resetAllProfiles();
    refresh();
}

} // namespace spoon
