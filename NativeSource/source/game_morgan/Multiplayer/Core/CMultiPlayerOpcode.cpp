#include "../../../../Main.h"
#include "CMultiPlayerOpcode.h"
#include "RPCData/eRPCID.h"
#include "../../Enums/eConnectionRejectedType.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"
#include "../Guis/CGui.h"
#include "../../CGame.h"
#include "../../CClock.h"
#include "../../CTimer.h"
#include "../../CWeather.h"
#include "../../CTheZones.h"
#include "../../CEntryExitManager.h"
#include "../../Entity/CPhysical.h"

int bytesToBits(int bytes) 
{
	return bytes * 8;
}

void InitGame(RakNet::BitStream bsInitGame)
{
	CNecessary::Log(OBFUSCATE("RPC: InitGame"));

	uint16_t MyPlayerID;
	bool bLanMode, bStuntBonus;
	int iLagCompensation = 0;

    int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(bool)) +
		bytesToBits(sizeof(bool));

	if (bsInitGame.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for InitGameRPC"));
        return;
    }

	if (!CMultiPlayer::GetActiveNetwork()) {
		return;
	}

	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bZoneNames);							// unknown
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bUseCJWalk);							// native UsePlayerPedAnims(); +
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bAllowWeapons);						// native AllowInteriorWeapons(allow); +
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bLimitGlobalChatRadius);				// native LimitGlobalChatRadius(Float:chat_radius); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_fGlobalChatRadius);								// +
	bsInitGame.ReadCompressed(bStuntBonus);																// native EnableStuntBonusForAll(enable); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_fNameTagDrawDistance);							// native SetNameTagDrawDistance(Float:distance); +
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bDisableEnterExits);					// native DisableInteriorEnterExits(); +
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bNameTagLOS);							// native DisableNameTagLOS(); +
	CMultiPlayer::GetActiveNetwork()->m_bNameTagLOS = true;
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bManualVehicleEngineAndLight);		// native ManualVehicleEngineAndLights(); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iSpawnsAvailable);								// +
	bsInitGame.Read(MyPlayerID);																		// 
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bShowPlayerTags);						// native ShowNameTags(show); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iShowPlayerMarkers);							// native ShowPlayerMarkers(mode); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_byteWorldTime);									// native SetWorldTime(hour); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_byteWeather);									// native SetWeather(weatherid); +
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_fGravity);										// native SetGravity(Float:gravity); +
	bsInitGame.ReadCompressed(bLanMode);																// 
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iDeathDropMoney);								// native SetDeathDropAmount(amount); +
	bsInitGame.ReadCompressed(CMultiPlayer::GetActiveNetwork()->m_bInstagib);							// always 0

	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iNetModeNormalOnfootSendRate);
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iNetModeNormalInCarSendRate);
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iNetModeFiringSendRate);
	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iNetModeSendMultiplier);

	bsInitGame.Read(iLagCompensation);								// lagcomp +

	switch (iLagCompensation)
    {
		case 1: {
			CMultiPlayer::GetActiveNetwork()->m_iLagCompensation = 0;
            break;
		}

		case 2: {
			CMultiPlayer::GetActiveNetwork()->m_iLagCompensation = 1;
            break;
		}

		default: {
			CMultiPlayer::GetActiveNetwork()->m_iLagCompensation = 2;
			break;
		}
	}

	uint8_t byteStrLen;
	bsInitGame.Read(byteStrLen);
	if (byteStrLen)																						// SetGameModeText(); +
	{
		memset(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostName, 0, sizeof(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostName));
		bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostName, byteStrLen);
	}

	CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostName[byteStrLen] = '\0';

	uint8_t byteVehicleModels[212];
	bsInitGame.Read((char*)&byteVehicleModels[0], 212);

	// CGame::SetRequiredVehicleModels(byteVehicleModels); // ใน Mobile มันโหลด Model เยอะมาก เพราะเหมือนใน gta sa มันโหลดมา10-20คันแล้ว (รอตรวจสอบ)

	bsInitGame.Read(CMultiPlayer::GetActiveNetwork()->m_iVehicleFriendlyFire);

	if (bLanMode)
    {
        CMultiPlayer::GetActiveNetwork()->m_bLanMode = true;
    }

	CMultiPlayer::GetActiveNetwork()->InitialiseGameLogic(true);
    CPhysical::SetGravity(CMultiPlayer::GetActiveNetwork()->m_fGravity);

    if (CMultiPlayer::GetActiveNetwork()->m_bDisableEnterExits)
    {
		CEntryExitManager::DisableInteriorEnterExits();
	}

	// if (CMultiPlayer::GetActiveNetwork()->m_iVehicleFriendlyFire)
	// {
	// 	CHooks::InstallInflictDamage();
	// }

    CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_CONNECTED);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()) 
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->SetLocalID(MyPlayerID);
			CNecessary::Log(OBFUSCATE("LocalID : %d"), MyPlayerID);

			if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpectating()) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->HandleClassSelection();
			}
		}
	}

	// CGame::EnableStuntBonus(bStuntBonus);
    CClock::SetGameTime(CMultiPlayer::GetActiveNetwork()->m_byteWorldTime, 0);
	CWeather::SetGameWeather((eWeatherType)CMultiPlayer::GetActiveNetwork()->m_byteWeather);
    CPlayerPed::ProcessAnimGroups(CMultiPlayer::GetActiveNetwork()->m_bUseCJWalk);
    CTheZones::EnableZoneNames(CMultiPlayer::GetActiveNetwork()->m_bZoneNames);

    CNecessary::Log(OBFUSCATE("Connected to {B9C9BF}%.64s"), CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostName);

	if (CGui::GetChat())
	{
		CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Connected to {B9C9BF}%.64s"), CMultiPlayer::GetActiveNetwork()->GetNetworkData().HostName);
	}
}

