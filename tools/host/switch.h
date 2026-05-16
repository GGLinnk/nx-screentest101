// Mock <switch.h> for the off-device host build (tools/host).
//
// It declares just enough of the libnx surface that source/ touches so the
// renderer and the test modes compile and run on a PC. Struct layouts mirror
// libnx by field name; they need not be binary-compatible since nothing here
// talks to real hardware.
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef u32 Result;
#define R_SUCCEEDED(res) ((res) == 0)
#define R_FAILED(res)    ((res) != 0)

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

// --- framebuffer / window ---------------------------------------------------
#define PIXEL_FORMAT_RGBA_8888 1
#define RGBA8(r, g, b, a) (((r) & 0xff) | (((g) & 0xff) << 8) | \
                           (((b) & 0xff) << 16) | (((a) & 0xff) << 24))
#define RGBA8_MAXALPHA(r, g, b) RGBA8(r, g, b, 0xff)

typedef struct { int unused; } NWindow;
typedef struct { int unused; } Framebuffer;

NWindow* nwindowGetDefault(void);
Result   framebufferCreate(Framebuffer* fb, NWindow* win, u32 width, u32 height,
                           u32 format, u32 num_fbs);
Result   framebufferMakeLinear(Framebuffer* fb);
void     framebufferClose(Framebuffer* fb);
void*    framebufferBegin(Framebuffer* fb, u32* out_stride);
void     framebufferEnd(Framebuffer* fb);

// --- buttons / styles / ids -------------------------------------------------
enum {
    HidNpadButton_A          = BIT(0),
    HidNpadButton_B          = BIT(1),
    HidNpadButton_X          = BIT(2),
    HidNpadButton_Y          = BIT(3),
    HidNpadButton_StickL     = BIT(4),
    HidNpadButton_StickR     = BIT(5),
    HidNpadButton_L          = BIT(6),
    HidNpadButton_R          = BIT(7),
    HidNpadButton_ZL         = BIT(8),
    HidNpadButton_ZR         = BIT(9),
    HidNpadButton_Plus       = BIT(10),
    HidNpadButton_Minus      = BIT(11),
    HidNpadButton_Left       = BIT(12),
    HidNpadButton_Up         = BIT(13),
    HidNpadButton_Right      = BIT(14),
    HidNpadButton_Down       = BIT(15),
    HidNpadButton_StickLLeft = BIT(16),
    HidNpadButton_StickLUp   = BIT(17),
    HidNpadButton_StickLRight= BIT(18),
    HidNpadButton_StickLDown = BIT(19),
    HidNpadButton_StickRLeft = BIT(20),
    HidNpadButton_StickRUp   = BIT(21),
    HidNpadButton_StickRRight= BIT(22),
    HidNpadButton_StickRDown = BIT(23),
    HidNpadButton_AnyLeft    = HidNpadButton_Left  | HidNpadButton_StickLLeft  | HidNpadButton_StickRLeft,
    HidNpadButton_AnyUp      = HidNpadButton_Up    | HidNpadButton_StickLUp    | HidNpadButton_StickRUp,
    HidNpadButton_AnyRight   = HidNpadButton_Right | HidNpadButton_StickLRight | HidNpadButton_StickRRight,
    HidNpadButton_AnyDown    = HidNpadButton_Down  | HidNpadButton_StickLDown  | HidNpadButton_StickRDown,
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

// --- touch ------------------------------------------------------------------
enum {
    HidTouchAttribute_Start = BIT(0),
    HidTouchAttribute_End   = BIT(1),
};

typedef struct HidTouchState {
    u64 delta_time;
    u32 attributes;
    u32 finger_id;
    u32 x;
    u32 y;
    u32 diameter_x;
    u32 diameter_y;
    u32 rotation_angle;
    u32 reserved;
} HidTouchState;

typedef struct HidTouchScreenState {
    u64 sampling_number;
    s32 count;
    u32 reserved;
    HidTouchState touches[16];
} HidTouchScreenState;

// --- analog stick -----------------------------------------------------------
typedef struct HidAnalogStickState {
    s32 x;
    s32 y;
} HidAnalogStickState;

// --- six-axis sensor --------------------------------------------------------
typedef struct HidVector {
    float x, y, z;
} HidVector;

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
    HidGestureType_Idle     = 0,
    HidGestureType_Complete = 1,
    HidGestureType_Cancel   = 2,
    HidGestureType_Touch    = 3,
    HidGestureType_Press    = 4,
    HidGestureType_Tap      = 5,
    HidGestureType_Pan      = 6,
    HidGestureType_Swipe    = 7,
    HidGestureType_Pinch    = 8,
    HidGestureType_Rotate   = 9,
};

enum {
    HidGestureAttribute_IsNewTouch  = BIT(4),
    HidGestureAttribute_IsDoubleTap = BIT(8),
};

typedef struct HidGesturePoint {
    u32 x, y;
} HidGesturePoint;

typedef struct HidGestureState {
    u64 sampling_number;
    u64 context_number;
    u32 type;
    u32 direction;
    u32 x;
    u32 y;
    s32 delta_x;
    s32 delta_y;
    float velocity_x;
    float velocity_y;
    u32 attributes;
    float scale;
    float rotation_angle;
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
    u8 major, minor, micro, padding1;
    u8 revision_major, revision_minor, padding2, padding3;
    char platform[0x20];
    char version_hash[0x40];
    char display_version[0x18];
    char display_title[0x80];
} SetSysFirmwareVersion;

Result setsysInitialize(void);
void   setsysExit(void);
Result setsysGetFirmwareVersion(SetSysFirmwareVersion* out);

// --- ts (temperature) -------------------------------------------------------
typedef enum {
    TsLocation_Internal = 0,
    TsLocation_External = 1,
} TsLocation;

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
    AudioTarget_Invalid        = 0,
    AudioTarget_Speaker        = 1,
    AudioTarget_Headphone      = 2,
    AudioTarget_Tv             = 3,
    AudioTarget_UsbOutputDevice= 4,
    AudioTarget_Bluetooth      = 5,
} AudioTarget;

Result audctlInitialize(void);
void   audctlExit(void);
Result audctlGetTargetVolume(s32* volume_out, AudioTarget target);
Result audctlGetTargetVolumeMin(s32* volume_out);
Result audctlGetTargetVolumeMax(s32* volume_out);
Result audctlGetActiveOutputTarget(AudioTarget* target_out);
