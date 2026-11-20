#include "../../../Main.h"
#include "CPad.h"
#include "../CCameraAim.h"
#include "../CCameraController.h"
#include "../CGyro.h"
#include "../CSensitivity.h"
#include "../CTimer.h"
#include "../CTouchInterface.h"
#include "../CUtil.h"
#include "../CWorld.h"
#include "../Entity/Ped/CPedPointerRecorder.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Multiplayer/Guis/CGui.h"

#include <mach-o/dyld.h>

PAD_KEYS CPad::LocalPlayerKeys;
PAD_KEYS CPad::RemotePlayerKeys[eMaxItem::PLAYER_PED_SLOTS]{};

Context CPad::m_pCurrentContextOnFoot = Context::LOCAL_PLAYER;
Context CPad::m_pCurrentContextInCar = Context::LOCAL_PLAYER;
uint8_t CPad::m_byteCurDriver = 0;
uint8_t CPad::m_byteCurPlayer = 0;
bool CPad::m_bEnableUpdatePosition = true;
uint32_t CPad::m_iSaveBlockPunch = 0;
int CPad::m_iCrouchStyle = 0;
bool CPad::m_bCameraToggleStyle = false;
uint32_t CPad::m_iSaveTargetingActive = 0;
uint32_t CPad::m_iSaveMeleeAttack = 0;
uint16_t CPad::m_iSaveAccelerate = 0;
CPedGta* CPad::dwCurWeaponProcessingPlayer;
uint16_t wSavedCameraMode;
uint16_t wSavedCameraMode2;
uint8_t byteSavedControlFlags;
uint8_t byteHornSaveControlFlags;
CMatrix *matSavedMatrix;
CVector vecSavedMoveSpeed;
static uint8_t byteInternalPlayer = 0;

float fSavedBikeLean;
float dwSavedBikeFlags;

uint32_t SwimJumpJustDown_VAR = 0;
int JumpJustDown_VAR = 0;
int CPad::SprintGet = 0;

extern uint32_t m_dwJumpTickCounter;
extern uint32_t m_dwMeleeAttackTickCounter;
extern uint32_t m_dwDiveTickCounter;
extern uint32_t m_dwAbortClimbTickCounter;

static bool padProcessControlInstalled = false;
static bool pedMethodHooksInstalled = false;
static bool taskUseGunHookInstalled = false;
static bool weaponHooksInstalled = false;
static bool weaponSkillHookInstalled = false;
static uint16_t byteSavedCameraMode = 0;

extern "C"
{
    __attribute__((visibility("default"))) uint8_t CPad_AllowAimingRotation = 1;
}

void CPad::LocalProcessControl(uintptr_t thiz, void (*CPed_ProcessControl)(uintptr_t))
{
    const uint8_t savedAllowAimingRotation = CPad_AllowAimingRotation;
    CPad_AllowAimingRotation = 1;
    CPed_ProcessControl(thiz);
    CPad_AllowAimingRotation = savedAllowAimingRotation;
}

uint16_t CPad::GetPedWalk(CPad* thiz, eJoystickKeys keyType, int16_t (*CPad_GetPedWalk)(CPad*))
{
	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		uint16_t dwResult = 0;
		switch (keyType)
		{
			case eJoystickKeys::KEY_LR: {
				dwResult = CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].wKeyLR;
				break;
			}

			case eJoystickKeys::KEY_UD: {
				dwResult = CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].wKeyUD;
				break;
			}
		}

		bool bWalk = CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_WALK];

		if (bWalk)
		{
			if (CMultiPlayer::GetActiveNetwork() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools())
			{
				CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
				if (pPlayerPool)
				{
					uintptr_t pPed = CPedPointerRecorder::FindPlayerPointerFromNum(CPad::m_byteCurPlayer);
					if (pPed)
					{
						uint16_t playerId = pPlayerPool->FindRemotePlayerIDFromGtaPtr(reinterpret_cast<CPedGta*>(pPed));
						if (playerId != eInvalidId::INVALID_PLAYER_ID)
						{
							if (pPlayerPool->IsPlayerNPC(playerId))
							{
								dwResult = 0x20;
							}
						}
					}
				}
			}
		}

		if ((dwResult == 0xFF80 || dwResult == 0x80) && bWalk)
		{
			dwResult = 0x40;
		}

		return dwResult;
	}
	else
	{
		uint16_t dwResult = 0;
		switch (keyType)
		{
			case eJoystickKeys::KEY_LR: {
				dwResult = CPad_GetPedWalk(thiz);

				if (CPad::LocalPlayerKeys.wKeyLR == 0)
				{
					CPad::LocalPlayerKeys.wKeyLR = dwResult;
				}

				break;
			}

			case eJoystickKeys::KEY_UD: {
				dwResult = CPad_GetPedWalk(thiz);

				if (CPad::LocalPlayerKeys.wKeyUD == 0)
				{
					CPad::LocalPlayerKeys.wKeyUD = dwResult;
				}

				break;
			}
		}

		return dwResult;
	}
}

void CPad::RemoteProcessControl(uintptr_t thiz, void (*CPed_ProcessControl)(uintptr_t))
{
	wSavedCameraMode = *pbyteCameraMode;
	*pbyteCameraMode = CCameraAim::GameGetPlayerCameraMode(CPad::m_byteCurPlayer);

	wSavedCameraMode2 = *wCameraMode2;
	*wCameraMode2 = CCameraAim::GameGetPlayerCameraMode(CPad::m_byteCurPlayer);
	if (*wCameraMode2 == 4) 
	{
		*wCameraMode2 = 0;
	}

	CCameraAim::GameStoreLocalPlayerCameraExtZoom();
	CCameraAim::GameSetRemotePlayerCameraExtZoom(CPad::m_byteCurPlayer);

	CCameraAim::GameStoreLocalPlayerAim();
	CCameraAim::GameSetRemotePlayerAim(CPad::m_byteCurPlayer);

	CCameraAim::GameStoreLocalPlayerSkills();
	CCameraAim::GameSetRemotePlayerSkills(CPad::m_byteCurPlayer);

	CWorld::PlayerInFocus = CPad::m_byteCurPlayer;

	CPad::m_bEnableUpdatePosition = false;
    CPad::RemoveInvalidRemotePlayer(thiz, CPed_ProcessControl);
	CPad::m_bEnableUpdatePosition = true;

	CCameraAim::GameSetLocalPlayerSkills();

	*pbyteCameraMode = wSavedCameraMode;
	*wCameraMode2 = wSavedCameraMode2;

	CCameraAim::GameSetLocalPlayerCameraExtZoom();

	CWorld::PlayerInFocus = 0;

	CPad::SwitchOnFootContextToLocalPlayer();
	CCameraAim::GameSetLocalPlayerAim();
}

