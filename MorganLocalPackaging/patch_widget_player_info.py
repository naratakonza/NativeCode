"""Redirect the verified GTA iOS armor bar to CWidgetPlayerInfo."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "e767a83a2ccdce88e2e2f5d1e1d683c84b4ceb30d4ed0fffd8b701f89916badb"
SITE = 0x20931C
PROLOGUE = bytes.fromhex("FF4301D1 ED33016D EB2B026D")
THUNK = 0x4975D0
SLOT = 0x4B54E0
SYMBOL = b"_CWidgetPlayerInfo_RenderArmorBar_hook"


def patch_widget_player_info(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CWidgetPlayerInfo requires the verified GTA iOS executable with the CStats patch")

    assert binary[SITE:SITE + len(PROLOGUE)] == PROLOGUE
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + 24 <= 0x498000 and not any(binary[THUNK:THUNK + 24])
    assert binary[0xA08:0xA18].rstrip(b"\0") == b"__mg_hud"
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    assert hud_offset + hud_size == SLOT and not any(binary[SLOT:SLOT + 8])
    data_const_file = unpack_from("<Q", binary, 0x790 + 40)[0]

    bind_start, bind_size, weak_start = unpack_from("<III", binary, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = binary[bind_start:bind_end].rstrip(b"\0")
    table += b"\x20\x30\x40" + SYMBOL + b"\x00\x51\x60\x00"
    table += b"\x72" + uleb(SLOT - data_const_file) + b"\x90\x00"
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

    pack_into("<Q", image, 0x768, text_size + 24)
    pack_into("<Q", image, 0xA30, hud_size + 8)
    pack_into("<I", image, SITE, branch(SITE, THUNK))
    # Keep x0, w1, s0 and LR. Pass the stock trampoline in x2.
    pack_into("<6I", image, THUNK,
              0x10000082,  # adr x2, thunk + 16
              adrp(16, THUNK + 4, SLOT),
              0xF9400210 | (((SLOT & 0xFFF) // 8) << 10),
              0xD61F0200,  # br x16
              unpack_from("<I", PROLOGUE)[0],
              branch(THUNK + 20, SITE + 4))
    return bytes(image)
