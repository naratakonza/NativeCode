#include "../../Main.h"
#include "CMainMenuScreen.h"
#include "Multiplayer/CMultiPlayer.h"

extern "C" __attribute__((visibility("default"))) void MGMainMenuOnStartGame(void*)
{
    CMainMenuScreen::OnStartGame();
}

void CMainMenuScreen::OnStartGame()
{
    CMainMenuScreen::OnExit();
}

bool CMainMenuScreen::Update()
{
    CMultiPlayer::StartSAMP();
    return true;
}

void CMainMenuScreen::OnExit()
{
    ExitGame();
}