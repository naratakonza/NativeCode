"""Pass GTA's world coordinates to the existing iOS radar drawing hooks."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch


BASE_SHA = "d72cdada712dc0d4bc835ea404f8e12ac00d0410bbf8350e4cfb1cd28a2e6278"
THUNKS = 0x49723C
SYMBOLS = (
    b"_CRadar_DrawRadarSprite_hook",
    b"_CRadar_DrawRotatingRadarSprite_hook",
    b"_CRadar_ShowRadarTraceWithHeight_hook",
)


def patch_radar_blips(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Radar bridges require the verified GTA iOS 2.2.21 image with the CD streaming repair")

    # The original bridges still set the original function argument. Set the next
    # integer argument to the resolved world XY or NULL for non-trace HUD sprites.
    # DrawCoordBlip places its world position at SP+0x20, DrawEntityBlip at SP+0x58.
    bridges = (
        (0x497078, 3, None, (0x1F08E4,)),
        (0x49708C, 3, None, (0x1F0EEC, 0x1F1130, 0x1F133C)),
        (0x497078, 3, 0x20, (0x1F2118,)),
        (0x4970A0, 7, 0x20, (0x1F2204,)),
        (0x497078, 3, 0x58, (0x1F2AF8,)),
        (0x4970A0, 7, 0x58, (0x1F2DCC, 0x1F3078, 0x1F30BC)),
        (0x49708C, 3, 0x58, (0x1F2E60, 0x1F2F8C, 0x1F3174)),
    )
    end = THUNKS + len(bridges) * 8
    assert not any(binary[THUNKS:end])
    image = bytearray(binary)

    cursor = 32
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<II", binary, cursor)
        if command == 0x19 and binary[cursor + 8:cursor + 24].rstrip(b"\0") == b"__TEXT":
            break
        cursor += size
    else:
        raise ValueError("Missing GTA __TEXT")

    assert end <= unpack_from("<Q", binary, cursor + 48)[0]
    thunk_section = None
    for i in range(unpack_from("<I", binary, cursor + 64)[0]):
        section = cursor + 72 + i * 80
        name = binary[section:section + 16].rstrip(b"\0")
        address, size, offset = unpack_from("<QQI", binary, section + 32)
        assert address + size <= BASE + THUNKS or address >= BASE + end
        if name == b"__mg_gameproc":
            assert address == BASE + offset and offset + size == THUNKS
            thunk_section = section + 40, end - offset
    assert thunk_section is not None
    pack_into("<Q", image, *thunk_section)

    for i, (target, register, stack_offset, sites) in enumerate(bridges):
        thunk = THUNKS + i * 8
        position = 0xD2800000 | register if stack_offset is None else 0x910003E0 | (stack_offset << 10) | register
        pack_into("<II", image, thunk, position, branch(thunk + 4, target))
        for site in sites:
            assert unpack_from("<I", binary, site)[0] == branch(site, target, 0x94000000)
            pack_into("<I", image, site, branch(site, thunk, 0x94000000))

    return bytes(image)
