#pragma once
#include "mode.hpp"

// Touchscreen test with three views (cycled with Y):
//   Live  - 16-finger colour-coded multitouch, per-finger readout, report
//           rate (Hz) and single-finger jitter measurement
//   Paint - persistent canvas swept to reveal dead / insensitive zones
//   Grid  - cell-coverage map: tap every cell to confirm the whole digitizer
class TouchMode : public View {
public:
    void onEnter() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    void renderOverlay(Gfx& g) override;
    const char* name() const override { return "Touchscreen Test"; }
    const char* controls() const override;
    bool transparentChrome() const override { return view_ != VIEW_LIVE; }

private:
    enum View { VIEW_LIVE = 0, VIEW_PAINT, VIEW_GRID, VIEW_COUNT };

    void resetStats();
    void clearGrid();

    int    view_     = VIEW_LIVE;
    double paintHue_ = 0.0;     // advancing hue for the rainbow paint brush

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

    // Single-finger jitter: position spread while exactly one finger rests.
    double jitN_ = 0.0, jitSumX_ = 0.0, jitSumY_ = 0.0;
    double jitSumXX_ = 0.0, jitSumYY_ = 0.0;
    double jitterPx_ = 0.0;

    // Grid-coverage map.
    static constexpr int GRID_COLS = 16, GRID_ROWS = 9;
    bool gridHit_[GRID_COLS * GRID_ROWS] = {};
};
