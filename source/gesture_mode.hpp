#pragma once
#include "mode.hpp"

// Gesture recognition test: reads the HID gesture engine and reports the
// gesture type (tap / press / pan / swipe / pinch / rotate), direction,
// velocity, scale and rotation, plus a short log of recent gestures.
//
// Each gesture sequence keeps the *most significant* type it reached (so a
// pinch is not masked by the trailing pan as a finger lifts), and the readout
// is held on screen briefly after release so it stays readable.
class GestureMode : public Mode {
public:
    void onEnter() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    const char* name() const override { return "Gesture Test"; }
    const char* controls() const override { return "Touch & drag to perform gestures   X: Clear log   B: Menu"; }

private:
    static constexpr int LOG_MAX = 8;

    HidGestureState shown_{};   // state shown (held briefly after release)
    u64    ctx_      = 0;       // gesture context currently being tracked
    u32    bestType_ = 0;       // highest-ranked type seen in this context
    u32    bestDir_  = 0;
    double hold_     = 0.0;     // seconds left to keep the gesture on screen

    struct LogEntry { u32 type; u32 dir; u32 x; u32 y; };
    LogEntry log_[LOG_MAX]{};
    int logCount_ = 0;
};
