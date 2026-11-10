#pragma once

#include <cstdint>

#include "../../../../vendor/imgui/imgui.h"

class CFontRender
{
public:
    CFontRender();
    ~CFontRender();
    void Render();
    void RenderText(ImVec2& pos, ImU32 color, bool outline, const char* text, const char* textEnd = nullptr);

private:
    ImGuiContext* m_pContext = nullptr;
    std::uint32_t m_dwLastFrameTick = 0;
    bool m_bBackendReady = false;
};
