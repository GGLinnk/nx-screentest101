// Host stub implementations for NX Screen Test's UI-preview build. The HID,
// sensor and system-service calls return canned data; the gesture / six-axis
// / capture / volume state is overridable via mockctl.h so the asset
// generator can script a full feature walk-through.
#include "switch.h"
#include "mockctl.h"
#include <cstring>

// --- scene-controlled state -------------------------------------------------
namespace {
HidGestureState       g_gesture;
HidSixAxisSensorState g_sixaxis = []{
    HidSixAxisSensorState s{};
    s.acceleration = HidVector{ 0.0f, -1.0f, 0.0f };   // gravity, lying flat
    s.attributes   = HidSixAxisSensorAttribute_IsConnected;
    s.direction[0] = s.direction[4] = s.direction[8] = 1.0f;
    return s;
}();
bool g_capture = false;
s32  g_volume  = 40;
u64  g_sampling = 0;
} // namespace

void mockSetGesture(const HidGestureState* g) { if (g) g_gesture = *g; }
void mockSetSixAxis(const HidSixAxisSensorState* s) { if (s) g_sixaxis = *s; }
void mockSetCapture(bool pressed) { g_capture = pressed; }
void mockSetVolume(s32 volume) { g_volume = volume; }

// --- gestures ---------------------------------------------------------------
size_t hidGetGestureStates(HidGestureState* states, size_t count) {
    if (states && count) states[0] = g_gesture;
    return count ? 1 : 0;
}

// --- six-axis sensor --------------------------------------------------------
Result hidGetSixAxisSensorHandles(HidSixAxisSensorHandle* handles, s32 total,
                                  HidNpadIdType, HidNpadStyleTag) {
    for (s32 i = 0; i < total; i++) handles[i] = (HidSixAxisSensorHandle)(i + 1);
    return 0;
}
Result hidStartSixAxisSensor(HidSixAxisSensorHandle) { return 0; }
Result hidStopSixAxisSensor(HidSixAxisSensorHandle)  { return 0; }
size_t hidGetSixAxisSensorStates(HidSixAxisSensorHandle, HidSixAxisSensorState* s,
                                 size_t count) {
    if (s && count) *s = g_sixaxis;
    return count ? 1 : 0;
}

// --- vibration --------------------------------------------------------------
Result hidInitializeVibrationDevices(HidVibrationDeviceHandle* handles, s32 total,
                                     HidNpadIdType, HidNpadStyleTag) {
    for (s32 i = 0; i < total; i++) handles[i] = (HidVibrationDeviceHandle)(i + 1);
    return 0;
}
Result hidSendVibrationValues(const HidVibrationDeviceHandle*, HidVibrationValue*, s32) {
    return 0;
}

// --- npad style / capture / hidsys ------------------------------------------
u32 hidGetNpadStyleSet(HidNpadIdType) { return HidNpadStyleTag_NpadHandheld; }
size_t hidGetCaptureButtonStates(HidCaptureButtonState* states, size_t count) {
    if (states && count) {
        states[0].sampling_number = ++g_sampling;
        states[0].buttons = g_capture ? 1 : 0;
    }
    return count ? 1 : 0;
}
Result hidsysInitialize(void) { return 0; }
void   hidsysExit(void) {}
Result hidsysActivateCaptureButton(void) { return 0; }

// --- applet -----------------------------------------------------------------
AppletOperationMode appletGetOperationMode(void) { return AppletOperationMode_Handheld; }

// --- psm --------------------------------------------------------------------
Result psmInitialize(void) { return 0; }
void   psmExit(void) {}
Result psmGetBatteryChargePercentage(u32* o) { if (o) *o = 76; return 0; }
Result psmGetChargerType(PsmChargerType* o) { if (o) *o = PsmChargerType_EnoughPower; return 0; }

// --- setsys -----------------------------------------------------------------
Result setsysInitialize(void) { return 0; }
void   setsysExit(void) {}
Result setsysGetFirmwareVersion(SetSysFirmwareVersion* o) {
    if (o) {
        *o = SetSysFirmwareVersion{};
        o->major = 20;
        strcpy(o->platform, "NX");
        strcpy(o->display_version, "20.0.0-host");
    }
    return 0;
}

// --- ts ---------------------------------------------------------------------
Result tsInitialize(void) { return 0; }
void   tsExit(void) {}
Result tsGetTemperature(TsLocation, s32* t) { if (t) *t = 43; return 0; }
Result tsOpenSession(TsSession*, u32) { return 0; }
Result tsSessionGetTemperature(TsSession*, float* t) { if (t) *t = 43.5f; return 0; }
void   tsSessionClose(TsSession*) {}

// --- audctl -----------------------------------------------------------------
Result audctlInitialize(void) { return 0; }
void   audctlExit(void) {}
Result audctlGetTargetVolume(s32* v, AudioTarget) { if (v) *v = g_volume; return 0; }
Result audctlGetTargetVolumeMin(s32* v)           { if (v) *v = 0;  return 0; }
Result audctlGetTargetVolumeMax(s32* v)           { if (v) *v = 64; return 0; }
Result audctlGetActiveOutputTarget(AudioTarget* t){ if (t) *t = AudioTarget_Speaker; return 0; }

// --- console (dead code on host: the framebuffer never fails) ---------------
void consoleInit(void*)   {}
void consoleUpdate(void*) {}
void consoleExit(void*)   {}
