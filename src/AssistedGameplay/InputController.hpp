/**
 * SpoonClient / AssistedGameplay/InputController.hpp
 *
 * The InputController is the ONLY component that's allowed to push inputs
 * through the actual GD input pipeline. It goes through the BufferManager
 * — never directly to PlayerObject. This guarantees:
 *
 *   1. Buffer settings still apply. If the user disables the buffer for the
 *      current gamemode, AG inputs go in with zero-frame buffer (same as
 *      the human player's inputs would).
 *   2. No conflicting inputs. The human player's queued input is always
 *      replaced (not stacked) with AG's input for that slot — the buffer
 *      is a single-slot queue per player.
 *   3. Emergency stop works in O(1). Calling `clearAll()` on the buffer
 *      wipes both pending inputs in one call, no traversal needed.
 *
 * The controller also remembers the *last* plan it executed so that
 * `wantHold` decisions can be extended across multiple ticks without
 * spamming the buffer with redundant queueInput() calls.
 */
#pragma once
#include "State.hpp"
#include "ActionPlanner.hpp"

namespace gd { class PlayLayer; }

namespace spoon::ag {

class InputController {
public:
    static InputController& get();

    // Apply a plan to the actual GD input pipeline via the BufferManager.
    // `currentFrame` is the gameplay frame counter (from StateAnalyzer).
    void apply(const Plan& plan, int64_t currentFrame);

    // Reset internal "last pressed" state. Called when the manager stops AG
    // so we don't carry a stale "still holding" decision into the next run.
    void reset();

    // For debug overlay: are we currently holding input?
    bool isHolding(bool player2) const { return m_holding[player2 ? 1 : 0]; }

private:
    InputController();
    bool m_holding[2] = { false, false };
};

} // namespace spoon::ag
