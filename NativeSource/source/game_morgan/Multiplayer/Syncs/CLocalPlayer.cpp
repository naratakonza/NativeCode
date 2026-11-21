#include "../../../../Main.h"
#include "Core/CCheckPoint.h"
#include "Core/CHeadMove.h"
#include "CLocalPlayer.h"
#include "../../CGame.h"
#include "../../CCamera.h"
#include "../../CCameraAim.h"
#include "../../CRadar.h"
#include "../../CTheZones.h"
#include "../CMultiPlayer.h"
#include "../Audio/CAudio.h"
#include "../Guis/CGui.h"
#include "../../Base/CPad.h"
#include "../../Base/CWidgets.h"
#include "../../Base/Graphics/CRealTimeShadowManager.h"
#include "../../Scripts/CScripting.h"

uint32_t dwEnterVehTimeElasped = -1;
bool bFirstSpawn = true;

CLocalPlayer::CLocalPlayer()
{
    CNecessary::Log(OBFUSCATE("CLocalPlayer Initialise"));

    CLocalPlayer::m_pPlayerPed = CGame::GetPlayer();

	CLocalPlayer::m_bInRCMode = false;
	CLocalPlayer::m_bIsActive = false;
	CLocalPlayer::m_bIsWasted = false;
	CLocalPlayer::m_bIsSpectating = false;
	CLocalPlayer::m_iDisplayZoneTick = 0;
	CLocalPlayer::m_byteSpectateType = SPECTATE_TYPE_NONE;
	CLocalPlayer::m_SpectateID = 0xFFFFFFFF;
	CLocalPlayer::m_bSpawnDialogShowed = false;

	// CLocalPlayer::m_surfData.bIsActive = false;
	// CLocalPlayer::m_surfData.lastUpdate = CNecessary::GetTickCount();
	// CLocalPlayer::m_surfData.iSyncMode = eSurfingMode::SURFING_MODE_NONE;

	CLocalPlayer::m_dwLastAnySendTick = CNecessary::GetTickCount();
	CLocalPlayer::m_dwLastSendTick = CNecessary::GetTickCount();
	CLocalPlayer::m_dwLastSendAimTick = CNecessary::GetTickCount();
	CLocalPlayer::m_dwLastSendSpecTick = CNecessary::GetTickCount();
	CLocalPlayer::m_dwLastStatsUpdateTick = CNecessary::GetTickCount();
	CLocalPlayer::m_dwLastUpdateModelTick = CNecessary::GetTickCount();
	CLocalPlayer::m_headData.m_dwLastHeadUpdate = CNecessary::GetTickCount();
	CLocalPlayer::m_headData.m_dwLastHeadLook = CNecessary::GetTickCount();

	CLocalPlayer::ResetData();
	CLocalPlayer::ApplySpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);

	CLocalPlayer::SetLocalScore(0);
	CLocalPlayer::SetLocalPing(0);

	for (uint8_t i = 0; i < 13; i++)
	{
		CLocalPlayer::m_weaponsData.m_byteLastWeapon[i] = 0;
		CLocalPlayer::m_weaponsData.m_dwLastAmmo[i] = 0;
	}

	CLocalPlayer::m_byteTeamId = NO_TEAM;

    // if (CGui::GetChat())
    // {
    //     CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CLocalPlayer Initialise"));
    // }
}

CLocalPlayer::~CLocalPlayer()
{
    CNecessary::Log(OBFUSCATE("CLocalPlayer Uninitialise"));

	CLocalPlayer::ApplySpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);
	CLocalPlayer::ResetData();
	CLocalPlayer::m_pPlayerPed = nullptr;
}

void CLocalPlayer::ResetData()
{
	CLocalPlayer::m_bHasSpawnInfo = false;
	CLocalPlayer::m_bWaitingForSpawnRequestReply = false;
	CLocalPlayer::m_bEnableAfterDeath = false;
    CLocalPlayer::m_iSelectedClass = 0;
    CLocalPlayer::m_byteCurInterior = 0;
    CLocalPlayer::m_LastVehicle = eInvalidId::INVALID_VEHICLE_ID;
    CLocalPlayer::m_bInRCMode = false;
	CLocalPlayer::m_damageData.m_nVehicleUpdating = eInvalidId::INVALID_VEHICLE_ID;
	CLocalPlayer::m_cameraTargetData.m_nObject = eInvalidId::INVALID_OBJECT_ID;
	CLocalPlayer::m_cameraTargetData.m_nVehicle = eInvalidId::INVALID_VEHICLE_ID;
	CLocalPlayer::m_cameraTargetData.m_nPlayer = eInvalidId::INVALID_PLAYER_ID;
	CLocalPlayer::m_cameraTargetData.m_nActor = eInvalidId::INVALID_ACTOR_ID;
	CLocalPlayer::m_headData.m_vecHeadDir.x = 0.0f;
	CLocalPlayer::m_headData.m_vecHeadDir.y = 0.0f;
	CLocalPlayer::m_headData.m_vecHeadDir.z = 0.0f;
	CLocalPlayer::m_weaponsData.m_nAimedPlayer = eInvalidId::INVALID_PLAYER_ID;
	CLocalPlayer::m_weaponsData.m_nAimedActor = eInvalidId::INVALID_ACTOR_ID;
	CLocalPlayer::m_weaponsData.m_nCurrentWeapon = 0;

	memset(&m_OnFootData, 0, sizeof(ONFOOT_SYNC_DATA));
	// memset(&m_InCarData, 0, sizeof(INCAR_SYNC_DATA));
	// memset(&m_PassengerData, 0, sizeof(PASSENGER_SYNC_DATA));
	memset(&m_aimData, 0, sizeof(AIM_SYNC_DATA));
	// memset(&m_TrailerData, 0, sizeof(TRAILER_SYNC_DATA));
	// memset(&m_UnoccupiedData, 0, sizeof(UNOCCUPIED_SYNC_DATA));

	CLocalPlayer::m_dwAnimationValue = 0;
	CLocalPlayer::m_bAnimationReset = false;

	CLocalPlayer::m_dwLastWeaponsUpdateTick = CNecessary::GetTickCount();
	CLocalPlayer::m_dwLastCameraTargetUpdateTick = CNecessary::GetTickCount();

	// CLocalPlayer::m_surfData.pVehicle = nullptr;
	// CLocalPlayer::m_surfData.bIsStuck = false;
}

bool CLocalPlayer::Process()
{
	uint32_t dwThisTick = CNecessary::GetTickCount();

	assert(CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool() != (CPlayerPool*)nullptr);
	// assert(CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool() != (CVehiclePool*)nullptr);

    if (CLocalPlayer::m_bIsActive && CLocalPlayer::GetPlayerPed() && CLocalPlayer::GetPlayerPed()->VerifyInstance())
	{
        if ((!CLocalPlayer::m_bIsWasted && CLocalPlayer::GetPlayerPed()->GetActionTrigger() == eAction::ACTION_DEATH) || CLocalPlayer::GetPlayerPed()->IsDead())
		{
			CLocalPlayer::HandleDead();
			return true;
		}

        CLocalPlayer::HandleDancing();

		CLocalPlayer::GetPlayerPed()->ProcessCuffedControl();
		CLocalPlayer::GetPlayerPed()->ProcessCarryControl();

		CLocalPlayer::HandleUrinating();
		CLocalPlayer::HandleHandsUp();
		CLocalPlayer::HandleStuff();
        CLocalPlayer::DisplayZone(dwThisTick);

        CLocalPlayer::GetPlayerPed()->ProcessDrunk();
		CLocalPlayer::GetPlayerPed()->ResetPlayerTimeLastEaten();

		CLocalPlayer::ProcessAnimation();

		// CLocalPlayer::HandleVehicleEntryTimeout(dwThisTick);

		CCheckPoint::Process();

        CLocalPlayer::HandleInteriorChanging();
        CLocalPlayer::UpdateWeapons();
		// CLocalPlayer::UpdateCameraTarget();
		CLocalPlayer::SendStats(dwThisTick);

        if (CLocalPlayer::IsSpectating())
		{
			CLocalPlayer::ProcessSpectating();
			CLocalPlayer::m_bPassengerDriveByMode = false;
		}
		else if (CLocalPlayer::GetPlayerPed()->IsInVehicle() && !CLocalPlayer::GetPlayerPed()->IsAPassenger())
		{
			CLocalPlayer::ProcessDriver(dwThisTick);
		}
		else if (CLocalPlayer::GetPlayerPed()->GetActionTrigger() == eAction::ACTION_NORMAL || CLocalPlayer::GetPlayerPed()->GetActionTrigger() == eAction::ACTION_SCOPE)
		{
			CLocalPlayer::ProcessOnFoot(dwThisTick);
			CLocalPlayer::ProcessAimSync(dwThisTick);
		}
		else if (CLocalPlayer::GetPlayerPed()->IsInVehicle() && CLocalPlayer::GetPlayerPed()->IsAPassenger())
		{
			// CLocalPlayer::ProcessPassenger(dwThisTick);
		}
    }

	CLocalPlayer::HandleDrunkState();

    if (CLocalPlayer::IsSpectating() && !CLocalPlayer::m_bIsActive)
	{
		if (CLocalPlayer::IsSpawnDialog())
		{
			CLocalPlayer::m_bSpawnDialogShowed = false;

			if (CGui::GetSpawnScreen())
			{
				CGui::GetSpawnScreen()->Enable(false);
			}
		}

		CLocalPlayer::ProcessSpectating();
		return true;
	}

    CLocalPlayer::HandleAfterDeathToggle();

    if (CLocalPlayer::m_bIsWasted && CLocalPlayer::GetPlayerPed()->GetActionTrigger() != eAction::ACTION_WASTED && CLocalPlayer::GetPlayerPed()->GetActionTrigger() != eAction::ACTION_DEATH)
	{
		CLocalPlayer::HandleNeedsToRespawn();
		return true;
	}

	CLocalPlayer::HandleInitialSpawnConditions();

	return true;
}

