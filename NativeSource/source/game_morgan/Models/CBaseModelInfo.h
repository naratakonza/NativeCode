#pragma once

#include <cstddef>
#include <cstdint>

class CPedModelInfo;
struct RwObject;

struct CBaseModelInfo
{
    std::uintptr_t vtable;
    std::uint32_t m_hashKey;
    char m_modelName[21];
    std::byte reserved[0x22 - 0x21];
    std::uint16_t m_numRefs;
    std::byte reservedAfterRefs[0x30 - 0x24];
    void* m_pColModel;
    float m_fDrawDistance;
    std::byte reservedAfterDrawDistance[4];
    RwObject* m_pRwObject;

    CPedModelInfo* AsPedModelInfoPtr() { return reinterpret_cast<CPedModelInfo*>(this); }
};

static_assert(sizeof(CBaseModelInfo) == 0x48);
static_assert(offsetof(CBaseModelInfo, m_hashKey) == 0x08);
static_assert(offsetof(CBaseModelInfo, m_modelName) == 0x0C);
static_assert(offsetof(CBaseModelInfo, m_numRefs) == 0x22);
static_assert(offsetof(CBaseModelInfo, m_pColModel) == 0x30);
static_assert(offsetof(CBaseModelInfo, m_fDrawDistance) == 0x38);
static_assert(offsetof(CBaseModelInfo, m_pRwObject) == 0x40);