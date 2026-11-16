"""Route the verified iOS gameplay camera call to CDraw in MorganLauncher."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import branch
from patch_render_title import adrp, uleb


BASE_SHA = "75e4d054e2eace116baf29e42b4259dfc0c755a6e7f20e8e98eddf99f6313dce"
SET_FOV = 0x2D7C70
CALLSITE = 0x13727C
THUNK = 0x497280
SLOT = 0x4B54A8
SYMBOL = b"_CDraw_SetFov_hook"


def patch_cdraw(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CDraw requires the verified CameraSize executable")

    image = bytearray(binary)
    # Offsets into the verified Mach-O load commands and classic dyld bind stream.
    for section_size, start, length, old_size in ((0x768, THUNK, 12, 0x280), (0xA30, SLOT, 8, 0xA8)):
        assert unpack_from("<Q", binary, section_size)[0] == old_size
        assert binary[start:start + length] == bytes(length)
        pack_into("<Q", image, section_size, old_size + length)

    bind_end = 0x4EF16F
    record = b"\x20" + uleb(48) + b"\x40" + SYMBOL + b"\x00\x51\x60\x00\x72" + uleb(SLOT - 0x498000) + b"\x90"
    assert len(record) < 33 and binary[bind_end:bind_end + 33] == bytes(33)
    image[bind_end:bind_end + len(record)] = record

    # ARM64 passes the float in s0 and the bool in w0. Keep both and the original LR.
    pack_into("<3I", image, THUNK, adrp(16, THUNK, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10), 0xD61F0200)
    assert unpack_from("<I", binary, CALLSITE)[0] == branch(CALLSITE, SET_FOV, 0x94000000)
    pack_into("<I", image, CALLSITE, branch(CALLSITE, THUNK, 0x94000000))
    return bytes(image)
