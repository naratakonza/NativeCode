#include "../../../../Main.h"
#include "CRemotePlayer.h"
#include "Core/CHeadMove.h"
#include "Core/CMarkerIcon.h"
#include "../../CGame.h"
#include "../../CRadar.h"
#include "../../CUtil.h"
#include "../../CCommon.h"
#include "../../CCameraAim.h"
#include "../Guis/CGui.h"
#include "../CMultiPlayer.h"
#include "../Texts/CGameText.h"

static std::vector<int> camModes = {7, 8, 45, 46, 51, 65};

CRemotePlayer::CRemotePlayer(const char* szPlayerName, uint16_t playerId, bool bIsNPC)
{
    CRemotePlayer::ResetAll();

    CRemotePlayer::SetRemoteName(szPlayerName);
    CRemotePlayer::SetRemotePlayerID(playerId);
    CRemotePlayer::SetRemoteNPC(bIsNPC);
    CRemotePlayer::SetRemoteAFK(true);
	CRemotePlayer::SetRemoteShowNameTag(true);
    CRemotePlayer::SetRemotePing(0);
    CRemotePlayer::SetRemoteScore(0);
	CRemotePlayer::DestroyGlobalMarker();
}

CRemotePlayer::~CRemotePlayer()
{
	CRemotePlayer::DestroyGlobalMarker();
	CRemotePlayer::RemoveGamePlayer(1);
}

void CRemotePlayer::Process()
{
	if (CRemotePlayer::IsActive())
	{
		CRemotePlayer::UpdateOnceRequiredSync();
		CRemotePlayer::UpdateAllFrames();
	}
	else
	{
		CRemotePlayer::RemoveGamePlayer(3);
	}
}

void CRemotePlayer::ResetSpeedAndKeysForGamePlayer()
{
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
	if (!pGamePlayer) {
		return;
	}

	if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
		CNecessary::Log(OBFUSCATE("ResetSpeedAndKeysForGamePlayer: pGamePlayer is no longer valid."));
		return;
	}

	CVector vecMoveSpeed = CVector();
	CRemotePlayer::SetGamePlayerMoveSpeed(vecMoveSpeed);
	pGamePlayer->SetKeys(0, 0, 0);
	CRemotePlayer::SetRemotePassengerDriveBy(false);
}

void CRemotePlayer::UpdateOnceRequiredSync()
{
	if (CRemotePlayer::IsOnFootSync())
	{
		CRemotePlayer::DoOnFootSync();
	}
	// else if (CRemotePlayer::IsInCarSync())
	// {
	// 	CRemotePlayer::DoInCarSync();
	// }
	// else if (CRemotePlayer::IsPassengerSync())
	// {
	// 	CRemotePlayer::DoPassengerSync();
	// }

	CRemotePlayer::SetNetworkUpdateType(eNetworkUpdateType::UPDATE_TYPE_NONE);
}

void CRemotePlayer::UpdateAllFrames()
{
	if (CRemotePlayer::IsOnFootState())
	{
	    CRemotePlayer::DoOnFootFrame();
	}
	// else if (CRemotePlayer::IsInCarState())
	// {
	// 	CRemotePlayer::DoInCarFrame();
	// }
	// else if (CRemotePlayer::IsPassengerState())
	// {
	// 	CRemotePlayer::DoPassengerFrame();
	// }
	else
	{
		CRemotePlayer::ResetSpeedAndKeysForGamePlayer();
	}

	CRemotePlayer::ProcessGamePlayerHead();

	if ((CNecessary::GetTickCount() - CRemotePlayer::GetReceiveSyncTick()) < NETMODE_PLAYERS_UPDATE_DELAY / 2) 
	{
		CRemotePlayer::SetRemoteAFK(false);
	}

	if (CRemotePlayer::GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_WASTED)
	{
		CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
		if (!pGamePlayer) {
			return;
		}

		if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
			CNecessary::Log(OBFUSCATE("UpdateAllFrames: pGamePlayer is no longer valid."));
			return;
		}

		pGamePlayer->SetHealth(1000.0f);
	}

	CRemotePlayer::DoChatBubbleMessages();
}

void CRemotePlayer::DoChatBubbleMessages()
{
    if (!CRemotePlayer::m_bChatBubbleIsActive) {
        return;
    }

    if (CNecessary::GetTickCount() - CRemotePlayer::m_dwChatBubbleLastTick >= CRemotePlayer::m_dwChatBubbleCurDelay)
    {
        CRemotePlayer::m_bChatBubbleIsActive = false;
    }
}

CPlayerPed* CRemotePlayer::CreateGamePlayer(unsigned int iSkin, CVector *vecPos, float fRotation)
{
	CRemotePlayer::RemoveGamePlayer();

	CPlayerPed* pPlayer = CGame::NewPlayer(iSkin, vecPos->x, vecPos->y, vecPos->z, fRotation);

	if (pPlayer) 
    {
		CRemotePlayer::SetGamePlayer(pPlayer);
		return pPlayer;
	}

	return nullptr;
}

bool CRemotePlayer::SpawnGamePlayer(uint8_t byteTeam, unsigned int iSkin, CVector *vecPos, float fRotation, uint32_t dwColor, [[maybe_unused]] eFightingStyle byteFightingStyle, [[maybe_unused]] const uint16_t weaponSkills[11])
{
	bool IsStarted = CGame::IsStarted();
	if (IsStarted)
	{
		CPlayerPed* pGamePlayer = CRemotePlayer::CreateGamePlayer(iSkin, vecPos, fRotation);

		if (pGamePlayer)
		{
			if (dwColor != 0) 
			{
				CRemotePlayer::UpdateColorOnRadar(dwColor);
			}

			CRemotePlayer::DestroyGlobalMarker();

			if (CMultiPlayer::GetActiveNetwork()->m_iShowPlayerMarkers)
			{
				pGamePlayer->ShowMarker(CRemotePlayer::GetRemotePlayerID());
			}

			for (int i = 0; i < 11; i++)
			{
				pGamePlayer->SetWeaponSkill(i, weaponSkills[i]);
			}

			if (byteTeam != NO_TEAM)
			{
				CRemotePlayer::SetRemoteTeam(byteTeam);
			}

			CRemotePlayer::SetReportedHealth(100.0f);
			CRemotePlayer::SetReportedArmour(0.0f);

			if (CRemotePlayer::IsRemoteNPC())
			{
				pGamePlayer->SetImmunities(0, 1, 1, 0, 0);
			}

			if (byteFightingStyle != eFightingStyle::STYLE_STANDARD)
			{
				pGamePlayer->SetFightingStyle(static_cast<int>(byteFightingStyle));
			}

			if (CRemotePlayer::GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_SPAWNED)
			{
				CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_SPAWNED);
			}

			return true;
		}

		if (CRemotePlayer::GetNetworkPlayerState())
		{
			CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_NONE);
		}

		return false;
	}

	return IsStarted;
}