void CLocalPlayer::HandleDrunkState()
{
    if (!CLocalPlayer::m_bIsActive)
    {
        if (CLocalPlayer::GetPlayerPed()->GetDrunkLevel() > 0)
        {
            CLocalPlayer::GetPlayerPed()->SetDrunkLevel(0);
            CLocalPlayer::GetPlayerPed()->ProcessDrunk();
        }
    }
}

void CLocalPlayer::HandleAfterDeathToggle()
{
	if (!CLocalPlayer::m_bEnableAfterDeath && CGui::GetButton() && CGui::GetButton()->m_bKeyAfterDeath)
    {
		CGui::GetButton()->m_bKeyAfterDeath = false;
		CLocalPlayer::m_bEnableAfterDeath = true;
		CGui::GetChat()->AddInfoMessage(OBFUSCATE("Returning to class selection after next death"));
    }
}

void CLocalPlayer::HandleNeedsToRespawn()
{
	// if (CLocalPlayer::GetPlayerPed()->HasAccessory())
	// {
	// 	CLocalPlayer::GetPlayerPed()->DeleteAllAccessories(true);
	// }

	if (CLocalPlayer::GetPlayerPed()->IsCuffed())
	{
		CLocalPlayer::GetPlayerPed()->ToggleCuffed(false);
	}

	if (CLocalPlayer::m_bClearedToSpawn && !CLocalPlayer::m_bEnableAfterDeath && CMultiPlayer::GetActiveNetwork()->GetNetworkState() == eNetworkState::GAMESTATE_CONNECTED)
	{
		if (CLocalPlayer::GetPlayerPed()->GetHealth() > 0.0f) 
		{
			CLocalPlayer::Spawn();
		}
	}
	else
	{
		if (!CGame::IsMenuActive())
		{
			CGame::DisplayHUD(false);
			CPad::DisplayWidgets(false);
		}

        if (CGui::GetButton() && CGui::GetDebug() && CGui::GetXSing() /*&& CGui::GetGButton()*/ && CGui::GetDialog())
		{
			CGui::GetDebug()->SetRenderFramerate(false);
			CGui::GetDebug()->SetRenderMemory(false);
			CGui::GetButton()->Enable(false);
			CGui::GetXSing()->Enable(false);
			// CGui::GetGButton()->Enable(false);
			CGui::GetDialog()->Enable(false);
		}

        // CVoice::Enable(false);

		CLocalPlayer::m_bIsWasted = false;
		CLocalPlayer::HandleClassSelection();
		CLocalPlayer::m_bEnableAfterDeath = false;
	}
}

void CLocalPlayer::HandleInitialSpawnConditions()
{
	if (CLocalPlayer::GetPlayerPed()->GetActionTrigger() != eAction::ACTION_WASTED && CLocalPlayer::GetPlayerPed()->GetActionTrigger() != eAction::ACTION_DEATH)
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() == eNetworkState::GAMESTATE_CONNECTED && !CLocalPlayer::m_bIsActive && !CLocalPlayer::IsSpectating())
		{
            uint32_t dwTicksSinceLastSelection;
            uint32_t dwThisTick = CNecessary::GetTickCount();

			if (!CGame::IsMenuActive())
			{
				CGame::DisplayHUD(false);
				CPad::DisplayWidgets(false);
			}

			if (CGui::GetDialog() && CGui::GetDialog()->GetActiveDialog()) {
				return;
			}

            if ((dwThisTick - CLocalPlayer::m_dwInitialSelectionTick) < 2000) {
                return;
            }

			if (!CLocalPlayer::m_bSpawnDialogShowed)
			{
				if (CGui::GetSpawnScreen())
				{
					CGui::GetSpawnScreen()->Enable(true);
				}
				
				CLocalPlayer::RequestClass(CLocalPlayer::m_iSelectedClass);
				CLocalPlayer::m_bSpawnDialogShowed = true;
			}
            else
            {
                if (!CLocalPlayer::m_bClearedToSpawn || !CGui::GetSpawnScreen()) {
                    return;
                }

                if (!CLocalPlayer::m_bWaitingForSpawnRequestReply && CGui::GetSpawnScreen()->m_bSendSpawn)
                {
                    CGui::GetSpawnScreen()->m_bSendSpawn = false;
                    CLocalPlayer::SendSpawnRequest();
                }
                else
		        {
                    dwTicksSinceLastSelection = dwThisTick - CLocalPlayer::m_dwLastSpawnSelectionTick;

                    if (dwTicksSinceLastSelection > 250)
                    {
                        if (CGui::GetSpawnScreen()->m_bSendPrev)
                        {
                            CLocalPlayer::m_dwLastSpawnSelectionTick = dwThisTick;

                            CGui::GetSpawnScreen()->m_bSendPrev = false;
                            CLocalPlayer::PrevClass();
                        }
                        else if (CGui::GetSpawnScreen()->m_bSendNext)
                        {
                            CLocalPlayer::m_dwLastSpawnSelectionTick = dwThisTick;

                            CGui::GetSpawnScreen()->m_bSendNext = false;
                            CLocalPlayer::NextClass();
                        }
                    }
                }
            }
		}
	}
}

void CLocalPlayer::HandleDancing()
{
    if (CLocalPlayer::GetPlayerPed()->IsDancing())
    {
        CGame::DisplayHUD(false);
        CLocalPlayer::GetPlayerPed()->ProcessDancing();

        if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] || CLocalPlayer::GetPlayerPed()->IsInVehicle())
        {
            CLocalPlayer::GetPlayerPed()->StopDancing();
        }
    }
}

void CLocalPlayer::HandleUrinating()
{
    if (CLocalPlayer::GetPlayerPed()->IsPissing())
    {
        if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] || CLocalPlayer::GetPlayerPed()->IsInVehicle())
        {
            CLocalPlayer::GetPlayerPed()->StopPissing();
        }
    }
}

void CLocalPlayer::HandleHandsUp()
{
    if (CLocalPlayer::GetPlayerPed()->HasHandsUp())
    {
        if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] || CLocalPlayer::GetPlayerPed()->IsInVehicle())
        {
            CLocalPlayer::GetPlayerPed()->LockControl(false);
            CLocalPlayer::GetPlayerPed()->ToggleControllable(true);
        }
    }
}

void CLocalPlayer::HandleStuff()
{
    if (CLocalPlayer::GetPlayerPed()->GetStuff() != eStuffType::STUFF_TYPE_NONE)
    {
        if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] || CLocalPlayer::GetPlayerPed()->IsInVehicle())
        {
            CLocalPlayer::GetPlayerPed()->DropStuff();
        }
        else
        {
            CLocalPlayer::GetPlayerPed()->ApplyStuff();
        }
    }
}

