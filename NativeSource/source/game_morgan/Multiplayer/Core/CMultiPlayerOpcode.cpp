#include "../../../../Main.h"
#include "../Syncs/Core/CCheckPoint.h"
#include "CMultiPlayerOpcode.h"
#include "RPCData/eRPCID.h"
#include "../../Enums/eConnectionRejectedType.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"
#include "../Guis/CGui.h"
#include "../Pools/Process/CMapIconPool.h"
#include "../../CGame.h"
#include "../../CClock.h"
#include "../../CTimer.h"
#include "../../CWeather.h"
#include "../../CTheZones.h"
#include "../../CEntryExitManager.h"
#include "../../Entity/CPhysical.h"
#include "../Audio/CAudio.h"
#include "../Texts/CGameText.h"

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

void ServerJoin(RakNet::BitStream bsData)
{
	//CNecessary::Log(OBFUSCATE("RPC: ServerJoin"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ServerJoin"));
        return;
    }

	char szPlayerName[eMaxItem::MAX_PLAYER_NAME + 1];
	uint16_t playerId;
	uint8_t byteNameLen = 0;
	uint8_t bIsNPC = 0;
	uint32_t color;

	bsData.Read(playerId);
	bsData.Read(color);
	bsData.Read(bIsNPC);
	bsData.Read(byteNameLen);

	if (byteNameLen >= 24 + 1) {
		CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ServerJoin (2)"));
		return;
	}

	bsData.Read(szPlayerName, byteNameLen);
	szPlayerName[byteNameLen] = '\0';

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->Add(playerId, szPlayerName, bIsNPC, color);
	}
}

void ServerQuit(RakNet::BitStream bsData)
{
	//CNecessary::Log(OBFUSCATE("RPC: ServerQuit"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ServerQuit"));
        return;
    }

	uint16_t playerId;
	uint8_t byteReason;

	bsData.Read(playerId);
    bsData.Read(byteReason);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->Delete(playerId, byteReason);
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

void Chat(RakNet::BitStream bsChat)
{
	CNecessary::Log(OBFUSCATE("RPC: Chat"));

    uint16_t playerId;
    uint8_t byteTextLen;

    bsChat.Read(playerId);
    bsChat.Read(byteTextLen);

    uint8_t szText[256];
    memset(szText, 0, 256);
    bsChat.Read((char*)szText, byteTextLen);
    szText[byteTextLen] = '\0';

    if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
		return;
	}

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
        if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
		{
            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID() == playerId)
			{
                CGui::GetChat()->AddChatMessage(CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName, CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerColor(), (char*)szText);
            }
			else
			{
                if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId))
				{
                    CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->Say((uint8_t*)szText);
                }
            }
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

void SetPlayerVelocity(RakNet::BitStream bsData)
{
    CNecessary::Log(OBFUSCATE("RPC: SetPlayerVelocity"));

    CVector vecMoveSpeed;

    bsData.Read((char*)&vecMoveSpeed, sizeof(CVector));

    CPlayerPed *pPlayer = CGame::GetPlayer();
    if (pPlayer)
    {
        if (pPlayer->IsOnGround())
        {
            uint32_t dwStateFlags = pPlayer->GetStateFlags();
            pPlayer->SetStateFlags(dwStateFlags ^ 3);
        }

        pPlayer->SetMoveSpeedVector(vecMoveSpeed);
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

void ToggleWidescreen(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ToggleWidescreen"));

	if (bsData.GetNumberOfUnreadBits() < bytesToBits(sizeof(uint8_t))) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ToggleWidescreen"));
        return;
    }

	uint8_t enable;

	bsData.Read(enable);

	if (CGui::GetChat())
	{
		CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("Widescreen = %i"), enable);
	}

	CGame::ToggleWidescreen(enable);
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

void SetPlayerColor(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerColor"));

	if (bsData.GetNumberOfUnreadBits() < bytesToBits(sizeof(uint16_t)) + bytesToBits(sizeof(uint32_t))) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerColor"));
        return;
    }

	uint16_t playerId;
	uint32_t dwColor;

	bsData.Read(playerId);
	bsData.Read(dwColor);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
		{
			if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->SetPlayerColor(dwColor);
			} 
			else 
			{
				if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)) 
				{
					CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->UpdateColorOnRadar(dwColor);
				}
			}
		}
	}
}

void ShowGameText(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ShowGameText"));

	char szMessage[512]{};
	int iType;
	int iTime;
	int iLength = 0;

	bsData.Read(iType);
	bsData.Read(iTime);

	bsData.Read(iLength);
	if (static_cast<std::size_t>(iLength) >= ARRAY_SIZE(szMessage)) {
		return;
	}

	bsData.Read(szMessage, iLength);
	szMessage[iLength] = '\0';

	CGameText::Print(szMessage, iTime, (eMessageStyle)iType);
}