void CRemotePlayer::RemoveGamePlayer(uint8_t reset, bool bValid)
{
	CRemotePlayer::m_bChatBubbleIsActive = false;
	
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
	if (pGamePlayer)
	{
		if (bValid && !CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
			CNecessary::Log(OBFUSCATE("RemoveGamePlayer: pGamePlayer is no longer valid."));
			return;
		}

		if (reset == 3 || reset == 4) 
		{
			CRemotePlayer::ResetData();
		}

		CGame::RemovePlayer(pGamePlayer);
		CRemotePlayer::SetGamePlayer(nullptr);
	}

	if (reset == 1) 
	{
		CRemotePlayer::ResetAll();
	}

	if (reset == 2) 
	{
		CRemotePlayer::ResetAllSync();
	}

	if (reset == 4) 
	{
		CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_NONE);
		CRemotePlayer::SetRemoteShowNameTag(true);
	}
}

void CRemotePlayer::ResetData()
{
	// CRemotePlayer::SetRemoteVehicleID(0);
	CRemotePlayer::SetReportedHealth(0.0f);
	CRemotePlayer::SetReportedArmour(0.0f);
	// CRemotePlayer::SetGameVehicle(nullptr);
	// CRemotePlayer::SetVehicleSeatId(0);
	CRemotePlayer::SetRemotePassengerDriveBy(false);
	CRemotePlayer::SetSpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);
	CRemotePlayer::SetPlayingAnimIndex(0);
	CRemotePlayer::SetRemoteAFK(true);

	CRemotePlayer::ResetAllSync();
	CRemotePlayer::SetPlayerHeadDirectionPos(CVector());
	CRemotePlayer::DestroyGlobalMarker();
	CRemotePlayer::UpdateAllDataTime();
}

void CRemotePlayer::ResetAll()
{
    CRemotePlayer::SetRemoteName(OBFUSCATE(""));
	// CRemotePlayer::SetRemoteVehicleID(0);
    CRemotePlayer::SetRemotePlayerID(eInvalidId::INVALID_PLAYER_ID);
    CRemotePlayer::SetRemoteNPC(false);
	CRemotePlayer::SetRemoteAFK(true);
	CRemotePlayer::SetRemoteShowNameTag(true);
	CRemotePlayer::SetRemoteColor(0xE2C063FF);
	CRemotePlayer::SetRemoteTeam(NO_TEAM);
	CRemotePlayer::SetRemotePassengerDriveBy(false);
	CRemotePlayer::SetReportedHealth(0.0f);
	CRemotePlayer::SetReportedArmour(0.0f);
	CRemotePlayer::SetSpecialAction(eSpecialAction::SPECIAL_ACTION_NONE);
	CRemotePlayer::SetPlayingAnimIndex(0);
	CRemotePlayer::SetPerformingCustomAnimation(false);
	CRemotePlayer::SetMarkerID(eMarkerType::MARKER_TYPE_LOCAL, 0);
    CRemotePlayer::SetMarkerID(eMarkerType::MARKER_TYPE_GLOBAL, 0);
	CRemotePlayer::SetNetworkUpdateType(eNetworkUpdateType::UPDATE_TYPE_NONE);
	CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_NONE);
	CRemotePlayer::SetGamePlayer(nullptr);
	// CRemotePlayer::SetGameVehicle(nullptr);
	// CRemotePlayer::SetVehicleSeatId(0);
	CRemotePlayer::SetPlayerHeadDirectionPos(CVector());
	CRemotePlayer::SetPlayerTargetPos(CVector());
    CRemotePlayer::SetPlayerTargetSpeed(CVector());
	// CRemotePlayer::SetVehicleTargetPos(CVector());
    // CRemotePlayer::SetVehicleTargetSpeed(CVector());
	CRemotePlayer::ResetAllSync();
	CRemotePlayer::UpdateAllDataTime();
}

void CRemotePlayer::ResetAllSync()
{
    memset(&m_ofSyncData, 0, sizeof(m_ofSyncData));
	// memset(&m_icSyncData, 0, sizeof(m_icSyncData));
	// memset(&m_psSyncData, 0, sizeof(m_psSyncData));
	// memset(&m_aimSyncData, 0, sizeof(m_aimSyncData));
	// memset(&m_bulletSyncData, 0, sizeof(m_bulletSyncData));
	// memset(&m_trailerSyncData, 0, sizeof(m_trailerSyncData));
	// memset(&m_unoccupiedSyncData, 0, sizeof(m_unoccupiedSyncData));
}

void CRemotePlayer::UpdateAllDataTime()
{
	CRemotePlayer::UpdateReceiveDataTimeSyncTick();
}

void CRemotePlayer::HandleRemovingFromVehicle()
{
    CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (pGamePlayer)
    {
		if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
			CNecessary::Log(OBFUSCATE("HandleRemovingFromVehicle: pGamePlayer is no longer valid."));
			return;
		}

        if (pGamePlayer->IsInVehicle())
        {
            if (CRemotePlayer::GetSpecialAction() != eSpecialAction::SPECIAL_ACTION_ENTER_VEHICLE && CRemotePlayer::GetSpecialAction() != eSpecialAction::SPECIAL_ACTION_EXIT_VEHICLE && !pGamePlayer->IsExitingVehicle())
            {
                CRemotePlayer::RemoveGamePlayerFromVehicle();
            }
        }
    }
}

