#include "../../../Main.h"
#include "CCrashFix.h"

extern "C" __attribute__((visibility("default"))) RwBool RpMaterialDestroy_hook(RpMaterial* material, RwBool (*original)(RpMaterial*))
{
    if (CNecessary::IsBadWritePtr(material, sizeof(*material))) {
        CNecessary::Log(OBFUSCATE("RpMaterialDestroy: material is nullptr or invalid/read-only!"));
        return 1;
    }

    return original(material);
}

extern "C" __attribute__((visibility("default"))) RwBool RwTextureDestroy_hook(RwTexture* texture, RwBool (*original)(RwTexture*))
{
    if (CNecessary::IsBadWritePtr(texture, sizeof(*texture))) {
        CNecessary::Log(OBFUSCATE("RwTextureDestroy: texture is nullptr or invalid!"));
        return 1;
    }

    // เดียวมาต่อภายหลัง
    // if (CGui::GetLicensePlate() && CGui::GetLicensePlate()->m_pDefaultPlate && texture == CGui::GetLicensePlate()->m_pDefaultPlate) {
    //     return 1;
    // }

    // if (texture->raster && !CNecessary::IsBadReadPtr(texture->raster))
    // {
    //     if (CLicensePlate::IsActiveLicensePlateTexture(texture)) {
    //         return 1;
    //     }
    // }

    return original(texture);
}

extern "C" __attribute__((visibility("default"))) RwFrame* RwFrameAddChild_hook(RwFrame* parent, RwFrame* child, RwFrame* (*original)(RwFrame*, RwFrame*))
{
    if (child && parent)
    {
        return original(parent, child);
    }
    else
    {
        return nullptr;
    }
}

extern "C" __attribute__((visibility("default"))) bool CAnimBlendNode_FindKeyFrame_hook(uintptr_t thiz, float fCurrentTime, bool (*original)(uintptr_t, float))
{
    if (!thiz || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(thiz), 0x18))
    {
        CNecessary::Log(OBFUSCATE("FindKeyFrame: thiz is null"));
        return false;
    }

    uintptr_t pSequence = *reinterpret_cast<uintptr_t*>(thiz + 0x10);
    if (pSequence && CNecessary::IsValidUserPointer(reinterpret_cast<void*>(pSequence), 0x10))
    {
        return original(thiz, fCurrentTime);
    }

    return false;
}

extern "C" __attribute__((visibility("default"))) void CAnimManager_UncompressAnimation_hook(uintptr_t pAnim, void (*original)(uintptr_t))
{
    if (pAnim)
    {
        original(pAnim);
    }
    else
    {
        CNecessary::Log(OBFUSCATE("CAnimManager: pAnim is null"));
    }
}

extern "C" __attribute__((visibility("default"))) void CCustomRoadsignMgr_RenderRoadsignAtomic_hook(RpAtomic* pAtomic, const CVector* camPos, void (*original)(RpAtomic*, const CVector*))
{
    if (pAtomic)
    {
        original(pAtomic, camPos);
    }
    else
    {
        CNecessary::Log(OBFUSCATE("CCustomRoadsignMgr: pAtomic is null"));
    }
}

extern "C" __attribute__((visibility("default"))) RpMaterial* MaterialUpdateUVAnimCB_hook(RpMaterial* material, void* data, RpMaterial* (*original)(RpMaterial*, void*))
{
    if (!material)
    {
        CNecessary::Log(OBFUSCATE("MaterialUpdateUVAnimCB: thiz is null"));
        return nullptr;
    }

    return original(material, data);
}

extern "C" __attribute__((visibility("default"))) void CRadar_ClearBlip_hook(uint32_t index, void (*original)(uint32_t))
{
    if (static_cast<uint16_t>(index) > 249)
    {
        CNecessary::Log(OBFUSCATE("Blip Detected"));
    }
    else
    {
        original(index);
    }
}

