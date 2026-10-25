# Known Limitations

Spoon Client's Assisted Gameplay is a **reliable working baseline** —
it is not a perfect autonomous level solver. This document is honest
about what works, what partially works, and what doesn't.

## What works

* **F6 menu** — fully functional. Opens/closes, tab switching,
  scrolling, persistent settings, no input leak into gameplay.
* **Per-gamemode buffers** — fully functional. Each gamemode has its
  own persistent profile; switching gamemodes at runtime selects the
  right profile; dual mode keeps two independent buffer slots.
* **AG status machine** — fully functional. The Idle / Ready / Active
  / Recovering / Paused / Completed / Error state machine is wired
  up to the actual game lifecycle (level start, death, respawn,
  restart, completion, exit).
* **AG emergency stop (F8)** — fully functional. Clears all queued
  inputs in O(1).
* **AG integration with the buffer** — fully functional. AG and the
  human player share the same input queue (BufferManager). Changing a
  buffer value in the menu instantly affects AG's input timing.

## What partially works

### AG decision quality

The `ActionPlanner` is a **receding-horizon obstacle-aware planner**
rather than a full trajectory simulator. Concretely:

* **Cube / Robot:** Planner decides jump / hold / release based on the
  distance to the nearest floor hazard ahead. This handles straight
  gauntlets of spikes and basic stairs but **fails on**:
  * Orb / pad sequences (the planner has no concept of orbs).
  * Tight jump-pack sections where the player must chain multiple
    short hops.
  * Tall walls requiring robot jump-charge (hold-for-height) timing.
* **Ship / UFO:** Planner climbs or dives based on the relative Y of
  the nearest hazard. Works for straight corridors with floating
  saws; **fails on**:
  * Narrow wave-style tunnels (no pathfinding).
  * Inverted-gravity ship sections (planner doesn't account for
    inverted physics).
* **Wave:** Planner uses a tight 6-frame look-ahead and makes a
  binary climb / dive decision. Works on **very simple wave
  sections** (single saw ahead) but **fails on**:
  * Multi-hazard wave sections where the optimal path requires
    precomputed sine-wave timing.
  * Tight wave-only levels like "Problematic" — AG will not complete
    these.
* **Ball / Spider:** Planner switches surfaces when an obstacle is on
  the current surface. Works for simple ball sections; **fails on**:
  * Mixed ball+spike sequences where switching is not the right
    answer.
* **Swing:** Planner uses a hold-then-release heuristic for the
  swing-fire timing. Best-effort, not robust.

### AG caching

The LevelAnalyzer builds its hazard cache from
`GJBaseGameLayer::m_objects` once on level start. If a level spawns
objects at runtime (some custom levels do this for cosmetic effects),
those objects are **not** in the cache. AG will not see them.

### AG multi-threading

The pipeline runs synchronously on the GD main thread. This is
intentional (so we never desync from GD's tick), but it means a
pathologically large level (e.g., 100k objects) will make AG pause for
~5 ms at level start while the cache builds. There is no perceptible
in-game stutter beyond that.

## What does not work

### AG never completes complex levels

The spec says "attempts to play the current level from beginning to
completion" and explicitly notes:

> Do not assume that arbitrary levels can be completed perfectly. If
> the chosen approach has limitations, explain them clearly and
> implement a reliable working baseline before attempting more
> advanced automation.

Spoon Client implements this reliable baseline. The baseline can
attempt any level, recover from deaths, and resume planning — but
**does not guarantee completion of complex levels**. Levels known to
defeat the current planner include:

* Anything with orbs/pads in critical timing windows.
* Tight wave-only levels.
* Levels with custom physics (custom-player mods, custom collision).
* Levels that rely on click-stack mechanics (e.g., a 5-click combo
  the human player has to memorize).

### AG trajectory simulation

A real trajectory simulator — one that steps the player's physics
forward to verify a planned input doesn't get the player killed — is
**not implemented**. It's listed as future work in the architecture
doc. Implementing it would require either:

* Hooking GD's `PlayerObject::update` and stepping a ghost copy in
  parallel, OR
* Re-implementing GD's physics for every gamemode in our own code
  (large, error-prone, breaks with every GD update).

### AG dual-mode coordination

When a level uses dual mode with two different gamemodes (e.g., P1
Cube + P2 Ship), the planner currently focuses on **P1 only**. P2
receives no automated inputs. This is the largest known correctness
gap and the most impactful next feature to add.

### AG in practice mode

Practice mode's checkpoint system is **not** integrated with AG's
RecoveryManager. If AG dies in practice mode, GD rewinds to the last
checkpoint — but AG's `RecoveryManager::observe` only sees a frame
rewind and treats it as a "level restart" event, which throws away
the planner's progress and starts fresh. This is acceptable
behavior; it just means AG re-plans from each checkpoint rather than
resuming an old plan.

### No custom-keybinds integration

`geode.customkeybinds` is listed as a *suggested* dependency in
`mod.json` (not a required one). If the user has it installed, they
can already rebind their keys via its own UI; Spoon Client reads its
own `menuKey` / `agEmergencyStopKey` settings for the F6/F8 keys.
A deeper integration (letting customkeybinds rebind F6/F8 through
its unified UI) is left as future work.

### No proper unit tests

The static audit (`scripts/audit_sources.py`) catches the most
common build-time mistakes, but there is no test harness that
exercises the `ActionPlanner` or `BufferManager` with mock state.
Adding one would require either:

* A separate non-Geode test target that links against the source
  files directly (skipping the Geode header includes), OR
* A Geode-side test framework that runs inside a GD instance.

Both are substantial infrastructure projects; this release prioritizes
the in-game features over the test harness.