void CRemotePlayer::WritePlayerSyncData(ONFOOT_SYNC_DATA *syncData, uint32_t dwTime)
{
	// CRemotePlayer::SetGameVehicle(nullptr);

	if (dwTime == 0 || dwTime - CRemotePlayer::GetReceiveDataTimeSyncTick() >= 0)
    {
		CRemotePlayer::m_dwLastStoredSyncDataTime = dwTime;

		memcpy(&m_ofSyncData, syncData, sizeof(ONFOOT_SYNC_DATA));

		CRemotePlayer::SetReportedHealth(static_cast<float>(syncData->byteHealth));
		CRemotePlayer::SetReportedArmour(static_cast<float>(syncData->byteArmour));
		CRemotePlayer::SetNetworkUpdateType(eNetworkUpdateType::UPDATE_TYPE_ONFOOT);
		
		CRemotePlayer::UpdateReceiveSyncTick();
		
		CRemotePlayer::SetSpecialAction(static_cast<eSpecialAction>(syncData->byteSpecialAction));
		CRemotePlayer::HandlePlayerSyncAnimation();
		CRemotePlayer::HandleRemovingFromVehicle();
	}

	if (CRemotePlayer::GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_ONFOOT)
	{
		CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_ONFOOT);
	}
}

void CRemotePlayer::DoOnFootSync()
{
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (!pGamePlayer) {
        return;
    }

	if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
		CNecessary::Log(OBFUSCATE("DoOnFootSync: pGamePlayer is no longer valid."));
		return;
	}

	// if (!CRemotePlayer::IsSurfingOnVehicle())
	// {
	// 	CRemotePlayer::UpdatePlayerPositionAndSpeed(&m_ofSyncData.vecPos, &m_ofSyncData.vecMoveSpeed);
	// 	CRemotePlayer::UpdatePlayerTargetPosition();
	// }
	// else
	// {
	// 	CRemotePlayer::UpdatePlayerPositionAndSpeed(&m_ofSyncData.vecPos, &m_ofSyncData.vecMoveSpeed);
	// }

    CRemotePlayer::UpdatePlayerPositionAndSpeed(&m_ofSyncData.vecPos, &m_ofSyncData.vecMoveSpeed);
	CRemotePlayer::UpdatePlayerTargetPosition();

    CRemotePlayer::UpdateGamePlayerWeapon(static_cast<uint8_t>(m_ofSyncData.byteCurrentWeapon & 0x3F));

	if (CRemotePlayer::m_ofSyncData.wKeys < 0)
	{
		if (pGamePlayer->GetCurrentWeapon() == eWeaponSampId::WEAPON_SNIPER || pGamePlayer->GetCurrentWeapon() == eWeaponSampId::WEAPON_ROCKETLAUNCHER || pGamePlayer->GetCurrentWeapon() == eWeaponSampId::WEAPON_HEATSEEKER)
		{
			pGamePlayer->SetActionTrigger(eAction::ACTION_SCOPE);
		}
	}
	else
	{
		if (pGamePlayer->GetActionTrigger() == eAction::ACTION_SCOPE)
		{
			pGamePlayer->SetActionTrigger(eAction::ACTION_NORMAL);
		}
	}

	CRemotePlayer::ProcessGamePlayerAnimation();
	
	// CRemotePlayer::SetGameVehicle(nullptr);
}

void CRemotePlayer::UpdatePlayerTargetPosition()
{
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (!pGamePlayer || !pGamePlayer->GetEntity() || !pGamePlayer->HasMatrix()) {
        return;
    }

	if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
		CNecessary::Log(OBFUSCATE("UpdatePlayerTargetPosition: pGamePlayer is no longer valid."));
		return;
	}

	CMatrix mat;
	CVector vec;

    pGamePlayer->GetMatrix(&mat);

	if (!pGamePlayer->IsAdded())
	{
		mat.pos.x = CRemotePlayer::m_playerData.vecTargetPos.x;
		mat.pos.y = CRemotePlayer::m_playerData.vecTargetPos.y;
		mat.pos.z = CRemotePlayer::m_playerData.vecTargetPos.z;

		pGamePlayer->SetMatrix(mat);
		return;
	}

	m_vecPosOffset.x = CUtil::FloatOffset(CRemotePlayer::m_playerData.vecTargetPos.x, mat.pos.x);
	m_vecPosOffset.y = CUtil::FloatOffset(CRemotePlayer::m_playerData.vecTargetPos.y, mat.pos.y);
	m_vecPosOffset.z = CUtil::FloatOffset(CRemotePlayer::m_playerData.vecTargetPos.z, mat.pos.z);

	if (m_vecPosOffset.x > 0.00001f || m_vecPosOffset.y > 0.00001f || m_vecPosOffset.z > 0.00001f)
	{
		if (m_vecPosOffset.x > 2.0f || m_vecPosOffset.y > 2.0f || m_vecPosOffset.z > 1.0f)
		{
			mat.pos.x = CRemotePlayer::m_playerData.vecTargetPos.x;
			mat.pos.y = CRemotePlayer::m_playerData.vecTargetPos.y;
			mat.pos.z = CRemotePlayer::m_playerData.vecTargetPos.z;
			pGamePlayer->SetMatrix(mat);
			return;
		}

		pGamePlayer->GetMoveSpeedVector(&vec);
        vec.x += (m_vecPosOffset.x > 0.00001f) ? (CRemotePlayer::m_playerData.vecTargetPos.x - mat.pos.x) * 0.1f : 0.0f;
        vec.y += (m_vecPosOffset.y > 0.00001f) ? (CRemotePlayer::m_playerData.vecTargetPos.y - mat.pos.y) * 0.1f : 0.0f;
        vec.z += (m_vecPosOffset.z > 0.00001f) ? (CRemotePlayer::m_playerData.vecTargetPos.z - mat.pos.z) * 0.1f : 0.0f;

		CRemotePlayer::SetGamePlayerMoveSpeed(vec);
	}
}

