# Gameplay sound memory: rc.4

The user confirmed that rc.3 entered gameplay and played well on the physical
PSP Go, then exited with `Out of memory converting sound`. The console returned
without requiring a power cycle. This establishes physical gameplay for rc.3,
but does not establish long-session stability or certify a frame rate.

The installed rc.3 executable was verified as SHA-256
`1c7aba469ce518b041ecb806c4dab2a0c192969ef4f5f390ccdc51f44353b865`.
Its collected log records 1,691,656 free heap bytes entering gameplay and
337,272 at error cleanup, about 57 seconds later. The music worker reported
6,962 blocks and zero starvation events. The heap reservation remained 12 MiB;
approximately 4.98 MB of separate kernel memory was still free.

## Cause and correction

The previous SDL_mixer backend converted each effect from 11025 Hz U8 mono to
44100 Hz S16 stereo the first time it played, then retained the converted copy
for the lifetime of the game. The 70 shareware effects contain 827,545 original
sample bytes but require 13,240,720 bytes after that 16-fold expansion. The
largest single effect has 69,976 source bytes and needs 1,119,616 converted
bytes. A conversion allocation failure called the engine's fatal error handler.

The backend now reads the already-resident U8 samples directly while filling
the output buffer. It uses 64 fixed playback slots and a 256-frame S32 summing
buffer: 5,888 bytes total on PSP, independent of sample count, duration and
number of effects encountered. Starting and mixing an effect performs no
allocation, file I/O or whole-sample conversion. Source-rate timing, loops,
volume and stereo panning are applied as samples are mixed; accumulation clips
once to S16 after adding music and all effects.

Music retains its 44100 Hz stereo output and bounded PCM ring. The new effect
mixer uses SDL_mixer's post-mix callback, which runs after the music hook.
Callback registration and main-thread slot access use SDL's audio lock; the
callback never locks recursively. See the upstream [SDL_mixer 1.2 callback
implementation](https://github.com/libsdl-org/SDL_mixer/blob/SDL-1.2/mixer.c).

Channel queries now report actual playback state. Invalid sound/channel indices
are rejected, finished engine references are retired before slot reuse, and a
full set of 64 active slots rejects the extra effect without corrupting memory
or exiting. The old path could write `channels[-1]` when no channel was free.
Sound slots stop before original/custom asset buffers are freed or replaced.
Shutdown detaches the callback before closing audio. Runtime logs report starts,
peak simultaneous effects, full-channel rejections and zero conversion bytes.

## Verification

`bash scripts/test-sound-mixer.sh assets/shareware/descent.pig` links the actual
backend against SDL dummy audio under AddressSanitizer and UndefinedBehaviorSanitizer.
The optional PIG is the verified local shareware asset; it is not checked into
the repository. Without an argument, the synthetic functional tests still run.

- Played all 70 shareware effects to completion 20 times (1,400 full plays),
  including the longest reactor effect; then mixed 64 different looping effects
  through 2,048,000 more output frames. Allocation failure injection recorded
  zero attempted playback allocations.
- Checked exact sample duration, 11025-to-44100 and non-divisor rate timing,
  preservation of a pre-existing music signal, left/right pan, live volume,
  loop boundaries, silence, clipping and channel completion.
- Checked invalid indices, full-channel rejection, sound-object/queue ownership,
  stop/reset, a real asynchronous SDL callback, and close/reinitialization.
- PSP build and desktop alignment build passed. Existing fixed-arithmetic,
  asset, model-alignment and pilot-settings regression checks passed.
- The exact rc.4 EBOOT loaded a copy of the physical pilot in PPSSPP 1.20.4
  (standard 32 MB PSP model, software graphics, bad-memory-access ignoring
  disabled), entered level one with music, fired repeatedly and moved, then
  exited normally through the game menu after about 184 seconds in gameplay.
  It logged 862 effect starts, a peak of 13 simultaneous effects, no full-channel
  rejections and zero conversion bytes. Music logged 10,168 blocks and zero
  starvation events. Free heap was 1,695,976 bytes at gameplay entry and
  1,759,656 at the subsequent return-to-title music transition. These checkpoints
  are different game states, not continuous allocation profiling.
- The public PSP ZIP setup verified 16 files after preparing shareware locally.
  The matching debug ZIP relinked to a byte-identical PRX. The private package
  with locally generated music verified all 28 immutable files.

rc.4 EBOOT SHA-256:
`9b6b6c419107ee63e08df6dc29b734370f9ba506a7c38d28db5d46d17dfea64f`.

Extended physical gameplay of rc.4 remains to be tested. The correction removes
the identified sound-conversion allocation path; it does not establish that
every remaining engine allocation can never fail. Later levels, reactor escape,
physical suspend/resume and sustained dense combat remain acceptance gaps.
