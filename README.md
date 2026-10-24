# Input Buffer Demo (Geode mod)

**Status: source only. This project has NOT been compiled or tested.** No `.geode` file exists yet.
The build sandbox could not reach the Windows SDK/CRT downloads that Geode needs for a Windows build.

## Get the `.geode` file (easiest: GitHub Actions)
1. Create a new GitHub repository and push this folder to its `main` branch.
2. Open the repo's **Actions** tab. The "Build Geode Mod" workflow runs on push (or run it manually).
3. When it finishes, download the **Build Output** artifact. Inside is `ilyaskjidaa.input-buffer-demo.geode`.

## Or build locally (Windows)
Prerequisites: Visual Studio 2022 (C++ desktop workload), CMake, and the Geode CLI.
```
geode sdk install
geode sdk install-binaries
set GEODE_SDK=<path to geode sdk>
geode build
```
The package is written to `build/ilyaskjidaa.input-buffer-demo.geode`.

## Install
Copy the `.geode` file into `Geometry Dash/geode/mods/`, or drag it onto the game window, then restart.

## Controls
**F6** cycles OFF -> 2 -> 4 -> 8 -> 16 -> 32 -> 64 ticks -> OFF. Holding the key does not repeat; only a fresh press counts.

## Console window (Windows)
The first F6 press opens a separate console window titled "Input Buffer Demo". It logs each mode change, the full status line (ON/OFF, buffer size, stage, counters), and every input event (normal, held, buffered, expired). Its close button is disabled because closing a console window would close the whole game; it closes when you quit Geometry Dash. On non-Windows platforms the same lines go to the Geode log.

## Overlay (top-left during a level)
Mod status (ON/OFF), stage (idle / ready / armed), buffer size, and counters: normal / buffered / expired inputs.

## Behavior
- OFF: every input is passed straight to the game, untouched.
- ON: a press made while the player is airborne is remembered for N ticks. If the player lands inside the window, the press is replayed once (as a tap if you already released). If not, it expires and is counted as expired.
- Every buffered (replayed) click flashes the screen red for a quarter second.

## Supported versions
Written against Geode SDK v5.11.2 and Geometry Dash 2.2081 (Windows/Android/Mac declared in `mod.json`). Adjust `mod.json` if your installed versions differ. Android/Mac builds need those platforms' toolchains and are not covered by the workflow.

## Limitations
- Untested: hook behavior in real levels is unverified.
- The original specification was not included in the request, so the buffering logic is my interpretation of "configurable early-input buffering".
- Only the first early press is tracked at a time, and it uses the player's ground state, so ship/ball/wave modes are not specially handled.
- Using this in a level you submit for records may be treated as cheating by leaderboards.