void CRemotePlayer::DoOnFootFrame()
{
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
	if (!pGamePlayer) {
		return;
	}

	if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
		CNecessary::Log(OBFUSCATE("DoOnFootFrame: pGamePlayer is no longer valid."));
		return;
	}

	CMatrix matPlayer, matVehicle;
	CVector vecMoveSpeed = CVector();

	CRemotePlayer::UpdateGamePlayerRotation();

	CRemotePlayer::ProcessGamePlayerHead();
	CRemotePlayer::ProcessGamePlayerSpecialAction(CRemotePlayer::GetSpecialAction());

	CRemotePlayer::SetRemotePassengerDriveBy(false);

	pGamePlayer->SetKeys(CRemotePlayer::m_ofSyncData.wKeys, CRemotePlayer::m_ofSyncData.lrAnalog, CRemotePlayer::m_ofSyncData.udAnalog);

	// if (CRemotePlayer::IsSurfingOnVehicle())
	// {
	// 	if (CRemotePlayer::m_ofSyncData.wSurfInfo < 2000)
	// 	{
	// 		CRemotePlayer::GetGamePlayer()->SetGravityProcessing(false);
	// 		CRemotePlayer::GetGamePlayer()->SetCollisionChecking(false);
	// 	}
	// }
	// else
	// {
	// 	if (CRemotePlayer::m_ofSyncData.byteSpecialAction == eSpecialAction::SPECIAL_ACTION_ENTER_VEHICLE)
	// 	{
	// 		CRemotePlayer::GetGamePlayer()->SetGravityProcessing(false);
	// 		CRemotePlayer::GetGamePlayer()->SetCollisionChecking(false);
	// 	}
	// 	else
	// 	{
	// 		CRemotePlayer::GetGamePlayer()->SetGravityProcessing(true);
	// 		CRemotePlayer::GetGamePlayer()->SetCollisionChecking(true);
	// 	}
	// }

    if (CRemotePlayer::m_ofSyncData.byteSpecialAction == eSpecialAction::SPECIAL_ACTION_ENTER_VEHICLE)
	{
		CRemotePlayer::GetGamePlayer()->SetGravityProcessing(false);
		CRemotePlayer::GetGamePlayer()->SetCollisionChecking(false);
	}
	else
	{
		CRemotePlayer::GetGamePlayer()->SetGravityProcessing(true);
		CRemotePlayer::GetGamePlayer()->SetCollisionChecking(true);
	}

	if (CRemotePlayer::m_ofSyncData.vecMoveSpeed.x != 0.0f && CRemotePlayer::m_ofSyncData.vecMoveSpeed.y != 0.0f && CRemotePlayer::m_ofSyncData.vecMoveSpeed.z != 0.0f)
	{
		if ((CNecessary::GetTickCount() - CRemotePlayer::GetReceiveSyncTick()) > NETMODE_PLAYERS_UPDATE_DELAY / 2)
		{
			CRemotePlayer::SetRemoteAFK(true);
		}
	}
	else
	{
		CRemotePlayer::SetGamePlayerMoveSpeed(CVector());
		if ((CNecessary::GetTickCount() - CRemotePlayer::GetReceiveSyncTick()) > NETMODE_PLAYERS_UPDATE_DELAY)
		{
			CRemotePlayer::SetRemoteAFK(true);
		}
	}

	if (CRemotePlayer::IsRemoteAFK() && (CNecessary::GetTickCount() - CRemotePlayer::GetReceiveSyncTick()) > NETMODE_PLAYERS_UPDATE_DELAY)
	{
		CRemotePlayer::m_ofSyncData.lrAnalog = 0;
		CRemotePlayer::m_ofSyncData.udAnalog = 0;

		vecMoveSpeed.x = 0.0f;
		vecMoveSpeed.y = 0.0f;
		vecMoveSpeed.z = 0.0f;
		CRemotePlayer::SetGamePlayerMoveSpeed(vecMoveSpeed);

		pGamePlayer->GetMatrix(&matPlayer);
		matPlayer.pos.x = CRemotePlayer::m_ofSyncData.vecPos.x;
		matPlayer.pos.y = CRemotePlayer::m_ofSyncData.vecPos.y;
		matPlayer.pos.z = CRemotePlayer::m_ofSyncData.vecPos.z;
		pGamePlayer->SetMatrix(matPlayer);
	}
}

void CRemotePlayer::UpdateGamePlayerRotation()
{
    if (!CRemotePlayer::IsValidInstance()) {
        return;
    }

    CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (!pGamePlayer) {
        return;
    }

	if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
		CNecessary::Log(OBFUSCATE("UpdateGamePlayerRotation: pGamePlayer is no longer valid."));
		return;
	}

    if (!pGamePlayer->IsAdded()) {
        return;
    }

    CMatrix mat;
    pGamePlayer->GetMatrix(&mat);

    CQuaternion quatPlayer;
    quatPlayer.SetFromMatrix(mat);

    CQuaternion quatResult;
    quatResult.Slerp(&m_ofSyncData.quat, &quatPlayer, 0.75f);

    quatResult.GetMatrix(&mat);
    pGamePlayer->SetMatrix(mat);

    if (std::isnan(mat.up.x) || std::isnan(mat.up.y)) {
        CNecessary::Log(OBFUSCATE("CRemotePlayer: Invalid matrix values detected."));
        return;
    }

    float fZ = atan2(-mat.up.x, mat.up.y) * 57.295776f;
    if (fZ < 0.0f)
    {
        fZ += 360.0f;
    }
    else if (fZ >= 360.0f)
    {
        fZ -= 360.0f;
    }

    pGamePlayer->SetRotation(fZ);
}

void CRemotePlayer::UpdateGamePlayerWeapon(uint8_t byteCurWeapon, bool armed)
{
    if (!CRemotePlayer::IsValidInstance()) {
        return;
    }

    CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (!pGamePlayer) {
        return;
    }

	if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
		CNecessary::Log(OBFUSCATE("UpdateGamePlayerWeapon: pGamePlayer is no longer valid."));
		return;
	}

    if (!pGamePlayer->IsAdded()) {
        return;
    }

    if (pGamePlayer->GetCurrentWeapon() != byteCurWeapon)
    {
        pGamePlayer->SetArmedWeapon(byteCurWeapon, armed);
        if (pGamePlayer->GetCurrentWeapon() != byteCurWeapon)
        {
            pGamePlayer->GiveWeapon(byteCurWeapon, 9999);
            pGamePlayer->SetArmedWeapon(byteCurWeapon, armed);
        }
    }
}

