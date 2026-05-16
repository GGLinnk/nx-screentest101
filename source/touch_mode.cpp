#include "touch_mode.hpp"
#include "gfx.hpp"
#include <cstdio>
#include <cstring>

namespace {

// Persistent paint canvas (full screen). ~3.6 MB of BSS.
u32 s_canvas[Gfx::W * Gfx::H];

const u32 kCanvasBg = 0xFF0C0E14; // dark, packed RGBA8888

struct RGB { u8 r, g, b; };
const RGB kFinger[16] = {
    {255, 64, 64}, {255,148, 32}, {240,224, 32}, {140,230, 40},
    { 32,210, 90}, { 32,210,190}, { 64,200,255}, { 64,140,255},
    { 90, 90,255}, {150, 90,255}, {200, 80,240}, {255, 80,200},
    {255,120,160}, {200,140, 90}, {235,235,235}, {150,160,175},
};

u32 fingerColor(u32 id) {
    const RGB& c = kFinger[id & 15];
    return Gfx::rgb(c.r, c.g, c.b);
}

// Full-saturation rainbow colour; hue in turns ([0,1) completes one cycle).
u32 rainbow(float h) {
    h -= __builtin_floorf(h);
    float x = h * 6.0f;
    int   i = (int)x;
    float f = x - (float)i;
    float r, g, b;
    switch (i % 6) {
        case 0:  r = 1;     g = f;     b = 0;     break;
        case 1:  r = 1 - f; g = 1;     b = 0;     break;
        case 2:  r = 0;     g = 1;     b = f;     break;
        case 3:  r = 0;     g = 1 - f; b = 1;     break;
        case 4:  r = f;     g = 0;     b = 1;     break;
        default: r = 1;     g = 0;     b = 1 - f; break;
    }
    return Gfx::rgb((u8)(r * 255), (u8)(g * 255), (u8)(b * 255));
}

// Filled disc rasterised straight into the persistent canvas buffer.
void canvasDisc(int cx, int cy, int r, u32 color) {
    if (r < 1) r = 1;
    for (int dy = -r; dy <= r; dy++) {
        int yy = cy + dy;
        if (yy < 0 || yy >= Gfx::H) continue;
        int span = (int)__builtin_sqrtf((float)(r * r - dy * dy));
        int x0 = cx - span, x1 = cx + span;
        if (x0 < 0) x0 = 0;
        if (x1 > Gfx::W - 1) x1 = Gfx::W - 1;
        u32* row = s_canvas + yy * Gfx::W;
        for (int xx = x0; xx <= x1; xx++) row[xx] = color;
    }
}

// Reset the whole paint canvas to the background colour.
void clearCanvas() {
    for (int i = 0; i < Gfx::W * Gfx::H; i++) s_canvas[i] = kCanvasBg;
}

} // namespace

void TouchMode::onEnter() {
    view_ = VIEW_LIVE;
    clearCanvas();
    clearGrid();
    resetStats();
}

void TouchMode::resetStats() {
    maxFingers_   = 0;
    haveSampling_ = false;
    prevSampling_ = 0;
    rateWindow_   = 0.0;
    rateSamples_  = 0;
    curHz_ = minHz_ = maxHz_ = 0.0;
    lastDeltaTime_ = 0;
    jitN_ = jitSumX_ = jitSumY_ = jitSumXX_ = jitSumYY_ = 0.0;
    jitterPx_ = 0.0;
}

void TouchMode::clearGrid() {
    for (bool& cell : gridHit_) cell = false;
}

