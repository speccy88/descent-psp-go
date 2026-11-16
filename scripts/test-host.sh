#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
cc -std=gnu99 -g -fsanitize=address,undefined -fno-sanitize-recover=all -ffunction-sections -fdata-sections -Iengine/include tests/fixed.c engine/maths/fixc.c engine/maths/tables.c -Wl,--gc-sections -lm -o build/tests/fixed
build/tests/fixed
python3 -m unittest discover -s tests -v
