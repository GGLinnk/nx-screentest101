#include "gesture_mode.hpp"
#include "gfx.hpp"
#include <cstdio>

namespace {

const char* kTypeName[] = {
    "Idle", "Complete", "Cancel", "Touch", "Press",
    "Tap", "Pan", "Swipe", "Pinch", "Rotate",
};
const char* kDirName[] = { "None", "Left", "Up", "Right", "Down" };

// How long a finished gesture stays on screen after release (seconds).
constexpr double HOLD_SEC = 1.5;

const char* typeName(u32 t) {
    return t < (sizeof(kTypeName) / sizeof(kTypeName[0])) ? kTypeName[t] : "?";
}
const char* dirName(u32 d) {
    return d < (sizeof(kDirName) / sizeof(kDirName[0])) ? kDirName[d] : "?";
}

bool meaningful(u32 t) {
    return t != HidGestureType_Idle && t != HidGestureType_Cancel;
}

// Significance ranking: a sequence keeps its highest-ranked type, so a pinch
// is not overwritten by the trailing pan reported as a finger lifts off.
int rank(u32 t) {
    switch (t) {
        case HidGestureType_Rotate: return 6;
        case HidGestureType_Pinch:  return 5;
        case HidGestureType_Swipe:  return 4;
        case HidGestureType_Pan:    return 3;
        case HidGestureType_Tap:    return 2;
        case HidGestureType_Press:  return 1;
        case HidGestureType_Touch:  return 0;
        default:                    return -1;
    }
}

} // namespace

void GestureMode::onEnter() {
    shown_ = HidGestureState{};
    ctx_ = 0;
    bestType_ = bestDir_ = 0;
    hold_ = 0.0;
    logCount_ = 0;
}

void GestureMode::update(const Input& in) {
    if (in.down & HidNpadButton_X)
        logCount_ = 0;

    HidGestureState st{};
    bool got = hidGetGestureStates(&st, 1) > 0;

    if (got && meaningful(st.type) && st.context_number != 0) {
        if (st.context_number != ctx_) {
            // New gesture sequence: open a fresh log entry.
            ctx_ = st.context_number;
            bestType_ = st.type;
            bestDir_  = st.direction;
            for (int i = LOG_MAX - 1; i > 0; i--) log_[i] = log_[i - 1];
            if (logCount_ < LOG_MAX) logCount_++;
        } else if (rank(st.type) > rank(bestType_)) {
            // Upgrade to the more significant type (e.g. Pan -> Pinch).
            bestType_ = st.type;
            bestDir_  = st.direction;
        }
        shown_ = st;
        shown_.type      = bestType_;
        shown_.direction = bestDir_;
        hold_ = HOLD_SEC;
        log_[0] = { bestType_, bestDir_, st.x, st.y };
    } else if (hold_ > 0.0) {
        // Keep the finished gesture readable for a moment after release.
        hold_ -= in.dtSec;
        if (hold_ <= 0.0) {
            hold_ = 0.0;
            ctx_ = 0;
            shown_ = HidGestureState{};
        }
    }
}

void GestureMode::render(Gfx& g) {
    const u32 white  = Gfx::rgb(255, 255, 255);
    const u32 dim    = Gfx::rgb(150, 156, 178);
    const u32 accent = Gfx::rgb(120, 180, 255);
    const u32 panel  = Gfx::rgb(22, 26, 38);

    g.clear(Gfx::rgb(12, 14, 20));

    const HidGestureState& s = shown_;
    bool held = hold_ > 0.0 && hold_ < HOLD_SEC;   // released but still shown
    char line[96];

    // --- live gesture readout (left panel) ---
    const int px = 8, py = 38, pw = 540, ph = 360;
    g.fillRect(px, py, pw, ph, panel);
    int tx = px + 14, ty = py + 12;

    g.drawText(tx, ty, 2, accent, held ? "LAST GESTURE" : "LIVE GESTURE");
    ty += 30;
    g.drawText(tx, ty, 4, white, typeName(s.type)); ty += 44;

    snprintf(line, sizeof(line), "Direction : %s", dirName(s.direction));
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Position  : %u, %u", s.x, s.y);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Delta     : %d, %d", s.delta_x, s.delta_y);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Velocity  : %.0f, %.0f", s.velocity_x, s.velocity_y);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Scale     : %.3f", s.scale);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Rotation  : %.1f deg", s.rotation_angle);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "Points    : %d", s.point_count);
    g.drawText(tx, ty, 2, dim, line); ty += 24;
    snprintf(line, sizeof(line), "DoubleTap : %s",
             (s.attributes & HidGestureAttribute_IsDoubleTap) ? "yes" : "no");
    g.drawText(tx, ty, 2, dim, line);

    // --- recent gesture log (right panel) ---
    const int lx = px + pw + 16, ly = 38, lw = Gfx::W - lx - 8, lh = 360;
    g.fillRect(lx, ly, lw, lh, panel);
    g.drawText(lx + 14, ly + 12, 2, accent, "RECENT GESTURES");
    for (int i = 0; i < logCount_; i++) {
        snprintf(line, sizeof(line), "%2d. %-7s %-6s @ %u,%u",
                 i + 1, typeName(log_[i].type), dirName(log_[i].dir),
                 log_[i].x, log_[i].y);
        g.drawText(lx + 14, ly + 44 + i * 24, 2, i == 0 ? white : dim, line);
    }
    if (logCount_ == 0)
        g.drawText(lx + 14, ly + 44, 2, dim, "perform a gesture...");

    // --- gesture point visualisation ---
    for (int i = 0; i < s.point_count && i < 4; i++) {
        const HidGesturePoint& p = s.points[i];
        g.fillCircle((int)p.x, (int)p.y, 10, accent);
        if (i > 0)
            g.drawLine((int)s.points[i - 1].x, (int)s.points[i - 1].y,
                       (int)p.x, (int)p.y, accent);
    }
    if (meaningful(s.type) && s.type != HidGestureType_Touch) {
        u32 mark = Gfx::rgb(255, 200, 64);
        g.drawCircle((int)s.x, (int)s.y, 26, mark);
        g.drawCross((int)s.x, (int)s.y, 38, mark);
    }
}
