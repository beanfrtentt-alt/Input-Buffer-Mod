# Test Procedures

This document describes the test plan used to verify Spoon Client
behaves as specified. Two categories of tests are listed:

* **Static tests** — can be run from a build script without launching
  Geometry Dash.
* **In-game tests** — must be executed manually by a human player with
  Geometry Dash + the mod installed.

## Static tests

### T1. Source-tree audit

```bash
python3 scripts/audit_sources.py
```

Asserts: no `gd::` namespace forward-declarations, no calls to
non-existent symbols (`get_decide`, `GEODE_MOD_IMPL`, etc.), no use of
STL types without their required `#include`s.

**Current status:** PASS — 39 files scanned, 0 issues.

### T2. Project structure

```bash
ls -la download/spoon-client/
ls -la download/spoon-client/src/{UI,AssistedGameplay,Hooks}
```

Asserts: every directory listed in `CMakeLists.txt` exists, every
source file referenced in `SOURCES` is present on disk.

### T3. mod.json schema

```bash
python3 -c "import json; json.load(open('download/spoon-client/mod.json'))"
```

Asserts: `mod.json` parses as valid JSON. The Geode CLI does the same
check at build time.

## In-game tests

Each test below is a numbered procedure. Tests are grouped by primary
feature (F6 menu, Buffer, AG, Integration).

### F6 menu

#### T10. F6 opens the menu

1. Launch Geometry Dash.
2. Be on the main menu screen.
3. Press F6.

**Pass:** A dark panel appears with the title "Spoon Client" at the
top, three tabs ("Modules", "Buffer", "Assisted"), and a close (X)
button in the top-right.

#### T11. F6 closes the menu

1. With the menu open, press F6 again.

**Pass:** The menu disappears immediately.

#### T12. Escape closes the menu

1. Open the menu with F6.
2. Press Escape.

**Pass:** The menu disappears.

#### T13. Tab switching

1. Open the menu.
2. Click the "Buffer" tab.
3. Click the "Assisted" tab.
4. Click the "Modules" tab.

**Pass:** Each click visibly switches the page content. Scroll
positions of pages are preserved when you return to a tab you visited
earlier.

#### T14. Menu inputs don't leak into gameplay

1. Start a level (Practice Mode for safety).
2. Press F6 to open the menu mid-gameplay.
3. Click around the menu (especially clicking toggles near the bottom
   of the Buffer page).
4. Close the menu with F6.

**Pass:** No jumps / no inputs are triggered in the level during the
menu interaction.

#### T15. Settings persist across GD restart

1. Open the menu, go to Buffer.
2. Set Cube buffer to 18 frames. Toggle Cube buffer disabled.
3. Close the menu and quit Geometry Dash.
4. Relaunch GD, open the menu, go to Buffer.

**Pass:** Cube buffer shows 18 frames and the toggle is in the disabled
state — same as you left it.

### Buffer

#### T20. Per-gamemode independence

1. Set Cube buffer to 25 frames.
2. Set Ship buffer to 4 frames.
3. Quit + relaunch GD.
4. Open the menu, go to Buffer.

**Pass:** Cube shows 25, Ship shows 4. The two values did not
overwrite each other.

#### T21. Active gamemode indicator

1. Start a level that starts as Cube.
2. Open the menu, observe the footer.

**Pass:** The footer reads `Active gamemode: Cube | Buffer: <set
value> frames | AG: OFF`.

3. Close the menu, play until a Ship portal, open the menu again.

**Pass:** The footer now reads `Active gamemode: Ship | Buffer: <set
value> frames`.

#### T22. Reset one gamemode

1. In the Buffer page, set Cube to 25 frames.
2. Click "Reset" on the Cube row.

**Pass:** Cube frames goes back to 8 (the default).

#### T23. Reset all

1. Modify several gamemodes' buffer values.
2. Click "Reset all to defaults".

**Pass:** Every gamemode's frame count is back to its default value.

