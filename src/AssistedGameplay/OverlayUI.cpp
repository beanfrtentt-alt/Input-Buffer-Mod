/**
 * SpoonClient / AssistedGameplay/OverlayUI.cpp
 *
 * The overlay is a thin CCLayer that anchors to the top-left of the screen.
 * We use bigFont.fnt for everything — that's a guaranteed-available font
 * shipped with GD and reuses its texture atlas so we don't allocate new
 * textures on every state change.
 */
#include "OverlayUI.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <cstdio>
#include <cstring>

using namespace geode;
using namespace geode::prelude;
using namespace cocos2d;

namespace spoon::ag {

OverlayUI& OverlayUI::get() {
    static OverlayUI instance;
    return instance;
}

OverlayUI::OverlayUI() = default;

static CCLabelBMFont* makeLabel(const char* text, float scale) {
    auto* l = CCLabelBMFont::create(text, "bigFont.fnt");
    l->setScale(scale);
    l->setAnchorPoint({0.f, 0.5f});
    return l;
}

void OverlayUI::build() {
    if (m_layer) return;
    m_layer = CCLayer::create();
    m_layer->setTouchEnabled(false);
    m_layer->setKeyboardEnabled(false);
    m_layer->setAnchorPoint({0.f, 0.f});

    m_titleLbl     = makeLabel("Spoon Client", 0.7f);
    m_statusLbl    = makeLabel("Status: Idle", 0.5f);
    m_gamemodeLbl  = makeLabel("Gamemode: -", 0.5f);
    m_bufferLbl    = makeLabel("Buffer: -", 0.5f);
    m_reasonLbl    = makeLabel("", 0.45f);

    CCSize visible = CCDirector::get()->getWinSize();

    m_titleLbl   ->setPosition({15.f, visible.height - 20.f});
    m_statusLbl  ->setPosition({15.f, visible.height - 38.f});
    m_gamemodeLbl->setPosition({15.f, visible.height - 54.f});
    m_bufferLbl  ->setPosition({15.f, visible.height - 70.f});
    m_reasonLbl  ->setPosition({15.f, visible.height - 86.f});

    ccColor3B dim { 200, 200, 200 };
    m_statusLbl  ->setColor(dim);
    m_gamemodeLbl->setColor(dim);
    m_bufferLbl  ->setColor(dim);
    m_reasonLbl  ->setColor({255, 220, 90});

    m_layer->addChild(m_titleLbl);
    m_layer->addChild(m_statusLbl);
    m_layer->addChild(m_gamemodeLbl);
    m_layer->addChild(m_bufferLbl);
    m_layer->addChild(m_reasonLbl);
}

void OverlayUI::rebuild() {
    if (!m_layer) build();
    if (!m_layer) return;
    if (!m_lastReason.empty()) m_reasonLbl->setString(m_lastReason.c_str());

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Status: %s", statusLabel(m_lastStatus).data());
    m_statusLbl->setString(buf);

    std::snprintf(buf, sizeof(buf), "Gamemode: %s", gamemodeName(m_lastGm).data());
    m_gamemodeLbl->setString(buf);

    std::snprintf(buf, sizeof(buf), "Buffer: %d frames", m_lastBuf);
    m_bufferLbl->setString(buf);
}

void OverlayUI::show() {
    if (m_visible) return;
    build();
    if (!m_layer) return;
    auto* scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    scene->addChild(m_layer, /*z=*/9999);
    m_visible = true;
    rebuild();
}

void OverlayUI::hide() {
    if (!m_visible || !m_layer) return;
    m_layer->removeFromParent();
    m_visible = false;
}

void OverlayUI::update(Status s, std::string_view reason, Gamemode gm, int bufFrames) {
    bool changed = false;
    if (s != m_lastStatus) { m_lastStatus = s; changed = true; }
    if (gm != m_lastGm)    { m_lastGm = gm; changed = true; }
    if (bufFrames != m_lastBuf) { m_lastBuf = bufFrames; changed = true; }
    if (reason != m_lastReason) { m_lastReason = std::string(reason); changed = true; }
    if (changed) rebuild();
}

} // namespace spoon::ag
