#pragma once

#include "CRealTimeShadow.h"

class CRealTimeShadowManager
{
public:
	static constexpr int32_t NUM_REALTIME_SHADOWS = 40;
	static int m_nShadowStyle;

public:
	bool m_bInitialised{};
    uint8_t pad01[7];
	CRealTimeShadow* m_pRealTimeShadows[NUM_REALTIME_SHADOWS]{};
	bool m_updatingShadows{};
    uint8_t pad02[7];
	CShadowCamera m_IPCameraMB{};
	CShadowCamera m_IPCameraGradient{};

public:
	static void Initialise();
	static CRealTimeShadow* GetRealTimeShadow(CRealTimeShadowManager* thiz, CPhysicalGta* pPhysical);
	static void DoShadowThisFrame(CRealTimeShadowManager* thiz, CPhysicalGta* pPhysical);
	static void ReturnRealTimeShadow(CRealTimeShadowManager* thiz, CRealTimeShadow* pShadow);
	static void SetShadowStyle(int style);
	static int GetShadowStyle();
};
static_assert(sizeof(CRealTimeShadowManager) == 0x170);
static_assert(offsetof(CRealTimeShadowManager, m_pRealTimeShadows) == 0x08);
static_assert(offsetof(CRealTimeShadowManager, m_updatingShadows) == 0x148);
static_assert(offsetof(CRealTimeShadowManager, m_IPCameraMB) == 0x150);