extern "C" __attribute__((visibility("default"))) void CPed_PlayFootSteps_hook(uintptr_t thiz, void (*original)(uintptr_t))
{
    if (!thiz || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(thiz), 0x28)) {
        return;
    }

    uintptr_t clump = *reinterpret_cast<uintptr_t*>(thiz + 0x20);
    if (!clump || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(clump), sizeof(RpClump)))
    {
        CNecessary::Log(OBFUSCATE("CPed_PlayFootSteps: invalid clump pointer %p on ped %p"), reinterpret_cast<void*>(clump), reinterpret_cast<void*>(thiz));
        return;
    }

    // RpAnimBlendClumpGetFirstAssociation ของ GTA SA iOS 2.2.21.
    uintptr_t assoc = reinterpret_cast<uintptr_t (*)(uintptr_t)>(SA_ADDR(0x41AA0) + _dyld_get_image_vmaddr_slide(0))(clump);
    if (!assoc || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(assoc), 0x48))
    {
        CNecessary::Log(OBFUSCATE("CPed_PlayFootSteps: null/invalid anim association for clump %p on ped %p"), reinterpret_cast<void*>(clump), reinterpret_cast<void*>(thiz));
        return;
    }

    original(thiz);
}

extern "C" __attribute__((visibility("default"))) void* CAnimManager_GetAnimAssociation_hook(int groupId, int animId, void* (*original)(int, int))
{
    void* assoc = original(groupId, animId);

    if (assoc)
    {
        uint8_t* pAssoc = reinterpret_cast<uint8_t*>(assoc);
        uint8_t* m_pAnimBlendHierarchy = *reinterpret_cast<uint8_t**>(pAssoc + 0x18);
        if (m_pAnimBlendHierarchy)
        {
            uint8_t* m_pSequences = *reinterpret_cast<uint8_t**>(m_pAnimBlendHierarchy + 0x08);
            if (m_pSequences)
            {
                return assoc;
            }
        }
    }

    CNecessary::Log(OBFUSCATE("[CRASH DETECTED] groupId: %d, animId: %d"), groupId, animId);
    CExceptions::Print(OBFUSCATE("AnimBlend"), OBFUSCATE("AnimBlend"));

    // คง fallback แบบ zero-node ของ Thunder; initialise pointer ครั้งเดียว.
    alignas(uintptr_t) static uint8_t dummySequence[0x20]{};
    static CCrashFixAnimHierarchy dummyHierarchy{0, dummySequence, {}};
    static CCrashFixAnimAssociation dummyAssoc{{}, &dummyHierarchy};

    return &dummyAssoc;
}

extern "C" __attribute__((visibility("default"))) bool CTaskSimpleUseGun_ProcessPed_hook(uintptr_t thiz, uintptr_t ped, bool (*original)(uintptr_t, uintptr_t))
{
    if (!thiz || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(thiz), 0x48)) {
        return false;
    }

    if (!ped || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(ped))) {
        return false;
    }

    uintptr_t pAnim = *reinterpret_cast<uintptr_t*>(thiz + 0x40); // thiz->m_pAnim
    bool bInvalid = false;

    if (pAnim)
    {
        if (CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pAnim), 0x30))
        {
            bInvalid = true;
        }
        else
        {
            uintptr_t pHierarchy = *reinterpret_cast<uintptr_t*>(pAnim + 0x28); // pAnim->m_pAnimBlendHierarchy
            if (!pHierarchy || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pHierarchy), 0x1C))
            {
                bInvalid = true;
            }
        }
    }

    if (bInvalid)
    {
        CNecessary::Log(OBFUSCATE("CTaskSimpleUseGun::ProcessPed: Detected invalid dangling anim - resetting to nullptr"));
        *reinterpret_cast<uintptr_t*>(thiz + 0x40) = 0;
        *reinterpret_cast<bool*>(thiz + 0x10) = true; // thiz->m_bIsFinished = true
        return true;
    }

    return original(thiz, ped);
}

