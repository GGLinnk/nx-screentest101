#include "display_mode.hpp"
#include "gfx.hpp"
#include <cstdio>
#include <iterator>

namespace {

enum Pattern {
    P_SOLID = 0, P_GRADIENT, P_GEOMETRY, P_COLORBARS,
    P_CHECKER, P_CONTRAST, P_STROBE, P_COUNT
};

const char* kPatternName[P_COUNT] = {
    "Solid Colour", "Gradients", "Geometry Grid", "Colour Bars",
    "Pixel Checkerboard", "Contrast Steps", "Strobe / Flicker",
};

struct NamedColor { const char* n; u8 r, g, b; };
const NamedColor kSolids[] = {
    {"RED",      255,   0,   0}, {"GREEN",      0, 255,   0},
    {"BLUE",       0,   0, 255}, {"WHITE",    255, 255, 255},
    {"BLACK",      0,   0,   0}, {"GREY 50%", 128, 128, 128},
    {"CYAN",       0, 255, 255}, {"MAGENTA",  255,   0, 255},
    {"YELLOW",   255, 255,   0},
};
constexpr int kSolidCount = (int)std::size(kSolids);

} // namespace

void DisplayMode::onEnter() {
    pattern_ = P_SOLID;
    solidIdx_ = 0;
    strobeAcc_ = 0.0;
    strobeWhite_ = false;
}

void DisplayMode::update(const Input& in) {
    if (in.down & HidNpadButton_AnyRight)
        pattern_ = (pattern_ + 1) % P_COUNT;
    if (in.down & HidNpadButton_AnyLeft)
        pattern_ = (pattern_ + P_COUNT - 1) % P_COUNT;

    if (in.down & HidNpadButton_Y)
        hud_ = !hud_;

    if (pattern_ == P_SOLID) {
        if (in.down & (HidNpadButton_AnyDown | HidNpadButton_A))
            solidIdx_ = (solidIdx_ + 1) % kSolidCount;
        if (in.down & HidNpadButton_AnyUp)
            solidIdx_ = (solidIdx_ + kSolidCount - 1) % kSolidCount;
    } else if (pattern_ == P_STROBE) {
        if (in.down & HidNpadButton_AnyUp)   strobeHz_ = strobeHz_ < 30 ? strobeHz_ + 1 : 30;
        if (in.down & HidNpadButton_AnyDown) strobeHz_ = strobeHz_ >  1 ? strobeHz_ - 1 :  1;

        strobeAcc_ += in.dtSec;
        double halfPeriod = 0.5 / (double)strobeHz_;
        while (strobeAcc_ >= halfPeriod) {
            strobeAcc_ -= halfPeriod;
            strobeWhite_ = !strobeWhite_;
        }
    }
}

const char* DisplayMode::controls() const {
    if (pattern_ == P_SOLID)
        return "<-/-> Pattern   Up/Down/A Colour   Y HUD   B Menu";
    if (pattern_ == P_STROBE)
        return "<-/-> Pattern   Up/Down Rate   Y HUD   B Menu";
    return "<-/-> Pattern   Y HUD   B Menu";
}

