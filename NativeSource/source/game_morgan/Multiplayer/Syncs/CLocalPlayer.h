#pragma once

#include "../utils/Quat/CQuaternion.h"

// #include "../Core/CMultiPlayerShot.h"
#include "../../Entity/Ped/CPlayerPed.h"
#include "../../Entity/Ped/CPlayerPedSlot.h"
// #include "../../Entity/Vehicle/CVehicle.h"
#include "../../Enums/eMaxItem.h"
#include "../../Enums/eAction.h"
#include "../../Enums/eInvalidId.h"
#include "../../Enums/eRcModeType.h"
#include "../../Enums/eSurfingMode.h"
#include "../../Enums/eSurfingType.h"
#include "../../Enums/eSpecialAction.h"
#include "../vendor/raknet/BitStream.h"

#define SPECTATE_TYPE_NONE				0
#define SPECTATE_TYPE_PLAYER			1
#define SPECTATE_TYPE_VEHICLE			2

#define BULLET_HIT_TYPE_NONE            0
#define BULLET_HIT_TYPE_PLAYER          1
#define BULLET_HIT_TYPE_VEHICLE         2
#define BULLET_HIT_TYPE_OBJECT          3
#define BULLET_HIT_TYPE_PLAYER_OBJECT   4

#define IS_TARGETING(x) (x & 128)
#define IS_FIRING(x) (x & 4)

#pragma pack(push, 1)
typedef struct _SPECTATOR_SYNC_DATA
{
	uint16_t lrAnalog;
	uint16_t udAnalog;
	uint16_t wKeys;
	CVector vecPos;
} SPECTATOR_SYNC_DATA;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _ONFOOT_SYNC_DATA
{
	uint16_t lrAnalog;				// +0
	uint16_t udAnalog;				// +2
	uint16_t wKeys;					// +4
	CVector vecPos;					// +6
	CQuaternion quat;				// +18
	uint8_t byteHealth;				// +34
	uint8_t byteArmour;				// +35
	uint8_t byteCurrentWeapon;		// +36
	uint8_t byteSpecialAction;		// +37
	CVector vecMoveSpeed;			// +38
	CVector vecSurfOffsets;			// +50
	uint16_t wSurfInfo;				// +62
	int32_t dwAnimation;			// 64
} ONFOOT_SYNC_DATA;					// size = 68
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _INCAR_SYNC_DATA
{
	uint16_t VehicleID;			// +0
	uint16_t lrAnalog;				// +2
	uint16_t udAnalog;				// +4
	uint16_t wKeys;					// +6
	CQuaternion quat;				// +8
	CVector vecPos;					// +24
	CVector vecMoveSpeed;			// +36
	float fCarHealth;				// +48
	uint8_t bytePlayerHealth;		// +52
	uint8_t bytePlayerArmour;		// +53
	uint8_t byteCurrentWeapon;		// +54
	uint8_t byteSirenOn;			// +55
	uint8_t byteLandingGearState;	// +56
	uint16_t TrailerID;			// +57
	union {							// +59
		uint16_t sHydraThrustAngle[2];
		float fTrainSpeed;
	};
} INCAR_SYNC_DATA;					// size = 63
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _PASSENGER_SYNC_DATA
{
	uint16_t VehicleID;			// +0
	uint8_t byteSeatId;				// +2
	uint8_t byteCurrentWeapon;		// +3
	uint8_t bytePlayerHealth;		// +4
	uint8_t bytePlayerArmour;		// +5
	uint16_t lrAnalog;				// +6
	uint16_t udAnalog;				// +8
	uint16_t wKeys;					// +10
	CVector vecPos;					// +12
} PASSENGER_SYNC_DATA;				// size = 24
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _PLAYER_SPAWN_INFO
{
    uint8_t byteTeam;
    uint32_t iSkin;
    uint8_t unused;
    CVector vecPos;
    float fRotation;
    int iSpawnWeapons[3];
    int iSpawnWeaponsAmmo[3];
} PLAYER_SPAWN_INFO;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _TRAILER_SYNC_DATA
{
	uint16_t trailerId;	// +0
	CVector vecPos;			// +2
	CQuaternion quat;		// +14
	CVector vecMoveSpeed;	// +30
	CVector vecTurnSpeed;	// +42
} TRAILER_SYNC_DATA; 		// size = 54
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _UNOCCUPIED_SYNC_DATA
{
	uint16_t vehicleId;	// +0
	uint8_t byteSeatId;		// +2
	CVector vecRoll;			// +3
	CVector vecDirection;	// +15
	CVector vecPos;			// +27
	CVector vecMoveSpeed;	// +39
	CVector vecTurnSpeed;	// +51
	float fCarHealth;		// +63
} UNOCCUPIED_SYNC_DATA;		// size = 67
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _CAMERA_TARGET_DATA
{
	uint16_t m_nObject;
	uint16_t m_nVehicle;
	uint16_t m_nPlayer;
	uint16_t m_nActor;
} CAMERA_TARGET_DATA; // size = 8
#pragma pack(pop)

