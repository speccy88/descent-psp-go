#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PSPDEV:?Set PSPDEV to official PSPDEV distribution}"
export PATH="$PSPDEV/bin:$PATH"
psp-cmake -S . -B "${1:-build/psp}" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_PRX=ON -DENC_PRX=OFF -DDESCENT_DIAGNOSTIC="${2:-OFF}"
cmake --build "${1:-build/psp}" --parallel "${JOBS:-8}"
