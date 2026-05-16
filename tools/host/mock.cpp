// Mock implementation of the libnx surface declared in tools/host/switch.h.
//
// The framebuffer calls render into a host-side buffer; the HID / system
// service calls return canned data (overridable for sensors via mockctl.h).
#include <switch.h>
#include <cstring>
#include "mockctl.h"

static const int kW = 1280, kH = 720;
static u32 g_fb[kW * kH];

static HidGestureState       g_gesture;
static HidSixAxisSensorState g_sixaxis;
static bool                  g_capture;
static u64                   g_sampling;

void mockSetGesture(const HidGestureState* g) { g_gesture = *g; }
void mockSetSixAxis(const HidSixAxisSensorState* s) { g_sixaxis = *s; }
void mockSetCapture(bool pressed) { g_capture = pressed; }
const u32* mockFramebuffer(void) { return g_fb; }

// --- framebuffer ------------------------------------------------------------
static NWindow g_window;
NWindow* nwindowGetDefault(void) { return &g_window; }

Result framebufferCreate(Framebuffer*, NWindow*, u32, u32, u32, u32) { return 0; }
Result framebufferMakeLinear(Framebuffer*) { return 0; }
void   framebufferClose(Framebuffer*) {}

void* framebufferBegin(Framebuffer*, u32* out_stride) {
    if (out_stride) *out_stride = kW * sizeof(u32);
    return g_fb;
}
void framebufferEnd(Framebuffer*) {}

// --- hid --------------------------------------------------------------------
size_t hidGetGestureStates(HidGestureState* states, size_t count) {
    if (count == 0) return 0;
    states[0] = g_gesture;
    return 1;
}

Result hidGetSixAxisSensorHandles(HidSixAxisSensorHandle* handles, s32 total,
                                  HidNpadIdType, HidNpadStyleTag) {
    for (s32 i = 0; i < total; i++) handles[i] = 0;
    return 0;
}
Result hidStartSixAxisSensor(HidSixAxisSensorHandle) { return 0; }
Result hidStopSixAxisSensor(HidSixAxisSensorHandle) { return 0; }

size_t hidGetSixAxisSensorStates(HidSixAxisSensorHandle, HidSixAxisSensorState* states,
                                 size_t count) {
    if (count == 0) return 0;
    states[0] = g_sixaxis;
    return 1;
}

Result hidInitializeVibrationDevices(HidVibrationDeviceHandle* handles, s32 total,
                                     HidNpadIdType, HidNpadStyleTag) {
    for (s32 i = 0; i < total; i++) handles[i] = 0;
    return 0;
}
Result hidSendVibrationValues(const HidVibrationDeviceHandle*, HidVibrationValue*, s32) {
    return 0;
}

u32 hidGetNpadStyleSet(HidNpadIdType id) {
    return id == HidNpadIdType_Handheld ? HidNpadStyleTag_NpadHandheld : 0;
}

size_t hidGetCaptureButtonStates(HidCaptureButtonState* states, size_t count) {
    if (count == 0) return 0;
    states[0].sampling_number = ++g_sampling;
    states[0].buttons = g_capture ? 1 : 0;
    return 1;
}

Result hidsysInitialize(void) { return 0; }
void   hidsysExit(void) {}
Result hidsysActivateCaptureButton(void) { return 0; }

// --- applet -----------------------------------------------------------------
AppletOperationMode appletGetOperationMode(void) { return AppletOperationMode_Handheld; }

// --- psm --------------------------------------------------------------------
Result psmInitialize(void) { return 0; }
void   psmExit(void) {}
Result psmGetBatteryChargePercentage(u32* out) { *out = 76; return 0; }
Result psmGetChargerType(PsmChargerType* out) { *out = PsmChargerType_EnoughPower; return 0; }

// --- setsys -----------------------------------------------------------------
Result setsysInitialize(void) { return 0; }
void   setsysExit(void) {}
Result setsysGetFirmwareVersion(SetSysFirmwareVersion* out) {
    std::memset(out, 0, sizeof(*out));
    std::strcpy(out->display_version, "18.1.0");
    return 0;
}

// --- ts ---------------------------------------------------------------------
Result tsInitialize(void) { return 0; }
void   tsExit(void) {}
Result tsGetTemperature(TsLocation, s32* temperature) { *temperature = 42; return 0; }
Result tsOpenSession(TsSession*, u32) { return 0; }
Result tsSessionGetTemperature(TsSession*, float* temperature) { *temperature = 42.5f; return 0; }
void   tsSessionClose(TsSession*) {}

// --- audctl -----------------------------------------------------------------
Result audctlInitialize(void) { return 0; }
void   audctlExit(void) {}
Result audctlGetTargetVolume(s32* volume_out, AudioTarget) { *volume_out = 60; return 0; }
Result audctlGetTargetVolumeMin(s32* volume_out) { *volume_out = 0; return 0; }
Result audctlGetTargetVolumeMax(s32* volume_out) { *volume_out = 100; return 0; }
Result audctlGetActiveOutputTarget(AudioTarget* target_out) {
    *target_out = AudioTarget_Speaker;
    return 0;
}
