// Host (PC) mock <switch.h> for NX Screen Test's UI-preview build.
//
// It builds on nxdisplaylib's framework mock (nxd_host_libnx.h) and adds the
// HID / sensor / system-service surface the test screens touch. UI mock only:
// the readings are dummy data so the screens can be previewed on a desktop.
#pragma once
#include "nxd_host_libnx.h"   // framework base: types, framebuffer, pad, touch

// --- extra buttons (the base has the 16 primary ones) -----------------------
enum {
    HidNpadButton_StickLLeft  = BIT(16),
    HidNpadButton_StickLUp    = BIT(17),
    HidNpadButton_StickLRight = BIT(18),
    HidNpadButton_StickLDown  = BIT(19),
    HidNpadButton_StickRLeft  = BIT(20),
    HidNpadButton_StickRUp    = BIT(21),
    HidNpadButton_StickRRight = BIT(22),
    HidNpadButton_StickRDown  = BIT(23),
    HidNpadButton_AnyLeft  = HidNpadButton_Left  | HidNpadButton_StickLLeft  | HidNpadButton_StickRLeft,
    HidNpadButton_AnyUp    = HidNpadButton_Up    | HidNpadButton_StickLUp    | HidNpadButton_StickRUp,
    HidNpadButton_AnyRight = HidNpadButton_Right | HidNpadButton_StickLRight | HidNpadButton_StickRRight,
    HidNpadButton_AnyDown  = HidNpadButton_Down  | HidNpadButton_StickLDown  | HidNpadButton_StickRDown,
};

typedef enum {
    HidNpadIdType_No1      = 0,
    HidNpadIdType_Handheld = 0x20,
} HidNpadIdType;

typedef enum {
    HidNpadStyleTag_NpadFullKey  = BIT(0),
    HidNpadStyleTag_NpadHandheld = BIT(1),
    HidNpadStyleTag_NpadJoyDual  = BIT(2),
    HidNpadStyleTag_NpadJoyLeft  = BIT(3),
    HidNpadStyleTag_NpadJoyRight = BIT(4),
} HidNpadStyleTag;

// --- six-axis sensor --------------------------------------------------------
typedef struct HidVector { float x, y, z; } HidVector;

enum {
    HidSixAxisSensorAttribute_IsConnected    = BIT(0),
    HidSixAxisSensorAttribute_IsInterpolated = BIT(1),
};

typedef struct HidSixAxisSensorState {
    u64 delta_time;
    u64 sampling_number;
    HidVector acceleration;
    HidVector angular_velocity;
    HidVector angle;
    float direction[9];
    u32 attributes;
    u32 reserved;
} HidSixAxisSensorState;

typedef u64 HidSixAxisSensorHandle;
typedef u64 HidVibrationDeviceHandle;

typedef struct HidVibrationValue {
    float amp_low, freq_low, amp_high, freq_high;
} HidVibrationValue;

// --- gestures ---------------------------------------------------------------
enum {
    HidGestureType_Idle = 0, HidGestureType_Complete = 1, HidGestureType_Cancel = 2,
    HidGestureType_Touch = 3, HidGestureType_Press = 4, HidGestureType_Tap = 5,
    HidGestureType_Pan = 6, HidGestureType_Swipe = 7, HidGestureType_Pinch = 8,
    HidGestureType_Rotate = 9,
};
enum {
    HidGestureAttribute_IsNewTouch  = BIT(4),
    HidGestureAttribute_IsDoubleTap = BIT(8),
};

typedef struct HidGesturePoint { u32 x, y; } HidGesturePoint;

typedef struct HidGestureState {
    u64 sampling_number;
    u64 context_number;
    u32 type, direction, x, y;
    s32 delta_x, delta_y;
    float velocity_x, velocity_y;
    u32 attributes;
    float scale, rotation_angle;
    s32 point_count;
    HidGesturePoint points[4];
} HidGestureState;

