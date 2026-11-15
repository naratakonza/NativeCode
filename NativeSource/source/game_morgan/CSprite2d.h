#pragma once

#include <cstdint>

#include "CRect.h"
#include "CRGBA.h"

struct RwTexture;

class CSprite2d
{
public:
    RwTexture* m_pTexture;

public:
    static void Constructor(RwTexture *pTexture);
    static void Destructor(RwTexture *pTexture);
    static void InitPerFrame();
    static RwTexture *SetTexture(const char *szTexture);
    static void Draw(RwTexture *pTexture, CRect *rect, uint32_t dwColor, float *uv);
    static void Draw(RwTexture* pTexture, CRect* rect, uint32_t dwColor);
    static void DrawBarChart(float x, float y, uint16_t w, uint8_t h, float value, uint8_t inrease, uint8_t legend, uint8_t border, CRGBA c, uint8_t* c2);
};
static_assert(sizeof(CSprite2d) == 0x8);