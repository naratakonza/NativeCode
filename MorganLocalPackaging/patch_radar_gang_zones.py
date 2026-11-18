"""Verified GTA SA iOS 2.2.21 radar bridges and Thunder colour/map patches."""
from hashlib import sha256
from struct import pack_into, unpack_from
from patch_cd_stream import branch
from patch_render_title import adrp, uleb

BASE_SHA = '3280315911960b4cbea14fe4bd4c32c895a51bfac67ebd7a325a94bc6933014b'
THUNK = 0x497888
SLOT = 0x4B55B0
HOOKS = (
    (0x1F47D4, 4, b'_CRadar_SetCoordBlip_bridge'),
    (0x1ECD6C, 1, b'_PlaceRedMarker_bridge'),
    (0x1F5D00, 1, b'_CRadar_DrawRadarGangOverlay_bridge'),
    (0x2187B0, 2, b'_CHudColours_GetIntColour_bridge'),
    (0x1F5A68, 3, b'_CRadar_DrawAreaOnRadar_bridge'),
)
CRASH_FIX_HOOKS = (
    (0x2C57F8, 1, b'_RpMaterialDestroy_hook'),
    (0x2B03EC, 1, b'_RwTextureDestroy_hook'),
)
CRASH_FIX_EXPECTED = {site: bytes.fromhex('F44FBEA9') for site, _, _ in CRASH_FIX_HOOKS}

PATCHES = (
    (0x1ED22C, bytes.fromhex('096EA852')),
    (0x1F42C0, bytes.fromhex('E103142A')),
    (0x1F42C4, bytes.fromhex('1F000014')),
    (0x1F2810, bytes.fromhex('E103162A')),
    (0x1F2814, bytes.fromhex('22000014')),
    (0x1F1E30, bytes.fromhex('E103162A')),
    (0x1F1E34, bytes.fromhex('0F000014')),
)
EXPECTED = {0x1F47D4: bytes.fromhex('08008092'), 0x1ECD6C: bytes.fromhex('f44fbea9'), 0x1F5D00: bytes.fromhex('ff0302d1'), 0x1F5A68: bytes.fromhex('FF8304D1'), 0x2187B0: bytes.fromhex('0848218b'), 0x1ED22C: bytes.fromhex('097ca852'), 0x1F42C0: bytes.fromhex('9f220071'), 0x1F42C4: bytes.fromhex('68040054'), 0x1F2810: bytes.fromhex('df220071'), 0x1F2814: bytes.fromhex('e8040054'), 0x1F1E30: bytes.fromhex('df220071'), 0x1F1E34: bytes.fromhex('68020054')}  # Filled from the verified unmodified instructions, never inferred at packaging time.

def patch_radar_gang_zones(binary, *, radar=True, crash_fix=False):
    hooks = (HOOKS if radar else ()) + (CRASH_FIX_HOOKS if crash_fix else ())
    patches = PATCHES if radar else ()
    if not hooks:
        return binary

    if sha256(binary).hexdigest() != BASE_SHA:
        raise ValueError('Radar or RenderWare crash fixes require the verified Keybar25 executable')
    for offset, data in ((EXPECTED if radar else {}) | (CRASH_FIX_EXPECTED if crash_fix else {})).items():
        if binary[offset:offset + len(data)] != data:
            raise ValueError(f'Unexpected radar instruction at 0x{offset:X}')
    text_size, text_offset = unpack_from('<QI', binary, 0x768)
    hud_size, hud_offset = unpack_from('<QI', binary, 0xA30)
    assert text_size + text_offset == THUNK and hud_size + hud_offset == SLOT
    assert not any(binary[THUNK:THUNK+32*len(hooks)])
    assert not any(binary[SLOT:SLOT+8*len(hooks)])
    assert THUNK + 32*len(hooks) <= 0x498000 and SLOT + 8*len(hooks) <= 0x4B8000
    data_file = unpack_from('<Q', binary, 0x7B8)[0]
    bind_start, bind_size, weak_start = unpack_from('<III', binary, 0xFA8)
    bind_end = bind_start + bind_size
    assert bind_end == weak_start
    table = binary[bind_start:bind_end].rstrip(b'\0')
    for idx, (_, _, symbol) in enumerate(hooks):
        table += b'\x20\x30\x40' + symbol + b'\0\x51\x60\x00\x72' + uleb(SLOT + idx*8 - data_file) + b'\x90'
    table += b'\0'
    table += bytes((bind_size-len(table)) % 16)
    growth = len(table)-bind_size
    vm_size, link_offset, link_size = unpack_from('<QQQ', binary, 0xF70)
    assert growth > 0 and link_offset+link_size == len(binary) and link_size+growth <= vm_size
    image = bytearray(binary)
    image[bind_start:bind_end] = table
    pack_into('<I', image, 0xFAC, len(table))
    pack_into('<Q', image, 0xF80, link_size+growth)
    for field in (0xFB0, 0xFB8, 0xFC0, 0xFD0, 0xFD8, 0x1000, 0x1008, 0x1010, 0x1018, 0x1020, 0x1028, 0x1E28, 0x1E38, 0x1E48):
        offset = unpack_from('<I', binary, field)[0]
        if offset:
            assert bind_end <= offset <= link_offset + link_size
            pack_into('<I', image, field, offset+growth)
    pack_into('<Q', image, 0x768, text_size+32*len(hooks))
    pack_into('<Q', image, 0xA30, hud_size+8*len(hooks))
    for idx, (site, reg, _) in enumerate(hooks):
        thunk = THUNK + idx*32
        original = thunk+24
        slot = SLOT + idx*8
        pack_into('<8I', image, thunk,
            adrp(reg, thunk, original), 0x91000000 | ((original & 0xFFF) << 10) | (reg << 5) | reg,
            adrp(16, thunk+8, slot), 0xF9400210 | (((slot & 0xFFF)//8) << 10),
            0xD61F0200, 0xD503201F, unpack_from('<I', binary, site)[0], branch(thunk+28, site+4))
        pack_into('<I', image, site, branch(site, thunk))
    for site, data in patches:
        image[site:site+len(data)] = data
    return bytes(image)
