#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
pkg_config="${PKG_CONFIG:-$(command -v pkg-config || command -v pkgconf)}"
cc -std=gnu99 -O2 -g -fsanitize=alignment -fno-sanitize-recover=alignment \
  -ffunction-sections -fdata-sections -DWORDS_NEED_ALIGNMENT -D__LINUX__ -DNO_ASM \
  -Iengine/include -Iengine/main -Iengine/arch/include -Iplatform/psp \
  $("$pkg_config" --cflags sdl physfs) \
  tests/alignment.c engine/main/polyobj.c engine/main/bmread.c \
  engine/maths/fixc.c engine/maths/tables.c \
  -Wl,--gc-sections -lm -o build/tests/alignment
build/tests/alignment
