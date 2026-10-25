# Architecture

This document explains the high-level architecture of Spoon Client so
future contributors know where to add new features without breaking
existing contracts.

## Layer diagram

```
┌──────────────────────────────────────────────────────────────────┐
│  Geode SDK (loader, hooks, setting API, cocos2d binding)         │
├──────────────────────────────────────────────────────────────────┤
│  SpoonClient Hooks (src/Hooks/)                                  │
│   PlayLayerHook       PlayerObjectHook                           │
│   BaseGameLayerHook   KeyboardDispatchHook                       │
├──────────────────────────────────────────────────────────────────┤
│  SpoonClient Subsystems                                          │
│   ┌───────────────┐    ┌────────────────────────┐                │
│   │ BufferManager │◄───│ Assisted Gameplay Mgr  │                │
│   │  (per-gm)     │    │  ├ StateAnalyzer      │                │
│   └───────────────┘    │  ├ LevelAnalyzer      │                │
│          ▲             │  ├ ActionPlanner      │                │
│          │             │  ├ InputController    │                │
│   ┌──────┴──────┐      │  ├ RecoveryManager    │                │
│   │  Settings   │      │  └ OverlayUI          │                │
│   │ (per-gm)    │      └────────────────────────┘                │
│   └─────────────┘                                                │
├──────────────────────────────────────────────────────────────────┤
│  SpoonClient UI (src/UI/)                                       │
│   SpoonMenuLayer   BufferPage   AssistedGameplayPage   ModulesPage │
└──────────────────────────────────────────────────────────────────┘
```

## The BufferManager — single source of truth for inputs

The **BufferManager** is the *only* component in the mod that decides
when a queued input reaches `PlayerObject`. Both the human player (via
`PlayerObjectHook::pushButton`) and Assisted Gameplay (via
`InputController`) push inputs *into* the buffer; the buffer, configured
per-gamemode, decides when to *release* them.

This design guarantees three properties the spec asked for:

1. **AG and the human share the input path** — AG can never bypass the
   buffer, so changing a buffer value in the F6 menu instantly affects
   AG's input timing.
2. **No stuck inputs** — disabling AG calls
   `BufferManager::clearAll()`, which wipes both P1 and P2 pending
   inputs in O(1).
3. **Per-gamemode behavior is real, not cosmetic** — the runtime buffer
   has a separate profile per gamemode and per player slot (P1/P2 in
   dual mode). A dual-mode level where P1 is Cube and P2 is Ship
   keeps **two** independent buffers with different frame counts.

## The Assisted Gameplay pipeline

The Manager orchestrates a 5-step pipeline every gameplay frame:

```
PlayLayerHook::hookUpdate
       │
       ▼
  ag::Manager::onUpdate(playLayer)
       │
       ├─► StateAnalyzer::analyze(playLayer)  ──► Snapshot
       │
       ├─► RecoveryManager::observe(snap)    ──► Event
       │    └─ (PlayerDied → flush, PlayerRespawned → Active,
       │       LevelRestart → reload, GamemodeChanged → flush)
       │
       ├─► ActionPlanner::decide(snap, lookahead) ──► Plan
       │    (throttled: runs every 3rd frame)
       │
       ├─► InputController::apply(plan, frame)
       │    └─► BufferManager::queueInput / releaseInput
       │
       └─► OverlayUI::update(status, reason, gm, frames)
```

The throttle is important: the planner runs at ~80 Hz (every 3rd
240 Hz frame) rather than 240 Hz. Between planner ticks, the
InputController remembers the last decision and continues to apply
"hold" inputs at full rate, while the BufferManager continues to
drain at full rate. This keeps CPU usage under 1% of a frame while
keeping input timing tight.

## Settings flow

```
disk (mod.json) ─► Settings::load() ─► in-memory Settings singleton
                       ▲                          │
                       │                          ▼
                  saveXxx()              every runtime subsystem
                                              │
                                              ▼
                                         BufferManager
                                         ActionPlanner
                                         OverlayUI
                                         ...
```

* `Settings::load()` is called from `main.cpp`'s `GEODE_API(Mod)`.
* Every subsystem keeps a `Settings&` reference (or calls
  `Settings::get()` on demand) so it sees the latest values.
* The F6 menu writes back through `saveBufferProfile` /
  `saveAgSettings` / `saveKeybinds`, which call Geode's
  `Mod::setSettingValue<T>(key, val)` and immediately update the
  in-memory state.

## Migration

`Settings::migrate()` bumps the saved `configVersion` from 1 → 2
(rename `agLookahead` from a millisecond value to a frame count) and
preserves the user's old value as closely as possible. Unknown /
missing keys silently fall back to the `mod.json` defaults. Future
migrations add new `if (v < N)` blocks here.

## Hook registration

All hooks are registered via `HookManager::registerAll()`, which calls
each hook class's `register_()` static method. The pattern is:

```cpp
Mod::get()->hook(
    reinterpret_cast<void*>(geode::addresser::nonVirtual(&Class::method)),
    reinterpret_cast<void*>(&Self::hookMethod),
    "Class::method",
    tulip::hook::TulipHook::HandlerOrder::Late);
```

`Late` order runs our hook after the original method; `Pre` order
runs before. We use `Pre` for `pushButton` / `releaseButton` so the
buffer sees the input before GD's own pipeline processes it.

Geode automatically unregisters every hook owned by the mod when the
mod is unloaded, so we don't need an explicit unload path.