class CLocalPlayer
{
private:
	uint16_t m_LocalPlayerID;
	int Score;
	int Ping;

public:
	CLocalPlayer();
	~CLocalPlayer();

    bool Process();
	void UpdateRemoteInterior(uint8_t byteInterior);
	void HandleDrunkState();
	void HandleAfterDeathToggle();
	void HandleNeedsToRespawn();
	void HandleInitialSpawnConditions();
	void HandleDancing();
	void HandleUrinating();
	void HandleHandsUp();
	void HandleStuff();
	void HandleVehicleEntryTimeout(uint32_t dwThisTick);
	void HandlePassengerEntry();
	void HandleInteriorChanging();
	void DisplayZone(uint32_t dwThisTick);
	void SendStats(uint32_t dwThisTick);
	void UpdateWeapons();
	void UpdateCameraTarget();
	void GetCameraTarget(CAMERA_TARGET_DATA* target);
	void UpdateModel(uint32_t dwThisTick);
	void HandleDead();
	bool NeedsToUpdate(const void *pOld, const void *pNew, unsigned int nLen, uint32_t tickDef = 500);
	void ProcessVehicleDamageUpdates(uint16_t CurrentVehicle);
	void ProcessOnFoot(uint32_t dwThisTick);
	void ProcessDriver(uint32_t dwThisTick);
	void ProcessPassenger(uint32_t dwThisTick);
	void ProcessAimSync(uint32_t dwThisTick);
	void ProcessAnimation();

    void SetLocalID(uint16_t ID) {
		m_LocalPlayerID = ID;
	};

	uint16_t GetLocalID() {
		return m_LocalPlayerID;
	};

	int GetLocalScore() {
		return Score;
	};

	int GetLocalPing() {
		return Ping;
	};

	void SetLocalPing(int ping) {
		Ping = ping;
	};

	void SetLocalScore(int score) {
		Score = score;
	};

	void Deactivate()
	{
		m_bIsActive = false;
		m_iSelectedClass = 0;
	};

	void EnableSpectating(bool bToggle);
	void SpectateForPlayer(uint16_t playerId);
	void SpectateForVehicle(uint16_t VehicleID);
	void HandleClassSelection();
	void HandleClassSelectionOutcome(bool bToggle);
	void RequestClass(int iClass);
	void SetSpawnInfo(PLAYER_SPAWN_INFO *pSpawn);
	void RequestSpawn();
	void NextClass();
	void PrevClass();
	void SendSpawnRequest();
	bool Spawn();

	void ResetData();
	void ReturnToClassSelection() { m_bEnableAfterDeath = true; };
	CPlayerPed * GetPlayerPed() { return m_pPlayerPed; };
	bool IsSpectating() { return m_bIsSpectating; };
	bool IsSpawnDialog() { return m_bSpawnDialogShowed; };
	bool IsSpawnInfo() { return m_bHasSpawnInfo; };
	bool IsInRCMode() { return m_bInRCMode; };

	void SendWastedNotification();
	void SendOnFootFullSyncData();
	void SendPassengerFullSyncData();
	void SendInCarFullSyncData();
	void SendAimSyncData();
	void SendTrailerData(uint16_t vehicleId);
	void SendUnoccupiedData(uint16_t vehicleId, uint8_t syncMode);

	// void EndSurfing();
	// void ProcessSurfing();
	// void SetSurfing(bool bStuck, CVehicle* pVehicle);
	// void UpdateSurfing();

	void UpdateRCMode(eRcModeType type);
	void ProcessOnFootWorldBounds();
	void ProcessInCarWorldBounds();
	void ProcessPassengerDriveBy();
	// bool ProcessUnoccupied(uint16_t vehicleId, CVehicle* pVehicle);
	void ApplySpecialAction(uint8_t byteSpecialAction);
	uint8_t GetSpecialAction();
	void SetLocalTeam(uint8_t byteTeam);
	uint8_t GetLocalTeam();
	void ProcessHead();
	bool NeedsToSendOnfootData(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog);
	int GetOptimumOnFootSendRate();
	int GetOptimumInCarSendRate();
	int GetOptimumUnoccupiedSendRate();
	uint8_t DetermineNumberOfPlayersInLocalRange();
	void ProcessSpectating();
	uint32_t GetPlayerColor();
	void SetPlayerColor(uint32_t dwColor);
	uint32_t GetPlayerColorAsARGB();