void CPad::RemoveInvalidRemotePlayer(uintptr_t thiz, void (*CPed_ProcessControl)(uintptr_t))
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    if (!pPlayerPool) {
        return;
    }

    uint16_t playerId = pPlayerPool->FindRemotePlayerIDFromGtaPtr(reinterpret_cast<CPedGta*>(thiz));

    try
    {
        CPed_ProcessControl(thiz);
    }
    catch (...)
    {
        if (playerId != eInvalidId::INVALID_PLAYER_ID)
        {
            CRemotePlayer* pPlayer = pPlayerPool->GetRemotePlayer(playerId);
            if (pPlayer)
            {
                pPlayer->RemoveGamePlayer(4);
            }

            if (CGui::GetChat())
            {
                CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Removed player %u due to error."), playerId);
                CNecessary::Log(OBFUSCATE("Removed player %u due to error."), playerId);
            }
        }
    }
}

extern "C" __attribute__((visibility("default"))) void CPed_UpdatePosition_hook(uintptr_t thiz, void (*CPed_UpdatePosition)(uintptr_t))
{
    if (!padProcessControlInstalled)
    {
        CPed_UpdatePosition(thiz);
        return;
    }

    if (!CPad::m_bEnableUpdatePosition) {
        return;
    }

    CPedGta* pPed = reinterpret_cast<CPedGta*>(thiz);
    CPhysicalGta* pSavedGroundPhysical = nullptr;
    bool bFixApplied = false;

    if (pPed && pPed->m_pGroundPhysical)
    {
        CEntityGta* pStandingOn = pPed->m_pEntityStandingOn;
        if (pStandingOn && pStandingOn->m_nType == eEntityType::ENTITY_TYPE_OBJECT)
        {
            CPhysicalGta* pGroundPhys = pPed->m_pGroundPhysical;
            float speedSq = pGroundPhys->vecMoveSpeed.x * pGroundPhys->vecMoveSpeed.x + pGroundPhys->vecMoveSpeed.y * pGroundPhys->vecMoveSpeed.y + pGroundPhys->vecMoveSpeed.z * pGroundPhys->vecMoveSpeed.z;

            // แก้ไขพื้นลื่นขณะวิ่งบน Object
            if (speedSq < 0.001f * 0.001f)
            {
                pSavedGroundPhysical = pPed->m_pGroundPhysical;
                pPed->m_pGroundPhysical = nullptr;
                bFixApplied = true;
            }
        }
    }

    CPed_UpdatePosition(thiz);

    if (bFixApplied)
    {
        pPed->m_pGroundPhysical = pSavedGroundPhysical;
    }
}

uint32_t CPad::TaskUseGun(uintptr_t thiz, uintptr_t pPed)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x3499C4), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9", 8, "__text");
    if (!verified) {
        return 0;
    }

    if (!taskUseGunHookInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive()) {
        return reinterpret_cast<uint32_t (*)(uintptr_t, uintptr_t)>(SA_ADDR(0x3499C4) + _dyld_get_image_vmaddr_slide(0))(thiz, pPed);
    }

    uint32_t pedPositionResult = 0;
    byteInternalPlayer = CWorld::PlayerInFocus;
    CPad::m_byteCurPlayer = CPedPointerRecorder::FindPlayerNumFromPointer(pPed);

    if (thiz && CPad::m_byteCurPlayer != 0 && byteInternalPlayer == 0)
    {
        CPad::SwitchOnFootContextToRemotePlayer(CPad::m_byteCurPlayer);

        byteSavedCameraMode = *pbyteCameraMode;
        *pbyteCameraMode = CCameraAim::GameGetPlayerCameraMode(CPad::m_byteCurPlayer);

        wSavedCameraMode2 = *wCameraMode2;
        *wCameraMode2 = CCameraAim::GameGetPlayerCameraMode(CPad::m_byteCurPlayer);
        if (*wCameraMode2 == 4)
        {
            *wCameraMode2 = 0;
        }

        CCameraAim::GameStoreLocalPlayerCameraExtZoom();
        CCameraAim::GameSetRemotePlayerCameraExtZoom(CPad::m_byteCurPlayer);

        CCameraAim::GameStoreLocalPlayerAim();
        CCameraAim::GameSetRemotePlayerAim(CPad::m_byteCurPlayer);

        CCameraAim::GameStoreLocalPlayerSkills();
        CCameraAim::GameSetRemotePlayerSkills(CPad::m_byteCurPlayer);

        CWorld::PlayerInFocus = CPad::m_byteCurPlayer;

        pedPositionResult = reinterpret_cast<uint32_t (*)(uintptr_t, uintptr_t)>(SA_ADDR(0x3499C4) + _dyld_get_image_vmaddr_slide(0))(thiz, pPed);

        *pbyteCameraMode = byteSavedCameraMode;
        *wCameraMode2 = wSavedCameraMode2;

        CCameraAim::GameSetLocalPlayerSkills();

        CCameraAim::GameSetLocalPlayerCameraExtZoom();

        CWorld::PlayerInFocus = 0;

        CPad::SwitchOnFootContextToLocalPlayer();
        CCameraAim::GameSetLocalPlayerAim();
    }
    else
    {
        pedPositionResult = reinterpret_cast<uint32_t (*)(uintptr_t, uintptr_t)>(SA_ADDR(0x3499C4) + _dyld_get_image_vmaddr_slide(0))(thiz, pPed);
    }

    return pedPositionResult;
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_TaskUseGun_hook(uintptr_t thiz, uintptr_t pPed)
{
    return CPad::TaskUseGun(thiz, pPed);
}