extern "C" __attribute__((visibility("default"))) bool CTaskSimpleGetUp_ProcessPed_hook(uintptr_t thiz, uintptr_t ped, bool (*original)(uintptr_t, uintptr_t))
{
    if (!thiz || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(thiz), 0x20)) {
        return false;
    }

    if (!ped || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(ped))) {
        return false;
    }

    uintptr_t pAnim = *reinterpret_cast<uintptr_t*>(thiz + 0x18); // thiz->m_pAnim
    bool bInvalid = false;

    if (pAnim)
    {
        if (CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pAnim), 0x30))
        {
            bInvalid = true;
        }
        else
        {
            uintptr_t pHierarchy = *reinterpret_cast<uintptr_t*>(pAnim + 0x28); // pAnim->m_pHierarchy
            if (!pHierarchy || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pHierarchy), 0x1C))
            {
                bInvalid = true;
            }
        }
    }

    if (bInvalid)
    {
        CNecessary::Log(OBFUSCATE("CTaskSimpleGetUp::ProcessPed: Detected invalid/dangling anim - resetting to nullptr"));
        *reinterpret_cast<uintptr_t*>(thiz + 0x18) = 0;
    }

    return original(thiz, ped);
}

extern "C" __attribute__((visibility("default"))) void CTaskSimpleUseGun_RemoveStanceAnims_hook(uintptr_t thiz, uintptr_t ped, float value, void (*original)(uintptr_t, uintptr_t, float))
{
    if (!thiz || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(thiz), 0x48)) {
        return;
    }

    if (!ped || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(ped))) {
        return;
    }

    uintptr_t pAnim = *reinterpret_cast<uintptr_t*>(thiz + 0x40); // thiz->m_pAnim
    bool bInvalid = false;

    if (pAnim)
    {
        if (CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pAnim), 0x30))
        {
            bInvalid = true;
        }
        else
        {
            uintptr_t pHierarchy = *reinterpret_cast<uintptr_t*>(pAnim + 0x28); // pAnim->m_pAnimBlendHierarchy
            if (!pHierarchy || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pHierarchy), 0x1C))
            {
                bInvalid = true;
            }
        }
    }

    if (bInvalid)
    {
        CNecessary::Log(OBFUSCATE("CTaskSimpleUseGun::RemoveStanceAnims: Invalid dangling anim - resetting to nullptr"));
        *reinterpret_cast<uintptr_t*>(thiz + 0x40) = 0;
        return;
    }

    original(thiz, ped, value);
}

extern "C" __attribute__((visibility("default"))) void CObject_ProcessGarageDoorBehaviour_hook(uintptr_t thiz, void (*original)(uintptr_t))
{
    if (!thiz || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(thiz), 0x1F0)) {
        return;
    }

    uintptr_t pDummyObject = *reinterpret_cast<uintptr_t*>(thiz + 0x1E8);
    if (!pDummyObject || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(pDummyObject), 0x20)) {
        return;
    }

    original(thiz);
}

extern "C" __attribute__((visibility("default"))) uint64_t emu_ArraysGetID_hook(uintptr_t vertexRef, uint64_t (*original)(uintptr_t))
{
    if (!vertexRef || CNecessary::IsBadReadPtr(reinterpret_cast<void*>(vertexRef), 0x38))
    {
        CNecessary::Log(OBFUSCATE("emu_ArraysGetID: invalid vertexRef"));
        return 0;
    }

    uint64_t id = *reinterpret_cast<uint64_t*>(vertexRef + 0x30);
    if (!id)
    {
        CNecessary::Log(OBFUSCATE("emu_ArraysGetID: invalid id"));
        return 0;
    }

    return original(vertexRef);
}

void CCrashFix::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCrashFix Initialise"));

    // CPlaceable::InitMatrixArray ของ iOS ถูก inline; ขยาย 900 -> 10000 ใน Mach-O ก่อนเริ่มเกม.
    // คง allocator, array cookie และการเชื่อม CMatrixLinkList ของเอนจินเดิมทั้งหมด.
}