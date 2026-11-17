#pragma once

#include "Process/CPlayerPool.h"
// #include "Process/CVehiclePool.h"
// #include "Process/CTextDrawPool.h"
#include "Process/CActorPool.h"
// #include "Process/CGangZonePool.h"
// #include "Process/CTextLabelPool.h"
// #include "Process/CPickupPool.h"
// #include "Process/CObjectPool.h"
// #include "Process/CMenuPool.h"

class CNetGamePool
{
	CPlayerPool* m_pPlayerPool;
	// CVehiclePool* m_pVehiclePool;
	// CTextDrawPool* m_pTextDrawPool;
	CActorPool* m_pActorPool;
	// CGangZonePool* m_pGangZonePool;
	// CTextLabelPool* m_pTextLabelPool;
	// CPickupPool* m_pPickupPool;
	// CObjectPool* m_pObjectPool;
	// CMenuPool* m_pMenuPool;

	[[maybe_unused]] int iVehiclePoolProcessFlag = 0;
	[[maybe_unused]] int iPickupPoolProcessFlag = 0;
public:
    CNetGamePool();
	~CNetGamePool();

    void Process();
	void DeleteAll();

    CPlayerPool* GetPlayerPool() {
		return CNetGamePool::m_pPlayerPool;
	};

	// CVehiclePool* GetVehiclePool() {
	// 	return CNetGamePool::m_pVehiclePool;
	// };

	// CTextDrawPool* GetTextDrawPool() {
	// 	return CNetGamePool::m_pTextDrawPool;
	// };

	CActorPool* GetActorPool() {
		return CNetGamePool::m_pActorPool;
	};

	// CGangZonePool* GetGangZonePool() {
	// 	return CNetGamePool::m_pGangZonePool;
	// };

	// CTextLabelPool* GetTextLabelPool() {
	// 	return CNetGamePool::m_pTextLabelPool;
	// };

	// CPickupPool* GetPickupPool() {
	// 	return CNetGamePool::m_pPickupPool;
	// };

	// CObjectPool* GetObjectPool() {
	// 	return CNetGamePool::m_pObjectPool;
	// };

	// CMenuPool* GetMenuPool() {
	// 	return CNetGamePool::m_pMenuPool;
	// };
};