void CLocalPlayer::HandleInteriorChanging()
{
	if (CGame::GetActiveInterior() != CLocalPlayer::m_byteCurInterior) 
	{
		CLocalPlayer::UpdateRemoteInterior(CGame::GetActiveInterior());
	}
}

void CLocalPlayer::DisplayZone(uint32_t dwThisTick)
{
	if (dwThisTick >= static_cast<uint32_t>(CLocalPlayer::m_iDisplayZoneTick))
	{
		CTheZones::EnableZoneNames(CMultiPlayer::GetActiveNetwork()->m_bZoneNames);
	}
}

void CLocalPlayer::SendStats(uint32_t dwThisTick)
{
	if ((dwThisTick - CLocalPlayer::m_dwLastStatsUpdateTick) > 1000)
	{
		int playerMoney = CGame::GetPlayerMoney();
		int drunkLevel = CLocalPlayer::GetPlayerPed()->GetDrunkLevel();

		if (CMultiPlayer::GetActiveNetwork())
		{
			RakNet::BitStream bsStats;
			bsStats.Write(static_cast<uint8_t>(ID_STATS_UPDATE));
			bsStats.Write(playerMoney);
			bsStats.Write(drunkLevel);
			CMultiPlayer::GetActiveNetwork()->GetRakClient()->Send(&bsStats, HIGH_PRIORITY, UNRELIABLE, 0);
		}

		CLocalPlayer::m_dwLastStatsUpdateTick = dwThisTick;
	}
}

void CLocalPlayer::UpdateWeapons()
{
    if (!CLocalPlayer::GetPlayerPed()) {
        return;
    }

    uint8_t byteCurWeapon = CLocalPlayer::GetPlayerPed()->GetCurrentWeapon();
    uint16_t targetPlayerId = eInvalidId::INVALID_PLAYER_ID;
    uint16_t targetActorID = eInvalidId::INVALID_ACTOR_ID;
    bool bNeedUpdate = false;
    RakNet::BitStream bsWeapons;

    CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    CActorPool* pActorPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool();

    CEntityGta* targetEntity = CLocalPlayer::GetPlayerPed()->GetTargetEntity();
    if (targetEntity)
    {
        if (pPlayerPool)
        {
            targetPlayerId = pPlayerPool->FindRemotePlayerIDFromGtaEntity(targetEntity);
        }

        if (pActorPool)
        {
            targetActorID = pActorPool->FindTargetEntityID(targetEntity);
        }
    }

    if (targetPlayerId != CLocalPlayer::m_weaponsData.m_nAimedPlayer || targetActorID != CLocalPlayer::m_weaponsData.m_nAimedActor)
    {
        CLocalPlayer::m_weaponsData.m_nAimedPlayer = targetPlayerId;
        CLocalPlayer::m_weaponsData.m_nAimedActor = targetActorID;
        bNeedUpdate = true;
    }
    else
    {
        if (byteCurWeapon == CLocalPlayer::m_weaponsData.m_nCurrentWeapon)
        {
            if ((CNecessary::GetTickCount() - CLocalPlayer::m_dwLastWeaponsUpdateTick) < 100)
            {
                return;
            }
        }
    }

    if (byteCurWeapon != CLocalPlayer::m_weaponsData.m_nCurrentWeapon)
    {
        CLocalPlayer::m_weaponsData.m_nCurrentWeapon = byteCurWeapon;
    }

    bsWeapons.Write(static_cast<uint8_t>(ID_WEAPONS_UPDATE));
    bsWeapons.Write(static_cast<uint16_t>(CLocalPlayer::m_weaponsData.m_nAimedPlayer));
    bsWeapons.Write(static_cast<uint16_t>(CLocalPlayer::m_weaponsData.m_nAimedActor));

    for (uint8_t slot = 0; slot < 13; ++slot)
    {
        uint32_t weaponType = CLocalPlayer::GetPlayerPed()->GetPed()->WeaponSlots[slot].dwType;
        uint32_t weaponAmmo = CLocalPlayer::GetPlayerPed()->GetPed()->WeaponSlots[slot].dwAmmo;

        if (CLocalPlayer::m_weaponsData.m_byteLastWeapon[slot] != weaponType || CLocalPlayer::m_weaponsData.m_dwLastAmmo[slot] != weaponAmmo)
        {
            CLocalPlayer::m_weaponsData.m_byteLastWeapon[slot] = weaponType;
            CLocalPlayer::m_weaponsData.m_dwLastAmmo[slot] = weaponAmmo;

            bsWeapons.Write(slot);
            bsWeapons.Write(static_cast<uint8_t>(CLocalPlayer::m_weaponsData.m_byteLastWeapon[slot]));
            bsWeapons.Write(static_cast<uint16_t>(CLocalPlayer::m_weaponsData.m_dwLastAmmo[slot]));

            bNeedUpdate = true;
        }
    }

    if (bNeedUpdate)
    {
        CLocalPlayer::m_dwLastWeaponsUpdateTick = CNecessary::GetTickCount();
        CMultiPlayer::GetActiveNetwork()->GetRakClient()->Send(&bsWeapons, HIGH_PRIORITY, UNRELIABLE, 0);
    }
}

void CLocalPlayer::HandleDead()
{
	CLocalPlayer::EnableSpectating(false);

	if (CLocalPlayer::GetPlayerPed()->IsDancing())
	{
		CLocalPlayer::GetPlayerPed()->StopDancing();
	}

	if (CLocalPlayer::GetPlayerPed()->IsCellphoneEnabled())
	{
		CLocalPlayer::GetPlayerPed()->ToggleCellphone(0);
	}

	if (CLocalPlayer::GetPlayerPed()->IsPissing())
	{
		CLocalPlayer::GetPlayerPed()->StopPissing();
	}

	if (CLocalPlayer::GetPlayerPed()->GetStuff())
	{
		CLocalPlayer::GetPlayerPed()->DropStuff();
	}

	CLocalPlayer::GetPlayerPed()->SetDrunkLevel(0);
	CLocalPlayer::GetPlayerPed()->LockControl(false);
	CLocalPlayer::GetPlayerPed()->ToggleControllable(true);

	// CLocalPlayer::UpdateRCMode(eRcModeType::RC_MODE_OFF);

	// if (CLocalPlayer::GetPlayerPed()->IsInVehicle() && !CLocalPlayer::GetPlayerPed()->IsAPassenger())
	// {
	// 	CLocalPlayer::SendInCarFullSyncData();
	// 	CLocalPlayer::m_LastVehicle = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool()->FindIDFromGtaPtr((CVehicleGta*)CLocalPlayer::GetPlayerPed()->GetVehicle());
	// }

	CLocalPlayer::GetPlayerPed()->ExtinguishFire();
	CLocalPlayer::SendWastedNotification();

	CLocalPlayer::m_bIsActive = false;
	CLocalPlayer::m_bIsWasted = true;
	CTheZones::EnableZoneNames(false);
}

void CLocalPlayer::HandleClassSelection()
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer HandleClassSelection"));

	CLocalPlayer::m_bClearedToSpawn = false;

	if (CLocalPlayer::GetPlayerPed())
	{
		CLocalPlayer::GetPlayerPed()->SetInitialState();
		CLocalPlayer::GetPlayerPed()->SetHealth(100.0f);
		CLocalPlayer::GetPlayerPed()->ToggleControllable(false);
	}

    CLocalPlayer::m_dwInitialSelectionTick = CNecessary::GetTickCount();
    CLocalPlayer::m_dwLastSpawnSelectionTick = CNecessary::GetTickCount();
}

void CLocalPlayer::HandleClassSelectionOutcome(bool bToggle)
{
	if (bToggle)
	{
		if (CLocalPlayer::GetPlayerPed())
		{
			CLocalPlayer::GetPlayerPed()->ClearAllWeapons();
			CLocalPlayer::GetPlayerPed()->SetModelIndex(CLocalPlayer::m_SpawnInfo.iSkin);
		}

		CLocalPlayer::m_bClearedToSpawn = true;
	}
}

void CLocalPlayer::UpdateRemoteInterior(uint8_t byteInterior)
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer UpdateRemoteInterior : %d"), byteInterior);

	CLocalPlayer::m_byteCurInterior = byteInterior;
	if (CMultiPlayer::GetActiveNetwork())
	{
		RakNet::BitStream bsUpdateInterior;
		bsUpdateInterior.Write(byteInterior);
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_SetInteriorId, &bsUpdateInterior, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	}
}

