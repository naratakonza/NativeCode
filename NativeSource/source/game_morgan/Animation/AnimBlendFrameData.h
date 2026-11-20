#pragma once

#include <cstddef>
#include "../RenderWare/rwcore.h"

#include "../Core/CVector.h"
#include "../../utils/Quat/CQuaternion.h"
#include "../RenderWare/RpHAnimBlendInterpFrame.h"

class AnimBlendFrameData
{
public:
    union {
        struct {
            uint8_t m_bf1 : 1;                                    // doesn't seem to be used
            uint8_t m_IsIFrameOrientationToAffectedByNodes : 1;   // m_IFrame orientation will be affected
            uint8_t m_IsIFrameTranslationToAffectedByNodes : 1;   // m_IFrame translation will be affected
            uint8_t m_bIsInitialized : 1;
            uint8_t m_bUpdateSkinnedWith3dVelocityExtraction : 1;
            uint8_t m_bCheckBlendNodeClumpKeyFrames : 1;          // key frames of CAnimBlendNode bones will be checked
            uint8_t m_bIsCompressed : 1;
            uint8_t m_bUpdatingFrame : 1;                         // doesn't seem to be used
        };
        uint8_t m_nFlags;
    };
    uint8_t pad0[3];

    CVector                  m_vecOffset;
    union {
        RpHAnimBlendInterpFrame* m_pIFrame;
        RwFrame*                 m_pFrame;
    };
    uint32_t                   m_nNodeId; // In case of peds it's ePedBone (NOTE: I might be wrong, see `IsPedHeadAbovePos`)

    // NOTSA
    CQuaternion& GetFrameOrientation() const { return m_pIFrame->orientation; }
    CVector& GetFrameTranslation() const { return m_pIFrame->translation; }
};
static_assert(sizeof(AnimBlendFrameData) == 0x20);
static_assert(offsetof(AnimBlendFrameData, m_pIFrame) == 0x10);
static_assert(offsetof(AnimBlendFrameData, m_nNodeId) == 0x18);