void CreateExplosion(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: CreateExplosion"));

	float X, Y, Z, Radius;
	uint32_t iType;

	bsData.Read(X);
	bsData.Read(Y);
	bsData.Read(Z);
	bsData.Read(iType);
	bsData.Read(Radius);

	CGame::CreateExplosion(X, Y, Z, iType, Radius);
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

void ChatBubble(RakNet::BitStream bsData)
{
    CNecessary::Log(OBFUSCATE("RPC: ChatBubble"));

    uint16_t playerId;

    uint32_t color;
    uint32_t expireTime;
    uint8_t texLength;

    float drawDistance;

    bsData.Read(playerId);
    bsData.Read(color);
    bsData.Read(drawDistance);
    bsData.Read(expireTime);
    bsData.Read(texLength);

    if (texLength >= 255) {
        return;
    }

    char pText[256]{};
    bsData.Read(pText, texLength);

    pText[texLength] = 0;

    if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
    {
        return;
    }

    if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)) {
        return;
    }

    CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->AddBubbleMessage(expireTime, pText, color, drawDistance);
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

void SetMapIcon(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetMapIcon"));

	int bitsTotal =
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(CVector)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetMapIcon"));
        return;
    }

	uint8_t byteIndex, byteIcon;
	uint32_t dwColor;
	CVector vecPos;
	uint8_t byteStyle;

	bsData.Read(byteIndex);
	bsData.Read((char*)&vecPos, sizeof(CVector));
	bsData.Read(byteIcon);
	bsData.Read(dwColor);
	bsData.Read(byteStyle);

	CMapIconPool::Add(byteIndex, vecPos.x, vecPos.y, vecPos.z, byteIcon, dwColor, byteStyle);
}

void RemoveMapIcon(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: RemoveMapIcon"));

	int bitsTotal =
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for RemoveMapIcon"));
        return;
    }

	uint8_t byteIndex;

	bsData.Read(byteIndex);

	CMapIconPool::Delete(byteIndex);
}

void GivePlayerMoney(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: GivePlayerMoney"));

	int bitsTotal =
		bytesToBits(sizeof(int));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for GivePlayerMoney"));
        return;
    }

	int iMoney;
	bsData.Read(iMoney);
	CGame::AddPlayerMoney(iMoney);
}

void ResetPlayerMoney(RakNet::BitStream)
{
	CNecessary::Log(OBFUSCATE("RPC: ResetPlayerMoney"));

	CGame::ResetPlayerMoney();
}

void PlayAudioStream(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: PlayAudioStream"));

	uint8_t byteTextLen;
	char szURL[256]{};
	float X, Y, Z;
	float fRadius;
	uint8_t bUsePos;

	if (!bsData.Read(byteTextLen) || !byteTextLen || !bsData.Read(szURL, byteTextLen) || std::memchr(szURL, '\0', byteTextLen) || !bsData.Read(X) || !bsData.Read(Y) || !bsData.Read(Z) || !bsData.Read(fRadius) || !bsData.Read(bUsePos)) {
		return;
	}

	if (CAudio::GetAudioStream())
	{
		CAudio::GetAudioStream()->Play(szURL, X, Y, Z, fRadius, bUsePos != 0);
	}
}

void StopAudioStream(RakNet::BitStream)
{
	CNecessary::Log(OBFUSCATE("RPC: StopAudioStream"));

	if (CAudio::GetAudioStream())
	{
		CAudio::GetAudioStream()->Stop(false);
	}
}

void PlaySound(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: PlaySound"));

	uint32_t iSound;
	float fX, fY, fZ;
	bsData.Read(iSound);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	CAudio::Play(iSound, fX, fY, fZ);
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

void WorldPlayerAdd(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: WorldPlayerAdd"));

	int bitsStandard =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(CVector)) +
		bytesToBits(sizeof(float)) +
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsStandard) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for WorldPlayerAdd"));
        return;
    }

	uint16_t playerId;
	uint8_t byteTeam;
	uint32_t iSkin;
	CVector vecPos;
	float fRotation;
	uint32_t dwColor;
	uint8_t byteFightingStyle;
	uint16_t weaponSkills[11] = { 0 };

	bsData.Read(playerId);
	bsData.Read(byteTeam);
	bsData.Read(iSkin);
	bsData.Read((char*)&vecPos, sizeof(CVector));
	bsData.Read(fRotation);
	bsData.Read(dwColor);
	bsData.Read(byteFightingStyle);

	int bitsSkills = bytesToBits(sizeof(uint16_t) * 11);
	if (bsData.GetNumberOfUnreadBits() >= bitsSkills)
	{
		bsData.Read((char*)weaponSkills, sizeof(weaponSkills));
	}

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId))
		{
			if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->SpawnGamePlayer(byteTeam, iSkin, &vecPos, fRotation, dwColor, static_cast<eFightingStyle>(byteFightingStyle), weaponSkills))
			{
				if (CGui::GetChat())
				{
					CGui::GetChat()->AddDebugMessage(1, OBFUSCATE("Warning: Couldn't spawn player(%u)"), playerId);
				}
			}
		}
	}
}

void WorldPlayerRemove(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: WorldPlayerRemove"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for WorldPlayerRemove"));
        return;
    }

	uint16_t playerId;

	bsData.Read(playerId);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)) 
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->RemoveGamePlayer(4);
		}
	}
}

