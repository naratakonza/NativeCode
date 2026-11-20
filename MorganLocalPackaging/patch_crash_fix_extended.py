"""Port thirteen Thunder crash-fix hooks and the 10000-node iOS matrix pool."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import branch
from patch_render_title import adrp, uleb


BASE_SHA = {"688124c22885cb2d3ff00783ed3d9d6cf1a42b0cd5bf9af3eff17c6a1032129b", "77fdda1ec5885deb7d6e3f983fcae9d428099096120928fcc6476540c6309b67"}
PATCHED_SHA = {'d14833a5229535ce554535c496bd2d5ece52dd64fa7f43b991da9e16a285837b', '6445aa595d7ecc25a0712d66a366638187a6a7395a2ea44373b374a5d64438f6'}
THUNK = 0x497C1C
SLOT = 0x4B5670
END = 0x497D54
HOOKS = (
    (0x2AD268, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_RwFrameAddChild_hook", 2, 0x497C1C, 0x4B5670),
    (0x3EEA8, bytes.fromhex("E923BD6DF44F01A9FD7B02A9"), b"_CAnimBlendNode_FindKeyFrame_hook", 1, 0x497C34, 0x4B5678),
    (0x41F68, bytes.fromhex("F657BDA9F44F01A9FD7B02A9"), b"_CAnimManager_UncompressAnimation_hook", 1, 0x497C4C, 0x4B5680),
    (0x2D7B88, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CCustomRoadsignMgr_RenderRoadsignAtomic_hook", 2, 0x497C64, 0x4B5688),
    (0x143478, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_MaterialUpdateUVAnimCB_hook", 2, 0x497C7C, 0x4B5690),
    (0x1F4A78, bytes.fromhex("1F040031C0020054093C0012"), b"_CRadar_ClearBlip_hook", 1, 0x497C94, 0x4B5698),
    (0x24039C, bytes.fromhex("FF8302D1E923056DF85F06A9"), b"_CPed_PlayFootSteps_hook", 1, 0x497CAC, 0x4B56A0),
    (0x4230C, bytes.fromhex("88320090089940F9E903002A"), b"_CAnimManager_GetAnimAssociation_hook", 2, 0x497CC4, 0x4B56A8),
    (0x347AEC, bytes.fromhex("FFC302D1EB2B056DE923066D"), b"_CTaskSimpleUseGun_ProcessPed_hook", 2, 0x497CDC, 0x4B56B0),
    (0x3AE724, bytes.fromhex("F657BDA9F44F01A9FD7B02A9"), b"_CTaskSimpleGetUp_ProcessPed_hook", 2, 0x497CF4, 0x4B56B8),
    (0x3478A8, bytes.fromhex("F657BDA9F44F01A9FD7B02A9"), b"_CTaskSimpleUseGun_RemoveStanceAnims_hook", 2, 0x497D0C, 0x4B56C0),
    (0xC4A18, bytes.fromhex("F44FBEA9FD7B01A9FD430091"), b"_CObject_ProcessGarageDoorBehaviour_hook", 1, 0x497D24, 0x4B56C8),
    (0x293DB4, bytes.fromhex("001840F9C0035FD6F44FBEA9"), b"_emu_ArraysGetID_hook", 1, 0x497D3C, 0x4B56D0),
)
MATRIX_COOKIE = 0x4B56E0
DATA_END = MATRIX_COOKIE + 16

SYMBOLS = tuple(row[2] for row in HOOKS)


def patch_crash_fix_extended(binary):
    digest = sha256(binary).hexdigest()
    if digest in PATCHED_SHA:
        return binary
    if digest not in BASE_SHA:
        raise ValueError("Extended crash fix requires the verified 2e7e2b8 WeaponSkill executable")
    for site, original, _, _, _, _ in HOOKS:
        if binary[site:site + len(original)] != original:
            raise ValueError(f"Unexpected crash-fix function prologue at {site:#x}")

    # iOS inlines CMatrixLinkList::Init(900) inside CPlaceable::InitMatrixArray.
    # Keep that allocator and all list linking, changing the allocation count and cookie together.
    matrix_originals = {
        0x16CB5C: bytes.fromhex("13A78D523300A072"),
        0x16CB6C: bytes.fromhex("A81700D000F1C23D"),
        0x462BC0: bytes.fromhex("68000000000000008403000000000000"),
    }
    if binary[0x16CB50:0x16CB5C] != bytes.fromhex("F44FBEA9FD7B01A9FD430091"):
        raise ValueError("Unexpected CPlaceable::InitMatrixArray prologue")
    for site, expected in matrix_originals.items():
        if binary[site:site + len(expected)] != expected:
            raise ValueError(f"Unexpected matrix initializer at {site:#x}")

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
        raise ValueError("Extended crash fix hook requires the existing Morgan classic-dyld dependency")

    text_size, text_offset = unpack_from("<QI", binary, 0x768)
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    if (text_offset + text_size, hud_offset + hud_size) not in {(0x497BEC, 0x4B5660), (THUNK, SLOT)}:
        raise ValueError("Unexpected extended crash fix thunk or bind-slot allocation")
    if any(binary[text_offset + text_size:END]) or any(binary[hud_offset + hud_size:DATA_END]):
        raise ValueError("Extended crash fix thunk or slot space is already occupied")
    if END > 0x498000 or DATA_END > 0x4B8000:
        raise ValueError("Insufficient existing segment capacity")

    data_index, data_command = next((index, offset) for index, (name, offset) in enumerate(segments) if name == b"__DATA_CONST")
    data_vm, _, data_file, data_size = unpack_from("<4Q", binary, data_command + 24)
    if data_vm != 0x100000000 + data_file or data_index > 15 or not data_file <= SLOT < DATA_END <= data_file + data_size:
        raise ValueError("Unexpected extended crash fix bind-slot segment")
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
        raise ValueError("Extended crash fix hook binding already exists without the expected patch")
    table += entries + b"\0"
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size
    link = next(offset for name, offset in segments if name == b"__LINKEDIT")
    link_vm_size, link_file, link_size = unpack_from("<3Q", binary, link + 32)
    if growth <= 0 or link_file + link_size != len(binary) or link_size + growth > link_vm_size:
        raise ValueError("Insufficient existing LINKEDIT capacity for extended crash fix binding")

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
    pack_into("<Q", image, 0xA30, DATA_END - hud_offset)
    for site, prologue, _, argument, pc, slot in HOOKS:
        # Pass the original trampoline after the four-instruction dispatch stub.
        pack_into("<4I", image, pc,
                  0x10000080 | argument,
                  adrp(16, pc + 4, slot),
                  0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200)
        original = unpack_from("<I", prologue)[0]
        if site == 0x4230C:
            # Original first instruction is ADRP x8, assoc-group table. Relocate it.
            original = adrp(8, pc + 16, 0x692000)
        pack_into("<2I", image, pc + 16, original, branch(pc + 20, site + 4))
        pack_into("<I", image, site, branch(site, pc))
    count_bytes = (10000 - 1) * 0x68
    pack_into("<2I", image, 0x16CB5C, 0x52800013 | ((count_bytes & 0xFFFF) << 5), 0x72A00013 | ((count_bytes >> 16) << 5))
    pack_into("<2I", image, 0x16CB6C, adrp(8, 0x16CB6C, MATRIX_COOKIE), 0x3DC00100 | (((MATRIX_COOKIE & 0xFFF) // 16) << 10))
    pack_into("<2Q", image, MATRIX_COOKIE, 0x68, 10000)
    return bytes(image)
