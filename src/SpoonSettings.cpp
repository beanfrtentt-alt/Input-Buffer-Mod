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

static int readIntClamped(const char* key, int def, int lo, int hi) {
    auto mod = Mod::get();
    int v = def;
    if (auto val = mod->getSettingValue<std::int64_t>(key)) v = static_cast<int>(*val);
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

static bool readBool(const char* key, bool def) {
    auto mod = Mod::get();
    if (auto val = mod->getSettingValue<bool>(key)) return *val;
    return def;
}

static std::string readString(const char* key, std::string def) {
    auto mod = Mod::get();
    if (auto val = mod->getSettingValue<std::string>(key)) return *val;
    return def;
}

void Settings::load() {
    // Migrate first so older saved layouts are normalized before reading.
    migrate();

    for (int i = 0; i < 8; ++i) {
        m_profiles[i].gamemode = static_cast<Gamemode>(i);
        m_profiles[i].enabled  = readBool(kBufEnabledKey[i], kDefaultEnabled[i]);
        m_profiles[i].frames   = readIntClamped(kBufFramesKey[i],
                                                kDefaultFrames[i], 0, 30);
    }

    m_ag.enabled         = readBool("agEnabled", false);
    m_ag.strategy        = readString("agStrategy", "receding-horizon");
    m_ag.lookaheadFrames = readIntClamped("agLookaheadFrames", 18, 4, 90);
    m_ag.showOverlay     = readBool("agShowOverlay", true);
    m_ag.showDebug       = readBool("agShowDebug", false);

    m_kb.menu          = readString("menuKey", "F6");
    m_kb.emergencyStop = readString("agEmergencyStopKey", "F8");
}

int Settings::migrate() {
    auto mod = Mod::get();
    int v = 1;
    if (auto cv = mod->getSettingValue<std::int64_t>("configVersion")) {
        v = static_cast<int>(*cv);
    }

    // v1 -> v2: the old `agLookahead` was a millisecond value; new layout uses
    // frames and a different key. We honor the old value (rounded to frames)
    // so user customization is preserved.
    if (v < 2) {
        // Read the legacy field (may be absent; getSettingValue returns
        // std::nullopt in that case).
        if (auto old = mod->getSettingValue<std::int64_t>("agLookahead")) {
            int frames = static_cast<int>((*old + 16) / 16); // ~16ms per frame
            frames = clamp(frames, 4, 90);
            (void)mod->setSettingValue<std::int64_t>("agLookaheadFrames",
                                                     std::int64_t(frames));
        }
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
        // Caller bug — return Cube as a safe default rather than crashing.
        return m_profiles[0];
    }
    return m_profiles[static_cast<int>(g)];
}

void Settings::saveBufferProfile(const BufferProfile& p) {
    if (p.gamemode == Gamemode::Unknown) return;
    int i = static_cast<int>(p.gamemode);
    auto mod = Mod::get();
    (void)mod->setSettingValue<bool>(kBufEnabledKey[i], p.enabled);
    (void)mod->setSettingValue<std::int64_t>(kBufFramesKey[i],
        std::int64_t(clamp(p.frames, 0, 30)));
    m_profiles[i] = p;
}

void Settings::saveAgSettings() {
    auto mod = Mod::get();
    (void)mod->setSettingValue<bool>("agEnabled", m_ag.enabled);
    (void)mod->setSettingValue<std::string>("agStrategy", m_ag.strategy);
    (void)mod->setSettingValue<std::int64_t>("agLookaheadFrames",
        std::int64_t(clamp(m_ag.lookaheadFrames, 4, 90)));
    (void)mod->setSettingValue<bool>("agShowOverlay", m_ag.showOverlay);
    (void)mod->setSettingValue<bool>("agShowDebug", m_ag.showDebug);
}

void Settings::saveKeybinds() {
    auto mod = Mod::get();
    (void)mod->setSettingValue<std::string>("menuKey", m_kb.menu);
    (void)mod->setSettingValue<std::string>("agEmergencyStopKey", m_kb.emergencyStop);
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
    return std::string(gamemodeName(Gamemode::Cube)); // updated at runtime by BufferManager
}

} // namespace spoon
