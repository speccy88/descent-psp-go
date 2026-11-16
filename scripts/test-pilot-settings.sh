#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
pkg_config="${PKG_CONFIG:-$(command -v pkg-config || command -v pkgconf)}"
cc -std=gnu99 -O2 -g -DNDEBUG -D__LINUX__ -DNO_ASM '-DSHAREPATH="."' \
  -DDXX_VERSION_MAJORi=0 -DDXX_VERSION_MINORi=58 -DDXX_VERSION_MICROi=1 \
  -fsanitize=alignment -fno-sanitize-recover=alignment \
  -ffunction-sections -fdata-sections \
  -Iengine/include -Iengine/main -Iengine/arch/include -Iplatform/psp \
  $("$pkg_config" --cflags sdl physfs) \
  tests/pilot-settings.c engine/main/playsave.c engine/misc/strio.c \
  engine/misc/strutil.c engine/misc/physfsx.c engine/misc/ignorecase.c \
  -Wl,--gc-sections -Wl,--wrap=__isoc99_sscanf -Wl,--wrap=__isoc23_sscanf \
  $("$pkg_config" --libs physfs) -o build/tests/pilot-settings
build/tests/pilot-settings