void CLocalPlayer::SetSpawnInfo(PLAYER_SPAWN_INFO *pSpawn)
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer SetSpawnInfo : %d %d %f %f %f %f"), pSpawn->byteTeam, pSpawn->iSkin, pSpawn->vecPos.x, pSpawn->vecPos.y, pSpawn->vecPos.z, pSpawn->fRotation);

	memcpy(&m_SpawnInfo, pSpawn, sizeof(PLAYER_SPAWN_INFO));
	CLocalPlayer::m_bHasSpawnInfo = true;
}

void CLocalPlayer::EnableSpectating(bool bToggle)
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer EnableSpectating : %d"), bToggle);

	if (!bToggle)
	{
		if (CLocalPlayer::IsSpectating() && !CLocalPlayer::m_bEnableAfterDeath)
		{
			CLocalPlayer::Spawn();
		}
		else if (CLocalPlayer::m_bEnableAfterDeath)
        {
            CLocalPlayer::m_bIsActive = false;
            CLocalPlayer::m_bHasSpawnInfo = false;
        }
	}

	CLocalPlayer::m_bIsSpectating = bToggle;
	CLocalPlayer::m_byteSpectateType = SPECTATE_TYPE_NONE;
	CLocalPlayer::m_SpectateID = eInvalidId::INVALID_ID;
	CLocalPlayer::m_bSpectateProcessed = false;
}

void CLocalPlayer::SpectateForPlayer(uint16_t playerId)
{
    CNecessary::Log(OBFUSCATE("CLocalPlayer SpectateForPlayer"));

    if (playerId >= eMaxItem::MAX_PLAYERS) {
        return;
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(playerId))
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_NONE && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_WASTED)
        {
            CLocalPlayer::m_byteSpectateType = SPECTATE_TYPE_PLAYER;
            CLocalPlayer::m_SpectateID = playerId;
            CLocalPlayer::m_bSpectateProcessed = false;
        }
    }
}

bool CLocalPlayer::Spawn()
{
    if (!CLocalPlayer::IsSpawnInfo()) {
		return false;
	}

    CNecessary::Log(OBFUSCATE("CLocalPlayer Spawn"));

    if (CLocalPlayer::IsSpawnDialog())
	{
		CLocalPlayer::m_bSpawnDialogShowed = false;

		if (CGui::GetSpawnScreen())
		{
			CGui::GetSpawnScreen()->Enable(false);
		}
	}

	if (CGame::GetCamera() && CGame::GetPlayer())
    {
		CGame::GetCamera()->Restore();
		CGame::GetCamera()->SetBehindPlayer();
	}

    CLocalPlayer::GetPlayerPed()->LockControl(false);
	CLocalPlayer::GetPlayerPed()->ToggleControllable(true);

	CGame::DisplayHUD(true);
	CWidgets::SetToggle(true);

    if (!bFirstSpawn)
	{
		CLocalPlayer::GetPlayerPed()->SetInitialState();
	}
	else
	{
		bFirstSpawn = false;
	}

	CGame::RefreshStreamingAt(CLocalPlayer::m_SpawnInfo.vecPos);

	if (CLocalPlayer::GetPlayerPed()->IsCuffed())
	{
		CLocalPlayer::GetPlayerPed()->ToggleCuffed(false);
	}

	CLocalPlayer::GetPlayerPed()->RestartIfWastedAt(&m_SpawnInfo.vecPos, m_SpawnInfo.fRotation);
	CLocalPlayer::GetPlayerPed()->SetModelIndex(CLocalPlayer::m_SpawnInfo.iSkin);

	if (CRealTimeShadowManager::GetShadowStyle() == 1 && pthread_main_np() && CLocalPlayer::GetPlayerPed()->VerifyInstance())
	{
		CRealTimeShadow* pShadow = CLocalPlayer::GetPlayerPed()->GetPed()->m_pRealTimeShadow;
		if (pShadow && pShadow->m_pPhysical == CLocalPlayer::GetPlayerPed()->GetPed())
		{
			CRealTimeShadowManager::ReturnRealTimeShadow(nullptr, pShadow);
		}
	}

	CLocalPlayer::GetPlayerPed()->ClearAllWeapons();
	CLocalPlayer::GetPlayerPed()->ResetDamageEntity();

	CLocalPlayer::ApplySpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);

	// CLocalPlayer::m_surfData.bIsActive = false;
	// CLocalPlayer::m_surfData.vecPosition.x = 0.0;
	// CLocalPlayer::m_surfData.vecPosition.y = 0.0;
	// CLocalPlayer::m_surfData.vecPosition.z = 0.0;
	// CLocalPlayer::m_surfData.entityId = eInvalidId::INVALID_ID;
	// CLocalPlayer::m_surfData.pVehicle = nullptr;
	// CLocalPlayer::m_surfData.bIsStuck = false;

	CScripting::ScriptCommand(&enable_train_traffic, 0);

	if (CLocalPlayer::m_SpawnInfo.iSpawnWeapons[2] != eInvalidId::INVALID_ID)
	{
		CLocalPlayer::GetPlayerPed()->GiveWeapon(CLocalPlayer::m_SpawnInfo.iSpawnWeapons[2], CLocalPlayer::m_SpawnInfo.iSpawnWeaponsAmmo[2]);
	}

	if (CLocalPlayer::m_SpawnInfo.iSpawnWeapons[1] != eInvalidId::INVALID_ID)
	{
		CLocalPlayer::GetPlayerPed()->GiveWeapon(CLocalPlayer::m_SpawnInfo.iSpawnWeapons[1], CLocalPlayer::m_SpawnInfo.iSpawnWeaponsAmmo[1]);
	}

	if (CLocalPlayer::m_SpawnInfo.iSpawnWeapons[0] != eInvalidId::INVALID_ID)
	{
		CLocalPlayer::GetPlayerPed()->GiveWeapon(CLocalPlayer::m_SpawnInfo.iSpawnWeapons[0], CLocalPlayer::m_SpawnInfo.iSpawnWeaponsAmmo[0]);
	}

	// CCamera::Fade
	CMemory::makeRET(SA_ADDR(0x13524C));

	CLocalPlayer::GetPlayerPed()->TeleportTo(CLocalPlayer::m_SpawnInfo.vecPos.x, CLocalPlayer::m_SpawnInfo.vecPos.y, CLocalPlayer::m_SpawnInfo.vecPos.z + 0.5f);
	CLocalPlayer::GetPlayerPed()->ForceTargetRotation(CLocalPlayer::m_SpawnInfo.fRotation);

	CNecessary::Log(OBFUSCATE("CLocalPlayer Spawn : %d %d %f %f %f %f"), CLocalPlayer::m_SpawnInfo.byteTeam, CLocalPlayer::m_SpawnInfo.iSkin, CLocalPlayer::m_SpawnInfo.vecPos.x, CLocalPlayer::m_SpawnInfo.vecPos.y, CLocalPlayer::m_SpawnInfo.vecPos.z, CLocalPlayer::m_SpawnInfo.fRotation);

	CLocalPlayer::m_bIsWasted = false;
	CLocalPlayer::m_bIsActive = true;
	CLocalPlayer::m_bWaitingForSpawnRequestReply = false;

    if (CGui::GetButton() && CGui::GetDebug() && CGui::GetXSing() /*&& CGui::GetGButton()*/)
	{
		CGui::GetDebug()->SetRenderFramerate(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowFps")));
		CGui::GetDebug()->SetRenderMemory(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowMem")));
		CGui::GetButton()->Enable(true);
		CGui::GetXSing()->Enable(true);
		// CGui::GetGButton()->Enable(true);
	}

	// CVoice::Enable(true);

	if (CMultiPlayer::GetActiveNetwork())
	{
		RakNet::BitStream bsSendSpawn;
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_Spawn, &bsSendSpawn, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, NULL);
	}

	CLocalPlayer::m_iDisplayZoneTick = CNecessary::GetTickCount() + 1000;

    return true;
}

void CLocalPlayer::NextClass()
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer NextClass"));

	if (!CLocalPlayer::IsSpawnDialog()) {
		return;
	}

	CMatrix matPlayer;
	CLocalPlayer::m_bClearedToSpawn = false;

	if (CLocalPlayer::GetPlayerPed())
	{
		CLocalPlayer::GetPlayerPed()->GetMatrix(&matPlayer);
	}

	if (static_cast<uint32_t>(CLocalPlayer::m_iSelectedClass) == CMultiPlayer::GetActiveNetwork()->m_iSpawnsAvailable - 1)
	{
		CLocalPlayer::m_iSelectedClass = 0;
	}
	else 
	{
		CLocalPlayer::m_iSelectedClass++;
	}

	CAudio::Play(1052, matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z);
	CLocalPlayer::RequestClass(CLocalPlayer::m_iSelectedClass);
}

