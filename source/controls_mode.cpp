#include "controls_mode.hpp"
#include "gfx.hpp"
#include <cstdio>
#include <iterator>

namespace {

// Button pills, drawn as a 4x4 grid; each lights up while its button is held.
struct Btn { const char* label; u64 mask; };
const Btn kButtons[] = {
    {"A", HidNpadButton_A},           {"B", HidNpadButton_B},
    {"X", HidNpadButton_X},           {"Y", HidNpadButton_Y},
    {"Up", HidNpadButton_Up},         {"Down", HidNpadButton_Down},
    {"Left", HidNpadButton_Left},     {"Right", HidNpadButton_Right},
    {"L", HidNpadButton_L},           {"R", HidNpadButton_R},
    {"ZL", HidNpadButton_ZL},         {"ZR", HidNpadButton_ZR},
    {"+", HidNpadButton_Plus},        {"-", HidNpadButton_Minus},
    {"LStick", HidNpadButton_StickL}, {"RStick", HidNpadButton_StickR},
};
constexpr int kButtonCount = (int)std::size(kButtons);

// Vibration presets: silent, and a two-band buzz for the rumble test.
const HidVibrationValue kSilent = { 0.0f, 160.0f, 0.0f, 320.0f };
const HidVibrationValue kBuzz   = { 0.6f, 160.0f, 0.6f, 320.0f };

constexpr s32 kStickMax = 32767;   // analog-stick full deflection

// Analog-stick visualiser: travel circle, deadzone ring, drift box (the
// min/max excursion seen so far) and a live dot.
void drawStick(Gfx& g, int bx, int by, const HidAnalogStickState& s,
               s32 minX, s32 maxX, s32 minY, s32 maxY) {
    const int SZ = 220, R = 100;
    int cx = bx + SZ / 2, cy = by + SZ / 2;

    g.fillRect(bx, by, SZ, SZ, Gfx::rgb(22, 26, 38));
    g.drawRect(bx, by, SZ, SZ, Gfx::rgb(46, 52, 70));
    g.hLine(bx, cy, SZ, Gfx::rgb(38, 43, 58));
    g.vLine(cx, by, SZ, Gfx::rgb(38, 43, 58));
    g.drawCircle(cx, cy, R, Gfx::rgb(46, 52, 70));
    g.drawCircle(cx, cy, R * 15 / 100, Gfx::rgb(70, 76, 98));   // deadzone

    // Drift box: stick Y is positive-up, screen Y grows down.
    int ex0 = cx + minX * R / kStickMax, ex1 = cx + maxX * R / kStickMax;
    int ey0 = cy - maxY * R / kStickMax, ey1 = cy - minY * R / kStickMax;
    g.drawRect(ex0, ey0, ex1 - ex0 + 1, ey1 - ey0 + 1, Gfx::rgb(200, 160, 60));

    int dx = cx + s.x * R / kStickMax, dy = cy - s.y * R / kStickMax;
    g.fillCircle(dx, dy, 7, Gfx::rgb(120, 180, 255));
}

} // namespace

void ControlsMode::onEnter() {
    held_ = 0;
    lstick_ = rstick_ = HidAnalogStickState{};
    lMinX_ = lMaxX_ = lMinY_ = lMaxY_ = 0;
    rMinX_ = rMaxX_ = rMinY_ = rMaxY_ = 0;
    six_ = HidSixAxisSensorState{};

    hidGetSixAxisSensorHandles(&sixHandheld_, 1, HidNpadIdType_Handheld,
                               HidNpadStyleTag_NpadHandheld);
    hidGetSixAxisSensorHandles(&sixFullKey_, 1, HidNpadIdType_No1,
                               HidNpadStyleTag_NpadFullKey);
    hidGetSixAxisSensorHandles(sixJoyDual_, 2, HidNpadIdType_No1,
                               HidNpadStyleTag_NpadJoyDual);
    hidStartSixAxisSensor(sixHandheld_);
    hidStartSixAxisSensor(sixFullKey_);
    hidStartSixAxisSensor(sixJoyDual_[0]);
    hidStartSixAxisSensor(sixJoyDual_[1]);

    hidInitializeVibrationDevices(vibHandheld_, 2, HidNpadIdType_Handheld,
                                  HidNpadStyleTag_NpadHandheld);
    hidInitializeVibrationDevices(vibFullKey_, 2, HidNpadIdType_No1,
                                  HidNpadStyleTag_NpadFullKey);
    hidInitializeVibrationDevices(vibJoyDual_, 2, HidNpadIdType_No1,
                                  HidNpadStyleTag_NpadJoyDual);
}

void ControlsMode::onExit() {
    HidVibrationValue off[2] = { kSilent, kSilent };
    hidSendVibrationValues(vibHandheld_, off, 2);
    hidSendVibrationValues(vibFullKey_, off, 2);
    hidSendVibrationValues(vibJoyDual_, off, 2);

    hidStopSixAxisSensor(sixHandheld_);
    hidStopSixAxisSensor(sixFullKey_);
    hidStopSixAxisSensor(sixJoyDual_[0]);
    hidStopSixAxisSensor(sixJoyDual_[1]);
}

