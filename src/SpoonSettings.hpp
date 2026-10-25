/**
 * SpoonClient / SpoonSettings.hpp
 *
 * Central, validated configuration store. Acts as the single source of truth
 * for every runtime subsystem: BufferManager, AssistedGameplay, the F6 menu,
 * and the keyboard hooks.
 *
 * Layout
 * ------
 *  * Each gamemode has its own BufferProfile (enabled + frame count).
 *  * Global ag-* settings cover Assisted Gameplay preferences.
 *  * Keybinds live in here too (menu key + emergency stop key).
 *  * A `configVersion` field powers migration from older layouts.
 *
 * Storage
 * -------
 * The Mod's saved values are read via `Mod::get()->getSettingValue(...)`.
 * We mirror them into an in-memory struct for fast, branchless access
 * during gameplay; writing goes back through `setSettingValue` so the
 * values persist across GD restarts.
 */
#pragma once
#include <Geode/Geode.hpp>
#include "Utils.hpp"

namespace spoon {

struct BufferProfile {
    Gamemode gamemode;
    bool   enabled  = true;
    int    frames   = 8;     // 0..30
};

struct AgSettings {
    bool        enabled          = false;
    std::string strategy        = "receding-horizon"; // or "static-analysis"
    int         lookaheadFrames  = 18;
    bool        showOverlay      = true;
    bool        showDebug        = false;
};

struct Keybinds {
    std::string menu       = "F6";
    std::string emergencyStop = "F8";
};

class Settings {
public:
    // Singleton accessor. Constructed on first use; safe to call from any thread
    // that the GD main loop is on, since Geode's setting API is main-thread-only.
    static Settings& get();

    // Load (or re-load) every value from the Geode saved settings. Called from
    // main.cpp `setGeodeInitialized` and from the F6 menu "Apply" button so the
    // in-memory state stays in sync with the disk values.
    void load();

    // Push a single profile back to disk. Used by the buffer page when a slider
    // or toggle is changed.
    void saveBufferProfile(const BufferProfile& p);

    void saveAgSettings();
    void saveKeybinds();

    // Bump to current configVersion and re-apply defaults if a malformed value
    // was found. Returns the version that was migrated to.
    int  migrate();

    // --- Accessors (return reference to internal state; lifetime tied to
    //     Settings singleton; safe to capture pointers in hooks) ---
    const BufferProfile& profile(Gamemode g) const;
    BufferProfile&       profileMut(Gamemode g);

    const AgSettings&  ag()         const { return m_ag; }
    AgSettings&        agMut()            { return m_ag; }

    const Keybinds&    keybinds()   const { return m_kb; }
    Keybinds&          keybindsMut()      { return m_kb; }

    int configVersion() const { return m_configVersion; }
    void setConfigVersion(int v) { m_configVersion = v; }

    // Per-profile reset to default values.
    void resetProfile(Gamemode g);
    void resetAllProfiles();

    // For display: returns the human-readable label of the active gamemode.
    std::string activeGamemodeLabel() const;

private:
    Settings();
    BufferProfile m_profiles[8];
    AgSettings    m_ag;
    Keybinds       m_kb;
    int            m_configVersion = 0;
};

} // namespace spoon
