#pragma once

#include "../CEntity.h"
#include "../Ped/CPedGta.h"

class CActor : public CEntity
{
private:
	bool m_bInvulnerable;
	CPedGta* m_pPed;
public:
	CActor(uint16_t usModel, VECTOR vecPosition, float fRotation);
	~CActor() override;

	void ApplyAnimation(char *szAnimName, char *szAnimFile, float fDelta, int bLoop, int bLockX, int bLockY, int bFreeze, int uiTime);
	void ClearAnimations();
	void SetPosition(VECTOR vecPosition);
	void SetAngle(float fAngle);
	void SetHealth(float fHealth);
	void SetInvulnerable(bool bEnable);

	bool IsInvulnerable() {
		return m_bInvulnerable;
	}
};