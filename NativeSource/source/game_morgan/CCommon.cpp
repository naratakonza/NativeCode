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