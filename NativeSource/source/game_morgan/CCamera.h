#pragma once

#include "Core/CMatrix.h"
class CEntity;

// #include "Enums/eCamMode.h"
// #include "Enums/eSwitchType.h"

class CCamera
{
public:
	static float& m_fMouseAccelHorzntl;
	// static float m_fStreamingDistanceScale;
	static CCamera& TheCamera;
	static float& m_f3rdPersonCHairMultX;
	static float& m_f3rdPersonCHairMultY;
	static bool m_bPCCamera;

public:
	CCamera();
	~CCamera() {}

	void SetBehindPlayer();
	void Restore();
	void SetPosition(float fX, float fY, float fZ, float fRotationX, float fRotationY, float fRotationZ);
	void LookAtPoint(float fX, float fY, float fZ, int iType);
	void Fade(int iInOut);

	void GetMatrix(CMatrix* mat);
	float GetDistanceToPoint(CVector *pos);
	void InterpolateCameraPos(CVector *posFrom, CVector *posTo, int time, uint8_t mode);
	void InterpolateCameraLookAt(CVector *posFrom, CVector *posTo, int time, uint8_t mode);

	// void Attach(CEntity* pEntity);

	CEntity* GetAttach() {
		return m_pAttachedTo;
	};

	// void Update();
	static void LerpFOV(float from, float to, float time, bool bSetSmoothEnds);

private:
	CMatrix *m_matPos = nullptr;
	CEntity* m_pAttachedTo;
};