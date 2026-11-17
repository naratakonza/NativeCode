"""Bind GTA SA iOS widget methods to the Morgan CWidgets hooks."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "0da22d60bba1ff82acf8d2b187b557d88d2eca9ca2d0cae7a69aaff523c9f312"
THUNK = 0x497644
SLOT = 0x4B5508
HOOKS = (
    (0x1FF208, bytes.fromhex("FF8301D1 E923036D F44F04A9"), b"_CWidgetButton_Draw_hook", 1),
    (0x1FD8A0, bytes.fromhex("FF4301D1 E923026D F44F03A9"), b"_CWidget_Update_hook", 1),
    (0x1FEBAC, bytes.fromhex("01640139 C0035FD6"), b"_CWidget_SetEnabled_hook", 2),
    (0x1FECC0, bytes.fromhex("08644139 0060413D 00D8617E"), b"_CWidget_ManageAlpha_hook", 1),
)


def patch_widgets(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CWidgets requires the verified GTA iOS executable from IPA d951f76")

    thunk_size = 24 * len(HOOKS)
    slot_size = 8 * len(HOOKS)
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + thunk_size <= 0x498000 and not any(binary[THUNK:THUNK + thunk_size])
    assert binary[0xA08:0xA18].rstrip(b"\0") == b"__mg_hud"
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    assert hud_offset + hud_size == SLOT and SLOT + slot_size <= 0x4B8000
    assert not any(binary[SLOT:SLOT + slot_size])
    data_const_file = unpack_from("<Q", binary, 0x790 + 40)[0]

    bind_start, bind_size, weak_start = unpack_from("<III", binary, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = binary[bind_start:bind_end].rstrip(b"\0")
    for index, (_, _, symbol, _) in enumerate(HOOKS):
        table += b"\x20\x30\x40" + symbol + b"\x00\x51\x60\x00"
        table += b"\x72" + uleb(SLOT + index * 8 - data_const_file) + b"\x90"
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

    pack_into("<Q", image, 0x768, text_size + thunk_size)
    pack_into("<Q", image, 0xA30, hud_size + slot_size)
    for index, (site, prologue, _, register) in enumerate(HOOKS):
        thunk = THUNK + index * 24
        slot = SLOT + index * 8
        assert binary[site:site + len(prologue)] == prologue
        pack_into("<I", image, site, branch(site, thunk))
        # Pass the original trampoline in x1, or x2 when w1 holds bEnabled.
        pack_into("<6I", image, thunk,
                  0x10000080 | register,
                  adrp(16, thunk + 4, slot),
                  0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200,
                  unpack_from("<I", prologue)[0],
                  branch(thunk + 20, site + 4))
    return bytes(image)
