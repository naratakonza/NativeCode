#pragma once

#include <cstdint>

#include "../../../../vendor/imgui/imgui.h"

class CFontRender
{
public:
    ~CFontRender();
    void Render();
    void RenderText(const ImVec2& pos, ImU32 color, bool outline, const char* text);

private:
    ImGuiContext* m_pContext = nullptr;
    std::uint32_t m_dwLastFrameTick = 0;
};
