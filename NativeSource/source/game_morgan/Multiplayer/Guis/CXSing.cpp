#include "../../../../Main.h"
#include "CXSing.h"
#include "CGui.h"
// #include "CVoiceMenu.h" // รอโยกระบบ VoiceMenu
#include "../CMultiPlayer.h"
#include "../../CTxdManager.h"
#include "../../CGame.h"

CXSing::CXSing()
{
    CXSing::m_bRenderStatus = false;
    CXSing::m_bKeysStatus = false;
    CXSing::p_XSingButtonTexture = nullptr;
    CXSing::p_XSingHoverButtonTexture = nullptr;

    CXSing::LoadTexture();
}

CXSing::~CXSing()
{

}

void CXSing::LoadTexture()
{
    CNecessary::Log(OBFUSCATE("CXSing LoadTexture"));

    CXSing::p_XSingButtonTexture = (RwTexture*)CTxdManager::LoadTextureFromDB(OBFUSCATE("mobile"), OBFUSCATE("hud_x"));
    CXSing::p_XSingHoverButtonTexture = (RwTexture*)CTxdManager::LoadTextureFromDB(OBFUSCATE("mobile"), OBFUSCATE("hud_plus"));

    if (!CXSing::p_XSingButtonTexture)
    {
        CXSing::p_XSingButtonTexture = (RwTexture*)CTxdManager::LoadTextureFromDB(OBFUSCATE("samp"), OBFUSCATE("invisible"));
    }

    if (!CXSing::p_XSingHoverButtonTexture)
    {
        CXSing::p_XSingHoverButtonTexture = (RwTexture*)CTxdManager::LoadTextureFromDB(OBFUSCATE("samp"), OBFUSCATE("invisible"));
    }

    return;
}

void CXSing::Render()
{
    if (CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED || CGame::IsMenuActive()) {
            return;
        }

        // รอโยกระบบ ScoreBoard, MainMenuScreens และ VoiceMenu
        // if (CGui::GetScoreBoard()->GetActiveScore() || CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode) /*|| CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
        //     return;
        // }

        if (CXSing::m_bRenderStatus)
        {
            const CRect screenArea = CGame::GetScreenSafeArea();
            const float screenWidth = screenArea.X2 - screenArea.X1;
            const float screenHeight = screenArea.Y2 - screenArea.Y1;
            if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
                return;
            }

            const float screenScale = std::min(screenWidth * MULT_X, screenHeight * MULT_Y);
            ImVec2 vecButSize_XSing = ImVec2(110.0f * screenScale, 110.0f * screenScale);
            const ImGuiStyle& currentStyle = ImGui::GetStyle();
            ImVec2 windowSize(std::max(currentStyle.WindowMinSize.x, vecButSize_XSing.x + 2.0f * (currentStyle.FramePadding.x + currentStyle.WindowPadding.x)), std::max(currentStyle.WindowMinSize.y, vecButSize_XSing.y + 2.0f * (currentStyle.FramePadding.y + currentStyle.WindowPadding.y)));
            ImVec2 winPos = ImVec2(screenArea.X1 + 90.0f * screenScale, screenArea.Y1 + screenHeight / 3.0f);
            winPos.x = std::clamp(winPos.x, screenArea.X1, std::max(screenArea.X1, screenArea.X2 - windowSize.x));
            winPos.y = std::clamp(winPos.y, screenArea.Y1, std::max(screenArea.Y1, screenArea.Y2 - windowSize.y));

            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0x00, 0x00, 0x00, 0x00).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0x00, 0x00, 0x00, 0x00).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0x00, 0x00, 0x00, 0x00).Value);

            ImGuiStyle style;
            style.FrameBorderSize = ImGui::GetStyle().FrameBorderSize;
            ImGui::GetStyle().FrameBorderSize = 0.0f;

            ImGui::SetNextWindowPos(winPos);
            ImGui::SetNextWindowSize(windowSize);
            ImGui::Begin(OBFUSCATE("XSing"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

            if (p_XSingHoverButtonTexture && p_XSingButtonTexture && p_XSingHoverButtonTexture->raster && p_XSingButtonTexture->raster)
            {
                if (ImGui::ImageButton(CXSing::m_bKeysStatus ? (ImTextureID)p_XSingHoverButtonTexture->raster : (ImTextureID)p_XSingButtonTexture->raster, vecButSize_XSing))
                {
                    if (!CXSing::m_bKeysStatus)
                    {
                        CXSing::m_bKeysStatus = true;

                        if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
                        {
                            CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->SetSelectState(false, 0);
                        }
                    }
                }
                else
                {
                    if (CXSing::m_bKeysStatus)
                    {
                        CXSing::m_bKeysStatus = false;
                    }
                }
            }

            ImGui::End();

            ImGui::PopStyleColor(3);
            ImGui::GetStyle().FrameBorderSize = style.FrameBorderSize;
        }
    }
}

void CXSing::Enable(bool status)
{
    CXSing::m_bRenderStatus = status;
}

bool CXSing::IsKeyXSingActive()
{
    return CXSing::m_bKeysStatus;
}

bool CXSing::IsXSingActive()
{
    return CXSing::m_bRenderStatus;
}