void SetPlayerTeam(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerTeam"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerTeam"));
        return;
    }

	uint16_t playerId;
	uint8_t byteTeamId;

	bsData.Read(playerId);
	bsData.Read(byteTeamId);

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
		{
			if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
			{
				if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
				{
					CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->SetLocalTeam(byteTeamId);
				}
			}
			else
			{
				if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId))
				{
					CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->SetRemoteTeam(byteTeamId);
				}
			}
		}
	}
}

void ShowActor(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ShowActor"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(VECTOR)) +
		bytesToBits(sizeof(float) * 2) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ShowActor"));
        return;
    }

	uint16_t actorId;
	uint32_t iSkinId;
	VECTOR vecPos;
	float fRotation, fHealth;
	uint8_t bInvulnerable;

	bsData.Read(actorId);
	bsData.Read(iSkinId);
	bsData.Read((char*)&vecPos, sizeof(VECTOR));
	bsData.Read(fRotation);
	bsData.Read(fHealth);
	bsData.Read(bInvulnerable);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->Add(actorId, iSkinId, vecPos, fRotation, fHealth, bInvulnerable);
	}
}

void HideActor(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: HideActor"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for HideActor"));
        return;
    }

	uint16_t actorId;

	bsData.Read(actorId);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->Delete(actorId, true);
	}
}

void SetActorHealth(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetActorHealth"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetActorHealth"));
        return;
    }

	uint16_t actorId;
	float fHealth;

	bsData.Read(actorId);
	bsData.Read(fHealth);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActorInSlot(actorId)) 
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)->SetHealth(fHealth);
			}
		}
	}
}

void SetActorFacingAngle(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetActorFacingAngle"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetActorFacingAngle"));
        return;
    }

	uint16_t actorId;
	float fAngle;

	bsData.Read(actorId);
	bsData.Read(fAngle);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActorInSlot(actorId)) 
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)->SetAngle(fAngle);
			}
		}
	}
}

void SetActorPos(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetActorPos"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(VECTOR));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetActorPos"));
        return;
    }

	uint16_t actorId;
	VECTOR vecPos;

	bsData.Read(actorId);
	bsData.Read((char*)&vecPos, sizeof(VECTOR));

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActorInSlot(actorId)) 
		{
			if(CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)->SetPosition(vecPos);
			}
		}
	}
}

void ApplyActorAnimation(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ApplyActorAnimation"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(float)) +
		bytesToBits(sizeof(bool)) +
		bytesToBits(sizeof(bool)) +
		bytesToBits(sizeof(bool)) +
		bytesToBits(sizeof(bool)) +
		bytesToBits(sizeof(uint32_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ApplyActorAnimation"));
        return;
    }

	uint16_t actorId;
	uint8_t uiAnimLibLen;
	char szAnimLib[64] = { '\0' };
	uint8_t uiAnimNameLen;
	char szAnimName[64] = { '\0' };
	float fDelta;
	bool bLoop;
	bool bLockX;
	bool bLockY;
	bool bFreeze;
	uint32_t uiTime;

	bsData.Read(actorId);

	bsData.Read(uiAnimLibLen);
	if (uiAnimLibLen < 0 || uiAnimLibLen >= sizeof(szAnimLib)) {
		return;
	}

	bsData.Read(szAnimLib, uiAnimLibLen);
	szAnimLib[uiAnimLibLen] = '\0';

	bsData.Read(uiAnimNameLen);
	if (uiAnimNameLen < 0 || uiAnimNameLen >= sizeof(szAnimName)) {
		return;
	}

	bsData.Read(szAnimName, uiAnimNameLen);
	szAnimName[uiAnimNameLen] = '\0';

	bsData.Read(fDelta);
	bsData.Read(bLoop);
	bsData.Read(bLockX);
	bsData.Read(bLockY);
	bsData.Read(bFreeze);
	bsData.Read(uiTime);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActorInSlot(actorId)) 
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)->ApplyAnimation(szAnimName, szAnimLib, fDelta, bLoop, bLockX, bLockY, bFreeze, uiTime);
			}
		}
	}
}

void ClearActorAnimation(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ClearActorAnimation"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ClearActorAnimation"));
        return;
    }

	uint16_t actorId;

	bsData.Read(actorId);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActorInSlot(actorId)) 
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetActorPool()->GetActor(actorId)->ClearAnimations();
			}
		}
	}
}

void SetPlayerFightingStyle(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerFightingStyle"));

	uint16_t playerId;
	uint8_t byteFightingStyle = static_cast<uint8_t>(eFightingStyle::STYLE_STANDARD);

	bsData.Read(playerId);
	bsData.Read(byteFightingStyle);

	CPlayerPed* pPlayerPed = nullptr;

	if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
	{
		pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
	}
	else
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId))
		{
			pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetGamePlayer();
		}
	}

	if (pPlayerPed)
	{
		pPlayerPed->SetFightingStyle(byteFightingStyle);
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
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(iPlayerID) && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(iPlayerID)->GetGamePlayer()) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(iPlayerID)->GetGamePlayer()->SetModelIndex(uiSkin);
			}
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

