#include "../../../Main.h"
#include "CPad.h"

#include <mach-o/dyld.h>

static bool HasPadControlFlag()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x15B6F8), "\xC8\x2E\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x15B6FC), "\x08\xF1\x28\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x43AE90), "\x08\x20\x42\x79");
    return verified;
}

void CPad::DisplayWidgets(bool bDisp)
{
    if (!HasPadControlFlag()) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    *reinterpret_cast<std::uint16_t*>(SA_ADDR(0x734A3C + 0x110) + slide) = static_cast<std::uint16_t>(!bDisp); // Pads[0].DisablePlayerControls
}

bool CPad::IsActiveWidgets()
{
    return HasPadControlFlag() && *reinterpret_cast<const std::uint16_t*>(SA_ADDR(0x734A3C + 0x110) + _dyld_get_image_vmaddr_slide(0)) != 0;
}