#### T24. Range clamping

1. Open the Buffer page.
2. Tap "+" on Cube until the value stops increasing.

**Pass:** The value caps at 30 (the max from mod.json).

3. Tap "-" until it stops decreasing.

**Pass:** The value caps at 0.

### Assisted Gameplay

#### T30. Enable / disable toggle

1. Open the Assisted tab.
2. Toggle "Assisted Gameplay" on.

**Pass:** The toggle shows ON, the Status label updates to "Ready",
and (if Show Overlay is on) the on-screen AG overlay appears.

3. Toggle "Assisted Gameplay" off.

**Pass:** Toggle shows OFF, status reads "Idle", overlay disappears.

#### T31. AG attempts a level

1. Enable AG.
2. Start Stereo Madness (or any easy main-level).
3. Watch the AG overlay.

**Pass:** Status transitions from `Ready` → `Active`. AG presses /
holds keys to attempt to clear obstacles. Even if AG dies, the system
recovers: status transitions to `Recovering`, then `Active` after the
respawn.

#### T32. Emergency stop (F8)

1. Enable AG, start a level.
2. While AG is running, press F8.

**Pass:** AG immediately stops issuing inputs. Status reads "Paused".
Inputs in flight are cancelled (the cube stops jumping immediately,
no extra jump fires).

#### T33. Disable restores normal control

1. Enable AG, start a level.
2. While AG is running, toggle AG off in the F6 menu.

**Pass:** AG stops. The human player can immediately jump / hold
without any stuck input.

#### T34. Buffer affects AG timing

1. Set Cube buffer to 0 frames. Start a level. Enable AG.
2. Observe AG's inputs: jumps should fire roughly on the planner tick.

3. Set Cube buffer to 30 frames (huge). Restart the level.

**Pass:** AG's inputs appear to "linger" — a press issued one tick
keeps attempting to fire for up to 30 frames.

#### T35. Lookahead slider

1. Open the Assisted page.
2. Set Lookahead to 4 (minimum).
3. Start a level with AG enabled.

**Pass:** AG looks ahead very few frames; can be jittery on tight
sections.

4. Set Lookahead to 90 (maximum).

**Pass:** AG reacts earlier to upcoming hazards.

### Integration

#### T40. Death does not leave stale inputs

1. Enable AG, start a level where AG will eventually die.
2. Watch the AG overlay through the death.

**Pass:** On death, the overlay status becomes `Recovering`. After
the respawn (~60 frames), the status returns to `Active` and AG
resumes planning — no stale input from before the death fires.

#### T41. Restart (R key) resets AG state

1. Enable AG, start a level.
2. Let AG run for a few seconds.
3. Press R to restart the level.

**Pass:** AG status becomes `Ready`, then `Active`. No leftover hold
input from before the restart.

#### T42. Level exit cleans up

1. Enable AG, start a level.
2. Quit the level (Esc).

**Pass:** AG status becomes `Idle`. The overlay disappears.

#### T43. Dual mode — independent buffers

1. Open the menu, ensure Cube = 18 frames and Ship = 5 frames.
2. Start a dual-mode level (e.g., a custom level with a dual portal
   at the start where P1 is Cube and P2 is Ship).
3. Play with AG disabled; observe the footer.

**Pass:** Footer shows "Active gamemode: Cube" (since it always
reports P1's gamemode). The runtime buffer for P2 (Ship) is
independent — this is verified by code review, not visually.

## Known testing gaps

The following tests cannot be executed automatically on this dev
container because there is no MinGW toolchain available. They are
expected to pass on a Windows build host:

* **T50.** Project compiles cleanly with `cmake --build build`.
* **T51.** The produced `.geode` package is loadable by the Geode
  loader (i.e., no symbol resolution errors at load time).
* **T52.** All five hooks register successfully (verifiable via the
  Geode log viewer — should see "Registered hook: PlayLayer::init"
  etc.).