void CLocalPlayer::PrevClass()
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer PrevClass"));

	if (!CLocalPlayer::IsSpawnDialog()) {
		return;
	}

	CMatrix matPlayer;
	CLocalPlayer::m_bClearedToSpawn = false;
	
	if (CLocalPlayer::GetPlayerPed())
	{
		CLocalPlayer::GetPlayerPed()->GetMatrix(&matPlayer);
	}

	if (CLocalPlayer::m_iSelectedClass == 0) 
	{
		CLocalPlayer::m_iSelectedClass = (CMultiPlayer::GetActiveNetwork()->m_iSpawnsAvailable - 1);
	}
	else 
	{
		CLocalPlayer::m_iSelectedClass--;
	}

	CAudio::Play(1053, matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z);
	CLocalPlayer::RequestClass(CLocalPlayer::m_iSelectedClass);
}

void CLocalPlayer::SendSpawnRequest()
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer SendSpawnRequest"));

	if (!CLocalPlayer::IsSpawnDialog()) {
		return;
	}

	CLocalPlayer::RequestSpawn();
	CLocalPlayer::m_bWaitingForSpawnRequestReply = true;
}

void CLocalPlayer::RequestClass(int iClass)
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer RequestClass"));

	if (CMultiPlayer::GetActiveNetwork())
	{
		RakNet::BitStream bsSpawnRequest;
		bsSpawnRequest.Write(iClass);

		CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_RequestClass, &bsSpawnRequest, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, 0);
	}
}

void CLocalPlayer::RequestSpawn()
{
	CNecessary::Log(OBFUSCATE("CLocalPlayer RequestSpawn"));

	if (CMultiPlayer::GetActiveNetwork())
	{
		RakNet::BitStream bsSpawnRequest;
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_RequestSpawn, &bsSpawnRequest, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, 0);
	}
}

bool CLocalPlayer::NeedsToUpdate(const void *pOld, const void *pNew, unsigned int nLen, uint32_t tickDef)
{
	if (CNecessary::GetTickCount() - CLocalPlayer::m_dwLastAnySendTick > tickDef || memcmp(pOld, pNew, nLen))
	{
		CLocalPlayer::m_dwLastAnySendTick = CNecessary::GetTickCount();
		return true;
	}

	return false;
}

void CLocalPlayer::ProcessOnFoot(uint32_t dwThisTick)
{
	uint16_t lrAnalog, udAnalog;
    uint16_t wKeys = CLocalPlayer::GetPlayerPed()->GetKeys(&lrAnalog, &udAnalog, false);

	CLocalPlayer::ProcessHead();
	// CLocalPlayer::ProcessSurfing();

	// CLocalPlayer::UpdateRCMode(eRcModeType::RC_MODE_OFF);
	// CLocalPlayer::ProcessOnFootWorldBounds();

	// if (CLocalPlayer::m_CurrentVehicle != eInvalidId::INVALID_VEHICLE_ID)
	// {
	// 	CLocalPlayer::m_LastVehicle = CLocalPlayer::m_CurrentVehicle;
	// 	CLocalPlayer::m_CurrentVehicle = eInvalidId::INVALID_VEHICLE_ID;
	// }
	
	if (CLocalPlayer::NeedsToSendOnfootData(wKeys, lrAnalog, udAnalog) || (dwThisTick - CLocalPlayer::m_dwLastSendTick) > (uint32_t)CLocalPlayer::GetOptimumOnFootSendRate())
	{
		CLocalPlayer::m_dwLastSendTick = CNecessary::GetTickCount();
		CLocalPlayer::SendOnFootFullSyncData();
	}

	CLocalPlayer::m_bPassengerDriveByMode = false;
}

void CLocalPlayer::ProcessDriver([[maybe_unused]] uint32_t dwThisTick)
{
    // CVehiclePool* pVehiclePool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool();
    // CLocalPlayer::m_CurrentVehicle = pVehiclePool->FindIDFromGtaPtr((CVehicleGta*)CLocalPlayer::GetPlayerPed()->GetVehicle());

    CLocalPlayer::GetPlayerPed()->RemoveWeaponWhenEnteringVehicle();
    CLocalPlayer::ProcessHead();
    // CLocalPlayer::UpdateRCMode(eRcModeType::RC_MODE_ON);
    // CLocalPlayer::ProcessInCarWorldBounds();

    // if ((dwThisTick - CLocalPlayer::m_dwLastSendAimTick) > 1000)
    // {
        // CLocalPlayer::m_dwLastSendAimTick = dwThisTick;
        // CLocalPlayer::SendAimSyncData();
    // }

    // if ((dwThisTick - CLocalPlayer::m_dwLastSendTick) > (uint32_t)CLocalPlayer::GetOptimumInCarSendRate())
    // {
        // CLocalPlayer::m_dwLastSendTick = CNecessary::GetTickCount();
        // CLocalPlayer::SendInCarFullSyncData();
    // }

    // CLocalPlayer::ProcessVehicleDamageUpdates(CLocalPlayer::m_CurrentVehicle);
    CLocalPlayer::m_bPassengerDriveByMode = false;
}

void CLocalPlayer::ProcessAimSync(uint32_t dwThisTick)
{
    uint16_t lrAnalog, udAnalog;
    uint16_t wKeys = CLocalPlayer::GetPlayerPed()->GetKeys(&lrAnalog, &udAnalog, false);

    bool bWannaSync = false;
    if ((wKeys & 0x84) != 0)
    {
        if (CMultiPlayer::GetActiveNetwork()->m_iLagCompensation == 2)
        {
            bWannaSync = (dwThisTick - CLocalPlayer::m_dwLastSendAimTick) > CMultiPlayer::GetActiveNetwork()->m_iNetModeFiringSendRate;
        }
        else
        {
            bWannaSync = (dwThisTick - CLocalPlayer::m_dwLastSendAimTick) > NETMODE_AIM_SENDRATE;
        }
    }
    else
    {
        bWannaSync = (dwThisTick - CLocalPlayer::m_dwLastSendAimTick) > NETMODE_HEADSYNC_SENDRATE;
    }

    if (bWannaSync)
    {
        CLocalPlayer::m_dwLastSendAimTick = dwThisTick;
        CLocalPlayer::SendAimSyncData();
    }
}

void CLocalPlayer::ProcessHead()
{
    if (!CHeadMove::GetHeadMove()) {
        return;
    }

    if (CLocalPlayer::GetPlayerPed()->GetStuff()) {
        return;
    }

    CMatrix mat;
    CLocalPlayer::GetPlayerPed()->GetMatrix(&mat);

    if (CNecessary::GetTickCount() - CLocalPlayer::m_headData.m_dwLastHeadUpdate > 1000)
    {
        CAMERA_AIM* Aim = CCameraAim::GameGetInternalAim();

        float lookX = Aim->f1x * 20.0 + mat.pos.x;
        float lookY = Aim->f1y * 20.0 + mat.pos.y;
        float lookZ = Aim->f1z * 20.0 + mat.pos.z;

        CLocalPlayer::m_headData.m_vecHeadDir.x = lookX - mat.pos.x;
        CLocalPlayer::m_headData.m_vecHeadDir.y = lookY - mat.pos.y;
        CLocalPlayer::m_headData.m_vecHeadDir.z = lookZ - mat.pos.z;

        CLocalPlayer::m_headData.m_dwLastHeadUpdate = CNecessary::GetTickCount();
    }

    if (CNecessary::GetTickCount() - CLocalPlayer::m_headData.m_dwLastHeadLook > 200)
    {
        CVector LookAt;
        LookAt.x = mat.pos.x + CLocalPlayer::m_headData.m_vecHeadDir.x;
        LookAt.y = mat.pos.y + CLocalPlayer::m_headData.m_vecHeadDir.y;
        LookAt.z = mat.pos.z + CLocalPlayer::m_headData.m_vecHeadDir.z;

        CLocalPlayer::GetPlayerPed()->ApplyCommandTask(OBFUSCATE("FollowPedSA"), 0, 2000, -1, &LookAt, 0, 0.1f, 500, 3, 0);
        CLocalPlayer::m_headData.m_dwLastHeadLook = CNecessary::GetTickCount();
    }
}

