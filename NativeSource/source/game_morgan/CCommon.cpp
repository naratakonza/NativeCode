#include "CCommon.h"
#include "CSprite2d.h"
#include "RenderWare/RenderWare.h"
#include "Models/CModelInfo.h"
#include "Enums/eMaxItem.h"
#include "../utils/Memory/CMemory.h"

#include <mach-o/dyld.h>

std::unordered_map<int, bool> CCommon::bTextDrawTextureSlotState;
std::unordered_map<int, RwTexture*> CCommon::pTextDrawTexture;

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

bool CCommon::IsExistForModelInfo(int iModelID)
{
	if (iModelID < 0 || iModelID >= eMaxItem::MAX_MODELINFO) {
		return false;
	}

	uintptr_t modelInfo = CCommon::GetModelInfoByID(iModelID);
	return (modelInfo && *(uintptr_t*)modelInfo != (uintptr_t)0xFFFFFFEC);
}

bool CCommon::ModelInfoGetCol(int iModel, [[maybe_unused]] const char* opcode)
{
	uintptr_t modelInfo = CCommon::GetModelInfoByID(iModel);
    if (!modelInfo) {
        return false;
    }

    uintptr_t colModel = *reinterpret_cast<uintptr_t*>(modelInfo + 0x30);
    if (!colModel)
	{
        return false;
    }

	return true;
}

bool CCommon::IsPointInRect(float x, float y, CRect* rect)
{
    return (x >= rect->X1 && x <= rect->X2 && y >= rect->Y1 && y <= rect->Y2);
}

void CCommon::RwMatrixRotate(CMatrix* mat, int axis, float angle)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2B8668), "\xFF\x43\x01\xD1\xE9\x23\x01\x6D", 8, "__text");
    if (!verified || !mat || axis < 0 || axis > 2) {
        return;
    }

    static float matt[3][3] =
    {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    reinterpret_cast<CMatrix* (*)(CMatrix*, float*, float, int)>(SA_ADDR(0x2B8668) + _dyld_get_image_vmaddr_slide(0))(mat, matt[axis], angle, 1);
}

bool CCommon::rpWorldAddCamera(RwCamera* camera)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x1527EC), "\x34\x3E\x00\xD0\x94\xE2\x2A\x91\x80\x06\x00\xF9", 12, "__text");
    if (!verified) {
        return false;
    }

    RpWorld* rpWorld = *reinterpret_cast<RpWorld**>(SA_ADDR(0x918AB8) + _dyld_get_image_vmaddr_slide(0));
    if (rpWorld && camera && RpWorldAddCamera)
    {
        return RpWorldAddCamera(rpWorld, camera) != nullptr;
    }

    return false;
}

void CCommon::rpWorldRemoveCamera(RwCamera* camera)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x1527EC), "\x34\x3E\x00\xD0\x94\xE2\x2A\x91\x80\x06\x00\xF9", 12, "__text");
    if (!verified) {
        return;
    }

    RpWorld* rpWorld = *reinterpret_cast<RpWorld**>(SA_ADDR(0x918AB8) + _dyld_get_image_vmaddr_slide(0));
    if (rpWorld && camera && RpWorldRemoveCamera)
    {
        RpWorldRemoveCamera(rpWorld, camera);
    }
}

void CCommon::SetRenderWareCamera(RwCamera* camera)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x31865C), "\xA8\x2E\x00\xF0\x00\x79\x03\xF9\x08\x04\x40\xF9", 12, "__text");
    if (!verified) {
        return;
    }

    reinterpret_cast<void (*)(RwCamera*)>(SA_ADDR(0x31865C) + _dyld_get_image_vmaddr_slide(0))(camera);
}

RwCamera* CCommon::GetRenderWareCamera()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x31865C), "\xA8\x2E\x00\xF0\x00\x79\x03\xF9\x08\x04\x40\xF9", 12, "__text");
    if (!verified) {
        return nullptr;
    }

    return *reinterpret_cast<RwCamera**>(SA_ADDR(0x8EF6F0) + _dyld_get_image_vmaddr_slide(0));
}

bool CCommon::rpWorldAddLight(RpLight* light)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x1527EC), "\x34\x3E\x00\xD0\x94\xE2\x2A\x91\x80\x06\x00\xF9", 12, "__text");
    if (!verified) {
        return false;
    }

    RpWorld* rpWorld = *reinterpret_cast<RpWorld**>(SA_ADDR(0x918AB8) + _dyld_get_image_vmaddr_slide(0));
    if (rpWorld && light && RpWorldAddLight)
    {
        return RpWorldAddLight(rpWorld, light) != nullptr;
    }

    return false;
}

