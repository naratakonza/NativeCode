#include "Main.h"
#include "source/game_morgan/Audio/CBass.h"
#include "source/game_morgan/Multiplayer/CMultiPlayer.h"
#include "source/game_morgan/Multiplayer/CNetGame.h"

void InitialiseClient()
{
	CNecessary::Log(OBFUSCATE("Morgan Client Initialise"));

	CBass::Initialise();

	if (!CSetting::pSetting)
	{
		CSetting::pSetting = new CSetting();
	}

	InitialiseRenderWare();

	CGame::Initialize();
	CMultiPlayer::Initialise();
}

void UninitialiseClient()
{
	CNecessary::Log(OBFUSCATE("Client Uninitialise"));

	CGame::Uninitialise();

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CMultiPlayer::GetActiveNetwork()->GetRakClient())
		{
			CMultiPlayer::GetActiveNetwork()->GetRakClient()->Disconnect(500, 0);
		}
	}

	CMultiPlayer::Uninitialise();

	// SAFE_DELETE(CCrashHandler::pCrashHandler);
}