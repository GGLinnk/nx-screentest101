#pragma once
#include "mode.hpp"

// System information screen: firmware version, battery charge and charger
// state, console temperature and the current operation mode.
class HwInfoMode : public View {
public:
    void onEnter() override;
    void onExit() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    const char* name() const override { return "System Info"; }
    const char* controls() const override { return "B Menu"; }
    bool capturesCycle() const override { return true; }

private:
    char  firmware_[32] = "unknown";
    u32   battery_  = 0;
    int   charger_  = 0;
    float tempC_    = 0.0f;
    bool  haveTemp_ = false;

    TsSession tsSession_{};
    bool      tsSessionOk_ = false;
};
