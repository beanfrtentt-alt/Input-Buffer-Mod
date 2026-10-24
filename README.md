# Input Buffer Demo (Geode mod)

**Status: source only. This project has NOT been compiled as a Geode mod or tested in-game.**
No `.geode` file exists yet. The build sandbox could not reach the Windows SDK/CRT downloads Geode needs.
Only `src/predictor.hpp` (pure math) was compiled and unit-tested on its own (`tests/predictor_test.cpp`).

## Get the `.geode` file (easiest: GitHub Actions)
1. Create a GitHub repository and push this folder to its `main` branch.
2. Open the **Actions** tab. "Build Geode Mod" runs on push (or run it manually).
3. Download the **Build Output** artifact: `ilyaskjidaa.input-buffer-demo.geode`.

## Or build locally (Windows)
Needs Visual Studio 2022 (C++ desktop workload), CMake and the Geode CLI.
```
geode sdk install
geode sdk install-binaries
set GEODE_SDK=<path to geode sdk>
geode build
```

## Install
Copy the `.geode` file into `Geometry Dash/geode/mods/` (or drag it onto the game window) and restart.

## Controls
| Key | Action |
|---|---|
| **F6** | Cycle the input buffer: OFF, 2, 4, 8, 16, 32, 64 ticks, OFF |
| **F8** | Toggle the Auto-Correction click assistant (OFF by default and at the start of every level) |

Both keys act only on the initial press; holding a key does not repeat. F6 and F8 are independent and both states are always shown in the overlay.

## Settings (Geode mod settings menu)
Show predicted trajectory, prediction horizon (10-240 ticks), visualization detail (1-3), debug mode, auto-click scan range (10-100 ticks).
The project has no separate "dashboard" window or "profiles" feature; the in-game overlay is the dashboard and the Geode settings screen is the configuration menu.

## Overlay (top-left, in a level)
`AUTO-CORRECTION: ON` / `OFF` (plus a red banner while ON), its live status, auto-input counters (issued / confirmed by game / unconfirmed), buffer state, prediction summary (mode, confidence, ticks to hazard, landing, next action) and statistics (normal / buffered / AUTO / expired inputs, prediction accuracy, deaths warned vs unwarned). Buffered clicks flash the screen red; automated clicks flash orange.

## Console window (Windows)
The first F6 or F8 press opens a separate console. It logs mode changes, status lines and input events. Its close button is disabled (closing it would close the game).
Every automated input and every change of assistant state is also appended to `auto-input-log.txt` in the mod's save directory, separate from player inputs.

## How it works
**Buffer (F6):** with the buffer ON, a press made while airborne is remembered for N ticks and replayed once on landing. OFF passes every input through unchanged.

**Trajectory prediction:** measures the player's per-tick movement (and its change) each tick and extrapolates it, drawn as a *dashed cyan* line. Hazards and solids come from the game's visible object sections (hazard hitboxes shrunk by a 0.6 heuristic). Red X = predicted collision, yellow dot = predicted landing. It is a prediction, never a confirmed event.

**Auto-Correction (F8) = automatic click assistant, cube only.** Every game tick, from live state (not a timer):
1. It learns your own jump arc from jumps that were actually clicked (so you must do one normal jump first).
2. It predicts the straight path; if a *hazard* (spike/saw) is on it, it simulates a jump started now, or N ticks later, and finds the window in which a jump clears the hazard and lands past it.
3. Classification, shown in the overlay:
   - **CLICK EXPECTED**: the window is still open or not open yet. You may click yourself. Nothing is sent.
   - **PLAYER CLICK REGISTERED**: you clicked (or are holding) recently. Nothing is sent, so no duplicate.
   - **WOULD BE MISSED**: this is the last tick where a jump is predicted to clear, and you have not clicked, so exactly one jump press is sent. A cooldown blocks any further auto input until the player has left the ground and landed again.
   - **WINDOW MISSED**: no jump from here is predicted to clear; nothing is sent.
   - **UNAVAILABLE: reason**: required information is missing (no jump profile yet, wrong mode, airborne, blocked by a solid block...). It says so instead of acting.
4. **Honest feedback:** an auto click is only reported as *CONFIRMED* if the game then shows the player taking off within 6 ticks. Otherwise it is logged as *NOT CONFIRMED*. "Issued" only means the mod called the game's input handler.
5. Turning F8 OFF releases the button it pressed and stops immediately. Dying, restarting, finishing the level, quitting or changing movement mode clears all pending state.
6. Your own clicks are never altered or delayed by this feature.

**Debug mode:** also draws the actual trail (green) and logs predicted-vs-actual discrepancies and whether a prediction had warned before each death.

## Supported versions
Geode SDK v5.11.2, Geometry Dash 2.2081 (Windows/Android/Mac declared in `mod.json`; only Windows is built by the workflow).

## Known inaccuracies and limitations
- **Nothing here has been tested in-game.** Do not assume the click assistant works until you have tried it. Only the decision logic (`tests/predictor_test.cpp`) and a mock-API syntax check (`tests/check.sh`) have been run; neither proves the real Geode build or in-game behavior.
- Prediction is extrapolation, not the game's physics. It ignores triggers, portals, pads/orbs, slopes, moving/rotating objects, speed changes, gravity flips mid-horizon and platformer mode.
- Reasonably modelled: cube (medium confidence), ball/wave (low). Ship, UFO, robot, spider and swing use the same straight/parabolic guess (very low confidence) and the click assistant ignores them.
- Only objects in the currently visible sections are considered, so very long horizons can run past the scanned area. Hazard hitboxes are heuristic.
- The click assistant handles hazards (spikes/saws) on a flat floor at constant speed. It does not climb blocks, use orbs/pads/portals, handle gravity flips mid-jump, or act on player 2. It needs one clean clicked jump at the current speed before it has a profile.
- It can only act at the last safe tick; if prediction is off by a tick or two (likely, given the heuristics), it may still die. It cannot rescue a jump that is already too late.
- Whether a press passed to the game's input handler is processed exactly like a physical click (and the exact release timing) is unverified; the CONFIRMED/NOT CONFIRMED counters exist to surface this.
- "Prediction correct" in the statistics means the position 10 ticks later was within 10 units of the prediction; it is not a measure of hazard-prediction accuracy.
- The original buffering specification was not supplied, so the buffer logic is my interpretation.
- Automatic clicks are assisted play. Leaderboards and record verification may treat them as cheating; use only where that is acceptable. The separate log exists so assisted runs stay identifiable.

## Self-checks (no Geode SDK needed)
`sh tests/check.sh` compiles and runs the decision-logic test and a syntax check of `src/main.cpp` against a minimal mock of the Geode API. The mock only catches typos/scoping in this project's own code, not mismatches with the real SDK.
