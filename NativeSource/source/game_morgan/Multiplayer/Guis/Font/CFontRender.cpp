#include "../../../../../Main.h"
#include "CFontRender.h"
#include "../CGui.h"
#include "../../../../utils/Storage/CStorage.h"
#include "../../../../vendor/imgui/imgui_impl_renderware.h"

#include <CommonCrypto/CommonDigest.h>

static constexpr float atlasFontSize = 30.0f;

CFontRender::CFontRender()
{
	CNecessary::Log(OBFUSCATE("Initializing CFontRender.."));

	// setup ImGUI
	ImGuiContext* previous = ImGui::GetCurrentContext();
	IMGUI_CHECKVERSION();
	m_pContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(m_pContext);
	ImGuiIO &io = ImGui::GetIO();

	m_bBackendReady = ImGui_ImplRenderWare_Init();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	// setup style
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowBorderSize = 0.0f;
	ImGui::StyleColorsDark();

	// load font
	const char* fontName = OBFUSCATE("arial_bold.ttf");
	CNecessary::Log(OBFUSCATE("CFontRender | Loading font: %s"), fontName);
	io.FontDefault = CFontRender::LoadFont(fontName, atlasFontSize);

	style.WindowRounding = 0.0f;
	ImGui::SetCurrentContext(previous);
}

CFontRender::~CFontRender()
{
	CNecessary::Log(OBFUSCATE("CFontRender Uninitialise"));

	ImGuiContext* previous = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(m_pContext);
	ImGui_ImplRenderWare_ShutDown();
	ImGui::DestroyContext(m_pContext);

	if (previous != m_pContext) {
		ImGui::SetCurrentContext(previous);
	}
}

void CFontRender::Render()
{
	if (!m_bBackendReady) {
		return;
	}

	ImGuiContext* previous = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(m_pContext);
	if (!ImGui_ImplRenderWare_NewFrame())
	{
		ImGui::SetCurrentContext(previous);
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	const auto now = CNecessary::GetTickCount();
	io.DeltaTime = m_dwLastFrameTick ? std::max(0.001f, (now - m_dwLastFrameTick) / 1000.0f) : 1.0f / 60.0f;
	m_dwLastFrameTick = now;

	auto* settings = CSetting::GetConfig();
	const float size = settings ? settings->GetFloatValue(OBFUSCATE("FontSize")) : atlasFontSize;
	const float fontSize = std::isfinite(size) ? std::clamp(size, 1.0f, 80.0f) : atlasFontSize;
	io.FontGlobalScale = SCALE_Y(io, fontSize) / atlasFontSize;

	ImGui::NewFrame();

	if (CGui::GetDebug())
	{
		CGui::GetDebug()->RenderTitle();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());

	ImGui::SetCurrentContext(previous);
}

ImFont* CFontRender::LoadFont(const char* font, float fontsize)
{
	using namespace morgan::fileio;
	ImGuiIO &io = ImGui::GetIO();

	ImFontConfig fontCfg;
	fontCfg.SizePixels = fontsize;
	fontCfg.OversampleH = 1;
	fontCfg.OversampleV = 1;

	// Use the verified Arial Bold asset from the selected SAMP pack.
	constexpr std::size_t size = 352224;
	constexpr unsigned char expected[CC_SHA256_DIGEST_LENGTH] = {
		0xDF, 0x70, 0x59, 0x7F, 0x0B, 0xDF, 0x49, 0xDA, 0x3A, 0xF2, 0x70, 0x13, 0x8F, 0x8A, 0x34, 0x39,
		0x6E, 0x4F, 0x56, 0x18, 0xC6, 0x71, 0xA1, 0xDB, 0x34, 0x80, 0xE6, 0x26, 0xF3, 0x8A, 0xAE, 0xCE,
	};
	char path[0xFF];
	std::snprintf(path, sizeof(path), "SAMP/fonts/%s", font);
	auto result = CStorage::OpenActiveResource(nullptr, ResolveGameAssetPath(path));
	void* data = result.file ? ImGui::MemAlloc(size) : nullptr;
	if (data)
	{
		unsigned char actual[CC_SHA256_DIGEST_LENGTH];
		if (std::fread(data, 1, size, result.file.get()) == size && std::fgetc(result.file.get()) == EOF &&
		    CC_SHA256(data, static_cast<CC_LONG>(size), actual) && std::memcmp(actual, expected, sizeof(expected)) == 0)
		{
			return io.Fonts->AddFontFromMemoryTTF(data, static_cast<int>(size), fontsize, &fontCfg);
		}
		ImGui::MemFree(data);
	}

	return io.Fonts->AddFontDefault(&fontCfg);
}

void CFontRender::RenderText(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end)
{
	auto* settings = CSetting::GetConfig();
	int iOffset = settings ? std::clamp(settings->GetIntValue(OBFUSCATE("FontOutline")), 0, 8) : 2;

	if (bOutline && iOffset > 0)
	{
		// left
		posCur.x -= iOffset;
		ImGui::GetBackgroundDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.x += iOffset;
		// right
		posCur.x += iOffset;
		ImGui::GetBackgroundDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.x -= iOffset;
		// above
		posCur.y -= iOffset;
		ImGui::GetBackgroundDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.y += iOffset;
		// below
		posCur.y += iOffset;
		ImGui::GetBackgroundDrawList()->AddText(posCur, ImColor(IM_COL32_BLACK), text_begin, text_end);
		posCur.y -= iOffset;
	}

	ImGui::GetBackgroundDrawList()->AddText(posCur, col, text_begin, text_end);
}
