#include "../../../../Main.h"
#include "CButton.h"
#include "CGui.h"
#include "../../Base/CPad.h"
#include "../CMultiPlayer.h"
#include "../../CGame.h"
#include "../../RadarMod/CTiltedRadar.h"
#include "../../../../Launcher/MGFlutter.h"

std::mutex CButton::m_KeyInputMutex;
std::vector<int> CButton::m_KeyInputQueue;

bool CButton::m_bToggleKeyF4 = false;
bool CButton::m_bToggleKeyUPDOWN = false;
bool CButton::m_bToggleKeyLEFTRIGHT = false;

CButton::CButton()
{
    CButton::m_bRenderStatus = false;
    CButton::m_bKeysStatus = false;
    CButton::m_bKeyAfterDeath = false;
}

CButton::~CButton()
{
    CButton::m_bRenderStatus = false;
	CButton::m_bKeysStatus = false;
    CButton::m_bKeyAfterDeath = false;

    {
        std::lock_guard<std::mutex> lock(CButton::m_KeyInputMutex);
        CButton::m_KeyInputQueue.clear();
    }

    MGUpdateFlutterKeysBar(false, false, false, false, -1, -1);
}

bool CButton::QueueKeyInput(int keyId)
{
    if (keyId < static_cast<int>(eKeysBarKey::KEY_TAB) || keyId > static_cast<int>(eKeysBarKey::KEY_F8)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(CButton::m_KeyInputMutex);
    if (CButton::m_KeyInputQueue.size() >= 64) {
        return false;
    }

    CButton::m_KeyInputQueue.push_back(keyId);
    return true;
}

void CButton::Render()
{
    std::vector<int> keys;
    {
        std::lock_guard<std::mutex> lock(CButton::m_KeyInputMutex);
        keys.swap(CButton::m_KeyInputQueue);
    }

    for (int keyId : keys)
    {
        CButton::SendKeyInput(keyId);
    }

    float radarLeft = 0.0f;
    float radarBottom = 0.0f;
    const bool radarBoundsValid = RsGlobal && RsGlobal->maximumWidth > 0 && RsGlobal->maximumHeight > 0 && CTiltedRadar::GetWidgetBottomLeft(radarLeft, radarBottom);
    const int radarLeftPerMille = radarBoundsValid ? static_cast<int>(std::round(radarLeft * 1000.0f / RsGlobal->maximumWidth)) : -1;
    const int radarBottomPerMille = radarBoundsValid ? static_cast<int>(std::ceil(radarBottom * 1000.0f / RsGlobal->maximumHeight)) : -1;

    bool bShouldShow = false;

    if (CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() == eNetworkState::GAMESTATE_CONNECTED && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()) 
        {
            bool bDialogActive = CGui::GetDialog() && CGui::GetDialog()->GetActiveDialog();

            // เดียวมาทำต่อภายหลัง
            // bool bScoreActive = CGui::GetScoreBoard() && CGui::GetScoreBoard()->GetActiveScore();
            // bool bVoiceActive = CGui::GetVoiceMenu() && CGui::GetVoiceMenu()->GetActiveVoiceMenu();
            // bool bMainMenuActive = CGui::GetMainMenuScreens() && CGui::GetMainMenuScreens()->GetActiveMainMenuScreens();

            bool bTextDrawActive = false;
            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()) 
            {
                bTextDrawActive = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState();
            }

            if (!bDialogActive /*&& !bScoreActive && !bMainMenuActive && !bVoiceActive*/ && !bTextDrawActive && CButton::m_bRenderStatus)
            {
                bShouldShow = true;
            }
        }
    }

    bool bIsInVehicle = false;
    if (CGame::GetPlayer())
    {
        bIsInVehicle = CGame::GetPlayer()->IsInVehicle();
    }

    bool bIsMenuState = false;
    // เดียวมาทำต่อภายหลัง
    // if (CMultiPlayer::GetActiveNetwork() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetMenuPool())
    // {
    //     bIsMenuState = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetMenuPool()->GetState();
    // }

    bool bIsF4Enabled = CButton::m_bToggleKeyF4;

    static bool bLastShow = false;
    static bool bLastVehicle = false;
    static bool bLastMenu = false;
    static bool bLastF4 = false;
    static int lastRadarLeft = -1;
    static int lastRadarBottom = -1;

    if (!CButton::m_bStateSent || bShouldShow != bLastShow || bIsInVehicle != bLastVehicle || bIsMenuState != bLastMenu || bIsF4Enabled != bLastF4 || radarLeftPerMille != lastRadarLeft || radarBottomPerMille != lastRadarBottom)
    {
        if (MGUpdateFlutterKeysBar(bShouldShow, bIsInVehicle, bIsMenuState, bIsF4Enabled, radarLeftPerMille, radarBottomPerMille))
        {
            CButton::m_bStateSent = true;
            bLastShow = bShouldShow;
            bLastVehicle = bIsInVehicle;
            bLastMenu = bIsMenuState;
            bLastF4 = bIsF4Enabled;

            lastRadarLeft = radarLeftPerMille;
            lastRadarBottom = radarBottomPerMille;
        }
    }
}

