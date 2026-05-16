// NX Screen Test - Nintendo Switch homebrew display & touchscreen diagnostics.
// Entry point: wires the test views into a nxd::Runner, with a text-console
// fallback if the framebuffer cannot be brought up.

#include <switch.h>
#include <cstdio>
#include <strings.h>
#include "nxdisplaylib/runner.hpp"
#include "mode.hpp"
#include "menu_mode.hpp"
#include "display_mode.hpp"
#include "touch_mode.hpp"
#include "gesture_mode.hpp"
#include "controls_mode.hpp"
#include "hwinfo_mode.hpp"

// Optional launch argument: a mode name to jump straight into (e.g. "touch").
static int parseStartMode(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "display"))  return Display;
        if (!strcasecmp(argv[i], "touch"))    return Touch;
        if (!strcasecmp(argv[i], "gesture"))  return Gesture;
        if (!strcasecmp(argv[i], "controls")) return Controls;
        if (!strcasecmp(argv[i], "hwinfo"))   return HwInfo;
    }
    return Menu;
}

int main(int argc, char* argv[]) {
    static MenuMode     menu;
    static DisplayMode  display;
    static TouchMode    touch;
    static GestureMode  gesture;
    static ControlsMode controls;
    static HwInfoMode   hwinfo;

    // View list - index order matches the ModeId enum (Menu = 0).
    static View* views[] = {
        &menu, &display, &touch, &gesture, &controls, &hwinfo,
    };

    RunnerConfig cfg;
    cfg.homeIndex   = Menu;     // B returns to the menu
    cfg.cycleCount  = HwInfo;   // ZL/ZR cycle covers Menu..Controls; HwInfo excluded
    cfg.showFps     = true;
    cfg.initGesture = true;     // the gesture test needs the HID gesture engine
#ifdef NXD_HOST
    cfg.exitOnHomeBack = true;  // host preview: B/Esc on the menu quits
#endif

    Runner runner;
    if (!runner.init(views, COUNT, cfg, parseStartMode(argc, argv))) {
        // Fall back to a text console so the failure is visible.
        consoleInit(NULL);
        printf("NX Screen Test: failed to initialise framebuffer.\n");
        printf("Press + to exit.\n");

        PadState pad;
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        while (appletMainLoop()) {
            padUpdate(&pad);
            if (padGetButtonsDown(&pad) & HidNpadButton_Plus) break;
            consoleUpdate(NULL);
        }
        consoleExit(NULL);
        return 1;
    }

    runner.run();
    runner.deinit();
    return 0;
}