void GameModeRestart()
{
	CNecessary::Log(OBFUSCATE("RPC: GameModeRestart"));

	CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_RESTARTING);
	CMultiPlayer::Reconnect();
}

void ConnectionRejected(RakNet::BitStream bsData)
{
	uint8_t byteRejectReason;

	bsData.Read(byteRejectReason);

	if (CGui::GetChat())
	{
		if (byteRejectReason == eConnectionRejectedType::REJECT_REASON_BAD_VERSION)
		{
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("CONNECTION REJECTED: Incorrect Version."));
		}
		else if (byteRejectReason == eConnectionRejectedType::REJECT_REASON_BAD_NICKNAME)
		{
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("CONNECTION REJECTED: Unacceptable NickName"));
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("Please choose another nick between and 3-20 characters"));
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("Please use only a-z, A-Z, 0-9"));
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("Use /q or /quit to exit or press ESC and select Quit Game"));
		}
		else if (byteRejectReason == eConnectionRejectedType::REJECT_REASON_BAD_MOD)
		{
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("CONNECTION REJECTED: Bad mod version."));
		}
		else if (byteRejectReason == eConnectionRejectedType::REJECT_REASON_BAD_PLAYERID)
		{
			CGui::GetChat()->AddInfoMessage(OBFUSCATE("CONNECTION REJECTED: Unable to allocate a player slot."));
		}

		if (CMultiPlayer::GetActiveNetwork())
		{
			CMultiPlayer::GetActiveNetwork()->GetRakClient()->Disconnect(500);
		}
	}
}

void ShowDialog(RakNet::BitStream bsDialog)
{
    CNecessary::Log(OBFUSCATE("RPC: ShowDialog"));

	if (!CGui::GetDialog()) {
		return;
	}

	int bitsTotal =
		bytesToBits(sizeof(int16_t)) +
		bytesToBits(sizeof(uint8_t) * 4);

	if (bsDialog.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ShowDialog"));
        return;
    }

	int16_t wDialogID = -1;
	uint8_t byteDialogStyle = 0;
	uint8_t len;

	char szBuff[4096 + 1] = { '\0' };
    char title[256 + 1]   = { '\0' };
    char info[4096 + 1]   = { '\0' };
    char button1[256 + 1] = { '\0' };
    char button2[256 + 1] = { '\0' };

	CGui::GetDialog()->Close();

	bsDialog.Read(wDialogID);

	if (wDialogID < 0) {
		return;
	}

	bsDialog.Read(byteDialogStyle);
	CGui::GetDialog()->m_wDialogID = wDialogID;
	CGui::GetDialog()->m_byteDialogStyle = byteDialogStyle;

	bsDialog.Read(len);
	bsDialog.Read(szBuff, len);

	szBuff[len] = '\0';
	CNecessary::ConvertLocalizedEncodingToUTF8(title, szBuff, sizeof(title));
	CNecessary::ConvertLocalizedEncodingToUTF8(CGui::GetDialog()->m_utf8Title, szBuff, sizeof(CGui::GetDialog()->m_utf8Title));

	bsDialog.Read(len);
	bsDialog.Read(szBuff, len);
	szBuff[len] = '\0';
	CNecessary::ConvertLocalizedEncodingToUTF8(button1, szBuff, sizeof(button1));
	CNecessary::ConvertLocalizedEncodingToUTF8(CGui::GetDialog()->m_utf8Button1, szBuff, sizeof(CGui::GetDialog()->m_utf8Button1));

	bsDialog.Read(len);
	bsDialog.Read(szBuff, len);
	szBuff[len] = '\0';
	CNecessary::ConvertLocalizedEncodingToUTF8(button2, szBuff, sizeof(button2));
	CNecessary::ConvertLocalizedEncodingToUTF8(CGui::GetDialog()->m_utf8Button2, szBuff, sizeof(CGui::GetDialog()->m_utf8Button2));

	stringCompressor->DecodeString(szBuff, 4096, &bsDialog);
	CNecessary::ConvertLocalizedEncodingToUTF8(info, szBuff, sizeof(info));
	CGui::GetDialog()->SetInfo(szBuff, CNecessary::MPSTRLEN(szBuff));

	CGui::GetDialog()->Show();
}

