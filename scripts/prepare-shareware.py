#!/usr/bin/env python3
"""Prepare a Descent PSP Go release folder on your computer."""
import argparse
import hashlib
import json
import runpy
import shutil
import tarfile
import tempfile
import urllib.request
import wave
from pathlib import Path, PurePosixPath

verifier = runpy.run_path(str(Path(__file__).with_name("verify-package.py")))
verify = verifier["verify"]
verify_pbp = verifier["verify_pbp"]

SHAREWARE_URL = "https://icculus.org/d2x/data/desc14sw.tar.gz"
SHAREWARE_SHA256 = "8b65eda01d03401d7a2dc71976528fbb7e0b07a93ce8ee0a9f1d9103623ae249"
EBOOT_SHA256 = "b03b65752ee11d75beac99566c57e2c547ebe80058a28846f755dbe955a49c88"


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def unpack_shareware(archive, scratch, edition):
    if sha256(archive) != SHAREWARE_SHA256:
        raise ValueError("Shareware archive SHA-256 mismatch")
    with tarfile.open(archive, "r:gz") as source:
        for name in (*edition["files"], "readme.txt"):
            matches = [item for item in source.getmembers()
                       if PurePosixPath(item.name).name.lower() == name and item.isfile()]
            if len(matches) != 1:
                raise ValueError(f"Expected one {name} in the shareware archive")
            with source.extractfile(matches[0]) as stream:
                (scratch / name).write_bytes(stream.read())
    for name, info in edition["files"].items():
        path = scratch / name
        if path.stat().st_size != info["bytes"] or sha256(path) != info["sha256"]:
            raise ValueError(f"Shareware data hash mismatch: {name}")


def copy_music(source, target, edition):
    tracks = [song for song in edition["songs"] if "pcm" in song]
    for song in tracks:
        path = source / song["pcm"]
        with wave.open(str(path), "rb") as stream:
            if (stream.getnchannels(), stream.getsampwidth(), stream.getframerate()) != (2, 2, 44100):
                raise ValueError(f"Expected 44.1 kHz 16-bit stereo PCM: {path.name}")
            if stream.getnframes() == 0:
                raise ValueError(f"Empty music track: {path.name}")
    target.mkdir(exist_ok=True)
    for song in tracks:
        name = song["pcm"]
        shutil.copyfile(source / name, target / name)
        song["pcm_sha256"] = sha256(target / name)
        loop = (source / name).with_suffix(".loop")
        if loop.is_file():
            shutil.copyfile(loop, target / loop.name)


def prepare(root, archive, music):
    eboot = root / "EBOOT.PBP"
    if sha256(eboot) != EBOOT_SHA256:
        raise ValueError("EBOOT does not match this release")
    verify_pbp(eboot)
    edition_path = root / "edition.json"
    edition = json.loads(edition_path.read_text(encoding="utf-8"))
    with tempfile.TemporaryDirectory(prefix="descent-shareware-") as directory:
        scratch = Path(directory)
        if archive is None:
            archive = scratch / "desc14sw.tar.gz"
            with urllib.request.urlopen(SHAREWARE_URL, timeout=30) as source, archive.open("wb") as file:
                shutil.copyfileobj(source, file)
        unpack_shareware(archive, scratch, edition)
        (root / "data").mkdir(exist_ok=True)
        for name in edition["files"]:
            shutil.copyfile(scratch / name, root / "data" / name)
        shutil.copyfile(scratch / "readme.txt", root / "licenses" / "Descent-shareware-readme.txt")
    for folder in ("userdata", "logs"):
        (root / folder).mkdir(exist_ok=True)
    if music is not None:
        copy_music(music, root / "music", edition)
    edition["music_included"] = all((root / "music" / song["pcm"]).is_file()
                                    for song in edition["songs"] if "pcm" in song)
    for song in edition["songs"]:
        if "pcm" in song and edition["music_included"]:
            song["pcm_sha256"] = sha256(root / "music" / song["pcm"])
    edition_path.write_text(json.dumps(edition, indent=2) + "\n", encoding="utf-8")

    paths = [eboot, edition_path, root / "README-PSP.txt"]
    for folder in ("data", "licenses", "music"):
        paths.extend(path for path in sorted((root / folder).glob("*")) if path.is_file())
    manifest = {
        "version": "0.1.0-rc.2",
        "edition": edition["edition"],
        "hardware_tested": False,
        "files": [{"path": path.relative_to(root).as_posix(),
                   "bytes": path.stat().st_size, "sha256": sha256(path)} for path in paths],
    }
    (root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(verify(root), indent=2))
    print("Shareware setup complete. Copy the Descent-Go folder to PSP/GAME.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path, help="extracted Descent-Go folder on your computer")
    parser.add_argument("--archive", type=Path, help="desc14sw.tar.gz already downloaded")
    parser.add_argument("--music-dir", type=Path, help="folder of locally rendered WAV and loop files")
    args = parser.parse_args()
    prepare(args.folder, args.archive, args.music_dir)
