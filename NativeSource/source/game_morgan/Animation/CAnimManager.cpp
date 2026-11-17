#include "../../../Main.h"
#include "CAnimManager.h"

#include <mach-o/dyld.h>

uintptr_t CAnimManager::GetAnimationBlock(const char* name)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x42150), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9\xFD\x7B\x02\xA9", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x4217C), "\xE0\x03\x14\xAA\xE1\x03\x13\xAA\xA6\xF1\x0F\x94", 12, "__text");
    if (!verified || !name) {
        return 0;
    }

    return reinterpret_cast<uintptr_t (*)(const char*)>(SA_ADDR(0x42150) + _dyld_get_image_vmaddr_slide(0))(name);
}

void CAnimManager::LoadAnimFile(RwStream* pStream, bool bLoadCompressed, char const(*uncompressedAnimations)[32])
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x43058), "\xED\x33\xB7\x6D\xEB\x2B\x01\x6D\xE9\x23\x02\x6D", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x430A4), "\x02\x01\x80\x52\x78\xDC\x09\x94\xEA\x8B\x40\xB9", 12, "__text");
    if (!verified || !pStream || !pthread_main_np()) {
        return;
    }

    reinterpret_cast<void (*)(RwStream*, bool, char const(*)[32])>(SA_ADDR(0x43058) + _dyld_get_image_vmaddr_slide(0))(pStream, bLoadCompressed, uncompressedAnimations);
}