void SetPlayerPos(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerPos"));

	CVector vecPos;
	bsData.Read(vecPos.x);
	bsData.Read(vecPos.y);
	bsData.Read(vecPos.z);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed())
		{
			// CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->EndSurfing();
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->TeleportTo(vecPos.x, vecPos.y, vecPos.z);
		}
	}
}

void SetPlayerFacingAngle(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerFacingAngle"));

	int bitsTotal =
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerFacingAngle"));
        return;
    }

	float fAngle;

	bsData.Read(fAngle);

	if (CGame::GetPlayer()) 
	{
		CGame::GetPlayer()->ForceTargetRotation(fAngle);
	}
}

void SetCameraPos(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetCameraPos"));

	CVector vecPos = CVector();
	bsData.Read(vecPos.x);
	bsData.Read(vecPos.y);
	bsData.Read(vecPos.z);

	if (CGame::GetCamera())
    {
		CGame::GetCamera()->SetPosition(vecPos.x, vecPos.y, vecPos.z, 0.0f, 0.0f, 0.0f);
	}
}

void SetCameraLookAt(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetCameraLookAt"));

	CVector vecPos = CVector();
	uint8_t mode;

	bsData.Read(vecPos.x);
	bsData.Read(vecPos.y);
	bsData.Read(vecPos.z);

	bsData.Read(mode);
	if (mode < 1 || mode > 2)
	{
		mode = 2;
	}

	if (CGame::GetCamera())
    {
		CGame::GetCamera()->LookAtPoint(vecPos.x, vecPos.y, vecPos.z, mode);
	}
}

void SetCameraBehindPlayer()
{
	CNecessary::Log(OBFUSCATE("RPC: SetCameraBehindPlayer"));

	if (CGame::GetCamera())
    {
		CGame::GetCamera()->SetBehindPlayer();
	}
}

void InterpolateCamera(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: InterpolateCamera"));

    bool bSetPos = true;
    CVector vecFrom, vecDest;
    int time;
    uint8_t mode;

    bsData.Read(bSetPos);
    bsData.Read(vecFrom.x);
    bsData.Read(vecFrom.y);
    bsData.Read(vecFrom.z);
    bsData.Read(vecDest.x);
    bsData.Read(vecDest.y);
    bsData.Read(vecDest.z);
    bsData.Read(time);
    bsData.Read(mode);

    if (mode < 1 || mode > 2)
	{
        mode = 2;
    }

	if (CGame::GetCamera())
    {
		if (time > 0)
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->m_bSpectateProcessed = true;
			}

			if (bSetPos)
			{
				CGame::GetCamera()->InterpolateCameraPos(&vecFrom, &vecDest, time, mode);
			}
			else
			{
				CGame::GetCamera()->InterpolateCameraLookAt(&vecFrom, &vecDest, time, mode);
			}
		}
	}
}

void TogglePlayerSpectating(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: TogglePlayerSpectating"));

	if (bsData.GetNumberOfUnreadBits() < 32) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for TogglePlayerSpectating"));
		return;
	}

	uint32_t bToggle;
	bsData.Read(bToggle);
	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->EnableSpectating(bToggle);
		}
	}
}

void SetInterior(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetInterior"));

	if (bsData.GetNumberOfUnreadBits() < bytesToBits(sizeof(uint8_t))) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetInterior"));
        return;
    }

	uint8_t byteInterior;
	bsData.Read(byteInterior);

	if (CGame::GetPlayer())
    {
		CGame::GetPlayer()->SetInterior(byteInterior, true);
	}
}

