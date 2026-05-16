#include "hwinfo_mode.hpp"
#include "nxdisplaylib/gfx.hpp"
#include <cstdio>

namespace {

const char* chargerName(int t) {
    switch (t) {
        case PsmChargerType_Unconnected: return "Not connected";
        case PsmChargerType_EnoughPower: return "Connected";
        case PsmChargerType_LowPower:    return "Connected (low power)";
        default:                        return "Unknown";
    }
}

// One label/value row inside the info panel.
void drawRow(Gfx& g, int x, int y, const char* label, const char* value) {
    g.drawText(x,       y, 3, Gfx::rgb(150, 156, 178), label);
    g.drawText(x + 360, y, 3, Gfx::rgb(236, 238, 248), value);
}

} // namespace

void HwInfoMode::onEnter() {
    psmInitialize();
    setsysInitialize();
    tsInitialize();
    tsSessionOk_ = R_SUCCEEDED(tsOpenSession(&tsSession_, TsDeviceCode_LocationInternal));

    SetSysFirmwareVersion fw{};
    if (R_SUCCEEDED(setsysGetFirmwareVersion(&fw)))
        snprintf(firmware_, sizeof(firmware_), "%s", fw.display_version);
    else
        snprintf(firmware_, sizeof(firmware_), "unknown");
}

void HwInfoMode::onExit() {
    if (tsSessionOk_) tsSessionClose(&tsSession_);
    tsExit();
    setsysExit();
    psmExit();
}

void HwInfoMode::update(const Input&) {
    if (R_FAILED(psmGetBatteryChargePercentage(&battery_)))
        battery_ = 0;

    PsmChargerType ct = PsmChargerType_Unconnected;
    psmGetChargerType(&ct);
    charger_ = (int)ct;

    // tsSessionGetTemperature works on current firmware; the legacy
    // tsGetTemperature is only a fallback for pre-14.0.0 systems.
    float t = 0.0f;
    if (tsSessionOk_) {
        haveTemp_ = R_SUCCEEDED(tsSessionGetTemperature(&tsSession_, &t));
    } else {
        s32 ti = 0;
        haveTemp_ = R_SUCCEEDED(tsGetTemperature(TsLocation_Internal, &ti));
        t = (float)ti;
    }
    if (haveTemp_) tempC_ = t;
}

void HwInfoMode::render(Gfx& g) {
    const u32 accent = Gfx::rgb(120, 180, 255);
    const u32 panel  = Gfx::rgb(22, 26, 38);
    char value[48];

    g.clear(Gfx::rgb(12, 14, 20));

    const int px = 140, py = 110, pw = 1000, ph = 420;
    g.fillRect(px, py, pw, ph, panel);
    g.drawRect(px, py, pw, ph, Gfx::rgb(46, 52, 70));

    int tx = px + 48, ty = py + 40;
    g.drawText(tx, ty, 3, accent, "SYSTEM INFORMATION");
    ty += 76;

    drawRow(g, tx, ty, "Firmware", firmware_); ty += 58;

    snprintf(value, sizeof(value), "%lu %%", (unsigned long)battery_);
    drawRow(g, tx, ty, "Battery", value); ty += 58;

    drawRow(g, tx, ty, "Charger", chargerName(charger_)); ty += 58;

    if (haveTemp_) snprintf(value, sizeof(value), "%.1f C", tempC_);
    else           snprintf(value, sizeof(value), "n/a");
    drawRow(g, tx, ty, "Temperature", value); ty += 58;

    drawRow(g, tx, ty, "Operation mode",
            appletGetOperationMode() == AppletOperationMode_Console
                ? "Docked / TV" : "Handheld");
}
