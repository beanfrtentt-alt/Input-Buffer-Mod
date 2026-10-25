/**
 * SpoonClient / SpoonSettings.cpp
 *
 * Concrete implementation of the validated configuration store. Maps the
 * flat `mod.json` settings (per-gamemode `buffer{Mode}{Enabled|Frames}` etc.)
 * to typed structs in memory and back.
 *
 * Migration
 * ---------
 *  * v1 -> v2: renamed `agLookahead` (ms) to `agLookaheadFrames` (frames).
 *    Existing user values are kept; the unit is interpreted as frames going
 *    forward and clamped to [4, 90].
 *  * Unknown keys are silently ignored. Missing keys fall back to defaults
 *    defined in `mod.json`.
 */
#include "SpoonSettings.hpp"
#include <Geode/Geode.hpp>
#include <vector>

using namespace geode;
using namespace geode::prelude;

namespace spoon {

static const char* kBufEnabledKey[] = {
    "bufferCubeEnabled",  "bufferShipEnabled",  "bufferBallEnabled",
    "bufferUfoEnabled",   "bufferWaveEnabled",  "bufferRobotEnabled",
    "bufferSpiderEnabled","bufferSwingEnabled"
};
static const char* kBufFramesKey[] = {
    "bufferCubeFrames",   "bufferShipFrames",   "bufferBallFrames",
    "bufferUfoFrames",    "bufferWaveFrames",    "bufferRobotFrames",
    "bufferSpiderFrames", "bufferSwingFrames"
};

static const int kDefaultFrames[] = { 8, 6, 6, 6, 4, 8, 6, 6 };
static const bool kDefaultEnabled[] = { true, true, true, true, true, true, true, true };

Settings& Settings::get() {
    static Settings instance;
    return instance;
}

Settings::Settings() {
    for (int i = 0; i < 8; ++i) {
        m_profiles[i] = BufferProfile{
            .gamemode = static_cast<Gamemode>(i),
            .enabled  = kDefaultEnabled[i],
            .frames   = kDefaultFrames[i]
        };
    }
}

static std::string readString(const char* key, std::string def) {
    auto v = Mod::get()->getSettingValue<std::string>(key);
    return v.empty() ? def : v;
}

static std::string readKeybindLabel(const char* key, const char* fallback) {
    auto binds = Mod::get()->getSettingValue<std::vector<Keybind>>(key);
    if (binds.empty()) return fallback;
    return binds.front().toString();
}

void Settings::load() {
    migrate();

    for (int i = 0; i < 8; ++i) {
        m_profiles[i].gamemode = static_cast<Gamemode>(i);
        m_profiles[i].enabled  = Mod::get()->getSettingValue<bool>(kBufEnabledKey[i]);
        int frames = static_cast<int>(Mod::get()->getSettingValue<std::int64_t>(kBufFramesKey[i]));
        m_profiles[i].frames = clamp(frames, 0, 30);
    }

    m_ag.enabled         = Mod::get()->getSettingValue<bool>("agEnabled");
    m_ag.strategy        = readString("agStrategy", "receding-horizon");
    m_ag.lookaheadFrames = clamp(
        static_cast<int>(Mod::get()->getSettingValue<std::int64_t>("agLookaheadFrames")),
        4, 90);
    m_ag.showOverlay     = Mod::get()->getSettingValue<bool>("agShowOverlay");
    m_ag.showDebug       = Mod::get()->getSettingValue<bool>("agShowDebug");

    m_kb.menu          = readKeybindLabel("menuKey", "F6");
    m_kb.emergencyStop = readKeybindLabel("agEmergencyStopKey", "F8");
}

int Settings::migrate() {
    auto* mod = Mod::get();
    int v = static_cast<int>(mod->getSettingValue<std::int64_t>("configVersion"));
    if (v <= 0) v = 1;

    if (v < 2) {
        int frames = clamp(
            static_cast<int>(mod->getSettingValue<std::int64_t>("agLookaheadFrames")),
            4, 90);
        (void)mod->setSettingValue<std::int64_t>("agLookaheadFrames", std::int64_t(frames));
        v = 2;
        (void)mod->setSettingValue<std::int64_t>("configVersion", std::int64_t(2));
    }

    m_configVersion = v;
    return v;
}

const BufferProfile& Settings::profile(Gamemode g) const {
    static BufferProfile s_unknown{
        .gamemode = Gamemode::Unknown,
        .enabled = false,
        .frames = 0
    };
    if (g == Gamemode::Unknown) return s_unknown;
    return m_profiles[static_cast<int>(g)];
}

BufferProfile& Settings::profileMut(Gamemode g) {
    if (g == Gamemode::Unknown) {
        return m_profiles[0];
    }
    return m_profiles[static_cast<int>(g)];
}

void Settings::saveBufferProfile(const BufferProfile& p) {
    if (p.gamemode == Gamemode::Unknown) return;
    int i = static_cast<int>(p.gamemode);
    auto* mod = Mod::get();
    (void)mod->setSettingValue<bool>(kBufEnabledKey[i], p.enabled);
    (void)mod->setSettingValue<std::int64_t>(kBufFramesKey[i],
        std::int64_t(clamp(p.frames, 0, 30)));
    m_profiles[i] = p;
}

void Settings::saveAgSettings() {
    auto* mod = Mod::get();
    (void)mod->setSettingValue<bool>("agEnabled", m_ag.enabled);
    (void)mod->setSettingValue<std::string>("agStrategy", m_ag.strategy);
    (void)mod->setSettingValue<std::int64_t>("agLookaheadFrames",
        std::int64_t(clamp(m_ag.lookaheadFrames, 4, 90)));
    (void)mod->setSettingValue<bool>("agShowOverlay", m_ag.showOverlay);
    (void)mod->setSettingValue<bool>("agShowDebug", m_ag.showDebug);
}

void Settings::saveKeybinds() {
    // Keybind settings are edited through Geode's own settings UI. Persist
    // the cached labels so the F6 footer stays in sync after a reload.
    load();
}

void Settings::resetProfile(Gamemode g) {
    if (g == Gamemode::Unknown) return;
    int i = static_cast<int>(g);
    BufferProfile p{
        .gamemode = g,
        .enabled  = kDefaultEnabled[i],
        .frames   = kDefaultFrames[i]
    };
    saveBufferProfile(p);
}

void Settings::resetAllProfiles() {
    for (int i = 0; i < 8; ++i)
        resetProfile(static_cast<Gamemode>(i));
}

std::string Settings::activeGamemodeLabel() const {
    return std::string(gamemodeName(Gamemode::Cube));
}

} // namespace spoon
