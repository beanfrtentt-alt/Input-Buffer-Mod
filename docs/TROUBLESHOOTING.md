# Troubleshooting

A categorized list of the things most likely to go wrong, with the
exact fix for each.

## Build workflow failures

### "Build the mod" step fails with `Unable to find Geode SDK`

This happens when the geode-sdk/build-geode-mod action can't locate
the SDK. Possible causes:

1. **The pinned SDK tag doesn't exist** — e.g., you bumped `sdk:` to
   a typo'd version. Open `.github/workflows/build.yml` and verify the
   version exists at https://github.com/geode-sdk/geode/releases.
2. **The action can't reach GitHub** — usually a transient Actions
   runner issue. Re-run the failed job; if it persists for >30 min,
   check https://www.githubstatus.com/ for outages.

### Compilation errors during the build step

The build log will show the exact C++ error. The most common ones for
Geode v5 / GD 2.2081 are:

#### `'class KeyboardEventDispatcher' has no member named 'dispatchKeyboardPressed'`

The Geode v5 binding renamed or restructured the keyboard dispatcher.
Open `src/Hooks/KeyboardDispatchHook.cpp` and check the actual method
names in the Geode binding header
(`$GEODE_SDK/loader/include/Geode/binding/KeyboardEventDispatcher.hpp`).
The method names in the v5 binding are stable but they were renamed
between Geode v4 and v5.

#### `'m_isOnGroundTop' was not declared in 'PlayerObject'`

The Geode v5 binding may have renamed this member. Search the
`PlayerObject.hpp` binding header for the actual member name (likely
`m_onGroundTop` or `m_isUpsideDown` depending on the exact binding
revision). Edit `src/AssistedGameplay/StateAnalyzer.cpp` accordingly.

#### `'m_playerSpeed' was not declared in 'PlayLayerGameState'`

Same issue — the PlayLayerGameState binding may name this
differently. Common alternatives: `m_playerSpeedMultiplier`,
`m_speedMultiplier`. Search the binding header.

#### `Mod::hook` not found

If you removed the `$modify` refactor and kept the raw hook API,
you'll need to use the v5 low-level hook function instead. The
recommended fix is to refactor to `$modify` (see the four files in
`src/Hooks/` — they're already done). If you really need raw hooks,
consult the Geode v5 source for the new signature.

### Workflow succeeds but the .geode is tiny (<1 KB)

This means the build step didn't produce a real package. Check the
build log:

1. Look for "package" or "geode package" in the log — the
   build-geode-mod action runs the Geode CLI's packaging step
   automatically.
2. If the package step is missing or fails silently, the build
   itself may have succeeded but produced no mod binary. Re-run with
   `build-config: RelWithDebInfo` for more verbose output.

## In-game issues

### F6 doesn't open the menu

1. Verify the mod loaded: open the Geode log viewer
   (Window → Geode → Logs, or F12) and search for `[SpoonClient]`.
   If the startup line isn't there, the mod failed to load.
2. If the startup line IS there but F6 doesn't open anything, the
   keyboard hook didn't fire. This is the most common v5 binding
   mismatch — see the troubleshooting item about
   `KeyboardEventDispatcher` above.
3. As a fallback, try remapping the menu key in mod.json's `menuKey`
   setting (e.g., to `F9`) and reloading GD.

### Menu opens but inputs leak into gameplay

The menu's touch+keyboard intercept should prevent this. If you see
inputs firing while clicking around the menu:

1. Check that `setTouchEnabled(true)` and `setKeyboardEnabled(true)`
   are set in `SpoonMenuLayer::init()`.
2. Check that `ccTouchBegan` returns `true` (which prevents the
   event from propagating to other layers).
3. If only some inputs leak (e.g., F-keys work but Space doesn't),
   the menu's `keyDown` may be missing the relevant case — extend
   the dispatch in `SpoonMenuLayer::keyDown`.

### Per-gamemode buffer settings don't persist

1. Verify Geode's settings UI shows the values you set (Window →
   Geode → Mods → Spoon Client → Settings).
2. If the values reset on GD restart, check `Settings::migrate()`
   in `SpoonSettings.cpp`. The migration logic may be bumping
   `configVersion` to 2 on every load if the saved value is somehow
   invalid.
3. If the values show in Geode's settings UI but the runtime behavior
   doesn't match, the `BufferManager` may have a stale cached
   profile — restart the level to force a profile reload.

### Assisted Gameplay doesn't make the cube jump

1. Open the F6 menu → Assisted tab → confirm AG is enabled (toggle
   shows ON, status reads "Active" or "Ready").
2. Start a level. The on-screen overlay should appear in the
   top-left with status "Active" and a planner reason like "Floor
   hazard ahead — jump".
3. If the overlay doesn't appear, the `agShowOverlay` setting may be
   off. Toggle it in the AG tab.
4. If the overlay shows "Active" but no inputs fire, the
   `BufferManager` may have a stale pending input — press F8
   (emergency stop) and toggle AG off and back on.

### AG crashes GD

1. Disable AG immediately via the F8 emergency-stop key.
2. Open the Geode log viewer and look for the last log line before
   the crash. If it mentions `[SpoonClient]`, note the file:line.
3. The most common crash cause is a stale `PlayLayer*` pointer
   captured by the AG pipeline after the level exits. If the log
   shows a crash in `Manager::runPipeline` shortly after a level
   exit, the `BaseGameLayerHook` destructor isn't firing — see the
   "I removed the dtor hook" comment in
   `src/Hooks/BaseGameLayerHook.cpp` for why and how to add it back.

## Targeting a different GD version

### I want to target 2.2091 instead of 2.2081

As of October 2026, Geode v5.11.2 has partial crash-fix support for
2.2091 but is "still primarily for 2.2081" (per the release notes).
You can try 2.2091 by:

1. Editing `mod.json`:
   ```json
   "gd": { "win": "2.2091", ... }
   ```
2. Re-running the workflow.
3. If it crashes at runtime, switch back to 2.2081.

### I want to target an older GD version (e.g., 2.2074)

You'd need Geode v4 (which targets GD 2.2074 / 2.2). However, this
mod was migrated to v5 patterns (`$modify` macro, C++23, v5
`setup_geode_mod` CMake). To target v4:

1. Revert CMakeLists.txt to v4 (`geode_mod(...)` function, C++20,
   `GEODE_SDK_PATH` env var).
2. Revert `mod.json` to v4 (`"geode": "4.0.0"`, `gd` block with
   2.2074, dependencies as array-of-objects).
3. Convert `$modify` blocks back to raw `Mod::get()->hook(...)` calls
   (or keep them — v4 also supports `$modify`).

This is a substantial downgrade; consider opening an issue in the
Spoon Client repo if you actually need v4 support.

### I really did mean "2.801" — does that exist?

No. As of October 2026, the Geometry Dash versions that have shipped
are: 2.0, 2.01, 2.1, 2.11, 2.2, 2.2074, 2.2081, 2.2081, 2.2091.
There is no 2.801. If you saw this number somewhere, it was almost
certainly a typo of 2.2081 (missing the leading "2") — and that's
what this mod targets.

## When in doubt

1. Check the [Geode FAQ](https://geode-sdk.org/faq/).
2. Search the [Geode Discord](https://discord.gg/geode) for the
   error message.
3. Open an issue in your `spoon-client` GitHub repo with the full
   build log attached.
