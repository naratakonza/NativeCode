#include "../../../Main.h"
#include "CMultiPlayer.h"
#include "CServer.h"
#include "Audio/CAudio.h"
#include "Guis/CGui.h"
#include "../CAspectRatio.h"
#include "../CCameraAim.h"
#include "../CCameraController.h"
#include "../CDraw.h"
#include "../CHud.h"
#include "../CRadar.h"
#include "../CSensitivity.h"
#include "../CGyro.h"
#include "../CTimer.h"
#include "../../../Launcher/MGGyro.h"
#include "../CStartGameScreen.h"
#include "../CTheZones.h"
#include "../Entity/CPhysical.h"
#include "../Base/CPatches.h"
#include "../Base/CHooks.h"
#include "../Base/CWidgets.h"
#include "Pools/Process/CMapIconPool.h"
#include "../Base/Graphics/CRealTimeShadowManager.h"
#include "../COptimizeDistance.h"

CNetGame* CMultiPlayer::pNetGame = nullptr;

bool CMultiPlayer::m_bSAMPInited = false;
bool CMultiPlayer::m_bPlaySAMP = false;
bool CMultiPlayer::m_bGameStarted = false;

void CMultiPlayer::Initialise()
{
	CNecessary::Log(OBFUSCATE("CMultiPlayer Initialise"));

	CMultiPlayer::SetThread(eThreadType::THREAD_TYPE_ONLOAD);
}

void CMultiPlayer::Uninitialise()
{
	CNecessary::Log(OBFUSCATE("CMultiPlayer Uninitialise"));

    CGyro::Enable(false);
    MGUpdateGyro(false);

    CGui::Uninitialise();
	CAudio::Uninitialise();

	SAFE_DELETE(CMultiPlayer::pNetGame);

	CGame::SetPlayerStatsLocked(false);

	CMultiPlayer::m_bSAMPInited = false;
    CMultiPlayer::m_bPlaySAMP = false;
	CMultiPlayer::m_bGameStarted = false;
}

void CMultiPlayer::Process()
{
    if (CGui::GetChat())
	{
        CGui::GetChat()->ProcessPendingChatInputs();
    }

    MGUpdateGyro(CGyro::IsGyro() && !CTimer::IsUserPause());

    if (CMultiPlayer::IsSampInit())
    {
        if (CMultiPlayer::GetActiveNetwork())
        {
            CMultiPlayer::GetActiveNetwork()->Process();

            if (CGui::GetDialog())
            {
                CGui::GetDialog()->ProcessPendingResponse();
            }
        }

        if (CAudio::GetAudioStream())
        {
            CAudio::GetAudioStream()->Process();
            CAudio::GetAudioStream()->ControlGameRadio();
        }

        CAudio::Process();
    }
    else
    {
		CRadar::ResetRadarColor();
        CMultiPlayer::SetThread(eThreadType::THREAD_TYPE_PROCESS);

        CMultiPlayer::pNetGame = new CNetGame(CServer::GetIpConnect(), CServer::GetPortConnect(), CMultiPlayer::GetPlayerName(), CServer::GetPassConnect(), 576);

        CMultiPlayer::m_bSAMPInited = true;
    }
}

void CMultiPlayer::Reconnect(bool waitconnect)
{
    if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer())
		{
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->ResetData();
			CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->EnableSpectating(false);
		}

		CCameraAim::GameResetLocalPlayerSkills();

		CGui::GetButton()->Enable(false);
		CGui::GetSpawnScreen()->Enable(false);
		CGui::GetDialog()->Enable(false);
		
		CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->DeleteAll();
		CMapIconPool::DeleteAll();
		CMultiPlayer::GetActiveNetwork()->InitialiseGameLogic();

		if (CGame::GetPlayer())
		{
			CGame::GetPlayer()->SetInterior(0, true);
			// CGame::GetPlayer()->SetDead(); เดียวมาทำต่อภายหลัง
			CGame::GetPlayer()->SetArmour(0.0f);
		}

		if (CGui::GetGPSMarker())
		{
			CGui::GetGPSMarker()->SetPosition(0.0f, 0.0f, 0.0f);
			CGui::GetGPSMarker()->SetVisible(false);
		}

		CRadar::ResetRadarColor();
		CGame::ResetPlayerMoney();
		CClock::EnableGameClock(false);
		CTheZones::EnableZoneNames(false);
		CPhysical::SetGravity(CMultiPlayer::GetActiveNetwork()->m_fGravity);

		if (waitconnect)
		{
			CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_WAIT_CONNECT);
		}
	}
}

