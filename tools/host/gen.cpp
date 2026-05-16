// Off-device asset generator for NX Screen Test.
//
// It drives the views headlessly through the nxdisplaylib Runner (step /
// select / savePpm) and the mockctl.h scene hooks, writing a PPM still of
// every view plus a fully scripted feature walk-through. tools/host/Makefile
// turns those into the dist/ PNG stills and the showcase video via ffmpeg.
//
// The walk-through covers every feature at a watchable pace: all 11 display
// patterns, the three touch views, the four gestures, the controls test with
// live sensors, System Info, and a menu interlude between each segment.
#include "nxdisplaylib/runner.hpp"
#include "mode.hpp"
#include "menu_mode.hpp"
#include "display_mode.hpp"
#include "touch_mode.hpp"
#include "gesture_mode.hpp"
#include "controls_mode.hpp"
#include "hwinfo_mode.hpp"
#include "mockctl.h"
#include <cstdio>
#include <cmath>

using namespace nxd;

static const double kDt = 1.0 / 30.0;   // walk-through renders at 30 fps

static Runner      g_runner;
static const char* g_dir     = "dist/frames";
static int         g_frame   = 0;
static int         g_menuSel = 0;   // mirrors MenuMode's persistent cursor

// Step one walk-through frame and write it to the clip sequence.
static void emit(const Input& in) {
    const u32* px = g_runner.step(in);
    char path[256];
    snprintf(path, sizeof(path), "%s/clip_%05d.ppm", g_dir, g_frame++);
    savePpm(path, px);
}

static HidTouchState mkTouch(u32 id, int x, int y, u32 dia) {
    HidTouchState t{};
    t.finger_id  = id;
    t.x = (u32)(x < 0 ? 0 : x);
    t.y = (u32)(y < 0 ? 0 : y);
    t.diameter_x = t.diameter_y = dia;
    return t;
}

// --- segments ---------------------------------------------------------------

// Show the menu and walk the cursor to card `target`, between segments.
// MenuMode keeps its selection across visits, so the walk is relative to
// wherever the last interlude left it (wrapping over the four cards).
static void menuInterlude(int target, int frames) {
    g_runner.select(Menu);
    int moves = (target - g_menuSel + 4) % 4;
    g_menuSel = target;
    for (int f = 0; f < frames; f++) {
        Input in{};
        in.dtSec = kDt;
        if (moves > 0 && f >= 6 && f % 8 == 0) {
            in.down = HidNpadButton_AnyDown;
            moves--;
        }
        emit(in);
    }
}

// All 11 display patterns; pause on each, cycle the solid colour and motion.
static void segDisplay() {
    g_runner.select(Display);
    for (int p = 0; p < 11; p++) {
        for (int f = 0; f < 52; f++) {
            Input in{};
            in.dtSec = kDt;
            if (f == 0 && p > 0)                in.down  = HidNpadButton_AnyRight;
            if (p == 0  && (f == 16 || f == 34)) in.down |= HidNpadButton_AnyDown;
            if (p == 10 && (f == 14 || f == 30)) in.down |= HidNpadButton_AnyUp;
            emit(in);
        }
    }
}

// The three touch views: live multitouch, the paint canvas, the coverage grid.
static void segTouch() {
    g_runner.select(Touch);
    u64 samp = 0;
    for (int f = 0; f < 120; f++) {                 // Live
        Input in{};
        in.dtSec = kDt;
        samp += 4;
        in.touch.sampling_number = samp;
        in.touch.count = 2 + f / 45;
        for (int i = 0; i < in.touch.count; i++)
            in.touch.touches[i] = mkTouch(i,
                340 + i * 240 + (int)(150 * sinf(f * 0.10f + i)),
                280 + i * 110 + (int)(90  * cosf(f * 0.12f + i)), 72 + i * 12);
        emit(in);
    }
    { Input y{}; y.dtSec = kDt; y.down = HidNpadButton_Y; emit(y); }
    for (int f = 0; f < 120; f++) {                 // Paint
        Input in{};
        in.dtSec = kDt;
        in.touch.count = 1;
        in.touch.touches[0] = mkTouch(0, 110 + f * 9,
                                      360 + (int)(230 * sinf(f * 0.14f)), 60);
        emit(in);
    }
    { Input y{}; y.dtSec = kDt; y.down = HidNpadButton_Y; emit(y); }
    for (int f = 0; f < 150; f++) {                 // Grid
        Input in{};
        in.dtSec = kDt;
        int cell = f < 144 ? f : 143;
        in.touch.count = 1;
        in.touch.touches[0] = mkTouch(0, (cell % 16) * 80 + 40,
                                      (cell / 16) * 80 + 40, 60);
        emit(in);
    }
}

// Tap, swipe, pinch and rotate - each fed as the moving point sequence the
// HID gesture engine would report.
static void segGesture() {
    g_runner.select(Gesture);
    const int seg = 64, activeFor = 46, cx = 720, cy = 380;
    for (int s = 0; s < 4; s++) {
        for (int f = 0; f < seg; f++) {
            bool  active = f < activeFor;
            float t = (float)f / (float)activeFor;
            HidGestureState g{};
            g.context_number = s + 1;
            if (s == 0) {                                   // Tap
                g.type = active ? HidGestureType_Tap : HidGestureType_Idle;
                g.x = cx; g.y = cy;
                g.point_count = 1;
                g.points[0].x = cx; g.points[0].y = cy;
            } else if (s == 1) {                            // Swipe
                int x = cx - 200 + (int)(400 * t);
                g.type = active ? HidGestureType_Swipe : HidGestureType_Idle;
                g.direction = 3;
                g.x = (u32)x; g.y = cy;
                g.delta_x = 28; g.velocity_x = 940.0f;
                g.point_count = 1;
                g.points[0].x = (u32)x; g.points[0].y = cy;
            } else if (s == 2) {                            // Pinch
                int spread = 36 + (int)(170 * t);
                g.type = active ? HidGestureType_Pinch : HidGestureType_Idle;
                g.x = cx; g.y = cy;
                g.scale = 1.0f + 1.6f * t;
                g.point_count = 2;
                g.points[0].x = (u32)(cx - spread); g.points[0].y = cy;
                g.points[1].x = (u32)(cx + spread); g.points[1].y = cy;
            } else {                                        // Rotate
                float a = t * 2.1f;
                const int r = 130;
                g.type = active ? HidGestureType_Rotate : HidGestureType_Idle;
                g.x = cx; g.y = cy;
                g.rotation_angle = a * 57.2958f;
                g.point_count = 2;
                g.points[0].x = (u32)(cx + r * cosf(a));
                g.points[0].y = (u32)(cy + r * sinf(a));
                g.points[1].x = (u32)(cx - r * cosf(a));
                g.points[1].y = (u32)(cy - r * sinf(a));
            }
            mockSetGesture(&g);
            Input in{};
            in.dtSec = kDt;
            emit(in);
        }
    }
}

// Controls test: every button lit in turn, both sticks orbiting, the motion
// sensor tilting, the capture button flashing and the volume sliding.
static void segControls() {
    g_runner.select(Controls);
    static const u64 buttons[] = {
        HidNpadButton_A, HidNpadButton_B, HidNpadButton_X, HidNpadButton_Y,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right,
        HidNpadButton_L, HidNpadButton_R, HidNpadButton_ZL, HidNpadButton_ZR,
    };
    for (int f = 0; f < 270; f++) {
        Input in{};
        in.dtSec = kDt;
        in.held = buttons[(f / 11) % 12];
        in.lstick.x = (s32)(26000 * sinf(f * 0.10f));
        in.lstick.y = (s32)(26000 * cosf(f * 0.10f));
        in.rstick.x = (s32)(22000 * sinf(f * 0.08f + 1.6f));
        in.rstick.y = (s32)(22000 * cosf(f * 0.08f + 1.6f));

        HidSixAxisSensorState s{};
        s.attributes = HidSixAxisSensorAttribute_IsConnected;
        s.acceleration.x = 0.6f * sinf(f * 0.06f);
        s.acceleration.y = 0.6f * cosf(f * 0.05f);
        s.acceleration.z = 0.8f;
        s.angular_velocity.x = 0.4f * cosf(f * 0.06f);
        s.angular_velocity.z = 0.3f * sinf(f * 0.07f);
        s.direction[0] = s.direction[4] = s.direction[8] = 1.0f;
        mockSetSixAxis(&s);
        mockSetCapture((f % 80) < 4);
        mockSetVolume(40 + (int)(20 * sinf(f * 0.07f)));
        emit(in);
    }
    mockSetCapture(false);
}

static void segHwInfo() {
    g_runner.select(HwInfo);
    for (int f = 0; f < 80; f++) {
        Input in{};
        in.dtSec = kDt;
        emit(in);
    }
}

// --- main -------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc > 1) g_dir = argv[1];

    static MenuMode     menu;
    static DisplayMode  display;
    static TouchMode    touch;
    static GestureMode  gesture;
    static ControlsMode controls;
    static HwInfoMode   hwinfo;
    static View* views[] = { &menu, &display, &touch, &gesture, &controls, &hwinfo };

    RunnerConfig cfg;
    cfg.homeIndex  = Menu;
    cfg.cycleCount = COUNT;
    cfg.showFps    = true;

    if (!g_runner.init(views, COUNT, cfg)) {
        printf("gen: framebuffer init failed\n");
        return 1;
    }

    // --- one settled still per view ------------------------------------
    for (int i = 0; i < COUNT; i++) {
        g_runner.select(i);
        const u32* px = nullptr;
        Input idle{};
        idle.dtSec = kDt;
        for (int s = 0; s < 14; s++) px = g_runner.step(idle);
        char tag[64];
        snprintf(tag, sizeof(tag), "%s", g_runner.view(i)->name());
        for (char* c = tag; *c; c++) if (*c == ' ' || *c == '/') *c = '_';
        char path[256];
        snprintf(path, sizeof(path), "%s/still_%s.ppm", g_dir, tag);
        savePpm(path, px);
    }

    // --- scripted feature walk-through ---------------------------------
    menuInterlude(0, 70);   // -> Display Test
    segDisplay();
    menuInterlude(1, 56);   // step down -> Touchscreen Test
    segTouch();
    menuInterlude(2, 56);   // step down -> Gesture Test
    segGesture();
    menuInterlude(3, 56);   // step down -> Controls Test
    segControls();
    menuInterlude(3, 50);   // stays on Controls; System Info opens with Minus
    segHwInfo();
    menuInterlude(0, 60);   // wrap back round to Display Test

    g_runner.deinit();
    printf("gen: %d stills + %d walk-through frames -> %s\n", COUNT, g_frame, g_dir);
    return 0;
}
