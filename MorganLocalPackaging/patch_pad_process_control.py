"""Install the Thunder player processing hooks in the verified Widgets executable."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch
from patch_render_title import adrp, uleb


BASE_SHA = "6b44cd017b25aa11ee7322eed57d8af424e06bc3b4dc85e648fbf318cf8d0c8f"
THUNK = 0x4976A4
SLOT = 0x4B5528
HOOKS = (
    (0x26E010, bytes.fromhex("FF0302D1 E923016D FC6F02A9"), b"_CPed_ProcessControl_hook"),
    (0x23EE00, bytes.fromhex("FFC300D1 F44F01A9 FD7B02A9"), b"_CPed_UpdatePosition_hook"),
)
WALK_HOOKS = (
    (0x15CE14, bytes.fromhex("FFC300D1 F44F01A9 FD7B02A9 FD830091 08204279"), b"_CPad_GetPedWalkLeftRight_hook"),
    (0x15CF34, bytes.fromhex("FFC300D1 F44F01A9 FD7B02A9 FD830091 08204279"), b"_CPad_GetPedWalkUpDown_hook"),
)
JUMP_JUST_DOWN = 0x15F0A0
GET_JUMP = 0x15F030
BUTTON_HOOKS = (
    (0x15ED84, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 09204279"), b"_CPad_DuckJustDown_hook"),
    (JUMP_JUST_DOWN, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 08204279 08030035 08C84439"), b"_CPad_JumpJustDown_hook"),
    (0x15F24C, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 08204279 C92E00F0"), b"_CPad_GetSprint_hook"),
    (GET_JUMP, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 08204279 68020035 08C84439"), b"_CPad_GetJump_hook"),
    (0x15F1AC, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 08204279"), b"_CPad_SwimJumpJustDown_hook"),
    (0x15E034, bytes.fromhex("F657BDA9 F44F01A9 FD7B02A9 FD830091 08204279"), b"_CPad_MeleeAttackJustDown_hook"),
    (0x15EE4C, bytes.fromhex("FF8300D1 FD7B01A9 FD430091 883100D0"), b"_CPad_GetAutoClimb_hook"),
    (0x15EF1C, bytes.fromhex("FF8300D1 FD7B01A9 FD430091 0804C079"), b"_CPad_GetAbortClimb_hook"),
    (0x15F124, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 A83100B0"), b"_CPad_DiveJustDown_hook"),
    (0x15E3E4, bytes.fromhex("FD7BBFA9 FD030091 57B60194 1F040071"), b"_CPad_GetBlock_hook"),
)
RETURN_ZERO = (
    (0x15ECD8, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 08204279")),  # GetDuck
    (0x15F330, bytes.fromhex("F44FBEA9 FD7B01A9 FD430091 08204279")),  # SprintJustDown
    (0x15E000, bytes.fromhex("FD7BBFA9 FD030091 08204279 88000034")),  # GetMeleeAttack
)
BLOCK_TEXTURE = 0x480D96
AIM_STORES = (
    (0x3C1350, 0xBD06CA60),  # PlayerControlZelda
    (0x3C1020, 0xBD06CA60),  # PlayerControlDucked, movement without the target-vector path
    (0x3C1274, 0xBD06CA6A),  # PlayerControlDucked, target-vector path
)
AIM_FLAG = b"_CPad_AllowAimingRotation"
SYMBOLS = tuple(symbol for _, _, symbol in HOOKS) + (AIM_FLAG,) + tuple(symbol for _, _, symbol in WALK_HOOKS + BUTTON_HOOKS)
SHADOW_HOOKS = (
    (0x2DBE70, bytes.fromhex("28684139 08090012 1F0D0071 61010054"), b"_CRealTimeShadowManager_DoShadowThisFrame_hook"),
    (0x2DBE58, bytes.fromhex("08004039 88000034 280040F9 1FC900F9 3F0000F9 C0035FD6"), b"_CRealTimeShadowManager_ReturnRealTimeShadow_hook"),
)
SHADOW_SYMBOLS = tuple(symbol for _, _, symbol in SHADOW_HOOKS)


def patch_pad_process_control(binary, *, shadow_style=False):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CPad ProcessControl requires the verified Widgets executable")

    hooks = HOOKS + WALK_HOOKS + BUTTON_HOOKS + (SHADOW_HOOKS if shadow_style else ())
    symbols = SYMBOLS + (SHADOW_SYMBOLS if shadow_style else ())
    thunk_size = len(hooks) * 24 + len(AIM_STORES) * 32 + 4
    slot_size = len(symbols) * 8
    assert binary[0x740:0x750].rstrip(b"\0") == b"__mg_gameproc"
    text_address, text_size, text_offset = unpack_from("<QQI", binary, 0x760)
    assert text_address == BASE + text_offset and text_offset + text_size == THUNK
    assert THUNK + thunk_size <= 0x498000 and not any(binary[THUNK:THUNK + thunk_size])
    assert binary[0xA08:0xA18].rstrip(b"\0") == b"__mg_hud"
    hud_size, hud_offset = unpack_from("<QI", binary, 0xA30)
    assert hud_offset + hud_size == SLOT and SLOT + slot_size <= 0x4B8000
    assert not any(binary[SLOT:SLOT + slot_size])
    data_const_file = unpack_from("<Q", binary, 0x790 + 40)[0]

    bind_start, bind_size, weak_start = unpack_from("<III", binary, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = binary[bind_start:bind_end].rstrip(b"\0")
    for index, symbol in enumerate(symbols):
        table += b"\x20\x30\x40" + symbol + b"\x00\x51\x60\x00"
        table += b"\x72" + uleb(SLOT + index * 8 - data_const_file) + b"\x90"
    table += b"\x00"
    table += bytes((bind_size - len(table)) % 16)

    growth = len(table) - bind_size
    link_vm_size, link_offset, link_size = unpack_from("<QQQ", binary, 0xF70)
    assert growth > 0 and link_offset + link_size == len(binary) and link_size + growth <= link_vm_size
    image = bytearray(binary)
    image[bind_start:bind_end] = table
    pack_into("<I", image, 0xFAC, len(table))
    pack_into("<Q", image, 0xF80, link_size + growth)
    for field in (0xFB0, 0xFB8, 0xFC0, 0xFD0, 0xFD8, 0x1000, 0x1008, 0x1010, 0x1018, 0x1020, 0x1028, 0x1E28, 0x1E38, 0x1E48):
        offset = unpack_from("<I", binary, field)[0]
        if offset:
            assert bind_end <= offset <= link_offset + link_size
            pack_into("<I", image, field, offset + growth)

    pack_into("<Q", image, 0x768, text_size + thunk_size)
    pack_into("<Q", image, 0xA30, hud_size + slot_size)
    thunk = THUNK
    for index, (site, prologue, symbol) in enumerate(hooks):
        if index == len(HOOKS):
            thunk += len(AIM_STORES) * 32
        slot = SLOT + symbols.index(symbol) * 8
        assert binary[site:site + len(prologue)] == prologue
        pack_into("<I", image, site, branch(site, thunk))
        if site == JUMP_JUST_DOWN:
            jump_original = thunk + 16
        if site == GET_JUMP:
            # Thunder GetJump uses the original JumpJustDown for local input.
            delta = jump_original - thunk
            pack_into("<I", image, thunk, 0x10000002 | ((delta & 3) << 29) | (((delta >> 2) & 0x7FFFF) << 5))
            thunk += 4
        original_register = 2 if symbol in (b"_CPad_DuckJustDown_hook", b"_CPad_GetSprint_hook") + SHADOW_SYMBOLS else 1
        pack_into("<6I", image, thunk,
                  0x10000080 | original_register,  # adr x1/x2, original trampoline
                  adrp(16, thunk + 4, slot),
                  0xF9400210 | (((slot & 0xFFF) // 8) << 10),
                  0xD61F0200,  # br x16
                  unpack_from("<I", prologue)[0],
                  branch(thunk + 20, site + 4))
        thunk += 24

    flag_slot = SLOT + len(HOOKS) * 8
    for index, (site, store) in enumerate(AIM_STORES):
        thunk = THUNK + len(HOOKS) * 24 + index * 32
        assert unpack_from("<I", binary, site)[0] == store
        pack_into("<I", image, site, branch(site, thunk))
        # Preserve x16, NZCV and the original store operands at these inline sites.
        pack_into("<8I", image, thunk,
                  0xF81F0FF0,  # str x16, [sp, #-16]!
                  adrp(16, thunk + 4, flag_slot),
                  0xF9400210 | (((flag_slot & 0xFFF) // 8) << 10),
                  0x39400210,  # ldrb w16, [x16]
                  0x34000050,  # cbz w16, skip original store
                  store,
                  0xF84107F0,  # ldr x16, [sp], #16
                  branch(thunk + 28, site + 4))
    for site, prologue in RETURN_ZERO:
        assert binary[site:site + len(prologue)] == prologue
        pack_into("<2I", image, site, 0x52800000, 0xD65F03C0)  # mov w0, #0; ret

    original_texture = b"hud_Cellphone\0"
    texture_init = bytes.fromhex("491400B0 29593691 2B0140F9 0B0102F9 0B011091 296140F8 696100F8")
    assert binary[0x1F7550:0x1F7550 + len(texture_init)] == texture_init
    assert binary[BLOCK_TEXTURE:BLOCK_TEXTURE + len(original_texture)] == original_texture
    image[BLOCK_TEXTURE:BLOCK_TEXTURE + len(original_texture)] = b"GymAccept".ljust(len(original_texture), b"\0")
    return bytes(image)
