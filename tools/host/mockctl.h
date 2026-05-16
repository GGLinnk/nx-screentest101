// Scene-control hooks for the asset generator. The walk-through script in
// gen.cpp uses these to feed canned HID / sensor / system data into the modes
// that no nxd::Input field carries (gestures, six-axis, capture, volume).
#pragma once
#include <switch.h>

void mockSetGesture(const HidGestureState* g);
void mockSetSixAxis(const HidSixAxisSensorState* s);
void mockSetCapture(bool pressed);
void mockSetVolume(s32 volume);