extern "C" __attribute__((visibility("default"))) void CCivilianPed_ProcessControl_hook(CPedGta* pPed)
{
    if (!pedMethodHooksInstalled)
    {
        reinterpret_cast<void (*)(CPedGta*)>(SA_ADDR(0x23B32C) + _dyld_get_image_vmaddr_slide(0))(pPed);
        return;
    }

    if ((pPed->m_nPhysicalFlags & 0x40000) != 0 && (pPed->m_nPhysicalFlags & 0x800000) != 0 && (pPed->m_nPhysicalFlags & 0x200000) != 0)
    {
        pPed->vecMoveSpeed.x = 0.0f;
        pPed->vecMoveSpeed.y = 0.0f;
    }

    if (pPed->mat)
    {
        matSavedMatrix = pPed->mat;
        vecSavedMoveSpeed.x = pPed->vecMoveSpeed.x;
        vecSavedMoveSpeed.y = pPed->vecMoveSpeed.y;
        vecSavedMoveSpeed.z = pPed->vecMoveSpeed.z;
    }

    reinterpret_cast<void (*)(CPedGta*)>(SA_ADDR(0x23B32C) + _dyld_get_image_vmaddr_slide(0))(pPed);

    if ((pPed->m_nPhysicalFlags & 0x40000) != 0 && (pPed->m_nPhysicalFlags & 0x800000) != 0 && (pPed->m_nPhysicalFlags & 0x200000) != 0)
    {
        if (pPed->mat)
        {
            pPed->mat->pos.x = matSavedMatrix->pos.x;
            pPed->mat->pos.y = matSavedMatrix->pos.y;
            pPed->mat->pos.z = matSavedMatrix->pos.z;
        }

        pPed->vecMoveSpeed.x = 0.0f;
        pPed->vecMoveSpeed.y = 0.0f;
    }
}

extern "C" __attribute__((visibility("default"))) void CPlayerPed_ProcessCollision_hook(CPhysicalGta* thiz)
{
    if (!thiz) {
        return;
    }

    if (!pedMethodHooksInstalled || ((thiz->m_nEntityFlags >> 31) != 1))
    {
        reinterpret_cast<void (*)(CPhysicalGta*)>(SA_ADDR(0x162974) + _dyld_get_image_vmaddr_slide(0))(thiz);
    }
}

bool CPad::isLocalPlayerSprinting()
{
    CPedGta* pPed = CUtil::GamePool_FindPlayerPed();
    if (pPed)
    {
        if (!pPed->m_pPlayerData) {
            return false;
        }

        float fSprintCounter = *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(pPed->m_pPlayerData) + 0x28);
        if (fSprintCounter <= 0.0f) {
            return false;
        }

        eMoveState MoveState = pPed->m_eMoveState;
        if (!MoveState) {
            return false;
        }

        CVector vec = CVector();
        if (CGame::GetPlayer())
        {
            CGame::GetPlayer()->GetMoveSpeedVector(&vec);
        }

        float magnitude = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
        if (magnitude >= 0.11f)
        {
            return true;
        }
    }

    return false;
}

extern "C" __attribute__((visibility("default"))) int16_t CPad_GetPedWalkLeftRight_hook(CPad* thiz, int16_t (*CPad_GetPedWalkLeftRight)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_GetPedWalkLeftRight(thiz);
    }

    return CPad::GetPedWalk(thiz, eJoystickKeys::KEY_LR, CPad_GetPedWalkLeftRight);
}

