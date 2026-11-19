"""Extend GTA iOS's CWorld player records before the executable starts."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_cd_stream import BASE, branch, branch_cond
from patch_clothes_rebuild import BASE_SHA as SENSITIVITY_SHA
from patch_render_title import adrp, uleb


BASELINES = {
    SENSITIVITY_SHA: 0x49728C,
    "f69e1bc7bff9686e50c2636e24291f064c1ad6e238d400af87366c3cc31fc92b": 0x4972A0,
}
WORLD_PAGE = 0x741000
STOCK_PLAYERS = 0x741A68
PLAYER_IN_FOCUS = 0x741E18
PLAYER_SIZE = 0x1D8
MAX_PLAYERS = 1004
PLAYER_PED_SLOTS = 210
ACTIVE_PED_GROUPS = 0x7F2688
PLAYERS = 0xA9C000
EMPTY_RECORD = PLAYERS + MAX_PLAYERS * PLAYER_SIZE
LINKEDIT = (EMPTY_RECORD + PLAYER_SIZE + 0x3FFF) & ~0x3FFF
TEXT_START = 0x8000
TEXT_END = 0x43D94C

FIND_PLAYER_SLOT = 0x18CBB0
FIND_PLAYER_SLOT_SYMBOL = b"__ZN6CWorld28FindPlayerSlotWithPedPointerEP7CPedGta"
REMOVE_PED = 0x26D07C
CLEAR_PLAYER = 0x18E63C
RELEASE_PLAYER_DATA = 0x19234C
RET = 0xD65F03C0

PLAYER_REFERENCES = {0xA68: 235, 0xA70: 3, 0xB48: 1, 0xC40: 1}
PED_LOADS = (0x186B80, 0x186BA0, 0x18CC00, 0x18CC18)


def add_immediate(register, source, value):
    assert 0 <= value < 0x1000
    return 0x91000000 | value << 10 | source << 5 | register


def patch_world_player_slot_reuse(binary):
    """Release an occupied remote record before CREATE_PLAYER registers its replacement."""
    expected_engine = {
        0x18E63C: bytes.fromhex("F44FBEA9FD7B01A9FD430091"),
        0x19234C: bytes.fromhex("F44FBEA9FD7B01A9FD430091"),
        0x26D080: bytes.fromhex("FD7B01A9FD430091684100F0"),
        0x26CBA4: bytes.fromhex("0021009160A202F9BE95FC97"),
    }
    if any(binary[at:at + len(data)] != data for at, data in expected_engine.items()):
        raise ValueError("Unexpected iOS player-pool ownership or cleanup instructions")
    image = bytearray(binary)
    entry = 0xDDA0C
    word = unpack_from("<I", binary, entry)[0]
    if word >> 26 != 5:
        raise ValueError("Player-slot reuse requires the installed iOS CWorld patch")
    displacement = word & 0x3FFFFFF
    gate = entry + (displacement - (0x4000000 if displacement & 0x2000000 else 0)) * 4
    expected = (0x71034A9F, 0x54000202, 0x71000A9F, 0x54000163)
    if unpack_from("<4I", binary, gate) != expected:
        raise ValueError("Unexpected CREATE_PLAYER slot validation")
    section = 0x740
    if binary[section:section + 16].rstrip(b"\0") != b"__mg_gameproc":
        raise ValueError("Missing iOS engine trampoline section")
    text_size, text_offset = unpack_from("<QI", binary, section + 40)
    end = text_offset + text_size
    old_branch = branch_cond(gate + 36, gate + 68, 0xB5000008)
    installed = unpack_from("<I", binary, gate + 36)[0]
    if installed == old_branch:
        start = end
    else:
        displacement = (installed >> 5) & 0x7FFFF
        start = gate + 36 + (displacement - (0x80000 if displacement & 0x40000 else 0)) * 4
        if installed & 0xFF00001F != 0xB5000008 or not text_offset <= start < end:
            raise ValueError("Unexpected occupied-slot branch")

    words = []
    labels = {}
    fixups = []

    def emit(*instructions):
        words.extend(instructions)

    def mark(name):
        labels[name] = start + 4 * len(words)

    def jump(name, opcode):
        fixups.append((len(words), name, opcode))
        emit(0)

    def addr(register, target):
        emit(*address(register, start + 4 * len(words), target))

    def call(target):
        emit(branch(start + 4 * len(words), target, 0x94000000))

    emit(0xA9BF7BF7, 0x2A1403E0)  # preserve x23/LR, pass the remote slot
    call(REMOVE_PED)
    addr(8, PLAYERS)
    emit(0x52803B09, 0x9B292288, 0xF9400108)
    jump("create_failed", 0xB5000008)
    emit(0xA8C17BF7, branch(start + 4 * (len(words) + 1), gate + 40))
    mark("create_failed")
    emit(0xA8C17BF7, branch(start + 4 * (len(words) + 1), gate + 68))

    # Validate removals too: reset/shutdown may encounter a directly destroyed Ped.
    mark("remove")
    emit(0x7100001F | PLAYER_PED_SLOTS << 10)
    jump("return", 0x54000002)
    emit(0x7100081F)
    jump("stock", 0x54000003)
    emit(0xA9BE5FF4, 0xF9000BFE, 0x2A0003F4)  # save x20/x23/LR, slot in w20
    addr(23, PLAYERS)
    emit(0x52803B09, 0x9B295E97, 0xF94002E8)
    jump("done", 0xB4000008)
    addr(9, 0x734FD8)
    emit(0xF9400129)
    jump("done", 0xB4000009)
    emit(0xF940012A, 0xF940052B, 0xB940112C)
    jump("done", 0xB400000A)
    jump("done", 0xB400000B)
    emit(0xCB0A010D, 0x5281390E)
    emit(0x9ACE09AF, 0x9B0EB5ED)
    jump("stale", 0xB500000D)
    emit(0xEB0C01FF)
    jump("stale", 0x54000002)
    emit(0x8B0F016B, 0x3940016B, 0x1219016B)
    jump("stale", 0x3500000B)
    addr(10, 0x4A8378)
    emit(0xF9400109, 0xEB0A013F)
    jump("stale", 0x54000001)
    emit(0xF942A109, add_immediate(10, 23, 8), 0xEB0A013F)
    jump("stale", 0x54000001)
    emit(0x2A1403E0, 0xF9400BFE, 0xA8C25FF4)
    jump("stock", 0x14000000)

    mark("stale")
    emit(0xF90002FF)
    addr(8, PLAYER_IN_FOCUS)
    emit(0x39400109, 0x6B14013F)
    jump("clear", 0x54000001)
    emit(0x3900011F)
    mark("clear")
    emit(0xAA1703E0)
    call(CLEAR_PLAYER)
    emit(add_immediate(0, 23, 8))
    call(RELEASE_PLAYER_DATA)
    mark("done")
    emit(0xF9400BFE, 0xA8C25FF4)
    mark("return")
    emit(RET)
    mark("stock")
    emit(0xA9BE4FF4, branch(start + 4 * (len(words) + 1), REMOVE_PED + 4))

    for index, name, opcode in fixups:
        pc = start + index * 4
        words[index] = branch(pc, labels[name]) if opcode == 0x14000000 else branch_cond(pc, labels[name], opcode)
    payload = b"".join(instruction.to_bytes(4, "little") for instruction in words)
    remove_branch = unpack_from("<I", binary, REMOVE_PED)[0]
    displacement = remove_branch & 0x3FFFFFF
    remove_gate = REMOVE_PED + (displacement - (0x4000000 if displacement & 0x2000000 else 0)) * 4
    if remove_branch >> 26 != 5 or not text_offset <= remove_gate < start:
        raise ValueError("Unexpected iOS player-removal gate")
    removal_redirect = branch(remove_gate, labels["remove"])
    if installed != old_branch:
        if unpack_from("<I", binary, remove_gate)[0] != removal_redirect or start + len(payload) != end or binary[start:end] != payload:
            raise ValueError("Unexpected existing slot-reuse trampoline")
        return binary
    if start + len(payload) > 0x498000 or any(binary[start:start + len(payload)]):
        raise ValueError("Insufficient verified iOS trampoline space")
    if unpack_from("<I", binary, remove_gate)[0] != 0x7103481F:
        raise ValueError("Unexpected original remote-slot removal limit")
    image[start:start + len(payload)] = payload
    pack_into("<I", image, remove_gate, removal_redirect)
    pack_into("<I", image, gate + 36, branch_cond(gate + 36, start, 0xB5000008))
    pack_into("<Q", image, section + 40, text_size + len(payload))
    return bytes(image)


def address(register, pc, target):
    return adrp(register, pc, target), add_immediate(register, register, target & 0xFFF)


def world_references(binary):
    for site in range(TEXT_START, TEXT_END - 4, 4):
        page, access = unpack_from("<II", binary, site)
        register = page & 31
        if page != adrp(register, site, WORLD_PAGE):
            continue
        for offset in (*PLAYER_REFERENCES, PLAYER_IN_FOCUS & 0xFFF):
            if access == add_immediate(register, register, offset):
                yield site, register, offset, False
                break
        if site in PED_LOADS:
            offset = (access >> 10 & 0xFFF) * 8
            assert offset in (0xA68, 0xC40)
            assert access == 0xF9400000 | (offset // 8 << 10) | register << 5 | register
            yield site, register, offset, True


def bind_player_slot_lookup(image):
    # Match the verified Mach-O layout. The next __mg_hud slot follows the
    # current bindings to MorganLauncher (dylib ordinal 48).
    data_const, hud, link = 0x790, 0xA08, 0xF50
    assert image[data_const + 8:data_const + 24].rstrip(b"\0") == b"__DATA_CONST"
    assert image[hud:hud + 16].rstrip(b"\0") == b"__mg_hud"
    data_file = unpack_from("<Q", image, data_const + 40)[0]
    hud_file = unpack_from("<I", image, hud + 48)[0]
    hud_size = unpack_from("<Q", image, hud + 40)[0]
    slot = hud_file + hud_size
    assert slot in (0x4B54B0, 0x4B54B8) and not any(image[slot:slot + 8])

    bind_start, bind_size, weak_start = unpack_from("<III", image, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = image[bind_start:bind_end].rstrip(b"\0")
    table += b"\x20\x30\x40" + FIND_PLAYER_SLOT_SYMBOL + b"\x00\x51\x60\x00"
    table += b"\x72" + uleb(slot - data_file) + b"\x90\x00"
    # Keep the alignment of the existing following LINKEDIT payloads.
    table += bytes((bind_size - len(table)) % 16)
    growth = len(table) - bind_size
    vm_size, file_offset, file_size = unpack_from("<QQQ", image, link + 32)
    assert growth > 0 and file_offset + file_size == len(image) and file_size + growth <= vm_size

    image[bind_start:bind_end] = table
    pack_into("<I", image, 0xFAC, len(table))
    pack_into("<Q", image, link + 48, file_size + growth)
    pack_into("<Q", image, hud + 40, hud_size + 8)
    # dyld info, symbol tables, function starts, data-in-code and code signature.
    for field in (0xFB0, 0xFB8, 0xFC0, 0xFD0, 0xFD8, 0x1000, 0x1008, 0x1010, 0x1018, 0x1020, 0x1028, 0x1E28, 0x1E38, 0x1E48):
        offset = unpack_from("<I", image, field)[0]
        if offset:
            assert bind_end <= offset <= file_offset + file_size
            pack_into("<I", image, field, offset + growth)
    return slot


def patch_world_player_slots(binary):
    text_end = BASELINES.get(sha256(binary).hexdigest())
    if text_end is None:
        raise ValueError("CWorld requires the verified GTA iOS Sensitivity or Clothes executable")

    image = bytearray(binary)
    lookup_slot = bind_player_slot_lookup(image)
    cursor = text_end

    def write(site, *words):
        pack_into(f"<{len(words)}I", image, site, *words)

    def redirect(site, expected, target, opcode=0x14000000):
        assert unpack_from("<I", binary, site)[0] == expected, hex(site)
        write(site, branch(site, target, opcode))

    def call(site, target):
        return branch(site, target, 0x94000000)

    def emit(*words):
        nonlocal cursor
        write(cursor, *words)
        cursor += 4 * len(words)

    # Extend terminal __common and move __LINKEDIT in VM. Preserve the
    # original globals and executable addresses.
    data, common, link = 0xA58, 0xEB0, 0xF50
    assert binary[data + 8:data + 24].rstrip(b"\0") == b"__DATA"
    assert binary[common:common + 16].rstrip(b"\0") == b"__common"
    assert binary[link + 8:link + 24].rstrip(b"\0") == b"__LINKEDIT"
    data_vm, data_size = unpack_from("<QQ", binary, data + 24)
    common_vm, common_size, common_file = unpack_from("<QQI", binary, common + 32)
    assert common_file == 0 and common_vm + common_size <= BASE + PLAYERS
    assert data_vm + data_size == BASE + PLAYERS == unpack_from("<Q", binary, link + 24)[0]
    pack_into("<Q", image, common + 40, EMPTY_RECORD + PLAYER_SIZE + BASE - common_vm)
    pack_into("<Q", image, data + 32, BASE + LINKEDIT - data_vm)
    pack_into("<Q", image, link + 24, BASE + LINKEDIT)

    refs = list(world_references(binary))
    assert sum(offset == 0xE18 for _, _, offset, _ in refs) == 140
    assert [site for site, _, _, load in refs if load] == list(PED_LOADS)
    for offset, count in PLAYER_REFERENCES.items():
        assert sum(ref[2] == offset and not ref[3] for ref in refs) == count
    for site, register, offset, load in refs:
        if offset == 0xE18:
            continue
        target = PLAYERS + (offset - (STOCK_PLAYERS & 0xFFF))
        if load:
            write(site, branch(site, cursor), 0xD503201F)
            emit(*address(register, cursor, target),
                 0xF9400000 | register << 5 | register,
                 branch(cursor + 12, site + 8))
        else:
            write(site, *address(register, site, target))

    # Tail-branch to CWorld::FindPlayerSlotWithPedPointer with x0 and LR intact.
    redirect(FIND_PLAYER_SLOT, adrp(8, FIND_PLAYER_SLOT, WORLD_PAGE), cursor)
    emit(adrp(16, cursor, lookup_slot),
         0xF9400210 | ((lookup_slot & 0xFFF) // 8 << 10),  # ldr x16, [x16, #offset]
         0xD61F0200)  # br x16

    # Callers that immediately index a lookup result use zero data for a miss.
    for index, instruction in (
        (0xF989C, 0x9B2A2409), (0xFCE18, 0x9B292008),
        (0x208A0C, 0x9B292008), (0x26E920, 0x9B286C08),
    ):
        start = cursor
        redirect(index, instruction, start)
        register = instruction & 31
        emit(0x37F80060,  # tbnz w0, #31, missing
             instruction, branch(start + 8, index + 4),
             *address(register, start + 12, EMPTY_RECORD),
             branch(start + 20, index + 4))

    # The static initializer passes slot 1 to the second constructor call.
    # Continue through the extended table with the original CPlayerInfo ctor.
    start = cursor
    redirect(0x18D01C, call(0x18D01C, 0x18E564), start, 0x94000000)
    emit(0xA9BE53F3, 0xF9000BFE,  # save x19, x20, lr
         0xAA0003F3,              # mov x19, x0
         *address(20, start + 12, EMPTY_RECORD),
         0xAA1303E0, call(start + 24, 0x18E564),
         add_immediate(19, 19, PLAYER_SIZE),
         0xEB14027F,              # cmp x19, x20
         branch_cond(start + 36, start + 20, 0x54000001),
         0xF9400BFE, 0xA8C253F3, RET)

    # Preserve Clear's stock write for the two game slots. Clear a remote
    # record's own field even when another player is in focus.
    start = cursor
    redirect(0x18E6C8, adrp(8, 0x18E6C8, WORLD_PAGE), start)
    emit(*address(8, start, PLAYERS + 2 * PLAYER_SIZE),
         0xEB08027F,  # cmp x19, x8
         branch_cond(start + 12, start + 24, 0x54000003),
         0x7903027F,  # strh wzr, [x19, #0x180]
         branch(start + 20, 0x18E6E8),
         adrp(8, start + 24, PLAYER_IN_FOCUS), branch(start + 28, 0x18E6CC))

    # Reject invalid delete indices before touching the table.
    start = cursor
    redirect(REMOVE_PED, 0xA9BE4FF4, start)
    emit(0x7100001F | PLAYER_PED_SLOTS << 10,
         branch_cond(start + 4, start + 16, 0x54000002),
         0xA9BE4FF4, branch(start + 12, REMOVE_PED + 4), RET)

    # After the stock destructor, reset the remote record and release its
    # CPlayerData allocations. Keep the two local records on the stock path.
    start = cursor
    redirect(0x26D0F8, 0xF900027F, start)
    emit(0xF900027F,
         *address(8, start + 4, PLAYERS + 2 * PLAYER_SIZE),
         0xEB08027F,
         branch_cond(start + 16, start + 76, 0x54000003),
         *address(8, start + 20, PLAYER_IN_FOCUS),
         0x39400109,              # ldrb w9, [x8]
         0x52803B0A,              # mov w10, #0x1d8
         *address(11, start + 36, PLAYERS),
         0x9B2A2D29,              # smaddl x9, w9, w10, x11
         0xEB09027F,              # cmp x19, x9
         branch_cond(start + 52, start + 60, 0x54000001),
         0x3900011F,              # strb wzr, [x8]
         0xAA1303E0, call(start + 64, CLEAR_PLAYER),
         add_immediate(0, 19, 8), call(start + 72, RELEASE_PLAYER_DATA),
         branch(start + 76, 0x26D0FC))

    # Tear down occupied remote records before the game's reset/shutdown
    # routines start releasing world entities or player-data allocations.
    reset = cursor
    emit(0xA9BE53F3, 0xF9000BFE,  # save x19, x20, lr
         *address(19, reset + 8, PLAYERS + 2 * PLAYER_SIZE),
         0x52800054,              # mov w20, #2
         0xF9400268,              # ldr x8, [x19]
         branch_cond(reset + 24, reset + 36, 0xB4000008),
         0x2A1403E0, call(reset + 32, REMOVE_PED),
         add_immediate(19, 19, PLAYER_SIZE),
         0x11000694,              # add w20, w20, #1
         0x7100029F | PLAYER_PED_SLOTS << 10,
         branch_cond(reset + 48, reset + 20, 0x54000003),
         0xF9400BFE, 0xA8C253F3, RET)
    for entry in (0x1511EC, 0x151388, 0x151574, 0x151770):
        start = cursor
        redirect(entry, 0xA9BE4FF4, start)
        emit(0xA9BF7BFD, call(start + 4, reset), 0xA8C17BFD,
             0xA9BE4FF4, branch(start + 16, entry + 4))

    # CREATE_PLAYER uses a signed int SCM operand. Validate it before the
    # original constructor, and reject duplicate remote slots.
    start = cursor
    redirect(0xDDA0C, 0x93407E96, start)
    emit(0x7100029F | PLAYER_PED_SLOTS << 10,
         branch_cond(start + 4, start + 68, 0x54000002),
         0x71000A9F,  # cmp w20, #2
         branch_cond(start + 12, start + 56, 0x54000003),
         *address(8, start + 16, PLAYERS),
         0x52803B09,  # mov w9, #0x1d8
         0x9B292288,  # smaddl x8, w20, w9, x8
         0xF9400108,  # ldr x8, [x8]
         branch_cond(start + 36, start + 68, 0xB5000008),
         # AddGroup(0x25E2F4) scans eight flags and returns -1 when all are 1.
         # The stock CPlayerPed constructor immediately uses that result.
         adrp(8, start + 40, ACTIVE_PED_GROUPS),
         0xF9434508,  # ldr x8, [x8, #0x688]
         0xB200C3E9,  # mov x9, #0x0101010101010101
         0xEB09011F,  # cmp x8, x9
         0x93407E96, branch_cond(start + 60, start + 68, 0x54000000),
         branch(start + 64, 0xDDA10),
         0x12800008, 0xB90172A8,  # ScriptParams[0] = -1
         0xAA1303E0, 0x52800021, call(start + 84, 0xDAC28),
         branch(start + 88, 0xDD708))
    create_failed = start + 68

    # The stock Ped pool allocator can return null. Exit SetupPlayerPed
    # before dereferencing it, and return -1 from the opcode.
    start = cursor
    redirect(0x26CFB0, 0xAA0003F3, start)
    emit(branch_cond(start, start + 12, 0xB4000000),
         0xAA0003F3, branch(start + 8, 0x26CFB4), branch(start + 12, 0x26D050))
    start = cursor
    redirect(0xDDA58, 0xF94002C0, start)
    emit(0xF94002C0,
         branch_cond(start + 4, start + 12, 0xB4000000),
         branch(start + 8, 0xDDA5C), branch(start + 12, create_failed))

    # GET_PLAYER_CHAR (01F5) computes its Ped pool handle inline. Check the
    # index and the Ped before the subtraction.
    start = cursor
    redirect(0xF8AF4, 0x52803B0A, start)
    emit(0x7100013F | PLAYER_PED_SLOTS << 10,
         branch_cond(start + 4, start + 16, 0x54000002),
         0x52803B0A, branch(start + 12, 0xF8AF8),
         0x1280000A, 0xB900010A, branch(start + 24, 0xF98F0))
    get_char_failed = start + 16
    start = cursor
    redirect(0xF8B10, 0xF8696949, start)
    emit(0xF8696949,
         branch_cond(start + 4, start + 12, 0xB4000009),
         branch(start + 8, 0xF8B14), branch(start + 12, get_char_failed))

    # A queried slot can have no Ped during creation or after removal.
    start = cursor
    redirect(0x1914D0, 0x39561509, start)
    emit(branch_cond(start, start + 12, 0xB4000008),
         0x39561509, branch(start + 8, 0x1914D4),
         *address(0, start + 12, EMPTY_RECORD + 0x68), RET)

    assert 0x497000 + unpack_from("<Q", binary, 0x768)[0] == text_end
    assert cursor <= 0x498000 and not any(binary[text_end:cursor])
    pack_into("<Q", image, 0x768, cursor - 0x497000)
    return bytes(image)
