#include "../../../Main.h"
#include "CAudioEngine.h"

#include <mach-o/dyld.h>

void CAudioEngine::StartRadio(std::int8_t uiStation)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x07C8FC), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x0482C8), "\xC0\x32\x00\xD0\x00\x00\x09\x91\x42\x00\x80\x52", 12, "__text");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    reinterpret_cast<void (*)(uintptr_t, std::int8_t, std::int8_t)>(SA_ADDR(0x07C8FC) + slide)(SA_ADDR(0x6A2240) + slide, uiStation, 0);
}

void CAudioEngine::StopRadio()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x07C9AC), "\x00\x23\x00\xF0\x00\x60\x26\x91\x39\x83\xFF\x17", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x048510), "\xC0\x32\x00\xD0\x00\x00\x09\x91\x01\x00\x80\xD2\x02\x00\x80\x52", 16, "__text");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    reinterpret_cast<void (*)(uintptr_t, void*, bool)>(SA_ADDR(0x07C9AC) + slide)(SA_ADDR(0x6A2240) + slide, nullptr, false);
}