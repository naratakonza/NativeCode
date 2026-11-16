#pragma once

#include "CRect.h"
#include "CRGBA.h"

class CFont
{
public:
    static void SetWrapx(float fVal);
    static void SetCentreSize(float fVal);
    static void SetDropColor(CRGBA* dwColor);
    static void SetDropColor(uint32_t* dwColor);
    static void SetDropShadowPosition(uint8_t iPos);
    static void SetEdge(uint8_t iPos);
    static void SetBackgroundColor(uint32_t* dwColor);
    static void SetJustify(uint8_t iPos);
    static void SetScale(float fScale);
    static void SetScale(float fScaleX, float fScaleY);
    static void SetBackground(uint8_t unk0, uint8_t unk1);
    static void SetProportional(uint8_t iProp);
    static void SetOrientation(uint8_t iOrient);
    static void SetColor(uint32_t* dwColor);
    static void SetFontStyle(uint8_t iStyle);
    static void AsciiToGxtChar(const char* ascii, uint16_t* gxt);
    static void RenderFontBuffer();
    static void PrintString(float posX, float posY, const char* string);
    static float GetHeight(bool unk);
    static float GetStringWidth(uint16_t* str);
    static void GetTextRect(CRect* rect, float x, float y);

private:
    static uint16_t ms_text[0x800];
};