void ShowPlayerNameTag(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ShowPlayerNameTag"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ShowPlayerNameTag"));
        return;
    }

	uint16_t playerId;
	uint8_t show;

	bsData.Read(playerId);
	bsData.Read(show);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)) 
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->SetRemoteShowNameTag(show);
		}
	}
}

void ApplyAnimation(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ApplyAnimation"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(float)) +
		bytesToBits(sizeof(bool) * 4) +
		bytesToBits(sizeof(uint32_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ApplyAnimation"));
        return;
    }

	uint16_t playerId;
	uint8_t uiAnimLibLen;
	char szAnimLib[64] = { '\0' };
	uint8_t uiAnimNameLen;
	char szAnimName[64] = { '\0' };
	float fDelta;
	bool bLoop;
	bool bLockX;
	bool bLockY;
	bool bFreeze;
	uint32_t uiTime;

	bsData.Read(playerId);

	bsData.Read(uiAnimLibLen);
	if (uiAnimLibLen < 0 || uiAnimLibLen >= sizeof(szAnimLib)) {
		return;
	}

	bsData.Read(szAnimLib, uiAnimLibLen);
	szAnimLib[uiAnimLibLen] = '\0';

	bsData.Read(uiAnimNameLen);
	if (uiAnimNameLen < 0 || uiAnimNameLen >= sizeof(szAnimName)) {
		return;
	}

	bsData.Read(szAnimName, uiAnimNameLen);
	szAnimName[uiAnimNameLen] = '\0';

	bsData.Read(fDelta);
	bsData.Read(bLoop);
	bsData.Read(bLockX);
	bsData.Read(bLockY);
	bsData.Read(bFreeze);
	bsData.Read(uiTime);

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
		{
			CPlayerPed *pPlayerPed = (CPlayerPed*)nullptr;
			if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
			{
				pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
			}
			else if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(playerId))
			{
				pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetGamePlayer();
			}

			if (pPlayerPed)
			{
				pPlayerPed->ApplyAnimation(szAnimName, szAnimLib, fDelta, (int)bLoop, (int)bLockX, (int)bLockY, (int)bFreeze, (int)uiTime);
			}
		}
	}
}

void ClearAnimations(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ClearAnimations"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for ClearAnimations"));
        return;
    }

	uint16_t playerId;
	bsData.Read(playerId);

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
		{
			CPlayerPed *pPlayerPed = (CPlayerPed*)nullptr;
			if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
			{
				pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
			}
			else if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(playerId))
			{
				pPlayerPed = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetGamePlayer();
			}

			if (pPlayerPed) 
			{
				pPlayerPed->ClearAnimations();
			}
		}
	}
}

void SetPlayerName(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerName"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint8_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerName"));
        return;
    }

	uint16_t playerId;
	uint8_t byteNickLen = 0;
	char szNewName[24 + 1] = { '\0' };
	uint8_t byteSuccess;

	bsData.Read(playerId);
	bsData.Read(byteNickLen);
	if (byteNickLen >= eMaxItem::MAX_PLAYER_NAME + 1) {
		return;
	}

	bsData.Read(szNewName, byteNickLen);
	bsData.Read(byteSuccess);
	szNewName[byteNickLen] = '\0';

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		CNecessary::Log(OBFUSCATE("RPC: SetPlayerName byteSuccess = %d"), byteSuccess);
		if (byteSuccess == 1)
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId))
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->SetRemoteName(szNewName);
			}
		}

		if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
		{
			strcpy(CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName, szNewName);
		}
	}
}

void SendGameTimeUpdate(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SendGameTimeUpdate"));

	int32_t time = 0;

	bsData.Read(time);
	
	CTimer::SetGameTimer(time);
}

void GivePlayerWeapon(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: GivePlayerWeapon"));

	int bitsTotal =
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(uint32_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for GivePlayerWeapon"));
        return;
    }

	uint32_t iWeaponID;
	uint32_t iAmmo;
	bsData.Read(iWeaponID);
	bsData.Read(iAmmo);

	if (CGame::GetPlayer())
	{
		CGame::GetPlayer()->GiveWeapon(iWeaponID, iAmmo);
	}
}

void SetWeaponAmmo(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetWeaponAmmo"));

	int bitsTotal =
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetWeaponAmmo"));
        return;
    }

	uint8_t bWeaponID;
	uint16_t wAmmo;
	bsData.Read(bWeaponID);
	bsData.Read(wAmmo);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->SetAmmo(bWeaponID, wAmmo);
	}
}

