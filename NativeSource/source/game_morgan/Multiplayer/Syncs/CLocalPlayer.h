#pragma once

#include "../../../utils/Quat/CQuaternion.h"

// #include "../Core/CMultiPlayerShot.h"
#include "../../Entity/Ped/CPlayerPed.h"
#include "../../Entity/Ped/CPlayerPedSlot.h"
// #include "../../Entity/Vehicle/CVehicle.h"
// #include "../../Enums/eMaxItem.h"
// #include "../../Enums/eAction.h"
// #include "../../Enums/eInvalidId.h"
// #include "../../Enums/eRcModeType.h"
// #include "../../Enums/eSurfingMode.h"
// #include "../../Enums/eSurfingType.h"
// #include "../../Enums/eSpecialAction.h"
#include "../../../vendor/raknet/BitStream.h"

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

public:
	CLocalPlayer();
	~CLocalPlayer();
    void ResetData();

    bool Process();

public:

private:
	CPlayerPed*			m_pPlayerPed;
};