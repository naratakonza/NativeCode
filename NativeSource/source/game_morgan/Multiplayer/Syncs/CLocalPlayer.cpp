#include "../../../../Main.h"
#include "CLocalPlayer.h"
#include "../../CGame.h"
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
	// CLocalPlayer::ApplySpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);

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

	// CLocalPlayer::ApplySpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);
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
	// memset(&m_aimData, 0, sizeof(AIM_SYNC_DATA));
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

        // CLocalPlayer::HandleDancing();

		// CLocalPlayer::GetPlayerPed()->ProcessCuffedControl();
		// CLocalPlayer::GetPlayerPed()->ProcessCarryControl();

		// CLocalPlayer::HandleUrinating();
		// CLocalPlayer::HandleHandsUp();
		// CLocalPlayer::HandleStuff();
        CLocalPlayer::DisplayZone(dwThisTick);

        // CLocalPlayer::GetPlayerPed()->ProcessDrunk();
		// CLocalPlayer::GetPlayerPed()->ResetPlayerTimeLastEaten();

		// CLocalPlayer::ProcessAnimation();

		// CLocalPlayer::HandleVehicleEntryTimeout(dwThisTick);

		// CCheckPoint::Process();

        CLocalPlayer::HandleInteriorChanging();
        // CLocalPlayer::UpdateWeapons();
		// CLocalPlayer::UpdateCameraTarget();
		// CLocalPlayer::SendStats(dwThisTick);

        if (CLocalPlayer::IsSpectating())
		{
			// CLocalPlayer::ProcessSpectating();
			CLocalPlayer::m_bPassengerDriveByMode = false;
		}
		else if (CLocalPlayer::GetPlayerPed()->IsInVehicle() && !CLocalPlayer::GetPlayerPed()->IsAPassenger())
		{
			// CLocalPlayer::ProcessDriver(dwThisTick);
		}
		else if (CLocalPlayer::GetPlayerPed()->GetActionTrigger() == eAction::ACTION_NORMAL || CLocalPlayer::GetPlayerPed()->GetActionTrigger() == eAction::ACTION_SCOPE)
		{
			CLocalPlayer::ProcessOnFoot(dwThisTick);
			// CLocalPlayer::ProcessAimSync(dwThisTick);
		}
		else if (CLocalPlayer::GetPlayerPed()->IsInVehicle() && CLocalPlayer::GetPlayerPed()->IsAPassenger())
		{
			// CLocalPlayer::ProcessPassenger(dwThisTick);
		}
    }

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

		// CLocalPlayer::ProcessSpectating();
		return true;
	}

    if (CLocalPlayer::m_bIsWasted && CLocalPlayer::GetPlayerPed()->GetActionTrigger() != eAction::ACTION_WASTED && CLocalPlayer::GetPlayerPed()->GetActionTrigger() != eAction::ACTION_DEATH)
	{
		CLocalPlayer::HandleNeedsToRespawn();
		return true;
	}

	CLocalPlayer::HandleInitialSpawnConditions();

	return true;
}

