#include "../../Main.h"
#include "CPickup.h"
#include "CCommon.h"
#include "CStreaming.h"
#include "Scripts/CScripting.h"

CPickup::CPickup(int iModel, int iType, float x, float y, float z, int* pickId, bool isWeaponPickup)
{
	CNecessary::Log(OBFUSCATE("CPickup CPickup"));

	CPickup::m_dwHandle = 0;
	CPickup::m_iPickupId = 0;
	CPickup::m_iType = iType;
	CPickup::m_vecPos.x = x;
	CPickup::m_vecPos.y = y;
	CPickup::m_vecPos.z = z;
	
	int waitCounter = 0;
	if (!CStreaming::IsModelLoaded(iModel))
	{
		CStreaming::RequestModel(iModel);
		CStreaming::LoadAllRequestedModels(false);

		while (!CStreaming::IsModelLoaded(iModel)) 
		{
			usleep(1000);
			waitCounter++;
			if (waitCounter > 250)
			{
			    break;
			}
		}
	}

	uint32_t hnd = 0;
	if (isWeaponPickup)
    {
		CScripting::ScriptCommand(&create_pickup_with_ammo, iModel, iType, 30, x, y, z, &hnd);
	}
	else
    {
		CScripting::ScriptCommand(&create_pickup, iModel, iType, x, y, z, &hnd);
	}

	CPickup::m_dwHandle = hnd;

	int lol = 32 * static_cast<uint16_t>(hnd);
	if (lol) 
    {
        lol /= 32;
    }

	if (pickId) 
    {
        *pickId = lol;
    }
}

CPickup::~CPickup()
{
	if (CPickup::m_dwHandle > 0)
    {
		CScripting::ScriptCommand(&destroy_pickup, CPickup::m_dwHandle);
	}

	CPickup::m_dwHandle = 0;
	CPickup::m_iPickupId = 0;
}

bool CPickup::IsPickedUp()
{
	if (CPickup::m_dwHandle <= 0) {
		return false;
	}

	return CScripting::ScriptCommand(&is_pickup_picked_up, CPickup::m_dwHandle);
}

void CPickup::SetID(int id)
{
	CPickup::m_iPickupId = id;
}