void CMultiPlayer::ChangeServer(const char* ip, int port)
{
    if (CMultiPlayer::GetActiveNetwork())
    {
        CMultiPlayer::GetActiveNetwork()->GetRakClient()->Disconnect(0);
        CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_NETWORKCHANGED);
        CMultiPlayer::Reconnect();

        SAFE_DELETE(CMultiPlayer::pNetGame);

        CMultiPlayer::pNetGame = new CNetGame(ip, port, CMultiPlayer::GetPlayerName(), OBFUSCATE(""), 576, true);
    }
}

void CMultiPlayer::StartSAMP()
{
    if (CMultiPlayer::IsSampActive()) {
		return;
	}

    if (!CStartGameScreen::OnNewGameCheck()) {
		return;
	}

	CNecessary::Log(OBFUSCATE("CMultiPlayer StartSAMP"));

    CServer::SetIpConnect(OBFUSCATE("morgan-games.com"));
	CServer::SetPassConnect("");
	CServer::SetPortConnect(7777);

    CScripting::Init();

    CMultiPlayer::SetThread(eThreadType::THREAD_TYPE_START);

    CCameraAim::Initialise();
	CCameraController::Initialise();
    CGui::Initialise();
    CAudio::Initialise();

    CMultiPlayer::m_bPlaySAMP = true;
}

void CMultiPlayer::SetThread(eThreadType type)
{
	CNecessary::Log(OBFUSCATE("InstallSetThread type : %d"), static_cast<int>(type));

	switch (type) {
		case eThreadType::THREAD_TYPE_ONLOAD: {
			CPatches::SetPatches(ePatchType::PATCH_TYPE_ZERO);
			CHooks::SetHooking(eHookType::HOOK_TYPE_ONLOAD);

			break;
		}

		case eThreadType::THREAD_TYPE_START: {
			CPatches::SetPatches(ePatchType::PATCH_TYPE_MENU);
			CHooks::SetHooking(eHookType::HOOK_TYPE_START);
			CWidgets::SetAlpha(CSetting::GetConfig()->GetIntValue(OBFUSCATE("WidgetAlpha")));
			CRadar::SetAlpha(CSetting::GetConfig()->GetIntValue(OBFUSCATE("RadarAlpha")));
			CHud::SetAlpha(CSetting::GetConfig()->GetIntValue(OBFUSCATE("HudAlpha")));
			CRealTimeShadowManager::SetShadowStyle(CSetting::GetConfig()->GetIntValue(OBFUSCATE("Shadow")));
			CGame::SetFramerate(static_cast<std::uint8_t>(std::clamp(CSetting::GetConfig()->GetIntValue(OBFUSCATE("SetFps")), 0, static_cast<int>(UINT8_MAX))));
			CSensitivity::SetOnfootTouch(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("OnfootSensitivity"))));
			CSensitivity::SetAimTouch(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("AimSensitivity"))));
			CSensitivity::SetIncarTouch(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("IncarSensitivity"))));
			CDraw::SetFov(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("SetFov"))));
			CGyro::SetGyroMode(static_cast<eGyroMode>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("GyroMode"))));
			CGyro::Enable(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("GyroScope")));
			CGyro::SetAimSensitivity(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("GyroAimSensitivity"))));
			CGyro::SetOnfootSensitivity(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("GyroOnfootSensitivity"))));
			CGyro::SetIncarSensitivity(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("GyroIncarSensitivity"))));
			CAspectRatio::SetSize((eAspectRatioType)CSetting::GetConfig()->GetIntValue(OBFUSCATE("AspectRatio")));
			CButton::SetKeyF4(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowKeyF4")));
			CButton::SetKeyUPDOWN(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowKeyUPDOWN")));
			CButton::SetKeyLEFTRIGHT(CSetting::GetConfig()->GetBoolValue(OBFUSCATE("ShowKeyLEFTRIGHT")));

			break;
		}

		case eThreadType::THREAD_TYPE_PROCESS: {
			CPatches::SetPatches(ePatchType::PATCH_TYPE_GAME);
			CHooks::SetHooking(eHookType::HOOK_TYPE_GAME);
			COptimizeDistance::OptimizeStreamingMemory();
			CGame::MenuSwitchOff();
			CGame::SetPlayerMaxStats();

			break;
		}
	}
}