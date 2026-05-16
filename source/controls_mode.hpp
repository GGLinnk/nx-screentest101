#pragma once
#include "mode.hpp"

// Controls test: lights up every button, visualises both analog sticks with
// a deadzone ring and a max-excursion box (analog-stick drift check), shows
// the motion sensor (gyro + accelerometer with a tilt bubble), and fires HD
// rumble while ZL/ZR are held. ZL/ZR are captured here so they can be tested
// rather than cycling modes; B still returns to the menu.
class ControlsMode : public View {
public:
    void onEnter() override;
    void onExit() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    const char* name() const override { return "Controls Test"; }
    const char* controls() const override {
        return "Hold inputs   ZL/ZR Rumble   L or R + B  Exit to menu";
    }
    bool capturesCycle() const override { return true; }
    bool capturesExit()  const override { return true; }

private:
    u64 held_ = 0;
    HidAnalogStickState lstick_{}, rstick_{};

    // System buttons that are not part of the npad button mask.
    double captureFlash_ = 0.0;
    s32    volume_ = 0, volMin_ = 0, volMax_ = 0;
    s32    prevVolume_ = 0;
    bool   haveVolume_ = false, havePrevVol_ = false;
    double volUpFlash_ = 0.0, volDownFlash_ = 0.0;

    // Per-stick min/max excursion since entering the mode (drift box).
    s32 lMinX_ = 0, lMaxX_ = 0, lMinY_ = 0, lMaxY_ = 0;
    s32 rMinX_ = 0, rMaxX_ = 0, rMinY_ = 0, rMaxY_ = 0;

    // Motion sensor. Handles are acquired for every controller style and
    // update() reads whichever sensor currently reports as connected.
    HidSixAxisSensorState  six_{};
    HidSixAxisSensorHandle sixHandles_[6]{};
    int                    sixCount_ = 0;

    // HD-rumble device handles, one set per supported controller style.
    HidVibrationDeviceHandle vibHandheld_[2]{};
    HidVibrationDeviceHandle vibFullKey_[2]{};
    HidVibrationDeviceHandle vibJoyDual_[2]{};
};
