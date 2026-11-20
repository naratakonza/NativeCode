"""Bind CTaskSimpleUseGun::SetPedPosition to the iOS CPad::TaskUseGun bridge."""

from struct import pack_into, unpack_from

from patch_render_title import uleb


BASE = 0x100000000
HOOKS = (
    (0x4A9CB0, 0x3499C4, bytes.fromhex("F657BDA9 F44F01A9 FD7B02A9 FD830091"), b"_CPad_TaskUseGun_hook"),
)
SYMBOLS = tuple(row[3] for row in HOOKS)


def patch_task_use_gun(binary):
    if binary[:8] != bytes.fromhex("CFFAEDFE 0C000001") or unpack_from("<I", binary, 12)[0] != 2:
        raise ValueError("TaskUseGun method hooks require the verified ARM64 iOS executable")

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
        raise ValueError("TaskUseGun methods require the existing Morgan classic-dyld dependency")
    data_index, data_command = next((index, offset) for index, (name, offset) in enumerate(segments) if name == b"__DATA_CONST")
    data_vm, _, data_file, data_size = unpack_from("<4Q", binary, data_command + 24)
    if data_vm != BASE + data_file or data_index > 15:
        raise ValueError("Unexpected iOS vtable segment mapping")

    entries = []
    for site, original, prologue, symbol in HOOKS:
        if not data_file <= site <= data_file + data_size - 8 or unpack_from("<Q", binary, site)[0] != BASE + original:
            raise ValueError(f"Unexpected original vtable method at 0x{site:X}")
        if binary[original:original + len(prologue)] != prologue:
            raise ValueError(f"Unexpected original TaskUseGun method at 0x{original:X}")
        entries.append(b"\x20" + uleb(launcher_ordinal) + b"\x40" + symbol + b"\0\x51\x60\0" + bytes([0x70 + data_index]) + uleb(site - data_file) + b"\x90")

    info = next(offset for command, offset in commands if command == 0x80000022)
    bind_start, bind_size, weak_start = unpack_from("<3I", binary, info + 16)
    bind_end = bind_start + bind_size
    if bind_end != weak_start:
        raise ValueError("Unexpected classic-dyld bind layout")
    table = binary[bind_start:bind_end].rstrip(b"\0")
    present = [entry in table for entry in entries]
    if any(present):
        if not all(present) or any(table.count(entry) != 1 for entry in entries):
            raise ValueError("Incomplete or duplicate TaskUseGun method bindings")
        return binary

    table += b"".join(entries) + b"\0"
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size
    link = next(offset for name, offset in segments if name == b"__LINKEDIT")
    link_vm_size, link_file, link_size = unpack_from("<3Q", binary, link + 32)
    if growth <= 0 or link_file + link_size != len(binary) or link_size + growth > link_vm_size:
        raise ValueError("Insufficient existing LINKEDIT capacity for TaskUseGun binding")

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

    # Existing rebase entries first slide the original game pointers. The regular
    # bind pass then replaces exactly this slot with the native callbacks.
    return bytes(image)
