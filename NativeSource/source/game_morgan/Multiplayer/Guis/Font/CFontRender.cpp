#include "../../../../../Main.h"
#include "CFontRender.h"
#include "../CGui.h"
#include "../../../RadarMod/CTiltedRadar.h"
#include "../../../../../Launcher/MGFlutter.h"
#include "../../../../utils/Storage/CStorage.h"
#include "../../../../vendor/imgui/imgui_impl_renderware.h"

#include <CoreGraphics/CoreGraphics.h>

static constexpr float atlasFontSize = 30.0f;

CFontRender::CFontRender()
{
	CNecessary::Log(OBFUSCATE("Initializing CFontRender.."));

	ImGuiContext* previous = ImGui::GetCurrentContext();
	IMGUI_CHECKVERSION();
	CFontRender::m_pContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(CFontRender::m_pContext);
	ImGuiIO &io = ImGui::GetIO();

	CFontRender::m_bBackendReady = ImGui_ImplRenderWare_Init();
	if (!CFontRender::m_bBackendReady)
	{
		CNecessary::Log(OBFUSCATE("CFontRender | RenderWare bindings unavailable"));
	}

	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	// setup style
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowBorderSize = 0.0f;
	ImGui::StyleColorsDark();

	// load font
	const auto fontName = OBFUSCATE("arial.ttf");
	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF,
		0x0100, 0x017F,
		0x0400, 0x04FF,
		0x0E00, 0x0E5B,
		0x2DE0, 0x2DFF,
		0xA640, 0xA69F,
		0xF020, 0xF0FF,
		0x2010, 0x205E,
		0
	};

	io.Fonts->TexDesiredWidth = 2048;
	CNecessary::Log(OBFUSCATE("CFontRender | Loading font: %s"), static_cast<const char*>(fontName));
	io.FontDefault = CFontRender::LoadFont(fontName, atlasFontSize, ranges);
	CNecessary::Log(OBFUSCATE("CFontRender | ImFont pointer = %p"), static_cast<void*>(io.FontDefault));
	if (!io.FontDefault)
	{
		CNecessary::Log(OBFUSCATE("CFontRender | Could not load SAMP/fonts/arial.ttf"));
		CFontRender::m_bBackendReady = false;
	}

	style.WindowRounding = 0.0f;
	ImGui::SetCurrentContext(previous);
}

CFontRender::~CFontRender()
{
	CNecessary::Log(OBFUSCATE("CFontRender Uninitialise"));

	ImGuiContext* previous = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(CFontRender::m_pContext);
	ImGui_ImplRenderWare_ShutDown();
	ImGui::DestroyContext(CFontRender::m_pContext);

	if (previous != CFontRender::m_pContext)
	{
		ImGui::SetCurrentContext(previous);
	}
}

