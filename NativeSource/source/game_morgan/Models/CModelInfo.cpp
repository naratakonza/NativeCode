#include "../../../Main.h"
#include "CModelInfo.h"

#include <mach-o/dyld.h>

bool CModelInfo::ms_modelPatchesVerified = false;
CBaseModelInfo** CModelInfo::ms_modelInfoPtrs = nullptr;

void CModelInfo::InstallModelPatches()
{
    ms_modelPatchesVerified = CMemory::HasMappedInstruction(SA_ADDR(0x22D3E8), "\xE8\x37\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x22DC48), "\xE8\x42\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x22DB98), "\x48\x43\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0x22EA18), "\x16\x20\x9F\x52") && CMemory::HasMappedInstruction(SA_ADDR(0x22EA1C), "\xB6\x02\xA0\x72") && CMemory::HasMappedInstruction(SA_ADDR(0x22EB4C), "\x17\xB1\x92\x52") && CMemory::HasMappedInstruction(SA_ADDR(0x22E998), "\x09\x71\x83\x52") && CMemory::HasMappedInstruction(SA_ADDR(0x22D98C), "\xD5\xA5\x09\x14");

    if (!ms_modelPatchesVerified)
    {
        CNecessary::Log(OBFUSCATE("CModelInfo: iOS model pool patch unavailable"));
    }
}

void CModelInfo::Initialise()
{
    if (ms_modelPatchesVerified && CMemory::HasMappedInstruction(SA_ADDR(0x22D9B4), "\x69\x2C\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x22D9B8), "\x29\xC1\x05\x91"))
    {
        ms_modelInfoPtrs = reinterpret_cast<CBaseModelInfo**>(SA_ADDR(0x7BC170) + _dyld_get_image_vmaddr_slide(0));
        CNecessary::Log(OBFUSCATE("CModelInfo Initialise"));
    }
}

CBaseModelInfo* CModelInfo::GetModelInfo(int index)
{
    return ms_modelInfoPtrs && index >= 0 && index < NUM_MODEL_INFOS ? ms_modelInfoPtrs[index] : nullptr;
}

CBaseModelInfo* CModelInfo::GetModelInfo(const char* name, int start, int end)
{
    if (!ms_modelInfoPtrs || !name || start < 0 || end >= NUM_MODEL_INFOS || start > end) {
        return nullptr;
    }

    return reinterpret_cast<CBaseModelInfo* (*)(const char*, int, int)>(SA_ADDR(0x22D98C) + _dyld_get_image_vmaddr_slide(0))(name, start, end);
}

void CModelInfo::SetModelInfo(int index, CBaseModelInfo* info)
{
    if (ms_modelInfoPtrs && index >= 0 && index < NUM_MODEL_INFOS)
    {
        ms_modelInfoPtrs[index] = info;
    }
}

template <typename ModelInfo>
static ModelInfo* AddModel(int index, std::uint64_t function, std::uint64_t store, std::uint32_t capacity)
{
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    if (*reinterpret_cast<const std::uint32_t*>(store + slide) >= capacity)
    {
        return nullptr;
    }

    return reinterpret_cast<ModelInfo* (*)(int)>(function + slide)(index);
}

CAtomicModelInfo* CModelInfo::AddAtomicModel(int index)
{
    return ms_modelInfoPtrs && index >= 0 && index < NUM_MODEL_INFOS ? AddModel<CAtomicModelInfo>(index, SA_ADDR(0x22D3D8), SA_ADDR(0x92C608), NUM_ATOMIC_MODEL_INFOS) : nullptr;
}

CPedModelInfo* CModelInfo::AddPedModel(int index)
{
    return ms_modelInfoPtrs && index >= 0 && index < NUM_MODEL_INFOS ? AddModel<CPedModelInfo>(index, SA_ADDR(0x22DC38), SA_ADDR(0xA8CA70), NUM_PED_MODEL_INFOS) : nullptr;
}

CClumpModelInfo* CModelInfo::AddClumpModel(int index)
{
    return ms_modelInfoPtrs && index >= 0 && index < NUM_MODEL_INFOS ? AddModel<CClumpModelInfo>(index, SA_ADDR(0x22DB88), SA_ADDR(0xA973E0), NUM_CLUMP_MODEL_INFOS) : nullptr;
}