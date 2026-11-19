"""NOP only the two verified iOS weapon-target and death-streaming calls."""


NOP = bytes.fromhex("1F2003D5")
PATCHES = (
    (0x26F5CC, 0x26F5C4, bytes.fromhex("E02500B0 00203C91 4A10FB97 602247B9")),
    (0x0B7BC4, 0x0B7BC0, bytes.fromhex("E00313AA 5C4A0994 564B0994")),
)


def patch_ped_camera_streaming(binary):
    for site, start, expected in PATCHES:
        offset = site - start
        patched = expected[:offset] + NOP + expected[offset + 4:]
        if binary[start:start + len(expected)] not in (expected, patched):
            raise ValueError(f"Unexpected iOS Ped camera/streaming call at 0x{site:X}")

    image = bytearray(binary)
    for site, _, _ in PATCHES:
        image[site:site + 4] = NOP
    return bytes(image)
