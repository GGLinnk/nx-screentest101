#pragma once
#include <switch.h>

class Gfx;

// Identifies every screen the App can show. COUNT also doubles as "no request".
enum class ModeId {
    Menu = 0,
    Display,
    Touch,
    Gesture,
    Controls,
    HwInfo,
    COUNT
};

// Per-frame input snapshot handed to the active mode.
struct Input {
    u64 down  = 0;   // buttons newly pressed this frame
    u64 up    = 0;   // buttons newly released this frame
    u64 held  = 0;   // buttons currently held
    HidTouchScreenState touch{};   // up to 16 simultaneous touch points
    HidAnalogStickState lstick{};  // left analog stick position
    HidAnalogStickState rstick{};  // right analog stick position
    double dtSec = 0.0;            // wall-clock seconds since previous frame
};

// Abstract test screen. The App drives update() then render() each frame.
class Mode {
public:
    virtual ~Mode() {}

    virtual void onEnter() {}
    virtual void onExit()  {}
    virtual void update(const Input& in) = 0;
    virtual void render(Gfx& g) = 0;
    // Drawn after the App chrome, so it appears on top of the top/bottom bars.
    virtual void renderOverlay(Gfx& g) { (void)g; }

    virtual const char* name() const = 0;
    virtual const char* controls() const { return ""; }

    // When false, the App suppresses its own top/bottom chrome this frame
    // (used by the display test for an unobstructed panel view).
    virtual bool showChrome() const { return true; }

    // When true, the App draws its chrome text without the solid bars,
    // so the underlying view shows through (used by the paint canvas).
    virtual bool transparentChrome() const { return false; }

    // When true, the App leaves ZL/ZR to the mode instead of using them to
    // cycle modes (the Controls test needs every button free to be tested).
    virtual bool capturesCycle() const { return false; }

    // A mode sets this to ask the App to switch screens; the App clears it.
    ModeId requestMode = ModeId::COUNT;
    void requestSwitch(ModeId m) { requestMode = m; }
};
