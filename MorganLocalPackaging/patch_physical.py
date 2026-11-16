"""Redirect GTA iOS CPhysical::ProcessControl at SA-MP startup."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch, branch_cond
from patch_render_title import adrp, uleb


BASE_SHA = "ceea9c59fe71a3a2a8ed5a8e13f5e8fa2b5a8f46391e6b5e7c494be082cb8325"
PROCESS_CONTROL = 0x162788
ORIGINAL = 0x6DBD23E9  # stp d9, d8, [sp, #-0x30]!
THUNK = 0x497568
FLAG = 0xB0FCF9
SLOT = 0x4B54C0
SYMBOL = b"__ZN9CPhysical14ProcessControlEP12CPhysicalGta"


def patch_physical(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CPhysical requires the verified World Player Slots, Clothes Rebuild, CJ walk and HeadMove executable")

    assert unpack_from("<I", binary, PROCESS_CONTROL)[0] == ORIGINAL
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + 32 <= 0x498000 and not any(binary[THUNK:THUNK + 32])

    assert binary[0xEB0:0xEC0].rstrip(b"\0") == b"__common"
    common_address, common_size, common_offset = unpack_from("<QQI", binary, 0xED0)
    data_address, data_size = unpack_from("<QQ", binary, 0xA58 + 24)
    assert common_offset == 0 and common_address + common_size == BASE + FLAG
    assert BASE + FLAG + 1 <= data_address + data_size

    assert binary[0xA08:0xA18].rstrip(b"\0") == b"__mg_hud"
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    assert hud_offset + hud_size == SLOT and not any(binary[SLOT:SLOT + 8])
    assert SLOT + 8 <= 0x4B8000
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
    pack_into("<Q", image, 0xED8, common_size + 1)
    pack_into("<I", image, PROCESS_CONTROL, branch(PROCESS_CONTROL, THUNK))
    # Preserve x0 and LR. Before Initialise(), replay the displaced stock instruction.
    pack_into("<8I", image, THUNK,
              adrp(16, THUNK, FLAG),
              0x39400210 | ((FLAG & 0xFFF) << 10),
              branch_cond(THUNK + 8, THUNK + 24, 0x34000010),
              adrp(16, THUNK + 12, SLOT),
              0xF9400210 | (((SLOT & 0xFFF) // 8) << 10),
              0xD61F0200,
              ORIGINAL,
              branch(THUNK + 28, PROCESS_CONTROL + 4))
    return bytes(image)
