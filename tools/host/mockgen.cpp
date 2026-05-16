// Off-device scene generator: drives each test mode with scripted input and
// writes the rendered frames as PPM images into out/. tools/host/gen-assets.sh
// turns those into the PNG/GIF assets used by the README.
#include <cstdio>
#include <cstring>
#include <cstdlib>
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

static const double kDt = 1.0 / 60.0;
static Gfx g_gfx;

static void writePPM(const char* name, const u32* px) {
    char path[256];
    snprintf(path, sizeof(path), "out/%s.ppm", name);
    FILE* f = fopen(path, "wb");
    if (!f) { printf("  cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", Gfx::W, Gfx::H);
    for (int i = 0; i < Gfx::W * Gfx::H; i++) {
        u32 p = px[i];
        unsigned char rgb[3] = { (unsigned char)(p & 0xff),
                                 (unsigned char)((p >> 8) & 0xff),
                                 (unsigned char)((p >> 16) & 0xff) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("  out/%s.ppm\n", name);
}

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

static void capture(Mode* m, const char* name) {
    g_gfx.beginFrame();
    m->render(g_gfx);
    if (m->showChrome())
        drawChrome(m);
    m->renderOverlay(g_gfx);
    g_gfx.endFrame();
    writePPM(name, mockFramebuffer());
}

static HidTouchState mkTouch(u32 id, u32 x, u32 y, u32 dia) {
    HidTouchState t{};
    t.finger_id  = id;
    t.x = x; t.y = y;
    t.diameter_x = dia; t.diameter_y = dia;
    return t;
}

int main() {
    if (system("mkdir -p out") != 0) return 1;
    if (!g_gfx.init()) { printf("gfx init failed\n"); return 1; }

    // --- menu ---
    {
        MenuMode m; m.onEnter();
        Input in{}; m.update(in);
        capture(&m, "menu");
    }

    // --- display patterns (stills) + the moving-box pattern as an animation ---
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
        // m is now on the motion pattern: let the box travel.
        for (int f = 0; f < 48; f++) {
            Input in{}; in.dtSec = kDt; m.update(in);
            char nm[64]; snprintf(nm, sizeof(nm), "anim_motion_%03d", f);
            capture(&m, nm);
        }
    }

    // --- touchscreen (still + sweeping animation) ---
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

    // --- gesture ---
    {
        GestureMode m; m.onEnter();
        HidGestureState g{};
        g.type = HidGestureType_Swipe;
        g.direction = 3;            // Right
        g.context_number = 1;
        g.x = 760; g.y = 380;
        g.delta_x = 52; g.delta_y = 6;
        g.velocity_x = 920.0f; g.velocity_y = 40.0f;
        g.scale = 1.0f;
        g.point_count = 1;
        g.points[0].x = 760; g.points[0].y = 380;
        mockSetGesture(&g);
        Input in{}; in.dtSec = kDt; m.update(in);
        capture(&m, "gesture");
    }

    // --- controls ---
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

    // --- system info ---
    {
        HwInfoMode m; m.onEnter();
        Input in{}; m.update(in);
        capture(&m, "hwinfo");
    }

    g_gfx.deinit();
    printf("done.\n");
    return 0;
}
