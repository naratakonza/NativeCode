#include "../../../../Main.h"
#include "CRealTimeShadowManager.h"
#include "../../Entity/Ped/CPedGta.h"
#include "../../Entity/CPhysicalGta.h"
#include "../../RenderWare/rpworld.h"

int CRealTimeShadowManager::m_nShadowStyle = 0;
static bool shadowHooksInstalled = false;

void CRealTimeShadowManager::Initialise()
{
	CNecessary::Log(OBFUSCATE("CRealTimeShadowManager Initialise"));
    
    shadowHooksInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x2DBE70), "\x7A\xEE\x06\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x2DBE58), "\x86\xEE\x06\x14");
}

extern "C" __attribute__((visibility("default"))) void CRealTimeShadowManager_DoShadowThisFrame_hook(CRealTimeShadowManager* thiz, CPhysicalGta* pPhysical, void (*DoShadowThisFrame)(CRealTimeShadowManager*, CPhysicalGta*))
{
    if (!shadowHooksInstalled)
    {
        DoShadowThisFrame(thiz, pPhysical);
        return;
    }

    CRealTimeShadowManager::DoShadowThisFrame(thiz, pPhysical);
}

extern "C" __attribute__((visibility("default"))) void CRealTimeShadowManager_ReturnRealTimeShadow_hook(CRealTimeShadowManager* thiz, CRealTimeShadow* pShadow, void (*ReturnRealTimeShadow)(CRealTimeShadowManager*, CRealTimeShadow*))
{
    if (!shadowHooksInstalled)
    {
        ReturnRealTimeShadow(thiz, pShadow);
        return;
    }

    CRealTimeShadowManager::ReturnRealTimeShadow(thiz, pShadow);
}

void CRealTimeShadowManager::ReturnRealTimeShadow(CRealTimeShadowManager* thiz, CRealTimeShadow* pShadow)
{
	if (!pShadow) {
		return;
	}

	if (pShadow->m_pPhysical)
	{
		if (pShadow->m_pPhysical->m_pRealTimeShadow == pShadow)
		{
			pShadow->m_pPhysical->m_pRealTimeShadow = nullptr;
		}
		pShadow->m_pPhysical = nullptr;
	}
}

CRealTimeShadow* CRealTimeShadowManager::GetRealTimeShadow(CRealTimeShadowManager* thiz, CPhysicalGta* pPhysical)
{
	if (!thiz || !thiz->m_bInitialised || !pPhysical || !pPhysical->m_pRwObject) {
		return nullptr;
	}

    const auto type = pPhysical->m_pRwObject->type;
    if (type != rpATOMIC && type != rpCLUMP) {
        return nullptr;
    }

	for (auto pShadow : thiz->m_pRealTimeShadows)
    {
		if (pShadow && !pShadow->m_pPhysical && pShadow->m_camera && pShadow->m_camera->m_pShadowCamera)
        {
            if (!CRealTimeShadow::SetShadowedObject(pShadow, pPhysical))
            {
                pShadow->m_pPhysical = nullptr;
                return nullptr;
            }

			pPhysical->m_pRealTimeShadow = pShadow;
			pShadow->m_usedThisFrame = true;
			return pShadow;
		}
	}

	return nullptr;
}

void CRealTimeShadowManager::DoShadowThisFrame(CRealTimeShadowManager* thiz, CPhysicalGta* pPhysical)
{
	if (!thiz || !thiz->m_bInitialised || !pPhysical) {
		return;
	}

	auto shadows = CRealTimeShadowManager::GetShadowStyle();
    if (shadows == 0)
    {
        shadows = 0;
    }
    else if (shadows == 1)
    {
        shadows = 2;
    }

	switch (shadows)
    {
		case 2: {
			break;
        }

		case 1: {
			if (pPhysical->m_nType == 3)
            {
				CPedGta* pPed = reinterpret_cast<CPedGta*>(pPhysical);
				if (pPed->dwPedType == 0)
                {
					break;
				}
			}

			return;
		}

		default: {
			return;
        }
	}

	if (const auto pShadow = pPhysical->m_pRealTimeShadow)
    {
		pShadow->m_usedThisFrame = true;
	}
	else
    {
		(void)CRealTimeShadowManager::GetRealTimeShadow(thiz, pPhysical);
	}
}

void CRealTimeShadowManager::SetShadowStyle(int style)
{
	CRealTimeShadowManager::m_nShadowStyle = style;
}

int CRealTimeShadowManager::GetShadowStyle()
{
	return CRealTimeShadowManager::m_nShadowStyle;
}