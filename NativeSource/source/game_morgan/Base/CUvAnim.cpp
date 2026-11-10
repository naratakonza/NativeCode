#include "../../../Main.h"
#include "CUvAnim.h"

static_assert(offsetof(RpUVAnimKeyFrame, data) == 0x0C);
static_assert(offsetof(RpUVAnimInterpFrame, data) == 0x10);

void CUvAnim::Initialise()
{
    CNecessary::Log(OBFUSCATE("CUvAnim Initialise"));

    // RpUVAnimParamKeyFrameApply อ่าน payload ของ interpolated frame ที่ +0x10
    CMemory::WriteProcessMemory(SA_ADDR(0x29E5AC), (uintptr_t)"\x20\x14\x40\xBD", 4); // LDR s0, [x1, #0x14]
    CMemory::WriteProcessMemory(SA_ADDR(0x29E5B4), (uintptr_t)"\x20\x1C\x40\xBD", 4); // LDR s0, [x1, #0x1c]
    CMemory::WriteProcessMemory(SA_ADDR(0x29E5C4), (uintptr_t)"\x20\x18\x40\xBD", 4); // LDR s0, [x1, #0x18]
    CMemory::WriteProcessMemory(SA_ADDR(0x29E5D8), (uintptr_t)"\x20\x20\x40\xBD", 4); // LDR s0, [x1, #0x20]
    CMemory::WriteProcessMemory(SA_ADDR(0x29E5E0), (uintptr_t)"\x20\x24\x40\xBD", 4); // LDR s0, [x1, #0x24]
    CMemory::WriteProcessMemory(SA_ADDR(0x29E604), (uintptr_t)"\x60\x12\x40\xBD", 4); // LDR s0, [x19, #0x10]

    // RpMaterialUVAnimApplyUpdate มีทาง Param UV ที่ inline การอ่านชุดเดียวกัน
    CMemory::WriteProcessMemory(SA_ADDR(0x29F6D8), (uintptr_t)"\xC2\x02\x43\x2D", 4); // LDP s2, s0, [x22, #0x18]
    CMemory::WriteProcessMemory(SA_ADDR(0x29F6DC), (uintptr_t)"\xC1\x16\x40\xBD", 4); // LDR s1, [x22, #0x14]
    CMemory::WriteProcessMemory(SA_ADDR(0x29F708), (uintptr_t)"\xC0\x22\x40\xBD", 4); // LDR s0, [x22, #0x20]
    CMemory::WriteProcessMemory(SA_ADDR(0x29F710), (uintptr_t)"\xC0\x26\x40\xBD", 4); // LDR s0, [x22, #0x24]
    CMemory::WriteProcessMemory(SA_ADDR(0x29F738), (uintptr_t)"\xC0\x12\x40\xBD", 4); // LDR s0, [x22, #0x10]
}