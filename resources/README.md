# Spoon Client resource directory

Place any custom sprites (PNGs) you want bundled with the mod here. They
will be referenced from source files using the `_spr` literal, e.g.:

    CCSprite::create("spoon-logo.png"_spr);

The Geode build pipeline picks up every `*.png` under this directory and
embeds it into the resulting `.geode` package. Fonts that ship with GD
(`bigFont.fnt`, `goldFont.fnt`, etc.) are NOT bundled — they're loaded
directly from the GD resource archives at runtime.

No resources are required for the mod to function correctly. The menu
uses GD's stock sprite frames (e.g., `GJ_button_01.png`,
`GJ_checkOn_001.png`) which are already available in every GD install.
