# PSP Go startup investigation

The first release uses a 40 MiB heap and requests extended memory. A physical
PSP Go showed a black screen. The console was disconnected during investigation,
so its original logs and actual failure point remain unknown.

The startup candidate requests standard memory, uses a 12 MiB heap, and keeps
the original 900-segment and 3,600-vertex limits on PSP. All seven included
shareware level headers fit those limits. The largest reports 786 segments and
2,722 vertices. The level loader rejects counts outside the compiled limits.

PPSSPP 1.20.4 in its 32 MB profile reported a contiguous allocation failure for
an intermediate 16 MiB heap with the original desktop level arrays. The final
candidate booted to the title and entered level one with music. Its immediate
boot log recorded the first displayed frame, audio initialization, and the
completed game data load. The desktop build and focused host tests passed.

The main build writes startup milestones to logs/boot.log. To isolate audio,
hold L through launch. The minimal separate Descent Boot Diagnostic uses a
2 MiB heap and tests EBOOT launch, display, storage logging, and Cross to exit.
Its log and exit were verified in PPSSPP. Build it on PSPDEV with
-DESCENT_BOOT_PROBE=ON, -DBUILD_PRX=ON, and -DENC_PRX=OFF.

Next physical evidence: the diagnostic result, the last line of boot.log,
runtime.log and error.log, and whether HOME responds if the screen goes black.
