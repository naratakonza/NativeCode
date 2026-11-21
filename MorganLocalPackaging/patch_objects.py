"""Bind Thunder objects, model resolution and attached cameras on iOS."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import branch
from patch_render_title import adrp, uleb


BASE_SHA = {"beb295564dfe5b786b5653ec66be711dbb34b65e04c9e60db6f3ae5528ba0814"}
PATCHED_SHA = {"6c68f831d910810d0794eb04b604412fe9fae3a2096dc970882fead74d9cac68"}
THUNK = 0x497DB4
SLOT = 0x4B5710
END = 0x497E04
HOOKS = (
    (0x4A6FC0, bytes.fromhex("A477230001000000"), b"_CObject_Render_hook", 1, 0x497DB4, 0x4B5710),
    (0x16AF5C, bytes.fromhex("EF3BB86DED33016D"), b"_CPhysical_PositionAttachedEntity_hook", 1, 0x497DC8, 0x4B5718),
    (0xF9CB8, bytes.fromhex("E000F836"), b"_CObject_CreateObjectModel_hook", 0, 0x497DE0, 0x4B5720),
    (0x1363E8, bytes.fromhex("FF0305D1ED330B6D"), b"_CCamera_Process_hook", 1, 0x497DEC, 0x4B5728),
)
SYMBOLS = tuple(row[2] for row in HOOKS)


def patch_objects(binary):
    digest = sha256(binary).hexdigest()
    if digest in PATCHED_SHA:
        return binary
    if digest not in BASE_SHA:
        raise ValueError("Object hooks requires the verified 5aec3a9 Object-Hooks executable")
    for site, original, _, _, _, _ in HOOKS:
        if binary[site:site + len(original)] != original:
            raise ValueError(f"Unexpected object hook function prologue at {site:#x}")

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
        raise ValueError("Object hooks hook requires the existing Morgan classic-dyld dependency")

    text_size, text_offset = unpack_from("<QI", binary, 0x768)
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    if text_offset + text_size != THUNK or hud_offset + hud_size != SLOT:
        raise ValueError("Unexpected Object hooks thunk or bind-slot allocation")
    if any(binary[text_offset + text_size:END]) or any(binary[hud_offset + hud_size:SLOT + len(HOOKS) * 8]):
        raise ValueError("Object hooks thunk or slot space is already occupied")
    if END > 0x498000 or SLOT + len(HOOKS) * 8 > 0x4B8000:
        raise ValueError("Insufficient existing segment capacity")

    data_index, data_command = next((index, offset) for index, (name, offset) in enumerate(segments) if name == b"__DATA_CONST")
    data_vm, _, data_file, data_size = unpack_from("<4Q", binary, data_command + 24)
    if data_vm != 0x100000000 + data_file or data_index > 15 or not data_file <= SLOT <= data_file + data_size - 8:
        raise ValueError("Unexpected Object hooks bind-slot segment")
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
        raise ValueError("Object hooks hook binding already exists without the expected patch")
    table += entries + b"\0"
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size
    link = next(offset for name, offset in segments if name == b"__LINKEDIT")
    link_vm_size, link_file, link_size = unpack_from("<3Q", binary, link + 32)
    if growth <= 0 or link_file + link_size != len(binary) or link_size + growth > link_vm_size:
        raise ValueError("Insufficient existing LINKEDIT capacity for Object hooks binding")

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

    pack_into("<Q", image, 0x768, END - text_offset)
    pack_into("<Q", image, 0xA30, SLOT + len(HOOKS) * 8 - hud_offset)
    for site, prologue, _, argument, pc, slot in HOOKS:
        if site == 0x4A6FC0:
            # Hook only CObject's virtual Render slot, retaining the original method in x1.
            pack_into("<5I", image, pc, adrp(1, pc, 0x2377A4), 0x91000021 | ((0x2377A4 & 0xFFF) << 10), adrp(16, pc + 8, slot), 0xF9400210 | (((slot & 0xFFF) // 8) << 10), 0xD61F0200)
            pack_into("<Q", image, site, 0x100000000 + pc)
        elif site == 0xF9CB8:
            # Resolve ScriptParams[0], then resume after the original negative-model branch.
            pack_into("<3I", image, pc, adrp(16, pc, slot), 0xF9400210 | (((slot & 0xFFF) // 8) << 10), 0xD61F0200)
            pack_into("<I", image, site, branch(site, pc))
        else:
            pack_into("<4I", image, pc, 0x10000080 | argument, adrp(16, pc + 4, slot), 0xF9400210 | (((slot & 0xFFF) // 8) << 10), 0xD61F0200)
            pack_into("<2I", image, pc + 16, unpack_from("<I", prologue)[0], branch(pc + 20, site + 4))
            pack_into("<I", image, site, branch(site, pc))
    return bytes(image)
