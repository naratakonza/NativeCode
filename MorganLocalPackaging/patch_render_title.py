"""Install the ARM64 title bridge before the main camera ends its frame."""

from hashlib import sha256
from struct import pack_into, unpack_from


BASE_SHA = "5795d508e064007073a7d43ccb33484e2cd6d5c2fdd7c07d4df66b66b7f34bd1"
BASE = 0x100000000
END_FRAME_2D = 0x319178
CALLSITE = 0x15A2D0
THUNK = 0x49716C
SLOT = 0x4B5488
SYMBOL = b"_Render2dStuff_hook"
PAUSE_RETURNS = (0x17FC8C, 0x17FC98)
PAUSE_SYMBOLS = (b"_MGTimerStartUserPause", b"_MGTimerEndUserPause")


def uleb(value):
    data = bytearray()
    while value >= 0x80:
        data.append((value & 0x7F) | 0x80)
        value >>= 7
    data.append(value)
    return bytes(data)


def adrp(register, pc, target):
    page_delta = (target >> 12) - (pc >> 12)
    assert -(1 << 20) <= page_delta < (1 << 20)
    immediate = page_delta & 0x1FFFFF
    return 0x90000000 | ((immediate & 3) << 29) | ((immediate >> 2) << 5) | register


def patch_render_title(binary, timer_pause=False):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("RenderTitle expects the verified LastGameLog GTA iOS executable")
    assert binary[END_FRAME_2D:END_FRAME_2D + 4] == bytes.fromhex("C0 03 5F D6")
    assert binary[CALLSITE:CALLSITE + 12] == bytes.fromhex("AA FB 06 94 C0 06 40 F9 EC 42 05 94")
    if timer_pause:
        assert binary[0x17FC80:0x17FC9C] == bytes.fromhex(
            "08 2E 00 D0 29 00 80 52 09 31 29 39 C0 03 5F D6 08 2E 00 D0 1F 31 29 39 C0 03 5F D6"
        )

    commands = []
    cursor = 32
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<2I", binary, cursor)
        commands.append((command, cursor, size))
        cursor += size
    segments = {
        binary[off + 8:off + 24].rstrip(b"\0"): off
        for command, off, _ in commands if command == 0x19
    }
    text, data, link = (segments[name] for name in (b"__TEXT", b"__DATA_CONST", b"__LINKEDIT"))

    def extend_section(segment, name, new_start, length, protection):
        vmaddr, _, fileoff, filesize, _, prot = unpack_from("<4Q2I", binary, segment + 24)
        assert vmaddr == BASE + fileoff and prot == protection
        assert fileoff <= new_start <= fileoff + filesize - length
        assert not any(binary[new_start:new_start + length])
        match = None
        for index in range(unpack_from("<I", binary, segment + 64)[0]):
            off = segment + 72 + 80 * index
            address, size, section_fileoff = unpack_from("<QQI", binary, off + 32)
            if binary[off:off + 16].rstrip(b"\0") == name:
                assert address == BASE + section_fileoff and section_fileoff + size == new_start
                match = off + 40, size + length
            else:
                assert address + size <= BASE + new_start or address >= BASE + new_start + length
        assert match
        return match

    title_size = 28 if timer_pause else 20
    text_size, new_text_size = extend_section(text, b"__mg_gameproc", THUNK, title_size + (24 if timer_pause else 0), 5)
    hud_size, new_hud_size = extend_section(data, b"__mg_hud", SLOT, 24 if timer_pause else 8, 3)
    dependencies = [
        binary[off + unpack_from("<I", binary, off + 8)[0]:off + size].split(b"\0")[0]
        for command, off, size in commands if command in (0xC, 0x80000018, 0x8000001F, 0x80000023)
    ]
    ordinal = dependencies.index(b"@rpath/MorganLauncher.framework/MorganLauncher") + 1
    assert ordinal == 48
    data_index = list(segments).index(b"__DATA_CONST")
    data_fileoff = unpack_from("<Q", binary, data + 40)[0]
    assert data_index == 2

    def binding(symbol, slot):
        return b"\x20" + uleb(ordinal) + b"\x40" + symbol + b"\0\x51\x60\x00" + bytes([0x70 + data_index]) + uleb(slot - data_fileoff) + b"\x90"

    info = next(off for command, off, _ in commands if command == 0x80000022)
    bind, size, weak_bind = unpack_from("<3I", binary, info + 16)
    end = bind + size
    assert end == weak_bind
    previous = binary[bind:end].rstrip(b"\0")
    assert previous.endswith(binding(b"_CStreaming_LoadExtraImgDirectories_hook", SLOT - 8))
    table = previous + binding(SYMBOL, SLOT)
    if timer_pause:
        table += b"".join(binding(symbol, SLOT + 8 + 8 * i) for i, symbol in enumerate(PAUSE_SYMBOLS))
    table += b"\0"
    table += bytes((size - len(table)) % 16)
    growth = len(table) - size
    link_vm_size, link_off, link_size = unpack_from("<3Q", binary, link + 32)
    assert 0 < growth and link_off + link_size == len(binary) and link_size + growth <= link_vm_size

    result = bytearray(binary[:bind] + table + binary[end:])
    pack_into("<Q", result, text_size, new_text_size)
    pack_into("<Q", result, hud_size, new_hud_size)
    pack_into("<Q", result, link + 48, link_size + growth)
    pack_into("<I", result, info + 20, len(table))
    linkedit_offsets = {
        0x80000022: (24, 32, 40), 0x2: (8, 16), 0xB: (32, 40, 48, 56, 64, 72),
        0x1D: (8,), 0x26: (8,), 0x29: (8,),
    }
    for command, off, _ in commands:
        for field in linkedit_offsets.get(command, ()):
            value = unpack_from("<I", binary, off + field)[0]
            if value:
                assert end <= value <= len(binary)
                pack_into("<I", result, off + field, value + growth)

    pack_into("<I", result, CALLSITE, 0x94000000 | ((THUNK - CALLSITE) // 4))
    title_entry = THUNK
    if timer_pause:
        assert binary[0x15A08C:0x15A094] == bytes.fromhex("01 00 80 52 13 00 80 52")
        assert binary[0x15A284:0x15A288] == bytes.fromhex("93 04 00 34")
        # w19 is the gameplay render width. Zero selects the frontend frame.
        pack_into("<I", result, THUNK, 0x34000000 | ((24 // 4) << 5) | 19)
        stock_end = THUNK + 24
        pack_into("<I", result, stock_end, 0x14000000 | (((END_FRAME_2D - stock_end) // 4) & 0x03FFFFFF))
        title_entry += 4
    # Keep the caller's LR and pass the original end-of-frame stub in x0.
    pack_into("<5I", result, title_entry, adrp(0, title_entry, END_FRAME_2D), 0x91000000 | ((END_FRAME_2D & 0xFFF) << 10),
              adrp(16, title_entry + 8, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10), 0xD61F0200)
    if timer_pause:
        for i, return_pc in enumerate(PAUSE_RETURNS):
            thunk = THUNK + title_size + i * 12
            slot = SLOT + 8 + i * 8
            # The game has already stored the pause flag. Tail-call the bridge with the original LR.
            pack_into("<I", result, return_pc, 0x14000000 | ((thunk - return_pc) // 4))
            pack_into("<3I", result, thunk, adrp(16, thunk, slot), 0xF9400210 | (((slot & 0xFFF) // 8) << 10), 0xD61F0200)
    return bytes(result)
