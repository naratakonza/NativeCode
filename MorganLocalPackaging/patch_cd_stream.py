"""Fix rejected directory lookups and failed CD reads in the verified GTA iOS build."""

from hashlib import sha256
from struct import pack_into, unpack_from

from patch_render_title import adrp


BASE_SHA = "1bb5409def07f30f27dd5fc0de81d82e9f8a54d963593b31c575eaa0c55bad3c"
BASE = 0x100000000
TEXT_THUNK = 0x4971A0
SPECIAL_MODEL = 0x30C4F4
SPECIAL_DONE = 0x30C768
CD_READ = 0x219894
CD_STATUS = 0x2198C0
DIRECTORY_LOOKUP = 0x141924


def branch(pc, target, opcode=0x14000000):
    words = (target - pc) // 4
    assert (target - pc) % 4 == 0 and -(1 << 25) <= words < (1 << 25)
    return opcode | (words & 0x03FFFFFF)


def branch_cond(pc, target, opcode):
    words = (target - pc) // 4
    assert (target - pc) % 4 == 0 and -(1 << 18) <= words < (1 << 18)
    return opcode | ((words & 0x7FFFF) << 5)


def patch_cd_stream(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("CD streaming patch requires the verified GTA iOS 2.2.21 executable")

    special = TEXT_THUNK
    read = special + 10 * 4
    failed = read + 8 * 4
    clothes = failed + 2 * 4
    # RequestSpecialModel already holds the name in x21 and owns stack slots +8 and +12.
    # Check the directory before changing model metadata. A miss takes the ordinary epilogue.
    special_words = (
        adrp(0, special, 0x8E8000),
        0xF940F400,  # ldr x0, [x0, #0x1E8]
        0xAA1503E1,  # mov x1, x21
        0x910023E2,  # add x2, sp, #8
        0x910033E3,  # add x3, sp, #12
        branch(special + 20, DIRECTORY_LOOKUP, 0x94000000),
        branch_cond(special + 24, special + 32, 0x35000000),  # cbnz w0, resume
        branch(special + 28, SPECIAL_DONE),
        0x79C046C8,  # ldrsh w8, [x22, #0x22] (displaced instruction)
        branch(special + 36, SPECIAL_MODEL + 4),
    )
    read_words = (
        branch_cond(read, failed, 0xB4000000),  # cbz x0, failed
        0xB9400348,  # ldr w8, [x26] (channel sector)
        0x7144011F,  # cmp w8, #0x100, lsl #12 (first non-representable sector)
        branch_cond(read + 12, failed, 0x54000002),  # b.hs failed
        0x53155101,  # lsl w1, w8, #11
        branch(read + 20, 0x34818, 0x94000000),
        branch_cond(read + 24, failed, 0x35000000),  # cbnz w0, failed
        branch(read + 28, CD_READ + 12),
        0xB9001757,  # str w23, [x26, #0x14] (stock read error 254)
        branch(failed + 4, CD_STATUS),
    )
    # Both clothes call sites use the same two stack outputs. Keep the caller's LR
    # and skip only the missing geometry request (including its stream wait).
    clothes_sites = ((0x82EA0, 0x82EC4), (0x82F08, 0x82F30))
    queue_handle = clothes + 16 * len(clothes_sites)
    invalid_handle = queue_handle + 28
    queue_words = (
        0x7100815F,  # cmp w10, #32
        branch_cond(queue_handle + 4, invalid_handle, 0x54000002),  # b.hs invalid
        adrp(9, queue_handle + 8, 0x509000),
        0x91132129,  # add x9, x9, #0x4C8 (CD state)
        0x8B2A4D29,  # add x9, x9, w10, uxtw #3
        0xF9401D29,  # ldr x9, [x9, #0x38]
        branch(queue_handle + 24, queue_handle + 32),
        0xAA1F03E9,  # mov x9, xzr
        0xF9001509,  # str x9, [x8, #0x28]
        0x12005C49,  # and w9, w2, #0xFFFFFF (displaced instruction)
        branch(queue_handle + 40, 0x219B1C),
    )

    image = bytearray(binary)
    cursor = 32
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<II", binary, cursor)
        if command == 0x19 and binary[cursor + 8:cursor + 24].rstrip(b"\0") == b"__TEXT":
            break
        cursor += size
    else:
        raise ValueError("Missing GTA __TEXT")

    extent = 4 * (len(special_words) + len(read_words) + len(queue_words)) + 16 * len(clothes_sites)
    text_end = unpack_from("<Q", binary, cursor + 48)[0]
    assert TEXT_THUNK + extent <= text_end
    assert not any(binary[TEXT_THUNK:TEXT_THUNK + extent])
    thunk_section = None
    for index in range(unpack_from("<I", binary, cursor + 64)[0]):
        section = cursor + 72 + index * 80
        name = binary[section:section + 16].rstrip(b"\0")
        address, size, offset = unpack_from("<QQI", binary, section + 32)
        assert address + size <= BASE + TEXT_THUNK or address >= BASE + TEXT_THUNK + extent
        if name == b"__mg_gameproc":
            assert address == BASE + offset and offset + size == TEXT_THUNK
            thunk_section = section + 40, size + extent
    assert thunk_section is not None
    pack_into("<Q", image, thunk_section[0], thunk_section[1])

    assert unpack_from("<I", binary, SPECIAL_MODEL)[0] == special_words[8]
    assert unpack_from("<I", binary, 0x30C6F0)[0] == branch(0x30C6F0, DIRECTORY_LOOKUP, 0x94000000)
    assert unpack_from("<3I", binary, CD_READ) == (0xB8686928, 0x53155101, 0x97F86BDF)
    assert unpack_from("<2I", binary, 0x21984C) == (0x54000101, 0x12800008)

    pack_into("<I", image, SPECIAL_MODEL, branch(SPECIAL_MODEL, special))
    pack_into("<I", image, 0x30C6F0, 0xD503201F)  # Reuse the checked directory result.
    pack_into("<3I", image, CD_READ, branch(CD_READ, read), 0xD503201F, 0xD503201F)
    # A semaphore wakeup with an empty queue must never index channel -1.
    pack_into("<2I", image, 0x21984C, branch_cond(0x21984C, 0x219838, 0x54000000), branch(0x219850, 0x21986C))
    pack_into(f"<{len(special_words)}I", image, special, *special_words)
    pack_into(f"<{len(read_words)}I", image, read, *read_words)

    # Resolve the handle after the busy checks, so rejected reads leave the active channel intact.
    assert unpack_from("<3I", binary, 0x219AE4) == (0x8B2A4D29, 0xF9401D29, 0xF9001509)
    assert unpack_from("<I", binary, 0x219B18)[0] == 0x12005C49
    pack_into("<3I", image, 0x219AE4, 0xD503201F, 0xD503201F, 0xD503201F)
    pack_into("<I", image, 0x219B18, branch(0x219B18, queue_handle))
    pack_into(f"<{len(queue_words)}I", image, queue_handle, *queue_words)
    # Patch the post-lookup LDP, replaying it at each successful branch target.
    # Keep the existing geometry load block and use separate targets for each skip.
    assert unpack_from("<I", binary, 0x82EA0)[0] == 0x294187E2
    assert unpack_from("<I", binary, 0x82F08)[0] == 0x294187E2
    for i, (site, missing) in enumerate(clothes_sites):
        thunk = clothes + i * 16
        pack_into("<I", image, site, branch(site, thunk))
        pack_into("<4I", image, thunk,
                  branch_cond(thunk, thunk + 8, 0x35000000), branch(thunk + 4, missing),
                  0x294187E2, branch(thunk + 12, site + 4))
    return bytes(image)
