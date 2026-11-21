#include "../../../../Main.h"
#include "CDebugInfo.h"
#include "../Audio/CAudio.h"
#include "../CMultiPlayer.h"
#include "../Guis/CGui.h"
#include "../../CGame.h"
#include "../../CFont.h"
#include "../../CStreaming.h"
#include "../../Enums/eFontStyle.h"
#include "../../Enums/eFontAlignment.h"
#include "../../../../Launcher/MGFlutter.h"

#include <mach-o/dyld.h>

CDebugInfo::CDebugInfo()
{
    CDebugInfo::m_dwLastFramerateUpdateTick = CNecessary::GetTickCount();
    CDebugInfo::m_dwLastTitleUpdateTick = CNecessary::GetTickCount();
    CDebugInfo::m_dwLastMemoryUpdateTick = CNecessary::GetTickCount();

    CDebugInfo::SetRenderFramerate(false);
    CDebugInfo::SetRenderMemory(false);
}

CDebugInfo::~CDebugInfo()
{
    CDebugInfo::SetRenderFramerate(false);
    CDebugInfo::SetRenderMemory(false);
}

void CDebugInfo::RenderInfo()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
        return;
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
        {
            return;
        }
    }

    // รอโยกระบบ MainMenuScreens และ VoiceMenu.
    if ((CGui::GetScoreBoard() && CGui::GetScoreBoard()->GetActiveScore()) /*|| CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode) || CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
        return;
    }

    // if (CGui::GetEditObject()->IsEditModeActive())
    // {
    //     return;
    // }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x159CEC), "\x48\x2F\x00\x90\x08\xF1\x28\x91\x00\x01\x40\xBD", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x159D5C), "\x54\x1C\x00\xB0\x80\x7E\x0B\xBD", 8, "__text");
    if (!verified) {
        return;
    }

    const CRect screenArea = CGame::GetScreenSafeArea();
    const float screenWidth = screenArea.X2 - screenArea.X1;
    const float screenHeight = screenArea.Y2 - screenArea.Y1;
    if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
        return;
    }

    uint32_t color = 0xFFFFFFFF;
    CFont::SetBackground(0, 0);
    CFont::SetScale(0.6f, 1.14f);
    CFont::SetOrientation(eFontAlignment::ALIGN_LEFT);
    CFont::SetJustify(0);
    CFont::SetWrapx(screenArea.X2);
    CFont::SetProportional(0);
    CFont::SetFontStyle(eFontStyle::FONT_SUBTITLES);
    CFont::SetEdge(0);
    CFont::SetColor(&color);

    const float posX = screenArea.X1 + (screenWidth / 640.0f) * 4.0f;
    const float fontHeight = std::max(0.0f, CFont::GetHeight(false));
    const float lineSpacing = std::max(fontHeight, (screenHeight / 448.0f) * 10.0f);
    const float gap = (screenHeight / 448.0f) * 3.0f;
    float infoY = screenArea.Y1 + (screenHeight / 448.0f) * 220.0f;
    float keybarBottom = 0.0f;
    if (MGGetFlutterKeysBarBottom(keybarBottom))
    {
        infoY = keybarBottom + gap;
    }

    const float maxY = std::max(screenArea.Y1, screenArea.Y2 - fontHeight - lineSpacing);
    infoY = std::clamp(infoY, screenArea.Y1, maxY);

    if (CDebugInfo::m_uiDrawFPS)
    {
        static char bufferDebugFps[16 + 1]{};
        if ((CNecessary::GetTickCount() - CDebugInfo::m_dwLastFramerateUpdateTick) > 1000)
        {
            float* pFramerate = reinterpret_cast<float*>(SA_ADDR(0x4E2B7C) + _dyld_get_image_vmaddr_slide(0));
            std::snprintf(bufferDebugFps, sizeof(bufferDebugFps), OBFUSCATE("FPS: %.0f"), *pFramerate);

            if (*pFramerate > 120 && *pFramerate <= 130)
            {
                std::snprintf(bufferDebugFps, sizeof(bufferDebugFps), OBFUSCATE("FPS: %.0f"), 125.0);
            }
            else if (*pFramerate > 90 && *pFramerate <= 93)
            {
                std::snprintf(bufferDebugFps, sizeof(bufferDebugFps), OBFUSCATE("FPS: %.0f"), 90.91);
            }
            else if (*pFramerate > 60.25f && *pFramerate <= 63)
            {
                std::snprintf(bufferDebugFps, sizeof(bufferDebugFps), OBFUSCATE("FPS: %.0f"), 62.5);
            }

            CDebugInfo::m_dwLastFramerateUpdateTick = CNecessary::GetTickCount();
        }

        CFont::PrintString(posX, infoY, bufferDebugFps);
    }

    if (CDebugInfo::m_uiDrawMem)
    {
        static char bufferDebugMem[20 + 1]{};
        if ((CNecessary::GetTickCount() - CDebugInfo::m_dwLastMemoryUpdateTick) > 1000)
        {
            int pMemoryUse = static_cast<int>(CStreaming::ms_memoryUsed);
            int ms_memoryAvailable = static_cast<int>(CStreaming::ms_memoryAvailable);
            std::snprintf(bufferDebugMem, sizeof(bufferDebugMem), OBFUSCATE("MEM: %d/%d"), pMemoryUse >> 20, ms_memoryAvailable >> 20);
            CDebugInfo::m_dwLastMemoryUpdateTick = CNecessary::GetTickCount();
        }

        CFont::PrintString(posX, std::min(infoY + lineSpacing, std::max(screenArea.Y1, screenArea.Y2 - fontHeight)), bufferDebugMem);
    }
}

void CDebugInfo::RenderTitle()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    if (CGui::GetScoreBoard()->GetActiveScore() /*|| (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode) || (CGui::GetMainMenuScreens()->IsRadarSection() || CGui::GetMainMenuScreens()->IsAdjustableHUD()) || CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
        return;
    }

    const auto now = CNecessary::GetTickCount();
    if (now - CDebugInfo::m_dwLastTitleUpdateTick > 1000)
    {
        std::snprintf(CDebugInfo::m_szTitle, sizeof(CDebugInfo::m_szTitle), "%s", static_cast<const char*>(OBFUSCATE("NOXARTH STUDIO - GTA SA IOS 2.2.21 (Build 4.0)")));
        CDebugInfo::m_dwLastTitleUpdateTick = now;
    }

    ImGuiIO& io = ImGui::GetIO();
    const float screenScale = std::min(SCALE_X(io, 1.0f), SCALE_Y(io, 1.0f));
    const float fontSize = 22.0f * screenScale;
    const ImVec2 textSize = ImGui::CalcTextSize(fontSize, CDebugInfo::m_szTitle);
    ImVec2 pos((io.DisplaySize.x - textSize.x) * 0.5f, io.DisplaySize.y - textSize.y - 12.0f * screenScale);
    CGui::GetFontRender()->RenderText(pos, IM_COL32(212, 212, 212, 255), true, CDebugInfo::m_szTitle, nullptr, fontSize);
}

void CDebugInfo::RenderAudioStreamTag()
{
    auto* stream = CAudio::GetAudioStream();
    if (!CMultiPlayer::GetActiveNetwork() || !stream || !stream->GetStreamTags()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const float screenScale = std::min(SCALE_X(io, 1.0f), SCALE_Y(io, 1.0f));
    const float fontSize = 22.0f * screenScale;
    const ImVec2 textSize = ImGui::CalcTextSize(fontSize, stream->m_streamLabel);
    ImVec2 pos(10.0f * screenScale, io.DisplaySize.y - textSize.y * 2.0f - 12.0f * screenScale);
    CGui::GetFontRender()->RenderText(pos, IM_COL32(212, 212, 212, 255), true, stream->m_streamLabel, nullptr, fontSize);
}

void CDebugInfo::SetRenderFramerate(uint32_t bDraw)
{
    CDebugInfo::m_uiDrawFPS = bDraw;
}

void CDebugInfo::SetRenderMemory(uint32_t bDraw)
{
    CDebugInfo::m_uiDrawMem = bDraw;
}