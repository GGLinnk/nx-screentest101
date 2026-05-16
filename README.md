# Screen Tester 101

A Nintendo Switch homebrew **display, touchscreen and controller tester**,
built with devkitPro / libnx. It renders entirely in software via the libnx
Framebuffer API — no RomFS or external assets required.

Use it to check a Switch when buying, repairing or QA-checking a console:
dead/stuck pixels, backlight bleed, colour banding, geometry, flicker, image
retention, multitouch behaviour, touch report rate, dead touch zones, button
faults, analog-stick drift, the motion sensor and HD rumble.

## Demo

https://github.com/user-attachments/assets/e0a55cde-528d-4510-914c-47ad8ada1162
> A full feature walk-through — every test mode in turn — rendered off-device
> by the host renderer (see `tools/host`).

## Features

**Display Test** — full-screen patterns cycled with the D-pad:

| Pattern | What it checks |
|---|---|
| Solid Colour | Dead/stuck pixels, uniformity, backlight bleed (9 colours, optional auto-cycle) |
| Gradients | Colour banding (grey + R/G/B ramps) |
| Geometry Grid | Alignment, sharpness, scaling (grid, diagonals, circles) |
| Colour Bars | Colour reproduction (SMPTE-style bars) |
| RGB Sub-pixels | Partial sub-pixel defects (1px pure R/G/B columns) |
| Pixel Checkerboard | Pixel response / panel inversion (1px alternation) |
| Contrast Steps | Black→white luminance staircase |
| Black/White Clipping | Shadow crush & highlight clipping (near-black/near-white patches) |
| Retention Split | Image retention / burn-in (black/white split that swaps every 2s) |
| Strobe / Flicker | Backlight PWM / flicker, rate adjustable 1–30 Hz |
| Motion / Pursuit | Pixel response / ghosting (eye-trackable moving box, adjustable speed) |

**Touchscreen Test** — three views cycled with `Y`:
- **Live** — 16-finger colour-coded multitouch with per-finger property
  readout (position, diameter, rotation, Start/End), touch **report-rate
  measurement** in Hz (current / min / max) and a **single-finger jitter**
  readout (position spread in pixels).
- **Paint** — sweep the whole panel to reveal dead / insensitive zones.
- **Grid** — a 16×9 cell coverage map; tap every cell to systematically
  confirm the entire digitizer responds.

**Gesture Test**
- Live recognition of tap, press, pan, swipe, pinch and rotate gestures, with
  direction, velocity, scale, rotation angle and a recent-gesture log.

**Controls Test**
- Every button lights up while held, including the Capture button and the
  console volume buttons.
- Both analog sticks shown with a deadzone ring and a max-excursion box for
  spotting **stick drift**.
- Motion sensor readout (gyro + accelerometer) with a tilt bubble.
- HD rumble test: hold `ZL` / `ZR` to buzz each actuator.
- `B` and `+` are inhibited here so they can be tested; hold `L` or `R`
  together with `B` to return to the menu.

**System Info**
- Firmware version, battery charge and charger state, console temperature
  and the current operation mode (handheld or docked).
- Opened with `-` (Minus) from the main menu.

A shared HUD shows the active mode and the live **render FPS**.

## Controls

| Input | Action |
|---|---|
| D-pad + `A` / touch | Select a test from the menu |
| `-` | (Menu) open System Info |
| `ZL` / `ZR` | Cycle between modes (except in Controls Test) |
| `B` | Return to menu |
| `+` | Exit to hbmenu |
| `←` / `→` | (Display) change pattern |
| `↑` / `↓` / `A` | (Display) change solid colour, strobe rate or motion speed |
| `X` | (Display) toggle solid auto-cycle · (Touch/Gesture) reset stats / log |
| `Y` | (Display) toggle HUD · (Touch) cycle Live / Paint / Grid view |
| `A` | (Touch) clear canvas or reset grid |
| hold inputs · `ZL`/`ZR` | (Controls) test buttons/sticks · fire HD rumble |
| `L` / `R` + `B` | (Controls) return to menu (`B` and `+` alone are inhibited) |

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
screentest101.nro controls
screentest101.nro hwinfo
```

With no argument it opens the main menu as usual. (Useful for hbmenu
"forwarders" / title-redirect entries.)

### Runtime verification checklist

- Each display pattern fills the whole screen; `←`/`→` cycles all 11.
- Touching the screen draws a colour-coded circle per finger; multiple fingers
  raise "Active touches" and the "Max simultaneous" record (up to 16).
- The touch report-rate panel shows a plausible non-zero Hz while touching.
- The paint canvas accumulates strokes; the grid view marks tapped cells.
- Gesture mode reports tap / swipe / pinch / rotate and logs them.
- Controls mode lights up every button (including Capture and the volume
  buttons), tracks both sticks, shows live motion data and rumbles on
  `ZL`/`ZR`; `L`/`R` + `B` returns to the menu.
- System Info, opened with `-` from the menu, shows a valid firmware version,
  battery level and a non-zero temperature.
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
  touch_mode.{hpp,cpp}    multitouch / report-rate / jitter / paint / grid
  gesture_mode.{hpp,cpp}  gesture recognition
  controls_mode.{hpp,cpp} button / stick-drift / motion / rumble test
  hwinfo_mode.{hpp,cpp}   firmware / battery / temperature readout
```
