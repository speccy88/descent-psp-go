# Validation — 2026-11-15

The original release results below are historical. Later physical PSP Go tests exposed startup alignment faults and a saved-pilot loading failure; rc.3 then reached gameplay, which the user reported worked well until sound conversion exhausted the heap. See [startup-investigation.md](startup-investigation.md) for the startup fixes and [sound-memory.md](sound-memory.md) for the rc.4 correction, stress tests and remaining physical acceptance.

Candidate 0.1.0, Descent 1 PC shareware 1.4 (seven levels). PSP EBOOT SHA-256: `b7f075bfa9946adbc799443bfffe59a1aafc258ecafae19c24396d6b11e28553`.

## Builds and focused checks

- PSP GCC 15.2.0 / pinned official PSPDEV: engine and native diagnostic compile/link/package successfully. Matched unstripped ELF, map and PRX retained in dist/debug. Warnings from legacy code remain; compilation is not a full UB or parser audit.
- Fresh desktop reference CMake build passed using locally extracted, pinned packages. Earlier dummy-video smoke reached initialization and was deliberately stopped after eight seconds. Desktop interactive rendering and audio were not acceptance-tested.
- ASan/UBSan fixed arithmetic helper edge cases passed (121 input pairs). Four Python test groups passed: HOG directory bounds/truncation, PIG header/count validation, all eight HMP conversions/truncation, HMI delta decoding/malformed input.
- Package verification passed: monotonic bounded PBP sections, SFO title/MEMSIZE/EFAWARE, MIPS ELF/segment bounds, actual data/music hashes and 28 immutable-file hashes. Clean userdata and logs; no emulator state shipped.

## Emulator observations

PPSSPP 1.20.4, isolated memstick, software graphics, bad-memory-access ignoring disabled. This is software validation, not physical PSP acceptance.

Observed title, main menu, original briefing, level-one software-rendered geometry/cockpit, movement, firing and combat damage, automap, pause menu, compact controls menu, native pilot OSK, saved pilot/settings and original shareware save restriction. Eleven direct input-state assertions passed: forward; modifier replacement; modifier release; slide; roll replacement; vertical movement; primary fire; secondary suppression; primary restoration; analog pitch/yaw; neutral release. The final packaged EBOOT repeated level one, primary/secondary/neutral probes, automap exit, save-restriction display and clean exit. See evidence screenshots and JSON.

The final run produced 5,575 frame samples and an empty error.log. Exit invoked normal cleanup and flushed telemetry. Audio capture was 44,100 Hz, stereo, 113.28 seconds; peak 29,423, RMS 3,355.27, 95.41% nonzero samples. The music hook reported 4,712 blocks and zero starvation events. Numerical non-silence is not a subjective listening evaluation or a physical output-underrun test. Earlier failed experiments and intermediate screenshots stay isolated under .local and are not acceptance evidence.

Pilot creation persisted a 2,262-byte .plr and settings .plx; restart reused the pilot. Physical power-loss recovery and retail in-level save/load are untested. Shareware explicitly blocks in-level saves per its original readme.

## Performance interpretation

The emulator session includes menus/loading. Median interval 16.683 ms, p95 16.684 ms, p99 33.367 ms; maximum 4.922 seconds includes transitions. CPU-work median 8.018 ms / p95 15.057 ms; GE wait median 0.013 ms. These values cannot certify physical PSP frame rates. The 60 FPS default is a target; 30 FPS is an explicit user-selectable fallback. No physical dense-combat, reactor escape or worst-level optimization claim is made.

The 40 MiB application heap reservation logged 41,073,752 bytes estimated remaining at final shutdown, after game cleanup; this is not peak gameplay headroom. Actual stack high-water, peak gameplay memory, hardware GE timings and physical audio latency remain unmeasured.

## Remaining hardware and regression acceptance

1. Safely eject and launch on PSP Go; verify icon/title, native controls and first-level picture/audio.
2. Play later shareware levels, combat-heavy areas, death/restart, reactor countdown/escape and campaign progression; record gameplay-only frame distributions and peak memory/stack.
3. Test suspend/resume, HOME exit, analog centering and stuck-button suppression, music transitions/long loops, prolonged music plus effects, and storage persistence on the device.
4. With user-supplied registered HOG/PIG and a separately validated edition manifest, test retail save/load and retail campaign/secret levels. Current package intentionally contains only shareware.
5. Complete broader legacy fixed-point UB and malformed-data parser audit. Focused tests do not cover the whole engine.

No physical gameplay or 60 FPS certification is claimed. USB storage access cannot press console buttons or establish these results.
