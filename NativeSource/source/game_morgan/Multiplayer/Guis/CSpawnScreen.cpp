#include "CSpawnScreen.h"
#include "../CMultiPlayer.h"
#include "../../../../Launcher/MGFlutter.h"

CSpawnScreen::CSpawnScreen()
{
    CSpawnScreen::m_bRenderStatus = false;
    CSpawnScreen::m_bSendSpawn = false;
    CSpawnScreen::m_bSendPrev = false;
    CSpawnScreen::m_bSendNext = false;
}

CSpawnScreen::~CSpawnScreen()
{
    if (CSpawnScreen::m_bRenderStatus)
    {
        CSpawnScreen::Enable(false);
    }
}

void CSpawnScreen::Enable(bool status)
{
    if (CSpawnScreen::m_bRenderStatus != status)
    {
        CSpawnScreen::m_bRenderStatus = status;
        MGSetFlutterSpawnVisible(status);
    }
}

bool CSpawnScreen::GetActiveSpawnScreen()
{
    return CSpawnScreen::m_bRenderStatus;
}

bool CSpawnScreen::HandleAction(eSpawnAction action)
{
    if (!CSpawnScreen::m_bRenderStatus || !CMultiPlayer::GetActiveNetwork() || CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
        return false;
    }

    switch (action)
    {
        case eSpawnAction::PREV_SKIN: {
            CSpawnScreen::m_bSendPrev = true;
            return true;
        }

        case eSpawnAction::SPAWN_PLAYER: {
            CSpawnScreen::m_bSendSpawn = true;
            return true;
        }

        case eSpawnAction::NEXT_SKIN: {
            CSpawnScreen::m_bSendNext = true;
            return true;
        }
    }

    return false;
}