void CFontRender::Render()
{
	if (!CFontRender::m_bBackendReady) {
		return;
	}

	ImGuiContext* previous = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(CFontRender::m_pContext);
	if (!ImGui_ImplRenderWare_NewFrame())
	{
		ImGui::SetCurrentContext(previous);
		return;
	}

	ImGuiIO& io = ImGui::GetIO();

	// Temporary Flutter KeyBar preview, aligned beneath the iOS radar widget.
	float radarLeft = 0.0f;
	float radarBottom = 0.0f;
	const bool showKeyBarPreview = !CGame::IsRadarActive() && CTiltedRadar::GetWidgetBottomLeft(radarLeft, radarBottom);
	MGSetFlutterKeyBarPreview(showKeyBarPreview, radarLeft / io.DisplaySize.x, radarBottom / io.DisplaySize.y);

	auto* settings = CSetting::GetConfig();
	const float size = settings ? settings->GetFloatValue(OBFUSCATE("FontSize")) : atlasFontSize;
	const float fontSize = std::isfinite(size) ? std::clamp(size, 1.0f, 80.0f) : atlasFontSize;
	io.FontGlobalScale = SCALE_Y(io, fontSize) / atlasFontSize;

	if (!CFontRender::m_dwLastFrameTick)
	{
		CNecessary::Log(OBFUSCATE("CFontRender | Scale factor: %f, %f Font size: %f"), io.DisplaySize.x * MULT_X, io.DisplaySize.y * MULT_Y, SCALE_Y(io, fontSize));
	}

	const auto now = CNecessary::GetTickCount();
	io.DeltaTime = CFontRender::m_dwLastFrameTick ? std::max(0.001f, (now - CFontRender::m_dwLastFrameTick) / 1000.0f) : 1.0f / 60.0f;
	CFontRender::m_dwLastFrameTick = now;

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

ImFont* CFontRender::LoadFont(const char* font, float fontsize, const ImWchar* ranges)
{
	ImGuiIO &io = ImGui::GetIO();

	ImFontConfig fontCfg;
	fontCfg.SizePixels = fontsize;
	fontCfg.OversampleH = 1;
	fontCfg.OversampleV = 1;

	char path[0xFF];
	std::snprintf(path, sizeof(path), OBFUSCATE("SAMP/fonts/%s"), font);
	auto result = morgan::fileio::CStorage::OpenActiveResource(nullptr, morgan::fileio::ResolveGameAssetPath(path));
	if (!result.file)
	{
		CNecessary::Log(OBFUSCATE("CFontRender | Font open failed: %s (error %d)"), path, result.error.value());
		return nullptr;
	}
	if (std::fseek(result.file.get(), 0, SEEK_END) != 0) {
		return nullptr;
	}

	const long size = std::ftell(result.file.get());
	if (size < 12 || size > 4 * 1024 * 1024 || std::fseek(result.file.get(), 0, SEEK_SET) != 0) {
		return nullptr;
	}

	auto* data = static_cast<unsigned char*>(ImGui::MemAlloc(static_cast<std::size_t>(size)));
	if (data)
	{
		const auto length = static_cast<std::size_t>(size);
		if (std::fread(data, 1, length, result.file.get()) == length && (std::memcmp(data, "\x00\x01\x00\x00", 4) == 0 || std::memcmp(data, "true", 4) == 0))
		{
			CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, data, length, nullptr);
			CGFontRef validated = provider ? CGFontCreateWithDataProvider(provider) : nullptr;
			if (validated) CGFontRelease(validated);
			if (provider) CGDataProviderRelease(provider);

			if (validated)
            {
				return io.Fonts->AddFontFromMemoryTTF(data, static_cast<int>(size), fontsize, &fontCfg, ranges);
			}
		}

		ImGui::MemFree(data);
	}

	return nullptr;
}

void CFontRender::RenderText(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float fontSize)
{
	auto* settings = CSetting::GetConfig();
	float offset = settings ? std::clamp(settings->GetIntValue(OBFUSCATE("FontOutline")), 0, 8) : 2;
	if (fontSize > 0.0f)
	{
		offset = std::min(offset, fontSize / 12.0f);
	}

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	if (bOutline && offset > 0.0f)
	{
		// left
		drawList->AddText(nullptr, fontSize, ImVec2(posCur.x - offset, posCur.y), IM_COL32_BLACK, text_begin, text_end);
		// right
		drawList->AddText(nullptr, fontSize, ImVec2(posCur.x + offset, posCur.y), IM_COL32_BLACK, text_begin, text_end);
		// above
		drawList->AddText(nullptr, fontSize, ImVec2(posCur.x, posCur.y - offset), IM_COL32_BLACK, text_begin, text_end);
		// below
		drawList->AddText(nullptr, fontSize, ImVec2(posCur.x, posCur.y + offset), IM_COL32_BLACK, text_begin, text_end);
	}

	drawList->AddText(nullptr, fontSize, posCur, col, text_begin, text_end);
}