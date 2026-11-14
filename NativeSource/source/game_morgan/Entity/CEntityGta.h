#pragma once
#include "CPlaceableGta.h"
#include "../Enums/eEntityType.h"

struct CEntityInfo
{
    uint8_t m_type : 3;
    uint8_t m_status : 5;
};

struct CEntityGta : CPlaceableGta
{
    union {
        struct RwObject* m_pRwObject;
        struct RpClump*  m_pRwClump;
        struct RpAtomic* m_pRwAtomic;
        uintptr_t pdwRenderWare; // ;rwObject
    };

    union {
        uint32_t m_nEntityFlags;
        struct {
            uint32_t m_bUsesCollision : 1;
            uint32_t m_bCollisionProcessed : 1;
            uint32_t m_bIsStatic : 1;
            uint32_t m_bHasContacted : 1;
            uint32_t m_bIsStuck : 1;
            uint32_t m_bIsInSafePosition : 1;
            uint32_t m_bWasPostponed : 1;
            uint32_t m_bIsVisible : 1;

            uint32_t m_bIsBIGBuilding : 1;
            uint32_t m_bRenderDamaged : 1;
            uint32_t m_bStreamingDontDelete : 1;
            uint32_t m_bRemoveFromWorld : 1;
            uint32_t m_bHasHitWall : 1;
            uint32_t m_bImBeingRendered : 1;
            uint32_t m_bDrawLast :1;
            uint32_t m_bDistanceFade : 1;

            uint32_t m_bDontCastShadowsOn : 1;
            uint32_t m_bOffscreen : 1;
            uint32_t m_bIsStaticWaitingForCollision : 1;
            uint32_t m_bDontStream : 1;
            uint32_t m_bUnderwater : 1;
            uint32_t m_bHasPreRenderEffects : 1;
            uint32_t m_bIsTempBuilding : 1;
            uint32_t m_bDontUpdateHierarchy : 1;

            uint32_t m_bHasRoadsignText : 1;
            uint32_t m_bDisplayedSuperLowLOD : 1;
            uint32_t m_bIsProcObject : 1;
            uint32_t m_bBackfaceCulled : 1;
            uint32_t m_bLightObject : 1;
            uint32_t m_bUnimportantStream : 1;
            uint32_t m_bTunnel : 1;
            uint32_t m_bTunnelTransition : 1;
        } nEntityFlags;
    };

    uint32_t dummy;
    union {
        struct {
            uint16_t m_nRandomSeedUpperByte : 8;
            uint16_t m_nRandomSeedSecondByte : 8;
        };
        uint16_t m_nRandomSeed;
    };

    uint16_t nModelIndex;
    uint8_t pad[4];
    uintptr_t pReferences;
    uintptr_t m_pLastRenderedLink;
    uint16_t m_nScanCode;
    uint8_t m_nIplIndex;
    uint8_t m_areaCode;
    uint8_t pad2[4];
    union {
        int32_t m_nLodIndex; // -1 - without LOD model
        uintptr_t m_pLod;
    };
    int8_t m_nNumLodChildren;
    uint8_t m_nNumLodChildrenRendered;
    union {
        struct
        {
            eEntityType m_nType     : 3;
            uint8_t     m_nStatus   : 5;
        };
        uint8_t nControlFlags;
    };
    uint8_t pad3[1];
};
static_assert(sizeof(CEntityGta) == 0x60);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(CEntityGta, m_pRwObject) == 0x20);
static_assert(offsetof(CEntityGta, nModelIndex) == 0x32);
static_assert(offsetof(CEntityGta, pReferences) == 0x38);
static_assert(offsetof(CEntityGta, nControlFlags) == 0x5A);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif