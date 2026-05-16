// Off-device scene generator. Two modes:
//   ./mockgen           render each mode to build/*.ppm (stills + short anims)
//   ./mockgen --video   render a full scripted feature walk-through, piped to
//                       ffmpeg as dist/showcase.mp4
//
// tools/host/gen-assets.sh turns the build/*.ppm frames into dist/ PNGs/GIFs.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <switch.h>
#include "mockctl.h"
#include "gfx.hpp"
#include "mode.hpp"
#include "menu_mode.hpp"
#include "display_mode.hpp"
#include "touch_mode.hpp"
#include "gesture_mode.hpp"
#include "controls_mode.hpp"
#include "hwinfo_mode.hpp"

static const double kDt = 1.0 / 30.0;
static Gfx   g_gfx;
static FILE* g_pipe = nullptr;                       // ffmpeg stdin, in video mode
static unsigned char g_rgb[Gfx::W * Gfx::H * 3];

// Replica of App::drawChrome so off-device frames match the on-device look.
static void drawChrome(Mode* m) {
    const u32 barBg  = Gfx::rgb(18, 20, 28);
    const u32 fg     = Gfx::rgb(232, 234, 244);
    const u32 accent = Gfx::rgb(120, 180, 255);
    const u32 shadow = Gfx::rgb(0, 0, 0);
    const int topH = 30, botH = 30;
    const int botY = Gfx::H - botH + 7;
    const char* fps = "FPS 60.0";
    int fpsX = Gfx::W - g_gfx.textWidth(2, fps) - 12;

    if (m->transparentChrome()) {
        g_gfx.drawTextShadow(12, 7, 2, accent, shadow, m->name());
        g_gfx.drawTextShadow(fpsX, 7, 2, fg, shadow, fps);
        g_gfx.drawTextShadow(12, botY, 2, fg, shadow, m->controls());
    } else {
        g_gfx.fillRect(0, 0, Gfx::W, topH, barBg);
        g_gfx.drawText(12, 7, 2, accent, m->name());
        g_gfx.drawText(fpsX, 7, 2, fg, fps);
        g_gfx.fillRect(0, Gfx::H - botH, Gfx::W, botH, barBg);
        g_gfx.drawText(12, botY, 2, fg, m->controls());
    }
}

static void renderFrame(Mode* m) {
    g_gfx.beginFrame();
    m->render(g_gfx);
    if (m->showChrome())
        drawChrome(m);
    m->renderOverlay(g_gfx);
    g_gfx.endFrame();
}

static void writePPM(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "build/%s.ppm", name);
    FILE* f = fopen(path, "wb");
    if (!f) { printf("  cannot write %s\n", path); return; }
    const u32* px = mockFramebuffer();
    fprintf(f, "P6\n%d %d\n255\n", Gfx::W, Gfx::H);
    for (int i = 0; i < Gfx::W * Gfx::H; i++) {
        u32 p = px[i];
        unsigned char rgb[3] = { (unsigned char)(p & 0xff),
                                 (unsigned char)((p >> 8) & 0xff),
                                 (unsigned char)((p >> 16) & 0xff) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("  build/%s.ppm\n", name);
}

// Stills capture: render and write a PPM.
static void capture(Mode* m, const char* name) {
    renderFrame(m);
    writePPM(name);
}

// Video capture: render and push an rgb24 frame to the ffmpeg pipe.
static void videoFrame(Mode* m) {
    renderFrame(m);
    const u32* px = mockFramebuffer();
    for (int i = 0; i < Gfx::W * Gfx::H; i++) {
        u32 p = px[i];
        g_rgb[i * 3 + 0] = p & 0xff;
        g_rgb[i * 3 + 1] = (p >> 8) & 0xff;
        g_rgb[i * 3 + 2] = (p >> 16) & 0xff;
    }
    fwrite(g_rgb, 1, sizeof(g_rgb), g_pipe);
}

static HidTouchState mkTouch(u32 id, int x, int y, u32 dia) {
    HidTouchState t{};
    t.finger_id  = id;
    t.x = (u32)(x < 0 ? 0 : x);
    t.y = (u32)(y < 0 ? 0 : y);
    t.diameter_x = dia;
    t.diameter_y = dia;
    return t;
}

// ---------------------------------------------------------------------------
// Stills: one PPM per scene, plus two short animations.
// ---------------------------------------------------------------------------
static int generateStills() {
    if (system("mkdir -p build") != 0) return 1;

    {
        MenuMode m; m.onEnter();
        Input in{}; m.update(in);
        capture(&m, "menu");
    }
    {
        const char* names[] = {
            "display_01_solid",   "display_02_gradient", "display_03_geometry",
            "display_04_colorbars","display_05_subpixel","display_06_checker",
            "display_07_contrast","display_08_clipping", "display_09_retention",
            "display_10_strobe",  "display_11_motion",
        };
        DisplayMode m; m.onEnter();
        for (int p = 0; p < 11; p++) {
            if (p > 0) { Input in{}; in.down = HidNpadButton_AnyRight; m.update(in); }
            Input in{}; in.dtSec = kDt; m.update(in);
            capture(&m, names[p]);
        }
        for (int f = 0; f < 48; f++) {
            Input in{}; in.dtSec = kDt; m.update(in);
            char nm[64]; snprintf(nm, sizeof(nm), "anim_motion_%03d", f);
            capture(&m, nm);
        }
    }
    {
        TouchMode m; m.onEnter();
        Input in{}; in.dtSec = kDt;
        in.touch.count = 3;
        in.touch.touches[0] = mkTouch(0, 880, 300, 70);
        in.touch.touches[1] = mkTouch(1, 1040, 470, 90);
        in.touch.touches[2] = mkTouch(2, 760, 560, 60);
        m.update(in);
        capture(&m, "touch");
        for (int f = 0; f < 48; f++) {
            Input a{}; a.dtSec = kDt;
            a.touch.count = 2;
            a.touch.touches[0] = mkTouch(0, 300 + f * 13, 300 + f * 4, 80);
            a.touch.touches[1] = mkTouch(1, 980 - f * 11, 480 - f * 5, 100);
            m.update(a);
            char nm[64]; snprintf(nm, sizeof(nm), "anim_touch_%03d", f);
            capture(&m, nm);
        }
    }
    {
        GestureMode m; m.onEnter();
        HidGestureState g{};
        g.type = HidGestureType_Swipe;
        g.direction = 3;
        g.context_number = 1;
        g.x = 760; g.y = 380;
        g.delta_x = 52; g.velocity_x = 920.0f; g.scale = 1.0f;
        g.point_count = 1; g.points[0].x = 760; g.points[0].y = 380;
        mockSetGesture(&g);
        Input in{}; in.dtSec = kDt; m.update(in);
        capture(&m, "gesture");
    }
    {
        ControlsMode m; m.onEnter();
        HidSixAxisSensorState s{};
        s.attributes = HidSixAxisSensorAttribute_IsConnected;
        s.acceleration.x = 0.32f; s.acceleration.y = -0.18f; s.acceleration.z = 0.93f;
        s.angular_velocity.x = 0.12f; s.angular_velocity.z = -0.05f;
        mockSetSixAxis(&s);
        mockSetCapture(true);
        Input in{}; in.dtSec = kDt;
        in.held = HidNpadButton_A | HidNpadButton_ZR | HidNpadButton_Up;
        in.lstick.x = 19000; in.lstick.y = 9000;
        in.rstick.x = -7000; in.rstick.y = -23000;
        m.update(in);
        capture(&m, "controls");
    }
    {
        HwInfoMode m; m.onEnter();
        Input in{}; m.update(in);
        capture(&m, "hwinfo");
    }
    printf("done.\n");
    return 0;
}

// ---------------------------------------------------------------------------
// Video: a continuous scripted walk-through of every feature.
// ---------------------------------------------------------------------------
static void videoMenu() {
    MenuMode m; m.onEnter();
    for (int f = 0; f < 80; f++) {
        Input in{}; in.dtSec = kDt;
        if (f == 24 || f == 40 || f == 56) in.down = HidNpadButton_AnyDown;
        m.update(in);
        videoFrame(&m);
    }
}

static void videoDisplay() {
    DisplayMode m; m.onEnter();
    for (int p = 0; p < 11; p++) {
        for (int f = 0; f < 52; f++) {
            Input in{}; in.dtSec = kDt;
            if (f == 0 && p > 0) in.down = HidNpadButton_AnyRight;
            if (p == 0 && (f == 16 || f == 32)) in.down |= HidNpadButton_AnyDown;
            if (p == 10 && (f == 14 || f == 26)) in.down |= HidNpadButton_AnyUp;
            m.update(in);
            videoFrame(&m);
        }
    }
}

static void videoTouch() {
    TouchMode m; m.onEnter();
    u64 samp = 0;

    for (int f = 0; f < 120; f++) {                       // Live view
        Input in{}; in.dtSec = kDt;
        samp += 4;
        in.touch.sampling_number = samp;
        in.touch.count = 2 + f / 45;
        for (int i = 0; i < in.touch.count; i++)
            in.touch.touches[i] = mkTouch(i,
                340 + i * 240 + (int)(150 * sinf(f * 0.10f + i)),
                280 + i * 110 + (int)(90  * cosf(f * 0.12f + i)), 72 + i * 12);
        m.update(in);
        videoFrame(&m);
    }
    { Input in{}; in.dtSec = kDt; in.down = HidNpadButton_Y; m.update(in); }
    for (int f = 0; f < 120; f++) {                       // Paint view
        Input in{}; in.dtSec = kDt;
        in.touch.count = 1;
        in.touch.touches[0] = mkTouch(0, 110 + f * 9,
                                      360 + (int)(230 * sinf(f * 0.14f)), 60);
        m.update(in);
        videoFrame(&m);
    }
    { Input in{}; in.dtSec = kDt; in.down = HidNpadButton_Y; m.update(in); }
    for (int f = 0; f < 150; f++) {                       // Grid view
        Input in{}; in.dtSec = kDt;
        int cell = f < 144 ? f : 143;
        in.touch.count = 1;
        in.touch.touches[0] = mkTouch(0, (cell % 16) * 80 + 40,
                                      (cell / 16) * 80 + 40, 60);
        m.update(in);
        videoFrame(&m);
    }
}

static void videoGesture() {
    GestureMode m; m.onEnter();
    const u32 types[] = { HidGestureType_Tap, HidGestureType_Swipe,
                          HidGestureType_Pinch, HidGestureType_Rotate };
    const u32 dirs[]  = { 0, 3, 0, 0 };
    for (int s = 0; s < 4; s++) {
        for (int f = 0; f < 52; f++) {
            HidGestureState g{};
            bool active = f < 34;
            g.context_number = s + 1;
            g.type      = active ? types[s] : (u32)HidGestureType_Idle;
            g.direction = dirs[s];
            g.x = 720; g.y = 360;
            g.delta_x = 44; g.velocity_x = 780.0f;
            g.scale          = types[s] == HidGestureType_Pinch
                                   ? 1.0f + 0.5f * sinf(f * 0.2f) : 1.0f;
            g.rotation_angle = types[s] == HidGestureType_Rotate ? f * 4.0f : 0.0f;
            g.point_count = 1; g.points[0].x = 720; g.points[0].y = 360;
            mockSetGesture(&g);
            Input in{}; in.dtSec = kDt; m.update(in);
            videoFrame(&m);
        }
    }
}

static void videoControls() {
    ControlsMode m; m.onEnter();
    const u64 buttons[] = {
        HidNpadButton_A, HidNpadButton_B, HidNpadButton_X, HidNpadButton_Y,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right,
        HidNpadButton_L, HidNpadButton_R, HidNpadButton_ZL, HidNpadButton_ZR,
    };
    for (int f = 0; f < 270; f++) {
        Input in{}; in.dtSec = kDt;
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
        mockSetSixAxis(&s);
        mockSetCapture((f % 80) < 4);
        mockSetVolume(55 + (int)(25 * sinf(f * 0.07f)));

        m.update(in);
        videoFrame(&m);
    }
}

static void videoHwInfo() {
    HwInfoMode m; m.onEnter();
    for (int f = 0; f < 80; f++) {
        Input in{}; in.dtSec = kDt; m.update(in);
        videoFrame(&m);
    }
}

static int generateVideo() {
    if (system("command -v ffmpeg >/dev/null 2>&1") != 0) {
        printf("ffmpeg not found - install ffmpeg to render the video\n");
        return 1;
    }
    if (system("mkdir -p dist") != 0) return 1;

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -loglevel error -f rawvideo -pixel_format rgb24 "
        "-video_size %dx%d -framerate 30 -i - "
        "-c:v libx264 -pix_fmt yuv420p -crf 18 dist/showcase.mp4",
        Gfx::W, Gfx::H);
    g_pipe = popen(cmd, "w");
    if (!g_pipe) { printf("cannot start ffmpeg\n"); return 1; }

    printf("rendering feature walk-through...\n");
    videoMenu();
    videoDisplay();
    videoTouch();
    videoGesture();
    videoControls();
    videoHwInfo();

    int rc = pclose(g_pipe);
    g_pipe = nullptr;
    if (rc != 0) { printf("ffmpeg failed\n"); return 1; }
    printf("wrote dist/showcase.mp4\n");
    return 0;
}

int main(int argc, char** argv) {
    if (!g_gfx.init()) { printf("gfx init failed\n"); return 1; }
    int rc = (argc > 1 && std::strcmp(argv[1], "--video") == 0)
                 ? generateVideo()
                 : generateStills();
    g_gfx.deinit();
    return rc;
}
