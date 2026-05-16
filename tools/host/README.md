# Host renderer

An off-device build that runs the test modes on a PC to generate screenshots
and GIFs for documentation — no Switch and no emulator needed.

It works because `Gfx` is already a pure software renderer: it writes `u32`
pixels into a buffer. The mock libnx layer here (`switch.h` + `mock.cpp`)
swaps the framebuffer for a host buffer and returns canned data for the HID
and system-service calls, so `source/`'s renderer and every mode compile and
run unmodified.

## Usage

```sh
make run       # build and render every scene into out/*.ppm
make assets    # also convert out/ to PNG stills and GIFs (needs ffmpeg or ImageMagick)
make clean
```

`mockgen` writes one PPM per scene (and frame sequences for the animated
ones); `gen-assets.sh` turns those into `out/*.png` and `out/*.gif`. Pick the
assets you want and reference them from the main README.

## Files

```
switch.h     mock libnx header (types + service declarations)
mock.cpp     host framebuffer + canned HID / system-service data
mockctl.h    hooks the scene script uses to feed sensors and read frames
mockgen.cpp  scene script: poses each mode with scripted input, captures frames
```

## Adding a scene

Each mode is driven directly: construct it, call `onEnter()`, build an
`Input`, call `update()`, then `capture()`. Sensor-backed modes read canned
data from `mock.cpp`; override gesture / six-axis / capture state with the
`mockSet*` hooks in `mockctl.h`.