void CRemotePlayer::ProcessGamePlayerHead()
{
    if (!CRemotePlayer::IsValidInstance()) {
        return;
    }

    CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (!pGamePlayer) {
        return;
    }

    if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
        CNecessary::Log(OBFUSCATE("ProcessGamePlayerHead: pGamePlayer is no longer valid."));
        return;
    }

    if (!pGamePlayer->IsAdded() || CRemotePlayer::IsRemoteNPC() || pGamePlayer->GetStuff()) {
        return;
    }

    if (!CHeadMove::GetHeadMove()) {
        return;
    }

    CMatrix mat;
    pGamePlayer->GetMatrix(&mat);

    if (CNecessary::GetTickCount() - CRemotePlayer::m_headData.m_dwLastHeadUpdate > 1000)
    {
        CAMERA_AIM* Aim = CCameraAim::GameGetRemotePlayerAim(pGamePlayer->m_bytePlayerNumber);

        float lookX = Aim->f1x * 20.0f + mat.pos.x;
        float lookY = Aim->f1y * 20.0f + mat.pos.y;
        float lookZ = Aim->f1z * 20.0f + mat.pos.z;

        CRemotePlayer::m_headData.m_vecHeadDir.x = lookX - mat.pos.x;
        CRemotePlayer::m_headData.m_vecHeadDir.y = lookY - mat.pos.y;
        CRemotePlayer::m_headData.m_vecHeadDir.z = lookZ - mat.pos.z;

        CRemotePlayer::m_headData.m_dwLastHeadUpdate = CNecessary::GetTickCount();
    }

    if (CNecessary::GetTickCount() - CRemotePlayer::m_headData.m_dwLastHeadLook > 200)
    {
        CVector LookAt;
        LookAt.x = mat.pos.x + CRemotePlayer::m_headData.m_vecHeadDir.x;
        LookAt.y = mat.pos.y + CRemotePlayer::m_headData.m_vecHeadDir.y;
        LookAt.z = mat.pos.z + CRemotePlayer::m_headData.m_vecHeadDir.z;

        pGamePlayer->ApplyCommandTask(OBFUSCATE("FollowPedSA"), 0, 1000, -1, &LookAt, 0, 0.1f, 500, 3, 0);
        CRemotePlayer::m_headData.m_dwLastHeadLook = CNecessary::GetTickCount();
    }
}

void CRemotePlayer::ProcessGamePlayerSpecialAction(eSpecialAction byteSpecialAction)
{
    if (!CRemotePlayer::IsValidInstance()) {
        return;
    }

    CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (pGamePlayer) 
    {
        if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
            CNecessary::Log(OBFUSCATE("ProcessGamePlayerSpecialAction: pGamePlayer is no longer valid."));
            return;
        }

        if (!pGamePlayer->IsAdded()) {
            return;
        }

        if (pGamePlayer->IsCellphoneEnabled() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_USECELLPHONE)
        {
            pGamePlayer->ToggleCellphone(0);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_USECELLPHONE && !pGamePlayer->IsCellphoneEnabled())
        {
            pGamePlayer->ToggleCellphone(1);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_STOPUSECELLPHONE && pGamePlayer->IsCellphoneEnabled())
        {
            pGamePlayer->ToggleCellphone(0);
        }
        
        if (pGamePlayer->IsInJetpack() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_USEJETPACK)
        {
            pGamePlayer->StopJetpack();
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_USEJETPACK && !pGamePlayer->IsInJetpack())
        {
            pGamePlayer->StartJetpack();
        }

        if (pGamePlayer->HasHandsUp() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_HANDSUP)
        {
            pGamePlayer->LockControl(false);
            pGamePlayer->ToggleControllable(true);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_HANDSUP && !pGamePlayer->HasHandsUp())
        {
            pGamePlayer->HandsUp();
        }

        if (pGamePlayer->IsPissing() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_URINATING)
        {
            pGamePlayer->StopPissing();
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_URINATING && !pGamePlayer->IsPissing())
        {
            pGamePlayer->StartPissing();
        }

        if (pGamePlayer->IsCrouching() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_DUCK)
        {
            pGamePlayer->ResetCrouch();
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DUCK && !pGamePlayer->IsCrouching())
        {
            pGamePlayer->ApplyCrouch();
        }

        if (pGamePlayer->IsCarry() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_CARRY && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_ENTER_VEHICLE && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_EXIT_VEHICLE)
        {
            pGamePlayer->ToggleCarry(false);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_CARRY && !pGamePlayer->IsCarry())
        {
            pGamePlayer->ToggleCarry(true);
        }

        if (pGamePlayer->IsCuffed() && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_CUFFED && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_ENTER_VEHICLE && byteSpecialAction != eSpecialAction::SPECIAL_ACTION_EXIT_VEHICLE)
        {
            pGamePlayer->ToggleCuffed(false);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_CUFFED && !pGamePlayer->IsCuffed())
        {
            pGamePlayer->ToggleCuffed(true);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DRINK_BEER && pGamePlayer->GetStuff() != eStuffType::STUFF_TYPE_BEER)
        {
            pGamePlayer->GiveStuff(eStuffType::STUFF_TYPE_BEER);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DRINK_WINE && pGamePlayer->GetStuff() != eStuffType::STUFF_TYPE_DYN_BEER)
        {
            pGamePlayer->GiveStuff(eStuffType::STUFF_TYPE_DYN_BEER);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DRINK_SPRUNK && pGamePlayer->GetStuff() != eStuffType::STUFF_TYPE_PINT_GLASS)
        {
            pGamePlayer->GiveStuff(eStuffType::STUFF_TYPE_PINT_GLASS);
        }

        if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_SMOKE_CIGGY && pGamePlayer->GetStuff() != eStuffType::STUFF_TYPE_CIGGI)
        {
            pGamePlayer->GiveStuff(eStuffType::STUFF_TYPE_CIGGI);
        }

        if (pGamePlayer->GetStuff())
        {
            if (byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DRINK_BEER || byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DRINK_WINE || byteSpecialAction == eSpecialAction::SPECIAL_ACTION_DRINK_SPRUNK || byteSpecialAction == eSpecialAction::SPECIAL_ACTION_SMOKE_CIGGY)
            {
                pGamePlayer->ApplyStuff();
            }
            else
            {
                pGamePlayer->DropStuff();
            }
        }

        if (pGamePlayer->GetCurrentWeapon() != eWeaponSampId::WEAPON_PARACHUTE)
        {
            pGamePlayer->DestroyParachute();
        }
    }
}

