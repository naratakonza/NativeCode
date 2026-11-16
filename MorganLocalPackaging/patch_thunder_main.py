"""Apply the three verified Thunder main patches to a copy of GTA iOS."""

from hashlib import sha256
from struct import pack

from patch_cd_stream import branch


BASE_SHA = "ccb5210aa2c66998a64478e5ed8854ff2463767a0e525b22587c4f6a9c5e8788"
SKY_POLYS = 0x467718
PLAYER_WEAPON = 0x3BFB20
PLAYER_WEAPON_ANIMATION = 0x3C033C
FRAME_HORIZON = 0x1593D0
FRAME_HORIZON_CLEAR = 0x159440


def patch_thunder_main(binary, player_weapon=True):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Thunder main patches require the verified CDraw executable")

    patches = (
        (SKY_POLYS, bytes.fromhex("3333B3BF3333B33F"), pack("<2f", -10.0, 10.0)),
        (FRAME_HORIZON, bytes.fromhex("00008012"), pack("<I", branch(FRAME_HORIZON, FRAME_HORIZON_CLEAR))),
    )
    if player_weapon:
        patches += ((PLAYER_WEAPON, bytes.fromhex("190080D2"), pack("<I", branch(PLAYER_WEAPON, PLAYER_WEAPON_ANIMATION))),)
    for offset, expected, _ in patches:
        if binary[offset:offset + len(expected)] != expected:
            raise ValueError(f"Unexpected original bytes at GTA offset 0x{offset:X}")

    image = bytearray(binary)
    for offset, _, replacement in patches:
        image[offset:offset + len(replacement)] = replacement
    return bytes(image)
