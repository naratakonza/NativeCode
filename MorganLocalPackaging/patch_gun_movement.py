"""Apply Thunder's aim movement constants to the verified GTA SA iOS executable."""

from hashlib import sha256

from patch_player_ped_ctor import BASE_SHA

SUPPORTED_SHA = {BASE_SHA, "ff9abea335af1c4a5aa51290775b104456cdb2dcb2ac674d13a193198adf0e61"}
PATCHES = (
    (0x34944C, bytes.fromhex("41 C0 20 1E 00 28 21 1E A8 99 99 52 88 B9 A7 72 01 01 27 1E 00 20 21 1E"),
     bytes.fromhex("E8 51 98 52 A8 8E A7 72")),
    (0x349940, bytes.fromhex("08 E1 28 91 00 01 40 BD 28 85 8B 52 E8 B1 A7 72 01 01 27 1E 00 08 21 1E 22 04 40 BD"),
     bytes.fromhex("08 00 A8 52 1F 20 03 D5")),
    (0x34AB2C, bytes.fromhex("08 E1 28 91 00 01 40 BD 28 85 8B 52 E8 B1 A7 72 01 01 27 1E 00 08 21 1E A3 22 40 BD"),
     bytes.fromhex("08 00 A8 52 1F 20 03 D5")),
)


def patch_gun_movement(binary):
    if sha256(binary).hexdigest() not in SUPPORTED_SHA:
        raise ValueError("Gun movement requires the verified GTA SA iOS SpawnFade executable")

    for offset, context, _ in PATCHES:
        start = offset - 8
        if binary[start:start + len(context)] != context:
            raise ValueError(f"Unexpected gun movement instructions at {offset:#x}")

    patched = bytearray(binary)
    for offset, _, replacement in PATCHES:
        patched[offset:offset + len(replacement)] = replacement
    return bytes(patched)
