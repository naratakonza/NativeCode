#pragma once

#include "CShadowCamera.h"
#include "../../Models/Col/CSphere.h"
#include <cstddef>

struct CPhysicalGta;
struct RpLight;

class CRealTimeShadow
{
public:
    CPhysicalGta* m_pPhysical{};
    bool m_usedThisFrame{};
    uint8_t m_nIntensity{};
    uint8_t pad01[2];
    CShadowCamera* m_camera{};
    bool m_bBlurred{};
    uint8_t pad02[3];
    uint32_t m_nBlurPasses{};
    bool m_bDrawMoreBlur;
    uint8_t pad03[3];
    uint32_t m_nRwObjectType{(uint32_t) -1};
    RpLight* m_pLight{};
	CSphere m_boundingSphere{};
	CSphere m_baseSphere{};

public:
    static bool SetShadowedObject(CRealTimeShadow* thiz, CPhysicalGta* pPhysical);
};
static_assert(sizeof(CRealTimeShadow) == 0x50);
static_assert(offsetof(CRealTimeShadow, m_usedThisFrame) == 0x08);
static_assert(offsetof(CRealTimeShadow, m_camera) == 0x10);