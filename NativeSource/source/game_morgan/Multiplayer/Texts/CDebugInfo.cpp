#include "../../../../Main.h"
#include "CDebugInfo.h"
#include "../CMultiPlayer.h"
#include "../Guis/CGui.h"

CDebugInfo::CDebugInfo()
{
    CDebugInfo::m_dwLastTitleUpdateTick = CNecessary::GetTickCount();
}

void CDebugInfo::RenderTitle()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
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