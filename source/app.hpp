#pragma once
#include <switch.h>
#include "gfx.hpp"
#include "mode.hpp"

// Top-level application: owns the renderer, input devices and the test modes,
// runs the main loop, dispatches input and draws the shared chrome (top bar
// with mode name + FPS, bottom bar with the active mode's controls).
class App {
public:
    bool init(ModeId start = ModeId::Menu);
    void run();
    void deinit();

private:
    void switchTo(ModeId id);
    void drawChrome(Mode* m);

    Gfx      gfx_;
    PadState pad_{};
    Mode*    modes_[(int)ModeId::COUNT] = {};
    ModeId   current_ = ModeId::Menu;

    // Display render-rate measurement (rolling average over ~0.5 s).
    u64    lastTick_  = 0;
    double fpsAcc_    = 0.0;
    int    fpsFrames_ = 0;
    double fpsValue_  = 0.0;
};
