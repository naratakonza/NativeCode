#include "../../../../Main.h"
#include "CInterface.h"
#include "CGui.h"
#include "../CMultiPlayer.h"
#include "../../CGame.h"
#include "../../Base/CWidgets.h"
// #include "CVoiceMenu.h"
// #include "../utils/Java/CJava.h"

void CInterface::Control()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    if (CGui::GetScoreBoard()->GetActiveScore() /*|| CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode) || CGui::GetClientMenu()->GetActiveClientMenu()*/)
    {
        CGame::DisplayHUD(false);
    }

    if (CGame::GetPlayer())
    {
        if (CGame::GetPlayer()->GetDrunkLevel() > 5000)
        {
            CGame::DisplayHUD(false);
        }
        else if (CGame::GetPlayer()->GetDrunkLevel() > 1 && CGame::GetPlayer()->GetDrunkLevel() <= 4999)
        {
            if (!CGui::GetScoreBoard()->GetActiveScore() /*&& !CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() && !CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CGui::GetClientMenu()->GetActiveClientMenu()*/ && !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
            {
                CGame::DisplayHUD(true);
            }
        }
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
        return;
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
    {
        CGame::DisplayHUD(false);
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpawnInfo()) {
        return;
    }

    CGame::DisplayHUD(false);
    CWidgets::SetToggle(false);
}