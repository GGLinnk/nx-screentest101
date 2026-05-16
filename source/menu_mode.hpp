#pragma once
#include "mode.hpp"

// Landing screen. Lists the test modes; pickable with the D-pad + A or by
// tapping an entry (which doubles as a first touchscreen smoke test).
class MenuMode : public Mode {
public:
    void onEnter() override;
    void update(const Input& in) override;
    void render(Gfx& g) override;
    const char* name() const override { return "Screen Tester 101"; }
    bool showChrome() const override { return false; }

private:
    int sel_ = 0;
};
