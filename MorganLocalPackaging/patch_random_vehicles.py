"""Apply the three vehicle generation RETs enabled in the current native patches."""

from hashlib import sha256


BASE_SHA = "0e64a57a7d702b5e8f0286209bb2081c86e7f1c8e95c59b6799934103c483c1c"
RET = bytes.fromhex("C0035FD6")
PATCHES = (
    (0x42674C, bytes.fromhex("F44FBEA9FD7B01A9FD430091")),
    (0x0970C0, bytes.fromhex("F44FBEA9FD7B01A9FD430091")),
    (0x4042C0, bytes.fromhex("FF4303D1EB2B066DE923076D")),
)


def patch_random_vehicles(binary):
    normalized = bytearray(binary)
    for site, original in PATCHES:
        if binary[site:site + len(original)] not in (original, RET + original[4:]):
            raise ValueError(f"Unexpected vehicle generation prologue at 0x{site:X}")
        normalized[site:site + 4] = original[:4]
    if sha256(normalized).hexdigest() != BASE_SHA:
        raise ValueError("Random vehicle RETs require the verified baseline executable")
    image = bytearray(binary)
    for site, _ in PATCHES:
        image[site:site + 4] = RET
    return bytes(image)