extern "C" __attribute__((visibility("default"))) int16_t CPad_GetPedWalkUpDown_hook(CPad* thiz, int16_t (*CPad_GetPedWalkUpDown)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_GetPedWalkUpDown(thiz);
    }

    return CPad::GetPedWalk(thiz, eJoystickKeys::KEY_UD, CPad_GetPedWalkUpDown);
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_DuckJustDown_hook(CPad* thiz, uintptr_t ped, uint32_t (*CPad_DuckJustDown)(CPad*, uintptr_t))
{
    if (!padProcessControlInstalled) {
        return CPad_DuckJustDown(thiz, ped);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		return 0;
	}
	else 
	{
		if (CPad::IsActiveWidgets()) {
			return 0;
		}

		if (CMultiPlayer::GetActiveNetwork())
		{
			if (CGame::GetPlayer() && CGame::GetPlayer()->m_bCrouchControl)
			{
				return 0;
			}
		}

		return CPad_DuckJustDown(thiz, ped);
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_JumpJustDown_hook(CPad* thiz, uint32_t (*CPad_JumpJustDown)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_JumpJustDown(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		if (!CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bIgnoreJump && CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_JUMP] && !CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_HANDBRAKE])
		{
			CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bIgnoreJump = true;
			return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_JUMP];
		}

		return 0;
	}
	else
	{
		if (CPad::IsActiveWidgets()) {
			return 0;
		}

		if (CMultiPlayer::GetActiveNetwork())
		{
			if (CGame::GetPlayer() && CGame::GetPlayer()->m_bJumpControl)
			{
				return 0;
			}
		}

		uint32_t dwResult = CPad_JumpJustDown(thiz);
		JumpJustDown_VAR = dwResult;

		if (dwResult)
		{
			m_dwJumpTickCounter = CNecessary::GetTickCount() + 300;
		}

		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT] = CPad::isLocalPlayerSprinting();
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = dwResult;

		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetJump_hook(CPad* thiz, uint32_t (*CPad_GetJump)(CPad*), uint32_t (*CPad_JumpJustDown)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_GetJump(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		if (CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bIgnoreJump) {
			return 0;
		}

		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_JUMP];
	}
	else
	{
		if (CPad::IsActiveWidgets()) {
			return 0;
		}

		if (CMultiPlayer::GetActiveNetwork())
		{
			if (CGame::GetPlayer() && CGame::GetPlayer()->m_bJumpControl)
			{
				return 0;
			}
		}

		uint32_t dwResult = CPad_JumpJustDown(thiz);
		JumpJustDown_VAR = dwResult == 0x1 ? 1 : 0;

		if (dwResult)
		{
			m_dwJumpTickCounter = CNecessary::GetTickCount() + 300;
		}

		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT] = CPad::isLocalPlayerSprinting();
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = dwResult;
		
		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetSprint_hook(CPad* thiz, int nSprintType, uint32_t (*CPad_GetSprint)(CPad*, int))
{
    if (!padProcessControlInstalled) {
        return CPad_GetSprint(thiz, nSprintType);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_SPRINT];
	}
	else
	{
		if (CPad::IsActiveWidgets()) {
			return 0;
		}

		if (CMultiPlayer::GetActiveNetwork())
		{
			if (CGame::GetPlayer())
			{
				if (CGame::GetPlayer()->m_bSprintControl)
				{
					return 0;
				}

				if (!CGame::GetPlayer()->IsInWater())
				{
					if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] || JumpJustDown_VAR > 0)
					{
						return 0;
					}
				}
			}
		}

		uint32_t dwResult = CPad_GetSprint(thiz, nSprintType);
		CPad::SprintGet = dwResult ? 1 : 0;
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT] = dwResult;
		return CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT];
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_SwimJumpJustDown_hook(CPad* thiz, uint32_t (*CPad_SwimJumpJustDown)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_SwimJumpJustDown(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_JUMP];
	}
	else
	{
		if (CMultiPlayer::GetActiveNetwork())
		{
			if (CGame::GetPlayer() && CGame::GetPlayer()->m_bJumpControl)
			{
				return 0;
			}
		}

		uint32_t dwResult = CPad_SwimJumpJustDown(thiz);
		SwimJumpJustDown_VAR = dwResult ? 1 : 0;

		if (dwResult)
		{
			m_dwJumpTickCounter = CNecessary::GetTickCount() + 300;
		}

		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = dwResult;

		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_MeleeAttackJustDown_hook(CPad* thiz, uint32_t (*CPad_MeleeAttackJustDown)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_MeleeAttackJustDown(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		if (CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_HANDBRAKE] && CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_SECONDARY_ATTACK])
		{
			return 2;
		}

		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_FIRE];
	} 
	else 
	{
		if (CMultiPlayer::GetActiveNetwork())
		{
			// เดียวมาทำภายหลัง
			// if (CGame::GetPlayer()->IsPissing() || CGame::GetPlayer()->HasHandsUp() || CGame::GetPlayer()->IsDancing())
			// {
			// 	if (CTouchInterface::IsReleased(1, 0, 1))
			// 	{
			// 		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true;
			// 	}

			// 	return 0;
			// }

			// if (CGui::GetObjectEdit())
			// {
			// 	if (CGui::GetObjectEdit()->GetActiveObjectEdit())
			// 	{
			// 		return 0;
			// 	}
			// }
		}

		uint32_t dwResult = CPad_MeleeAttackJustDown(thiz);
		CPad::m_iSaveMeleeAttack = dwResult;

		if (dwResult == 2)
		{
			m_dwMeleeAttackTickCounter = CNecessary::GetTickCount() + 300;
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = true;
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true;

			return dwResult; // 2 เปิดใช้หมัด F หากเป็น 0 จะปิดหมัด F สำหรับตัวเรา
		}
		
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = dwResult;
		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetAutoClimb_hook(CPad* thiz, uint32_t (*CPad_GetAutoClimb)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_GetAutoClimb(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER) 
	{
		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_JUMP];
	} 
	else 
	{
		uint32_t dwResult = CPad_GetAutoClimb(thiz);
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = dwResult;

		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetAbortClimb_hook(CPad* thiz, uint32_t (*CPad_GetAbortClimb)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_GetAbortClimb(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_SECONDARY_ATTACK];
	} 
	else 
	{
		uint32_t dwResult = CPad_GetAbortClimb(thiz);
		if (dwResult)
		{
			m_dwAbortClimbTickCounter = CNecessary::GetTickCount() + 300;
		}

		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = dwResult;
		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_DiveJustDown_hook(CPad* thiz, uint32_t (*CPad_DiveJustDown)(CPad*))
{
    if (!padProcessControlInstalled) {
        return CPad_DiveJustDown(thiz);
    }

	if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
	{
		return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_FIRE];
	} 
	else 
	{
		uint32_t dwResult = CPad_DiveJustDown(thiz);
		if (dwResult)
		{
			m_dwDiveTickCounter = CNecessary::GetTickCount() + 300;
		}

		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = dwResult;
		return dwResult;
	}
}

extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetBlock_hook(uintptr_t thiz, uint32_t (*CPad_GetBlock)(uintptr_t))
{
    if (!padProcessControlInstalled) {
        return CPad_GetBlock(thiz);
    }

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        if (CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_JUMP] && CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_HANDBRAKE])
        {
            return 1;
        }

        return 0;
    }
    else
    {
		// เดียวมาทำต่อภายหลัง
		// if (CGui::GetObjectEdit())
		// {
		// 	if (CGui::GetObjectEdit()->GetActiveObjectEdit())
		// 	{
		// 		return 0;
		// 	}
		// }

        bool isBlocking = CPad_GetBlock(thiz);
        CPad::m_iSaveBlockPunch = isBlocking;

        if (CTouchInterface::IsTouched(8, nullptr, 1))
        {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = true;
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = true;
            CPad::m_iSaveBlockPunch = true;
            isBlocking = true;
        }
    
        return CPad::m_iSaveBlockPunch;
    }
}

