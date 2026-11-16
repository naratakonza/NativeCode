#pragma once

#include "../CEntity.h"
#include "CPedGta.h"

#include "../../CCameraAim.h"
#include "../../Enums/eMaxItem.h"
#include "../../Enums/eStuffType.h"

typedef struct _ATTACHED_OBJECT_INFO
{
	uint32_t dwModelId;
	uint32_t dwBone;
	CVector vecOffset;
	CVector vecRotation;
	CVector vecScale;
	uint32_t firstMaterialColor;
	uint32_t secondMaterialColor;
} ATTACHED_OBJECT_INFO;

typedef struct _ATTACHED_OBJECT_INFO_INTERNAL
{
	uint32_t dwModelId;
	uint32_t dwBone;
	CVector vecOffset;
	CVector vecRotation;
	CVector vecScale;
	uint32_t firstMaterialColor;
	uint32_t secondMaterialColor;
	
	bool bState;
	class CObject* pObject;
} ATTACHED_OBJECT_INFO_INTERNAL;

typedef struct _BULLET_DATA {
	CVector vecOrigin;
	CVector vecPos;
	CVector vecOffset;
	CEntityGta* pEntity;
} BULLET_DATA;

class CPlayerPed : public CEntity
{
private:
	bool 		m_bLockControl;
	uint8_t 	lastSpecialAction;

	ATTACHED_OBJECT_INFO_INTERNAL m_Accessories[eMaxItem::MAX_ATTACHED_OBJECTS];

public:
    CPlayerPed();	// local
	CPlayerPed(uint8_t bytePlayerNumber, int iSkin, float fX, float fY, float fZ, float fRotation); // remote
	~CPlayerPed();

	void SetMoney(int iAmount);
	void ForceTargetRotation(float fRotation);
	float GetTargetRotation();
	void  SetRotation(float fRotation);
	static void ProcessAnimGroups(bool bUseCJWalk);
	void SetModelIndex(unsigned int uiModel);
	void SetInterior(uint8_t byteID, bool refresh = false);
	void DestroyFollowPedTask();
	bool VerifyInstance();
	void RefreshStreamingByPos();
	void SetInitialState();
	void RestartIfWastedAt(CVector *vecRestart, float fRotation);
	bool IsInVehicle();
	bool IsAPassenger();

	void SetAlpha(int nAlpha);
	void SetHealth(float fHealth);
	void SetArmour(float fArmour);

	int GetAlpha();
	float GetHealth();
	float GetArmour();
	bool IsDead();

	void GiveWeapon(int iWeaponID, int iAmmo);
	void SetArmedWeapon(int iWeaponID, bool unk = false);
	void SetWeaponSkill(uint32_t iWeaponType, uint16_t byteSkill);
	void SetAmmo(uint8_t bWeaponID, uint16_t wAmmo);
	WEAPON_SLOT_TYPE* FindWeaponSlot(uint32_t dwWeapon);
	WEAPON_SLOT_TYPE* GetCurrentWeaponSlot();
	uint8_t GetCurrentWeapon();
	bool CurrentWeaponHasAmmo();
	void GetAllWeapons(uint8_t* pOutWeapons);
	void ClearAllWeapons();
	void ResetDamageEntity();

	void LockControl(bool bLock);
	void ToggleControllable(bool bToggle);
	void ToggleControllableWithoutLock(bool bToggle);

	void ApplyAnimation(char *szAnimName, char *szAnimFile, float fT, int opt1, int opt2, int opt3, int opt4, int iUnk);
	void ClearAnimations();
	void RemoveFromVehicleAndPutAt(float fX, float fY, float fZ);
	uint8_t GetExtendedKeys();
	void ShuffleKeyLogic(int key);
	uint16_t GetKeys(uint16_t *lrAnalog, uint16_t *udAnalog, bool clear = true);
	void SetKeys(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog);
	void ClumpUpdateAnimations(float step, int flag);
	void SetStateFlags(uint32_t dwState);
	uint32_t GetStateFlags();
	bool IsOnGround();
	void GetBonePosition(int iBoneID, CVector* vecOut);

	void ApplyCrouch();
	void ResetCrouch();
	bool IsCrouching();

	void SetDead();
	void ExtinguishFire();

	bool IsPlayingAnimation(const char*);
	void ClearAllTasks();
	void ProcessCarry();
	void ProcessCarryControl();
	void ToggleCarry(bool carry);

	bool IsCarry() { 
		return CPlayerPed::m_bCarryState; 
	};

	int GetVehicleSeatID();
	void ApplyCommandTask(char* a2, int a4, int a5, int a6, CVector* a7, char a8, float a9, int a10, int a11, char a12);
	uint8_t FindDeathReasonAndResponsiblePlayer(uint16_t *nPlayer);

