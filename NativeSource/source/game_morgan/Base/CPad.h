#pragma once

#include "../Entity/Ped/CPedGta.h"
#include "../Enums/ePadKeys.h"
#include "../Enums/eMaxItem.h"
#include "../Enums/eWeaponSampId.h"

typedef struct
{
	uint16_t wKeyLR;
	uint16_t wKeyUD;
	bool bKeys[ePadKeys::SIZE + 4];
	bool bIgnoreJump;
	bool bIgnoreSwim;
    bool bIgnoreCrouch;
	bool bFightingReady;
	bool bCarryActive;
	bool bCarEngineOff;
} PAD_KEYS;

enum eJoystickKeys {
	KEY_LR,
	KEY_UD
};

enum Context {
    LOCAL_PLAYER,
    REMOTE_PLAYER
};

class CPad
{
public:
	static Context m_pCurrentContextOnFoot;
	static Context m_pCurrentContextInCar;
	static uint8_t m_byteCurDriver;
	static uint8_t m_byteCurPlayer;
	static bool m_bEnableUpdatePosition;
	static CPedGta* dwCurWeaponProcessingPlayer;
	static uint32_t m_iSaveMeleeAttack;
	static uint32_t m_iSaveBlockPunch;
	static int m_iCrouchStyle;
	static uint32_t m_iSaveTargetingActive;
	static uint16_t m_iSaveAccelerate;
	static int SprintGet;
	static bool m_bCameraToggleStyle;

public:
	static void SwitchOnFootContextToLocalPlayer(uint8_t byteCur = 0) {
		m_byteCurPlayer = byteCur;
		m_pCurrentContextOnFoot = Context::LOCAL_PLAYER;
	};

	static void SwitchOnFootContextToRemotePlayer(uint8_t byteCur) {
		m_byteCurPlayer = byteCur;
		m_pCurrentContextOnFoot = Context::REMOTE_PLAYER;
	};

	static void SwitchInCarContextToLocalPlayer(uint8_t byteCur = 0) {
		m_byteCurDriver = byteCur;
		m_pCurrentContextInCar = Context::LOCAL_PLAYER;
	};

	static void SwitchInCarContextToRemotePlayer(uint8_t byteCur) {
		m_byteCurDriver = byteCur;
		m_pCurrentContextInCar = Context::REMOTE_PLAYER;
	};

public:
    static PAD_KEYS LocalPlayerKeys;
    static PAD_KEYS RemotePlayerKeys[eMaxItem::PLAYER_PED_SLOTS];

public:
    static void Initialise();
    static void LocalProcessControl(uintptr_t thiz, void (*CPed_ProcessControl)(uintptr_t));
    static void RemoteProcessControl(uintptr_t thiz, void (*CPed_ProcessControl)(uintptr_t));
    static uint16_t GetPedWalk(CPad* thiz, eJoystickKeys keyType, int16_t (*CPad_GetPedWalk)(CPad*));
    static bool isLocalPlayerSprinting();
    static void DisplayWidgets(bool bDisp);
    static bool IsActiveWidgets();
};