// --- capture button ---------------------------------------------------------
typedef struct HidCaptureButtonState {
    u64 sampling_number;
    u64 buttons;
} HidCaptureButtonState;

// --- hid functions ----------------------------------------------------------
size_t hidGetGestureStates(HidGestureState* states, size_t count);
Result hidGetSixAxisSensorHandles(HidSixAxisSensorHandle* handles, s32 total,
                                  HidNpadIdType id, HidNpadStyleTag style);
Result hidStartSixAxisSensor(HidSixAxisSensorHandle handle);
Result hidStopSixAxisSensor(HidSixAxisSensorHandle handle);
size_t hidGetSixAxisSensorStates(HidSixAxisSensorHandle handle,
                                 HidSixAxisSensorState* states, size_t count);
Result hidInitializeVibrationDevices(HidVibrationDeviceHandle* handles, s32 total,
                                     HidNpadIdType id, HidNpadStyleTag style);
Result hidSendVibrationValues(const HidVibrationDeviceHandle* handles,
                              HidVibrationValue* values, s32 count);
u32    hidGetNpadStyleSet(HidNpadIdType id);
size_t hidGetCaptureButtonStates(HidCaptureButtonState* states, size_t count);
Result hidsysInitialize(void);
void   hidsysExit(void);
Result hidsysActivateCaptureButton(void);

// --- applet -----------------------------------------------------------------
typedef enum {
    AppletOperationMode_Handheld = 0,
    AppletOperationMode_Console  = 1,
} AppletOperationMode;
AppletOperationMode appletGetOperationMode(void);

// --- psm (battery) ----------------------------------------------------------
typedef enum {
    PsmChargerType_Unconnected = 0,
    PsmChargerType_EnoughPower = 1,
    PsmChargerType_LowPower    = 2,
} PsmChargerType;
Result psmInitialize(void);
void   psmExit(void);
Result psmGetBatteryChargePercentage(u32* out);
Result psmGetChargerType(PsmChargerType* out);

// --- setsys (firmware) ------------------------------------------------------
typedef struct {
    u8 major, minor, micro, padding1, revision_major, revision_minor, padding2, padding3;
    char platform[0x20], version_hash[0x40], display_version[0x18], display_title[0x80];
} SetSysFirmwareVersion;
Result setsysInitialize(void);
void   setsysExit(void);
Result setsysGetFirmwareVersion(SetSysFirmwareVersion* out);

// --- ts (temperature) -------------------------------------------------------
typedef enum { TsLocation_Internal = 0, TsLocation_External = 1 } TsLocation;
typedef enum {
    TsDeviceCode_LocationInternal = 0x41000001u,
    TsDeviceCode_LocationExternal = 0x41000002u,
} TsDeviceCode;
typedef struct { int unused; } TsSession;
Result tsInitialize(void);
void   tsExit(void);
Result tsGetTemperature(TsLocation location, s32* temperature);
Result tsOpenSession(TsSession* s, u32 device_code);
Result tsSessionGetTemperature(TsSession* s, float* temperature);
void   tsSessionClose(TsSession* s);

// --- audctl (volume) --------------------------------------------------------
typedef enum {
    AudioTarget_Invalid = 0, AudioTarget_Speaker = 1, AudioTarget_Headphone = 2,
    AudioTarget_Tv = 3, AudioTarget_UsbOutputDevice = 4, AudioTarget_Bluetooth = 5,
} AudioTarget;
Result audctlInitialize(void);
void   audctlExit(void);
Result audctlGetTargetVolume(s32* volume_out, AudioTarget target);
Result audctlGetTargetVolumeMin(s32* volume_out);
Result audctlGetTargetVolumeMax(s32* volume_out);
Result audctlGetActiveOutputTarget(AudioTarget* target_out);

// --- console (only the unreachable framebuffer-failure fallback uses it) -----
void consoleInit(void* console);
void consoleUpdate(void* console);
void consoleExit(void* console);
