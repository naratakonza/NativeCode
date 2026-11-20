#include "../../../Main.h"
#include "CAnimBlendClumpData.h"

AnimBlendFrameData* CAnimBlendClumpData::GetFrameDataByNodeId(uint32_t nodeId)
{
    for (uint32_t i = 0; i < m_numFrames; i++)
    {
        AnimBlendFrameData* frameData = &m_pFrameDataArray[i];
        if (frameData->m_nNodeId == nodeId)
        {
            return frameData;
        }
    }

    return nullptr;
}