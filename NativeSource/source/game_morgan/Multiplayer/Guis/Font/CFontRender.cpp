#include "../../../../../Main.h"
#include "CFontRender.h"
#include "../CGui.h"
#include "../../../../utils/Storage/CStorage.h"
#include "../../../../vendor/imgui/imgui_impl_renderware.h"

#include <CommonCrypto/CommonDigest.h>

static constexpr float atlasFontSize = 30.0f;

static bool LoadTitleFont(ImFontAtlas& atlas, const ImFontConfig& config)
{
    using namespace morgan::fileio;
    // Match the Arial Bold font shipped in the GTA asset pack before passing it to stb_truetype.
    constexpr std::size_t size = 352224;
    constexpr unsigned char expected[CC_SHA256_DIGEST_LENGTH] = {
        0xDF, 0x70, 0x59, 0x7F, 0x0B, 0xDF, 0x49, 0xDA, 0x3A, 0xF2, 0x70, 0x13, 0x8F, 0x8A, 0x34, 0x39,
        0x6E, 0x4F, 0x56, 0x18, 0xC6, 0x71, 0xA1, 0xDB, 0x34, 0x80, 0xE6, 0x26, 0xF3, 0x8A, 0xAE, 0xCE,
    };
    auto result = CStorage::OpenActiveResource(nullptr, ResolveGameAssetPath("SAMP/fonts/arial_bold.ttf"));
    if (!result.file) {
        return false;
    }
    auto* font = ImGui::MemAlloc(size);
    if (!font) {
        return false;
    }

    unsigned char actual[CC_SHA256_DIGEST_LENGTH];
    if (std::fread(font, 1, size, result.file.get()) == size && std::fgetc(result.file.get()) == EOF &&
        CC_SHA256(font, static_cast<CC_LONG>(size), actual) && std::memcmp(actual, expected, sizeof(expected)) == 0)
    {
        atlas.AddFontFromMemoryTTF(font, static_cast<int>(size), atlasFontSize, &config);
        return true;
    }

    ImGui::MemFree(font);
    return false;
}

CFontRender::CFontRender()
{
    auto* previous = ImGui::GetCurrentContext();
    IMGUI_CHECKVERSION();
    m_pContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_pContext);
    m_bBackendReady = ImGui_ImplRenderWare_Init();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    ImFontConfig config;
    config.SizePixels = atlasFontSize;
    config.OversampleH = 1;
    config.OversampleV = 1;
    if (!LoadTitleFont(*io.Fonts, config)) {
        io.Fonts->AddFontDefault(&config);
    }
    ImGui::SetCurrentContext(previous);
}

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
    if (!m_bBackendReady) {
        return;
    }

    auto* previous = ImGui::GetCurrentContext();
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
        if (CGui::GetDebug()) {
            CGui::GetDebug()->RenderTitle();
        }
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
    }
    ImGui::SetCurrentContext(previous);
}

void CFontRender::RenderText(ImVec2& pos, ImU32 color, bool outline, const char* text, const char* textEnd)
{
    auto* drawList = ImGui::GetBackgroundDrawList();
    auto* settings = CSetting::GetConfig();
    const float offset = settings ? static_cast<float>(std::clamp(settings->GetIntValue(OBFUSCATE("FontOutline")), 0, 8)) : 2.0f;
    if (outline && offset > 0.0f)
    {
        drawList->AddText(ImVec2(pos.x - offset, pos.y), IM_COL32_BLACK, text, textEnd);
        drawList->AddText(ImVec2(pos.x + offset, pos.y), IM_COL32_BLACK, text, textEnd);
        drawList->AddText(ImVec2(pos.x, pos.y - offset), IM_COL32_BLACK, text, textEnd);
        drawList->AddText(ImVec2(pos.x, pos.y + offset), IM_COL32_BLACK, text, textEnd);
    }
    drawList->AddText(pos, color, text, textEnd);
}
