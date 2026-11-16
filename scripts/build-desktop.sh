#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Requires SDL 1.2, SDL_mixer and PhysicsFS development libraries. Project-local
# extracted Debian packages may be selected with PKG_CONFIG_PATH/LD_LIBRARY_PATH.
[ ! -f scripts/desktop-env.sh ] || source scripts/desktop-env.sh
cmake -S . -B "${1:-build/desktop}" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "${1:-build/desktop}" --parallel "${JOBS:-8}"
