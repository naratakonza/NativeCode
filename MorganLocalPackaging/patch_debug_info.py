"""Disable only original FPS/temporary diagnostic drawing, retaining the FPS calculator."""

from hashlib import sha256


BASE_SHA = {
    "d14833a5229535ce554535c496bd2d5ece52dd64fa7f43b991da9e16a285837b",
    "a00582dd957486098c3e57e24f74f21335fb5ab6fd0c1f24ee998959d4b2fac7",
}
NOP = bytes.fromhex("1F2003D5")
PATCHES = (
    (0x1591F8, 0x1591F0, bytes.fromhex("2108221EE0A3039159640694A8835BF8")),
    (0x159EC4, 0x159EBC, bytes.fromhex("0001271EE0C300912661069400008012")),
    (0x159F34, 0x159F2C, bytes.fromhex("E0C300910041201E0A610694481C00B0")),
    (0x159F88, 0x159F80, bytes.fromhex("E0C300910041201EF5600694A8835BF8")),
)


def patch_debug_info(binary):
    normalized = bytearray(binary)
    for site, start, expected in PATCHES:
        offset = site - start
        patched = expected[:offset] + NOP + expected[offset + 4:]
        if binary[start:start + len(expected)] not in (expected, patched):
            raise ValueError(f"Unexpected debug-info font call at {site:#x}")
        normalized[site:site + 4] = expected[offset:offset + 4]
    if sha256(normalized).hexdigest() not in BASE_SHA:
        raise ValueError("DebugInfo requires the verified f2ed210 engine, optionally with vehicle-weapon")

    image = bytearray(binary)
    for site, _, _ in PATCHES:
        image[site:site + 4] = NOP
    return bytes(image)