void CLocalPlayer::HandleNeedsToRespawn()
{
	// if (CLocalPlayer::GetPlayerPed()->HasAccessory())
	// {
	// 	CLocalPlayer::GetPlayerPed()->DeleteAllAccessories(true);
	// }

	// if (CLocalPlayer::GetPlayerPed()->IsCuffed())
	// {
	// 	CLocalPlayer::GetPlayerPed()->ToggleCuffed(false);
	// }

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

        if (/*CGui::GetButton() && CGui::GetDebug() && CGui::GetXSing() && CGui::GetGButton() &&*/ CGui::GetDialog())
		{
			// CGui::GetDebug()->SetRenderFramerate(false);
			// CGui::GetDebug()->SetRenderMemory(false);
			// CGui::GetButton()->Enable(false);
			// CGui::GetXSing()->Enable(false);
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

void CLocalPlayer::HandleDead()
{
	CLocalPlayer::EnableSpectating(false);

	// if (CLocalPlayer::GetPlayerPed()->IsDancing())
	// {
	// 	CLocalPlayer::GetPlayerPed()->StopDancing();
	// }

	// if (CLocalPlayer::GetPlayerPed()->IsCellphoneEnabled())
	// {
	// 	CLocalPlayer::GetPlayerPed()->ToggleCellphone(0);
	// }

	// if (CLocalPlayer::GetPlayerPed()->IsPissing())
	// {
	// 	CLocalPlayer::GetPlayerPed()->StopPissing();
	// }

	// if (CLocalPlayer::GetPlayerPed()->GetStuff()) 
	// {
	// 	CLocalPlayer::GetPlayerPed()->DropStuff();
	// }

	// CLocalPlayer::GetPlayerPed()->SetDrunkLevel(0);
	CLocalPlayer::GetPlayerPed()->LockControl(false);
	CLocalPlayer::GetPlayerPed()->ToggleControllable(true);

	// CLocalPlayer::UpdateRCMode(eRcModeType::RC_MODE_OFF);

	// if (CLocalPlayer::GetPlayerPed()->IsInVehicle() && !CLocalPlayer::GetPlayerPed()->IsAPassenger())
	// {
	// 	CLocalPlayer::SendInCarFullSyncData();
	// 	CLocalPlayer::m_LastVehicle = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool()->FindIDFromGtaPtr((CVehicleGta*)CLocalPlayer::GetPlayerPed()->GetVehicle());
	// }

	// CLocalPlayer::GetPlayerPed()->ExtinguishFire();
	// CLocalPlayer::SendWastedNotification();

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

	// if (CLocalPlayer::GetPlayerPed()->IsCuffed())
	// {
	// 	CLocalPlayer::GetPlayerPed()->ToggleCuffed(false);
	// }

	CLocalPlayer::GetPlayerPed()->RestartIfWastedAt(&m_SpawnInfo.vecPos, m_SpawnInfo.fRotation);
	CLocalPlayer::GetPlayerPed()->SetModelIndex(CLocalPlayer::m_SpawnInfo.iSkin);

	if (CRealTimeShadowManager::GetShadowStyle() == 1 && pthread_main_np() && CLocalPlayer::GetPlayerPed()->VerifyInstance())
	{
		auto* ped = CLocalPlayer::GetPlayerPed()->GetPed();
		auto* shadow = ped->m_pRealTimeShadow;
		if (shadow && shadow->m_pPhysical == ped)
		{
			CRealTimeShadowManager::ReturnRealTimeShadow(nullptr, shadow);
		}
	}

	CLocalPlayer::GetPlayerPed()->ClearAllWeapons();
	CLocalPlayer::GetPlayerPed()->ResetDamageEntity();

	// CLocalPlayer::ApplySpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);

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

    // if (CGui::GetButton() && CGui::GetDebug() && CGui::GetXSing() && CGui::GetGButton())
	// {
	// 	CGui::GetDebug()->SetRenderFramerate(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowFps")));
	// 	CGui::GetDebug()->SetRenderMemory(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowMem")));
	// 	CGui::GetButton()->Enable(true);
	// 	CGui::GetXSing()->Enable(true);
	// 	CGui::GetGButton()->Enable(true);
	// }

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

	// CLocalPlayer::ProcessHead();
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

    // เดียวมาทำต่อไอสัส
	// if (CLocalPlayer::GetPlayerPed()->IsCrouching()) 
	// {
	// 	ofSync.byteSpecialAction = eSpecialAction::SPECIAL_ACTION_DUCK;
	// }
	// else 
	// {
	// 	ofSync.byteSpecialAction = CLocalPlayer::GetSpecialAction();
	// }

    ofSync.byteSpecialAction = eSpecialAction::SPECIAL_ACTION_NONE;

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