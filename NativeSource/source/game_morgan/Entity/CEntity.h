#pragma once

#include "CPhysicalGta.h"
#include "../Core/CVector.h"
#include "../Core/CMatrix.h"
#include "../Enums/eEntityType.h"

class CVehicle;

class CEntity
{
public:
    CEntity() = default;
	virtual ~CEntity() = default;
    virtual void Add();
    virtual void Remove();
    void Render();
	void SetMoveSpeedVector(CVector Vector);

	void GetTurnSpeedVector(CVector* Vector);
	void GetEulerInverted(float* fX, float* fY, float* fZ);
	void SetTurnSpeedVector(CVector Vector);
    uint16_t GetModelIndex();
	void TeleportTo(float x, float y, float z);
    void SetPosition(CVector vecPos);
    void GetPosition(CVector* vecRet);
	void SetMatrix(CMatrix Matrix, bool checkValid = true);
	void GetMatrix(CMatrix* Matrix);
	bool IsAdded();
	bool IsStationary();
	void GetBoundCentre(CVector* centre);
	bool SetModelIndex(unsigned int uiModel);
    float GetDistanceFromCamera();
	float GetDistanceFromLocalPlayerPed();
	float GetDistanceFromVehicle(CVehicle *pVehicle);
	float GetDistanceFromPoint(CVector vec);
	float GetDistanceFromPoint(float x, float y, float z);
    void GetMoveSpeedVector(CVector* Vector);

	uintptr_t GetRWObject();
	void DeleteRWObject();
	bool IsRenderable();
	void SetRwObjectAlpha(int nAlpha);
	void SetObjectColor(uint32_t dwColor1, uint32_t dwColor2);
	void SetObjectCollision(bool bEnable);
	void SetVisable(bool bVisable);
    CVector GetMoveSpeedVector();

	bool EnforceWorldBoundries(float fPX, float fZX, float fPY, float fNY);
	bool HasExceededWorldBoundries(float fPX, float fZX, float fPY, float fNY);
    void SetGravityProcessing(bool bProcess);
    void ClearStaticWaitingForCollision();

    void SetCollisionChecking(bool bCheck);
	void SetCollisionCheckingNP(bool bCheck);
    bool GetCollisionChecking();

	bool IsPointInsideEntity(CVector* pointStart, CVector* pointEnd, CVector* pOffset, float scaleFactor, bool enablePrecheck);

    void UpdateRwMatrixAndFrame();
    void UpdateMatrix(CMatrix mat);
	void UpdateRwFrame(CEntityGta* Entity);
	static void RemoveLighting(bool reset);

    bool HasMatrix() {
		if (m_pEntity) {
			return m_pEntity->mat ? true : false;
		}

		return false;
	}

	void AddPhysical();
	void RemovePhysical();
	void UpdateRpHAnim(CEntityGta* thiz);
	void SetPosn(float x, float y, float z);
	bool IsEntityIgnored();
	bool TestSphereCastVsEntity(uintptr_t CColSphere1, uintptr_t CColSphere2, bool bTrue);

public:
	static void CleanUpOldReference(CEntityGta* thiz, CEntityGta **ppAddress);

public:
	CPhysicalGta	*m_pEntity = nullptr;
	uint32_t m_dwGTAId = 0;
};