extern "C" __attribute__((visibility("default"))) float CPad_AimWeaponLeftRight_hook(uintptr_t thiz, CPedGta* pPed, bool *pbTouchInputX, float (*CPad_AimWeaponLeftRight)(uintptr_t, CPedGta*, bool*))
{
	if (!pthread_main_np() || !CMultiPlayer::GetActiveNetwork() || CTimer::IsUserPause()) {
		return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
	}

	if (CGyro::IsGyro())
	{
		float tmp = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
		float aimSens = CGyro::GetAimSensitivity() / 100.0f;
		float onfootSens = CGyro::GetOnfootSensitivity() / 100.0f;
		float inCarSens = CGyro::GetIncarSensitivity() / 100.0f;

		float gyroY = CGyro::GetGyroY();
		if (std::fabs(gyroY) < 0.015f) 
		{
			gyroY = 0.0f; // โซนไร้การสั่นสะเทือนเพื่อกรองการสั่นสะเทือนเล็กน้อย
		}

		const double elapsedTime = CGyro::GetElapsedTime();

		float gyroOffset;
		if (CGyro::GetGyroMode() == eGyroMode::GYRO_ALL_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * aimSens;
				return (tmp + gyroOffset) * aimSens;
			}

			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return tmp + gyroOffset;
				}
				else
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return (tmp * (CSensitivity::GetIncarTouch() / 100.0f)) + gyroOffset;
				}
			}
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_AIM_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * aimSens;
				return (tmp + gyroOffset) * aimSens;
			}

			return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_ONFOOT_AND_INCAR_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				float aimInput = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
				return aimInput * (CSensitivity::GetAimTouch() / 100.0f);
			}
			
			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return tmp + gyroOffset;
				}
				else
				{
					gyroOffset = gyroY * -1000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return (tmp * (CSensitivity::GetIncarTouch() / 100.0f)) + gyroOffset;
				}
			}
		}
	}
	else
	{
		if (CCameraController::IsCameraAllAiming(false, false))
		{
			float tmp = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
			return tmp * (CSensitivity::GetAimTouch() / 100.0f);
		}

		if (CGame::GetPlayer() && CGame::GetPlayer()->IsInVehicle())
		{
			float tmp = CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
			return tmp * (CSensitivity::GetIncarTouch() / 100.0f);
		}

		return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
	}

	return CPad_AimWeaponLeftRight(thiz, pPed, pbTouchInputX);
}

extern "C" __attribute__((visibility("default"))) int16_t CPad_AimWeaponUpDown_hook(uintptr_t thiz, CPedGta* pPed, bool *pbTouchInputX, int16_t (*CPad_AimWeaponUpDown)(uintptr_t, CPedGta*, bool*))
{
	if (!pthread_main_np() || !CMultiPlayer::GetActiveNetwork() || CTimer::IsUserPause()) {
		return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
	}

	const auto clampInput = [](float value)
	{
		return static_cast<int16_t>(std::clamp(value, static_cast<float>(INT16_MIN), static_cast<float>(INT16_MAX)));
	};

	if (CGyro::IsGyro())
	{
		int16_t tmp = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
		float aimSens = CGyro::GetAimSensitivity() / 100.0f;
		float onfootSens = CGyro::GetOnfootSensitivity() / 100.0f;
		float inCarSens = CGyro::GetIncarSensitivity() / 100.0f;

		float gyroX = CGyro::GetGyroX();
		if (std::fabs(gyroX) < 0.015f)
		{
			gyroX = 0.0f; // โซนไร้การสั่นสะเทือนเพื่อกรองการสั่นสะเทือนเล็กน้อย
		}

		const double elapsedTime = CGyro::GetElapsedTime();

		float gyroOffset;
		if (CGyro::GetGyroMode() == eGyroMode::GYRO_ALL_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * aimSens;
				float scaled = (static_cast<float>(tmp) + gyroOffset) * aimSens;
				return clampInput(scaled);
			}

			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return clampInput(static_cast<float>(tmp) + gyroOffset);
				}
				else
				{
					gyroOffset = gyroX * 3000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return clampInput(static_cast<float>(tmp) * (CSensitivity::GetIncarTouch() / 100.0f) + gyroOffset);
				}
			}
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_AIM_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * aimSens;
				float scaled = (static_cast<float>(tmp) + gyroOffset) * aimSens;
				return clampInput(scaled);
			}

			return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
		}
		else if (CGyro::GetGyroMode() == eGyroMode::GYRO_ONFOOT_AND_INCAR_MODE)
		{
			if (CCameraController::IsCameraAllAiming(false, false))
			{
				int16_t aimInput = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
				float scaled = static_cast<float>(aimInput) * (CSensitivity::GetAimTouch() / 100.0f);

				return clampInput(scaled);
			}

			if (CGame::GetPlayer())
			{
				if (!CGame::GetPlayer()->IsInVehicle())
				{
					gyroOffset = gyroX * 1000.0f * static_cast<float>(elapsedTime) * onfootSens;
					return clampInput(static_cast<float>(tmp) + gyroOffset);
				}
				else
				{
					gyroOffset = gyroX * 3000.0f * static_cast<float>(elapsedTime) * inCarSens;
					return clampInput(static_cast<float>(tmp) * (CSensitivity::GetIncarTouch() / 100.0f) + gyroOffset);
				}
			}
		}
	}
	else
	{
		if (CCameraController::IsCameraAllAiming(false, false))
		{
			int16_t tmp = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
			float scaled = static_cast<float>(tmp) * (CSensitivity::GetAimTouch() / 100.0f);

			return clampInput(scaled);
		}

		if (CGame::GetPlayer() && CGame::GetPlayer()->IsInVehicle())
		{
			int16_t tmp = CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
			float scaled = static_cast<float>(tmp) * (CSensitivity::GetIncarTouch() / 100.0f);

			return clampInput(scaled);
		}

		return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
	}

	return CPad_AimWeaponUpDown(thiz, pPed, pbTouchInputX);
}

