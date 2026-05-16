// Screen Tester 101 - Nintendo Switch homebrew display & touchscreen tester.
// Entry point: starts the App, with a text-console fallback on failure.

#include <switch.h>
#include <cstdio>
#include <strings.h>
#include "app.hpp"

// Optional launch argument: a mode name to jump straight into (e.g. "touch").
static ModeId parseStartMode(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "display"))  return ModeId::Display;
        if (!strcasecmp(argv[i], "touch"))    return ModeId::Touch;
        if (!strcasecmp(argv[i], "gesture"))  return ModeId::Gesture;
        if (!strcasecmp(argv[i], "controls")) return ModeId::Controls;
        if (!strcasecmp(argv[i], "hwinfo"))   return ModeId::HwInfo;
    }
    return ModeId::Menu;
}

int main(int argc, char* argv[]) {
    App app;
    if (!app.init(parseStartMode(argc, argv))) {
        // Fall back to a text console so the failure is visible.
        consoleInit(NULL);
        printf("Screen Tester 101: failed to initialise framebuffer.\n");
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

    app.run();
    app.deinit();
    return 0;
}
