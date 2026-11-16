"""Bind the two verified GTA iOS CPad aim methods to Morgan's gyro hooks."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "8e46cee1063913c88a5a3f60091f6bc8f698178f1c3e82f58d9721d416d59380"
THUNK = 0x497588
SLOT = 0x4B54C8
HOOKS = (
    (0x15FA84, bytes.fromhex("FF4301D1 E923016D F65702A9"), b"_CPad_AimWeaponLeftRight_hook"),
    (0x15FE44, bytes.fromhex("FF0301D1 F65701A9 F44F02A9"), b"_CPad_AimWeaponUpDown_hook"),
)


def patch_gyro(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Gyro requires the verified GTA iOS executable with the Physical and World Player Slots patches")

    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + 48 <= 0x498000 and not any(binary[THUNK:THUNK + 48])
    assert binary[0xA08:0xA18].rstrip(b"\0") == b"__mg_hud"
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    assert hud_offset + hud_size == SLOT and not any(binary[SLOT:SLOT + 16])
    data_const_file = unpack_from("<Q", binary, 0x790 + 40)[0]

    bind_start, bind_size, weak_start = unpack_from("<III", binary, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = binary[bind_start:bind_end].rstrip(b"\0")
    for index, (_, _, symbol) in enumerate(HOOKS):
        slot = SLOT + index * 8
        table += b"\x20\x30\x40" + symbol + b"\x00\x51\x60\x00"
        table += b"\x72" + uleb(slot - data_const_file) + b"\x90"
    table += b"\x00"
    table += bytes((bind_size - len(table)) % 16)

    growth = len(table) - bind_size
    link_vm_size, link_offset, link_size = unpack_from("<QQQ", binary, 0xF70)
    assert growth > 0 and link_offset + link_size == len(binary) and link_size + growth <= link_vm_size
    image = bytearray(binary)
    image[bind_start:bind_end] = table
    pack_into("<I", image, 0xFAC, len(table))
    pack_into("<Q", image, 0xF80, link_size + growth)
    for field in (0xFB0, 0xFB8, 0xFC0, 0xFD0, 0xFD8, 0x1000, 0x1008, 0x1010, 0x1018, 0x1020, 0x1028, 0x1E28, 0x1E38, 0x1E48):
        offset = unpack_from("<I", binary, field)[0]
        if offset:
            assert bind_end <= offset <= link_offset + link_size
            pack_into("<I", image, field, offset + growth)

    pack_into("<Q", image, 0x768, text_size + 48)
    pack_into("<Q", image, 0xA30, hud_size + 16)
    for index, (site, prologue, _) in enumerate(HOOKS):
        thunk = THUNK + index * 24
        slot = SLOT + index * 8
        assert binary[site:site + len(prologue)] == prologue
        pack_into("<I", image, site, branch(site, thunk))
        # Pass the stock trampoline as x3. Keep x0–x2 and the caller's LR.
        pack_into("<6I", image, thunk,
                  0x10000083,  # adr x3, thunk + 16
                  adrp(16, thunk + 4, slot),
                  0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200,  # br x16
                  unpack_from("<I", prologue)[0],
                  branch(thunk + 20, site + 4))
    return bytes(image)
