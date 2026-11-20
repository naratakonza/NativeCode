"""Port Thunder's four RemoveWeaponWhenEnteringVehicle patches to iOS 2.2.21."""

from hashlib import sha256


BASE_SHA = {"d14833a5229535ce554535c496bd2d5ece52dd64fa7f43b991da9e16a285837b"}
NOP = bytes.fromhex("1F2003D5")
PATCHES = (
    (0x24353C, 3, 0x243538, bytes.fromhex("E00313AA88C20094083C46398806003468B247B9")),
    (0x37AF04, 1, 0x37AEFC, bytes.fromhex("E00314AA010080527D21FB97681900B0")),
    (0x37C848, 1, 0x37C840, bytes.fromhex("E00313AA010080522C1BFB97080340F9")),
    (0x37BF48, 1, 0x37BF40, bytes.fromhex("E00314AA010080526C1DFB97681240F9")),
)


def patch_vehicle_weapon(binary):
    normalized = bytearray(binary)
    for site, count, start, expected in PATCHES:
        offset = site - start
        patched = expected[:offset] + NOP * count + expected[offset + count * 4:]
        if binary[start:start + len(expected)] not in (expected, patched):
            raise ValueError(f"Unexpected vehicle-weapon instructions at {site:#x}")
        normalized[site:site + count * 4] = expected[offset:offset + count * 4]
    if sha256(normalized).hexdigest() not in BASE_SHA:
        raise ValueError("Vehicle weapon patch requires the verified f2ed210 executable")

    image = bytearray(binary)
    for site, count, _, _ in PATCHES:
        image[site:site + count * 4] = NOP * count
    return bytes(image)
