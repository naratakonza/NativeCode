#pragma once

#include <cstdint>

#include "../../../Enums/eTouchType.h"
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

    ImFont* GetFont() { return m_pFont; }
    ImFont* GetSampFont() { return m_pSampFont; }
    ImGuiContext* GetContext() { return m_pContext; }
    ImFont* GetWebdingsFont() { return m_pWebdingsFont; }
    ImFont* GetWeaponFont() { return m_pFontGTAWeap; }
    float GetFontSize() { return m_fFontSize; }
    void RenderTextDeathMessage(ImVec2& pos, ImU32 col, bool bOutline, const char* text_begin, const char* text_end = nullptr, float font_size = 0.0f, ImFont* font = nullptr, bool bOutlineUseTextColor = false);
    void Render3DLabel(float fontSize, ImVec2 pos, char* utf8string, uint32_t dwColor);
    ImVec2 CalcTextSizeWithoutTags(char* szStr);
    bool ProcessInlineHexColor(const char* start, const char* end, ImVec4& color);
    bool IsColorDark(ImU32 col);
    void RenderTextMethod(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float font_size, ImFont* font, bool bOutlineUseTextColor);
    void TextWithColors(ImFont* font, float font_size, uint8_t outline, ImVec2 pos, ImColor col, const char* text);
    ImVec2 CalcTextSizeWithoutTags(float font_size, const char* text);
    void TextWithColorsW(ImFont* font, const float font_size, uint8_t outline, ImVec2 pos, ImColor col, const char* szStr);

	void Render();
	bool OnTouchEvent(eTouchType type, ImVec2 pos);
	void RenderText(ImVec2& pos, ImU32 col, bool bOutline, const char* text_begin, const char* text_end = nullptr, float fontSize = 0.0f);

private:
    ImFont* m_pFont = nullptr;
    ImFont* m_pSampFont = nullptr;
    ImFont* m_pFontGTAWeap = nullptr;
    ImFont* m_pWebdingsFont = nullptr;
    float m_fFontSize = 30.0f;
	ImFont* LoadFont(const char* font, float fontsize, const ImWchar* ranges = nullptr);

	void PreProcessInput();
	void PostProcessInput();

	ImVec2 m_vTouchPos{-1.0f, -1.0f};
	bool m_bMouseDown = false;
	bool m_bNeedClearMousePos = false;
	bool m_bNextClear = false;

	ImGuiContext* m_pContext = nullptr;
	std::uint32_t m_dwLastFrameTick = 0;
	bool m_bBackendReady = false;
};