void TouchMode::update(const Input& in) {
    touch_ = in.touch;

    if (touch_.count > maxFingers_)
        maxFingers_ = touch_.count;
    if (touch_.count > 0)
        lastDeltaTime_ = touch_.touches[0].delta_time;

    if (in.down & HidNpadButton_Y) view_ = (view_ + 1) % VIEW_COUNT;
    if (in.down & HidNpadButton_X) resetStats();
    if (in.down & HidNpadButton_A) {
        if (view_ == VIEW_PAINT) clearCanvas();
        else if (view_ == VIEW_GRID) clearGrid();
    }

    // Advance the rainbow brush so the paint trail varies in colour, making
    // overlapping passes and coverage gaps easier to distinguish.
    paintHue_ += in.dtSec * 0.45;
    while (paintHue_ >= 1.0) paintHue_ -= 1.0;

    if (view_ == VIEW_PAINT) {
        // Accumulate paint while a finger is down (movement leaves a trail).
        for (int i = 0; i < touch_.count; i++) {
            const HidTouchState& t = touch_.touches[i];
            int d = (int)((t.diameter_x + t.diameter_y) / 2);
            int r = d / 2;
            if (r < 6) r = 6;
            canvasDisc((int)t.x, (int)t.y, r,
                       rainbow((float)paintHue_ + (float)t.finger_id * 0.13f));
        }
    } else if (view_ == VIEW_GRID) {
        // Light up every grid cell a contact passes through.
        for (int i = 0; i < touch_.count; i++) {
            int c = (int)touch_.touches[i].x / (Gfx::W / GRID_COLS);
            int r = (int)touch_.touches[i].y / (Gfx::H / GRID_ROWS);
            if (c >= 0 && c < GRID_COLS && r >= 0 && r < GRID_ROWS)
                gridHit_[r * GRID_COLS + c] = true;
        }
    }

    // Single-finger jitter: spread of the contact point while exactly one
    // finger rests on the panel (cumulative variance over the contact).
    if (touch_.count == 1) {
        double x = touch_.touches[0].x, y = touch_.touches[0].y;
        jitN_     += 1.0;
        jitSumX_  += x;      jitSumY_  += y;
        jitSumXX_ += x * x;  jitSumYY_ += y * y;
        if (jitN_ >= 8.0) {
            double mx = jitSumX_ / jitN_, my = jitSumY_ / jitN_;
            double vx = jitSumXX_ / jitN_ - mx * mx;
            double vy = jitSumYY_ / jitN_ - my * my;
            if (vx < 0.0) vx = 0.0;
            if (vy < 0.0) vy = 0.0;
            jitterPx_ = __builtin_sqrt(vx + vy);
        }
    } else {
        jitN_ = jitSumX_ = jitSumY_ = jitSumXX_ = jitSumYY_ = 0.0;
        jitterPx_ = 0.0;
    }

    // Touch report-rate measurement from the HID sampling counter.
    u64 s = touch_.sampling_number;
    if (haveSampling_) {
        u64 delta = (s >= prevSampling_) ? (s - prevSampling_) : 0;
        rateSamples_ += delta;
        rateWindow_  += in.dtSec;
        if (rateWindow_ >= 0.5) {
            curHz_ = (double)rateSamples_ / rateWindow_;
            if (curHz_ > maxHz_) maxHz_ = curHz_;
            if (curHz_ > 0.0 && (minHz_ == 0.0 || curHz_ < minHz_)) minHz_ = curHz_;
            rateSamples_ = 0;
            rateWindow_  = 0.0;
        }
    }
    prevSampling_ = s;
    haveSampling_ = true;
}

const char* TouchMode::controls() const {
    if (view_ == VIEW_PAINT)
        return "A Clear canvas   X Reset stats   Y Grid view   B Menu";
    if (view_ == VIEW_GRID)
        return "A Reset grid   X Reset stats   Y Live view   B Menu";
    return "X Reset stats   Y Paint canvas   B Menu";
}