void ControlsMode::update(const Input& in) {
    held_   = in.held;
    lstick_ = in.lstick;
    rstick_ = in.rstick;

    // Grow each stick's drift box to cover every position seen.
    if (lstick_.x < lMinX_) lMinX_ = lstick_.x;
    if (lstick_.x > lMaxX_) lMaxX_ = lstick_.x;
    if (lstick_.y < lMinY_) lMinY_ = lstick_.y;
    if (lstick_.y > lMaxY_) lMaxY_ = lstick_.y;
    if (rstick_.x < rMinX_) rMinX_ = rstick_.x;
    if (rstick_.x > rMaxX_) rMaxX_ = rstick_.x;
    if (rstick_.y < rMinY_) rMinY_ = rstick_.y;
    if (rstick_.y > rMaxY_) rMaxY_ = rstick_.y;

    // Select the sensor / rumble device set for the connected controller.
    HidSixAxisSensorHandle  motion = sixHandheld_;
    HidVibrationDeviceHandle* vib  = vibHandheld_;
    if (!(hidGetNpadStyleSet(HidNpadIdType_Handheld) & HidNpadStyleTag_NpadHandheld)) {
        u32 s1 = hidGetNpadStyleSet(HidNpadIdType_No1);
        if (s1 & HidNpadStyleTag_NpadFullKey)      { motion = sixFullKey_;    vib = vibFullKey_; }
        else if (s1 & HidNpadStyleTag_NpadJoyDual) { motion = sixJoyDual_[0]; vib = vibJoyDual_; }
    }

    HidSixAxisSensorState st{};
    if (hidGetSixAxisSensorStates(motion, &st, 1) > 0)
        six_ = st;

    // HD rumble: ZL drives the left actuator, ZR the right.
    HidVibrationValue vals[2];
    vals[0] = (held_ & HidNpadButton_ZL) ? kBuzz : kSilent;
    vals[1] = (held_ & HidNpadButton_ZR) ? kBuzz : kSilent;
    hidSendVibrationValues(vib, vals, 2);
}

void ControlsMode::render(Gfx& g) {
    const u32 white  = Gfx::rgb(236, 238, 248);
    const u32 dim    = Gfx::rgb(150, 156, 178);
    const u32 accent = Gfx::rgb(120, 180, 255);
    const u32 panel  = Gfx::rgb(22, 26, 38);
    const u32 lit    = Gfx::rgb(70, 200, 110);
    char line[96];

    g.clear(Gfx::rgb(12, 14, 20));

    // --- analog sticks ---
    g.drawText(48,  42, 2, accent, "L STICK");
    g.drawText(296, 42, 2, accent, "R STICK");
    drawStick(g, 48,  64, lstick_, lMinX_, lMaxX_, lMinY_, lMaxY_);
    drawStick(g, 296, 64, rstick_, rMinX_, rMaxX_, rMinY_, rMaxY_);
    snprintf(line, sizeof(line), "%6d,%6d", lstick_.x, lstick_.y);
    g.drawText(48,  292, 2, dim, line);
    snprintf(line, sizeof(line), "%6d,%6d", rstick_.x, rstick_.y);
    g.drawText(296, 292, 2, dim, line);

    // --- buttons ---
    g.drawText(560, 42, 2, accent, "BUTTONS");
    const int BX = 560, BY = 64, BW = 168, BH = 46, GAP = 12;
    for (int i = 0; i < kButtonCount; i++) {
        int x = BX + (i % 4) * (BW + GAP);
        int y = BY + (i / 4) * (BH + GAP);
        bool on = held_ & kButtons[i].mask;
        g.fillRect(x, y, BW, BH, on ? lit : panel);
        g.drawRect(x, y, BW, BH, on ? Gfx::rgb(120, 220, 150) : Gfx::rgb(46, 52, 70));
        int tw = g.textWidth(2, kButtons[i].label);
        g.drawText(x + (BW - tw) / 2, y + 15, 2,
                   on ? Gfx::rgb(10, 22, 12) : dim, kButtons[i].label);
    }

    // --- motion sensor + rumble panel ---
    const int mx = 48, my = 330, mw = 1188, mh = 338;
    g.fillRect(mx, my, mw, mh, panel);
    int tx = mx + 22, ty = my + 18;
    g.drawText(tx, ty, 2, accent, "MOTION SENSOR"); ty += 34;
    snprintf(line, sizeof(line), "Gyro    X %8.2f   Y %8.2f   Z %8.2f   rot/s",
             six_.angular_velocity.x, six_.angular_velocity.y, six_.angular_velocity.z);
    g.drawText(tx, ty, 2, white, line); ty += 28;
    snprintf(line, sizeof(line), "Accel   X %8.2f   Y %8.2f   Z %8.2f   g",
             six_.acceleration.x, six_.acceleration.y, six_.acceleration.z);
    g.drawText(tx, ty, 2, white, line); ty += 52;

    g.drawText(tx, ty, 2, accent, "HD RUMBLE"); ty += 34;
    g.drawText(tx, ty, 2, dim, "Hold ZL / ZR to vibrate the actuators"); ty += 28;
    snprintf(line, sizeof(line), "ZL %s     ZR %s",
             (held_ & HidNpadButton_ZL) ? "BUZZ" : "off",
             (held_ & HidNpadButton_ZR) ? "BUZZ" : "off");
    g.drawText(tx, ty, 2, white, line);

    // Tilt bubble: accelerometer X/Y projected into a circle.
    int bx = mx + mw - 220, by = my + mh / 2;
    int br = 130;
    g.drawCircle(bx, by, br, Gfx::rgb(46, 52, 70));
    g.drawCircle(bx, by, br / 2, Gfx::rgb(38, 43, 58));
    g.drawCross(bx, by, 10, Gfx::rgb(70, 76, 98));
    float ax = six_.acceleration.x, ay = six_.acceleration.y;
    if (ax < -1.0f) ax = -1.0f; else if (ax > 1.0f) ax = 1.0f;
    if (ay < -1.0f) ay = -1.0f; else if (ay > 1.0f) ay = 1.0f;
    g.fillCircle(bx + (int)(ax * br), by - (int)(ay * br), 10, accent);
    g.drawText(bx - g.textWidth(2, "TILT") / 2, by - br - 24, 2, accent, "TILT");
}
