# Descent PSP Go

Native Descent 1 PSP Go development candidate, based on pinned D1X-Rebirth 0.58.1. The package contains PC shareware 1.4 (seven levels), not the registered campaign. Build and emulator validation are documented in docs; physical gameplay remains unverified.

The startup/alignment recovery candidate and its current hardware evidence are documented in [docs/startup-investigation.md](docs/startup-investigation.md).

## Install a release

Download the PSP ZIP and follow [INSTALL.md](INSTALL.md). The release includes a helper that fetches and verifies the original shareware data. Sound effects work with that data, and the guide includes steps to generate the optional music tracks.

## Build in Linux/WSL

Keep compilation on a Linux filesystem. From this repository on that filesystem:

```bash
python3 scripts/fetch-inputs.py
python3 scripts/prepare-host-deps.py   # extracts pinned packages locally; no sudo
source scripts/desktop-env.sh
bash scripts/build-desktop.sh
bash scripts/test-host.sh
bash scripts/test-alignment.sh
export PSPDEV=/absolute/path/to/pspdev
bash scripts/build-psp.sh build/diagnostic ON
bash scripts/build-psp.sh build/psp OFF
bash scripts/prepare-music.sh
python3 scripts/package.py
python3 scripts/verify-package.py dist/Descent-Go
```

Set PSPDEV to the extracted official v20261101 distribution whose URL and SHA-256 are in provenance.lock.json. The build uses psp-cmake, RelWithDebInfo, BUILD_PRX=ON and ENC_PRX=OFF. CMake preserves the unstripped ELF and map; PRX and PBP are produced by PSPSDK. No global host packages, USB drivers or elevated configuration are needed for this host setup.

`dist/Descent-Go/` is the clean application. `dist/debug/` has matched symbols and unencrypted PRX. `dist/Descent-Go-0.1.0-shareware.zip` is the application ZIP. Source stays separate from assets; the original shareware archive is retained locally in reference. Do not publish retail assets or assume unrestricted commercial licensing.

## Installation

Use scripts/Install-DescentGo.ps1 with an explicitly supplied root and the volume/device IDs printed by Plan. It rejects the system volume, Lexar storage, ambiguous identity, traversal and reparse points. Extract the package into a normal local directory before running the installer.

Modes: Plan (read-only), Install (staged copy with hash verification), Verify (immutable-file readback), CollectLogs (copy logs locally). -WhatIf is supported. -DestinationConfirmed records the user's confirmation. Existing installations require an explicit -Update decision and are backed up locally and verified; unknown/user files are retained, with the old device folder kept in an installer-owned sibling. No broad mirroring, deletion, formatting or repair is performed.

See docs/README-PSP.txt for controls, edition restrictions and launch instructions. See docs/architecture.md for implementation and remaining acceptance limits, docs/source-selection.md for source recovery results, and provenance.lock.json for locked inputs.

Validation evidence and remaining physical acceptance: [docs/validation.md](docs/validation.md).
