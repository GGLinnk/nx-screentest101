#pragma once
#include "nxdisplaylib/view.hpp"
#include "nxdisplaylib/gfx.hpp"
#include "nxdisplaylib/runner.hpp"

// App-wide umbrella: the nxd display/view framework names (View, Gfx, Input,
// Runner ...) are used unqualified throughout NX Screen Test.
using namespace nxd;

// Screen indices into the Runner's view list. A plain enum, so the values
// convert directly to the int indices the framework navigates by.
enum ModeId {
    Menu = 0,
    Display,
    Touch,
    Gesture,
    Controls,
    HwInfo,
    COUNT
};
