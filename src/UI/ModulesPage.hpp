/**
 * SpoonClient / UI/ModulesPage.hpp
 *
 * The Modules tab of the F6 menu. Shows a flat list of every "module" in
 * the mod (buffer feature, AG feature, keybind menu, etc.) with an enable
 * / disable toggle and a one-line description.
 *
 * Currently this is a thin shell — most modules are toggled in their
 * respective pages. The page exists primarily for discoverability so the
 * user can see at-a-glance what's enabled. New modules should add their
 * rows here as they're introduced.
 */
#pragma once
#include <Geode/Geode.hpp>

namespace spoon {

class ModulesPage : public cocos2d::CCLayer {
public:
    static ModulesPage* create();
    bool init() override;

private:
    void build();
};

} // namespace spoon
