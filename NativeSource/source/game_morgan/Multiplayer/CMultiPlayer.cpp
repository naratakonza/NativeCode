#include "../../../Main.h"
#include "CMultiPlayer.h"
#include "CServer.h"
#include "CNetGame.h"
#include "Guis/CGui.h"
#include "../CAspectRatio.h"
#include "../CCameraAim.h"
#include "../CCameraController.h"
#include "../CDraw.h"
#include "../CSensitivity.h"
#include "../CStartGameScreen.h"
#include "../CTheZones.h"
#include "../Base/CPatches.h"
#include "../Base/CHooks.h"
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

    CGui::Uninitialise();

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

    if (CMultiPlayer::IsSampInit())
    {
        if (CMultiPlayer::GetActiveNetwork())
        {
            CMultiPlayer::GetActiveNetwork()->Process();
        }
    }
    else
    {
        CMultiPlayer::SetThread(eThreadType::THREAD_TYPE_PROCESS);

        CMultiPlayer::pNetGame = new CNetGame(CServer::GetIpConnect(), CServer::GetPortConnect(), "ArtiwlizDev", CServer::GetPassConnect(), 576);

        CMultiPlayer::m_bSAMPInited = true;
    }
}

void CMultiPlayer::Reconnect(bool waitconnect)
{
    if (CMultiPlayer::GetActiveNetwork())
	{
		CCameraAim::GameResetLocalPlayerSkills();
		
		CMultiPlayer::GetActiveNetwork()->InitialiseGameLogic();

		CClock::EnableGameClock(false);
		CTheZones::EnableZoneNames(false);

		if (waitconnect)
		{
			CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_WAIT_CONNECT);
		}
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
			CGame::SetFramerate(static_cast<std::uint8_t>(std::clamp(CSetting::GetConfig()->GetIntValue(OBFUSCATE("SetFps")), 0, static_cast<int>(UINT8_MAX))));
			CSensitivity::SetOnfootTouch(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("OnfootSensitivity"))));
			CSensitivity::SetAimTouch(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("AimSensitivity"))));
			CSensitivity::SetIncarTouch(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("IncarSensitivity"))));
			CDraw::SetFov(static_cast<float>(CSetting::GetConfig()->GetIntValue(OBFUSCATE("SetFov"))));
			CAspectRatio::SetSize((eAspectRatioType)CSetting::GetConfig()->GetIntValue(OBFUSCATE("AspectRatio")));

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