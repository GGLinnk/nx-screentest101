# Screen Tester 101

A Nintendo Switch homebrew **display & touchscreen tester**, built with
devkitPro / libnx. It renders entirely in software via the libnx Framebuffer
API — no RomFS or external assets required.

Use it to check a Switch panel and digitizer when buying, repairing or
QA-checking a console: dead/stuck pixels, backlight bleed, colour banding,
geometry, flicker, multitouch behaviour, touch report rate, and dead touch
zones.

## Features

**Display Test** — full-screen patterns cycled with the D-pad:

| Pattern | What it checks |
|---|---|
| Solid Colour | Dead/stuck pixels, panel uniformity, backlight bleed (9 colours) |
| Gradients | Colour banding (grey + R/G/B ramps) |
| Geometry Grid | Alignment, sharpness, scaling (grid, diagonals, circles) |
| Colour Bars | Colour reproduction (SMPTE-style bars) |
| Pixel Checkerboard | Pixel response / panel inversion (1px alternation) |
| Contrast Steps | Black→white luminance staircase |
| Strobe / Flicker | Backlight PWM / flicker, rate adjustable 1–30 Hz |

**Touchscreen Test**
- 16-finger colour-coded multitouch visualisation (contact size + rotation).
- Per-finger property readout: position, diameter, rotation angle,
  `delta_time`, Start/End attributes.
- **Touch report-rate measurement** in Hz (current / min / max), derived from
  the HID `sampling_number` counter.
- **Paint canvas**: sweep the whole panel to reveal dead / insensitive
  digitizer zones.

**Gesture Test**
- Live recognition of tap, press, pan, swipe, pinch and rotate gestures, with
  direction, velocity, scale, rotation angle and a recent-gesture log.

A shared HUD shows the active mode and the live **render FPS**.

## Controls

| Input | Action |
|---|---|
| D-pad + `A` / touch | Select a test from the menu |
| `ZL` / `ZR` | Cycle between modes |
| `B` | Return to menu |
| `+` | Exit to hbmenu |
| `←` / `→` | (Display) change pattern |
| `↑` / `↓` / `A` | (Display) change solid colour / strobe rate |
| `Y` | (Display) toggle HUD · (Touch) toggle paint canvas |
| `A` | (Touch) clear canvas |
| `X` | (Touch) reset rate stats · (Gesture) clear log |

## Building

Requires devkitPro with the `switch-dev` group (`devkitA64` + `libnx`).

```sh
DEVKITPRO=/opt/devkitpro make
```

This produces `screentest101.nro`. `make clean` removes build artifacts.
(`DEVKITPRO` is only needed if it is not already exported in your environment.)

## Running

Copy `screentest101.nro` to the `/switch/` folder of your SD card and launch it
from the homebrew menu, or load the `.nro` in a Switch emulator.

### Launch shortcuts

A mode name can be passed as a launch argument to jump straight past the menu:

```
screentest101.nro display
screentest101.nro touch
screentest101.nro gesture
```

With no argument it opens the main menu as usual. (Useful for hbmenu
"forwarders" / title-redirect entries.)

### Runtime verification checklist

- Each display pattern fills the whole screen; `←`/`→` cycles all 7.
- Touching the screen draws a colour-coded circle per finger; multiple fingers
  raise "Active touches" and the "Max simultaneous" record (up to 16).
- The touch report-rate panel shows a plausible non-zero Hz while touching.
- The paint canvas (`Y`) accumulates strokes and `A` clears it.
- Gesture mode reports tap / swipe / pinch / rotate and logs them.
- `+` returns cleanly to the homebrew menu.

## Project layout

```
source/
  main.cpp            entrypoint + framebuffer-failure fallback
  app.{hpp,cpp}       main loop, mode dispatch, FPS HUD, chrome
  gfx.{hpp,cpp}       framebuffer wrapper + drawing primitives
  font.{hpp,cpp}      embedded 8x8 bitmap font
  mode.hpp            abstract Mode interface + Input struct
  menu_mode.{hpp,cpp}     landing menu
  display_mode.{hpp,cpp}  display panel patterns
  touch_mode.{hpp,cpp}    multitouch / report-rate / paint canvas
  gesture_mode.{hpp,cpp}  gesture recognition
```
