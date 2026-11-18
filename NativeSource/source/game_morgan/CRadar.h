#pragma once

#include "CRect.h"
#include "CSprite2d.h"
#include "Core/CVector2D.h"

#define NUM_RADAR_COLORS 1000

class CRadar
{
public:
    static int m_iAlpha;

public:
    static void Initialise();
    static void DrawRadarGangOverlay(bool inMenu);
    static uint32_t GetRadarTraceColor(uintptr_t thiz, uint32_t color);
    static void SetRadarColor(int nIndex, uint32_t dwColor);
    static void ResetRadarColor();
    static uint32_t TranslateColorCodeToRGBA(int iCode);
    static void SetAlpha(int Alpha);
	static float GetAlpha();
    static void DrawAreaOnRadar(CRect const& rect, uint32_t dwCurColor, bool inMenu);
    static void ClearBlip(int nIndex);
    static void TransformRealWorldPointToRadarSpace(CVector2D& out, CVector2D const& in);
    static void TransformRadarPointToScreenSpace(CVector2D& out, CVector2D const& in);
    static void LimitRadarPoint(CVector2D& in);
};