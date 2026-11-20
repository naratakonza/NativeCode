"""Bind the two camera gyro methods while preserving integer, floating-point arguments and LR."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import branch
from patch_render_title import adrp, uleb


BASE_SHA = {
    "242826b5d6ae10e09f0113b4d1e5804028868cbb02360030979b78bfbb91a082",
    "a5a1da132d034e03f8cbd7c2df614c53b52277434a6e1afade722869764261ad",
}
PATCHED_SHA = {'31bc6241f01c1a4d0dcab6545febb979cc257251f160ab9e689e96b6eca47b9d', '38af5be5b4dfba8a321e4c74a7133b7dd750f530731d2be056c255b7f6e57983'}
THUNK = 0x497AEC
SLOT = 0x4B5608
SYMBOLS = (b"_Process_M16_1stPerson_hook", b"_Process_Rocket_hook")
TARGETS = (0x11FEB8, 0x121584)
SITES = ((0x11C058, bytes.fromhex("980F0094"), THUNK),
         (0x11C0E4, bytes.fromhex("28150094"), THUNK + 20),
         (0x11C1B0, bytes.fromhex("F5140094"), THUNK + 20))


def patch_camera_gyro(binary):
    digest = sha256(binary).hexdigest()
    if digest in PATCHED_SHA:
        return binary
    if digest not in BASE_SHA:
        raise ValueError("Camera gyro requires the verified Pickup or Camera gyro executable")
    for site, original, _ in SITES:
        if binary[site:site + 4] != original:
            raise ValueError(f"Unexpected camera gyro call at {site:#x}")
    if binary[0x11FEB8:0x11FEC0] != bytes.fromhex("FF4303D1ED33066D") or binary[0x121584:0x12158C] != bytes.fromhex("FFC302D1E923066D"):
        raise ValueError("Unexpected original function prologues")

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
        raise ValueError("Camera gyro hook requires the existing Morgan classic-dyld dependency")

    text_size, text_offset = unpack_from("<QI", binary, 0x768)
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    if text_offset + text_size not in (0x497AC4, THUNK) or hud_offset + hud_size not in (0x4B55F8, SLOT):
        raise ValueError("Unexpected camera gyro thunk or bind-slot allocation")
    if any(binary[text_offset + text_size:THUNK + 40]) or any(binary[hud_offset + hud_size:SLOT + 16]):
        raise ValueError("Camera gyro thunk or slot space is already occupied")
    if THUNK + 40 > 0x498000 or SLOT + 16 > 0x4B8000:
        raise ValueError("Insufficient existing segment capacity")

    data_index, data_command = next((index, offset) for index, (name, offset) in enumerate(segments) if name == b"__DATA_CONST")
    data_vm, _, data_file, data_size = unpack_from("<4Q", binary, data_command + 24)
    if data_vm != 0x100000000 + data_file or data_index > 15 or not data_file <= SLOT <= data_file + data_size - 8:
        raise ValueError("Unexpected camera gyro bind-slot segment")
    entries = b""
    for index, symbol in enumerate(SYMBOLS):
        entry = b"\x20" + uleb(launcher_ordinal) + b"\x40" + symbol + b"\0\x51\x60\0"
        entry += bytes([0x70 + data_index]) + uleb(SLOT + index * 8 - data_file) + b"\x90"
        entries += entry

    info = next(offset for command, offset in commands if command == 0x80000022)
    bind_start, bind_size, weak_start = unpack_from("<3I", binary, info + 16)
    bind_end = bind_start + bind_size
    if bind_end != weak_start:
        raise ValueError("Unexpected classic-dyld bind layout")
    table = binary[bind_start:bind_end].rstrip(b"\0")
    if any(symbol in table for symbol in SYMBOLS):
        raise ValueError("Camera gyro hook binding already exists without the expected patch")
    table += entries + b"\0"
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size
    link = next(offset for name, offset in segments if name == b"__LINKEDIT")
    link_vm_size, link_file, link_size = unpack_from("<3Q", binary, link + 32)
    if growth <= 0 or link_file + link_size != len(binary) or link_size + growth > link_vm_size:
        raise ValueError("Insufficient existing LINKEDIT capacity for camera gyro binding")

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

    pack_into("<Q", image, 0x768, THUNK + 40 - text_offset)
    pack_into("<Q", image, 0xA30, SLOT + 16 - hud_offset)
    for index, (target, argument) in enumerate(zip(TARGETS, (2, 3))):
        pc = THUNK + index * 20
        slot = SLOT + index * 8
        # Original function in x2 for M16, x3 for Rocket. Preserve s0/s1/s2 and Rocket bool in x2.
        pack_into("<5I", image, pc,
                  adrp(argument, pc, target), 0x91000000 | ((target & 0xFFF) << 10) | (argument << 5) | argument,
                  adrp(16, pc + 8, slot), 0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200)
    for site, _, thunk in SITES:
        pack_into("<I", image, site, branch(site, thunk) | 0x80000000)
    return bytes(image)
