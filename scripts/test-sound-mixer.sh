#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
pkg_config="${PKG_CONFIG:-$(command -v pkg-config || command -v pkgconf)}"
cc -std=gnu99 -O2 -g -DNDEBUG -D__LINUX__ -DNO_ASM \
  -fsanitize=address,undefined -fno-sanitize-recover=all \
  -Iengine/include -Iengine/main -Iengine/arch/include \
  $("$pkg_config" --cflags sdl SDL_mixer physfs) \
  tests/sound-mixer.c engine/arch/sdl/digi_mixer.c engine/main/snddecom.c \
  -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=Mix_SetPostMix \
  $("$pkg_config" --libs sdl SDL_mixer) -o build/tests/sound-mixer
SDL_AUDIODRIVER=dummy build/tests/sound-mixer "$@"