void CRemotePlayer::ProcessGamePlayerAnimation()
{
    if (!CRemotePlayer::IsValidInstance()) {
        return;
    }

    CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (pGamePlayer) 
    {
        if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
            CNecessary::Log(OBFUSCATE("ProcessGamePlayerAnimation: pGamePlayer is no longer valid."));
            return;
        }

        int32_t nAnimValue = CRemotePlayer::m_ofSyncData.dwAnimation;
        if (CRemotePlayer::GetAnimPlayingIndex() != nAnimValue)
        {
            if (nAnimValue != 0)
            {
                pGamePlayer->PlayAnimationByIndex(nAnimValue);
            }

            if (nAnimValue == 0 && CRemotePlayer::GetAnimPlayingIndex() == SYNC_ANIM_VEND_EAT_P)
            {
                pGamePlayer->ApplyAnimation(OBFUSCATE("VEND_EAT_P"), OBFUSCATE("VENDING"), 4.0, 0, 0, 0, 0, -1);
                CRemotePlayer::m_ofSyncData.dwAnimation = eInvalidId::INVALID_ID;
            }

            CRemotePlayer::SetPlayingAnimIndex(CRemotePlayer::m_ofSyncData.dwAnimation);
            CRemotePlayer::SetPerformingCustomAnimation(false);
        }

        if (!CRemotePlayer::IsPerformingCustomAnimation() && nAnimValue == 0 && pGamePlayer->IsPerformingCarryTask())
        {
            pGamePlayer->ApplyAnimation(OBFUSCATE("crry_prtial"), OBFUSCATE("CARRY"), 4.0f, 0, 0, 0, 0, 0);
        }
    }
}

void CRemotePlayer::UpdatePlayerPositionAndSpeed(CVector *vecPos, CVector *vecMove)
{
	CRemotePlayer::SetPlayerTargetPos(*vecPos);
	CRemotePlayer::SetPlayerTargetSpeed(*vecMove);
	CRemotePlayer::SetGamePlayerMoveSpeed(CRemotePlayer::m_playerData.vecTargetSpeed);
}

void CRemotePlayer::SetPlayerTargetPos(CVector pos) 
{
	CRemotePlayer::m_playerData.vecTargetPos = pos;
}

void CRemotePlayer::SetPlayerTargetSpeed(CVector speed) 
{
	CRemotePlayer::m_playerData.vecTargetSpeed = speed;
}

void CRemotePlayer::SetGamePlayerMoveSpeed(CVector vecSpeed)
{
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
    if (!pGamePlayer) {
        return;
    }

	pGamePlayer->SetMoveSpeedVector(vecSpeed);
}

void CRemotePlayer::SetPlayerHeadDirectionPos(CVector direction)
{
	CRemotePlayer::m_headData.m_vecHeadDir = direction;
}

void CRemotePlayer::SetCollisionCheckingState(bool state)
{
	CRemotePlayer::m_playerData.bCollisionChecking = state;
}

bool CRemotePlayer::GetCollisionCheckingState()
{
	return static_cast<bool>(CRemotePlayer::m_playerData.bCollisionChecking);
}

bool CRemotePlayer::IsValidInstance()
{
	if (CRemotePlayer::GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_NONE) {
		return false;
	}

	uint16_t playerId = CRemotePlayer::GetRemotePlayerID();
	if (playerId < eMaxItem::MAX_PLAYERS)
	{
		CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
		if (pGamePlayer != nullptr) 
		{
			eNetworkUpdateType updateType = CRemotePlayer::GetNetworkUpdateType();
			eNetworkPlayerState playerState = CRemotePlayer::GetNetworkPlayerState();

			if (updateType == eNetworkUpdateType::UPDATE_TYPE_ONFOOT || playerState == eNetworkPlayerState::PLAYER_STATE_ONFOOT) {
				return true;
			}

            // Vehicle
			// if (updateType == eNetworkUpdateType::UPDATE_TYPE_INCAR || updateType == eNetworkUpdateType::UPDATE_TYPE_PASSENGER || playerState == eNetworkPlayerState::PLAYER_STATE_DRIVER || playerState == eNetworkPlayerState::PLAYER_STATE_PASSENGER)
			// {
			// 	uint16_t vehicleId = CRemotePlayer::GetRemoteVehicleID();
			// 	if (vehicleId < eMaxItem::MAX_VEHICLES)
			// 	{
			// 		CVehicle* pVehicle = CRemotePlayer::GetGameVehicle();
			// 		return (pVehicle != nullptr);
			// 	}
			// }
		}
	}

	return false;
}

void CRemotePlayer::RemoveGamePlayerFromVehicle()
{
	if (CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer())
	{
		if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
			CNecessary::Log(OBFUSCATE("RemoveGamePlayerFromVehicle: pGamePlayer is no longer valid."));
			return;
		}

		if (pGamePlayer->IsInVehicle())
		{
			CMatrix mat;
			pGamePlayer->GetMatrix(&mat);
			pGamePlayer->RemoveFromVehicleAndPutAt(mat.pos.x, mat.pos.y, mat.pos.z);
			// CRemotePlayer::SetGameVehicle(nullptr);
		}
	}
}