void ClientMessage(RakNet::BitStream bsClientMessage)
{
    uint32_t dwStrLen;
    uint32_t dwColor;

    bsClientMessage.Read(dwColor);
    bsClientMessage.Read(dwStrLen);

	if (dwStrLen <= 0xFF)
	{
		char* szMsg = (char*)malloc(dwStrLen + 1);
		bsClientMessage.Read(szMsg, dwStrLen);
		szMsg[dwStrLen] = 0;

		if (CGui::GetChat()) 
		{
			CGui::GetChat()->AddClientMessage(dwColor, szMsg);
		}
	}
}

void SetGravity(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetGravity"));

	int bitsTotal =
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetGravity"));
        return;
    }

	float gravity;
	bsData.Read(gravity);
	CPhysical::SetGravity(gravity);
}

void ToggleClock(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ToggleClock"));

	int bitsTotal =
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ToggleClock"));
        return;
    }

	uint8_t toggle;
	bsData.Read(toggle);

	if (toggle)
	{
		CMultiPlayer::GetActiveNetwork()->m_bHoldTime = false;
	}
	else
	{
		CMultiPlayer::GetActiveNetwork()->m_bHoldTime = true;
		CClock::GetWorldTime((int*)&CMultiPlayer::GetActiveNetwork()->m_byteWorldTime, (int*)&CMultiPlayer::GetActiveNetwork()->m_byteWorldMinute);
	}
}

void RequestSpawn(RakNet::BitStream bsRS)
{
	CNecessary::Log(OBFUSCATE("RPC: RequestSpawn"));

	uint8_t byteRequestOutcome = 0;
	bsRS.Read(byteRequestOutcome);

	CNecessary::Log(OBFUSCATE("RPC: byteRequestOutcome = %d"), byteRequestOutcome);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
		{
			if (byteRequestOutcome == 2 || (byteRequestOutcome && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->m_bWaitingForSpawnRequestReply))
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->Spawn();
			}
			else
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->m_bWaitingForSpawnRequestReply = false;
			}
		}
	}
}

void RequestClass(RakNet::BitStream bsRC)
{
	CNecessary::Log(OBFUSCATE("RPC: RequestClass"));

    uint8_t byteRequestOutcome = 0;
    bsRC.Read(byteRequestOutcome);

	PLAYER_SPAWN_INFO SpawnInfo;

	if (bsRC.GetNumberOfUnreadBits() < bytesToBits(sizeof(PLAYER_SPAWN_INFO))) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetSpawnInfo (RequestClass)"));
		return;
	}

	bsRC.Read((char*)&SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) {
		return;
	}

	if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()) {
		return;
	}

    if (byteRequestOutcome) 
	{
        CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->SetSpawnInfo(&SpawnInfo);
        CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->HandleClassSelectionOutcome(true);
    }
	else
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->HandleClassSelectionOutcome(false);
	}
}

void ForceSpawnSelection()
{
	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()) 
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->ReturnToClassSelection();
		}
	}
}

void SetSpawnInfo(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetSpawnInfo"));

	PLAYER_SPAWN_INFO SpawnInfo;

	if (bsData.GetNumberOfUnreadBits() < bytesToBits(sizeof(PLAYER_SPAWN_INFO))) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetSpawnInfo"));
        return;
    }

	bsData.Read((char*)&SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()) 
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->SetSpawnInfo(&SpawnInfo);
		}
	}
}

void SetPlayerSkin(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerSkin"));

	int iPlayerID;
	unsigned int uiSkin;

	int bitsTotal =
		bytesToBits(sizeof(int)) +
		bytesToBits(sizeof(unsigned int));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerSkin"));
        return;
    }

	if (!bsData.Read(iPlayerID) || !bsData.Read(uiSkin)) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerSkin (2)"));
		return;
	}

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) 
	{
		if (iPlayerID == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID()) 
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->SetModelIndex(uiSkin);
			}
		} 
		else
		{
			// if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(iPlayerID) && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(iPlayerID)->GetGamePlayer()) 
			// {
			// 	CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(iPlayerID)->GetGamePlayer()->SetModelIndex(uiSkin);
			// }
		}
	}
}

void SetPlayerPosFindZ(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerPosFindZ"));

	alignas(float) VECTOR vecPos = VECTOR();
	if (bsData.GetNumberOfUnreadBits() >= 96) 
	{
		bsData.Read(vecPos.x);
		bsData.Read(vecPos.y);
		bsData.Read(vecPos.z);
	} 
	else 
	{
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerPosFindZ"));
	}

	vecPos.z = CGame::FindGroundZForCoord(vecPos.x, vecPos.y, vecPos.z);
	vecPos.z += 1.5f;

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()) 
		{
			// CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->EndSurfing();
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->TeleportTo(vecPos.x, vecPos.y, vecPos.z);
		}
	}
}