void TouchMode::render(Gfx& g) {
    const u32 white  = Gfx::rgb(255, 255, 255);
    const u32 dim    = Gfx::rgb(150, 156, 178);
    const u32 panel  = Gfx::rgb(22, 26, 38);
    const u32 shadow = Gfx::rgb(0, 0, 0);
    char line[96];

    if (view_ == VIEW_PAINT) {
        g.blitFull(s_canvas);
        g.drawTextShadow(12, 40, 2, white, shadow,
                         "PAINT CANVAS - sweep every part of the panel");
        g.drawTextShadow(12, 62, 2, dim, shadow,
                         "uncoloured areas reveal dead / insensitive zones");
        return;
    }

    if (view_ == VIEW_GRID) {
        const int cw = Gfx::W / GRID_COLS, ch = Gfx::H / GRID_ROWS;
        int hits = 0;
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                bool hit = gridHit_[r * GRID_COLS + c];
                if (hit) hits++;
                g.fillRect(c * cw, r * ch, cw, ch,
                           hit ? Gfx::rgb(38, 132, 74) : Gfx::rgb(20, 23, 32));
                g.drawRect(c * cw, r * ch, cw, ch, Gfx::rgb(46, 52, 70));
            }
        }
        snprintf(line, sizeof(line), "GRID COVERAGE   %d / %d cells",
                 hits, GRID_COLS * GRID_ROWS);
        g.drawTextShadow(12, 40, 2, white, shadow, line);
        g.drawTextShadow(12, 62, 2, dim, shadow,
                         "tap every cell to map the full digitizer");
        return;
    }

    // --- VIEW_LIVE ---
    g.clear(Gfx::rgb(12, 14, 20));
    const u32 grid = Gfx::rgb(26, 30, 42);
    for (int x = 0; x < Gfx::W; x += 80) g.vLine(x, 0, Gfx::H, grid);
    for (int y = 0; y < Gfx::H; y += 80) g.hLine(0, y, Gfx::W, grid);

    // Left panel: per-finger properties.
    const int px = 8, py = 38, pw = 620;
    int rows = 5 + (touch_.count > 0 ? touch_.count : 1);
    int ph = 16 + rows * 22;
    g.fillRect(px, py, pw, ph, panel);

    int tx = px + 12, ty = py + 10;
    snprintf(line, sizeof(line), "Active touches: %d", touch_.count);
    g.drawText(tx, ty, 2, white, line); ty += 24;
    snprintf(line, sizeof(line), "Max simultaneous: %d / 16", maxFingers_);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Primary delta_time: %.2f ms",
             (double)lastDeltaTime_ / 1.0e6);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Single-finger jitter: %.2f px", jitterPx_);
    g.drawText(tx, ty, 2, dim, line); ty += 28;

    g.drawText(tx, ty, 2, Gfx::rgb(120,180,255),
               "id  x     y     dia     rot    state");
    ty += 24;

    if (touch_.count == 0) {
        g.drawText(tx, ty, 2, dim, "-- touch the screen --");
    } else {
        for (int i = 0; i < touch_.count; i++) {
            const HidTouchState& t = touch_.touches[i];
            // Start/End are one-frame events; in between a finger is "HELD".
            const char* flag = (t.attributes & HidTouchAttribute_Start) ? "START"
                             : (t.attributes & HidTouchAttribute_End)   ? "END"
                             : "HELD";
            // rotation_angle is u32 in libnx but the value is signed:
            // negative angles wrap to huge values, so read it back as s32.
            snprintf(line, sizeof(line), "%-3lu %4u  %4u  %3ux%-3u %5d  %-5s",
                     (unsigned long)t.finger_id, t.x, t.y,
                     t.diameter_x, t.diameter_y,
                     (int)(s32)t.rotation_angle, flag);
            g.drawText(tx, ty, 2, fingerColor(t.finger_id), line);
            ty += 22;
        }
    }

    // Report-rate panel.
    const int fx = 8, fy = py + ph + 12, fw = 620, fh = 130;
    g.fillRect(fx, fy, fw, fh, panel);
    int rx = fx + 12, ry = fy + 10;
    g.drawText(rx, ry, 2, Gfx::rgb(120,180,255), "TOUCH REPORT RATE"); ry += 26;
    snprintf(line, sizeof(line), "Current: %6.1f Hz", curHz_);
    g.drawText(rx, ry, 3, white, line); ry += 30;
    snprintf(line, sizeof(line), "Min %5.1f   Max %5.1f Hz", minHz_, maxHz_);
    g.drawText(rx, ry, 2, dim, line); ry += 24;
    snprintf(line, sizeof(line), "sampling_number: %lu",
             (unsigned long)touch_.sampling_number);
    g.drawText(rx, ry, 2, dim, line);
}

// Live touch markers are drawn here, after the App chrome, so the touch
// "bubbles" sit on top of the header and footer bars too.
void TouchMode::renderOverlay(Gfx& g) {
    const u32 white = Gfx::rgb(255, 255, 255);
    for (int i = 0; i < touch_.count; i++) {
        const HidTouchState& t = touch_.touches[i];
        u32 col = fingerColor(t.finger_id);
        int cx = (int)t.x, cy = (int)t.y;
        int ring = (int)((t.diameter_x + t.diameter_y) / 4);
        if (ring < 14) ring = 14;

        g.drawCircle(cx, cy, ring, col);
        g.drawCircle(cx, cy, ring + 1, col);
        g.fillCircle(cx, cy, 6, col);
        g.drawCross(cx, cy, 34, col);

        // Keep the finger label on-screen even near the right edge.
        char tag[16];
        snprintf(tag, sizeof(tag), "F%lu", (unsigned long)t.finger_id);
        int lx = cx + ring + 6;
        if (lx + 4 * 8 > Gfx::W - 8) lx = cx - ring - 6 - 4 * 8;
        g.drawTextBg(lx, cy - 8, 2, white, Gfx::rgb(0, 0, 0), tag);
    }
}
