"""Expand the verified iOS CFont::ProcessCurrentString stack buffer."""

from hashlib import sha256


FUNCTION_START = 0x2F2630
FUNCTION_END = 0x2F2E74
STOCK_SHA256 = "675b9929b5a7c1bbb58425852eac3237eea2c1b26a0c5ce27233ba39a430eba8"
PATCHES = (
    (0x2F265C, bytes.fromhex("FF C3 0D D1"), bytes.fromhex("FF 03 32 D1")),
    (0x2F273C, bytes.fromhex("09 81 0C 91"), bytes.fromhex("09 71 30 91")),
    (0x2F2E40, bytes.fromhex("FF C3 0D 91"), bytes.fromhex("FF 03 32 91")),
)


def patch_font_current_string(binary):
    if len(binary) < FUNCTION_END:
        raise ValueError("Truncated iOS font function")

    original = bytearray(binary[FUNCTION_START:FUNCTION_END])
    for address, before, after in PATCHES:
        offset = address - FUNCTION_START
        if original[offset:offset + 4] not in (before, after):
            raise ValueError(f"Unexpected iOS font instruction at 0x{address:X}")
        original[offset:offset + 4] = before

    if sha256(original).hexdigest() != STOCK_SHA256:
        raise ValueError("Unexpected iOS CFont::ProcessCurrentString body")

    image = bytearray(binary)
    for address, _, after in PATCHES:
        image[address:address + 4] = after
    return bytes(image)
