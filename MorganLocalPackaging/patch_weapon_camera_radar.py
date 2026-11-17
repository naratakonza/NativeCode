"""Apply the verified sniper, resurrection-camera and GPS marker NOPs."""

from hashlib import sha256


BASE_SHA = "db088a5cc73809ac813a688326b8b230a04f1222735fec199e78667d345c2742"
NOP = bytes.fromhex("1F2003D5")
PATCHES = (
    (0x4334DC, bytes.fromhex("8CF2F397")),  # FireSniper -> CamShakeNoPos
    (0x0B7AB4, bytes.fromhex("EDF40194")),  # Resurrection -> RestoreWithJumpCut
    (0x1F21E0, bytes.fromhex("B3420494")),  # DrawCoordBlip -> sprite Draw
    (0x1F2DA0, bytes.fromhex("C33F0494")),  # DrawEntityBlip -> sprite Draw
)


def patch_weapon_camera_radar(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Weapon/camera/radar patches require the verified Pickup executable")

    for offset, expected in PATCHES:
        if binary[offset:offset + 4] != expected:
            raise ValueError(f"Unexpected instruction at GTA offset 0x{offset:X}")

    image = bytearray(binary)
    for offset, _ in PATCHES:
        image[offset:offset + 4] = NOP
    return bytes(image)
