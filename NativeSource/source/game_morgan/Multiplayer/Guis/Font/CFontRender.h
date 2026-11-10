#pragma once

#include <cstdint>

#include "../../../../vendor/imgui/imgui.h"

#define MULT_X 0.00052083333f // 1/1920
#define MULT_Y 0.00092592592f // 1/1080

#define SCALE_X(io, val) ((io).DisplaySize.x * MULT_X * (val))
#define SCALE_Y(io, val) ((io).DisplaySize.y * MULT_Y * (val))

class CFontRender
{
public:
	CFontRender();
	~CFontRender();

	void Render();
	void RenderText(ImVec2& pos, ImU32 col, bool bOutline, const char* text_begin, const char* text_end = nullptr);

private:
	ImFont* LoadFont(const char* font, float fontsize);

	ImGuiContext* m_pContext = nullptr;
	std::uint32_t m_dwLastFrameTick = 0;
	bool m_bBackendReady = false;
};
