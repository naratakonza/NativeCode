#include "../../../../Main.h"
#include "CNetGamePool.h"

CNetGamePool::CNetGamePool()
{
    CNecessary::Log(OBFUSCATE("CNetGamePool Initialise"));

    CNetGamePool::m_pPlayerPool = new CPlayerPool();
    // CNetGamePool::m_pVehiclePool = new CVehiclePool();
    CNetGamePool::m_pTextDrawPool = new CTextDrawPool();
    CNetGamePool::m_pActorPool = new CActorPool();
    CNetGamePool::m_pGangZonePool = new CGangZonePool();
    // CNetGamePool::m_pTextLabelPool = new CTextLabelPool();
    CNetGamePool::m_pPickupPool = new CPickupPool();
    // CNetGamePool::m_pObjectPool = new CObjectPool();
    // CNetGamePool::m_pMenuPool = new CMenuPool();

    // if (CGui::GetChat()) 
    // {      
    //     CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CNetGamePool Initialise"));
    // }
}

CNetGamePool::~CNetGamePool()
{
    CNecessary::Log(OBFUSCATE("CNetGamePool Uninitialise"));

    SAFE_DELETE(CNetGamePool::m_pPlayerPool);
    // SAFE_DELETE(CNetGamePool::m_pVehiclePool);
    SAFE_DELETE(CNetGamePool::m_pTextDrawPool);
    SAFE_DELETE(CNetGamePool::m_pActorPool);
    SAFE_DELETE(CNetGamePool::m_pGangZonePool);
    // SAFE_DELETE(CNetGamePool::m_pTextLabelPool);
    SAFE_DELETE(CNetGamePool::m_pPickupPool);
    // SAFE_DELETE(CNetGamePool::m_pObjectPool);
    // SAFE_DELETE(CNetGamePool::m_pMenuPool);
}

void CNetGamePool::Process()
{
	if (CNetGamePool::GetPlayerPool())
	{
		CNetGamePool::GetPlayerPool()->Process();
	}

    try
    {
		if (CNetGamePool::GetPickupPool() && CNetGamePool::iPickupPoolProcessFlag > 5)
		{
			CNetGamePool::GetPickupPool()->Process();
			CNetGamePool::iPickupPoolProcessFlag = 0;
		}
		else
		{
			++CNetGamePool::iPickupPoolProcessFlag;
		}
    }
    catch (const std::exception& error)
    {
        CExceptions::Print(OBFUSCATE("s005"), error.what());
    }
    catch (...)
    {
        CExceptions::Print(OBFUSCATE("s005"), OBFUSCATE("unknown C++ exception"));
    }
}

void CNetGamePool::DeleteAll()
{
    // // Vehicle
    // if (CNetGamePool::GetVehiclePool())
    // {
    //     SAFE_DELETE(CNetGamePool::m_pVehiclePool);
    // }

    // CNetGamePool::m_pVehiclePool = new CVehiclePool();

    // // Object
    // if (CNetGamePool::GetObjectPool())
    // {
    //     SAFE_DELETE(CNetGamePool::m_pObjectPool);
    // }

    // CNetGamePool::m_pObjectPool = new CObjectPool();

    // Pickup
    if (CNetGamePool::GetPickupPool())
    {
        SAFE_DELETE(CNetGamePool::m_pPickupPool);
    }

    CNetGamePool::m_pPickupPool = new CPickupPool();

    // GangZone
    if (CNetGamePool::GetGangZonePool())
    {
        SAFE_DELETE(CNetGamePool::m_pGangZonePool);
    }

    CNetGamePool::m_pGangZonePool = new CGangZonePool();

    // Actor
    if (CNetGamePool::GetActorPool())
    {
        SAFE_DELETE(CNetGamePool::m_pActorPool);
    }

    CNetGamePool::m_pActorPool = new CActorPool();

    // // TextLabel
    // if (CNetGamePool::GetTextLabelPool())
    // {
    //     SAFE_DELETE(CNetGamePool::m_pTextLabelPool);
    // }

    // CNetGamePool::m_pTextLabelPool = new CTextLabelPool();

    // TextDraw
    if (CNetGamePool::GetTextDrawPool())
    {
        SAFE_DELETE(CNetGamePool::m_pTextDrawPool);
    }

    CNetGamePool::m_pTextDrawPool = new CTextDrawPool();

    // // Menu
    // if (CNetGamePool::GetMenuPool())
    // {
    //     SAFE_DELETE(CNetGamePool::m_pMenuPool);
    // }

    // CNetGamePool::m_pMenuPool = new CMenuPool();
}