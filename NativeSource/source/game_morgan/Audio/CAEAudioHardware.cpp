#include "../../../Main.h"
#include "CAEAudioHardware.h"

#include <mach-o/dyld.h>

void CAEAudioHardware::SetMusicMasterScalingFactor(float fScalingFactor)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x04A8B4), "\x00\xF0\x01\xBD\xC0\x03\x5F\xD6", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x07C864), "\xC0\x30\x00\xB0\x00\x40\x0B\x91\x12\x38\xFF\x17", 12, "__text");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    reinterpret_cast<void (*)(uintptr_t, float)>(SA_ADDR(0x04A8B4) + slide)(SA_ADDR(0x6952D0) + slide, fScalingFactor);
}

float CAEAudioHardware::GetMusicMasterScalingFactor()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x04A8DC), "\x00\xF0\x41\xBD\xC0\x03\x5F\xD6", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x07C864), "\xC0\x30\x00\xB0\x00\x40\x0B\x91\x12\x38\xFF\x17", 12, "__text");
    if (!verified) {
        return 0.0f;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    const auto getVolume = reinterpret_cast<float (*)(uintptr_t)>(SA_ADDR(0x04A8DC) + slide);
    return getVolume(SA_ADDR(0x6952D0) + slide);
}