	void SendTakeDamage(uint16_t PlayerID, float fDamageFactor, uint32_t weaponType, uint32_t pedPieceType);
	void SendGiveDamage(uint16_t PlayerID, float fDamageFactor, uint32_t weaponType, uint32_t pedPieceType);
	void SendGiveDamageActor(uint16_t ActorID, float fDamageFactor, uint32_t weaponType, uint32_t pedPieceType);
	void SendEnterVehicleNotification(uint16_t VehicleID, bool bPassenger);
	void SendExitVehicleNotification(uint16_t VehicleID);
	void SendSCMEvent(RakNet::BitStream bsData);

public:
	bool				m_bWaitingForSpawnRequestReply;
	uint8_t 			m_byteTeamId;

	bool				m_bIsSpectating;
	uint8_t				m_byteSpectateMode;
	uint8_t				m_byteSpectateType;
	uint32_t			m_SpectateID;
	bool				m_bSpectateProcessed;
	int32_t				m_dwAnimationValue;
	bool				m_bAnimationReset;

	uint16_t			m_CurrentVehicle;
	uint16_t 			m_LastVehicle;

    bool				m_bIsActive;

	// struct  
	// {
	// 	uint16_t     entityId;
	// 	uint32_t     lastUpdate;

	// 	union {
	// 		CVehicle* pVehicle;
	// 		CObject*  pObject;
	// 	};

	// 	bool         bIsStuck;
	// 	bool         bIsActive;
	// 	CVector      vecPosition;
	// 	int          iUnknown;
	// 	int          iSyncMode;
	// } m_surfData;

	struct  
	{
		CVector m_vecHeadDir;
		uint32_t m_dwLastHeadUpdate;
		uint32_t m_dwLastHeadLook;
	} m_headData;

	struct  
	{
		uint16_t m_nAimedPlayer;
		uint16_t m_nAimedActor;
		uint8_t m_nCurrentWeapon;
		uint8_t	m_byteLastWeapon[13];
		uint32_t m_dwLastAmmo[13];
	} m_weaponsData;

	struct  
	{
		uint16_t m_nVehicleUpdating;
		uint32_t m_nPanel;
		uint32_t m_nDoor;
		uint8_t m_bLight;
		uint8_t m_bTire;
	} m_damageData;

	CAMERA_TARGET_DATA 	m_cameraTargetData;

private:
	CPlayerPed*			m_pPlayerPed;

	bool				m_bIsWasted;
	uint8_t				m_byteCurInterior;
	bool				m_bInRCMode;
	bool				m_bSpawnDialogShowed;
	bool				m_bCheckShowUISpectating;
	bool				m_bPassengerDriveByMode;
	unsigned long		m_ulThisSyncFrame;
	unsigned long		m_ulLastSyncFrame;

	PLAYER_SPAWN_INFO 	m_SpawnInfo;
	ONFOOT_SYNC_DATA 	m_OnFootData;
	// INCAR_SYNC_DATA 	m_InCarData;
	// PASSENGER_SYNC_DATA m_PassengerData;
	// AIM_SYNC_DATA		m_aimData;
	// TRAILER_SYNC_DATA 	m_TrailerData;
	// UNOCCUPIED_SYNC_DATA m_UnoccupiedData;

	int					m_iSelectedClass;
	bool				m_bHasSpawnInfo;
	bool				m_bEnableAfterDeath;
	bool				m_bClearedToSpawn;

	int					m_iDisplayZoneTick;
	uint32_t 			m_dwLastAnySendTick;
	uint32_t 			m_dwLastSendTick;
	uint32_t			m_dwLastSendSpecTick;
	uint32_t			m_dwLastSendAimTick;
	uint32_t			m_dwLastStatsUpdateTick;
	uint32_t			m_dwLastUpdateModelTick;
	uint32_t			m_dwLastWeaponsUpdateTick;
	uint32_t			m_dwLastCameraTargetUpdateTick;
	uint32_t			m_dwLastSpawnSelectionTick;
	uint32_t			m_dwInitialSelectionTick;
};