"""Connect the verified iOS pickup collection site to the existing native hook."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import branch
from patch_render_title import adrp, uleb


BASE_SHA = "f8e3266a7debf24a6965ae8994a74cd76e64e4b0773f43a120fad67573bc64d5"
PATCHED_SHA = "242826b5d6ae10e09f0113b4d1e5804028868cbb02360030979b78bfbb91a082"
SITE = 0x0CE30C
THUNK = 0x497AB4
SLOT = 0x4B55F0
SYMBOL = b"_PickUpPickup_hook"
ORIGINAL = bytes.fromhex("688240391F4D0071480B0054")


def patch_pickup(binary):
    digest = sha256(binary).hexdigest()
    if digest == PATCHED_SHA:
        return binary
    if digest != BASE_SHA:
        raise ValueError("Pickup hook requires the verified cc79059 executable")
    if binary[SITE:SITE + len(ORIGINAL)] != ORIGINAL:
        raise ValueError("Unexpected CPickup::Update collection instructions")

    commands = []
    segments = []
    ordinal = 0
    launcher_ordinal = None
    cursor = 32
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<II", binary, cursor)
        if size < 8 or cursor + size > 32 + unpack_from("<I", binary, 20)[0]:
            raise ValueError("Invalid Mach-O load command")
        commands.append((command, cursor))
        if command == 0x19:
            segments.append((binary[cursor + 8:cursor + 24].rstrip(b"\0"), cursor))
        if command in (0xC, 0x80000018, 0x8000001F, 0x80000023):
            ordinal += 1
            start = cursor + unpack_from("<I", binary, cursor + 8)[0]
            if binary[start:binary.index(b"\0", start)] == b"@rpath/MorganLauncher.framework/MorganLauncher":
                launcher_ordinal = ordinal
        cursor += size
    if launcher_ordinal is None or any(command == 0x80000034 for command, _ in commands):
        raise ValueError("Pickup hook requires the existing Morgan classic-dyld dependency")

    text_size, text_offset = unpack_from("<QI", binary, 0x768)
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    if text_offset + text_size != THUNK or hud_offset + hud_size != SLOT:
        raise ValueError("Unexpected pickup thunk or bind-slot allocation")
    if any(binary[THUNK:THUNK + 16]) or any(binary[SLOT:SLOT + 8]):
        raise ValueError("Pickup thunk or slot space is already occupied")
    if THUNK + 16 > 0x498000 or SLOT + 8 > 0x4B8000:
        raise ValueError("Insufficient existing segment capacity")

    data_index, data_command = next((index, offset) for index, (name, offset) in enumerate(segments) if name == b"__DATA_CONST")
    data_vm, _, data_file, data_size = unpack_from("<4Q", binary, data_command + 24)
    if data_vm != 0x100000000 + data_file or data_index > 15 or not data_file <= SLOT <= data_file + data_size - 8:
        raise ValueError("Unexpected pickup bind-slot segment")
    entry = b"\x20" + uleb(launcher_ordinal) + b"\x40" + SYMBOL + b"\0\x51\x60\0"
    entry += bytes([0x70 + data_index]) + uleb(SLOT - data_file) + b"\x90"

    info = next(offset for command, offset in commands if command == 0x80000022)
    bind_start, bind_size, weak_start = unpack_from("<3I", binary, info + 16)
    bind_end = bind_start + bind_size
    if bind_end != weak_start:
        raise ValueError("Unexpected classic-dyld bind layout")
    table = binary[bind_start:bind_end].rstrip(b"\0")
    if SYMBOL in table:
        raise ValueError("Pickup hook binding already exists without the expected patch")
    table += entry + b"\0"
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size
    link = next(offset for name, offset in segments if name == b"__LINKEDIT")
    link_vm_size, link_file, link_size = unpack_from("<3Q", binary, link + 32)
    if growth <= 0 or link_file + link_size != len(binary) or link_size + growth > link_vm_size:
        raise ValueError("Insufficient existing LINKEDIT capacity for pickup binding")

    image = bytearray(binary[:bind_start] + table + binary[bind_end:])
    pack_into("<I", image, info + 20, len(table))
    pack_into("<Q", image, link + 48, link_size + growth)
    for command, offset in commands:
        fields = ()
        if command == 0x80000022:
            fields = (8, 16, 24, 32, 40)
        elif command == 2:
            fields = (8, 16)
        elif command == 0xB:
            fields = (32, 40, 48, 56, 64, 72)
        elif command in (0x1D, 0x1E, 0x26, 0x29, 0x2B, 0x2E, 0x80000033):
            fields = (8,)
        for field in fields:
            value = unpack_from("<I", binary, offset + field)[0]
            if value >= bind_end:
                pack_into("<I", image, offset + field, value + growth)

    pack_into("<Q", image, 0x768, text_size + 16)
    pack_into("<Q", image, 0xA30, hud_size + 8)
    pack_into("<4I", image, THUNK,
              adrp(16, THUNK, SLOT), 0xF9400210 | (((SLOT & 0xFFF) // 8) << 10),
              0xD61F0200, 0xD503201F)
    # Branch without link: the naked hook must retain the engine's original LR.
    pack_into("<I", image, SITE, branch(SITE, THUNK))
    return bytes(image)
