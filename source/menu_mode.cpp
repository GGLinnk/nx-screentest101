#include "menu_mode.hpp"
#include "gfx.hpp"
#include <iterator>

namespace {

struct Item { const char* label; const char* desc; ModeId target; };

const Item kItems[] = {
    {"Display Test",     "Solid colours, gradients, geometry, flicker",  ModeId::Display},
    {"Touchscreen Test", "Multitouch, report rate, dead-zone canvas",    ModeId::Touch},
    {"Gesture Test",     "Tap, swipe, pinch and rotate recognition",     ModeId::Gesture},
    {"Controls Test",    "Buttons, stick drift, motion sensor, rumble",  ModeId::Controls},
};
constexpr int kCount = (int)std::size(kItems);

// Cards span a wide centred column so descriptions and the footer hint fit.
constexpr int IT_X = 160, IT_W = 960, IT_H = 84, IT_GAP = 20, IT_Y0 = 252;

void itemRect(int i, int& x, int& y, int& w, int& h) {
    x = IT_X; w = IT_W; h = IT_H;
    y = IT_Y0 + i * (IT_H + IT_GAP);
}

bool inRect(int px, int py, int x, int y, int w, int h) {
    return px >= x && py >= y && px < x + w && py < y + h;
}

} // namespace

void MenuMode::onEnter() { sel_ = 0; }

void MenuMode::update(const Input& in) {
    if (in.down & HidNpadButton_AnyDown)
        sel_ = (sel_ + 1) % kCount;
    if (in.down & HidNpadButton_AnyUp)
        sel_ = (sel_ + kCount - 1) % kCount;

    if (in.down & HidNpadButton_A)
        requestSwitch(kItems[sel_].target);

    // Touch: hovering highlights, a new contact (Start) on an item selects it.
    for (int i = 0; i < in.touch.count; i++) {
        int x, y, w, h;
        const HidTouchState& t = in.touch.touches[i];
        for (int k = 0; k < kCount; k++) {
            itemRect(k, x, y, w, h);
            if (inRect((int)t.x, (int)t.y, x, y, w, h)) {
                sel_ = k;
                if (t.attributes & HidTouchAttribute_Start)
                    requestSwitch(kItems[k].target);
            }
        }
    }
}

void MenuMode::render(Gfx& g) {
    const u32 bg      = Gfx::rgb(14, 16, 24);
    const u32 fg      = Gfx::rgb(236, 238, 248);
    const u32 dim     = Gfx::rgb(150, 156, 178);
    const u32 accent  = Gfx::rgb(120, 180, 255);
    const u32 cardBg  = Gfx::rgb(30, 34, 48);
    const u32 cardSel = Gfx::rgb(48, 84, 140);

    g.clear(bg);

    // Title and subtitle, horizontally centred.
    const char* title = "SCREEN TESTER 101";
    const char* sub   = "Nintendo Switch display & touchscreen diagnostics";
    g.drawText((Gfx::W - g.textWidth(6, title)) / 2, 96,  6, accent, title);
    g.drawText((Gfx::W - g.textWidth(2, sub))   / 2, 168, 2, dim,    sub);

    // Test entries.
    for (int i = 0; i < kCount; i++) {
        int x, y, w, h;
        itemRect(i, x, y, w, h);
        bool on = (i == sel_);
        g.fillRect(x, y, w, h, on ? cardSel : cardBg);
        if (on) g.drawRectThick(x, y, w, h, 3, accent);
        // Centre the label + description block within the button.
        int lw  = g.textWidth(3, kItems[i].label);
        int dw  = g.textWidth(2, kItems[i].desc);
        int top = y + (h - 46) / 2;
        g.drawText(x + (w - lw) / 2, top,      3, fg, kItems[i].label);
        g.drawText(x + (w - dw) / 2, top + 30, 2, on ? fg : dim, kItems[i].desc);
    }

    // Footer hints: centred horizontally, anchored near the bottom edge.
    const char* hint =
        "D-Pad: Move    A / Touch: Select    ZL/ZR: Cycle modes    +: Exit";
    g.drawText((Gfx::W - g.textWidth(2, hint)) / 2, Gfx::H - 44, 2, dim, hint);
}
