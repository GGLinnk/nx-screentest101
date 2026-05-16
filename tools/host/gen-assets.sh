#!/usr/bin/env sh
# Convert the PPM frames in build/ into PNG stills and animated GIFs in dist/.
# Uses ffmpeg if present, otherwise ImageMagick (magick or convert).
set -e
cd "$(dirname "$0")"
[ -d build ] || { echo "no build/ directory - run 'make run' first"; exit 1; }
mkdir -p dist

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
        ffmpeg -y -loglevel error -framerate 30 -i "build/${1}_%03d.ppm" \
            -vf "scale=640:-1:flags=lanczos" "$2"
    else
        mgk=convert; have magick && mgk=magick
        "$mgk" -delay 3 -loop 0 -resize 640x build/${1}_*.ppm "$2"
    fi
}

for f in build/menu.ppm build/display_*.ppm build/touch.ppm build/gesture.ppm \
         build/controls.ppm build/hwinfo.ppm; do
    [ -f "$f" ] || continue
    png="dist/$(basename "${f%.ppm}").png"
    still_to_png "$f" "$png"
    echo "  $png"
done

seq_to_gif anim_motion dist/motion.gif && echo "  dist/motion.gif"
seq_to_gif anim_touch  dist/touch.gif  && echo "  dist/touch.gif"

echo "assets written to dist/"
