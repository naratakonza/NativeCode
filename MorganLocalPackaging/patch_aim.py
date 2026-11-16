"""Apply the six Thunder targeting changes at verified iOS ARM64 sites."""

from hashlib import sha256

from patch_hud import BASE_SHA as AMBIENT_AUDIO_SHA


SUPPORTED_SHA = {
    AMBIENT_AUDIO_SHA,
    "55360782af2425c30f1a570ef7e72051384a8da1a4a91d8a50615bb6c08154d7",
}
RET = bytes.fromhex("C0 03 5F D6")
NOP = bytes.fromhex("1F 20 03 D5")
CMP_W0_ZERO = bytes.fromhex("1F 00 00 71")
PATCHES = (
    (0x270DC0, bytes.fromhex("FF 43 02 D1 ED 33 01 6D EB 2B 02 6D"), RET),
    (0x271FA4, bytes.fromhex("FF 03 03 D1 ED 33 03 6D EB 2B 04 6D"), RET),
    (0x246C38, bytes.fromhex("F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9"), RET),
    (0x159B20, bytes.fromhex("73 7F 0B 94 00 00 80 52 F4 06 00 94"), NOP),
    (0x3BF784, bytes.fromhex("1F 08 00 71 61 53 00 54 68 A2 42 F9"), CMP_W0_ZERO),
    (0x2733D8, bytes.fromhex("1F 08 00 71 E1 16 00 54 68 72 A3 39"), CMP_W0_ZERO),
)


def patch_aim(binary):
    if sha256(binary).hexdigest() not in SUPPORTED_SHA:
        raise ValueError("Aim patches require the verified AmbientAudio executable, optionally with HUD")
    if binary[0x1CBD48:0x1CBD50] != bytes.fromhex("00 00 80 52 C0 03 5F D6"):
        raise ValueError("Aim Indicator requires the existing iOS GetInputType patch")

    for offset, expected, _ in PATCHES:
        if binary[offset:offset + len(expected)] != expected:
            raise ValueError(f"Unexpected targeting instructions at {offset:#x}")

    patched = bytearray(binary)
    for offset, _, replacement in PATCHES:
        patched[offset:offset + len(replacement)] = replacement
    return bytes(patched)
