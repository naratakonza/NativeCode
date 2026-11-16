"""Bind the stock GTA SA iOS crosshair and player-info draw hooks."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "83d6b4d3d5cf233c82fe058c44e9f5fae7621a04528bc31d6927fe86e0309bd9"
CROSSHAIR_CALL = 0x218378
CROSSHAIR_DRAW = 0x215334
PLAYER_INFO_VTABLE_ENTRY = 0x4A3EB8
PLAYER_INFO_DRAW = 0x208888
THUNK = 0x4975FC
SLOT = 0x4B54F0
HOOKS = (
    (b"_CHud_DrawCrosshairs_hook", CROSSHAIR_DRAW, 0),
    (b"_CWidgetPlayerInfo_Draw_hook", PLAYER_INFO_DRAW, 1),
)


def patch_hud(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("HUD requires the verified AmbientAudio executable from IPA f221753")

    assert unpack_from("<I", binary, CROSSHAIR_CALL)[0] == branch(CROSSHAIR_CALL, CROSSHAIR_DRAW, 0x94000000)
    assert unpack_from("<Q", binary, PLAYER_INFO_VTABLE_ENTRY)[0] == BASE + PLAYER_INFO_DRAW
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    thunk_size = 20 * len(HOOKS)
    assert THUNK + thunk_size <= 0x498000 and not any(binary[THUNK:THUNK + thunk_size])

    assert binary[0xA08:0xA18].rstrip(b"\0") == b"__mg_hud"
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    slot_size = 8 * len(HOOKS)
    assert hud_offset + hud_size == SLOT and not any(binary[SLOT:SLOT + slot_size])
    data_const_file = unpack_from("<Q", binary, 0x790 + 40)[0]

    bind_start, bind_size, weak_start = unpack_from("<III", binary, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = binary[bind_start:bind_end].rstrip(b"\0")
    for index, (symbol, _, _) in enumerate(HOOKS):
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
    pack_into("<I", image, CROSSHAIR_CALL, branch(CROSSHAIR_CALL, THUNK, 0x94000000))
    # This pointer retains its original dyld rebase.
    pack_into("<Q", image, PLAYER_INFO_VTABLE_ENTRY, BASE + THUNK + 20)
    for index, (_, original, reg) in enumerate(HOOKS):
        thunk = THUNK + index * 20
        slot = SLOT + index * 8
        # Preserve LR and pass the original in x0 for crosshair, x1 for Player Info.
        pack_into("<5I", image, thunk,
                  adrp(reg, thunk, original), 0x91000000 | ((original & 0xFFF) << 10) | (reg << 5) | reg,
                  adrp(16, thunk + 8, slot), 0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200)
    return bytes(image)
