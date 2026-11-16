"""Apply the CCamera::Fade RET patch to the verified GTA iOS executable."""

from hashlib import sha256
BASE_SHA = "7352fe50bb883051a508246fe6b6d667bec4dbb1c67f85de6917275c5c0384e7"
FADE = 0x13524C
PROLOGUE = bytes.fromhex("29008052 095C0139 01B81879 00300CBD")
RET = bytes.fromhex("C0035FD6")


def patch_spawn_fade(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Spawn fade requires the verified GTA iOS Widget Player Info executable")

    if binary[FADE:FADE + len(PROLOGUE)] != PROLOGUE:
        raise ValueError("Unexpected CCamera::Fade prologue")

    return binary[:FADE] + RET + binary[FADE + len(RET):]
