"""Port Thunder's movement-group and FollowPedSA head-look patches to GTA iOS."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch, branch_cond
from patch_render_title import adrp


BASE_SHA = "b96b83d91a0a870f914812b8adf4e896472eee0413cc8bdcbd4bcd7f12cd7d23"
CALLSITE = 0x26FCB8
THUNK = 0x497554
FLAG = 0xB0FCF8
STORE_GROUP = 0xB9063268  # str w8, [x19, #0x630]
HEAD_MOVE_CALL = 0x11F134
HEAD_MOVE_ORIGINAL = 0x2D1F94
NOP = 0xD503201F


def patch_cj_walk(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CJ walk requires the verified GTA iOS executable with Clothes Rebuild and World Player Slots")

    assert unpack_from("<I", binary, CALLSITE)[0] == STORE_GROUP
    assert binary[CALLSITE - 4:CALLSITE] == bytes.fromhex("80 00 00 54")
    assert binary[CALLSITE + 4:CALLSITE + 12] == bytes.fromhex("E0 03 13 AA DB 01 00 94")
    assert unpack_from("<I", binary, HEAD_MOVE_CALL)[0] == branch(HEAD_MOVE_CALL, HEAD_MOVE_ORIGINAL, 0x94000000)
    assert binary[HEAD_MOVE_CALL - 0x1C:HEAD_MOVE_CALL - 0x18] == bytes.fromhex("21 A4 21 91")

    # Reuse the executable padding after the World Player Slots trampoline.
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + 20 <= 0x498000 and not any(binary[THUNK:THUNK + 20])

    # Reserve one zero-initialized byte in the existing writable __DATA segment.
    assert binary[0xA60:0xA70].rstrip(b"\0") == b"__DATA"
    assert binary[0xEB0:0xEC0].rstrip(b"\0") == b"__common"
    data_address, data_size = unpack_from("<QQ", binary, 0xA58 + 24)
    common_address, common_size, common_offset = unpack_from("<QQI", binary, 0xEB0 + 32)
    assert common_offset == 0 and common_address + common_size == BASE + FLAG
    assert BASE + FLAG + 1 <= data_address + data_size

    image = bytearray(binary)
    pack_into("<Q", image, 0x768, text_size + 20)
    pack_into("<Q", image, 0xEB0 + 40, common_size + 1)
    pack_into("<I", image, CALLSITE, branch(CALLSITE, THUNK))
    # x9 is dead at CALLSITE. CBNZ leaves the original NZCV flags intact.
    pack_into("<5I", image, THUNK,
              adrp(9, THUNK, FLAG),
              0x39400129 | ((FLAG & 0xFFF) << 10),  # ldrb w9, [x9, #flag offset]
              branch_cond(THUNK + 8, THUNK + 16, 0x35000009),
              STORE_GROUP,
              branch(THUNK + 16, CALLSITE + 4))

    # Skip only the LookAt call issued by FollowPedSA.
    pack_into("<I", image, HEAD_MOVE_CALL, NOP)
    return bytes(image)
