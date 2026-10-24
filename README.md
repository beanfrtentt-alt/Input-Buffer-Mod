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
| **F8** | Toggle Auto-Correction (OFF by default and at the start of every level) |

Both keys act only on the initial press; holding a key does not repeat. F6 and F8 are independent and both states are always shown in the overlay.

## Settings (Geode mod settings menu)
Show predicted trajectory, prediction horizon (10-240 ticks), visualization detail (1-3), debug mode, Auto-Correction max delay (1-30 ticks).
Note: the project has no separate "dashboard" window or "profiles" feature; the in-game overlay is the dashboard and the Geode settings screen is the configuration menu.

## Overlay (top-left, in a level)
Buffer state and stage, Auto-Correction state (considered / issued / reason), prediction summary (mode, confidence, ticks to hazard, landing, next action), and statistics: normal / buffered / **AUTO** / expired inputs, prediction accuracy, deaths warned vs unwarned.
A red banner is shown whenever Auto-Correction is ON. Buffered clicks flash the screen red; automated inputs flash orange.

## Console window (Windows)
The first F6 or F8 press opens a separate console. It logs mode changes, status lines and every input event. Its close button is disabled (closing it would close the game).
Every automated input and correction decision is also appended to `auto-input-log.txt` in the mod's save directory.

## How it works
**Buffer (F6):** with the buffer ON, a press made while airborne is remembered for N ticks and replayed once on landing. OFF passes every input through unchanged.

**Trajectory prediction:** measures the player's per-tick movement (and its change) each tick and extrapolates it for the chosen horizon, drawn as a *dashed cyan* line. Hazards and solids come from the game's visible object sections (hazard hitboxes shrunk by a 0.6 heuristic). Red X = predicted collision, yellow dot = predicted landing. "Next action" = ticks until a cube can jump again. It is a prediction, never a confirmed event.

**Auto-Correction (F8), cube only:** learns your own jump arc from jumps you clicked. When you press jump on the ground and the predictor expects that immediate jump to hit a hazard but a jump up to N ticks later to clear it, the press is held back and re-issued once at that delay (duplicate presses in the meantime are suppressed). It can only *delay* inputs, never make them earlier. Turning F8 OFF, dying, restarting, finishing, quitting or changing mode cancels any scheduled input immediately.

**Debug mode:** also draws the actual trail (green) and logs predicted-vs-actual discrepancies and whether a prediction had warned before each death.

## Supported versions
Geode SDK v5.11.2, Geometry Dash 2.2081 (Windows/Android/Mac declared in `mod.json`; only Windows is built by the workflow).

## Known inaccuracies and limitations
- **Nothing here has been tested in-game.** Do not assume Auto-Correction works until you have tried it.
- Prediction is extrapolation, not the game's physics. It ignores triggers, portals, pads/orbs, slopes, moving/rotating objects, speed changes, gravity flips mid-horizon and platformer mode.
- Reasonably modelled: cube (medium confidence), ball/wave (low). Ship, UFO, robot, spider and swing use the same straight/parabolic guess (very low confidence) and Auto-Correction ignores them.
- Only objects in the currently visible sections are considered, so very long horizons can run past the scanned area. Hazard hitboxes are heuristic.
- Auto-Correction needs one clean, clicked jump at the current speed before it has a profile, assumes a flat floor and constant speed, and only acts on player 1.
- "Prediction correct" in the statistics means the position 10 ticks later was within 10 units of the prediction; it is not a measure of hazard-prediction accuracy.
- The original buffering specification was not supplied, so the buffer logic is my interpretation.
- Assisted inputs may be treated as cheating by leaderboards or record verification; use it for demonstration/practice only.
