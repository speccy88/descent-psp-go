# PSP Go startup investigation

## Physical evidence

The original release requested extended memory with a 40 MiB heap and showed a
black screen on the user's PSP Go. Its exact failure point was not captured.
The startup candidate switched to standard memory, a 12 MiB heap, and the
original 900-segment / 3,600-vertex PSP limits. All seven shareware levels fit;
the largest reports 786 segments and 2,722 vertices. The loader checks these
limits before loading a level.

On 2026-11-15 the user confirmed that the startup candidate displayed the debug
startup text, Interplay logo and red Descent title, with music, then froze and
required a power cycle. Logs collected over USB ended at `loading game data`.
They confirm successful SDL, audio, GU and first-frame initialization. The
runtime log reported 5,054,208 bytes of free kernel memory and 12,582,248 bytes
available in the reserved heap at startup. These are not peak gameplay figures.

## Alignment faults and correction

Running the normal shareware startup path on the host with
`-fsanitize=alignment -fno-sanitize-recover=alignment` reproduced an unaligned
32-bit read in `pof_read_int`, called while loading the first robot model from
`gamedata_read_tbl`. POF has a six-byte header; its first chunk length is at
byte offset 10. Casting that byte address to `int *` is invalid on a processor
that requires aligned accesses.

Continuing the instrumented startup after correcting the model reader exposed
two more instances: robot fixed-point arrays in packed `robot_info`, and
animation frame-count output pointers into packed clip structures. The changes
copy model values and robot array elements through aligned locals, and pass
local frame counts to the animation loader. The binary data layouts remain
compatible. Truncated model scalar fields now produce a controlled error.

The physical console did not provide an exception PC. These are reproduced
faults in the same startup stage, and the likely cause of the observed freeze;
physical confirmation of the correction is still required.

## Validation of the alignment candidate

Local hardware candidate: `0.1.0-alignment-test`. Public release: `v0.1.0-rc.2`,
with the identical EBOOT.
EBOOT SHA-256:
`b03b65752ee11d75beac99566c57e2c547ebe80058a28846f755dbe955a49c88`.

- The complete shareware startup with `-notitles -norun -nosound -nomusic`
  exits successfully under the alignment sanitizer. The previous build fails
  in `pof_read_int` on that same path.
- `bash scripts/test-alignment.sh` passes scalar reads at all four byte offsets,
  truncated scalar rejection, packed robot array parsing and field-of-view
  conversion, with alignment instrumentation and assertions enabled.
- The fixed arithmetic tests and all four asset test groups pass.
- The PSP build and verification of all 28 immutable package files pass.
- PPSSPP 1.20.4, software graphics, 32 MB profile, bad-memory-access ignoring
  disabled: fresh userdata reaches pilot creation, persists a new pilot, enters
  level one and responds to movement/fire and the pause/end-game menu. Music
  streams transition between title, briefing and level tracks. The game also
  exits through its menu, flushes telemetry and reports zero music starvation
  events. This is emulator
  evidence, not physical gameplay acceptance.

To reproduce the startup check, prepare the host dependencies as in README.md,
source `scripts/desktop-env.sh`, and run:

```bash
cmake -S . -B build/desktop-alignment -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_FLAGS="-DWORDS_NEED_ALIGNMENT -fsanitize=alignment -fno-sanitize-recover=alignment -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=alignment
cmake --build build/desktop-alignment --parallel 8
mkdir -p build/tests/alignment-home
HOME="$PWD/build/tests/alignment-home" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  UBSAN_OPTIONS=print_stacktrace=1 build/desktop-alignment/descent-go \
  -hogdir "$PWD/assets/shareware" -notitles -norun -nosound -nomusic
bash scripts/test-alignment.sh
```

The corrected local package was installed on the identified Sony PSP Go on
2026-11-15. All 28 immutable files passed device readback; all five existing
user-state/log files were preserved byte-for-byte, and a verified full local
backup was retained. This confirms installation, not a successful physical
launch.

The rc.2 public ZIP was extracted and its shareware setup helper passed both
without music (16 verified files) and with locally prepared music (25 verified
files). Relinking the shipped debug objects, fixing imports and generating a
PRX with the pinned toolchain reproduces the release PRX byte-for-byte.

## Diagnostics and remaining hardware check

The PSP writes immediate milestones, including each model filename, to
`logs/boot.log`. Hold L through launch to disable audio for diagnosis. A separate
minimal display/launch probe is available with `-DDESCENT_BOOT_PROBE=ON`;
it uses a 2 MiB heap and exits with Cross. Its log and exit were checked in PPSSPP.

Next physical check: pass the red title, confirm or create a pilot, start a new
game, and enter level one. If it fails, collect the new boot/runtime/error logs
before another update. Actual hardware gameplay, sustained performance,
suspend/resume and HOME exit remain unverified.

## Follow-up: New Game freeze

The user subsequently confirmed that rc.2 reaches the main menu on the physical
PSP Go. A new freeze occurs after New Game, around the briefing or level-loading
screen. The collected log confirms `game data loaded`; runtime.log records the
title music but no briefing-track start. A pilot and settings were successfully
written. The log does not yet establish the failing operation.

The ordinary host New Game flow was driven through the difficulty menu and
briefing into level one under the alignment sanitizer, using a local copy of
the PSP pilot. A captured framebuffer confirms gameplay; no alignment error
was reported before the 22-second test watchdog stopped the process. Separately,
60 title/briefing/level music changes with pause/resume and worker joins passed
under ASan/UBSan using the PSP music implementation and SDL's dummy audio driver.
Neither check reproduces the physical freeze.

The follow-up diagnostic adds checkpoints around mission selection, player
initialization, briefing text and pictures, mixer detachment, music worker joins,
track startup, level resources and gameplay setup. Checkpoints record free heap,
free kernel memory and remaining stack. Its transition log was exercised through
level one in PPSSPP. It is diagnostic instrumentation, not a confirmed fix for
this second failure. An audio-disabled physical test (hold L during launch) is
requested to narrow the next investigation. No new gameplay claim is made.
