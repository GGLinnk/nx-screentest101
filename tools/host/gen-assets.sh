#!/usr/bin/env sh
# Convert the PPM frames in out/ into PNG stills and animated GIFs.
# Uses ffmpeg if present, otherwise ImageMagick (magick or convert).
set -e
cd "$(dirname "$0")"
[ -d out ] || { echo "no out/ directory - run ./mockgen first"; exit 1; }

have() { command -v "$1" >/dev/null 2>&1; }
have ffmpeg || have magick || have convert || {
    echo "need ffmpeg or ImageMagick"; exit 1; }

still_to_png() {
    if have ffmpeg; then ffmpeg -y -loglevel error -i "$1" "$2"
    elif have magick; then magick "$1" "$2"
    else convert "$1" "$2"
    fi
}

seq_to_gif() {
    if have ffmpeg; then
        ffmpeg -y -loglevel error -framerate 30 -i "out/${1}_%03d.ppm" \
            -vf "scale=640:-1:flags=lanczos" "$2"
    else
        mgk=convert; have magick && mgk=magick
        "$mgk" -delay 3 -loop 0 -resize 640x out/${1}_*.ppm "$2"
    fi
}

for f in out/menu.ppm out/display_*.ppm out/touch.ppm out/gesture.ppm \
         out/controls.ppm out/hwinfo.ppm; do
    [ -f "$f" ] || continue
    png="out/$(basename "${f%.ppm}").png"
    still_to_png "$f" "$png"
    echo "  $png"
done

seq_to_gif anim_motion out/motion.gif && echo "  out/motion.gif"
seq_to_gif anim_touch  out/touch.gif  && echo "  out/touch.gif"

echo "assets written to out/ (*.png, *.gif)"
