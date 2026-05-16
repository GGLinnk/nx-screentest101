# Host renderer

An off-device build that runs the test modes on a PC to generate screenshots,
GIFs and a feature video for documentation — no Switch and no emulator needed.

It works because `Gfx` is already a pure software renderer: it writes `u32`
pixels into a buffer. The mock libnx layer here (`switch.h` + `mock.cpp`)
swaps the framebuffer for a host buffer and returns canned data for the HID
and system-service calls, so `source/`'s renderer and every mode compile and
run unmodified.

## Usage

```sh
make run       # render every scene into build/*.ppm
make assets    # also convert build/ into dist/ PNG stills + GIFs
make video     # render the full feature walk-through to dist/showcase.mp4
make clean
```

`make assets` needs ffmpeg or ImageMagick; `make video` needs ffmpeg.

## Output layout

```
build/   intermediate artifacts: the mockgen binary and raw PPM frames
dist/    finished assets: PNG stills, GIFs, showcase.mp4
```

Both directories are gitignored — copy the assets you want from `dist/` to
wherever the main README references them.

## Files

```
switch.h     mock libnx header (types + service declarations)
mock.cpp     host framebuffer + canned HID / system-service data
mockctl.h    hooks the scene script uses to feed sensors and read frames
mockgen.cpp  scene script: stills (default) and the --video walk-through
```

## Adding a scene

Each mode is driven directly: construct it, call `onEnter()`, build an
`Input`, call `update()`, then capture the frame. Sensor-backed modes read
canned data from `mock.cpp`; override gesture / six-axis / capture / volume
state with the `mockSet*` hooks in `mockctl.h`. The `--video` walk-through is
a sequence of per-mode segments in `mockgen.cpp`.