void CRemotePlayer::HandleWasted()
{
	if (CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer())
	{
		if (!CRemotePlayer::IsGamePlayerValid(CRemotePlayer::GetRemotePlayerID(), pGamePlayer)) {
			CNecessary::Log(OBFUSCATE("HandleWasted: pGamePlayer is no longer valid."));
			return;
		}

		pGamePlayer->SetKeys(0, 0, 0);
		pGamePlayer->SetDead();
	}

	if (CRemotePlayer::GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_WASTED)
	{
		CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_WASTED);
	}

	CRemotePlayer::ResetData();
}

void CRemotePlayer::CreateGlobalMarker(short sX, short sY, short sZ)
{
	CRemotePlayer::DestroyGlobalMarker();

	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
	if (!pGamePlayer)
	{
		CRemotePlayer::SetMarkerID(eMarkerType::MARKER_TYPE_GLOBAL, CMarkerIcon::Create(0, static_cast<float>(sX), static_cast<float>(sY), static_cast<float>(sZ), CRemotePlayer::GetRemotePlayerID(), 0));
	}
}

void CRemotePlayer::DestroyGlobalMarker()
{
	if (CRemotePlayer::GetGlobalMarkerID())
	{
		CMarkerIcon::Destroy(CRemotePlayer::m_playerData.dwGlobalMarkerID);
		CRemotePlayer::SetMarkerID(eMarkerType::MARKER_TYPE_GLOBAL, 0);
	}
}

void CRemotePlayer::HandlePlayerSyncAnimation()
{
	if (m_ofSyncData.dwAnimation < 0)
	{
		m_ofSyncData.dwAnimation = 0;
	}
}

void CRemotePlayer::UpdateReceiveSyncTick()
{
    CRemotePlayer::m_dwLastReceiveSyncTick = static_cast<uint32_t>(CNecessary::GetTickCount());
}

uint32_t CRemotePlayer::GetReceiveSyncTick()
{
	return static_cast<uint32_t>(CRemotePlayer::m_dwLastReceiveSyncTick);
}

void CRemotePlayer::UpdateReceiveDataTimeSyncTick()
{
    CRemotePlayer::m_dwLastStoredSyncDataTime = static_cast<uint32_t>(CNecessary::GetTickCount());
}

uint32_t CRemotePlayer::GetReceiveDataTimeSyncTick()
{
	return static_cast<uint32_t>(CRemotePlayer::m_dwLastStoredSyncDataTime);
}

void CRemotePlayer::SetRemotePlayerID(uint16_t newPlayerId)
{
	CRemotePlayer::m_playerData.playerId = static_cast<uint16_t>(newPlayerId);
}

uint16_t CRemotePlayer::GetRemotePlayerID()
{
	return static_cast<uint16_t>(CRemotePlayer::m_playerData.playerId);
}

void CRemotePlayer::SetRemoteNPC(bool bNPC)
{
	CRemotePlayer::m_playerData.bIsNPC = static_cast<bool>(bNPC);
}

bool CRemotePlayer::IsRemoteNPC()
{
	return static_cast<bool>(CRemotePlayer::m_playerData.bIsNPC);
}

void CRemotePlayer::SetRemoteName(const char* newPlayerName)
{
	if (newPlayerName == nullptr || newPlayerName[0] == '\0')
	{
		memset(CRemotePlayer::m_playerData.szPlayerName, 0, sizeof(CRemotePlayer::m_playerData.szPlayerName));
		return;
	}

	CNecessary::ConvertLocalizedEncodingToUTF8(CRemotePlayer::m_playerData.szPlayerName, newPlayerName, sizeof(CRemotePlayer::m_playerData.szPlayerName));
}

char* CRemotePlayer::GetRemoteName()
{
	return reinterpret_cast<char*>(CRemotePlayer::m_playerData.szPlayerName);
}

void CRemotePlayer::SetRemoteAFK(bool bIsAFK) 
{
	CRemotePlayer::m_playerData.bIsAFK = static_cast<bool>(bIsAFK);
}

bool CRemotePlayer::IsRemoteAFK()
{
	return static_cast<bool>(CRemotePlayer::m_playerData.bIsAFK);
}

void CRemotePlayer::SetRemoteShowNameTag(bool bIsNameTag) 
{
	CRemotePlayer::m_playerData.bShowNameTag = static_cast<bool>(bIsNameTag);
}

bool CRemotePlayer::IsRemoteNameTag()
{
	return static_cast<bool>(CRemotePlayer::m_playerData.bShowNameTag);
}

void CRemotePlayer::SetReportedHealth(float fHealth) 
{
	CRemotePlayer::m_fReportedHealth = static_cast<float>(fHealth);
}

void CRemotePlayer::SetReportedArmour(float fArmour) 
{
	CRemotePlayer::m_fReportedArmour = static_cast<float>(fArmour);
}

float CRemotePlayer::GetReportedHealth() 
{
	return static_cast<float>(CRemotePlayer::m_fReportedHealth);
}

float CRemotePlayer::GetReportedArmour() 
{
	return static_cast<float>(CRemotePlayer::m_fReportedArmour);
}

void CRemotePlayer::SetRemoteTeam(uint8_t byteTeam)
{
	CRemotePlayer::m_playerData.byteTeam = static_cast<uint8_t>(byteTeam);
}

uint8_t CRemotePlayer::GetRemoteTeam()
{
	return static_cast<uint8_t>(CRemotePlayer::m_playerData.byteTeam);
}

void CRemotePlayer::SetRemotePassengerDriveBy(bool enable)
{
	CRemotePlayer::m_bPassengerDriveByMode = enable;
}

bool CRemotePlayer::GetRemotePassengerDriveBy()
{
	return CRemotePlayer::m_bPassengerDriveByMode;
}

void CRemotePlayer::SetRemoteColor(uint32_t dwColor)
{
	CRemotePlayer::m_playerData.dwColor = static_cast<uint32_t>(dwColor);
}

uint32_t CRemotePlayer::GetRemoteColor()
{
	return static_cast<uint32_t>(CRemotePlayer::m_playerData.dwColor);
}

uint32_t CRemotePlayer::GetPlayerColorAsARGB()
{
	return (CRadar::TranslateColorCodeToRGBA(CRemotePlayer::m_playerData.playerId) >> 8) | 0xFF000000;
}