extern "C" __attribute__((visibility("default"))) void CPed_ProcessControl_hook(uintptr_t thiz, void (*CPed_ProcessControl)(uintptr_t))
{
    if (!padProcessControlInstalled)
    {
        CPed_ProcessControl(thiz);
        return;
    }

	byteInternalPlayer = CWorld::PlayerInFocus;
	CPad::m_byteCurPlayer = CPedPointerRecorder::FindPlayerNumFromPointer(thiz);

	if (thiz && CPad::m_byteCurPlayer != 0 && byteInternalPlayer == 0)
	{
		if (CPad::m_byteCurPlayer >= eMaxItem::PLAYER_PED_SLOTS)
		{
			CPad::SwitchOnFootContextToLocalPlayer();
		    CPad::LocalProcessControl(thiz, CPed_ProcessControl);
		    return;
		}

		CPad::SwitchOnFootContextToRemotePlayer(CPad::m_byteCurPlayer);
		CPad::RemoteProcessControl(thiz, CPed_ProcessControl);
	}
	else
	{
		CPlayerPed* pPlayer = CGame::GetPlayer();
		if (pPlayer)
		{
			// เดียวมาทำภายหลัง
			// if (!pPlayer->IsInVehicle() && !pPlayer->IsInJetpack())
			// {
			// 	if (pPlayer->IsPissing() || pPlayer->HasHandsUp() || pPlayer->IsDancing())
			// 	{
			// 		if (CTouchInterface::IsReleased(1, 0, 1))
			// 		{
			// 			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true;
			// 		}

			// 		// if (pPlayer->IsDancing())
			// 		// {
			// 		// 	if (CTouchInterface::IsReleased(13, 0, 1))
			// 		// 	{
			// 		// 		CPad::LocalPlayerKeys.wKeyUD = 255;
			// 		// 	}
			// 		// }
			// 	}

			// 	if (pPlayer->GetStuff() != eStuffType::STUFF_TYPE_NONE && !pPlayer->IsPlayingAnimation(OBFUSCATE("DNK_STNDF_LOOP")) && !pPlayer->IsPlayingAnimation(OBFUSCATE("DNK_STNDM_LOOP")) && !pPlayer->IsPlayingAnimation(OBFUSCATE("smkcig_prtl")) && !CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] && JumpJustDown_VAR == 0)
			// 	{
			// 		if (CTouchInterface::IsReleased(1, 0, 1))
			// 		{
			// 			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;
			// 		}
			// 	}

			// 	if (CGame::GetPlayer()->IsInWater() && !CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] && SwimJumpJustDown_VAR == 0)
			// 	{
			// 		if (CTouchInterface::IsReleased(1, 0, 1))
			// 		{
			// 			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;
			// 		}
			// 	}
			// }
			
			// if (!pPlayer->IsInVehicle() && !pPlayer->IsDead())
			// {
			// 	if (CTouchInterface::IsReleased(18, 0, 1))
			// 	{
			// 		if (CFirstPerson::GetActiveFirstPerson())
			// 		{
			// 			CFirstPerson::SetFirstPerson(false);
			// 			*reinterpret_cast<int32_t*>(SA_ADDR(0x9F86F8 + 0xCC)) = 3; // TheCamera->m_nPedZoom
			// 		}
			// 		else
			// 		{
			// 			int32_t currentZoom = *reinterpret_cast<int32_t*>(SA_ADDR(0x9F86F8 + 0xCC));
			// 			if (currentZoom == 1)
			// 			{
			// 				CFirstPerson::SetFirstPerson(true);
			// 			}
			// 			else if (currentZoom == 3)
			// 			{
			// 				*reinterpret_cast<int32_t*>(SA_ADDR(0x9F86F8 + 0xCC)) = 2;
			// 			}
			// 			else if (currentZoom == 2)
			// 			{
			// 				*reinterpret_cast<int32_t*>(SA_ADDR(0x9F86F8 + 0xCC)) = 1;
			// 			}
			// 			else
			// 			{
			// 				*reinterpret_cast<int32_t*>(SA_ADDR(0x9F86F8 + 0xCC)) = 3;
			// 			}
			// 		}
			// 	}
			// }

			// 	eMoveState MoveState = pPed->m_eMoveState;
			// 	if (MoveState == eMoveState::PEDMOVE_WALK || MoveState == eMoveState::PEDMOVE_JOG || MoveState == eMoveState::PEDMOVE_RUN || MoveState == eMoveState::PEDMOVE_SPRINT || MoveState == eMoveState::PEDMOVE_TURN_L || MoveState == eMoveState::PEDMOVE_TURN_R)
			// 	{
			// 		if (CGui::GetPlayerInteract())
			// 		{
			// 			CGui::GetPlayerInteract()->ResetTouch();
			// 		}

			// 		if (CGui::GetVehicleInteract())
			// 		{
			// 			CGui::GetVehicleInteract()->ResetTouch();
			// 		}
			// 	}
			// }
		}

		CPad::SwitchOnFootContextToLocalPlayer();
		CPad::LocalProcessControl(thiz, CPed_ProcessControl);
	}
}

uint32_t (*CPad_GetWeapon)(uintptr_t thiz, CPedGta* pPed, bool bShoulderPossible);
extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetWeapon_hook(uintptr_t thiz, CPedGta* pPed, bool bShoulderPossible, uint32_t (*original)(uintptr_t, CPedGta*, bool))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz, pPed, bShoulderPossible);
    }

    CPad_GetWeapon = original;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_FIRE] ? 0xFF : 0x00;
    }
    else
    {
        // // Quick Aim
        // if (!CPad::m_iSaveTargetingActive && CMultiPlayer::GetActiveNetwork() && CGame::GetPlayer())
        // {
        //     uint8_t currentWeapon = CGame::GetPlayer()->GetCurrentWeapon();
        //     if (currentWeapon != eWeaponSampId::WEAPON_GRENADE && currentWeapon != eWeaponSampId::WEAPON_TEARGAS && currentWeapon != eWeaponSampId::WEAPON_MOLTOV && !(currentWeapon >= eWeaponSampId::WEAPON_SATCHEL && currentWeapon < eWeaponSampId::WEAPON_FIREEXTINGUISHER))
        //     {
        //         if (CTouchInterface::IsTouched(0x15, 0, 1))
        //         {
        //             uint8_t old = CWorld::PlayerInFocus;
        //             CWorld::PlayerInFocus = 0;
        //             CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = 255;
        //             CWorld::PlayerInFocus = old;
        //             return CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE];
        //         }
        //     }
        // }

        uint8_t old = CWorld::PlayerInFocus;
        CWorld::PlayerInFocus = CPad::m_byteCurPlayer;
        CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = CPad_GetWeapon(thiz, pPed, bShoulderPossible);
        CWorld::PlayerInFocus = old;
        return CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE];
    }
}