void SetPlayerHealth(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerHealth"));

	int bitsTotal =
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerHealth"));
        return;
    }

	if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) {
		return;
	}

	if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()) {
		return;
	}

	float fHealth;
	if (!bsData.Read(fHealth)) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerHealth (2)"));
		return;
	}

	if (fHealth > 8000000 && fHealth <= 8000100)
	{
		float fHealthFix = static_cast<int>(fHealth) % 1000;
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->SetHealth(fHealthFix);
	}
	else
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->SetHealth(fHealth);
	}
}

void SetPlayerArmour(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerArmour"));

	int bitsTotal =
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerArmour"));
        return;
    }

	if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) {
		return;
	}

	if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()) {
		return;
	}

	float fArm;
	if (!bsData.Read(fArm)) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerArmour (2)"));
		return;
	}

	CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->SetArmour(fArm);
}

void TogglePlayerControllable(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: TogglePlayerControllable"));

	int bitsTotal =
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for TogglePlayerControllable"));
        return;
    }

	uint8_t byteControllable;
	bsData.Read(byteControllable);
	if (CGame::GetPlayer())
	{
		CGame::GetPlayer()->LockControl(!byteControllable);
		CGame::GetPlayer()->ToggleControllable(static_cast<int>(byteControllable));
	}
}

void SetGameTime(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetGameTime"));

	uint8_t byteHour;
	bsData.Read(byteHour);

	if (CMultiPlayer::GetActiveNetwork())
	{
		CMultiPlayer::GetActiveNetwork()->m_byteWorldTime = byteHour;
	}
}

void SetPlayerTime(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerTime"));

	uint8_t byteHour;
	uint8_t byteMinute;

	bsData.Read(byteHour);
	bsData.Read(byteMinute);

	if (CMultiPlayer::GetActiveNetwork())
	{
		CMultiPlayer::GetActiveNetwork()->m_byteWorldTime = byteHour;
		CMultiPlayer::GetActiveNetwork()->m_byteWorldMinute = byteMinute;
		CClock::SetGameTime(byteHour, byteMinute);
	}
}

void SetGameWeather(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetGameWeather"));

	uint8_t byteWeather;

	bsData.Read(byteWeather);

	if (CMultiPlayer::GetActiveNetwork())
	{
		CMultiPlayer::GetActiveNetwork()->m_byteWeather = byteWeather;
		CWeather::SetGameWeather(static_cast<eWeatherType>(byteWeather));
	}
}

void SendGameTimeUpdate(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SendGameTimeUpdate"));

	int32_t time = 0;

	bsData.Read(time);
	
	CTimer::SetGameTimer(time);
}

