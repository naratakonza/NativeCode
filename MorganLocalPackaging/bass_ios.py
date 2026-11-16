"""Read the pinned Un4seen BASS iPhoneOS SDK without extracting its archive."""

from hashlib import sha256
from pathlib import Path
import plistlib
from struct import unpack_from
from zipfile import ZipFile


SDK_SHA256 = "087bdb8aec6735a8b8de21253e07270f647cfb7bb3e1f276efa7ba979d46836e"
ARM64 = 0x0100000C
FRAMEWORK = "bass.framework/"


def bass_framework_files(sdk: Path):
    if sha256(sdk.read_bytes()).hexdigest() != SDK_SHA256:
        raise ValueError("Unexpected BASS iOS SDK. Use the verified Un4seen BASS 2.4.18.3 ZIP.")
    with ZipFile(sdk) as archive:
        info = plistlib.loads(archive.read("bass.xcframework/Info.plist"))
        [device] = [
            entry for entry in info["AvailableLibraries"]
            if entry["SupportedPlatform"] == "ios"
            and not entry.get("SupportedPlatformVariant")
            and "arm64" in entry["SupportedArchitectures"]
        ]
        if device["LibraryPath"] != FRAMEWORK.rstrip("/"):
            raise ValueError("Unexpected BASS framework path")
        folder = f"bass.xcframework/{device['LibraryIdentifier']}/{FRAMEWORK}"
        manifest = archive.read(folder + "Info.plist")
        binary = archive.read(folder + "bass")
        notice = archive.read("bass.txt")

    magic, count = unpack_from(">II", binary)
    if magic != 0xCAFEBABE:
        raise ValueError("Expected the BASS SDK's fat iPhoneOS executable")
    [arm64] = [
        (offset, length)
        for cpu, _, offset, length, _ in (
            unpack_from(">5I", binary, 8 + index * 20) for index in range(count)
        )
        if cpu == ARM64
    ]
    offset, length = arm64
    if offset + length > len(binary):
        raise ValueError("Invalid BASS ARM64 slice bounds")
    image = binary[offset:offset + length]
    if unpack_from("<4I", image) != (0xFEEDFACF, ARM64, 0, 6):
        raise ValueError("Expected an ARM64 BASS dylib")
    info = plistlib.loads(manifest)
    if info.get("CFBundleExecutable") != "bass" or info.get("CFBundleSupportedPlatforms") != ["iPhoneOS"]:
        raise ValueError("Expected an iPhoneOS BASS framework")
    return {
        FRAMEWORK + "bass": (image, 0o755),
        FRAMEWORK + "Info.plist": (manifest, 0o644),
        FRAMEWORK + "bass.txt": (notice, 0o644),
    }
