"""Bind weapon input and OnFoot weapon processing, plus the TaskUseGun vtable method."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import branch
from patch_render_title import adrp, uleb


BASE_SHA = {
 "242826b5d6ae10e09f0113b4d1e5804028868cbb02360030979b78bfbb91a082",
 "a5a1da132d034e03f8cbd7c2df614c53b52277434a6e1afade722869764261ad",
 "38af5be5b4dfba8a321e4c74a7133b7dd750f530731d2be056c255b7f6e57983",
 "31bc6241f01c1a4d0dcab6545febb979cc257251f160ab9e689e96b6eca47b9d",
}
PATCHED_SHA = {'b84efbe4afa34cd256115300a335bc202e84bab39325ffcd152204f3097c2f1c', '2a2d22956941473640432c05a5de91dee6833c10a507ba402c08033c76fa3cb7', 'd382ff4e21ed1aa67df9ce21e2b994a10282e599fe4ef5efcc3d1b8492259fb3', 'c6a74f8f4a899bdd0db516698528956e0af2d530bd26579b8c0bdef8c60596d3'}
THUNK = 0x497B14
SLOT = 0x4B5618
END = 0x497BD8
HOOKS = (
    (0x15DB74, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CPad_GetWeapon_hook", 3, 0x497B14, 0x4B5618),
    (0x15DE3C, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CPad_WeaponJustDown_hook", 4, 0x497B2C, 0x4B5620),
    (0x13A780, bytes.fromhex("010100B428A042F908914079"), b"_CCamera_IsTargetingActive_hook", 2, 0x497B44, 0x4B5628),
    (0x15E610, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CPad_GetEnterTargeting_hook", 1, 0x497B60, 0x4B5630),
    (0x15E854, bytes.fromhex("F657BDA9F44F01A9FD7B02A9"), b"_CPad_GetTarget_hook", 2, 0x497B78, 0x4B5638),
    (0x15E2C4, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CPad_CycleWeaponLeftJustDown_hook", 1, 0x497B90, 0x4B5640),
    (0x15E354, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CPad_CycleWeaponRightJustDown_hook", 1, 0x497BA8, 0x4B5648),
    (0x3BECD0, bytes.fromhex("FF0304D1EB2B086DE923096D"), b"_CTaskSimplePlayerOnFoot_ProcessPlayerWeapon_hook", 2, 0x497BC0, 0x4B5650),
)
SYMBOLS = tuple(row[2] for row in HOOKS)


def patch_weapon_sync(binary):
    digest = sha256(binary).hexdigest()
    if digest in PATCHED_SHA:
        return binary
    if digest not in BASE_SHA:
        raise ValueError("Camera gyro requires the verified Pickup or Camera gyro executable")
    for site, original, _, _, _, _ in HOOKS:
        if binary[site:site + len(original)] != original:
            raise ValueError(f"Unexpected weapon function prologue at {site:#x}")

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
    if text_offset + text_size not in (0x497AC4, 0x497AEC, THUNK) or hud_offset + hud_size not in (0x4B55F8, 0x4B5608, SLOT):
        raise ValueError("Unexpected camera gyro thunk or bind-slot allocation")
    if any(binary[text_offset + text_size:END]) or any(binary[hud_offset + hud_size:SLOT + len(HOOKS) * 8]):
        raise ValueError("Camera gyro thunk or slot space is already occupied")
    if END > 0x498000 or SLOT + len(HOOKS) * 8 > 0x4B8000:
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

    pack_into("<Q", image, 0x768, END - text_offset)
    pack_into("<Q", image, 0xA30, SLOT + len(HOOKS) * 8 - hud_offset)
    for site, prologue, _, argument, pc, slot in HOOKS:
        # Pass the original trampoline after the four-instruction dispatch stub.
        pack_into("<4I", image, pc,
                  0x10000080 | argument,
                  adrp(16, pc + 4, slot),
                  0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200)
        original = unpack_from("<I", prologue)[0]
        if site == 0x13A780:
            # Original CBZ x1 is out of range from the new section: invert it,
            # then use two full-range B instructions for taken/fallthrough paths.
            assert original == 0xB4000101
            target = site + (((original >> 5) & 0x7FFFF) << 2)
            inverted = ((original ^ 0x01000000) & ~0x00FFFFE0) | (2 << 5)
            pack_into("<3I", image, pc + 16, inverted,
                      branch(pc + 20, target), branch(pc + 24, site + 4))
        else:
            pack_into("<2I", image, pc + 16, original, branch(pc + 20, site + 4))
        pack_into("<I", image, site, branch(site, pc))
    from patch_task_use_gun import patch_task_use_gun
    return patch_task_use_gun(bytes(image))
