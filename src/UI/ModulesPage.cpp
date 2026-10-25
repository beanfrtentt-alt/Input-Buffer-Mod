/**
 * SpoonClient / UI/ModulesPage.cpp
 *
 * Hard-coded list of modules. Each module's row is a CCMenuItemSpriteExt
 * with an always-disabled toggle that displays the current state and a
 * description label next to it. The page itself is non-interactive; to
 * change a module's enabled state the user goes to the dedicated page.
 */
#include "ModulesPage.hpp"
#include "../SpoonSettings.hpp"
#include "../BufferManager.hpp"
#include "../AssistedGameplay/Manager.hpp"

using namespace geode;
using namespace geode::prelude;
using namespace cocos2d;

namespace spoon {

ModulesPage* ModulesPage::create() {
    auto* p = new ModulesPage();
    if (p && p->init()) { p->autorelease(); return p; }
    CC_SAFE_DELETE(p);
    return nullptr;
}

bool ModulesPage::init() {
    if (!CCLayer::init()) return false;
    build();
    return true;
}

static void addRow(CCMenu* menu, const char* name, const char* desc,
                   bool enabled, float y) {
    auto* n = CCLabelBMFont::create(name, "bigFont.fnt");
    n->setScale(0.6f);
    n->setColor({255, 220, 90});
    n->setAnchorPoint({0.f, 0.5f});
    auto* nItem = CCMenuItemSpriteExt::create(n, n, nullptr, nullptr);
    nItem->setEnabled(false);
    nItem->setPosition({ -330.f, y });
    menu->addChild(nItem);

    auto* d = CCLabelBMFont::create(desc, "bigFont.fnt");
    d->setScale(0.45f);
    d->setColor({200, 200, 200});
    d->setAnchorPoint({0.f, 0.5f});
    auto* dItem = CCMenuItemSpriteExt::create(d, d, nullptr, nullptr);
    dItem->setEnabled(false);
    dItem->setPosition({ -330.f, y - 18.f });
    menu->addChild(dItem);

    auto* tLbl = CCLabelBMFont::create(enabled ? "ON" : "OFF", "bigFont.fnt");
    tLbl->setScale(0.5f);
    tLbl->setColor(enabled ? ccColor3B{120, 220, 120} : ccColor3B{200, 90, 90});
    auto* tItem = CCMenuItemSpriteExt::create(tLbl, tLbl, nullptr, nullptr);
    tItem->setEnabled(false);
    tItem->setPosition({ 120.f, y });
    menu->addChild(tItem);
}

void ModulesPage::build() {
    auto* menu = CCMenu::create();
    menu->setPosition({0, 0});
    addChild(menu, 1);

    constexpr float kTopY = 220.f;
    constexpr float kRowH = 50.f;

    auto& s = Settings::get();
    bool bufferEnabled = s.profile(Gamemode::Cube).enabled;
    addRow(menu, "Input Buffering", "Per-gamemode buffered input release. Tune in the Buffer tab.", bufferEnabled, kTopY);
    addRow(menu, "Assisted Gameplay", "Receding-horizon autonomous input planner. Toggle in the Assisted tab.", s.ag().enabled, kTopY - kRowH);
    addRow(menu, "F6 Menu", "In-game Spoon Client settings panel. Toggle with F6 (configurable in mod.json settings).", true, kTopY - 2 * kRowH);
    addRow(menu, "AG Overlay", "On-screen AG status indicator.", s.ag().showOverlay, kTopY - 3 * kRowH);
    addRow(menu, "Emergency Stop", "Instantly clears all AG + buffer state. Bound to F8.", true, kTopY - 4 * kRowH);
}

} // namespace spoon
