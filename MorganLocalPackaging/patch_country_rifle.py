"""Keep Country Rifle in third person using the two verified iOS checks."""

from hashlib import sha256


SUPPORTED_BASELINES = {
    "02658d3065602f23504ab4d995759975b7604f0b2dab131088257646aaafe6e3",
    "a3cd1af625086a54b2f2c3470458f7914d7be447da023549ccc89bddc53c9167",
}
PATCHES = (
    (0x3BEE50, bytes.fromhex("7F87007160010054"), bytes.fromhex("1F2003D51F2003D5")),
    (0x3BF3D0, bytes.fromhex("7F870071"), bytes.fromhex("7FFF0371")),
)


def patch_country_rifle(binary):
    normalized = bytearray(binary)
    for site, original, patched in PATCHES:
        if binary[site:site + len(original)] not in (original, patched):
            raise ValueError(f"Unexpected Country Rifle check at 0x{site:X}")
        normalized[site:site + len(original)] = original
    if sha256(normalized).hexdigest() not in SUPPORTED_BASELINES:
        raise ValueError("Country Rifle requires a verified GTA SA iOS executable")

    image = bytearray(binary)
    for site, _, patched in PATCHES:
        image[site:site + len(patched)] = patched
    return bytes(image)
