#include "app.hpp"
#include "menu_mode.hpp"
#include "display_mode.hpp"
#include "touch_mode.hpp"
#include "gesture_mode.hpp"
#include <cstdio>

// Concrete mode instances live for the whole program.
static MenuMode    s_menu;
static DisplayMode s_display;
static TouchMode   s_touch;
static GestureMode s_gesture;

bool App::init(ModeId start) {
    if (!gfx_.init())
        return false;

    // A single player reading handheld + standard controllers.
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad_);

    // Bring up the HID subsystems the test modes rely on.
    hidInitializeTouchScreen();
    hidInitializeGesture();

    modes_[(int)ModeId::Menu]    = &s_menu;
    modes_[(int)ModeId::Display] = &s_display;
    modes_[(int)ModeId::Touch]   = &s_touch;
    modes_[(int)ModeId::Gesture] = &s_gesture;

    current_ = (start >= ModeId::Menu && start < ModeId::COUNT) ? start : ModeId::Menu;
    modes_[(int)current_]->onEnter();

    lastTick_ = armGetSystemTick();
    return true;
}

void App::deinit() {
    gfx_.deinit();
}

void App::switchTo(ModeId id) {
    if (id == ModeId::COUNT || id == current_) return;
    modes_[(int)current_]->onExit();
    current_ = id;
    Mode* m = modes_[(int)current_];
    m->requestMode = ModeId::COUNT;
    m->onEnter();
}

void App::drawChrome(Mode* m) {
    const u32 barBg  = Gfx::rgb(18, 20, 28);
    const u32 fg     = Gfx::rgb(232, 234, 244);
    const u32 accent = Gfx::rgb(120, 180, 255);
    const u32 shadow = Gfx::rgb(0, 0, 0);
    const int topH = 30, botH = 30;
    const int botY = Gfx::H - botH + 7;

    char fps[32];
    snprintf(fps, sizeof(fps), "FPS %.1f", fpsValue_);
    int fpsX = Gfx::W - gfx_.textWidth(2, fps) - 12;

    if (m->transparentChrome()) {
        // No solid bars: shadowed text so the underlying view shows through.
        gfx_.drawTextShadow(12, 7, 2, accent, shadow, m->name());
        gfx_.drawTextShadow(fpsX, 7, 2, fg, shadow, fps);
        gfx_.drawTextShadow(12, botY, 2, fg, shadow, m->controls());
    } else {
        // Top bar: mode name (left) and live render FPS (right).
        gfx_.fillRect(0, 0, Gfx::W, topH, barBg);
        gfx_.drawText(12, 7, 2, accent, m->name());
        gfx_.drawText(fpsX, 7, 2, fg, fps);

        // Bottom bar: the active mode's controls.
        gfx_.fillRect(0, Gfx::H - botH, Gfx::W, botH, barBg);
        gfx_.drawText(12, botY, 2, fg, m->controls());
    }
}

void App::run() {
    while (appletMainLoop()) {
        // --- timing ---
        u64 now = armGetSystemTick();
        double dt = (double)armTicksToNs(now - lastTick_) / 1.0e9;
        lastTick_ = now;
        if (dt > 0.25) dt = 0.25;   // clamp after suspend / long stalls

        fpsAcc_ += dt;
        fpsFrames_++;
        if (fpsAcc_ >= 0.5) {
            fpsValue_  = fpsFrames_ / fpsAcc_;
            fpsAcc_    = 0.0;
            fpsFrames_ = 0;
        }

        // --- input ---
        padUpdate(&pad_);
        Input in;
        in.down  = padGetButtonsDown(&pad_);
        in.up    = padGetButtonsUp(&pad_);
        in.held  = padGetButtons(&pad_);
        in.dtSec = dt;
        hidGetTouchScreenStates(&in.touch, 1);

        // --- global navigation ---
        if (in.down & HidNpadButton_Plus)
            break;  // return to hbmenu

        if (in.down & HidNpadButton_ZR)
            switchTo((ModeId)(((int)current_ + 1) % (int)ModeId::COUNT));
        else if (in.down & HidNpadButton_ZL)
            switchTo((ModeId)(((int)current_ + (int)ModeId::COUNT - 1) % (int)ModeId::COUNT));
        else if ((in.down & HidNpadButton_B) && current_ != ModeId::Menu)
            switchTo(ModeId::Menu);

        // --- update active mode ---
        Mode* m = modes_[(int)current_];
        m->update(in);
        if (m->requestMode != ModeId::COUNT) {
            ModeId req = m->requestMode;
            m->requestMode = ModeId::COUNT;
            switchTo(req);
            m = modes_[(int)current_];
        }

        // --- render ---
        gfx_.beginFrame();
        m->render(gfx_);
        if (m->showChrome())
            drawChrome(m);
        m->renderOverlay(gfx_);   // drawn on top of the chrome bars
        gfx_.endFrame();
    }
}
