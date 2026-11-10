#include "../../../../../Main.h"
#include "CFontRender.h"
#include "../CGui.h"
#include "../../../../vendor/imgui/imgui_impl_renderware.h"

static constexpr float atlasFontSize = 30.0f;

CFontRender::~CFontRender()
{
    if (m_pContext)
    {
        auto* previous = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(m_pContext);
        ImGui_ImplRenderWare_ShutDown();
        ImGui::DestroyContext(m_pContext);
        if (previous != m_pContext) {
            ImGui::SetCurrentContext(previous);
        }
    }
}

void CFontRender::Render()
{
    auto* previous = ImGui::GetCurrentContext();
    if (!m_pContext)
    {
        IMGUI_CHECKVERSION();
        m_pContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_pContext);
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        ImFontConfig config;
        config.SizePixels = atlasFontSize;
        io.Fonts->AddFontDefault(&config);
    }

    ImGui::SetCurrentContext(m_pContext);
    if (ImGui_ImplRenderWare_NewFrame())
    {
        ImGuiIO& io = ImGui::GetIO();
        const auto now = CNecessary::GetTickCount();
        io.DeltaTime = m_dwLastFrameTick ? std::max(0.001f, (now - m_dwLastFrameTick) / 1000.0f) : 1.0f / 60.0f;
        m_dwLastFrameTick = now;

        auto* settings = CSetting::GetConfig();
        const float size = settings ? settings->GetFloatValue(OBFUSCATE("FontSize")) : atlasFontSize;
        io.FontGlobalScale = (std::isfinite(size) ? std::clamp(size, 1.0f, 80.0f) : atlasFontSize) * io.DisplaySize.y / (1080.0f * atlasFontSize);

        ImGui::NewFrame();
        CGui::GetDebug()->RenderTitle();
        ImGui::Render();
        ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
    }
    ImGui::SetCurrentContext(previous);
}

void CFontRender::RenderText(const ImVec2& pos, ImU32 color, bool outline, const char* text)
{
    auto* drawList = ImGui::GetBackgroundDrawList();
    auto* settings = CSetting::GetConfig();
    const float offset = settings ? static_cast<float>(std::clamp(settings->GetIntValue(OBFUSCATE("FontOutline")), 0, 8)) : 2.0f;
    if (outline && offset > 0.0f)
    {
        drawList->AddText(ImVec2(pos.x - offset, pos.y), IM_COL32_BLACK, text);
        drawList->AddText(ImVec2(pos.x + offset, pos.y), IM_COL32_BLACK, text);
        drawList->AddText(ImVec2(pos.x, pos.y - offset), IM_COL32_BLACK, text);
        drawList->AddText(ImVec2(pos.x, pos.y + offset), IM_COL32_BLACK, text);
    }
    drawList->AddText(pos, color, text);
}
