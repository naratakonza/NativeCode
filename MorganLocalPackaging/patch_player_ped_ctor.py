"""Flip the player group branch in the verified GTA SA iOS constructor."""

from hashlib import sha256

BASE_SHA = "95689a569eebebc2c9ab54bcb1e009d9c59c1810ec8dbad1734722f7cc745fca"
BRANCH = 0x26CBF4
CONTEXT = bytes.fromhex("A01000D0 00802891 7655F797 54050037 BFC5FF97 68A242F9 004D00B9")
TARGET = 0x26CC9C
TARGET_BYTES = bytes.fromhex("20018052 D235FC97 60B206BD")
TBZ = bytes.fromhex("54050036")


def patch_player_ped_ctor(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Player ped constructor patch requires the verified GTA SA iOS SpawnFade executable")

    start = BRANCH - 12
    if binary[start:start + len(CONTEXT)] != CONTEXT or binary[TARGET:TARGET + len(TARGET_BYTES)] != TARGET_BYTES:
        raise ValueError("Unexpected CPlayerPed constructor branch or target")

    return binary[:BRANCH] + TBZ + binary[BRANCH + len(TBZ):]
