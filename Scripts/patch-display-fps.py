"""Patch the DisplayFPS overlay in the verified 62 FPS test IPA."""

import argparse
from copy import copy
from hashlib import sha256
from pathlib import Path
import tempfile
from zipfile import ZipFile


BASE_IPA_SHA256 = "e7f9bc7c5664def8a3c53ab1aa35ddba14e752b5a35bd73f6b8be1d032c25e7a"
GTA_PATH = "Payload/gtasa.app/gtasa"
DISPLAY_FPS_OFFSET = 0x159190
DISPLAY_FPS_ORIGINAL = bytes.fromhex("01 01 27 1E 20 18 20 1E")
DISPLAY_FPS_FIXED = bytes.fromhex("48 4F A8 52 00 01 27 1E")  # mov w8, #0x427A0000; fmov s0, w8


def patch_display_fps(executable):
    offset = DISPLAY_FPS_OFFSET
    end = offset + len(DISPLAY_FPS_ORIGINAL)
    if executable[offset:end] != DISPLAY_FPS_ORIGINAL:
        raise ValueError("Unexpected GTA iOS DisplayFPS instructions")
    return executable[:offset] + DISPLAY_FPS_FIXED + executable[end:]


def package_ipa(baseline, output):
    if output.exists():
        raise FileExistsError(output)
    if sha256(baseline.read_bytes()).hexdigest() != BASE_IPA_SHA256:
        raise ValueError("Unexpected baseline IPA")
    with ZipFile(baseline) as source, tempfile.TemporaryDirectory(dir=output.parent) as temp:
        patched = patch_display_fps(source.read(GTA_PATH))
        candidate = Path(temp) / output.name
        with ZipFile(candidate, "x") as target:
            target.comment = source.comment
            for member in source.infolist():
                content = patched if member.filename == GTA_PATH else source.read(member)
                target.writestr(copy(member), content)
        with ZipFile(candidate) as target:
            if target.testzip() is not None or target.namelist() != source.namelist():
                raise ValueError("Invalid IPA ZIP")
            changed = [name for name in source.namelist() if source.read(name) != target.read(name)]
            if changed != [GTA_PATH] or target.read(GTA_PATH) != patched:
                raise ValueError("Unexpected IPA member changes")
        candidate.rename(output)
    return sha256(output.read_bytes()).hexdigest()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(package_ipa(args.baseline, args.output))

