"""Route gameplay CameraSize calls to Morgan on the verified GTA iOS image."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "0782ef989a92dbbb6d468d437db564c74abe2311473807a9f4adc6a52ca75e9e"
CAMERA_SIZE = 0x315498
CALLSITES = (0x1593C4, 0x15A220)
THUNK = 0x497274
SLOT = 0x4B54A0
SYMBOL = b"_CameraSize_hook"


def patch_camera_size(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CameraSize requires the verified IMG Read Fix executable")

    segments = []
    dylibs = []
    cursor = 32
    bind_info = None
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<II", binary, cursor)
        if command == 0x19:
            name = binary[cursor + 8:cursor + 24].rstrip(b"\0")
            segments.append((name, cursor))
        elif command in (0xC, 0x80000018, 0x8000001F, 0x23, 0x80000023):
            name = cursor + unpack_from("<I", binary, cursor + 8)[0]
            dylibs.append(binary[name:cursor + size].split(b"\0", 1)[0])
        elif command == 0x80000022:
            bind_info = unpack_from("<II", binary, cursor + 16)
        cursor += size

    image = bytearray(binary)

    def extend_section(segment_name, section_name, start, length, protection):
        segment = dict(segments)[segment_name]
        vmaddr, _, fileoff, filesize, _, prot = unpack_from("<4Q2I", binary, segment + 24)
        assert vmaddr == BASE + fileoff and prot == protection
        assert fileoff <= start <= fileoff + filesize - length
        assert not any(binary[start:start + length])
        for index in range(unpack_from("<I", binary, segment + 64)[0]):
            off = segment + 72 + index * 80
            name = binary[off:off + 16].rstrip(b"\0")
            address, size, offset = unpack_from("<QQI", binary, off + 32)
            if name == section_name:
                assert address == BASE + offset and offset + size == start
                pack_into("<Q", image, off + 40, size + length)
            else:
                assert address + size <= BASE + start or address >= BASE + start + length
        return fileoff

    extend_section(b"__TEXT", b"__mg_gameproc", THUNK, 12, 5)
    fileoff = extend_section(b"__DATA_CONST", b"__mg_hud", SLOT, 8, 3)
    segment_index = next(i for i, (name, _) in enumerate(segments) if name == b"__DATA_CONST")
    ordinal = dylibs.index(b"@rpath/MorganLauncher.framework/MorganLauncher") + 1
    assert segment_index == 2 and ordinal == 48
    bind, size = bind_info
    table = binary[bind:bind + size].rstrip(b"\0")
    assert table.endswith(b"_MGTimerEndUserPause\0\x51\x60\x00\x72\x98\xa9\x07\x90")
    record = b"\x20" + uleb(ordinal) + b"\x40" + SYMBOL + b"\x00\x51\x60\x00" + bytes([0x70 + segment_index]) + uleb(SLOT - fileoff) + b"\x90"
    assert len(table) + len(record) < size
    image[bind + len(table):bind + len(table) + len(record)] = record

    # Keep x0/x1, s0/s1 and the caller's LR. Only x16 is scratch.
    pack_into("<3I", image, THUNK, adrp(16, THUNK, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10), 0xD61F0200)
    assert binary[CAMERA_SIZE:CAMERA_SIZE + 4] == bytes.fromhex("80 08 00 B4")
    for site in CALLSITES:
        assert unpack_from("<I", binary, site)[0] == branch(site, CAMERA_SIZE, 0x94000000)
        pack_into("<I", image, site, branch(site, THUNK, 0x94000000))

    return bytes(image)
