"""Replace only the synchronous DAT read and decode block in LoadFullTexture."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "d7c0469d73f63187ad6723c1926e3e5e44f3dd79dc842347c5134418f84b17aa"
SITE = 0x2BE7BC
THUNK = 0x497624
SLOT = 0x4B5500
SUCCESS = 0x2BE8E8
RETURN_ZERO = 0x2BE958
SYMBOL = b"_LoadFullTexture_hook"


def patch_texture_load(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Texture load fix requires the verified HUD and AimPatches executable from IPA c0d03c0")

    assert unpack_from("<I", binary, SITE)[0] == branch(SITE, 0x034A00, 0x94000000)
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + 32 <= 0x498000 and not any(binary[THUNK:THUNK + 32])
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

    pack_into("<Q", image, 0x768, text_size + 32)
    pack_into("<Q", image, 0xA30, hud_size + 8)
    pack_into("<I", image, SITE, branch(SITE, THUNK))
    # x0 is the iOS file handle and x1 points to the stack texture descriptor.
    # On failure the stock epilogue checks the canary and multiplies w8 by w9.
    pack_into("<8I", image, THUNK,
              adrp(16, THUNK, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10),
              0xD63F0200,                      # blr x16
              0x36000040,                      # tbz w0, #0, thunk + 20
              branch(THUNK + 16, SUCCESS),
              0x2A1F03E8, 0x2A1F03E9,         # mov w8, wzr; mov w9, wzr
              branch(THUNK + 28, RETURN_ZERO))
    return bytes(image)
