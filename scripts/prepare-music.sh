#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 scripts/prepare-assets.py > docs/assets-validation.log
cc -O2 -Ireference/TinySoundFont tools/render-midi.c -lm -o tools/render-midi
bank=deps/root/usr/share/sounds/sf2/TimGM6mb.sf2
for midi in assets/music/*.mid; do tools/render-midi "$midi" "$bank" "${midi%.mid}.wav"; done
