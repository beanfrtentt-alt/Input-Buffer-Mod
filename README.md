# Spoon Client

A polished Geometry Dash **2.2081** mod built on the **Geode SDK v5**.
Spoon Client ships three primary features:

1. **F6 in-game menu** — a clean, tabbed settings panel styled after GD's
   own UI, opened with the F6 key.
2. **Per-gamemode input buffering** — every player gamemode (Cube, Ship,
   Ball, UFO, Wave, Robot, Spider, Swing) gets its own persistent buffer
   profile. Dual mode is handled correctly with two independent buffer
   slots.
3. **Assisted Gameplay** — a working automation system that uses
   receding-horizon obstacle-aware planning to drive the player from
   level start to completion (where the level layout permits it).

## Target environment

| Component | Version |
|-----------|---------|
| Geometry Dash | 2.2081 (the latest stable GD that Geode v5 fully supports — Geode v5.11.2 also has partial crash-fix support for the just-released 2.2091, but 2.2081 is the primary target) |
| Geode SDK | v5.x (workflow pins v5.11.2; bump in `.github/workflows/build.yml` to track new Geode releases) |
| C++ standard | C++23 (Geode v5 requirement) |
| Build OS | Windows (the primary platform GD runs on) |
| Build system | CMake ≥ 3.21 + the Geode CLI |

> **About "GD 2.801"**: this version doesn't exist. The user request
> appears to have been a typo of "2.2081" (dropping the leading 2). The
> closest valid Geode-supported GD versions are **2.2081** (stable,
> targeted here) and **2.2091** (just-released, with partial Geode
> support — not recommended yet). See `docs/TROUBLESHOOTING.md` if you
> actually wanted to target 2.2091.

## Quick navigation

| Topic              | File                                  |
|--------------------|----------------------------------------|
| Build instructions | [docs/BUILD.md](docs/BUILD.md)              |
| Architecture       | [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| Test procedures    | [docs/TEST_PROCEDURES.md](docs/TEST_PROCEDURES.md) |
| Known limitations   | [docs/KNOWN_LIMITATIONS.md](docs/KNOWN_LIMITATIONS.md) |
| Buffer & AG interaction | [docs/BUFFER_AG_INTERACTION.md](docs/BUFFER_AG_INTERACTION.md) |
| GitHub Actions troubleshooting | [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) |

## The fastest path: let GitHub Actions build it for you

You don't need to install any tools locally.

1. **Download the ZIP** of this repository (or fork it on GitHub).
2. Create a new GitHub repository named `spoon-client`.
3. Push the ZIP contents to the repository (commit them to `main`).
4. Open the repository's **Actions** tab.
5. The workflow named **Build Spoon Client** will run automatically.
6. Once the run finishes (typically 5–8 minutes), click into the run,
   scroll to the **Artifacts** section, and download the
   `SpoonClient-Geode-v5-GD-2.2081-<commit>` artifact.
7. Unzip the downloaded artifact — it contains the `SpoonClient.geode`
   file plus (optionally) the `.pdb` debug symbols.
8. Drop `SpoonClient.geode` into your Geometry Dash mods directory
   (`%APPDATA%/GeometryDash/geode/mods/dev.spoonclient.mod/`).
9. Launch Geometry Dash. Open the Geode log viewer
   (Window → Geode → Logs, or press F12). You should see a line:
   `[SpoonClient] v2.0.0 initialized. F6 menu key = 'F6', AG emergency stop = 'F8'.`
10. Press F6 in any menu or during gameplay — the Spoon Client panel
    appears.

Full step-by-step instructions are in `docs/BUILD.md`.

## Features at a glance

### F6 Menu

* 3 tabs: Modules, Buffer, Assisted Gameplay.
* Persistent settings (saved to disk via Geode's setting API).
* Live status footer showing the active gamemode + buffer size + AG state.
* Closes with F6, Escape, or the close button.
* Touch + keyboard intercept while open — gameplay inputs are not
  triggered while you're navigating the menu.

### Per-gamemode buffers

* One profile per gamemode, each with its own `enabled` toggle and
  `frames` (0..30) integer.
* Profiles are stored in Geode's saved settings as flat keys
  (`bufferCubeFrames`, `bufferShipFrames`, ...) so they survive GD
  restarts and survive the mod being uninstalled+reinstalled.
* Dual mode has **two** runtime buffer slots — P1 and P2 are
  independent, even when they're in different gamemodes.
* A `Reset to defaults` button per row, plus `Reset all`.

### Assisted Gameplay

* Toggleable from the F6 menu or the saved Geode settings.
* 5-component architecture: StateAnalyzer, LevelAnalyzer,
  ActionPlanner, InputController, RecoveryManager, plus an OverlayUI.
* Live on-screen status: Idle / Ready / Active / Recovering / Paused /
  Completed / Error.
* Emergency-stop keybind (F8 by default) clears all queued inputs in
  O(1).
* All AG inputs go through the BufferManager — AG and the human player
  share the same input queue, so changing a buffer value in the menu
  immediately affects AG's input timing.

## Repository layout

```
spoon-client/
├── mod.json                       # Geode v5 mod metadata + settings schema
├── CMakeLists.txt                # v5-style CMake (C++23, GLOB_RECURSE, setup_geode_mod)
├── README.md
├── .gitignore
├── .github/workflows/build.yml   # GitHub Actions: builds for Win64 + uploads artifact
├── docs/
│   ├── ARCHITECTURE.md
│   ├── BUILD.md
│   ├── TEST_PROCEDURES.md
│   ├── KNOWN_LIMITATIONS.md
│   ├── BUFFER_AG_INTERACTION.md
│   └── TROUBLESHOOTING.md
├── resources/
│   └── README.md                 # how to add custom sprites
└── src/
    ├── main.cpp                  # MenuLayer::init $modify boot hook
    ├── Utils.hpp                 # gamemode enum, clamp, debug log macros
    ├── SpoonSettings.{hpp,cpp}   # validated config store + migration
    ├── BufferManager.{hpp,cpp}   # per-gamemode runtime buffer
    ├── UI/
    │   ├── SpoonMenuLayer.{hpp,cpp}
    │   ├── BufferPage.{hpp,cpp}
    │   ├── AssistedGameplayPage.{hpp,cpp}
    │   └── ModulesPage.{hpp,cpp}
    ├── AssistedGameplay/
    │   ├── State.hpp              # plain-data snapshot
    │   ├── Manager.{hpp,cpp}
    │   ├── StateAnalyzer.{hpp,cpp}
    │   ├── LevelAnalyzer.{hpp,cpp}
    │   ├── ActionPlanner.{hpp,cpp}
    │   ├── InputController.{hpp,cpp}
    │   ├── RecoveryManager.{hpp,cpp}
    │   └── OverlayUI.{hpp,cpp}
    └── Hooks/
        ├── HookManager.{hpp,cpp}  # links all hook .cpp files
        ├── PlayLayerHook.{hpp,cpp}      # $modify(PlayLayer)
        ├── PlayerObjectHook.{hpp,cpp}   # $modify(PlayerObject)
        ├── BaseGameLayerHook.{hpp,cpp}  # no-op (cleanup delegated to PlayLayer::onQuit)
        └── KeyboardDispatchHook.{hpp,cpp} # $modify(KeyboardEventDispatcher)
```

## License & attribution

Spoon Client is provided as source for the GD community. The mod uses
the Geode SDK (BSL-1.0) and Geometry Dash binding headers; see the
Geode repository for their respective licenses.
