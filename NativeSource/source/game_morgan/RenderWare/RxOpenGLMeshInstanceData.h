#pragma once

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)
struct RxOpenGLMeshInstanceData
{
    uint8_t _pad0[0x44];
    uintptr_t validate;
    uint8_t _pad1[0x0C];
};
#pragma pack(pop)
static_assert(sizeof(RxOpenGLMeshInstanceData) == 0x58);
static_assert(offsetof(RxOpenGLMeshInstanceData, validate) == 0x44);
