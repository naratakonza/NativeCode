#include "../../Main.h"
#include "CSprite.h"

#include <mach-o/dyld.h>

bool CSprite::CalcScreenCoors(const CVector* In, CVector* pResult, float* pScaleX, float* pScaleY, bool ClipFarPlane, bool ClipNearPlane)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2E235C), "\xE9\x23\xBB\x6D\xF8\x5F\x01\xA9\xF6\x57\x02\xA9\xF4\x4F\x03\xA9", 16, "__text");
    if (!verified) {
        return false;
    }

    return reinterpret_cast<bool (*)(const CVector*, CVector*, float*, float*, bool, bool)>(
        SA_ADDR(0x2E235C) + _dyld_get_image_vmaddr_slide(0))(In, pResult, pScaleX, pScaleY, ClipFarPlane, ClipNearPlane);
}
