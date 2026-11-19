#include "../../../../Main.h"
#include "CTaskSimpleJetPack.h"

#include <mach-o/dyld.h>

void CTaskSimpleJetPack::Constructor(uintptr_t thiz, const CVector* pVecTargetPos, float fCruiseHeight, int nHoverTime, CEntityGta* pTargetEnt)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3B5954), "\xE9\x23\xBC\x6D\xF6\x57\x01\xA9", 8, "__text");
    if (verified && thiz)
    {
        reinterpret_cast<void (*)(uintptr_t, const CVector*, float, int, CEntityGta*)>(SA_ADDR(0x3B5954) + _dyld_get_image_vmaddr_slide(0))(thiz, pVecTargetPos, fCruiseHeight, nHoverTime, pTargetEnt);
    }
}

void CTaskSimpleJetPack::Destructor(uintptr_t thiz)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3B5AC0), "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91\xD3\xFF\xFF\x97", 12, "__text");
    if (verified && thiz)
    {
        reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x3B5AC0) + _dyld_get_image_vmaddr_slide(0))(thiz);
    }
}