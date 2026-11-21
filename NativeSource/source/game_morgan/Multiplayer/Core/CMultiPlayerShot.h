#pragma once

#include "../../Entity/CEntityGta.h"

enum eSAMPWeaponState
{
	WS_NO_BULLETS = 0,
	WS_LAST_BULLET = 1,
	WS_MORE_BULLETS = 2,
	WS_RELOADING = 3,
};

#pragma pack(push, 1)
typedef struct _BULLET_SYNC_DATA
{
	uint8_t targetType;	// +0
	uint16_t targetId;	// +1
	CVector vecOrigin;	// +3
	CVector vecPos;		// +15
	CVector vecOffset;	// +27
	uint8_t weaponId;	// +28
} BULLET_SYNC_DATA;					// size = 29
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _AIM_SYNC_DATA
{
    uint8_t	byteCamMode; // +0
    CVector vecAimf; // +1
    CVector vecAimPos; // +13
    float fAimZ; // +25
    uint8_t byteCamExtZoom : 6;	// +29 - 0-63 normalized
    uint8_t byteWeaponState : 2; // +30 - see eWeaponState
	uint8_t byteAspectRatio; // +31
} AIM_SYNC_DATA; // size = 31 (zoom and weapon state share one byte)
static_assert(sizeof(AIM_SYNC_DATA) == 31);
#pragma pack(pop)

class CMultiPlayerShot
{
public:
	static void Initialize();
	static void SendBulletSync(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CEntityGta **ppEntity);
};