void CLocalPlayer::ProcessAnimation()
{
    if (!CLocalPlayer::GetPlayerPed()) {
        return;
    }

    if (CLocalPlayer::GetPlayerPed()->GetBlendedAnimationsIndex())
    {
        CLocalPlayer::m_dwAnimationValue = CLocalPlayer::GetPlayerPed()->GetBlendedAnimationsIndex();
        CLocalPlayer::m_bAnimationReset = false;
    }
    else if (CLocalPlayer::m_dwAnimationValue && CLocalPlayer::m_bAnimationReset)
    {
        CLocalPlayer::m_dwAnimationValue = 0;
    }
}

void CLocalPlayer::ProcessSpectating()
{
    if (!CLocalPlayer::GetPlayerPed()) {
        return;
    }

    if (!CGame::GetCamera()) {
        return;
    }

    RakNet::BitStream bsSpectatorSync;
    SPECTATOR_SYNC_DATA spSync;
    CMatrix matCam;
    CGame::GetCamera()->GetMatrix(&matCam);
    uint16_t lrAnalog, udAnalog;
    uint16_t wKeys = CLocalPlayer::GetPlayerPed()->GetKeys(&lrAnalog, &udAnalog);

    spSync.vecPos = matCam.pos;
    spSync.lrAnalog = lrAnalog;
    spSync.udAnalog = udAnalog;
    spSync.wKeys = wKeys;

    if (CNecessary::GetTickCount() - CLocalPlayer::m_dwLastSendSpecTick > static_cast<uint32_t>(CLocalPlayer::GetOptimumOnFootSendRate()))
    {
        CLocalPlayer::m_dwLastSendSpecTick = CNecessary::GetTickCount();
        bsSpectatorSync.Write(static_cast<uint8_t>(ID_SPECTATOR_SYNC));
        bsSpectatorSync.Write((const char*)&spSync, sizeof(SPECTATOR_SYNC_DATA));
        CMultiPlayer::GetActiveNetwork()->GetRakClient()->Send(&bsSpectatorSync, HIGH_PRIORITY, UNRELIABLE, 0);

        if (CNecessary::GetTickCount() - CLocalPlayer::m_dwLastSendAimTick > static_cast<uint32_t>(CLocalPlayer::GetOptimumOnFootSendRate() * 2))
        {
            CLocalPlayer::m_dwLastSendAimTick = CNecessary::GetTickCount();
            CLocalPlayer::SendAimSyncData();
        }
    }

    CGame::DisplayHUD(false);

    CLocalPlayer::GetPlayerPed()->SetHealth(100.0f);
    CLocalPlayer::GetPlayerPed()->TeleportTo(spSync.vecPos.x, spSync.vecPos.y, spSync.vecPos.z + 20.0f);

    if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool() /* || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool() */) {
        return;
    }

    if (CLocalPlayer::m_byteSpectateType == SPECTATE_TYPE_PLAYER)
    {
        if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(static_cast<uint16_t>(CLocalPlayer::m_SpectateID)) || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(static_cast<uint16_t>(CLocalPlayer::m_SpectateID))->IsActive() || CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(static_cast<uint16_t>(CLocalPlayer::m_SpectateID))->GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_WASTED)
        {
            CLocalPlayer::m_byteSpectateType = SPECTATE_TYPE_NONE;
            CLocalPlayer::m_bSpectateProcessed = false;
        }
    }

    if (CLocalPlayer::m_bSpectateProcessed) {
        return;
    }

    if (CLocalPlayer::m_byteSpectateType == SPECTATE_TYPE_NONE)
    {
        CLocalPlayer::GetPlayerPed()->RemoveFromVehicleAndPutAt(0.0f, 0.0f, 10.0f);
        CGame::GetCamera()->SetPosition(50.0f, 50.0f, 50.0f, 0.0f, 0.0f, 0.0f);
        CGame::GetCamera()->LookAtPoint(60.0f, 60.0f, 50.0f, 2);
        CLocalPlayer::m_bSpectateProcessed = true;
    }
    else if (CLocalPlayer::m_byteSpectateType == SPECTATE_TYPE_PLAYER)
    {
        CPlayerPed* pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(CLocalPlayer::m_SpectateID)->GetGamePlayer();
        if (pPlayerPed)
        {
            CCamera::TakeControl(&CCamera::TheCamera, pPlayerPed->GetPed(), static_cast<eCamMode>(CLocalPlayer::m_byteSpectateMode), eSwitchType::SWITCHTYPE_JUMPCUT, 1);
            CLocalPlayer::m_bSpectateProcessed = true;
        }
    }
    // Vehicle Spectate: ยังไม่เปิดใช้งาน
    // else if (CLocalPlayer::m_byteSpectateType == SPECTATE_TYPE_VEHICLE)
    // {
        // // Vehicle
        // CVehicle* pVehicle = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool()->GetVehicle(static_cast<uint16_t>(CLocalPlayer::m_SpectateID));
        // if (pVehicle)
        // {
            // if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool()->IsValidPointer(static_cast<uint16_t>(CLocalPlayer::m_SpectateID), pVehicle)) {
                // CNecessary::Log(OBFUSCATE("ProcessSpectating: pVehicle is no longer valid."));
                // return;
            // }

            // CCamera::TakeControl(&CCamera::TheCamera, pVehicle->GetGtaVehicle(), static_cast<eCamMode>(CLocalPlayer::m_byteSpectateMode), eSwitchType::SWITCHTYPE_JUMPCUT, 1);
            // CLocalPlayer::m_bSpectateProcessed = true;
        // }
    // }
}

void CLocalPlayer::SendWastedNotification()
{
    RakNet::BitStream bsPlayerDeath;
    uint8_t byteDeathReason = 0;
    uint16_t WhoWasResponsible = eInvalidId::INVALID_PLAYER_ID;

    byteDeathReason = CLocalPlayer::GetPlayerPed()->FindDeathReasonAndResponsiblePlayer(&WhoWasResponsible);

    bsPlayerDeath.Write(byteDeathReason);
    bsPlayerDeath.Write(WhoWasResponsible);

    CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_Death, &bsPlayerDeath, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr); // มีปัญหา บางทีมันไม่ยอมเข้าสถานะการตาย
}