void SetArmedWeapon(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetArmedWeapon"));

	int bitsTotal =
		bytesToBits(sizeof(uint32_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetArmedWeapon"));
        return;
    }

	uint32_t weaponID;
	bsData.Read(weaponID);

	if (CGame::GetPlayer())
	{
		CGame::GetPlayer()->SetArmedWeapon(weaponID);
	}
}

void ResetPlayerWeapons(RakNet::BitStream)
{
	CNecessary::Log(OBFUSCATE("RPC: ResetPlayerWeapons"));

	if (CGame::GetPlayer())
	{
		CGame::GetPlayer()->ClearAllWeapons();
	}
}

void UpdateScoresPingsIPs(RakNet::BitStream bsData)
{
	// CNecessary::Log(OBFUSCATE("RPC: UpdateScoresPingsIPs"));

	uint16_t bytePlayerId;
	int iPlayerScore;
	uint32_t dwPlayerPing;

	int iBitLength = bsData.GetNumberOfBitsUsed();

	for (uint16_t i = 0; i <(iBitLength / 8) / 9; i++)
	{ 
		bsData.Read(bytePlayerId);
		bsData.Read(iPlayerScore);
		bsData.Read(dwPlayerPing);

		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->UpdateScore(bytePlayerId, iPlayerScore);
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->UpdatePing(bytePlayerId, dwPlayerPing);
		}
	}
}

void ShowTextDraw(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ShowTextDraw"));
	
	uint16_t wTextId;
	TEXT_DRAW_TRANSMIT TextDrawTransmit;
	uint16_t byteTextLen;
	char cText[eMaxItem::MAX_TEXT_DRAW_LINE + 1];

	bsData.Read(wTextId);
	bsData.Read((char*)&TextDrawTransmit, sizeof(TEXT_DRAW_TRANSMIT));
	bsData.Read(byteTextLen);

	if (byteTextLen > sizeof(cText)) {
		CGui::GetChat()->AddInfoMessage(OBFUSCATE("Warning: ignoring large TextDraw size=%u"), byteTextLen);
		return;
	}

	bsData.Read(cText, byteTextLen);
	cText[byteTextLen] = '\0';

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->Add(wTextId, &TextDrawTransmit, cText);
		}
	}
}

void HideTextDraw(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: HideTextDraw"));

	uint16_t wTextId;
	bsData.Read(wTextId);

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->Delete(wTextId);
		}
	}
}

void TextDrawSetString(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: TextDrawSetString"));

	uint16_t wTextId;
	uint16_t byteTextLen;
	char cText[eMaxItem::MAX_TEXT_DRAW_LINE + 1];

	bsData.Read(wTextId);
	bsData.Read(byteTextLen);

	if (byteTextLen > sizeof(cText)) {
		return;
	}

	bsData.Read(cText, byteTextLen);
	cText[byteTextLen] = '\0';

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
		{	
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetTextDraw(wTextId))
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetTextDraw(wTextId)->SetText(cText);
			}		
		}
	}
}

void Create3DTextLabel(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: Create3DTextLabel"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(CVector)) +
		bytesToBits(sizeof(float)) +
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for Create3DTextLabel"));
        return;
    }

	uint16_t wLabelId;
	uint32_t dwColor;
	CVector vecPos;
	float fDrawDistance;
	uint8_t byteTestLOS;
	uint16_t playerId;
	uint16_t vehicleId;
	char szBuff[4096 + 1] = { '\0' };

	bsData.Read(wLabelId);
	bsData.Read(dwColor);
	bsData.Read((char*)&vecPos, sizeof(CVector));
	bsData.Read(fDrawDistance);
	bsData.Read(byteTestLOS);
	bsData.Read(playerId);
	bsData.Read(vehicleId);
	stringCompressor->DecodeString(szBuff, sizeof(szBuff), &bsData);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextLabelPool()) 
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextLabelPool()->Add(wLabelId, szBuff, dwColor, vecPos, fDrawDistance, byteTestLOS, playerId, vehicleId);
	}
}

void Destroy3DTextLabel(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: Destroy3DTextLabel"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for Destroy3DTextLabel"));
        return;
    }

	uint16_t wLabelId;

	bsData.Read(wLabelId);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextLabelPool()) 
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextLabelPool()->Delete(wLabelId);
	}
}

void ToggleSelectTextDraw(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: ToggleSelectTextDraw"));

	bool bEnable;
	uint32_t dwColor;

	bsData.Read(bEnable);
	bsData.Read(dwColor);

	if (CMultiPlayer::GetActiveNetwork())
	{
		// if (CGui::GetObjectSelection() && CGui::GetObjectSelection()->GetActiveSelectObject())
		// {
		// 	CGui::GetObjectSelection()->Enable(false);
		// }

		// if (CGui::GetObjectEdit() && CGui::GetObjectEdit()->GetActiveObjectEdit())
		// {
		// 	CGui::GetObjectEdit()->Enable(false);
		// }

		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->SetSelectState(bEnable, dwColor);
		}
	}
}

void CreatePickup(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: CreatePickup"));

	int bitsTotal =
		bytesToBits(sizeof(int)) +
		bytesToBits(sizeof(int)) +
		bytesToBits(sizeof(int)) +
		bytesToBits(sizeof(float)) +
		bytesToBits(sizeof(float)) +
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for CreatePickup"));
        return;
    }

	int iIndex;
	int iModel;
	int iType;
	float fX;
	float fY;
	float fZ;

	bsData.Read(iIndex);
	bsData.Read(iModel);
	bsData.Read(iType);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPickupPool())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPickupPool()->Add(iModel, iType, fX, fY, fZ, iIndex);
	}
}

