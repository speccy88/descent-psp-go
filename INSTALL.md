# Install Descent PSP Go

This prerelease runs the seven-level Descent 1 PC Shareware 1.4 campaign. It targets PSP Go with homebrew support enabled. The EBOOT was tested in PPSSPP. This candidate corrects startup memory use and alignment faults in model and pilot loading. The previous build reached pilot selection but froze while loading the saved pilot on a physical PSP Go. Physical gameplay with this correction still needs confirmation. See [ARK-4’s homebrew documentation](https://github.com/PSP-Archive/ARK-4/wiki/Homebrew-Support/d97f824abbab504f6da37d591fd159fdcac4f1ba) for an example of how unsigned PSP homebrew is launched.

1. Download [Descent-Go-0.1.0-rc.3-psp.zip](https://github.com/speccy88/descent-psp-go/releases/download/v0.1.0-rc.3/Descent-Go-0.1.0-rc.3-psp.zip) and extract it on your computer.
2. Open a terminal in the extracted folder. With Python 3.11 or newer, run:

   ```powershell
   python scripts/prepare-shareware.py Descent-Go
   ```

   The script downloads and checks the original archive, extracts the two required data files, and verifies the application folder. On systems where Python is named `python3`, use that command instead. If the download fails, get [desc14sw.tar.gz](https://icculus.org/d2x/data/desc14sw.tar.gz) and rerun with `--archive /path/to/desc14sw.tar.gz`. You can also extract `descent.hog` and `descent.pig` from that archive manually into `Descent-Go/data/`. Use only the data files. The archive's DOS executables are unnecessary.
3. Connect the PSP Go in USB mode and copy the prepared **Descent-Go** folder into **PSP/GAME/** on System Storage. If you already have an installation, back it up first and retain its `userdata` directory. The executable should be at `PSP/GAME/Descent-Go/EBOOT.PBP`, with `data/descent.hog` and `data/descent.pig` beside it under the `data` folder.
4. Safely eject the drive, leave USB mode, and open **Game → System Storage → Descent PSP Go**. Create or select a pilot and choose **New Game**. Press **Start → PSP controls** for bindings and the 30 FPS fallback.

The original shareware data supplies the levels and sound effects. Pilot progress is saved. In-level saves require registered data.

## Optional music

The release ZIP leaves the soundtrack to local preparation. On Linux or WSL, install a C compiler and Git, then download or clone this release's source. From the source directory run:

```bash
python3 scripts/fetch-inputs.py
python3 scripts/prepare-host-deps.py
bash scripts/prepare-music.sh
```

The pinned host packages use Ubuntu 26.04 x86-64. These commands fetch the verified shareware inputs, synthesizer source, and sound bank, then generate eight WAV tracks and loop metadata under `assets/music`. They require no PSP compiler. From the extracted release folder, rerun the setup command with `--music-dir /path/to/source/assets/music` before copying **Descent-Go** to the PSP. From Windows, use an accessible Windows path to that directory. The resulting app takes about 264 MiB with music.

## Verify and install from Windows

For a staged copy with SHA-256 readback, first run the Python setup command above. From the extracted folder run `.\scripts\Install-DescentGo.ps1 -Mode Plan -Package .\Descent-Go -PspRoot 'X:\'`, replacing X with the PSP's drive letter. Check the device and target in the output. To install, run the same command with `-Mode Install`, pass the printed IDs as `-ExpectedVolumeId` and `-ExpectedDeviceId`, and add `-DestinationConfirmed`. The script requires `-Update` before replacing an existing installation. See [README.md](https://github.com/speccy88/descent-psp-go/blob/v0.1.0-rc.3/README.md) for build and installer details.

## Startup diagnosis

If an earlier build freezes at the title or pilot selection, use this rc.3 executable and retain the entire `logs` folder after returning to USB mode. `logs/boot.log` records each model read and whether game-data loading completed. Holding L while launching disables audio for diagnosis. See [the investigation and validation record](https://github.com/speccy88/descent-psp-go/blob/v0.1.0-rc.3/docs/startup-investigation.md).
