Descent PSP Go - 0.1.0-rc.4 candidate
Native D1X-Rebirth 0.58.1 port / Descent 1 PC shareware 1.4 / seven levels.
The user confirmed gameplay on PSP Go with rc.3, then encountered a sound-memory exit.
rc.4 fixes the growing sound cache; extended physical gameplay and frame rates remain unverified.

Launch: Game > System Storage > Descent PSP Go after safely ejecting USB.
Keep EBOOT.PBP, data, userdata, logs and licenses in this application folder.
Keep the music folder here if you generated the soundtrack.
Both ef0: and ms0: launch paths are supported. Never copy emulator userdata into this install.

Controls (default; remap through Start > PSP controls):
Analog yaw/pitch. Triangle/Cross forward/reverse. Square/Circle slide left/right.
R primary fire. L+R secondary fire suppresses primary.
L+Triangle/Cross slide up/down. L+Square/Circle roll left/right.
D-pad left/right cycle primary, down cycles secondary, up fires flare.
Select automap. Start pause/quick menu. L+Up drops bomb; L+Down rear view.
Automap: analog rotates; thrust buttons zoom; slides move; Select/Circle exits.
Menus: D-pad, Cross confirm, Circle cancel, Triangle toggle, Square keyboard in text fields.
HOME exits through the normal system callback. No use of HOLD or volume buttons.

Start menu exposes options, remapping, independent inversion, sensitivity/dead zone,
cockpit modes, bombs, save/load, controller help and end-game confirmation.
View setting: 0 classic aspect-correct 4:3; 1 raw 320x200; 2 stretched 480x272.
60 FPS is the default presentation target; 30 FPS is explicitly a fallback, not certified performance.

Shareware in-level saving/loading is disabled to preserve the original edition restriction.
Pilot progress and settings persist in userdata. Backup files use .bak; interrupted writes may
leave .pending files. Back up userdata before importing anything; foreign save compatibility
is unverified. The original HOG/PIG are never used for save state.

Optional music: eight original HMP sequences rendered offline with TinySoundFont and TimGM6mb,
streamed as 44.1kHz 16-bit stereo PCM from bounded buffers. This is a synthesis choice,
not a claim of identical output to every historical sound card. The five level songs repeat
across seven levels. Original shareware supplies no endlevel.hmp/endgame.hmp: the engine's
original missing-track behavior is retained. No retail soundtrack or campaign is included.

Logs: runtime.log, error.log, and bounded frames.csv on clean exit. Collect through installer.
Do not disconnect USB or remove storage during installation. Suspend/resume and dense combat,
reactor escape, later levels and long-duration audio still require physical PSP testing.

This is a noncommercial development candidate. Original Parallax and D1X licenses
apply. See licenses and project provenance.lock.json. Do not redistribute proprietary retail
assets or assume permission to publish generated soundtrack/bank material.
