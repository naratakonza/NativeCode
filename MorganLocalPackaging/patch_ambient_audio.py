"""Redirect the GTA SA iOS weather-audio vtable entry to Morgan."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE
from patch_gun_movement import SUPPORTED_SHA as SPAWN_FADE_SHA
from patch_render_title import adrp, uleb


SUPPORTED_SHA = SPAWN_FADE_SHA | {
    "6597c46b769ce4f4d2f14042ca4196dec3be560a615c82302d7d8221576cf83d",
    "3bd55c5669364caea189c896aa35b6338c92d9537c1df3b69fd2df6c49c5ed6b",
}
ORIGINAL = 0x07AF7C
VTABLE_ENTRY = 0x49A430
THUNK = 0x4975E8
SLOT = 0x4B54E8
SYMBOL = b"_CAmbientAuidoTrack_Process_hook"


def patch_ambient_audio(binary):
    if sha256(binary).hexdigest() not in SUPPORTED_SHA:
        raise ValueError("Ambient audio requires a verified GTA SA iOS SpawnFade executable")

    assert unpack_from("<Q", binary, VTABLE_ENTRY)[0] == BASE + ORIGINAL
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + 20 <= 0x498000 and not any(binary[THUNK:THUNK + 20])
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

    pack_into("<Q", image, 0x768, text_size + 20)
    pack_into("<Q", image, 0xA30, hud_size + 8)
    # The existing dyld rebase still applies to this vtable entry.
    pack_into("<Q", image, VTABLE_ENTRY, BASE + THUNK)
    # Preserve the original arguments and LR. Pass the original method in x3.
    pack_into("<5I", image, THUNK,
              adrp(3, THUNK, ORIGINAL), 0x91000063 | ((ORIGINAL & 0xFFF) << 10),
              adrp(16, THUNK + 8, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10),
              0xD61F0200)
    return bytes(image)
