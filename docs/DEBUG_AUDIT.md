# Static Debug Audit (2026-10-25)

## Changes made

- Updated the GitHub Actions checkout and artifact actions to Node.js 24-compatible major versions.
- Fixed the artifact verification step: `geode-sdk/build-geode-mod` returns a directory, so the workflow now searches that directory for a non-empty `.geode` package.
- Switched the keyboard hook to `CCKeyboardDispatcher::dispatchKeyboardMSG`, which is the public keyboard-dispatch API for the target bindings.
- Replaced the nonexistent `CCMenuItemSpriteExt` spelling with `CCMenuItemSpriteExtra`.
- Corrected the Assisted Gameplay include path to `../BufferManager.hpp`.
- Corrected several PlayerObject API/member references to names present in the GD 2.2081 bindings, and removed a questionable binding include.
- Changed level-object iteration to range-based iteration over `m_objects` and used the exposed `m_maxObjectX` as an estimated level length.

## Important verification status

This was a static source audit. The Geode SDK and its Windows toolchain were not available in this environment, so no successful `.geode` build is claimed. Push these changes to GitHub and inspect the next Actions run; the compiler may reveal additional SDK-specific mismatches.

## Runtime limitations still requiring work

The current input-buffer and Assisted Gameplay pipeline is not end-to-end implemented yet. `PlayLayerHook::update` discards the `BufferManager::drain` result, while the PlayerObject hook still forwards human button presses immediately. `InputController` queues plans but does not itself inject press/release events into the game. Therefore, even after compilation succeeds, do not assume the configured buffer or automatic gameplay works until this dispatch path is implemented and tested in-game.