void DisplayMode::render(Gfx& g) {
    switch (pattern_) {
    case P_SOLID: {
        const NamedColor& c = kSolids[solidIdx_];
        g.clear(Gfx::rgb(c.r, c.g, c.b));
        break;
    }
    case P_GRADIENT: {
        // Four horizontal ramps: grey, red, green, blue (banding check).
        int band = Gfx::H / 4;
        for (int x = 0; x < Gfx::W; x++) {
            u8 v = (u8)(x * 255 / (Gfx::W - 1));
            g.vLine(x, 0,        band, Gfx::rgb(v, v, v));
            g.vLine(x, band,     band, Gfx::rgb(v, 0, 0));
            g.vLine(x, band * 2, band, Gfx::rgb(0, v, 0));
            g.vLine(x, band * 3, Gfx::H - band * 3, Gfx::rgb(0, 0, v));
        }
        break;
    }
    case P_GEOMETRY: {
        const u32 line = Gfx::rgb(255, 255, 255);
        g.clear(Gfx::rgb(0, 0, 0));
        for (int x = 0; x <= Gfx::W; x += 64) g.vLine(x >= Gfx::W ? Gfx::W - 1 : x, 0, Gfx::H, line);
        for (int y = 0; y <= Gfx::H; y += 64) g.hLine(0, y >= Gfx::H ? Gfx::H - 1 : y, Gfx::W, line);
        g.drawRectThick(0, 0, Gfx::W, Gfx::H, 2, Gfx::rgb(255, 64, 64));
        g.drawLine(0, 0, Gfx::W - 1, Gfx::H - 1, Gfx::rgb(64, 255, 64));
        g.drawLine(Gfx::W - 1, 0, 0, Gfx::H - 1, Gfx::rgb(64, 255, 64));
        int cx = Gfx::W / 2, cy = Gfx::H / 2;
        for (int r = 60; r < 380; r += 60) g.drawCircle(cx, cy, r, Gfx::rgb(255, 255, 0));
        g.drawCross(cx, cy, 40, Gfx::rgb(0, 200, 255));
        break;
    }
    case P_COLORBARS: {
        const u32 bars[8] = {
            Gfx::rgb(255,255,255), Gfx::rgb(255,255,0),
            Gfx::rgb(0,255,255),   Gfx::rgb(0,255,0),
            Gfx::rgb(255,0,255),   Gfx::rgb(255,0,0),
            Gfx::rgb(0,0,255),     Gfx::rgb(0,0,0),
        };
        int bw = Gfx::W / 8;
        for (int i = 0; i < 8; i++)
            g.fillRect(i * bw, 0, (i == 7 ? Gfx::W - 7 * bw : bw), Gfx::H, bars[i]);
        break;
    }
    case P_CHECKER: {
        // 1px alternating black/white grid (pixel response / panel inversion).
        for (int y = 0; y < Gfx::H; y++)
            for (int x = 0; x < Gfx::W; x++)
                g.putPixel(x, y, ((x ^ y) & 1) ? Gfx::rgb(255,255,255) : Gfx::rgb(0,0,0));
        break;
    }
    case P_CONTRAST: {
        const int steps = 16;
        int sw = Gfx::W / steps;
        for (int i = 0; i < steps; i++) {
            u8 v = (u8)(i * 255 / (steps - 1));
            g.fillRect(i * sw, 0, (i == steps - 1 ? Gfx::W - i * sw : sw), Gfx::H,
                       Gfx::rgb(v, v, v));
        }
        break;
    }
    case P_STROBE:
        g.clear(strobeWhite_ ? Gfx::rgb(255,255,255) : Gfx::rgb(0,0,0));
        break;
    }

    if (!hud_) return;

    // On-screen overlay (drawn below the App's top chrome bar).
    char line[96];
    snprintf(line, sizeof(line), "%d/%d  %s", pattern_ + 1, P_COUNT, kPatternName[pattern_]);
    g.drawTextBg(14, 44, 2, Gfx::rgb(255,255,255), Gfx::rgb(0,0,0), line);

    if (pattern_ == P_SOLID) {
        snprintf(line, sizeof(line), "Colour: %s  (%d/%d)",
                 kSolids[solidIdx_].n, solidIdx_ + 1, kSolidCount);
        g.drawTextBg(14, 76, 2, Gfx::rgb(255,255,255), Gfx::rgb(0,0,0), line);
    } else if (pattern_ == P_STROBE) {
        snprintf(line, sizeof(line), "Rate: %d Hz", strobeHz_);
        g.drawTextBg(14, 76, 2, Gfx::rgb(255,255,255), Gfx::rgb(0,0,0), line);
    }
}