uint32_t (*CPad_WeaponJustDown)(uintptr_t thiz, uintptr_t ped, bool bShoulderPossible, bool bEnableTouch);
extern "C" __attribute__((visibility("default"))) uint32_t CPad_WeaponJustDown_hook(uintptr_t thiz, uintptr_t ped, bool bShoulderPossible, bool bEnableTouch, uint32_t (*original)(uintptr_t, uintptr_t, bool, bool))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz, ped, bShoulderPossible, bEnableTouch);
    }

    CPad_WeaponJustDown = original;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_FIRE];
    }
    else
    {
        return CPad_WeaponJustDown(thiz, ped, bShoulderPossible, bEnableTouch);
    }
}

uint32_t (*CCamera_IsTargetingActive)(uintptr_t thiz, CPedGta* pPed);
extern "C" __attribute__((visibility("default"))) uint32_t CCamera_IsTargetingActive_hook(uintptr_t thiz, CPedGta* pPed, uint32_t (*original)(uintptr_t, CPedGta*))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz, pPed);
    }

    CCamera_IsTargetingActive = original;

    if (pPed != CUtil::GamePool_FindPlayerPed())
    {
        return CPad::RemotePlayerKeys[CPad::m_byteCurPlayer].bKeys[ePadKeys::KEY_HANDBRAKE] ? 1 : 0;
    }
    else
    {
        if (CGui::GetScoreBoard()->GetActiveScore() /*|| CGui::GetMainMenuScreens()->GetActiveMainMenuScreens()*/ || CGui::GetDialog()->GetActiveDialog() /*|| CGui::GetVoiceMenu()->GetActiveVoiceMenu() || CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
            *wCameraMode2 = 0;
            return 0;
        }

        if (CMultiPlayer::GetActiveNetwork())
        {
            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
            {
                if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
                {
                    CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
                    *wCameraMode2 = 0;
                    return 0;
                }
            }

            // if (CGui::GetObjectEdit())
            // {
                // if (CGui::GetObjectEdit()->GetActiveObjectEdit())
                // {
                    // CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
                    // *wCameraMode2 = 0;
                    // return 0;
                // }
            // }
        }

        if (CPad::m_iSaveBlockPunch) {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
            return 0;
        }

        uint32_t result = reinterpret_cast<uint32_t (*)(uintptr_t)>(SA_ADDR(0x133670) + _dyld_get_image_vmaddr_slide(0))(SA_ADDR(0x72CF08) + _dyld_get_image_vmaddr_slide(0));
        CPad::m_iSaveTargetingActive = result;
        CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = result;
        return result;
    }
}

uint32_t (*CPad_GetEnterTargeting)(uintptr_t thiz);
extern "C" __attribute__((visibility("default"))) uint32_t CPad_GetEnterTargeting_hook(uintptr_t thiz, uint32_t (*original)(uintptr_t))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz);
    }

    CPad_GetEnterTargeting = original;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        return 0;
    }
    else
    {
        if (CGui::GetScoreBoard()->GetActiveScore() /*|| CGui::GetMainMenuScreens()->GetActiveMainMenuScreens()*/ || CGui::GetDialog()->GetActiveDialog() /*|| CGui::GetVoiceMenu()->GetActiveVoiceMenu() || CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
            *wCameraMode2 = 0;
            return 0;
        }

        if (CMultiPlayer::GetActiveNetwork())
        {
            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
            {
                if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
                {
                    CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
                    *wCameraMode2 = 0;
                    return 0;
                }
            }

            // if (CGui::GetObjectEdit())
            // {
                // if (CGui::GetObjectEdit()->GetActiveObjectEdit())
                // {
                    // CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
                    // *wCameraMode2 = 0;
                    // return 0;
                // }
            // }
        }

        if (CPad::m_iSaveBlockPunch) {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
            return 0;
        }

        uintptr_t result = CPad_GetEnterTargeting(thiz);
        CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = result;
        return result;
    }
}

bool (*CPad_GetTarget)(uintptr_t *thiz, bool bCheckWidget);
extern "C" __attribute__((visibility("default"))) bool CPad_GetTarget_hook(uintptr_t *thiz, bool bCheckWidget, bool (*original)(uintptr_t*, bool))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz, bCheckWidget);
    }

    CPad_GetTarget = original;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        if (CPad::dwCurWeaponProcessingPlayer) {
            return CPad::dwCurWeaponProcessingPlayer->bIsAimingGun;
        }

        return false;
    }

    return CPad_GetTarget(thiz, bCheckWidget);
}

uint32_t (*CPad_CycleWeaponLeftJustDown)(uintptr_t thiz);
extern "C" __attribute__((visibility("default"))) uint32_t CPad_CycleWeaponLeftJustDown_hook(uintptr_t thiz, uint32_t (*original)(uintptr_t))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz);
    }

    CPad_CycleWeaponLeftJustDown = original;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        return 0;
    }
    else
    {
        CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_LEFT] = CPad_CycleWeaponLeftJustDown(thiz);
        return CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_LEFT];
    }
}

uint32_t (*CPad_CycleWeaponRightJustDown)(uintptr_t thiz);
extern "C" __attribute__((visibility("default"))) uint32_t CPad_CycleWeaponRightJustDown_hook(uintptr_t thiz, uint32_t (*original)(uintptr_t))
{
    if (!weaponHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz);
    }

    CPad_CycleWeaponRightJustDown = original;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        return 0;
    }
    else
    {
        if (CTouchInterface::IsReleased(29, 0, 1))
        {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_RIGHT] = true;
            return CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_RIGHT];
        }

        CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_RIGHT] = CPad_CycleWeaponRightJustDown(thiz);
        return CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_RIGHT];
    }
}

uint32_t (*CPed_GetWeaponSkill)(CPedGta* thiz);
extern "C" __attribute__((visibility("default"))) uint32_t CPed_GetWeaponSkill_hook(CPedGta* thiz, uint32_t (*original)(CPedGta*))
{
    if (!weaponSkillHookInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz);
    }

    CPed_GetWeaponSkill = original;

    uintptr_t calledFrom = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
    calledFrom -= SA_ADDR(0x0) + _dyld_get_image_vmaddr_slide(0);

    if (calledFrom == 0x316D40)
    {
        static bool bWeaponSkillsStored = false;
        byteInternalPlayer = CWorld::PlayerInFocus;
        CPad::m_byteCurPlayer = CPedPointerRecorder::FindPlayerNumFromPointer((uintptr_t)thiz);

        if (thiz && CPad::m_byteCurPlayer != 0 && byteInternalPlayer == 0)
        {
            CCameraAim::GameStoreLocalPlayerSkills();
            CCameraAim::GameSetRemotePlayerSkills(CPad::m_byteCurPlayer);
            bWeaponSkillsStored = true;
        }

        uint32_t result = reinterpret_cast<uint32_t (*)(CPedGta*, uint32_t)>(SA_ADDR(0x2438E0) + _dyld_get_image_vmaddr_slide(0))(thiz, thiz->WeaponSlots[thiz->byteCurWeaponSlot].dwType);

        if (bWeaponSkillsStored)
        {
            CCameraAim::GameSetLocalPlayerSkills();
            bWeaponSkillsStored = false;
        }

        return result;
    }

    return CPed_GetWeaponSkill(thiz);
}

void CPad::Initialise()
{
    CNecessary::Log(OBFUSCATE("CPad Initialise"));

    padProcessControlInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x26E010), "\xA5\xA5\x08\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x23EE00), "\x2F\x62\x09\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x3C1350), "\xE1\x58\x03\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x3C1020), "\xB5\x59\x03\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x3C1274), "\x28\x59\x03\x14");

	pedMethodHooksInstalled = CMemory::InstallMethodHook(SA_ADDR(0x4A7150), (uintptr_t)CCivilianPed_ProcessControl_hook);
    pedMethodHooksInstalled = CMemory::InstallMethodHook(SA_ADDR(0x4A83D8), (uintptr_t)CPlayerPed_ProcessCollision_hook) && pedMethodHooksInstalled;

    taskUseGunHookInstalled = CMemory::InstallMethodHook(SA_ADDR(0x4A9CB0), reinterpret_cast<uintptr_t>(CPad_TaskUseGun_hook));

    // สลับ WeaponSkill ของผู้เล่นอื่นเฉพาะตอนวาดอาวุธ ตาม Thunder
    weaponSkillHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x316D3C), "\xA7\x03\x06\x94") && CMemory::HasMappedBytes(SA_ADDR(0x23E7A0), "\xE9\x23\xBC\x6D\xF6\x57\x01\xA9", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x2438E0), "\xE9\x23\xBC\x6D\xF6\x57\x01\xA9", 8, "__text");

    // สำหรับ ONFOOT
    padProcessControlInstalled = padProcessControlInstalled && CMemory::HasMappedInstruction(SA_ADDR(0x15CE14), "\x48\xEA\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15CF34), "\x06\xEA\x0C\x14");
    padProcessControlInstalled = padProcessControlInstalled && CMemory::HasMappedInstruction(SA_ADDR(0x15ED84), "\x78\xE2\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15F0A0), "\xB7\xE1\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15F030), "\xDF\xE1\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15F24C), "\x52\xE1\x0C\x14");
    padProcessControlInstalled = padProcessControlInstalled && CMemory::HasMappedInstruction(SA_ADDR(0x15F1AC), "\x87\xE1\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15E034), "\xEB\xE5\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15EE4C), "\x6B\xE2\x0C\x14");
    padProcessControlInstalled = padProcessControlInstalled && CMemory::HasMappedInstruction(SA_ADDR(0x15EF1C), "\x3D\xE2\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15F124), "\xC1\xE1\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15E3E4), "\x17\xE5\x0C\x14");
    if (padProcessControlInstalled)
    {
        CPad_AllowAimingRotation = 0;
    }

	// สำหรับ Weapon
    weaponHooksInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x15DB74), "\xE8\xE7\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15DE3C), "\x3C\xE7\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x13A780), "\xF1\x74\x0D\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15E610), "\x54\xE5\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15E854), "\xC9\xE4\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15E2C4), "\x33\xE6\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x15E354), "\x15\xE6\x0C\x14") && CMemory::HasMappedBytes(SA_ADDR(0x133670), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text");
}

void CPad::DisplayWidgets(bool bDisp)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x15B6F8), "\xC8\x2E\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x15B6FC), "\x08\xF1\x28\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x43AE90), "\x08\x20\x42\x79");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    *reinterpret_cast<std::uint16_t*>(SA_ADDR(0x734A3C + 0x110) + slide) = static_cast<std::uint16_t>(!bDisp); // Pads[0].DisablePlayerControls
}

bool CPad::IsActiveWidgets()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x15B6F8), "\xC8\x2E\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x15B6FC), "\x08\xF1\x28\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x43AE90), "\x08\x20\x42\x79");
    return verified && *reinterpret_cast<const std::uint16_t*>(SA_ADDR(0x734A3C + 0x110) + _dyld_get_image_vmaddr_slide(0)) != 0;
}