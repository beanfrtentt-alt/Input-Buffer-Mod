/**
 * SpoonClient / Hooks/HookManager.cpp
 *
 * In Geode v5, hook files use the `$modify` macro which auto-registers
 * at static-init time. There's nothing for this file to do at runtime
 * except log that all hook files are linked.
 *
 * The .cpp file still exists to ensure the hook .cpp files are pulled
 * into the link graph — without this translation unit, the linker
 * could drop the hook files (which only contain $modify blocks, no
 * externally-visible symbols) due to ELF/PE dead-strip rules. By
// including each hook .cpp file's header here transitively, we
 * guarantee the corresponding .cpp is compiled.
 */
#include "HookManager.hpp"
#include "PlayLayerHook.hpp"          // ensures PlayLayerHook.cpp is linked
#include "PlayerObjectHook.hpp"
#include "BaseGameLayerHook.hpp"
#include "KeyboardDispatchHook.hpp"

namespace spoon::hooks {

HookManager& HookManager::get() {
    static HookManager instance;
    return instance;
}

void HookManager::registerAll() {
    // No-op: $modify blocks self-register. Logging the call here gives
    // us a trace line in the Geode log viewer to confirm the call ran.
}

} // namespace spoon::hooks
