#include "../../../../Main.h"
#include "CDebugInfo.h"
#include "../CMultiPlayer.h"
#include "../Guis/CGui.h"

CDebugInfo::CDebugInfo()
    : m_dwLastTitleUpdateTick(CNecessary::GetTickCount())
{
}

void CDebugInfo::RenderTitle()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    const auto now = CNecessary::GetTickCount();
    if (now - m_dwLastTitleUpdateTick > 1000)
    {
        std::snprintf(m_szTitle, sizeof(m_szTitle), "%s", static_cast<const char*>(OBFUSCATE("THUNDER - SAMP (GTASA 2.11.311 x64) v1.0.8.4")));
        m_dwLastTitleUpdateTick = now;
    }

    ImGuiIO& io = ImGui::GetIO();
    const float textWidth = ImGui::CalcTextSize(m_szTitle).x;
    ImVec2 pos(io.DisplaySize.x - textWidth - io.DisplaySize.x * (10.0f / 1920.0f), io.DisplaySize.y * (1047.0f / 1080.0f));
    CGui::GetFontRender()->RenderText(pos, 0xFFC8C8C8, true, m_szTitle);
}