void CCommon::rpWorldRemoveLight(RpLight* light)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x1527EC), "\x34\x3E\x00\xD0\x94\xE2\x2A\x91\x80\x06\x00\xF9", 12, "__text");
    if (!verified) {
        return;
    }

    RpWorld* rpWorld = *reinterpret_cast<RpWorld**>(SA_ADDR(0x918AB8) + _dyld_get_image_vmaddr_slide(0));
    if (rpWorld && light && RpWorldRemoveLight)
    {
        RpWorldRemoveLight(rpWorld, light);
    }
}

void CCommon::DefinedState()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x31917C), "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91\x40\x00\x80\x52", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)()>(SA_ADDR(0x31917C) + _dyld_get_image_vmaddr_slide(0))();
    }
}

uint16_t CCommon::ModelInfoGetReferenceCount(int iModelID)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x22C5C8), "\x08\x44\x40\x79\x08\x05\x00\x11\x08\x44\x00\x79", 12, "__text");
    const auto* modelInfo = reinterpret_cast<const CBaseModelInfo*>(CCommon::GetModelInfoByID(iModelID));
    return verified && modelInfo ? modelInfo->m_numRefs : 0;
}

int CCommon::GetFreeTextDrawTextureSlot()
{
    for (int slot = 0; slot < 200; ++slot)
    {
        auto stateIter = CCommon::bTextDrawTextureSlotState.find(slot);
        if (stateIter == CCommon::bTextDrawTextureSlotState.end())
        {
            CCommon::bTextDrawTextureSlotState[slot] = true;
            return slot;
        }
        else if (!stateIter->second)
        {
            CCommon::bTextDrawTextureSlotState[slot] = true;
            return slot;
        }
    }

    return -1;
}

void CCommon::DestroyTextDrawTexture(int index)
{
    auto textureIter = CCommon::pTextDrawTexture.find(index);
    auto stateIter = CCommon::bTextDrawTextureSlotState.find(index);

    if (textureIter != CCommon::pTextDrawTexture.end())
    {
        if (textureIter->second)
        {
            CSprite2d::Destructor(textureIter->second);
        }

        CCommon::pTextDrawTexture.erase(textureIter);
    }

    if (stateIter != CCommon::bTextDrawTextureSlotState.end())
    {
        stateIter->second = false;
    }
}

void CCommon::DrawTextureUV(RwTexture *texture, CRect *rect, uint32_t dwColor, float *uv)
{
    if (texture)
    {
        if (RwRenderStateSet)
        {
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, reinterpret_cast<void*>(rwFILTERLINEAR));
        }
        
        CSprite2d::Draw(texture, rect, dwColor, uv);
    }
}

bool CCommon::IsTaskRunNamedOrSlideToCoord(void* pTask)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x341860), "\x49\x0B\x00\x90\x29\x41\x25\x91", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x3A86F8), "\x48\x08\x00\x90\x08\x41\x22\x91", 8, "__text");
    if (!verified) {
        return false;
    }

    uintptr_t dwVTable = *reinterpret_cast<uintptr_t*>(pTask);
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    if (dwVTable == SA_ADDR(0x4B0890) + slide || dwVTable == SA_ADDR(0x4A9950) + slide) // CTaskSimpleSlideToCoord CTaskSimpleRunNamedAnim
    {
        return true;
    }

    return false;
}

int CCommon::GetTaskTypeFromTask(void* task)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3764FC), "\x00\x58\x80\x52\xC0\x03\x5F\xD6", 8, "__text");
    if (!task || !verified) {
        return 0;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    uintptr_t pVTableTask = *reinterpret_cast<uintptr_t*>(task);
    if (pVTableTask < SA_ADDR(0x4987B0) + slide || pVTableTask > SA_ADDR(0x4B48F0) + slide) {
        return 0;
    }

    return reinterpret_cast<int (*)(void*)>(*reinterpret_cast<uintptr_t*>(pVTableTask + 0x28))(task);
}

void* CCommon::GetNextTaskFromTask(void* task)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0xB7420), "\x00\x00\x80\xD2\xC0\x03\x5F\xD6", 8, "__text");
    if (!task || !verified) {
        return nullptr;
    }

    uintptr_t pTask = reinterpret_cast<uintptr_t>(task);
    uintptr_t pVTableTask = *reinterpret_cast<uintptr_t*>(pTask);
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    if (pVTableTask < SA_ADDR(0x4987B0) + slide || pVTableTask > SA_ADDR(0x4B48F0) + slide) {
        return nullptr;
    }

    return reinterpret_cast<void* (*)(void*)>(*reinterpret_cast<uintptr_t*>(pVTableTask + 0x18))(task); // ดึง offset vtable จาก _ZNK11CTaskSimple10GetSubTaskEv ; CTaskSimple::GetSubTask(void) ของ CTaskSimpleRunNamedAnim
}