void CLocalPlayer::SendOnFootFullSyncData()
{
	ONFOOT_SYNC_DATA ofSync;

	uint16_t lrAnalog, udAnalog;
	uint16_t wKeys = CLocalPlayer::GetPlayerPed()->GetKeys(&lrAnalog, &udAnalog);

	CMatrix matPlayer;
	CLocalPlayer::GetPlayerPed()->GetMatrix(&matPlayer);
	CVector vecMoveSpeed;
	CLocalPlayer::GetPlayerPed()->GetMoveSpeedVector(&vecMoveSpeed);

	ofSync.lrAnalog = lrAnalog;
	ofSync.udAnalog = udAnalog;
	ofSync.wKeys = wKeys;

	ofSync.vecPos.x = matPlayer.pos.x;
	ofSync.vecPos.y = matPlayer.pos.y;
	ofSync.vecPos.z = matPlayer.pos.z;

	uint8_t exKeys = CLocalPlayer::GetPlayerPed()->GetExtendedKeys();
	ofSync.byteCurrentWeapon = (exKeys << 6) | (ofSync.byteCurrentWeapon & 0x3F);

	ofSync.quat.SetFromMatrix(matPlayer);
	ofSync.quat.Normalize();

	float quatEpsilon = 0.0000099999997f;
	if (std::abs(ofSync.quat.w - m_OnFootData.quat.w) < quatEpsilon && std::abs(ofSync.quat.x - m_OnFootData.quat.x) < quatEpsilon && std::abs(ofSync.quat.y - m_OnFootData.quat.y) < quatEpsilon && std::abs(ofSync.quat.z - m_OnFootData.quat.z) < quatEpsilon)
	{
		ofSync.quat = m_OnFootData.quat;
	}

	ofSync.byteHealth = static_cast<uint8_t>(CLocalPlayer::GetPlayerPed()->GetHealth());
	ofSync.byteArmour = static_cast<uint8_t>(CLocalPlayer::GetPlayerPed()->GetArmour());

	ofSync.byteCurrentWeapon ^= (ofSync.byteCurrentWeapon ^ CLocalPlayer::GetPlayerPed()->GetCurrentWeapon()) & 0x3F;

	ofSync.byteSpecialAction = CLocalPlayer::GetSpecialAction();

	ofSync.vecMoveSpeed.x = vecMoveSpeed.x;
	ofSync.vecMoveSpeed.y = vecMoveSpeed.y;
	ofSync.vecMoveSpeed.z = vecMoveSpeed.z;

    // เดียวมาทำต่อไอสัส
	// if (CLocalPlayer::m_surfData.bIsActive)
    // {
	// 	if (CLocalPlayer::m_surfData.entityId != eInvalidId::INVALID_VEHICLE_ID)
	// 	{
	// 		ofSync.vecSurfOffsets.x = CLocalPlayer::m_surfData.vecPosition.x;
	// 		ofSync.vecSurfOffsets.y = CLocalPlayer::m_surfData.vecPosition.y;
	// 		ofSync.vecSurfOffsets.z = CLocalPlayer::m_surfData.vecPosition.z;
	// 		ofSync.wSurfInfo = CLocalPlayer::m_surfData.entityId;
	// 	}
    // }
    // else
    // {
    //     ofSync.vecSurfOffsets.x = 0.0f;
	// 	ofSync.vecSurfOffsets.y = 0.0f;
	// 	ofSync.vecSurfOffsets.z = 0.0f;
    //     ofSync.wSurfInfo = 0;
    // }

    ofSync.vecSurfOffsets.x = 0.0f;
	ofSync.vecSurfOffsets.y = 0.0f;
	ofSync.vecSurfOffsets.z = 0.0f;
    ofSync.wSurfInfo = 0;

	ofSync.dwAnimation = CLocalPlayer::m_dwAnimationValue;
	CLocalPlayer::m_bAnimationReset = true;

	if (CLocalPlayer::NeedsToUpdate(&m_OnFootData, &ofSync, sizeof(ONFOOT_SYNC_DATA)))
	{
		RakNet::BitStream bsPlayerSync;
		bsPlayerSync.Write(static_cast<uint8_t>(ID_PLAYER_SYNC));
		bsPlayerSync.Write((char*)&ofSync, sizeof(ONFOOT_SYNC_DATA));
		CMultiPlayer::GetActiveNetwork()->GetRakClient()->Send(&bsPlayerSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
		memcpy(&m_OnFootData, &ofSync, sizeof(ONFOOT_SYNC_DATA));
	}
}

void CLocalPlayer::SendAimSyncData()
{
    AIM_SYNC_DATA aimSync = {};
    CAMERA_AIM* caAim = CLocalPlayer::GetPlayerPed()->GetCurrentAim();

    aimSync.byteCamMode = static_cast<uint8_t>(CLocalPlayer::GetPlayerPed()->GetCameraMode());
    aimSync.vecAimf.x = caAim->f1x;
    aimSync.vecAimf.y = caAim->f1y;
    aimSync.vecAimf.z = caAim->f1z;
    aimSync.vecAimPos.x = caAim->pos1x;
    aimSync.vecAimPos.y = caAim->pos1y;
    aimSync.vecAimPos.z = caAim->pos1z;
    aimSync.fAimZ = CLocalPlayer::GetPlayerPed()->GetAimZ();
    aimSync.byteCamExtZoom = static_cast<uint8_t>(CLocalPlayer::GetPlayerPed()->GetCameraExtendedZoom() * 63.0f) & 0x3F;
    aimSync.byteAspectRatio = static_cast<uint8_t>(CCameraAim::GameGetAspectRatio() * 255.0f);

    WEAPON_SLOT_TYPE* pwstWeapon = CLocalPlayer::GetPlayerPed()->GetCurrentWeaponSlot();
    if (pwstWeapon->dwState == 2)
    {
        aimSync.byteWeaponState = eSAMPWeaponState::WS_RELOADING;
    }
    else
    {
        aimSync.byteWeaponState = (pwstWeapon->dwAmmoInClip > 1) ? eSAMPWeaponState::WS_MORE_BULLETS : pwstWeapon->dwAmmoInClip;
    }

    if (CLocalPlayer::NeedsToUpdate(&m_aimData, &aimSync, sizeof(AIM_SYNC_DATA)))
    {
        RakNet::BitStream bsAimSync;
        bsAimSync.Write(static_cast<uint8_t>(ID_AIM_SYNC));
        bsAimSync.Write((char*)&aimSync, sizeof(AIM_SYNC_DATA));
        CMultiPlayer::GetActiveNetwork()->GetRakClient()->Send(&bsAimSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);

        memcpy(&m_aimData, &aimSync, sizeof(AIM_SYNC_DATA));
    }
}

void CLocalPlayer::ApplySpecialAction(uint8_t byteSpecialAction)
{
    switch (byteSpecialAction)
    {
        case eSpecialAction::SPECIAL_ACTION_NONE: {
            if (CLocalPlayer::GetPlayerPed()->IsCellphoneEnabled())
            {
                CLocalPlayer::GetPlayerPed()->ToggleCellphone(0);
            }

            if (CLocalPlayer::GetPlayerPed()->IsInJetpack())
            {
                CLocalPlayer::GetPlayerPed()->StopJetpack();
            }

            if (CLocalPlayer::GetPlayerPed()->HasHandsUp())
            {
                CLocalPlayer::GetPlayerPed()->LockControl(false);
                CLocalPlayer::GetPlayerPed()->ToggleControllable(true);
            }

            if (CLocalPlayer::GetPlayerPed()->IsPissing())
            {
                CLocalPlayer::GetPlayerPed()->StopPissing();
            }

            if (CLocalPlayer::GetPlayerPed()->IsCarry())
            {
                CLocalPlayer::GetPlayerPed()->ToggleCarry(false);
            }

            if (CLocalPlayer::GetPlayerPed()->IsCuffed())
            {
                CLocalPlayer::GetPlayerPed()->ToggleCuffed(false);
            }

            if (CLocalPlayer::GetPlayerPed()->IsDancing())
            {
                CLocalPlayer::GetPlayerPed()->StopDancing();
            }

            if (CLocalPlayer::GetPlayerPed()->GetStuff())
            {
                CLocalPlayer::GetPlayerPed()->DropStuff();
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_USECELLPHONE: {
            if (!CLocalPlayer::GetPlayerPed()->IsInVehicle())
            {
                if (!CLocalPlayer::GetPlayerPed()->IsCellphoneEnabled())
                {
                    CLocalPlayer::GetPlayerPed()->ToggleCellphone(1);
                }
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_STOPUSECELLPHONE: {
            if (CLocalPlayer::GetPlayerPed()->IsCellphoneEnabled())
            {
                CLocalPlayer::GetPlayerPed()->ToggleCellphone(0);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_USEJETPACK: {
            if (!CLocalPlayer::GetPlayerPed()->IsInJetpack())
            {
                CLocalPlayer::GetPlayerPed()->StartJetpack();
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_HANDSUP: {
            if (!CLocalPlayer::GetPlayerPed()->HasHandsUp())
            {
                CLocalPlayer::GetPlayerPed()->HandsUp();
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_URINATING: {
            if (!CLocalPlayer::GetPlayerPed()->IsInVehicle())
            {
                if (!CLocalPlayer::GetPlayerPed()->IsPissing())
                {
                    CLocalPlayer::GetPlayerPed()->StartPissing();
                }
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_CARRY: {
            if (!CLocalPlayer::GetPlayerPed()->IsCarry())
            {
                CLocalPlayer::GetPlayerPed()->ToggleCarry(true);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_CUFFED: {
            if (!CLocalPlayer::GetPlayerPed()->IsCuffed())
            {
                CLocalPlayer::GetPlayerPed()->ToggleCuffed(true);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DANCE1: {
            if (!CLocalPlayer::GetPlayerPed()->IsDancing())
            {
                CLocalPlayer::GetPlayerPed()->StartDancing(0);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DANCE2: {
            if (!CLocalPlayer::GetPlayerPed()->IsDancing())
            {
                CLocalPlayer::GetPlayerPed()->StartDancing(1);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DANCE3: {
            if (!CLocalPlayer::GetPlayerPed()->IsDancing())
            {
                CLocalPlayer::GetPlayerPed()->StartDancing(2);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DANCE4: {
            if (!CLocalPlayer::GetPlayerPed()->IsDancing())
            {
                CLocalPlayer::GetPlayerPed()->StartDancing(3);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DRINK_BEER: {
            if (!CLocalPlayer::GetPlayerPed()->IsInVehicle())
            {
                CLocalPlayer::GetPlayerPed()->GiveStuff(eStuffType::STUFF_TYPE_BEER);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_SMOKE_CIGGY: {
            if (!CLocalPlayer::GetPlayerPed()->IsInVehicle())
            {
                CLocalPlayer::GetPlayerPed()->GiveStuff(eStuffType::STUFF_TYPE_CIGGI);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DRINK_WINE: {
            if (!CLocalPlayer::GetPlayerPed()->IsInVehicle())
            {
                CLocalPlayer::GetPlayerPed()->GiveStuff(eStuffType::STUFF_TYPE_DYN_BEER);
            }

            break;
        }

        case eSpecialAction::SPECIAL_ACTION_DRINK_SPRUNK: {
            if (!CLocalPlayer::GetPlayerPed()->IsInVehicle())
            {
                CLocalPlayer::GetPlayerPed()->GiveStuff(eStuffType::STUFF_TYPE_PINT_GLASS);
            }

            break;
        }

        default: {
            return;
        }
    }
}

uint8_t CLocalPlayer::GetSpecialAction()
{
    if (CLocalPlayer::GetPlayerPed()->IsInJetpack())
    {
        return eSpecialAction::SPECIAL_ACTION_USEJETPACK;
    }

    if (CLocalPlayer::GetPlayerPed()->IsDancing())
    {
        switch (CLocalPlayer::GetPlayerPed()->m_iDanceStyle)
        {
            case 0: {
                return eSpecialAction::SPECIAL_ACTION_DANCE1;
            }

            case 1: {
                return eSpecialAction::SPECIAL_ACTION_DANCE2;
            }

            case 2: {
                return eSpecialAction::SPECIAL_ACTION_DANCE3;
            }

            case 3: {
                return eSpecialAction::SPECIAL_ACTION_DANCE4;
            }

            default: {
                break;
            }
        }
    }

    if (CLocalPlayer::GetPlayerPed()->HasHandsUp())
    {
        return eSpecialAction::SPECIAL_ACTION_HANDSUP;
    }

    if (CLocalPlayer::GetPlayerPed()->IsCellphoneEnabled())
    {
        return eSpecialAction::SPECIAL_ACTION_USECELLPHONE;
    }

    if (CLocalPlayer::GetPlayerPed()->IsPissing())
    {
        return eSpecialAction::SPECIAL_ACTION_URINATING;
    }

    if (CLocalPlayer::GetPlayerPed()->IsCrouching())
    {
        return eSpecialAction::SPECIAL_ACTION_DUCK;
    }

    if (CLocalPlayer::GetPlayerPed()->IsEnteringVehicle())
    {
        return eSpecialAction::SPECIAL_ACTION_ENTER_VEHICLE;
    }

    if (CLocalPlayer::GetPlayerPed()->IsExitingVehicle())
    {
        return eSpecialAction::SPECIAL_ACTION_EXIT_VEHICLE;
    }

    if (CLocalPlayer::GetPlayerPed()->GetStuff())
    {
        if (CLocalPlayer::GetPlayerPed()->GetStuff() == eStuffType::STUFF_TYPE_BEER)
        {
            return eSpecialAction::SPECIAL_ACTION_DRINK_BEER;
        }

        if (CLocalPlayer::GetPlayerPed()->GetStuff() == eStuffType::STUFF_TYPE_DYN_BEER)
        {
            return eSpecialAction::SPECIAL_ACTION_DRINK_WINE;
        }

        if (CLocalPlayer::GetPlayerPed()->GetStuff() == eStuffType::STUFF_TYPE_PINT_GLASS)
        {
            return eSpecialAction::SPECIAL_ACTION_DRINK_SPRUNK;
        }

        if (CLocalPlayer::GetPlayerPed()->GetStuff() == eStuffType::STUFF_TYPE_CIGGI)
        {
            return eSpecialAction::SPECIAL_ACTION_SMOKE_CIGGY;
        }
    }

    if (CLocalPlayer::GetPlayerPed()->IsCuffed())
    {
        return eSpecialAction::SPECIAL_ACTION_CUFFED;
    }

    if (CLocalPlayer::GetPlayerPed()->IsCarry())
    {
        return eSpecialAction::SPECIAL_ACTION_CARRY;
    }

    return eSpecialAction::SPECIAL_ACTION_NONE;
}

void CLocalPlayer::SetLocalTeam(uint8_t byteTeam)
{
	CLocalPlayer::m_byteTeamId = byteTeam; 
}

uint8_t CLocalPlayer::GetLocalTeam()
{
	return CLocalPlayer::m_byteTeamId;
}

bool CLocalPlayer::NeedsToSendOnfootData(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog)
{
	return wKeys != CLocalPlayer::m_OnFootData.wKeys || udAnalog != CLocalPlayer::m_OnFootData.udAnalog || lrAnalog != CLocalPlayer::m_OnFootData.lrAnalog;
}

int CLocalPlayer::GetOptimumOnFootSendRate()
{
	if (!CLocalPlayer::GetPlayerPed()) 
	{
		return 1000;
	}

	if (CMultiPlayer::GetActiveNetwork()->m_bLanMode)
    {
        return 15;
    }

	return (CMultiPlayer::GetActiveNetwork()->m_iNetModeNormalOnfootSendRate + CLocalPlayer::DetermineNumberOfPlayersInLocalRange());
}

int CLocalPlayer::GetOptimumInCarSendRate()
{
	if (!CLocalPlayer::GetPlayerPed()) 
	{
		return 1000;
	}

	if (CMultiPlayer::GetActiveNetwork()->m_bLanMode)
    {
        return 15;
    }

	return (CMultiPlayer::GetActiveNetwork()->m_iNetModeNormalInCarSendRate + CLocalPlayer::DetermineNumberOfPlayersInLocalRange());
}

uint8_t CLocalPlayer::DetermineNumberOfPlayersInLocalRange()
{
	int iNumPlayersInRange = 0;
	for (int i = 2; i < eMaxItem::PLAYER_PED_SLOTS; i++)
	{
		if (CPlayerPedSlot::IsActive(i)) 
		{
			iNumPlayersInRange++;
		}
	}

	return iNumPlayersInRange;
}

uint32_t CLocalPlayer::GetPlayerColor()
{
	return CRadar::TranslateColorCodeToRGBA(CLocalPlayer::GetLocalID());
}

void CLocalPlayer::SetPlayerColor(uint32_t dwColor)
{
	CRadar::SetRadarColor(CLocalPlayer::GetLocalID(), dwColor);
}

uint32_t CLocalPlayer::GetPlayerColorAsARGB()
{
	return (CRadar::TranslateColorCodeToRGBA(CLocalPlayer::GetLocalID()) >> 8) | 0xFF000000;
}

void CLocalPlayer::SendTakeDamage(uint16_t PlayerID, float fDamageFactor, uint32_t weaponType, uint32_t pedPieceType)
{
	fDamageFactor *= 0.33000001;

	RakNet::BitStream bsSend;
	bsSend.Write(true);
	bsSend.Write(PlayerID);
	bsSend.Write(fDamageFactor);
	bsSend.Write((uint32_t)weaponType);
	bsSend.Write((uint32_t)pedPieceType);

	CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_PlayerGiveTakeDamage, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CLocalPlayer::SendGiveDamage(uint16_t PlayerID, float fDamageFactor, uint32_t weaponType, uint32_t pedPieceType)
{
	fDamageFactor *= 0.33000001;

	RakNet::BitStream bsSend;
	bsSend.Write(false);
	bsSend.Write(PlayerID);
	bsSend.Write(fDamageFactor);
	bsSend.Write((uint32_t)weaponType);
	bsSend.Write((uint32_t)pedPieceType);

	CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_PlayerGiveTakeDamage, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CLocalPlayer::SendGiveDamageActor(uint16_t ActorID, float fDamageFactor, uint32_t weaponType, uint32_t pedPieceType)
{
	RakNet::BitStream bsSend;
	bsSend.Write(false);
	bsSend.Write(ActorID);
	bsSend.Write(fDamageFactor);
	bsSend.Write(weaponType);
	bsSend.Write(pedPieceType);

	CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_GiveDamageActor, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}