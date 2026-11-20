#pragma once

#include "AnimBlendFrameData.h"
#include "../Core/CVector.h"

struct ListLink
{
    void* next;
    void* prev;
};

class CAnimBlendClumpData
{
public:
    ListLink m_list;
    uint32_t m_numFrames;
    CVector* m_pExtractedVelocity;
    AnimBlendFrameData* m_pFrameDataArray;

    AnimBlendFrameData* GetFrameDataByNodeId(uint32_t nodeId);
};
static_assert(sizeof(CAnimBlendClumpData) == 0x28);
static_assert(offsetof(CAnimBlendClumpData, m_numFrames) == 0x10);
static_assert(offsetof(CAnimBlendClumpData, m_pFrameDataArray) == 0x20);