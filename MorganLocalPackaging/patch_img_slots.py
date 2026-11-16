"""Run the native GTA iOS IMG directory pass across all 32 archive slots."""

from hashlib import sha256
from struct import pack, pack_into, unpack_from

from patch_cd_stream import branch, branch_cond
from patch_render_title import adrp


BASE_SHA = "18a75f1a454bbe8771d5dce96cc22b09104b296eb6bd5a81b55b0ca5c3e57848"
DIRECTORIES = 0x14D268
CONTINUE = 0x14D480
OLD_SYMBOL = b"_CStreaming_LoadExtraImgDirectories_hook"
SYMBOLS = (b"_CStreaming_InitImageList_hook", b"_CStreaming_AddImageToList_hook")
IMG32_SHA = "2b791ae656c31065f2a148ffbbf5c6c9d11c69686b3a891064e2ae22fcd4adf7"
CD_WORKER = 0x14D2D4
CD_FAILURES = 0x14D304


def patch_img_slots(binary):
    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError("IMG slot patch requires the verified RadarMod executable")

    # Replace the unrolled eight-entry block. Keep the game's directory and progress calls.
    words = (
        adrp(19, 0x14D268, 0xA99000),
        0x91100273,  # add x19, x19, #0x400
        0x52800016,  # mov w22, #0
        adrp(21, 0x14D274, 0x7AE000),
        0x910E82B5,  # add x21, x21, #0x3a0
        0x39400268,  # loop: ldrb w8, [x19]
        branch_cond(0x14D280, 0x14D2B8, 0x34000008),  # cbz w8, done
        0x3940A268,  # ldrb w8, [x19, #0x28]
        branch_cond(0x14D288, 0x14D298, 0x34000008),  # cbz w8, progress
        0xAA1303E0,  # mov x0, x19
        0x2A1603E1,  # mov w1, w22
        branch(0x14D294, 0x14D5CC, 0x94000000),  # LoadCdDirectory
        0x394002A8,  # progress: ldrb w8, [x21]
        branch_cond(0x14D29C, 0x14D2C0, 0x34000008),  # cbz w8, update
        branch(0x14D2A0, 0x21907C, 0x94000000),  # DisplayPCScreen
        branch(0x14D2A4, 0x14D2A8),
        0x110006D6,  # next: add w22, w22, #1
        0x9100C273,  # add x19, x19, #48
        0x710082DF,  # cmp w22, #32
        branch_cond(0x14D2B4, 0x14D27C, 0x54000003),  # b.lo loop
        adrp(8, 0x14D2B8, 0x8E7000),  # done: restore input to the continuation
        branch(0x14D2BC, CONTINUE),
        adrp(8, 0x14D2C0, 0x8E8000),  # update:
        0x39496108,  # ldrb w8, [x8, #0x258]
        branch_cond(0x14D2C8, 0x14D2A8, 0x34000008),  # cbz w8, next
        branch(0x14D2CC, 0x15A480, 0x94000000),  # UpdateLoading
        branch(0x14D2D0, 0x14D2A8),
    )
    code = pack(f"<{len(words)}I", *words)
    unused = CONTINUE - DIRECTORIES - len(code)
    image = bytearray(binary)
    image[DIRECTORIES:CONTINUE] = code + pack("<I", 0xD503201F) * (unused // 4)

    # Each surrounding bind specifies its own dylib, symbol, type, addend and address.
    bind_end = 0x4ED038 + 0x2158
    record = b"\x20\x30\x40" + OLD_SYMBOL + b"\x00\x51\x60\x00\x72\x80\xa9\x07\x90"
    start = binary.index(record, 0x4ED038, bind_end)
    rest = start + len(record)
    assert binary.startswith(b"\x20\x30\x40_Render2dStuff_hook\0", rest)
    image[start:bind_end] = binary[rest:bind_end] + bytes(len(record))
    return bytes(image)


def patch_img_read_failures(binary):
    if sha256(binary).hexdigest() != IMG32_SHA:
        raise ValueError("CD read repair requires the verified GTA iOS IMG32 executable")

    # Stock seek/read errors use 254. Only a null handle or an unrepresentable
    # sector gets 253, since resubmitting either request cannot possibly help.
    worker = (
        branch_cond(0x14D2D4, 0x14D2F4, 0xB4000000),  # cbz x0, terminal
        0xB9400348,  # ldr w8, [x26]
        0x7144011F,  # cmp w8, #0x100, lsl #12
        branch_cond(0x14D2E0, 0x14D2F4, 0x54000002),  # b.hs terminal
        0x53155101,  # lsl w1, w8, #11
        branch(0x14D2E8, 0x34818, 0x94000000),  # seek
        branch_cond(0x14D2EC, 0x14D300, 0x35000000),  # cbnz w0, seek_error
        branch(0x14D2F0, 0x2198A0),  # read
        0x52801FA8,  # terminal: mov w8, #253
        0xB9001748,  # str w8, [x26, #0x14]
        branch(0x14D2FC, 0x2198C0),  # normal queue completion
        branch(0x14D300, 0x4971E8),  # seek_error: preserve stock status 254
    )
    assert CD_WORKER + 4 * len(worker) == CD_FAILURES

    clear_channel = 0x14D34C
    failures = (
        # ProcessLoadingChannel
        0x7103F51F,  # cmp w8, #253
        branch_cond(0x14D308, 0x14D318, 0x54000001),  # b.ne original
        0x2A1303E0,  # mov w0, w19
        branch(0x14D310, clear_channel, 0x94000000),
        branch(0x14D314, 0x307D34),
        adrp(9, 0x14D318, 0x8E7000),  # displaced instruction
        branch(0x14D31C, 0x307B84),
        # RetryLoadFile can consume the status at either of its two polls.
        0x7103F41F,  # cmp w0, #253
        branch_cond(0x14D324, 0x14D340, 0x54000000),  # b.eq retry_terminal
        0x7103E81F,  # cmp w0, #250 (displaced)
        branch(0x14D32C, 0x307A60),
        0x7103F41F,  # cmp w0, #253
        branch_cond(0x14D334, 0x14D340, 0x54000000),  # b.eq retry_terminal
        0x7103FC1F,  # cmp w0, #255 (displaced)
        branch(0x14D33C, 0x307A78),
        0x2A1303E0,  # retry_terminal: mov w0, w19
        branch(0x14D344, clear_channel, 0x94000000),
        branch(0x14D348, 0x307AA0),  # stock epilogue resets the retry index
    )

    # Drop the failed channel's queued models through GTA's own RemoveModel.
    # Channel 0 alone can own the big-model buffer shared with channel 1.
    cleanup = (
        0xA9BD4FF4,  # stp x20, x19, [sp, #-0x30]!
        0xF9000BF5,  # str x21, [sp, #0x10]
        0xA9027BFD,  # stp x29, x30, [sp, #0x20]
        0x910083FD,  # add x29, sp, #0x20
        0x2A0003F5,  # mov w21, w0
        adrp(8, 0x14D360, 0x8E7000),
        0x913C6108,  # add x8, x8, #0xf18
        0x52801309,  # mov w9, #152
        0x9B2922B3,  # smaddl x19, w21, w9, x8
        0x52800014,  # mov w20, #0
        0xB8745A60,  # next_model: ldr w0, [x19, w20, uxtw #2]
        0x3100041F,  # cmn w0, #1
        branch_cond(0x14D37C, 0x14D38C, 0x54000000),  # b.eq next
        branch(0x14D380, 0x304CC0, 0x94000000),  # RemoveModel
        0x12800008,  # mov w8, #-1
        0xB8345A68,  # str w8, [x19, w20, uxtw #2]
        0x11000694,  # next: add w20, w20, #1
        0x7100429F,  # cmp w20, #16
        branch_cond(0x14D394, 0x14D374, 0x54000003),  # b.lo next_model
        0xB900827F,  # str wzr, [x19, #0x80]
        branch_cond(0x14D39C, 0x14D3C4, 0x35000015),  # cbnz w21, done
        adrp(8, 0x14D3A0, 0x8E8000),
        0x39418109,  # ldrb w9, [x8, #0x60]
        branch_cond(0x14D3A8, 0x14D3C4, 0x34000009),  # cbz w9, done
        0x3901811F,  # strb wzr, [x8, #0x60]
        adrp(8, 0x14D3B0, 0x8E7000),
        0x913EC108,  # add x8, x8, #0xfb0
        0x6F07E7E0,  # movi v0.2d, #-1
        0xAD000100,  # stp q0, q0, [x8]
        0xAD010100,  # stp q0, q0, [x8, #0x20]
        0xA9427BFD,  # done: ldp x29, x30, [sp, #0x20]
        0xF9400BF5,  # ldr x21, [sp, #0x10]
        0xA8C34FF4,  # ldp x20, x19, [sp], #0x30
        0xD65F03C0,  # ret
    )
    assert CD_FAILURES + 4 * len(failures) == clear_channel
    assert CD_WORKER + 4 * (len(worker) + len(failures) + len(cleanup)) <= CONTINUE
    image = bytearray(binary)
    words = worker + failures + cleanup
    assert unpack_from(f"<{len(words)}I", binary, CD_WORKER) == (0xD503201F,) * len(words)
    pack_into(f"<{len(words)}I", image, CD_WORKER, *words)

    for site, original, target in (
        (0x219894, branch(0x219894, 0x4971C8), CD_WORKER),
        (0x307B80, adrp(9, 0x307B80, 0x8E7000), CD_FAILURES),
        (0x307A5C, 0x7103E81F, 0x14D320),
        (0x307A74, 0x7103FC1F, 0x14D330),
        # A missing current clothing model already has a stock empty-slot path.
        (0x4971F4, branch(0x4971F4, 0x82EC4), 0x82E50),
    ):
        assert unpack_from("<I", binary, site)[0] == original
        pack_into("<I", image, site, branch(site, target))
    return bytes(image)
