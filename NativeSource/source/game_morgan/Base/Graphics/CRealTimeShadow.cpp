#include "../../../../Main.h"
#include "CRealTimeShadow.h"

#include <mach-o/dyld.h>

bool CRealTimeShadow::SetShadowedObject(CRealTimeShadow* thiz, CPhysicalGta* pPhysical)
{
    static const auto SetShadowedObject = CMemory::HasMappedBytes(SA_ADDR(0x2DB378), "\xFF\x83\x01\xD1\xEB\x2B\x01\x6D", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x2DB394), "\x01\x00\x00\xF9\x34\x10\x40\xF9\x34\x02\x00\xB4", 12, "__text") ? reinterpret_cast<bool (*)(CRealTimeShadow*, CPhysicalGta*)>(SA_ADDR(0x2DB378) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return SetShadowedObject && SetShadowedObject(thiz, pPhysical);
}