// Controls for the mock libnx layer: the scene harness uses these to feed
// canned sensor data into the modes and to read back the rendered framebuffer.
#pragma once
#include <switch.h>

// Data returned by the mocked hidGet* service calls.
void mockSetGesture(const HidGestureState* g);
void mockSetSixAxis(const HidSixAxisSensorState* s);
void mockSetCapture(bool pressed);

// The host framebuffer (Gfx::W * Gfx::H pixels, RGBA8888) after endFrame().
const u32* mockFramebuffer(void);