	void AddAccessory(ATTACHED_OBJECT_INFO* pInfo, int iSlot);
	void DeleteAccessory(int iSlot);
	void DeleteAllAccessories(bool destroyObject = false);
	bool HasAccessory();
	ATTACHED_OBJECT_INFO_INTERNAL& GetAccessory(uint32_t index);
	bool GetAccessoryState(int iSlot);
	void ProcessAccessories();
	void ProcessHeadMatrix();
	bool IsAnimationPlaying(int animIndex);
	void PlayAnimationByIndex(uint32_t dwAnimation);
	void FireInstant();
	void ProcessBulletData(BULLET_DATA* btData);
	void GetWeaponInfoForFire(int bIsLeftHand, CVector *vecBone, CVector *vecOut);
	CVector* GetCurrentWeaponFireOffset();
	void GetTransformedBonePosition(int iBoneID, CVector* vecOut);

	void PutDirectlyInVehicle(uint32_t iVehicleID, uint8_t iSeat);
	void EnterVehicle(int iVehicleID, bool bPassenger);
	void ExitCurrentVehicle();
	CVehicleGta* GetVehicle();

	uint8_t IsEnteringVehicle();
	bool IsExitingVehicle();
	bool IsJumpTask();
	bool IsTakeDamageFallTask();
	void* IsPerformingCarryTask();

	CEntityGta* GetEntityUnderPlayer();
	CEntityGta* GetTargetEntity();
	void RemoveWeaponWhenEnteringVehicle();

public:
	bool IsControlLocked() { return m_bLockControl; };

	CPedGta* GetPed() {
		return m_pPed;
	};

	CPhysicalGta* GetEntity() {
		return m_pEntity;
	}
	
	void SetDrunkLevel(uint32_t dwLevel);
	void ProcessDrunk();
	void SetDrunkVisuals(uint32_t dwVisuals);
	void SetDrunkHandling(uint32_t dwHandling);

	uint32_t GetDrunkLevel() { 
		return CPlayerPed::m_stuffData.dwDrunkLevel; 
	};

	void  SetFightingStyle(int iStyle);
	int   GetFightingStyle();

	void SetImmunities(int BP, int FP, int EP, int CP, int MP);
	void ShowMarker(uint32_t iMarkerColorID);
	void HideMarker();

	void SetActionTrigger(uint8_t action);
	uint8_t GetActionTrigger();

	void SetAimZ(float fAimZ);
	float GetAimZ();

	void SetCurrentAim(CAMERA_AIM* pAim);
	CAMERA_AIM* GetCurrentAim();

	void SetCameraMode(uint16_t byteCamMode);
	uint8_t GetCameraMode();

	void SetCameraExtendedZoom(float, float);
	float GetCameraExtendedZoom();

	void StartJetpack();
	void StopJetpack();
	bool IsInJetpack();

	bool StartPassengerDriveByMode();
	void StopPassengerDriveByMode();
	bool IsInPassengerDriveByMode();

	bool OpenParachute();
	void DestroyParachute();
	void ProcessParachuteEvent(const char* szAnimName);

	bool IsPissing() { 
		return CPlayerPed::m_bPissingState; 
	};
	
	void StartPissing();
	void StopPissing();

	void ToggleCellphone(int enable);
	
	int IsCellphoneEnabled() { 
		return CPlayerPed::m_iCellPhoneEnabled; 
	}

	void ProcessCuffed();
	void ProcessCuffedControl();
	void ToggleCuffed(bool cuff);

	bool IsCuffed() { 
		return CPlayerPed::m_bCuffedState; 
	};

	void HandsUp();
	bool HasHandsUp();

	void StartDancing(int danceId);
	void StopDancing();
	void ProcessDancing();
	char* GetDanceAnimForMove(int iMove);
	
	bool IsDancing() {
		return CPlayerPed::m_bDanceState;
	};
	
	void ApplyStuff();
	void GiveStuff(eStuffType type);
	void DropStuff();
	eStuffType GetStuff();
	int GetPedStat();
	bool IsPerformingCustomAnim();
	void ProcessVehicleHorn();
	bool IsInWater();
	void SetEntryExit(char* szShopName);
	uintptr_t* GetEntryExit();
	char* GetLoadedShoppingDataSubsection();
	void ResetPlayerTimeLastEaten();
	void* GetNamedAnimationTask();
	int32_t GetBlendedAnimationsIndex();

public:
	CPedGta*		m_pPed;
	uint8_t		m_bytePlayerNumber;
	uint32_t	m_dwArrow;

	int 		m_iSpecialAction;
	int			m_iAlpha;

	bool		m_bHaveBulletData;
	BULLET_DATA m_bulletData;
	
	uint32_t	m_dwParachuteObject;

	bool		m_bPissingState;
	uint32_t	m_dwPissParticlesHandle;

	bool		m_bDanceState;
	int			m_iDanceStyle;
	int			m_iLastDanceMove;
	
	bool 		m_bCuffedState;
	bool		m_bCarryState;
	int			m_iCellPhoneEnabled;

	struct {
		eStuffType type;
		uint32_t dwObject;
		uint32_t dwDrunkLevel;
		uint32_t dwLastUpdateTick;
	} m_stuffData;

	bool m_bJumpControl;
	bool m_bCrouchControl;
	bool m_bSprintControl;

	CMatrix m_HeadBoneMatrix;
public:
	static bool m_bRebuildPlayer;
};