void CButton::Enable(bool status)
{
    CButton::m_bRenderStatus = status;
}

void CButton::SetKeyF4(bool enable)
{
    CButton::m_bToggleKeyF4 = enable;
}

void CButton::SetKeyUPDOWN(bool enable)
{
    CButton::m_bToggleKeyUPDOWN = enable;
}

void CButton::SetKeyLEFTRIGHT(bool enable)
{
    CButton::m_bToggleKeyLEFTRIGHT = enable;
}

void CButton::SendKeyInput(int keyId)
{
    if (!CMultiPlayer::GetActiveNetwork() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()) {
        return;
    }

    switch (static_cast<eKeysBarKey>(keyId))
    {
        case eKeysBarKey::KEY_TAB: {
            // เดียวมาทำต่อภายหลัง
            // if (CGui::GetScoreBoard())
            // {
            //     CGui::GetScoreBoard()->Draw();
            // }

            break;
        }

        case eKeysBarKey::KEY_ALT: {
            if (CGame::GetPlayer())
            {
                if (CGame::GetPlayer()->IsInVehicle())
                {
                    CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;
                }
                else
                {
                    CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK] = true;
                }
            }

            break;
        }

        case eKeysBarKey::KEY_SPC: {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT] = true;
            break;
        }

        case eKeysBarKey::KEY_SHIFT: {
            if (CGame::GetPlayer())
            {
                if (!CGame::GetPlayer()->IsInVehicle())
                {
                    CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = true;
                }
                else
                {
                    CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;
                }
            }

            break;
        }

        case eKeysBarKey::KEY_ENTER: {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true;
            break;
        }

        case eKeysBarKey::KEY_C: {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_CROUCH] = true;
            break;
        }

        case eKeysBarKey::KEY_H: {
            // เดียวมาทำต่อภายหลัง
            // if (!CGame::GetPlayer()->IsInPassengerDriveByMode())
            // {
            //     CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = true;
            // }
            // else
            // {
            //     CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_CROUCH] = true;
            // }

            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = true;

            break;
        }

        case eKeysBarKey::KEY_Y: {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = true;
            break;
        }

        case eKeysBarKey::KEY_N: {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = true;
            break;
        }

        case eKeysBarKey::KEY_SUBMISSION: {
            CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION] = true;
            break;
        }

        case eKeysBarKey::KEY_F4: {
            if (CGui::GetButton())
            {
                CGui::GetButton()->m_bKeyAfterDeath = true;
            }

            break;
        }

        case eKeysBarKey::KEY_F8: {
            // เดียวมาทำต่อภายหลัง
            // if (CScreenShot::IsScreenShot() != eScreenShotMode::SCREENSHOT_TYPE_NONE) {
            //     return;
            // }
            
            // CScreenShot::SetScreenShot(eScreenShotMode::SCREENSHOT_TYPE_F8);
            // CScreenShot::TakeScreenshot();

            break;
        }

        case eKeysBarKey::KEY_F11: {
            // เดียวมาทำต่อภายหลัง
            // if (CGui::GetVoiceMenu())
            // {
            //     CGui::GetVoiceMenu()->Draw();
            // }

            break;
        }

        case eKeysBarKey::KEY_UP: {
            CPad::LocalPlayerKeys.wKeyUD = 0xFF80;

            break;
        }

        case eKeysBarKey::KEY_DOWN: {
            CPad::LocalPlayerKeys.wKeyUD = 0x80;

            break;
        }

        case eKeysBarKey::KEY_LEFT: {
            CPad::LocalPlayerKeys.wKeyLR = 0xFF80;

            break;
        }

        case eKeysBarKey::KEY_RIGHT: {
            CPad::LocalPlayerKeys.wKeyLR = 0x80;

            break;
        }

        case eKeysBarKey::KEY_SETTINGS: {

            break;
        }
    }
}