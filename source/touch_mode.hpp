#pragma once
#include "mode.hpp"

// Touchscreen test: 16-finger colour-coded multitouch visualisation, per-finger
// property readout, touch report-rate (Hz) measurement, and a persistent paint
// canvas for sweeping the panel to reveal dead / insensitive digitizer zones.
class TouchMode : public Mode {
public:
    void onEnter() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    void renderOverlay(Gfx& g) override;
    const char* name() const override { return "Touchscreen Test"; }
    const char* controls() const override;
    bool transparentChrome() const override { return canvasView_; }

private:
    void resetStats();

    bool   canvasView_ = false; // Y: paint-canvas view vs. live multitouch view
    double paintHue_   = 0.0;   // advancing hue for the rainbow paint brush

    // Live touch snapshot (copied each frame for rendering).
    HidTouchScreenState touch_{};
    int  maxFingers_ = 0;       // most simultaneous touches seen this session

    // Report-rate measurement, derived from HidTouchScreenState.sampling_number.
    u64    prevSampling_ = 0;
    bool   haveSampling_ = false;
    double rateWindow_   = 0.0; // seconds accumulated in the current window
    u64    rateSamples_  = 0;   // sampling_number deltas accumulated
    double curHz_ = 0.0, minHz_ = 0.0, maxHz_ = 0.0;
    u64    lastDeltaTime_ = 0;  // delta_time of the primary touch (ns)
};
