# Buffer ↔ Assisted Gameplay Interaction

This document explains exactly how the buffer and Assisted Gameplay
systems work together, so future contributors don't accidentally
break the contract.

## The single contract

> **AG never calls `PlayerObject::pushButton` directly.** Every AG
> input goes through `BufferManager::queueInput()` /
> `BufferManager::releaseInput()`.

This is the single most important contract in the mod. AG and the
human player share the same BufferManager, which means:

1. **Buffer settings always apply.** If the user disables the Cube
   buffer (sets frames to 0), AG inputs go in with zero-frame buffer —
   exactly the same as the human player's inputs would.
2. **No conflicting inputs.** The buffer is a single-slot queue per
   player slot. A fresh `queueInput` replaces (not stacks) any
   previously-queued input. This prevents AG and the human from
   fighting over the same slot.
3. **Emergency stop is O(1).** `BufferManager::clearAll()` wipes both
   P1 and P2 pending inputs in one call, with no traversal.

## Per-gamemode profile lifecycle

```
[Mod load]
     │
     ▼
Settings::load()              ◄── reads saved per-gamemode profiles
     │
     ▼
[Level start]
     │
     ▼
BufferManager::onLevelStart()
     │   - clears both pending-input slots
     └─► reloadProfile(false/true)
              │
              ▼
          Settings::profile(activeGamemode[slot])
              │
              ▼
          m_profiles[slot] = profile from disk
     │
     ▼
[Player passes through a portal]
     │
     ▼
PlayerObjectHook::hookUpdate detects m_gamemode changed
     │
     ▼
BufferManager::onGamemodeChange(player2, newMode)
     │   - flushes the previously-active buffer slot
     └─► reloadProfile(player2)   ◄── picks up the new gamemode's profile
     │
     ▼
[Buffer drain on every gameplay frame]
     │
     ▼
BufferManager::drain(currentFrame) returns DrainResult
     │
     ▼
Hook passes DrainResult back to GD's input dispatcher
     │
     ▼
PlayerObject::pushButton / releaseButton fires
```

## AG's interaction with the buffer

The ActionPlanner runs every 3rd gameplay frame (throttled in
`Manager::runPipeline`). On each planning tick:

1. The Manager calls `StateAnalyzer::analyze(playLayer)` to get a
   fresh `Snapshot`.
2. The Manager calls `ActionPlanner::decide(snapshot, lookahead)` to
   get a `Plan` struct with `wantPress`, `wantHold`, `wantRelease`,
   and `targetPlayer2` fields.
3. The Manager calls `InputController::apply(plan, currentFrame)`,
   which calls into `BufferManager`:
   * If `wantPress` — `queueInput(targetPlayer2, isHold=true, frame)`
   * If `wantHold` and not currently holding — `queueInput(...)`
   * If `wantHold` and already holding — no-op (the buffer keeps
     draining the hold)
   * If `wantRelease` — `releaseInput(targetPlayer2, frame)`

Between planning ticks (the 2 of every 3 frames the planner doesn't
run), the InputController does **nothing** — the buffer continues to
drain the previous decision at the gamemode's frame rate. This is
intentional:

* It keeps the buffer's per-frame drain behavior consistent regardless
  of when the planner happens to run.
* It means the buffer's `frames` value has real meaning for AG inputs:
  a Cube buffer of 8 means AG's queued input will linger up to 8
  frames waiting for a clean place to apply — exactly the same as a
  human press.

## Changing a buffer value mid-AG

The user can change a buffer value in the F6 menu while AG is running.
The new value affects **all subsequently-queued inputs** —
BufferManager's per-slot profile is read at queue time, so a
queueInput call after the menu change uses the new frame count.

Inputs already in the queue before the menu change keep their old
remainingFrames counter (we don't retroactively resize them). This
is the correct behavior — if the user lowers the buffer mid-input,
they don't want the existing input to suddenly extend further.

## Switching gamemodes mid-AG

When the player passes through a gamemode portal:

1. `PlayerObjectHook::hookUpdate` sees `m_gamemode` has changed.
2. It calls `BufferManager::onGamemodeChange(player2, newMode)`.
3. The buffer flushes the previously-active slot — any pending input
   for the old gamemode is dropped. This is important: holding a
   tap-style input across a portal would otherwise cause an unwanted
   double-jump on the new gamemode.
4. The buffer reloads its profile from `Settings::profile(newMode)` —
   so the new gamemode's frame count and enabled flag take effect
   immediately.
5. The Manager's next planning tick (`RecoveryManager::observe`)
   sees `GamemodeChanged` event and calls
   `InputController::reset()`, clearing any "still holding" state.
   The next plan starts fresh on the new gamemode.

## Death, respawn, and restart

| Event              | AG action                                     | Buffer action                       |
|--------------------|-----------------------------------------------|--------------------------------------|
| `PlayerDied`       | Reset InputController holding state           | `onDeath(player2)` — flushes slot    |
| `PlayerRespawned`  | Status → Active; resume planning              | Slot is already empty from death    |
| `LevelRestart`     | Reload LevelAnalyzer cache; reset planner     | `onLevelRestart()` — full flush      |
| `LevelExited`      | Stop AG; hide overlay                         | `clearAll()`                         |
| `GamemodeChanged`  | Reset planner state; status → Active          | `onGamemodeChange()` — flush + reload|

## Emergency stop (F8)

Pressing F8 (or whatever key is bound to `agEmergencyStopKey`):

1. `KeyboardDispatchHook::hookPressed` returns `true` — the event is
   consumed, so GD's own pipeline doesn't also see F8.
2. `Manager::emergencyStop()` is called, which:
   * Calls `InputController::reset()` — clears the holding state.
   * Calls `BufferManager::clearAll()` — wipes both P1 and P2 pending
     inputs in O(1).
   * Sets status to `Paused`.
3. AG remains "enabled" in settings but is in a paused state — the
   next planning tick will not issue any inputs because the Manager
   sees status `Paused` and returns early.

To resume AG, the user toggles it off and back on in the F6 menu
(which calls `setEnabled(false)` then `setEnabled(true)`), or
restarts the level (which fires `LevelRestart` and resets status to
`Ready`).

## What this means for users

If you tweak a buffer value in the F6 menu while AG is running:

* The next AG input is queued with the new value.
* Inputs already in the queue keep their old remainingFrames counter.
* You don't need to restart the level for the change to take effect.

If you switch gamemodes (e.g., a portal sequence) while AG is running:

* The buffer for the previous gamemode is flushed — no stale input
  carries across the portal.
* AG re-plans from the new gamemode's first frame.

If you press F8 (emergency stop):

* AG stops issuing inputs immediately.
* The current pending input is cancelled — the cube stops jumping
  the moment you press F8.
* The buffer is cleared.

If AG dies:

* The dead player's buffer slot is cleared (P1's slot if P1 died,
  P2's if P2 died in dual mode).
* AG's planner state is reset.
* After the respawn invulnerability window (~60 frames), AG resumes
  with a fresh plan based on the current player state.
