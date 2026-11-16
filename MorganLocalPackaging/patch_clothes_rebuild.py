"""Route the verified GTA iOS clothes rebuild calls through MorganLauncher."""

from hashlib import sha256
from struct import pack, pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "36ce7a0f06caddb1c357a95089fee83c45381527fa7bd164f34668063af36754"
ORIGINAL = 0x86450
TAIL_CALLS = (0x86D4C, 0xADE80, 0xADEC8, 0xADF28)
CALLS = (0xAE154, 0xAE428, 0xAE8E4, 0xAEAF4, 0xB7B2C, 0xB85C4, 0xB8884, 0xB8C08, 0xE2D6C, 0x31A47C)
THUNK = 0x49728C
SLOT = 0x4B54B0
SYMBOL = b"_CClothes_RebuildPlayer_hook"


def patch_clothes_rebuild(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("Clothes rebuild requires the verified GTA iOS executable from the Sensitivity test IPA")

    commands = []
    cursor = 32
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<II", binary, cursor)
        commands.append((command, cursor, size))
        cursor += size
    segments = {
        binary[offset + 8:offset + 24].rstrip(b"\0"): offset
        for command, offset, _ in commands if command == 0x19
    }
    dylibs = [
        binary[offset + unpack_from("<I", binary, offset + 8)[0]:offset + size].split(b"\0", 1)[0]
        for command, offset, size in commands if command in (0xC, 0x80000018, 0x8000001F, 0x23, 0x80000023)
    ]
    ordinal = dylibs.index(b"@rpath/MorganLauncher.framework/MorganLauncher") + 1
    data_index = list(segments).index(b"__DATA_CONST")
    data_file_offset = unpack_from("<Q", binary, segments[b"__DATA_CONST"] + 40)[0]

    info = next(offset for command, offset, _ in commands if command == 0x80000022)
    bind_offset, bind_size, weak_offset = unpack_from("<III", binary, info + 16)
    bind_end = bind_offset + bind_size
    assert bind_end == weak_offset
    table = binary[bind_offset:bind_end].rstrip(b"\0")
    assert table.endswith(b"_CDraw_SetFov_hook\0\x51\x60\x00\x72\xa8\xa9\x07\x90")
    table += b"\x20" + uleb(ordinal) + b"\x40" + SYMBOL + b"\x00\x51\x60\x00"
    table += bytes([0x70 + data_index]) + uleb(SLOT - data_file_offset) + b"\x90\x00"
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size

    link = segments[b"__LINKEDIT"]
    link_vm_size, link_off, link_size = unpack_from("<QQQ", binary, link + 32)
    assert 0 < growth and link_off + link_size == len(binary) and link_size + growth <= link_vm_size
    image = bytearray(binary[:bind_offset] + table + binary[bind_end:])
    pack_into("<Q", image, link + 48, link_size + growth)
    pack_into("<I", image, info + 20, len(table))

    linkedit_offsets = {
        0x80000022: (24, 32, 40),
        0x2: (8, 16),
        0xB: (32, 40, 48, 56, 64, 72),
        0x1D: (8,),
        0x26: (8,),
        0x29: (8,),
    }
    for command, offset, _ in commands:
        for field in linkedit_offsets.get(command, ()):
            value = unpack_from("<I", binary, offset + field)[0]
            if value:
                assert bind_end <= value <= len(binary)
                pack_into("<I", image, offset + field, value + growth)

    for segment, section, start, code in (
        (b"__TEXT", b"__mg_gameproc", THUNK, pack(
            "<5I",
            adrp(2, THUNK, ORIGINAL), 0x91000042 | ((ORIGINAL & 0xFFF) << 10),
            adrp(16, THUNK + 8, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10),
            0xD61F0200,
        )),
        (b"__DATA_CONST", b"__mg_hud", SLOT, bytes(8)),
    ):
        segment_offset = segments[segment]
        address, _, fileoff, size = unpack_from("<4Q", binary, segment_offset + 24)
        assert address == BASE + fileoff and fileoff <= start <= fileoff + size - len(code)
        assert binary[start:start + len(code)] == bytes(len(code))
        for index in range(unpack_from("<I", binary, segment_offset + 64)[0]):
            offset = segment_offset + 72 + index * 80
            name = binary[offset:offset + 16].rstrip(b"\0")
            address, size, fileoff = unpack_from("<QQI", binary, offset + 32)
            if name == section:
                assert address == BASE + fileoff and fileoff + size == start
                pack_into("<Q", image, offset + 40, size + len(code))
            else:
                assert address + size <= BASE + start or address >= BASE + start + len(code)
        image[start:start + len(code)] = code

    # Preserve x0/x1 and the caller's LR. Supply the unchanged original in x2.
    for sites, opcode in ((TAIL_CALLS, 0x14000000), (CALLS, 0x94000000)):
        for site in sites:
            assert unpack_from("<I", binary, site)[0] == branch(site, ORIGINAL, opcode)
            pack_into("<I", image, site, branch(site, THUNK, opcode))

    return bytes(image)