void DestroyPickup(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: DestroyPickup"));

	int bitsTotal =
		bytesToBits(sizeof(int));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for DestroyPickup"));
        return;
    }

	int iIndex;

	bsData.Read(iIndex);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPickupPool())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPickupPool()->Delete(iIndex);
	}
}

void SetCheckpoint(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetCheckpoint"));

	int bitsTotal =
		bytesToBits(sizeof(VECTOR)) +
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetCheckpoint"));
        return;
    }

	VECTOR vecPos, vecExtent;
	float fSize;

	bsData.Read((char*)&vecPos, sizeof(VECTOR));
	bsData.Read(fSize);

	vecExtent = {fSize, fSize, fSize};

	CCheckPoint::Create((CVector*)&vecPos, (CVector*)&vecExtent);
	CCheckPoint::ToggleCheckpoints(true);
}

void DisableCheckpoint(RakNet::BitStream)
{
	CNecessary::Log(OBFUSCATE("RPC: DisableCheckpoint"));

	CCheckPoint::ToggleCheckpoints(false);
}

void SetRaceCheckpoint(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetRaceCheckpoint"));

	int bitsTotal =
		bytesToBits(sizeof(uint8_t)) +
		bytesToBits(sizeof(CVector)) +
		bytesToBits(sizeof(CVector)) +
		bytesToBits(sizeof(float));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetRaceCheckpoint"));
        return;
    }

	uint8_t byteType;
	CVector vecPos, vecNextPos;
	float fSize;

	bsData.Read(byteType);
	bsData.Read((char*)&vecPos, sizeof(CVector));
	bsData.Read((char*)&vecNextPos, sizeof(CVector));
	bsData.Read(fSize);

	CCheckPoint::CreateRace(byteType, &vecPos, &vecNextPos, fSize);
	CCheckPoint::ToggleRaceCheckpoints(true);
}

void DisableRaceCheckpoint(RakNet::BitStream)
{
	CNecessary::Log(OBFUSCATE("RPC: DisableRaceCheckpoint"));

	CCheckPoint::ToggleRaceCheckpoints(false);
}

void AddGangZone(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: AddGangZone"));

	int bitsTotal =
		bytesToBits(sizeof(int16_t)) +
		bytesToBits(sizeof(float) * 4) +
		bytesToBits(sizeof(uint32_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for AddGangZone"));
        return;
    }

	int16_t wZoneID;
	float minx, miny, maxx, maxy;
	uint32_t dwColor;

	bsData.Read(wZoneID);
	bsData.Read(minx);
	bsData.Read(miny);
	bsData.Read(maxx);
	bsData.Read(maxy);
	bsData.Read(dwColor);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool()->Add(wZoneID, minx, miny, maxx, maxy, dwColor);
	}
}

void GangZoneDestroy(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: GangZoneDestroy"));

	int bitsTotal =
		bytesToBits(sizeof(int16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for GangZoneDestroy"));
        return;
    }

	int16_t wZoneID;

	bsData.Read(wZoneID);
	if (wZoneID < 0 || wZoneID >= eMaxItem::MAX_GANG_ZONES) {
		return;
	}

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool())
	{
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool()->Delete(wZoneID);
	}
}

void GangZoneFlash(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: GangZoneFlash"));

	int bitsTotal =
		bytesToBits(sizeof(int16_t)) +
		bytesToBits(sizeof(uint32_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for GangZoneFlash"));
        return;
    }

	int16_t wZoneID;
	uint32_t dwColor;

	bsData.Read(wZoneID);
	if (wZoneID < 0 || wZoneID >= eMaxItem::MAX_GANG_ZONES) {
		return;
	}

	bsData.Read(dwColor);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool()->GetGangZoneInSlot(wZoneID))
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool()->GetGangZone(wZoneID)->Flash(dwColor);
		}
	}
}

void GangZoneStopFlash(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: GangZoneStopFlash"));

	int bitsTotal =
		bytesToBits(sizeof(int16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for GangZoneStopFlash"));
        return;
    }

	int16_t wZoneID;

	bsData.Read(wZoneID);
	if (wZoneID < 0 || wZoneID >= eMaxItem::MAX_GANG_ZONES) {
		return;
	}

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool()->GetGangZoneInSlot(wZoneID))
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetGangZonePool()->GetGangZone(wZoneID)->StopFlash();
		}
	}
}

void SetPlayerSkillLevel(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: SetPlayerSkillLevel"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t)) +
		bytesToBits(sizeof(uint32_t)) +
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SetPlayerSkillLevel"));
        return;
    }

	uint16_t playerId;
	uint32_t dSkillID;
	uint16_t wLevel;

	bsData.Read(playerId);
	bsData.Read(dSkillID);
	bsData.Read(wLevel);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) 
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
		{
			if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->SetWeaponSkill(dSkillID, wLevel);
			} 
			else 
			{
				if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(playerId)) 
				{
					CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetGamePlayer()->SetWeaponSkill(dSkillID, wLevel);
				}
			}
		}
	}
}

