/**
 * SpoonClient / AssistedGameplay/InputController.cpp
 *
 * Maps Plan decisions -> BufferManager.queueInput/releaseInput calls.
 *
 * State diagram (per player slot):
 *
 *              ┌──── (wantHold OR wantPress) ─────┐
 *              │                                  ▼
 *     [Idle] ──┤                            [Holding]
 *              │                                  │
 *              └── (wantRelease) ◀────────────────┘
 *
 * Wantpress is treated as a queue + immediate hold so the buffer drains
 * the press this frame and continues holding for subsequent frames until
 * the planner issues a wantRelease.
 */
#include "InputController.hpp"
#include "../../BufferManager.hpp"
#include <utility>

namespace spoon::ag {

InputController& InputController::get() {
    static InputController instance;
    return instance;
}

InputController::InputController() = default;

void InputController::apply(const Plan& plan, int64_t currentFrame) {
    int slot = plan.targetPlayer2 ? 1 : 0;
    bool& holding = m_holding[slot];

    if (plan.wantPress) {
        // Fresh press: queue the input now. The buffer will drain it on the
        // next gameplay frame and we'll be in the "holding" state.
        spoon::BufferManager::get().queueInput(
            /*isPlayer2=*/plan.targetPlayer2,
            /*isHold=*/true,    // presses are always followed by hold
            currentFrame);
        holding = true;
        return;
    }
    if (plan.wantHold && holding) {
        // Already holding; no need to re-queue. The buffer's hold drain
        // continues to emit the input until release.
        return;
    }
    if (plan.wantHold && !holding) {
        // Lost track of state somehow — start a fresh hold.
        spoon::BufferManager::get().queueInput(
            plan.targetPlayer2, /*isHold=*/true, currentFrame);
        holding = true;
        return;
    }
    if (plan.wantRelease && holding) {
        spoon::BufferManager::get().releaseInput(
            plan.targetPlayer2, currentFrame);
        holding = false;
        return;
    }
    // No input desired this tick — keep current holding state unchanged.
    // If we're holding, the buffer keeps draining hold inputs automatically.
}

void InputController::reset() {
    m_holding[0] = false;
    m_holding[1] = false;
    spoon::BufferManager::get().clearAll();
}

} // namespace spoon::ag