bool CMultiPlayerOpcode::Receive(RakNet::BitStream* bsData, uint8_t rpcid)
{
    if (rpcid >= 255) {
		return false;
	}

    if (!CMultiPlayer::IsSampActive()) {
		return false;
	}

    switch (rpcid)
	{
        case RPC_InitGame: {
		    InitGame(*bsData);
		    break;
		}

		// case RPC_ServerJoin: {
		//     ServerJoin(*bsData);
		//     break;
		// }

		// case RPC_ServerQuit: {
		//     ServerQuit(*bsData);
		//     break;
		// }

		case RPC_GameModeRestart: {
			GameModeRestart();
			break;
		}

		case RPC_ConnectionRejected: {
			ConnectionRejected(*bsData);
			break;
		}

		// case RPC_Chat: {
		//     Chat(*bsData);
		//     break;
		// }

        case RPC_ShowDialog: {
		    ShowDialog(*bsData);
		    break;
		}

		case RPC_SetPlayerPos: {
		    SetPlayerPos(*bsData);
		    break;
		}

		case RPC_SetPlayerFacingAngle: {
		    SetPlayerFacingAngle(*bsData);
		    break;
		}

		case RPC_SetPlayerCameraPos: {
		    SetCameraPos(*bsData);
		    break;
		}

		case RPC_SetPlayerCameraLookAt: {
		    SetCameraLookAt(*bsData);
		    break;
		}

		case RPC_SetCameraBehindPlayer: {
		    SetCameraBehindPlayer();
		    break;
		}

		case RPC_InterpolateCamera: {
		    InterpolateCamera(*bsData);
		    break;
		}

		// case RPC_AttachCameraToObject: {
		//     AttachCameraToObject(*bsData);
		//     break;
		// }

		case RPC_TogglePlayerSpectating: {
		    TogglePlayerSpectating(*bsData);
		    break;
		}

		case RPC_SetPlayerInterior: {
		    SetInterior(*bsData);
		    break;
		}

		// case RPC_SetPlayerColor: {
		//     SetPlayerColor(*bsData);
		//     break;
		// }

		case RPC_SendClientMessage: {
		    ClientMessage(*bsData);
		    break;
		}

		// case RPC_SetPlayerShopName: {
		//     SetShopName(*bsData);
		//     break;
		// }

		// case RPC_ScmEvent: {
		// 	ScmEvent(*bsData);
		// 	break;
		// }

		// case RPC_SetPlayerVelocity: {
		// 	SetPlayerVelocity(*bsData);
		// 	break;
		// }

		// case RPC_ShowMenu: {
		// 	ShowMenu(*bsData);
		// 	break;
		// }

		// case RPC_HideMenu: {
		// 	HideMenu(*bsData);
		// 	break;
		// }

		// case RPC_InitMenu: {
		// 	InitMenu(*bsData);
		// 	break;
		// }

		// case RPC_ToggleWidescreen: {
		// 	ToggleWidescreen(*bsData);
		// 	break;
		// }

		// case RPC_SetTargeting: {
		// 	ToggleCameraTarget(*bsData);
		// 	break;
		// }

		// case RPC_ShowGameText: {
		// 	ShowGameText(*bsData);
		// 	break;
		// }

		// case RPC_CreateExplosion: {
		// 	CreateExplosion(*bsData);
		// 	break;
		// }

		case RPC_SetGravity: {
			SetGravity(*bsData);
			break;
		}

		// case RPC_SetPlayerWantedLevel: {
		// 	SetPlayerWantedLevel(*bsData);
		// 	break;
		// }

		// case RPC_ChatBubble: {
		// 	ChatBubble(*bsData);
		// 	break;
		// }

		case RPC_ToggleClock: {
			ToggleClock(*bsData);
			break;
		}

		// case RPC_SetPlayerMapIcon: {
		//     SetMapIcon(*bsData);
		//     break;
		// }

		// case RPC_RemovePlayerMapIcon: {
		//     RemoveMapIcon(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerWorldBounds: {
		//     SetWorldBounds(*bsData);
		//     break;
		// }

		// case RPC_GivePlayerMoney: {
		//     GivePlayerMoney(*bsData);
		//     break;
		// }

		// case RPC_ResetPlayerMoney: {
		//     ResetPlayerMoney(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerTeam: {
		//     SetPlayerTeam(*bsData);
		//     break;
		// }

		// case RPC_ShowActor: {
		//     ShowActor(*bsData);
		//     break;
		// }

		// case RPC_HideActor: {
		//     HideActor(*bsData);
		//     break;
		// }

		// case RPC_SetActorHealth: {
		//     SetActorHealth(*bsData);
		//     break;
		// }

		// case RPC_SetActorFacingAngle: {
		//     SetActorFacingAngle(*bsData);
		//     break;
		// }

		// case RPC_SetActorPos: {
		//     SetActorPos(*bsData);
		//     break;
		// }

		// case RPC_ApplyActorAnimation: {
		//     ApplyActorAnimation(*bsData);
		//     break;
		// }

		// case RPC_ClearActorAnimation: {
		//     ClearActorAnimation(*bsData);
		//     break;
		// }

		// case RPC_PlayCrimeReport: {
		// 	PlayCrimeReport(*bsData);
		// 	break;
		// }

		// case RPC_PlayAudioStream: {
		// 	PlayAudioStream(*bsData);
		// 	break;
		// }

		// case RPC_StopAudioStream: {
		// 	StopAudioStream(*bsData);
		// 	break;
		// }

		// case RPC_PlaySound: {
		// 	PlaySound(*bsData);
		// 	break;
		// }

		case RPC_RequestSpawn: {
		    RequestSpawn(*bsData);
		    break;
		}

		case RPC_RequestClass: {
			RequestClass(*bsData);
			break;
		}

		case RPC_ForceClassSelection: {
		    ForceSpawnSelection();
		    break;
		}

		case RPC_SetSpawnInfo: {
		    SetSpawnInfo(*bsData);
		    break;
		}

		// case RPC_WorldPlayerAdd: {
		//     WorldPlayerAdd(*bsData);
		//     break;
		// }

		// case RPC_WorldPlayerRemove: {
		//     WorldPlayerRemove(*bsData);
		//     break;
		// }

		// case RPC_WorldVehicleAdd: {
		//     WorldVehicleAdd(*bsData);
		//     break;
		// }

		// case RPC_WorldVehicleRemove: {
		//     WorldVehicleRemove(*bsData);
		//     break;
		// }

		// case RPC_LinkVehicleToInterior: {
		//     LinkVehicleToInterior(*bsData);
		//     break;
		// }

		// case RPC_SetVehicleHealth: {
		//     SetVehicleHealth(*bsData);
		//     break;
		// }

		// case RPC_SetVehiclePos: {
		//     SetVehiclePos(*bsData);
		//     break;
		// }

		// case RPC_SetVehicleVelocity: {
		//     SetVehicleVelocity(*bsData);
		//     break;
		// }

		// case RPC_SetVehicleZAngle: {
		//     SetVehicleZAngle(*bsData);
		//     break;
		// }

		// case RPC_PutPlayerInVehicle: {
		//     PutPlayerInVehicle(*bsData);
		//     break;
		// }

		// case RPC_DamageVehicle: {
		//     DamageVehicle(*bsData);
		//     break;
		// }

		// case RPC_SetVehicleTireStatus: {
		//     SetVehicleTireStatus(*bsData);
		//     break;
		// }

		// case RPC_EnterVehicle: {
		// 	EnterVehicle(*bsData);
		// 	break;
		// }

		// case RPC_ExitVehicle: {
		// 	ExitVehicle(*bsData);
		// 	break;
		// }

		// case RPC_RemoveVehicleComponent: {
		//     RemoveVehicleComponent(*bsData);
		//     break;
		// }

		// case RPC_RemovePlayerFromVehicle: {
		//     RemovePlayerFromVehicle(*bsData);
		//     break;
		// }

		// case RPC_AttachTrailerToVehicle: {
		// 	AttachTrailerToVehicle(*bsData);
		// 	break;
		// }

		// case RPC_DetachTrailerFromVehicle: {
		// 	DetachTrailerFromVehicle(*bsData);
		// 	break;
		// }

		// case RPC_VehicleParamsEx: {
		//     SetVehicleParamsEx(*bsData);
		//     break;
		// }

		// case RPC_SetVehicleParamsForPlayer: {
		//     SetVehicleParams(*bsData);
		//     break;
		// }

		// case RPC_DisableVehicleCollisions: {
		//     DisableVehicleCollisions(*bsData);
		//     break;
		// }

		// case RPC_StopObjectCameraCollision: {
		//     StopObjectCameraCollision(*bsData);
		//     break;
		// }

		// case RPC_WorldPlayerDeath: {
		//     DeathBroadcast(*bsData);
		//     break;
		// }

		// case RPC_DeathMessage: {
		//     SendDeathMessage(*bsData);
		//     break;
		// }

		// case RPC_SetVehicleNumberPlate: {
		//     SetVehicleNumberPlate(*bsData);
		//     break;
		// }

		// case RPC_SpectatePlayer: {
		//     SpectatePlayer(*bsData);
		//     break;
		// }

		// case RPC_SpectateVehicle: {
		//     SpectateVehicle(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerFightingStyle: {
		//     SetPlayerFightingStyle(*bsData);
		//     break;
		// }

		case RPC_SetPlayerSkin: {
		    SetPlayerSkin(*bsData);
		    break;
		}

		case RPC_SetPlayerPosFindZ: {
		    SetPlayerPosFindZ(*bsData);
		    break;
		}

		case RPC_SetPlayerHealth: {
		    SetPlayerHealth(*bsData);
		    break;
		}

		case RPC_SetPlayerArmour: {
		    SetPlayerArmour(*bsData);
		    break;
		}

		// case RPC_CreateObject: {
		//     CreateObject(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerObjectMaterial: {
		//     SetObjectMaterial(*bsData);
		//     break;
		// }

		// case RPC_SetObjectPos: {
		//     SetObjectPos(*bsData);
		//     break;
		// }

		// case RPC_SetObjectRot: {
		//     SetObjectRotation(*bsData);
		//     break;
		// }

		// case RPC_DestroyObject: {
		//     DestroyObject(*bsData);
		//     break;
		// }

		// case RPC_MoveObject: {
		//     MoveObject(*bsData);
		//     break;
		// }

		// case RPC_StopObject: {
		//     StopObject(*bsData);
		//     break;
		// }

		case RPC_TogglePlayerControllable: {
		    TogglePlayerControllable(*bsData);
		    break;
		}

		// case RPC_AttachObjectToPlayer: {
		//     AttachObjectToPlayer(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerAttachedObject: {
		//     SetPlayerAttachedObject(*bsData);
		//     break;
		// }

		case RPC_SetWorldTime: {
		    SetGameTime(*bsData);
		    break;
		}

		case RPC_SetPlayerTime: {
		    SetPlayerTime(*bsData);
		    break;
		}

		case RPC_SetPlayerWeather: {
		    SetGameWeather(*bsData);
		    break;
		}

		// case RPC_ShowPlayerNameTag: {
		//     ShowPlayerNameTag(*bsData);
		//     break;
		// }

		// case RPC_ApplyPlayerAnimation: {
		//     ApplyAnimation(*bsData);
		//     break;
		// }

		// case RPC_ClearPlayerAnimation: {
		//     ClearAnimations(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerName: {
		//     SetPlayerName(*bsData);
		//     break;
		// }

		case RPC_PlayerUpdate: {
		    SendGameTimeUpdate(*bsData);
		    break;
		}

		// case RPC_GivePlayerWeapon: {
		//     GivePlayerWeapon(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerAmmo: {
		//     SetWeaponAmmo(*bsData);
		//     break;
		// }

		// case RPC_SetArmedWeapon: {
		//     SetArmedWeapon(*bsData);
		//     break;
		// }

		// case RPC_ResetPlayerWeapons: {
		//     ResetPlayerWeapons(*bsData);
		//     break;
		// }

		// case RPC_UpdateScoresPingsIps: {
		//     UpdateScoresPingsIPs(*bsData);
		//     break;
		// }

		// case RPC_RemoveBuildingForPlayer: {
		//     RemoveBuilding(*bsData);
		//     break;
		// }

		// case RPC_ClickTextdraw: {
		//     ToggleSelectTextDraw(*bsData);
		//     break;
		// }

		// case RPC_ShowTextDraw: {
		//     ShowTextDraw(*bsData);
		//     break;
		// }

		// case RPC_HideTextDraw: {
		//     HideTextDraw(*bsData);
		//     break;
		// }

		// case RPC_TextDrawSetString: {
		//     TextDrawSetString(*bsData);
		// 	break;
		// }

		// case RPC_Create3DTextLabel: {
		//     Create3DTextLabel(*bsData);
		//     break;
		// }

		// case RPC_Update3DTextLabel: {
		//     Destroy3DTextLabel(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerSpecialAction: {
		//     SetPlayerSpecialAction(*bsData);
		//     break;
		// }

		// case RPC_CreatePickup: {
		//     CreatePickup(*bsData);
		//     break;
		// }

		// case RPC_DestroyPickup: {
		//     DestroyPickup(*bsData);
		//     break;
		// }

		// case RPC_SetCheckpoint: {
		//     SetCheckpoint(*bsData);
		//     break;
		// }

		// case RPC_DisableCheckpoint: {
		//     DisableCheckpoint(*bsData);
		//     break;
		// }

		// case RPC_SetRaceCheckpoint: {
		//     SetRaceCheckpoint(*bsData);
		//     break;
		// }

		// case RPC_DisableRaceCheckpoint: {
		//     DisableRaceCheckpoint(*bsData);
		//     break;
		// }

		// case RPC_AddGangZone: {
		//     AddGangZone(*bsData);
		//     break;
		// }

		// case RPC_GangZoneDestroy: {
		//     GangZoneDestroy(*bsData);
		//     break;
		// }

		// case RPC_GangZoneFlash: {
		//     GangZoneFlash(*bsData);
		//     break;
		// }

		// case RPC_GangZoneStopFlash: {
		//     GangZoneStopFlash(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerSkillLevel: {
		// 	SetPlayerSkillLevel(*bsData);
		// 	break;
		// }

		// case RPC_SetPlayerDrunkLevel: {
		//     SetPlayerDrunkLevel(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerDrunkVisuals: {
		//     SetPlayerDrunkVisuals(*bsData);
		//     break;
		// }

		// case RPC_SetPlayerDrunkHandling: {
		//     SetPlayerDrunkHandling(*bsData);
		//     break;
		// }

		// case RPC_EnableStuntBonus: {
		//     EnableStuntBonus(*bsData);
		//     break;
		// }

		// case RPC_EnterEditObject: {
		//     EnterEditObject(*bsData);
		//     break;
		// }

		// case RPC_CancelEdit: {
		// 	CancelEdit(*bsData);
		// 	break;
		// }

		// case RPC_EditAttachedObject: {
		// 	EditAttachedObject(*bsData);
		// 	break;
		// }

		// case RPC_EditObject: {
		// 	EditObject(*bsData);
		// 	break;
		// }
    }

    return true;
}
