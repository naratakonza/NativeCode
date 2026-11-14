#include "CCommon.h"
#include "Models/CModelInfo.h"
#include "../utils/Memory/CMemory.h"

#include <mach-o/dyld.h>

uintptr_t CCommon::GetModelInfoByID(int iModelID)
{
    return reinterpret_cast<uintptr_t>(CModelInfo::GetModelInfo(iModelID));
}

bool CCommon::IsGameEntityArePlaceable(CEntityGta *pEntity)
{
    if (!pEntity) {
        return false;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16C634), "\x68\x19\x00\xF0\x08\x41\x35\x91\x08\x7C\x00\xA9", 12, "__text");
    return verified && pEntity->vtable == SA_ADDR(0x49BD50) + _dyld_get_image_vmaddr_slide(0);
}

bool CCommon::IsGameEntityRenderable(CEntityGta *pEntity)
{
    return pEntity && pEntity->vtable && !CCommon::IsGameEntityArePlaceable(pEntity) && pEntity->m_pRwObject;
}

bool CCommon::IsValidGamePed(CPedGta *pPed)
{
    if (!pPed || !pthread_main_np()) {
        return false;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x245768), "\x68\x27\x00\xF0\x08\x61\x3F\x91\x08\x01\x40\xF9", 12, "__text");
    if (!verified) {
        return false;
    }

    return reinterpret_cast<bool (*)(CPedGta*)>(SA_ADDR(0x24575C) + _dyld_get_image_vmaddr_slide(0))(pPed);
}

bool CCommon::IsPedModel(unsigned int iModelID)
{
    if (iModelID >= CModelInfo::NUM_MODEL_INFOS) {
        return false;
    }

    const auto* modelInfo = reinterpret_cast<const CBaseModelInfo*>(CCommon::GetModelInfoByID(static_cast<int>(iModelID)));
    if (!modelInfo) {
        return false;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x22EB40), "\xC8\x13\x00\x90\x08\x01\x29\x91\x16\x41\x00\x91", 12, "__text") && CMemory::HasMappedInstruction(SA_ADDR(0x22EB60), "\x96\x02\x00\xF9");
    return verified && modelInfo->vtable == SA_ADDR(0x4A6A50) + _dyld_get_image_vmaddr_slide(0);
}
