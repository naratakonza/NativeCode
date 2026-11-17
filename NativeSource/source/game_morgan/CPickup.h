#pragma once

#include "Core/CVector.h"
#include "Enums/eSpecialObject.h"

class CPickup
{
private:
	uint32_t m_dwHandle;
	int 	 m_iPickupId;
	int 	 m_iType;
	CVector  m_vecPos;

public:
	CPickup(int iModel, int iType, float x, float y, float z, int* pickId, bool isWeaponPickup = false);
	~CPickup();

	bool IsPickedUp();
    void SetID(int id);

	uint32_t GetHandle() {
		return CPickup::m_dwHandle;
	};

	int GetID() {
		return CPickup::m_iPickupId;
	};

	int GetType() {
		return CPickup::m_iType;
	};

	CVector GetPosition() {
		return CPickup::m_vecPos;
	};
};