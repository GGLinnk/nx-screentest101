#pragma once
#include "mode.hpp"

// Display panel test: cycles through full-screen patterns used to spot dead /
// stuck pixels, backlight bleed, colour banding, geometry and flicker issues.
class DisplayMode : public Mode {
public:
    void onEnter() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    const char* name() const override { return "Display Test"; }
    const char* controls() const override;
    bool showChrome() const override { return hud_; }

private:
    int    pattern_   = 0;     // index into the pattern list
    int    solidIdx_  = 0;     // sub-index for the solid-colour pattern
    bool   hud_       = true;  // Y toggles the on-screen overlay
    int    strobeHz_  = 4;     // 1..30 Hz, adjustable with D-pad Up/Down
    bool   strobeWhite_ = false;
    double strobeAcc_ = 0.0;
};
