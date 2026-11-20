#include "../../../Main.h"
#include "CKeyGen.h"

#include <mach-o/dyld.h>

uint32_t CKeyGen::GetUppercaseKey(const char* str)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x158B6C), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9\xFD\x7B\x02\xA9", 12, "__text");
    if (!verified)
    {
        return 0;
    }

    return reinterpret_cast<uint32_t (*)(const char*)>(SA_ADDR(0x158B6C) + _dyld_get_image_vmaddr_slide(0))(str);
}