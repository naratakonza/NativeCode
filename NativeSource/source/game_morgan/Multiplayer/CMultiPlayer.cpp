#include "../../../Main.h"
#include "CMultiPlayer.h"
#include "CServer.h"
#include "CNetGame.h"
#include "Guis/CGui.h"
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

    CServer::SetIpConnect(OBFUSCATE("gta-irz.com"));
	CServer::SetPassConnect("");
	CServer::SetPortConnect(7776);

    CScripting::Init();

    CMultiPlayer::SetThread(eThreadType::THREAD_TYPE_START);

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
			CGame::SetFramerate(60);

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
