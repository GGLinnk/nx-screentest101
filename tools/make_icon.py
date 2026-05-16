#!/usr/bin/env python3
"""Generate the Screen Tester 101 homebrew icon.

Draws a 32x32 pixel-art Nintendo Switch - blue + red Joy-Cons with analog
sticks, and a centre screen showing colour test bars with a touch crosshair -
then nearest-neighbour upscales it to the 256x256 JPG the homebrew menu wants.

Run with:  uv run --with pillow python tools/make_icon.py
"""
from pathlib import Path
from math import hypot
from PIL import Image

S = 32          # logical pixel-art grid
SCALE = 8       # -> 256x256 final icon
OUT = Path(__file__).resolve().parent.parent / "icon.jpg"

# --- palette ---
BG       = (13, 15, 23)
JOY_BLUE = (12, 185, 226)     # left Joy-Con (neon blue)
JOY_RED  = (243, 72, 58)      # right Joy-Con (neon red)
BODY     = (20, 22, 30)       # centre tablet body
FRAME    = (8, 9, 14)         # screen bezel
STICK_O  = (14, 15, 21)       # analog stick recess
STICK_I  = (66, 70, 84)       # analog stick cap
BTN      = (28, 24, 30)       # face buttons / d-pad
BARS = [
    (236, 236, 236),  # white
    (240, 224,  40),  # yellow
    ( 54, 202,  94),  # green
    ( 60, 200, 235),  # cyan
    (220,  74, 204),  # magenta
    (238,  74,  74),  # red
]
TOUCH   = (255, 176, 48)
TOUCH_O = (8, 8, 12)

px = [[BG for _ in range(S)] for _ in range(S)]


def put(x, y, c):
    if 0 <= x < S and 0 <= y < S:
        px[y][x] = c


def rect(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(x, y, c)


def disc(cx, cy, r, c):
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if hypot(dx, dy) <= r + 0.4:
                put(cx + dx, cy + dy, c)


def ring(cx, cy, r, c):
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if abs(hypot(dx, dy) - r) <= 0.6:
                put(cx + dx, cy + dy, c)


# --- console: left Joy-Con | tablet | right Joy-Con ---
CY0, CY1 = 6, 25
rect(2,  CY0,  8, CY1, JOY_BLUE)
rect(9,  CY0, 22, CY1, BODY)
rect(23, CY0, 29, CY1, JOY_RED)

# round the four outer corners of the console
R = 4
for cx, cy, sx, sy in ((2, CY0, 1, 1), (29, CY0, -1, 1),
                       (2, CY1, 1, -1), (29, CY1, -1, -1)):
    acx, acy = cx + sx * (R - 1), cy + sy * (R - 1)
    for dx in range(R):
        for dy in range(R):
            if hypot(cx + sx * dx - acx, cy + sy * dy - acy) > R - 0.5:
                put(cx + sx * dx, cy + sy * dy, BG)

# --- screen: vertical colour bars ---
SX0, SY0, SX1, SY1 = 10, 8, 21, 23
sw = SX1 - SX0 + 1
for y in range(SY0, SY1 + 1):
    for x in range(SX0, SX1 + 1):
        b = min((x - SX0) * len(BARS) // sw, len(BARS) - 1)
        put(x, y, BARS[b])
for x in range(SX0 - 1, SX1 + 2):
    put(x, SY0 - 1, FRAME)
    put(x, SY1 + 1, FRAME)
for y in range(SY0 - 1, SY1 + 2):
    put(SX0 - 1, y, FRAME)
    put(SX1 + 1, y, FRAME)

# --- touch crosshair on the screen ---
TCX, TCY, TR = 15, 15, 2
for dy in range(-TR - 1, TR + 2):
    for dx in range(-TR - 1, TR + 2):
        if TR - 0.6 <= hypot(dx, dy) <= TR + 1.3:
            put(TCX + dx, TCY + dy, TOUCH_O)
ring(TCX, TCY, TR, TOUCH)
for d in (TR + 1, TR + 2):
    put(TCX + d, TCY, TOUCH)
    put(TCX - d, TCY, TOUCH)
    put(TCX, TCY + d, TOUCH)
    put(TCX, TCY - d, TOUCH)
put(TCX, TCY, TOUCH)

# --- Joy-Con controls ---
# left: analog stick (upper) + d-pad (lower)
disc(5, 11, 2, STICK_O)
disc(5, 11, 1, STICK_I)
for dx, dy in ((0, 0), (0, -1), (0, 1), (-1, 0), (1, 0)):
    put(5 + dx, 20 + dy, BTN)
# right: face buttons (upper) + analog stick (lower)
disc(26, 20, 2, STICK_O)
disc(26, 20, 1, STICK_I)
for dx, dy in ((0, -2), (0, 2), (-2, 0), (2, 0)):
    put(26 + dx, 11 + dy, BTN)

# --- emit ---
img = Image.new("RGB", (S, S))
img.putdata([px[y][x] for y in range(S) for x in range(S)])
img = img.resize((S * SCALE, S * SCALE), Image.NEAREST)
img.save(OUT, "JPEG", quality=95, subsampling=0)
print(f"wrote {OUT} ({img.width}x{img.height})")
