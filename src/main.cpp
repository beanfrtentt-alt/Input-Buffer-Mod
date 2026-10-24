#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/utils/Keyboard.hpp>

using namespace geode::prelude;

namespace {
    // Shared state. Only touched from the main (cocos) thread.
    constexpr int kModes[] = {0, 2, 4, 8, 16};   // 0 == OFF
    int g_modeIndex = 0;

    struct PendingPress {
        bool active = false;
        int  ticksLeft = 0;
        int  button = 1;
        bool isPlayer1 = true;
        bool userStillHolding = false;
    };
    PendingPress g_pending;

    int g_bufferedCount = 0;   // presses replayed through the buffer
    int g_normalCount = 0;     // presses processed normally
    int g_expiredCount = 0;    // buffered presses that timed out
    bool g_replaying = false;  // re-entrancy guard while we call the original

    int bufferTicks() { return kModes[g_modeIndex]; }

    void resetCounters() {
        g_pending = {};
        g_bufferedCount = g_normalCount = g_expiredCount = 0;
    }
}

class $modify(IBPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* label = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        resetCounters();

        // Overlay lives on the UI layer so it is not affected by camera movement.
        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(0.5f);
        label->setOpacity(190);
        label->setZOrder(9999);
        auto winSize = CCDirector::get()->getWinSize();
        label->setPosition({6.f, winSize.height - 6.f});
        m_uiLayer->addChild(label);
        m_fields->label = label;
        this->refreshOverlay();

        // F6 cycles the mode. Action::Press fires once per physical press;
        // Action::Repeat (key held) is ignored, so holding F6 cannot cycle repeatedly.
        this->addEventListener(KeyboardInputEvent(), [this](KeyboardInputData& data) {
            if (data.key == KEY_F6 && data.action == KeyboardInputData::Action::Press) {
                g_modeIndex = (g_modeIndex + 1) % std::size(kModes);
                g_pending = {};
                this->refreshOverlay();
            }
        });
        return true;
    }

    void refreshOverlay() {
        if (!m_fields->label) return;
        auto ticks = bufferTicks();
        std::string stage = "idle";
        if (ticks > 0) stage = g_pending.active ? "armed" : "ready";
        auto text = fmt::format(
            "IBD: {}\nstage: {}\nbuffer: {}\ninputs: {} normal / {} buffered / {} expired",
            ticks == 0 ? "OFF" : "ON", stage,
            ticks == 0 ? "-" : fmt::format("{} ticks", ticks),
            g_normalCount, g_bufferedCount, g_expiredCount
        );
        m_fields->label->setString(text.c_str());
    }

    void resetLevel() {
        g_pending = {};
        PlayLayer::resetLevel();
        this->refreshOverlay();
    }
};

class $modify(IBGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        // OFF, or our own replay: never touch the input.
        if (bufferTicks() == 0 || g_replaying) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto player = isPlayer1 ? m_player1 : m_player2;
        bool canActNow = player && (player->m_isOnGround || player->m_isDead);

        if (down) {
            if (canActNow || !player) {
                g_normalCount++;
                GJBaseGameLayer::handleButton(down, button, isPlayer1);
            }
            else {
                // Early press (player airborne): hold it for up to N ticks.
                g_pending = {true, bufferTicks(), button, isPlayer1, true};
                // Still forward it so vanilla behavior (orbs, pads, hold-jump) is unchanged.
                GJBaseGameLayer::handleButton(down, button, isPlayer1);
            }
        }
        else {
            if (g_pending.active && g_pending.button == button && g_pending.isPlayer1 == isPlayer1) {
                g_pending.userStillHolding = false;
            }
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (bufferTicks() == 0 || !g_pending.active || isHalfTick) return;

        auto player = g_pending.isPlayer1 ? m_player1 : m_player2;
        if (!player) { g_pending = {}; return; }

        if (player->m_isOnGround && !player->m_isDead) {
            // Landed inside the window: replay the early press once.
            // If the user already let go, press+release so the input is a clean tap.
            g_replaying = true;
            GJBaseGameLayer::handleButton(true, g_pending.button, g_pending.isPlayer1);
            if (!g_pending.userStillHolding) {
                GJBaseGameLayer::handleButton(false, g_pending.button, g_pending.isPlayer1);
            }
            g_replaying = false;
            g_bufferedCount++;
            g_pending = {};
        }
        else if (--g_pending.ticksLeft <= 0) {
            g_expiredCount++;
            g_pending = {};
        }

        if (auto pl = PlayLayer::get()) {
            static_cast<IBPlayLayer*>(pl)->refreshOverlay();
        }
    }
};
