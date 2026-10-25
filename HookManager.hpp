/**
 * SpoonClient / Hooks/HookManager.hpp
 *
 * In Geode v5, `$modify` blocks register themselves at static-init
 * time, so this class's `registerAll()` is mostly a no-op. It still
 * exists for two reasons:
 *
 *   1. main.cpp's startup hook calls it for explicit, log-visible
 *      confirmation that every hook file is included in the link.
 *   2. Future hooks that use raw `Mod::hook(...)` calls (e.g., for
 *      functions outside the Geode binding headers) have an obvious
 *      place to live.
 */
#pragma once

namespace spoon::hooks {

class HookManager {
public:
    static HookManager& get();

    // No-op in v5. Each hook file's `$modify` block handles its own
    // registration at static-init time. We keep this call so main.cpp
    // can confirm all hook files are linked into the binary.
    void registerAll();
};

} // namespace spoon::hooks