void CRemotePlayer::UpdateColorOnRadar(uint32_t dwColor)
{
	CRadar::SetRadarColor(static_cast<int>(CRemotePlayer::m_playerData.playerId), static_cast<uint32_t>(dwColor));
	CRemotePlayer::SetRemoteColor(CRadar::TranslateColorCodeToRGBA(static_cast<int>(CRemotePlayer::m_playerData.playerId)));
}

void CRemotePlayer::SetSpecialAction(eSpecialAction byteSpecialAction) 
{
	CRemotePlayer::m_playerData.byteSpecialAction = static_cast<eSpecialAction>(byteSpecialAction);
}

eSpecialAction CRemotePlayer::GetSpecialAction() 
{
	return static_cast<eSpecialAction>(CRemotePlayer::m_playerData.byteSpecialAction);
}

bool CRemotePlayer::IsOnFootSync() 
{
	CPlayerPed* pGamePlayer = CRemotePlayer::GetGamePlayer();
	return CRemotePlayer::GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_ONFOOT && CRemotePlayer::GetNetworkUpdateType() == eNetworkUpdateType::UPDATE_TYPE_ONFOOT && !pGamePlayer->IsInVehicle();
}

void CRemotePlayer::SetNetworkUpdateType(eNetworkUpdateType type)
{
	CRemotePlayer::m_byteNetworkUpdateType = static_cast<eNetworkUpdateType>(type);
}

eNetworkUpdateType CRemotePlayer::GetNetworkUpdateType() 
{
	return static_cast<eNetworkUpdateType>(CRemotePlayer::m_byteNetworkUpdateType);
}

void CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState state) 
{
	CRemotePlayer::m_byteNetworkPlayerState = static_cast<eNetworkPlayerState>(state);
}

eNetworkPlayerState CRemotePlayer::GetNetworkPlayerState()
{
	return  static_cast<eNetworkPlayerState>(CRemotePlayer::m_byteNetworkPlayerState);
}

void CRemotePlayer::SetMarkerID(eMarkerType byteMarkerType, uint32_t dwMarkerID)
{
	switch (byteMarkerType) {
		case eMarkerType::MARKER_TYPE_LOCAL: {
			CRemotePlayer::m_playerData.dwLocalMarkerID = dwMarkerID;

			break;
		}

		case eMarkerType::MARKER_TYPE_GLOBAL: {
			CRemotePlayer::m_playerData.dwGlobalMarkerID = dwMarkerID;

			break;
		}

		default: {
			break;
		}
	}
}

uint32_t CRemotePlayer::GetGlobalMarkerID()
{
	return static_cast<uint32_t>(CRemotePlayer::m_playerData.dwGlobalMarkerID);
}

void CRemotePlayer::Say(unsigned char* szText)
{
	if (CGui::GetChat())
	{
		CGui::GetChat()->AddChatMessage(CRemotePlayer::GetRemoteName(), CRemotePlayer::GetRemoteColor(), reinterpret_cast<char*>(szText));
	}
}

void CRemotePlayer::AddBubbleMessage(uint32_t expireTime, const char* pText, uint32_t color, float drawDistance)
{
    CRemotePlayer::m_dwChatBubbleColor = color;
    CRemotePlayer::m_dwChatBubbleCurDelay = expireTime;

    CRemotePlayer::m_fChatBubbleDrawDistance = drawDistance;
    CNecessary::ConvertLocalizedEncodingToUTF8(CRemotePlayer::m_szChatBubbleMessage, pText, sizeof(CRemotePlayer::m_szChatBubbleMessage));

    CRemotePlayer::m_dwChatBubbleLastTick = CNecessary::GetTickCount();
    CRemotePlayer::m_bChatBubbleIsActive = true;
}

void CRemotePlayer::SetPlayingAnimIndex(int32_t Index)
{
	CRemotePlayer::m_usPlayingAnimIndex = Index;
}

int32_t CRemotePlayer::GetAnimPlayingIndex()
{
	return CRemotePlayer::m_usPlayingAnimIndex;
}

void CRemotePlayer::SetPerformingCustomAnimation(bool Enable)
{
	CRemotePlayer::m_bPerformingCustomAnimation = Enable;
}

bool CRemotePlayer::IsPerformingCustomAnimation()
{
	return CRemotePlayer::m_bPerformingCustomAnimation;
}

bool CRemotePlayer::IsActive() 
{
	if (CRemotePlayer::GetGamePlayer() && CRemotePlayer::GetNetworkPlayerState() != eNetworkPlayerState::PLAYER_STATE_NONE)
	{
		return true;
	}

	return false;
}

void CRemotePlayer::Deactivate()
{
	CRemotePlayer::SetNetworkPlayerState(eNetworkPlayerState::PLAYER_STATE_NONE);
}

void CRemotePlayer::SetGamePlayer(CPlayerPed* pGamePlayer) 
{
	CRemotePlayer::m_playerData.pGamePlayer = pGamePlayer;
}

CPlayerPed* CRemotePlayer::GetGamePlayer() 
{
	return CRemotePlayer::m_playerData.pGamePlayer;
}

bool CRemotePlayer::IsGamePlayerValid(uint16_t id, CPlayerPed* pGamePlayer, bool bDebug)
{
    CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    if (!pPlayerPool) {
        return false;
    }

    CRemotePlayer* pRemotePlayer = pPlayerPool->GetRemotePlayer(id);
	if (!pRemotePlayer) {
		return false;
	}

    if (pGamePlayer && pRemotePlayer->GetGamePlayer() != pGamePlayer)
    {
        if (bDebug)
		{
            // CExceptions::Print(OBFUSCATE("PlayerProcess"), OBFUSCATE("PlayerProcess"));
        }

        return false;
    }

    return true;
}

bool CRemotePlayer::IsOnFootState() 
{
	return CRemotePlayer::GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_ONFOOT && !CRemotePlayer::GetGamePlayer()->IsInVehicle();
}

bool CRemotePlayer::IsInCarState()
{
	return CRemotePlayer::GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_DRIVER && CRemotePlayer::GetGamePlayer()->IsInVehicle();
}

bool CRemotePlayer::IsPassengerState()
{
	return CRemotePlayer::GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_PASSENGER;
}