void DeathBroadcast(RakNet::BitStream bsData)
{
	CNecessary::Log(OBFUSCATE("RPC: DeathBroadcast"));

	int bitsTotal =
		bytesToBits(sizeof(uint16_t));

	if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
    	CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for DeathBroadcast"));
        return;
    }

	uint16_t playerId;

	bsData.Read(playerId);

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)) 
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->HandleWasted();
		}
	}
}

void SendDeathMessage(RakNet::BitStream bsData)
{
    CNecessary::Log(OBFUSCATE("RPC: SendDeathMessage"));

    int bitsTotal =
        bytesToBits(sizeof(uint16_t) * 2) +
        bytesToBits(sizeof(uint8_t));

    if (bsData.GetNumberOfUnreadBits() < bitsTotal) {
        CNecessary::Log(OBFUSCATE("ERROR: Insufficient data for SendDeathMessage"));
        return;
    }

    uint16_t killerId, playerId;
    uint8_t reason;

    bsData.Read(killerId);
    bsData.Read(playerId);
    bsData.Read(reason);

    std::string killerName, playerName;
    uint32_t killerColor = 0xFFFFFFFF, playerColor = 0xFFFFFFFF;
    killerName.clear();
    playerName.clear();

    uint32_t fontcolor = 0xFFFFFFFF;

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool())
    {
        if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
        {
            playerName = CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName;
            playerColor = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerColorAsARGB();
        }
        else
        {
            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(playerId))
            {
                playerName = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetRemoteName();
                playerColor = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetPlayerColorAsARGB();
            }
            else
            {
                playerName = OBFUSCATE("  ");

                if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID() != killerId && !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(killerId))
                {
                    killerName = OBFUSCATE("  ");
                    reason = 255;
                }
            }
        }

        if (killerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
        {
            killerName = CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName;
            killerColor = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerColorAsARGB();
        }
        else
        {
            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(killerId))
            {
                killerName = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(killerId)->GetRemoteName();
                killerColor = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(killerId)->GetPlayerColorAsARGB();
            }
            else
            {
                killerName = OBFUSCATE("  ");
            }
        }

        if (killerId == eInvalidId::INVALID_PLAYER_ID)
        {
            if (reason == 200 || reason == 201)
            {
                fontcolor = (reason == 200) ? 0xFFFF0000 : 0xFF0000FF;
            }

            playerName = OBFUSCATE("  ");

            if (playerId == CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetLocalID())
            {
                killerName = CMultiPlayer::GetActiveNetwork()->GetNetworkData().PlayerName;
                killerColor = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerColorAsARGB();
            }
            else if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayerInSlot(playerId))
            {
                killerName = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetRemoteName();
                killerColor = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetRemotePlayer(playerId)->GetPlayerColorAsARGB();
            }
        }
    }

    CGui::GetDeathWindow()->AddEntry(playerName.c_str(), playerColor, killerName.c_str(), killerColor, reason, fontcolor);
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

		case RPC_ServerJoin: {
		    ServerJoin(*bsData);
		    break;
		}

		case RPC_ServerQuit: {
		    ServerQuit(*bsData);
		    break;
		}

		case RPC_GameModeRestart: {
			GameModeRestart();
			break;
		}

		case RPC_ConnectionRejected: {
			ConnectionRejected(*bsData);
			break;
		}

		case RPC_Chat: {
		    Chat(*bsData);
		    break;
		}

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

		case RPC_SetPlayerColor: {
		    SetPlayerColor(*bsData);
		    break;
		}

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

        case RPC_SetPlayerVelocity: {
            SetPlayerVelocity(*bsData);
            break;
        }

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

		case RPC_ToggleWidescreen: {
			ToggleWidescreen(*bsData);
			break;
		}

		// case RPC_SetTargeting: {
		// 	ToggleCameraTarget(*bsData);
		// 	break;
		// }

		case RPC_ShowGameText: {
			ShowGameText(*bsData);
			break;
		}

		case RPC_CreateExplosion: {
			CreateExplosion(*bsData);
			break;
		}

		case RPC_SetGravity: {
			SetGravity(*bsData);
			break;
		}

		// case RPC_SetPlayerWantedLevel: {
		// 	SetPlayerWantedLevel(*bsData);
		// 	break;
		// }

		case RPC_ChatBubble: {
			ChatBubble(*bsData);
			break;
		}

		case RPC_ToggleClock: {
			ToggleClock(*bsData);
			break;
		}

		case RPC_SetPlayerMapIcon: {
		    SetMapIcon(*bsData);
		    break;
		}

		case RPC_RemovePlayerMapIcon: {
		    RemoveMapIcon(*bsData);
		    break;
		}

		// case RPC_SetPlayerWorldBounds: {
		//     SetWorldBounds(*bsData);
		//     break;
		// }

		case RPC_GivePlayerMoney: {
		    GivePlayerMoney(*bsData);
		    break;
		}

		case RPC_ResetPlayerMoney: {
		    ResetPlayerMoney(*bsData);
		    break;
		}

		case RPC_SetPlayerTeam: {
		    SetPlayerTeam(*bsData);
		    break;
		}

		case RPC_ShowActor: {
		    ShowActor(*bsData);
		    break;
		}

		case RPC_HideActor: {
		    HideActor(*bsData);
		    break;
		}

		case RPC_SetActorHealth: {
		    SetActorHealth(*bsData);
		    break;
		}

		case RPC_SetActorFacingAngle: {
		    SetActorFacingAngle(*bsData);
		    break;
		}

		case RPC_SetActorPos: {
		    SetActorPos(*bsData);
		    break;
		}

		case RPC_ApplyActorAnimation: {
		    ApplyActorAnimation(*bsData);
		    break;
		}

		case RPC_ClearActorAnimation: {
		    ClearActorAnimation(*bsData);
		    break;
		}

		// case RPC_PlayCrimeReport: {
		// 	PlayCrimeReport(*bsData);
		// 	break;
		// }

		case RPC_PlayAudioStream: {
			PlayAudioStream(*bsData);
			break;
		}

		case RPC_StopAudioStream: {
			StopAudioStream(*bsData);
			break;
		}

		case RPC_PlaySound: {
			PlaySound(*bsData);
			break;
		}

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

		case RPC_WorldPlayerAdd: {
		    WorldPlayerAdd(*bsData);
		    break;
		}

		case RPC_WorldPlayerRemove: {
		    WorldPlayerRemove(*bsData);
		    break;
		}

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

		case RPC_WorldPlayerDeath: {
		    DeathBroadcast(*bsData);
		    break;
		}

		case RPC_DeathMessage: {
		    SendDeathMessage(*bsData);
		    break;
		}

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

		case RPC_SetPlayerFightingStyle: {
		    SetPlayerFightingStyle(*bsData);
		    break;
		}

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

		case RPC_ShowPlayerNameTag: {
		    ShowPlayerNameTag(*bsData);
		    break;
		}

		case RPC_ApplyPlayerAnimation: {
		    ApplyAnimation(*bsData);
		    break;
		}

		case RPC_ClearPlayerAnimation: {
		    ClearAnimations(*bsData);
		    break;
		}

		case RPC_SetPlayerName: {
		    SetPlayerName(*bsData);
		    break;
		}

		case RPC_PlayerUpdate: {
		    SendGameTimeUpdate(*bsData);
		    break;
		}

		case RPC_GivePlayerWeapon: {
		    GivePlayerWeapon(*bsData);
		    break;
		}

		case RPC_SetPlayerAmmo: {
		    SetWeaponAmmo(*bsData);
		    break;
		}

		case RPC_SetArmedWeapon: {
		    SetArmedWeapon(*bsData);
		    break;
		}

		case RPC_ResetPlayerWeapons: {
		    ResetPlayerWeapons(*bsData);
		    break;
		}

		case RPC_UpdateScoresPingsIps: {
		    UpdateScoresPingsIPs(*bsData);
		    break;
		}

		// case RPC_RemoveBuildingForPlayer: {
		//     RemoveBuilding(*bsData);
		//     break;
		// }

		case RPC_ClickTextdraw: {
		    ToggleSelectTextDraw(*bsData);
		    break;
		}

		case RPC_ShowTextDraw: {
		    ShowTextDraw(*bsData);
		    break;
		}

		case RPC_HideTextDraw: {
		    HideTextDraw(*bsData);
		    break;
		}

		case RPC_TextDrawSetString: {
		    TextDrawSetString(*bsData);
			break;
		}

		case RPC_Create3DTextLabel: {
		    Create3DTextLabel(*bsData);
		    break;
		}

		case RPC_Update3DTextLabel: {
		    Destroy3DTextLabel(*bsData);
		    break;
		}

		// case RPC_SetPlayerSpecialAction: {
		//     SetPlayerSpecialAction(*bsData);
		//     break;
		// }

		case RPC_CreatePickup: {
		    CreatePickup(*bsData);
		    break;
		}

		case RPC_DestroyPickup: {
		    DestroyPickup(*bsData);
		    break;
		}

		case RPC_SetCheckpoint: {
		    SetCheckpoint(*bsData);
		    break;
		}

		case RPC_DisableCheckpoint: {
		    DisableCheckpoint(*bsData);
		    break;
		}

		case RPC_SetRaceCheckpoint: {
		    SetRaceCheckpoint(*bsData);
		    break;
		}

		case RPC_DisableRaceCheckpoint: {
		    DisableRaceCheckpoint(*bsData);
		    break;
		}

		case RPC_AddGangZone: {
		    AddGangZone(*bsData);
		    break;
		}

		case RPC_GangZoneDestroy: {
		    GangZoneDestroy(*bsData);
		    break;
		}

		case RPC_GangZoneFlash: {
		    GangZoneFlash(*bsData);
		    break;
		}

		case RPC_GangZoneStopFlash: {
		    GangZoneStopFlash(*bsData);
		    break;
		}

		case RPC_SetPlayerSkillLevel: {
			SetPlayerSkillLevel(*bsData);
			break;
		}

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